# Finding 403 — Configuration and Text Parsing Helpers

This finding maps the 26 functions assigned to `config-text-parsing`. The main format path is the block-text scanner and path lookup described in Finding 12: load a file into memory, split it into in-place tokens, chain records from the shared token pool, then query nested records by a path. The remaining functions provide common token, scalar, RGB, named-option, and numeric-conversion helpers.

| Address | Name | Observed behavior | Data/caller flow |
| --- | --- | --- | --- |
| `0x00477440` | `BlockTextTokenSplit` | Splits a mutable string in place on a supplied delimiter string and returns the number of token pointers written. | Used by path lookup, RGB parsing, entity names, and other delimited configuration values. |
| `0x004774b0` | `WhitespaceTokenize` | Replaces locale-recognized whitespace with NUL bytes and stores pointers to each token. | Mutable-buffer tokenizer; returns zero for an empty input. |
| `0x00477550` | `StringDuplicate` | Allocates `strlen(input)+1` bytes and copies the string. | Shared heap-backed string helper used by config and UI paths. |
| `0x00477590` | `FormattedTextEscapeDecodeAndEmit` (corpus alias; visible emission unconfirmed) | Formats into a 256-byte temporary, maps `\n` to newline and `_` to space into another 256-byte buffer, then calls `StringDuplicate`. | The recovered body discards the duplicate helper's return value; output ownership/consumer contract is unresolved. |
| `0x00477610` | `CaseInsensitiveSubstringFind` | Scans each candidate start using a case-insensitive bounded compare and returns the first matching pointer, or null. | Search string and candidate must both be nonempty. |
| `0x00477670` | `GameStringHashCompute` | Accumulates a position-weighted character hash using multiplier `0x600937`; optional flags skip whitespace and uppercase each character before hashing. | Used by named lookup tables; exact collision policy belongs to callers. |
| `0x00477710` | `ConfigBooleanParse` | Case-insensitively maps `YES/TRUE/ON` to 1, `NO/FALSE/OFF` to 0, and other text to 2. | Shared by configuration boolean consumers; value 2 is the unrecognized/default state. |
| `0x00478df0` | `BlockTextTokenPoolInit` | Clears 32 chunk pointers and resets the free-list head, chunk count, and initial growth exponent. | Called during WinMain initialization before block-text files are loaded. |
| `0x00478e20` | `BlockTextTokenPoolDestroy` | Frees each non-null token-pool chunk, then clears the free-list head and growth exponent. | Called during WinMain teardown; Finding 95 traces the shared 32-byte-record arena. |
| `0x00478e60` | `BlockTextOpenAndScan` | Loads a file to memory, clears semicolon comments and whitespace in place, then creates linked 32-byte token records while maintaining nesting depth from brace tokens. | Shared parser for `Lego.cfg` and other block-text resources; it does not validate a `Lego*` signature. |
| `0x00478f50` | `ConfigEntryFind` | Joins the root and supplied path components with the `::` delimiter into static scratch `DAT_00506098`, then returns that string pointer. It does not search the record chain. | Callers pass the returned path to `ConfigPathValueGet` or `ConfigNestedPathChildFind`; actual record lookup occurs in `ConfigBlockPathFind` (`0x004792e0`). Static scratch makes this helper stateful. |
| `0x00478ff0` | `ConfigNestedPathChildFind` | Calls `ConfigBlockPathFind`, then returns the next record only when it is deeper than the matched record. | Subgroup iterator used by block-text consumers. |
| `0x00479020` | `SortedEntryNextEqualKey` | Advances through linked records while the depth is nondecreasing and returns the next record at the original depth. | Supports sibling iteration over the flat record chain. |
| `0x00479050` | `ConfigStringCopy` | Finds a path record, then allocates and returns a copy of its value string; returns null if record/value is absent. | Caller owns the returned buffer; `ConfigBooleanGetDefault2` frees its copy after parsing. |
| `0x004790b0` | `ConfigPathValueGet` | Returns the value pointer (`record+8`) for a matching path, or null. | Borrowed pointer into the parsed file buffer; unlike `ConfigStringCopy`, it does not allocate. |
| `0x004790d0` | `ConfigBooleanGetDefault2` | Returns 2 when no value exists; otherwise copies the string, parses it with `ConfigBooleanParse`, and frees the copy. | Shared lookup/parser path for boolean settings. |
| `0x00479110` | `ReadAngleSettingRadians` | Reads a numeric value or a default, converts it through the float parser, then multiplies by `2π/360`. | Interprets the stored setting as degrees and returns radians; zero stays zero. |
| `0x00479170` | `ConfigRgbVectorRead` | Copies a config value, splits on commas, requires exactly three components, parses each integer, and scales each channel by `1/255`. | Writes normalized RGB floats only on a three-token parse; frees its copied input. |
| `0x00479270` | `BlockTextTokenAllocate` | Pops a 32-byte record from the global free list, grows the pool if empty, clears link/value fields, and optionally links the new record to its parent. | Used by `BlockTextOpenAndScan`; Finding 95 maps pool growth and shutdown lifetime. |
| `0x004792c0` | `GlobalRecordListPushHead` | Links a record's `+0x1c` field to the current global head and makes that record the new head. | Shared record-list helper; exact list semantics depend on caller. |
| `0x004792e0` | `ConfigBlockPathFind` | Splits a query on the block path delimiter, scans records by depth/name, and follows parent links to match ancestor components. | Case-insensitive matching with a first-component prefix wildcard when the stored name contains `*`; full rules are in Finding 12. |
| `0x00479490` | `BlockTextTokenPoolGrow` | Allocates a chunk of `1 << growthExponent` 32-byte records, chains records through `+0x1c`, and prepends them to the free list. | Shared pool grows geometrically; Finding 95 records the 32-chunk tracking array and teardown. |
| `0x00481ac0` | `StringTrailingDigitsSplit` | When a string ends in digits, copies it and removes the trailing digits in the copy; outputs the parsed integer and prefix length. | Returns a non-null output pointer on success and null when no valid trailing digit suffix is found. |
| `0x00481b80` | `CounterFormatStringBuild` | Copies a base string, inserts a formatted numeric suffix and padding to a requested width, and increments the supplied counter. | Companion to trailing-digit parsing; buffer sizes/capacity are caller-managed. |
| `0x00485e30` | `NamedOptionIndexFindCaseInsensitive` | Scans a 256-slot string table case-insensitively and writes the matching slot index to the output byte. | Returns 1 on match and 0 when absent; table contents are initialized elsewhere. |
| `0x004907f0` | `ParseEntityHealthValue` | Calls the CRT numeric scanner and extended-float conversion, writes result-kind flags, consumed length, and a 64-bit parsed value into shared scratch, then returns that scratch pointer. | Used by entity health parsing; Finding 91 documents caller fallback behavior when the parsed value is zero. |

