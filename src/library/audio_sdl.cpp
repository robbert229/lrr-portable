// SPDX-License-Identifier: LGPL-3.0-or-later
/* The audio library (ADR 0007, [library:audio]): SDL3 audio, and miniaudio
 * for the CD tracks' files. See audio_sdl.h for the interface. */
#include "audio_sdl.h"

#include <SDL3/SDL.h>

#include "miniaudio.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <set>
#include <string>
#include <vector>

/* --- DirectSound ---------------------------------------------------------- */

struct AudioListener {
    AudioDevice* device;
    float pos[3];
    float rolloff;
};

struct AudioDevice {
    SDL_AudioDeviceID id;
    LONG volume;                 /* the primary buffer's */
    AudioListener listener;
    std::set<AudioBuffer*> buffers3D;
};

struct AudioBuffer {
    AudioDevice* device;
    int refs;
    bool primary;
    std::shared_ptr<std::vector<Uint8>> data;
    DWORD size;
    DWORD blockAlign;
    SDL_AudioSpec spec;
    SDL_AudioStream* stream;
    bool bound;
    /* Under the stream's lock: the mix reads them. */
    DWORD cursor;   /* the next byte the stream is fed */
    bool playing;
    bool looping;
    LONG volume;
    /* IDirectSound3DBuffer. */
    bool is3D;
    DWORD mode3D;
    float pos[3];
    float minDistance;
    float maxDistance;
};

namespace {

/* DirectSound's attenuation, in hundredths of a decibel, as a gain. */
float GainOfVolume(LONG volume)
{
    if (volume <= -10000) return 0.0f;
    if (volume >= 0) return 1.0f;
    return powf(10.0f, (float)volume / 2000.0f);
}

/* DirectSound's default rolloff: full gain inside the minimum distance,
 * minimum / distance (to the rolloff factor's power) beyond it, and silence
 * at the maximum distance (DSBCAPS_MUTE3DATMAXDISTANCE). */
float GainOf3D(const AudioBuffer* buffer)
{
    if (!buffer->is3D || buffer->mode3D == AUDIO_3D_MODE_DISABLE) return 1.0f;
    const AudioListener& listener = buffer->device->listener;
    float dx = buffer->pos[0] - listener.pos[0];
    float dy = buffer->pos[1] - listener.pos[1];
    float dz = buffer->pos[2] - listener.pos[2];
    float distance = sqrtf(dx * dx + dy * dy + dz * dz);
    if (distance >= buffer->maxDistance) return 0.0f;
    if (distance <= buffer->minDistance || buffer->minDistance <= 0.0f) return 1.0f;
    return powf(buffer->minDistance / distance, listener.rolloff);
}

void ApplyGain(AudioBuffer* buffer)
{
    if (buffer->stream) {
        SDL_SetAudioStreamGain(buffer->stream,
                               GainOfVolume(buffer->volume) * GainOf3D(buffer));
    }
}

/* The stream's get callback, on the audio thread with the stream locked:
 * feed it from the cursor, wrapping a looping buffer and stopping at the
 * end of a one-shot. */
void SDLCALL FeedBuffer(void* userdata, SDL_AudioStream* stream,
                        int additional, int total)
{
    (void)total;
    AudioBuffer* buffer = static_cast<AudioBuffer*>(userdata);
    if (buffer->size == 0) {
        buffer->playing = false;
        return;
    }
    while (additional > 0 && buffer->playing) {
        DWORD left = buffer->size - buffer->cursor;
        if (left == 0) {
            if (buffer->looping) {
                buffer->cursor = 0;
                continue;
            }
            buffer->playing = false;
            /* Let the resampler's held-back tail out, so the queue drains
             * and GetStatus sees the end. */
            SDL_FlushAudioStream(stream);
            break;
        }
        DWORD bytes = std::min(left, (DWORD)additional);
        /* Whole frames only; the buffer's size is a whole number of them. */
        bytes = ((bytes + buffer->blockAlign - 1) / buffer->blockAlign) * buffer->blockAlign;
        if (bytes > left) bytes = left;
        SDL_PutAudioStreamData(stream, buffer->data->data() + buffer->cursor, (int)bytes);
        buffer->cursor += bytes;
        additional -= (int)bytes;
    }
}

bool SpecOfFormat(const WAVEFORMATEX* format, SDL_AudioSpec* spec)
{
    if (!format || format->wFormatTag != WAVE_FORMAT_PCM) return false;
    if (format->nChannels < 1 || format->nChannels > 8) return false;
    if (format->nSamplesPerSec == 0) return false;
    if (format->wBitsPerSample == 8) spec->format = SDL_AUDIO_U8;
    else if (format->wBitsPerSample == 16) spec->format = SDL_AUDIO_S16LE;
    else return false;
    spec->channels = format->nChannels;
    spec->freq = (int)format->nSamplesPerSec;
    return true;
}

AudioBuffer* NewBuffer(AudioDevice* device)
{
    AudioBuffer* buffer = new AudioBuffer();
    buffer->device = device;
    buffer->refs = 1;
    buffer->blockAlign = 1;
    buffer->minDistance = 1.0f;     /* DS3D_DEFAULTMINDISTANCE */
    buffer->maxDistance = 1.0e9f;   /* DS3D_DEFAULTMAXDISTANCE */
    return buffer;
}

bool OpenStream(AudioBuffer* buffer)
{
    buffer->stream = SDL_CreateAudioStream(&buffer->spec, &buffer->spec);
    if (!buffer->stream) return false;
    SDL_SetAudioStreamGetCallback(buffer->stream, FeedBuffer, buffer);
    ApplyGain(buffer);
    return true;
}

} // namespace

