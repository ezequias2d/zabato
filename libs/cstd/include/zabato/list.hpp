#pragma once

#include <assert.h>
#include <stddef.h>

namespace zabato
{

/**
 * @struct bidirectional_iterator_tag
 * @brief An empty struct used to identify an iterator as bidirectional.
 */
struct bidirectional_iterator_tag
{
};

/**
 * @struct list_node
 * @brief The node to be embedded in a user's data structure.
 *
 * An object becomes part of an intrusive list by having a `list_node` as a
 * public member.
 */
struct list_node
{
    list_node *prev = nullptr;
    list_node *next = nullptr;
};

// Forward declaration of the list class
template <typename T, list_node T::*NodeMember> class list;

/**
 * @class list_iterator
 * @brief A C++-style iterator for the intrusive list.
 *
 * Supports bidirectional iteration and allows the use of range-based for loops.
 */
template <typename T, list_node T::*NodeMember> class list_iterator
{
public:
    // Custom iterator traits, removing dependency on <iterator>
    using iterator_category = bidirectional_iterator_tag;
    using value_type        = T;
    using difference_type   = ptrdiff_t;
    using pointer           = T *;
    using reference         = T &;

    /** @brief Default constructor. */
    list_iterator() : m_node(nullptr) {}

    /** @brief Constructs an iterator from a list node pointer. */
    explicit list_iterator(list_node *node) : m_node(node) {}

    /** @brief Dereferences the iterator to get a reference to the user's
     * object. */
    reference operator*() const { return *get_container(); }

    /** @brief Dereferences the iterator to get a pointer to the user's object.
     */
    pointer operator->() const { return get_container(); }

    /** @brief Pre-increment operator. Advances the iterator to the next
     * element. */
    list_iterator &operator++()
    {
        m_node = m_node->next;
        return *this;
    }

    /** @brief Post-increment operator. Advances the iterator to the next
     * element. */
    list_iterator operator++(int)
    {
        list_iterator temp = *this;
        m_node             = m_node->next;
        return temp;
    }

    /** @brief Pre-decrement operator. Moves the iterator to the previous
     * element. */
    list_iterator &operator--()
    {
        m_node = m_node->prev;
        return *this;
    }

    /** @brief Post-decrement operator. Moves the iterator to the previous
     * element. */
    list_iterator operator--(int)
    {
        list_iterator temp = *this;
        m_node             = m_node->prev;
        return temp;
    }

    /** @brief Equality comparison. */
    bool operator==(const list_iterator &rhs) const
    {
        return m_node == rhs.m_node;
    }
    /** @brief Inequality comparison. */
    bool operator!=(const list_iterator &rhs) const
    {
        return m_node != rhs.m_node;
    }

private:
    list_node *m_node;

    /**
     * @brief Calculates the pointer to the containing struct from the node
     * member. This is the core of the intrusive container logic.
     */
    pointer get_container() const
    {
        // Calculate the offset of the node member within the type T.
        const size_t offset = (size_t)&(((T *)nullptr)->*NodeMember);
        // Apply the offset to the current node pointer to find the start of the
        // container.
        return (T *)((char *)m_node - offset);
    }
};

/**
 * @class list
 * @brief An intrusive, doubly-linked list container.
 * @tparam T The type of the object that will be stored in the list.
 * @tparam NodeMember A pointer-to-member specifying which `list_node` in T to
 * use.
 *
 * Example Usage:
 * @code
 * struct MyObject {
 * int data;
 * list_node node;
 * };
 *
 * list<MyObject, &MyObject::node> my_list;
 * MyObject obj1, obj2;
 * my_list.push_back(obj1);
 * my_list.push_back(obj2);
 *
 * for(MyObject& obj : my_list) {
 * // ...
 * }
 * @endcode
 */
template <typename T, list_node T::*NodeMember> class list
{
public:
    using iterator = list_iterator<T, NodeMember>;

    /** @brief Initializes an empty list with a sentinel node. */
    list()
    {
        m_sentinel.next = &m_sentinel;
        m_sentinel.prev = &m_sentinel;
    }

    /** @brief Destroys the list and unlinks all nodes. */
    ~list() { clear(); }

    /** @brief Returns an iterator to the beginning of the list. */
    iterator begin() { return iterator(m_sentinel.next); }
    /** @brief Returns an iterator to the end of the list (the sentinel). */
    iterator end() { return iterator(&m_sentinel); }

    /** @brief Checks if the list is empty. */
    bool empty() const { return m_sentinel.next == &m_sentinel; }

    /** @brief Returns a reference to the first element. Undefined if empty. */
    T &front() { return *begin(); }

    /** @brief Returns a reference to the last element. Undefined if empty. */
    T &back() { return *(--end()); }

    /**
     * @brief Adds an item to the front of the list.
     * @param item The item to add. It must not already be in a list.
     */
    void push_front(T &item)
    {
        list_node *node = &(item.*NodeMember);
        assert(node->prev == nullptr && node->next == nullptr);

        node->next            = m_sentinel.next;
        node->prev            = &m_sentinel;
        m_sentinel.next->prev = node;
        m_sentinel.next       = node;
    }

    /**
     * @brief Adds an item to the back of the list.
     * @param item The item to add. It must not already be in a list.
     */
    void push_back(T &item)
    {
        list_node *node = &(item.*NodeMember);
        assert(node->prev == nullptr && node->next == nullptr);

        node->prev            = m_sentinel.prev;
        node->next            = &m_sentinel;
        m_sentinel.prev->next = node;
        m_sentinel.prev       = node;
    }

    /**
     * @brief Removes an item from whatever list it is currently in.
     * This is a static function because the node itself contains all necessary
     * information to remove itself from its neighbors.
     * @param item The item to remove.
     */
    static void remove(T &item)
    {
        list_node *node = &(item.*NodeMember);
        if (node->prev && node->next)
        { // Only remove if it's actually in a list
            node->prev->next = node->next;
            node->next->prev = node->prev;
            // Reset the node's pointers to prevent dangling references
            node->prev = nullptr;
            node->next = nullptr;
        }
    }

    /** @brief Unlinks all nodes from the list. */
    void clear()
    {
        list_node *node = m_sentinel.next;
        while (node != &m_sentinel)
        {
            list_node *next = node->next;
            node->prev      = nullptr;
            node->next      = nullptr;
            node            = next;
        }
        m_sentinel.next = &m_sentinel;
        m_sentinel.prev = &m_sentinel;
    }

    /** @brief Returns the number of elements in the list. O(N). */
    size_t size() const
    {
        size_t count          = 0;
        const list_node *node = m_sentinel.next;
        while (node != &m_sentinel)
        {
            count++;
            node = node->next;
        }
        return count;
    }

private:
    list_node m_sentinel;
};

} // namespace zabato