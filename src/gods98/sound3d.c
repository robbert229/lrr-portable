// SPDX-License-Identifier: LGPL-3.0-or-later
/* Original translation unit G98-009 (text 0047a640-0047c170; .rdata
 * 0049e964-0049e968; .data 004aae30-004aae5c; .bss 005065b8-00533904;
 * finding 505): DirectSound. It owns the 600-slot sample table, the primary
 * buffer and its 3D listener, the buffers attached to frames, and the two
 * streamed wave buffers (one plain, one looping). The functions keep the
 * port's names; the comments give descriptive Sound3D_* names, which the
 * binary does not attest.
 *
 * DirectSound is outside miniwin's scope. ADR 0007's audio substitution
 * replaces each DirectSound call with the audio library's SDL3 version
 * (library/audio_sdl.h), marked `// [library:audio]`; frames are miniwin's
 * IDirect3DRMFrame3. */
#include "sound3d.h"
#include "audio.h"



#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "crt/vc5names.h"
#include "winmm.h"
#include "soundsys.h"
#include "resource.h"
#include "files_paths.h"
#include "container_frame.h"
#include "files_native.h"
#include "library/audio_sdl.h"

#include <miniwin/d3drm.h>

/* The DirectSound types the unit fills in. MSVC 5.0's dsound.h is DirectX
 * 3's and has no 3D interfaces, so the unit declares the DirectX 5 layouts
 * it uses. */
typedef struct Sound3DIID {
    uint32_t data1;
    unsigned short data2;
    unsigned short data3;
    unsigned char data4[8];
} Sound3DIID;

typedef D3DVECTOR Sound3DVector;

/* DSBUFFERDESC, DirectX 5 (0x14 bytes). */
typedef struct Sound3DBufferDesc {
    DWORD dwSize;
    DWORD dwFlags;
    DWORD dwBufferBytes;
    DWORD dwReserved;
    WAVEFORMATEX* lpwfxFormat;
} Sound3DBufferDesc;

/* [library:audio] DirectSound's objects are the audio library's
 * (library/audio_sdl.h, SDL3 audio): each call the unit made through their
 * method tables is a call into the library, marked where it is made. The
 * 3D buffer is the buffer itself. */
typedef AudioDevice Sound3DDevice;                   /* IDirectSound */
typedef AudioBuffer Sound3DBuffer;                   /* IDirectSoundBuffer */
typedef AudioListener Sound3DListener;               /* IDirectSound3DListener */
typedef AudioBuffer Sound3DBuffer3D;                 /* IDirectSound3DBuffer */
typedef IDirect3DRMFrame3 Sound3DFrame;              /* miniwin */

#define SOUND3D_DS_OK 0
#define SOUND3D_DSERR_BUFFERLOST ((long)0x88780096)
#define SOUND3D_DSSCL_PRIORITY 2
#define SOUND3D_DSBPLAY_LOOPING 1
#define SOUND3D_DSBSTATUS_PLAYING 1
#define SOUND3D_DS3D_IMMEDIATE 0
#define SOUND3D_DS3D_DEFERRED 1
#define SOUND3D_DS3DMODE_NORMAL 0
#define SOUND3D_DS3DMODE_DISABLE 2
/* DSBCAPS_PRIMARYBUFFER | DSBCAPS_CTRL3D | DSBCAPS_CTRLVOLUME */
#define SOUND3D_PRIMARY_CAPS 0x91
/* DSBCAPS_STATIC | DSBCAPS_CTRL3D | DSBCAPS_CTRLFREQUENCY | DSBCAPS_CTRLPAN
 * | DSBCAPS_CTRLVOLUME | DSBCAPS_MUTE3DATMAXDISTANCE */
#define SOUND3D_SAMPLE_CAPS 0x200f2
/* DSBCAPS_CTRLFREQUENCY | DSBCAPS_CTRLPAN | DSBCAPS_CTRLVOLUME
 * | DSBCAPS_GETCURRENTPOSITION2 */
#define SOUND3D_STREAM_CAPS 0x100e0

#define SOUND3D_MAXSAMPLES 600

/* Sample flags. */
#define SOUND3D_FLAG_USED 0x1
#define SOUND3D_FLAG_ACTIVE 0x2
#define SOUND3D_FLAG_MULTI 0x4
#define SOUND3D_FLAG_STREAM 0x8

/* One sample (0x134 bytes). */
typedef struct Sound3DSoundData {
    char fName[260];               /* +0x000 */
    unsigned long size;            /* +0x104 */
    unsigned long avgBytesPerSec;  /* +0x108 streamed samples */
    DWORD freq;                    /* +0x10c */
    LONG volume;                   /* +0x110 */
    unsigned long offset;          /* +0x114 */
    BYTE* pbData;                  /* +0x118 */
    WAVEFORMATEX* pwf;             /* +0x11c */
    Sound3DBuffer* lpDsb3D[3];     /* +0x120 */
    unsigned long voice;           /* +0x12c */
    unsigned long flags;           /* +0x130 */
} Sound3DSoundData;

/* A frame's attached sound (0x14 bytes), listed from its app data. */
typedef struct Sound3DSoundRecord {
    Sound3DBuffer3D* sound3DBuff;
    Sound3DVector pos;
    struct Sound3DSoundRecord* next;
} Sound3DSoundRecord;

/* The frame app data (Container.c's Container_AppData); the unit reads only
 * its sound list. The fields mirror Container_AppData's types so the sound
 * list lands on the same offset on every ABI: on a 64-bit build it is +0x40,
 * not the original's +0x24. */
typedef struct Sound3DFrameData {
    void* ownerContainer;            /* +0x00 */
    char* animSetFileName;           /* +0x04 */
    char* frameName;                 /* +0x08 */
    unsigned long frameCount;        /* +0x0c */
    float currTime;                  /* +0x10 */
    float transCo;                   /* +0x14 */
    char* activitySample;            /* +0x18 */
    void* animClone;                 /* +0x1c */
    unsigned long trigger;           /* +0x20 */
    Sound3DSoundRecord* soundRecord; /* +0x24 */
} Sound3DFrameData;

/* A playing 3D buffer (0x10 bytes). */
typedef struct Sound3DSoundFrameRecord {
    Sound3DFrame* frame;
    Sound3DBuffer* soundBuff;
    Sound3DBuffer3D* sound3DBuff;
    struct Sound3DSoundFrameRecord* next;
} Sound3DSoundFrameRecord;

