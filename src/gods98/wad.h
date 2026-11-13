// SPDX-License-Identifier: LGPL-3.0-or-later
/* Wad.c (original translation unit G98-030, finding 591): the WWAD archive
 * index and the WAD entry handles that Files.c reads through.
 *
 * C-compatible header (ADR 0005). wad.c is compiled as C in the matching
 * build and as C++ in the native port. It declares only the six entry
 * points gods98/files.c calls, in the order #598 (finding 583) declared
 * them, so files.c's symbol numbering is unchanged; the unit's other
 * functions and its types are private to wad.c.
 *
 * The native port keeps its own WAD reader (gods98/wad_port.cpp and
 * gods98/resource.cpp, the port tail) under the port's names. Nothing
 * native calls these. Shared C++ headers do not include this file. */
#ifndef LEGORRR_GODS98_WAD_H
#define LEGORRR_GODS98_WAD_H

#ifdef __cplusplus
extern "C" {
#endif

/* Wad_Load (0048b4e0): mount one archive; its number, or -1. */
int wad_load(const char* fName);

/* Wad_IsFileInWad (0048bd50): the entry's index in archive wadNo, or in the
 * newest active archive that has it when wadNo is -1; -1 if none. */
int wad_is_file_in_wad(const char* fName, int wadNo);

/* Wad_FileOpen (0048bf70): read an entry into a handle; -1 on failure. */
int wad_file_open(const char* fName, int wadNo);

/* Wad_FileClose (0048bfc0): free a handle's data. */
void wad_file_close(int handle);

/* Wad_hLength (0048c010) and Wad_hData (0048c030): a handle's
 * uncompressed length and data. */
int wad_file_length(int handle);
void* wad_file_data(int handle);

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GODS98_WAD_H */