extern "C" {

long audio_create(AudioDevice** device)
{
    *device = NULL;
    if (!SDL_WasInit(SDL_INIT_AUDIO) && !SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        SDL_LogWarn(SDL_LOG_CATEGORY_AUDIO, "audio: %s", SDL_GetError());
        return AUDIO_ERR_NODRIVER;
    }
    SDL_AudioDeviceID id = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, NULL);
    if (id == 0) {
        SDL_LogWarn(SDL_LOG_CATEGORY_AUDIO, "audio: %s", SDL_GetError());
        return AUDIO_ERR_NODRIVER;
    }
    AudioDevice* created = new AudioDevice();
    created->id = id;
    created->volume = 0;
    created->listener.device = created;
    created->listener.rolloff = 1.0f;
    *device = created;
    return AUDIO_OK;
}

unsigned long audio_release(AudioDevice* device)
{
    if (device) {
        /* Closing the device unbinds every stream; buffers the unit still
         * holds stay valid and silent. */
        SDL_CloseAudioDevice(device->id);
        device->id = 0;
    }
    return 0;
}

long audio_set_cooperative_level(AudioDevice* device, void* window, DWORD level)
{
    (void)device;
    (void)window;
    (void)level;
    return AUDIO_OK;
}

long audio_create_buffer(AudioDevice* device, DWORD flags, DWORD bytes,
                         const WAVEFORMATEX* format, AudioBuffer** buffer)
{
    *buffer = NULL;
    if (!device) return AUDIO_ERR_INVALIDPARAM;
    if (flags & AUDIO_BUFFER_PRIMARY) {
        AudioBuffer* primary = NewBuffer(device);
        primary->primary = true;
        *buffer = primary;
        return AUDIO_OK;
    }
    SDL_AudioSpec spec;
    if (!SpecOfFormat(format, &spec)) return AUDIO_ERR_BADFORMAT;
    if (bytes == 0) return AUDIO_ERR_INVALIDPARAM;

    AudioBuffer* created = NewBuffer(device);
    created->spec = spec;
    created->blockAlign = (DWORD)SDL_AUDIO_FRAMESIZE(spec);
    created->size = bytes - bytes % created->blockAlign;
    created->data = std::make_shared<std::vector<Uint8>>(bytes, Uint8(spec.format == SDL_AUDIO_U8 ? 0x80 : 0));
    if (!OpenStream(created)) {
        delete created;
        return AUDIO_ERR_OUTOFMEMORY;
    }
    *buffer = created;
    return AUDIO_OK;
}

long audio_duplicate_buffer(AudioDevice* device, AudioBuffer* original,
                            AudioBuffer** duplicate)
{
    *duplicate = NULL;
    if (!device || !original || original->primary) return AUDIO_ERR_INVALIDPARAM;
    AudioBuffer* created = NewBuffer(device);
    created->spec = original->spec;
    created->blockAlign = original->blockAlign;
    created->size = original->size;
    created->data = original->data;
    created->volume = original->volume;
    if (!OpenStream(created)) {
        delete created;
        return AUDIO_ERR_OUTOFMEMORY;
    }
    *duplicate = created;
    return AUDIO_OK;
}

