# Finding 222 — WAV Format Extension Read Is Not Bounded by the `fmt ` Chunk

Both WAVE format parsers accept a `fmt ` chunk when its size is greater than
15 bytes, read the first 16 bytes, and, for a non-PCM format tag, read the
two-byte `cbSize` and then that many extension bytes. They do not compare
`cbSize` with the remaining bytes in the `fmt ` chunk before reading. Since
`mmioRead` is issued against the file handle at its current position and the
parser does not ascend from the `fmt ` chunk until after the extension read,
those reads can consume bytes after the chunk when the MMIO source contains
enough data; an actual end-of-file or source-read failure can still produce a
short read.

In the streamed registration path, the caller rejects a parsed format whose
tag is not PCM, but only after this parser has performed the extension read.
The in-memory load path also uses the memory-backed parser and does not make
that PCM check at this point. This describes malformed or non-PCM input
handling; a scan of the 412 loose WAV files in the reference tree found each
has a 16-byte PCM `fmt ` chunk, so those files do not exercise the extension
read branch.

**Evidence.** In shipped `reference/LegoRR.exe` (SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`),
`FUN_00488c70` (`0x488c70`) checks `fmt.cksize > 0xf`, reads 16 bytes, sets the
extra-size field to zero only for tag `1`, otherwise reads two bytes into that
field, allocates `cbSize + 0x12` bytes, and reads `cbSize` bytes before calling
`mmioAscend`. `FUN_00488f10` (`0x488f10`) follows the same sequence. Its caller
`FUN_0047baa0` (`0x47baa0`) rejects a non-PCM tag only after the parser returns
successfully. The parser decompilation is in
`docs/decompiles/full/LEGORR.full.c`. Microsoft's [`mmioRead` reference](https://learn.microsoft.com/en-us/windows/win32/api/mmiscapi/nf-mmiscapi-mmioread)
defines its count as bytes read from the file handle; [`MMCKINFO.cksize`](https://learn.microsoft.com/en-us/windows/win32/api/mmiscapi/ns-mmiscapi-mmckinfo)
holds the data size of a chunk. Neither parser uses that size to bound the
extension read.
