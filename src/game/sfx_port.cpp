// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-041 SFX.c port tail (finding 585): the port's models of the unit,
 * unannotated. The original-shape unit is sfx.c; these models keep the
 * port's contracts (the port's alias hashing and chain pool, NULL guards and
 * bounds, the smoke emitters' renderer hooks and 64-bit link table) for the
 * native port, which calls them by these names.
 *
 * In the matching build (MSVC 5.0) sfx.c defines these names itself
 * (sfx_link_names.h), so their definitions here are compiled out there;
 * the state that sound.cpp reads stays defined in every build. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include "resource.hpp"
#include "heap.hpp"
#include "sound.hpp"
#include "audio.hpp"
#include "string.hpp"
#include "record.hpp"
#include "view.hpp"
#include "block.hpp"
#include "game/util.hpp"
#include <float.h>
#include <errno.h>
#include "gods98/texture.hpp"
#include "crt/crt.hpp"
#include "crt/crt_for_sfx.hpp"
#include "effects.hpp"
#include "combat_ray_ports.hpp"
#include "entity.hpp"
#include "frontend.hpp"
#include "engine.hpp"
#include "ai.hpp"
#include "simtick.hpp"
#include "gods98/scene_pool.hpp"
#include "gods98/scene_resources.hpp"
#include "gods98/surface.hpp"
#include "level.hpp"
#include "weapon.hpp"
#include "bezier.h"
#include <stddef.h>
#include "effects_for_sfx.hpp"
#include "gods98/sound_for_sfx.hpp"

/* Port model of 0x00502a24 */
int   s_alias_dynamic_count;

/* Port model of 0x00502a44 */
BatchEntry s_batch_entry[SOUND_BATCH_QUEUE_CAPACITY];
/* Port model of 0x00502b20 */
int        s_batch_count;   /* DAT_00502b20 */
/* Port model of 0x00502b24 */
float s_cooldown_timer;   /* _DAT_00502b24 (zero-initialised) */
/* Port model of 0x00502b28 */
int   s_cooldown_handle;  /* DAT_00502b28  (zero-initialised) */
/* _DAT_00502b2c — the global-cooldown word (the corpus types it as a
 * double; it holds the alias submitted under the cooldown, whose writer
 * lands with #296). */
/* Port model of 0x00502b2c */
double s_cooldown_alias;

/* Port model of 0x00502b30 */
int s_queue_alias[SOUND_REQUEST_QUEUE_CAPACITY];  /* DAT_00502b30 */
/* Port model of 0x00502b58 */
int s_queue_loop[SOUND_REQUEST_QUEUE_CAPACITY];   /* DAT_00502b58 */

/* The frame tick's active replay copies (DAT_00502b80 alias words,
 * DAT_00502ba8 loop words, count DAT_00502bd4). */
/* Port model of 0x00502b80 */
int s_active_alias[SOUND_REQUEST_QUEUE_CAPACITY];
/* Port model of 0x00502ba8 */
int s_active_loop[SOUND_REQUEST_QUEUE_CAPACITY];
/* Port model of 0x00502bd0 */
int s_sound_queue_count;                                /* DAT_00502bd0 */

/* Port model of 0x00502bd4 */
int s_active_count;
/* sfx.c owns these names in the matching build (sfx_link_names.h), and
 * the state below only serves them. */
#if !(defined(_MSC_VER) && _MSC_VER < 1200)

/* 0x49e73c's role: the 0.0 constant the duration and timer read. */
static const float s_deferred_zero = 0.0f;

/* The corpus's DAT_00502a2c sound flag word: bit 0 subsystem-enabled
 * (enable-set @ 0x465260), bit 1 dynamic-alias registration, bit 3 batch
 * mode (batch-mode-get @ 0x4652b0). */
/* Port model of 0x00502a2c */
static uint32_t s_sound_flags;

/* SoundAliasLookup compares the GameStringHashCompute result, not the
 * original text. Retain the hashes in slot order so collisions resolve to
 * the first registered alias as they do in the reference. */
static uint32_t* s_alias_hashes;
static int s_alias_hashes_initialized;

static int         s_variant_pool_next;   /* DAT_00502a28 */
/* Sample variant selection shares the game's VC5 rand stream. */
extern "C" short rng_thunk_draw15(void);

/* Defined below (forward declarations added by tools/relocate_unit.py). */
static int sound_alias_builtin_hashes_init(void);
static uint32_t sound_alias_hash_compute(const char* name);
static void sound_alias_scene_sample_callback(const char* sample_name, void* scene_object, void* context);
static char* s_dupstr(const char* s);
static int s_variant_pool_alloc(void);
static int s_variant_rand(void);
static void s_subsystem_stop_all(void);
static void* transient_effect_scene_object_get(void* instance);
static void* transient_effect_next_get(void* instance);
static void* transient_effect_previous_get(void* instance);
static void* transient_effect_render_record_get(void* instance);

/* Port model of 0x00464740 */
void sound_alias_registry_init(void)
{
    /* Keep the dynamic count and names, as the reference does, but rebuild
     * their hash slots along with the 45 built-ins. Reinitialization must
     * not leave the retained aliases represented by zeroed/uninitialized
     * words in the replacement 0x7bc-byte table. */
    uint32_t* replacement = (uint32_t*)engine_heap_alloc(0x7bcL);
    int index;
    if (replacement != NULL) {
        uint32_t* previous = s_alias_hashes;
        s_alias_hashes = replacement;
        s_alias_hashes_initialized = 0;
        if (sound_alias_builtin_hashes_init()) {
            for (index = 0; index < s_alias_dynamic_count; ++index) {
                s_alias_hashes[SOUND_ALIAS_BUILTINS + index] =
                    sound_alias_hash_compute(s_alias_dynamic[index]);
            }
            if (previous != NULL) engine_heap_free(previous);
        } else {
            s_alias_hashes = previous;
            engine_heap_free(replacement);
        }
    }

    /* SoundAliasRegistryInit registers LAB_00464c20 through the scene sample
     * callback globals. Keep the existing owner callback and add the sound
     * resolver as an independent, idempotent subscriber. */
    (void)ddiscene_sample_hook_add(sound_alias_scene_sample_callback, NULL);
}

/* Port model of 0x00464c50 */
void sound_alias_registration_set(int enable)
{
    /* SoundDynamicAliasRegistrationSet @ 0x00464c50: raises/clears the
     * value-2 bit of DAT_00502a2c (`|= 2` / `&= 0xfffffffd`). */
    if (enable != 0) {
        s_sound_flags |= SOUND_FLAG_REG_UNKNOWN;
    } else {
        s_sound_flags &= ~SOUND_FLAG_REG_UNKNOWN;
    }
}

