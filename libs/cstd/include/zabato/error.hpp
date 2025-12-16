#pragma once

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>

namespace zabato
{
/**
 * @namespace cc
 * @brief ANSI color codes for console output formatting.
 */
namespace cc
{
constexpr const char *reset  = "\x1B[0m";
constexpr const char *red    = "\x1B[0;31m";
constexpr const char *green  = "\x1B[0;32m";
constexpr const char *yellow = "\x1B[0;33m";
constexpr const char *blue   = "\x1B[0;34m";
constexpr const char *cyan   = "\x1B[0;36m";
} // namespace cc

#pragma region Enums and Result Type

/**
 * @enum report_type
 * @brief Categorizes the type of log or report message.
 */
enum class report_type
{
    /** @brief Critical error that may stop execution or indicate a bug. */
    error,
    /** @brief General informational message. */
    info,
    /** @brief Warning about potential issues that are not critical. */
    warning,
    /** @brief Detailed tracing information for debugging. */
    trace,
};

/**
 * @class error_code
 * @brief A type-safe class for error codes that is contextually convertible to
 * bool.
 *
 * An error_code represents the success or failure of an operation. It is
 * considered 'true' (failure) if its internal value is negative, and 'false'
 * (success) otherwise.
 *
 * Use static members like `error_code::ok` for success or
 * `error_code::operation` for generic failures to access standardized codes.
 */
class error_code
{
public:
    constexpr bool operator==(const error_code &other) const
    {
        return m_value == other.m_value;
    }
    constexpr bool operator!=(const error_code &other) const
    {
        return m_value != other.m_value;
    }

    /**
     * @brief Contextual conversion to bool.
     * @return true if this error_code represents a failure (value < 0), false
     * otherwise.
     */
    explicit constexpr operator bool() const { return m_value < 0; }

    /**
     * @brief Conversion to the underlying integer error value.
     */
    constexpr operator int32_t() const { return m_value; }

