// SPDX-License-Identifier: LGPL-3.0-or-later
/* gods98 container loading for units rebuilt as C.
 *
 * C boundary header (ADR 0005). The port's 0x00472cd0 (entity_assembly_parse)
 * has a different contract from the original Container_Load, so this header
 * declares an unannotated adapter with the original signature, defined in
 * gods98/entity.cpp (ADR 0005 step 6). Shared C++ headers do not include
 * this file (finding 506). */
#ifndef LEGORRR_GODS98_ENTITY_H
#define LEGORRR_GODS98_ENTITY_H


#ifdef __cplusplus
extern "C" {
#endif

/* Container_Load's signature: load `filename` as a `typeString` container
 * under `parent`. The port has no such loader yet; the adapter returns
 * NULL. */
void* container_load(void* parent, const char* filename,
                     const char* typeString, int looseLoad);

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GODS98_ENTITY_H */
