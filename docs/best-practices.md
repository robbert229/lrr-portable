# Best practices for recovered layouts

These practices make reconstructed records and interfaces easier to compare with
the original executable and safer to change.

## Provenance

This document is an original, repository-specific adaptation of layout
recommendations published by the [LEGO Island Decompilation Project in
`reccmp`'s recommendations](https://github.com/isledecomp/reccmp/blob/master/docs/recommendations.md).
It covers the same general topics—record sizes, field offsets, virtual-table
slots, and uncertain scalar types—but rewrites them for `lrr-portable` and its
cross-platform C++ code. Consult the upstream document for the source project's
own conventions.

## Record sizes

When evidence supports a record's size, state that size near its declaration
and enforce it with a compile-time assertion. Assertions catch accidental
compiler-layout changes; they do not prove that the recovered size is correct.
Base the expected value on the executable, decompilation, or a documented
finding.

```cpp
#include <cstddef>
#include <cstdint>

struct ExampleRecord {
    std::uint32_t kind;  // +0x00
    std::uint16_t flags; // +0x04
    std::uint16_t count; // +0x06
};

static_assert(sizeof(ExampleRecord) == 0x08);
static_assert(offsetof(ExampleRecord, count) == 0x06);
```

Use `offsetof` checks for fields whose positions matter independently of the
overall size. If a layout contains pointers or other target-dependent types,
account for the original 32-bit ABI and the project's native build before
asserting a size shared across targets. Prefer fixed-width fields or explicit
target-specific representations where the binary layout requires them.

## Field offsets

Annotate fields with offsets from the beginning of the recovered record. Keep
the offsets beside the declarations so reviewers can compare code directly
with the finding or pseudocode. Preserve gaps as explicit padding or named
unknown fields when their widths are known; do not let compiler padding
silently stand in for bytes that have not been modeled.

Use names that reflect the evidence. A field with a known position but unknown
meaning should remain clearly unknown, rather than receiving a guessed gameplay
name. Link the declaration or its surrounding comment to the finding that
supports the layout.

## Virtual tables

When the recovered interface has a virtual table, record the table's original
address only when it is established by evidence. Annotate each understood
entry with its byte offset from the start of the table. Keep that offset
distinct from the object's field offsets and from source-language method order.
Do not infer an entry's meaning from a plausible method name alone; cite the
observed dispatch or call site.

## Scalar types and uncertainty

Use fixed-width integer types for values read from the original binary or
stored in its records. Avoid host-dependent types such as `long` or `size_t`
for fields whose width is part of the recovered format.

Preserve uncertainty in the type name and comments until evidence resolves it.
Signed and unsigned values of the same width can behave differently in
comparisons, extensions, and bounds checks, so retain the observed behavior
instead of choosing a signed type for convenience. Update the type, offset
notes, and any layout assertions together when new evidence changes the
interpretation.
