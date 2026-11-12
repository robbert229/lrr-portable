// SPDX-License-Identifier: LGPL-3.0-or-later
/* C boundary header (ADR 0005) for the gods98 file helpers (Files.c,
 * G98-014) that gods98/sound3d.c calls, beyond read_whole_file in
 * gods98/resource.h, under the port's names. They are not added to
 * resource.h because config.c includes it, and every declaration shifts its
 * symbol numbering (finding 538); they left gods98/files.h when it became
 * the unit's header (finding 583). In the matching build gods98/files.c
 * defines both names (files.h); natively they are the port tail's
 * (gods98/files_port.cpp). Shared C++ headers do not include this file;
 * src/game/test_c_boundaries.cpp checks it against resource.hpp. */
#ifndef LEGORRR_GODS98_FILES_PATHS_H
#define LEGORRR_GODS98_FILES_PATHS_H

#ifdef __cplusplus
extern "C" {
#endif

/* File_VerifyFilename (FUN_00480220): the path under the data root, or
 * NULL. */
const char* resource_path_resolve_against_base(const char* path);

/* File_GetCDFilePath (FUN_0047f660) with the original's argument order:
 * format name's CD path into out (MAX_PATH bytes); 0 when no CD drive is
 * configured. Natively an adapter at the end of files_port.cpp: the port's
 * ares_cd_fallback_path takes (name, out, size). */
int resource_cd_file_path_get(char* out, const char* name);

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GODS98_FILES_PATHS_H */
