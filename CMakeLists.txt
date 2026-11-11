# SPDX-License-Identifier: LGPL-3.0-or-later
cmake_minimum_required(VERSION 3.20)

# The original LegoRR executable has no MSVCRT.DLL import table entry, so its
# VC5 CRT is linked into the image. Reccmp has matched CRT objects from the
# VC5 single-thread static runtime (/ML, LIBC.LIB).
if(LRR_CLASSIC_MSVC5)
  if(POLICY CMP0091)
    cmake_policy(SET CMP0091 NEW)
  endif()
  # CMake has no model for VC5's single-thread /ML runtime. Disable its
  # default /MD selection; the classic-profile flags below supply /ML.
  set(CMAKE_MSVC_RUNTIME_LIBRARY "")
endif()

project(lrr-portable VERSION 0.1.0 LANGUAGES CXX)
# The matching build compiles units rebuilt in original shape as C, as the
# original was (ADR 0005). Other builds compile those sources as C++.
if(LRR_CLASSIC_MSVC5)
  enable_language(C)
endif()

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
option(LRR_RECCMP "Generate a link PDB for Reccmp comparisons" OFF)

if(LRR_RECCMP AND NOT LRR_CLASSIC_MSVC5)
  message(FATAL_ERROR "LRR_RECCMP requires LRR_CLASSIC_MSVC5")
endif()

if(LRR_CLASSIC_MSVC5)
  # CMake's Windows-MSVC module emits newer spellings that the classic
  # toolchain's ReproBit command model does not admit.
  include_directories(BEFORE "${CMAKE_CURRENT_SOURCE_DIR}/src/compat/msvc5")

  foreach(_lang IN ITEMS C CXX)
    string(REGEX REPLACE "[ /-][Mm][DdTtLl][Dd]?" "" _classic_flags
      " ${CMAKE_${_lang}_FLAGS}")
    string(REGEX REPLACE "[ /]Zm[0-9]+" "" _classic_flags "${_classic_flags}")
    string(STRIP "${_classic_flags}" _classic_flags)
    string(APPEND _classic_flags " /ML")
    set(CMAKE_${_lang}_FLAGS "${_classic_flags}" CACHE STRING
      "Classic MSVC 5.0 compatible compiler flags" FORCE)
  endforeach()

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

  if(LRR_RECCMP)
    # Reccmp reads the linker PDB, so retain CMake's /PDB:<TARGET_PDB> rule
    # and add /DEBUG explicitly after filtering newer CMake/MSVC flags above.
    # The matching build links without incremental mode (ADR 0006 decision
    # 2): its thunks change call sites and it adds a .reloc section the
    # original lacks. The trailing /INCREMENTAL:NO overrides the default that
    # /DEBUG and CMake's RelWithDebInfo flags select. The program target also
    # writes a /MAP, which by itself would disable incremental mode (LNK4075).
    string(APPEND CMAKE_CXX_LINK_EXECUTABLE " /DEBUG /INCREMENTAL:NO")
  else()
    # ReproBit's current evidence model treats PDBs as compile-stage
    # artifacts, so omit the link PDB and its /DEBUG switch for that profile.
    string(REGEX REPLACE "[ /][Pp][Dd][Bb]:<TARGET_PDB>" ""
      _classic_exe_link_rule "${CMAKE_CXX_LINK_EXECUTABLE}")
    set(CMAKE_CXX_LINK_EXECUTABLE "${_classic_exe_link_rule}")
  endif()

  # CMake's MSVC rule shares one compiler PDB per target. Give each object
  # its own PDB so ReproBit can model the compile steps as isolated producers.
  set(CMAKE_CXX_COMPILE_OBJECT
    "<CMAKE_CXX_COMPILER> /nologo /TP <DEFINES> <INCLUDES> <FLAGS> /Fo<OBJECT> /Fd<OBJECT>.pdb -c <SOURCE>")
  set(CMAKE_C_COMPILE_OBJECT
    "<CMAKE_C_COMPILER> /nologo /TC <DEFINES> <INCLUDES> <FLAGS> /Fo<OBJECT> /Fd<OBJECT>.pdb -c <SOURCE>")
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
set(CRT_DIR "${SRC_ROOT}/crt")
set(PLATFORM_DIR "${SRC_ROOT}/platform")

# The link-order list (ADR 0006): every game, gods98, platform and program
# source, in the matching build's link order, with its library.
get_filename_component(LRR_SRC_ROOT_ABS "${SRC_ROOT}" ABSOLUTE
  BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(LRR_LINK_ORDER_FILE "${LRR_SRC_ROOT_ABS}/link_order.cmake")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
  "${LRR_LINK_ORDER_FILE}")
set(LRR_LINK_SOURCES "")
set(LRR_LINK_TARGETS "")
set(LRR_ORIGINAL_SHAPE_SOURCES "")
foreach(_lib IN ITEMS game gods98 platform_glue program)
  set(LRR_LINK_SOURCES_${_lib} "")
endforeach()
macro(lrr_link kind key library source)
  if("${ARGN}" STREQUAL "" OR "${ARGN}" STREQUAL "${PLATFORM}")
    list(APPEND LRR_LINK_SOURCES "${SRC_ROOT}/${source}")
    list(APPEND LRR_LINK_TARGETS "${library}")
    list(APPEND LRR_LINK_SOURCES_${library} "${SRC_ROOT}/${source}")
    # Units rebuilt in original shape (ADR 0005) are the list's .c files.
    if("${source}" MATCHES "\\.c$")
      list(APPEND LRR_ORIGINAL_SHAPE_SOURCES "${SRC_ROOT}/${source}")
    endif()
  endif()
