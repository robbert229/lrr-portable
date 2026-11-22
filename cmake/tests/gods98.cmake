# SPDX-License-Identifier: LGPL-3.0-or-later

# Movie playback blits a decoded frame and clears its sample at end of stream.
lrr_unit_test(movie_unit SOURCES ${ENGINE_DIR}/test_movie_unit.cpp
  WRAP video_open video_get_size video_get_duration video_start video_update video_close
  LABELS movie_unit)

# Parse container types and exercise a retained-mode parent and child.
lrr_unit_test(container SOURCES ${ENGINE_DIR}/test_container.cpp LABELS container)

# Validate viewport sizing, camera, clipping and field on a dummy-driver device.
lrr_unit_test(viewports SOURCES ${ENGINE_DIR}/test_viewports.cpp
  LABELS viewports ENVIRONMENT "SDL_VIDEO_DRIVER=dummy")
