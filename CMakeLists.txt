# SPDX-License-Identifier: LGPL-3.0-or-later
cmake_minimum_required(VERSION 3.20)

project(lrr-portable VERSION 1.0.0 LANGUAGES C CXX)

# The pre-release label (docs/releasing.md): empty for a release, or a label
# such as beta1 for a pre-release of project(VERSION), which makes the
# version 1.0.0-beta1. project(VERSION) takes numbers only, so the label is
# set here, and LRR_VERSION is the full version that packages and the game
# report.
set(LRR_VERSION_PRERELEASE "beta3")
if(LRR_VERSION_PRERELEASE STREQUAL "")
  set(_lrr_version "${PROJECT_VERSION}")
else()
  set(_lrr_version "${PROJECT_VERSION}-${LRR_VERSION_PRERELEASE}")
endif()
set(LRR_VERSION "${_lrr_version}" CACHE INTERNAL
  "The full version: project(VERSION) and the pre-release label")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# No fused multiply-add: the original's x87 code rounds every product, and
# miniwin's rasterizer needs a shared edge to evaluate to exactly opposite
# values from its two triangles. Clang contracts a * b + c by default where
# the target has FMA (arm64), and GCC can too. MSVC's /fp:precise (its
# default, stated here) does not contract since Visual Studio 2022;
# /fp:contract would.
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options(-ffp-contract=off)
elseif(MSVC)
  add_compile_options(/fp:precise)
endif()

# Line coverage (docs/building.md, "Coverage"): every target compiles and
# links with gcov instrumentation, and each test run adds to the .gcda
# counts in the build tree. tools/coverage.py reports them. GCC only, since
# the report reads the counts with gcov.
option(LRR_COVERAGE "Instrument the build for gcov line coverage" OFF)
if(LRR_COVERAGE)
  if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    message(FATAL_ERROR "LRR_COVERAGE needs GCC; the compiler is ${CMAKE_CXX_COMPILER_ID}")
  endif()
  add_compile_options(--coverage)
  add_link_options(--coverage)
endif()

# The web build (issue #926), with isle-portable's Emscripten flags (its
# CMakeLists.txt, LGPL-3.0) less OffscreenCanvas. -sPROXY_TO_PTHREAD runs
# main, and so WinMain's blocking loop and the units' nested modal loops,
# on a worker thread, where blocking is allowed. The canvas stays on the
# page: a WebGL canvas, the page's or one transferred to the worker, shows
# a frame only when its thread returns to the browser, which WinMain never
# does. So the web build draws with miniwin's software renderer through
# SDL's software render driver (entry.cpp), which presents each frame on
# the page's canvas from the main browser thread, and leaves out miniwin's
# OpenGL ES backends, whose device enumeration would claim the canvas for
# WebGL (docs/building.md: WebGL 2 needs a main loop that returns to the
# browser between frames). SDL's DOM event handlers run on the main
# browser thread and queue the events for the worker. Every object compiles
# with -pthread, since the threads share one wasm memory. WASMFS and
# FORCE_FILESYSTEM back the OPFS saves and the streamed game data
# (emscripten/filesystem.cpp).
#
# The game data is streamed over a WasmFS fetch backend from
# <LRR_EMSCRIPTEN_HOST>/LegoRR (issue #908), as isle-portable streams from
# ISLE_EMSCRIPTEN_HOST/LEGO: a URL prefix such as https://example.com, empty
# for the page's own origin. A file over twice LRR_EMSCRIPTEN_CHUNK_SIZE
# (bytes, at most 2147483647: the backend reads it as a signed 32-bit
# number) is fetched in chunks of that size by range requests, a smaller one
# whole on first use. LRR_EMSCRIPTEN_PRELOAD_WADS fetches the WADs at start,
# a few chunks per request, sending the page "dataProgress" events.
if(EMSCRIPTEN)
  set(LRR_EMSCRIPTEN_HOST "" CACHE STRING
    "Host URL the web build streams the game data from, e.g. https://example.com")
  set(LRR_EMSCRIPTEN_CHUNK_SIZE 524288 CACHE STRING
    "Bytes per range request when the web build streams a large game file (the WADs)")
  if(LRR_EMSCRIPTEN_CHUNK_SIZE LESS 1 OR LRR_EMSCRIPTEN_CHUNK_SIZE GREATER 2147483647)
    message(FATAL_ERROR
      "LRR_EMSCRIPTEN_CHUNK_SIZE must be from 1 to 2147483647, not ${LRR_EMSCRIPTEN_CHUNK_SIZE}")
  endif()
  option(LRR_EMSCRIPTEN_PRELOAD_WADS
    "The web build fetches the WADs at start, with progress events for the page" ON)
