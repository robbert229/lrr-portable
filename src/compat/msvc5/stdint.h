/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* Fixed-width integer types for the 32-bit MSVC 5.0 target. */
#ifndef LRR_CLASSIC_MSVC5_STDINT_H
#define LRR_CLASSIC_MSVC5_STDINT_H

typedef signed char        int8_t;
typedef unsigned char      uint8_t;
typedef signed short       int16_t;
typedef unsigned short     uint16_t;
typedef signed int         int32_t;
typedef unsigned int       uint32_t;
typedef signed __int64     int64_t;
typedef unsigned __int64   uint64_t;
typedef unsigned int       uintptr_t;

/* Translate compiler/library conveniences absent from the VC5 frontend. */
/* Keep this header safe to include from Windows SDK headers: declarations
 * from the CRT belong to the source files that use them. */
#define __builtin_memcpy memcpy
#define __builtin_memset memset
#define __builtin_sqrtf(value) ((float)sqrt((double)(value)))
#define snprintf _snprintf
#define strtof(value, end) ((float)strtod((value), (end)))

/* The reconstruction expects round-to-nearest-even for its grid conversion. */
static float lrr_msvc5_nearbyintf(float value)
{
    int integral = (int)value;
    float fraction = value - (float)integral;
    if (fraction > 0.5f || (fraction == 0.5f && (integral & 1))) {
        ++integral;
    } else if (fraction < -0.5f ||
               (fraction == -0.5f && (integral & 1))) {
        --integral;
    }
    return (float)integral;
}
#define __builtin_nearbyintf(value) lrr_msvc5_nearbyintf(value)

/* C++11 static_assert replacement; a false condition creates a bad array. */
#define static_assert(condition, message) \
    typedef char lrr_msvc5_static_assertion[(condition) ? 1 : -1]

#endif /* LRR_CLASSIC_MSVC5_STDINT_H */
