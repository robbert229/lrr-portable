// SPDX-License-Identifier: LGPL-3.0-or-later
/* Header inline copy that closes Maths.c's object (finding 694).
 *
 * A ghost d3drm import: a member whose only reference is code /OPT:REF
 * removed. Its slot takes part in LINK's qsort of the import table and is
 * then dropped. The original's IAT order needs one after Maths.c's
 * D3DRMVectorRandom. Maths.c wraps D3DRMVector* functions; which one the
 * removed wrapper called is not known, and D3DRMVectorModulus is a
 * placeholder.
 *
 * As in sound3d_tail.h (finding 693), an unused `extern __inline` copy
 * carries the reference and /OPT:REF removes its code. Nothing calls it.
 * Only the matching build needs it. */
#ifndef LEGORRR_GODS98_MATHS_TAIL_H
#define LEGORRR_GODS98_MATHS_TAIL_H
#if defined(_MSC_VER) && _MSC_VER < 1200
float __stdcall D3DRMVectorModulus(float* v);
extern __inline float maths_tail_modulus(float* v) { return D3DRMVectorModulus(v); }
#endif
#endif
