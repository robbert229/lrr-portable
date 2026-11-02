// SPDX-License-Identifier: LGPL-3.0-or-later
/* gods98 engine — AVI movie module (finding 14 §4, finding 23 callbacks).
 *
 * AVI playback follows the corpus's AVIFIL32 path on Windows —
 * AviStreamModule @ 0x47e800 / MovieOpen @ 0x47ec90. VfW returns a DIB for
 * each frame; supported RGB DIBs are decoded to RGBA for the portable API,
 * while the original DIB remains available to the DirectDraw copy path. The
 * native backend uses FFmpeg, loaded dynamically, to produce RGBA32 pixels
 * and pass decoded audio to the platform PCM seam.
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
 * Dependency discipline (ADR 0003/0004): the native build declares **no** ffmpeg
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
 * The Windows backend dynamically resolves documented AVIFIL32 entry points
 * from avifil32.dll; it needs no VfW import library.
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

/* 1 when the configured playback API is available: supported FFmpeg runtime
 * libraries on native, or the required AVIFIL32 entry points on Windows. */
int movie_available(void);

/* Release the cached dlopen handles (used between test runs to prove the
 * probe re-runs; the frame loop never calls this). */
void movie_unload(void);

/* --- decode -------------------------------------------------------------- */

typedef struct Movie Movie;
struct MovieOut;

/* Open an AVI by filesystem path. Native playback decodes the first frame on
 * the first movie_info/movie_next call; Windows reads stream metadata at open
 * and pulls frames on movie_next. Returns NULL on open/decoder failure. */
Movie* movie_open_path(const char* os_path);
/* AVI constructor path: return the stream owner after stream acquisition even
 * when the later frame-retriever or stream-info stage fails. */
Movie* movie_avi_retriever_open(const char* os_path);

void movie_close(Movie* m);

typedef struct MovieInfo {
    int width, height;    /* video frame size (pixels)            */
    int fps_num, fps_den; /* frames per second (rational)         */
    int audio_rate;      /* Hz, 0 = no audio track               */
    int audio_channels;  /* always 2: the track is normalized    */
} MovieInfo;

/* Logical view of the AVI playback object fields read by the frame-count
 * and playback-index helpers (reference offsets +0x2c and +0xb8). */
typedef struct AviPlaybackState {
    unsigned char reserved_00[8];
    uint8_t surface_locked;
    unsigned char reserved_09[0x17];
    uint32_t fps_numerator;
    uint32_t fps_denominator;
    uint32_t reserved_28;
    uint32_t stream_frame_count;
    unsigned char reserved_30[0x69];
    uint8_t frame_ready;
    unsigned char reserved_9a[0x12];
    float frame_delta;
    float frame_clock;
    float frame_rate;
    uint32_t frame_index;
    void* surface_object; /* host-width sidecar for movie display surface */
    uint32_t display_width;
    uint32_t display_height;
    uint32_t stream_width;  /* source rectangle used by AviFrameDecodeAndCopy */
    uint32_t stream_height;
    uint32_t display_bits_per_pixel;
    int display_fullscreen;
    void* display_device;
    void* surface_pixels; /* host-width lock result sidecar */
    int surface_pitch;    /* host-width row pitch paired with surface_pixels */
    uint8_t surface_owned;
    uint8_t player_owned; /* set only by avi_player_create */
    void* stream_object;
    void* frame_retriever_object;
    /* Target thiscall entry points use the player object in ECX on x86. */
    void frame_retriever_open(const char* path);
    /* Host adapter for target 0x47e510. Retail ECX is a separately
     * allocated 0x98-byte DirectDraw-state subobject; the portable state
     * keeps its surface and descriptor data in this record's sidecars. */
    int display_mode_setup(uint32_t width, uint32_t height,
                           uint32_t bits_per_pixel, int fullscreen);
    uint32_t surface_lock();
    uint32_t surface_unlock();
    uint32_t stream_frame_count_get() const;
    uint32_t playback_frame_index_get() const;
    void frame_seek_and_clock_rebase(uint32_t frame_index);
    int frame_update_and_blit();
    void frame_decode_and_copy(const uint8_t* dib);
    void frame_copy_rgba(const struct MovieOut* frame);
    uint8_t frame_update();
    void frame_retriever_destroy();
    void player_destroy(uint8_t flags);
} AviPlaybackState;
typedef uint32_t (*AviTimeHook)(void* context);
typedef int (*AviFrameUpdateHook)(void* context, AviPlaybackState* state);
typedef void (*AviFrameLoopHook)(void* context, AviPlaybackState* state,
                                 void* destination);
