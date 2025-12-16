#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zabato/berg.h>

// TODO: support others operation system
#include <unistd.h>

typedef struct
{
    uint8_t *data;
    size_t size;
    size_t capacity;
} ByteBuffer;

void byte_buffer_init(ByteBuffer *buf)
{
    buf->data     = NULL;
    buf->size     = 0;
    buf->capacity = 0;
}
void byte_buffer_free(ByteBuffer *buf)
{
    if (buf->data)
    {
        free(buf->data);
    }
    byte_buffer_init(buf);
}

bool byte_buffer_reserve(ByteBuffer *buf, size_t min_capacity)
{
    if (buf->capacity < min_capacity)
    {
        size_t new_capacity = buf->capacity == 0 ? 8192 : buf->capacity;
        while (new_capacity < min_capacity)
        {
            new_capacity *= 2;
        }
        uint8_t *new_data = (uint8_t *)realloc(buf->data, new_capacity);
        if (!new_data)
        {
            return false; // Allocation failed
        }
        buf->data     = new_data;
        buf->capacity = new_capacity;
    }
    return true;
}

bool byte_buffer_append(ByteBuffer *buf, const uint8_t *data, size_t size)
{
    if (!byte_buffer_reserve(buf, buf->size + size))
    {
        return false;
    }
    memcpy(buf->data + buf->size, data, size);
    buf->size += size;
    return true;
}

bool byte_buffer_resize(ByteBuffer *buf, size_t new_size)
{
    if (!byte_buffer_reserve(buf, new_size))
    {
        return false;
    }
    buf->size = new_size;
    return true;
}

int read_input_to_buffer(const char *filename, ByteBuffer *buffer)
{
    if (strcmp(filename, "-") == 0) // Read from stdin
    {
        const size_t chunk_size = 8192;
        uint8_t read_buf[chunk_size];
        while (true)
        {
            size_t bytes_read = fread(read_buf, 1, chunk_size, stdin);
            if (bytes_read > 0)
            {
                if (!byte_buffer_append(buffer, read_buf, bytes_read))
                {
                    fprintf(
                        stderr,
                        "Error: Failed to allocate memory for stdin data\n");
                    return -1;
                }
            }

            if (bytes_read < chunk_size)
            {
                if (ferror(stdin))
                {
                    fprintf(stderr, "Error: Failed to read from stdin\n");
                    return -1;
                }
                break; // End of file
            }
        }
    }
    else // Read from a file
    {
        FILE *file = fopen(filename, "rb");
        if (!file)
        {
            fprintf(stderr,
                    "Error: Could not open file '%s' for reading\n",
                    filename);
            return -1;
        }

        fseek(file, 0, SEEK_END);
        long file_size = ftell(file);
        fseek(file, 0, SEEK_SET);

        if (file_size <= 0)
        {
            fclose(file);
            if (file_size < 0)
            {
                fprintf(
                    stderr, "Error: Invalid file size for '%s'\n", filename);
                return -1;
            }
            // Reading a 0-byte file is not an error, just return success.
            return 0;
        }

        if (!byte_buffer_resize(buffer, (size_t)file_size))
        {
            fclose(file);
            fprintf(stderr,
                    "Error: Failed to allocate memory for file '%s'\n",
                    filename);
            return -1;
        }

        size_t bytes_read = fread(buffer->data, 1, (size_t)file_size, file);
        fclose(file);

        if (bytes_read != (size_t)file_size)
        {
            fprintf(
                stderr, "Error: Could not read complete file '%s'\n", filename);
            return -1;
        }
    }
    return 0; // Success
}

int write_buffer_to_output(const ByteBuffer *buffer, FILE *f)
{
    if (fwrite(buffer->data, 1, buffer->size, f) != buffer->size)
    {
        fprintf(stderr, "Error: Could not write complete data\n");
        return -1;
    }
    if (f == stdout)
    {
        fflush(stdout);
    }
    return 0; // Success
}