/* A streamed wave (0x5c bytes). */
typedef struct Sound3DStreamData {
    int fileOpen;                  /* +0x00 */
    WAVEFORMATEX* pwf;             /* +0x04 */
    HMMIO hmmio;                   /* +0x08 */
    MMCKINFO mmckinfoData;         /* +0x0c */
    MMCKINFO mmckinfoParent;       /* +0x20 */
    unsigned long bufferSize;      /* +0x34 */
    unsigned long notifySize;      /* +0x38 */
    unsigned long writeOffset;     /* +0x3c */
    unsigned long playProgress;    /* +0x40 */
    unsigned long writeProgress;   /* +0x44 */
    unsigned long lastPos;         /* +0x48 */
    int playEnd;                   /* +0x4c */
    int looping;                   /* +0x50 */
    int fileEnd;                   /* +0x54 */
    int playing;                   /* +0x58 */
} Sound3DStreamData;

typedef struct Sound3DGlobs {
    Sound3DDevice* lpDSnd;                       /* +0x00 */
    Sound3DBuffer* lpDSBuff;                     /* +0x04 primary */
    Sound3DListener* lp3DListenerInfo;           /* +0x08 */
    Sound3DBuffer* lpDSStreamBuff;               /* +0x0c */
    Sound3DBuffer* lpDSLoopStreamBuff;           /* +0x10 */
    Sound3DStreamData streamData;                /* +0x14 */
    Sound3DStreamData loopStreamData;            /* +0x70 */
    Sound3DSoundFrameRecord* soundFrameRecord;   /* +0xcc */
    Sound3DFrame* listenerFrame;                 /* +0xd0 */
    float minDistanceForAttenuation;             /* +0xd4 */
    float maxDistance;                           /* +0xd8 */
    LONG volume;                                 /* +0xdc */
    Sound3DSoundData soundTable[SOUND3D_MAXSAMPLES]; /* +0xe0 */
} Sound3DGlobs;

#define SOUND3D_STREAM_BUFFER(loop) \
    ((loop) ? s_sound3d.lpDSLoopStreamBuff : s_sound3d.lpDSStreamBuff)

/* IID_IDirectSound3DListener and IID_IDirectSound3DBuffer, which the
 * original links from dxguid.lib. */
// GLOBAL: LEGORR 0x004a04f8
static const Sound3DIID IID_IDirectSound3DListener =
    {0x279afa84, 0x4981, 0x11ce, {0xa5, 0x21, 0x00, 0x20, 0xaf, 0x0b, 0xe5, 0x60}};
// GLOBAL: LEGORR 0x004a0508
static const Sound3DIID IID_IDirectSound3DBuffer =
    {0x279afa86, 0x4981, 0x11ce, {0xa5, 0x21, 0x00, 0x20, 0xaf, 0x0b, 0xe5, 0x60}};

extern Sound3DGlobs s_sound3d; /* defined at the end of the file */

#define s_sound3d_frames (sound3dState.frames)

/* With C linkage: other units call them (ADR 0005). */
extern "C" {
int sound_bank_free_slot_find(int* handle);
void sound_listener_frame_update(Sound3DFrame* frame, void* arg,
                                 float delta);
void sound_position_set_from_vector(Sound3DBuffer3D* sound3DBuff,
                                    const Sound3DVector* wPos,
                                    const Sound3DVector* vel);
int sound_attached_stop_by_entity(Sound3DFrame* frame,
                                  Sound3DBuffer3D* sound3DBuff);
void sound_buffer_association_add(Sound3DFrame* frame,
                                  Sound3DBuffer* soundBuff,
                                  Sound3DBuffer3D* sound3DBuff);
Sound3DBuffer* sound_handle_buffer_get(int handle);
void sound_instance_track_register(Sound3DFrame* frame,
                                   Sound3DBuffer3D* sound3DBuff);
void sound_instance_track_stop_and_remove(Sound3DFrame* frame,
                                          Sound3DBuffer3D* sound3DBuff);
void keyed_resource_entry_remove(Sound3DFrame* frame,
                                 Sound3DBuffer3D* sound3DBuff);
int nested_record_list_remove_by_key_recursive(
    Sound3DFrame* frame, Sound3DBuffer3D* sound3DBuff,
    Sound3DSoundFrameRecord* record);
void sound_source_spatial_refresh(Sound3DFrame* frame, void* arg,
                                  float delta);
void sound_tracked_buffers_set_max_distance(float dist);
void audio_sound_volume_minus_300(void);
int sound_wave_sample_load(Sound3DSoundData* sound, const char* fName,
                           int simultaneous);
int sound_record_create_buffers(Sound3DSoundData* sound, int simultaneous);
int sound_wave_sample_duplicate_buffer_create(Sound3DSoundData* sound);
int sound_streamed_wave_play(const char* fName, int loop, int volume);
int sound_streamed_wave_buffer_setup(const char* fName, int loop,
                                     int volume);
int sound_stream_drain_check(int loop);
void sound_stream_ring_buffer_tick(int loop);
float ledger_stub_0047c100_DirectSoundListenerRolloffSet(float rolloff);
}


/* Sound3D_Initialised (00533878). Shared by both backends: soundsys.c and
 * sound.cpp read it. */

/* The SFX slider's attenuation steps, in millibels (audio.cpp reads it). */
// GLOBAL: LEGORR 0x004aae30
int s_sfx_attenuation[11] = {
    -10000, -5000, -3500, -2000, -1500, -1000,
      -700,  -400,  -200,   -80,     0
};

