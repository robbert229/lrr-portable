// SPDX-License-Identifier: LGPL-3.0-or-later
/* C boundary header (ADR 0005) for the RNC payload expander that Wad.c
 * (gods98/wad.c, G98-030) calls: resource.cpp's wad_expand_payload
 * (0049c7c0, LATE-001), which resource.hpp gives C linkage in place.
 * Only wad.c includes it; src/game/test_c_boundaries.cpp checks it against
 * resource.hpp. */
#ifndef LEGORRR_GODS98_WAD_PAYLOAD_H
#define LEGORRR_GODS98_WAD_PAYLOAD_H

#ifdef __cplusplus
extern "C" {
#endif

/* Expand an RNC-packed entry into a new heap buffer (*out_payload); the
 * expanded size, 0 on failure. */
long wad_expand_payload(const void* header, void** out_payload);

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GODS98_WAD_PAYLOAD_H */
