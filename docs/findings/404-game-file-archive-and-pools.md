# Finding 404 — Game Files, Archives, and Record Pools

This finding maps the 73 functions assigned to `game-file-archive-and-pools`. The central abstraction is a game-file handle that routes between WAD-backed memory resources and standard files. WAD entries are indexed in mounted archive contexts, opened into fixed resource slots, and optionally expanded through one of two decoders. Finding 20 contains the detailed dispatcher, mount, archive layout, and codec analysis; this note maps the assigned functions and adds file-system and lifetime context.

## Mounting, dispatch, and game-file handles

| Address | Name | Observed behavior and flow |
| --- | --- | --- |
| `0x0047f130` | `WadMountStartup` | Tries archive names `prefix + index + .wad` for indices 0–9, tracks whether a mount succeeds, and enters the CD/data-missing fallback path when needed. WinMain supplies the executable-derived prefix. |
| `0x0047f4f0` | `FindCdKeyDrive` | Scans candidate drive roots for the expected CD marker and records/returns the discovered drive; exact environment assumptions are described in Finding 20. |
| `0x0047f590` | `WadMountPathCacheStore` | Caches a nonempty path up to 0x400 bytes and sets a validity flag; clears the cache for invalid input. Called from mount startup (Finding 234). |
| `0x0047f640` | `MountWadFromPath` | Resets the current WAD/archive state with `0x00484c90(0)`, then loads the archive index through `WwadArchiveIndexLoad` (`0x0048b4e0`). |
| `0x0047f6a0` | `SaveDirectoryEnsureExists` | Formats the configured save-directory path and ensures the directory exists through the file-system layer. |
| `0x0047f6e0` | `GameFileOpenDispatch` | Selects WAD or standard-file opening through `FileOpenRouteSelect`, allocates a game-file wrapper, opens the selected backing source, and returns its handle. The complete route is in Finding 20. |
| `0x0047fe30` | `FileOpenRouteSelect` | Tests the mode string's first character: `w`/`W` selects the standard-file route so writes bypass WAD lookup; other modes search mounted archives, selecting WAD on a hit and standard-file fallback on a miss. Invalid inputs return an unsupported selector. |
| `0x0047fe20` | `GameFileSystemTypeGet` | Returns the type field at the beginning of the game-file wrapper; callers use it to choose WAD versus standard operations (Finding 234). |
| `0x0047fed0` | `GameFileHandleAllocate` | Allocates and initializes the game-file wrapper used by the open dispatcher; wrapper state includes the type and backing-resource pointer. |
| `0x0047ff50` | `GameFileHandleRelease` | Closes the backing stream/resource when present, then frees the wrapper allocation. |
| `0x0047ffc0` | `MountedPathPrefixRewrite` | When the cached mount path matches the input prefix case-insensitively, emits the remaining suffix into a shared output buffer (Finding 234). |

## Game-file operations and whole-file helpers

| Address | Name | Observed behavior and flow |
| --- | --- | --- |
| `0x0047f850` | `WadFileSeek` | Updates a WAD-backed cursor relative to the requested origin and constrains movement against the entry length; see Findings 19 and 65 for cursor and end-relative details. |
| `0x0047f980` | `WadFileRead` | Bounds reads by remaining WAD entry length, copies from the resource pointer plus cursor, advances the cursor, and returns the number of complete elements read. |
| `0x0047fa50` | `GameFileWrite` (game handle) | Dispatches writes according to handle type and rejects WAD-backed writes. |
| `0x0047fac0` | `GameFileClose` | Closes the appropriate backing resource and releases the game-file wrapper; handle lifetime and WAD slot release are detailed in Finding 64. |
| `0x0047fb10` | `WadFileCursorGet` | Returns the current cursor from the WAD-backed stream state (Finding 64). |
| `0x0047fc20` | `GameFileReadByte` | Reads one byte from a WAD cursor or buffered standard stream; returns EOF at the respective boundary (Finding 234). |
| `0x0047fca0` | `GameFileLengthGet` | Returns the backing entry/file length through the appropriate path. |
| `0x0047fce0` | `GameFileReadLine` | Reads characters until NUL, newline, or `0xff`, terminates the caller buffer, and reports a line only when data was consumed (Finding 234). |
| `0x0047fd40` | `GameFileReadLineDispatch` | Routes line reads through the WAD or standard-file implementation based on wrapper type; diagnoses unsupported types (Finding 234). |
| `0x0047fdb0` | `GameFilePrintFormat` | Rejects formatted output to WAD streams, diagnoses unsupported types, and forwards standard streams to formatted writing (Finding 234). |
| `0x0047fea0` | `WadEntryReadIntoStreamSlot` | Opens/reads a WAD entry into a fixed resource slot and stores its entry identity and resource pointer for later reads. |
| `0x00480050` | `ReadCrlfLine` | Calls `GameFileReadLineDispatch` with the supplied read limit, then removes trailing LF and CR bytes; the dispatch supports both WAD-backed and standard-file handles. The decompiled body does not establish that the limit is the caller buffer's safe capacity. |
| `0x004800a0` | `ReadWholeFile` | Calls `ReadWholeFileWithProgress` with progress enabled. |
| `0x004800c0` | `ReadWholeFileWithProgress` | Opens a game file, obtains length, allocates a buffer, reads it, closes the stream, and optionally calls the progress callback; stream and buffer lifetimes are separated in Finding 96. |
| `0x00480170` | `MapOpen2` | Resolves/opens a map resource through the shared game-file path and prepares its output state. |
| `0x00480220` | `ResourcePathResolveAgainstBase` | Resolves a requested resource path against the active base/mount directory into shared static output `DAT_005341a0`; no allocation path is visible in the recovered body. |
| `0x004802b0` | `LoaderProgressCallbackSet` | Stores the progress callback and context in globals `0x005343ac`/`0x005343b0`; the whole-file loader invokes them before reading (Finding 96). |
| `0x004802d0` | `FileTokenListLoadAndTreeScan` | Loads a text file and scans it into token/list/tree state; the only resolved direct caller is `StartupConfigStreamLoad` (`0x0048b2f0`). |
| `0x00480390` | `DirectoryTreeScan` | Enumerates files/directories under a root and builds the token records consumed by the directory comparator. |
| `0x00480570` | `DirectoryTokenListCompare` | Compares directory-token records using their stored names/metadata to support ordered scans. |

