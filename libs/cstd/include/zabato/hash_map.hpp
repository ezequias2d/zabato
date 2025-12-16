#pragma once

#include <zabato/allocator.hpp>
#include <zabato/utils.hpp>

#include <assert.h>

namespace zabato
{
/**
 * @class hash_map
 * @brief A dictionary container implemented using open-addressing with linear
 * probing.
 * @tparam Key The type of keys.
 * @tparam Value The type of values.
 * @tparam Hasher A function object to compute the hash of a key.
 * @tparam KeyEqual A function object to compare two keys for equality.
 * @tparam Allocator The allocator to use for memory management.
 */
template <typename Key,
          typename Value,
          typename Hasher    = hash<Key>,
          typename KeyEqual  = equal_to<Key>,
          typename Allocator = allocator<Value>>
class hash_map
{
public:
    enum class entry_state : uint8_t
    {
        empty,
        tombstone,
        occupied
    };

    struct entry
    {
        Key key;
        Value value;
        entry_state state;
    };

private:
    using entry_allocator_type =
        typename Allocator::template rebind<entry>::other;

public:
    class iterator
    {
    public:
        iterator(entry *ptr, entry *end) : m_ptr(ptr), m_end(end)
        {
            // Advance to the first valid element
            advance_to_occupied();
        }

        entry &operator*() const { return *m_ptr; }
        entry *operator->() const { return m_ptr; }

        /** @brief Pre-increment operator */
        iterator &operator++()
        {
            if (m_ptr != m_end)
            {
                m_ptr++;
                advance_to_occupied();
            }
            return *this;
        }

        /** @brief Post-increment operator */
        iterator operator++(int)
        {
            iterator tmp = *this;
            ++(*this); // Call pre-increment
            return tmp;
        }

        bool operator==(const iterator &other) const
        {
            return m_ptr == other.m_ptr;
        }

        bool operator!=(const iterator &other) const
        {
            return m_ptr != other.m_ptr;
        }

    private:
        void advance_to_occupied()
        {
            while (m_ptr != m_end && m_ptr->state != entry_state::occupied)
            {
                m_ptr++;
            }
        }

        entry *m_ptr;
        entry *m_end;
    };

    class const_iterator
    {
    public:
        const_iterator(const entry *ptr, const entry *end)
            : m_ptr(ptr), m_end(end)
        {
            // Advance to the first valid element
            advance_to_occupied();
        }

        const entry &operator*() const { return *m_ptr; }
        const entry *operator->() const { return m_ptr; }

        /** @brief Pre-increment operator */
        const_iterator &operator++()
        {
            if (m_ptr != m_end)
            {
                m_ptr++;
                advance_to_occupied();
            }
            return *this;
        }

        /** @brief Post-increment operator */
        const_iterator operator++(int)
        {
            const_iterator tmp = *this;
            ++(*this); // Call pre-increment
            return tmp;
        }

        bool operator==(const const_iterator &other) const
        {
            return m_ptr == other.m_ptr;
        }
        bool operator!=(const const_iterator &other) const
        {
            return m_ptr != other.m_ptr;
        }

    private:
        void advance_to_occupied()
        {
            while (m_ptr != m_end && m_ptr->state != entry_state::occupied)
            {
                m_ptr++;
            }
        }

        const entry *m_ptr;
        const entry *m_end;
    };

    /** @brief Constructs an empty hash_map. */
    hash_map() noexcept
        : m_entries(nullptr), m_size(0), m_capacity(0), m_allocator(),
          m_hasher(), m_key_equal()
    {
    }

    /** @brief Constructs an empty hash_map with an initial capacity. */
    hash_map(size_t capacity) : hash_map() { resize(capacity); }

    /** @brief Destroys the hash_map and its elements. */
    ~hash_map() { clear_and_free(); }

    iterator begin() { return iterator(m_entries, m_entries + m_capacity); }

    iterator end()
    {
        return iterator(m_entries + m_capacity, m_entries + m_capacity);
    }

    const_iterator begin() const
    {
        return const_iterator(m_entries, m_entries + m_capacity);
    }

    const_iterator end() const
    {
        return const_iterator(m_entries + m_capacity, m_entries + m_capacity);
    }

    const_iterator cbegin() const
    {
        return const_iterator(m_entries, m_entries + m_capacity);
    }
    const_iterator cend() const
    {
        return const_iterator(m_entries + m_capacity, m_entries + m_capacity);
    }

    /** @brief Removes all elements from the hash map. */
    void clear()
    {
        if (!is_pod<Key>::value || !is_pod<Value>::value)
        {
            for (size_t i = 0; i < m_capacity; ++i)
            {
                if (m_entries[i].state == entry_state::occupied)
                {
                    m_entries[i].key.~Key();
                    m_entries[i].value.~Value();
                }
            }
        }
        for (size_t i = 0; i < m_capacity; ++i)
        {
            m_entries[i].state = entry_state::empty;
        }
        m_size = 0;
    }

    /** @return The number of elements in the hash map. */
    size_t size() const { return m_size; }