## Main data path and boundaries

1. `BlockTextOpenAndScan` calls `ReadWholeFile` (`0x004800a0`) and stores tokens in a shared 32-byte-record pool. The scanner strips semicolon comments to newline, splits on whitespace, and records brace nesting as depth; this is a flat linked record chain rather than a recursively allocated tree.
2. `ConfigBlockPathFind` splits a query and matches case-insensitive names at the expected depth, then validates ancestors through parent links. A `*` in the stored first/root name supplies a prefix match. Finding 12 documents the grammar, wildcard rules, and shipped-file validation.
3. `ConfigPathValueGet` returns a borrowed value pointer into the loaded buffer; `ConfigStringCopy` returns a separately allocated string. Typed helpers either parse the borrowed value or allocate/free their own copy. Callers must honor these different lifetimes.
4. Pool reset occurs at startup and chunk cleanup at shutdown. Parsed record chunks are shared across opened documents and are not released per document; see Finding 95.

## Open questions and focused follow-up

- Establish bounds for fixed temporary/output buffers in text escape decoding, path splitting, formatted counters, and RGB tokenization; most helpers rely on caller-provided capacity.
- Determine the intended ownership/side effect of the duplicate returned by `StringDuplicate` in `FormattedTextEscapeDecodeAndEmit`, whose recovered body discards that return.
- Document the initialization and collision behavior of the 256-slot named-option table and the consumer contract for hash flags.
- Reconcile exact token/value ownership for block documents, including which caller frees source buffers versus retaining them through the shared parser pool.
