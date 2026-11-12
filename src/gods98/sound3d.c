// SPDX-License-Identifier: LGPL-3.0-or-later
/* Original translation unit G98-009 (text 0047a640-0047c170; .rdata
 * 0049e964-0049e968; .data 004aae30-004aae5c; .bss 005065b8-00533904;
 * finding 505): DirectSound. It owns the 600-slot sample table, the primary
 * buffer and its 3D listener, the buffers attached to frames, and the two
 * streamed wave buffers (one plain, one looping). The functions keep the
 * port's names; the comments give descriptive Sound3D_* names, which the
 * binary does not attest.
 *
 * Two backends (ADR 0006 step 4):
 * - SOUND3D_DIRECTSOUND, the Windows builds (the matching build and the
 *   MinGW reference build), compile the original's code. It calls
 *   DirectSound and Direct3DRM frames through their COM method tables.
 * - The native port has no DirectSound, and its frames are not COM objects.
 *   It compiles the port's model of the same functions over the SDL audio
 *   player (audio.cpp) and the port's sample bank (sound.cpp).
 * Each function has one annotation over an #if group that holds both
 * bodies; the Windows body comes first. Native-only helpers are at the end.
 *
 * C-compatible (ADR 0005): C in the matching build, C++ elsewhere, so the
 * Windows branch is also valid C++ (MinGW compiles it as C++). */
#include "sound3d.h"
#include "audio.h"

#if !defined(LEGORRR_BACKEND_MINIWIN)
#define SOUND3D_DIRECTSOUND 1
#endif

#if defined(SOUND3D_DIRECTSOUND)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "crt/vc5names.h"
#include "winmm.h"
#include "soundsys.h"
#include "resource.h"
#include "files_paths.h"
#include "container_frame.h"

/* The COM interfaces the unit calls, with the method-table slots at their
 * Win32 offsets. MSVC 5.0's dsound.h is DirectX 3's and has no 3D
 * interfaces, so the unit declares the DirectX 5 layouts it uses. */
typedef struct Sound3DIID {
    unsigned long data1;
    unsigned short data2;
    unsigned short data3;
    unsigned char data4[8];
} Sound3DIID;

typedef struct Sound3DVector {
    float x;
    float y;
    float z;
} Sound3DVector;

/* DSBUFFERDESC, DirectX 5 (0x14 bytes). */
typedef struct Sound3DBufferDesc {
    DWORD dwSize;
    DWORD dwFlags;
    DWORD dwBufferBytes;
    DWORD dwReserved;
    WAVEFORMATEX* lpwfxFormat;
} Sound3DBufferDesc;

typedef struct Sound3DDevice Sound3DDevice;          /* IDirectSound */
typedef struct Sound3DBuffer Sound3DBuffer;          /* IDirectSoundBuffer */
typedef struct Sound3DListener Sound3DListener;      /* IDirectSound3DListener */
typedef struct Sound3DBuffer3D Sound3DBuffer3D;      /* IDirectSound3DBuffer */
typedef struct Sound3DFrame Sound3DFrame;            /* IDirect3DRMFrame3 */

typedef struct Sound3DDeviceVtbl {
    long (WINAPI* QueryInterface)(Sound3DDevice*, const Sound3DIID*, void**);
    unsigned long (WINAPI* AddRef)(Sound3DDevice*);
    unsigned long (WINAPI* Release)(Sound3DDevice*);
    long (WINAPI* CreateSoundBuffer)(Sound3DDevice*, Sound3DBufferDesc*,
                                     Sound3DBuffer**, void*);
    long (WINAPI* GetCaps)(Sound3DDevice*, void*);
    long (WINAPI* DuplicateSoundBuffer)(Sound3DDevice*, Sound3DBuffer*,
                                        Sound3DBuffer**);
    long (WINAPI* SetCooperativeLevel)(Sound3DDevice*, void*, DWORD);
} Sound3DDeviceVtbl;

struct Sound3DDevice {
    const Sound3DDeviceVtbl* lpVtbl;
};

typedef struct Sound3DBufferVtbl {
    long (WINAPI* QueryInterface)(Sound3DBuffer*, const Sound3DIID*, void**);
    unsigned long (WINAPI* AddRef)(Sound3DBuffer*);
    unsigned long (WINAPI* Release)(Sound3DBuffer*);
    long (WINAPI* GetCaps)(Sound3DBuffer*, void*);
    long (WINAPI* GetCurrentPosition)(Sound3DBuffer*, DWORD*, DWORD*);
    long (WINAPI* GetFormat)(Sound3DBuffer*, WAVEFORMATEX*, DWORD, DWORD*);
    long (WINAPI* GetVolume)(Sound3DBuffer*, LONG*);
    long (WINAPI* GetPan)(Sound3DBuffer*, LONG*);
    long (WINAPI* GetFrequency)(Sound3DBuffer*, DWORD*);
    long (WINAPI* GetStatus)(Sound3DBuffer*, DWORD*);
    long (WINAPI* Initialize)(Sound3DBuffer*, Sound3DDevice*,
                              Sound3DBufferDesc*);
    long (WINAPI* Lock)(Sound3DBuffer*, DWORD, DWORD, void**, DWORD*, void**,
                        DWORD*, DWORD);
    long (WINAPI* Play)(Sound3DBuffer*, DWORD, DWORD, DWORD);
    long (WINAPI* SetCurrentPosition)(Sound3DBuffer*, DWORD);
    long (WINAPI* SetFormat)(Sound3DBuffer*, WAVEFORMATEX*);
    long (WINAPI* SetVolume)(Sound3DBuffer*, LONG);
    long (WINAPI* SetPan)(Sound3DBuffer*, LONG);
    long (WINAPI* SetFrequency)(Sound3DBuffer*, DWORD);
    long (WINAPI* Stop)(Sound3DBuffer*);
    long (WINAPI* Unlock)(Sound3DBuffer*, void*, DWORD, void*, DWORD);
    long (WINAPI* Restore)(Sound3DBuffer*);
} Sound3DBufferVtbl;

struct Sound3DBuffer {
    const Sound3DBufferVtbl* lpVtbl;
};