## WAD index, entry slots, and resource handles

| Address | Name | Observed behavior and flow |
| --- | --- | --- |
| `0x0048b4e0` | `WwadArchiveIndexLoad` | Opens the archive, checks `WWAD`, reads the count, filename tables, and fixed-size TOC records, then stores archive metadata in a context slot (Finding 20 and Finding 229). |
| `0x0048bd30` | `WadEntrySlotAllocate` | Finds a free one of 16 WAD resource slots or returns failure. |
| `0x0048bd50` | `WadEntryFindAcrossMountedArchives` | Searches the requested archive or scans mounted contexts from later to earlier when no archive is specified; returns an entry index or -1. |
| `0x0048bda0` | `WadEntryNameLookup` | Case-insensitively scans an archive's filename table for the requested entry. |
| `0x0048be00` | `FixedRecordSlotAllocate` | Finds an unused fixed resource-handle slot in the 16-entry table. |
| `0x0048be40` | `WadEntryMemoryStreamOpen` | Resolves an archive entry, allocates/reads its stored data, optionally expands flagged payloads, and records the resulting pointer plus archive/entry indices in a fixed slot. |
| `0x0048bf70` | `WadPathOpen` | Opens within a specified archive or searches mounted archives in reverse order when passed the sentinel index. |
| `0x0048bfc0` | `FixedRecordSlotResourceRelease` | Frees an occupied slot's resource pointer and clears its occupied flag. |
| `0x0048bff0` | `FixedRecordSlotResourceGet` | Returns the resource pointer for an occupied slot, otherwise zero. |
| `0x0048c010` | `WadStreamLengthGet` | Resolves archive and entry indices stored in a slot, then calls `RegistryEntryValueGet` for the associated metadata. Ghidra recovered a `void` signature, while callers consume the apparent call result as the stream length; return behavior is therefore caller-evidenced but ABI-inferred. |

## Standard-file and Win32 compatibility layer

| Address | Name | Observed behavior and flow |
| --- | --- | --- |
| `0x0048eae0` | `FileResourceHandleClose` | Closes the OS file/resource handle and releases associated wrapper state. |
| `0x0048eb60` | `StdFileOpenIntoHandle` | Acquires a pooled standard-file handle and opens the requested path/mode into it. |
| `0x0048eb90` | `FileOpenAndReadWholeStream` | Standard-file convenience wrapper around `StdFileOpenIntoHandle` with mode `0x40`. |
| `0x0048f460` | `FileResourceHandleSeek` | Performs a bounded seek on a standard-file resource handle and updates its stream position. |
| `0x0048f500` | `GameFileWrite` (standard-file handle) | Writes a count of fixed-size elements to a raw standard-file handle and returns the number written; short-write result handling is documented in Finding 105. |
| `0x0048f650` | `GameFileDelete` | Wraps the Win32 file-delete operation and maps its success/failure result. |
| `0x0048f680` | `FileSetReadOnlyAttribute` | Reads file attributes and sets or clears the read-only bit, mapping errors through the compatibility layer (Finding 237). |
| `0x0048f6d0` | `GameDirectoryCreate` | Wraps directory creation and translates the OS result to the game's return convention. |
| `0x0048f700` | `GameFileMetadataFind` | Starts directory enumeration and copies metadata for the first matching entry into the caller record. |
| `0x0048f830` | `FileFindNextRecord` | Advances `FindNextFileA`, maps end/error state, and copies attributes, timestamps, size, and name (Finding 237). |
| `0x0048f970` | `FileTimeToLocalDateTimeStamp` | Converts a Win32 `FILETIME` into the game's local date/time representation. |
| `0x0048fa00` | `CurrentDirectoryGet` | Convenience wrapper that asks `DriveCurrentDirectoryGet` for drive zero. |
| `0x0048fa20` | `DriveCurrentDirectoryGet` | Reads the process directory or resolves a requested drive's `X:.` path, copying into a caller buffer or allocating one (Finding 237). |
| `0x0048fb50` | `DriveLetterRootTypeAvailable` | Checks whether a drive root exists/is ready using the Win32 drive-type query (Finding 237). |
| `0x0048fb90` | `GameFileReadBuffered` | Reads buffered bytes from a standard-file resource into the caller's buffer and advances its stream state. |
| `0x0048ffa0` | `PathResolveFull` | Wraps `GetFullPathNameA`, using a supplied buffer or allocated storage and the current-directory helper for empty input (Finding 237). |
| `0x00494e30` | `StdFileOpen` | Parses the requested mode, applies the standard-file path/open flags, and initializes the OS file handle. |
| `0x00495000` | `StdFileHandleAcquire` | Acquires or initializes a pooled standard-file handle record. |

