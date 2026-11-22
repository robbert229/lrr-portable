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

# Pin token boundaries, escape decoding, hash normalization and boolean spellings.
lrr_unit_test(utils SOURCES ${ENGINE_DIR}/test_utils.cpp LABELS utils)

# Exercise command-line options and texture-management flags without starting the game.
lrr_unit_test(main SOURCES ${ENGINE_DIR}/test_main.cpp LABELS main)

# Parse nested block text and check wildcard lookups, sibling traversal and typed values.
lrr_unit_test(gods98_config SOURCES ${ENGINE_DIR}/test_config.cpp LABELS config)

# Check projected triangle geometry, intersections and matrix composition.
lrr_unit_test(maths SOURCES ${ENGINE_DIR}/test_maths.cpp LABELS maths)

# Measure variable-width glyphs, tabs and line breaks without a font texture.
lrr_unit_test(font SOURCES ${ENGINE_DIR}/test_font.cpp LABELS font)

# Pin sound-bank volume limits and disabled-audio return values.
lrr_unit_test(sound3d SOURCES ${ENGINE_DIR}/test_sound3d.cpp LABELS sound3d)

# Verify RGB565 packing and the byte layout of a synthetic screenshot BMP.
lrr_unit_test(directdraw SOURCES ${ENGINE_DIR}/test_directdraw.cpp LABELS directdraw)
