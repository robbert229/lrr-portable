// SPDX-License-Identifier: LGPL-3.0-or-later
/* G98-009 G98-009: the unit's functions in original address order and the
 * state they own, relocated verbatim by tools/relocate_unit.py (ADR 0006).
 * Body hashes: docs/coverage/relocations/G98-009.csv. */
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
#include <limits.h>
#include <stdarg.h>
#include "scene_pool.hpp"
#include "message.hpp"
#include "simtick.hpp"
#include "game/level.hpp"
#include "game/ai.hpp"
#include "game/cmd.hpp"
#include "game/select.hpp"
#include "game/entity.hpp"
#include "game/effects.hpp"
#include "game/render.hpp"
#include "game/terrain_grid.hpp"
#include "scene_resources.hpp"
#include "surface.hpp"
#include "model.hpp"
#include "sound_for_sound3d.hpp"
#include "audio_for_sound3d.hpp"
#include "simtick_for_sound3d.hpp"
#include "audio_from_sound3d.hpp"

typedef struct SoundInstanceHostRecord {
    void* provider;
    void* instance;
    AudioBuffer* buffer;
    struct SoundInstanceHostRecord* next;
} SoundInstanceHostRecord;

/* The association record stores its interface in word 2 and its next node
 * in word 3. These setters mirror the target's vtable dispatch while using
 * the host-widened record stride established by SoundBufferAssociationAdd. */
class SoundTrackedBufferInterface
{
public:
    virtual void slot_00(void) = 0;
    virtual void slot_04(void) = 0;
    virtual void slot_08(void) = 0;
    virtual void slot_0c(void) = 0;
    virtual void slot_10(void) = 0;
    virtual void slot_14(void) = 0;
    virtual void slot_18(void) = 0;
    virtual void slot_1c(void) = 0;
    virtual void slot_20(void) = 0;
    virtual void slot_24(void) = 0;
    virtual void slot_28(void) = 0;
    virtual void slot_2c(void) = 0;
    virtual void slot_30(void) = 0;
    virtual void slot_34(void) = 0;
    virtual void slot_38(void) = 0;
    virtual void slot_3c(void) = 0;
    virtual void max_distance_set(uint32_t value, uint32_t zero) = 0;
    virtual void min_distance_set(uint32_t value, uint32_t zero) = 0;
};

namespace unported_ledger_stubs {
// NestedRecordListRemoveMatchingKey
typedef struct NestedResourceRecord {
    void* key;
    void* reserved[3];
    struct NestedResourceRecord* child_or_next;
} NestedResourceRecord;
} // namespace unported_ledger_stubs

// GLOBAL: LEGORR 0x004aae30
const int s_sfx_attenuation[11] = {
    -10000, -5000, -3500, -2000, -1500, -1000,
      -700,  -400,  -200,   -80,     0
};

/* SoundBufferAssociationAdd (FUN_0047b020) — see sound.hpp. */
// GLOBAL: LEGORR 0x00506684
void* s_buffer_assoc_head; /* DAT_00506684 role: unowned */

// GLOBAL: LEGORR 0x00506688
static int s_listener_view = 0;

// GLOBAL: LEGORR 0x0050668c
static uint32_t s_tracked_buffer_min_distance;
// GLOBAL: LEGORR 0x00506690
static uint32_t s_tracked_buffer_max_distance;

// GLOBAL: LEGORR 0x00506694
static int s_audio_buffer_volume;

/* Corpus-faithful default: the original's DAT_00533878 image bytes are
 * zero-fill (past the reference .data file span).  The corpus clears it
 * on sound-system init entry (0x47a649), raises it to 1 only when
 * DirectSound init succeeds (0x47a766), and clears it at teardown
 * (0x47a842); the frameloop's audio-open success branch carries that
 * 0x47a766 write.  Plain zero-init (.bss) keeps both gates green: the
 * // GLOBAL: annotation resolves (same shape as the 0x4aa450/0x4aa458
 * pair above) and datacmp sees (uninitialized) on both sides, like the
 * corpus-zero s_listener_view.  A file-backed 0 instead (explicit
 * .data segment) keeps datacmp flagging the pair anyway and perturbs
 * reccmp's neighbouring-global verification — measured, so avoided. */
// GLOBAL: LEGORR 0x00533878
int             s_bank_sound_on = 0; /* DAT_00533878 seam, see sound.hpp */

static void* s_sound_instances[20];

static SoundInstanceHostRecord* s_sound_instance_host_records;

static float s_audio_system_initial_volume = 1.0f;

static SoundInstanceHostRecord* sound_instance_host_record_get(
    void* provider, void* instance, int create);
static void sound_instance_host_records_remove(void* provider,
                                               void* instance);

static void sound_frame_release(void* frame);

/* The retail provider is an IDirect3DRMFrame3. Its GetScene and GetPosition
 * slots are +0x68 and +0x60, while linked source records live in the private
 * scene-animation record returned by the provider's +0x1c accessor. Each
 * linked source record is five dwords: DirectSound buffer, cached xyz, next.
 * These COM layouts are only valid in the 32-bit reference backend; the SDL
 * backend's AudioBuffer is a port object and cannot be called as DirectSound. */
#if defined(_MSC_VER) && defined(_M_IX86)
static void sound_frame_scene_get(void* frame, void** scene_out)
{
    __asm {
        mov ecx, frame
        mov eax, [ecx]
        mov edx, scene_out
        push edx
        call dword ptr [eax + 68h]
    }
}