bool test_roundtrip(const ByteBuffer *original_data)
{
    printf("Testing compression/decompression roundtrip...\n");

    ByteBuffer compressed_data;
    byte_buffer_init(&compressed_data);

    size_t max_compressed_size =
        berg_estimate_max_compressed_size(original_data->size);
    if (!byte_buffer_resize(&compressed_data, max_compressed_size))
    {
        printf("ERROR: Failed to allocate memory for compressed data\n");
        return false;
    }

    size_t compressed_data_size = 0;
    berg_config config          = berg_get_default_config();

    // Compress
    berg_error_t err = berg_compress(original_data->data,
                                     original_data->size,
                                     compressed_data.data,
                                     compressed_data.capacity,
                                     &compressed_data_size,
                                     &config);
    if (err < 0)
    {
        printf("ERROR: Compression failed with error code: %d\n", (int)err);
        byte_buffer_free(&compressed_data);
        return false;
    }
    compressed_data.size = compressed_data_size; // Adjust to actual size

    printf("Original size: %zu bytes\n", original_data->size);
    printf("Compressed size: %zu bytes\n", compressed_data.size);

    if (original_data->size > 0)
    {
        double ratio =
            (double)compressed_data.size * 100.0 / (double)original_data->size;
        printf("Compression ratio: %.2f%%\n", ratio);
        if (compressed_data.size < original_data->size)
        {
            printf("Space saved: %zu bytes (%.1f%%)\n",
                   original_data->size - compressed_data.size,
                   100.0 *
                       (double)(original_data->size - compressed_data.size) /
                       original_data->size);
        }
        else
        {
            printf("Data expanded by: %zu bytes (%.1f%%)\n",
                   compressed_data.size - original_data->size,
                   100.0 *
                       (double)(compressed_data.size - original_data->size) /
                       original_data->size);
        }
    }

    // Decompress
    ByteBuffer decompressed_data;
    byte_buffer_init(&decompressed_data);
    if (!byte_buffer_resize(&decompressed_data, original_data->size))
    {
        printf("ERROR: Failed to allocate memory for decompressed data\n");
        byte_buffer_free(&compressed_data);
        return false;
    }

    size_t decompressed_data_size = 0;
    err                           = berg_decompress(compressed_data.data,
                          compressed_data.size,
                          decompressed_data.data,
                          decompressed_data.capacity,
                          &decompressed_data_size);

    byte_buffer_free(&compressed_data);

    if (err < 0)
    {
        printf("ERROR: Decompression failed with error code: %d\n", (int)err);
        byte_buffer_free(&decompressed_data);
        return false;
    }
    decompressed_data.size = decompressed_data_size;

    // Verify roundtrip
    if (decompressed_data.size != original_data->size)
    {
        printf("ERROR: Size mismatch after roundtrip!\n");
        printf("  Original: %zu bytes\n", original_data->size);
        printf("  Decompressed: %zu bytes\n", decompressed_data.size);
        byte_buffer_free(&decompressed_data);
        return false;
    }

    if (memcmp(original_data->data,
               decompressed_data.data,
               original_data->size) != 0)
    {
        size_t i;
        for (i = 0; i < original_data->size; ++i)
        {
            if (decompressed_data.data[i] != original_data->data[i])
            {
                printf("ERROR: Data mismatch at byte %zu!\n", i);
                printf("  Original: 0x%02X\n", original_data->data[i]);
                printf("  Decompressed: 0x%02X\n", decompressed_data.data[i]);
                break; // Only report first mismatch
            }
        }
        byte_buffer_free(&decompressed_data);
        return false;
    }

    byte_buffer_free(&decompressed_data);
    printf("SUCCESS: Roundtrip test passed!\n");
    return true;
}

