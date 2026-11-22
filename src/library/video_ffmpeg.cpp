// SPDX-License-Identifier: LGPL-3.0-or-later
/* The video library (ADR 0007, [library:video]): FFmpeg, bound at runtime.
 * See video_ffmpeg.h for the interface.
 *
 * This is the decoder of the deleted port shell's movie module (movie.cpp,
 * before PR #862), trimmed to what the two video units call. The library
 * includes no FFmpeg header. Every FFmpeg object is opaque, except for
 * these reads of public fields whose places have not moved since FFmpeg 7
 * (libavformat 61), the oldest version the library accepts:
 *
 *   - AVFormatContext: nb_streams (+0x2c) and streams (+0x30);
 *   - AVStream: index (+0x08), codecpar (+0x10) and nb_frames (+0x38); the
 *     index read is checked against the stream's position, so a layout
 *     change fails the open instead of misreading;
 *   - AVCodecParameters: codec_type (+0x00);
 *   - AVPacket: stream_index (+0x24);
 *   - AVFrame: data[8], linesize[8], extended_data, width, height,
 *     nb_samples and format, the struct's head.
 *
 * Enumerations whose values change between versions (pixel and sample
 * formats) are looked up by name. Audio parameters come from the codec
 * context's AVOptions ("ar", "ch_layout"). */
#include "video_ffmpeg.h"

#include <SDL3/SDL.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* The build switch (issue #919): with LRR_MOVIES off, this library is a
 * stub that always reports "no movies"; the units skip their movies as
 * they do when FFmpeg is missing. On by default. */
#ifndef LRR_MOVIES
#define LRR_MOVIES 1
#endif

#if !LRR_MOVIES

extern "C" {

int video_available(void)
{
    return 0;
}

VideoFile* video_open(const char* path, int withAudio)
{
    (void)path;
    (void)withAudio;
    return NULL;
}

void video_close(VideoFile* video)
{
    (void)video;
}

void video_get_size(VideoFile* video, unsigned int* width, unsigned int* height)
{
    (void)video;
    *width = 0;
    *height = 0;
}

void video_get_rate(VideoFile* video, unsigned int* rate, unsigned int* scale)
{
    (void)video;
    *rate = 0;
    *scale = 0;
}

unsigned int video_get_frame_count(VideoFile* video)
{
    (void)video;
    return 0;
}

unsigned int video_get_duration(VideoFile* video)
{
    (void)video;
    return 0xffffffffu;
}

void video_start(VideoFile* video)
{
    (void)video;
}

int video_update(VideoFile* video, void* pixels, long pitch,
                 unsigned int bitCount, uint32_t redMask, uint32_t greenMask,
                 uint32_t blueMask)
{
    (void)video;
    (void)pixels;
    (void)pitch;
    (void)bitCount;
    (void)redMask;
    (void)greenMask;
    (void)blueMask;
    return 0;
}

const void* video_get_frame_dib(VideoFile* video, unsigned int index,
                                unsigned int bitCount, uint32_t redMask,
                                uint32_t greenMask, uint32_t blueMask)
{
    (void)video;
    (void)index;
    (void)bitCount;
    (void)redMask;
    (void)greenMask;
    (void)blueMask;
    return NULL;
}

} /* extern "C" */

#else /* LRR_MOVIES */

#ifndef LRR_VIDEO_LIBAVFORMAT
#define LRR_VIDEO_LIBAVFORMAT \
    "libavformat.so.63,libavformat.so.62,libavformat.so.61,libavformat.so"
#endif
#ifndef LRR_VIDEO_LIBAVCODEC
#define LRR_VIDEO_LIBAVCODEC \
    "libavcodec.so.63,libavcodec.so.62,libavcodec.so.61,libavcodec.so"
#endif
#ifndef LRR_VIDEO_LIBAVUTIL
#define LRR_VIDEO_LIBAVUTIL \
    "libavutil.so.61,libavutil.so.60,libavutil.so.59,libavutil.so"