static void sound_frame_position_get(void* frame, void* scene,
                                     float position[3])
{
    __asm {
        mov ecx, frame
        mov eax, [ecx]
        mov edx, position
        push edx
        push scene
        call dword ptr [eax + 60h]
    }
}

static void sound_frame_release(void* frame)
{
    __asm {
        mov ecx, frame
        mov eax, [ecx]
        call dword ptr [eax + 8]
    }
}

static void sound_buffer_position_set(void* buffer,
                                      const float position[3])
{
    __asm {
        mov ecx, buffer
        mov eax, [ecx]
        push 0
        mov edx, position
        push dword ptr [edx + 8]
        push dword ptr [edx + 4]
        push dword ptr [edx]
        call dword ptr [eax + 4Ch]
    }
}
#elif defined(__i386__) && (defined(__GNUC__) || defined(__clang__))
typedef long (__attribute__((thiscall)) *SoundFrameGetSceneFn)(
    void*, void**);
typedef long (__attribute__((thiscall)) *SoundFrameGetPositionFn)(
    void*, void*, float*);
typedef unsigned long (__attribute__((thiscall)) *SoundFrameReleaseFn)(void*);
typedef long (__attribute__((thiscall)) *SoundBufferSetPositionFn)(
    void*, float, float, float, unsigned long);

static void sound_frame_scene_get(void* frame, void** scene_out)
{
    void** vtable = *(void***)frame;
    ((SoundFrameGetSceneFn)vtable[0x68 / sizeof(void*)])(frame, scene_out);
}

static void sound_frame_position_get(void* frame, void* scene,
                                     float position[3])
{
    void** vtable = *(void***)frame;
    ((SoundFrameGetPositionFn)vtable[0x60 / sizeof(void*)])(
        frame, scene, position);
}

static void sound_frame_release(void* frame)
{
    void** vtable = *(void***)frame;
    ((SoundFrameReleaseFn)vtable[8])(frame);
}

static void sound_buffer_position_set(void* buffer,
                                      const float position[3])
{
    void** vtable = *(void***)buffer;
    ((SoundBufferSetPositionFn)vtable[0x4c / sizeof(void*)])(
        buffer, position[0], position[1], position[2], 0ul);
}
#endif

/* Defined below (forward declarations added by tools/relocate_unit.py). */
static void sound_instance_host_buffer_set(void* provider, AudioBuffer* buffer);
static int sound_instance_callback_record_attach(void* instance);
static void* sound_instance_property_get(void* object);
static int keyed_sweep(int key, void* node);

// FUNCTION: LEGORR 0x0047a640
int audio_system_init(void* window)
{
    int initialized = 0;
    sound_bank_sound_on_set(0);
    if (s_audio_system_init_hook != NULL)
        initialized = s_audio_system_init_hook(s_audio_system_init_context,
                                               window);
    else if (s_active_audio_player != NULL)
        initialized = audio_open(s_active_audio_player) == 0;
    if (initialized != 0) {
        sound_bank_sound_on_set(1);
        s_listener_rolloff = 1.0f;
        if (s_active_audio_player != NULL) {
            s_audio_system_initial_volume = s_active_audio_player->volume;
            audio_set_volume(s_active_audio_player, 1.0f);
        }
    }
    /* The reference's completion result is one after either a successful
     * device setup or its fallback/skip path; only its retry-dialog code
     * loops. */
    return 1;
}
// FUNCTION: LEGORR 0x0047a800
void audio_active_shutdown(void)
{
    if (!sound_bank_sound_on()) return;
    if (s_active_audio_player != NULL) {
        audio_stop(s_active_audio_player);
        audio_set_volume(s_active_audio_player,
                         s_audio_system_initial_volume);
        audio_close(s_active_audio_player);
    }
    sound_bank_sound_on_set(0);
}

// FUNCTION: LEGORR 0x0047a850
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) int sound_volume_value_valid(int millibels)
{
    __asm {
        mov eax, dword ptr [esp + 4]
        test eax, eax
        jg sound_volume_invalid
        cmp eax, -10000
        jl sound_volume_invalid
        mov eax, 1
        ret
    sound_volume_invalid:
        xor eax, eax
        ret
    }
}
#else
int sound_volume_value_valid(int millibels)
{
    /* SoundVolumeValueValid @ 0x47a850: (-10001 < v < 1). Deviation: the
     * corpus gates with test/jg then cmp -10000/jl; MSVC5 canonicalizes
     * the split-gate source shape into cmp/setge form, so keep the
     * two-sided expression which scores best. */
    return (millibels < 1 && -10001 < millibels) ? 1 : 0;
}
#endif

