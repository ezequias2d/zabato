#ifndef ZABATO_BERG_H
#define ZABATO_BERG_H

#include <stdbool.h>
#include <stddef.h>

#define BERG_DEFAULT_WINDOW_SIZE ((size_t)4096)
#define BERG_DEFAULT_LOOKAHEAD_SIZE ((size_t)256)

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief Error codes for C API
 */
typedef enum berg_error_t
{
    BERG_OK_NO_OUTPUT               = 1,
    BERG_OK                         = 0,
    BERG_ERROR_INVALID_PARAM        = -1,
    BERG_ERROR_BUFFER_TOO_SMALL     = -2,
    BERG_ERROR_COMPRESSION_FAILED   = -3,
    BERG_ERROR_DECOMPRESSION_FAILED = -4,
    BERG_ERROR_CORRUPT_DATA         = -5,
    BERG_ERROR_CALLBACK_FAILED      = -6
} berg_error_t;

/**
 * @struct berg_config
 * @brief Configuration parameters for Berg compression
 */
typedef struct berg_config
{
    size_t lookahead_size; ///< Size of the lookahead buffer
} berg_config;

/**
 * @brief Callback function for streaming output
 * @param buffer Pointer to data to write
 * @param size Number of bytes to write
 * @param user_data User-provided context pointer
 * @return 0 on success, non-zero on error
 */
typedef berg_error_t (*berg_write_callback_t)(const void *buffer,
                                              size_t size,
                                              void *user_data);

/**
 * @brief Get default configuration
 * @return Default berg configuration
 */
berg_config berg_get_default_config(void);

/**
 * @brief Estimate maximum compressed size for given input size
 * @param input_size Size of input data
 * @return Maximum possible compressed size (worst case)
 */
size_t berg_estimate_max_compressed_size(size_t input_size);

/**
 * @brief Compress data using preallocated output buffer
 * @param input Pointer to input data
 * @param input_size Size of input data in bytes
 * @param output Pointer to preallocated output buffer
 * @param output_capacity Size of output buffer in bytes
 * @param compressed_size Pointer to receive actual compressed size
 * @param config Compression configuration (NULL for default)
 * @return BERG_OK on success, error code on failure
 */
berg_error_t berg_compress(const void *input,
                           size_t input_size,
                           void *output,
                           size_t output_capacity,
                           size_t *compressed_size,
                           const berg_config *config);

/**
 * @brief Decompress data using preallocated output buffer
 * @param compressed Pointer to compressed data
 * @param compressed_size Size of compressed data in bytes
 * @param output Pointer to preallocated output buffer
 * @param output_capacity Size of output buffer in bytes
 * @param decompressed_size Pointer to receive actual decompressed size
 * @return BERG_OK on success, error code on failure
 */
berg_error_t berg_decompress(const void *compressed,
                             size_t compressed_size,
                             void *output,
                             size_t output_capacity,
                             size_t *decompressed_size);

/**
 * @brief Raw compress data (no header) using preallocated output buffer
 * @param input Pointer to input data
 * @param input_size Size of input data in bytes
 * @param output Pointer to preallocated output buffer
 * @param output_capacity Size of output buffer in bytes
 * @param compressed_size Pointer to receive actual compressed size
 * @param config Compression configuration (NULL for default)
 * @return BERG_OK on success, error code on failure
 */
berg_error_t berg_compress_raw(const void *input,
                               size_t input_size,
                               void *output,
                               size_t output_capacity,
                               size_t *compressed_size,
                               const berg_config *config);

/**
 * @brief Raw decompress data (no header) using preallocated output buffer
 * @param compressed Pointer to compressed data
 * @param compressed_size Size of compressed data in bytes
 * @param output Pointer to preallocated output buffer
 * @param output_capacity Size of output buffer in bytes
 * @param original_size Expected size of decompressed data
 * @param decompressed_size Pointer to receive actual decompressed size
 * @return BERG_OK on success, error code on failure
 */
berg_error_t berg_decompress_raw(const void *compressed,
                                 size_t compressed_size,
                                 void *output,
                                 size_t output_capacity,
                                 size_t original_size,
                                 size_t *decompressed_size);

/**
 * @brief Raw compress data with streaming output via callback
 * @param input Pointer to input data
 * @param input_size Size of input data in bytes
 * @param callback Function to call when buffer is ready to write
 * @param user_data User context pointer passed to callback
 * @param buffer Pointer to external buffer for compressed data
 * @param buffer_size Size of external buffer in bytes
 * @param config Compression configuration (NULL for default)
 * @return BERG_OK on success, error code on failure
 */
berg_error_t berg_compress_raw_stream(const void *input,
                                      size_t input_size,
                                      berg_write_callback_t callback,
                                      void *user_data,
                                      void *buffer,
                                      size_t buffer_size,
                                      const berg_config *config);

/**
 * @brief Raw decompress data with streaming output via callback
 * @param compressed Pointer to compressed data
 * @param compressed_size Size of compressed data in bytes
 * @param original_size Expected size of decompressed data
 * @param callback Function to call when buffer has data to write
 * @param user_data User context pointer passed to callback
 * @param buffer Pointer to external buffer for decompressed data
 * @param buffer_size Size of external buffer in bytes
 * @return BERG_OK on success, error code on failure
 */
berg_error_t berg_decompress_raw_stream(const void *compressed,
                                        size_t compressed_size,
                                        size_t original_size,
                                        berg_write_callback_t callback,
                                        void *user_data,
                                        void *buffer,
                                        size_t buffer_size);

berg_error_t berg_compress_stream(const void *input,
                                  size_t input_size,
                                  berg_write_callback_t callback,
                                  void *user_data,
                                  void *buffer,
                                  size_t buffer_size,
                                  const berg_config *config);

berg_error_t berg_decompress_stream(const void *compressed,
                                    size_t compressed_size,
                                    berg_write_callback_t callback,
                                    void *user_data,
                                    void *buffer,
                                    size_t buffer_size);

#ifdef __cplusplus
}
#endif

#endif // ZABATO_BERG_H