// SPDX-License-Identifier: LGPL-3.0-or-later
/* miniaudio's implementation, for the audio library's CD tracks
 * (audio_sdl.cpp): its decoders only, with Vorbis from the stb_vorbis that
 * ships in miniaudio's extras. SDL3 does the playback, so miniaudio's own
 * device I/O, engine and encoders are compiled out (CMakeLists.txt). */
#define STB_VORBIS_HEADER_ONLY
#include "extras/stb_vorbis.c"

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#undef STB_VORBIS_HEADER_ONLY
#include "extras/stb_vorbis.c"