## WAD payload expansion and bitstream decoders

| Address | Name | Observed behavior and flow |
| --- | --- | --- |
| `0x0049c7c0` | `ExpandWadPayload` | Reads the payload header, allocates the declared output size, and dispatches expansion based on the codec type. |
| `0x0049c840` | `WadPayloadDecodeDispatch` | Selects raw copy, type-1 Huffman/bitstream decoding, or type-2 run/backreference decoding from the header type. |
| `0x0049c8e0` | `DecodeWadPayloadType1` | Decodes the type-1 stream using LSB-first bit reads and canonical Huffman tables. |
| `0x0049ca60` | `DecodeWadPayloadType2` | Decodes the type-2 stream using MSB-first reads, run lengths, and backreference offsets. |
| `0x0049cc70` | `WadBitstreamOutputInitialize` | Initializes global decoder input/output state for the requested source and destination buffers. |
| `0x0049ccc0` | `WadReadBitsLsbFirst` | Reads the requested number of bits from the input stream in least-significant-bit-first order. |
| `0x0049cd30` | `WadReadBitsMsbFirst` | Reads bits from the input stream in most-significant-bit-first order. |
| `0x0049cd90` | `WadHuffmanTableRead` | Reads code-length/table metadata and prepares a decoder table. |
| `0x0049ce00` | `WadHuffmanSymbolDecode` | Decodes one symbol from a prepared Huffman table using the active bitstream state. |
| `0x0049ce70` | `WadRunLengthDecode` | Decodes the type-2 run-length token/value. |
| `0x0049ceb0` | `WadBackreferenceOffsetDecode` | Decodes the type-2 backreference distance/offset. |
| `0x0049cf50` | `WadHuffmanTableClear` | Clears/initializes Huffman table state before a table is populated. |
| `0x0049cf90` | `WadHuffmanCanonicalCodesBuild` | Builds canonical Huffman code ranges/lookup state from code lengths. |
| `0x0049d000` | `WadReverseLowBits` | Reverses the requested low-order bits, supporting the LSB-first code path. |

## End-to-end flow and limits

- Startup attempts to mount numbered WAD archives. The `GameFileOpenDispatch` path searches mounted indexes first and falls back to standard files; WAD writes and formatted output are rejected. Finding 20 gives the mount order and archive layout.
- A WAD read resolves a case-insensitive name in an archive context, opens the matching entry into a fixed slot, optionally expands it, and advances a cursor over the resulting in-memory resource. The game-file wrapper then shares read/seek/length/close operations across WAD and standard files.
- Whole-file consumers allocate their own byte buffer, read through the wrapper, and close the stream; parsed/tokenized data may outlive the handle. Finding 96 traces that lifetime split.
- The shipped WADs contain uncompressed entries, so both codec implementations are present but not exercised by the shipped corpus. Finding 20 records the byte-identical archive round-trip evidence and the dormant-codec limit.
- Existing findings 64, 65, 96, 105, 20, 229, 234, and 237 provide deeper evidence for handle state, seek boundaries, archive format, and Win32 wrappers. This finding indexes all assigned functions and connects those notes into one area map.

## Open questions and focused follow-up

- Confirm type/ownership contracts for the standard-file and game-file wrapper records, including whether every failure path releases partially allocated handles.
- Exercise both compressed WAD codecs with independently sourced compressed payloads; the shipped archive set cannot validate those decoder branches.
- Determine exact path and error semantics for directory scanning, save-directory creation, and CD fallback on representative systems.
- Audit caller buffer capacities for line/string reads and path formatting, since several helpers rely on caller-provided storage.
