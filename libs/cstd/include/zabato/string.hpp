#pragma once

#include <zabato/allocator.hpp>
#include <zabato/endian.hpp>
#include <zabato/utils.hpp>

#include <assert.h>
#include <stddef.h>
#include <string.h>
#include <type_traits>

namespace zabato
{
template <typename Allocator> class basic_string;

class string_view
{
public:
    using iterator        = const char *;
    using const_iterator  = const char *;
    using value_type      = char;
    using reference       = const char &;
    using const_reference = const char &;
    using size_type       = size_t;
    using difference_type = ptrdiff_t;

    static constexpr size_type npos = size_type(-1);

    /** @brief Constructs empty string view. */
    string_view() : m_data(""), m_size(0) {}

    /** @brief Constructs from C-string. */
    string_view(const char *s) : m_data(s), m_size(strlen(s)) {}

    /** @brief Constructs from pointer and length. */
    string_view(const char *s, size_t count) : m_data(s), m_size(count) {}

    template <class Allocator>
    constexpr string_view(const basic_string<Allocator> &str)
        : m_data(str.c_str()), m_size(str.size())
    {
    }

    /** @brief Pointer to data. */
    constexpr const char *data() const { return m_data; }

    /** @brief Size of the view. */
    size_t size() const { return m_size; }
    /** @brief Length of the view. */
    size_t length() const { return m_size; }
    /** @brief Checks if empty. */
    bool empty() const { return m_size == 0; }

    /** @brief Access char at index. */
    const char &operator[](size_t index) const { return m_data[index]; }

    constexpr bool operator==(const string_view &rhs) const
    {
        if (size() != rhs.size())
            return false;
        return memcmp(data(), rhs.data(), size()) == 0;
    }

    /** @brief Returns const iterator to beginning. */
    constexpr iterator begin() const { return m_data; }

    /** @brief Returns const iterator to end. */
    constexpr iterator end() const { return m_data + m_size; }

    /** @brief Returns const iterator to beginning. */
    constexpr const_iterator cbegin() const { return m_data; }

    /** @brief Returns const iterator to end. */
    constexpr const_iterator cend() const { return m_data + m_size; }

    /** @brief Remove prefix characters. */
    constexpr void remove_prefix(size_t n)
    {
        assert(n <= m_size);
        m_data += n;
        m_size -= n;
    }

    /** @brief Remove suffix characters. */
    constexpr void remove_suffix(size_t n)
    {
        assert(n <= m_size);
        m_size -= n;
    }

    /** @brief Removes the last character. */
    constexpr void pop_back() { remove_suffix(1); }

    /** @brief Removes the first character. */
    constexpr void pop_front() { remove_prefix(1); }

    /** @brief Returns a substring. */
    constexpr string_view substr(size_t pos = 0, size_t count = npos) const
    {
        assert(pos <= m_size);
        size_t rcount = min(count, m_size - pos);
        return string_view(m_data + pos, rcount);
    }

#pragma region Find

    /** @brief Checks if string starts with prefix. */
    constexpr bool starts_with(string_view sv) const
    {
        return size() >= sv.size() && memcmp(data(), sv.data(), sv.size()) == 0;
    }

    /** @brief Checks if string starts with char. */
    constexpr bool starts_with(char c) const
    {
        return !empty() && front() == c;
    }

    /** @brief Checks if string ends with suffix. */
    constexpr bool ends_with(string_view sv) const
    {
        return size() >= sv.size() &&
               memcmp(data() + size() - sv.size(), sv.data(), sv.size()) == 0;
    }

    /** @brief Checks if string ends with char. */
    constexpr bool ends_with(char c) const { return !empty() && back() == c; }

    /** @brief Finds a substring. */
    constexpr size_t find(string_view v, size_t pos = 0) const
    {
        if (pos > size())
            return npos;
        if (v.empty())
            return pos;

        const char *p      = m_data + pos;
        const char *end    = m_data + m_size;
        const char *needle = v.data();
        size_t needle_len  = v.size();

        // Simple brute force for constexpr compatibility
        while (p + needle_len <= end)
        {
            if (memcmp(p, needle, needle_len) == 0)
                return p - m_data;
            ++p;
        }
        return npos;
    }