typedef struct Sound3DListenerVtbl {
    long (WINAPI* QueryInterface)(Sound3DListener*, const Sound3DIID*,
                                  void**);
    unsigned long (WINAPI* AddRef)(Sound3DListener*);
    unsigned long (WINAPI* Release)(Sound3DListener*);
    long (WINAPI* GetAllParameters)(Sound3DListener*, void*);
    long (WINAPI* GetDistanceFactor)(Sound3DListener*, float*);
    long (WINAPI* GetDopplerFactor)(Sound3DListener*, float*);
    long (WINAPI* GetOrientation)(Sound3DListener*, Sound3DVector*,
                                  Sound3DVector*);
    long (WINAPI* GetPosition)(Sound3DListener*, Sound3DVector*);
    long (WINAPI* GetRolloffFactor)(Sound3DListener*, float*);
    long (WINAPI* GetVelocity)(Sound3DListener*, Sound3DVector*);
    long (WINAPI* SetAllParameters)(Sound3DListener*, void*, DWORD);
    long (WINAPI* SetDistanceFactor)(Sound3DListener*, float, DWORD);
    long (WINAPI* SetDopplerFactor)(Sound3DListener*, float, DWORD);
    long (WINAPI* SetOrientation)(Sound3DListener*, float, float, float,
                                  float, float, float, DWORD);
    long (WINAPI* SetPosition)(Sound3DListener*, float, float, float, DWORD);
    long (WINAPI* SetRolloffFactor)(Sound3DListener*, float, DWORD);
    long (WINAPI* SetVelocity)(Sound3DListener*, float, float, float, DWORD);
    long (WINAPI* CommitDeferredSettings)(Sound3DListener*);
} Sound3DListenerVtbl;

struct Sound3DListener {
    const Sound3DListenerVtbl* lpVtbl;
};

typedef struct Sound3DBuffer3DVtbl {
    long (WINAPI* QueryInterface)(Sound3DBuffer3D*, const Sound3DIID*,
                                  void**);
    unsigned long (WINAPI* AddRef)(Sound3DBuffer3D*);
    unsigned long (WINAPI* Release)(Sound3DBuffer3D*);
    long (WINAPI* GetAllParameters)(Sound3DBuffer3D*, void*);
    long (WINAPI* GetConeAngles)(Sound3DBuffer3D*, DWORD*, DWORD*);
    long (WINAPI* GetConeOrientation)(Sound3DBuffer3D*, Sound3DVector*);
    long (WINAPI* GetConeOutsideVolume)(Sound3DBuffer3D*, LONG*);
    long (WINAPI* GetMaxDistance)(Sound3DBuffer3D*, float*);
    long (WINAPI* GetMinDistance)(Sound3DBuffer3D*, float*);
    long (WINAPI* GetMode)(Sound3DBuffer3D*, DWORD*);
    long (WINAPI* GetPosition)(Sound3DBuffer3D*, Sound3DVector*);
    long (WINAPI* GetVelocity)(Sound3DBuffer3D*, Sound3DVector*);
    long (WINAPI* SetAllParameters)(Sound3DBuffer3D*, void*, DWORD);
    long (WINAPI* SetConeAngles)(Sound3DBuffer3D*, DWORD, DWORD, DWORD);
    long (WINAPI* SetConeOrientation)(Sound3DBuffer3D*, float, float, float,
                                      DWORD);
    long (WINAPI* SetConeOutsideVolume)(Sound3DBuffer3D*, LONG, DWORD);
    long (WINAPI* SetMaxDistance)(Sound3DBuffer3D*, float, DWORD);
    long (WINAPI* SetMinDistance)(Sound3DBuffer3D*, float, DWORD);
    long (WINAPI* SetMode)(Sound3DBuffer3D*, DWORD, DWORD);
    long (WINAPI* SetPosition)(Sound3DBuffer3D*, float, float, float, DWORD);
    long (WINAPI* SetVelocity)(Sound3DBuffer3D*, float, float, float, DWORD);
} Sound3DBuffer3DVtbl;

struct Sound3DBuffer3D {
    const Sound3DBuffer3DVtbl* lpVtbl;
};

/* IDirect3DRMFrame3, with the slots the unit calls. */
typedef struct Sound3DFrameVtbl {
    long (WINAPI* QueryInterface)(Sound3DFrame*, const Sound3DIID*, void**);
    unsigned long (WINAPI* AddRef)(Sound3DFrame*);
    unsigned long (WINAPI* Release)(Sound3DFrame*);         /* +0x08 */
    void* slots_0c[4];
    void* (WINAPI* GetAppData)(Sound3DFrame*);              /* +0x1c */
    void* slots_20[16];
    long (WINAPI* GetPosition)(Sound3DFrame*, Sound3DFrame*,
                               Sound3DVector*);             /* +0x60 */
    void* slots_64[1];
    long (WINAPI* GetScene)(Sound3DFrame*, Sound3DFrame**); /* +0x68 */
    void* slots_6c[4];
    long (WINAPI* GetOrientation)(Sound3DFrame*, Sound3DFrame*,
                                  Sound3DVector*, Sound3DVector*); /* +0x7c */
} Sound3DFrameVtbl;

struct Sound3DFrame {
    const Sound3DFrameVtbl* lpVtbl;
};

/* DirectSoundCreate (DSOUND ordinal 1), called through its import thunk. */
long WINAPI DirectSoundCreate(Sound3DIID* guid, Sound3DDevice** device,
                              void* outer);

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
#define SOUND3D_MAXUPDATEFRAMES 20

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

