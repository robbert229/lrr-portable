// SPDX-License-Identifier: LGPL-3.0-or-later
/* SFX.c, original translation unit GAME-041 (finding 505), rebuilt in
 * original shape (ADR 0005; finding 585): text 00464740-004661c0, .rdata
 * 0049e73c-0049e77c, literals 004a94d0-004a9794, .bss 00501468-00502bd8,
 * commons 00552100-00552134.
 *
 * The unit names the sound effects (SFX_NULL to SFX_AmbientMusicLoop, plus
 * the samples the config adds), loads each one's chain of Sound3D samples,
 * picks one at random to play, keeps the global sample and the play queue,
 * and replays queued sounds when the queue mode ends. Its second half
 * (00465380-004661c0) runs the smoke emitters: the textures, the emitter
 * list, the particle groups and their mesh. That half has its own .rdata
 * constants (a second 0.0f at 0049e750) and only commons, so it is probably
 * the original's Smoke.c, a unit of its own (finding 585).
 *
 * The port's implementation stays in the port tail sfx_port.cpp,
 * unannotated. In the matching build sfx_link_names.h gives this unit's
 * functions the port's names, so every caller reaches them; natively the
 * port tail serves those names and nothing calls these. */
#include "sfx_link_names.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "crt/vc5names.h"
#include "sfx.h"
#include "sfx_imports.h"

/* The unit's functions have C linkage in the native C++ compile too. */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct SFX_Vector3F {
    float x, y, z;
} SFX_Vector3F;

#include "sfx_maths.h"

#define SFX_TYPE_COUNT 45        /* SFX_NULL..SFX_AmbientMusicLoop */
#define SFX_MAXSAMPLES 495       /* the hash list: 0x7bc bytes */
#define SFX_MAXINSTANCES 10
#define SFX_MAXQUEUE 10

#define SFX_GLOB_FLAG_SOUNDON 0x1
#define SFX_GLOB_FLAG_POPULATEMODE 0x2
#define SFX_GLOB_FLAG_QUEUEMODE 0x8

/* One sample of an SFX's chain. */
typedef struct SFX_Property {
    int sound3DHandle;                /* +0x0 Sound3D_Load's sample */
    struct SFX_Property* next;        /* +0x4 */
} SFX_Property;

/* A sound played while the queue mode is on (SFX_SetQueueMode). */
typedef struct SFX_Instance {
    int sfxID;                        /* +0x00 */
    void* frame;                      /* +0x04 */
    SFX_Vector3F wPos;                /* +0x08 */
    int sound3D : 1;                  /* +0x14 bit 0 */
    int loop : 1;                     /* +0x14 bit 1 */
    int onCont : 1;                   /* +0x14 bit 2 */
} SFX_Instance;

typedef struct SFX_Globs {
    SFX_Property sfxProperties[SFX_MAXSAMPLES];  /* 00501468 chain heads */
    SFX_Property samplePropTable[200];           /* 005023e0 chain links */
    int* hashNameList;                           /* 00502a20 */
    unsigned int sfxCount;                       /* 00502a24 added samples */
    unsigned int samplePropCount;                /* 00502a28 */
    unsigned int flags;                          /* 00502a2c */
    SFX_Instance sfxInstanceTable[SFX_MAXINSTANCES]; /* 00502a30 */
    unsigned int sfxInstanceCount;               /* 00502b20 */
    float globalSampleDuration;                  /* 00502b24 */
    int globalSampleSoundHandle;                 /* 00502b28 */
    int globalSampleSFXType;                     /* 00502b2c */
    int queueSFXTypeTable[SFX_MAXQUEUE];         /* 00502b30 */
    int queueSoundModeTable[SFX_MAXQUEUE];       /* 00502b58 */
    int queueSFXTypeList[SFX_MAXQUEUE];          /* 00502b80 */
    int queueSoundModeList[SFX_MAXQUEUE];        /* 00502ba8 */
    unsigned int queueCount;                     /* 00502bd0 */
    unsigned int queueListCount;                 /* 00502bd4 */
} SFX_Globs;                                     /* 0x1770 bytes */

#define SMOKE_MAXGROUPS 10

#define SMOKE_FLAG_HIDDEN 0x1
#define SMOKE_FLAG_OWNCONTAINER 0x2
#define SMOKE_FLAG_GROUPS 0x4
#define SMOKE_FLAG_ONESHOT 0x8
#define SMOKE_FLAG_ENDING 0x10
#define SMOKE_FLAG_SOUNDDONE 0x20

/* One particle of an emitter, drawn as one mesh group. */
typedef struct Smoke_Group {
    int active;                       /* +0x00 */
    SFX_Vector3F randDir;             /* +0x04 */
    SFX_Vector3F velocity;            /* +0x10 */
    SFX_Vector3F position;            /* +0x1c */
    float width;                      /* +0x28 */
    float height;                     /* +0x2c */
    float time;                       /* +0x30 */
    int maxDistance;                  /* +0x34 */
    float distance;                   /* +0x38 */
} Smoke_Group;                        /* 0x3c bytes */