/* Port model of 0x00464c70 */
int sound_alias_lookup(const char* name, int* out_index)
{
    /* SoundAliasLookup @ 0x00464c70 compares GameStringHashCompute values
     * and returns the first matching slot. */
    if (name == NULL || out_index == NULL) {
        return 0; /* corpus's param_1 != 0 guard */
    }
    if (!sound_alias_builtin_hashes_init()) return 0;
    int i;
    uint32_t hash = sound_alias_hash_compute(name);
    for (i = 0; i < SOUND_ALIAS_BUILTINS + s_alias_dynamic_count; ++i) {
        if (s_alias_hashes[i] == hash) {
            *out_index = i;
            return 1;
        }
    }
    if ((s_sound_flags & SOUND_FLAG_REG_UNKNOWN) != 0) {
        if (s_alias_dynamic_count >= SOUND_ALIAS_MAX - SOUND_ALIAS_BUILTINS) {
            return 0; /* port bound: the corpus writes past its 0x7bc
                       * table instead of failing */
        }
        int index = SOUND_ALIAS_BUILTINS + s_alias_dynamic_count;
        char* copy = s_dupstr(name);
        if (copy == NULL) {
            return 0;
        }
        s_alias_hashes[index] = hash;
        s_alias_dynamic[s_alias_dynamic_count] = copy;
        ++s_alias_dynamic_count;
        *out_index = index;
        return 1;
    }
    return 0;
}

/* Port model of 0x00464d00 */
int sound_sample_marker_parse(const char* spec, int alias_index)
{
    if (spec == NULL || alias_index < 0 || alias_index >= SOUND_ALIAS_MAX) {
        return 0; /* corpus: NULL deref / out-of-bounds scribble */
    }

    /* Chain reset: the head's next word (DAT_0050146c) zeroes — the head
     * slot itself keeps its value until the first write. */
    s_variant_next[alias_index] = 0;

    char* buf;
    size_t spec_length = strlen(spec);
    size_t token_capacity = 1u;
    const char* scan;
    /* Keep the config value immutable, but size the copy from the complete
     * sample list so long lists are not truncated at the adapter's old
     * 1023-byte limit. */
    buf = (char*)malloc(spec_length + 1u);
    if (buf == NULL) return 0;
    memcpy(buf, spec, spec_length + 1u);
    for (scan = spec; *scan != '\0' && scan[1] != '\0'; ++scan) {
        if (*scan == ',') ++token_capacity;
    }
    if (token_capacity > SOUND_VARIANT_TOKENS) {
        free(buf);
        return 0;
    }
    char* tokens[SOUND_VARIANT_TOKENS];
    int ntok = sound_variant_split(buf, tokens, ",");

    int stream_mode = 0;            /* '@' marker, sticky (finding 135) */
    int ok = 1;

    /* Corpus cursors: W (piVar9) is the link written next, C (local_1d4)
     * the previous link. W starts at the chain head, C starts null. */
    int w_head = 1, w_id = alias_index;
    int c_head = 0, c_id = 0;
    int c_valid = 0;

    for (int t = 0; t < ntok; ++t) {
        char* p = tokens[t];
        int looping = (p[0] == '*'); /* '*' = duplicate-buffer request */
        if (looping) {
            ++p;
        }
        int volume = 0;
        if (p[0] == '#') {           /* '#N#' millibel marker, finding 142 */
            ++p;
            char digits[64];         /* corpus local_1d0[64] */
            size_t d = 0;
            if (*p != '#') {         /* empty '#...#' parses as 0 (the
                                     * corpus reads its stack garbage) */
                for (; *p != '\0' && *p != '#'; ++p) {
                    if (d < sizeof(digits) - 1) {
                        digits[d++] = *p;
                    }
                }
                if (*p == '\0') {
                    /* Unterminated '#': the corpus walks past the NUL and
                     * parses its stack; the port parses what it captured
                     * and loads the (empty) tail as the path. */
                }
            }
            digits[d] = '\0';
            if (*p == '#') {
                ++p;                 /* step over the closing '#' */
            }
            volume = atoi(digits); /* VC5 _atoi at FUN_0048db70 */
        }
        if (p[0] == '@') {
            stream_mode = 1;
            ++p;
        }

        int slot = sound_bank_entry_load(p, stream_mode, looping, volume);
        if (w_head) {
            s_variant_slot[w_id] = slot;   /* *piVar9 = iVar5 */
        } else {
            s_variant_pool[w_id].slot = slot;
        }

        if (slot == -1) {
            ok = 0;  /* failed link stays linked with its -1 (as shipped);
                      * both cursors stay where they were */
            continue;
        }
        if (c_valid) {
            int node = s_variant_pool_alloc();
            if (node != 0) {
                if (c_head) {
                    s_variant_next[c_id] = node;
                } else {
                    s_variant_pool[c_id].next = node;
                }
                w_head = 0;
                w_id   = node;
            }
            /* Pool full: the link is dropped (the corpus scribbles past
             * the pool region instead). */
        }
        c_valid = 1;
        c_head  = w_head;
        c_id    = w_id;
    }
    free(buf);
    return ok;
}

/* Port model of 0x00464e20 */
int sound_variant_select(int alias_index)
{
    if (alias_index <= 0 || alias_index >= SOUND_ALIAS_MAX) {
        return -1; /* corpus: alias 0 (the silent alias) returns -1 and
                    * negatives scribble out of bounds */
    }
    if (s_variant_next[alias_index] != 0) {
        int count = 1;
        for (int node = s_variant_next[alias_index]; node != 0;
             node = s_variant_pool[node].next) {
            ++count;
        }
        int pick = s_variant_rand() % count;
        int head_slot = s_variant_slot[alias_index];
        if (pick != 0) {
            int node = s_variant_next[alias_index];
            for (int i = 1; i < pick; ++i) {
                node = s_variant_pool[node].next;
            }
            int tmp = s_variant_pool[node].slot;
            s_variant_pool[node].slot = head_slot;
            head_slot = tmp; /* corpus swaps picked slot with head */
        }
        s_variant_slot[alias_index] = head_slot;
    }
    return s_variant_slot[alias_index];
}
/* Port model of 0x00464e80 */
void sound_deferred_play_expire(void)
{
    /* DeferredSoundPlayExpire @ 0x464e80. */
    if (s_cooldown_handle != -1 && s_cooldown_timer > 0.0f) {
        /* SoundHandleStopForward -> SoundBufferStopHandle. The portable
         * bank validates the handle and stops that duplicate buffer's
         * active voice. */
        sound_handle_stop_forward(s_cooldown_handle);
        s_cooldown_handle = -1;
        s_cooldown_timer = 0.0f;
    }
}
/* Port model of 0x00464ec0 */
int sound_deferred_timer_set_if_idle(float seconds)
{
    /* DeferredSoundTimerSetIfIdle @ 0x464ec0. */
    if (s_cooldown_timer <= s_deferred_zero) {
        s_cooldown_handle = -1;
        s_cooldown_timer = seconds;
        return 1;
    }
    return 0;
}