/* The frame app data (Container.c); the unit reads only its sound list. */
typedef struct Sound3DFrameData {
    void* reserved[9];
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

/* The globals after the initialised flag (00533878). */
typedef struct Sound3DFrameGlobs {
    LONG windowsVolume;                          /* +0x00 */
    unsigned long reserved_04;
    Sound3DFrame* updateFrameList[SOUND3D_MAXUPDATEFRAMES]; /* +0x08 */
    unsigned long reserved_58;
    Sound3DVector oldPos;                        /* +0x5c */
    unsigned long reserved_68;
    Sound3DVector oldOrientFront;                /* +0x6c */
    unsigned long reserved_78;
    Sound3DVector oldOrientUp;                   /* +0x7c */
} Sound3DFrameGlobs;

#define SOUND3D_STREAM_BUFFER(loop) \
    ((loop) ? s_sound3d.lpDSLoopStreamBuff : s_sound3d.lpDSStreamBuff)

// GLOBAL: LEGORR 0x004a04f8
static const Sound3DIID s_iid_directsound3d_listener = {
    0x279afa84, 0x4981, 0x11ce,
    { 0xa5, 0x21, 0x00, 0x20, 0xaf, 0x0b, 0xe5, 0x60 }
};

// GLOBAL: LEGORR 0x004a0508
static const Sound3DIID s_iid_directsound3d_buffer = {
    0x279afa86, 0x4981, 0x11ce,
    { 0xa5, 0x21, 0x00, 0x20, 0xaf, 0x0b, 0xe5, 0x60 }
};

// GLOBAL: LEGORR 0x005065b8
static Sound3DGlobs s_sound3d;

// GLOBAL: LEGORR 0x0053387c
static Sound3DFrameGlobs s_sound3d_frames;

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

#else /* native port */

#ifdef __cplusplus
/* C linkage for GAME-021 lego_unit.c, which calls these two. */
extern "C" void sound_tracked_buffers_set_max_distance(uint32_t value);
extern "C" float ledger_stub_0047c100_DirectSoundListenerRolloffSet(float rolloff);
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "resource.hpp"
#include "heap.hpp"
#include "sound.hpp"
#include "audio.hpp"
#include "record.hpp"
#include "view.hpp"
#include "simtick.hpp"
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

// NestedRecordListRemoveMatchingKey
typedef struct NestedResourceRecord {
    void* key;
    void* reserved[3];
    struct NestedResourceRecord* child_or_next;
} NestedResourceRecord;

/* SoundBufferAssociationAdd (FUN_0047b020) — see sound.hpp. */
void* s_buffer_assoc_head; /* DAT_00506684 role: unowned */

static int s_listener_view = 0;
static uint32_t s_tracked_buffer_min_distance;
static uint32_t s_tracked_buffer_max_distance;
static int s_audio_buffer_volume;

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
#if defined(__i386__) && (defined(__GNUC__) || defined(__clang__))
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

#endif /* SOUND3D_DIRECTSOUND */

/* Sound3D_Initialised (00533878). Shared by both backends: soundsys.c and
 * sound.cpp read it. */
// GLOBAL: LEGORR 0x00533878
int s_bank_sound_on = 0;

/* The SFX slider's attenuation steps, in millibels (audio.cpp reads it). */
// GLOBAL: LEGORR 0x004aae30
int s_sfx_attenuation[11] = {
    -10000, -5000, -3500, -2000, -1500, -1000,
      -700,  -400,  -200,   -80,     0
};

// FUNCTION: LEGORR 0x0047a640
#if defined(SOUND3D_DIRECTSOUND)
int audio_system_init(void* window)
{
    Sound3DBufferDesc dsbd;
    WAVEFORMATEX wfx;
    int r;

    s_bank_sound_on = 0;
    s_sound3d.listenerFrame = NULL;

    do {
        if (DirectSoundCreate(NULL, &s_sound3d.lpDSnd, NULL) == SOUND3D_DS_OK) {
            if (s_sound3d.lpDSnd->lpVtbl->SetCooperativeLevel(
                    s_sound3d.lpDSnd, window, SOUND3D_DSSCL_PRIORITY) == SOUND3D_DS_OK) {
                memset(&dsbd, 0, sizeof(dsbd));
                dsbd.dwSize = sizeof(dsbd);
                dsbd.dwFlags = SOUND3D_PRIMARY_CAPS;
                if (s_sound3d.lpDSnd->lpVtbl->CreateSoundBuffer(
                        s_sound3d.lpDSnd, &dsbd, &s_sound3d.lpDSBuff, NULL) == SOUND3D_DS_OK) {
                    wfx.wFormatTag = 1;
                    wfx.nChannels = 2;
                    wfx.wBitsPerSample = 16;
                    wfx.nSamplesPerSec = 22050;
                    wfx.nBlockAlign = wfx.nChannels * wfx.wBitsPerSample / 8;
                    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
                    wfx.cbSize = 0;
                    s_sound3d.lpDSBuff->lpVtbl->SetFormat(s_sound3d.lpDSBuff, &wfx);
                    if (s_sound3d.lpDSBuff->lpVtbl->QueryInterface(
                            s_sound3d.lpDSBuff, &s_iid_directsound3d_listener,
                            (void**)&s_sound3d.lp3DListenerInfo) == SOUND3D_DS_OK) {
                        s_sound3d.lp3DListenerInfo->lpVtbl->SetDopplerFactor(
                            s_sound3d.lp3DListenerInfo, 100.0f, SOUND3D_DS3D_IMMEDIATE);
                    } else {
                        s_sound3d.lpDSBuff->lpVtbl->Release(s_sound3d.lpDSBuff);
                        s_sound3d.lpDSnd->lpVtbl->Release(s_sound3d.lpDSnd);
                        s_sound3d.lpDSnd = NULL;
                    }
                } else {
                    s_sound3d.lpDSnd->lpVtbl->Release(s_sound3d.lpDSnd);
                    s_sound3d.lpDSnd = NULL;
                }
            } else {
                s_sound3d.lpDSnd->lpVtbl->Release(s_sound3d.lpDSnd);
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
        s_sound3d.lpDSBuff->lpVtbl->Play(s_sound3d.lpDSBuff, 0, 0, SOUND3D_DSBPLAY_LOOPING);
        s_sound3d.lp3DListenerInfo->lpVtbl->SetRolloffFactor(
            s_sound3d.lp3DListenerInfo, 1.0f, SOUND3D_DS3D_DEFERRED);
        s_sound3d.lp3DListenerInfo->lpVtbl->SetDopplerFactor(
            s_sound3d.lp3DListenerInfo, 0.0f, SOUND3D_DS3D_DEFERRED);
        s_sound3d.lp3DListenerInfo->lpVtbl->CommitDeferredSettings(s_sound3d.lp3DListenerInfo);
        s_sound3d.minDistanceForAttenuation = 1.0f;
        s_sound3d.maxDistance = 800.0f;
        s_sound3d.lpDSBuff->lpVtbl->GetVolume(s_sound3d.lpDSBuff, &s_sound3d_frames.windowsVolume);
        audio_sound_volume_minus_300();
    }

    return 1;
}
#else
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
#endif

// FUNCTION: LEGORR 0x0047a800
#if defined(SOUND3D_DIRECTSOUND)
void audio_active_shutdown(void)
{
    if (s_bank_sound_on) {
        audio_sound_volume_set(s_sound3d_frames.windowsVolume);
        s_sound3d.lpDSBuff->lpVtbl->Stop(s_sound3d.lpDSBuff);
        s_sound3d.lpDSBuff->lpVtbl->Release(s_sound3d.lpDSBuff);
        s_sound3d.lpDSnd->lpVtbl->Release(s_sound3d.lpDSnd);
        s_sound3d.lpDSnd = NULL;
        s_bank_sound_on = 0;
    }
}
#else
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
#endif

// FUNCTION: LEGORR 0x0047a850
int sound_volume_value_valid(int millibels)
{
    if (millibels <= 0 && millibels >= -10000) return 1;
    return 0;
}

// FUNCTION: LEGORR 0x0047a870
#if defined(SOUND3D_DIRECTSOUND)
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
            if ((mfp = fopen(fullName, "r")) != NULL) {
                useName = fullName;
                fclose(mfp);
            } else if (resource_cd_file_path_get(cdFileName, fileName)) {
                useName = cdFileName;
            }
            if (useName != NULL && (mfp = fopen(useName, "r")) != NULL) {
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
#else
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
#endif

// FUNCTION: LEGORR 0x0047aad0
#if defined(SOUND3D_DIRECTSOUND)
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
#else
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
#endif

// FUNCTION: LEGORR 0x0047ab00
#if defined(SOUND3D_DIRECTSOUND)
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
                sound->lpDsb3D[i]->lpVtbl->Release(sound->lpDsb3D[i]);
                sound->lpDsb3D[i] = NULL;
            }
        }
        return 1;
    }
    return 0;
}
#else
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
#endif

