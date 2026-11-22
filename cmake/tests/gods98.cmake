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

# Convert indexed and BGR bitmap rows to top-down RGB565 with stride padding.
lrr_unit_test(images_unit SOURCES ${ENGINE_DIR}/test_images_unit.cpp LABELS images_unit)

# Check AVI metadata, seeking and failed stream setup with a synthetic decoder.
lrr_unit_test(avi_player SOURCES ${ENGINE_DIR}/test_avi_player.cpp
  WRAP video_open video_get_rate video_get_size video_get_frame_count video_get_frame_dib video_close
  LABELS avi_player)

# Drive current and previous key maps through synthetic SDL snapshots and focus loss.
lrr_unit_test(input_unit SOURCES ${ENGINE_DIR}/test_input_unit.cpp
  WRAP SDL_GetKeyboardState LABELS input_unit)

# Exercise CRLF reads, load notifications and the mounted path rewrite.
lrr_unit_test(files_dispatch SOURCES ${ENGINE_DIR}/test_files_dispatch.cpp LABELS files)

# Check texture sequence names across zero padding and a decimal carry.
lrr_unit_test(mesh_unit SOURCES ${ENGINE_DIR}/test_mesh_unit.cpp LABELS mesh_unit)