/* Port model of 0x00464ef0 */
int sound_submit_with_global_cooldown(int alias, int* out_handle)
{
    /* SoundSubmitWithGlobalCooldown @ 0x464ef0. */
    if (s_cooldown_timer <= 0.0f) {
        /* The target stores the encoded handle returned by
         * SoundBufferPlayWithPosition through the SoundAliasPlay call. */
        int handle = sound_alias_play(alias, 0);

        if (handle != -1) {
            s_cooldown_alias = (double)alias;   /* _DAT_00502b2c */
            s_cooldown_timer =
                (float)(sound_object_duration_get(alias) * 25.0);
            s_cooldown_handle = handle;         /* DAT_00502b28 */
            if (out_handle != NULL) {
                *out_handle = handle;
            }
            return 1;
        }
    }
    return 0;
}
/* Port model of 0x00464f60 */
void sound_request_queue_submit(int alias, int loop)
{
    /* SoundRequestQueueSubmit @ 0x464f60. */
    /* 0x464f68 uses the unsigned `jae` full gate. The private count starts at
     * zero and only this bounded submit path increments it. */
    if ((unsigned int)s_sound_queue_count >=
        (unsigned int)SOUND_REQUEST_QUEUE_CAPACITY) {
        return; /* corpus drop-on-full: no counter bump, no error */
    }
    s_queue_alias[s_sound_queue_count] = alias;
    s_queue_loop[s_sound_queue_count] = loop;
    ++s_sound_queue_count;
}

/* Port model of 0x00464fa0 */
int sound_alias_play(int alias, int loop)
{
    /* SoundAliasPlay @ 0x464fa0. */
    if ((s_sound_flags & SOUND_FLAG_BATCH) == 0) {
        if (alias != 0 && (s_sound_flags & SOUND_FLAG_ENABLED) != 0) {
            int slot = sound_variant_select(alias);
            /* Corpus quirk kept: a chain head reading 0 — a real bank
             * slot, or the unwritten head — counts as "no sound". */
            if (slot != 0) {
                return s_play_seam(2, 0, slot, loop, NULL);
            }
        }
    } else if (s_batch_count < SOUND_BATCH_QUEUE_CAPACITY) {
        /* Deferred alias form writes only the alias and clears the mode
         * word's positional bit — the loop bit (and position words)
         * keep whatever an earlier batch record left in the slot
         * (corpus quirk: the deferred alias form never carries a loop
         * flag). */
        s_batch_entry[s_batch_count].alias = alias;
        s_batch_entry[s_batch_count].mode &= ~1u;
        ++s_batch_count;
        return -1;
    }
    return -1;
}
/* Port model of 0x00465010 */
void sound_variant_property_set(int alias_index, int millibels)
{
    /* ConfiguredVariantPropertySet @ 0x465010. Keep the selector nested in
     * the volume setter's first argument: MSVC 5 /Od evaluates the setter's
     * right argument first, staging millibels before calling the selector.
     * The corpus loads [esp+8] then [esp+4], pushes value then alias, calls
     * the selector, and reuses the staged value for the volume write. */
    sound_bank_entry_volume_set(sound_variant_select(alias_index), millibels);
}

/* Port model of 0x00465030 */
void sound_variant_property_read(int alias_index)
{
    /* ConfiguredVariantPropertyRead @ 0x465030: only the variant draw
     * is observable — the corpus reads IndexedRecordStride134WordGet's
     * word (@ 0x47b0d0, via sound_bank_entry_volume) but discards it. */
    int slot = sound_variant_select(alias_index);
    (void)sound_bank_entry_volume(slot);
}

/* Port model of 0x00465050 */
int sound_submission_route_wrapper(const void* wrapped_position, int alias,
                                    int loop, int positional,
                                    const float* xyz)
{
    int position = 0;
    if (wrapped_position != NULL) {
        position = (int)scene_object_wrapped_pointer_get(wrapped_position);
    }
    return sound_positional_sound_submit(position, alias, loop, positional, xyz);
}
/* Port model of 0x00465090 */
int sound_positional_sound_submit(int position, int alias, int loop,
                                  int positional, const float* xyz)
{
    /* PositionalSoundSubmit @ 0x465090. */
    if ((s_sound_flags & SOUND_FLAG_BATCH) == 0) {
        if (alias != 0 && (s_sound_flags & SOUND_FLAG_ENABLED) != 0) {
            int slot = sound_variant_select(alias);
            if (slot != 0) {
                if (positional != 0) {
                    return s_play_seam(0, position, slot, loop, NULL);
                } else {
                    return s_play_seam(1, 0, slot, loop, xyz);
                }
            }
        }
    /* ORIG 0x4650a4 also uses `jae`; the queue count is bounded by both
     * append gates and reset by replay drain @ 0x4652d0. */
    } else if ((unsigned int)s_batch_count <
               (unsigned int)SOUND_BATCH_QUEUE_CAPACITY) {
        BatchEntry* e = &s_batch_entry[s_batch_count];
        ++s_batch_count;
        e->alias = alias;
        e->position = position;
        e->mode = (uint32_t)((((positional & 1) << 1) | (loop & 1)) << 1)
                | (e->mode & 0xfffffff9u) | 1u;
        if (xyz != NULL) {
            e->xyz[0] = xyz[0];
            e->xyz[1] = xyz[1];
            e->xyz[2] = xyz[2];
        }
    }
    return 0;
}

/* Port model of 0x00465160 */
double sound_object_duration_get(int alias)
{
    /* SoundObjectDurationGet @ 0x465160: duration of whatever the alias's
     * chain selects — one PRNG draw per call (a global-cooldown submit
     * therefore draws twice: play's select and this one, as corpus). The
     * corpus tests the select (test eax,eax / je 0x46517b): a zero slot
     * never reaches the step-rate, it loads the 0x49e73c zero instead. */
    int slot = sound_variant_select(alias);
    if (slot != 0) {
        return sound_streamed_wave_buffer_step_rate(slot);
    }
    return s_deferred_zero; /* the corpus's test/je tail: a zero chain head
                             * (no sound) loads the 0x49e73c zero instead */
}
/* Port model of 0x00465190 */
void sound_handle_stop_forward(int buffer_handle)
{
    sound_buffer_stop_handle(buffer_handle);
}

