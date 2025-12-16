#pragma once

#include <new>
#include <zabato/allocator.hpp>
#include <zabato/utils.hpp>

namespace zabato
{
template <typename Key,
          typename Hash      = hash<Key>,
          typename Equals    = equal_to<Key>,
          typename Allocator = allocator<Key>>
class hash_set
{
private:
    enum class entry_state : uint8_t
    {
        empty,
        tombstone,
        occupied
    };

    struct entry
    {
        Key key;
        entry_state state;
    };

public:
    /**
     * @brief A non-constant iterator for the hash_set.
     */
    class iterator
    {
    public:
        using value_type      = Key;
        using difference_type = ptrdiff_t;
        using pointer         = Key *;
        using reference       = Key &;

        iterator() : m_ptr(nullptr), m_end(nullptr) {}
        iterator(entry *ptr, entry *end) : m_ptr(ptr), m_end(end) {}

        /** @brief Dereferences the iterator to get the key. */
        reference operator*() const { return m_ptr->key; }

        /** @brief Dereferences the iterator to get the key. */
        pointer operator->() const { return &m_ptr->key; }

        /** @brief Advances the iterator to the next occupied element. (Prefix)
         */
        iterator &operator++()
        {
            // Move to the next slot
            ++m_ptr;
            // Scan forward to find the next occupied one
            while (m_ptr < m_end && m_ptr->state != entry_state::occupied)
            {
                ++m_ptr;
            }
            return *this;
        }

        /** @brief Advances the iterator. (Postfix) */
        iterator operator++(int)
        {
            iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        /** @brief Checks for equality. */
        bool operator==(const iterator &other) const
        {
            return m_ptr == other.m_ptr;
        }

        /** @brief Checks for inequality. */
        bool operator!=(const iterator &other) const
        {
            return m_ptr != other.m_ptr;
        }

    private:
        entry *m_ptr;
        entry *m_end;
    };

    /**
     * @brief A constant iterator for the hash_set.
     */
    class const_iterator
    {
    public:
        using value_type      = const Key;
        using difference_type = ptrdiff_t;
        using pointer         = const Key *;
        using reference       = const Key &;

        const_iterator() : m_ptr(nullptr), m_end(nullptr) {}
        const_iterator(const entry *ptr, const entry *end)
            : m_ptr(ptr), m_end(end)
        {
        }

        /** @brief Dereferences the iterator to get the key. */
        reference operator*() const { return m_ptr->key; }

        /** @brief Dereferences the iterator to get the key. */
        pointer operator->() const { return &m_ptr->key; }

        /** @brief Advances the iterator to the next occupied element. (Prefix)
         */
        const_iterator &operator++()
        {
            ++m_ptr;
            while (m_ptr < m_end && m_ptr->state != entry_state::occupied)
            {
                ++m_ptr;
            }
            return *this;
        }

        /** @brief Advances the iterator. (Postfix) */
        const_iterator operator++(int)
        {
            const_iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        /** @brief Checks for equality. */
        bool operator==(const const_iterator &other) const
        {
            return m_ptr == other.m_ptr;
        }

        /** @brief Checks for inequality. */
        bool operator!=(const const_iterator &other) const
        {
            return m_ptr != other.m_ptr;
        }

    private:
        const entry *m_ptr;
        const entry *m_end;
    };

    /** @brief Constructs an empty hash_set. */
    hash_set() : hash_set(0) {}

    /** @brief Constructs an empty hash_set with an initial capacity. */
    hash_set(size_t capacity) : m_size(0), m_capacity(0), m_entries(nullptr)
    {
        resize(capacity);
    }

    /** @brief Destroys the hash_set and its elements. */
    ~hash_set() { clear_and_free(); }

    /**  @brief Remove all items from this set */
    void clear()
    {
        if (!is_pod<Key>::value)
            for (size_t i = 0; i < m_capacity; ++i)
                if (m_entries[i].state == entry_state::occupied)
                    m_entries[i].key.~Key();

        for (size_t i = 0; i < m_capacity; ++i)
            m_entries[i].state = entry_state::empty;
        m_size = 0;
    }

    /** @return The number of items in the set. */
    size_t count() const { return m_size; }

    /**
     * @brief Checks if the set contains a specific key.
     * @tparam K The type of the key to search for (for transparent lookup).
     * @param key The key to search for.
     * @return True if the key is in the set, false otherwise.
     */
    template <typename K> bool contains(const K &key) const
    {
        if (m_size == 0)
            return false;
        const entry *target = find_entry_internal(key);
        return target && target->state == entry_state::occupied;
    }

    /**
     * @brief Removes a key from the set.
     * @tparam K The type of the key to remove (for transparent lookup).
     * @param key The key to remove.
     */
    template <typename K> void remove(const K &key)
    {
        if (m_size == 0)
            return;
        entry *target = find_entry_internal(key);
        if (target && target->state == entry_state::occupied)
        {
            target->key.~Key();
            target->state = entry_state::tombstone;
            m_size--;
        }
    }

