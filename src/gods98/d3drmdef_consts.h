// SPDX-License-Identifier: LGPL-3.0-or-later
/* d3drmdef.h's four static consts (finding 524): D3DRMMAP_WRAPU,
 * D3DRMMAP_WRAPV, D3DRMMAP_PERSPCORRECT and D3DRMGROUP_ALLGROUPS. Every
 * original object that includes d3drm.h carries its own unreferenced copy,
 * 16 bytes of .rdata (finding 505 counts 20). Units that do not include
 * the SDK header take this file instead; where it sits in the unit sets
 * where the copy lands in the unit's .rdata. */
#ifndef LEGORRR_GODS98_D3DRMDEF_CONSTS_H
#define LEGORRR_GODS98_D3DRMDEF_CONSTS_H

static const unsigned long d3drmdef_map_wrap_u = 1;
static const unsigned long d3drmdef_map_wrap_v = 2;
static const unsigned long d3drmdef_map_persp_correct = 4;
static const long d3drmdef_group_all_groups = -1;

#endif /* LEGORRR_GODS98_D3DRMDEF_CONSTS_H */