#endif
#ifndef LRR_VIDEO_LIBSWSCALE
#define LRR_VIDEO_LIBSWSCALE \
    "libswscale.so.10,libswscale.so.9,libswscale.so.8,libswscale.so"
#endif

namespace {

/* The oldest libavformat and libavcodec majors (FFmpeg 7) whose layouts the
 * field reads above match. */
const int kMinMajor = 61;

const int kMediaVideo = 0; /* AVMEDIA_TYPE_VIDEO */
const int kMediaAudio = 1; /* AVMEDIA_TYPE_AUDIO */
const int kErrorAgain = -EAGAIN; /* AVERROR(EAGAIN) */
const int kSeekBackward = 1; /* AVSEEK_FLAG_BACKWARD */
const int kSwsBilinear = 2; /* SWS_BILINEAR */

struct Rational {
    int num, den;
};

/* The head of AVFrame. */
struct FrameHead {
    uint8_t* data[8];
    int linesize[8];
    uint8_t** extended_data;
    int width, height;
    int nb_samples;
    int format;
};

/* The head of AVPacket. */
struct PacketHead {
    void* buf;
    int64_t pts;
    int64_t dts;
    uint8_t* data;
    int size;
    int stream_index;
};

/* AVChannelLayout, read for nb_channels only; sized generously. */
struct ChannelLayout {
    int order;
    int nb_channels;
    unsigned char rest[48];
};

struct Lib {
    bool tried;
    bool ok;
    SDL_SharedObject* format;
    SDL_SharedObject* codec;
    SDL_SharedObject* util;
    SDL_SharedObject* scale;

    unsigned (*avformat_version)(void);
    int (*avformat_open_input)(void**, const char*, void*, void*);
    int (*avformat_find_stream_info)(void*, void*);
    void (*avformat_close_input)(void**);
    int (*av_find_best_stream)(void*, int, int, int, const void**, int);
    int (*av_read_frame)(void*, void*);
    int (*av_seek_frame)(void*, int, int64_t, int);
    Rational (*av_guess_frame_rate)(void*, void*, void*);
    unsigned (*avcodec_version)(void);
    void* (*avcodec_alloc_context3)(const void*);
    int (*avcodec_parameters_to_context)(void*, const void*);
    int (*avcodec_open2)(void*, const void*, void*);
    int (*avcodec_send_packet)(void*, const void*);
    int (*avcodec_receive_frame)(void*, void*);
    void (*avcodec_flush_buffers)(void*);
    void (*avcodec_free_context)(void**);
    void* (*av_packet_alloc)(void);
    void (*av_packet_free)(void**);
    void (*av_packet_unref)(void*);
    void* (*av_frame_alloc)(void);
    void (*av_frame_free)(void**);
    int (*av_get_pix_fmt)(const char*);
    int (*av_get_sample_fmt)(const char*);
    int (*av_opt_get_int)(void*, const char*, int, int64_t*);
    int (*av_opt_get_chlayout)(void*, const char*, int, ChannelLayout*);
    void (*av_channel_layout_uninit)(ChannelLayout*);
    void* (*sws_getContext)(int, int, int, int, int, int, int, void*,
                            const double*, const double*);
    int (*sws_scale)(void*, const uint8_t* const*, const int*, int, int,
                     uint8_t* const*, const int*);
    void (*sws_freeContext)(void*);
    void (*av_log_set_level)(int);

    /* Sample formats, by name: packed then planar. */
    int sampleU8, sampleS16, sampleS32, sampleFlt;
    int sampleU8p, sampleS16p, sampleS32p, sampleFltp;
};

Lib lib;

/* Loads the first name in a comma-separated list that opens. */
SDL_SharedObject* OpenFirst(const char* names)
{
    char buffer[512];
    SDL_strlcpy(buffer, names, sizeof(buffer));
    char* save = NULL;
    for (char* name = SDL_strtok_r(buffer, ",", &save); name;
         name = SDL_strtok_r(NULL, ",", &save)) {
        if (SDL_SharedObject* handle = SDL_LoadObject(name)) return handle;
    }
    return NULL;
}

template <typename F>
bool Bind(F& fn, SDL_SharedObject* handle, const char* name)
{
    fn = reinterpret_cast<F>(SDL_LoadFunction(handle, name));
    if (!fn) SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "video: no %s", name);
    return fn != NULL;
}