void print_usage(void)
{
    const char *program_name = "berg";

    fprintf(stderr, "Berg Compression Tool v1.0\n");
    fprintf(stderr, "Usage:\n");
    fprintf(
        stderr, "  %s [options] [input_file] [output_file]\n", program_name);
    fprintf(stderr, "\nCommands:\n");
    fprintf(stderr, "  compress <input> <output>     - Compress a file\n");
    fprintf(stderr, "  decompress <input> <output>   - Decompress a file\n");
    fprintf(stderr,
            "  test <input>                  - Test roundtrip compression\n");
    fprintf(stderr,
            "  benchmark <input>             - Benchmark compression "
            "performance\n");
    fprintf(stderr, "\nPipe Operations:\n");
    fprintf(stderr,
            "  %s [output_file]              - Compress from stdin to file or "
            "stdout\n",
            program_name);
    fprintf(
        stderr,
        "  %s -d [input_file]            - Decompress from file or stdin to "
        "stdout\n",
        program_name);
    fprintf(stderr, "\nOptions:\n");
    fprintf(stderr, "  -d, --decompress              - Decompress mode\n");
    fprintf(stderr,
            "  -c, --stdout                  - Write to stdout (default "
            "for pipes)\n");
    fprintf(stderr,
            "  -f, --force                   - Overwrite output files\n");
    fprintf(stderr, "  -h, --help                    - Show this help\n");
    fprintf(stderr, "\nPipe Examples:\n");
    fprintf(
        stderr, "  tar -cf - folder | %s > archive.tar.berg\n", program_name);
    fprintf(stderr, "  tar -cf - folder | %s archive.tar.berg\n", program_name);
    fprintf(stderr, "  %s -d < archive.tar.berg | tar -xf -\n", program_name);
    fprintf(stderr,
            "  cat file.txt | %s | %s -d > restored.txt\n",
            program_name,
            program_name);
    fprintf(stderr, "\nFile Examples:\n");
    fprintf(stderr, "  %s compress data.txt data.berg\n", program_name);
    fprintf(
        stderr, "  %s decompress data.berg data_restored.txt\n", program_name);
    fprintf(stderr, "  %s test data.txt\n", program_name);
}

bool is_pipe_mode() { return !isatty(fileno(stdin)); }

#define STREAM_BUFFER_SIZE (1024 * 1024)

typedef struct
{
    FILE *f;
    size_t total_written;
} FileWriteInfo;

berg_error_t
file_write_callback(const void *buffer, size_t size, void *user_data)
{
    FILE *f = (FILE *)user_data;
    if (fwrite(buffer, 1, size, f) != size)
    {
        return BERG_ERROR_CALLBACK_FAILED;
    }
    return BERG_OK;
}

// Callback that also counts the total bytes written.
berg_error_t
file_write_and_count_callback(const void *buffer, size_t size, void *user_data)
{
    FileWriteInfo *info = (FileWriteInfo *)user_data;
    if (fwrite(buffer, 1, size, info->f) != size)
    {
        return BERG_ERROR_CALLBACK_FAILED;
    }
    info->total_written += size;
    return BERG_OK;
}

static inline uint32_t read_le32(const uint8_t *data)
{
    return (uint32_t)data[0] | ((uint32_t)data[1] << 8) |
           ((uint32_t)data[2] << 16) | ((uint32_t)data[3] << 24);
}

