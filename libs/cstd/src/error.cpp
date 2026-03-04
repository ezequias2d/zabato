#include <zabato/error.hpp>

namespace zabato
{
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
constexpr error_code error_code::not_implemented{-24};
constexpr error_code error_code::failed_to_require_zshader{-25};
constexpr error_code error_code::failed_to_load_zshader{-26};
constexpr error_code error_code::no_script_system{-27};
constexpr error_code error_code::not_supported{-28};
constexpr error_code error_code::unknown{-128};

const char *get_error_message(error_code error)
{
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
            return "Failed to decompress data with berg for chunk ID "
                   "'%s'(0x%x), "
                   "berg code: %d";
        case error_code::not_implemented:
            return "Not implemented";
        case error_code::failed_to_require_zshader:
            return "Failed to require zshader: %s";
        case error_code::failed_to_load_zshader:
            return "Failed to load zshader: %s";
        case error_code::no_script_system:
            return "No script system";
        case error_code::not_supported:
            return "Operation not supported";
        case error_code::unknown:
            return "Unknown error";
        default:
            return "Unknown error code";
        }
    }
}
} // namespace zabato