/* DirectSoundListenerViewSet (FUN_0047ab80) — see simtick.hpp. */
// FUNCTION: LEGORR 0x0047ab80
#if defined(SOUND3D_DIRECTSOUND)
void simtick_directsound_listener_view_set(Sound3DFrame* frame)
{
    if (s_bank_sound_on) s_sound3d.listenerFrame = frame;
}
#else
void simtick_directsound_listener_view_set(int view)
{
    if (s_bank_sound_on != 0) {
        s_listener_view = view;
    }
}
#endif

// FUNCTION: LEGORR 0x0047aba0
#if defined(SOUND3D_DIRECTSOUND)
void simtick_sound_keepalive(struct EngineContext* c)
{
    if (s_bank_sound_on && s_sound3d.listenerFrame) {
        sound_listener_frame_update(s_sound3d.listenerFrame, NULL, 0.0f);
    }
}
#else
void simtick_sound_keepalive(struct EngineContext* c)
{
    (void)c;
    /* corpus: if (sound system on && an ambient entry exists) -> poke it. */
    if (s_sound_on != 0 && s_sound_active != 0 && s_keepalive != NULL) {
        s_keepalive(0, 0, 0);
    }
}
#endif

// FUNCTION: LEGORR 0x0047abc0
#if defined(SOUND3D_DIRECTSOUND)
void sound_listener_frame_update(Sound3DFrame* frame, void* arg, float delta)
{
    Sound3DFrame* scene;
    Sound3DVector vec;
    Sound3DVector up;
    int changed = 0;

    frame->lpVtbl->GetScene(frame, &scene);
    frame->lpVtbl->GetPosition(frame, scene, &vec);
    if (!vec_exact_equal(&vec.x, &s_sound3d_frames.oldPos.x)) {
        s_sound3d.lp3DListenerInfo->lpVtbl->SetPosition(
            s_sound3d.lp3DListenerInfo, vec.x, vec.y, vec.z, SOUND3D_DS3D_DEFERRED);
        s_sound3d_frames.oldPos = vec;
        changed = 1;
    }

    frame->lpVtbl->GetOrientation(frame, scene, &vec, &up);
    if (!vec_exact_equal(&vec.x, &s_sound3d_frames.oldOrientFront.x) ||
        !vec_exact_equal(&up.x, &s_sound3d_frames.oldOrientUp.x)) {
        s_sound3d.lp3DListenerInfo->lpVtbl->SetOrientation(
            s_sound3d.lp3DListenerInfo, vec.x, vec.y, vec.z, up.x, up.y, up.z,
            SOUND3D_DS3D_DEFERRED);
        s_sound3d_frames.oldOrientFront = vec;
        s_sound3d_frames.oldOrientUp = up;
        changed = 1;
    }

    if (changed) {
        s_sound3d.lp3DListenerInfo->lpVtbl->CommitDeferredSettings(s_sound3d.lp3DListenerInfo);
    }

    scene->lpVtbl->Release(scene);
}
#endif

// FUNCTION: LEGORR 0x0047ad10
#if defined(SOUND3D_DIRECTSOUND)
void sound_position_set_from_vector(Sound3DBuffer3D* sound3DBuff,
                                    const Sound3DVector* wPos,
                                    const Sound3DVector* vel)
{
    sound3DBuff->lpVtbl->SetPosition(sound3DBuff, wPos->x, wPos->y, wPos->z,
                                     SOUND3D_DS3D_IMMEDIATE);
}
#else
void sound_position_set_from_vector(void* buffer_object,
                                    const float position[3])
{
    if (buffer_object == NULL || position == NULL
        || s_sound_position_hook == NULL) return;
    s_sound_position_hook(s_sound_position_context, buffer_object, position,
                          0u);
}
#endif

// FUNCTION: LEGORR 0x0047ad30
#if defined(SOUND3D_DIRECTSOUND)
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
#else
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
#endif

/* Instance-layer play seam. The portable audio API can represent an
 * explicit source position as stereo pan for one-shot samples, while
 * object-attached sources are tracked through the existing spatial hooks.
 * Stream selection and loop-buffer ownership remain with the bank path. */