/* A smoke emitter (0x2a8 bytes), a node of the emitter list. */
typedef struct Smoke {
    Smoke_Group groups[SMOKE_MAXGROUPS]; /* +0x000 */
    float delay;                      /* +0x258 until the next group shows */
    SFX_Vector3F worldPos;            /* +0x25c */
    SFX_Vector3F dir;                 /* +0x268 */
    float r, g, b;                    /* +0x274 */
    unsigned int groupCount;          /* +0x280 */
    unsigned int shownCount;          /* +0x284 */
    float speed;                      /* +0x288 */
    void* cont;                       /* +0x28c */
    void* mesh;                       /* +0x290 */
    unsigned int flags;               /* +0x294 */
    struct Smoke* next;               /* +0x298 */
    struct Smoke* prev;               /* +0x29c */
    int sfxID;                        /* +0x2a0 */
    int soundHandle;                  /* +0x2a4 */
} Smoke;

// GLOBAL: LEGORR 0x00501468
static SFX_Globs sfxGlobs;

/* Smoke's state: commons (the original defines them as common symbols). */
// GLOBAL: LEGORR 0x00552100
void* smokeTextures[10];
// GLOBAL: LEGORR 0x00552128
Smoke* smokeListHead;
// GLOBAL: LEGORR 0x0055212c
Smoke* smokeListTail;
// GLOBAL: LEGORR 0x00552130
unsigned int smokeTextureCount;

/* Util_HashString(name, FALSE, TRUE). */
#define SFX_HASH(name) game_string_hash_compute((const unsigned char*)(name), 0, 1)