typedef uint32_t (*AviSurfaceLockHook)(void* context, void* surface,
                                       int lock, uint32_t flags);
typedef int (*AviDisplayModeHook)(void* context, AviPlaybackState* state,
                                  uint32_t width, uint32_t height,
                                  uint32_t bits_per_pixel,
                                  int fullscreen, void* display_device);
typedef void (*AviReleaseHook)(void* context, void* object);
typedef void (*AviDisplayReleaseHook)(void* context);
void avi_time_hook_set(AviTimeHook hook, void* context);
void avi_frame_hooks_set(AviFrameUpdateHook update,
                         AviFrameLoopHook loop, void* context);
void avi_surface_lock_hook_set(AviSurfaceLockHook hook, void* context);
void avi_display_mode_hook_set(AviDisplayModeHook hook, void* context);
void avi_release_hooks_set(AviReleaseHook release_object,
                           AviDisplayReleaseHook release_display,
                           void* context);
void avi_file_initialize(void* context); /* 0x0047ec80 */
/* Creates the player record and opens its frame retriever. */
AviPlaybackState* avi_player_create(const char* path); /* 0x0047ec90 */
int avi_frame_decode_and_copy(Movie* movie, uint8_t* destination,
                              int width, int height, int pitch);
void avi_frame_retriever_open(AviPlaybackState* state, const char* path);
int avi_display_mode_setup(AviPlaybackState* state, uint32_t width,
                           uint32_t height, uint32_t bits_per_pixel,
                           int fullscreen, void* display_device);
void avi_playback_state_initialize(AviPlaybackState* state, uint32_t width,
                                   uint32_t height,
                                   uint32_t bits_per_pixel, int fullscreen,
                                   void* surface_descriptor);
void avi_frame_retriever_destroy(AviPlaybackState* state);
void avi_player_destroy(AviPlaybackState* state, uint8_t flags);
uint32_t avi_surface_lock(AviPlaybackState* state); /* 0x0047e760 */
uint32_t avi_surface_unlock(AviPlaybackState* state); /* 0x0047e7b0 */
void avi_playback_mode_set(int disabled);
uint32_t avi_stream_frame_count_get(const AviPlaybackState* state);
uint32_t avi_playback_frame_index_get(const AviPlaybackState* state);
uint32_t movie_frame_count(Movie* movie);
int movie_seek_frame(Movie* movie, uint32_t frame_index);
void avi_frame_seek_and_clock_rebase(AviPlaybackState* state,
                                     uint32_t frame_index);
int avi_frame_update_and_blit(AviPlaybackState* state);
void avi_playback_frame_loop(AviPlaybackState* state, void* destination);

/* Fills info (pulling the first frame if needed). 0 on success. */
int movie_info(Movie* m, MovieInfo* out);

typedef struct MovieOut {
    /* RGBA32 (byte order R,G,B,A on little-endian) — width*height rows of
     * rgba_pitch bytes. Owned by the module; valid until the next
     * movie_next. */
    const uint8_t* rgba;
    int            rgba_pitch;
    /* Windows VfW keeps the source DIB available for the frame-copy path.
     * Native FFmpeg frames leave dib NULL and expose RGBA above. Compressed
     * VfW formats without a parsed RGB layout fail movie_next. */
    const uint8_t* dib;
    int            dib_pitch;
    int            dib_bits_per_pixel;
    /* Byte offset from dib to pixel rows, including any palette or masks. */
    int            dib_header_bytes;
    /* For indexed VfW DIBs, points at `dib_palette_entries` RGBQUAD values
     * (blue, green, red, reserved). Like dib, this remains valid only until
     * the next movie_next call. Native FFmpeg frames leave it NULL/zero. */
    const uint8_t* dib_palette;
    unsigned int   dib_palette_entries;
    /* Interleaved stereo s16 PCM for all audio samples that fall on or
     * before this frame's presentation time — the caller feeds it to the
     * audio seam per video frame. Owned; valid until the next movie_next. */
    const int16_t* audio;
    int            audio_frames;
    int            audio_rate;
} MovieOut;