// FUNCTION: LEGORR 0x0047ad70
#if defined(SOUND3D_DIRECTSOUND)
int s_play_seam(int mode, int position, int slot, int loop,
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
                if ((mfp = fopen(fullName, "r")) != NULL) {
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

                soundBuff->lpVtbl->QueryInterface(soundBuff, &s_iid_directsound3d_buffer,
                                                  (void**)&sound3DBuff);

                volume = sound->volume;
                if (mode == 2) volume -= 800;
                soundBuff->lpVtbl->SetVolume(soundBuff, volume);

                sound3DBuff->lpVtbl->SetMinDistance(sound3DBuff,
                    s_sound3d.minDistanceForAttenuation, SOUND3D_DS3D_DEFERRED);
                sound3DBuff->lpVtbl->SetMaxDistance(sound3DBuff,
                    s_sound3d.maxDistance, SOUND3D_DS3D_DEFERRED);

                if (mode == 0) {
                    sound3DBuff->lpVtbl->SetMode(sound3DBuff, SOUND3D_DS3DMODE_NORMAL,
                                                 SOUND3D_DS3D_DEFERRED);
                    frame->lpVtbl->GetScene(frame, &scene);
                    sound_attached_stop_by_entity(frame, sound3DBuff);
                    sound_instance_track_register(frame, sound3DBuff);
                    sound_buffer_association_add(frame, soundBuff, sound3DBuff);
                    frame->lpVtbl->GetPosition(frame, scene, &pos);
                    sound3DBuff->lpVtbl->SetPosition(sound3DBuff, pos.x, pos.y, pos.z,
                                                     SOUND3D_DS3D_DEFERRED);
                    scene->lpVtbl->Release(scene);
                } else if (mode == 1) {
                    sound3DBuff->lpVtbl->SetMode(sound3DBuff, SOUND3D_DS3DMODE_NORMAL,
                                                 SOUND3D_DS3D_DEFERRED);
                    sound_position_set_from_vector(sound3DBuff, (const Sound3DVector*)xyz,
                                                   NULL);
                    sound_attached_stop_by_entity(NULL, sound3DBuff);
                    sound_buffer_association_add(NULL, soundBuff, sound3DBuff);
                } else if (mode == 2) {
                    sound3DBuff->lpVtbl->SetMode(sound3DBuff, SOUND3D_DS3DMODE_DISABLE,
                                                 SOUND3D_DS3D_DEFERRED);
                    sound_attached_stop_by_entity(NULL, sound3DBuff);
                    sound_buffer_association_add(NULL, soundBuff, sound3DBuff);
                }

                s_sound3d.lp3DListenerInfo->lpVtbl->CommitDeferredSettings(
                    s_sound3d.lp3DListenerInfo);
                soundBuff->lpVtbl->SetCurrentPosition(soundBuff, 0);
                if (loop) soundBuff->lpVtbl->Play(soundBuff, 0, 0, SOUND3D_DSBPLAY_LOOPING);
                else soundBuff->lpVtbl->Play(soundBuff, 0, 0, 0);

                return (slot * 3) + sound->voice;
            }
        }
    }

    return -1;
}
#else
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
#endif

// FUNCTION: LEGORR 0x0047b020
#if defined(SOUND3D_DIRECTSOUND)
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
#else
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
#endif

// FUNCTION: LEGORR 0x0047b050
#if defined(SOUND3D_DIRECTSOUND)
void sound_bank_entry_volume_set(int slot, int millibels)
{
    Sound3DSoundData* sound = &s_sound3d.soundTable[slot];
    int i;

    sound->volume = sound_volume_value_valid(millibels) ? millibels : 0;
    if (sound->flags & SOUND3D_FLAG_MULTI) {
        for (i = 0; i < 3; i++) {
            if (sound->lpDsb3D[i] != NULL) {
                sound->lpDsb3D[i]->lpVtbl->SetVolume(sound->lpDsb3D[i], sound->volume);
            }
        }
    } else if (sound->lpDsb3D[0] != NULL) {
        sound->lpDsb3D[0]->lpVtbl->SetVolume(sound->lpDsb3D[0], sound->volume);
    }
}
#else
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
#endif

// FUNCTION: LEGORR 0x0047b0d0
#if defined(SOUND3D_DIRECTSOUND)
int sound_bank_entry_volume(int slot)
{
    return s_sound3d.soundTable[slot].volume;
}
#else
int sound_bank_entry_volume(int slot)
{
    /* IndexedRecordStride134WordGet @ 0x47b0d0 (record +0x110 word). */
    return s_bank[slot].volume;
}
#endif

// FUNCTION: LEGORR 0x0047b0f0
#if defined(SOUND3D_DIRECTSOUND)
Sound3DBuffer* sound_handle_buffer_get(int handle)
{
    unsigned long voice;

    if (handle == -1) return NULL;
    voice = handle % 3;
    return s_sound3d.soundTable[(handle - voice) / 3].lpDsb3D[voice];
}
#else
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
#endif

// FUNCTION: LEGORR 0x0047b130
#if defined(SOUND3D_DIRECTSOUND)
void sound_buffer_stop_handle(int buffer_handle)
{
    Sound3DBuffer* soundBuff = sound_handle_buffer_get(buffer_handle);

    if (s_bank_sound_on && buffer_handle != -1 && soundBuff != NULL) {
        soundBuff->lpVtbl->Stop(soundBuff);
    }
}
#else
void sound_buffer_stop_handle(int buffer_handle)
{
    AudioBuffer* buffer = sound_handle_buffer_get(buffer_handle);
    /* The retail body resolves the handle first, then checks the bank gate,
     * the -1 sentinel, and the returned interface before calling Stop. */
    if (s_bank_sound_on != 0 && buffer_handle != -1 && buffer != NULL)
        audio_buffer_stop(buffer);
}
#endif

// FUNCTION: LEGORR 0x0047b160
#if defined(SOUND3D_DIRECTSOUND)
void sound_subsystem_stop_all(void)
{
    Sound3DSoundFrameRecord* item;

    for (item = s_sound3d.soundFrameRecord; item != NULL; item = item->next) {
        item->soundBuff->lpVtbl->Stop(item->soundBuff);
    }
    sound_stream_stop_and_release(0);
    sound_stream_stop_and_release(1);
}
#else
void sound_subsystem_stop_all(void)
{
    sound_buffer_association_stop_all();
    sound_instance_host_records_remove(NULL, NULL);
    sound_stream_stop_and_release(0);
    sound_stream_stop_and_release(1);
    audio_stop(audio_system_player_get());
}
#endif