int main(int argc, char *argv[])
{
    bool decompress_mode    = false;
    bool stdout_mode        = false;
    bool force_mode         = false;
    const char *input_file  = NULL;
    const char *output_file = NULL;

    int arg_idx = 1;
    while (arg_idx < argc)
    {
        const char *arg = argv[arg_idx];

        if (strcmp(arg, "-d") == 0 || strcmp(arg, "--decompress") == 0)
        {
            decompress_mode = true;
        }
        else if (strcmp(arg, "-c") == 0 || strcmp(arg, "--stdout") == 0)
        {
            stdout_mode = true;
        }
        else if (strcmp(arg, "-f") == 0 || strcmp(arg, "--force") == 0)
        {
            force_mode = true;
        }
        else if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0)
        {
            print_usage();
            return 0;
        }
        else if (strcmp(arg, "compress") == 0)
        {
            if (argc < 4)
            {
                fprintf(stderr,
                        "Error: compress command requires input and output "
                        "files\n");
                print_usage();
                return 1;
            }
            input_file  = argv[arg_idx + 1];
            output_file = argv[arg_idx + 2];
            arg_idx += 2;
            break;
        }
        else if (strcmp(arg, "decompress") == 0)
        {
            decompress_mode = true;
            if (argc < 4)
            {
                fprintf(stderr,
                        "Error: decompress command requires input and output "
                        "files\n");
                print_usage();
                return 1;
            }
            input_file  = argv[arg_idx + 1];
            output_file = argv[arg_idx + 2];
            arg_idx += 2;
            break;
        }
        else if (strcmp(arg, "test") == 0)
        {
            if (argc < 3)
            {
                fprintf(stderr, "Error: test command requires input file\n");
                print_usage();
                return 1;
            }
            input_file = argv[arg_idx + 1];

            fprintf(stderr,
                    "Testing Berg compression with file '%s'...\n",
                    input_file);
            ByteBuffer input_data;
            byte_buffer_init(&input_data);
            if (read_input_to_buffer(input_file, &input_data) != 0)
            {
                byte_buffer_free(&input_data);
                return 1;
            }

            bool success = test_roundtrip(&input_data);
            byte_buffer_free(&input_data);

            if (!success)
            {
                fprintf(stderr, "FAILED: Roundtrip test failed!\n");
                return 1;
            }

            fprintf(stderr, "PASSED: All tests successful!\n");
            return 0;
        }
        else if (strcmp(arg, "benchmark") == 0)
        {
            if (argc < 3)
            {
                fprintf(stderr,
                        "Error: benchmark command requires input file\n");
                print_usage();
                return 1;
            }
            input_file = argv[arg_idx + 1];

            fprintf(stderr,
                    "Benchmarking Berg compression with file '%s'...\n",
                    input_file);
            ByteBuffer input_data;
            byte_buffer_init(&input_data);
            if (read_input_to_buffer(input_file, &input_data) != 0)
            {
                byte_buffer_free(&input_data);
                return 1;
            }

            berg_config configs[] = {{8}, {12}, {18}, {24}};

            const char *config_names[] = {"Small (8 byte lookahead)",
                                          "Medium (12 byte lookahead)",
                                          "Default (18 byte lookahead)",
                                          "Large (24 byte lookahead)"};

            fprintf(stderr, "\nTesting different configurations:\n");
            fprintf(stderr, "=================================\n");

            int i;
            for (i = 0; i < 4; ++i)
            {
                fprintf(stderr, "\n--- %s ---\n", config_names[i]);

                ByteBuffer compressed_data;
                byte_buffer_init(&compressed_data);
                size_t max_size =
                    berg_estimate_max_compressed_size(input_data.size);
                byte_buffer_resize(&compressed_data, max_size);

                size_t compressed_data_size = 0;
                berg_error_t err            = berg_compress(input_data.data,
                                                 input_data.size,
                                                 compressed_data.data,
                                                 compressed_data.capacity,
                                                 &compressed_data_size,
                                                 &configs[i]);

                if (err < 0)
                {
                    fprintf(stderr, "ERROR: Compression failed\n");
                    byte_buffer_free(&compressed_data);
                    continue;
                }
                compressed_data.size = compressed_data_size;

                ByteBuffer decompressed_data;
                byte_buffer_init(&decompressed_data);
                byte_buffer_resize(&decompressed_data, input_data.size);

                size_t decompressed_data_size = 0;
                berg_decompress(compressed_data.data,
                                compressed_data.size,
                                decompressed_data.data,
                                decompressed_data.capacity,
                                &decompressed_data_size);
                decompressed_data.size = decompressed_data_size;
                if (err < 0)
                {
                    fprintf(stderr, "ERROR: Decompression failed\n");
                    byte_buffer_free(&compressed_data);
                    byte_buffer_free(&decompressed_data);
                    continue;
                }

                bool correct = (decompressed_data.size == input_data.size) &&
                               (memcmp(input_data.data,
                                       decompressed_data.data,
                                       input_data.size) == 0);

                fprintf(stderr, "Original size: %zu bytes\n", input_data.size);
                fprintf(stderr,
                        "Compressed size: %zu bytes\n",
                        compressed_data.size);
                if (input_data.size > 0)
                {
                    double ratio = (double)compressed_data.size * 100.0 /
                                   (double)input_data.size;
                    fprintf(stderr, "Compression ratio: %.2f%%\n", ratio);
                }
                fprintf(stderr, "Correctness: %s\n", correct ? "PASS" : "FAIL");

                byte_buffer_free(&compressed_data);
                byte_buffer_free(&decompressed_data);
            }
            byte_buffer_free(&input_data);
            return 0;
        }
        else if (arg[0] != '-')
        {
            if (!input_file)
                input_file = arg;
            else if (!output_file)
                output_file = arg;
            else
            {
                fprintf(stderr, "Error: Too many arguments\n");
                print_usage();
                return 1;
            }
        }
        else
        {
            fprintf(stderr, "Error: Unknown option '%s'\n", arg);
            print_usage();
            return 1;
        }
        arg_idx++;
    }

    if (!input_file && !output_file && !is_pipe_mode())
    {
        print_usage();
        return 1;
    }

    bool using_stdin  = false;
    bool using_stdout = false;

    if (!input_file || strcmp(input_file, "-") == 0)
    {
        if (!input_file && !is_pipe_mode())
        {
            fprintf(stderr,
                    "Error: No input file specified and not in pipe mode\n");
            print_usage();
            return 1;
        }
        input_file  = "-";
        using_stdin = true;
    }

    if (!output_file)
    {
        if (using_stdin || stdout_mode)
        {
            output_file  = "-";
            using_stdout = true;
        }
        else
        {
            static char generated_output[1024];
            if (decompress_mode)
            {
                strncpy(
                    generated_output, input_file, sizeof(generated_output) - 1);
                generated_output[sizeof(generated_output) - 1] = '\0';
                char *dot = strrchr(generated_output, '.');
                if (dot && strcmp(dot, ".berg") == 0)
                {
                    *dot = '\0';
                }
                else
                {
                    strncat(generated_output,
                            ".out",
                            sizeof(generated_output) -
                                strlen(generated_output) - 1);
                }
            }
            else
            {
                snprintf(generated_output,
                         sizeof(generated_output),
                         "%s.berg",
                         input_file);
            }
            output_file = generated_output;
        }
    }
    else if (strcmp(output_file, "-") == 0)
    {
        using_stdout = true;
    }

    if (!using_stdout)
    {
        fprintf(stderr,
                "%s '%s' to '%s'...\n",
                decompress_mode ? "Decompressing" : "Compressing",
                using_stdin ? "<stdin>" : input_file,
                output_file);
    }

    ByteBuffer input_data;
    byte_buffer_init(&input_data);
    if (read_input_to_buffer(input_file, &input_data) != 0)
    {
        return 1;
    }

    FILE *out_f = NULL;
    if (strcmp(output_file, "-") == 0)
    {
        out_f = stdout;
    }
    else
    {
        out_f = fopen(output_file, "wb");
        if (!out_f)
        {
            fprintf(stderr,
                    "Error: Could not open output file '%s' for writing\n",
                    output_file);
            byte_buffer_free(&input_data);
            return 1;
        }
    }

    uint8_t stream_buffer[STREAM_BUFFER_SIZE];

    if (decompress_mode)
    {
        berg_error_t err = berg_decompress_stream(input_data.data,
                                                  input_data.size,
                                                  file_write_callback,
                                                  out_f,
                                                  stream_buffer,
                                                  sizeof(stream_buffer));

        if (err < 0)
        {
            fprintf(stderr,
                    "ERROR: Decompression failed with error code: %d\n",
                    (int)err);
            byte_buffer_free(&input_data);
            if (out_f != stdout)
                fclose(out_f);
            return 1;
        }

        if (!using_stdout)
        {
            // Read original size from header for the summary message
            uint32_t original_size =
                (input_data.size >= 8) ? read_le32(input_data.data + 4) : 0;
            fprintf(stderr, "Decompression completed successfully!\n");
            fprintf(stderr, "Compressed size: %zu bytes\n", input_data.size);
            fprintf(stderr, "Decompressed size: %u bytes\n", original_size);
        }
    }
    else // Compress
    {
        FileWriteInfo write_info = {out_f, 0};
        berg_config config       = berg_get_default_config();

        berg_error_t err = berg_compress_stream(input_data.data,
                                                input_data.size,
                                                file_write_and_count_callback,
                                                &write_info,
                                                stream_buffer,
                                                sizeof(stream_buffer),
                                                &config);

        if (err < 0)
        {
            fprintf(stderr,
                    "ERROR: Compression failed with error code: %d\n",
                    (int)err);
            byte_buffer_free(&input_data);
            if (out_f != stdout)
                fclose(out_f);
            return 1;
        }

        if (!using_stdout)
        {
            fprintf(stderr, "Compression completed successfully!\n");
            fprintf(stderr, "Original size: %zu bytes\n", input_data.size);
            fprintf(stderr,
                    "Compressed size: %zu bytes\n",
                    write_info.total_written);
            if (input_data.size > 0)
            {
                double ratio = (double)write_info.total_written * 100.0 /
                               (double)input_data.size;
                fprintf(stderr, "Compression ratio: %.2f%%\n", ratio);
            }
        }
    }

    byte_buffer_free(&input_data);
    if (out_f != stdout)
    {
        fclose(out_f);
    }
    return 0;
}
