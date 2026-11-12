// SPDX-License-Identifier: LGPL-3.0-or-later
/* gods98 engine — AVI movie module (see movie.hpp for the design).
 *
 * The native FFmpeg binding uses no headers or link dependency. The Windows
 * backend separately binds the documented AVIFIL32 API at runtime. The four
 * versioned shared libraries are dlopen'd on first use and the ~26-symbol
 * subset is dlsym'ed once into a table. All library structs are opaque handles except
 * two deliberately tiny reads:
 *
 *   - the AVFrame *prefix* (data[8]/linesize[8]/width/height/nb_samples/
 *     format) — these members have sat at the top of the struct untouched
 *     since FFmpeg 0.x and are read by every binding in existence;
 *   - `sample_rate` in AVCodecParameters (offset 0x88 on the FFmpeg >= 7
 *     layout, after the 32-byte AVChannelLayout) — sanity-checked against
 *     8k..192k with a 22050 Hz fallback, and pinned by `movie_test --real`.
 *
 * Everything else (format context, codec contexts, packets, sws) is opaque.
 * The library-strict layering is why the version gate reads libavcodec
 * >= 61 (FFmpeg 7+): older builds carry the deprecated `channels` fields
 * that shift the sample-rate offset.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "movie.hpp"
#include "engine.hpp"
#include "resource.hpp"
#include "string.hpp"
#include "surface.hpp"
#include "heap.hpp"
#include "platform/platform.h"

#if defined(LEGORRR_BACKEND_WINAPI)
#include <windows.h>
#endif

/* FFmpeg runtime sonames to dlopen, ';'-joined. The meson toolchain owns
 * these — the same literals drive the configure-time probe and are passed as
 * defines (meson.build, `movie_libs`); the lists below are the fallback when
 * building outside meson (kept in sync by hand). */
#if defined(__APPLE__)
#ifndef LEGORRR_MOVIE_LIBAVFORMAT
#define LEGORRR_MOVIE_LIBAVFORMAT                                              \
    "libavformat.63.dylib;libavformat.62.dylib;libavformat.61.dylib;libavformat.dylib"
#endif
#ifndef LEGORRR_MOVIE_LIBAVCODE
#define LEGORRR_MOVIE_LIBAVCODE                                                \
    "libavcodec.63.dylib;libavcodec.62.dylib;libavcodec.61.dylib;libavcodec.dylib"
#endif
#ifndef LEGORRR_MOVIE_LIBAVUTIL
#define LEGORRR_MOVIE_LIBAVUTIL                                                \
    "libavutil.61.dylib;libavutil.60.dylib;libavutil.59.dylib;libavutil.dylib"
#endif
#ifndef LEGORRR_MOVIE_LIBSWSCALE
#define LEGORRR_MOVIE_LIBSWSCALE                                               \
    "libswscale.10.dylib;libswscale.9.dylib;libswscale.8.dylib;libswscale.dylib"
#endif
#else
#ifndef LEGORRR_MOVIE_LIBAVFORMAT
#define LEGORRR_MOVIE_LIBAVFORMAT                                              \
    "libavformat.so.63;libavformat.so.62;libavformat.so.61;libavformat.so"
#endif
#ifndef LEGORRR_MOVIE_LIBAVCODE
#define LEGORRR_MOVIE_LIBAVCODE                                                \
    "libavcodec.so.63;libavcodec.so.62;libavcodec.so.61;libavcodec.so"
#endif
#ifndef LEGORRR_MOVIE_LIBAVUTIL
#define LEGORRR_MOVIE_LIBAVUTIL                                                \
    "libavutil.so.61;libavutil.so.60;libavutil.so.59;libavutil.so"
#endif
#ifndef LEGORRR_MOVIE_LIBSWSCALE
#define LEGORRR_MOVIE_LIBSWSCALE                                               \
    "libswscale.so.9;libswscale.so.8;libswscale.so.7;libswscale.so"
#endif
#endif

/* ------------------------------------------------------------------ *
 *  FFmpeg runtime binding (native backend only)
 * ------------------------------------------------------------------ */
#if defined(LEGORRR_BACKEND_MINIWIN)

#include <dlfcn.h>
#include <dirent.h>
#include <sys/stat.h>

#endif

/* The decoder machinery itself is compiled out when the build is configured
 * with `-Dmovies=disabled` (LEGORRR_MOVIES_CFG_DISABLED); the binding above
 * (dlfcn) is still in scope on miniwin for the includes below. */
#if defined(LEGORRR_BACKEND_MINIWIN) && !defined(LEGORRR_MOVIES_CFG_DISABLED)

typedef struct ff_rat { int num, den; } ff_rat;

/* AVFormatContext prefix — nb_streams/streams at the start of the struct.
 * Offsets verified empirically against libavformat 62 (FFmpeg 8.1) on this
 * box; sanity-checked at use so a layout drift fails loudly, not corruptly:
 *   av_find_best_stream on this build returns the *index* (not a pointer)
 *   and additionally skips audio streams via an internal codecpar gate, so
 *   stream selection is walked manually instead. */
typedef struct ff_fmt {
    char   _pad[0x2c];
    int    nb_streams;   /* 0x2c */
    void** streams;      /* 0x30 */
} ff_fmt;

/* AVStream prefix — verified leading members: index@0, id@4, codecpar@0x10
 * (this build keeps a private field between id and codecpar). */
typedef struct ff_stream {
    int    index;
    int    id;
    int    _pad0;        /* 0x08 */
    int    _pad1;        /* 0x0c */
    void*  codecpar;     /* 0x10 — codecpar->codec_type@0, codec_id@4 */
} ff_stream;

/* AVCodecParameters, FFmpeg >= 7 layout. `sample_rate` at 0x88: every field
 * up to video_delay (0x60) is a fixed set of ints/enums; then the
 * 32-byte AVChannelLayout (0x68..0x88); then sample_rate. See header. */
typedef struct ff_codecpar {
    int      codec_type;
    int      codec_id;
    uint32_t codec_tag;
    void*    extradata;
    int      extradata_size;
    int      format;
    int64_t  bit_rate;
    int      bits_per_coded_sample;
    int      bits_per_raw_sample;
    int      profile;
    int      level;
    int      width;
    int      height;
    ff_rat   sample_aspect_ratio;
    int      field_order;
    int      color_range;
    int      color_primaries;
    int      color_trc;
    int      color_space;
    int      chroma_location;
    int      video_delay;
    /* AVChannelLayout ch_layout (32 bytes, FFmpeg >= 7) */
    int      ch_order;
    int      ch_nb;
    int64_t  ch_map;
    int      ch_mask;
    void*    ch_opaque;
    /* 0x88 */
    int      sample_rate;      /* <-- offset 136 */
    int      block_align;
    int      frame_size;
    int      initial_padding;
    int      trailing_padding;
    int      seek_preroll;
} ff_codecpar;

/* AVFrame prefix — data/linesize/width/height/nb_samples/format are the
 * stable public head of the struct; nothing below format is read. */
typedef struct ff_frame {
    uint8_t* data[8];
    int      linesize[8];
    uint8_t** extended_data;
    int      width, height;
    int      nb_samples;
    int      format;
} ff_frame;

/* AVPacket prefix — buf/pts/dts/data/size/stream_index/flags are the
 * stable public head of the struct (frozen since FFmpeg 3.x). */
typedef struct ff_pkt {
    void*    buf;          /* AVBufferRef* */
    int64_t  pts;
    int64_t  dts;
    uint8_t* data;
    int      size;
    int      stream_index; /* offset 0x24 */
    int      flags;
} ff_pkt;

/* --- opaque library types ----------------------------------------------- */
typedef struct FFFormatCtx FFFormatCtx;
typedef struct FFCtx      FFCtx;   /* AVCodecContext */
typedef struct FFPacket    FFPacket;
typedef struct FFSws      FFSws;
typedef struct FFCodec    FFCodec;

/* Decode error / pacing constants (mirror libavutil/error.h for the cases
 * we branch on). av_read_frame uses negative error codes; EAGAIN is the
 * standard "need more input" signal. */
#define FF_EAGAIN (-11)   /* -EAGAIN */

typedef int          (*f_get_pix_fmt)(const char*);

typedef int          (*f_open_in)(FFFormatCtx**, const char*, void*, void*);
typedef int          (*f_find_info)(FFFormatCtx*, void*);
typedef void         (*f_close_in)(FFFormatCtx**);
typedef int          (*f_read_frame)(FFFormatCtx*, FFPacket*);
typedef ff_rat       (*f_guess_rate)(FFFormatCtx*, void*, ff_frame*);
typedef unsigned     (*f_codec_version)(void);
typedef const FFCodec* (*f_find_decoder)(int);
typedef FFCtx*       (*f_alloc_ctx3)(const FFCodec*);
typedef int          (*f_params_to_ctx)(FFCtx*, const ff_codecpar*);
typedef int          (*f_open2)(FFCtx*, const FFCodec*, void*);
typedef int          (*f_send_pkt)(FFCtx*, const FFPacket*);
typedef int          (*f_recv_frame)(FFCtx*, ff_frame*);
typedef void         (*f_free_ctx)(FFCtx**);
typedef FFPacket*    (*f_pkt_alloc)(void);
typedef void         (*f_pkt_free)(FFPacket**);
typedef void         (*f_pkt_unref)(FFPacket*);
typedef ff_frame*    (*f_frame_alloc)(void);
typedef void         (*f_frame_free)(ff_frame**);
typedef void         (*f_frame_unref)(ff_frame*);
typedef int          (*f_bytes_per_sample)(int);
typedef int          (*f_sample_fmt_planar)(int);
typedef void*        (*f_sws_get)(int, int, int, int, int, int, int,
                                 void*, const double*, const double*);
typedef int          (*f_sws_scale)(void*, const uint8_t* const*, const int[],
                                    int, int, uint8_t* const[], const int[]);
typedef void         (*f_sws_free)(void*);

static struct {
    int ok;
    int codec_major;
    void* fmt_h;
    void* codec_h;
    void* util_h;
    void* scale_h;
    f_open_in          avformat_open_input;
    f_find_info        avformat_find_stream_info;
    f_close_in         avformat_close_input;
    f_read_frame       av_read_frame;
    f_guess_rate       av_guess_frame_rate;
    f_codec_version    avcodec_version;
    f_find_decoder     avcodec_find_decoder;
    f_alloc_ctx3       avcodec_alloc_context3;
    f_params_to_ctx    avcodec_parameters_to_context;
    f_open2            avcodec_open2;
    f_send_pkt         avcodec_send_packet;
    f_recv_frame       avcodec_receive_frame;
    f_free_ctx         avcodec_free_context;
    f_pkt_alloc        av_packet_alloc;
    f_pkt_free         av_packet_free;
    f_pkt_unref        av_packet_unref;
    f_frame_alloc      av_frame_alloc;
    f_frame_free       av_frame_free;
    f_frame_unref      av_frame_unref;
    f_bytes_per_sample av_get_bytes_per_sample;
    f_sample_fmt_planar av_sample_fmt_is_planar;
    f_get_pix_fmt     av_get_pix_fmt;
    f_sws_get          sws_getContext;
    f_sws_scale        sws_scale;
    f_sws_free         sws_freeContext;
} ff;

/* Split the ';'-joined soname list and dlopen the first one that resolves.
 * The lists are owned by meson (movie_libs in meson.build); this is the same
 * probe the configure-time check runs. */
static void* ff_dlopen_first(const char* joined)
{
    char buf[512];
    if (joined == NULL || joined[0] == '\0') {
        return NULL;
    }
    size_t n = strlen(joined);
    if (n >= sizeof(buf)) {
        n = sizeof(buf) - 1;
    }
    memcpy(buf, joined, n);
    buf[n] = '\0';
    char* p = buf;
    for (;;) {
        while (*p == ';') {
            ++p;
        }
        if (*p == '\0') {
            break;
        }
        char* end = strchr(p, ';');
        if (end != NULL) {
            *end = '\0';
        }
        void* h = dlopen(p, RTLD_LAZY | RTLD_LOCAL);
        if (h != NULL) {
            return h;
        }
        if (end == NULL) {
            break;
        }
        p = end + 1;
    }
    return NULL;
}