void CloseLibraries()
{
    SDL_SharedObject* handles[] = { lib.format, lib.codec, lib.util, lib.scale };
    for (SDL_SharedObject* handle : handles) {
        if (handle) SDL_UnloadObject(handle);
    }
    lib.format = lib.codec = lib.util = lib.scale = NULL;
}

bool BindLibraries()
{
    if (lib.tried) return lib.ok;
    lib.tried = true;

    lib.format = OpenFirst(LRR_VIDEO_LIBAVFORMAT);
    lib.codec = OpenFirst(LRR_VIDEO_LIBAVCODEC);
    lib.util = OpenFirst(LRR_VIDEO_LIBAVUTIL);
    lib.scale = OpenFirst(LRR_VIDEO_LIBSWSCALE);
    if (!lib.format || !lib.codec || !lib.util || !lib.scale) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                    "video: FFmpeg libraries not found; movies are skipped");
        CloseLibraries();
        return false;
    }

    bool ok = Bind(lib.avformat_version, lib.format, "avformat_version") &&
              Bind(lib.avcodec_version, lib.codec, "avcodec_version");
    if (ok && ((int)(lib.avformat_version() >> 16) < kMinMajor ||
               (int)(lib.avcodec_version() >> 16) < kMinMajor)) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                    "video: FFmpeg older than 7; movies are skipped");
        ok = false;
    }
    ok = ok && Bind(lib.avformat_open_input, lib.format, "avformat_open_input") &&
         Bind(lib.avformat_find_stream_info, lib.format, "avformat_find_stream_info") &&
         Bind(lib.avformat_close_input, lib.format, "avformat_close_input") &&
         Bind(lib.av_find_best_stream, lib.format, "av_find_best_stream") &&
         Bind(lib.av_read_frame, lib.format, "av_read_frame") &&
         Bind(lib.av_seek_frame, lib.format, "av_seek_frame") &&
         Bind(lib.av_guess_frame_rate, lib.format, "av_guess_frame_rate") &&
         Bind(lib.avcodec_alloc_context3, lib.codec, "avcodec_alloc_context3") &&
         Bind(lib.avcodec_parameters_to_context, lib.codec, "avcodec_parameters_to_context") &&
         Bind(lib.avcodec_open2, lib.codec, "avcodec_open2") &&
         Bind(lib.avcodec_send_packet, lib.codec, "avcodec_send_packet") &&
         Bind(lib.avcodec_receive_frame, lib.codec, "avcodec_receive_frame") &&
         Bind(lib.avcodec_flush_buffers, lib.codec, "avcodec_flush_buffers") &&
         Bind(lib.avcodec_free_context, lib.codec, "avcodec_free_context") &&
         Bind(lib.av_packet_alloc, lib.codec, "av_packet_alloc") &&
         Bind(lib.av_packet_free, lib.codec, "av_packet_free") &&
         Bind(lib.av_packet_unref, lib.codec, "av_packet_unref") &&
         Bind(lib.av_frame_alloc, lib.util, "av_frame_alloc") &&
         Bind(lib.av_frame_free, lib.util, "av_frame_free") &&
         Bind(lib.av_get_pix_fmt, lib.util, "av_get_pix_fmt") &&
         Bind(lib.av_get_sample_fmt, lib.util, "av_get_sample_fmt") &&
         Bind(lib.av_opt_get_int, lib.util, "av_opt_get_int") &&
         Bind(lib.av_opt_get_chlayout, lib.util, "av_opt_get_chlayout") &&
         Bind(lib.av_channel_layout_uninit, lib.util, "av_channel_layout_uninit") &&
         Bind(lib.sws_getContext, lib.scale, "sws_getContext") &&
         Bind(lib.sws_scale, lib.scale, "sws_scale") &&
         Bind(lib.sws_freeContext, lib.scale, "sws_freeContext");
    if (!ok) {
        CloseLibraries();
        return false;
    }

    /* Optional: keep FFmpeg's notices out of the game's log. */
    lib.av_log_set_level = reinterpret_cast<decltype(lib.av_log_set_level)>(
        SDL_LoadFunction(lib.util, "av_log_set_level"));
    if (lib.av_log_set_level) lib.av_log_set_level(16); /* AV_LOG_ERROR */

    lib.sampleU8 = lib.av_get_sample_fmt("u8");
    lib.sampleS16 = lib.av_get_sample_fmt("s16");
    lib.sampleS32 = lib.av_get_sample_fmt("s32");
    lib.sampleFlt = lib.av_get_sample_fmt("flt");
    lib.sampleU8p = lib.av_get_sample_fmt("u8p");
    lib.sampleS16p = lib.av_get_sample_fmt("s16p");
    lib.sampleS32p = lib.av_get_sample_fmt("s32p");
    lib.sampleFltp = lib.av_get_sample_fmt("fltp");
    lib.ok = true;
    return true;
}

