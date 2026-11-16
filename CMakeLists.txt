# SPDX-License-Identifier: LGPL-3.0-or-later
cmake_minimum_required(VERSION 3.20)

project(lrr-portable VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(SRC_ROOT "src" CACHE PATH "Directory containing the reconstruction source tree")
set(TEST_WADS_DIR "" CACHE PATH "Directory containing shipped wads for real-data tests")
set(MOVIES "auto" CACHE STRING "Movie runtime support (auto, enabled, disabled)")
set_property(CACHE MOVIES PROPERTY STRINGS auto enabled disabled)

if(NOT MOVIES MATCHES "^(auto|enabled|disabled)$")
  message(FATAL_ERROR "MOVIES must be auto, enabled, or disabled")
endif()

# The port shell still has guards on the backend; miniwin is the only one.
add_compile_definitions(LEGORRR_BACKEND_MINIWIN=1)

set(ENGINE_DIR "${SRC_ROOT}/gods98")
set(GAME_DIR "${SRC_ROOT}/game")
set(CRT_DIR "${SRC_ROOT}/crt")
set(PLATFORM_DIR "${SRC_ROOT}/platform")

# The source list (src/link_order.cmake): every game, gods98, platform and
# program source, with its library.
get_filename_component(LRR_SRC_ROOT_ABS "${SRC_ROOT}" ABSOLUTE
  BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(LRR_LINK_ORDER_FILE "${LRR_SRC_ROOT_ABS}/link_order.cmake")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
  "${LRR_LINK_ORDER_FILE}")
set(LRR_ORIGINAL_SHAPE_SOURCES "")
foreach(_lib IN ITEMS game gods98 platform_glue program)
  set(LRR_LINK_SOURCES_${_lib} "")
endforeach()
macro(lrr_link kind key library source)
  list(APPEND LRR_LINK_SOURCES_${library} "${SRC_ROOT}/${source}")
  # Units rebuilt in original shape (ADR 0005) are the list's .c files.
  if("${source}" MATCHES "\\.c$")
    list(APPEND LRR_ORIGINAL_SHAPE_SOURCES "${SRC_ROOT}/${source}")
  endif()
endmacro()
include("${LRR_LINK_ORDER_FILE}")

# Units rebuilt in original shape compile as C++17 (ADR 0007 decision 5);
# miniwin's headers are C++-only.
set_source_files_properties(${LRR_ORIGINAL_SHAPE_SOURCES} PROPERTIES
  LANGUAGE CXX)

# The list's sources grouped into libraries (ADR 0003).
add_library(gods98 STATIC ${LRR_LINK_SOURCES_gods98})
target_include_directories(gods98 PUBLIC ${SRC_ROOT} ${ENGINE_DIR})
add_library(game STATIC ${LRR_LINK_SOURCES_game})
target_include_directories(game PUBLIC ${SRC_ROOT} ${GAME_DIR} ${ENGINE_DIR})
target_link_libraries(game PUBLIC gods98)
# gods98 (frame loop, sim tick) calls into game; static libraries may be
# mutually dependent, so every consumer of either one resolves both.
target_link_libraries(gods98 PUBLIC game)
add_library(platform_glue STATIC ${LRR_LINK_SOURCES_platform_glue})
target_include_directories(platform_glue PUBLIC ${SRC_ROOT})
target_link_libraries(gods98 PUBLIC platform_glue ${CMAKE_DL_LIBS})

set(MINIWIN "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/isle-portable/miniwin")
if(NOT EXISTS "${MINIWIN}/include/miniwin/windows.h")
  message(FATAL_ERROR "miniwin is not bootstrapped; run tools/setup-miniwin.sh")
endif()
find_package(SDL3 CONFIG REQUIRED)
set(MINIWIN_SOURCES
  windows/windows.cpp ddraw/ddpalette.cpp ddraw/ddraw.cpp ddraw/ddsurface.cpp
  ddraw/framebuffer.cpp dinput/dinput.cpp d3d/d3dmaterial.cpp
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
target_compile_definitions(miniwin PRIVATE MINIWIN USE_SOFTWARE_RENDER USE_PALETTE_SW_RENDER)
target_link_libraries(miniwin PUBLIC SDL3::SDL3)
target_compile_definitions(gods98 PUBLIC MINIWIN)
target_link_libraries(gods98 PUBLIC miniwin SDL3::SDL3)
target_link_libraries(platform_glue PUBLIC miniwin)

if(NOT MOVIES STREQUAL "disabled")
  # The movie module resolves FFmpeg at runtime. These are the ordered soname
  # candidates it tries.
  if(APPLE)
    set(MOVIE_FORMAT "libavformat.63.dylib;libavformat.62.dylib;libavformat.61.dylib;libavformat.dylib")
    set(MOVIE_CODEC "libavcodec.63.dylib;libavcodec.62.dylib;libavcodec.61.dylib;libavcodec.dylib")
    set(MOVIE_UTIL "libavutil.61.dylib;libavutil.60.dylib;libavutil.59.dylib;libavutil.dylib")
    set(MOVIE_SCALE "libswscale.10.dylib;libswscale.9.dylib;libswscale.8.dylib;libswscale.dylib")
  else()
    set(MOVIE_FORMAT "libavformat.so.63;libavformat.so.62;libavformat.so.61;libavformat.so")
    set(MOVIE_CODEC "libavcodec.so.63;libavcodec.so.62;libavcodec.so.61;libavcodec.so")
    set(MOVIE_UTIL "libavutil.so.61;libavutil.so.60;libavutil.so.59;libavutil.so")
    set(MOVIE_SCALE "libswscale.so.10;libswscale.so.9;libswscale.so.8;libswscale.so")
  endif()
  foreach(pair IN ITEMS "LIBAVFORMAT;${MOVIE_FORMAT}" "LIBAVCODEC;${MOVIE_CODEC}" "LIBAVUTIL;${MOVIE_UTIL}" "LIBSWSCALE;${MOVIE_SCALE}")
    list(GET pair 0 key)
    list(REMOVE_AT pair 0)
    string(REPLACE ";" "\\;" value "${pair}")
    target_compile_definitions(gods98 PUBLIC "LEGORRR_MOVIE_${key}=\"${value}\"")
  endforeach()
else()
  target_compile_definitions(gods98 PUBLIC LEGORRR_MOVIES_CFG_DISABLED=1)
endif()

# These game sources register hooks that nothing references, so the
# program compiles them itself to keep them in the static link.
set(LRR_SOURCES ${LRR_LINK_SOURCES_program}
  ${GAME_DIR}/dependency_grid.cpp ${GAME_DIR}/screen_pick.cpp)
add_executable(program ${LRR_SOURCES})
set_target_properties(program PROPERTIES OUTPUT_NAME "LegoRR")
# The frame-loop object in gods98 calls into game, while game itself uses
# gods98 APIs. Repeat the archive so GNU ld resolves this static-library
# cycle.
target_link_libraries(program PRIVATE game gods98 game platform_glue)
if(WIN32)
  set_target_properties(program PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

include(CTest)
if(BUILD_TESTING)
  set(GODS98_TESTS resource texture sound input audio camera view message airbeat
    mainsim simtick enginebind block model entity_ae movie flic media draw)
  set(GODS98_TEST_SOURCES test_resource.cpp test_texture.cpp test_sound.cpp test_input.cpp
    test_audio.cpp test_camera.cpp test_view.cpp test_message.cpp test_airbeat.cpp
    test_mainsim.cpp test_simtick.cpp test_engine_bind.cpp test_block.cpp test_model.cpp
    test_entity.cpp test_movie.cpp test_flic.cpp test_media.cpp test_draw.cpp)
  list(LENGTH GODS98_TESTS count)
  math(EXPR last "${count} - 1")
  foreach(i RANGE ${last})
    list(GET GODS98_TESTS ${i} name)
    list(GET GODS98_TEST_SOURCES ${i} source)
    if(MOVIES STREQUAL "disabled" AND name STREQUAL "movie")
      continue()
    endif()
    add_executable(${name}_test ${ENGINE_DIR}/${source})
    target_link_libraries(${name}_test PRIVATE gods98)
    if(TEST_WADS_DIR AND name MATCHES "^(resource|texture|sound|block|model|entity_ae|movie|flic)$")
      add_test(NAME ${name} COMMAND ${name}_test --real "${TEST_WADS_DIR}")
    else()
      add_test(NAME ${name} COMMAND ${name}_test)
    endif()
    set_tests_properties(${name} PROPERTIES LABELS ${name})
  endforeach()
  # Compile-only check that the C boundary headers match their C++ headers
  # (ADR 0005). It needs their include paths, not the libraries.
  if(NOT WIN32)
    add_executable(registry_test ${ENGINE_DIR}/test_registry.cpp ${ENGINE_DIR}/g98_029.c)
    target_include_directories(registry_test PRIVATE ${SRC_ROOT})
    target_link_libraries(registry_test PRIVATE miniwin)
    add_test(NAME registry COMMAND registry_test)
  endif()

  add_executable(c_boundaries_test ${GAME_DIR}/test_c_boundaries.cpp)
  target_include_directories(c_boundaries_test PRIVATE ${SRC_ROOT} ${GAME_DIR}
    ${ENGINE_DIR})
  target_compile_definitions(c_boundaries_test PRIVATE MINIWIN)
  target_link_libraries(c_boundaries_test PRIVATE miniwin)
  add_test(NAME c_boundaries COMMAND c_boundaries_test)
  set_tests_properties(c_boundaries PROPERTIES LABELS c_boundaries)
  add_executable(crt_test ${CRT_DIR}/test_crt.cpp)
  target_link_libraries(crt_test PRIVATE gods98)
  add_test(NAME crt COMMAND crt_test)
  set_tests_properties(crt PROPERTIES LABELS crt)
  set(GAME_TESTS level objective savegame mission entity cmd select ai render bind frontend options
    bezier steering animation_object)
  set(GAME_TEST_SOURCES test_level.cpp test_objective.cpp test_savegame.cpp test_mission.cpp
    test_entity.cpp test_cmd.cpp test_select.cpp test_ai.cpp test_render.cpp test_bind.cpp
    test_frontend.cpp test_options.cpp test_bezier.cpp test_steering.cpp
    test_animation_object.cpp)
  list(LENGTH GAME_TESTS count)
  math(EXPR last "${count} - 1")
  foreach(i RANGE ${last})
    list(GET GAME_TESTS ${i} name)
    list(GET GAME_TEST_SOURCES ${i} source)
    add_executable(${name}_test ${GAME_DIR}/${source})
    target_link_libraries(${name}_test PRIVATE game gods98)
    if(TEST_WADS_DIR AND name STREQUAL "level")
      add_test(NAME ${name} COMMAND ${name}_test --real "${TEST_WADS_DIR}")
    else()
      add_test(NAME ${name} COMMAND ${name}_test)
    endif()
    set_tests_properties(${name} PROPERTIES LABELS ${name})
  endforeach()
  # Shipped Lego.cfg Stats read back through the retail accessors (finding
  # 520). A separate process: the probe loads process-wide config tables.
  if(TEST_WADS_DIR)
    add_test(NAME entity_stats_real
      COMMAND entity_test --real-stats "${TEST_WADS_DIR}")
    set_tests_properties(entity_stats_real PROPERTIES LABELS entity)
    # Shipped-type entities driven through the readers of the members the
    # 32-bit layout places at their original offsets (finding 522).
    add_test(NAME entity_layout_real
      COMMAND entity_test --real-layout "${TEST_WADS_DIR}")
    set_tests_properties(entity_layout_real PROPERTIES LABELS entity)
  endif()
  set_tests_properties(audio PROPERTIES ENVIRONMENT "SDL_AUDIODRIVER=dummy")
endif()
