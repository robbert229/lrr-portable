// SPDX-License-Identifier: LGPL-3.0-or-later
/* C boundary header (ADR 0005) for the Wad.c (G98-030, gods98/wad.cpp and
 * gods98/resource.cpp) entries that gods98/files.c calls, with the
 * original's contracts. Wad.c is not converted yet (finding 583):
 *
 * - wad_entry_find_across_mounted_archives (Wad_IsFileInWad, 0048bd50) and
 *   fixed_record_slot_resource_release (Wad_FileClose, 0048bfc0) are the
 *   port's own functions, whose contracts match; resource.hpp gives them C
 *   linkage in place.
 * - The other four are unannotated adapters in gods98/files_port.cpp: the
 *   port's functions at those addresses take or return other things
 *   (Resource records, no return value), and wwad_archive_index_load is
 *   declared only in a frozen header with C++ linkage. Wad.c's conversion
 *   replaces them.
 *
 * Shared C++ headers do not include this file; src/game/test_c_boundaries.cpp
 * checks the two direct entries against resource.hpp. */
#ifndef LEGORRR_GODS98_WAD_H
#define LEGORRR_GODS98_WAD_H

#ifdef __cplusplus
extern "C" {
#endif

/* Wad_Load (0048b4e0): mount one archive; its number, or -1. Adapter. */
int wad_load(const char* fName);

/* Wad_IsFileInWad (0048bd50): the entry's index in archive wadNo, or in the
 * newest archive that has it when wadNo is -1; -1 if none. */
int wad_entry_find_across_mounted_archives(const char* name, int wad_idx);

/* Wad_FileOpen (0048bf70): open an entry into a stream handle; -1 on
 * failure. Adapter. */
int wad_file_open(const char* fName, int wadNo);

/* Wad_FileClose (0048bfc0): free a stream handle's data. */
void fixed_record_slot_resource_release(int slot);

/* Wad_FileLength (0048c010) and Wad_FileData (0048c030): a stream handle's
 * length and data. Adapters. */
int wad_file_length(int handle);
void* wad_file_data(int handle);

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GODS98_WAD_H */