long audio_buffer_query_listener(AudioBuffer* buffer, AudioListener** listener)
{
    *listener = NULL;
    if (!buffer || !buffer->primary) return AUDIO_ERR_INVALIDPARAM;
    *listener = &buffer->device->listener;
    return AUDIO_OK;
}

long audio_buffer_query_3d(AudioBuffer* buffer, AudioBuffer** buffer3D)
{
    *buffer3D = NULL;
    if (!buffer || buffer->primary) return AUDIO_ERR_INVALIDPARAM;
    if (!buffer->is3D) {
        buffer->is3D = true;
        buffer->device->buffers3D.insert(buffer);
    }
    buffer->refs++;
    *buffer3D = buffer;
    return AUDIO_OK;
}

unsigned long audio_buffer_release(AudioBuffer* buffer)
{
    if (!buffer) return 0;
    if (--buffer->refs > 0) return (unsigned long)buffer->refs;
    if (buffer->is3D) buffer->device->buffers3D.erase(buffer);
    if (buffer->stream) SDL_DestroyAudioStream(buffer->stream);
    delete buffer;
    return 0;
}

long audio_buffer_set_format(AudioBuffer* buffer, const WAVEFORMATEX* format)
{
    /* Only the primary buffer's format is set: SDL mixes in the device's
     * own format. */
    (void)format;
    return buffer && buffer->primary ? AUDIO_OK : AUDIO_ERR_INVALIDPARAM;
}

long audio_buffer_play(AudioBuffer* buffer, DWORD reserved1, DWORD priority,
                       DWORD flags)
{
    (void)reserved1;
    (void)priority;
    if (!buffer) return AUDIO_ERR_INVALIDPARAM;
    if (buffer->primary) return AUDIO_OK;
    SDL_LockAudioStream(buffer->stream);
    buffer->playing = true;
    buffer->looping = (flags & AUDIO_PLAY_LOOPING) != 0;
    SDL_UnlockAudioStream(buffer->stream);
    if (!buffer->bound && buffer->device->id != 0) {
        buffer->bound = SDL_BindAudioStream(buffer->device->id, buffer->stream);
    }
    return AUDIO_OK;
}

long audio_buffer_stop(AudioBuffer* buffer)
{
    if (!buffer) return AUDIO_ERR_INVALIDPARAM;
    if (buffer->primary) return AUDIO_OK;
    SDL_LockAudioStream(buffer->stream);
    buffer->playing = false;
    SDL_ClearAudioStream(buffer->stream);
    SDL_UnlockAudioStream(buffer->stream);
    if (buffer->bound) {
        SDL_UnbindAudioStream(buffer->stream);
        buffer->bound = false;
    }
    return AUDIO_OK;
}

long audio_buffer_set_current_position(AudioBuffer* buffer, DWORD position)
{
    if (!buffer || buffer->primary) return AUDIO_ERR_INVALIDPARAM;
    SDL_LockAudioStream(buffer->stream);
    buffer->cursor = std::min(position - position % buffer->blockAlign, buffer->size);
    SDL_ClearAudioStream(buffer->stream);
    SDL_UnlockAudioStream(buffer->stream);
    return AUDIO_OK;
}

long audio_buffer_get_current_position(AudioBuffer* buffer, DWORD* play,
                                       DWORD* write)
{
    if (!buffer || buffer->primary || buffer->size == 0) return AUDIO_ERR_INVALIDPARAM;
    SDL_LockAudioStream(buffer->stream);
    /* The play cursor trails the fed cursor by what the stream still
     * holds, in the buffer's own format. */
    int queued = SDL_GetAudioStreamQueued(buffer->stream);
    DWORD cursor = buffer->cursor;
    SDL_UnlockAudioStream(buffer->stream);
    DWORD behind = queued > 0 ? (DWORD)queued % buffer->size : 0;
    DWORD position = (cursor + buffer->size - behind) % buffer->size;
    if (play) *play = position;
    if (write) *write = cursor % buffer->size;
    return AUDIO_OK;
}

long audio_buffer_get_status(AudioBuffer* buffer, DWORD* status)
{
    if (!buffer) return AUDIO_ERR_INVALIDPARAM;
    *status = 0;
    if (buffer->primary) {
        *status = AUDIO_STATUS_PLAYING | AUDIO_STATUS_LOOPING;
        return AUDIO_OK;
    }
    SDL_LockAudioStream(buffer->stream);
    if (buffer->playing) {
        *status = AUDIO_STATUS_PLAYING | (buffer->looping ? AUDIO_STATUS_LOOPING : 0);
    } else if (buffer->bound && SDL_GetAudioStreamQueued(buffer->stream) > 0) {
        /* Fed to the end, but the end is still playing. */
        *status = AUDIO_STATUS_PLAYING;
    }
    SDL_UnlockAudioStream(buffer->stream);
    return AUDIO_OK;
}