    /** @brief Finds a character. */
    constexpr size_t find(char c, size_t pos = 0) const
    {
        if (pos >= size())
            return npos;
        const char *p =
            static_cast<const char *>(memchr(m_data + pos, c, size() - pos));
        return p ? p - m_data : npos;
    }

    /** @brief Finds a substring from the end. */
    constexpr size_t rfind(string_view v, size_t pos = npos) const
    {
        if (size() < v.size())
            return npos;
        if (v.empty())
            return min(pos, size());

        pos           = min(pos, size() - v.size());
        const char *p = m_data + pos;

        for (; p >= m_data; --p)
        {
            if (memcmp(p, v.data(), v.size()) == 0)
                return p - m_data;
        }
        return npos;
    }

    /** @brief Finds a character from the end. */
    constexpr size_t rfind(char c, size_t pos = npos) const
    {
        if (empty())
            return npos;
        pos = min(pos, size() - 1);
        for (const char *p = m_data + pos; p >= m_data; --p)
        {
            if (*p == c)
                return p - m_data;
        }
        return npos;
    }

#pragma endregion Find

    /** @brief Returns reference to first character. */
    constexpr const char &front() const
    {
        assert(!empty());
        return m_data[0];
    }
    /** @brief Returns reference to last character. */
    constexpr const char &back() const
    {
        assert(!empty());
        return m_data[m_size - 1];
    }

private:
    const char *m_data;
    size_t m_size;
};

/**
 * @class basic_string
 * @brief A string class with Small String Optimization (SSO).
 * @tparam Allocator The allocator to use for heap allocations.
 */
template <typename Allocator = allocator<char>> class basic_string
{
public:
    using iterator        = char *;
    using const_iterator  = const char *;
    using value_type      = char;
    using reference       = char &;
    using const_reference = const char &;
    using size_type       = size_t;
    using difference_type = ptrdiff_t;

    using allocator_type = Allocator;

    static constexpr size_type npos = size_type(-1);

    /** @brief Default constructor. Creates an empty string. */
    basic_string() { set_small(0); }

    /** @brief Constructs a string from a C-style string literal. */
    basic_string(const char *s) : m_allocator()
    {
        size_t len = s ? strlen(s) : 0;
        if (len <= SSO_CAPACITY)
        {
            set_small(len);
            memcpy(small, s, len);
            small[len] = '\0';
        }
        else
        {
            allocate_large(len);
            memcpy(large.data, s, len);
            large.data[len] = '\0';

            large.data = static_cast<char *>(m_allocator.allocate(len + 1));
            memcpy(large.data, s, len);
            large.data[len] = '\0';
            large.size      = len;
            large.capacity  = len << 1;
        }
    }

    /** @brief Constructs a string from a C-style string literal. */
    basic_string(const char *s, size_t len) : m_allocator()
    {
        if (len <= SSO_CAPACITY)
        {
            set_small(0);
            memcpy(small, s, len);
            small[len] = '\0';
            set_small_size(len);
        }
        else
        {
            large.data = static_cast<char *>(m_allocator.allocate(len + 1));
            memcpy(large.data, s, len);
            large.data[len] = '\0';
            large.size      = len;
            large.capacity  = len << 1;
        }
    }

    /** @brief Constructs from string_view. */
    basic_string(const string_view &sv) : basic_string(sv.data(), sv.size()) {}

    /** @brief Copy constructor. */
    basic_string(const basic_string &other)
    {
        if (other.is_small())
        {
            memcpy(small, other.small, sizeof(small));
        }
        else
        {
            size_t size = other.large.size + 1;
            large.data  = static_cast<char *>(m_allocator.allocate(size));
            memcpy(large.data, other.large.data, size);
            large.size     = other.large.size;
            large.capacity = other.large.capacity;
        }
    }

    /** @brief Move constructor. */
    basic_string(basic_string &&other)
    {
        large       = other.large;
        m_allocator = move(other.m_allocator);
        other.set_small(0);
    }

    /** @brief Destructor. */
    ~basic_string()
    {
        if (!is_small())
            m_allocator.deallocate(large.data, (large.capacity >> 1) + 1);
    }

    /** @brief Returns reference to the last character. Assert if empty. */
    char &back()
    {
        assert(!empty());
        return begin()[size() - 1];
    }

    /** @brief Returns const reference to the last character. Assert if empty.
     */
    const char &back() const
    {
        assert(!empty());
        return begin()[size() - 1];
    }

    /** @brief Returns reference to the first character. Assert if empty. */
    char &front()
    {
        assert(!empty());
        return begin()[0];
    }

    /** @brief Returns const reference to the first character. Assert if empty.
     */
    const char &front() const
    {
        assert(!empty());
        return begin()[0];
    }

    char &at(size_t pos)
    {
        assert(pos < size());
        return begin()[pos];
    }

    const char &at(size_t pos) const
    {
        assert(pos < size());
        return begin()[pos];
    }

    /** @brief Appends a character. */
    void push_back(char c) { *this += c; }

    /** @brief Removes the last character. Assert if empty. */
    void pop_back()
    {
        assert(!empty());
        if (is_small())
        {
            size_t s = size();
            set_small_size(s - 1);
            small[s - 1] = '\0';
        }
        else
        {
            large.size--;
            large.data[large.size] = '\0';
        }
    }

    /** @brief Assignment operator. */
    basic_string &operator=(basic_string other)
    {
        swap(*this, other);
        return *this;
    }

    /** @brief Assignment from C-string. */
    basic_string &operator=(const char *s) { return assign(s, strlen(s)); }

    /** @brief Assignment from string_view. */
    basic_string &operator=(string_view sv)
    {
        return assign(sv.begin(), sv.size());
    }

    /** @brief Assigns from C-string with length. */
    basic_string &assign(const char *s, size_t len)
    {
        if (len <= SSO_CAPACITY)
        {
            if (!is_small())
                m_allocator.deallocate(large.data, large_capacity() + 1);
            set_small(len);
            memcpy(small, s, len);
            small[len] = '\0';
        }
        else
        {
            if (!is_small() && large_capacity() >= len)
                // Reuse existing buffer
                large.size = len;
            else
            {
                if (!is_small())
                    m_allocator.deallocate(large.data, large_capacity() + 1);

                allocate_large(len);
            }
            memcpy(large.data, s, len);
            large.data[len] = '\0';
        }
        return *this;
    }

    /** @brief Returns length of string. */
    constexpr size_t length() const { return size(); }

    /** @brief Returns size of string. */
    constexpr size_t size() const
    {
        if (is_small())
            return get_small_size();
        return large.size;
    }

    /**
     * @brief Manually sets the string length.
     * CAUTION: User must ensure buffer has enough capacity and
     * that valid data exists up to 'n'.
     * Useful after directly writing to data() via fread/memcpy.
     */
    void set_length(size_t n)
    {
        assert(n <= capacity() && "New length exceeds capacity");

        if (is_small())
        {
            small[n] = '\0';
            set_small_size(n);
        }
        else
        {
            large.data[n] = '\0';
            large.size    = n;
        }
    }

    constexpr const char *c_str() const
    {
        if (is_small())
            return small;
        return large.data;
    }

    constexpr char *data()
    {
        if (is_small())
            return small;
        return large.data;
    }

    constexpr const char *data() const { return c_str(); }

    /** @brief Checks if empty. */
    constexpr bool empty() const { return this->size() == 0; }

    /** @brief Append operator for C-string. */
    constexpr basic_string &operator+=(const char *s)
    {
        return append_raw(s, strlen(s));
    }

    /** @brief Append operator for char. */
    constexpr basic_string &operator+=(char c)
    {
        size_t clen      = size();
        size_t total_len = clen + 1;

        if (total_len <= SSO_CAPACITY)
        {
            if (!is_small())
            {
                // Optimization: stay large if we have space
                if (large_capacity() >= total_len)
                {
                    large.data[clen]      = c;
                    large.data[total_len] = '\0';
                    large.size            = total_len;
                    return *this;
                }
            }
            else
            {
                // Is small and fits in small
                small[clen]      = c;
                small[total_len] = '\0';
                set_small_size(total_len);
                return *this;
            }
        }

        return append_raw(&c, 1);
    }

    constexpr basic_string &operator+=(const basic_string &other)
    {
        return append_raw(other.c_str(), other.size());
    }

    constexpr basic_string &operator+=(const string_view &sv)
    {
        return append_raw(sv.data(), sv.size());
    }

    /** @brief Constructs from any type convertible to string_view (like
     * fixed_string). */
    template <
        typename T,
        typename std::enable_if<std::is_convertible<T, string_view>::value,
                                int>::type = 0>
    basic_string(const T &t) : basic_string(string_view(t))
    {
    }

    /** @brief Assignment from any type convertible to string_view. */
    template <
        typename T,
        typename std::enable_if<std::is_convertible<T, string_view>::value,
                                int>::type = 0>
    basic_string &operator=(const T &t)
    {
        return *this = string_view(t);
    }

    operator string_view() const { return string_view(data(), size()); }

    basic_string substr(size_t pos = 0, size_t count = npos) const
    {
        string_view sv = *this;
        return basic_string(sv.substr(pos, count));
    }

    size_t find(const char *s, size_t pos = 0) const
    {
        return string_view(*this).find(s, pos);
    }
    size_t find(char c, size_t pos = 0) const
    {
        return string_view(*this).find(c, pos);
    }
    size_t find(const basic_string &s, size_t pos = 0) const
    {
        return string_view(*this).find(s, pos);
    }

    size_t rfind(const char *s, size_t pos = npos) const
    {
        return string_view(*this).rfind(s, pos);
    }

    size_t rfind(char c, size_t pos = npos) const
    {
        return string_view(*this).rfind(c, pos);
    }

    size_t rfind(const basic_string &s, size_t pos = npos) const
    {
        return string_view(*this).rfind(s, pos);
    }

    bool starts_with(string_view sv) const
    {
        return string_view(*this).starts_with(sv);
    }

    bool starts_with(char c) const { return !empty() && front() == c; }

    bool ends_with(string_view sv) const
    {
        return string_view(*this).ends_with(sv);
    }

    bool ends_with(char c) const { return !empty() && back() == c; }

    /** @brief Returns an iterator to the beginning. */
    constexpr iterator begin()
    {
        if (is_small())
            return small;
        return large.data;
    }

    /** @brief Returns a const iterator to the beginning. */
    constexpr const_iterator begin() const
    {
        if (is_small())
            return small;
        return large.data;
    }

    /** @brief Returns an iterator to the end (one past the last character). */
    constexpr iterator end() { return begin() + size(); }

    /** @brief Returns a const iterator to the end. */
    constexpr const_iterator end() const { return begin() + size(); }

    /** @brief Returns a const iterator to the beginning. */
    constexpr const_iterator cbegin() const { return begin(); }

    /** @brief Returns a const iterator to the end. */
    constexpr const_iterator cend() const { return end(); }

    /** @brief Resizes the string. */
    void resize(size_t count, char ch = '\0')
    {
        size_t current_size = size();
        if (count < current_size)
        {
            if (is_small())
            {
                small[count] = '\0';
                set_small_size(count);
            }
            else
            {
                large.data[count] = '\0';
                large.size        = count;
            }
        }
        else if (count > current_size)
        {
            if (count > capacity())
                reserve(count);

            char *ptr = is_small() ? small : large.data;
            memset(ptr + current_size, ch, count - current_size);
            ptr[count] = '\0';

            if (is_small())
                set_small_size(count);
            else
                large.size = count;
        }
    }

    /** @brief Reserves capacity. */
    void reserve(size_t new_cap)
    {
        if (new_cap <= capacity())
            return;

        // Force transition to large
        char *new_data = static_cast<char *>(m_allocator.allocate(new_cap + 1));
        size_t current_size = size();
        memcpy(new_data, data(), current_size + 1); // +1 for null

        if (!is_small())
        {
            m_allocator.deallocate(large.data, large_capacity() + 1);
        }

        large.data = new_data;
        large.size = current_size;
        // Set capacity using helper to ensure endian-correct flags are 0
        set_large_capacity(new_cap << 1);
    }

    /** @brief Returns current capacity. */
    size_t capacity() const
    {
        return is_small() ? SSO_CAPACITY : large_capacity();
    }

    /** @brief Clears the string. */
    void clear()
    {
        if (is_small())
        {
            small[0] = '\0';
            set_small_size(0);
        }
        else
        {
            large.size    = 0;
            large.data[0] = '\0';
        }
    }

    /** @brief Swaps two strings. */
    friend void swap(basic_string &lhs, basic_string &rhs)
    {
        char temp[sizeof(basic_string)];
        memcpy(temp, &lhs, sizeof(basic_string));
        memcpy(&lhs, &rhs, sizeof(basic_string));
        memcpy(&rhs, temp, sizeof(basic_string));
    }

    /** @brief Access character at index without bounds checking. */
    constexpr char &operator[](size_t pos)
    {
        // Note: Using begin()[pos] unifies the SSO logic
        return begin()[pos];
    }

    /** @brief Access const character at index without bounds checking. */
    constexpr const char &operator[](size_t pos) const { return begin()[pos]; }

    /** @brief Appends string_view. */
    void append(const string_view &s) { append_raw(s.data(), s.size()); }

    /** @brief Prepends C-string. */
    basic_string &prepend(const char *s) { return prepend(s, strlen(s)); }

    /** @brief Prepends another string. */
    basic_string &prepend(const basic_string &str)
    {
        return prepend(str.c_str(), str.size());
    }

    /** @brief Prepends string_view. */
    basic_string &prepend(string_view sv)
    {
        return prepend(sv.data(), sv.size());
    }

    /** @brief Prepends raw chars. */
    basic_string &prepend(const char *s, size_t len)
    {
        size_t clen      = size();
        size_t total_len = clen + len;

        if (is_small())
        {
            if (total_len <= SSO_CAPACITY)
            {
                // Fits in small
                memmove(small + len, small, clen);
                memcpy(small, s, len);
                small[total_len] = '\0';
                set_small_size(total_len);
            }
            else
            {
                // Grow to Large
                size_t new_capacity = max(total_len << 1, size_t(64));
                new_capacity =
                    (new_capacity + 1) & ~size_t(1); // Ensure even for BE

                char *new_data =
                    static_cast<char *>(m_allocator.allocate(new_capacity + 1));

                memcpy(new_data, s, len);
                memcpy(new_data + len, small, clen);
                new_data[total_len] = '\0';

                large.data     = new_data;
                large.size     = total_len;
                large.capacity = new_capacity;
            }
        }
        else
        {
            // Already large
            size_t cap = large_capacity();
            if (total_len <= cap)
            {
                // Fits
                memmove(large.data + len, large.data, clen);
                memcpy(large.data, s, len);
                large.data[total_len] = '\0';
                large.size            = total_len;
            }
            else
            {
                // Realloc
                size_t new_capacity = max(total_len * 2, cap * 2);
                new_capacity        = (new_capacity + 1) & ~size_t(1);

                char *new_data =
                    static_cast<char *>(m_allocator.allocate(new_capacity + 1));

                memcpy(new_data, s, len);
                memcpy(new_data + len, large.data, clen);
                new_data[total_len] = '\0';

                m_allocator.deallocate(large.data, cap + 1);

                large.data     = new_data;
                large.size     = total_len;
                large.capacity = new_capacity;
            }
        }
        return *this;
    }

private:
    union
    {
        struct
        {
            char *data;
            size_t size;
            size_t capacity;
        } large;
        char small[sizeof(large)];
    };
    Allocator m_allocator;

    // Use the last byte of the buffer for the SSO flag/size.
    // This aligns with MSB of capacity on LE, and LSB of capacity on BE.
    static constexpr size_t SSO_CAPACITY  = sizeof(large) - 2;
    static constexpr size_t LAST_BYTE_IDX = sizeof(large) - 1;

    // Constants for flags
    // LE: Use MSB (bit 7) of the last byte.
    // BE: Use LSB (bit 0) of the last byte.
    static constexpr unsigned char LE_SMALL_FLAG = 0x01;
    static constexpr unsigned char BE_SMALL_FLAG = 0x80;

    constexpr bool is_small() const
    {
        if constexpr (is_little_endian)
            // Check high bit of the last byte (MSB of capacity)
            return (small[LAST_BYTE_IDX] & LE_SMALL_FLAG) != 0;
        else
            // Check low bit of the last byte (LSB of capacity)
            return (small[LAST_BYTE_IDX] & BE_SMALL_FLAG) != 0;
    }

    void set_small(size_t s)
    {
        if constexpr (is_little_endian)
        {
            // Set high bit to mark as small. Store size in lower bits.
            small[LAST_BYTE_IDX] = static_cast<char>(LE_SMALL_FLAG | s);
        }
        else
        {
            // Set low bit to mark as small. Store size in higher bits.
            small[LAST_BYTE_IDX] = static_cast<char>((s << 1) | BE_SMALL_FLAG);
        }
        small[0] = '\0'; // Safety null for empty strings
    }

    void set_small_size(size_t s)
    {
        assert(s <= SSO_CAPACITY);
        if constexpr (is_little_endian)
            small[LAST_BYTE_IDX] = static_cast<char>(LE_SMALL_FLAG | (s << 1));
        else
            small[LAST_BYTE_IDX] = static_cast<char>(BE_SMALL_FLAG | s);
    }

    constexpr size_t get_small_size() const
    {
        if constexpr (is_little_endian)
            return small[LAST_BYTE_IDX] >> 1;
        else
            return small[LAST_BYTE_IDX] & (~BE_SMALL_FLAG);
    }

    void set_large_capacity(size_t raw_cap_bytes)
    {
        if constexpr (is_little_endian)
        {
            size_t cap_even = raw_cap_bytes << 1;
            large.capacity  = cap_even;
        }
        else
            large.capacity =
                raw_cap_bytes & ~(size_t(1) << (sizeof(size_t) * 8 - 1));
    }

    constexpr size_t large_capacity() const
    {
        if constexpr (is_little_endian)
            return large.capacity >> 1;
        return large.capacity;
    }

    void allocate_large(size_t req_size, size_t req_cap = 0)
    {
        size_t cap = req_cap > req_size
                         ? req_cap
                         : max(req_size, size_t(SSO_CAPACITY * 2));
        large.data = static_cast<char *>(m_allocator.allocate(cap + 1));
        large.size = req_size;
        set_large_capacity(cap);
    }

    constexpr basic_string &append_raw(const char *s, size_t len)
    {
        size_t clen      = size();
        size_t total_len = clen + len;

        if (total_len <= SSO_CAPACITY)
        {
            if (!is_small())
            {
                char *old_data = large.data;
                size_t old_cap = large.capacity;
                set_small(clen);
                memcpy(small, old_data, clen);
                m_allocator.deallocate(old_data, (old_cap >> 1) + 1);
            }

            memcpy(small + clen, s, len);
            small[total_len] = '\0';
            set_small_size(total_len);
        }
        else
        {
            if (is_small())
            {
                char buffer[sizeof(large)];
                memcpy(buffer, small, clen);

                size_t new_capacity = max(total_len << 1, size_t(64));
                allocate_large(total_len, new_capacity);

                memcpy(large.data, buffer, clen);
            }
            else
            {
                size_t current_real_cap = large_capacity();
                if (total_len > current_real_cap)
                {
                    size_t new_capacity = max(total_len, current_real_cap * 2);

                    char *new_ptr = static_cast<char *>(
                        m_allocator.reallocate(large.data, new_capacity + 1));
                    large.data = new_ptr;
                    set_large_capacity(new_capacity);
                }
            }

            memcpy(large.data + clen, s, len);
            large.data[total_len] = '\0';
            large.size            = total_len;
        }
        return *this;
    }
};

template <class AllocatorL, class AllocatorR>
constexpr bool operator==(const basic_string<AllocatorL> &lhs,
                          const basic_string<AllocatorR> &rhs)
{
    if (lhs.size() != rhs.size())
        return false;
    return memcmp(lhs.c_str(), rhs.c_str(), lhs.size()) == 0;
}
template <class AllocatorL, class AllocatorR>
constexpr bool operator!=(const basic_string<AllocatorL> &lhs,
                          const basic_string<AllocatorR> &rhs)
{
    return !(lhs == rhs);
}

template <class Allocator>
constexpr bool operator==(const basic_string<Allocator> &lhs, const char *rstr)
{
    return strcmp(lhs.c_str(), rstr);
}
template <class Allocator>
constexpr bool operator!=(const basic_string<Allocator> &lhs, const char *rstr)
{
    return !(lhs == rstr);
}

template <class Allocator>
constexpr bool operator==(const char *lstr, const basic_string<Allocator> &rhs)
{
    return strcmp(lstr, rhs.c_str());
}

template <class Allocator>
constexpr bool operator!=(const char *lstr, const basic_string<Allocator> &rhs)
{
    return !(lstr == rhs);
}

// string + char
template <class Allocator>
constexpr basic_string<Allocator> operator+(const basic_string<Allocator> &lhs,
                                            char rhs)
{
    basic_string<Allocator> str = lhs;
    str += rhs;
    return str;
}

// char + string
template <class Allocator>
constexpr basic_string<Allocator> operator+(char lhs,
                                            const basic_string<Allocator> &rhs)
{
    // Need to construct a string from a single char first
    // Since we don't have a (char) constructor, we make a small temp buffer
    char tmp[2] = {lhs, '\0'};
    basic_string<Allocator> str(tmp);
    str += rhs;
    return str;
}

template <typename Allocator>
constexpr auto begin(basic_string<Allocator> &str) ->
    typename basic_string<Allocator>::iterator
{
    return str.begin();
}

template <typename Allocator>
constexpr auto end(basic_string<Allocator> &str) ->
    typename basic_string<Allocator>::iterator
{
    return str.end();
}

template <typename Allocator>
constexpr auto begin(const basic_string<Allocator> &str) ->
    typename basic_string<Allocator>::const_iterator
{
    return str.begin();
}

template <typename Allocator>
constexpr auto end(const basic_string<Allocator> &str) ->
    typename basic_string<Allocator>::const_iterator
{
    return str.end();
}

// Support for string_view as well
constexpr auto begin(string_view sv) -> string_view::iterator
{
    return sv.begin();
}

constexpr auto end(string_view sv) -> string_view::iterator { return sv.end(); }

template <typename Allocator> struct hash<basic_string<Allocator>>
{
    size_t operator()(const basic_string<Allocator> &str) const
    {
        size_t length = str.length();
        uint32_t hash = 2166136261u;
        for (size_t i = 0; i < length; i++)
        {
            hash ^= str[i];
            hash *= 16777619;
        }
        return hash;
    }
};

template <> struct hash<string_view>
{
    size_t operator()(const string_view &str) const
    {
        size_t length = str.length();
        uint32_t hash = 2166136261u;
        for (size_t i = 0; i < length; i++)
        {
            hash ^= str[i];
            hash *= 16777619;
        }
        return hash;
    }
};

using string = basic_string<allocator<char>>;

} // namespace zabato