endif()
if(EMSCRIPTEN)
  add_compile_options(-pthread)
  add_link_options(-pthread -sPROXY_TO_PTHREAD=1 -sPTHREAD_POOL_SIZE_STRICT=0
    -sALLOW_MEMORY_GROWTH=1 -sINITIAL_MEMORY=128mb -sMAXIMUM_MEMORY=2gb
    -sFORCE_FILESYSTEM=1 -sWASMFS=1 -sEXIT_RUNTIME=1
    -sABORT_ON_WASM_EXCEPTIONS=1)
  set(SDL_PTHREADS ON CACHE BOOL "Enable SDL pthreads" FORCE)
  set(ISLE_RENDERER_OPENGLES OFF CACHE BOOL "Build miniwin's OpenGL ES backends" FORCE)
endif()

# MinGW links its C and C++ runtimes and winpthread statically, as
# isle-portable does, so LegoRR.exe runs outside the msys2 shell.
if(MINGW)
  add_link_options(-static -static-libgcc -static-libstdc++)
endif()

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
# miniwin is upstream isle-portable plus patches/miniwin (ADR 0009). Every
# build checks that the series is applied, so a changed patch or a
# `git submodule update` cannot build unpatched sources.
find_program(LRR_BASH bash)
# The Flatpak build (packaging/linux/flatpak) copies miniwin out of a checkout
# that setup-miniwin.sh already patched, without the git directory that the
# check reads.
option(LRR_CHECK_MINIWIN_PATCHES "Check that patches/miniwin is applied" ON)
if(LRR_BASH AND LRR_CHECK_MINIWIN_PATCHES)
  execute_process(COMMAND "${LRR_BASH}" tools/miniwin-patches.sh check
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" RESULT_VARIABLE LRR_MINIWIN_PATCHED)
  if(NOT LRR_MINIWIN_PATCHED EQUAL 0)
    message(FATAL_ERROR "miniwin's patches are not applied; run tools/setup-miniwin.sh")
  endif()
  add_custom_target(miniwin_patches_check
    COMMAND "${LRR_BASH}" tools/miniwin-patches.sh check
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMENT "Checking patches/miniwin is applied")
endif()
# SDL3: an installed package when there is one, otherwise the same pinned
# release CI builds (3.4.18, commit 829a65d7), fetched as iniparser and
# miniaudio are below. The console hosts take isle-portable's per-platform
# forks; on Android, Gradle builds SDL as an AAR and CMake only reads its
# package.
find_package(SDL3 CONFIG QUIET)
if(NOT TARGET SDL3::SDL3)
  include(FetchContent)
  if(ANDROID)
    # Built by Gradle.
    find_package(SDL3 CONFIG REQUIRED COMPONENTS Shared)
  else()
    if(WINDOWS_STORE)
      FetchContent_Declare(
        SDL3
        GIT_REPOSITORY "https://github.com/Helloyunho/SDL3-uwp.git"
        GIT_TAG "main"
        EXCLUDE_FROM_ALL
      )
    elseif(NINTENDO_SWITCH)
      FetchContent_Declare(
        SDL3
        GIT_REPOSITORY "https://github.com/SnepOMatic/SDL3-Switch.git"
        GIT_TAG "switch-sdl-3.2.14"
        EXCLUDE_FROM_ALL
      )
      set(CMAKE_DISABLE_PRECOMPILE_HEADERS ON CACHE BOOL "Disable PCH globally" FORCE)
    else()
      # Keep the commit in step with SDL_COMMIT in native-build.yml.
      set(LRR_SDL3_GIT_REPO "https://github.com/libsdl-org/SDL.git" CACHE STRING "The SDL3 git repo")
      set(LRR_SDL3_GIT_TAG "829a65d769d935c4852f8159e964312c0957260a" CACHE STRING "The SDL3 git tag or commit")
      FetchContent_Declare(
        SDL3
        GIT_REPOSITORY "${LRR_SDL3_GIT_REPO}"
        GIT_TAG "${LRR_SDL3_GIT_TAG}"
        UPDATE_DISCONNECTED TRUE
        EXCLUDE_FROM_ALL
      )
    endif()
    # SDL keeps its own defaults otherwise, so a desktop build gets its
    # video drivers; a host without X11 or Wayland development libraries
    # configures with -DSDL_X11=OFF -DSDL_WAYLAND=OFF
    # -DSDL_UNIX_CONSOLE_BUILD=ON, as CI does. A static SDL leaves no
    # SDL3.dll to place beside each executable. The function scope keeps
    # these settings to SDL's own build.
    function(lrr_fetch_sdl3)
      set(BUILD_TESTING OFF)
      set(SDL_TESTS OFF)
      set(SDL_EXAMPLES OFF)
      set(SDL_SHARED OFF)
      set(SDL_STATIC ON)
      FetchContent_MakeAvailable(SDL3)
    endfunction()
    lrr_fetch_sdl3()
  endif()
endif()

if(IOS)
  add_compile_definitions(IOS)
endif()

# iniparser, for the port's configuration file ([library:config],
# src/platform/native/config.h), as isle-portable reads isle.ini: an
# installed package when there is one, otherwise the same pinned release
# isle-portable fetches.
add_library(lrr_iniparser INTERFACE)
find_package(iniparser CONFIG QUIET)
if(TARGET iniparser-static)
  target_link_libraries(lrr_iniparser INTERFACE iniparser-static)
elseif(TARGET iniparser-shared)
  target_link_libraries(lrr_iniparser INTERFACE iniparser-shared)
else()
  include(FetchContent)
  FetchContent_Declare(
    iniparser
    GIT_REPOSITORY "https://gitlab.com/iniparser/iniparser.git"
    GIT_TAG "v4.2.6"
    GIT_SHALLOW TRUE
    UPDATE_DISCONNECTED TRUE
    EXCLUDE_FROM_ALL
  )
  # A function scope keeps these settings to iniparser's own build.
  function(lrr_fetch_iniparser)
    set(BUILD_DOCS OFF)
    set(BUILD_EXAMPLES OFF)
    set(BUILD_TESTING OFF)
    set(BUILD_SHARED_LIBS OFF)
    FetchContent_MakeAvailable(iniparser)
  endfunction()
  lrr_fetch_iniparser()
  target_link_libraries(lrr_iniparser INTERFACE iniparser-static)
endif()
# miniwin builds through upstream's own miniwin/CMakeLists.txt, which picks
# each platform's renderer backends: software and palette software, SDL GPU,
# OpenGL 1.x and OpenGL ES 2/3 where their libraries are found, DirectX 9 on
# Windows, Citro3D on 3DS, GXM on Vita and Glide on DOS
# (ISLE_RENDERER_SOFTWARE and ISLE_RENDERER_SDLGPU switch the software and SDL
# GPU backends). On the desktop the software backend is the last hardware
# device miniwin enumerates, so it stays the device the game picks. The
# patches implement immediate-mode drawing on it and on SDL GPU, which
# LegoRR.ini's "3d device" selects.
add_subdirectory("${MINIWIN}" "${CMAKE_CURRENT_BINARY_DIR}/miniwin")
if(NOT ISLE_RENDERER_SOFTWARE)
  message(WARNING "ISLE_RENDERER_SOFTWARE is OFF: the game will pick another backend, and of the others only SDL GPU draws the level")
endif()
# The machine miniwin emulates for the game: a 16-bit RGB565 display (the
# units convert images to 16-bit only), and the software renderer described as
# a hardware device, since Init.c's -best accepts only HAL devices.
target_compile_definitions(miniwin PRIVATE
  MINIWIN_PIXELFORMAT=SDL_PIXELFORMAT_RGB565 MINIWIN_SOFTWARE_HAL)
# Upstream links SDL privately; the units and tests include SDL beside it.
target_link_libraries(miniwin PUBLIC SDL3::SDL3)
if(TARGET miniwin_patches_check)
  add_dependencies(miniwin miniwin_patches_check)
endif()

# The units, in one library (ADR 0003's game/gods98 split is a source
# grouping only: the units call each other both ways).
add_library(units STATIC ${LRR_UNIT_SOURCES})
target_include_directories(units PUBLIC ${SRC_ROOT} ${ENGINE_DIR} ${GAME_DIR})
target_compile_definitions(units PUBLIC MINIWIN)
target_link_libraries(units PUBLIC miniwin SDL3::SDL3)

# The libraries the units' substitutions call (ADR 0007, decision 3).
# The video library loads FFmpeg at run time, trying each library's
# comma-separated file names in order, so the build needs no FFmpeg package;
# without the libraries at run time the units skip their movies.
# LRR_MOVIES off builds the video library as a stub that always reports
# "no movies" (issue #919), for the hosts whose runtimes carry no FFmpeg.
include(CMakeDependentOption)
cmake_dependent_option(LRR_MOVIES
  "Build the FFmpeg movie library; off builds a no-movies stub" ON
  "NOT EMSCRIPTEN;NOT IOS;NOT ANDROID;NOT NINTENDO_3DS;NOT NINTENDO_SWITCH;NOT VITA;NOT DOS" OFF)
if(LRR_MOVIES)
  if(APPLE)
    set(LRR_VIDEO_LIBAVFORMAT "libavformat.63.dylib,libavformat.62.dylib,libavformat.61.dylib,libavformat.dylib")
    set(LRR_VIDEO_LIBAVCODEC "libavcodec.63.dylib,libavcodec.62.dylib,libavcodec.61.dylib,libavcodec.dylib")
    set(LRR_VIDEO_LIBAVUTIL "libavutil.61.dylib,libavutil.60.dylib,libavutil.59.dylib,libavutil.dylib")
    set(LRR_VIDEO_LIBSWSCALE "libswscale.10.dylib,libswscale.9.dylib,libswscale.8.dylib,libswscale.dylib")
    # The FFmpeg the macOS packages bundle (packaging/macos/ffmpeg/build.sh):
    # its prefix. Its four libraries go into LegoRR.app/Contents/Frameworks
    # (packaging/macos), and the game tries them there before the system's.
    set(LRR_FFMPEG_DIR "" CACHE PATH
      "An FFmpeg prefix from packaging/macos/ffmpeg/build.sh to bundle in LegoRR.app")
    set(LRR_BUNDLED_FFMPEG_LIBS "")
    if(LRR_FFMPEG_DIR)
      foreach(_lrr_lib IN ITEMS avformat avcodec avutil swscale)
        # lib<name>.<major>.dylib, the name the libraries link each other by.
        file(GLOB _lrr_candidates "${LRR_FFMPEG_DIR}/lib/lib${_lrr_lib}.*.dylib")
        set(_lrr_found "")
        foreach(_lrr_candidate IN LISTS _lrr_candidates)
          get_filename_component(_lrr_name "${_lrr_candidate}" NAME)
          if(_lrr_name MATCHES "^lib${_lrr_lib}\\.[0-9]+\\.dylib$")
            set(_lrr_found "${_lrr_candidate}")
          endif()
        endforeach()
        if(NOT _lrr_found)
          message(FATAL_ERROR "LRR_FFMPEG_DIR: no lib${_lrr_lib}.<major>.dylib in ${LRR_FFMPEG_DIR}/lib")
        endif()
        get_filename_component(_lrr_name "${_lrr_found}" NAME)
        string(TOUPPER "LIB${_lrr_lib}" _lrr_key)
        set(LRR_VIDEO_${_lrr_key} "@executable_path/../Frameworks/${_lrr_name},${LRR_VIDEO_${_lrr_key}}")
        list(APPEND LRR_BUNDLED_FFMPEG_LIBS "${_lrr_found}")
      endforeach()
      if(NOT EXISTS "${LRR_FFMPEG_DIR}/share/ffmpeg/COPYING.LGPLv2.1")
        message(FATAL_ERROR "LRR_FFMPEG_DIR: no share/ffmpeg/COPYING.LGPLv2.1 in ${LRR_FFMPEG_DIR}")
      endif()
    endif()
  elseif(WIN32)
    # The Windows builds' DLL names, for the same library majors as the
    # sonames below (FFmpeg 7 and later).
    set(LRR_VIDEO_LIBAVFORMAT "avformat-63.dll,avformat-62.dll,avformat-61.dll")
    set(LRR_VIDEO_LIBAVCODEC "avcodec-63.dll,avcodec-62.dll,avcodec-61.dll")
    set(LRR_VIDEO_LIBAVUTIL "avutil-61.dll,avutil-60.dll,avutil-59.dll")
    set(LRR_VIDEO_LIBSWSCALE "swscale-10.dll,swscale-9.dll,swscale-8.dll")
  else()
    set(LRR_VIDEO_LIBAVFORMAT "libavformat.so.63,libavformat.so.62,libavformat.so.61,libavformat.so")
    set(LRR_VIDEO_LIBAVCODEC "libavcodec.so.63,libavcodec.so.62,libavcodec.so.61,libavcodec.so")
    set(LRR_VIDEO_LIBAVUTIL "libavutil.so.61,libavutil.so.60,libavutil.so.59,libavutil.so")
    set(LRR_VIDEO_LIBSWSCALE "libswscale.so.10,libswscale.so.9,libswscale.so.8,libswscale.so")
  endif()
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
if(NINTENDO_3DS)
  # newlib on 3DS has no dlfcn.h, and the decoders load nothing at run time.
  target_compile_definitions(library PRIVATE MA_NO_RUNTIME_LINKING)
endif()
set_source_files_properties("${SRC_ROOT}/library/miniaudio_impl.c" PROPERTIES
  COMPILE_OPTIONS "$<IF:$<C_COMPILER_ID:MSVC>,/w,-w>")
if(LRR_MOVIES)
  foreach(_name IN ITEMS LIBAVFORMAT LIBAVCODEC LIBAVUTIL LIBSWSCALE)
    target_compile_definitions(library PRIVATE
      "LRR_VIDEO_${_name}=\"${LRR_VIDEO_${_name}}\"")
  endforeach()
  target_compile_definitions(library PRIVATE LRR_MOVIES=1)
else()
  target_compile_definitions(library PRIVATE LRR_MOVIES=0)
endif()
# devkitARM's 3DS toolchain names libdl, which newlib does not have.
if(NOT NINTENDO_3DS)
  target_link_libraries(library PUBLIC ${CMAKE_DL_LIBS})
endif()
target_link_libraries(library PUBLIC miniwin SDL3::SDL3)
if(UNIX)
  target_link_libraries(library PUBLIC m)
endif()
target_link_libraries(units PUBLIC library)

# The seam's remaining stubs (ADR 0007): the imports neither miniwin nor a
# library substitution provides yet.
set(LRR_ENTRY_SOURCE "${PLATFORM_DIR}/native/entry.cpp")
set(LRR_SEAM_SOURCES ${LRR_LINK_SOURCES_platform})
list(REMOVE_ITEM LRR_SEAM_SOURCES "${LRR_ENTRY_SOURCE}")
# The hosts whose files live elsewhere: their configuration defaults and,
# on the web, the OPFS save directory and the page's save events
# (src/platform/native/config.h). Adapted from isle-portable's ISLE/<host>/.
if(NINTENDO_3DS)
  list(APPEND LRR_SEAM_SOURCES "${PLATFORM_DIR}/native/3ds/config.cpp"
    "${PLATFORM_DIR}/native/3ds/apthooks.cpp")
endif()
if(NINTENDO_SWITCH)
  list(APPEND LRR_SEAM_SOURCES "${PLATFORM_DIR}/native/switch/config.cpp")
endif()
if(IOS)
  list(APPEND LRR_SEAM_SOURCES "${PLATFORM_DIR}/native/ios/config.cpp")
endif()
if(ANDROID)
  list(APPEND LRR_SEAM_SOURCES "${PLATFORM_DIR}/native/android/config.cpp")
endif()
if(EMSCRIPTEN)
  list(APPEND LRR_SEAM_SOURCES
    "${PLATFORM_DIR}/native/emscripten/config.cpp"
    "${PLATFORM_DIR}/native/emscripten/events.cpp"
    "${PLATFORM_DIR}/native/emscripten/filesystem.cpp"
    "${PLATFORM_DIR}/native/emscripten/manifest.cpp")
endif()
add_library(seam_stubs STATIC ${LRR_SEAM_SOURCES})
target_include_directories(seam_stubs PUBLIC ${SRC_ROOT})
target_link_libraries(seam_stubs PUBLIC miniwin lrr_iniparser)
if(EMSCRIPTEN)
  # The OPFS and fetch backends are WasmFS ones (emscripten/filesystem.cpp).
  target_link_options(seam_stubs PUBLIC "-sWASMFS=1" "-sFORCE_FILESYSTEM=1")
  # Only filesystem.cpp reads the three settings, so changing one rebuilds
  # that file and not the units.
  set_source_files_properties("${PLATFORM_DIR}/native/emscripten/filesystem.cpp"
    PROPERTIES COMPILE_DEFINITIONS
    "LRR_EMSCRIPTEN_HOST=\"${LRR_EMSCRIPTEN_HOST}\";LRR_EMSCRIPTEN_CHUNK_SIZE=${LRR_EMSCRIPTEN_CHUNK_SIZE};LRR_EMSCRIPTEN_PRELOAD_WADS=$<BOOL:${LRR_EMSCRIPTEN_PRELOAD_WADS}>")
endif()
target_link_libraries(units PUBLIC seam_stubs)

if(ANDROID)
  # SDL's Java activity loads the program as libLegoRR.so and calls its
  # SDL_main on SDL's main thread, where WinMain's loop may block
  # (packaging/android; isle-portable overrides add_executable the same way).
  add_library(program SHARED ${LRR_ENTRY_SOURCE})
else()
  add_executable(program ${LRR_ENTRY_SOURCE})
endif()
set_target_properties(program PROPERTIES OUTPUT_NAME "LegoRR")
target_link_libraries(program PRIVATE units)

# The version the game reports (--version and the log; docs/releasing.md).
# project(VERSION) and LRR_VERSION_PRERELEASE above are the one place the
# version is set, and a release is tagged v<version>. tools/version.cmake writes lrr_version.h at every
# build: the plain version at the release tag, <version>-dev+g<commit>
# elsewhere in a git checkout, and the plain version without git.
# LRR_VERSION_OVERRIDE, when set, is reported instead.
set(LRR_VERSION_OVERRIDE "" CACHE STRING
  "Version string the game reports, in place of the one tools/version.cmake derives")
set(LRR_VERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/generated/lrr_version.h")
add_custom_target(lrr_version
  COMMAND "${CMAKE_COMMAND}"
    "-DSOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}"
    "-DVERSION=${LRR_VERSION}"
    "-DOVERRIDE=${LRR_VERSION_OVERRIDE}"
    "-DOUTPUT=${LRR_VERSION_HEADER}"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/tools/version.cmake"
  BYPRODUCTS "${LRR_VERSION_HEADER}"
  COMMENT "Writing the version header"
  VERBATIM)
add_dependencies(program lrr_version)
target_include_directories(program PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated")

# Lint: compiler warnings this repository's own code is kept free of, as
# errors, so the native-build check fails on a new one. Third-party code
# (miniwin, miniaudio, SDL) is not covered. Add a warning here only once the
# code builds cleanly with it.
#   parentheses / C4706: an assignment used as a condition, as in
#     if (x = f()). Assign on its own line and compare: x = f(); if (x != NULL).
#     In a loop condition, write while ((x = next(x)) != NULL).
set(LRR_LINT_WARNINGS
  $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Werror=parentheses>
  $<$<CXX_COMPILER_ID:MSVC>:/we4706>)
foreach(_target IN ITEMS units seam_stubs program)
  target_compile_options(${_target} PRIVATE ${LRR_LINT_WARNINGS})
endforeach()
if(WIN32)
  set_target_properties(program PROPERTIES WIN32_EXECUTABLE TRUE)
  # A shared SDL3 (an installed package; the fetched one is static) goes
  # beside LegoRR.exe, as isle-portable copies its runtime DLLs. The tests
  # build into the same directory and find it there too.
  add_custom_command(TARGET program POST_BUILD
    COMMAND "$<$<BOOL:$<TARGET_RUNTIME_DLLS:program>>:${CMAKE_COMMAND};-E;copy_if_different;$<TARGET_RUNTIME_DLLS:program>;$<TARGET_FILE_DIR:program>>"
    COMMAND_EXPAND_LISTS)
endif()
if(EMSCRIPTEN)
  # LegoRR.html, beside LegoRR.js and LegoRR.wasm, from the port's page.
  set(LRR_EMSCRIPTEN_SHELL "${LRR_SRC_ROOT_ABS}/platform/native/emscripten/shell.html")
  set_target_properties(program PROPERTIES SUFFIX ".html"
    LINK_DEPENDS "${LRR_EMSCRIPTEN_SHELL}")
  target_link_options(program PRIVATE "--shell-file=${LRR_EMSCRIPTEN_SHELL}")
  # WebAssembly checks each call's signature, so a unit that declares
  # another's function with other parameters links to a trap; wasm-ld only
  # warns ("function signature mismatch"). Make that an error.
  target_link_options(program PRIVATE "LINKER:--fatal-warnings")
endif()
if(NINTENDO_3DS)
  # LegoRR.3dsx, and LegoRR.cia where bannertool and makerom are found
  # (packaging/3ds).
  add_subdirectory(packaging/3ds)
endif()

include(CTest)
if(BUILD_TESTING)
  # lrr_unit_test (cmake/LrrTesting.cmake): one call per test executable,
  # registered in the tests.cmake beside the test's source.
  include("${PROJECT_SOURCE_DIR}/cmake/LrrTesting.cmake")
  foreach(_lrr_test_dir IN ITEMS game crt gods98 platform/native library)
    include("${LRR_SRC_ROOT_ABS}/${_lrr_test_dir}/tests.cmake")
  endforeach()
endif()

# Install rules and CPack packages (packaging/, issue #922): on by default for
# the desktop hosts, where the packages are built.
if(CMAKE_SYSTEM_NAME MATCHES "^(Linux|Darwin|FreeBSD|Windows)$")
  set(_lrr_packaging_default ON)
else()
  set(_lrr_packaging_default OFF)
endif()
option(LRR_PACKAGING "Add install rules and CPack packages" ${_lrr_packaging_default})
if(LRR_PACKAGING)
  add_subdirectory(packaging)
endif()
# The bundled FFmpeg (LRR_FFMPEG_DIR) in the build tree's LegoRR.app too, so
# `make run` plays the movies. install() puts the same files in the package
# (packaging/CMakeLists.txt). LegoRR is a bundle only with packaging on
# (packaging/macos).
if(LRR_PACKAGING AND APPLE AND NOT IOS)
  foreach(_lrr_lib IN LISTS LRR_BUNDLED_FFMPEG_LIBS)
    get_filename_component(_lrr_name "${_lrr_lib}" NAME)
    get_filename_component(_lrr_real "${_lrr_lib}" REALPATH)
    add_custom_command(TARGET program POST_BUILD
      COMMAND "${CMAKE_COMMAND}" -E make_directory
        "$<TARGET_BUNDLE_CONTENT_DIR:program>/Frameworks"
      COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${_lrr_real}"
        "$<TARGET_BUNDLE_CONTENT_DIR:program>/Frameworks/${_lrr_name}")
  endforeach()
endif()