// FUNCTION: LEGORR 0x0047a640
int audio_system_init(void* window)
{
    Sound3DBufferDesc dsbd;
    WAVEFORMATEX wfx;
    int r;

    s_bank_sound_on = 0;
    s_sound3d.listenerFrame = NULL;

    do {
        if (audio_create(&s_sound3d.lpDSnd) == SOUND3D_DS_OK) { // [library:audio]
            if (audio_set_cooperative_level(s_sound3d.lpDSnd, window, SOUND3D_DSSCL_PRIORITY) == SOUND3D_DS_OK) { // [library:audio]
                memset(&dsbd, 0, sizeof(dsbd));
                dsbd.dwSize = sizeof(dsbd);
                dsbd.dwFlags = SOUND3D_PRIMARY_CAPS;
                if (audio_create_buffer(s_sound3d.lpDSnd, dsbd.dwFlags, dsbd.dwBufferBytes, dsbd.lpwfxFormat, &s_sound3d.lpDSBuff) == SOUND3D_DS_OK) { // [library:audio]
                    wfx.wFormatTag = 1;
                    wfx.nChannels = 2;
                    wfx.wBitsPerSample = 16;
                    wfx.nSamplesPerSec = 22050;
                    wfx.nBlockAlign = wfx.nChannels * wfx.wBitsPerSample / 8;
                    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
                    wfx.cbSize = 0;
                    audio_buffer_set_format(s_sound3d.lpDSBuff, &wfx); // [library:audio]
                    /* QueryInterface(IID_IDirectSound3DListener). */
                    if (audio_buffer_query_listener(s_sound3d.lpDSBuff, // [library:audio]
                            &s_sound3d.lp3DListenerInfo) == SOUND3D_DS_OK) {
                        audio_listener_set_doppler_factor(s_sound3d.lp3DListenerInfo, 100.0f, SOUND3D_DS3D_IMMEDIATE); // [library:audio]
                    } else {
                        audio_buffer_release(s_sound3d.lpDSBuff); // [library:audio]
                        audio_release(s_sound3d.lpDSnd); // [library:audio]
                        s_sound3d.lpDSnd = NULL;
                    }
                } else {
                    audio_release(s_sound3d.lpDSnd); // [library:audio]
                    s_sound3d.lpDSnd = NULL;
                }
            } else {
                audio_release(s_sound3d.lpDSnd); // [library:audio]
                s_sound3d.lpDSnd = NULL;
            }
        }

        if (s_sound3d.lpDSnd == NULL) {
            r = 5;
        } else {
            r = 1;
            s_bank_sound_on = 1;
        }
    } while (r == 4);

    if (s_bank_sound_on) {
        audio_buffer_play(s_sound3d.lpDSBuff, 0, 0, SOUND3D_DSBPLAY_LOOPING); // [library:audio]
        audio_listener_set_rolloff_factor(s_sound3d.lp3DListenerInfo, 1.0f, SOUND3D_DS3D_DEFERRED); // [library:audio]
        audio_listener_set_doppler_factor(s_sound3d.lp3DListenerInfo, 0.0f, SOUND3D_DS3D_DEFERRED); // [library:audio]
        audio_listener_commit_deferred_settings(s_sound3d.lp3DListenerInfo); // [library:audio]
        s_sound3d.minDistanceForAttenuation = 1.0f;
        s_sound3d.maxDistance = 800.0f;
        audio_buffer_get_volume(s_sound3d.lpDSBuff, &s_sound3d_frames.windowsVolume); // [library:audio]
        audio_sound_volume_minus_300();
    }

    return 1;
}

// FUNCTION: LEGORR 0x0047a800
void audio_active_shutdown(void)
{
    if (s_bank_sound_on) {
        audio_sound_volume_set(s_sound3d_frames.windowsVolume);
        audio_buffer_stop(s_sound3d.lpDSBuff); // [library:audio]
        audio_buffer_release(s_sound3d.lpDSBuff); // [library:audio]
        audio_release(s_sound3d.lpDSnd); // [library:audio]
        s_sound3d.lpDSnd = NULL;
        s_bank_sound_on = 0;
    }
}

// FUNCTION: LEGORR 0x0047a850
int sound_volume_value_valid(int millibels)
{
    if (millibels <= 0 && millibels >= -10000) return 1;
    return 0;
}

// FUNCTION: LEGORR 0x0047a870
int sound_bank_entry_load(const char* name, int streamed, int looping,
                          int volume)
{
    int handle;
    char fileName[260];
    char cdFileName[260];
    int ok = 0;

    if (s_bank_sound_on && sound_bank_free_slot_find(&handle)) {
        s_sound3d.soundTable[handle].flags = 0;
        sprintf(fileName,
                // STRING: LEGORR 0x004aae5c
                "%s.wav",
                name);
        if (streamed) {
            const char* fullName = resource_path_resolve_against_base(fileName);
            const char* useName = NULL;
            FILE* mfp;
            if ((mfp = files_native_fopen(fullName, "r")) != NULL) { // [library:filesystem]
                useName = fullName;
                fclose(mfp);
            } else if (resource_cd_file_path_get(cdFileName, fileName)) {
                useName = cdFileName;
            }
            if (useName != NULL && (mfp = files_native_fopen(useName, "r")) != NULL) { // [library:filesystem]
                fseek(mfp, 0, SEEK_END);
                s_sound3d.soundTable[handle].size = ftell(mfp);
                s_sound3d.soundTable[handle].avgBytesPerSec =
                    sound_wave_file_average_bytes_per_second_read(useName);
                s_sound3d.soundTable[handle].flags |= SOUND3D_FLAG_STREAM;
                fclose(mfp);
                ok = 1;
            }
        } else {
            if (sound_wave_sample_load(&s_sound3d.soundTable[handle], fileName, looping)) {
                s_sound3d.soundTable[handle].flags &= ~SOUND3D_FLAG_STREAM;
                if (looping) s_sound3d.soundTable[handle].flags |= SOUND3D_FLAG_MULTI;
                ok = 1;
            }
        }

        if (ok) {
            strcpy(s_sound3d.soundTable[handle].fName, fileName);
            if (sound_volume_value_valid(volume)) s_sound3d.soundTable[handle].volume = volume;
            else s_sound3d.soundTable[handle].volume = 0;
            s_sound3d.soundTable[handle].flags |= SOUND3D_FLAG_USED | SOUND3D_FLAG_ACTIVE;
            return handle;
        }
    }

    return -1;
}

// FUNCTION: LEGORR 0x0047aad0
int sound_bank_free_slot_find(int* handle)
{
    unsigned int i;
    for (i = 0; i < SOUND3D_MAXSAMPLES; i++) {
        if (!(s_sound3d.soundTable[i].flags & SOUND3D_FLAG_USED)) {
            *handle = i;
            return 1;
        }
    }
    return 0;
}

// FUNCTION: LEGORR 0x0047ab00
int sound_bank_entry_release(int slot)
{
    Sound3DSoundData* sound;
    int i;

    if (s_bank_sound_on && slot != -1) {
        sound = &s_sound3d.soundTable[slot];
        sound->flags &= ~SOUND3D_FLAG_USED;
        if (sound->pwf != NULL) {
            GlobalFree(sound->pwf);
            sound->pwf = NULL;
        }
        for (i = 0; i < 3; i++) {
            if (sound->lpDsb3D[i] != NULL) {
                audio_buffer_release(sound->lpDsb3D[i]); // [library:audio]
                sound->lpDsb3D[i] = NULL;
            }
        }
        return 1;
    }
    return 0;
}

/* DirectSoundListenerViewSet (FUN_0047ab80) — see simtick.hpp. */
// FUNCTION: LEGORR 0x0047ab80
extern "C" void simtick_directsound_listener_view_set(Sound3DFrame* frame)
{
    if (s_bank_sound_on) s_sound3d.listenerFrame = frame;
}

// FUNCTION: LEGORR 0x0047aba0
void simtick_sound_keepalive(void)
{
    if (s_bank_sound_on && s_sound3d.listenerFrame) {
        sound_listener_frame_update(s_sound3d.listenerFrame, NULL, 0.0f);
    }
}

