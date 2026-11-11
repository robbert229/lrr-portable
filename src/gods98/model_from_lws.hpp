// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what model.cpp uses from lws.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by model.cpp only. */
#ifndef LRR_GODS98_MODEL_FROM_LWS_HPP
#define LRR_GODS98_MODEL_FROM_LWS_HPP

extern int s_model_resource_primary_count;
extern int s_model_resource_secondary_count;

#endif /* LRR_GODS98_MODEL_FROM_LWS_HPP */
