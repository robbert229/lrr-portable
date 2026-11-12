// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what sound3d uses from sound.cpp.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by both files. */
#ifndef LRR_GODS98_SOUND_FOR_SOUND3D_HPP
#define LRR_GODS98_SOUND_FOR_SOUND3D_HPP

/* Bank record mirrors the corpus stride and the offsets used by its sound
 * routines: name at +0, volume at +0x110, three buffers at +0x120, the
 * rotating buffer index at +0x12c, and flags at +0x130. The reserved words
 * retain the original spacing around the port-side sample and buffer data. */
typedef struct BankSlot {
    char     name[260];      /* stored "<base>.wav" resource name */
    Sound*   sound;          /* loaded payload, NULL until loaded */
    uint8_t  reserved_108[8];
    int      volume;         /* +0x110: validated millibels */
    uint8_t  reserved_114[12];
    AudioBuffer* buffers[3]; /* portable duplicate-buffer state */
    int      next_buffer;    /* +0x12c */
    uint32_t flags;          /* SOUND_SLOT_* + SOUND_LOOPING/STREAMED */
} BankSlot;

extern BankSlot s_bank[SOUND_BANK_SLOTS];
int s_has_wav_suffix(const char* name);
extern SoundPositionApplyHook s_sound_position_hook;
extern void* s_sound_position_context;
extern SoundInstanceRegisterHook s_instance_register_hook;
extern SoundInstanceStopHook s_instance_stop_hook;
extern void* s_instance_hook_context;
extern SoundSpatialRefreshHook s_spatial_refresh_hook;
extern void* s_spatial_refresh_context;

#endif /* LRR_GODS98_SOUND_FOR_SOUND3D_HPP */