// FUNCTION: LEGORR 0x0047b1a0
#if defined(SOUND3D_DIRECTSOUND)
void sound_instance_track_register(Sound3DFrame* frame,
                                   Sound3DBuffer3D* sound3DBuff)
{
    Sound3DFrameData* data = (Sound3DFrameData*)frame->lpVtbl->GetAppData(frame);
    Sound3DSoundRecord* record =
        (Sound3DSoundRecord*)malloc(sizeof(Sound3DSoundRecord));
    unsigned int i;

    if (data != NULL) record->next = data->soundRecord;
    else record->next = NULL;
    record->sound3DBuff = sound3DBuff;
    record->pos.x = 0.0f;
    record->pos.y = 0.0f;
    record->pos.z = 0.0f;
    ddiscene_animation_hook_create(frame, 0, NULL, NULL, 0, NULL, NULL, 0, 0,
                                   (const uint32_t*)record);

    for (i = 0; i < SOUND3D_MAXUPDATEFRAMES; i++) {
        if (s_sound3d_frames.updateFrameList[i] == NULL) {
            s_sound3d_frames.updateFrameList[i] = frame;
            return;
        }
    }
}
#else
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
#endif

// FUNCTION: LEGORR 0x0047b220
#if defined(SOUND3D_DIRECTSOUND)
void sound_instance_track_stop_and_remove(Sound3DFrame* frame,
                                          Sound3DBuffer3D* sound3DBuff)
{
    Sound3DFrameData* data;
    Sound3DSoundRecord* temp;
    unsigned int i;

    sound3DBuff->lpVtbl->Release(sound3DBuff);

    if (frame == NULL) {
        keyed_resource_entry_remove(frame, sound3DBuff);
        return;
    }

    data = (Sound3DFrameData*)frame->lpVtbl->GetAppData(frame);
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
#else
void sound_instance_track_stop_and_remove(void* provider, void* instance)
{
    int index;
    int adapter = 0;

    /* The first argument is the provider frame (the reference obtains it
     * from the caller's this register). The second is the sound instance.
     * Release the instance before unlinking its source record, as the retail
     * vtable call does. */
#if defined(__i386__) && (defined(__GNUC__) || defined(__clang__))
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
#endif

// FUNCTION: LEGORR 0x0047b2a0
#if defined(SOUND3D_DIRECTSOUND)
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
#else
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
#endif

/* NestedRecordListRemoveByKeyRecursive (FUN_0047b2e0) — see record.hpp.
 * The corpus re-reads the node's next link after the recursion — a freed
 * deeper node can leave a different successor in the slot — so the port
 * re-reads before the unlink. */
// FUNCTION: LEGORR 0x0047b2e0
#if defined(SOUND3D_DIRECTSOUND)
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
#else
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
#endif

// FUNCTION: LEGORR 0x0047b330
#if defined(SOUND3D_DIRECTSOUND)
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
#else
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
#endif

// FUNCTION: LEGORR 0x0047b390
#if defined(SOUND3D_DIRECTSOUND)
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
        item->soundBuff->lpVtbl->GetStatus(item->soundBuff, &status);
        if (!(status & SOUND3D_DSBSTATUS_PLAYING)) {
            sound_instance_track_stop_and_remove(item->frame, item->sound3DBuff);
        }
        item = next;
    }

    sound_stream_ring_buffer_tick(0);
    sound_stream_ring_buffer_tick(1);
}
#else
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
#endif

