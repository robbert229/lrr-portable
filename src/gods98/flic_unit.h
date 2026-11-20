// SPDX-License-Identifier: LGPL-3.0-or-later
/* Flic.c (original translation unit G98-016, finding 632): gods98's FLIC
 * (.flh) player. Flic_Setup opens a file into a FLICSTRUCT and a
 * system-memory DirectDraw surface; Flic_Animate decodes the next frame into
 * the surface and blits it to the back buffer.
 *
 * C-compatible header (ADR 0005). flic_unit.c is compiled as C in the
 * matching build. The native port has no DirectDraw, so the unit compiles to
 * nothing there and the port's player (flic.hpp, flic.cpp, flic_port.cpp)
 * serves every caller. Shared C++ headers do not include this file: every
 * include and declaration a header adds shifts MSVC's symbol numbering
 * (finding 506). src/game/test_c_boundaries.cpp includes it.
 *
 * The names are the unit's own, after gods98's Flic_* convention. In the
 * matching build seven of them take the port's names (flic_link_names.h). */
#ifndef LEGORRR_GODS98_FLIC_UNIT_H
#define LEGORRR_GODS98_FLIC_UNIT_H

#include "gods98/flic_record.h"

#ifdef __cplusplus
extern "C" {
#endif

int Flic_Setup(const char* filename, LPFLICSTRUCT* fsp,
               unsigned int flags);                            /* 00483c80 */
int Flic_Close(LPFLICSTRUCT fsp);                               /* 00483f00 */
int Flic_LoadHeader(const char* filename, LPFLICSTRUCT* fsp);   /* 00483f60 */
int Flic_Animate(LPFLICSTRUCT fsp, const struct Area2F* destArea,
                 int advance, int trans);                      /* 00484070 */
int Flic_Memory(LPFLICSTRUCT fsp);                              /* 004841d0 */
int Flic_Load(LPFLICSTRUCT fsp);                                /* 00484260 */
int Flic_FindChunk(LPFLICSTRUCT fsp);                           /* 00484320 */
int Flic_FrameChunk(LPFLICSTRUCT fsp);                          /* 004844b0 */
int Flic_DoChunk(LPFLICSTRUCT fsp);                             /* 00484610 */
int Flic_LoadPointers(LPFLICSTRUCT fsp);                        /* 00484720 */
int Flic_LoadPalette64(LPFLICSTRUCT fsp);                       /* 004847d0 */
int Flic_Copy(LPFLICSTRUCT fsp);                                /* 00484880 */
int Flic_BrunDepack(LPFLICSTRUCT fsp);                          /* 004848d0 */
int Flic_BrunDepack16(LPFLICSTRUCT fsp);                        /* 004849d0 */
int Flic_Brun(LPFLICSTRUCT fsp);                                /* 00484b20 */
int Flic_Unsupported(LPFLICSTRUCT fsp);                         /* 00484b90 */
void Flic_Palette256To64k(LPFLICSTRUCT fsp);                    /* 00484ba0 */
int Flic_Colour256(LPFLICSTRUCT fsp);                           /* 00484c00 */
void Flic_Nothing(LPFLICSTRUCT fsp);                            /* 00484c90 */
void Flic_DeltaWord(LPFLICSTRUCT fsp);                          /* 00484ca0 */
void Flic_DeltaWord16(LPFLICSTRUCT fsp);                        /* 00484e50 */
int Flic_Delta(LPFLICSTRUCT fsp);                               /* 00485030 */
unsigned short Flic_GetPaletteColour(unsigned char index,
                                     LPFLICSTRUCT fsp);         /* 004850c0 */
int Flic_GetWidth(LPFLICSTRUCT fsp);    /* folded into 004897d0 */
int Flic_GetHeight(LPFLICSTRUCT fsp);                           /* 004850e0 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GODS98_FLIC_UNIT_H */