/* The stream at index, or NULL when the context's layout is not the one
 * the library reads. */
void* StreamAt(void* format, int index)
{
    unsigned int count = *(unsigned int*)((char*)format + 0x2c);
    void** streams = *(void***)((char*)format + 0x30);
    if (index < 0 || (unsigned int)index >= count || count > 64 || !streams) {
        return NULL;
    }
    void* stream = streams[index];
    if (!stream || *(int*)((char*)stream + 0x08) != index) return NULL;
    return stream;
}

void* StreamParameters(void* stream)
{
    return *(void**)((char*)stream + 0x10);
}

int64_t StreamFrameCount(void* stream)
{
    return *(int64_t*)((char*)stream + 0x38);
}

/* The FFmpeg pixel format whose memory layout matches a DirectDraw pixel
 * format, or -1. */
int PixelFormat(unsigned int bitCount, uint32_t red, uint32_t green,
                uint32_t blue)
{
    const char* name = NULL;
    if (bitCount == 16 && red == 0xf800 && green == 0x07e0 && blue == 0x001f) {
        name = "rgb565le";
    } else if (bitCount == 16 && red == 0x7c00 && green == 0x03e0 && blue == 0x001f) {
        name = "rgb555le";
    } else if (bitCount == 16 && red == 0x001f && green == 0x07e0 && blue == 0xf800) {
        name = "bgr565le";
    } else if (bitCount == 24 && red == 0xff0000 && green == 0xff00 && blue == 0xff) {
        name = "bgr24";
    } else if (bitCount == 24 && red == 0xff && green == 0xff00 && blue == 0xff0000) {
        name = "rgb24";
    } else if (bitCount == 32 && red == 0xff0000 && green == 0xff00 && blue == 0xff) {
        name = "bgr0";
    } else if (bitCount == 32 && red == 0xff && green == 0xff00 && blue == 0xff0000) {
        name = "rgb0";
    }
    return name ? lib.av_get_pix_fmt(name) : -1;
}

} /* namespace */

struct VideoFile {
    void* format;          /* AVFormatContext */
    int videoIndex;
    void* videoCodec;      /* AVCodecContext */
    int audioIndex;        /* -1 without audio */
    void* audioCodec;
    void* packet;
    FrameHead* frame;
    FrameHead* audioFrame;
    bool draining;         /* the demuxer reached the end */

    unsigned int width, height;
    Rational rate;
    unsigned int frameCount;
    unsigned int nextIndex; /* the index of the next frame decoded */

    bool started;
    Uint64 startTicks;     /* SDL_GetTicksNS at video_start */

    void* scaler;          /* SwsContext */
    int scalerFormat;
    uint8_t* image;        /* a top-down conversion for the DIB path */
    uint8_t* dib;
    size_t dibSize;
    unsigned int dibIndex;  /* the frame in dib, or UINT_MAX */
    int dibFormat;

