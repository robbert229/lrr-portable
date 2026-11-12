// SPDX-License-Identifier: LGPL-3.0-or-later
/* gods98 resource layer: whole-file loading.
 *
 * C boundary header (ADR 0005): C declarations, with C linkage, of the
 * resource functions that units rebuilt as C call. The C++ code keeps
 * using gods98/resource.hpp, whose matching declarations carry extern "C";
 * src/game/test_c_boundaries.cpp includes both and fails to compile if they
 * disagree. Shared C++ headers do not include this file: every include and
 * declaration a header adds shifts MSVC's symbol numbering, and with it the
 * code generated for unrelated functions (finding 506). */
#ifndef LEGORRR_GODS98_RESOURCE_H
#define LEGORRR_GODS98_RESOURCE_H

/* read_whole_file is File_LoadBinary. The port's version is in
 * resource.cpp, so files.c cannot take its name (files.h); in the matching
 * build the C units that include this header call files.c's function
 * instead, whose contract theirs is (finding 583). A macro renames the
 * declaration and the calls one for one: the symbol numbering does not
 * change. */
#if defined(_MSC_VER) && _MSC_VER < 1200
#define read_whole_file file_load_binary
#endif

#ifdef __cplusplus
extern "C" {
#endif

void* read_whole_file(const char* path, int* size_out); /* FUN_004800a0 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GODS98_RESOURCE_H */
