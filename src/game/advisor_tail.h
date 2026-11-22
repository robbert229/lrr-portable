// SPDX-License-Identifier: LGPL-3.0-or-later
/* Header inline copies that close Advisor.c's object (finding 693).
 *
 * In the original, LIBC's strstr.obj, atox.obj, free.obj, malloc.obj and
 * realloc.obj come straight after sprintf.obj, ahead of AITask.c's qsort,
 * although Advisor.c's code calls none of them and AITask.c's calls only
 * free, malloc and qsort. MSVC 5.0 emits an unused `extern __inline`
 * function as a pick-any COMDAT after the object's own functions, with its
 * external references; /OPT:REF then removes the code but LINK has already
 * pulled the members. These copies stand for the original headers' inline
 * helpers whose names are not known; only the order of their references
 * matters, and nothing calls them. The unit ranges already record header
 * inlines closing Advisor.c (translation-unit-ranges.csv). */
#ifndef LEGORRR_GAME_ADVISOR_TAIL_H
#define LEGORRR_GAME_ADVISOR_TAIL_H
#include <stdlib.h>
#include <string.h>
extern __inline int advisor_tail_find(const char* s, const char* t) { return strstr(s, t) != NULL; }
extern __inline int advisor_tail_int(const char* s) { return atoi(s); }
extern __inline void advisor_tail_free(void* p) { free(p); }
extern __inline void* advisor_tail_alloc(unsigned int n) { return malloc(n); }
extern __inline void* advisor_tail_realloc(void* p, unsigned int n) { return realloc(p, n); }
#endif