    int audioRate, audioChannels;
    bool audioInit;        /* SDL's audio subsystem is up */
    SDL_AudioStream* audio;
    SDL_AudioSpec audioSpec;
    void* audioScratch;
    size_t audioScratchSize;
};

namespace {

/* Opens the decoder for the stream at index. */
void* OpenCodec(VideoFile* video, int index, const void* decoder)
{
    void* stream = StreamAt(video->format, index);
    if (!stream || !decoder) return NULL;
    void* codec = lib.avcodec_alloc_context3(decoder);
    if (!codec) return NULL;
    if (lib.avcodec_parameters_to_context(codec, StreamParameters(stream)) < 0 ||
        lib.avcodec_open2(codec, decoder, NULL) < 0) {
        lib.avcodec_free_context(&codec);
        return NULL;
    }
    return codec;
}

void OpenAudio(VideoFile* video)
{
    const void* decoder = NULL;
    int index = lib.av_find_best_stream(video->format, kMediaAudio, -1,
                                        video->videoIndex, &decoder, 0);
    if (index < 0) return;
    void* codec = OpenCodec(video, index, decoder);
    if (!codec) return;

    int64_t rate = 0;
    ChannelLayout layout;
    memset(&layout, 0, sizeof(layout));
    int channels = 0;
    if (lib.av_opt_get_chlayout(codec, "ch_layout", 0, &layout) >= 0) {
        channels = layout.nb_channels;
        lib.av_channel_layout_uninit(&layout);
    }
    if (lib.av_opt_get_int(codec, "ar", 0, &rate) < 0 || rate < 4000 ||
        rate > 192000 || channels < 1 || channels > 8) {
        lib.avcodec_free_context(&codec);
        return;
    }
    video->audioIndex = index;
    video->audioCodec = codec;
    video->audioRate = (int)rate;
    video->audioChannels = channels;
}

/* SDL's audio format for an FFmpeg sample format, and whether it is
 * planar. */
bool AudioFormat(int format, SDL_AudioFormat* sdl, bool* planar)
{
    struct { int ffmpeg; SDL_AudioFormat sdl; bool planar; } formats[] = {
        { lib.sampleU8, SDL_AUDIO_U8, false },
        { lib.sampleS16, SDL_AUDIO_S16, false },
        { lib.sampleS32, SDL_AUDIO_S32, false },
        { lib.sampleFlt, SDL_AUDIO_F32, false },
        { lib.sampleU8p, SDL_AUDIO_U8, true },
        { lib.sampleS16p, SDL_AUDIO_S16, true },
        { lib.sampleS32p, SDL_AUDIO_S32, true },
        { lib.sampleFltp, SDL_AUDIO_F32, true },
    };
    for (const auto& entry : formats) {
        if (entry.ffmpeg >= 0 && entry.ffmpeg == format) {
            *sdl = entry.sdl;
            *planar = entry.planar;
            return true;
        }
    }
    return false;
}

/* Queues one decoded audio frame on the SDL stream. */
void QueueAudio(VideoFile* video, const FrameHead* frame)
{
    SDL_AudioFormat format;
    bool planar;
    if (!video->audio || frame->nb_samples <= 0 ||
        !AudioFormat(frame->format, &format, &planar)) {
        return;
    }
    if (format != video->audioSpec.format) {
        video->audioSpec.format = format;
        SDL_SetAudioStreamFormat(video->audio, &video->audioSpec, NULL);
    }
    int channels = video->audioChannels;
    size_t sampleSize = SDL_AUDIO_BYTESIZE(format);
    size_t bytes = (size_t)frame->nb_samples * channels * sampleSize;
    if (!planar) {
        SDL_PutAudioStreamData(video->audio, frame->data[0], (int)bytes);
        return;
    }
    if (bytes > video->audioScratchSize) {
        void* grown = realloc(video->audioScratch, bytes);
        if (!grown) return;
        video->audioScratch = grown;
        video->audioScratchSize = bytes;
    }
    uint8_t* out = (uint8_t*)video->audioScratch;
    uint8_t** planes = frame->extended_data ? frame->extended_data
                                            : (uint8_t**)frame->data;
    for (int i = 0; i < frame->nb_samples; i++) {
        for (int c = 0; c < channels; c++) {
            memcpy(out, planes[c] + (size_t)i * sampleSize, sampleSize);
            out += sampleSize;
        }
    }
    SDL_PutAudioStreamData(video->audio, video->audioScratch, (int)bytes);
}

void DecodeAudio(VideoFile* video, const void* packet)
{
    if (lib.avcodec_send_packet(video->audioCodec, packet) < 0) return;
    while (lib.avcodec_receive_frame(video->audioCodec, video->audioFrame) == 0) {
        QueueAudio(video, video->audioFrame);
    }
}

/* Decodes the next video frame into video->frame, reading packets as the
 * decoder asks for them and queueing the audio packets read on the way.
 * Returns false at the end of the stream or on an error. */
bool DecodeFrame(VideoFile* video)
{
    for (;;) {
        int result = lib.avcodec_receive_frame(video->videoCodec, video->frame);
        if (result == 0) {
            video->nextIndex++;
            return true;
        }
        if (result != kErrorAgain || video->draining) return false;

        if (lib.av_read_frame(video->format, video->packet) < 0) {
            video->draining = true;
            lib.avcodec_send_packet(video->videoCodec, NULL);
            if (video->audioCodec) DecodeAudio(video, NULL);
            continue;
        }
        int stream = ((PacketHead*)video->packet)->stream_index;
        if (stream == video->videoIndex) {
            /* A packet the decoder rejects is skipped, as a damaged frame. */
            lib.avcodec_send_packet(video->videoCodec, video->packet);
        } else if (stream == video->audioIndex && video->audioCodec) {
            DecodeAudio(video, video->packet);
        }
        lib.av_packet_unref(video->packet);
    }
}

/* Converts video->frame into a top-down image. */
bool ConvertFrame(VideoFile* video, int format, void* pixels, long pitch)
{
    FrameHead* frame = video->frame;
    if (frame->width != (int)video->width || frame->height != (int)video->height ||
        frame->format < 0) {
        return false;
    }
    if (!video->scaler || video->scalerFormat != format) {
        if (video->scaler) lib.sws_freeContext(video->scaler);
        video->scaler = lib.sws_getContext(frame->width, frame->height,
                                           frame->format, frame->width,
                                           frame->height, format, kSwsBilinear,
                                           NULL, NULL, NULL);
        video->scalerFormat = format;
        if (!video->scaler) return false;
    }
    uint8_t* planes[4] = { (uint8_t*)pixels, NULL, NULL, NULL };
    int pitches[4] = { (int)pitch, 0, 0, 0 };
    return lib.sws_scale(video->scaler, (const uint8_t* const*)frame->data,
                         frame->linesize, 0, frame->height, planes, pitches) > 0;
}

void StartAudio(VideoFile* video)
{
    if (!video->audioCodec) return;
    if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "video: no audio: %s",
                    SDL_GetError());
        return;
    }
    video->audioInit = true;
    video->audioSpec.format = SDL_AUDIO_S16;
    video->audioSpec.channels = video->audioChannels;
    video->audioSpec.freq = video->audioRate;
    video->audio = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
                                             &video->audioSpec, NULL, NULL);
    if (!video->audio) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "video: no audio: %s",
                    SDL_GetError());
        return;
    }
    SDL_ResumeAudioStreamDevice(video->audio);
}

} /* namespace */