/* Port model of 0x004651a0 */
void sound_subsystem_frame_tick(float dt)
{
    /* SoundSubsystemFrameTick @ 0x4651a0. */
    const int batch_saved = sound_batch_mode_get();
    int i;
    s_cooldown_timer -= dt;            /* _DAT_00502b24 step */
    sound_batch_queue_drain(0, 0);     /* batch off for the replay pass */
    /* Replay pass: the corpus re-reads the active count every iteration
     * (0x4651e2) and gates it unsigned, so neither is hoisted. */
    for (i = 0; i < (unsigned) s_active_count; ++i) {
        sound_alias_play(s_active_alias[i], s_active_loop[i]);
    }
    if (batch_saved != 0) {
        sound_deferred_queue_drain(1); /* restore batch mode */
    }
    /* Rotate the pending queue (#317) into the active replay copies:
     * two rep-movs passes over the pending count, then the count moves
     * and the pending count clears. */
    for (i = 0; i < (unsigned) s_sound_queue_count; ++i) {
        s_active_alias[i] = s_queue_alias[i];
    }
    for (i = 0; i < (unsigned) s_sound_queue_count; ++i) {
        s_active_loop[i] = s_queue_loop[i];
    }
    s_active_count = s_sound_queue_count;
    s_sound_queue_count = 0;
    /* SoundSourceSpatialRefreshAll @ 0x47b390: refresh attached sources,
     * remove stale associations, and advance both streamed-wave slots. */
    sound_source_spatial_refresh_all();
}
/* Port model of 0x00465260 */
void sound_subsystem_enable_set(int enable, int stop_active)
{
    /* SoundSubsystemEnableSet @ 0x00465260. */
    if (enable != 0) {
        s_sound_flags |= SOUND_FLAG_ENABLED;
        return;
    }
    /* Corpus disable order: the flag word is read once, bit 0 cleared and
     * stored back before stop_active is tested — the store precedes the
     * stop call, and both sub-paths then zero the cooldown word. */
    uint32_t cleared = s_sound_flags & ~SOUND_FLAG_ENABLED;
    s_sound_flags = cleared;
    if (stop_active != 0) {
        s_subsystem_stop_all();
    }
    s_cooldown_alias = 0.0;
}

/* Port model of 0x004652a0 */
void sound_subsystem_enable_set_and_stop(int enable)
{
    /* SoundSubsystemEnableSetAndStop @ 0x004652a0. */
    sound_subsystem_enable_set(enable, 1);
}

/* Port model of 0x004652b0 */
int sound_batch_mode_get(void)
{
    /* SoundBatchModeGet @ 0x004652b0: `return DAT_00502a2c & 8` — the
     * corpus returns the masked word, so 8 (not 1) when set. */
    return (int)(s_sound_flags & SOUND_FLAG_BATCH);
}

/* Port model of 0x004652c0 */
void sound_deferred_queue_drain(int batch_set)
{
    /* SoundDeferredQueueDrain @ 0x4652c0. */
    sound_batch_queue_drain(batch_set, 1);
}
/* Port model of 0x004652d0 */
void sound_batch_queue_drain(int batch_set, int replay)
{
    /* SoundBatchQueueDrain @ 0x4652d0. */
    if (batch_set != 0) {
        s_sound_flags |= SOUND_FLAG_BATCH; /* raise the bit and leave */
        return;
    }
    s_sound_flags &= ~SOUND_FLAG_BATCH;
    if (replay == 0) {
        return;
    }
    /* The corpus uses an unsigned zero gate and an unsigned back edge
     * (0x465309 jbe / 0x46535a jb). Both append sites cap this private
     * count at SOUND_BATCH_QUEUE_CAPACITY, so it remains nonnegative. */
    for (unsigned int i = 0; i < (unsigned int)s_batch_count; ++i) {
        const BatchEntry* e = &s_batch_entry[i];
        const uint32_t m = e->mode;
        if ((m & 1u) == 0) {
            /* Deferred alias form replays with the stale loop bit. */
            sound_alias_play(e->alias, (int)((m << 30) >> 31));
        } else {
            sound_positional_sound_submit(e->position, e->alias,
                                          (int)((m << 30) >> 31),
                                          (int)((m << 29) >> 31),
                                          (const float*)e->xyz);
        }
    }
    s_batch_count = 0;
}
/* Port model of 0x00465370 */
int sound_subsystem_enabled(void)
{
    return (s_sound_flags & SOUND_FLAG_ENABLED) != 0;
}

/* Port model of 0x00465380 */
int scene_effect_registry_reset(void)
{
    s_scene_effect_head = 0u;    /* corpus clears in address order */
    s_scene_effect_tail = 0u;
    s_scene_effect_shared = 0u;
    return 1;
}

/* Port model of 0x004653a0 */
void transient_effect_textures_preload(const char* filename_prefix,
                                      const char* filename_format,
                                      uint32_t texture_count)
{
    uint32_t i;
    if (s_transient_effect_hooks.textures_preload != NULL) {
        s_transient_effect_hooks.textures_preload(
            s_transient_effect_hooks.context, filename_prefix,
            filename_format, texture_count);
        return;
    }

    /* FUN_004653a0 keeps one cache reference per numbered bitmap.  The
     * executable's table is a DAT global; this port owns the same references
     * here until the next preload replaces the table. */
    for (i = 0; i < s_transient_effect_texture_count; ++i) {
        if (s_transient_effect_textures[i] != NULL)
            texture_release(s_transient_effect_textures[i]);
        s_transient_effect_textures[i] = NULL;
    }
    s_transient_effect_texture_count = texture_count > 12u
        ? 12u : texture_count;
    for (i = 0; i < s_transient_effect_texture_count; ++i) {
        char filename[260];
        if (filename_prefix == NULL || filename_format == NULL) break;
        sprintf(filename, filename_format, filename_prefix, (int)i);
        s_transient_effect_textures[i] = texture_get_or_load(NULL, filename);
    }
    if (i < s_transient_effect_texture_count)
        s_transient_effect_texture_count = i;
}

/* Renderer-owned resource creation stays behind the narrow record and mesh
 * hooks; the emitter record setup, simulation warm-up, and registry do not. */