long audio_buffer_set_volume(AudioBuffer* buffer, LONG volume)
{
    if (!buffer) return AUDIO_ERR_INVALIDPARAM;
    if (volume > 0 || volume < -10000) return AUDIO_ERR_INVALIDPARAM;
    if (buffer->primary) {
        buffer->device->volume = volume;
        if (buffer->device->id != 0) {
            SDL_SetAudioDeviceGain(buffer->device->id, GainOfVolume(volume));
        }
        return AUDIO_OK;
    }
    buffer->volume = volume;
    ApplyGain(buffer);
    return AUDIO_OK;
}

long audio_buffer_get_volume(AudioBuffer* buffer, LONG* volume)
{
    if (!buffer) return AUDIO_ERR_INVALIDPARAM;
    *volume = buffer->primary ? buffer->device->volume : buffer->volume;
    return AUDIO_OK;
}

long audio_buffer_get_frequency(AudioBuffer* buffer, DWORD* frequency)
{
    if (!buffer || buffer->primary) return AUDIO_ERR_INVALIDPARAM;
    *frequency = (DWORD)buffer->spec.freq;
    return AUDIO_OK;
}

long audio_buffer_lock(AudioBuffer* buffer, DWORD offset, DWORD bytes,
                       void** audio1, DWORD* bytes1, void** audio2,
                       DWORD* bytes2, DWORD flags)
{
    (void)flags;
    if (!buffer || buffer->primary || offset >= buffer->size || bytes > buffer->size) {
        return AUDIO_ERR_INVALIDPARAM;
    }
    SDL_LockAudioStream(buffer->stream);
    Uint8* data = buffer->data->data();
    DWORD first = std::min(bytes, buffer->size - offset);
    *audio1 = data + offset;
    *bytes1 = first;
    if (audio2) *audio2 = first < bytes ? data : NULL;
    if (bytes2) *bytes2 = bytes - first;
    return AUDIO_OK;
}

long audio_buffer_unlock(AudioBuffer* buffer, void* audio1, DWORD bytes1,
                         void* audio2, DWORD bytes2)
{
    (void)audio1;
    (void)bytes1;
    (void)audio2;
    (void)bytes2;
    if (!buffer || buffer->primary) return AUDIO_ERR_INVALIDPARAM;
    SDL_UnlockAudioStream(buffer->stream);
    return AUDIO_OK;
}

long audio_buffer_restore(AudioBuffer* buffer)
{
    (void)buffer;
    return AUDIO_OK;
}

long audio_buffer_set_3d_mode(AudioBuffer* buffer, DWORD mode, DWORD apply)
{
    (void)apply;
    if (!buffer || !buffer->is3D) return AUDIO_ERR_INVALIDPARAM;
    buffer->mode3D = mode;
    ApplyGain(buffer);
    return AUDIO_OK;
}

long audio_buffer_set_3d_position(AudioBuffer* buffer, float x, float y,
                                  float z, DWORD apply)
{
    (void)apply;
    if (!buffer || !buffer->is3D) return AUDIO_ERR_INVALIDPARAM;
    buffer->pos[0] = x;
    buffer->pos[1] = y;
    buffer->pos[2] = z;
    ApplyGain(buffer);
    return AUDIO_OK;
}

long audio_buffer_set_min_distance(AudioBuffer* buffer, float distance,
                                   DWORD apply)
{
    (void)apply;
    if (!buffer || !buffer->is3D) return AUDIO_ERR_INVALIDPARAM;
    buffer->minDistance = distance;
    ApplyGain(buffer);
    return AUDIO_OK;
}

long audio_buffer_set_max_distance(AudioBuffer* buffer, float distance,
                                   DWORD apply)
{
    (void)apply;
    if (!buffer || !buffer->is3D) return AUDIO_ERR_INVALIDPARAM;
    buffer->maxDistance = distance;
    ApplyGain(buffer);
    return AUDIO_OK;
}

long audio_listener_set_position(AudioListener* listener, float x, float y,
                                 float z, DWORD apply)
{
    (void)apply;
    if (!listener) return AUDIO_ERR_INVALIDPARAM;
    listener->pos[0] = x;
    listener->pos[1] = y;
    listener->pos[2] = z;
    return AUDIO_OK;
}