extern "C" {

int video_available(void)
{
    return BindLibraries() ? 1 : 0;
}

VideoFile* video_open(const char* path, int withAudio)
{
    if (!path || !*path || !BindLibraries()) return NULL;

    VideoFile* video = (VideoFile*)calloc(1, sizeof(VideoFile));
    if (!video) return NULL;
    video->audioIndex = -1;
    video->dibIndex = 0xffffffffu;

    if (lib.avformat_open_input(&video->format, path, NULL, NULL) < 0) {
        video->format = NULL;
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "video: cannot open %s", path);
        video_close(video);
        return NULL;
    }
    const void* decoder = NULL;
    if (lib.avformat_find_stream_info(video->format, NULL) < 0 ||
        (video->videoIndex = lib.av_find_best_stream(video->format, kMediaVideo,
                                                     -1, -1, &decoder, 0)) < 0 ||
        !(video->videoCodec = OpenCodec(video, video->videoIndex, decoder))) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                    "video: no decodable video in %s", path);
        video_close(video);
        return NULL;
    }
    void* stream = StreamAt(video->format, video->videoIndex);
    if (*(int*)StreamParameters(stream) != kMediaVideo) {
        video_close(video);
        return NULL;
    }
    if (withAudio) OpenAudio(video);

    video->packet = lib.av_packet_alloc();
    video->frame = (FrameHead*)lib.av_frame_alloc();
    video->audioFrame = (FrameHead*)lib.av_frame_alloc();
    if (!video->packet || !video->frame || !video->audioFrame) {
        video_close(video);
        return NULL;
    }

    /* The size comes from the first frame, which is then decoded again. */
    if (!DecodeFrame(video) || video->frame->width <= 0 || video->frame->height <= 0) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                    "video: cannot decode %s", path);
        video_close(video);
        return NULL;
    }
    video->width = (unsigned int)video->frame->width;
    video->height = (unsigned int)video->frame->height;

    video->rate = lib.av_guess_frame_rate(video->format, stream, NULL);
    if (video->rate.num <= 0 || video->rate.den <= 0) {
        video->rate.num = 15;
        video->rate.den = 1;
    }
    int64_t frames = StreamFrameCount(stream);
    video->frameCount = frames > 0 && frames < 0x7fffffff ? (unsigned int)frames : 0;

    /* Rewind; the audio read on the way is dropped with the rest. */
    if (lib.av_seek_frame(video->format, video->videoIndex, 0, kSeekBackward) < 0) {
        video_close(video);
        return NULL;
    }
    lib.avcodec_flush_buffers(video->videoCodec);
    if (video->audioCodec) lib.avcodec_flush_buffers(video->audioCodec);
    video->nextIndex = 0;
    video->draining = false;
    return video;
}

