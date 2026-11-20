# SPDX-License-Identifier: LGPL-3.0-or-later
cmake_minimum_required(VERSION 3.20)

project(lrr-portable VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(SRC_ROOT "src" CACHE PATH "Directory containing the reconstruction source tree")

set(ENGINE_DIR "${SRC_ROOT}/gods98")
set(GAME_DIR "${SRC_ROOT}/game")
set(PLATFORM_DIR "${SRC_ROOT}/platform")

# The source list (src/link_order.cmake): the units and the platform
# sources, with their library.
get_filename_component(LRR_SRC_ROOT_ABS "${SRC_ROOT}" ABSOLUTE
  BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(LRR_LINK_ORDER_FILE "${LRR_SRC_ROOT_ABS}/link_order.cmake")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
  "${LRR_LINK_ORDER_FILE}")
foreach(_lib IN ITEMS game gods98 platform library)
  set(LRR_LINK_SOURCES_${_lib} "")
endforeach()
macro(lrr_link kind key library source)
  list(APPEND LRR_LINK_SOURCES_${library} "${SRC_ROOT}/${source}")
endmacro()
include("${LRR_LINK_ORDER_FILE}")

# The units compile as C++17 (ADR 0007 decision 5); miniwin's headers are
# C++-only.
set(LRR_UNIT_SOURCES ${LRR_LINK_SOURCES_game} ${LRR_LINK_SOURCES_gods98})
set_source_files_properties(${LRR_UNIT_SOURCES} PROPERTIES LANGUAGE CXX)

set(MINIWIN "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/isle-portable/miniwin")
if(NOT EXISTS "${MINIWIN}/include/miniwin/windows.h")
  message(FATAL_ERROR "miniwin is not bootstrapped; run tools/setup-miniwin.sh")
endif()
find_package(SDL3 CONFIG REQUIRED)
set(MINIWIN_SOURCES
  windows/windows.cpp windows/windowmessages.cpp ddraw/ddpalette.cpp ddraw/ddraw.cpp ddraw/ddsurface.cpp
  ddraw/framebuffer.cpp d3d/d3dmaterial.cpp
  d3d/d3dviewport.cpp d3drm/d3drm.cpp d3drm/d3drmanimationset.cpp
  d3drm/d3drmdevice.cpp d3drm/d3drmframe.cpp d3drm/d3drmlight.cpp
  d3drm/d3drmmesh.cpp d3drm/d3drmmeshbuilder.cpp d3drm/d3drmtexture.cpp
  d3drm/d3drmuservisual.cpp d3drm/d3drmviewport.cpp d3drm/d3drmrenderer.cpp
  internal/meshutils.cpp d3drm/backends/software/renderer.cpp
  d3drm/backends/palettesw/renderer.cpp)
list(TRANSFORM MINIWIN_SOURCES PREPEND "${MINIWIN}/src/")
add_library(miniwin STATIC ${MINIWIN_SOURCES})
target_include_directories(miniwin PUBLIC "${MINIWIN}/include"
  "${MINIWIN}/src/internal"
  "${MINIWIN}/src/d3drm/backends/sdl3gpu/shaders/generated")
# The machine miniwin emulates for the game: a 16-bit RGB565 display (the
# units convert images to 16-bit only), and the software renderer described as
# a hardware device, since Init.c's -best accepts only HAL devices.
target_compile_definitions(miniwin PRIVATE MINIWIN USE_SOFTWARE_RENDER USE_PALETTE_SW_RENDER
  MINIWIN_PIXELFORMAT=SDL_PIXELFORMAT_RGB565 MINIWIN_SOFTWARE_HAL)
target_link_libraries(miniwin PUBLIC SDL3::SDL3)

# The units, in one library (ADR 0003's game/gods98 split is a source
# grouping only: the units call each other both ways).
add_library(units STATIC ${LRR_UNIT_SOURCES})
target_include_directories(units PUBLIC ${SRC_ROOT} ${ENGINE_DIR} ${GAME_DIR})
target_compile_definitions(units PUBLIC MINIWIN)
target_link_libraries(units PUBLIC miniwin SDL3::SDL3)

# The libraries the units' substitutions call (ADR 0007, decision 3).
# The video library loads FFmpeg at run time, trying each library's
# comma-separated sonames in order, so the build needs no FFmpeg package;
# without the libraries at run time the units skip their movies.
if(APPLE)
  set(LRR_VIDEO_LIBAVFORMAT "libavformat.63.dylib,libavformat.62.dylib,libavformat.61.dylib,libavformat.dylib")
  set(LRR_VIDEO_LIBAVCODEC "libavcodec.63.dylib,libavcodec.62.dylib,libavcodec.61.dylib,libavcodec.dylib")
  set(LRR_VIDEO_LIBAVUTIL "libavutil.61.dylib,libavutil.60.dylib,libavutil.59.dylib,libavutil.dylib")
  set(LRR_VIDEO_LIBSWSCALE "libswscale.10.dylib,libswscale.9.dylib,libswscale.8.dylib,libswscale.dylib")
else()
  set(LRR_VIDEO_LIBAVFORMAT "libavformat.so.63,libavformat.so.62,libavformat.so.61,libavformat.so")
  set(LRR_VIDEO_LIBAVCODEC "libavcodec.so.63,libavcodec.so.62,libavcodec.so.61,libavcodec.so")
  set(LRR_VIDEO_LIBAVUTIL "libavutil.so.61,libavutil.so.60,libavutil.so.59,libavutil.so")
  set(LRR_VIDEO_LIBSWSCALE "libswscale.so.10,libswscale.so.9,libswscale.so.8,libswscale.so")
endif()
# The audio library decodes the CD tracks' files with miniaudio: the copy
# isle-portable vendors when its submodules are checked out, else the same
# release fetched at configure time.
set(LRR_MINIAUDIO_DIR "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/isle-portable/3rdparty/miniaudio"
  CACHE PATH "miniaudio source tree (miniaudio.h and extras/stb_vorbis.c)")
if(NOT EXISTS "${LRR_MINIAUDIO_DIR}/miniaudio.h")
  include(FetchContent)
  if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
  endif()
  FetchContent_Declare(miniaudio
    URL https://github.com/mackron/miniaudio/archive/refs/tags/0.11.25.tar.gz
    URL_HASH MD5=6fae6da8f30afb3ddcba26fcaa64f540
    SOURCE_SUBDIR lrr-no-cmake)
  FetchContent_MakeAvailable(miniaudio)
  set(LRR_MINIAUDIO_DIR "${miniaudio_SOURCE_DIR}")
endif()
add_library(library STATIC ${LRR_LINK_SOURCES_library})
target_include_directories(library PUBLIC ${SRC_ROOT})
target_include_directories(library PRIVATE "${LRR_MINIAUDIO_DIR}")
# Only the decoders: SDL3 does the playback.
target_compile_definitions(library PRIVATE MA_NO_DEVICE_IO MA_NO_ENCODING
  MA_NO_GENERATION MA_NO_RESOURCE_MANAGER MA_NO_NODE_GRAPH MA_NO_ENGINE
  MA_NO_THREADING)
set_source_files_properties("${SRC_ROOT}/library/miniaudio_impl.c" PROPERTIES
  COMPILE_OPTIONS "$<IF:$<C_COMPILER_ID:MSVC>,/w,-w>")
foreach(_name IN ITEMS LIBAVFORMAT LIBAVCODEC LIBAVUTIL LIBSWSCALE)
  target_compile_definitions(library PRIVATE
    "LRR_VIDEO_${_name}=\"${LRR_VIDEO_${_name}}\"")
endforeach()
target_link_libraries(library PUBLIC miniwin SDL3::SDL3 ${CMAKE_DL_LIBS})
if(UNIX)
  target_link_libraries(library PUBLIC m)
endif()
target_link_libraries(units PUBLIC library)

# The seam's remaining stubs (ADR 0007): the imports neither miniwin nor a
# library substitution provides yet.
set(LRR_ENTRY_SOURCE "${PLATFORM_DIR}/native/entry.cpp")
set(LRR_SEAM_SOURCES ${LRR_LINK_SOURCES_platform})
list(REMOVE_ITEM LRR_SEAM_SOURCES "${LRR_ENTRY_SOURCE}")
add_library(seam_stubs STATIC ${LRR_SEAM_SOURCES})
target_include_directories(seam_stubs PUBLIC ${SRC_ROOT})
target_link_libraries(seam_stubs PUBLIC miniwin)
target_link_libraries(units PUBLIC seam_stubs)

add_executable(program ${LRR_ENTRY_SOURCE})
set_target_properties(program PROPERTIES OUTPUT_NAME "LegoRR")
target_link_libraries(program PRIVATE units)
if(WIN32)
  set_target_properties(program PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

include(CTest)
if(BUILD_TESTING)
  # The units' own tests: each calls one unit directly.
  foreach(name IN ITEMS bezier steering animation_object reward)
    add_executable(${name}_test ${GAME_DIR}/test_${name}.cpp)
    target_link_libraries(${name}_test PRIVATE units)
    add_test(NAME ${name} COMMAND ${name}_test)
    set_tests_properties(${name} PROPERTIES LABELS ${name})
  endforeach()
  # FrontEnd.c's random menu overlay roll on a simulated clock: the unit's
  # calls to main_time_get reach the test's __wrap_main_time_get.
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang" AND NOT APPLE AND NOT WIN32)
    add_executable(overlay_roll_test ${GAME_DIR}/test_overlay_roll.cpp)
    target_link_libraries(overlay_roll_test PRIVATE units)
    target_link_options(overlay_roll_test PRIVATE "LINKER:--wrap=main_time_get")
    add_test(NAME overlay_roll COMMAND overlay_roll_test)
    set_tests_properties(overlay_roll PROPERTIES LABELS frontend)
    # The menu edge scroll and the reward screen scroll on a simulated clock.
    add_executable(menu_frame_rate_test ${GAME_DIR}/test_menu_frame_rate.cpp)
    target_link_libraries(menu_frame_rate_test PRIVATE units)
    target_link_options(menu_frame_rate_test PRIVATE "LINKER:--wrap=main_time_get")
    add_test(NAME menu_frame_rate COMMAND menu_frame_rate_test)
    set_tests_properties(menu_frame_rate PROPERTIES LABELS frontend)
  endif()
  add_executable(draw_test ${ENGINE_DIR}/test_draw.cpp)
  target_link_libraries(draw_test PRIVATE units)
  add_test(NAME draw COMMAND draw_test)
  set_tests_properties(draw PROPERTIES LABELS draw)
  # The BMP and FLIC readers on synthetic data at odd addresses.
  add_executable(unaligned_readers_test ${ENGINE_DIR}/test_unaligned_readers.cpp)
  target_link_libraries(unaligned_readers_test PRIVATE units)
  add_test(NAME unaligned_readers COMMAND unaligned_readers_test)
  # Under -fsanitize=undefined, a misaligned load fails the test.
  set_tests_properties(unaligned_readers PROPERTIES LABELS unaligned_readers
    ENVIRONMENT "UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1")
  # G98-016's frame-size getters, which the menu overlays, pointers and
  # panels size their blits from.
  add_executable(flic_size_test ${ENGINE_DIR}/test_flic_size.cpp)
  target_link_libraries(flic_size_test PRIVATE units)
  add_test(NAME flic_size COMMAND flic_size_test)
  # G98-029's registry walk over the test's own registry: the unit alone,
  # without the seam's stubs.
  add_executable(registry_test ${ENGINE_DIR}/test_registry.cpp ${ENGINE_DIR}/g98_029.c)
  set_source_files_properties(${ENGINE_DIR}/g98_029.c TARGET_DIRECTORY registry_test
    PROPERTIES LANGUAGE CXX)
  target_include_directories(registry_test PRIVATE ${SRC_ROOT} ${ENGINE_DIR})
  target_link_libraries(registry_test PRIVATE miniwin)
  add_test(NAME registry COMMAND registry_test)
  set_tests_properties(registry PROPERTIES LABELS registry)
  # Input.c's keyboard substitution: the SDL scancode to DIK_* table.
  add_executable(input_keys_test ${ENGINE_DIR}/test_input_keys.cpp)
  target_include_directories(input_keys_test PRIVATE ${ENGINE_DIR})
  target_link_libraries(input_keys_test PRIVATE SDL3::SDL3)
  add_test(NAME input_keys COMMAND input_keys_test)
  set_tests_properties(input_keys PROPERTIES LABELS input)

  # The video library on a synthetic clip: against the host's FFmpeg
  # (skipped without it), and built with sonames that never load.
  set(LRR_VIDEO_TEST_CLIP "${LRR_SRC_ROOT_ABS}/library/testdata/synthetic.avi")
  add_executable(video_test ${SRC_ROOT}/library/test_video_ffmpeg.cpp)
  target_compile_definitions(video_test PRIVATE "LRR_VIDEO_TEST_CLIP=\"${LRR_VIDEO_TEST_CLIP}\"")
  target_link_libraries(video_test PRIVATE library)
  add_test(NAME video COMMAND video_test)
  set_tests_properties(video PROPERTIES LABELS video SKIP_RETURN_CODE 77
    ENVIRONMENT "SDL_AUDIO_DRIVER=dummy")
  add_executable(video_missing_test ${SRC_ROOT}/library/test_video_ffmpeg.cpp
    ${SRC_ROOT}/library/video_ffmpeg.cpp)
  target_include_directories(video_missing_test PRIVATE ${SRC_ROOT})
  target_compile_definitions(video_missing_test PRIVATE LRR_VIDEO_TEST_MISSING
    "LRR_VIDEO_TEST_CLIP=\"${LRR_VIDEO_TEST_CLIP}\""
    "LRR_VIDEO_LIBAVFORMAT=\"liblrr-missing-avformat.so\""
    "LRR_VIDEO_LIBAVCODEC=\"liblrr-missing-avcodec.so\""
    "LRR_VIDEO_LIBAVUTIL=\"liblrr-missing-avutil.so\""
    "LRR_VIDEO_LIBSWSCALE=\"liblrr-missing-swscale.so\"")
  target_link_libraries(video_missing_test PRIVATE SDL3::SDL3 ${CMAKE_DL_LIBS})
  add_test(NAME video_missing COMMAND video_missing_test)
  set_tests_properties(video_missing PROPERTIES LABELS video)

  # The audio library and the units' audio substitution on synthetic
  # waves: buffers, wave readers and the CD device on SDL's dummy driver,
  # and the mix of a streamed wave and a CD track on its disk driver.
  add_executable(audio_test ${SRC_ROOT}/library/test_audio_sdl.cpp)
  target_link_libraries(audio_test PRIVATE units)
  add_test(NAME audio COMMAND audio_test dummy)
  set_tests_properties(audio PROPERTIES LABELS audio SKIP_RETURN_CODE 77
    ENVIRONMENT "SDL_AUDIO_DRIVER=dummy")
  add_test(NAME audio_mix COMMAND audio_test disk)
  set_tests_properties(audio_mix PROPERTIES LABELS audio SKIP_RETURN_CODE 77)

  # The DirectX 6 interface versions the units use, declared in miniwin
  # (ADR 0007, decision 3): reachable, and failing rather than crashing
  # where miniwin has only a stub.
  add_executable(miniwin_dx6_test ${PLATFORM_DIR}/native/test_miniwin_dx6.cpp)
  target_link_libraries(miniwin_dx6_test PRIVATE miniwin)
  add_test(NAME miniwin_dx6 COMMAND miniwin_dx6_test)
  set_tests_properties(miniwin_dx6 PROPERTIES LABELS miniwin)

  # What the boot to the main menu needs from miniwin, on synthetic data:
  # display modes, DirectDraw 4 surfaces and blits, the framebuffer, and
  # Direct3DRM frame transforms.
  add_executable(miniwin_ddraw4_test ${PLATFORM_DIR}/native/test_miniwin_ddraw4.cpp)
  target_link_libraries(miniwin_ddraw4_test PRIVATE miniwin)
  add_test(NAME miniwin_ddraw4 COMMAND miniwin_ddraw4_test)
  set_tests_properties(miniwin_ddraw4 PROPERTIES LABELS miniwin)

  # Main.c's window procedure under miniwin: SDL mouse events as client
  # coordinate messages, and SDL's quit event as WM_CLOSE.
  add_executable(miniwin_messages_test ${PLATFORM_DIR}/native/test_miniwin_messages.cpp)
  target_link_libraries(miniwin_messages_test PRIVATE miniwin)
  add_test(NAME miniwin_messages COMMAND miniwin_messages_test)
  set_tests_properties(miniwin_messages PROPERTIES LABELS miniwin)
endif()