#define FF_NEED(ffname, sym, handle)                                        \
    do {                                                                    \
        *(void**)(&(ffname)->sym) = dlsym((handle), #sym);                  \
        if ((ffname)->sym == NULL) {                                        \
            fprintf(stderr, "movie: dlsym(%s) failed: %s\n", #sym,          \
                    dlerror());                                             \
            return 0;                                                       \
        }                                                                   \
    } while (0)

static void ff_dlopen_close(void)
{
    if (ff.fmt_h != NULL)   { dlclose(ff.fmt_h);   ff.fmt_h   = NULL; }
    if (ff.codec_h != NULL) { dlclose(ff.codec_h); ff.codec_h = NULL; }
    if (ff.util_h != NULL)  { dlclose(ff.util_h);  ff.util_h  = NULL; }
    if (ff.scale_h != NULL) { dlclose(ff.scale_h); ff.scale_h = NULL; }
}

static int ff_bind(void)
{
    if (ff.ok) {
        return 1;
    }
    ff.fmt_h = ff_dlopen_first(LEGORRR_MOVIE_LIBAVFORMAT);
    ff.codec_h = ff_dlopen_first(LEGORRR_MOVIE_LIBAVCODE);
    ff.util_h = ff_dlopen_first(LEGORRR_MOVIE_LIBAVUTIL);
    ff.scale_h = ff_dlopen_first(LEGORRR_MOVIE_LIBSWSCALE);
    if (ff.fmt_h == NULL || ff.codec_h == NULL || ff.util_h == NULL
        || ff.scale_h == NULL) {
        fprintf(stderr,
                "movie: FFmpeg runtime libs not found (dlopen probe failed on "
                "%s / %s / %s / %s) — movie playback disabled\n",
                LEGORRR_MOVIE_LIBAVFORMAT, LEGORRR_MOVIE_LIBAVCODE,
                LEGORRR_MOVIE_LIBAVUTIL, LEGORRR_MOVIE_LIBSWSCALE);
        ff_dlopen_close();
        return 0;
    }

    FF_NEED(&ff, avcodec_version, ff.codec_h);
    ff.codec_major = (int)(ff.avcodec_version() >> 16);
    if (ff.codec_major < 61) {
        fprintf(stderr, "movie: FFmpeg %d too old (need libavcodec >= 61, "
                        "FFmpeg 7+) — movie playback disabled\n",
                ff.codec_major);
        ff_dlopen_close();
        return 0;
    }

    FF_NEED(&ff, avformat_open_input, ff.fmt_h);
    FF_NEED(&ff, avformat_find_stream_info, ff.fmt_h);
    FF_NEED(&ff, avformat_close_input, ff.fmt_h);
    FF_NEED(&ff, av_read_frame, ff.fmt_h);
    FF_NEED(&ff, av_guess_frame_rate, ff.fmt_h);
    FF_NEED(&ff, avcodec_find_decoder, ff.codec_h);
    FF_NEED(&ff, avcodec_alloc_context3, ff.codec_h);
    FF_NEED(&ff, avcodec_parameters_to_context, ff.codec_h);
    FF_NEED(&ff, avcodec_open2, ff.codec_h);
    FF_NEED(&ff, avcodec_send_packet, ff.codec_h);
    FF_NEED(&ff, avcodec_receive_frame, ff.codec_h);
    FF_NEED(&ff, avcodec_free_context, ff.codec_h);
    FF_NEED(&ff, av_packet_alloc, ff.codec_h);
    FF_NEED(&ff, av_packet_free, ff.codec_h);
    FF_NEED(&ff, av_packet_unref, ff.codec_h);
    FF_NEED(&ff, av_frame_alloc, ff.codec_h);
    FF_NEED(&ff, av_frame_free, ff.codec_h);
    FF_NEED(&ff, av_frame_unref, ff.codec_h);
    FF_NEED(&ff, av_get_bytes_per_sample, ff.util_h);
    FF_NEED(&ff, av_sample_fmt_is_planar, ff.util_h);
    FF_NEED(&ff, av_get_pix_fmt, ff.util_h);
    FF_NEED(&ff, sws_getContext, ff.scale_h);
    FF_NEED(&ff, sws_scale, ff.scale_h);
    FF_NEED(&ff, sws_freeContext, ff.scale_h);

    ff.ok = 1;
    return 1;
}

int movie_available(void)
{
    return ff_bind();
}

#elif defined(LEGORRR_BACKEND_WINAPI) && !defined(LEGORRR_MOVIES_CFG_DISABLED)

/* The Windows backend uses the same documented AVIFIL32 entry points as the
 * reference executable. The developer distrobox has the AVIFIL32 import
 * library, but not vfw.h; resolve the documented exports at runtime and keep
 * AVIStream opaque. BITMAPINFOHEADER itself comes from windows.h. */
typedef HRESULT (WINAPI *movie_avi_open_fn)(void**, LPCSTR, DWORD, LONG,
                                             UINT, CLSID*);
typedef void (WINAPI *movie_avi_file_fn)(void);
typedef void* (WINAPI *movie_avi_getframe_open_fn)(void*, LPBITMAPINFOHEADER);
typedef LPVOID (WINAPI *movie_avi_getframe_fn)(void*, LONG);
typedef ULONG (WINAPI *movie_avi_release_fn)(void*);
typedef LONG (WINAPI *movie_avi_start_fn)(void*);
typedef struct movie_avi_stream_info {
    DWORD fccType;
    DWORD fccHandler;
    DWORD dwFlags;
    DWORD dwCaps;
    WORD wPriority;
    WORD wLanguage;
    DWORD dwScale;
    DWORD dwRate;
    DWORD dwStart;
    DWORD dwLength;
    DWORD dwInitialFrames;
    DWORD dwSuggestedBufferSize;
    DWORD dwQuality;
    DWORD dwSampleSize;
    RECT rcFrame;
    DWORD dwEditCount;
    DWORD dwFormatChangeCount;
    CHAR szName[64];
} movie_avi_stream_info;
typedef HRESULT (WINAPI *movie_avi_stream_info_fn)(void*,
                                                    movie_avi_stream_info*,
                                                    LONG);

typedef struct movie_winapi_binding {
    HMODULE module;
    int initialized;
    movie_avi_file_fn file_init;
    movie_avi_file_fn file_exit;
    movie_avi_open_fn stream_open;
    movie_avi_getframe_open_fn frame_open;
    movie_avi_getframe_fn get_frame;
    movie_avi_release_fn stream_release;
    movie_avi_start_fn stream_start;
    movie_avi_stream_info_fn stream_info;
} movie_winapi_binding;

static movie_winapi_binding movie_vfw;

#define MOVIE_VFW_LOAD(field, symbol, type)                                  \
    do {                                                                      \
        movie_vfw.field = (type)GetProcAddress(movie_vfw.module, symbol);     \
        if (movie_vfw.field == NULL) {                                        \
            FreeLibrary(movie_vfw.module);                                    \
            memset(&movie_vfw, 0, sizeof(movie_vfw));                         \
            return 0;                                                         \
        }                                                                     \
    } while (0)

typedef struct Movie {
    int ok;
    int width;
    int height;
    int fps_num;
    int fps_den;
    int has_pending;
    uint32_t frame_count;
    uint32_t next_frame_index;
    LONG stream_start;
    void* stream;
    void* retriever;
    char* path;
    uint8_t* rgba;
    int rgba_pitch;
} Movie;

static int movie_vfw_bind(void)
{
    if (movie_vfw.module != NULL) {
        return 1;
    }
    movie_vfw.module = LoadLibraryA("avifil32.dll");
    if (movie_vfw.module == NULL) {
        return 0;
    }
    MOVIE_VFW_LOAD(file_init, "AVIFileInit", movie_avi_file_fn);
    MOVIE_VFW_LOAD(file_exit, "AVIFileExit", movie_avi_file_fn);
    MOVIE_VFW_LOAD(stream_open, "AVIStreamOpenFromFileA",
                   movie_avi_open_fn);
    MOVIE_VFW_LOAD(frame_open, "AVIStreamGetFrameOpen",
                   movie_avi_getframe_open_fn);
    MOVIE_VFW_LOAD(get_frame, "AVIStreamGetFrame", movie_avi_getframe_fn);
    MOVIE_VFW_LOAD(stream_release, "AVIStreamRelease",
                   movie_avi_release_fn);
    MOVIE_VFW_LOAD(stream_start, "AVIStreamStart", movie_avi_start_fn);
    MOVIE_VFW_LOAD(stream_info, "AVIStreamInfoA",
                   movie_avi_stream_info_fn);
    return 1;
}

#else /* native movies disabled or unsupported backend */

int movie_available(void) { return 0; }
#endif

#if defined(LEGORRR_BACKEND_MINIWIN) && !defined(LEGORRR_MOVIES_CFG_DISABLED)
static void movie_avi_file_initialize(void)
{
    /* The native FFmpeg backend has no AVIFIL32 global lifecycle. */
}

#elif defined(LEGORRR_BACKEND_WINAPI) && !defined(LEGORRR_MOVIES_CFG_DISABLED)
/* AviFileInitialize is a separate retail startup operation. Resolving movie
 * APIs must not silently stand in for AVIFileInit: the target stores the
 * renderer's DirectDraw interface and then initializes AVIFIL32. */
static void movie_avi_file_initialize(void)
{
    if (movie_vfw_bind() && !movie_vfw.initialized) {
        movie_vfw.file_init();
        movie_vfw.initialized = 1;
    }
}

#else /* native movies disabled or unsupported backend */
static void movie_avi_file_initialize(void) { }
#endif

#if defined(LEGORRR_BACKEND_MINIWIN) && !defined(LEGORRR_MOVIES_CFG_DISABLED)
void movie_unload(void)
{
    ff_dlopen_close();
    memset(&ff, 0, sizeof(ff));
}

/* --- Movie state -------------------------------------------------------- */

struct Movie {
    int        ok;
    int        eof;              /* video stream finished (flushed too)   */
    int        v_flushed;        /* video decoder flushed at EOF           */
    int        a_flushed;        /* audio decoder flushed at EOF           */
    int        has_pending;      /* first frame decoded (info prime) and
                                    still waiting for the caller            */
    int        width, height;    /* from the first decoded frame           */
    int        fps_num, fps_den; /* from av_guess_frame_rate               */
    int        audio_rate;       /* from the audio stream codecpar         */
    uint32_t   frame_count;
    int        frame_count_known;
    uint32_t   next_frame_index;
    char*      path;

    FFFormatCtx* fmt;
    void*      vs;               /* AVStream* video (prefix-cast to read)  */
    void*      as;               /* AVStream* audio (or NULL)              */
    int        vstream;          /* packet stream index for video          */
    int        astream;          /* packet stream index for audio, or -1   */
    FFCtx*     vctx;             /* video codec context                    */
    FFCtx*     actx;             /* audio codec context (or NULL)          */
    FFPacket*  pkt;
    ff_frame*  vframe;
    ff_frame*  aframe;
    FFSws*     sws;              /* video -> RGBA converter                 */

    uint8_t*   rgba;             /* decoded frame buffer (RGBA32)          */
    int        rgba_pitch;

    int16_t*   audio;            /* accumulated interleaved stereo s16     */
    int        audio_cap;        /* capacity (frames)                       */
    int        audio_pos;        /* fill position (frames)                  */
};

static void movie_audio_reset(struct Movie* m)
{
    m->audio_pos = 0;
}

static void movie_audio_append(struct Movie* m, const int16_t* lr, int frames)
{
    if (frames <= 0) {
        return;
    }
    int need = m->audio_pos + frames;
    if (need > m->audio_cap) {
        int cap = m->audio_cap ? m->audio_cap : 2048;
        while (cap < need) {
            cap *= 2;
        }
        int16_t* nb = (int16_t*)realloc(m->audio, (size_t)cap * 2 * 2);
        if (nb == NULL) {
            return; /* drop on OOM rather than corrupt */
        }
        m->audio = nb;
        m->audio_cap = cap;
    }
    memcpy(m->audio + (size_t)m->audio_pos * 2, lr, (size_t)frames * 2 * 2);
    m->audio_pos += frames;
}

/* Normalize one decoded audio frame to interleaved s16 stereo (the seam's
 * transport). Supports the shipped formats (s16 packed mono/stereo, s16
 * planar, u8 packed mono); anything else is dropped. */
static void movie_audio_feed(struct Movie* m, ff_frame* af)
{
    int nb = af->nb_samples;
    int fmt = af->format;
    int bytes = ff.av_get_bytes_per_sample(fmt);
    if (nb <= 0 || bytes < 1) {
        return;
    }
    int planar = ff.av_sample_fmt_is_planar(fmt);

    /* Channel count, derived from the frame's buffer geometry (no struct
     * layout reads): planar has one plane per channel; packed lays
     * nb_samples*channels into data[0] so channels = linesize/(nb*bytes). */
    int planes = 0;
    for (int i = 0; i < 8; ++i) {
        if (af->data[i] != NULL) {
            planes = i + 1;
        }
    }
    int ch = planar ? planes : 0;
    if (!planar && nb > 0 && af->linesize[0] >= nb * bytes) {
        int c = af->linesize[0] / (nb * bytes);
        ch = (c >= 1 && c <= 8) ? c : planes;
    }
    if (ch < 1) {
        ch = 1;
    }
    if (ch > 2) {
        ch = 2; /* downmix to the first two channels */
    }

    if (planar && bytes == 2) {
        const int16_t* L = (const int16_t*)af->data[0];
        const int16_t* R = (const int16_t*)(planes >= 2 ? af->data[1]
                                                         : af->data[0]);
        int n = nb, off = 0;
        while (n > 0) {
            int c = n > 1024 ? 1024 : n;
            int16_t tmp[2048];
            for (int i = 0; i < c; ++i) {
                tmp[2 * i]     = L[off + i];
                tmp[2 * i + 1] = R[off + i];
            }
            movie_audio_append(m, tmp, c);
            off += c;
            n -= c;
        }
    } else if (!planar && bytes == 2 && ch == 2) {
        /* already interleaved stereo s16 — pass through */
        movie_audio_append(m, (const int16_t*)af->data[0], nb);
    } else if (!planar && bytes == 2 && ch == 1) {
        /* mono s16 -> duplicated to both channels */
        const int16_t* s = (const int16_t*)af->data[0];
        int n = nb, off = 0;
        while (n > 0) {
            int c = n > 1024 ? 1024 : n;
            int16_t tmp[2048];
            for (int i = 0; i < c; ++i) {
                tmp[2 * i] = tmp[2 * i + 1] = s[off + i];
            }
            movie_audio_append(m, tmp, c);
            off += c;
            n -= c;
        }
    } else if (!planar && bytes == 1 && ch == 1) {
        /* u8 mono -> s16 (defensive; shipped codecs are s16) */
        const uint8_t* s = (const uint8_t*)af->data[0];
        int n = nb, off = 0;
        while (n > 0) {
            int c = n > 1024 ? 1024 : n;
            int16_t tmp[2048];
            for (int i = 0; i < c; ++i) {
                int16_t v = (int16_t)(((int)s[off + i] - 128) << 8);
                tmp[2 * i] = tmp[2 * i + 1] = v;
            }
            movie_audio_append(m, tmp, c);
            off += c;
            n -= c;
        }
    }
    /* other layouts: dropped */
}

/* Produce the current video frame in m->rgba (RGBA32 at the frame's native
 * size) and fill `out` with it plus the audio FIFO. Leaves the FIFO position
 * in place (the caller resets it when it consumes the media). */
static int movie_present(struct Movie* m, MovieOut* out)
{
    ff_frame* vf = m->vframe;
    if (vf->width != m->width || vf->height != m->height) {
        if (m->sws != NULL) {
            ff.sws_freeContext(m->sws);
            m->sws = NULL;
        }
        m->width = vf->width;
        m->height = vf->height;
    }
    int w = m->width, h = m->height;
    if (w <= 0 || h <= 0) {
        return -1;
    }
    if (m->rgba == NULL) {
        m->rgba = (uint8_t*)malloc((size_t)w * h * 4);
    } else if (m->rgba_pitch != w * 4) {
        uint8_t* nb = (uint8_t*)realloc(m->rgba, (size_t)w * h * 4);
        if (nb == NULL) {
            return -1;
        }
        m->rgba = nb;
    }
    if (m->rgba == NULL) {
        return -1;
    }
    m->rgba_pitch = w * 4;

    if (m->sws == NULL) {
        /* Destination format resolved by name — the AV_PIX_FMT enum values
         * are not stable across FFmpeg versions (in 8.x AV_PIX_FMT_NV12 took
         * the slot that was RGBA in 7.x), so hard-coding a number is a real
         * hazard. av_get_pix_fmt("rgba") returns the build's value, and the
         * surface's byte order R,G,B,A matches AV_PIX_FMT_RGBA. */
        int dstfmt = ff.av_get_pix_fmt("rgba");
        if (dstfmt < 0 || vf->format < 0) {
            return -1;
        }
        m->sws = (FFSws*)ff.sws_getContext(w, h, vf->format, w, h, dstfmt,
                                           2 /* SWS_BILINEAR */, NULL, NULL,
                                           NULL);
    }
    if (m->sws != NULL) {
        uint8_t* dst[1] = { m->rgba };
        int dstp[1] = { w * 4 };
        ff.sws_scale(m->sws, (const uint8_t* const*)vf->data, vf->linesize,
                     0, h, dst, dstp);
    }

    out->rgba = m->rgba;
    out->rgba_pitch = m->rgba_pitch;
    out->audio = m->audio;
    out->audio_frames = m->audio_pos;
    out->audio_rate = m->audio_rate;
    return 0;
}

/* Drain the video decoder after av_read_frame hit EOF: send NULL, then
 * receive buffered frames one at a time. Returns 1 on a delivered frame,
 * 0 at true end, -1 on error. */
static int movie_flush_video(struct Movie* m, MovieOut* out)
{
    if (!m->v_flushed) {
        ff.avcodec_send_packet(m->vctx, NULL);
        m->v_flushed = 1;
    }
    int r = ff.avcodec_receive_frame(m->vctx, m->vframe);
    if (r == 0) {
        return movie_present(m, out) == 0 ? 1 : -1;
    }
    return 0;
}

static void movie_flush_audio(struct Movie* m)
{
    if (m->actx == NULL || m->a_flushed) {
        return;
    }
    ff.avcodec_send_packet(m->actx, NULL);
    m->a_flushed = 1;
    while (ff.avcodec_receive_frame(m->actx, m->aframe) == 0) {
        movie_audio_feed(m, m->aframe);
    }
}

/* The real decode loop: step to the next video frame, accumulating its
 * audio. Returns 0 framed (out filled), 1 end-of-stream, -1 error. */
static int movie_decode(struct Movie* m, MovieOut* out)
{
    if (m->eof) {
        return 1;
    }
    movie_audio_reset(m);
    for (;;) {
        int r = ff.av_read_frame(m->fmt, m->pkt);
        if (r < 0) {
            int vr = movie_flush_video(m, out);
            movie_flush_audio(m);
            if (vr == 1) {
                return 0;
            }
            m->eof = 1;
            return 1;
        }
        int sidx = ((ff_pkt*)m->pkt)->stream_index;
        if (sidx == m->vstream) {
            int e = ff.avcodec_send_packet(m->vctx, m->pkt);
            ff.av_frame_unref(m->vframe);
            if (e == 0 || e == FF_EAGAIN) {
                int rr = ff.avcodec_receive_frame(m->vctx, m->vframe);
                if (rr == 0) {
                    ff.av_packet_unref(m->pkt);
                    return movie_present(m, out);
                }
                /* EAGAIN (or error): read more packets before a frame
                 * emerges; a hard error still surfaces as decode failure. */
            }
            ff.av_packet_unref(m->pkt);
            if (e != 0 && e != FF_EAGAIN) {
                return -1;
            }
        } else if (m->actx != NULL && sidx == m->astream) {
            int e = ff.avcodec_send_packet(m->actx, m->pkt);
            if (e == 0 || e == FF_EAGAIN) {
                while (ff.avcodec_receive_frame(m->actx, m->aframe) == 0) {
                    movie_audio_feed(m, m->aframe);
                }
            }
            ff.av_packet_unref(m->pkt);
        } else {
            ff.av_packet_unref(m->pkt);
        }
    }
}

int movie_next(struct Movie* m, MovieOut* out)
{
    memset(out, 0, sizeof(*out));
    if (m == NULL || !m->ok) {
        return 1;
    }
    if (m->has_pending) {
        /* The prime (info) decode is still un-consumed — hand it over. */
        m->has_pending = 0;
        out->rgba = m->rgba;
        out->rgba_pitch = m->rgba_pitch;
        out->audio = m->audio;
        out->audio_frames = m->audio_pos;
        out->audio_rate = m->audio_rate;
        movie_audio_reset(m);
        ++m->next_frame_index;
        return m->eof ? 1 : 0;
    }
    {
        int result = movie_decode(m, out);
        if (result == 0) ++m->next_frame_index;
        return result;
    }
}

uint32_t movie_frame_count(Movie* m)
{
    Movie* scan;
    MovieOut frame;
    uint32_t count = 0;
    if (m == NULL || !m->ok) return 0u;
    if (m->frame_count_known) return m->frame_count;
    scan = movie_open_path(m->path);
    if (scan == NULL) return 0u;
    while (movie_next(scan, &frame) == 0) {
        if (count != 0xffffffffu) ++count;
    }
    movie_close(scan);
    m->frame_count = count;
    m->frame_count_known = 1;
    return count;
}

int movie_seek_frame(Movie* m, uint32_t frame_index)
{
    Movie* replacement;
    Movie old;
    MovieInfo info;
    MovieOut frame;
    uint32_t i;
    if (m == NULL || !m->ok || m->path == NULL) return 0;
    if (frame_index == m->next_frame_index) return 1;
    if (frame_index > m->next_frame_index &&
        (!m->frame_count_known || frame_index < m->frame_count)) {
        while (m->next_frame_index < frame_index) {
            if (movie_next(m, &frame) != 0) return 0;
        }
        return 1;
    }
    if (m->frame_count_known && frame_index >= m->frame_count) return 0;
    replacement = movie_open_path(m->path);
    if (replacement == NULL) return 0;
    if (movie_info(replacement, &info) != 0) {
        movie_close(replacement);
        return 0;
    }
    for (i = 0; i < frame_index; ++i) {
        if (movie_next(replacement, &frame) != 0) break;
    }
    if (i != frame_index) {
        movie_close(replacement);
        return 0;
    }
    replacement->frame_count = m->frame_count;
    replacement->frame_count_known = m->frame_count_known;
    old = *m;
    *m = *replacement;
    *replacement = old;
    movie_close(replacement);
    return 1;
}

Movie* movie_open_path(const char* os_path)
{
    if (!ff_bind() || os_path == NULL || os_path[0] == '\0') {
        return NULL;
    }
    FFFormatCtx* fmt = NULL;
    if (ff.avformat_open_input(&fmt, os_path, NULL, NULL) < 0) {
        fprintf(stderr, "movie: cannot open %s (FFmpeg)\n", os_path);
        return NULL;
    }
    if (ff.avformat_find_stream_info(fmt, NULL) < 0) {
        fprintf(stderr, "movie: find_stream_info failed for %s\n", os_path);
        ff.avformat_close_input(&fmt);
        return NULL;
    }

    struct Movie* m = (struct Movie*)calloc(1, sizeof(*m));
    if (m == NULL) {
        ff.avformat_close_input(&fmt);
        return NULL;
    }
    m->fmt = fmt;
    m->astream = -1;
    m->path = (char*)malloc(strlen(os_path) + 1);
    if (m->path == NULL) {
        movie_close(m);
        return NULL;
    }
    strcpy(m->path, os_path);

    /* Stream selection: walk ic->streams directly (this build's
     * av_find_best_stream returns an index — not a pointer — and skips
     * audio streams through an internal codecpar gate). Field offsets are
     * verified above in the prefix-struct comments and sanity-checked. */
    ff_fmt* f = (ff_fmt*)fmt;
    if (f->nb_streams <= 0 || f->nb_streams > 16) {
        fprintf(stderr, "movie: format context stream count %d out of range "
                        "(layout drift?)\n", f->nb_streams);
        movie_close(m);
        return NULL;
    }
    const int AVMEDIA_TYPE_VIDEO = 0, AVMEDIA_TYPE_AUDIO = 1;
    for (int i = 0; i < f->nb_streams; ++i) {
        ff_stream* st = (ff_stream*)f->streams[i];
        if (st == NULL || st->codecpar == NULL) {
            continue;
        }
        ff_codecpar* par = (ff_codecpar*)st->codecpar;
        if (par->codec_type < 0 || par->codec_type > 4 || par->codec_id <= 0) {
            continue; /* layout drift or a stream we cannot describe */
        }
        const FFCodec* dec = ff.avcodec_find_decoder(par->codec_id);
        if (par->codec_type == AVMEDIA_TYPE_VIDEO && m->vs == NULL) {
            if (dec == NULL) {
                fprintf(stderr, "movie: no decoder for video codec id 0x%x in "
                                "%s\n", par->codec_id, os_path);
                movie_close(m);
                return NULL;
            }
            m->vs = st;
            m->vstream = i; /* streams-array position == packet stream index
                             * for the demuxers we care about (AVI sets
                             * pkt->stream_index from the array position) */
            m->vctx = ff.avcodec_alloc_context3(dec);
            if (m->vctx == NULL
                || ff.avcodec_parameters_to_context(m->vctx, par) < 0
                || ff.avcodec_open2(m->vctx, dec, NULL) < 0) {
                fprintf(stderr, "movie: video codec setup failed for %s\n",
                        os_path);
                movie_close(m);
                return NULL;
            }
        } else if (par->codec_type == AVMEDIA_TYPE_AUDIO && m->as == NULL) {
            m->as = st;
            m->astream = i;
            m->audio_rate = par->sample_rate;
            if (m->audio_rate < 8000 || m->audio_rate > 192000) {
                /* layout read outside sanity — never trust it for pacing */
                m->audio_rate = 22050;
            }
            if (dec != NULL) {
                m->actx = ff.avcodec_alloc_context3(dec);
                if (m->actx != NULL) {
                    if (ff.avcodec_parameters_to_context(m->actx, par) < 0
                        || ff.avcodec_open2(m->actx, dec, NULL) < 0) {
                        ff.avcodec_free_context(&m->actx);
                        m->actx = NULL;
                        m->astream = -1;
                        m->audio_rate = 0;
                    }
                }
            }
        }
        /* keep scanning: audio (if any) comes after video in an AVI */
    }
    if (m->vs == NULL) {
        fprintf(stderr, "movie: no video stream in %s\n", os_path);
        movie_close(m);
        return NULL;
    }

    m->pkt = ff.av_packet_alloc();
    m->vframe = ff.av_frame_alloc();
    m->aframe = ff.av_frame_alloc();
    if (m->pkt == NULL || m->vframe == NULL || m->aframe == NULL) {
        movie_close(m);
        return NULL;
    }

    ff_rat fr = ff.av_guess_frame_rate(fmt, m->vs, NULL);
    m->fps_num = fr.num > 0 ? fr.num : 15;
    m->fps_den = fr.den > 0 ? fr.den : 1;

    m->ok = 1;
    return m;
}

Movie* movie_avi_retriever_open(const char* os_path)
{
    /* FFmpeg combines the target's stream/frame-retriever setup in its movie
     * decoder object. The caller still performs the metadata query as a
     * separate stage and retains this object if that query fails. */
    return movie_open_path(os_path);
}

void movie_close(struct Movie* m)
{
    if (m == NULL) {
        return;
    }
    if (m->sws != NULL) {
        ff.sws_freeContext(m->sws);
    }
    if (m->vctx != NULL) {
        ff.avcodec_free_context(&m->vctx);
    }
    if (m->actx != NULL) {
        ff.avcodec_free_context(&m->actx);
    }
    if (m->pkt != NULL) {
        ff.av_packet_free(&m->pkt);
    }
    if (m->vframe != NULL) {
        ff.av_frame_free(&m->vframe);
    }
    if (m->aframe != NULL) {
        ff.av_frame_free(&m->aframe);
    }
    if (m->fmt != NULL) {
        ff.avformat_close_input(&m->fmt);
    }
    free(m->rgba);
    free(m->audio);
    free(m->path);
    free(m);
}

int movie_info(struct Movie* m, MovieInfo* out)
{
    memset(out, 0, sizeof(*out));
    if (m == NULL || !m->ok) {
        return -1;
    }
    if (!m->has_pending && !m->eof && m->width == 0) {
        /* Prime: pull the first frame so dimensions/audio exist before the
         * caller presents anything. The result is kept for the next
         * movie_next. */
        MovieOut first;
        memset(&first, 0, sizeof(first));
        int r = movie_decode(m, &first);
        if (r == 0) {
            m->has_pending = 1;
        } else if (r == 1) {
            m->eof = 1;
        } else {
            m->ok = 0;
        }
    }
    out->width = m->width;
    out->height = m->height;
    out->fps_num = m->fps_num;
    out->fps_den = m->fps_den;
    out->audio_rate = m->audio_rate;
    out->audio_channels = m->audio_rate > 0 ? 2 : 0;
    return m->ok ? 0 : -1;
}

#elif defined(LEGORRR_BACKEND_WINAPI) && !defined(LEGORRR_MOVIES_CFG_DISABLED)
int movie_available(void)
{
    return movie_vfw_bind();
}

void movie_unload(void)
{
    if (movie_vfw.module != NULL && movie_vfw.initialized) {
        movie_vfw.file_exit();
    }
    if (movie_vfw.module != NULL) {
        FreeLibrary(movie_vfw.module);
    }
    memset(&movie_vfw, 0, sizeof(movie_vfw));
}

static int movie_winapi_read_info(Movie* m)
{
    movie_avi_stream_info info;
    HRESULT result;
    int64_t width;
    int64_t height;
    memset(&info, 0, sizeof(info));
    result = movie_vfw.stream_info(m->stream, &info, sizeof(info));
    if (result != 0) return 0;
    width = (int64_t)info.rcFrame.right - (int64_t)info.rcFrame.left;
    height = (int64_t)info.rcFrame.bottom - (int64_t)info.rcFrame.top;
    if (width <= 0 || width > 8192 || height <= 0 || height > 8192 ||
        info.dwRate > 0x7fffffffu || info.dwScale > 0x7fffffffu) return 0;
    m->width = (int)width;
    m->height = (int)height;
    m->frame_count = info.dwLength;
    m->fps_num = (int)info.dwRate;
    m->fps_den = (int)info.dwScale;
    return 1;
}

static Movie* movie_vfw_stream_retriever_open(const char* os_path)
{
    Movie* m;
    if (os_path == NULL || os_path[0] == '\0' || !movie_vfw_bind()) {
        return NULL;
    }
    movie_avi_file_initialize();
    if (!movie_vfw.initialized) return NULL;
    m = (Movie*)calloc(1, sizeof(*m));
    if (m == NULL) {
        return NULL;
    }
    if (movie_vfw.stream_open(&m->stream, os_path, 0x73646976UL, 0, 0, NULL)
        != 0 || m->stream == NULL) {
        movie_close(m);
        return NULL;
    }
    m->stream_start = movie_vfw.stream_start(m->stream);
    /* AVIGETFRAMEF_BESTDISPLAYFMT is documented as value 1; the original
     * constructor passes this same value to AVIStreamGetFrameOpen. */
    m->retriever = movie_vfw.frame_open(m->stream,
                                        (LPBITMAPINFOHEADER)1);
    if (m->retriever == NULL || !movie_winapi_read_info(m)) return m;
    m->ok = 1;
    return m;
}

Movie* movie_avi_retriever_open(const char* os_path)
{
    /* Preserve a stream acquired before the frame-retriever/info stages fail;
     * AviFrameRetrieverDestroy owns its later AVIStreamRelease. */
    return movie_vfw_stream_retriever_open(os_path);
}

Movie* movie_open_path(const char* os_path)
{
    Movie* m = movie_vfw_stream_retriever_open(os_path);
    if (m == NULL || !m->ok) {
        movie_close(m);
        return NULL;
    }
    m->path = (char*)malloc(strlen(os_path) + 1);
    if (m->path == NULL) {
        movie_close(m);
        return NULL;
    }
    strcpy(m->path, os_path);
    return m;
}

void movie_close(Movie* m)
{
    if (m == NULL) {
        return;
    }
    /* The reference AVI teardown releases the stream but never resolves or
     * calls AVIStreamGetFrameClose; retain that lifetime behavior here. */
    if (m->stream != NULL && movie_vfw.stream_release != NULL) {
        movie_vfw.stream_release(m->stream);
    }
    free(m->rgba);
    free(m->path);
    free(m);
}

int movie_info(Movie* m, MovieInfo* out)
{
    if (out == NULL) {
        return -1;
    }
    memset(out, 0, sizeof(*out));
    if (m == NULL || !m->ok) {
        return -1;
    }
    out->width = m->width;
    out->height = m->height;
    out->fps_num = m->fps_num;
    out->fps_den = m->fps_den;
    /* The reference AVIFIL32 movie object opens only the `vids` stream. */
    out->audio_rate = 0;
    out->audio_channels = 0;
    return 0;
}

static uint8_t movie_winapi_mask_component(uint32_t pixel, uint32_t mask)
{
    uint32_t maximum;
    uint32_t value;
    unsigned int source_bit;
    if (mask == 0u) return 0u;
    for (source_bit = 0u; source_bit < 32u; ++source_bit) {
        if ((mask & ((uint32_t)1u << source_bit)) != 0u) break;
    }
    maximum = mask >> source_bit;
    value = (pixel & mask) >> source_bit;
    return (uint8_t)(((uint64_t)value * 255u + maximum / 2u) / maximum);
}

static int movie_winapi_mask_contiguous(uint32_t mask)
{
    unsigned int shift;
    uint32_t shifted;
    if (mask == 0u) return 0;
    for (shift = 0u; shift < 32u; ++shift) {
        if ((mask & ((uint32_t)1u << shift)) != 0u) break;
    }
    shifted = mask >> shift;
    return (shifted & (shifted + 1u)) == 0u;
}

static int movie_winapi_dib_layout(const BITMAPINFOHEADER* header,
                                   size_t* pixel_offset,
                                   size_t* row_bytes, size_t* image_bytes,
                                   uint32_t masks[4],
                                   const BYTE** palette,
                                   unsigned int* palette_count)
{
    uint32_t compression;
    unsigned int bits;
    size_t offset;
    size_t row_bits;
    size_t calculated_image;
    uint32_t maximum_palette;
    unsigned int expected_masks = 0u;
    if (header == NULL || pixel_offset == NULL || row_bytes == NULL ||
        image_bytes == NULL || masks == NULL || palette == NULL ||
        palette_count == NULL) return 0;
    bits = (unsigned int)header->biBitCount;
    compression = (uint32_t)header->biCompression;
    if (header->biSize < sizeof(BITMAPINFOHEADER) || header->biSize > 4096u)
        return 0;
    if (bits != 1u && bits != 4u && bits != 8u && bits != 15u &&
        bits != 16u && bits != 24u && bits != 32u) return 0;
    if (compression != BI_RGB && compression != BI_BITFIELDS &&
        compression != 6u) return 0; /* BI_ALPHABITFIELDS */
    if ((compression == BI_BITFIELDS || compression == 6u) &&
        bits != 16u && bits != 32u) return 0;

    masks[0] = masks[1] = masks[2] = masks[3] = 0u;
    offset = (size_t)header->biSize;
    if (compression == BI_BITFIELDS || compression == 6u) {
        const uint32_t* dib_masks;
        expected_masks = compression == 6u ? 4u : 3u;
        if (header->biSize >= 52u) {
            if (expected_masks == 4u && header->biSize < 56u) return 0;
            dib_masks = (const uint32_t*)((const BYTE*)header + 40u);
            masks[0] = dib_masks[0];
            masks[1] = dib_masks[1];
            masks[2] = dib_masks[2];
            if (header->biSize >= 56u) masks[3] = dib_masks[3];
        }
        else {
            dib_masks = (const uint32_t*)((const BYTE*)header + offset);
            masks[0] = dib_masks[0];
            masks[1] = dib_masks[1];
            masks[2] = dib_masks[2];
            if (expected_masks == 4u) masks[3] = dib_masks[3];
            offset += (size_t)expected_masks * sizeof(uint32_t);
        }
        if (masks[0] == 0u || masks[1] == 0u || masks[2] == 0u ||
            (masks[0] & masks[1]) != 0u ||
            (masks[0] & masks[2]) != 0u ||
            (masks[1] & masks[2]) != 0u ||
            (masks[3] & (masks[0] | masks[1] | masks[2])) != 0u ||
            !movie_winapi_mask_contiguous(masks[0]) ||
            !movie_winapi_mask_contiguous(masks[1]) ||
            !movie_winapi_mask_contiguous(masks[2]) ||
            (masks[3] != 0u &&
             !movie_winapi_mask_contiguous(masks[3])) ||
            (bits == 16u && ((masks[0] | masks[1] | masks[2] |
                              masks[3]) & 0xffff0000u) != 0u))
            return 0;
    }
    else if (bits == 15u || bits == 16u) {
        /* BI_RGB 16-bit DIBs use the documented 5:5:5 layout. */
        masks[0] = 0x7c00u;
        masks[1] = 0x03e0u;
        masks[2] = 0x001fu;
    }
    else if (bits == 32u) {
        masks[0] = 0x00ff0000u;
        masks[1] = 0x0000ff00u;
        masks[2] = 0x000000ffu;
    }

    *palette = NULL;
    *palette_count = 0u;
    if (bits <= 8u) {
        maximum_palette = (uint32_t)1u << bits;
        *palette_count = header->biClrUsed != 0u
            ? header->biClrUsed : (unsigned int)maximum_palette;
        if (*palette_count > maximum_palette)
            *palette_count = (unsigned int)maximum_palette;
        if (*palette_count == 0u) return 0;
        *palette = (const BYTE*)header + offset;
        offset += (size_t)*palette_count * 4u;
    }
    else if (header->biClrUsed != 0u) {
        if (header->biClrUsed > 65536u) return 0;
        offset += (size_t)header->biClrUsed * 4u;
    }

    row_bits = (size_t)(uint32_t)header->biWidth * (size_t)bits;
    if (row_bits > ((size_t)-1) - 31u) return 0;
    *row_bytes = ((row_bits + 31u) / 32u) * 4u;
    if (*row_bytes != 0u &&
        (size_t)(header->biHeight < 0 ? -header->biHeight : header->biHeight)
            > ((size_t)-1) / *row_bytes) return 0;
    calculated_image = *row_bytes *
        (size_t)(header->biHeight < 0 ? -header->biHeight : header->biHeight);
    if (header->biSizeImage != 0u &&
        (size_t)header->biSizeImage < calculated_image) return 0;
    *pixel_offset = offset;
    *image_bytes = calculated_image;
    return 1;
}

int movie_next(Movie* m, MovieOut* out)
{
    LPVOID dib;
    BITMAPINFOHEADER* header;
    const BYTE* pixels;
    const BYTE* palette;
    uint32_t masks[4];
    unsigned int palette_count;
    unsigned int bits;
    int top_down, x, y;
    size_t row_bytes;
    size_t pixel_offset;
    size_t image_bytes;
    int64_t sample_index;
    if (out == NULL) {
        return -1;
    }
    memset(out, 0, sizeof(*out));
    if (m == NULL || !m->ok) {
        return 1;
    }
    if (m->next_frame_index >= m->frame_count) {
        return 1;
    }
    sample_index = (int64_t)m->stream_start +
        (int64_t)m->next_frame_index;
    if (sample_index < (int64_t)(-2147483647L - 1L) ||
        sample_index > 0x7fffffffL) return -1;
    dib = movie_vfw.get_frame(m->retriever, (LONG)sample_index);
    if (dib == NULL) {
        return -1;
    }
    header = (BITMAPINFOHEADER*)dib;
    if (header->biSize < sizeof(BITMAPINFOHEADER)
        || header->biSize > 4096u
        || header->biWidth != m->width || header->biHeight == 0
        || header->biHeight == (LONG)0x80000000
        || header->biHeight > 8192 || header->biHeight < -8192
        || (header->biHeight < 0 ? -header->biHeight
                                 : header->biHeight) != m->height) {
        return -1;
    }
    bits = (unsigned int)header->biBitCount;
    if (header->biWidth <= 0 || header->biWidth > 8192 ||
        m->height <= 0 || m->height > 8192 ||
        !movie_winapi_dib_layout(header, &pixel_offset, &row_bytes,
                                 &image_bytes, masks, &palette,
                                 &palette_count) ||
        row_bytes > 0x7fffffffu || pixel_offset > 0x7fffffffu ||
        image_bytes == 0u || pixel_offset > ((size_t)-1) - image_bytes) {
        return -1;
    }
    if (m->rgba == NULL) {
        size_t rgba_bytes;
        if ((size_t)m->width > ((size_t)-1) / 4u ||
            (size_t)m->height > ((size_t)-1) /
                ((size_t)m->width * 4u)) return -1;
        rgba_bytes = (size_t)m->width * (size_t)m->height * 4u;
        m->rgba = (uint8_t*)malloc(rgba_bytes);
        if (m->rgba == NULL) {
            return -1;
        }
    }
    m->rgba_pitch = m->width * 4;
    top_down = header->biHeight < 0;
    pixels = (const BYTE*)dib + pixel_offset;
    for (y = 0; y < m->height; ++y) {
        int source_y = top_down ? y : m->height - 1 - y;
        const BYTE* src = pixels + (size_t)source_y * row_bytes;
        uint8_t* dst = m->rgba + (size_t)y * m->rgba_pitch;
        for (x = 0; x < m->width; ++x) {
            unsigned r, g, b;
            unsigned a = 255u;
            if (bits <= 8u) {
                unsigned int index;
                if (bits == 8u) index = src[x];
                else if (bits == 4u)
                    index = (x & 1) == 0 ? src[x >> 1] >> 4
                                         : src[x >> 1] & 0x0fu;
                else
                    index = (src[x >> 3] >> (7u - ((unsigned int)x & 7u)))
                        & 1u;
                if (index >= palette_count) return -1;
                r = palette[index * 4u + 2u];
                g = palette[index * 4u + 1u];
                b = palette[index * 4u + 0u];
            }
            else if (bits == 24u) {
                r = src[(size_t)x * 3u + 2u];
                g = src[(size_t)x * 3u + 1u];
                b = src[(size_t)x * 3u + 0u];
            }
            else {
                uint32_t value;
                if (bits == 15u || bits == 16u) {
                    value = (uint32_t)((const WORD*)src)[x];
                }
                else {
                    value = ((const uint32_t*)src)[x];
                }
                r = movie_winapi_mask_component(value, masks[0]);
                g = movie_winapi_mask_component(value, masks[1]);
                b = movie_winapi_mask_component(value, masks[2]);
                if (masks[3] != 0u)
                    a = movie_winapi_mask_component(value, masks[3]);
            }
            dst[x * 4 + 0] = (uint8_t)r;
            dst[x * 4 + 1] = (uint8_t)g;
            dst[x * 4 + 2] = (uint8_t)b;
            dst[x * 4 + 3] = (uint8_t)a;
        }
    }
    ++m->next_frame_index;
    out->dib = (const uint8_t*)dib;
    out->dib_bits_per_pixel = (int)bits;
    out->dib_header_bytes = (int)pixel_offset;
    out->dib_palette = bits <= 8u ? palette : NULL;
    out->dib_palette_entries = bits <= 8u ? palette_count : 0u;
    out->dib_pitch = (int)row_bytes;
    out->rgba = m->rgba;
    out->rgba_pitch = m->rgba_pitch;
    return 0;
}

uint32_t movie_frame_count(Movie* m)
{
    return m != NULL && m->ok ? m->frame_count : 0u;
}

int movie_seek_frame(Movie* m, uint32_t frame_index)
{
    if (m == NULL || !m->ok || frame_index >= m->frame_count) {
        return 0;
    }
    m->next_frame_index = frame_index;
    return 1;
}

#undef MOVIE_VFW_LOAD

#else /* native movies disabled or unsupported backend */
void movie_unload(void) { }
Movie* movie_open_path(const char* os_path) { (void)os_path; return NULL; }
Movie* movie_avi_retriever_open(const char* os_path)
{ (void)os_path; return NULL; }
void movie_close(Movie* m) { (void)m; }
int movie_info(Movie* m, MovieInfo* out) { (void)m; (void)out; return -1; }
int movie_next(Movie* m, MovieOut* out) { (void)m; (void)out; return 1; }
uint32_t movie_frame_count(Movie* m) { (void)m; return 0u; }
int movie_seek_frame(Movie* m, uint32_t frame_index)
{ (void)m; (void)frame_index; return 0; }

#endif /* LEGORRR_BACKEND_MINIWIN */

// GLOBAL: LEGORR 0x00533998
static uint8_t s_avi_playback_disabled;
static AviTimeHook s_avi_time_hook;
static void* s_avi_time_context;
static AviFrameUpdateHook s_avi_frame_update_hook;
static AviFrameLoopHook s_avi_frame_loop_hook;
static void* s_avi_frame_hook_context;
static AviSurfaceLockHook s_avi_surface_lock_hook;
static void* s_avi_surface_lock_context;
static AviDisplayModeHook s_avi_display_mode_hook;
static void* s_avi_display_mode_context;
static AviReleaseHook s_avi_release_hook;
static AviDisplayReleaseHook s_avi_display_release_hook;
static void* s_avi_release_context;
// GLOBAL: LEGORR 0x0053399c
static void* s_avi_file_context;

static uint32_t avi_clock_milliseconds(void)
{
    if (s_avi_time_hook != NULL)
        return s_avi_time_hook(s_avi_time_context);
    return platform_time_milliseconds();
}

// FUNCTION: LEGORR 0x0047ec80
void avi_file_initialize(void* context)
{
    s_avi_file_context = context;
    /* Retail DAT_0076ac84 is the DirectDraw interface used by the surface
     * description path as well as the AVI display-mode code. */
    surface_description_interface_set(context);
    movie_avi_file_initialize();
}

// FUNCTION: LEGORR 0x0047ec90
AviPlaybackState* avi_player_create(const char* path)
{
    AviPlaybackState* state = (AviPlaybackState*)engine_heap_alloc(
        (long)sizeof(*state));
    DisplaySurface* render_target;
    if (state == NULL) return NULL;
    memset(state, 0, sizeof(*state));
    /* Retail allocates 0xc4 bytes before invoking the frame-retriever
     * constructor. This portable record adds backend sidecars, so use its
     * full size and zero those sidecars before the constructor fills the
     * fields modeled at retail offsets. */
    state->player_owned = 1u;
    /* The retail DirectDraw global supplies the cooperative-level owner
     * implicitly. The miniwin surface adapter needs its HWND explicitly. */
    render_target = surface_active_render_target_get();
    if (render_target != NULL) state->display_device = render_target->hwnd;
    state->frame_retriever_open(path);
    return state;
}

// FUNCTION: LEGORR 0x0047e800
void AviPlaybackState::frame_retriever_open(const char* path)
{
    Movie* movie;
    MovieInfo info;
    uint32_t now = 0;
    frame_ready = 0;
    surface_locked = 0;
    surface_pixels = NULL;
    surface_pitch = 0;
    frame_retriever_object = NULL;
    stream_object = NULL;
    surface_object = NULL;
    surface_owned = 0u;
    stream_frame_count = 0;
    frame_index = 0;
    fps_numerator = 0u;
    fps_denominator = 0u;
    frame_rate = 0.0f;
    frame_clock = 0.0f;
    frame_delta = 0.0f;
    display_width = 0u;
    display_height = 0u;
    stream_width = 0u;
    stream_height = 0u;
    if (path == NULL || s_avi_playback_disabled) return;
    movie = movie_avi_retriever_open(path);
    if (movie == NULL) return;
    /* Retail stores the AVI stream as soon as it opens. A later frame
     * retriever or AVIStreamInfo failure leaves it for the object's destructor
     * to release. The Movie wrapper owns both VFW handles on WinAPI. */
    stream_object = movie;
    if (movie_info(movie, &info) != 0) {
        return;
    }
    fps_numerator = (uint32_t)info.fps_num;
    fps_denominator = (uint32_t)info.fps_den;
    stream_width = (uint32_t)info.width;
    stream_height = (uint32_t)info.height;
    stream_frame_count = movie_frame_count(movie);
    frame_index = 0;
    now = avi_clock_milliseconds();
    frame_delta = (float)now * 0.001f;
    frame_clock = 0.0f;
    /* The reference marks the retriever ready before allocating its
     * surface.  A failed surface allocation therefore leaves a ready
     * retriever with a null surface, matching the staged constructor. */
    frame_ready = 1u;
    (void)display_mode_setup((uint32_t)info.width,
                             (uint32_t)info.height, 0u, 0);
}

void avi_frame_retriever_open(AviPlaybackState* state, const char* path)
{
    if (state != NULL) state->frame_retriever_open(path);
}

void avi_time_hook_set(AviTimeHook hook, void* context)
{
    s_avi_time_hook = hook;
    s_avi_time_context = context;
}

void avi_frame_hooks_set(AviFrameUpdateHook update,
                         AviFrameLoopHook loop, void* context)
{
    s_avi_frame_update_hook = update;
    s_avi_frame_loop_hook = loop;
    s_avi_frame_hook_context = context;
}

void avi_surface_lock_hook_set(AviSurfaceLockHook hook, void* context)
{
    s_avi_surface_lock_hook = hook;
    s_avi_surface_lock_context = context;
}

void avi_display_mode_hook_set(AviDisplayModeHook hook, void* context)
{
    s_avi_display_mode_hook = hook;
    s_avi_display_mode_context = context;
}

void avi_release_hooks_set(AviReleaseHook release_object,
                           AviDisplayReleaseHook release_display,
                           void* context)
{
    s_avi_release_hook = release_object;
    s_avi_display_release_hook = release_display;
    s_avi_release_context = context;
}

// FUNCTION: LEGORR 0x0047e510
int AviPlaybackState::display_mode_setup(uint32_t width, uint32_t height,
                                         uint32_t bits_per_pixel,
                                         int fullscreen)
{
    display_width = width != 0u ? width : display_width;
    display_height = height != 0u ? height : display_height;
    display_bits_per_pixel = bits_per_pixel;
    display_fullscreen = fullscreen != 0;
    if (s_avi_display_mode_hook == NULL) {
        DisplaySurface* surface;
        if (surface_object != NULL)
            return surface_object != NULL;
        surface = (DisplaySurface*)calloc(1, sizeof(DisplaySurface));
        if (surface == NULL) return 0;
        /* The WinAPI surface adapter builds the requested DirectDraw pixel
         * format, but creates its own DirectDraw object and does not set its
         * cooperative level. Miniwin creates an offscreen surface and uses
         * the HWND, but ignores pixel-format/fullscreen selection and the
         * retail 8-bit mode enumeration. Sharing DAT_0053399c and its full
         * display-mode behavior requires the absent renderer owner or the
         * AviDisplayModeHook. */
        if (surface_create_display_mode(surface, display_device,
                (int)width, (int)height, (int)bits_per_pixel,
                fullscreen) != 0) {
            free(surface);
            return 0;
        }
        surface_object = surface;
        display_width = (uint32_t)surface->w;
        display_height = (uint32_t)surface->h;
        surface_owned = 1u;
        /* DirectDrawDisplayModeSetup calls AviSurfaceLock followed by
         * AviFrameSurfaceUnlock after CreateSurface, even when the requested
         * dimensions were already supplied. The result is discarded; this
         * also verifies the newly-created AVI surface through the same
         * backend path used during frame updates. */
        (void)surface_lock();
        (void)surface_unlock();
        return 1;
    }
    return s_avi_display_mode_hook(s_avi_display_mode_context, this,
        width, height, bits_per_pixel, fullscreen, display_device) != 0;
}

int avi_display_mode_setup(AviPlaybackState* state, uint32_t width,
                           uint32_t height, uint32_t bits_per_pixel,
                           int fullscreen, void* display_device)
{
    if (state == NULL) return 0;
    state->display_device = display_device;
    return state->display_mode_setup(width, height, bits_per_pixel,
                                     fullscreen);
}

void avi_playback_state_initialize(AviPlaybackState* state, uint32_t width,
                                   uint32_t height,
                                   uint32_t bits_per_pixel, int fullscreen,
                                   void* surface_descriptor)
{
    if (state == NULL) return;
    memset(state, 0, sizeof(*state));
    /* The fifth retail argument is descriptor storage passed to
     * DirectDrawSurfaceDescriptorInitialize. It is not an HWND or display
     * device. Retail setup consumes the global interface bound by
     * AviFileInitialize; this host adapter still creates its backend surface
     * through the portable DisplaySurface API. */
    surface_descriptor_initialize(surface_descriptor);
    state->frame_ready = (uint8_t)state->display_mode_setup(
        width, height, bits_per_pixel, fullscreen);
}

// FUNCTION: LEGORR 0x0047e9a0
void AviPlaybackState::frame_retriever_destroy()
{
    if (s_avi_playback_disabled) return;
    if (stream_object != NULL) {
        if (s_avi_release_hook != NULL)
            s_avi_release_hook(s_avi_release_context, stream_object);
        else
            movie_close((Movie*)stream_object);
    }
    stream_object = NULL;
    frame_retriever_object = NULL;
    frame_ready = 0u;
    if (surface_object != NULL) {
        if (surface_owned != 0u) {
            surface_destroy((DisplaySurface*)surface_object);
            free(surface_object);
        }
        else if (s_avi_display_release_hook != NULL) {
            s_avi_display_release_hook(s_avi_release_context);
        }
    }
    surface_object = NULL;
    surface_owned = 0u;
    surface_locked = 0u;
    surface_pixels = NULL;
    surface_pitch = 0;
}

void avi_frame_retriever_destroy(AviPlaybackState* state)
{
    if (state != NULL) state->frame_retriever_destroy();
}

// FUNCTION: LEGORR 0x0047e980
void AviPlaybackState::player_destroy(uint8_t flags)
{
    int release_player;
    frame_retriever_destroy();
    release_player = (flags & 1u) != 0u && player_owned != 0u;
    if (release_player) engine_heap_free(this);
}

void avi_player_destroy(AviPlaybackState* state, uint8_t flags)
{
    if (state != NULL) state->player_destroy(flags);
}

// FUNCTION: LEGORR 0x0047e760
uint32_t AviPlaybackState::surface_lock()
{
    uint32_t result;
    void* pixels = NULL;
    int pitch = 0;
    int lock_result;
    if (surface_locked != 0) return 1u;
    if (s_avi_surface_lock_hook != NULL) {
        result = s_avi_surface_lock_hook(s_avi_surface_lock_context,
                                         surface_object, 1, 0x801u);
    } else {
        lock_result = ::surface_lock((DisplaySurface*)surface_object,
                                     &pixels, &pitch);
        result = lock_result == 0 ? 0u
            : (uint32_t)lock_result & 0xffffff00u;
        (void)pitch;
    }
    if (result == 0u) {
        surface_locked = 1u;
        surface_pixels = pixels;
        surface_pitch = pitch;
        return 1u;
    }
    return result;
}

// FUNCTION: LEGORR 0x0047e7b0
uint32_t AviPlaybackState::surface_unlock()
{
    uint32_t result;
    if (surface_locked == 0) return 0u;
    if (s_avi_surface_lock_hook != NULL) {
        result = s_avi_surface_lock_hook(s_avi_surface_lock_context,
                                         surface_object, 0, 0u);
    } else {
        ::surface_unlock((DisplaySurface*)surface_object, surface_pixels);
        result = 0u;
    }
    if (result == 0u) {
        surface_locked = 0u;
        surface_pixels = NULL;
        surface_pitch = 0;
        return 1u;
    }
    return result;
}

uint32_t avi_surface_lock(AviPlaybackState* state)
{
    return state != NULL ? state->surface_lock() : 0u;
}

uint32_t avi_surface_unlock(AviPlaybackState* state)
{
    return state != NULL ? state->surface_unlock() : 0u;
}

void avi_playback_mode_set(int disabled)
{
    s_avi_playback_disabled = disabled != 0;
}

// FUNCTION: LEGORR 0x0047ebe0
uint32_t AviPlaybackState::stream_frame_count_get() const
{
    if (s_avi_playback_disabled) return 0u;
    if (stream_frame_count != 0u) return stream_frame_count;
    return movie_frame_count((Movie*)stream_object);
}

uint32_t avi_stream_frame_count_get(const AviPlaybackState* state)
{
    return state != NULL ? state->stream_frame_count_get() : 0u;
}

// FUNCTION: LEGORR 0x0047ec60
uint32_t AviPlaybackState::playback_frame_index_get() const
{
    return frame_index &
        ~(uint32_t)(0u - (uint32_t)(s_avi_playback_disabled != 0));
}

uint32_t avi_playback_frame_index_get(const AviPlaybackState* state)
{
    return state != NULL ? state->playback_frame_index_get() : 0u;
}

// FUNCTION: LEGORR 0x0047ebf0
void AviPlaybackState::frame_seek_and_clock_rebase(uint32_t frame_index_value)
{
    uint32_t now;
    if (s_avi_playback_disabled) return;
    frame_index = frame_index_value;
    movie_seek_frame((Movie*)stream_object, frame_index_value);
    now = avi_clock_milliseconds();
    frame_clock = (float)now * 0.001f;
    frame_delta = frame_rate != 0.0f
        ? frame_clock - (float)frame_index / frame_rate : frame_clock;
}

void avi_frame_seek_and_clock_rebase(AviPlaybackState* state,
                                     uint32_t frame_index)
{
    if (state != NULL) state->frame_seek_and_clock_rebase(frame_index);
}

static int movie_surface_raw_dib_compatible(AviPlaybackState* state,
                                            const MovieOut* frame);
static int movie_surface_indexed_dib_copy(AviPlaybackState* state,
                                          const MovieOut* frame);

// FUNCTION: LEGORR 0x0047eb20
int AviPlaybackState::frame_update_and_blit()
{
    int frame_updated = 0;
    if (frame_ready == 0 || s_avi_playback_disabled) {
        return 0;
    }
    if (s_avi_frame_update_hook != NULL) {
        frame_updated = s_avi_frame_update_hook(s_avi_frame_hook_context,
                                                this) != 0;
    } else {
        MovieOut frame;
        Movie* movie = (Movie*)stream_object;
        if (movie == NULL) return 1;
        if (!movie_seek_frame(movie, frame_index)) return 1;
        if (movie_next(movie, &frame) != 0) {
            /* AVIStreamGetFrame returning null leaves the old surface and
             * index intact; the caller still performs its normal blit. */
            return 1;
        }
        frame_updated = 1;
        if (movie_surface_raw_dib_compatible(this, &frame))
            frame_decode_and_copy(frame.dib);
        else if (frame.dib_bits_per_pixel == 8)
            (void)movie_surface_indexed_dib_copy(this, &frame);
        else
            frame_copy_rgba(&frame);
    }
    if (frame_updated) {
        uint32_t now = avi_clock_milliseconds();
        frame_rate = fps_denominator != 0u
            ? (float)fps_numerator / (float)fps_denominator
            : 0.0f;
        frame_clock = (float)now * 0.001f;
        if (frame_rate != 0.0f) {
            float next_frame = (frame_clock - frame_delta) * frame_rate;
            if (next_frame >= 0.0f)
                frame_index = (uint32_t)next_frame;
        }
    }
    return 1;
}

uint8_t AviPlaybackState::frame_update()
{
    return (uint8_t)frame_update_and_blit();
}

int avi_frame_update_and_blit(AviPlaybackState* state)
{
    return state != NULL ? state->frame_update_and_blit() : 0;
}

static uint32_t movie_read_u32le(const uint8_t* bytes)
{
    return (uint32_t)bytes[0] | ((uint32_t)bytes[1] << 8) |
        ((uint32_t)bytes[2] << 16) | ((uint32_t)bytes[3] << 24);
}

static uint16_t movie_read_u16le(const uint8_t* bytes)
{
    return (uint16_t)((uint16_t)bytes[0] |
        ((uint16_t)bytes[1] << 8));
}

static int movie_surface_raw_dib_compatible(AviPlaybackState* state,
                                            const MovieOut* frame)
{
    DisplaySurface* surface = state != NULL
        ? (DisplaySurface*)state->surface_object : NULL;
    SurfacePixelFormat format;
    uint32_t header_size;
    uint16_t bits;
    uint32_t compression;
    int acquired = 0;
    int compatible = 0;
    if (frame == NULL || frame->dib == NULL || surface == NULL ||
        (frame->dib_bits_per_pixel != 15 &&
         frame->dib_bits_per_pixel != 16)) return 0;
    header_size = movie_read_u32le(frame->dib);
    if (header_size < 40u || header_size > 4096u) return 0;
    bits = movie_read_u16le(frame->dib + 14u);
    compression = movie_read_u32le(frame->dib + 16u);
    if ((bits != 15u && bits != 16u) || compression != 0u ||
        movie_read_u32le(frame->dib + 32u) != 0u ||
        frame->dib_header_bytes != (int)header_size) return 0;
    if (state->surface_locked == 0u) {
        if (state->surface_lock() == 0u) return 0;
        acquired = 1;
    }
    if (state->surface_pixels != NULL && state->surface_pitch > 0 &&
        frame->dib_pitch == state->surface_pitch) {
        if ((surface->format_flags & 1u) != 0u) {
            compatible = 1;
        }
        else {
            memset(&format, 0, sizeof(format));
            if (surface_directdraw_pixel_format_get(surface, &format) &&
                format.bits_per_pixel == 16u &&
                format.red_mask == 0x7c00u &&
                format.green_mask == 0x03e0u &&
                format.blue_mask == 0x001fu) compatible = 1;
        }
    }
    if (acquired) (void)state->surface_unlock();
    return compatible;
}

/* VfW returns an RGBQUAD palette beside 8-bit frame indices. Copy that table
 * into the destination's owned DirectDraw palette and copy the indexed rows
 * without expanding them. The frame memory belongs to AVIStreamGetFrame and
 * is only valid until the next retrieval, so both operations happen here. */
static int movie_surface_indexed_dib_copy(AviPlaybackState* state,
                                          const MovieOut* frame)
{
    DisplaySurface* surface = state != NULL
        ? (DisplaySurface*)state->surface_object : NULL;
    SurfacePixelFormat format;
    SurfacePaletteEntry entries[256];
    const uint8_t* source;
    uint8_t* target;
    uint32_t header_size;
    uint32_t compression;
    int32_t dib_height;
    unsigned int index;
    int width, height, x, y;
    int acquired = 0;
    if (state == NULL || frame == NULL || surface == NULL ||
        frame->dib == NULL || frame->dib_palette == NULL ||
        frame->dib_bits_per_pixel != 8 ||
        frame->dib_palette_entries == 0u ||
        frame->dib_palette_entries > 256u || frame->dib_pitch <= 0 ||
        frame->dib_header_bytes < 40 ||
        state->stream_width == 0u || state->stream_height == 0u ||
        state->stream_width > 8192u || state->stream_height > 8192u ||
        state->display_width == 0u || state->display_height == 0u ||
        state->display_width > 8192u || state->display_height > 8192u)
        return 0;
    header_size = movie_read_u32le(frame->dib);
    compression = movie_read_u32le(frame->dib + 16u);
    dib_height = (int32_t)movie_read_u32le(frame->dib + 8u);
    if (header_size < 40u || header_size > 4096u ||
        compression != 0u ||
        movie_read_u16le(frame->dib + 14u) != 8u ||
        frame->dib_header_bytes < (int)header_size ||
        (dib_height < 0 ? -(int64_t)dib_height : (int64_t)dib_height)
            != (int64_t)state->stream_height ||
        frame->dib_pitch < (int)state->stream_width ||
        surface->palette == NULL)
        return 0;
    memset(&format, 0, sizeof(format));
    if (!surface_directdraw_pixel_format_get(surface, &format) ||
        format.bits_per_pixel != 8u) return 0;
    for (index = 0u; index < frame->dib_palette_entries; ++index) {
        entries[index].red = frame->dib_palette[index * 4u + 2u];
        entries[index].green = frame->dib_palette[index * 4u + 1u];
        entries[index].blue = frame->dib_palette[index * 4u + 0u];
        entries[index].flags = 0u;
    }
    if (!surface_palette_entries_set(surface, 0u,
                                     frame->dib_palette_entries, entries))
        return 0;
    width = (int)state->display_width;
    height = (int)state->display_height;
    if (state->surface_locked == 0u) {
        if (state->surface_lock() == 0u) return 0;
        acquired = 1;
    }
    if (state->surface_pixels == NULL || state->surface_pitch < width)
        goto done;
    source = frame->dib + frame->dib_header_bytes;
    target = (uint8_t*)state->surface_pixels;
    for (y = 0; y < height; ++y) {
        uint32_t source_y = (uint32_t)(((uint64_t)(uint32_t)y *
            state->stream_height) / (uint32_t)height);
        uint32_t dib_y = dib_height < 0
            ? source_y : state->stream_height - 1u - source_y;
        const uint8_t* source_row = source +
            (size_t)dib_y * (size_t)frame->dib_pitch;
        uint8_t* target_row = target + (size_t)y *
            (size_t)state->surface_pitch;
        for (x = 0; x < width; ++x) {
            uint32_t source_x = (uint32_t)(((uint64_t)(uint32_t)x *
                state->stream_width) / (uint32_t)width);
            target_row[x] = source_row[source_x];
        }
    }
    if (acquired) (void)state->surface_unlock();
    return 1;
done:
    if (acquired) (void)state->surface_unlock();
    return 0;
}

// FUNCTION: LEGORR 0x0047e9e0
void AviPlaybackState::frame_decode_and_copy(const uint8_t* dib)
{
    DisplaySurface* surface = (DisplaySurface*)surface_object;
    int acquired = 0;
    int width = (int)stream_width;
    int height = (int)stream_height;
    int row_bytes;
    uint32_t header_bytes;
    uint32_t source_pitch;
    uint32_t surface_bits_per_pixel = 0u;
    SurfacePixelFormat surface_format;
    const uint8_t* source;
    uint8_t* target;
    int y;

    if (dib == NULL || surface == NULL) return;
    if (surface_locked == 0u) {
        if (surface_lock() == 0u) return;
        acquired = 1;
    }
    if (surface_pixels == NULL || surface_pitch <= 0) goto done;
    memset(&surface_format, 0, sizeof(surface_format));
    if ((surface->format_flags & 1u) != 0u) {
        surface_bits_per_pixel = 15u;
    }
    else if (surface_directdraw_pixel_format_get(surface,
                                                  &surface_format)) {
        surface_bits_per_pixel = surface_format.bits_per_pixel;
    }
    if (surface_bits_per_pixel != 15u && surface_bits_per_pixel != 16u)
        goto done;
    if (width <= 0 || height <= 0) goto done;
    header_bytes = (uint32_t)dib[0] | ((uint32_t)dib[1] << 8)
        | ((uint32_t)dib[2] << 16) | ((uint32_t)dib[3] << 24);
    row_bytes = width * 2;
    if (header_bytes < 40u || surface_pitch < row_bytes) goto done;
    /* Retail steps the frame source using the locked DirectDraw pitch, just
     * as it steps the destination. AVIStreamGetFrame's selected output is
     * expected to have that same row stride. */
    source_pitch = (uint32_t)surface_pitch;
    source = dib + header_bytes + (size_t)(height - 1) * source_pitch;
    target = (uint8_t*)surface_pixels;
    for (y = 0; y < height; ++y) {
        memcpy(target, source, (size_t)row_bytes);
        target += surface_pitch;
        source -= source_pitch;
    }
done:
    if (acquired) (void)surface_unlock();
}

void AviPlaybackState::frame_copy_rgba(const MovieOut* frame)
{
    DisplaySurface* surface = (DisplaySurface*)surface_object;
    int acquired = 0;
    int width = (int)display_width;
    int height = (int)display_height;
    uint32_t bits_per_pixel;
    uint32_t red_mask;
    uint32_t green_mask;
    uint32_t blue_mask;
    uint32_t bytes_per_pixel;
    SurfacePixelFormat format;
    int x, y;
    if (frame == NULL || frame->rgba == NULL || surface == NULL
        || width <= 0 || height <= 0 || stream_width == 0u ||
        stream_width > 0x7fffffffu / 4u || stream_height == 0u ||
        frame->rgba_pitch < (int)(stream_width * 4u))
        return;
    if (surface_locked == 0u) {
        if (surface_lock() == 0u) return;
        acquired = 1;
    }
    memset(&format, 0, sizeof(format));
    if ((surface->format_flags & 1u) != 0u) {
        bits_per_pixel = 15u;
        red_mask = 0x7c00u;
        green_mask = 0x03e0u;
        blue_mask = 0x001fu;
    }
    else if (surface_directdraw_pixel_format_get(surface, &format)) {
        bits_per_pixel = format.bits_per_pixel;
        red_mask = format.red_mask;
        green_mask = format.green_mask;
        blue_mask = format.blue_mask;
    }
    else goto done;
    if (bits_per_pixel != 15u && bits_per_pixel != 16u &&
        bits_per_pixel != 24u && bits_per_pixel != 32u) goto done;
    bytes_per_pixel = (bits_per_pixel + 7u) / 8u;
    if (width > 0x7fffffff / (int)bytes_per_pixel ||
        surface_pixels == NULL || surface_pitch < width * (int)bytes_per_pixel)
        goto done;
    if (bits_per_pixel == 24u || bits_per_pixel == 32u) {
        if (red_mask == 0u) red_mask = 0x00ff0000u;
        if (green_mask == 0u) green_mask = 0x0000ff00u;
        if (blue_mask == 0u) blue_mask = 0x000000ffu;
    }
    for (y = 0; y < height; ++y) {
        uint32_t source_y = (uint32_t)(((uint64_t)(uint32_t)y *
            stream_height) / (uint32_t)height);
        const uint8_t* source = frame->rgba +
            (size_t)source_y * (size_t)frame->rgba_pitch;
        uint8_t* target = (uint8_t*)surface_pixels +
            (size_t)y * (size_t)surface_pitch;
        for (x = 0; x < width; ++x) {
            uint32_t source_x = (uint32_t)(((uint64_t)(uint32_t)x *
                stream_width) / (uint32_t)width);
            const uint8_t* rgba = source + (size_t)source_x * 4u;
            uint32_t packed;
            unsigned int bit;
            uint32_t masks[3];
            unsigned int channel;
            masks[0] = red_mask;
            masks[1] = green_mask;
            masks[2] = blue_mask;
            packed = 0u;
            for (channel = 0u; channel < 3u; ++channel) {
                uint32_t mask = masks[channel];
                uint32_t maximum;
                uint32_t value;
                if (mask == 0u) continue;
                for (bit = 0u; bit < 32u; ++bit) {
                    if ((mask & ((uint32_t)1u << bit)) != 0u) break;
                }
                maximum = mask >> bit;
                value = (uint32_t)(((uint64_t)rgba[channel] * maximum +
                                    127u) / 255u);
                packed |= (value << bit) & mask;
            }
            target[(size_t)x * bytes_per_pixel + 0u] =
                (uint8_t)(packed & 0xffu);
            if (bytes_per_pixel > 1u)
                target[(size_t)x * bytes_per_pixel + 1u] =
                    (uint8_t)((packed >> 8) & 0xffu);
            if (bytes_per_pixel > 2u)
                target[(size_t)x * bytes_per_pixel + 2u] =
                    (uint8_t)((packed >> 16) & 0xffu);
            if (bytes_per_pixel > 3u)
                target[(size_t)x * bytes_per_pixel + 3u] =
                    (uint8_t)((packed >> 24) & 0xffu);
        }
    }
done:
    if (acquired) (void)surface_unlock();
}

/* Portable frame blit adapter. Retail 0x0047ed00 reads its source surface
 * from a renderer-owned player record at +0xc0/+0x18; AviPlaybackState keeps
 * its portable DisplaySurface pointer in a host sidecar instead. */
// FUNCTION: LEGORR 0x0047ed00
void avi_playback_frame_loop(AviPlaybackState* state, void* destination)
{
    if (state == NULL || state->frame_ready == 0) {
        return;
    }
    if (s_avi_frame_loop_hook != NULL) {
        s_avi_frame_loop_hook(s_avi_frame_hook_context, state, destination);
    } else if (destination != NULL && state->surface_object != NULL) {
        surface_blit_wait(destination, (DisplaySurface*)state->surface_object);
    }
}

/* ------------------------------------------------------------------ *
 *  Platform-neutral helpers: pixels, config surface, filesystem.
 *  (compiled on every backend)
 * ------------------------------------------------------------------ */

void movie_scale_rgba(const uint8_t* src, int sw, int sh, int spitch,
                      uint8_t* dst, int dw, int dh, int dpitch)
{
    if (src == NULL || dst == NULL || sw <= 0 || sh <= 0 || dw <= 0
        || dh <= 0 || dw > 0x7fffffff / 4 || sw > 0x7fffffff / 4
        || dpitch < dw * 4 || spitch < sw * 4) {
        return;
    }
    for (int y = 0; y < dh; ++y) {
        int sy = (int)(((int64_t)y * sh) / dh);
        if (sy >= sh) {
            sy = sh - 1;
        }
        const uint8_t* s = src + (size_t)sy * spitch;
        uint8_t* d = dst + (size_t)y * dpitch;
        for (int x = 0; x < dw; ++x) {
            int sx = (int)(((int64_t)x * sw) / dw);
            if (sx >= sw) {
                sx = sw - 1;
            }
            const uint8_t* p = s + (size_t)sx * 4;
            d[x * 4 + 0] = p[0];
            d[x * 4 + 1] = p[1];
            d[x * 4 + 2] = p[2];
            d[x * 4 + 3] = p[3];
        }
    }
}

int avi_frame_decode_and_copy(Movie* movie, uint8_t* destination,
                              int width, int height, int pitch)
{
    MovieOut frame;
    MovieInfo info;
    int result;
    if (movie == NULL || destination == NULL || width <= 0 || height <= 0
        || width > 0x7fffffff / 4 || pitch < width * 4) {
        return -1;
    }
    result = movie_next(movie, &frame);
    if (result != 0) {
        return result;
    }
    if (movie_info(movie, &info) != 0) {
        return -1;
    }
    if (frame.rgba == NULL) {
        return -1;
    }
    movie_scale_rgba(frame.rgba, info.width, info.height, frame.rgba_pitch,
                     destination, width, height, pitch);
    return 0;
}

/* ------------------------------------------------------------------ *
 *  Rect-aware video pump (port issue #303; DirectShowVideoPlaybackLoop
 *  @ 0x42efc0). See movie.hpp for the corpus mapping and the deviation
 *  note (finding 206: the corpus's own Blit ignores the rect).
 * ------------------------------------------------------------------ */

void movie_scale_rgba_rect(const uint8_t* src, int sw, int sh, int spitch,
                           uint8_t* dst, int dw, int dh, int dpitch,
                           const MovieRect* rect)
{
    if (src == NULL || dst == NULL || rect == NULL || sw <= 0 || sh <= 0
        || dw <= 0 || dh <= 0 || dpitch < dw * 4 || spitch < sw * 4
        || rect->w <= 0 || rect->h <= 0) {
        return;
    }
    int x0 = rect->x > 0 ? rect->x : 0;
    int y0 = rect->y > 0 ? rect->y : 0;
    int x1 = rect->x + rect->w < dw ? rect->x + rect->w : dw;
    int y1 = rect->y + rect->h < dh ? rect->y + rect->h : dh;
    for (int y = y0; y < y1; ++y) {
        int sy = (int)(((int64_t)(y - rect->y) * sh) / rect->h);
        if (sy >= sh) {
            sy = sh - 1;
        }
        const uint8_t* s = src + (size_t)sy * spitch;
        uint8_t* d = dst + (size_t)y * dpitch;
        for (int x = x0; x < x1; ++x) {
            int sx = (int)(((int64_t)(x - rect->x) * sw) / rect->w);
            if (sx >= sw) {
                sx = sw - 1;
            }
            const uint8_t* p = s + (size_t)sx * 4;
            d[x * 4 + 0] = p[0];
            d[x * 4 + 1] = p[1];
            d[x * 4 + 2] = p[2];
            d[x * 4 + 3] = p[3];
        }
    }
}

/* --- case-insensitive loose-file resolution ---------------------------- */

#if defined(LEGORRR_BACKEND_MINIWIN)

static int m_ci_eq(const char* a, const char* b)
{
    return strcasecmp_lite(a, b) == 0;
}

/* Resolve `name` under directory `dir`: exact-case path first (stat), else
 * a case-insensitive readdir match. Writes the resolving path into out (1)
 * or leaves out untouched (0). */
static int m_dir_find(const char* dir, const char* name, char* out, size_t sz)
{
    if (dir[0] == '\0') {
        snprintf(out, sz, "%s", name);
    } else {
        snprintf(out, sz, "%s/%s", dir, name);
    }
    struct stat st;
    if (stat(out, &st) == 0) {
        return 1;
    }
    DIR* d = opendir(dir);
    if (d == NULL) {
        return 0;
    }
    int found = 0;
    struct dirent* e;
    while ((e = readdir(d)) != NULL) {
        if (m_ci_eq(e->d_name, name)) {
            snprintf(out, sz, "%s/%s", dir, e->d_name);
            found = 1;
            break;
        }
    }
    closedir(d);
    return found;
}

/* Resolve the remaining '/'-separated components of `rel` under `dir`,
 * matching each directory component case-insensitively (the shipped files
 * live in `Data/AVI/` — uppercase — while the config values are lowercase). */
static int movie_path_resolve_tail(const char* dir, const char* rel,
                                   char* out, size_t sz)
{
    char cur[512];
    snprintf(cur, sizeof(cur), "%s", dir);
    const char* p = rel;
    while (*p == '/') {
        ++p;
    }
    while (*p != '\0') {
        const char* slash = strchr(p, '/');
        char comp[128];
        size_t n = slash ? (size_t)(slash - p) : strlen(p);
        if (n >= sizeof(comp)) {
            n = sizeof(comp) - 1;
        }
        memcpy(comp, p, n);
        comp[n] = '\0';
        char nxt[512];
        if (!m_dir_find(cur, comp, nxt, sizeof(nxt))) {
            return 0;
        }
        snprintf(cur, sizeof(cur), "%s", nxt);
        p = slash ? slash + 1 : p + n;
    }
    if (snprintf(out, sz, "%s", cur) >= (int)sz) {
        return 0;
    }
    return 1;
}

int movie_path_resolve(const char* base_dir, const char* rel,
                       char* out, size_t sz)
{
    if (base_dir == NULL) {
        base_dir = ".";
    }
    if (rel == NULL || rel[0] == '\0' || out == NULL || sz == 0) {
        return 0;
    }
    /* The config value names the file under the data dir without a "Data"
     * prefix (`avi\intro.avi`), so try <base>/Data/<rel> first, then the
     * bare <base>/<rel> (a direct run from the data dir). */
    char ddir[512];
    snprintf(ddir, sizeof(ddir), "%s/Data", base_dir);
    if (m_dir_find(base_dir, "Data", ddir, sizeof(ddir))) {
        char cand[512];
        if (movie_path_resolve_tail(ddir, rel, cand, sizeof(cand))) {
            snprintf(out, sz, "%s", cand);
            return 1;
        }
    }
    return movie_path_resolve_tail(base_dir, rel, out, sz);
}

#else /* WinAPI movie runtime; playback backend remains unavailable. */

static int movie_winapi_path_build(char* out, size_t capacity,
                                   const char* base, const char* subdir,
                                   const char* rel)
{
    size_t used = 0;
    size_t i;
    const char* parts[3];
    parts[0] = base;
    parts[1] = subdir;
    parts[2] = rel;
    for (i = 0; i < 3; ++i) {
        const char* part = parts[i];
        size_t j;
        if (part == NULL || part[0] == '\0') continue;
        if (used != 0 && out[used - 1] != '\\' && out[used - 1] != '/') {
            if (used + 1 >= capacity) return 0;
            out[used++] = '\\';
        }
        if (i == 2) {
            while (*part == '\\' || *part == '/') ++part;
        }
        for (j = 0; part[j] != '\0'; ++j) {
            char ch = part[j];
            if (used + 1 >= capacity) return 0;
            out[used++] = (ch == '/') ? '\\' : ch;
        }
    }
    if (capacity == 0 || used >= capacity) return 0;
    out[used] = '\0';
    return 1;
}

static int movie_winapi_file_exists(const char* path)
{
    DWORD attributes = GetFileAttributesA(path);
    return attributes != (DWORD)-1 &&
        (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

int movie_path_resolve(const char* base_dir, const char* rel,
                       char* out, size_t sz)
{
    char candidate[1024];
    if (rel == NULL || rel[0] == '\0' || out == NULL || sz == 0)
        return 0;
    if (base_dir == NULL || base_dir[0] == '\0') base_dir = ".";

    /* The config path is relative to Data first, then to the selected data
     * directory itself. Windows resolves component case without the native
     * backend's explicit directory scan. */
    if (movie_winapi_path_build(candidate, sizeof(candidate), base_dir,
                                "Data", rel) &&
        movie_winapi_file_exists(candidate)) {
        size_t length = strlen(candidate);
        if (length >= sz) return 0;
        memcpy(out, candidate, length + 1);
        return 1;
    }
    if (!movie_winapi_path_build(candidate, sizeof(candidate), base_dir,
                                 NULL, rel) ||
        !movie_winapi_file_exists(candidate)) return 0;
    {
        size_t length = strlen(candidate);
        if (length >= sz) return 0;
        memcpy(out, candidate, length + 1);
    }
    return 1;
}

#endif /* LEGORRR_BACKEND_MINIWIN */


namespace unported_ledger_stubs {
// FUNCTION: LEGORR 0x0047e4c0
void ledger_stub_0047e4c0_AviPlaybackStateInitialize(
    AviPlaybackState* state, uint32_t width, uint32_t height,
    uint32_t bits_per_pixel, int fullscreen, void* surface_descriptor)
{
    avi_playback_state_initialize(state, width, height, bits_per_pixel,
                                  fullscreen, surface_descriptor);
}

int ledger_stub_0047e510_DirectDrawDisplayModeSetup(
    AviPlaybackState* state, uint32_t width, uint32_t height,
    uint32_t bits_per_pixel, int fullscreen)
{
    return state != NULL ? state->display_mode_setup(width, height,
        bits_per_pixel, fullscreen) : 0;
}

uint32_t ledger_stub_0047e760_AviSurfaceLock(AviPlaybackState* state)
{
    return avi_surface_lock(state);
}

uint32_t ledger_stub_0047e7b0_AviFrameSurfaceUnlock(AviPlaybackState* state)
{
    return avi_surface_unlock(state);
}

void ledger_stub_0047e980_AviPlayerDestroy(AviPlaybackState* state,
                                            uint8_t flags)
{
    avi_player_destroy(state, flags);
}

void ledger_stub_0047e9a0_AviFrameRetrieverDestroy(AviPlaybackState* state)
{
    avi_frame_retriever_destroy(state);
}

void ledger_stub_0047e9e0_AviFrameDecodeAndCopy(
    AviPlaybackState* state, const uint8_t* dib)
{
    if (state != NULL) state->frame_decode_and_copy(dib);
}

int ledger_stub_0047eb20_AviFrameUpdateAndBlit(AviPlaybackState* state)
{
    return avi_frame_update_and_blit(state);
}

uint32_t ledger_stub_0047ebe0_AviStreamFrameCountGet(
    const AviPlaybackState* state)
{
    return avi_stream_frame_count_get(state);
}

struct AviFrameSeekRebaseMethod {
    void seek(uint32_t frame_index);
};

void AviFrameSeekRebaseMethod::seek(uint32_t frame_index)
{
    avi_frame_seek_and_clock_rebase((AviPlaybackState*)this, frame_index);
}

uint32_t ledger_stub_0047ec60_AviPlaybackFrameIndexGet(
    const AviPlaybackState* state)
{
    return avi_playback_frame_index_get(state);
}

void ledger_stub_0047ec80_AviFileInitialize(void* context)
{
    avi_file_initialize(context);
}

void ledger_stub_0047e800_AviFrameRetrieverOpen(AviPlaybackState* state,
                                                 const char* path)
{
    avi_frame_retriever_open(state, path);
}

// FUNCTION: LEGORR 0x0047ecf0
uint8_t ledger_stub_0047ecf0_AviFrameUpdate(AviPlaybackState* state)
{
    return (uint8_t)state->frame_update_and_blit();
}

void ledger_stub_0047ed00_AviPlaybackFrameLoop(AviPlaybackState* state,
                                                void* destination)
{
    avi_playback_frame_loop(state, destination);
}

// FUNCTION: LEGORR 0x0047ed40
void ledger_stub_0047ed40_AviFrameIndexAndClockReset(
    AviPlaybackState* state, uint32_t frame_index)
{
    ((AviFrameSeekRebaseMethod*)state)->seek(frame_index);
}

// FUNCTION: LEGORR 0x0047ed50
uint32_t ledger_stub_0047ed50_AviCurrentFrameIndexGet(
    const AviPlaybackState* state)
{
    return state->playback_frame_index_get();
}

// FUNCTION: LEGORR 0x0047ed60
uint32_t ledger_stub_0047ed60_AviStreamFrameCountGet(
    const AviPlaybackState* state)
{
    return state->stream_frame_count_get();
}


} // namespace unported_ledger_stubs