// FUNCTION: LEGORR 0x00464740
void sfx_initialise(void)
{
    sfxGlobs.hashNameList = (int*)malloc(sizeof(int) * SFX_MAXSAMPLES);

    // STRING: LEGORR 0x004a9794
    sfxGlobs.hashNameList[0] = SFX_HASH("SFX_NULL");
    // STRING: LEGORR 0x004a9788
    sfxGlobs.hashNameList[1] = SFX_HASH("SFX_Stamp");
    // STRING: LEGORR 0x004a977c
    sfxGlobs.hashNameList[2] = SFX_HASH("SFX_Drill");
    // STRING: LEGORR 0x004a976c
    sfxGlobs.hashNameList[3] = SFX_HASH("SFX_DrillFade");
    // STRING: LEGORR 0x004a975c
    sfxGlobs.hashNameList[4] = SFX_HASH("SFX_RockBreak");
    // STRING: LEGORR 0x004a9750
    sfxGlobs.hashNameList[5] = SFX_HASH("SFX_Drip");
    // STRING: LEGORR 0x004a9744
    sfxGlobs.hashNameList[6] = SFX_HASH("SFX_Ambient");
    // STRING: LEGORR 0x004a9734
    sfxGlobs.hashNameList[7] = SFX_HASH("SFX_AmbientLoop");
    // STRING: LEGORR 0x004a9728
    sfxGlobs.hashNameList[8] = SFX_HASH("SFX_Step");
    // STRING: LEGORR 0x004a9718
    sfxGlobs.hashNameList[9] = SFX_HASH("SFX_RockMonster");
    // STRING: LEGORR 0x004a9704
    sfxGlobs.hashNameList[10] = SFX_HASH("SFX_RockMonster2");
    // STRING: LEGORR 0x004a96f0
    sfxGlobs.hashNameList[11] = SFX_HASH("SFX_RockMonsterStep");
    // STRING: LEGORR 0x004a96e0
    sfxGlobs.hashNameList[12] = SFX_HASH("SFX_MFDeposit");
    // STRING: LEGORR 0x004a96cc
    sfxGlobs.hashNameList[13] = SFX_HASH("SFX_ButtonPressed");
    // STRING: LEGORR 0x004a96c0
    sfxGlobs.hashNameList[14] = SFX_HASH("SFX_MFLift");
    // STRING: LEGORR 0x004a96b4
    sfxGlobs.hashNameList[15] = SFX_HASH("SFX_MFThrow");
    // STRING: LEGORR 0x004a96a8
    sfxGlobs.hashNameList[16] = SFX_HASH("SFX_Walker");
    // STRING: LEGORR 0x004a969c
    sfxGlobs.hashNameList[17] = SFX_HASH("SFX_YesSir");
    // STRING: LEGORR 0x004a9690
    sfxGlobs.hashNameList[18] = SFX_HASH("SFX_Build");
    // STRING: LEGORR 0x004a9684
    sfxGlobs.hashNameList[19] = SFX_HASH("SFX_Okay");
    // STRING: LEGORR 0x004a9678
    sfxGlobs.hashNameList[20] = SFX_HASH("SFX_NotOkay");
    // STRING: LEGORR 0x004a965c
    sfxGlobs.hashNameList[21] = SFX_HASH("SFX_InterfaceSlideOnScreen");
    // STRING: LEGORR 0x004a9640
    sfxGlobs.hashNameList[22] = SFX_HASH("SFX_InterfaceSlideOffScreen");
    // STRING: LEGORR 0x004a9628
    sfxGlobs.hashNameList[23] = SFX_HASH("SFX_PanelSlideOnScreen");
    // STRING: LEGORR 0x004a9610
    sfxGlobs.hashNameList[24] = SFX_HASH("SFX_PanelSlideOffScreen");
    // STRING: LEGORR 0x004a9604
    sfxGlobs.hashNameList[25] = SFX_HASH("SFX_Siren");
    // STRING: LEGORR 0x004a95f0
    sfxGlobs.hashNameList[26] = SFX_HASH("SFX_CrystalRecharge");
    // STRING: LEGORR 0x004a95e4
    sfxGlobs.hashNameList[27] = SFX_HASH("SFX_Laser");
    // STRING: LEGORR 0x004a95d4
    sfxGlobs.hashNameList[28] = SFX_HASH("SFX_LaserHit");
    // STRING: LEGORR 0x004a95c0
    sfxGlobs.hashNameList[29] = SFX_HASH("SFX_LazerRecharge");
    // STRING: LEGORR 0x004a95b0
    sfxGlobs.hashNameList[30] = SFX_HASH("SFX_TopPriority");
    // STRING: LEGORR 0x004a959c
    sfxGlobs.hashNameList[31] = SFX_HASH("SFX_ImmovableRock");
    // STRING: LEGORR 0x004a9590
    sfxGlobs.hashNameList[32] = SFX_HASH("SFX_Wall");
    // STRING: LEGORR 0x004a9584
    sfxGlobs.hashNameList[33] = SFX_HASH("SFX_Floor");
    // STRING: LEGORR 0x004a9574
    sfxGlobs.hashNameList[34] = SFX_HASH("SFX_BoulderHit");
    // STRING: LEGORR 0x004a9568
    sfxGlobs.hashNameList[35] = SFX_HASH("SFX_Place");
    // STRING: LEGORR 0x004a9558
    sfxGlobs.hashNameList[36] = SFX_HASH("SFX_PlaceOre");
    // STRING: LEGORR 0x004a9544
    sfxGlobs.hashNameList[37] = SFX_HASH("SFX_PlaceCrystal");
    // STRING: LEGORR 0x004a9538
    sfxGlobs.hashNameList[38] = SFX_HASH("SFX_Lava");
    // STRING: LEGORR 0x004a9528
    sfxGlobs.hashNameList[39] = SFX_HASH("SFX_RockWipe");
    // STRING: LEGORR 0x004a951c
    sfxGlobs.hashNameList[40] = SFX_HASH("SFX_FallIn");
    // STRING: LEGORR 0x004a950c
    sfxGlobs.hashNameList[41] = SFX_HASH("SFX_MusicLoop");
    // STRING: LEGORR 0x004a94f8
    sfxGlobs.hashNameList[42] = SFX_HASH("SFX_CaptainSlide");
    // STRING: LEGORR 0x004a94e8
    sfxGlobs.hashNameList[43] = SFX_HASH("SFX_Dynamite");
    // STRING: LEGORR 0x004a94d0
    sfxGlobs.hashNameList[44] = SFX_HASH("SFX_AmbientMusicLoop");

    scene_sample_hook_register((void*)sfx_container_sound_trigger_callback, NULL);
}

// FUNCTION: LEGORR 0x00464c20
void sfx_container_sound_trigger_callback(const char* sampleName, void* cont,
                                         void* data)
{
    int sfxID;

    if (sfx_get_type(sampleName, &sfxID)) {
        sfx_random_play_sound3d_on_container(cont, sfxID, 0, 1, NULL);
    }
}

// FUNCTION: LEGORR 0x00464c50
void sfx_set_sample_populate_mode(int on)
{
    if (on) sfxGlobs.flags |= SFX_GLOB_FLAG_POPULATEMODE;
    else sfxGlobs.flags &= ~SFX_GLOB_FLAG_POPULATEMODE;
}

// FUNCTION: LEGORR 0x00464c70
int sfx_get_type(const char* sfxName, int* sfxID)
{
    unsigned int i, count;
    int hash;

    if (sfxName != NULL) {
        hash = SFX_HASH(sfxName);
        count = sfxGlobs.sfxCount + SFX_TYPE_COUNT;
        for (i = 0; i < count; i++) {
            if (hash == sfxGlobs.hashNameList[i]) {
                *sfxID = i;
                return 1;
            }
        }
        if (sfxGlobs.flags & SFX_GLOB_FLAG_POPULATEMODE) {
            *sfxID = count;
            sfxGlobs.hashNameList[count] = SFX_HASH(sfxName);
            sfxGlobs.sfxCount++;
            return 1;
        }
    }
    return 0;
}