// FUNCTION: LEGORR 0x0047a870
int sound_bank_entry_load(const char* name, int streamed, int looping,
                          int volume)
{
    /* SoundBankEntryLoad @ 0x47a870. */
    if (!s_bank_sound_on) {
        return -1; /* corpus's DAT_00533878 gate */
    }
    int slot;
    if (name == NULL || !sound_bank_free_slot_find(&slot)) {
        return -1;
    }

    BankSlot* rec = &s_bank[slot];
    rec->flags = 0;      /* claimed record starts with cleared flags */
    rec->sound = NULL;
    rec->buffers[0] = NULL;
    rec->buffers[1] = NULL;
    rec->buffers[2] = NULL;
    rec->next_buffer = 0;
    rec->volume = 0;
    rec->name[0] = '\0';

    char full[1024];
    if (!s_has_wav_suffix(name)) {
        if (strlen(name) + 5 >= sizeof(full)) {
            return -1;
        }
        snprintf(full, sizeof(full), "%s.wav", name);
    } else {
        if (strlen(name) + 1 >= sizeof(full)) {
            return -1;
        }
        snprintf(full, sizeof(full), "%s", name);
    }

    /* Loader seam: the port's resource-layer loader stands in for the
     * corpus's platform mmio path (InMemoryWaveSampleLoad's role),
     * producing the same observable loaded record. */
    Sound* s = sound_load(full);
    if (s == NULL) {
        return -1; /* flags stay zeroed: the slot was never claimed */
    }
    if (streamed != 0) {
        rec->flags |= SOUND_STREAMED;
    } else {
        if (looping != 0) {
            rec->flags |= SOUND_LOOPING;
        }
    }

    size_t n = strlen(full);
    if (n >= sizeof(rec->name)) {
        n = sizeof(rec->name) - 1; /* truncate like the record copy loop */
    }
    memcpy(rec->name, full, n);
    rec->name[n] = '\0';
    s->flags = rec->flags & (SOUND_LOOPING | SOUND_STREAMED);
    rec->sound = s;
    rec->volume = sound_volume_value_valid(volume) ? volume : 0;
    rec->flags |= SOUND_SLOT_BUSY | SOUND_SLOT_LOADED;
    if ((rec->flags & SOUND_STREAMED) == 0u
        && sound_record_create_buffers(slot) == 0) {
        audio_buffer_destroy(rec->buffers[0]);
        audio_buffer_destroy(rec->buffers[1]);
        audio_buffer_destroy(rec->buffers[2]);
        rec->buffers[0] = NULL;
        rec->buffers[1] = NULL;
        rec->buffers[2] = NULL;
        sound_free(rec->sound);
        rec->sound = NULL;
        rec->flags = 0;
        return -1;
    }
    return slot;
}
// FUNCTION: LEGORR 0x0047aad0
int sound_bank_free_slot_find(int* out_slot)
{
    /* SoundBankFreeSlotFind @ 0x47aad0. */
    if (out_slot == NULL) {
        return 0;
    }
    for (int i = 0; i < SOUND_BANK_SLOTS; ++i) {
        if ((s_bank[i].flags & SOUND_SLOT_BUSY) == 0) {
            *out_slot = i;
            return 1;
        }
    }
    return 0;
}

// FUNCTION: LEGORR 0x0047ab00
int sound_bank_entry_release(int slot)
{
    AudioPlayer* player;
    int stream_index;
    /* SoundBankEntryRelease @ 0x47ab00. */
    if (!s_bank_sound_on || slot < 0 || slot >= SOUND_BANK_SLOTS) {
        return 0;
    }
    s_bank[slot].flags &= ~(uint32_t)SOUND_SLOT_BUSY;
    player = audio_system_player_get();
    if (player != NULL && s_bank[slot].sound != NULL) {
        if (player->loop_sound == s_bank[slot].sound)
            audio_stop(player);
        for (stream_index = 0; stream_index < 2; ++stream_index) {
            if (player->stream_sample[stream_index] == s_bank[slot].sound)
                audio_stream_stop(player, stream_index);
        }
    }
    audio_buffer_destroy(s_bank[slot].buffers[0]);
    audio_buffer_destroy(s_bank[slot].buffers[1]);
    audio_buffer_destroy(s_bank[slot].buffers[2]);
    s_bank[slot].buffers[0] = NULL;
    s_bank[slot].buffers[1] = NULL;
    s_bank[slot].buffers[2] = NULL;
    if (s_bank[slot].sound != NULL) {
        sound_free(s_bank[slot].sound);
        s_bank[slot].sound = NULL;
    }
    return 1;
}

/* DirectSoundListenerViewSet (FUN_0047ab80) — see simtick.hpp. */
// FUNCTION: LEGORR 0x0047ab80
void simtick_directsound_listener_view_set(int view)
{
    if (s_bank_sound_on != 0) {
        s_listener_view = view;
    }
}
// FUNCTION: LEGORR 0x0047aba0
void simtick_sound_keepalive(EngineContext* c)
{
    (void)c;
    /* corpus: if (sound system on && an ambient entry exists) -> poke it. */
    if (s_sound_on != 0 && s_sound_active != 0 && s_keepalive != NULL) {
        s_keepalive(0, 0, 0);
    }
}

// FUNCTION: LEGORR 0x0047ad10
void sound_position_set_from_vector(void* buffer_object,
                                    const float position[3])
{
    if (buffer_object == NULL || position == NULL
        || s_sound_position_hook == NULL) return;
    s_sound_position_hook(s_sound_position_context, buffer_object, position,
                          0u);
}

// FUNCTION: LEGORR 0x0047ad30
int sound_attached_stop_by_entity(uint32_t entity)
{
    void* node = s_buffer_assoc_head;
    while (node != NULL) {
        char* words = (char*)node;
        void* provider = NULL;
        void* attached_entity = NULL;
        void* next = NULL;
        memcpy(&provider, words, sizeof(provider));
        memcpy(&attached_entity, words + 2 * sizeof(void*),
               sizeof(attached_entity));
        memcpy(&next, words + 3 * sizeof(void*), sizeof(next));
        if ((uint32_t)(uintptr_t)attached_entity == entity) {
            sound_instance_track_stop_and_remove(provider,
                                                 attached_entity);
            return 1;
        }
        node = next;
    }
    if (sizeof(void*) != 4u) {
        SoundInstanceHostRecord* record = s_sound_instance_host_records;
        while (record != NULL) {
            if ((uint32_t)(uintptr_t)record->instance == entity) {
                void* provider = record->provider;
                void* instance = record->instance;
                sound_instance_track_stop_and_remove(provider, instance);
                return 1;
            }
            record = record->next;
        }
    }
    return 0;
}
/* Instance-layer play seam. The portable audio API can represent an
 * explicit source position as stereo pan for one-shot samples, while
 * object-attached sources are tracked through the existing spatial hooks.
 * Stream selection and loop-buffer ownership remain with the bank path. */