long audio_listener_set_orientation(AudioListener* listener, float frontX,
                                    float frontY, float frontZ, float topX,
                                    float topY, float topZ, DWORD apply)
{
    /* Orientation only pans, which the library does not do. */
    (void)frontX; (void)frontY; (void)frontZ;
    (void)topX; (void)topY; (void)topZ; (void)apply;
    return listener ? AUDIO_OK : AUDIO_ERR_INVALIDPARAM;
}

long audio_listener_set_doppler_factor(AudioListener* listener, float factor,
                                       DWORD apply)
{
    (void)factor;
    (void)apply;
    return listener ? AUDIO_OK : AUDIO_ERR_INVALIDPARAM;
}

long audio_listener_set_rolloff_factor(AudioListener* listener, float factor,
                                       DWORD apply)
{
    (void)apply;
    if (!listener) return AUDIO_ERR_INVALIDPARAM;
    listener->rolloff = factor;
    return AUDIO_OK;
}

long audio_listener_commit_deferred_settings(AudioListener* listener)
{
    if (!listener) return AUDIO_ERR_INVALIDPARAM;
    /* The listener moved: re-attenuate every 3D buffer. */
    for (AudioBuffer* buffer : listener->device->buffers3D) ApplyGain(buffer);
    return AUDIO_OK;
}

} /* extern "C" */

/* --- mmio ----------------------------------------------------------------- */

struct HMMIO__ {
    std::vector<char> owned;
    char* data;
    LONG size;
    LONG pos;
};

namespace {

DWORD ReadLe32(const char* bytes)
{
    const unsigned char* b = reinterpret_cast<const unsigned char*>(bytes);
    return (DWORD)b[0] | ((DWORD)b[1] << 8) | ((DWORD)b[2] << 16) | ((DWORD)b[3] << 24);
}

/* The chunk header at the position, which moves past it (and past a RIFF
 * or LIST form type). */
bool ReadChunkHeader(HMMIO hmmio, LONG end, MMCKINFO* chunk)
{
    if (hmmio->pos < 0 || (long long)hmmio->pos + 8 > end) return false;
    chunk->ckid = ReadLe32(hmmio->data + hmmio->pos);
    chunk->cksize = ReadLe32(hmmio->data + hmmio->pos + 4);
    chunk->dwDataOffset = (DWORD)hmmio->pos + 8;
    chunk->dwFlags = 0;
    chunk->fccType = 0;
    hmmio->pos += 8;
    if (chunk->ckid == FOURCC_RIFF || chunk->ckid == FOURCC_LIST) {
        if ((long long)hmmio->pos + 4 > end) return false;
        chunk->fccType = ReadLe32(hmmio->data + hmmio->pos);
        hmmio->pos += 4;
    }
    return true;
}

} // namespace