// FUNCTION: LEGORR 0x00464d00
int sfx_load_sample_property(char* value, int sfxID)
{
    char* stringParts[100];
    char volumeStr[64];
    SFX_Property* sfxProp = &sfxGlobs.sfxProperties[sfxID];
    int result = 1;
    SFX_Property* prevProp = NULL;
    int isStream = 0;
    unsigned int i, count;

    sfxProp->next = NULL;
    /* "," (004a13e4) is pooled with GAME-001; advisor.c marks it. */
    count = sound_variant_split(value, stringParts, ",");
    for (i = 0; i < count; i++) {
        char* str = stringParts[i];
        int isMulti;
        int volume;

        if (*str == '*') {
            isMulti = 1;
            str++;
        }
        else isMulti = 0;

        volume = 0;
        if (*str == '#') {
            char* s;
            if (*++str != '#') {
                s = volumeStr;
                do {
                    *s++ = *str;
                } while (*++str != '#');
            }
            volume = atoi(volumeStr);
            str++;
        }
        if (*str == '@') {
            isStream = 1;
            str++;
        }

        if ((sfxProp->sound3DHandle = sound_bank_entry_load(str, isStream, isMulti,
                                                            volume)) != -1) {
            if (prevProp != NULL) {
                sfxProp = &sfxGlobs.samplePropTable[sfxGlobs.samplePropCount++];
                prevProp->next = sfxProp;
                sfxProp->next = NULL;
            }
            prevProp = sfxProp;
        }
        else result = 0;
    }
    return result;
}

// FUNCTION: LEGORR 0x00464e20
int sfx_random_get_sound3d_handle(int sfxID)
{
    unsigned int count = 0;
    SFX_Property* sfxProp;
    SFX_Property* prop;
    unsigned int rnd;
    int handle;

    if (sfxID == 0) return -1;
    if (sfxGlobs.sfxProperties[sfxID].next != NULL) {
        sfxProp = &sfxGlobs.sfxProperties[sfxID];
        for (prop = &sfxGlobs.sfxProperties[sfxID]; prop != NULL; prop = prop->next) count++;
        rnd = (unsigned int)rng_thunk_draw15() % count;
        for (prop = sfxProp; rnd > 0; rnd--) prop = prop->next;
        handle = prop->sound3DHandle;
        prop->sound3DHandle = sfxProp->sound3DHandle;
        sfxProp->sound3DHandle = handle;
    }
    return sfxGlobs.sfxProperties[sfxID].sound3DHandle;
}

// FUNCTION: LEGORR 0x00464e80
void sfx_stop_global_sample(void)
{
    if (sfxGlobs.globalSampleSoundHandle != -1 &&
        sfxGlobs.globalSampleDuration > 0.0f) {
        sfx_sound3d_stop_sound(sfxGlobs.globalSampleSoundHandle);
        sfxGlobs.globalSampleSoundHandle = -1;
        sfxGlobs.globalSampleDuration = 0.0f;
    }
}

// FUNCTION: LEGORR 0x00464ec0
int sfx_set_global_sample_duration_if_le0(float duration)
{
    if (sfxGlobs.globalSampleDuration <= 0.0f) {
        sfxGlobs.globalSampleSoundHandle = -1;
        sfxGlobs.globalSampleDuration = duration;
        return 1;
    }
    return 0;
}

// FUNCTION: LEGORR 0x00464ef0
int sfx_random_set_and_play_global_sample(int sfxID, int* handle)
{
    if (sfxGlobs.globalSampleDuration <= 0.0f) {
        if ((sfxGlobs.globalSampleSoundHandle =
                 sfx_random_play_sound_normal(sfxID, 0)) != -1) {
            sfxGlobs.globalSampleDuration =
                sfx_random_get_sample_play_time(sfxID) * 25.0f;
            sfxGlobs.globalSampleSFXType = sfxID;
            if (handle != NULL) *handle = sfxGlobs.globalSampleSoundHandle;
            return 1;
        }
    }
    return 0;
}

// FUNCTION: LEGORR 0x00464f60
void sfx_add_to_queue(int sfxID, int mode)
{
    if (sfxGlobs.queueCount < SFX_MAXQUEUE) {
        sfxGlobs.queueSFXTypeTable[sfxGlobs.queueCount] = sfxID;
        sfxGlobs.queueSoundModeTable[sfxGlobs.queueCount] = mode;
        sfxGlobs.queueCount++;
    }
}

/* Only the played path returns a handle; the queued and silent paths
 * return nothing, as the original (no eax is set). */