/* Port model of 0x00465430 */
void* transient_effect_emitter_create(void* scene_context,
                                      uint32_t position_x,
                                      uint32_t position_y, int* direction,
                                      float red, float green, float blue,
                                      float rate, int parameter_9, int mode,
                                      uint32_t seed, int auxiliary)
{
    static const uint16_t mesh_indices[6] = { 0u, 3u, 2u, 2u, 1u, 0u };
    unsigned char* bytes;
    uint32_t* words;
    const void* terrain = NULL;
    void* scene_object = NULL;
    void* render_record = NULL;
    uint32_t slot_count;
    unsigned int slot;
    float center[2];
    float position_x_world = 0.0f;
    float position_y_world = 0.0f;
    float position_z_world = 0.0f;
    float dx, dy, dz, inverse_length;
    void* instance;
    int owns_scene_object = 0;
    int render_failed = 0;

    instance = engine_heap_alloc(0x2a8L);
    if (instance == NULL || direction == NULL) {
        if (instance != NULL) engine_heap_free(instance);
        return NULL;
    }
    memset(instance, 0, 0x2a8u);
    bytes = (unsigned char*)instance;
    words = (uint32_t*)instance;

    if (mode == 0 || mode == 2) {
        GameContext* game;
        float block_size;
        int block_size_integer;
        int cell_x, cell_y;
        int sample_x, sample_y;
        uint32_t jitter_x, jitter_y;
        if (s_transient_effect_hooks.current_terrain_get != NULL)
            terrain = s_transient_effect_hooks.current_terrain_get(
                s_transient_effect_hooks.context);
        else {
            /* FUN_00465430 reads the active level's terrain handle. When no
             * renderer override is installed, GameContext owns that handle. */
            game = entity_pool_callback_context_get();
            if (game != NULL && game->level != NULL)
                terrain = game->level->ptr_terrain;
        }
        if (terrain == NULL) {
            engine_heap_free(instance);
            return NULL;
        }
        block_size = terrain_blocksize_get(terrain);
        block_size_integer = crt_float_to_long(block_size);
        if (block_size_integer <= 0 ||
            !terrain_cell_world_center_xy_get(
                terrain, (int)position_x, (int)position_y, center)) {
            engine_heap_free(instance);
            return NULL;
        }
        jitter_x = ((seed % 0x109eu) % (uint32_t)block_size_integer);
        jitter_y = ((seed % 0x5ebu) % (uint32_t)block_size_integer);
        position_x_world = (float)jitter_x - block_size * 0.5f + center[0];
        position_y_world = (float)jitter_y - block_size * 0.5f + center[1];
        if (terrain_world_xy_to_cell(terrain, position_x_world,
                                     position_y_world, &sample_x, &sample_y)) {
            position_z_world = (float)terrain_surface_height_get(
                terrain, sample_x, sample_y);
        }
        else {
            sample_x = (int)position_x;
            sample_y = (int)position_y;
            position_z_world = (float)terrain_surface_height_get(
                terrain, sample_x, sample_y);
        }
        scene_object = lego::ddiscene_object_allocate(
            (const lego::DDiSceneObjectDescriptor*)scene_root_object_get());
        if (scene_object == NULL) {
            engine_heap_free(instance);
            return NULL;
        }
        owns_scene_object = 1;
        (void)ddiscene_object_position_set(scene_object, NULL,
                                           position_x_world,
                                           position_y_world,
                                           position_z_world);
        words[0x97] = (uint32_t)crt_float_to_long(position_x_world);
        words[0x98] = (uint32_t)crt_float_to_long(position_y_world);
        words[0x99] = (uint32_t)crt_float_to_long(position_z_world);
        words[0xa5] = 2u;
        if ((seed & 3u) == 0u) words[0xa8] = (uint32_t)auxiliary;
    }
    else if (mode == 1) {
        scene_object = scene_context;
        words[0xa5] = 4u;
    }
    else {
        engine_heap_free(instance);
        return NULL;
    }

    dx = (float)direction[0];
    dy = (float)direction[1];
    dz = (float)direction[2];
    inverse_length = 1.0f / (float)sqrt(dx * dx + dy * dy + dz * dz);
    words[0x9a] = (uint32_t)(int)(dx * inverse_length);
    words[0x9b] = (uint32_t)(int)(dy * inverse_length);
    words[0x9c] = (uint32_t)(int)(dz * inverse_length);
    words[0x9d] = (uint32_t)(int)red;
    words[0x9e] = (uint32_t)(int)green;
    words[0x9f] = (uint32_t)(int)blue;

    if (mode == 2) {
        words[0xa0] = 4u;
        words[0xa1] = 0u;
        words[0xa5] |= 8u;
        words[0xa2] = 0x40000000u;
    }
    else {
        if ((0.0f < rate) || (rate < 1.0f))
            slot_count = (uint32_t)crt_float_to_long(rate);
        else
            slot_count = 10u;
        words[0xa0] = slot_count;
        words[0xa2] = (uint32_t)parameter_9;
    }
    if (scene_object == NULL ||
        s_transient_effect_hooks.render_record_create == NULL) {
        if (owns_scene_object) ddiscene_object_destroy(scene_object);
        engine_heap_free(instance);
        return NULL;
    }
    render_record = s_transient_effect_hooks.render_record_create(
        s_transient_effect_hooks.context,
        *(void**)((unsigned char*)scene_object + 4), 0x00465ee0u,
        0x4800u, instance);
    if (render_record == NULL) {
        if (owns_scene_object) ddiscene_object_destroy(scene_object);
        engine_heap_free(instance);
        return NULL;
    }
    transient_effect_instance_handles_set(instance, scene_object,
                                           render_record);

    if (words[0xa0] != 0u) {
        if (s_transient_effect_texture_count == 0u) {
            render_failed = 1;
        }
        for (slot = 0; !render_failed && slot < words[0xa0]; ++slot) {
            int render_slot = mesh_record_allocate_or_append(
                render_record, 4u, 2u, 3u, mesh_indices);
            if (render_slot < 0) {
                render_failed = 1;
                break;
            }
            lego::animated_texture_frame_surface_set(
                render_record, render_slot,
                s_transient_effect_textures[
                    slot % s_transient_effect_texture_count]);
            (void)render_object_color_vector_set(
                render_record, render_slot, red, green, blue, 0);
            (void)render_object_color_vector_set(
                render_record, render_slot, red * 0.2f, green * 0.2f,
                blue * 0.2f, 3);
            indexed_record_flag6c_bit0_set(render_record, render_slot, 1);
            {
                uint32_t random_value = (uint32_t)level_rand_u16();
                int32_t* particle = (int32_t*)(bytes +
                    (size_t)render_slot * 0x3cu);
                float first = (float)(random_value % 4u) + 6.0f;
                random_value = (uint32_t)level_rand_u16();
                particle[10] = (int32_t)first;
                particle[11] = (int32_t)((float)(random_value % 4u) +
                                           first + 2.0f);
                if ((words[0xa5] & 8u) == 0u)
                    particle[10] = (int32_t)((float)particle[10] * 0.5f);
                else
                    particle[11] = (int32_t)((float)particle[11] * 2.0f);
            }
        }
    }
    if (render_failed) {
        if (s_transient_effect_hooks.render_record_release != NULL)
            s_transient_effect_hooks.render_record_release(
                s_transient_effect_hooks.context, render_record);
        if (owns_scene_object) ddiscene_object_destroy(scene_object);
        engine_heap_free(instance);
        return NULL;
    }

    *(float*)(bytes + 0x258) = 0.0f;
    if (mode != 2) {
        int frame;
        for (frame = 0; frame < 10; ++frame) {
            if (*(float*)(bytes + 0x258) <= 0.0f) {
                for (slot = 0; slot < words[0xa0]; ++slot) {
                    if (*(uint32_t*)(bytes + (size_t)slot * 0x3cu) == 0u) {
                        (void)transient_effect_particle_slot_initialize(
                            instance, slot);
                        break;
                    }
                }
            }
            *(float*)(bytes + 0x258) -= 5.0f;
            for (slot = 0; slot < words[0xa0]; ++slot) {
                if (*(uint32_t*)(bytes + (size_t)slot * 0x3cu) != 0u)
                    transient_effect_segment_advance(instance, slot, 5.0f);
            }
        }
    }
    {
        TransientEffectInstanceLinks* links =
            transient_effect_links_add(instance);
        TransientEffectInstanceLinks* tail =
            transient_effect_links_find(s_transient_effect_tail);
        if (links == NULL) {
            if (s_transient_effect_hooks.render_record_release != NULL)
                s_transient_effect_hooks.render_record_release(
                    s_transient_effect_hooks.context, render_record);
            if (owns_scene_object) ddiscene_object_destroy(scene_object);
            if (s_transient_effect_hooks.instance_release != NULL)
                s_transient_effect_hooks.instance_release(
                    s_transient_effect_hooks.context, instance, NULL);
            else
                engine_heap_free(instance);
            return NULL;
        }
        links->next = NULL;
        links->previous = tail;
        transient_effect_link_pointer_set(instance, 0x298u, NULL);
        transient_effect_link_pointer_set(instance, 0x29cu,
                                          s_transient_effect_tail);
        if (tail != NULL) {
            tail->next = links;
            transient_effect_link_pointer_set(tail->instance, 0x298u,
                                              instance);
        }
        else
            s_transient_effect_head = instance;
    }
    if (s_transient_effect_tail == NULL)
        s_transient_effect_head = instance;
    s_transient_effect_tail = instance;
    return instance;
}