// FUNCTION: LEGORR 0x0047abc0
void sound_listener_frame_update(Sound3DFrame* frame, void* arg, float delta)
{
    Sound3DFrame* scene;
    Sound3DVector vec;
    Sound3DVector up;
    int changed = 0;

    frame->GetScene(&scene);
    frame->GetPosition(scene, &vec);
    if (!vec_exact_equal(&vec.x, &s_sound3d_frames.oldPos.x)) {
        audio_listener_set_position(s_sound3d.lp3DListenerInfo, vec.x, vec.y, vec.z, SOUND3D_DS3D_DEFERRED); // [library:audio]
        s_sound3d_frames.oldPos = vec;
        changed = 1;
    }

    frame->GetOrientation(scene, &vec, &up);
    if (!vec_exact_equal(&vec.x, &s_sound3d_frames.oldOrientFront.x) ||
        !vec_exact_equal(&up.x, &s_sound3d_frames.oldOrientUp.x)) {
        audio_listener_set_orientation(s_sound3d.lp3DListenerInfo, vec.x, vec.y, vec.z, up.x, up.y, up.z, // [library:audio]
            SOUND3D_DS3D_DEFERRED);
        s_sound3d_frames.oldOrientFront = vec;
        s_sound3d_frames.oldOrientUp = up;
        changed = 1;
    }

    if (changed) {
        audio_listener_commit_deferred_settings(s_sound3d.lp3DListenerInfo); // [library:audio]
    }

    scene->Release();
}

// FUNCTION: LEGORR 0x0047ad10
void sound_position_set_from_vector(Sound3DBuffer3D* sound3DBuff,
                                    const Sound3DVector* wPos,
                                    const Sound3DVector* vel)
{
    audio_buffer_set_3d_position(sound3DBuff, wPos->x, wPos->y, wPos->z, // [library:audio]
                                 SOUND3D_DS3D_IMMEDIATE);
}

// FUNCTION: LEGORR 0x0047ad30
int sound_attached_stop_by_entity(Sound3DFrame* frame,
                                  Sound3DBuffer3D* sound3DBuff)
{
    Sound3DSoundFrameRecord* item = s_sound3d.soundFrameRecord;

    while (item != NULL) {
        if (sound3DBuff == item->sound3DBuff) {
            sound_instance_track_stop_and_remove(item->frame, item->sound3DBuff);
            return 1;
        }
        item = item->next;
    }
    return 0;
}

/* Instance-layer play seam. The portable audio API can represent an
 * explicit source position as stereo pan for one-shot samples, while
 * object-attached sources are tracked through the existing spatial hooks.
 * Stream selection and loop-buffer ownership remain with the bank path. */
// FUNCTION: LEGORR 0x0047ad70
int s_play_seam(int mode, intptr_t position, int slot, int loop,
                const float* xyz)
{
    Sound3DFrame* frame = (Sound3DFrame*)position;
    Sound3DSoundData* sound;
    Sound3DBuffer* soundBuff;
    Sound3DBuffer3D* sound3DBuff;
    Sound3DFrame* scene;
    Sound3DVector pos;
    char cdFileName[260];
    LONG volume;

    if (s_bank_sound_on && slot != -1) {
        sound = &s_sound3d.soundTable[slot];
        if (sound->flags & SOUND3D_FLAG_ACTIVE) {
            if (sound->flags & SOUND3D_FLAG_STREAM) {
                const char* fullName = resource_path_resolve_against_base(sound->fName);
                const char* useName = NULL;
                FILE* mfp;
                if ((mfp = files_native_fopen(fullName, "r")) != NULL) { // [library:filesystem]
                    useName = fullName;
                    fclose(mfp);
                } else if (resource_cd_file_path_get(cdFileName, sound->fName)) {
                    useName = cdFileName;
                }
                if (useName != NULL) {
                    sound_streamed_wave_play(useName, loop, sound->volume);
                    return slot * 3;
                }
            } else {
                if (sound->flags & SOUND3D_FLAG_MULTI) {
                    if (++sound->voice >= 3) sound->voice = 0;
                    soundBuff = sound->lpDsb3D[sound->voice];
                } else {
                    soundBuff = sound->lpDsb3D[0];
                }

                /* QueryInterface(IID_IDirectSound3DBuffer). */
                audio_buffer_query_3d(soundBuff, &sound3DBuff); // [library:audio]

                volume = sound->volume;
                if (mode == 2) volume -= 800;
                audio_buffer_set_volume(soundBuff, volume); // [library:audio]

                audio_buffer_set_min_distance(sound3DBuff, s_sound3d.minDistanceForAttenuation, SOUND3D_DS3D_DEFERRED); // [library:audio]
                audio_buffer_set_max_distance(sound3DBuff, s_sound3d.maxDistance, SOUND3D_DS3D_DEFERRED); // [library:audio]

                if (mode == 0) {
                    audio_buffer_set_3d_mode(sound3DBuff, SOUND3D_DS3DMODE_NORMAL, // [library:audio]
                                             SOUND3D_DS3D_DEFERRED);
                    frame->GetScene(&scene);
                    sound_attached_stop_by_entity(frame, sound3DBuff);
                    sound_instance_track_register(frame, sound3DBuff);
                    sound_buffer_association_add(frame, soundBuff, sound3DBuff);
                    frame->GetPosition(scene, &pos);
                    audio_buffer_set_3d_position(sound3DBuff, pos.x, pos.y, pos.z, // [library:audio]
                                                 SOUND3D_DS3D_DEFERRED);
                    scene->Release();
                } else if (mode == 1) {
                    audio_buffer_set_3d_mode(sound3DBuff, SOUND3D_DS3DMODE_NORMAL, // [library:audio]
                                             SOUND3D_DS3D_DEFERRED);
                    sound_position_set_from_vector(sound3DBuff, (const Sound3DVector*)xyz,
                                                   NULL);
                    sound_attached_stop_by_entity(NULL, sound3DBuff);
                    sound_buffer_association_add(NULL, soundBuff, sound3DBuff);
                } else if (mode == 2) {
                    audio_buffer_set_3d_mode(sound3DBuff, SOUND3D_DS3DMODE_DISABLE, // [library:audio]
                                             SOUND3D_DS3D_DEFERRED);
                    sound_attached_stop_by_entity(NULL, sound3DBuff);
                    sound_buffer_association_add(NULL, soundBuff, sound3DBuff);
                }

                audio_listener_commit_deferred_settings(s_sound3d.lp3DListenerInfo); // [library:audio]
                audio_buffer_set_current_position(soundBuff, 0); // [library:audio]
                if (loop) audio_buffer_play(soundBuff, 0, 0, SOUND3D_DSBPLAY_LOOPING); // [library:audio]
                else audio_buffer_play(soundBuff, 0, 0, 0); // [library:audio]

                return (slot * 3) + sound->voice;
            }
        }
    }

    return -1;
}