extern "C" {

HMMIO audio_mmio_open(const char* path, LPMMIOINFO info, DWORD flags)
{
    (void)flags;
    HMMIO hmmio = new HMMIO__();
    if (info && info->fccIOProc == FOURCC_MEM) {
        hmmio->data = info->pchBuffer;
        hmmio->size = info->cchBuffer;
    } else {
        size_t length = 0;
        void* loaded = path ? SDL_LoadFile(path, &length) : NULL;
        if (!loaded || length > 0x7fffffff) {
            SDL_free(loaded);
            delete hmmio;
            if (info) info->wErrorRet = MMIOERR_CANNOTOPEN;
            return NULL;
        }
        hmmio->owned.assign(static_cast<char*>(loaded), static_cast<char*>(loaded) + length);
        SDL_free(loaded);
        hmmio->data = hmmio->owned.data();
        hmmio->size = (LONG)length;
    }
    if (!hmmio->data || hmmio->size < 0) {
        delete hmmio;
        if (info) info->wErrorRet = MMIOERR_CANNOTOPEN;
        return NULL;
    }
    return hmmio;
}

MMRESULT audio_mmio_close(HMMIO hmmio, UINT flags)
{
    (void)flags;
    if (!hmmio) return MMSYSERR_INVALHANDLE;
    delete hmmio;
    return MMSYSERR_NOERROR;
}

LONG audio_mmio_read(HMMIO hmmio, HPSTR buffer, LONG bytes)
{
    if (!hmmio || bytes < 0) return -1;
    LONG left = hmmio->size - hmmio->pos;
    if (left < 0) left = 0;
    LONG count = std::min(bytes, left);
    if (count > 0) memcpy(buffer, hmmio->data + hmmio->pos, (size_t)count);
    hmmio->pos += count;
    return count;
}

LONG audio_mmio_seek(HMMIO hmmio, LONG offset, int origin)
{
    if (!hmmio) return -1;
    long long base = origin == SEEK_CUR ? hmmio->pos : origin == SEEK_END ? hmmio->size : 0;
    long long pos = base + offset;
    if (pos < 0 || pos > hmmio->size) return -1;
    hmmio->pos = (LONG)pos;
    return hmmio->pos;
}

/* The whole file is the I/O buffer, so the info's read window is all of
 * it and advancing past its end finds nothing more. */
MMRESULT audio_mmio_get_info(HMMIO hmmio, LPMMIOINFO info, UINT flags)
{
    (void)flags;
    if (!hmmio || !info) return MMSYSERR_INVALHANDLE;
    memset(info, 0, sizeof(*info));
    info->cchBuffer = hmmio->size;
    info->pchBuffer = hmmio->data;
    info->pchNext = hmmio->data + hmmio->pos;
    info->pchEndRead = hmmio->data + hmmio->size;
    info->pchEndWrite = hmmio->data + hmmio->size;
    info->lDiskOffset = hmmio->size;
    info->hmmio = hmmio;
    return MMSYSERR_NOERROR;
}

MMRESULT audio_mmio_set_info(HMMIO hmmio, const MMIOINFO* info, UINT flags)
{
    (void)flags;
    if (!hmmio || !info) return MMSYSERR_INVALHANDLE;
    hmmio->pos = (LONG)(info->pchNext - hmmio->data);
    return MMSYSERR_NOERROR;
}

MMRESULT audio_mmio_advance(HMMIO hmmio, LPMMIOINFO info, UINT flags)
{
    if (!hmmio || !info) return MMSYSERR_INVALHANDLE;
    audio_mmio_set_info(hmmio, info, flags);
    return audio_mmio_get_info(hmmio, info, flags);
}

MMRESULT audio_mmio_descend(HMMIO hmmio, LPMMCKINFO chunk,
                            const MMCKINFO* parent, UINT flags)
{
    if (!hmmio || !chunk) return MMSYSERR_INVALHANDLE;
    LONG end = hmmio->size;
    if (parent) {
        long long parentEnd = (long long)parent->dwDataOffset + parent->cksize;
        if (parentEnd < end) end = (LONG)parentEnd;
    }
    FOURCC wantId = chunk->ckid;
    FOURCC wantType = chunk->fccType;
    for (;;) {
        LONG start = hmmio->pos;
        MMCKINFO found;
        if (!ReadChunkHeader(hmmio, end, &found)) {
            hmmio->pos = start;
            return MMIOERR_CHUNKNOTFOUND;
        }
        bool match = true;
        if (flags & MMIO_FINDCHUNK) match = found.ckid == wantId;
        else if (flags & MMIO_FINDRIFF) match = found.ckid == FOURCC_RIFF && found.fccType == wantType;
        else if (flags & MMIO_FINDLIST) match = found.ckid == FOURCC_LIST && found.fccType == wantType;
        if (match) {
            *chunk = found;
            return MMSYSERR_NOERROR;
        }
        long long next = (long long)found.dwDataOffset + found.cksize + (found.cksize & 1);
        if (next > end) {
            hmmio->pos = start;
            return MMIOERR_CHUNKNOTFOUND;
        }
        hmmio->pos = (LONG)next;
    }
}

MMRESULT audio_mmio_ascend(HMMIO hmmio, LPMMCKINFO chunk, UINT flags)
{
    (void)flags;
    if (!hmmio || !chunk) return MMSYSERR_INVALHANDLE;
    long long next = (long long)chunk->dwDataOffset + chunk->cksize + (chunk->cksize & 1);
    if (next > hmmio->size) next = hmmio->size;
    hmmio->pos = (LONG)next;
    return MMSYSERR_NOERROR;
}

} /* extern "C" */

/* --- MCI cdaudio ------------------------------------------------------------ */