// FUNCTION: LEGORR 0x00464fa0
int sfx_random_play_sound_normal(int sfxID, int loop)
{
    if (sfxGlobs.flags & SFX_GLOB_FLAG_QUEUEMODE) {
        if (sfxGlobs.sfxInstanceCount < SFX_MAXINSTANCES) {
            SFX_Instance* inst = &sfxGlobs.sfxInstanceTable[sfxGlobs.sfxInstanceCount++];
            inst->sfxID = sfxID;
            inst->sound3D = 0;
        }
    }
    else if (sfxID != 0 && (sfxGlobs.flags & SFX_GLOB_FLAG_SOUNDON)) {
        int handle = sfx_random_get_sound3d_handle(sfxID);
        if (handle != 0) return s_play_seam(2, 0, handle, loop, NULL);
    }
}

// FUNCTION: LEGORR 0x00465010
void sfx_random_set_buffer_volume(int sfxID, int volume)
{
    sound_bank_entry_volume_set(sfx_random_get_sound3d_handle(sfxID), volume);
}

// FUNCTION: LEGORR 0x00465030
int sfx_random_get_buffer_volume(int sfxID)
{
    return sound_bank_entry_volume(sfx_random_get_sound3d_handle(sfxID));
}

// FUNCTION: LEGORR 0x00465050
int sfx_random_play_sound3d_on_container(void* cont, int sfxID, int loop,
                                         int onCont, float* wPos)
{
    void* frame;

    if (cont != NULL) frame = (void*)(uintptr_t)scene_object_wrapped_pointer_get(cont);
    else frame = NULL;
    return sfx_random_play_sound3d_on_frame_internal(frame, sfxID, loop, onCont, wPos);
}

// FUNCTION: LEGORR 0x00465090
int sfx_random_play_sound3d_on_frame_internal(void* frame, int sfxID, int loop,
                                              int onCont, float* wPos)
{
    if (sfxGlobs.flags & SFX_GLOB_FLAG_QUEUEMODE) {
        if (sfxGlobs.sfxInstanceCount < SFX_MAXINSTANCES) {
            SFX_Instance* inst = &sfxGlobs.sfxInstanceTable[sfxGlobs.sfxInstanceCount++];
            inst->sfxID = sfxID;
            inst->frame = frame;
            inst->loop = loop;
            inst->onCont = onCont;
            inst->sound3D = 1;
            if (wPos != NULL) inst->wPos = *(SFX_Vector3F*)wPos;
        }
    }
    else if (sfxID != 0 && (sfxGlobs.flags & SFX_GLOB_FLAG_SOUNDON)) {
        int handle = sfx_random_get_sound3d_handle(sfxID);
        if (handle != 0) {
            if (onCont) return s_play_seam(0, (int)(uintptr_t)frame, handle, loop, NULL);
            return s_play_seam(1, 0, handle, loop, wPos);
        }
    }
    return 0;
}

// FUNCTION: LEGORR 0x00465160
float sfx_random_get_sample_play_time(int sfxID)
{
    int handle = sfx_random_get_sound3d_handle(sfxID);
    if (handle != 0) return sound_streamed_wave_buffer_step_rate(handle);
    return 0.0f;
}

// FUNCTION: LEGORR 0x00465190
void sfx_sound3d_stop_sound(int handle)
{
    sound_buffer_stop_handle(handle);
}

// FUNCTION: LEGORR 0x004651a0
void sfx_update(float elapsed)
{
    unsigned int i;
    int queueMode = sfx_is_queue_mode();

    sfxGlobs.globalSampleDuration -= elapsed;

    sfx_set_queue_mode(0, 0);
    for (i = 0; i < sfxGlobs.queueListCount; i++) {
        sfx_random_play_sound_normal(sfxGlobs.queueSFXTypeList[i],
                                     sfxGlobs.queueSoundModeList[i]);
    }
    if (queueMode) sfx_set_queue_mode_and_flush(1);

    memcpy(sfxGlobs.queueSFXTypeList, sfxGlobs.queueSFXTypeTable,
           sizeof(int) * sfxGlobs.queueCount);
    memcpy(sfxGlobs.queueSoundModeList, sfxGlobs.queueSoundModeTable,
           sizeof(int) * sfxGlobs.queueCount);
    sfxGlobs.queueListCount = sfxGlobs.queueCount;
    sfxGlobs.queueCount = 0;

    sfx_sound3d_update();
}

// FUNCTION: LEGORR 0x00465250
void sfx_sound3d_update(void)
{
    sound_source_spatial_refresh_all();
}

// FUNCTION: LEGORR 0x00465260
void sfx_set_sound_on(int on, int stopAll)
{
    if (on) sfxGlobs.flags |= SFX_GLOB_FLAG_SOUNDON;
    else {
        sfxGlobs.flags &= ~SFX_GLOB_FLAG_SOUNDON;
        if (stopAll) sound_subsystem_stop_all();
        sfxGlobs.globalSampleSFXType = 0;
    }
}

// FUNCTION: LEGORR 0x004652a0
void sfx_set_sound_on_and_stop_all(int on)
{
    sfx_set_sound_on(on, 1);
}