// FUNCTION: LEGORR 0x0047b410
#if defined(SOUND3D_DIRECTSOUND)
void sound_source_spatial_refresh(Sound3DFrame* frame, void* arg, float delta)
{
    Sound3DFrameData* data = (Sound3DFrameData*)frame->lpVtbl->GetAppData(frame);
    Sound3DSoundRecord* item = data->soundRecord;
    Sound3DFrame* scene;
    Sound3DVector pos;

    frame->lpVtbl->GetScene(frame, &scene);
    while (item != NULL) {
        frame->lpVtbl->GetPosition(frame, scene, &pos);
        if (!vec_exact_equal(&pos.x, &item->pos.x)) {
            item->sound3DBuff->lpVtbl->SetPosition(item->sound3DBuff, pos.x, pos.y, pos.z,
                                                   SOUND3D_DS3D_IMMEDIATE);
            item->pos = pos;
        }
        item = item->next;
    }
    scene->lpVtbl->Release(scene);
}
#else
void sound_source_spatial_refresh(void* sound_object)
{
#if defined(__i386__) && (defined(__GNUC__) || defined(__clang__))
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
#endif

// FUNCTION: LEGORR 0x0047b4a0
#if defined(SOUND3D_DIRECTSOUND)
void sound_tracked_buffers_set_min_distance(float dist)
{
    Sound3DSoundFrameRecord* item;

    s_sound3d.minDistanceForAttenuation = dist;
    for (item = s_sound3d.soundFrameRecord; item != NULL; item = item->next) {
        item->sound3DBuff->lpVtbl->SetMinDistance(item->sound3DBuff, dist,
                                                  SOUND3D_DS3D_IMMEDIATE);
    }
}
#else
void sound_tracked_buffers_set_min_distance(float value)
{
    uint32_t distance;
    void** record = (void**)s_buffer_assoc_head;
    memcpy(&distance, &value, sizeof(distance));
    s_tracked_buffer_min_distance = distance;
    while (record != NULL) {
        ((SoundTrackedBufferInterface*)record[2])->min_distance_set(distance, 0);
        record = (void**)record[3];
    }
}
#endif

// FUNCTION: LEGORR 0x0047b4d0
#if defined(SOUND3D_DIRECTSOUND)
void sound_tracked_buffers_set_max_distance(float dist)
{
    Sound3DSoundFrameRecord* item;

    s_sound3d.maxDistance = dist;
    for (item = s_sound3d.soundFrameRecord; item != NULL; item = item->next) {
        item->sound3DBuff->lpVtbl->SetMaxDistance(item->sound3DBuff, dist,
                                                  SOUND3D_DS3D_IMMEDIATE);
    }
}
#else
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
#endif

/* SoundBufferVolumeSet (FUN_0047b500): DirectSound's attenuation maps to
 * the port's single master stream gain. Invalid corpus attenuation values
 * are ignored by the same range predicate used by sound-bank records. */
// FUNCTION: LEGORR 0x0047b500
#if defined(SOUND3D_DIRECTSOUND)
void audio_sound_volume_set(int millibels)
{
    if (s_bank_sound_on) {
        if (sound_volume_value_valid(millibels)) {
            s_sound3d.volume = millibels;
            s_sound3d.lpDSBuff->lpVtbl->SetVolume(s_sound3d.lpDSBuff, millibels);
        }
    }
}
#else
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
#endif

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
#if defined(SOUND3D_DIRECTSOUND)
void audio_sound_volume_minus_300(void)
{
    if (s_bank_sound_on) {
        s_sound3d.volume = -300;
        s_sound3d.lpDSBuff->lpVtbl->SetVolume(s_sound3d.lpDSBuff, -300);
    }
}
#else
void audio_sound_volume_minus_300(void)
{
    if (s_bank_sound_on != 0)
        audio_sound_volume_set(-300);
}
#endif

// FUNCTION: LEGORR 0x0047b580
#if defined(SOUND3D_DIRECTSOUND)
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
            mmioClose(hmmio, 0);
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
#endif

// FUNCTION: LEGORR 0x0047b6c0
#if defined(SOUND3D_DIRECTSOUND)
int sound_record_create_buffers(Sound3DSoundData* sound, int simultaneous)
{
    Sound3DBufferDesc dsbd;
    unsigned long i;

    memset(&dsbd, 0, sizeof(dsbd));
    dsbd.dwSize = sizeof(dsbd);
    dsbd.dwFlags = SOUND3D_SAMPLE_CAPS;
    dsbd.dwBufferBytes = sound->size;
    dsbd.lpwfxFormat = sound->pwf;

    if (s_sound3d.lpDSnd->lpVtbl->CreateSoundBuffer(s_sound3d.lpDSnd, &dsbd,
                                                    &sound->lpDsb3D[0], NULL) != SOUND3D_DS_OK) {
        return 0;
    }

    sound->voice = 0;
    if (simultaneous) {
        for (i = 1; i < 3; i++) {
            if (s_sound3d.lpDSnd->lpVtbl->DuplicateSoundBuffer(
                    s_sound3d.lpDSnd, sound->lpDsb3D[0], &sound->lpDsb3D[i]) != SOUND3D_DS_OK) {
                return 0;
            }
        }
    }

    return 1;
}
#else
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
#endif

// FUNCTION: LEGORR 0x0047b790
#if defined(SOUND3D_DIRECTSOUND)
int sound_wave_sample_duplicate_buffer_create(Sound3DSoundData* sound)
{
    void* lpvAudio1;
    void* lpvAudio2;
    DWORD dwBytes1 = 0;
    DWORD dwBytes2 = 0;
    long hr;

    hr = sound->lpDsb3D[0]->lpVtbl->Lock(sound->lpDsb3D[0], sound->offset, sound->size,
                                         &lpvAudio1, &dwBytes1, &lpvAudio2, &dwBytes2, 0);
    if (hr == SOUND3D_DSERR_BUFFERLOST) {
        sound->lpDsb3D[0]->lpVtbl->Restore(sound->lpDsb3D[0]);
        sound->lpDsb3D[0]->lpVtbl->Lock(sound->lpDsb3D[0], sound->offset, sound->size,
                                        &lpvAudio1, &dwBytes1, &lpvAudio2, &dwBytes2, 0);
    } else if (hr != SOUND3D_DS_OK) {
        return 0;
    }

    memcpy(lpvAudio1, sound->pbData, dwBytes1);
    if (lpvAudio2 != NULL) {
        memcpy(lpvAudio2, sound->pbData + dwBytes1, dwBytes2);
    }

    if (sound->lpDsb3D[0]->lpVtbl->Unlock(sound->lpDsb3D[0], lpvAudio1, dwBytes1,
                                          lpvAudio2, dwBytes2) != SOUND3D_DS_OK) {
        return 0;
    }
    if (sound->lpDsb3D[0]->lpVtbl->GetFrequency(sound->lpDsb3D[0], &sound->freq) != SOUND3D_DS_OK) {
        return 0;
    }
    return sound->lpDsb3D[0]->lpVtbl->GetVolume(sound->lpDsb3D[0], &sound->volume) == SOUND3D_DS_OK;
}
#else
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
#endif

// FUNCTION: LEGORR 0x0047b8e0
#if defined(SOUND3D_DIRECTSOUND)
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
#else
float sound_streamed_wave_buffer_step_rate(int slot)
{
    /* StreamedWaveBufferStepRate @ 0x47b8e0. slot -1 is the variant
     * select's "no sound" result: zero, before any table read. */
    if (slot == -1) {
        return 0.0f;
    }
    if (slot < 0 || slot >= SOUND_BANK_SLOTS) {
        return 0.0f; /* deviation: the corpus reads past its record table */
    }
    const BankSlot* rec = &s_bank[slot];
    const Sound* snd = rec->sound;
    uint32_t bytes = (snd != NULL) ? snd->file_len : 0u;
    double rate = (snd != NULL) ? (double)snd->format.byte_rate : 0.0;
    return (float)((double)bytes / rate);
}
#endif

// FUNCTION: LEGORR 0x0047b970
#if defined(SOUND3D_DIRECTSOUND)
int sound_streamed_wave_play(const char* fName, int loop, int volume)
{
    Sound3DStreamData* stream = loop ? &s_sound3d.loopStreamData : &s_sound3d.streamData;

    if (stream->playing || stream->fileOpen) {
        sound_stream_stop_and_release(loop);
    }

    if (sound_streamed_wave_buffer_setup(fName, loop, volume)) {
        SOUND3D_STREAM_BUFFER(loop)->lpVtbl->SetCurrentPosition(SOUND3D_STREAM_BUFFER(loop), 0);
        SOUND3D_STREAM_BUFFER(loop)->lpVtbl->Play(SOUND3D_STREAM_BUFFER(loop), 0, 0,
                                                  SOUND3D_DSBPLAY_LOOPING);
        stream->playing = 1;
        return 1;
    }

    stream->playing = 0;
    return 0;
}
#endif

// FUNCTION: LEGORR 0x0047ba20
#if defined(SOUND3D_DIRECTSOUND)
int sound_stream_stop_and_release(int stream_index)
{
    Sound3DStreamData* stream =
        stream_index ? &s_sound3d.loopStreamData : &s_sound3d.streamData;

    stream->playing = 0;
    if (stream->fileOpen) {
        SOUND3D_STREAM_BUFFER(stream_index)->lpVtbl->Stop(SOUND3D_STREAM_BUFFER(stream_index));
        sound_wave_handle_and_buffer_release(&stream->hmmio, &stream->pwf);
        SOUND3D_STREAM_BUFFER(stream_index)->lpVtbl->Release(SOUND3D_STREAM_BUFFER(stream_index));
        stream->fileOpen = 0;
    }
    return 1;
}
#else
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
#endif

// FUNCTION: LEGORR 0x0047baa0
#if defined(SOUND3D_DIRECTSOUND)
int sound_streamed_wave_buffer_setup(const char* fName, int loop, int volume)
{
    Sound3DStreamData* stream = loop ? &s_sound3d.loopStreamData : &s_sound3d.streamData;
    Sound3DBufferDesc dsbd;
    Sound3DBuffer* soundBuff;
    unsigned long remainder;
    int nError;

    if (sound_wave_file_open_read_chunks(fName, &stream->hmmio, &stream->pwf,
                                         &stream->mmckinfoParent) != 0) {
        return 0;
    }

    if (stream->pwf->wFormatTag != 1) {
        sound_wave_handle_and_buffer_release(&stream->hmmio, &stream->pwf);
        return 0;
    }

    if ((nError = sound_wave_data_chunk_open(&stream->hmmio, &stream->mmckinfoData,
                                             &stream->mmckinfoParent)) != 0) {
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

    if (s_sound3d.lpDSnd->lpVtbl->CreateSoundBuffer(s_sound3d.lpDSnd, &dsbd, &soundBuff,
                                                    NULL) != SOUND3D_DS_OK) {
        return 0;
    }

    if (loop) s_sound3d.lpDSLoopStreamBuff = soundBuff;
    else s_sound3d.lpDSStreamBuff = soundBuff;

    SOUND3D_STREAM_BUFFER(loop)->lpVtbl->SetVolume(SOUND3D_STREAM_BUFFER(loop), volume);

    stream->fileEnd = 0;
    stream->writeOffset = 0;
    stream->writeProgress = stream->notifySize * 2;
    stream->looping = loop;
    sound_stream_drain_check(loop);
    stream->playEnd = 0;
    stream->fileOpen = 1;

    return 1;
}
#endif

// FUNCTION: LEGORR 0x0047bc30
#if defined(SOUND3D_DIRECTSOUND)
int sound_stream_drain_check(int loop)
{
    Sound3DStreamData* stream = loop ? &s_sound3d.loopStreamData : &s_sound3d.streamData;
    void* lpvAudio1;
    DWORD dwBytes1;
    void* lpvAudio2;
    DWORD dwBytes2;
    UINT cbActualRead;
    UINT pos;

    if (SOUND3D_STREAM_BUFFER(loop)->lpVtbl->Lock(SOUND3D_STREAM_BUFFER(loop), 0,
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

    SOUND3D_STREAM_BUFFER(loop)->lpVtbl->Unlock(SOUND3D_STREAM_BUFFER(loop), lpvAudio1,
                                                dwBytes1, NULL, 0);

    stream->writeOffset += dwBytes1;
    if (stream->writeOffset >= stream->bufferSize) stream->writeOffset -= stream->bufferSize;
    stream->playProgress = 0;
    stream->lastPos = 0;
#if !defined(_MSC_VER)
    return 1;
#endif
}
#else
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
#endif

// FUNCTION: LEGORR 0x0047bdb0
#if defined(SOUND3D_DIRECTSOUND)
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
        SOUND3D_STREAM_BUFFER(loop)->lpVtbl->GetCurrentPosition(SOUND3D_STREAM_BUFFER(loop),
                                                                &playPos, &writePos);
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
                if (SOUND3D_STREAM_BUFFER(loop)->lpVtbl->Lock(SOUND3D_STREAM_BUFFER(loop),
                        stream->writeOffset, stream->notifySize, &lpvAudio1, &dwBytes1,
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
                SOUND3D_STREAM_BUFFER(loop)->lpVtbl->Unlock(SOUND3D_STREAM_BUFFER(loop),
                                                            lpvAudio1, dwBytes1, NULL, 0);
                stream->writeOffset += dwBytes1;
                stream->writeProgress += dwBytes1;
                if (stream->writeOffset >= stream->bufferSize) {
                    stream->writeOffset -= stream->bufferSize;
                }
            } else {
                SOUND3D_STREAM_BUFFER(loop)->lpVtbl->Lock(SOUND3D_STREAM_BUFFER(loop),
                    stream->writeOffset, stream->notifySize, &lpvAudio1, &dwBytes1,
                    NULL, NULL, 0);
                memset(lpvAudio1, (BYTE)((stream->pwf->wBitsPerSample == 8) ? 128 : 0), dwBytes1);
                SOUND3D_STREAM_BUFFER(loop)->lpVtbl->Unlock(SOUND3D_STREAM_BUFFER(loop),
                                                            lpvAudio1, dwBytes1, NULL, 0);
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
#else
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
#endif

// FUNCTION: LEGORR 0x0047c0c0
int vec_exact_equal(const float* a, const float* b) /* FUN_0047c0c0 */
{
    return a[0] == b[0] && a[1] == b[1] && a[2] == b[2];
}

// FUNCTION: LEGORR 0x0047c100
#if defined(SOUND3D_DIRECTSOUND)
float ledger_stub_0047c100_DirectSoundListenerRolloffSet(float rolloff)
{
    if (s_bank_sound_on) {
        if (rolloff < 0.0f) rolloff = 0.0f;
        if (rolloff > 10.0f) rolloff = 10.0f;
        s_sound3d.lp3DListenerInfo->lpVtbl->SetRolloffFactor(
            s_sound3d.lp3DListenerInfo, rolloff, SOUND3D_DS3D_IMMEDIATE);
    }
    return rolloff;
}
#else
#ifdef __cplusplus
extern "C" /* GAME-021's lego_unit.c calls it (issue #544) */
#endif
float ledger_stub_0047c100_DirectSoundListenerRolloffSet(float rolloff)
{
    if (s_bank_sound_on == 0) return rolloff;
    if (rolloff < 0.0f) rolloff = 0.0f;
    if (rolloff > 10.0f) rolloff = 10.0f;
    return audio_listener_rolloff_set(rolloff);
}
#endif

// FUNCTION: LEGORR 0x0047c160
int32_t constant_negative_10000_get(void)
{
    return -10000;
}

#if !defined(SOUND3D_DIRECTSOUND)
/* The reference scene interfaces are 32-bit COM-style objects. This getter
 * reads the record that DDiSceneAnimationHookCreate owns at vtable +0x1c. */
static void* sound_instance_property_get(void* object)
{
#if defined(__i386__) && (defined(__GNUC__) || defined(__clang__))
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
#endif /* !SOUND3D_DIRECTSOUND */

