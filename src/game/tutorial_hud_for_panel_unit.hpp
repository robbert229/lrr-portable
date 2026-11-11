// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what panel_unit uses from tutorial_hud.cpp.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by both files. */
#ifndef LRR_GAME_TUTORIAL_HUD_FOR_PANEL_UNIT_HPP
#define LRR_GAME_TUTORIAL_HUD_FOR_PANEL_UNIT_HPP

typedef struct LegacyPanelPortRecord {
    float initial[4];
    float current[4];
    uint32_t flags;
    uint32_t image_flags;
    void* image;
    int image_mode;
    int image_texture_owned;
    uint32_t selection_flags[64];
    /* Mirrors the original per-entry value word (record entry +0x24),
       cleared by the interaction-state reset alongside selection_flags. */
    uint32_t selection_values[64];
    uint32_t selection_count;
} LegacyPanelPortRecord;

extern LegacyPanelPortRecord s_legacy_panels[18];

#endif /* LRR_GAME_TUTORIAL_HUD_FOR_PANEL_UNIT_HPP */
