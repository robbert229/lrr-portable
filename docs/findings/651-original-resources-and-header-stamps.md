# 651. The matching build carries the original's resources and header stamps

Issue #560, closing it. Follows finding 563 (the whole-binary audit, which
measured these differences) and finding 642 (the gate's link mode). Measured
on `main` `5f251885` with a full matching build before and after.

## Summary

1. **`.rsrc` is the original's, except where the section sits.** The matching
   build's resource script now has the original's three leaves and no
   `VERSIONINFO`. `make pe-audit` reports `resources: 3/3 leaves identical`:
   `ICON 1` (2,216 bytes, en-GB), `DIALOG 101` (458 bytes, en-US, already
   identical) and `GROUP_ICON 113` (20 bytes, en-GB). `.rsrc` is 2,936 bytes
   in both images. 2,930 of its bytes are identical. The other 6 are the high
   two bytes of the three data-entry RVAs (`0x37xxxx` against `0x3fxxxx`),
   which follow from where `.rsrc` sits. Each leaf's offset inside the section
   is the original's (`0xf0`, `0x2bc`, `0xb64`).
2. **The header stamps are the original's.** A post-link step writes the COFF
   `TimeDateStamp` `0x37EEB911`, the `TimeDateStamp` of every resource
   directory table (all seven in the original carry `0x37EEB90E`) and sets
   `IMAGE_FILE_LINE_NUMS_STRIPPED`, so `Characteristics` is `0x10F`. The
   checksum is 0 in both images and stays so.
3. **What still differs in the headers follows from section sizes.** `make
   pe-audit` lists `AddressOfEntryPoint`, `BaseOfData`, `SizeOfCode`,
   `SizeOfImage`, `SizeOfInitializedData` and the import, IAT and resource
   directories. Of the first 1,024 file bytes, 48 differ, all from `0x9d` on:
   those fields and the section table.
4. **No score changed.** `make reccmp-check` passes with every function at its
   baseline. The match baseline changes only in its input hash.
5. **The image end does not move.** `.rsrc` grows from 1,348 to 2,936 bytes
   but stays within its one `0x1000` page, so the image still ends at
   `0x7f5000`, 45,056 bytes below `0x800000` (finding 584).

## How

### Resources

`src/version.rc` is the resource script of the Windows builds. The matching
build (`LRR_RECCMP`) compiles it with `LRR_ORIGINAL_RESOURCES` defined
(`CMakeLists.txt`, a source-file property), which:

- leaves out the `VERSIONINFO` block;
- keeps the dialog first, then switches to `LANGUAGE 0x09, 0x02`
  (`LANG_ENGLISH`, `SUBLANG_ENGLISH_UK`) and declares `113 ICON
  "LegoRR.ico"`. RC numbers the icon image 1 itself.

The order matters: the original's resource data is laid out dialog, icon,
group icon, in the order of the resource script, and CVTRES 5.0 keeps it.

The reference build (`PLATFORM=winapi` without `LRR_RECCMP`) keeps the
`VERSIONINFO` and the dialog as before. The native build has no resource
script.

### The icon is extracted at build time

The icon is licensed art and is not in the repository. A custom command runs
`tools/pe_stamps.py icon reference/LegoRR.exe build-reccmp/LegoRR.ico`, which
rebuilds the `.ico` file from the original's `GROUP_ICON 113` and the `ICON`
leaves it names (one directory entry, `dwImageOffset` 22, then the image).
RC compiles it back to the original's two leaves byte for byte. The path is
the cache variable `LRR_ORIGINAL_EXE`, defaulting to `reference/LegoRR.exe`,
where every tool reads the original. Configuring the matching build without
it stops with a message naming the missing file. The tool fails the same way
if it is run without it.

### Header stamps

`tools/reccmp.sh` runs `tools/pe_stamps.py stamp build-reccmp/LegoRR.exe`
right after `strip_pe_debug.py`, so Reccmp, `binary_progress.py` and
`make pe-audit` read the stamped image. The stamp is a post-link step rather
than a link option because:

- LINK 5.0 takes the COFF timestamp from the clock and has no option to set
  it, and CVTRES does the same for the resource directories.
- The gate links with `/DEBUG` for Reccmp's PDB (finding 642), and `/DEBUG`
  clears `LINE_NUMS_STRIPPED`. A link without `/DEBUG` sets it, which is why
  the `opt-ref` relink (finding 576's second link) already has `0x10F`.

The step writes no other byte. The tests (`tools/tests/test_pe_stamps.py`,
in `make tools-test`) check that on a synthetic image with a resource tree,
that stamping is idempotent, that an image without resources is stamped, that
stamping `reference/LegoRR.exe` changes nothing, and the icon extraction and
its errors.

## Figures

| | `main` `5f251885` | this change |
| --- | --- | --- |
| Exact-function code bytes | 319,635 (49.74%) | 319,635 (49.74%) |
| Accuracy, full / game-only | 96.72% / 96.42% | 96.72% / 96.42% |
| `.rsrc` size, original 2,936 | 1,348 | 2,936 |
| `.rsrc` identical bytes | 479 (16.31%) | 2,930 (99.80%) |
| Resource leaves identical | 1 of 3, plus `VERSIONINFO` | 3 of 3 |
| COFF `TimeDateStamp` | link time | `0x37EEB911` |
| Resource directory `TimeDateStamp` | link time | `0x37EEB90E` |
| `Characteristics` | `0x10B` | `0x10F` |
| Image end | `0x7f5000` | `0x7f5000` |

## Left

- The `opt-ref` relinks (`make opt-ref-link`) are not stamped. They exist to
  measure code layout, and nothing reads their header stamps.
- The 6 `.rsrc` bytes and the header fields listed above close when the
  sections before `.rsrc` reach the original's sizes.
