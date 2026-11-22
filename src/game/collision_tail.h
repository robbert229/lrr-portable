// SPDX-License-Identifier: LGPL-3.0-or-later
/* Header inline copies that close Collision.c's object (finding 693).
 *
 * In the original, LIBC's rand.obj and 87fmod.obj come straight after
 * acos.obj (Collision.c's own reference), ahead of FrontEnd.c's vsprintf,
 * although nothing before FrontEnd.c calls rand and only LegoObject.c calls
 * fmod. As in advisor_tail.h, unused `extern __inline` copies carry the
 * references; /OPT:REF removes their code. Their original names are not
 * known; nothing calls them. */
#ifndef LEGORRR_GAME_COLLISION_TAIL_H
#define LEGORRR_GAME_COLLISION_TAIL_H
#include <math.h>
#include <stdlib.h>
extern __inline int collision_tail_rand(void) { return rand(); }
extern __inline double collision_tail_mod(double a, double b) { return fmod(a, b); }
#endif
