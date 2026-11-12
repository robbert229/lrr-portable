// SPDX-License-Identifier: LGPL-3.0-or-later
/* C boundary header (ADR 0005) for the gods98 file stream entries (Files.c,
 * G98-014; the port's gods98/files.cpp) that gods98/lws.c calls:
 * File_Open, File_Close and File_GetLine. They are not added to
 * gods98/files.h or gods98/resource.h because other C units include them,
 * and every declaration shifts their symbol numbering (finding 538). Shared
 * C++ headers do not include this file; src/game/test_c_boundaries.cpp
 * checks it against resource.hpp. */
#ifndef LEGORRR_GODS98_FILES_OPEN_H
#define LEGORRR_GODS98_FILES_OPEN_H

#ifdef __cplusplus
extern "C" {
#endif

struct GameFileHandle;

/* File_Close (FUN_0047fac0). */
int game_file_close(struct GameFileHandle* handle);

/* File_Open (FUN_0047f6e0): open from the WAD files or the data
 * directory; NULL on failure. */
struct GameFileHandle* game_file_open_dispatch(const char* path,
                                               const char* mode);

/* File_GetLine (FUN_00480050): the next line without its line break;
 * NULL at the end of the file. */
char* game_file_read_crlf_line(char* buffer, int capacity,
                               struct GameFileHandle* handle);

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GODS98_FILES_OPEN_H */
