# SPDX-License-Identifier: LGPL-3.0-or-later
cmake_minimum_required(VERSION 3.20)
project(lrr-portable VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(PLATFORM "native" CACHE STRING "Backend to build (native or winapi)")
set_property(CACHE PLATFORM PROPERTY STRINGS native winapi)
set(SRC_ROOT "src" CACHE PATH "Directory containing the reconstruction source tree")
set(TEST_WADS_DIR "" CACHE PATH "Directory containing shipped wads for real-data tests")
set(MOVIES "auto" CACHE STRING "Movie runtime support (auto, enabled, disabled)")
set_property(CACHE MOVIES PROPERTY STRINGS auto enabled disabled)
option(LRR_CLASSIC_MSVC5 "Use CMake linker flags compatible with MSVC 5.0" OFF)

if(LRR_CLASSIC_MSVC5)
  # CMake's Windows-MSVC module emits newer spellings that the classic
  # toolchain's ReproBit command model does not admit.
  include_directories(BEFORE "${CMAKE_CURRENT_SOURCE_DIR}/src/compat/msvc5")

  string(REGEX REPLACE "[ /]Zm[0-9]+" "" _classic_cxx_flags " ${CMAKE_CXX_FLAGS}")
  string(STRIP "${_classic_cxx_flags}" _classic_cxx_flags)
  set(CMAKE_CXX_FLAGS "${_classic_cxx_flags}" CACHE STRING
    "Classic MSVC 5.0 compatible compiler flags" FORCE)

  foreach(_link_kind IN ITEMS STATIC EXE SHARED MODULE)
    foreach(_config IN ITEMS "" DEBUG MINSIZEREL RELEASE RELWITHDEBINFO)
      if(_config)
        set(_link_flags "CMAKE_${_link_kind}_LINKER_FLAGS_${_config}")
      else()
        set(_link_flags "CMAKE_${_link_kind}_LINKER_FLAGS")
      endif()
      string(REGEX REPLACE "/[Mm][Aa][Cc][Hh][Ii][Nn][Ee]:[Xx]86" "/machine:i386"
        _classic_link_flags "${${_link_flags}}")
      string(REGEX REPLACE "/[Pp][Dd][Bb][Tt][Yy][Pp][Ee]:[Ss][Ee][Pp][Tt]"
        "" _classic_link_flags "${_classic_link_flags}")
      string(REGEX REPLACE "[ /][Dd][Ee][Bb][Uu][Gg]" ""
        _classic_link_flags "${_classic_link_flags}")
      set(${_link_flags} "${_classic_link_flags}" CACHE STRING
        "Classic MSVC 5.0 compatible linker flags" FORCE)
    endforeach()
  endforeach()

  # The final linker PDB depends on the static archives. ReproBit's current
  # evidence model treats PDBs as compile-stage artifacts, so omit the link
  # PDB and its /DEBUG switch for this classic build profile.
  string(REGEX REPLACE "[ /][Pp][Dd][Bb]:<TARGET_PDB>" ""
    _classic_exe_link_rule "${CMAKE_CXX_LINK_EXECUTABLE}")
  set(CMAKE_CXX_LINK_EXECUTABLE "${_classic_exe_link_rule}")

  # CMake's MSVC rule shares one compiler PDB per target. Give each object
  # its own PDB so ReproBit can model the compile steps as isolated producers.
  set(CMAKE_CXX_COMPILE_OBJECT
    "<CMAKE_CXX_COMPILER> /nologo /TP <DEFINES> <INCLUDES> <FLAGS> /Fo<OBJECT> /Fd<OBJECT>.pdb -c <SOURCE>")
endif()

if(NOT PLATFORM STREQUAL "native" AND NOT PLATFORM STREQUAL "winapi")
  message(FATAL_ERROR "PLATFORM must be native or winapi")
endif()
if(NOT MOVIES MATCHES "^(auto|enabled|disabled)$")
  message(FATAL_ERROR "MOVIES must be auto, enabled, or disabled")
endif()

if(PLATFORM STREQUAL "native")
  add_compile_definitions(LEGORRR_BACKEND_MINIWIN=1)
else()
  add_compile_definitions(LEGORRR_BACKEND_WINAPI=1)
endif()

set(ENGINE_DIR "${SRC_ROOT}/gods98")
set(GAME_DIR "${SRC_ROOT}/game")
set(PLATFORM_DIR "${SRC_ROOT}/platform")

add_library(gods98 STATIC
  ${ENGINE_DIR}/gods98.cpp ${ENGINE_DIR}/engine.cpp ${ENGINE_DIR}/tickers.cpp
  ${ENGINE_DIR}/resource.cpp ${ENGINE_DIR}/texture.cpp ${ENGINE_DIR}/sound.cpp
  ${ENGINE_DIR}/input.cpp ${ENGINE_DIR}/surface.cpp ${ENGINE_DIR}/frameloop.cpp
  ${ENGINE_DIR}/audio.cpp ${ENGINE_DIR}/camera.cpp ${ENGINE_DIR}/view.cpp
  ${ENGINE_DIR}/message.cpp ${ENGINE_DIR}/mainsim.cpp ${ENGINE_DIR}/simtick.cpp
  ${ENGINE_DIR}/block.cpp ${ENGINE_DIR}/model.cpp ${ENGINE_DIR}/entity.cpp
  ${ENGINE_DIR}/movie.cpp ${ENGINE_DIR}/flic.cpp)
target_include_directories(gods98 PUBLIC ${SRC_ROOT} ${ENGINE_DIR})

add_library(game STATIC
  ${GAME_DIR}/game.cpp ${GAME_DIR}/level.cpp ${GAME_DIR}/objective.cpp
  ${GAME_DIR}/savegame.cpp ${GAME_DIR}/mission.cpp ${GAME_DIR}/entity.cpp
  ${GAME_DIR}/cmd.cpp ${GAME_DIR}/select.cpp ${GAME_DIR}/ai.cpp
  ${GAME_DIR}/render.cpp ${GAME_DIR}/frontend.cpp ${GAME_DIR}/options.cpp
  ${GAME_DIR}/util.cpp)
target_include_directories(game PUBLIC ${SRC_ROOT} ${GAME_DIR} ${ENGINE_DIR})
target_link_libraries(game PUBLIC gods98)

if(PLATFORM STREQUAL "native")
  set(MINIWIN "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/isle-portable/miniwin")
  if(NOT EXISTS "${MINIWIN}/include/miniwin/windows.h")
    message(FATAL_ERROR "miniwin is not bootstrapped; run tools/setup-miniwin.sh")
  endif()
  find_package(SDL3 CONFIG REQUIRED)
  set(MINIWIN_SOURCES
    windows/windows.cpp ddraw/ddpalette.cpp ddraw/ddraw.cpp ddraw/ddsurface.cpp
    ddraw/framebuffer.cpp d3drm/d3drm.cpp d3drm/d3drmdevice.cpp
    d3drm/d3drmframe.cpp d3drm/d3drmlight.cpp d3drm/d3drmmesh.cpp
    d3drm/d3drmtexture.cpp d3drm/d3drmviewport.cpp d3drm/d3drmrenderer.cpp
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
  add_library(platform_glue STATIC ${PLATFORM_DIR}/native/frameloop.cpp)
  target_link_libraries(platform_glue PUBLIC miniwin)
  set(PLATFORM_ENTRY ${PLATFORM_DIR}/native/entry.cpp)
else()
  add_library(platform_glue STATIC ${PLATFORM_DIR}/winapi/frameloop.cpp)
  target_include_directories(platform_glue PUBLIC ${SRC_ROOT})
  set(PLATFORM_ENTRY ${PLATFORM_DIR}/winapi/entry.cpp)
endif()
target_include_directories(platform_glue PUBLIC ${SRC_ROOT})
target_link_libraries(gods98 PUBLIC platform_glue ${CMAKE_DL_LIBS})

if(PLATFORM STREQUAL "native")
  if(NOT MOVIES STREQUAL "disabled")
    # The movie module resolves FFmpeg at runtime. Keep these ordered soname
    # candidates in step with Meson's configure-time capability probe.
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
endif()

set(LRR_SOURCES ${SRC_ROOT}/main.cpp ${PLATFORM_ENTRY})
if(PLATFORM STREQUAL "winapi")
  list(APPEND LRR_SOURCES ${SRC_ROOT}/version.rc)
endif()
add_executable(program ${LRR_SOURCES})
# Keep the shipped executable name while using ReproBit's lowercase target ID.
set_target_properties(program PROPERTIES OUTPUT_NAME "LegoRR")
# The frame-loop object in gods98 calls into game, while game itself uses
# gods98 APIs. Repeat the archive so GNU ld resolves this static-library cycle.
target_link_libraries(program PRIVATE game gods98 game platform_glue)
if(WIN32)
  set_target_properties(program PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

include(CTest)
if(BUILD_TESTING AND PLATFORM STREQUAL "native")
  set(GODS98_TESTS resource texture sound input audio camera view message airbeat
    mainsim simtick enginebind block model entity_ae movie flic)
  set(GODS98_TEST_SOURCES test_resource.cpp test_texture.cpp test_sound.cpp test_input.cpp
    test_audio.cpp test_camera.cpp test_view.cpp test_message.cpp test_airbeat.cpp
    test_mainsim.cpp test_simtick.cpp test_engine_bind.cpp test_block.cpp test_model.cpp
    test_entity.cpp test_movie.cpp test_flic.cpp)
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
  set(GAME_TESTS level objective savegame mission entity cmd select ai render bind frontend options)
  set(GAME_TEST_SOURCES test_level.cpp test_objective.cpp test_savegame.cpp test_mission.cpp
    test_entity.cpp test_cmd.cpp test_select.cpp test_ai.cpp test_render.cpp test_bind.cpp
    test_frontend.cpp test_options.cpp)
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
  set_tests_properties(audio PROPERTIES ENVIRONMENT "SDL_AUDIODRIVER=dummy")
endif()