// FUNCTION: LEGORR 0x0047ad70
int s_play_seam(int mode, int position, int slot, int loop,
                       const float* xyz)
{
    Sound* sample;
    void* attached_provider = NULL;
    if (!s_bank_sound_on || slot < 0 || slot >= SOUND_BANK_SLOTS) return -1;

    if (mode == 0 && position != 0 &&
        (s_bank[slot].flags & SOUND_STREAMED) == 0u) {
        attached_provider = (void*)(uintptr_t)(uint32_t)position;
        sound_instance_track_register(attached_provider);
        sound_source_spatial_refresh(attached_provider);
    }

    /* The current portable 3D API is a one-shot pan operation. Keep
     * streamed/loop-selected and looping bank playback on its established
     * path; do not turn a DirectSound selector into a portable loop mode. */
    if (mode == 1 && xyz != NULL && loop == 0
        && (sound_bank_entry_flags(slot) & SOUND_STREAMED) == 0u) {
        int frames;
        int previous_buffer_index = s_bank[slot].next_buffer;
        float distance = (float)sqrt(xyz[0] * xyz[0]
                                   + xyz[1] * xyz[1]
                                   + xyz[2] * xyz[2]);
        float pan = 0.0f;
        if (distance > 0.0f) pan = xyz[0] / distance;
        sample = sound_bank_entry_sample_get(slot);
        if (sample != NULL) {
            if ((s_bank[slot].flags & SOUND_LOOPING) != 0u) {
                s_bank[slot].next_buffer =
                    (s_bank[slot].next_buffer + 1) % 3;
            }
            frames = audio_active_play_sound_3d(sample, pan);
            if (frames >= 0)
                return slot * 3 + s_bank[slot].next_buffer;
            s_bank[slot].next_buffer = previous_buffer_index;
        }
    }

    if (sound_bank_entry_play_mode(slot, loop, mode) < 0) return -1;
    if ((s_bank[slot].flags & SOUND_STREAMED) != 0u) return slot * 3;
    {
        int handle = slot * 3 + s_bank[slot].next_buffer;
        if (attached_provider != NULL)
            sound_instance_host_buffer_set(attached_provider,
                sound_handle_buffer_get(handle));
        return handle;
    }
}

// FUNCTION: LEGORR 0x0047b020
void sound_buffer_association_add(uint32_t a, uint32_t b, uint32_t c)
{
    void* node = engine_heap_alloc(4 * sizeof(void*));
    if (node == NULL) {
        return; /* deviation: the corpus strides the record blindly */
    }
    char* w = (char*)node;
    void* wa = (void*)(uintptr_t)a;
    void* wb = (void*)(uintptr_t)b;
    void* wc = (void*)(uintptr_t)c;
    __builtin_memcpy(w + 0 * sizeof(void*), &wa, sizeof wa);
    __builtin_memcpy(w + 1 * sizeof(void*), &wb, sizeof wb);
    __builtin_memcpy(w + 2 * sizeof(void*), &wc, sizeof wc);
    __builtin_memcpy(w + 3 * sizeof(void*), &s_buffer_assoc_head,
                     sizeof s_buffer_assoc_head);
    s_buffer_assoc_head = node;
}
// FUNCTION: LEGORR 0x0047b050
void sound_bank_entry_volume_set(int slot, int millibels)
{
    int last;
    int i;
    /* SoundBankEntryVolumeSet @ 0x47b050. */
    if (slot < 0 || slot >= SOUND_BANK_SLOTS) {
        return; /* corpus writes without bounds */
    }
    /* Validate-and-store: invalid millibel values land as 0. */
    s_bank[slot].volume = sound_volume_value_valid(millibels) ? millibels : 0;
    last = (s_bank[slot].flags & SOUND_LOOPING) != 0u ? 3 : 1;
    for (i = 0; i < last; ++i)
        audio_buffer_volume_set(s_bank[slot].buffers[i], s_bank[slot].volume);
}
// FUNCTION: LEGORR 0x0047b0d0
int sound_bank_entry_volume(int slot)
{
    /* IndexedRecordStride134WordGet @ 0x47b0d0 (record +0x110 word). */
    return s_bank[slot].volume;
}

// FUNCTION: LEGORR 0x0047b0f0
AudioBuffer* sound_handle_buffer_get(int buffer_handle)
{
    uint32_t slot;
    int buffer_index;
    if (buffer_handle < 0 ||
        buffer_handle >= SOUND_BANK_SLOTS * 3) return NULL;
    buffer_index = buffer_handle % 3;
    slot = (uint32_t)(buffer_handle - buffer_index) / 3u;
    return s_bank[slot].buffers[buffer_index];
}

