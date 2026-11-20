// SPDX-License-Identifier: LGPL-3.0-or-later
/* The video library of ADR 0007's substitution table: FFmpeg, in place of
 * DirectShow's multimedia streams (movie_unit.cpp, G98-001) and AVIFile
 * (avi_player.cpp, G98-012). The units call it at their `// [library:video]`
 * call sites; it holds no game logic.
 *
 * FFmpeg is a runtime capability, not a build requirement. The library
 * includes no FFmpeg header and links no FFmpeg library: it dlopens
 * libavformat, libavcodec, libavutil and libswscale on first use, trying the
 * ordered, comma-separated soname lists that CMake passes in
 * (LRR_VIDEO_LIB*), and resolves
 * the few symbols it calls. When they are missing, or older than FFmpeg 7,
 * video_open returns NULL and the unit plays no movie.
 *
 * The movie's audio track, which DirectShow's default audio renderer
 * played, goes to an SDL3 audio stream. */
#ifndef LEGORRR_LIBRARY_VIDEO_FFMPEG_H
#define LEGORRR_LIBRARY_VIDEO_FFMPEG_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct VideoFile VideoFile;

/* 1 when the FFmpeg runtime libraries load and bind. */
int video_available(void);

/* Opens a movie file for playback. With withAudio, its first audio track
 * is decoded to SDL3 audio once the clock starts. NULL when FFmpeg is
 * missing, or the file does not open or has no decodable video. */
VideoFile* video_open(const char* path, int withAudio);
void video_close(VideoFile* video);

/* The video's frame size in pixels. */
void video_get_size(VideoFile* video, unsigned int* width,
                    unsigned int* height);
/* The frame rate as dwRate / dwScale frames per second. */
void video_get_rate(VideoFile* video, unsigned int* rate,
                    unsigned int* scale);
/* The video stream's length in frames, 0 when the container does not
 * say, and in milliseconds, 0xffffffff when it does not. */
unsigned int video_get_frame_count(VideoFile* video);
unsigned int video_get_duration(VideoFile* video);

/* Starts the playback clock and the audio. */
void video_start(VideoFile* video);

/* The playback path (DirectShow's synchronous sample update): waits until
 * the next frame is due on the clock, dropping frames that are already
 * late, and converts it into pixels, a top-down image with the given row
 * pitch whose pixel format the bit count and RGB masks describe. Returns 1
 * on a frame, 0 at the end of the stream or on an error. */
int video_update(VideoFile* video, void* pixels, long pitch,
                 unsigned int bitCount, uint32_t redMask,
                 uint32_t greenMask, uint32_t blueMask);

/* The frame-indexed path (AVIStreamGetFrame): decodes frame `index` and
 * returns it as a packed DIB, a 40-byte BITMAPINFOHEADER followed by
 * bottom-up rows in the given format, each padded to 4 bytes. The memory
 * belongs to the video and stays valid until the next call. NULL past the
 * end or on an error. */
const void* video_get_frame_dib(VideoFile* video, unsigned int index,
                                unsigned int bitCount, uint32_t redMask,
                                uint32_t greenMask, uint32_t blueMask);

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_LIBRARY_VIDEO_FFMPEG_H */
