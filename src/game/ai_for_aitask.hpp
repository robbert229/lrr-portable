// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what aitask uses from ai.cpp.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by both files. */
#ifndef LRR_GAME_AI_FOR_AITASK_HPP
#define LRR_GAME_AI_FOR_AITASK_HPP

/* ActiveEntityListContainsCode (FUN_004031e0): a permitted-activity scan
 * (FUN_0044b480) over the selection when it is non-empty, else over the
 * corpus's global active-entity list. The selection branch caches the
 * count (the corpus's ebx), walks the record array with a pointer and an
 * unsigned counter (add reg,4 / inc / cmp-jb) and passes entries BLIND to
 * the scan — the corpus has no NULL-entry test there, the scan itself
 * answers 0; the fallback branch skips NULL entries (the corpus's
 * test/je) and re-reads the count every iteration. The selection branch
 * answers on the first hit and never falls back to the list
 * (pinned branch isolation).
 * The global list mirrors the entity pool's create/teardown lifecycle; the
 * explicit install adapter remains available to replace it for callers that
 * have a more specific active set. The 50-entry host bound matches the
 * corpus region; the corpus strides a NULL selection entry into the activity
 * scan, while the port's scan answers 0 for it (guarded). */
#define ACTIVE_ENTITY_LIST_MAX 50 /* corpus region DAT_004b335c..3424 */

#endif /* LRR_GAME_AI_FOR_AITASK_HPP */