namespace {

/* MCI's error codes (mmsystem.h). */
const MCIERROR kMciErrUnrecognizedCommand = 261;  /* MCIERR_BASE + 5 */
const MCIERROR kMciErrInvalidDeviceName = MCIERR_INVALID_DEVICE_NAME;
const MCIERROR kMciErrDeviceNotReady = 276;       /* MCIERR_BASE + 20 */
const MCIERROR kMciErrOutOfRange = 282;           /* MCIERR_BASE + 26 */

const int kCdRate = 44100;
const int kCdChannels = 2;

struct CdAudio {
    bool open;
    bool playing;      /* under the stream's lock */
    int track;         /* 1-based, under the stream's lock */
    bool decoding;
    ma_decoder decoder;
    SDL_AudioDeviceID device;
    SDL_AudioStream* stream;
};

CdAudio s_cd;

struct FindEntry {
    const char* want;
    std::string found;
};

SDL_EnumerationResult SDLCALL FindEntryNoCase(void* userdata, const char* dirname,
                                              const char* fname)
{
    (void)dirname;
    FindEntry* find = static_cast<FindEntry*>(userdata);
    if (SDL_strcasecmp(fname, find->want) == 0) {
        find->found = fname;
        return SDL_ENUM_SUCCESS;
    }
    return SDL_ENUM_CONTINUE;
}

/* The name of an entry of dir that matches want without case, or "". */
std::string FindNoCase(const std::string& dir, const char* want)
{
    FindEntry find = { want, std::string() };
    SDL_EnumerateDirectory(dir.c_str(), FindEntryNoCase, &find);
    return find.found;
}

/* The file of the disc's track (1-based), or "" when there is none. */
std::string TrackPath(int track)
{
    if (track < 1 || track > 99) return std::string();
    std::string dir = FindNoCase(".", "Music");
    if (dir.empty()) return std::string();
    static const char* const kExtensions[] = { "ogg", "flac", "mp3", "wav" };
    for (const char* extension : kExtensions) {
        char name[32];
        snprintf(name, sizeof(name), "Track%02d.%s", track, extension);
        std::string file = FindNoCase(dir, name);
        if (!file.empty()) return dir + "/" + file;
    }
    return std::string();
}

bool AnyTrack()
{
    for (int track = 1; track <= 99; track++) {
        if (!TrackPath(track).empty()) return true;
    }
    return false;
}

/* Under the stream's lock: start decoding track, or stop when it has no
 * file. */
bool StartTrack(int track)
{
    if (s_cd.decoding) {
        ma_decoder_uninit(&s_cd.decoder);
        s_cd.decoding = false;
    }
    std::string path = TrackPath(track);
    if (path.empty()) return false;
    ma_decoder_config config = ma_decoder_config_init(ma_format_s16, kCdChannels, kCdRate);
    if (ma_decoder_init_file(path.c_str(), &config, &s_cd.decoder) != MA_SUCCESS) {
        SDL_LogWarn(SDL_LOG_CATEGORY_AUDIO, "cdaudio: cannot decode %s", path.c_str());
        return false;
    }
    s_cd.decoding = true;
    s_cd.track = track;
    return true;
}

/* The stream's get callback: decode on, and at a track's end go on to the
 * next track, as a disc plays "from" a track to its end. */
void SDLCALL FeedCd(void* userdata, SDL_AudioStream* stream, int additional,
                    int total)
{
    (void)userdata;
    (void)total;
    Sint16 frames[4096 * kCdChannels];
    const int frameBytes = (int)sizeof(Sint16) * kCdChannels;
    while (additional > 0 && s_cd.playing) {
        if (!s_cd.decoding) {
            s_cd.playing = false;
            break;
        }
        ma_uint64 want = (ma_uint64)std::min(4096, (additional + frameBytes - 1) / frameBytes);
        ma_uint64 got = 0;
        ma_result result = ma_decoder_read_pcm_frames(&s_cd.decoder, frames, want, &got);
        if (got > 0) {
            SDL_PutAudioStreamData(stream, frames, (int)got * frameBytes);
            additional -= (int)got * frameBytes;
        }
        if (got < want || result != MA_SUCCESS) {
            if (!StartTrack(s_cd.track + 1)) {
                s_cd.track++;
                s_cd.playing = false;
                SDL_FlushAudioStream(stream);
            }
        }
    }
}

bool OpenCdStream()
{
    if (s_cd.stream) return true;
    if (!SDL_WasInit(SDL_INIT_AUDIO) && !SDL_InitSubSystem(SDL_INIT_AUDIO)) return false;
    SDL_AudioSpec spec = { SDL_AUDIO_S16, kCdChannels, kCdRate };
    /* A logical device of its own: the DirectSound primary buffer's volume
     * does not reach the CD, as on Windows. */
    s_cd.device = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, NULL);
    if (s_cd.device == 0) return false;
    s_cd.stream = SDL_CreateAudioStream(&spec, &spec);
    if (!s_cd.stream) {
        SDL_CloseAudioDevice(s_cd.device);
        s_cd.device = 0;
        return false;
    }
    SDL_SetAudioStreamGetCallback(s_cd.stream, FeedCd, NULL);
    SDL_BindAudioStream(s_cd.device, s_cd.stream);
    return true;
}