endmacro()
include("${LRR_LINK_ORDER_FILE}")

# Units rebuilt in original shape are C in the matching build and C++
# elsewhere, because the native port's miniwin headers are C++-only.
if(NOT LRR_CLASSIC_MSVC5)
  set_source_files_properties(${LRR_ORIGINAL_SHAPE_SOURCES} PROPERTIES
    LANGUAGE CXX)
endif()

if(LRR_CLASSIC_MSVC5)
  if(NOT PLATFORM STREQUAL "winapi")
    message(FATAL_ERROR "LRR_CLASSIC_MSVC5 builds PLATFORM=winapi only")
  endif()
  # The matching build links one object per list entry, explicitly and in
  # list order, then the list's libraries (ADR 0006 decision 1). LINK 5.0
  # keeps command-line object order in every section. Each object keeps the
  # include path of its library in the other builds.
  set(LRR_LINK_INCLUDES_game ${SRC_ROOT} ${GAME_DIR} ${ENGINE_DIR})
  set(LRR_LINK_INCLUDES_program ${SRC_ROOT} ${GAME_DIR} ${ENGINE_DIR})
  set(LRR_LINK_INCLUDES_gods98 ${SRC_ROOT} ${ENGINE_DIR} ${GAME_DIR})
  set(LRR_LINK_INCLUDES_platform_glue ${SRC_ROOT})
  set(LRR_LINK_OBJECTS "")
  set(_lrr_index 0)
  foreach(_src _lib IN ZIP_LISTS LRR_LINK_SOURCES LRR_LINK_TARGETS)
    math(EXPR _lrr_index "${_lrr_index} + 1")
    get_filename_component(_stem "${_src}" NAME_WE)
    string(LENGTH "00${_lrr_index}" _len)
    math(EXPR _len "${_len} - 3")
    string(SUBSTRING "00${_lrr_index}" ${_len} 3 _pos)
    set(_target "link_${_pos}_${_stem}")
    add_library(${_target} OBJECT ${_src})
    target_include_directories(${_target} PRIVATE ${LRR_LINK_INCLUDES_${_lib}})
    list(APPEND LRR_LINK_OBJECTS "$<TARGET_OBJECTS:${_target}>")
  endforeach()
else()
  # The native and reference builds group the list's sources into libraries
  # (ADR 0003); their order does not matter there.
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
endif()

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
  target_link_libraries(platform_glue PUBLIC miniwin)

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

if(LRR_CLASSIC_MSVC5)
  set(LRR_SOURCES ${LRR_LINK_OBJECTS})
else()
  # These game sources register hooks that nothing references, so the
  # program compiles them itself to keep them in the static link.
  set(LRR_SOURCES ${LRR_LINK_SOURCES_program}
    ${GAME_DIR}/seh_runtime.cpp
    ${GAME_DIR}/dependency_grid.cpp ${GAME_DIR}/screen_pick.cpp)
endif()
if(PLATFORM STREQUAL "winapi")
  list(APPEND LRR_SOURCES ${SRC_ROOT}/version.rc)
endif()
add_executable(program ${LRR_SOURCES})
# Keep the shipped executable name while using ReproBit's lowercase target ID.
set_target_properties(program PROPERTIES OUTPUT_NAME "LegoRR")
if(LRR_RECCMP)
  set_target_properties(program PROPERTIES
    PDB_NAME "LegoRR"
    PDB_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
  # The link map records the object contributions in link order (ADR 0006).
  target_link_options(program PRIVATE "/MAP:${CMAKE_BINARY_DIR}/LegoRR.map")
endif()
if(LRR_CLASSIC_MSVC5)
  target_link_libraries(program PRIVATE ${LRR_LINK_LIBRARIES})
  # Name the entry point up front. LIBC is searched before the linker would
  # infer it, and LIBC.LIB's symbol index resolves the startup data shared by
  # both entry objects to the console one (crt0.obj) first.
  target_link_options(program PRIVATE "/ENTRY:WinMainCRTStartup")
else()
  # The frame-loop object in gods98 calls into game, while game itself uses
  # gods98 APIs. Repeat the archive so GNU ld resolves this static-library
  # cycle.
  target_link_libraries(program PRIVATE game gods98 game platform_glue)
  if(PLATFORM STREQUAL "winapi")
    target_link_libraries(program PRIVATE winmm d3drm advapi32)
  endif()
endif()
if(WIN32)
  set_target_properties(program PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

include(CTest)
if(BUILD_TESTING AND PLATFORM STREQUAL "native")
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
    bezier)
  set(GAME_TEST_SOURCES test_level.cpp test_objective.cpp test_savegame.cpp test_mission.cpp
    test_entity.cpp test_cmd.cpp test_select.cpp test_ai.cpp test_render.cpp test_bind.cpp
    test_frontend.cpp test_options.cpp test_bezier.cpp)
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
  endif()
  set_tests_properties(audio PROPERTIES ENVIRONMENT "SDL_AUDIODRIVER=dummy")
endif()