// FUNCTION: LEGORR 0x0047b020
void sound_buffer_association_add(Sound3DFrame* frame,
                                  Sound3DBuffer* soundBuff,
                                  Sound3DBuffer3D* sound3DBuff)
{
    Sound3DSoundFrameRecord* record =
        (Sound3DSoundFrameRecord*)malloc(sizeof(Sound3DSoundFrameRecord));

    record->next = s_sound3d.soundFrameRecord;
    record->frame = frame;
    record->sound3DBuff = sound3DBuff;
    record->soundBuff = soundBuff;
    s_sound3d.soundFrameRecord = record;
}

// FUNCTION: LEGORR 0x0047b050
void sound_bank_entry_volume_set(int slot, int millibels)
{
    Sound3DSoundData* sound = &s_sound3d.soundTable[slot];
    int i;

    sound->volume = sound_volume_value_valid(millibels) ? millibels : 0;
    if (sound->flags & SOUND3D_FLAG_MULTI) {
        for (i = 0; i < 3; i++) {
            if (sound->lpDsb3D[i] != NULL) {
                audio_buffer_set_volume(sound->lpDsb3D[i], sound->volume); // [library:audio]
            }
        }
    } else if (sound->lpDsb3D[0] != NULL) {
        audio_buffer_set_volume(sound->lpDsb3D[0], sound->volume); // [library:audio]
    }
}

// FUNCTION: LEGORR 0x0047b0d0
int sound_bank_entry_volume(int slot)
{
    return s_sound3d.soundTable[slot].volume;
}

// FUNCTION: LEGORR 0x0047b0f0
Sound3DBuffer* sound_handle_buffer_get(int handle)
{
    unsigned long voice;

    if (handle == -1) return NULL;
    voice = handle % 3;
    return s_sound3d.soundTable[(handle - voice) / 3].lpDsb3D[voice];
}

// FUNCTION: LEGORR 0x0047b130
void sound_buffer_stop_handle(int buffer_handle)
{
    Sound3DBuffer* soundBuff = sound_handle_buffer_get(buffer_handle);

    if (s_bank_sound_on && buffer_handle != -1 && soundBuff != NULL) {
        audio_buffer_stop(soundBuff); // [library:audio]
    }
}

// FUNCTION: LEGORR 0x0047b160
void sound_subsystem_stop_all(void)
{
    Sound3DSoundFrameRecord* item;

    for (item = s_sound3d.soundFrameRecord; item != NULL; item = item->next) {
        audio_buffer_stop(item->soundBuff); // [library:audio]
    }
    sound_stream_stop_and_release(0);
    sound_stream_stop_and_release(1);
}

// FUNCTION: LEGORR 0x0047b1a0
void sound_instance_track_register(Sound3DFrame* frame,
                                   Sound3DBuffer3D* sound3DBuff)
{
    Sound3DFrameData* data = (Sound3DFrameData*)frame->GetAppData();
    Sound3DSoundRecord* record =
        (Sound3DSoundRecord*)malloc(sizeof(Sound3DSoundRecord));
    unsigned int i;

    if (data != NULL) record->next = data->soundRecord;
    else record->next = NULL;
    record->sound3DBuff = sound3DBuff;
    record->pos.x = 0.0f;
    record->pos.y = 0.0f;
    record->pos.z = 0.0f;
    ddiscene_animation_hook_create(frame, NULL, NULL, NULL, NULL, NULL, NULL,
                                   NULL, NULL, record);

    for (i = 0; i < SOUND3D_MAXUPDATEFRAMES; i++) {
        if (s_sound3d_frames.updateFrameList[i] == NULL) {
            s_sound3d_frames.updateFrameList[i] = frame;
            return;
        }
    }
}

// FUNCTION: LEGORR 0x0047b220
void sound_instance_track_stop_and_remove(Sound3DFrame* frame,
                                          Sound3DBuffer3D* sound3DBuff)
{
    Sound3DFrameData* data;
    Sound3DSoundRecord* temp;
    unsigned int i;

    audio_buffer_release(sound3DBuff); // [library:audio]

    if (frame == NULL) {
        keyed_resource_entry_remove(frame, sound3DBuff);
        return;
    }

    data = (Sound3DFrameData*)frame->GetAppData();
    if (nested_record_list_remove_matching_key_impl(frame, sound3DBuff, data->soundRecord)) {
        keyed_resource_entry_remove(frame, sound3DBuff);
        temp = data->soundRecord;
        data->soundRecord = temp->next;
        free(temp);
    }

    for (i = 0; i < SOUND3D_MAXUPDATEFRAMES; i++) {
        if (frame == s_sound3d_frames.updateFrameList[i]) {
            s_sound3d_frames.updateFrameList[i] = NULL;
        }
    }
}

// FUNCTION: LEGORR 0x0047b2a0
void keyed_resource_entry_remove(Sound3DFrame* frame,
                                 Sound3DBuffer3D* sound3DBuff)
{
    Sound3DSoundFrameRecord* temp;

    if (nested_record_list_remove_by_key_recursive(frame, sound3DBuff,
                                                   s_sound3d.soundFrameRecord)) {
        temp = s_sound3d.soundFrameRecord;
        s_sound3d.soundFrameRecord = temp->next;
        free(temp);
    }
}

/* NestedRecordListRemoveByKeyRecursive (FUN_0047b2e0) — see record.hpp.
 * The corpus re-reads the node's next link after the recursion — a freed
 * deeper node can leave a different successor in the slot — so the port
 * re-reads before the unlink. */
// FUNCTION: LEGORR 0x0047b2e0
int nested_record_list_remove_by_key_recursive(
    Sound3DFrame* frame, Sound3DBuffer3D* sound3DBuff,
    Sound3DSoundFrameRecord* record)
{
    Sound3DSoundFrameRecord* temp;

    if (record == NULL) return 0;

    if (nested_record_list_remove_by_key_recursive(frame, sound3DBuff, record->next)) {
        temp = record->next;
        record->next = temp->next;
        free(temp);
    }

    return (sound3DBuff == record->sound3DBuff);
}

// FUNCTION: LEGORR 0x0047b330
int nested_record_list_remove_matching_key_impl(void* owner, void* key,
                                                void* opaque_record)
{
    Sound3DFrame* frame = (Sound3DFrame*)owner;
    Sound3DBuffer3D* sound3DBuff = (Sound3DBuffer3D*)key;
    Sound3DSoundRecord* record = (Sound3DSoundRecord*)opaque_record;
    Sound3DSoundRecord* temp;

    if (record == NULL) return 0;

    if (nested_record_list_remove_matching_key_impl(frame, sound3DBuff, record->next)) {
        keyed_resource_entry_remove(frame, sound3DBuff);
        temp = record->next;
        record->next = temp->next;
        free(temp);
    }

    return (sound3DBuff == record->sound3DBuff);
}