/* Port model of 0x00465970 */
void transient_effect_registry_release_all(void)
{
    while (s_transient_effect_head != NULL) {
        void* instance = s_transient_effect_head;
        void* resource = transient_effect_scene_object_get(instance);
        transient_effect_instance_release(instance, resource);
    }
    s_transient_effect_head = NULL;
    s_transient_effect_tail = NULL;
}
/* Port model of 0x004659b0 */
void transient_effect_instance_release(void* instance, void* resource)
{
    unsigned char* bytes;
    void* render_record;
    uint32_t sound_handle;
    void* previous;
    void* next;
    TransientEffectInstanceLinks* links;
    TransientEffectInstanceLinks* all_previous;
    if (instance == NULL) return;
    bytes = (unsigned char*)instance;
    /* The reference selects the retained resource only for instances whose
     * ownership bit is set; otherwise its second argument is the resource
     * whose render instance must be detached. */
    if ((bytes[0x294] & 2u) != 0u)
        resource = transient_effect_scene_object_get(instance);
    render_record = transient_effect_render_record_get(instance);
    memcpy(&sound_handle, bytes + 0x2a4u, sizeof(sound_handle));
    links = transient_effect_links_find(instance);
    next = transient_effect_next_get(instance);
    previous = transient_effect_previous_get(instance);
    if (next != NULL) {
        TransientEffectInstanceLinks* next_links =
            transient_effect_links_find(next);
        if (next_links != NULL) next_links->previous = links != NULL
            ? links->previous : NULL;
        transient_effect_link_pointer_set(next, 0x29cu, previous);
    }
    else s_transient_effect_tail = previous;
    if (previous != NULL) {
        TransientEffectInstanceLinks* previous_links =
            transient_effect_links_find(previous);
        if (previous_links != NULL) previous_links->next = links != NULL
            ? links->next : NULL;
        transient_effect_link_pointer_set(previous, 0x298u, next);
    }
    else s_transient_effect_head = next;
    if (links != NULL) {
        all_previous = NULL;
        if (s_transient_effect_links == links)
            s_transient_effect_links = links->all_next;
        else {
            all_previous = s_transient_effect_links;
            while (all_previous != NULL && all_previous->all_next != links)
                all_previous = all_previous->all_next;
            if (all_previous != NULL) all_previous->all_next = links->all_next;
        }
    }
    /* FUN_004659b0 detaches the emitter render instance before releasing the
     * owned scene object, then stops its latched sound handle before freeing
     * the emitter record. */
    if (render_record != NULL &&
        s_transient_effect_hooks.render_record_release != NULL) {
        s_transient_effect_hooks.render_record_release(
            s_transient_effect_hooks.context, render_record);
    }
    if ((bytes[0x294] & 2u) != 0u && resource != NULL)
        ddiscene_object_destroy(resource);
    if (sound_handle != 0u)
        sound_handle_stop_forward((int)sound_handle);
    if (s_transient_effect_hooks.instance_release != NULL) {
        s_transient_effect_hooks.instance_release(
            s_transient_effect_hooks.context, instance, resource);
    }
    else {
        engine_heap_free(instance);
    }
    if (links != NULL) free(links);
}

/* Port model of 0x00465a90 */
void transient_effect_registry_enable_set(int enabled)
{
    void* instance;
    s_transient_effect_enabled = enabled != 0;
    for (instance = s_transient_effect_head; instance != NULL;
         instance = transient_effect_next_get(instance)) {
        transient_effect_enable_set(instance, s_transient_effect_enabled);
    }
}