// FUNCTION: LEGORR 0x004652b0
int sfx_is_queue_mode(void)
{
    return sfxGlobs.flags & SFX_GLOB_FLAG_QUEUEMODE;
}

// FUNCTION: LEGORR 0x004652c0
void sfx_set_queue_mode_and_flush(int on)
{
    sfx_set_queue_mode(on, 1);
}

// FUNCTION: LEGORR 0x004652d0
void sfx_set_queue_mode(int on, int flushQueued)
{
    unsigned int i;

    if (on) sfxGlobs.flags |= SFX_GLOB_FLAG_QUEUEMODE;
    else {
        sfxGlobs.flags &= ~SFX_GLOB_FLAG_QUEUEMODE;
        if (flushQueued) {
            for (i = 0; i < sfxGlobs.sfxInstanceCount; i++) {
                SFX_Instance* inst = &sfxGlobs.sfxInstanceTable[i];
                if (inst->sound3D) {
                    sfx_random_play_sound3d_on_frame_internal(inst->frame, inst->sfxID,
                        inst->loop, inst->onCont, (float*)&inst->wPos);
                }
                else sfx_random_play_sound_normal(inst->sfxID, inst->loop);
            }
            sfxGlobs.sfxInstanceCount = 0;
        }
    }
}

// FUNCTION: LEGORR 0x00465370
int sfx_is_sound_on(void)
{
    return sfxGlobs.flags & SFX_GLOB_FLAG_SOUNDON;
}

// FUNCTION: LEGORR 0x00465380
int smoke_initialise(void)
{
    smokeListHead = NULL;
    smokeListTail = NULL;
    smokeTextureCount = 0;
    return 1;
}

// FUNCTION: LEGORR 0x004653a0
void smoke_load_textures(const char* dirName, const char* baseName,
                         unsigned int textureCount)
{
    char name[260];
    char path[260];
    unsigned int i;

    smokeTextureCount = textureCount;
    for (i = 0; i < textureCount; i++) {
        // STRING: LEGORR 0x004a1d88
        sprintf(path, "%s\\", dirName);
        // STRING: LEGORR 0x004a1d7c
        sprintf(name, "%s%i.bmp", baseName, i);
        smokeTextures[i] = sfx_mesh_load_texture_adapter(path, name, NULL, NULL);
    }
}

// FUNCTION: LEGORR 0x00465430
Smoke* smoke_create_smoke_area(void* cont, int bx, int by, SFX_Vector3F* dir,
                               float r, float g, float b, float density,
                               float speed, int type, unsigned int seed,
                               int sfxID)
{
    unsigned int faceData[6] = { 0, 3, 2, 2, 1, 0 };
    SFX_Vector3F wPos;
    Smoke* smoke;
    unsigned int i = 0, j;

    smoke = (Smoke*)malloc(sizeof(Smoke));
    memset(smoke, 0, sizeof(Smoke));

    if (type == 0 || type == 2) {
        TerrainMap* map = SFX_LEGO_GET_MAP();
        void* root = scene_root_object_get();
        unsigned int range = (unsigned int)(map->blockSize * 0.6f);

        terrain_map_block_to_world(map, bx, by, &wPos.x, &wPos.y);
        wPos.x += (float)((seed % 0x109e) % range) - (float)range * 0.5f;
        wPos.y += (float)((seed % 0x5eb) % range) - (float)range * 0.5f;
        wPos.z = terrain_map_world_z_get(map, wPos.x, wPos.y);

        smoke->cont = lego_container_create_adapter(root);
        ddiscene_object_position_set(smoke->cont, NULL, wPos.x, wPos.y, wPos.z);
        smoke->flags = SMOKE_FLAG_OWNCONTAINER;
        smoke->worldPos = wPos;
        if ((seed & 3) == 0) smoke->sfxID = sfxID;
    }
    else if (type == 1) {
        smoke->flags = SMOKE_FLAG_GROUPS;
        smoke->cont = cont;
    }

    smoke->dir.x = dir->x;
    smoke->dir.y = dir->y;
    smoke->dir.z = dir->z;
    sfx_vector3d_normalise(&smoke->dir);
    smoke->r = r;
    smoke->g = g;
    smoke->b = b;

    if (type == 2) {
        smoke->groupCount = 4;
        smoke->shownCount = 0;
        smoke->flags |= SMOKE_FLAG_ONESHOT;
        smoke->speed = 2.0f;
    }
    else {
        if (density <= 0.0f && density >= 1.0f) smoke->groupCount = 10;
        else smoke->groupCount = (unsigned int)(density * 10.0f);
        smoke->speed = speed;
    }

    smoke->mesh = engine_task_callback_record_allocate_and_initialize(
        SFX_CONTAINER_FRAME(smoke->cont), (void*)smoke_mesh_render_callback,
        0x4800, smoke, 0);

    for (; i < smoke->groupCount; i++) {
        int groupID = mesh_record_allocate_and_fill(
            (struct MeshRecordPool*)smoke->mesh, 4, 2, 3,
            (const uint16_t*)faceData);
        Smoke_Group* group = &smoke->groups[groupID];

        sfx_mesh_set_group_texture_adapter(smoke->mesh, groupID,
                                           smokeTextures[i % smokeTextureCount]);
        render_object_color_vector_set(smoke->mesh, groupID, r, g, b, 0);
        render_object_color_vector_set(smoke->mesh, groupID, r * 0.2f, g * 0.2f,
                                       b * 0.2f, 3);
        indexed_record_flag6c_bit0_set(smoke->mesh, groupID, 1);

        group->width = (float)(rand() % 4) + 6.0f;
        group->height = (float)(rand() % 4) + group->width + 2.0f;
        if (smoke->flags & SMOKE_FLAG_ONESHOT) group->height += group->height;
        else group->width *= 0.5f;
    }

    smoke->delay = 0.0f;
    if (type != 2) {
        for (j = 0; j < 10; j++) {
            if (smoke->delay <= 0.0f) {
                for (i = 0; i < smoke->groupCount; i++) {
                    if (!smoke->groups[i].active) {
                        smoke_group_show(smoke, i);
                        break;
                    }
                }
            }
            smoke->delay -= 5.0f;
            for (i = 0; i < smoke->groupCount; i++) {
                if (smoke->groups[i].active) smoke_group_update(smoke, i, 5.0f);
            }
        }
    }

    smoke->prev = smokeListTail;
    if (smokeListHead == NULL) {
        smokeListHead = smoke;
        smokeListTail = smoke;
        return smoke;
    }
    smokeListTail->next = smoke;
    smokeListTail = smoke;
    return smoke;
}

