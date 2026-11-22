// SPDX-License-Identifier: LGPL-3.0-or-later
/* An original object whose code the link removed, between Message.c
 * (GAME-027) and the tutorial unit (GAME-028): a "data" entry in
 * src/link_order.cmake, placed before GAME-028. It keeps only what
 * /OPT:REF does not remove: its literal pool, the 1.0f at 0049e528 that
 * nothing reads, and its .bss, 1,720 bytes at 004ff2a0-004ff958 that
 * nothing in the image reads. It has no text, so it has no unit range in
 * docs/coverage/translation-unit-ranges.csv, and its name is not known.
 *
 * The evidence:
 * - The original has three never-read 1.0f literals at 0049e520-0049e528
 *   between Map3D.c's pool (0049e4e4-0049e51c) and GAME-028's (0049e52c).
 *   Each is one object's Maths.h 1.0 (gods98/maths_normalise_head.h;
 *   finding 693), and MSVC 5.0 pools a literal once an object, so three
 *   objects lie there: MeshLOD.c (GAME-053), Message.c and this one.
 * - No other object fits. Map3D.c's .bss ends at 004eaf20, where
 *   Message.c's begins. Message.c's functions all read its one static
 *   (004eaf20-004ff2a0). GAME-028 is one object: its function table
 *   (004a5948) is read by 00453320, whose 'Func Call %s' is the last of the
 *   unit's .data (004a7f7c), after the data that 00453900-00456830 read.
 * - The .bss between Message.c's and GAME-028's (004ff2a0-004ff958) has no
 *   reader. An object whose code is all unreferenced would leave exactly
 *   this: /OPT:REF removes functions, not data or literals.
 *
 * Nothing calls the unit and it declares nothing, so it has no header. */
#include "gods98/maths_normalise_head.h"

/* A zero-initialised external, so MSVC 5.0 emits it in .bss (finding 686).
 * The original's layout of the 1,720 bytes is not known. */
// GLOBAL: LEGORR 0x004ff2a0
unsigned int message_data_unknown_4ff2a0[430] = { 0 }; /* never read */