/* Port model of 0x00465ac0 */
void transient_effect_enable_set(void* instance, int enabled)
{
    unsigned char* bytes;
    uint32_t* flags;
    if (instance == NULL) return;
    bytes = (unsigned char*)instance;
    flags = (uint32_t*)(bytes + 0x294);
    record_flag2c_bit0_set(transient_effect_render_record_get(instance),
                            enabled);
    if (enabled != 0) *flags |= 1u;
    else *flags &= ~1u;
}
/* Port model of 0x00465b00 */
void transient_effect_instances_tick(float delta)
{
    void* instance = s_transient_effect_head;
    while (instance != NULL) {
        unsigned char* bytes = (unsigned char*)instance;
        void* next = transient_effect_next_get(instance);
        uint32_t flags;
        int all_inactive = 1;
        uint32_t slot_count;
        uint32_t slot;
        if ((*(uint32_t*)(bytes + 0x294) & 0x20u) == 0u &&
            *(uint32_t*)(bytes + 0x2a4) == 0u) {
            if (*(uint32_t*)(bytes + 0x2a0) != 0u) {
                int sound = sound_positional_sound_submit(0,
                    (int)*(uint32_t*)(bytes + 0x2a0), 1, 0,
                    (const float*)(bytes + 0x25c));
                *(uint32_t*)(bytes + 0x2a4) = (uint32_t)sound;
            }
            if (*(uint32_t*)(bytes + 0x2a4) == 0u)
                *(uint32_t*)(bytes + 0x294) |= 0x20u;
        }
        flags = *(uint32_t*)(bytes + 0x294);
        if ((flags & 1u) == 0u) {
            if ((flags & 8u) == 0u || (flags & 0x10u) == 0u) {
                if (*(float*)(bytes + 0x258) <= 0.0f) {
                    slot_count = *(uint32_t*)(bytes + 0x280);
                    for (slot = 0u; slot < slot_count; ++slot) {
                        if (*(uint32_t*)(bytes + (size_t)slot * 0x3cu) == 0u) {
                            transient_effect_particle_slot_initialize(
                                instance, slot);
                            break;
                        }
                    }
                }
                *(float*)(bytes + 0x258) -= delta;
            }
            slot_count = *(uint32_t*)(bytes + 0x280);
            for (slot = 0u; slot < slot_count; ++slot) {
                if (*(uint32_t*)(bytes + (size_t)slot * 0x3cu) != 0u) {
                    transient_effect_segment_advance(instance, slot, delta);
                    all_inactive = 0;
                }
            }
            flags = *(uint32_t*)(bytes + 0x294);
            if ((flags & 0x18u) == 0x18u && all_inactive)
                transient_effect_instance_release(instance,
                    transient_effect_scene_object_get(instance));
        }
        instance = next;
    }
}
/* Port model of 0x00465c50 */
int transient_effect_particle_slot_initialize(void* emitter,
                                              uint32_t slot)
{
    unsigned char* bytes = (unsigned char*)emitter;
    uint32_t* slot_words;
    uint32_t random_value;
    float random_vector[3];
    float position[3] = { 0.0f, 0.0f, 0.0f };
    float z;
    float scale;
    float speed;
    int result;
    if (emitter == NULL) return 0;
    slot_words = (uint32_t*)(bytes + (size_t)slot * 0x3cu);
    if ((*(uint32_t*)(bytes + 0x294) & 8u) != 0u) {
        uint32_t count = *(uint32_t*)(bytes + 0x284);
        if (count == 3u) *(uint32_t*)(bytes + 0x294) |= 0x10u;
        *(uint32_t*)(bytes + 0x284) = count + 1u;
    }
    slot_words[0] = 1u;
    indexed_record_flag6c_bit0_set(transient_effect_render_record_get(emitter),
                                   (int)slot, 0);
    d3drm_vector_random(random_vector);
    z = -fabsf(random_vector[2]) * 0.17f - 0.83f;
    ((float*)slot_words)[1] = random_vector[0] * 0.17f;
    ((float*)slot_words)[2] = random_vector[1] * 0.17f;
    ((float*)slot_words)[3] = z;
    z += *(float*)(bytes + 0x270);
    speed = *(float*)(bytes + 0x288);
    scale = -speed / z;
    z *= scale;
    ((float*)slot_words)[4] = scale *
        (*(float*)(bytes + 0x268) + random_vector[0] * 0.17f);
    ((float*)slot_words)[5] = scale *
        (*(float*)(bytes + 0x26c) + random_vector[1] * 0.17f);
    ((float*)slot_words)[6] = z;
    *(uint32_t*)(bytes + 0x294) = 4u;
    (void)ddiscene_object_position_get(
        transient_effect_scene_object_get(emitter), NULL, position);
    ((float*)slot_words)[7] = position[0];
    ((float*)slot_words)[8] = position[1];
    ((float*)slot_words)[9] = position[2];
    slot_words[12] = 0u;
    slot_words[14] = 0u;
    random_value = (uint32_t)level_rand_u16();
    slot_words[13] = random_value % 0x1eu + 0x28u;
    *(float*)(bytes + 600) = (0.5f / speed) *
                              ((float*)slot_words)[11];
    result = (int)(random_value / 0x1eu);
    return result;
}

/* Port model of 0x00465e00 */
void transient_effect_segment_advance(void* emitter, uint32_t slot,
                                      float delta)
{
    unsigned char* bytes = (unsigned char*)emitter;
    float* particle;
    float distance;
    void* resource;
    if (emitter == NULL) return;
    particle = (float*)(bytes + (size_t)slot * 0x3cu);
    resource = transient_effect_render_record_get(emitter);
    particle[7] += particle[4] * delta;
    particle[8] += particle[5] * delta;
    particle[9] += particle[6] * delta;
    distance = fabsf(particle[6] * delta) + particle[14];
    particle[14] = distance;
    if ((float)(int)particle[13] <= distance) {
        *(uint32_t*)particle = 0u;
        indexed_record_flag6c_bit0_set(resource, (int)slot, 1);
        return;
    }
    particle[12] += delta;
    (void)screen_effect_item_opacity_set(resource, (int)slot,
        0.7f - (distance / (float)(int)particle[13]) * 0.7f, 4);
}

/* InstanceQuadVerticesBuild (FUN_00465f40): use the particle's current
 * center/size/distance and scene orientation to build its four corners.
 * Hosts may override the orientation lookup, while the normal scene object
 * path uses the same pose interface as the retail +0x7c call. */
/* Port model of 0x00465f40 */
void transient_effect_instance_quad_vertices_build(
    void* instance, uint32_t slot, void* transform_source,
    const float triple[3])
{
    static const uint32_t uv_pairs[8] = {
        0u, 0x3f800000u, 0u, 0u,
        0x3f800000u, 0u, 0x3f800000u, 0x3f800000u
    };
    unsigned char* bytes = (unsigned char*)instance;
    float* particle;
    float orientation[3];
    float side[3];
    float up[3] = { 0.0f, 0.0f, -1.0f };
    float scale;
    float side_scale;
    float up_scale;
    float positions[12];
    void* record;
    int axis;

    if (instance == NULL || triple == NULL) return;
    if (slot >= *(uint32_t*)(bytes + 0x280u)) return;
    particle = (float*)(bytes + (size_t)slot * 0x3cu);
    record = transient_effect_render_record_get(instance);
    if (record == NULL) return;
    if (s_transient_effect_quad_basis_hook != NULL) {
        if (!s_transient_effect_quad_basis_hook(
                s_transient_effect_quad_basis_context, transform_source,
                orientation)) return;
    }
    else if (!ddiscene_object_facing_get(transform_source, NULL,
                                         orientation)) return;

    side[0] = orientation[1];
    side[1] = -orientation[0];
    side[2] = 0.0f;
    if (orientation[2] > 0.3f)
        d3drm_vector_rotate(up, up, side, 0.7f);

    scale = particle[14] / (float)(int)particle[13] + 1.0f;
    side_scale = particle[10] * scale;
    up_scale = particle[11] * scale;
    for (axis = 0; axis < 3; ++axis) {
        float center = particle[7 + axis];
        side[axis] *= side_scale;
        up[axis] *= up_scale;
        positions[axis] = center + side[axis];
        positions[3 + axis] = center - side[axis];
        positions[6 + axis] = positions[3 + axis] - up[axis];
        positions[9 + axis] = positions[axis] - up[axis];
    }
    instance_quad_vertex_data_fill(record, (int)slot, 0, 4, positions,
                                   triple, uv_pairs);
}

