# SPDX-License-Identifier: LGPL-3.0-or-later
# The tests of the port's audio and video libraries (src/library).
# CMakeLists.txt includes this file inside if(BUILD_TESTING), after
# cmake/LrrTesting.cmake; docs/building.md, "Writing a test".

# The video library on a synthetic clip: against the host's FFmpeg
# (skipped without it), built with sonames that never load, and built as
# the no-movies stub.
set(LRR_VIDEO_TEST_CLIP "${LRR_SRC_ROOT_ABS}/library/testdata/synthetic.avi")
lrr_unit_test(video SOURCES ${SRC_ROOT}/library/test_video_ffmpeg.cpp
  DEFINES "LRR_VIDEO_TEST_CLIP=\"${LRR_VIDEO_TEST_CLIP}\""
  LINK library
  LABELS video
  SKIP_RETURN_CODE 77
  ENVIRONMENT "SDL_AUDIO_DRIVER=dummy")
lrr_unit_test(video_missing
  SOURCES ${SRC_ROOT}/library/test_video_ffmpeg.cpp ${SRC_ROOT}/library/video_ffmpeg.cpp
  INCLUDES ${SRC_ROOT}
  DEFINES LRR_VIDEO_TEST_MISSING
    "LRR_VIDEO_TEST_CLIP=\"${LRR_VIDEO_TEST_CLIP}\""
    "LRR_VIDEO_LIBAVFORMAT=\"liblrr-missing-avformat.so\""
    "LRR_VIDEO_LIBAVCODEC=\"liblrr-missing-avcodec.so\""
    "LRR_VIDEO_LIBAVUTIL=\"liblrr-missing-avutil.so\""
    "LRR_VIDEO_LIBSWSCALE=\"liblrr-missing-swscale.so\""
  LINK SDL3::SDL3
  LABELS video)
# The FFmpeg the macOS packages bundle (LRR_FFMPEG_DIR), found first on
# DYLD_LIBRARY_PATH: with no SKIP_RETURN_CODE, failing to load it fails.
if(APPLE AND LRR_BUNDLED_FFMPEG_LIBS)
  lrr_unit_test_variant(video_bundled OF video
    LABELS video
    ENVIRONMENT "SDL_AUDIO_DRIVER=dummy;DYLD_LIBRARY_PATH=${LRR_FFMPEG_DIR}/lib")
endif()
# The LRR_MOVIES=OFF stub, which every build checks whatever the option.
lrr_unit_test(video_stub
  SOURCES ${SRC_ROOT}/library/test_video_ffmpeg.cpp ${SRC_ROOT}/library/video_ffmpeg.cpp
  INCLUDES ${SRC_ROOT}
  DEFINES LRR_VIDEO_TEST_MISSING
    LRR_MOVIES=0 "LRR_VIDEO_TEST_CLIP=\"${LRR_VIDEO_TEST_CLIP}\""
  LINK SDL3::SDL3
  LABELS video)

# The audio library and the units' audio substitution on synthetic
# waves: buffers, wave readers and the CD device on SDL's dummy driver,
# and the mix of a streamed wave and a CD track on its disk driver.
lrr_unit_test(audio SOURCES ${SRC_ROOT}/library/test_audio_sdl.cpp
  ARGS dummy
  LABELS audio
  SKIP_RETURN_CODE 77
  ENVIRONMENT "SDL_AUDIO_DRIVER=dummy")
lrr_unit_test_variant(audio_mix OF audio
  ARGS disk
  LABELS audio
  SKIP_RETURN_CODE 77)
