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

# Decode disjoint palette packets and verify the RGB565 lookup entries.
lrr_unit_test(flic_unit SOURCES ${ENGINE_DIR}/test_flic_unit.cpp LABELS flic_unit)

# Exercise the text ring and timed overlay with a glyph sink in memory.
lrr_unit_test(textwindow SOURCES ${ENGINE_DIR}/test_textwindow.cpp
  WRAP font_output_char LABELS textwindow)

# Resolve the original key-name table back to DirectInput codes.
lrr_unit_test(keys SOURCES ${ENGINE_DIR}/test_keys.cpp LABELS keys)

# Read a memory-backed PCM RIFF header and sample data across chunk boundaries.
lrr_unit_test(soundsys SOURCES ${ENGINE_DIR}/test_soundsys.cpp LABELS soundsys)

# Fill, release and reuse entries in the level-map handle table.
lrr_unit_test(g98_024 SOURCES ${ENGINE_DIR}/test_g98_024.cpp LABELS g98_024)

# Copy a retained-mode frame tree and verify walk termination and local transforms.
lrr_unit_test(animclone SOURCES ${ENGINE_DIR}/test_animclone.cpp LABELS animclone)

# Check packed BMP scanline strides and the default 256-entry palette.
lrr_unit_test(g98_026 SOURCES ${ENGINE_DIR}/test_g98_026.cpp LABELS g98_026)

# Verify DirectX result decoding, report metadata and bounded formatting.
lrr_unit_test(dxerror SOURCES ${ENGINE_DIR}/test_dxerror.cpp LABELS dxerror)

# Check startup file-list stream cleanup, scan output, and open failure.
lrr_unit_test(file_list_streams SOURCES ${ENGINE_DIR}/test_file_list_streams.cpp
  WRAP game_file_open_dispatch game_file_close resource_file_token_list_load_and_tree_scan_target
  LABELS file_list_streams)

# Mount synthetic WWAD archives and check precedence, packed entries and handles.
lrr_unit_test(wad SOURCES ${ENGINE_DIR}/test_wad.cpp LABELS wad)

# Parse a synthetic LightWave triangle and match surface records to the name table.
lrr_unit_test(lwo SOURCES ${ENGINE_DIR}/test_lwo.cpp LABELS lwo)

# Expand stored and method-2 RNC streams and pin canonical Huffman codes.
lrr_unit_test(rnc SOURCES ${ENGINE_DIR}/test_rnc.cpp LABELS rnc)

# Filter a synthetic display-mode list and look up its exact descriptions.
lrr_unit_test(init SOURCES ${ENGINE_DIR}/test_init.cpp LABELS init)