// FUNCTION: LEGORR 0x0047b390
void sound_source_spatial_refresh_all(void)
{
    Sound3DSoundFrameRecord* item = s_sound3d.soundFrameRecord;
    Sound3DSoundFrameRecord* next;
    DWORD status;
    unsigned int i;

    for (i = 0; i < SOUND3D_MAXUPDATEFRAMES; i++) {
        if (s_sound3d_frames.updateFrameList[i] != NULL) {
            sound_source_spatial_refresh(s_sound3d_frames.updateFrameList[i], NULL, 0.0f);
        }
    }

    while (item != NULL) {
        next = item->next;
        audio_buffer_get_status(item->soundBuff, &status); // [library:audio]
        if (!(status & SOUND3D_DSBSTATUS_PLAYING)) {
            sound_instance_track_stop_and_remove(item->frame, item->sound3DBuff);
        }
        item = next;
    }

    sound_stream_ring_buffer_tick(0);
    sound_stream_ring_buffer_tick(1);
}

// FUNCTION: LEGORR 0x0047b410
void sound_source_spatial_refresh(Sound3DFrame* frame, void* arg, float delta)
{
    Sound3DFrameData* data = (Sound3DFrameData*)frame->GetAppData();
    Sound3DSoundRecord* item = data->soundRecord;
    Sound3DFrame* scene;
    Sound3DVector pos;

    frame->GetScene(&scene);
    while (item != NULL) {
        frame->GetPosition(scene, &pos);
        if (!vec_exact_equal(&pos.x, &item->pos.x)) {
            audio_buffer_set_3d_position(item->sound3DBuff, pos.x, pos.y, pos.z, // [library:audio]
                                         SOUND3D_DS3D_IMMEDIATE);
            item->pos = pos;
        }
        item = item->next;
    }
    scene->Release();
}

// FUNCTION: LEGORR 0x0047b4a0
void sound_tracked_buffers_set_min_distance(float dist)
{
    Sound3DSoundFrameRecord* item;

    s_sound3d.minDistanceForAttenuation = dist;
    for (item = s_sound3d.soundFrameRecord; item != NULL; item = item->next) {
        audio_buffer_set_min_distance(item->sound3DBuff, dist, // [library:audio]
                                      SOUND3D_DS3D_IMMEDIATE);
    }
}

// FUNCTION: LEGORR 0x0047b4d0
void sound_tracked_buffers_set_max_distance(float dist)
{
    Sound3DSoundFrameRecord* item;

    s_sound3d.maxDistance = dist;
    for (item = s_sound3d.soundFrameRecord; item != NULL; item = item->next) {
        audio_buffer_set_max_distance(item->sound3DBuff, dist, // [library:audio]
                                      SOUND3D_DS3D_IMMEDIATE);
    }
}

/* SoundBufferVolumeSet (FUN_0047b500): DirectSound's attenuation maps to
 * the port's single master stream gain. Invalid corpus attenuation values
 * are ignored by the same range predicate used by sound-bank records. */
// FUNCTION: LEGORR 0x0047b500
void audio_sound_volume_set(int millibels)
{
    if (s_bank_sound_on) {
        if (sound_volume_value_valid(millibels)) {
            s_sound3d.volume = millibels;
            audio_buffer_set_volume(s_sound3d.lpDSBuff, millibels); // [library:audio]
        }
    }
}

/* SoundVolumeIndexSet (FUN_0047b530): the table lookup is bounded to the
 * eleven menu slider entries, then the selected attenuation is applied. */
// FUNCTION: LEGORR 0x0047b530
void audio_sound_volume_index_set(int index)
{
    if (index >= 0 && index <= 10) {
        audio_sound_volume_set(s_sfx_attenuation[index]);
    }
}

// FUNCTION: LEGORR 0x0047b550
void audio_sound_volume_minus_300(void)
{
    if (s_bank_sound_on) {
        s_sound3d.volume = -300;
        audio_buffer_set_volume(s_sound3d.lpDSBuff, -300); // [library:audio]
    }
}

// FUNCTION: LEGORR 0x0047b580
int sound_wave_sample_load(Sound3DSoundData* sound, const char* fName,
                           int simultaneous)
{
    void* fileData;
    int size;
    HMMIO hmmio;
    MMCKINFO mmckinfoParent;
    MMCKINFO mmckinfoSubchunk;
    UINT cbActualRead;
    int nError;

    memset(sound, 0, sizeof(Sound3DSoundData));

    if ((fileData = read_whole_file(fName, &size)) != NULL) {
        if ((nError = sound_wave_memory_format_read(fileData, size, &hmmio, &sound->pwf,
                                                    &mmckinfoParent)) != 0) goto ERROR_LOADING;
        if ((nError = sound_wave_data_chunk_open(&hmmio, &mmckinfoSubchunk,
                                                 &mmckinfoParent)) != 0) goto ERROR_LOADING;
        if ((sound->pbData = (BYTE*)GlobalAlloc(GMEM_FIXED, mmckinfoSubchunk.cksize)) == NULL)
            goto ERROR_LOADING;
        if ((nError = sound_wave_data_read(hmmio, mmckinfoSubchunk.cksize, sound->pbData,
                                           &mmckinfoSubchunk, &cbActualRead)) != 0) goto ERROR_LOADING;
        sound->size = cbActualRead;
        if (hmmio != NULL) {
            audio_mmio_close(hmmio, 0); // [library:audio]
            hmmio = NULL;
        }
        if (sound_record_create_buffers(sound, simultaneous)) {
            if (sound_wave_sample_duplicate_buffer_create(sound)) {
                if (sound->pbData != NULL) {
                    GlobalFree(sound->pbData);
                    sound->pbData = NULL;
                }
                free(fileData);
                return 1;
            }
        }
    }

ERROR_LOADING:
    if (sound->pbData != NULL) {
        GlobalFree(sound->pbData);
        sound->pbData = NULL;
    }
    return 0;
}

// FUNCTION: LEGORR 0x0047b6c0
int sound_record_create_buffers(Sound3DSoundData* sound, int simultaneous)
{
    Sound3DBufferDesc dsbd;
    unsigned long i;

    memset(&dsbd, 0, sizeof(dsbd));
    dsbd.dwSize = sizeof(dsbd);
    dsbd.dwFlags = SOUND3D_SAMPLE_CAPS;
    dsbd.dwBufferBytes = sound->size;
    dsbd.lpwfxFormat = sound->pwf;

    if (audio_create_buffer(s_sound3d.lpDSnd, dsbd.dwFlags, dsbd.dwBufferBytes, dsbd.lpwfxFormat, // [library:audio]
                            &sound->lpDsb3D[0]) != SOUND3D_DS_OK) {
        return 0;
    }

    sound->voice = 0;
    if (simultaneous) {
        for (i = 1; i < 3; i++) {
            if (audio_duplicate_buffer(s_sound3d.lpDSnd, sound->lpDsb3D[0], &sound->lpDsb3D[i]) != SOUND3D_DS_OK) { // [library:audio]
                return 0;
            }
        }
    }

    return 1;
}