/* Decode one video frame's worth of media. Returns 0 on a frame, 1 at
 * end-of-stream (the FIFO drained), -1 on a decode error. VfW BI_RGB frames
 * at 1/4/8/15/16/24/32 bpp and BI_BITFIELDS/BI_ALPHABITFIELDS at 16/32 bpp
 * are converted to RGBA; compressed DIB encodings without a supported RGB
 * layout are rejected. */
int movie_next(Movie* m, MovieOut* out);

/* --- pixels -------------------------------------------------------------- */

/* Nearest-neighbour scale-fill of an RGBA32 frame into a caller buffer
 * (the corner's blit of AVIStreamGetFrame DIB bytes to the 640×480
 * surface). Pure pixel math, testable without a decoder. */
void movie_scale_rgba(const uint8_t* src, int sw, int sh, int spitch,
                      uint8_t* dst, int dw, int dh, int dpitch);

/* --- rect-aware video pump (port issue #303) ------------------------------ */

/* DirectShowVideoPlaybackLoop @ 0x42efc0 replacement. The corpus takes a
 * destination rectangle (level load supplies its float dimensions; the
 * objective path the achieved-AVI record's) or NULL for the full window,
 * restores the render surface from the primary when a rect is given
 * (the corpus's RenderSurfaceRestoreFromPrimary role runs once before the
 * pump), blits frames through the sample-update role until it reports
 * false, polling the message pump each iteration, and closes the wrapper. Finding 206: the corpus's own
 * DirectDraw Blit passes null rectangles, so the computed rect never lands
 * on the blit — this port replacement *does* apply it (the shape-compatible
 * contract this issue asks for), and that is the deviation. */
typedef struct MovieRect {
    int x, y, w, h;   /* destination rectangle, window pixels */
} MovieRect;

/* Frame-source role (`DirectShowSampleUpdateAndBlit`): fill `out`, return 0
 * for one delivered frame, anything else to end the pump (EOF or error). */
typedef int (*MovieNextFn)(MovieOut* out, void* user);

/* Present role: the destination-side blit of one frame. The rect semantics
 * live here (NULL = full window); non-zero ends the pump. */
typedef int (*MoviePresentFn)(const MovieOut* out, const MovieInfo* info,
                              const MovieRect* rect, void* user);

/* Quit-poll role (`MainLoopFramePump`): non-zero leaves the branch. */
typedef int (*MovieQuitFn)(void* user);

/* Setup-time surface-restore seam (the rect branch's restore-first call).
 * Runs once before the first frame, and only when a rect is given; the
 * port's lock-present-blit path has no lasting surface state, so the
 * driver passes NULL and the hook exists for tests to observe the
 * setup-before-frames ordering. */
typedef void (*MovieSurfaceRestoreFn)(void* user);

/* Pump bundle (NULL `quit`/`restore` allowed). */
typedef struct MoviePump {
    MovieNextFn            next;
    void*                  next_user;
    const MovieInfo*       info;
    const MovieRect*       rect;        /* NULL = full-window branch */
    MoviePresentFn         present;
    void*                  present_user;
    MovieQuitFn            quit;
    void*                  quit_user;
    MovieSurfaceRestoreFn  restore;
    void*                  restore_user;
} MoviePump;

/* One entry point for both corpus branches: poll `quit` between frames,
 * blit through `present` while `next` keeps delivering. Returns the number
 * of frames pumped. */
int movie_video_playback_loop(const MoviePump* pump);

/* Nearest-neighbour scale-fill of an RGBA32 frame into a destination
 * sub-rectangle (pure pixel math, same mapping as movie_scale_rgba but
 * sampled over the rect; clipped to the destination with the rect-relative
 * mapping kept). */
void movie_scale_rgba_rect(const uint8_t* src, int sw, int sh, int spitch,
                           uint8_t* dst, int dw, int dh, int dpitch,
                           const MovieRect* rect);

/* --- config surface (finding 23 callbacks) ------------------------------ */

/* AvisEnabled @ 0x414ce0: play movies when `Lego* Main DontPlayAvis` is not
 * TRUE and engine app mode is below 3. Missing/FALSE defaults to enabled in
 * normal app mode. */
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