// FUNCTION: LEGORR 0x0047b130
void sound_buffer_stop_handle(int buffer_handle)
{
    AudioBuffer* buffer = sound_handle_buffer_get(buffer_handle);
    /* The retail body resolves the handle first, then checks the bank gate,
     * the -1 sentinel, and the returned interface before calling Stop. */
    if (s_bank_sound_on != 0 && buffer_handle != -1 && buffer != NULL)
        audio_buffer_stop(buffer);
}

// FUNCTION: LEGORR 0x0047b160
void sound_subsystem_stop_all(void)
{
    sound_buffer_association_stop_all();
    sound_instance_host_records_remove(NULL, NULL);
    sound_stream_stop_and_release(0);
    sound_stream_stop_and_release(1);
    audio_stop(audio_system_player_get());
}
// FUNCTION: LEGORR 0x0047b1a0
void sound_instance_track_register(void* instance)
{
    int index;
    if (instance == NULL) return;
    if (!sound_instance_callback_record_attach(instance) &&
        s_instance_register_hook != NULL)
        (void)s_instance_register_hook(s_instance_hook_context, instance);
    if (sizeof(void*) != 4u)
        (void)sound_instance_host_record_get(instance, instance, 1);
    for (index = 0; index < 20; index++) {
        if (s_sound_instances[index] == NULL) {
            s_sound_instances[index] = instance;
            return;
        }
    }
}

// FUNCTION: LEGORR 0x0047b220
void sound_instance_track_stop_and_remove(void* provider, void* instance)
{
    int index;
    int adapter = 0;

    /* The first argument is the provider frame (the reference obtains it
     * from the caller's this register). The second is the sound instance.
     * Release the instance before unlinking its source record, as the retail
     * vtable call does. */
#if (defined(_MSC_VER) && defined(_M_IX86)) || \
    (defined(__i386__) && (defined(__GNUC__) || defined(__clang__)))
    if (s_instance_stop_hook != NULL) {
        s_instance_stop_hook(s_instance_hook_context, provider, instance);
        adapter = 1;
    }
    else if (instance != NULL) {
        sound_frame_release(instance);
    }
#else
    if (s_instance_stop_hook != NULL)
        s_instance_stop_hook(s_instance_hook_context, provider, instance);
    adapter = 1;
#endif

    if (sizeof(void*) != 4u)
        sound_instance_host_records_remove(provider, instance);

    if (provider == NULL) {
        sound_buffer_association_remove_instance(instance);
        return;
    }

    if (!adapter) {
        void* property = sound_instance_property_get(provider);
        if (property != NULL) {
            void* record = *(void**)((char*)property + 0x24);
            if (nested_record_list_remove_matching_key(provider, instance,
                                                       record)) {
                void* next = NULL;
                sound_buffer_association_remove_instance(instance);
                memcpy(&next, (char*)record + 4 * sizeof(void*),
                       sizeof(next));
                memcpy((char*)property + 0x24, &next, sizeof(next));
                engine_heap_free(record);
            }
        }
    }
    else {
        sound_buffer_association_remove_instance(instance);
    }
    for (index = 0; index < 20; index++) {
        if (s_sound_instances[index] == provider) {
            s_sound_instances[index] = NULL;
        }
    }
}

// FUNCTION: LEGORR 0x0047b2a0
void keyed_resource_entry_remove(int key)
{
    void* head = sound_buffer_association_head_peek();
    if (head == NULL) {
        return; /* deviation: the corpus strides the head blindly */
    }
    if (keyed_sweep(key, head) != 0) {
        void* successor = NULL;
        __builtin_memcpy(&successor, (char*)head + 3 * sizeof(void*),
                         sizeof successor);
        sound_buffer_association_head_set(successor);
        engine_heap_free(head);
    }
}
/* NestedRecordListRemoveByKeyRecursive (FUN_0047b2e0) — see record.hpp.
 * The corpus re-reads the node's next link after the recursion — a freed
 * deeper node can leave a different successor in the slot — so the port
 * re-reads before the unlink. */
// FUNCTION: LEGORR 0x0047b2e0
int nested_record_list_remove_by_key_recursive(int key, void* node)
{
    if (node == NULL) {
        return 0; /* deviation: the corpus strides the node blindly */
    }
    void* next = NULL;
    __builtin_memcpy(&next, (char*)node + 0x0c, sizeof next);
    if (next != NULL
        && nested_record_list_remove_by_key_recursive(key, next) != 0) {
        void* successor = NULL;
        __builtin_memcpy(&successor, (char*)next + 0x0c, sizeof successor);
        __builtin_memcpy((char*)node + 0x0c, &successor, sizeof successor);
        engine_heap_free(next);
    }
    uint32_t node_key = 0u;
    __builtin_memcpy(&node_key, (char*)node + 0x08, sizeof node_key);
    return node_key == (uint32_t)key;
}

namespace unported_ledger_stubs {
// FUNCTION: LEGORR 0x0047b330
int nested_record_list_remove_matching_key_impl(
    void* owner, void* key, void* opaque_record)
{
    NestedResourceRecord* record = (NestedResourceRecord*)opaque_record;
    NestedResourceRecord* child;
    if (record == NULL) return 0;
    child = record->child_or_next;
    if (nested_record_list_remove_matching_key_impl(owner, key, child)) {
        /* The retail call removes the matching key from DAT_00506684;
         * its owner argument is forwarded but ignored by that helper. */
        (void)owner;
        sound_buffer_association_remove_instance(key);
        record->child_or_next = child->child_or_next;
        engine_heap_free(child);
    }
    return record->key == key ? 1 : 0;
}
} // namespace unported_ledger_stubs

