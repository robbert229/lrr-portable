# Finding 64 — WAD opens create memory-backed stream handles

*Target: shipped `LegoRR.exe`, SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.*
*Provider: Ghidra 12.1.4 via REA. Evidence: `docs/evidence/legorr-wad-stream-contract.bundle`.*

The generic open/read/close API presents both loose files and WAD members as
the same two-field handle, but the WAD branch uses a per-open memory view. This
adds the lifetime and read contract missing from the loader map in finding 01.

## Observed path

`FUN_0047fed0` allocates an 8-byte public handle. For WAD handles (type 0), it
also allocates a 12-byte cursor record; for standard files (type 1), the public
handle stores the CRT file handle directly. `FUN_0047f6e0` dispatches archive
opens through `FUN_0047fea0` and loose-file opens through `FUN_0048eb90`
(`ev_b35479191a446eb06e942fb26c228d1b13fea0525b305987e587ef582e42a1fa`,
`ev_f11de4c6e3913620186b17b9a6ac35eb5c273e15cb738905f3a4897825e122`).

On the archive path, `FUN_0048be40` claims a free slot from the 16-entry
runtime table, allocates a buffer sized from the TOC's original-length field,
seeks to the entry's stored offset, and reads its stored payload. If TOC flag
bit `0x2` is set, it expands the payload and releases the temporary stored
buffer; otherwise the stored buffer becomes the slot's data pointer. The slot
retains the WAD index and entry index alongside that pointer
(`ev_09a43fb1748c95b3680b1e01d252ddebe7830edcaa1b3e5719c94bcff1ea3658`,
`ev_c6f024d4da6deda2e148d1ed81d674c37fc4ecd8f69ef644fb9725b23a11ec76`).
The loader-map finding documents the mount table and entry flags; this trace
shows that reads happen from the materialized per-open buffer rather than by
re-reading archive ranges.

`FUN_0047f980` dispatches WAD reads to that buffer. It computes
`size * count`, limits the copy to the remaining entry bytes when the cursor is
within the entry, copies from `data + cursor`, advances the cursor by the bytes
copied, and returns `bytesCopied / size` (complete items, not raw bytes). The
standard-file branch delegates to the CRT read wrapper. `FUN_0047fb10` returns
the WAD cursor; `FUN_0047fac0` closes the public handle, and its WAD cleanup
frees the slot buffer and marks that pool slot unused
(`ev_a0a065d16b58f66810fb67b194228dbe8704b71b84aa9c0b096408511b91c8b5`,
`ev_5d07db9b82667c6c8b02065d3b88a9fbe43dbbef42908a7458482fe1c80e873e`,
`ev_6cdc20307e71bb02ed36901c53e1ec90fc34be48ad8d60d4f43f2ecb32a241b6`,
`ev_fa9cf1449e24cc8217189945622449122729f5b53cead196a13b91d73d88089f`,
`ev_e0740efd012ade1c4a61c5b78be52f148b75581be014d943f35b816bb87229f7`).

## Boundaries

- The clipped-read description assumes the cursor is at or before the entry
  length. Finding 65 records the end-relative seek exception that can move it
  outside that range.
- This is static shipped-binary evidence. It establishes the code path and
  buffer lifetime, not runtime behavior under allocation failure or malformed
  archives.
- The 12-byte cursor record's unused fields are not assigned semantic names.