void video_close(VideoFile* video)
{
    if (!video) return;
    if (video->audio) SDL_DestroyAudioStream(video->audio);
    if (video->audioInit) SDL_QuitSubSystem(SDL_INIT_AUDIO);
    if (video->scaler) lib.sws_freeContext(video->scaler);
    if (video->frame) lib.av_frame_free((void**)&video->frame);
    if (video->audioFrame) lib.av_frame_free((void**)&video->audioFrame);
    if (video->packet) lib.av_packet_free(&video->packet);
    if (video->videoCodec) lib.avcodec_free_context(&video->videoCodec);
    if (video->audioCodec) lib.avcodec_free_context(&video->audioCodec);
    if (video->format) lib.avformat_close_input(&video->format);
    free(video->image);
    free(video->dib);
    free(video->audioScratch);
    free(video);
}

void video_get_size(VideoFile* video, unsigned int* width, unsigned int* height)
{
    *width = video->width;
    *height = video->height;
}

void video_get_rate(VideoFile* video, unsigned int* rate, unsigned int* scale)
{
    *rate = (unsigned int)video->rate.num;
    *scale = (unsigned int)video->rate.den;
}

unsigned int video_get_frame_count(VideoFile* video)
{
    return video->frameCount;
}

unsigned int video_get_duration(VideoFile* video)
{
    if (!video->frameCount) return 0xffffffffu;
    return (unsigned int)((uint64_t)video->frameCount * 1000u *
                          (unsigned int)video->rate.den /
                          (unsigned int)video->rate.num);
}