// FUNCTION: LEGORR 0x0047b390
void sound_source_spatial_refresh_all(void)
{
    int index;
    void* node;
    for (index = 0; index < 20; index++) {
        if (s_sound_instances[index] != NULL)
            sound_source_spatial_refresh(s_sound_instances[index]);
    }
    node = s_buffer_assoc_head;
    while (node != NULL) {
        char* words = (char*)node;
        void* provider = NULL;
        void* buffer_object = NULL;
        void* attached_entity = NULL;
        void* next = NULL;
        int active = 1;
        memcpy(&provider, words, sizeof(provider));
        memcpy(&buffer_object, words + sizeof(void*), sizeof(buffer_object));
        memcpy(&attached_entity, words + 2 * sizeof(void*),
               sizeof(attached_entity));
        memcpy(&next, words + 3 * sizeof(void*), sizeof(next));
        /* The retail path calls the stored buffer's GetStatus (+0x24) and
         * removes the owner record when playback is no longer active. The
         * portable AudioBuffer has no voice-status query, so its owner must
         * provide this test through the refresh hook. */
        if (buffer_object != NULL && s_spatial_refresh_hook != NULL)
            active = s_spatial_refresh_hook(s_spatial_refresh_context,
                                            buffer_object);
        if (!active)
            sound_instance_track_stop_and_remove(provider, attached_entity);
        node = next;
    }
    sound_stream_ring_buffer_tick(0);
    sound_stream_ring_buffer_tick(1);
}
// FUNCTION: LEGORR 0x0047B410
void sound_source_spatial_refresh(void* sound_object)
{
#if (defined(_MSC_VER) && defined(_M_IX86)) || \
    (defined(__i386__) && (defined(__GNUC__) || defined(__clang__)))
    void* property;
    void* scene = NULL;
    void* record;
    float position[3] = { 0.0f, 0.0f, 0.0f };
    if (sound_object == NULL) return;
    property = sound_instance_property_get(sound_object);
    if (property == NULL) {
        if (s_spatial_refresh_hook != NULL)
            (void)s_spatial_refresh_hook(s_spatial_refresh_context,
                                         sound_object);
        return;
    }

    /* GetScene and GetPosition HRESULTs are deliberately ignored, matching
     * the retail routine. Its source cache advances even if SetPosition
     * reports failure. */
    sound_frame_scene_get(sound_object, &scene);
    record = *(void**)((char*)property + 0x24);
    while (record != NULL) {
        char* words = (char*)record;
        void* buffer = NULL;
        void* next = NULL;
        float cached[3];
        memcpy(&buffer, words, sizeof(buffer));
        memcpy(cached, words + sizeof(void*), sizeof(cached));
        memcpy(&next, words + 4 * sizeof(void*), sizeof(next));

        sound_frame_position_get(sound_object, scene, position);
        if (position[0] != cached[0] || position[1] != cached[1] ||
            position[2] != cached[2]) {
            if (buffer != NULL) sound_buffer_position_set(buffer, position);
            memcpy(words + sizeof(void*), position, sizeof(position));
        }
        record = next;
    }
    if (scene != NULL) sound_frame_release(scene);
#else
    if (sound_object != NULL && s_spatial_refresh_hook != NULL)
        (void)s_spatial_refresh_hook(s_spatial_refresh_context, sound_object);
#endif
}

// FUNCTION: LEGORR 0x0047b4a0
void sound_tracked_buffers_set_min_distance(uint32_t value)
{
    const uint32_t distance = value;
    void** record = (void**)s_buffer_assoc_head;
    s_tracked_buffer_min_distance = distance;
    while (record != NULL) {
        ((SoundTrackedBufferInterface*)record[2])->min_distance_set(distance, 0);
        record = (void**)record[3];
    }
}

// FUNCTION: LEGORR 0x0047b4d0
void sound_tracked_buffers_set_max_distance(uint32_t value)
{
    const uint32_t distance = value;
    void** record = (void**)s_buffer_assoc_head;
    s_tracked_buffer_max_distance = distance;
    while (record != NULL) {
        ((SoundTrackedBufferInterface*)record[2])->max_distance_set(distance, 0);
        record = (void**)record[3];
    }
}

/* SoundBufferVolumeSet (FUN_0047b500): DirectSound's attenuation maps to
 * the port's single master stream gain. Invalid corpus attenuation values
 * are ignored by the same range predicate used by sound-bank records. */
// FUNCTION: LEGORR 0x0047b500
void audio_sound_volume_set(int millibels)
{
    if (s_bank_sound_on != 0) {
        if (sound_volume_value_valid(millibels) != 0) {
            s_audio_buffer_volume = millibels;
            if (s_active_audio_player == NULL) return;
            const float gain = millibels <= -10000
                ? 0.0f : powf(10.0f, (float)millibels / 2000.0f);
            audio_set_volume(s_active_audio_player, gain);
        }
    }
}

/* SoundVolumeIndexSet (FUN_0047b530): the table lookup is bounded to the
 * eleven menu slider entries, then the selected attenuation is applied. */
// FUNCTION: LEGORR 0x0047b530
void audio_sound_volume_index_set(int index)
{
    if (index >= 0 && index < 11) {
        audio_sound_volume_set(s_sfx_attenuation[index]);
    }
}
// FUNCTION: LEGORR 0x0047b550
void audio_sound_volume_minus_300(void)
{
    if (s_bank_sound_on != 0)
        audio_sound_volume_set(-300);
}