    /**
     * @brief Adds a key-value pair to the map only if the key does not already
     * exist.
     * @param key The key of the element to add.
     * @param value The value to associate with the key.
     * @return True if the element was inserted, false if the key already
     * existed.
     */
    bool add(const Key &key, const Value &value)
    {
        if (m_capacity == 0 || m_size + 1 > m_capacity * 3 / 4)
            resize(m_capacity == 0 ? 8 : (m_capacity * 3) / 2);

        entry *target_entry = find_entry_internal(key);

        if (target_entry->state != entry_state::occupied)
        {
            new (&target_entry->key) Key(key);
            try
            {
                new (&target_entry->value) Value(value);
            }
            catch (...)
            {
                target_entry->key.~Key();
                throw;
            }
            target_entry->state = entry_state::occupied;
            m_size++;
            return true;
        }
        return false; // Key already exists
    }

    /**
     * @brief Adds a key-value pair to the map or updates the value if the key
     * already exists.
     * @param key The key of the element to add or update.
     * @param value The value to associate with the key.
     */
    void add_or_set(const Key &key, const Value &value)
    {
        if (m_capacity == 0 || m_size + 1 > m_capacity * 3 / 4)
            resize(m_capacity == 0 ? 8 : (m_capacity * 3) / 2);

        entry *target_entry = find_entry_internal(key);

        if (target_entry->state != entry_state::occupied)
        {
            new (&target_entry->key) Key(key);
            try
            {
                new (&target_entry->value) Value(value);
            }
            catch (...)
            {
                target_entry->key.~Key();
                throw;
            }
            target_entry->state = entry_state::occupied;
            m_size++;
        }
        else
            target_entry->value =
                value; // Key already exists, just update the value
    }

    /**
     * @brief Updates the value for an existing key.
     * @param key The key of the element to update.
     * @param value The new value to associate with the key.
     * @return True if the key was found and the value was updated, false
     * otherwise.
     */
    bool set(const Key &key, const Value &value)
    {
        if (m_size == 0)
            return false;
        entry *target_entry = find_entry_internal(key);
        if (target_entry && target_entry->state == entry_state::occupied)
        {
            target_entry->value = value;
            return true;
        }
        return false;
    }

    /**
     * @brief Tries to get the value associated with a specific key.
     * @param key The key to search for.
     * @param[out] out_value A reference where the found value will be stored.
     * @return True if the key was found and the value was retrieved, false
     * otherwise.
     */
    bool try_get_value(const Key &key, Value &out_value) const
    {
        if (m_size == 0)
            return false;
        const entry *target = find_entry_internal(key);
        if (target && target->state == entry_state::occupied)
        {
            out_value = target->value;
            return true;
        }
        return false;
    }

    /**
     * @brief Checks if the map contains a specific key.
     * @param key The key to search for.
     * @return True if the key exists in the map, false otherwise.
     */
    bool contains_key(const Key &key) const
    {
        if (m_size == 0)
            return false;
        const entry *target = find_entry_internal(key);
        return target && target->state == entry_state::occupied;
    }

    /**
     * @brief Removes an element from the map.
     * @param key The key of the element to remove.
     * @return True if an element was removed, false otherwise.
     */
    bool erase(const Key &key)
    {
        if (m_size == 0)
            return false;

        entry *target_entry = find_entry_internal(key);
        if (target_entry->state == entry_state::occupied)
        {
            target_entry->state = entry_state::tombstone;
            target_entry->key.~Key();
            target_entry->value.~Value();
            m_size--;
            return true;
        }
        return false;
    }

private:
    entry *find_entry_internal(const Key &key) const
    {
        size_t index     = m_hasher(key) % m_capacity;
        entry *tombstone = nullptr;

        for (size_t count = 0; count < m_capacity; count++)
        {
            entry *current = &m_entries[index];
            if (current->state == entry_state::occupied)
            {
                if (m_key_equal(current->key, key))
                    return current;
            }
            else if (current->state == entry_state::empty)
                return tombstone ? tombstone : current;
            else if (!tombstone)
                tombstone = current;

            index = (index + 1) % m_capacity;
        }

        assert(tombstone);
        return tombstone;
    }

    void resize(size_t new_capacity)
    {
        entry *old_entries  = m_entries;
        size_t old_capacity = m_capacity;

        m_entries  = m_allocator.allocate(new_capacity);
        m_capacity = new_capacity;
        m_size     = 0;

        // Initialize all new entries to empty
        for (size_t i = 0; i < m_capacity; ++i)
        {
            m_entries[i].state = entry_state::empty;
        }

        if (!old_entries)
            return;

        // Re-hash all existing elements into the new table
        for (size_t i = 0; i < old_capacity; ++i)
        {
            if (old_entries[i].state == entry_state::occupied)
            {
                entry *dest = find_entry_internal(old_entries[i].key);

                // Move construct the key and value
                new (&dest->key) Key(move(old_entries[i].key));
                try
                {
                    new (&dest->value) Value(move(old_entries[i].value));
                }
                catch (...)
                {
                    dest->key.~Key();
                    throw;
                }
                dest->state = entry_state::occupied;
                m_size++;

                // Destruct the old moved-from objects
                old_entries[i].key.~Key();
                old_entries[i].value.~Value();
            }
        }

        m_allocator.deallocate(old_entries, old_capacity);
    }

    void clear_and_free()
    {
        if (!m_entries)
            return;
        clear();
        m_allocator.deallocate(m_entries, m_capacity);
        m_entries  = nullptr;
        m_size     = 0;
        m_capacity = 0;
    }

    entry *m_entries;
    size_t m_size;
    size_t m_capacity;

    entry_allocator_type m_allocator;
    Hasher m_hasher;
    KeyEqual m_key_equal;
};
} // namespace zabato