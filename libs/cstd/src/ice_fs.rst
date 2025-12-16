ICE File System Specification
=============================

1. Introduction
---------------

The **ICE File System** is a specific application of the ICE Container Format
(defined in ``ice.rst``) used to bundle loose files into a single read-only
archive (``.ice``). It replaces loose file access to minimize OS overhead.

It utilizes specific chunks (``PACK``, ``IDEX``, ``FILE``) to create a flat,
index-based file system.

2. Physical Layout
------------------

An ICE file system archive typically follows this layout:

1.  **PACK Chunk:** The file signature and root pointers.
2.  **IDEX Chunk:** The complete file table and string block.
3.  **Data Chunks:** A sequence of ``FILE`` chunks containing the asset data.

2.1 PACK Chunk
~~~~~~~~~~~~~~

The entry point of the archive. It identifies the file as an ICE archive.

.. code-block:: c

    #define CHUNK_PACK "PACK"

    typedef struct __attribute__((packed)) {
        uint32_t version;          // e.g., 2
        uint32_t file_count;       // Total number of files in the archive
        uint64_t root_dir_offset;  // Byte offset to the start of the IDEX Chunk
    } ICE_PACK;

2.2 IDEX Chunk (Index)
~~~~~~~~~~~~~~~~~~~~~~

The central directory that maps file paths to data offsets.

.. code-block:: c

    #define CHUNK_INDEX "IDEX"

**Payload Layout:**

1.  **Count (uint64_t):** Number of entries.
2.  **Entry Table:** Array of ``ICE_INDEX_ENTRY`` structures.
3.  **String Block:** A packed blob of null-terminated strings (filenames).

**Index Entry:**

.. code-block:: c

    typedef struct __attribute__((packed)) {
        uint64_t path_offset; // Byte offset into the String Block (Relative to start of String Block)
        uint64_t data_offset; // Absolute byte offset to the Data Chunk (Header)
        uint64_t size;        // Uncompressed size of the data
        uint64_t flags;       // Reserved attributes (0)
    } ICE_INDEX_ENTRY;

**Note:** The packer sorts entries by path to allow for binary search lookups (O(log n)).

2.3 FILE Chunk
~~~~~~~~~~~~~~

Contains the raw data of an archived file.

.. code-block:: c

    #define CHUNK_FILE "FILE"

    // Payload is simply the raw bytes of the file.
    // Size is defined in the chunk_header.