// FUNCTION: LEGORR 0x00465970
void smoke_remove_all(void)
{
    Smoke* smoke = smokeListHead;

    while (smoke != NULL) {
        Smoke* next = smoke->next;
        smoke_remove(smoke, smoke->cont);
        smoke = next;
    }
    smokeListHead = NULL;
}

// FUNCTION: LEGORR 0x004659b0
void smoke_remove(Smoke* smoke, void* cont)
{
    if (smoke->flags & SMOKE_FLAG_OWNCONTAINER) {
        mesh_remove(smoke->mesh, SFX_CONTAINER_FRAME(smoke->cont));
        ledger_stub_00472a40_DDiSceneObjectDestroy(smoke->cont);
    }
    else mesh_remove(smoke->mesh, SFX_CONTAINER_FRAME(cont));

    if (smokeListTail == smoke) {
        if ((smokeListTail = smoke->prev) == NULL) {
            smokeListHead = NULL;
            free(smoke);
            return;
        }
    }
    if (smoke->next != NULL) {
        smoke->next->prev = smoke->prev;
        if (smoke->next->prev == NULL) smokeListHead = smoke->next;
    }
    if (smoke->prev != NULL) smoke->prev->next = smoke->next;
    if (smoke->soundHandle != 0) sfx_sound3d_stop_sound(smoke->soundHandle);
    free(smoke);
}

// FUNCTION: LEGORR 0x00465a90
void smoke_hide_all(int hide)
{
    Smoke* smoke;

    for (smoke = smokeListHead; smoke != NULL; smoke = smoke->next) {
        smoke_hide(smoke, hide);
    }
}

// FUNCTION: LEGORR 0x00465ac0
void smoke_hide(Smoke* smoke, int hide)
{
    record_flag2c_bit0_set(smoke->mesh, hide);
    if (hide) smoke->flags |= SMOKE_FLAG_HIDDEN;
    else smoke->flags &= ~SMOKE_FLAG_HIDDEN;
}

// FUNCTION: LEGORR 0x00465b00
void smoke_update(float elapsed)
{
    Smoke* smoke;
    Smoke* next;
    unsigned int i;
    int finished;

    for (smoke = smokeListHead; smoke != NULL; smoke = next) {
        next = smoke->next;
        finished = 1;

        if (!(smoke->flags & SMOKE_FLAG_SOUNDDONE) && smoke->soundHandle == 0) {
            if (smoke->sfxID != 0) {
                smoke->soundHandle = sfx_random_play_sound3d_on_container(
                    NULL, smoke->sfxID, 1, 0, (float*)&smoke->worldPos);
            }
            if (smoke->soundHandle == 0) smoke->flags |= SMOKE_FLAG_SOUNDDONE;
        }

        if (!(smoke->flags & SMOKE_FLAG_HIDDEN)) {
            if (!(smoke->flags & SMOKE_FLAG_ONESHOT) || !(smoke->flags & SMOKE_FLAG_ENDING)) {
                if (smoke->delay <= 0.0f) {
                    for (i = 0; i < smoke->groupCount; i++) {
                        if (!smoke->groups[i].active) {
                            smoke_group_show(smoke, i);
                            break;
                        }
                    }
                }
                smoke->delay -= elapsed;
            }
            for (i = 0; i < smoke->groupCount; i++) {
                if (smoke->groups[i].active) {
                    smoke_group_update(smoke, i, elapsed);
                    finished = 0;
                }
            }
            if ((smoke->flags & SMOKE_FLAG_ONESHOT) && (smoke->flags & SMOKE_FLAG_ENDING) &&
                finished) {
                smoke_remove(smoke, smoke->cont);
            }
        }
    }
}