void StopCd()
{
    if (!s_cd.stream) return;
    SDL_LockAudioStream(s_cd.stream);
    s_cd.playing = false;
    if (s_cd.decoding) {
        ma_decoder_uninit(&s_cd.decoder);
        s_cd.decoding = false;
    }
    SDL_ClearAudioStream(s_cd.stream);
    SDL_UnlockAudioStream(s_cd.stream);
}

void SetResult(LPSTR result, UINT capacity, const char* text)
{
    if (result && capacity) SDL_strlcpy(result, text, capacity);
}

} // namespace

extern "C" {

MCIERROR audio_mci_send_string(LPCSTR command, LPSTR result, UINT capacity,
                               HWND callback)
{
    (void)callback;
    SetResult(result, capacity, "");
    if (!command) return kMciErrUnrecognizedCommand;

    char verb[32] = "", device[32] = "", rest[64] = "";
    int number = 0;
    sscanf(command, "%31s %31s %63[^\n]", verb, device, rest);
    if (SDL_strcasecmp(device, "cdaudio") != 0) return kMciErrInvalidDeviceName;

    if (SDL_strcasecmp(verb, "open") == 0) {
        /* No track files: no disc in the drive. */
        if (!AnyTrack() || !OpenCdStream()) return kMciErrDeviceNotReady;
        s_cd.open = true;
        return 0;
    }
    if (!s_cd.open) return kMciErrInvalidDeviceName;

    if (SDL_strcasecmp(verb, "set") == 0) {
        return SDL_strcasecmp(rest, "time format tmsf") == 0 ? 0 : kMciErrUnrecognizedCommand;
    }
    if (SDL_strcasecmp(verb, "play") == 0) {
        if (sscanf(rest, "from %d", &number) != 1) return kMciErrUnrecognizedCommand;
        SDL_LockAudioStream(s_cd.stream);
        bool started = StartTrack(number);
        s_cd.playing = started;
        SDL_ClearAudioStream(s_cd.stream);
        SDL_UnlockAudioStream(s_cd.stream);
        return started ? 0 : kMciErrOutOfRange;
    }
    if (SDL_strcasecmp(verb, "status") == 0) {
        char text[16];
        SDL_LockAudioStream(s_cd.stream);
        bool playing = s_cd.playing || SDL_GetAudioStreamQueued(s_cd.stream) > 0;
        int track = s_cd.track;
        SDL_UnlockAudioStream(s_cd.stream);
        if (SDL_strcasecmp(rest, "mode") == 0) {
            SetResult(result, capacity, playing ? "playing" : "stopped");
            return 0;
        }
        if (SDL_strcasecmp(rest, "current track") == 0) {
            snprintf(text, sizeof(text), "%d", track);
            SetResult(result, capacity, text);
            return 0;
        }
        return kMciErrUnrecognizedCommand;
    }
    if (SDL_strcasecmp(verb, "stop") == 0) {
        StopCd();
        return 0;
    }
    if (SDL_strcasecmp(verb, "close") == 0) {
        StopCd();
        s_cd.open = false;
        return 0;
    }
    return kMciErrUnrecognizedCommand;
}

BOOL audio_mci_get_error_string(MCIERROR error, LPSTR text, UINT capacity)
{
    if (!text || !capacity) return FALSE;
    const char* message;
    switch (error) {
    case kMciErrUnrecognizedCommand: message = "The specified command was not recognized."; break;
    case kMciErrInvalidDeviceName: message = "The specified device is not open or is not recognized by MCI."; break;
    case kMciErrDeviceNotReady: message = "The device is not ready."; break;
    case kMciErrOutOfRange: message = "The specified parameter is out of range for the specified command."; break;
    default: snprintf(text, capacity, "MCI error %u", (unsigned)error); return error != 0;
    }
    SDL_strlcpy(text, message, capacity);
    return TRUE;
}

} /* extern "C" */