// FUNCTION: LEGORR 0x0047b6c0
int sound_record_create_buffers(int slot)
{
    int i;
    int created = 0;
    int count;
    if (sound_bank_entry_sample_get(slot) == NULL) return 0;
    /* The reference's streamed-file branch initializes a stream state and
     * skips SoundRecordCreateBuffers; duplicate buffers belong to the
     * in-memory sample path only. */
    if ((s_bank[slot].flags & SOUND_STREAMED) != 0u) return 0;
    count = (s_bank[slot].flags & SOUND_LOOPING) != 0u ? 3 : 1;
    for (i = 0; i < count; ++i) {
        if (!sound_wave_sample_duplicate_buffer_create(slot, i)) break;
        ++created;
    }
    return created == count ? created : 0;
}
// FUNCTION: LEGORR 0x0047b790
int sound_wave_sample_duplicate_buffer_create(int slot, int buffer_index)
{
    Sound* sample = sound_bank_entry_sample_get(slot);
    AudioBuffer* buffer;
    if (sample == NULL || buffer_index < 0 || buffer_index >= 3) return 0;
    buffer = audio_buffer_create(sample);
    if (buffer == NULL) return 0;
    audio_buffer_destroy(s_bank[slot].buffers[buffer_index]);
    s_bank[slot].buffers[buffer_index] = buffer;
    audio_buffer_volume_set(buffer, s_bank[slot].volume);
    return 1;
}

// FUNCTION: LEGORR 0x0047b8e0
double sound_streamed_wave_buffer_step_rate(int slot)
{
    /* StreamedWaveBufferStepRate @ 0x47b8e0. slot -1 is the variant
     * select's "no sound" result: zero, before any table read. */
    if (slot == -1) {
        return 0.0;
    }
    if (slot < 0 || slot >= SOUND_BANK_SLOTS) {
        return 0.0; /* deviation: the corpus reads past its record table */
    }
    const BankSlot* rec = &s_bank[slot];
    const Sound* snd = rec->sound;
    uint32_t bytes = (snd != NULL) ? snd->file_len : 0u;
    double rate = (snd != NULL) ? (double)snd->format.byte_rate : 0.0;
    return (double)bytes / rate;
}

// FUNCTION: LEGORR 0x0047ba20
int sound_stream_stop_and_release(int stream_index)
{
    AudioPlayer* p = s_active_audio_player;
    int slot = stream_index == 0 ? 0 : 1;
    if (p != NULL) {
        platform_audio_close(p->stream_dev[slot]);
        p->stream_dev[slot] = NULL;
        p->stream_active[slot] = 0;
        p->stream_sample[slot] = NULL;
        p->stream_buffer_bytes[slot] = 0;
        p->stream_cursor_bytes[slot] = 0;
        memset(&p->stream_format[slot], 0, sizeof(WavSample));
    }
    return 1;
}

// FUNCTION: LEGORR 0x0047bc30
int sound_stream_drain_check(int stream_index)
{
    AudioPlayer* p = s_active_audio_player;
    int slot = stream_index == 0 ? 0 : 1;
    const Sound* sample;
    uint32_t before;
    uint32_t frame_bytes;
    int queued_ms;
    int buffer_ms;
    int submitted;
    if (p == NULL || !p->stream_active[slot]
        || p->stream_dev[slot] == NULL) return 0;
    sample = p->stream_sample[slot];
    if (sample == NULL) return 0;
    queued_ms = platform_audio_queued_milliseconds(
        p->stream_dev[slot]);
    if (p->stream_cursor_bytes[slot] >= sample->data_len) {
        if (queued_ms == 0) (void)sound_stream_stop_and_release(slot);
        return 0;
    }
    frame_bytes = p->stream_format[slot].block_align;
    if (frame_bytes == 0u || p->stream_format[slot].byte_rate == 0u)
        return 0;
    buffer_ms = (int)(((double)p->stream_buffer_bytes[slot] * 1000.0)
                      / p->stream_format[slot].byte_rate);
    if (buffer_ms < 2) buffer_ms = 2;
    if (queued_ms >= buffer_ms / 2) return 0;
    before = p->stream_cursor_bytes[slot];
    submitted = audio_stream_submit_chunk(p, slot);
    if (submitted <= 0) return 0;
    return (int)(p->stream_cursor_bytes[slot] - before);
}

// FUNCTION: LEGORR 0x0047bdb0
void sound_stream_ring_buffer_tick(int stream_index)
{
    AudioPlayer* p = s_active_audio_player;
    int slot = stream_index == 0 ? 0 : 1;
    if (p == NULL || !p->stream_active[slot]
        || p->stream_sample[slot] == NULL
        || p->stream_dev[slot] == NULL) return;
    if (p->stream_cursor_bytes[slot] >= p->stream_sample[slot]->data_len
        && platform_audio_queued_milliseconds(p->stream_dev[slot]) == 0) {
        (void)sound_stream_stop_and_release(slot);
        return;
    }
    (void)sound_stream_drain_check(slot);
}

// FUNCTION: LEGORR 0x0047c0c0
int vec_exact_equal(const float* a, const float* b) /* FUN_0047c0c0 */
{
    return a[0] == b[0] && a[1] == b[1] && a[2] == b[2];
}

namespace unported_ledger_stubs {
// FUNCTION: LEGORR 0x0047c100
float ledger_stub_0047c100_DirectSoundListenerRolloffSet(float rolloff)
{
    if (s_bank_sound_on == 0) return rolloff;
    if (rolloff < 0.0f) rolloff = 0.0f;
    if (rolloff > 10.0f) rolloff = 10.0f;
    return audio_listener_rolloff_set(rolloff);
}
} // namespace unported_ledger_stubs