static char* s_dupstr(const char* s)
{
#if defined(_MSC_VER)
    /* Keep this allocation in the VC5 CRT heap; the alias table releases it
     * with free(). The runtime's _strdup performs the same NUL-inclusive
     * copy as the portable path below. */
    return _strdup(s);
#else
    size_t n = strlen(s) + 1;
    char* copy = (char*)calloc(n, 1);
    if (copy != NULL) {
        memcpy(copy, s, n);
    }
    return copy;
#endif
}

static uint32_t sound_alias_hash_compute(const char* name)
{
    const unsigned char* cursor = (const unsigned char*)name;
    uint32_t hash = 0u;
    uint32_t factor = 0x00600937u;
    uint32_t position = 1u;
    while (*cursor != 0u) {
        uint32_t value = (uint32_t)(unsigned char)toupper((int)*cursor);
        hash += position * factor * value;
        factor *= 0x00600937u;
        ++position;
        ++cursor;
    }
    return hash;
}

static int sound_alias_builtin_hashes_init(void)
{
    int index;
    if (s_alias_hashes_initialized != 0) return 1;
    if (s_alias_hashes == NULL) {
        s_alias_hashes = (uint32_t*)engine_heap_alloc(0x7bcL);
        if (s_alias_hashes == NULL) return 0;
    }
    for (index = 0; index < SOUND_ALIAS_BUILTINS; ++index)
        s_alias_hashes[index] = sound_alias_hash_compute(s_alias_builtins[index]);
    s_alias_hashes_initialized = 1;
    return 1;
}

static void sound_alias_scene_sample_callback(const char* sample_name,
                                              void* scene_object,
                                              void* context)
{
    int alias;
    (void)context;
    if (sound_alias_lookup(sample_name, &alias))
        sound_submission_route_wrapper(scene_object, alias, 0, 1, NULL);
}

/* SoundSubsystemStopAll @ 0x47b160 — the corpus's disable seam. */
static void s_subsystem_stop_all(void)
{
    sound_subsystem_stop_all();
}

static int s_variant_rand(void)
{
    /* FUN_00464e20 calls the same 0x48e170 generator used throughout level
     * and simulation code; keep sample selection on that shared stream. */
    return (int)rng_thunk_draw15();
}

static int s_variant_pool_alloc(void)
{
    if (s_variant_pool_next >= SOUND_VARIANT_POOL_NODES) {
        return 0; /* port bound; the corpus scribbles past the pool */
    }
    ++s_variant_pool_next;
    s_variant_pool[s_variant_pool_next].next = 0; /* DAT_005023e4 write */
    /* The slot field keeps its previous content: unwritten links read as
     * stale chain data, exactly like the corpus's global region. */
    return s_variant_pool_next;
}

static void* transient_effect_scene_object_get(void* instance)
{
    TransientEffectInstanceLinks* links =
        transient_effect_links_find(instance);
    if (links != NULL && links->scene_object != NULL)
        return links->scene_object;
    if (instance != NULL && sizeof(void*) == 4u)
        return *(void**)((unsigned char*)instance + 0x28c);
    return NULL;
}

static void* transient_effect_render_record_get(void* instance)
{
    TransientEffectInstanceLinks* links =
        transient_effect_links_find(instance);
    if (links != NULL && links->render_record != NULL)
        return links->render_record;
    if (instance != NULL && sizeof(void*) == 4u)
        return *(void**)((unsigned char*)instance + 0x290);
    return NULL;
}

static void* transient_effect_next_get(void* instance)
{
    TransientEffectInstanceLinks* links =
        transient_effect_links_find(instance);
    if (links != NULL) return links->next != NULL ? links->next->instance : NULL;
    if (instance != NULL && sizeof(void*) == 4u)
        return *(void**)((unsigned char*)instance + 0x298);
    return NULL;
}

static void* transient_effect_previous_get(void* instance)
{
    TransientEffectInstanceLinks* links =
        transient_effect_links_find(instance);
    if (links != NULL)
        return links->previous != NULL ? links->previous->instance : NULL;
    if (instance != NULL && sizeof(void*) == 4u)
        return *(void**)((unsigned char*)instance + 0x29c);
    return NULL;
}

#endif /* !(MSVC 5.0) */

/* Original-ABI adapter (ADR 0005 step 6) for GAME-021's lego_unit.c: the
 * original ConfiguredVariantPropertyRead (FUN_00465030) returns the
 * variant's volume; the port's entry above discards it. In the matching
 * build aitask_port_aliases.h points lego_unit.c at sfx.c's 00465030. */
#if !(defined(_MSC_VER) && _MSC_VER < 1200)
// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (sound_variant_select, sound_bank_entry_volume)
extern "C" int sound_variant_property_read_original(int alias_index)
{
    return sound_bank_entry_volume(sound_variant_select(alias_index));
}
#endif

/* Mesh_LoadTexture (FUN_00482e70) for sfx.c's Smoke_LoadTextures. The
 * port's get-or-load entry (texture.cpp) takes the directory and the name
 * and has C++ linkage; it reports no size. */
// PORT-ONLY: original-ABI adapter for GAME-041 sfx.c (00482e70; the port's entry takes other parameters)
extern "C" void* sfx_mesh_load_texture_adapter(const char* basePath,
                                               const char* name,
                                               unsigned int* width,
                                               unsigned int* height)
{
    (void)width;
    (void)height;
    return texture_get_or_load(basePath, name);
}

/* Mesh_SetGroupTexture (FUN_004830c0) for sfx.c's Smoke_CreateSmokeArea.
 * The port's entry has C++ linkage in namespace lego. */
// PORT-ONLY: original-ABI adapter for GAME-041 sfx.c (004830c0; C++ linkage in namespace lego)
extern "C" void sfx_mesh_set_group_texture_adapter(void* mesh, int group,
                                                   void* texture)
{
    lego::animated_texture_frame_surface_set(mesh, group,
                                             (const void*)texture);
}
