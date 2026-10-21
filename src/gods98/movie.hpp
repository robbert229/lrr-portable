// SPDX-License-Identifier: LGPL-3.0-or-later
/* gods98 engine — AVI movie module (finding 14 §4, finding 23 callbacks).
 *
 * Software AVI playback for the native port. The corpus's path is
 * AVIFIL32 — AviStreamModule @ 0x47e800 / MovieOpen @ 0x47ec90, whose
 * AVIStreamGetFrame returns DIB bytes blitted straight to the surface.
 * The modern analog used here: FFmpeg's in-process decoder (dlopen'd at
 * runtime) turns each frame into RGBA32 pixels and hands the audio track to
 * the platform PCM seam.
 *
 * Corpus surface reconstructed (finding 23):
 *
 *   AvisEnabled           @ 0x414ce0  — `Lego* Main DontPlayAvis` gate
 *   MoviePlayNamed        @ 0x415910  — play a config-key movie (boot path)
 *   MoviePlayLoop         @ 0x4157c0  — play to completion / up to ESC
 *   MoviePlayLevelVideo   @ 0x415990  — the level's `Video` config key
 *
 * Config keys (finding 14 §4, shipped Lego.cfg): `RRAvi` = avi\intro.avi,
 * `LegoAvi` = avi\lmi640.avi, per-level `Video` = Avi\*.avi, and the
 * `DontPlayAvis` master kill switch. The movies themselves are *loose* files
 * under `Data/AVI/` (never inside the wads).
 *
 * Dependency discipline (ADR 0003/0004): the build declares **no** ffmpeg
 * link dependency and includes **no** ffmpeg headers. `movie_available()`
 * dlopens the versioned runtime shared libraries (libavformat/libavcodec/
 * libavutil/libswscale) on first use and dlsym's the ~26-symbol subset the
 * decoder needs; every library struct is an opaque handle and only the
 * version-stable AVFrame prefix (data/linesize/width/height/nb_samples/
 * format) is ever dereferenced. Decode is thus a runtime capability, not a
 * build requirement: builds succeed everywhere, and machines without the
 * libs get a logged "movies unavailable" instead of playback. Hooking a GPL
 * ffmpeg at runtime (dlopen, no linking) also keeps this LGPL build
 * GPL-clean.
 *
 * The winapi reference build is real AVIFIL32 on Windows; this module is the
 * native backend's decode path and compiles to stubs there (like surface.*).
 */
#ifndef LEGORRR_MOVIE_HPP
#define LEGORRR_MOVIE_HPP

#include <stddef.h>
#include <stdint.h>

#include "block.hpp"

#ifdef __cplusplus
extern "C" {
#endif

/* --- capability probe ---------------------------------------------------- */

/* 1 when the FFmpeg runtime libs resolved and their API version is
 * supported (libavcodec >= 61, i.e. FFmpeg 7+ — the sample-rate layout the
 * decode reads). Idempotent; logs the failure reason to stderr. */
int movie_available(void);

/* Release the cached dlopen handles (used between test runs to prove the
 * probe re-runs; the frame loop never calls this). */
void movie_unload(void);

/* --- decode -------------------------------------------------------------- */

typedef struct Movie Movie;

/* Open an AVI by filesystem path. Decodes nothing yet; the first frame is
 * pulled (and cached) by the first movie_info/movie_next call so information
 * is complete before presentation. Returns NULL on open/codec failure. */
Movie* movie_open_path(const char* os_path);

void movie_close(Movie* m);

typedef struct MovieInfo {
    int width, height;    /* video frame size (pixels)            */
    int fps_num, fps_den; /* frames per second (rational)         */
    int audio_rate;      /* Hz, 0 = no audio track               */
    int audio_channels;  /* always 2: the track is normalized    */
} MovieInfo;

/* Fills info (pulling the first frame if needed). 0 on success. */
int movie_info(Movie* m, MovieInfo* out);

typedef struct MovieOut {
    /* RGBA32 (byte order R,G,B,A on little-endian) — width*height rows of
     * rgba_pitch bytes. Owned by the module; valid until the next
     * movie_next. */
    const uint8_t* rgba;
    int            rgba_pitch;
    /* Interleaved stereo s16 PCM for all audio samples that fall on or
     * before this frame's presentation time — the caller feeds it to the
     * audio seam per video frame. Owned; valid until the next movie_next. */
    const int16_t* audio;
    int            audio_frames;
    int            audio_rate;
} MovieOut;

/* Decode one video frame's worth of media. Returns 0 on a frame, 1 at
 * end-of-stream (the FIFO drained), -1 on a decode error. */
int movie_next(Movie* m, MovieOut* out);

/* --- pixels -------------------------------------------------------------- */

/* Nearest-neighbour scale-fill of an RGBA32 frame into a caller buffer
 * (the corner's blit of AVIStreamGetFrame DIB bytes to the 640×480
 * surface). Pure pixel math, testable without a decoder. */
void movie_scale_rgba(const uint8_t* src, int sw, int sh, int spitch,
                      uint8_t* dst, int dw, int dh, int dpitch);

/* --- config surface (finding 23 callbacks) ------------------------------ */

/* AvisEnabled @ 0x414ce0: play movies when `Lego* Main DontPlayAvis` is not
 * TRUE (missing/FALSE -> enabled). */
int  movie_avis_enabled(BlockFile* cfg);

/* MoviePlayNamed: read a `Lego* Main <key>` movie path and normalize its
 * backslashes to '/'. Returns 1 and fills out (<= sz) when the key has a
 * non-empty value. */
int  movie_key_path(BlockFile* cfg, const char* key, char* out, size_t sz);

/* MoviePlayLevelVideo: the `<level> Video` config value through the shipped
 * `Levels` container (falling back to the flat shape). */
int  movie_level_video_path(BlockFile* cfg, const char* level,
                            char* out, size_t sz);

/* --- filesystem ---------------------------------------------------------- */

/* Resolve a config path (`avi\intro.avi`, already '/' separated after
 * normalization) against the loose data tree to an existing file. Tries
 * `<base>/Data/<rel>` (the shipped layout; the config spells the dir without
 * a "Data" prefix) then `<base>/<rel>`, matching directory components
 * case-insensitively (the shipped files live in `Data/AVI/` — uppercase —
 * while the config values are lowercase). 1 + out filled on success. */
int movie_path_resolve(const char* base_dir, const char* rel,
                       char* out, size_t sz);

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_MOVIE_HPP */