void video_start(VideoFile* video)
{
    if (video->started) return;
    video->started = true;
    StartAudio(video);
    video->startTicks = SDL_GetTicksNS();
}

int video_update(VideoFile* video, void* pixels, long pitch,
                 unsigned int bitCount, uint32_t redMask, uint32_t greenMask,
                 uint32_t blueMask)
{
    int format = PixelFormat(bitCount, redMask, greenMask, blueMask);
    if (format < 0) return 0;
    if (!video->started) video_start(video);

    /* Frame n is due n / rate seconds after the start. Late frames are
     * decoded, for the codec's sake, and not shown. */
    const uint64_t num = (uint64_t)video->rate.num;
    const uint64_t den = (uint64_t)video->rate.den;
    uint64_t elapsed = SDL_GetTicksNS() - video->startTicks;
    uint64_t current = elapsed * num / (den * SDL_NS_PER_SECOND);
    while (video->nextIndex < current) {
        if (!DecodeFrame(video)) return 0;
    }
    if (!DecodeFrame(video)) return 0;

    uint64_t due = (uint64_t)(video->nextIndex - 1) * den * SDL_NS_PER_SECOND / num;
    elapsed = SDL_GetTicksNS() - video->startTicks;
    if (due > elapsed) SDL_DelayNS(due - elapsed);

    return ConvertFrame(video, format, pixels, pitch) ? 1 : 0;
}

const void* video_get_frame_dib(VideoFile* video, unsigned int index,
                                unsigned int bitCount, uint32_t redMask,
                                uint32_t greenMask, uint32_t blueMask)
{
    int format = PixelFormat(bitCount, redMask, greenMask, blueMask);
    if (format < 0) return NULL;
    if (video->dib && video->dibIndex == index && video->dibFormat == format) {
        return video->dib;
    }

    /* video->frame holds frame nextIndex - 1; an earlier frame is decoded
     * again from the start. */
    if (index + 1 < video->nextIndex) {
        if (lib.av_seek_frame(video->format, video->videoIndex, 0, kSeekBackward) < 0) {
            return NULL;
        }
        lib.avcodec_flush_buffers(video->videoCodec);
        video->nextIndex = 0;
        video->draining = false;
    }
    while (video->nextIndex <= index) {
        if (!DecodeFrame(video)) return NULL;
    }

    const size_t header = 40;
    long rowSize = (long)video->width * (long)(bitCount / 8);
    long stride = (rowSize + 3) & ~3L;
    size_t imageSize = (size_t)stride * video->height;
    if (!video->dib || video->dibSize != header + imageSize) {
        free(video->dib);
        free(video->image);
        video->dib = (uint8_t*)calloc(1, header + imageSize);
        video->image = (uint8_t*)malloc(imageSize);
        video->dibSize = video->dib && video->image ? header + imageSize : 0;
        if (!video->dibSize) return NULL;
    }
    if (!ConvertFrame(video, format, video->image, stride)) return NULL;

    /* BITMAPINFOHEADER, little-endian fixed-width fields. */
    uint8_t* dib = video->dib;
    SDL_memset(dib, 0, header);
    uint32_t size = (uint32_t)header, width = video->width, height = video->height;
    uint16_t planes = 1, bits = (uint16_t)bitCount;
    uint32_t imageBytes = (uint32_t)imageSize;
    SDL_memcpy(dib + 0, &size, 4);
    SDL_memcpy(dib + 4, &width, 4);
    SDL_memcpy(dib + 8, &height, 4);
    SDL_memcpy(dib + 12, &planes, 2);
    SDL_memcpy(dib + 14, &bits, 2);
    SDL_memcpy(dib + 20, &imageBytes, 4);
    for (unsigned int y = 0; y < video->height; y++) {
        SDL_memcpy(dib + header + (size_t)(video->height - 1 - y) * stride,
                   video->image + (size_t)y * stride, (size_t)stride);
    }
    video->dibIndex = index;
    video->dibFormat = format;
    return dib;
}

} /* extern "C" */

#endif /* LRR_MOVIES */