// FUNCTION: LEGORR 0x0047c160
int32_t constant_negative_10000_get(void)
{
    return -10000;
}

/* The reference scene interfaces are 32-bit COM-style objects. This getter
 * reads the record that DDiSceneAnimationHookCreate owns at vtable +0x1c. */
static void* sound_instance_property_get(void* object)
{
#if defined(_MSC_VER) && defined(_M_IX86)
    void* result;
    __asm {
        mov ecx, object
        mov eax, [ecx]
        call dword ptr [eax + 1Ch]
        mov result, eax
    }
    return result;
#elif defined(__i386__) && (defined(__GNUC__) || defined(__clang__))
    typedef void* (__attribute__((thiscall)) *GetProperty)(void*);
    void** vtable;
    GetProperty get_property;
    if (object == NULL) return NULL;
    vtable = *(void***)object;
    if (vtable == NULL || vtable[0x1c / sizeof(void*)] == NULL) return NULL;
    get_property = (GetProperty)vtable[0x1c / sizeof(void*)];
    return get_property(object);
#else
    typedef void* (*GetProperty)(void*);
    void** vtable;
    GetProperty get_property;
    if (object == NULL) return NULL;
    vtable = *(void***)object;
    if (vtable == NULL || vtable[0x1c / sizeof(void*)] == NULL) return NULL;
    get_property = (GetProperty)vtable[0x1c / sizeof(void*)];
    return get_property(object);
#endif
}

/* FUN_0047b1a0's 0x14-byte payload contains target pointers and a provider
 * list link. On 64-bit hosts keep the same ownership relationships in a
 * native-width sidecar instead of truncating those pointers into the retail
 * record layout. */
static SoundInstanceHostRecord* sound_instance_host_record_get(
    void* provider, void* instance, int create)
{
    SoundInstanceHostRecord* record = s_sound_instance_host_records;
    while (record != NULL) {
        if (record->provider == provider && record->instance == instance)
            return record;
        record = record->next;
    }
    if (!create || provider == NULL || instance == NULL) return NULL;
    record = (SoundInstanceHostRecord*)engine_heap_alloc(
        (long)sizeof(SoundInstanceHostRecord));
    if (record == NULL) return NULL;
    memset(record, 0, sizeof(*record));
    record->provider = provider;
    record->instance = instance;
    record->next = s_sound_instance_host_records;
    s_sound_instance_host_records = record;
    return record;
}

static void sound_instance_host_records_remove(void* provider,
                                               void* instance)
{
    SoundInstanceHostRecord** link = &s_sound_instance_host_records;
    while (*link != NULL) {
        SoundInstanceHostRecord* record = *link;
        if ((provider == NULL || record->provider == provider) &&
            (instance == NULL || record->instance == instance)) {
            if (record->buffer != NULL) audio_buffer_stop(record->buffer);
            *link = record->next;
            engine_heap_free(record);
        }
        else {
            link = &record->next;
        }
    }
}
static int sound_instance_callback_record_attach(void* instance)
{
    uint32_t* payload;
    void* frame_interface;
    if (sizeof(void*) != 4u || instance == NULL) return 0;

    frame_interface = sound_instance_property_get(instance);
    payload = (uint32_t*)engine_heap_alloc(0x14L);
    if (payload == NULL) return 0;
    memset(payload, 0, 0x14u);
    payload[0] = (uint32_t)(uintptr_t)instance;
    if (frame_interface != NULL)
        payload[4] = *(uint32_t*)((char*)frame_interface + 0x24);
    ddiscene_animation_hook_create(instance, 0, NULL, NULL, 0, NULL, NULL,
                                   0, 0, payload);
    /* The retail caller does not release its 0x14-byte payload after the
     * 0x475f70 call. That helper copies word 0, but no owner for this block is
     * visible in the checked-in path. Preserve the observed lifetime. */
    return sound_instance_property_get(instance) != NULL;
}

static void sound_instance_host_buffer_set(void* provider, AudioBuffer* buffer)
{
    SoundInstanceHostRecord* record =
        sound_instance_host_record_get(provider, provider, 1);
    if (record != NULL) record->buffer = buffer;
}

/* KeyedResourceEntryRemove (FUN_0047b2a0) — see record.hpp. The corpus
 * routes through FUN_0047b2e0's +0x08/+0x0c node view; the port's sound
 * association nodes widened that window, so the sweep reproduces the same
 * deep-first promotion/unlink/free walk over the native field slots. */
static int keyed_sweep(int key, void* node)
{
    if (node == NULL) {
        return 0; /* deviation: the corpus strides the node blindly */
    }
    void* next = NULL;
    __builtin_memcpy(&next, (char*)node + 3 * sizeof(void*), sizeof next);
    if (next != NULL
        && keyed_sweep(key, next) != 0) {
        void* successor = NULL;
        __builtin_memcpy(&successor, (char*)next + 3 * sizeof(void*),
                         sizeof successor);
        __builtin_memcpy((char*)node + 3 * sizeof(void*), &successor,
                         sizeof successor);
        engine_heap_free(next);
    }
    uint32_t node_key = 0u;
    __builtin_memcpy(&node_key, (char*)node + 2 * sizeof(void*),
                     sizeof node_key);
    return node_key == (uint32_t)key;
}