// FUNCTION: LEGORR 0x0047b790
int sound_wave_sample_duplicate_buffer_create(Sound3DSoundData* sound)
{
    void* lpvAudio1;
    void* lpvAudio2;
    DWORD dwBytes1 = 0;
    DWORD dwBytes2 = 0;
    long hr;

    hr = audio_buffer_lock(sound->lpDsb3D[0], sound->offset, sound->size, // [library:audio]
                           &lpvAudio1, &dwBytes1, &lpvAudio2, &dwBytes2, 0);
    if (hr == SOUND3D_DSERR_BUFFERLOST) {
        audio_buffer_restore(sound->lpDsb3D[0]); // [library:audio]
        audio_buffer_lock(sound->lpDsb3D[0], sound->offset, sound->size, // [library:audio]
                          &lpvAudio1, &dwBytes1, &lpvAudio2, &dwBytes2, 0);
    } else if (hr != SOUND3D_DS_OK) {
        return 0;
    }

    memcpy(lpvAudio1, sound->pbData, dwBytes1);
    if (lpvAudio2 != NULL) {
        memcpy(lpvAudio2, sound->pbData + dwBytes1, dwBytes2);
    }

    if (audio_buffer_unlock(sound->lpDsb3D[0], lpvAudio1, dwBytes1, // [library:audio]
                            lpvAudio2, dwBytes2) != SOUND3D_DS_OK) {
        return 0;
    }
    if (audio_buffer_get_frequency(sound->lpDsb3D[0], &sound->freq) != SOUND3D_DS_OK) { // [library:audio]
        return 0;
    }
    return audio_buffer_get_volume(sound->lpDsb3D[0], &sound->volume) == SOUND3D_DS_OK; // [library:audio]
}

// FUNCTION: LEGORR 0x0047b8e0
float sound_streamed_wave_buffer_step_rate(int slot)
{
    Sound3DSoundData* sound;

    if (slot == -1) return 0.0f;

    sound = &s_sound3d.soundTable[slot];
    if (sound->flags & SOUND3D_FLAG_STREAM) {
        return (float)sound->size / sound->avgBytesPerSec;
    }
    return (float)sound->size / sound->pwf->nAvgBytesPerSec;
}

// FUNCTION: LEGORR 0x0047b970
int sound_streamed_wave_play(const char* fName, int loop, int volume)
{
    Sound3DStreamData* stream = loop ? &s_sound3d.loopStreamData : &s_sound3d.streamData;

    if (stream->playing || stream->fileOpen) {
        sound_stream_stop_and_release(loop);
    }

    if (sound_streamed_wave_buffer_setup(fName, loop, volume)) {
        audio_buffer_set_current_position(SOUND3D_STREAM_BUFFER(loop), 0); // [library:audio]
        audio_buffer_play(SOUND3D_STREAM_BUFFER(loop), 0, 0, // [library:audio]
                          SOUND3D_DSBPLAY_LOOPING);
        stream->playing = 1;
        return 1;
    }

    stream->playing = 0;
    return 0;
}

// FUNCTION: LEGORR 0x0047ba20
int sound_stream_stop_and_release(int stream_index)
{
    Sound3DStreamData* stream =
        stream_index ? &s_sound3d.loopStreamData : &s_sound3d.streamData;

    stream->playing = 0;
    if (stream->fileOpen) {
        audio_buffer_stop(SOUND3D_STREAM_BUFFER(stream_index)); // [library:audio]
        sound_wave_handle_and_buffer_release(&stream->hmmio, &stream->pwf);
        audio_buffer_release(SOUND3D_STREAM_BUFFER(stream_index)); // [library:audio]
        stream->fileOpen = 0;
    }
    return 1;
}

// FUNCTION: LEGORR 0x0047baa0
int sound_streamed_wave_buffer_setup(const char* fName, int loop, int volume)
{
    Sound3DStreamData* stream = loop ? &s_sound3d.loopStreamData : &s_sound3d.streamData;
    Sound3DBufferDesc dsbd;
    Sound3DBuffer* soundBuff;
    unsigned long remainder;
    int nError;

    if (sound_wave_file_open_read_chunks(fName, &stream->hmmio, &stream->pwf,
                                         &stream->mmckinfoParent) != MMSYSERR_NOERROR) {
        return 0;
    }

    if (stream->pwf->wFormatTag != 1) {
        sound_wave_handle_and_buffer_release(&stream->hmmio, &stream->pwf);
        return 0;
    }

    if ((nError = sound_wave_data_chunk_open(&stream->hmmio, &stream->mmckinfoData,
                                             &stream->mmckinfoParent)) != MMSYSERR_NOERROR) {
        sound_wave_handle_and_buffer_release(&stream->hmmio, &stream->pwf);
        return 0;
    }

    stream->notifySize = (stream->pwf->nSamplesPerSec * stream->pwf->nBlockAlign * 3) / 4;
    remainder = stream->notifySize % stream->pwf->nBlockAlign;
    if (remainder) stream->notifySize += stream->pwf->nBlockAlign - remainder;

    memset(&dsbd, 0, sizeof(dsbd));
    dsbd.dwSize = sizeof(dsbd);
    dsbd.dwFlags = SOUND3D_STREAM_CAPS;
    dsbd.dwBufferBytes = stream->bufferSize = stream->notifySize * 4;
    dsbd.lpwfxFormat = stream->pwf;

    if (audio_create_buffer(s_sound3d.lpDSnd, dsbd.dwFlags, dsbd.dwBufferBytes, dsbd.lpwfxFormat, // [library:audio]
                            &soundBuff) != SOUND3D_DS_OK) {
        return 0;
    }

    if (loop) s_sound3d.lpDSLoopStreamBuff = soundBuff;
    else s_sound3d.lpDSStreamBuff = soundBuff;

    audio_buffer_set_volume(SOUND3D_STREAM_BUFFER(loop), volume); // [library:audio]

    stream->fileEnd = 0;
    stream->writeOffset = 0;
    stream->writeProgress = stream->notifySize * 2;
    stream->looping = loop;
    sound_stream_drain_check(loop);
    stream->playEnd = 0;
    stream->fileOpen = 1;

    return 1;
}