// FUNCTION: LEGORR 0x00465c50
void smoke_group_show(Smoke* smoke, unsigned int groupID)
{
    Smoke_Group* group = &smoke->groups[groupID];
    SFX_Vector3F vec;
    SFX_Vector3F pos;
    float scale;

    if (smoke->flags & SMOKE_FLAG_ONESHOT) {
        if (smoke->shownCount == 3) smoke->flags |= SMOKE_FLAG_ENDING;
        smoke->shownCount++;
    }
    group->active = 1;
    indexed_record_flag6c_bit0_set(smoke->mesh, groupID, 0);

    d3drm_vector_random((float*)&vec);
    vec.z = -(float)fabs(vec.z);
    sfx_vector3d_scale(&vec, &vec, 0.17f);
    vec.z -= 0.83f;
    group->randDir = vec;

    sfx_vector3d_add(&vec, &smoke->dir, &vec);
    scale = (-smoke->speed) / vec.z;
    sfx_vector3d_scale(&vec, &vec, scale);

    smoke->flags = SMOKE_FLAG_GROUPS;
    ddiscene_object_position_get(smoke->cont, NULL, (float*)&pos);
    group->position = pos;
    group->time = 0.0f;
    group->distance = 0.0f;
    group->velocity = vec;
    group->maxDistance = rand() % 30 + 40;
    smoke->delay = (0.5f / smoke->speed) * group->height;
}

// FUNCTION: LEGORR 0x00465e00
void smoke_group_update(Smoke* smoke, unsigned int groupID, float elapsed)
{
    Smoke_Group* group = &smoke->groups[groupID];
    SFX_Vector3F move;
    float alpha;

    sfx_vector3d_scale(&move, &group->velocity, elapsed);
    sfx_vector3d_add(&group->position, &group->position, &move);
    group->distance += (float)fabs(move.z);

    if (group->distance >= (float)group->maxDistance) {
        group->active = 0;
        indexed_record_flag6c_bit0_set(smoke->mesh, groupID, 1);
        return;
    }
    group->time += elapsed;
    alpha = 0.7f - (group->distance / (float)group->maxDistance) * 0.7f;
    mesh_group_material_values_set(smoke->mesh, groupID, alpha, 4);
}

// FUNCTION: LEGORR 0x00465ee0
void smoke_mesh_render_callback(void* mesh, Smoke* smoke, void* viewport)
{
    SFX_Vector3F camPos;
    unsigned int i;
    void* camera = (void*)(uintptr_t)lego_context_selected_global_pointer_get();

    ddiscene_object_position_get(camera, NULL, (float*)&camPos);
    for (i = 0; i < smoke->groupCount; i++) {
        if (smoke->groups[i].active) smoke_group_set_vertices(smoke, i, viewport, &camPos);
    }
}

// FUNCTION: LEGORR 0x00465f40
void smoke_group_set_vertices(Smoke* smoke, unsigned int groupID, void* viewport,
                              SFX_Vector3F* camPos)
{
    float uvs[8] = { 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f };
    Smoke_Group* group = &smoke->groups[groupID];
    SFX_Vector3F vertices[4];
    SFX_Vector3F dir, camUp, up, side;
    float scale;
    void* camera = (void*)(uintptr_t)scene_object_query_value_if_ready(viewport);

    ddiscene_object_facing_pair_get(camera, NULL, (float*)&dir, (float*)&camUp);

    up.x = 0.0f;
    up.y = 0.0f;
    up.z = -1.0f;
    sfx_vector3d_cross_product(&side, &up, &dir);
    if (dir.z > 0.3f) d3drm_vector_rotate((float*)&up, (float*)&up, (float*)&side, 0.7f);

    scale = group->distance / (float)group->maxDistance + 1.0f;
    sfx_vector3d_scale(&side, &side, group->width * scale);
    sfx_vector3d_scale(&up, &up, group->height * scale);

    sfx_vector3d_add(&vertices[0], &group->position, &side);
    sfx_vector3d_subtract(&vertices[1], &group->position, &side);
    sfx_vector3d_subtract(&vertices[2], &vertices[1], &up);
    sfx_vector3d_subtract(&vertices[3], &vertices[0], &up);

    instance_quad_vertex_data_fill(smoke->mesh, groupID, 0, 4, (float*)vertices,
                                   (float*)camPos, (const uint32_t*)uvs);
}

#ifdef __cplusplus
}
#endif