    /**
     * @brief Adds a key to the set.
     * @param key The key to add.
     * @return True if the key was newly inserted, false if it already existed.
     */
    bool add(const Key &key)
    {
        if (m_capacity == 0 || m_size + 1 > m_capacity * 3 / 4)
            resize(m_capacity == 0 ? 8 : (m_capacity * 3) / 2);

        entry *target = find_entry_internal(key);
        if (target->state == entry_state::occupied)
            return false; // Key already exists

        if (is_pod<Key>::value)
            target->key = key;
        else
            new (&target->key) Key(key);

        target->state = entry_state::occupied;
        m_size++;
        return true;
    }

    /**
     * @brief Tries to get the actual stored key that is equal to a given key.
     * @tparam K The type of the key to search for (for transparent lookup).
     * @param equalKey The key to search for.
     * @param[out] actualKey A reference to store the actual found key.
     * @return True if a key was found, false otherwise.
     */
    template <typename K> bool try_get(const K &equalKey, Key &actualKey) const
    {
        if (m_size == 0)
            return false;
        const entry *target = find_entry_internal(equalKey);
        if (target && target->state == entry_state::occupied)
        {
            actualKey = target->key;
            return true;
        }
        return false;
    }

    /**
     * @brief Returns an iterator to the first element in the set.
     */
    iterator begin()
    {
        if (m_size == 0)
            return end();

        entry *ptr     = m_entries;
        entry *end_ptr = m_entries + m_capacity;
        while (ptr < end_ptr && ptr->state != entry_state::occupied)
        {
            ++ptr;
        }
        return iterator(ptr, end_ptr);
    }

    /**
     * @brief Returns an iterator to one past the last element in the set.
     */
    iterator end()
    {
        return iterator(m_entries + m_capacity, m_entries + m_capacity);
    }

    /**
     * @brief Returns a const iterator to the first element in the set.
     */
    const_iterator begin() const
    {
        if (m_size == 0)
            return end();

        entry *ptr     = m_entries;
        entry *end_ptr = m_entries + m_capacity;
        while (ptr < end_ptr && ptr->state != entry_state::occupied)
        {
            ++ptr;
        }
        return const_iterator(ptr, end_ptr);
    }

    /**
     * @brief Returns a const iterator to one past the last element in the set.
     */
    const_iterator end() const
    {
        return const_iterator(m_entries + m_capacity, m_entries + m_capacity);
    }

private:
    using entry_allocator_type =
        typename Allocator::template rebind<entry>::other;

    template <typename K> entry *find_entry_internal(const K &key) const
    {
        if (m_capacity == 0)
            return nullptr;
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
            else // It's a tombstone
                if (!tombstone)
                    tombstone = current;
            index = (index + 1) % m_capacity;
        }
        return tombstone;
    }

    void resize(size_t new_capacity)
    {
        entry *new_entries = m_allocator.allocate(new_capacity);

        // Initialize new entries
        for (size_t i = 0; i < new_capacity; ++i)
            new_entries[i].state = entry_state::empty;

        try
        {
            // Re-hash all existing elements into the new table
            for (size_t i = 0; i < m_capacity; ++i)
            {
                if (m_entries[i].state == entry_state::occupied)
                {
                    Key &key = m_entries[i].key;
                    // Find slot in new buffer manually since we can't use
                    // member functions easily on raw buffer
                    size_t index     = m_hasher(key) % new_capacity;
                    entry *dest      = nullptr;
                    entry *tombstone = nullptr;

                    // Search for insertion spot
                    for (size_t c = 0; c < new_capacity; c++)
                    {
                        entry *curr = &new_entries[index];
                        if (curr->state == entry_state::empty)
                        {
                            dest = tombstone ? tombstone : curr;
                            break;
                        }
                        else if (curr->state == entry_state::tombstone)
                        {
                            if (!tombstone)
                                tombstone = curr;
                        }
                        index = (index + 1) % new_capacity;
                    }

                    // Perform move/copy
                    if (is_pod<Key>::value)
                        dest->key = key;
                    else
                        new (&dest->key) Key(move(key)); // Move construct

                    dest->state = entry_state::occupied;
                }
            }
        }
        catch (...)
        {
            // Rollback: destroy constructed elements in new_entries and free
            for (size_t i = 0; i < new_capacity; ++i)
            {
                if (new_entries[i].state == entry_state::occupied &&
                    !is_pod<Key>::value)
                    new_entries[i].key.~Key();
            }
            m_allocator.deallocate(new_entries, new_capacity);
            throw;
        }

        // Commit: Destroy old elements and swap
        if (!is_pod<Key>::value)
        {
            for (size_t i = 0; i < m_capacity; ++i)
                if (m_entries[i].state == entry_state::occupied)
                    m_entries[i].key.~Key();
        }

        if (m_entries)
            m_allocator.deallocate(m_entries, m_capacity);

        m_entries  = new_entries;
        m_capacity = new_capacity;
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
    Hash m_hasher;
    Equals m_key_equal;
};
} // namespace zabato