    /// @brief No error (0).
    static const error_code ok;
    /// @brief A required callback function was null or missing (-1).
    static const error_code missing_callback;
    /// @brief The operation precondition was not met or failed (-2).
    static const error_code operation;
    /// @brief An invalid value was provided (-3).
    static const error_code value;
    /// @brief A handle or pointer was null when it shouldn't be (-4).
    static const error_code null_handle;
    /// @brief The data chunk type is not supported (-5).
    static const error_code unsupported_chunk;
    /// @brief The expected data chunk was not found/reached (-6).
    static const error_code chunk_not_reached;
    /// @brief The chunk data appears corrupted or unreadable (-7).
    static const error_code chunk_broken;
    /// @brief General read failure (-8).
    static const error_code unable_to_read;
    /// @brief General write failure (-9).
    static const error_code unable_to_write;
    /// @brief Memory allocation failed (-10).
    static const error_code unable_to_alloc;
    /// @brief A matching item could not be found (-11).
    static const error_code unable_to_match;
    /// @brief The collision shape or type is unsupported (-12).
    static const error_code unsupported_collision;
    /// @brief Text system initialization failed (-13).
    static const error_code text_init_failed;
    /// @brief Entity initialization failed (-14).
    static const error_code entity_init_failed;
    /// @brief Loading Lua code failed (-15).
    static const error_code lua_load_code_failed;
    /// @brief Execution of a Lua event callback failed (-16).
    static const error_code lua_event_callback_failed;
    /// @brief Execution of a Lua update callback failed (-17).
    static const error_code lua_update_callback_failed;
    /// @brief Reached the end of the data stream (-18).
    static const error_code end_of_stream;
    /// @brief Berg compression failed (-19).
    static const error_code fail_to_compress_berg;
    /// @brief Berg decompression failed (-20).
    static const error_code fail_to_decompress_berg;
    /// @brief The file path is invalid (-21).
    static const error_code invalid_path;
    /// @brief The file could not be found (-22).
    static const error_code file_not_found;
    /// @brief The path could not be found (-23).
    static const error_code path_not_found;
    /// @brief An unknown error occurred (-128).
    static const error_code unknown;

private:
    // Private constructor to control instantiation from raw integers
    constexpr error_code(int64_t value) : m_value(value) {}
    int64_t m_value;
};

constexpr error_code error_code::ok = {0};
constexpr error_code error_code::missing_callback{-1};
constexpr error_code error_code::operation{-2};
constexpr error_code error_code::value{-3};
constexpr error_code error_code::null_handle{-4};
constexpr error_code error_code::unsupported_chunk{-5};
constexpr error_code error_code::chunk_not_reached{-6};
constexpr error_code error_code::chunk_broken{-7};
constexpr error_code error_code::unable_to_read{-8};
constexpr error_code error_code::unable_to_write{-9};
constexpr error_code error_code::unable_to_alloc{-10};
constexpr error_code error_code::unable_to_match{-11};
constexpr error_code error_code::unsupported_collision{-12};
constexpr error_code error_code::text_init_failed{-13};
constexpr error_code error_code::entity_init_failed{-14};
constexpr error_code error_code::lua_load_code_failed{-15};
constexpr error_code error_code::lua_event_callback_failed{-16};
constexpr error_code error_code::lua_update_callback_failed{-17};
constexpr error_code error_code::end_of_stream{-18};
constexpr error_code error_code::fail_to_compress_berg{-19};
constexpr error_code error_code::fail_to_decompress_berg{-20};
constexpr error_code error_code::invalid_path{-21};
constexpr error_code error_code::file_not_found{-22};
constexpr error_code error_code::path_not_found{-23};
constexpr error_code error_code::unknown{-128};

/**
 * @brief A helper function to check if an error_code represents a failure.
 * @param ec The error code to check.
 * @return True if the code is a negative value, false otherwise.
 */
inline constexpr bool is_error(error_code ec)
{
    return static_cast<int64_t>(ec) < 0;
}

/**
 * @struct result
 * @brief A container that holds either a value of type T or an error_code.
 *
 * This struct is used to return values from functions that can fail.
 * It provides convenient operators to check for errors and access the value.
 *
 * @tparam T The type of the success value.
 */
template <typename T> struct result
{
    error_code error;
    T value;

    result() : error(error_code::ok), value{} {}
    result(const T &val) : error(error_code::ok), value(val) {}
    result(T &&val) : error(error_code::ok), value(static_cast<T &&>(val)) {}
    result(error_code err) : error(err), value{} {}

    /** @brief Checks if the result contains an error. */
    bool has_error() const { return is_error(error); }
    /**
     * @brief Contextual conversion to bool. Returns true if NO error (success).
     */
    explicit operator bool() const { return !has_error(); }

    constexpr operator const T &() const { return value; }
    constexpr operator T &() { return value; }

    constexpr const T &operator*() const { return value; }
    constexpr T &operator*() { return value; }

    constexpr const T *operator->() const { return &value; }
    constexpr T *operator->() { return &value; }
};

/**
 * @struct result<void>
 * @brief Specialization of result for void types (operations that return
 * nothing but can fail).
 */
template <> struct result<void>
{
    error_code error;

    result() : error(error_code::ok) {}
    result(error_code err) : error(err) {}

    bool has_error() const { return bool(error); }
    explicit operator bool() const { return !bool(error); }
};

/**
 * @brief Retrieves a human-readable error message for a given error code.
 * @param error The error code.
 * @return A constant string describing the error.
 */
inline const char *get_error_message(error_code error)
{
    switch (error)
    {
    case error_code::ok:
        return "No error";
    case error_code::missing_callback:
        return "Missing callback for '%s'";
    case error_code::operation:
        return "Precondition cannot be satisfied, operation%s cannot be "
               "performed correctly.%s";
    case error_code::value:
        return "The value ('%s') does not satisfy the precondition";
    case error_code::null_handle:
        return "The handler is null";
    case error_code::unsupported_chunk:
        return "The chunk is not supported";
    case error_code::chunk_not_reached:
        return "Could not find chunk with ID '%s'(0x%x)";
    case error_code::chunk_broken:
        return "Could not read data from the chunk ID '%s'(0x%x), possibly "
               "corruption";
    case error_code::unable_to_read:
        return "Unable to read";
    case error_code::unable_to_write:
        return "Unable to write";
    case error_code::unable_to_alloc:
        return "Unable to alloc";
    case error_code::unable_to_match:
        return "Unable to match";
    case error_code::unsupported_collision:
        return "Unsupported collision type";
    case error_code::text_init_failed:
        return "Failed to open text init file: %s";
    case error_code::entity_init_failed:
        return "Failed to open entity init file: %s, animated: %s, id: %d";
    case error_code::lua_load_code_failed:
        return "Failed to load Lua code: %s";
    case error_code::lua_event_callback_failed:
        return "Lua failed to call event callback: %s";
    case error_code::lua_update_callback_failed:
        return "Lua failed to call update callback: %s";
    case error_code::end_of_stream:
        return "At the end of the stream";
    case error_code::fail_to_compress_berg:
        return "Failed to compress data with berg for chunk ID '%s'(0x%x), "
               "berg code: %d";
    case error_code::fail_to_decompress_berg:
        return "Failed to decompress data with berg for chunk ID '%s'(0x%x), "
               "berg code: %d";
    case error_code::unknown:
        return "Unknown error";
    default:
        return "Unknown error code";
    }
}

#pragma endregion

/**
 * @brief Prints a formatted report message to stdout.
 * @param type The severity/type of the message.
 * @param format The format string (printf style).
 * @param args The variable argument list.
 */
inline void vreport(report_type type, const char *format, va_list args)
{
    switch (type)
    {
    case report_type::error:
        printf("%sERR: %s", cc::red, cc::reset);
        break;
    case report_type::warning:
        printf("%sWAR: %s", cc::cyan, cc::reset);
        break;
    case report_type::info:
        printf("%sINF: %s", cc::blue, cc::reset);
        break;
    case report_type::trace:
        printf("%sTRA: %s", cc::yellow, cc::reset);
        break;
    default:
        printf("UNK:");
        break;
    }
    vprintf(format, args);
    printf("\n");
}

/**
 * @brief Prints a formatted report message to stdout.
 * @param type The severity/type of the message.
 * @param format The format string (printf style).
 * @param ... Variable arguments for the format string.
 */
inline void report(report_type type, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vreport(type, format, args);
    va_end(args);
}

/**
 * @brief Reports an error message and returns the error code.
 *
 * This is a helper for logging an error and returning it in one step,
 * which is a common pattern in error handling.
 *
 * @param error The error code to return.
 * @param ... Variable arguments for the error message format string (retrieved
 * via get_error_message).
 * @return The passed error code.
 */
inline error_code report_error(error_code error, ...)
{
    va_list args;
    va_start(args, error);
    vreport(report_type::error, get_error_message(error), args);
    va_end(args);
    return error;
}

static uint64_t g_debug_stack_depth = 0;
static bool g_debug_trace           = false;

/**
 * @class function_tracer
 * @brief An RAII-based tracer for logging function entry and exit.
 *
 * Creates a log message on construction and destruction, automatically managing
 * the global debug stack depth for indentation. Use the TRACE_FUNCTION` macro
 * to easily trace the current function.
 */
class function_tracer
{
public:
    explicit function_tracer(const char *name) : m_name(name)
    {
        if (g_debug_trace)
        {
            report(report_type::trace,
                   "Enter %s [Depth %llu]",
                   m_name,
                   g_debug_stack_depth);
        }
        g_debug_stack_depth++;
    }

    ~function_tracer()
    {
        g_debug_stack_depth--;

        if (g_debug_trace)
        {
            report(report_type::trace,
                   "Exit %s [Depth %llu]",
                   m_name,
                   g_debug_stack_depth);
        }
    }

private:
    const char *m_name;
};

/** @brief Macro to create a function_tracer for the current function scope. */
#define TRACE_FUNCTION function_tracer tracer(__func__);

/** @brief Accessor for structured bindings on `result<T>` (index 0: error). */
template <size_t I, typename T> auto &get(result<T> &res)
{
    static_assert(I < 2, "Index out of bounds for result");
    if constexpr (I == 0)
        return res.error;
    else if constexpr (I == 1)
        return res.value;
}

/**
 * @brief Const accessor for structured bindings on `result<T>` (index 0:
 * error).
 */
template <size_t I, typename T> const auto &get(const result<T> &res)
{
    static_assert(I < 2, "Index out of bounds for result");
    if constexpr (I == 0)
        return res.error;
    else if constexpr (I == 1)
        return res.value;
}

} // namespace zabato