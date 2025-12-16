ICE Container Format Specification
==================================

1. Introduction
---------------

ICE (Inter-Chunk Exchange) is a generic, chunk-based container format designed
for high-performance data exchange and asset streaming. It provides a
lightweight structure for organizing heterogeneous data into a unified stream,
suitable for both archives (filesystems) and individual asset files (models,
textures, etc.).

2. Architecture
---------------

The ICE format defines a flat sequence of **Chunks**.

2.1 Chunk Header
~~~~~~~~~~~~~~~~

Every chunk begins with a unified 8-byte header:

.. code-block:: c

    typedef struct {
        char     id[4];  // 4-Character Code (e.g., "PACK", "MESH")
        uint32_t size;   // Size of the chunk payload
    } chunk_header;

**Common Chunk IDs:**

*   **File System Chunks:**
    *   ``PACK``: Archive Header.
    *   ``IDEX``: Central Directory.
    *   ``FILE``: Raw file data.

*   **Asset Chunks:**
    *   ``MESH``: 3D Model data.
    *   ``TEXT``: Texture data.

3. Type System (Fixed Point)
----------------------------

ICE defines a set of compact, fixed-point data types used for serializing game
math (Vectors, Matrices) to save space with minimal loss of precision.

**Scale Factor:** ``64.0`` (All floating point values are multiplied by 64 and
truncated).

3.1 Primitives
~~~~~~~~~~~~~~

+-----------+------------+--------------------------------------------------+
| Type      | Underlying | Description                                      |
+===========+============+==================================================+
| ICE_R16   | int16_t    | Range: [-512.0, 512.0]. Precision: ~0.015        |
+-----------+------------+--------------------------------------------------+
| ICE_R8    | int8_t     | Range: [-2.0, 2.0]. Precision: ~0.015            |
+-----------+------------+--------------------------------------------------+
| ICE_N16   | int16_t    | Normalized Map [-1.0, 1.0] to [I16_MIN, I16_MAX] |
+-----------+------------+--------------------------------------------------+

3.2 Compounds
~~~~~~~~~~~~~

*   **ICE_VEC3_R16**: 3x ``ICE_R16`` (6 bytes). Used for positions/scales.
*   **ICE_NORM3**: Octahedral encoded Normal Vector stored in 2x ``ICE_N16`` (4 bytes).
*   **ICE_QUAT_R16**: Quaternion stored as 3x ``ICE_R16`` (Cayley-Klein parameterization).
*   **ICE_MAT4X4_R16**: 4x4 Matrix of ``ICE_R16`` (128 bytes).

4. Implementation Notes
-----------------------

*   **Endianness:** All values are Little Endian.
*   **Alignment:** Structures are packed (``#pragma pack(1)``).
