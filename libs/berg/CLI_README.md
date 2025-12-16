# Berg Compression CLI Tool

A command-line interface for the Berg compression algorithm.

## Building

From the project root:
```bash
mkdir build && cd build
cmake ..
make berg
```

The executable will be created as `berg`.

## Usage

### Compress a file
```bash
./berg compress input.txt output.berg
```

### Decompress a file
```bash
./berg decompress output.berg restored.txt
```

### Test compression/decompression roundtrip
```bash
./berg test input.txt
```

### Benchmark different configurations
```bash
./berg benchmark input.txt
```

## Commands

- **compress** - Compresses an input file and saves to output file
  - Shows compression statistics and ratio
  - Displays detailed algorithm performance metrics

- **decompress** - Decompresses a Berg-compressed file
  - Verifies file integrity during decompression

- **test** - Tests compression/decompression roundtrip
  - Compresses the file and then decompresses it
  - Verifies that the result matches the original exactly
  - Shows detailed compression statistics
  - Returns exit code 0 on success, 1 on failure

- **benchmark** - Tests multiple compression configurations
  - Small: 1KB window, 8 byte lookahead
  - Medium: 2KB window, 12 byte lookahead  
  - Default: 4KB window, 18 byte lookahead
  - Large: 8KB window, 24 byte lookahead
  - Shows compression ratios and correctness for each

## Example Usage

```bash
# Test with the provided test file
./berg test test_data.txt

# Compress a file
./berg compress test_data.txt test_data.berg

# Decompress it back
./berg decompress test_data.berg test_data_restored.txt

# Compare original and restored
diff test_data.txt test_data_restored.txt

# Benchmark different settings
./berg benchmark test_data.txt
```

## Error Handling

The tool will report specific errors for:
- File I/O problems (can't read/write files)
- Compression failures
- Decompression failures  
- Data corruption (roundtrip test failures)

Exit codes:
- 0: Success
- 1: Error occurred

## File Format

Berg compressed files use the `.berg` extension and contain:
- 4-byte magic number: "BERG"
- 4-byte original size (little-endian)
- Compressed token stream with literal data

The format is designed for efficiency on 32-bit systems with limited memory.