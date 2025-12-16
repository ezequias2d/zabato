#pragma once

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <type_traits>

namespace zabato
{
// Dynamic extent constant
inline constexpr size_t dynamic_extent = -1;

/**
 * @class span
 * @brief A non-owning view over a contiguous sequence of objects.
 */
template <typename T> class span
{
public:
    using element_type     = T;
    using value_type       = typename std::remove_cv<T>::type;
    using size_type        = size_t;
    using difference_type  = ptrdiff_t;
    using pointer          = T *;
    using const_pointer    = const T *;
    using reference        = T &;
    using const_reference  = const T &;
    using iterator         = T *;
    using const_iterator   = const T *;
    using reverse_iterator = T *;

#pragma region Constructors

    /** @brief Default constructor (empty span). */
    constexpr span() noexcept : m_data(nullptr), m_size(0) {}

    /** @brief Construct from pointer and size. */
    constexpr span(pointer ptr, size_type count) : m_data(ptr), m_size(count) {}

    /** @brief Construct from begin and end pointers. */
    constexpr span(pointer first, pointer last)
        : m_data(first), m_size(static_cast<size_t>(last - first))
    {
    }

    /** @brief Construct from a C-style array. */
    template <size_t N>
    constexpr span(element_type (&arr)[N]) noexcept : m_data(arr), m_size(N)
    {
    }

    /** * @brief Construct from a generic container (like zabato::vector).
     * check ensures Container has data() and size().
     */
    template <
        typename Container,
        typename = std::enable_if_t<
            !std::is_same_v<std::decay_t<Container>, span> &&
            std::is_convertible_v<decltype(std::declval<Container>().data()),
                                  pointer>>>
    constexpr span(Container &cont) : m_data(cont.data()), m_size(cont.size())
    {
    }

    /** * @brief Construct from a const container (for span<const T>).
     */
    template <typename Container,
              typename = std::enable_if_t<
                  !std::is_same_v<std::decay_t<Container>, span> &&
                  std::is_const_v<element_type> &&
                  std::is_convertible_v<
                      decltype(std::declval<const Container>().data()),
                      pointer>>>
    constexpr span(const Container &cont)
        : m_data(cont.data()), m_size(cont.size())
    {
    }

    /** @brief Copy constructor. */
    constexpr span(const span &other) noexcept = default;

    /** @brief Assignment operator. */
    constexpr span &operator=(const span &other) noexcept = default;

#pragma endregion Constructors

#pragma region Accessors
    constexpr pointer data() const noexcept { return m_data; }
    constexpr size_type size() const noexcept { return m_size; }
    constexpr size_type size_bytes() const noexcept
    {
        return m_size * sizeof(T);
    }
    constexpr bool empty() const noexcept { return m_size == 0; }

    constexpr reference operator[](size_type idx) const
    {
        assert(idx < m_size);
        return m_data[idx];
    }

    constexpr reference front() const
    {
        assert(m_size > 0);
        return m_data[0];
    }

    constexpr reference back() const
    {
        assert(m_size > 0);
        return m_data[m_size - 1];
    }
#pragma endregion Accessors

#pragma region Iterators
    constexpr iterator begin() const noexcept { return m_data; }
    constexpr iterator end() const noexcept { return m_data + m_size; }
    constexpr const_iterator cbegin() const noexcept { return m_data; }
    constexpr const_iterator cend() const noexcept { return m_data + m_size; }
#pragma endregion Iterators

#pragma region Subviews
    /** @brief Returns a subspan of the first Count elements. */
    constexpr span first(size_type count) const
    {
        assert(count <= m_size);
        return span(m_data, count);
    }

    /** @brief Returns a subspan of the last Count elements. */
    constexpr span last(size_type count) const
    {
        assert(count <= m_size);
        return span(m_data + (m_size - count), count);
    }

    /** @brief Returns a subspan starting at Offset. */
    constexpr span subspan(size_type offset,
                           size_type count = dynamic_extent) const
    {
        assert(offset <= m_size);
        size_type available    = m_size - offset;
        size_type actual_count = (count == dynamic_extent) ? available : count;
        assert(actual_count <= available);

        return span(m_data + offset, actual_count);
    }
#pragma endregion Subviews

private:
    pointer m_data;
    size_type m_size;
};

#pragma region Type Aliases

/** @brief A read-write view of raw bytes. */
using buffer = span<uint8_t>;

/** @brief A read-only view of raw bytes. */
using const_buffer = span<const uint8_t>;

#pragma endregion Type Aliases

#pragma region Helper Functions

namespace internal
{
template <typename T> struct is_span_impl : std::false_type
{
};
template <typename T> struct is_span_impl<span<T>> : std::true_type
{
};
} // namespace internal

/**
 * @brief Reinterprets the span as a read-only byte span.
 * Useful for writing objects to files.
 */
template <typename T> buffer as_writable_bytes(span<T> s) noexcept
{
    static_assert(!std::is_const_v<T>,
                  "Cannot convert const span to writable bytes");
    return buffer(reinterpret_cast<uint8_t *>(s.data()), s.size_bytes());
}

/**
 * @brief Creates a writable byte view of a single object (struct/primitive).
 * Enables usage like: file->read(as_writable_bytes(my_struct).data(),
 * sizeof(my_struct)); or directly with span-aware APIs.
 */
template <typename T>
typename std::enable_if_t<!internal::is_span_impl<std::decay_t<T>>::value &&
                              !std::is_pointer_v<std::decay_t<T>> &&
                              !std::is_const_v<T>,
                          buffer>
as_writable_bytes(T &obj) noexcept
{
    static_assert(std::is_trivially_copyable_v<T>,
                  "Object must be trivially copyable to be treated as bytes");
    return buffer(reinterpret_cast<uint8_t *>(&obj), sizeof(T));
}

/**
 * @brief Reinterprets the span as a read-only byte span.
 */
template <typename T> const_buffer as_bytes(span<T> s) noexcept
{
    return const_buffer(reinterpret_cast<const uint8_t *>(s.data()),
                        s.size_bytes());
}

/**
* @brief Creates a read-only byte view of a single object
 * (struct/primitive). Enables usage like:
 * file->write(as_bytes(my_struct).data(), sizeof(my_struct));
 */
template <typename T>
typename std::enable_if_t<!internal::is_span_impl<std::decay_t<T>>::value &&
                              !std::is_pointer_v<std::decay_t<T>>,
                          const_buffer>
as_bytes(const T &obj) noexcept
{
    static_assert(std::is_trivially_copyable_v<T>,
                  "Object must be trivially copyable to be treated as bytes");
    return const_buffer(reinterpret_cast<const uint8_t *>(&obj), sizeof(T));
}

#pragma endregion Helper Functions
} // namespace zabato