// FUNCTION: LEGORR 0x0047bc30
int sound_stream_drain_check(int loop)
{
    Sound3DStreamData* stream = loop ? &s_sound3d.loopStreamData : &s_sound3d.streamData;
    void* lpvAudio1;
    DWORD dwBytes1;
    void* lpvAudio2;
    DWORD dwBytes2;
    UINT cbActualRead;
    UINT pos;

    if (audio_buffer_lock(SOUND3D_STREAM_BUFFER(loop), 0, // [library:audio]
            stream->bufferSize, &lpvAudio1, &dwBytes1, &lpvAudio2, &dwBytes2, 0) != SOUND3D_DS_OK) {
        return 0;
    }

    sound_wave_data_read(stream->hmmio, dwBytes1, (BYTE*)lpvAudio1, &stream->mmckinfoData,
                         &cbActualRead);
    if (cbActualRead < dwBytes1) {
        if (!stream->looping) {
            if (cbActualRead < stream->notifySize) stream->fileEnd = 1;
            memset((BYTE*)lpvAudio1 + cbActualRead,
                   (BYTE)((stream->pwf->wBitsPerSample == 8) ? 128 : 0),
                   dwBytes1 - cbActualRead);
        } else {
            pos = cbActualRead;
            do {
                sound_wave_data_chunk_open(&stream->hmmio, &stream->mmckinfoData,
                                           &stream->mmckinfoParent);
                sound_wave_data_read(stream->hmmio, dwBytes1 - pos, (BYTE*)lpvAudio1 + pos,
                                     &stream->mmckinfoData, &cbActualRead);
                pos += cbActualRead;
            } while (pos < dwBytes1);
        }
    }

    audio_buffer_unlock(SOUND3D_STREAM_BUFFER(loop), lpvAudio1, // [library:audio]
                        dwBytes1, NULL, 0);

    stream->writeOffset += dwBytes1;
    if (stream->writeOffset >= stream->bufferSize) stream->writeOffset -= stream->bufferSize;
    stream->playProgress = 0;
    stream->lastPos = 0;
    return 1;
}

// FUNCTION: LEGORR 0x0047bdb0
void sound_stream_ring_buffer_tick(int loop)
{
    Sound3DStreamData* stream = loop ? &s_sound3d.loopStreamData : &s_sound3d.streamData;
    DWORD playPos;
    DWORD writePos;
    void* lpvAudio1;
    DWORD dwBytes1;
    UINT cbActualRead;
    UINT pos;
    int first;

    if (stream->playing && stream->fileOpen) {
        audio_buffer_get_current_position(SOUND3D_STREAM_BUFFER(loop), &playPos, &writePos); // [library:audio]
        stream->playProgress += ((playPos < stream->lastPos) ? stream->bufferSize + playPos : playPos) -
                                stream->lastPos;
        stream->lastPos = playPos;

        first = 0;
        while (stream->playProgress > stream->writeProgress) {
            cbActualRead = 0;
            if (stream->playEnd && !first) {
                sound_stream_stop_and_release(loop);
                return;
            }
            first = 1;

            if (!stream->fileEnd) {
                if (audio_buffer_lock(SOUND3D_STREAM_BUFFER(loop), stream->writeOffset, stream->notifySize, &lpvAudio1, &dwBytes1, // [library:audio]
                        NULL, NULL, 0) != SOUND3D_DS_OK) {
                    return;
                }
                sound_wave_data_read(stream->hmmio, dwBytes1, (BYTE*)lpvAudio1,
                                     &stream->mmckinfoData, &cbActualRead);
                if (cbActualRead < dwBytes1) {
                    if (!stream->looping) {
                        stream->fileEnd = 1;
                        memset((BYTE*)lpvAudio1 + cbActualRead,
                               (BYTE)((stream->pwf->wBitsPerSample == 8) ? 128 : 0),
                               dwBytes1 - cbActualRead);
                    } else {
                        pos = cbActualRead;
                        do {
                            sound_wave_data_chunk_open(&stream->hmmio, &stream->mmckinfoData,
                                                       &stream->mmckinfoParent);
                            sound_wave_data_read(stream->hmmio, dwBytes1 - pos,
                                                 (BYTE*)lpvAudio1 + pos,
                                                 &stream->mmckinfoData, &cbActualRead);
                            pos += cbActualRead;
                        } while (pos < dwBytes1);
                    }
                }
                audio_buffer_unlock(SOUND3D_STREAM_BUFFER(loop), lpvAudio1, dwBytes1, NULL, 0); // [library:audio]
                stream->writeProgress += dwBytes1;
                stream->writeOffset += dwBytes1;
                if (stream->bufferSize <= stream->writeOffset) {
                    stream->writeOffset -= stream->bufferSize;
                }
            } else {
                audio_buffer_lock(SOUND3D_STREAM_BUFFER(loop), stream->writeOffset, stream->notifySize, &lpvAudio1, &dwBytes1, // [library:audio]
                    NULL, NULL, 0);
                memset(lpvAudio1, (BYTE)((stream->pwf->wBitsPerSample == 8) ? 128 : 0), dwBytes1);
                audio_buffer_unlock(SOUND3D_STREAM_BUFFER(loop), lpvAudio1, dwBytes1, NULL, 0); // [library:audio]
                if ((stream->mmckinfoParent.cksize > stream->notifySize &&
                     stream->playProgress >= stream->mmckinfoParent.cksize - stream->notifySize) ||
                    stream->playProgress >= stream->mmckinfoParent.cksize) {
                    stream->playEnd = 1;
                }
                stream->writeProgress += dwBytes1;
            }
        }
    }
}

// FUNCTION: LEGORR 0x0047c0c0
int vec_exact_equal(const float* a, const float* b) /* FUN_0047c0c0 */
{
    return a[0] == b[0] && a[1] == b[1] && a[2] == b[2];
}

// FUNCTION: LEGORR 0x0047c100
float ledger_stub_0047c100_DirectSoundListenerRolloffSet(float rolloff)
{
    if (s_bank_sound_on) {
        if (rolloff < 0.0f) rolloff = 0.0f;
        if (rolloff > 10.0f) rolloff = 10.0f;
        audio_listener_set_rolloff_factor(s_sound3d.lp3DListenerInfo, rolloff, SOUND3D_DS3D_IMMEDIATE); // [library:audio]
    }
    return rolloff;
}

// FUNCTION: LEGORR 0x0047c160
int32_t constant_negative_10000_get(void)
{
    return -10000;
}


/* The unit's .bss, defined after every function so that no function's
 * symbol counter moves (finding 538), in the original's order: the 8 bytes
 * before s_sound3d that nothing in the image reads, s_sound3d,
 * Sound3D_Initialised, then the frame state. Zero-initialised externals,
 * which MSVC 5.0 emits in definition order (finding 686). */
// GLOBAL: LEGORR 0x005065b0
unsigned int sound3d_unknown_5065b0[2] = { 0 }; /* never read */
// GLOBAL: LEGORR 0x005065b8
Sound3DGlobs s_sound3d = { 0 };
/* Sound3D_Initialised and the frame state (audio.h's Sound3D_State in the
 * matching build). Shared by both backends: soundsys.c and sound.cpp read
 * the flag. */
// GLOBAL: LEGORR 0x00533878
Sound3D_State sound3dState = { 0 };
