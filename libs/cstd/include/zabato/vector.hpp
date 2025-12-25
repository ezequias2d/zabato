
#pragma once

#include <zabato/allocator.hpp>
#include <zabato/utils.hpp>

#include <assert.h>
#include <initializer_list>
#include <iterator>

namespace zabato
{
template <class T> class const_vector_iterator;

/**
 * @class vector_iterator
 * @brief A random-access iterator for the vector class.
 */
template <class T> class vector_iterator
{
public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type        = T;
    using difference_type   = ptrdiff_t;
    using pointer           = T *;
    using reference         = T &;

    /** @brief Constructs a new iterator. */
    vector_iterator(pointer ptr = nullptr) : m_ptr(ptr) {}

    friend class const_vector_iterator<T>;

    /** @brief Dereferences the iterator to get a reference to the element. */
    reference operator*() const { return *m_ptr; }
    /** @brief Dereferences the iterator to get a pointer to the element. */
    pointer operator->() const { return m_ptr; }

    /** @brief Pre-increments the iterator to the next element. */
    vector_iterator &operator++()
    {
        ++m_ptr;
        return *this;
    }
    /** @brief Post-increments the iterator to the next element. */
    vector_iterator operator++(int)
    {
        vector_iterator tmp = *this;
        ++(*this);
        return tmp;
    }
    /** @brief Pre-decrements the iterator to the previous element. */
    vector_iterator &operator--()
    {
        --m_ptr;
        return *this;
    }
    /** @brief Post-decrements the iterator to the previous element. */
    vector_iterator operator--(int)
    {
        vector_iterator tmp = *this;
        --(*this);
        return tmp;
    }

    /** @brief Advances the iterator by a given offset. */
    vector_iterator &operator+=(difference_type offset)
    {
        m_ptr += offset;
        return *this;
    }
    /** @brief Returns a new iterator advanced by a given offset. */
    vector_iterator operator+(difference_type offset) const
    {
        return vector_iterator(m_ptr + offset);
    }
    /** @brief Moves the iterator backward by a given offset. */
    vector_iterator &operator-=(difference_type offset)
    {
        m_ptr -= offset;
        return *this;
    }
    /** @brief Returns a new iterator moved backward by a given offset. */
    vector_iterator operator-(difference_type offset) const
    {
        return vector_iterator(m_ptr - offset);
    }
    /** @brief Calculates the distance between two iterators. */
    difference_type operator-(const vector_iterator &other) const
    {
        return m_ptr - other.m_ptr;
    }

    /** @brief Accesses an element at a given offset from the iterator's
     * position. */
    reference operator[](difference_type offset) const { return m_ptr[offset]; }

    bool operator==(const vector_iterator &other) const
    {
        return m_ptr == other.m_ptr;
    }
    bool operator!=(const vector_iterator &other) const
    {
        return m_ptr != other.m_ptr;
    }
    bool operator<(const vector_iterator &other) const
    {
        return m_ptr < other.m_ptr;
    }
    bool operator>(const vector_iterator &other) const
    {
        return m_ptr > other.m_ptr;
    }
    bool operator<=(const vector_iterator &other) const
    {
        return m_ptr <= other.m_ptr;
    }
    bool operator>=(const vector_iterator &other) const
    {
        return m_ptr >= other.m_ptr;
    }

private:
    pointer m_ptr;
};

/**
 * @class const_vector_iterator
 * @brief A const random-access iterator for the vector class.
 */
template <class T> class const_vector_iterator
{
public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type        = T;
    using difference_type   = ptrdiff_t;
    using pointer           = const T *;
    using reference         = const T &;

    /** @brief Constructs a new const_iterator. */
    const_vector_iterator(pointer ptr = nullptr) : m_ptr(ptr) {}
    /** @brief Constructs a const_iterator from a non-const iterator. */
    const_vector_iterator(const vector_iterator<T> &other) : m_ptr(other.m_ptr)
    {
    }

    /** @brief Dereferences the iterator to get a const reference to the
     * element. */
    reference operator*() const { return *m_ptr; }
    /**
     * @brief Dereferences the iterator to get a const pointer to the element.
     */
    pointer operator->() const { return m_ptr; }

    /** @brief Pre-increments the iterator to the next element. */
    const_vector_iterator &operator++()
    {
        ++m_ptr;
        return *this;
    }
    /** @brief Post-increments the iterator to the next element. */
    const_vector_iterator operator++(int)
    {
        const_vector_iterator tmp = *this;
        ++(*this);
        return tmp;
    }
    /** @brief Pre-decrements the iterator to the previous element. */
    const_vector_iterator &operator--()
    {
        --m_ptr;
        return *this;
    }
    /** @brief Post-decrements the iterator to the previous element. */
    const_vector_iterator operator--(int)
    {
        const_vector_iterator tmp = *this;
        --(*this);
        return tmp;
    }

    /** @brief Advances the iterator by a given offset. */
    const_vector_iterator &operator+=(difference_type offset)
    {
        m_ptr += offset;
        return *this;
    }
    /** @brief Returns a new iterator advanced by a given offset. */
    const_vector_iterator operator+(difference_type offset) const
    {
        return const_vector_iterator(m_ptr + offset);
    }
    /** @brief Moves the iterator backward by a given offset. */
    const_vector_iterator &operator-=(difference_type offset)
    {
        m_ptr -= offset;
        return *this;
    }
    /** @brief Returns a new iterator moved backward by a given offset. */
    const_vector_iterator operator-(difference_type offset) const
    {
        return const_vector_iterator(m_ptr - offset);
    }
    /** @brief Calculates the distance between two iterators. */
    difference_type operator-(const const_vector_iterator &other) const
    {
        return m_ptr - other.m_ptr;
    }

    /** @brief Accesses an element at a given offset from the iterator's
     * position. */
    reference operator[](difference_type offset) const { return m_ptr[offset]; }

    bool operator==(const const_vector_iterator &other) const
    {
        return m_ptr == other.m_ptr;
    }
    bool operator!=(const const_vector_iterator &other) const
    {
        return m_ptr != other.m_ptr;
    }
    bool operator<(const const_vector_iterator &other) const
    {
        return m_ptr < other.m_ptr;
    }
    bool operator>(const const_vector_iterator &other) const
    {
        return m_ptr > other.m_ptr;
    }
    bool operator<=(const const_vector_iterator &other) const
    {
        return m_ptr <= other.m_ptr;
    }
    bool operator>=(const const_vector_iterator &other) const
    {
        return m_ptr >= other.m_ptr;
    }

private:
    pointer m_ptr;
};

/**
 * @class vector
 * @tparam T The type of elements stored in the vector.
 * @tparam Allocator The allocator to use for memory management.
 */
template <class T, class Allocator = allocator<T>> class vector
{
public:
    using allocator_type = Allocator;
    using iterator       = vector_iterator<T>;
    using const_iterator = const_vector_iterator<T>;

    /** @brief Constructs an empty vector with zero size and capacity. */
    vector() noexcept : m_data(nullptr), m_size(0), m_capacity(0), m_allocator()
    {
    }

    /** @brief Constructs an empty vector and reserves space for `capacity`
     * elements. */
    vector(size_t size) : vector()
    {
        reserve(size);
        m_size = size;
    }

    /** @brief Iterator range constructor */
    template <typename InputIterator>
    vector(InputIterator first, InputIterator last) : vector()
    {
        for (auto it = first; it != last; ++it)
        {
            push_back(*it);
        }
    }

    /** @brief Copy constructor */
    vector(const vector &other)
        : m_data(nullptr), m_size(0), m_capacity(0), m_allocator()
    {
        if (other.m_size > 0)
        {
            reserve(other.m_size);
            if (is_pod<T>::value)
            {
                memcpy(m_data, other.m_data, other.m_size * sizeof(T));
            }
            else
            {
                for (size_t i = 0; i < other.m_size; ++i)
                {
                    new (&m_data[i]) T(other.m_data[i]);
                }
            }
            m_size = other.m_size;
        }
    }

    /** @brief Move constructor */
    vector(vector &&other) noexcept
        : m_data(other.m_data), m_size(other.m_size),
          m_capacity(other.m_capacity), m_allocator()
    {
        other.m_data     = nullptr;
        other.m_size     = 0;
        other.m_capacity = 0;
    }

    vector(std::initializer_list<T> init) : vector()
    {
        reserve(init.size());
        for (const auto &item : init)
        {
            push_back(item);
        }
    }

    /** @brief Copy assignment operator */
    vector &operator=(const vector &other)
    {
        if (this != &other)
        {
            clear();
            if (other.m_size > 0)
            {
                if (m_capacity < other.m_size)
                {
                    if (m_data)
                    {
                        m_allocator.deallocate(m_data, m_capacity);
                        m_data     = nullptr;
                        m_capacity = 0;
                    }
                    reserve(other.m_size);
                }
                if (is_pod<T>::value)
                {
                    memcpy(m_data, other.m_data, other.m_size * sizeof(T));
                }
                else
                {
                    for (size_t i = 0; i < other.m_size; ++i)
                    {
                        m_data[i] = other.m_data[i];
                    }
                }
                m_size = other.m_size;
            }
        }
        return *this;
    }

    /** @brief Move assignment operator */
    vector &operator=(vector &&other) noexcept
    {
        if (this != &other)
        {
            // Clean up current data
            if (!is_pod<T>::value)
            {
                for (size_t i = 0; i < m_size; ++i)
                {
                    m_data[i].~T();
                }
            }
            if (m_data)
            {
                m_allocator.deallocate(m_data, m_capacity);
            }

            // Take ownership of other's data
            m_data     = other.m_data;
            m_size     = other.m_size;
            m_capacity = other.m_capacity;

            // Reset other
            other.m_data     = nullptr;
            other.m_size     = 0;
            other.m_capacity = 0;
        }
        return *this;
    }

    /** @brief Destroys the vector, calling destructors on its elements and
     * freeing memory. */
    ~vector() noexcept
    {
        if (!is_pod<T>::value)
            for (size_t i = 0; i < m_size; ++i)
                m_data[i].~T();

        if (m_data)
            m_allocator.deallocate(m_data, m_capacity);
    }

    constexpr bool empty() const { return m_size == 0; }

    /** @brief Removes all elements from the vector, leaving the capacity
     * unchanged. */
    void clear()
    {
        if (!is_pod<T>::value)
        {
            for (size_t i = 0; i < m_size; ++i)
            {
                m_data[i].~T();
            }
        }
        m_size = 0;
    }

    /**
     * @brief Changes the number of elements stored.
     * @param new_size The new size of the vector.
     */
    void resize(size_t new_size, T val = T())
    {
        if (new_size > m_size)
        {
            if (new_size > m_capacity)
                reserve(new_size);

            if (!is_pod<T>::value)
                for (size_t i = m_size; i < new_size; ++i)
                    new (&m_data[i]) T(val);
        }
        else if (new_size < m_size)
        {
            if (!is_pod<T>::value)
                for (size_t i = new_size; i < m_size; ++i)
                    m_data[i].~T();
        }
        m_size = new_size;
    }

    /**
     * @brief Replaces the contents of the vector with a range of elements.
     * @param first Pointer to the first element to copy.
     * @param last Pointer one past the last element to copy.
     */
    void assign(const T *first, const T *last)
    {
        clear();
        const size_t count = last - first;
        if (count > m_capacity)
        {
            if (m_data)
                m_allocator.deallocate(m_data, m_capacity);
            m_data     = m_allocator.allocate(count);
            m_capacity = count;
        }

        memcpy(m_data, first, count * sizeof(T));

        m_size = count;
    }

    /** @brief Adds an element to the end of the vector by copying. */
    void push_back(const T &value)
    {
        if (m_size >= m_capacity)
            reserve(m_capacity == 0 ? 8 : (m_capacity * 3) / 2);

        new (&m_data[m_size]) T(value);
        ++m_size;
    }

    /** @brief Adds an element to the end of the vector by moving. */
    void push_back(T &&value)
    {
        if (m_size >= m_capacity)
            reserve(m_capacity == 0 ? 8 : m_capacity * 2);

        new (&m_data[m_size]) T(zabato::move(value));
        ++m_size;
    }

    /** @brief Constructs an element in-place at the end of the vector. */
    template <typename... Args> void emplace_back(Args &&...args)
    {
        if (m_size >= m_capacity)
            reserve(m_capacity == 0 ? 8 : (m_capacity * 3) / 2);

        new (&m_data[m_size]) T(static_cast<Args &&>(args)...);
        ++m_size;
    }

    /** @brief Removes the last element from the vector. */
    void pop_back()
    {
        assert(m_size > 0);
        --m_size;
        if (!is_pod<T>::value)
            m_data[m_size].~T();
    }

    /**
     * @brief Removes the element at the specified index.
     * @param index The index of the element to remove.
     */
    void remove_at(size_t index)
    {
        assert(index < m_size);

        if (is_pod<T>::value)
        {
            // For POD types, we can use memmove
            if (index < m_size - 1)
            {
                memmove(m_data + index,
                        m_data + index + 1,
                        (m_size - index - 1) * sizeof(T));
            }
        }
        else
        {
            // For non-POD types, we move assignments
            for (size_t i = index; i < m_size - 1; ++i)
            {
                m_data[i] = zabato::move(m_data[i + 1]);
            }
            // Destruct the last element which is now effectively moved-from
            m_data[m_size - 1].~T();
        }

        --m_size;
    }

    /**
     * @brief Removes the first occurrence of the specified value.
     * @param val The value to remove.
     * @return True if the value was found and removed, false otherwise.
     */
    bool remove(const T &val)
    {
        for (size_t i = 0; i < m_size; ++i)
        {
            if (m_data[i] == val)
            {
                remove_at(i);
                return true;
            }
        }
        return false;
    }

    /** @brief Accesses an element by index with bounds checking (in debug). */
    T &operator[](size_t index)
    {
        assert(index < m_size);
        return m_data[index];
    }

    /** @brief Accesses an element by index with bounds checking (in debug),
     * const version. */
    const T &operator[](size_t index) const
    {
        assert(index < m_size);
        return m_data[index];
    }

    /** @return The number of elements in the vector. */
    size_t size() const { return m_size; }

    /** @return The number of elements the vector can hold before reallocating.
     */
    size_t capacity() const { return m_capacity; }

    /** @brief Requests a change in capacity. */
    void reserve(size_t new_capacity)
    {
        if (new_capacity <= m_capacity)
            return;

        T *new_data;
        if (m_data == nullptr)
            new_data = m_allocator.allocate(new_capacity);
        else
            new_data = m_allocator.reallocate(m_data, new_capacity);

        assert(new_data != nullptr);

        m_data     = new_data;
        m_capacity = new_capacity;
    }

    /** @brief Shrinks the capacity of the vector to fit the size. */
    void shrink_to_fit()
    {
        m_data     = m_allocator.reallocate(m_data, m_size);
        m_capacity = m_size;
    }

    /** @return An iterator to the beginning of the vector. */
    iterator begin() { return iterator(m_data); }
    /** @return A const_iterator to the beginning of the vector. */
    const_iterator begin() const { return const_iterator(m_data); }
    /** @return A const_iterator to the beginning of the vector. */
    const_iterator cbegin() const { return const_iterator(m_data); }

    /** @return An iterator to one past the end of the vector. */
    iterator end() { return iterator(m_data + m_size); }
    /** @return A const_iterator to one past the end of the vector. */
    const_iterator end() const { return const_iterator(m_data + m_size); }
    /** @return A const_iterator to one past the end of the vector. */
    const_iterator cend() const { return const_iterator(m_data + m_size); }

    T *data() { return m_data; }
    const T *data() const { return m_data; }

    T &back() { return m_data[m_size - 1]; }
    const T &back() const { return m_data[m_size - 1]; }

private:
    T *m_data;             ///< Pointer to the dynamically allocated array.
    size_t m_size;         ///< Number of elements currently in the vector.
    size_t m_capacity;     ///< Total allocated capacity of the array.
    Allocator m_allocator; ///< The allocator instance.
};
} // namespace zabato
