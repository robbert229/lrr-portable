# SPDX-License-Identifier: LGPL-3.0-or-later
cmake_minimum_required(VERSION 3.20)

project(lrr-portable VERSION 0.1.0 LANGUAGES C CXX)

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
  "NOT EMSCRIPTEN;NOT IOS;NOT NINTENDO_3DS;NOT NINTENDO_SWITCH;NOT VITA;NOT DOS" OFF)
if(LRR_MOVIES)
  if(APPLE)
    set(LRR_VIDEO_LIBAVFORMAT "libavformat.63.dylib,libavformat.62.dylib,libavformat.61.dylib,libavformat.dylib")
    set(LRR_VIDEO_LIBAVCODEC "libavcodec.63.dylib,libavcodec.62.dylib,libavcodec.61.dylib,libavcodec.dylib")
    set(LRR_VIDEO_LIBAVUTIL "libavutil.61.dylib,libavutil.60.dylib,libavutil.59.dylib,libavutil.dylib")
    set(LRR_VIDEO_LIBSWSCALE "libswscale.10.dylib,libswscale.9.dylib,libswscale.8.dylib,libswscale.dylib")
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
# The hosts whose files live elsewhere: their configuration defaults and,
# on the web, the OPFS save directory and the page's save events
# (src/platform/native/config.h). Adapted from isle-portable's ISLE/<host>/.
if(NINTENDO_3DS)
  list(APPEND LRR_SEAM_SOURCES "${PLATFORM_DIR}/native/3ds/config.cpp")
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
    "${PLATFORM_DIR}/native/emscripten/filesystem.cpp")
endif()
add_library(seam_stubs STATIC ${LRR_SEAM_SOURCES})
target_include_directories(seam_stubs PUBLIC ${SRC_ROOT})
target_link_libraries(seam_stubs PUBLIC miniwin lrr_iniparser)
if(EMSCRIPTEN)
  # The OPFS backend is a WasmFS one (emscripten/filesystem.cpp).
  target_link_options(seam_stubs PUBLIC "-sWASMFS=1" "-sFORCE_FILESYSTEM=1")
endif()
target_link_libraries(units PUBLIC seam_stubs)

add_executable(program ${LRR_ENTRY_SOURCE})
set_target_properties(program PROPERTIES OUTPUT_NAME "LegoRR")
target_link_libraries(program PRIVATE units)
if(WIN32)
  set_target_properties(program PROPERTIES WIN32_EXECUTABLE TRUE)
  # A shared SDL3 (an installed package; the fetched one is static) goes
  # beside LegoRR.exe, as isle-portable copies its runtime DLLs. The tests
  # build into the same directory and find it there too.
  add_custom_command(TARGET program POST_BUILD
    COMMAND "$<$<BOOL:$<TARGET_RUNTIME_DLLS:program>>:${CMAKE_COMMAND};-E;copy_if_different;$<TARGET_RUNTIME_DLLS:program>;$<TARGET_FILE_DIR:program>>"
    COMMAND_EXPAND_LISTS)
endif()

include(CTest)
if(BUILD_TESTING)
  # The units' own tests: each calls one unit directly.
  foreach(name IN ITEMS bezier steering animation_object reward savegame)
    add_executable(${name}_test ${GAME_DIR}/test_${name}.cpp)
    target_link_libraries(${name}_test PRIVATE units)
    add_test(NAME ${name} COMMAND ${name}_test)
    set_tests_properties(${name} PROPERTIES LABELS ${name})
  endforeach()
  # The units' vsprintf prints "%b" as VC5 LIBC does, on every host.
  add_executable(vc5_vsprintf_test src/crt/test_vc5_vsprintf.cpp)
  target_include_directories(vc5_vsprintf_test PRIVATE ${CMAKE_SOURCE_DIR}/src)
  # vc5names.h compares without case through SDL.
  target_link_libraries(vc5_vsprintf_test PRIVATE SDL3::SDL3)
  add_test(NAME vc5_vsprintf COMMAND vc5_vsprintf_test)
  set_tests_properties(vc5_vsprintf PROPERTIES LABELS crt)
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
    # LegoObject.c's flock update reads the flock's own hasPosition, not
    # the getter the linker folded it into: the unit's calls to the Stats.c
    # and Container.c getters reach the test's __wrap_ stubs.
    add_executable(flock_position_test ${GAME_DIR}/test_flock_position.cpp)
    target_link_libraries(flock_position_test PRIVATE units)
    target_link_options(flock_position_test PRIVATE
      "LINKER:--wrap=entity_stats_word_get"
      "LINKER:--wrap=entity_stats_float_at120"
      "LINKER:--wrap=entity_stats_float_at124"
      "LINKER:--wrap=entity_stats_float_at128"
      "LINKER:--wrap=entity_stats_float_at12c"
      "LINKER:--wrap=entity_stats_float_at130"
      "LINKER:--wrap=entity_stats_float_at134"
      "LINKER:--wrap=entity_stats_float_at138"
      "LINKER:--wrap=ddiscene_object_position_get")
    add_test(NAME flock_position COMMAND flock_position_test)
    set_tests_properties(flock_position PROPERTIES LABELS legoobject)
    # LegoObject.c's run away frees only the route lists it allocated: the
    # unit's calls to free reach the test's __wrap_free, which records a
    # stack address instead of aborting on it.
    add_executable(run_away_test ${GAME_DIR}/test_run_away.cpp)
    target_link_libraries(run_away_test PRIVATE units)
    target_link_options(run_away_test PRIVATE
      "LINKER:--wrap=free"
      "LINKER:--wrap=ddiscene_object_position_get"
      "LINKER:--wrap=entity_stats_word_get"
      "LINKER:--wrap=entity_capability_flags_get"
      "LINKER:--wrap=entity_state_flags_get"
      "LINKER:--wrap=idle_register")
    add_test(NAME run_away COMMAND run_away_test)
    set_tests_properties(run_away PROPERTIES LABELS legoobject)
    # Info.c's $name$ tokens carry the whole image pointer to the panel
    # print and to Font's "%b": the units' image and panel calls reach the
    # test's __wrap_ stubs.
    add_executable(info_image_token_test ${GAME_DIR}/test_info_image_token.cpp)
    target_link_libraries(info_image_token_test PRIVATE units)
    target_link_options(info_image_token_test PRIVATE
      "LINKER:--wrap=Image_LoadBMPScaled"
      "LINKER:--wrap=Image_SetPenZeroTrans"
      "LINKER:--wrap=Image_DisplayScaled"
      "LINKER:--wrap=panel_text_window_print_f"
      "LINKER:--wrap=panel_state_flags_toggle"
      "LINKER:--wrap=ui_text_output_object_release_target"
      "LINKER:--wrap=ui_text_output_position_and_draw"
      "LINKER:--wrap=ui_record_callback_forward"
      "LINKER:--wrap=indexed_text_metrics_pair_set"
      "LINKER:--wrap=indexed_text_metrics_pair_get"
      "LINKER:--wrap=sound_submit_with_global_cooldown")
    add_test(NAME info_image_token COMMAND info_image_token_test)
    set_tests_properties(info_image_token PROPERTIES LABELS info)
    # The info panel prints a token's "%b" as the original's VC5 LIBC does,
    # through the real panel print, TextWindow and the units' vsprintf.
    add_executable(info_panel_percent_b_test ${GAME_DIR}/test_info_panel_percent_b.cpp)
    target_link_libraries(info_panel_percent_b_test PRIVATE units)
    target_link_options(info_panel_percent_b_test PRIVATE
      "LINKER:--wrap=Image_LoadBMPScaled"
      "LINKER:--wrap=Image_SetPenZeroTrans"
      "LINKER:--wrap=Image_DisplayScaled"
      "LINKER:--wrap=panel_state_flags_toggle"
      "LINKER:--wrap=ui_text_output_object_release_target"
      "LINKER:--wrap=ui_text_output_position_and_draw"
      "LINKER:--wrap=ui_record_callback_forward"
      "LINKER:--wrap=indexed_text_metrics_pair_set"
      "LINKER:--wrap=indexed_text_metrics_pair_get"
      "LINKER:--wrap=sound_submit_with_global_cooldown")
    add_test(NAME info_panel_percent_b COMMAND info_panel_percent_b_test)
    set_tests_properties(info_panel_percent_b PROPERTIES LABELS info)
  endif()
  # Files.c's SDL3 streams and path mapping, and the configuration file
  # ([library:filesystem], [library:config]).
  add_executable(files_test ${ENGINE_DIR}/test_files.cpp)
  target_link_libraries(files_test PRIVATE units)
  add_test(NAME files COMMAND files_test)
  set_tests_properties(files PROPERTIES LABELS files)
  # Files.c's start-up with the data directory from the diskpath
  # ([library:config]).
  add_executable(diskpath_test ${ENGINE_DIR}/test_diskpath.cpp)
  target_link_libraries(diskpath_test PRIVATE units)
  add_test(NAME diskpath COMMAND diskpath_test)
  set_tests_properties(diskpath PROPERTIES LABELS files)
  # The block grids' index (game/block_grid.h): a NaN position's INT_MIN
  # coordinates resolve to block 0, as the original's 32-bit arithmetic.
  # GNU ld's --wrap, as the frontend tests above.
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang" AND NOT APPLE AND NOT WIN32)
    add_executable(block_grid_test ${GAME_DIR}/test_block_grid.cpp)
    target_link_libraries(block_grid_test PRIVATE units)
    target_link_options(block_grid_test PRIVATE
      "LINKER:--wrap=ddiscene_object_position_get"
      "LINKER:--wrap=entity_state_flags_get"
      "LINKER:--wrap=container_mesh_hide_group"
      "LINKER:--wrap=scene_mesh_color_and_alpha_set")
    add_test(NAME block_grid COMMAND block_grid_test)
    set_tests_properties(block_grid PROPERTIES LABELS block_grid)
  endif()
  add_executable(config_test ${PLATFORM_DIR}/native/test_config.cpp)
  target_link_libraries(config_test PRIVATE units)
  add_test(NAME config COMMAND config_test)
  # Skipped (77) where the preference directory cannot be moved into the
  # fixture.
  set_tests_properties(config PROPERTIES LABELS config SKIP_RETURN_CODE 77)
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
  add_executable(flic_chunk_walk_test ${ENGINE_DIR}/test_flic_chunk_walk.cpp)
  target_link_libraries(flic_chunk_walk_test PRIVATE units)
  add_test(NAME flic_chunk_walk COMMAND flic_chunk_walk_test)
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
  # (skipped without it), built with sonames that never load, and built as
  # the no-movies stub.
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
  target_link_libraries(video_missing_test PRIVATE SDL3::SDL3)
  add_test(NAME video_missing COMMAND video_missing_test)
  set_tests_properties(video_missing PROPERTIES LABELS video)
  # The LRR_MOVIES=OFF stub, which every build checks whatever the option.
  add_executable(video_stub_test ${SRC_ROOT}/library/test_video_ffmpeg.cpp
    ${SRC_ROOT}/library/video_ffmpeg.cpp)
  target_include_directories(video_stub_test PRIVATE ${SRC_ROOT})
  target_compile_definitions(video_stub_test PRIVATE LRR_VIDEO_TEST_MISSING
    LRR_MOVIES=0 "LRR_VIDEO_TEST_CLIP=\"${LRR_VIDEO_TEST_CLIP}\"")
  target_link_libraries(video_stub_test PRIVATE SDL3::SDL3)
  add_test(NAME video_stub COMMAND video_stub_test)
  set_tests_properties(video_stub PROPERTIES LABELS video)

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

  # Immediate-mode drawing through a user visual on the software renderer
  # (ADR 0008, step 1): coverage, z, blending and culling read back by Lock.
  add_executable(miniwin_immediate_test ${PLATFORM_DIR}/native/test_miniwin_immediate.cpp)
  target_link_libraries(miniwin_immediate_test PRIVATE miniwin)
  add_test(NAME miniwin_immediate COMMAND miniwin_immediate_test)
  set_tests_properties(miniwin_immediate PROPERTIES LABELS miniwin)

  # Viewports and clears (ADR 0008, step 2): two viewports on one target,
  # Clear2 rectangles, the scissor, and z clears only on request.
  add_executable(miniwin_viewports_test ${PLATFORM_DIR}/native/test_miniwin_viewports.cpp)
  target_link_libraries(miniwin_viewports_test PRIVATE miniwin)
  add_test(NAME miniwin_viewports COMMAND miniwin_viewports_test)
  set_tests_properties(miniwin_viewports PROPERTIES LABELS miniwin)

  # Textures (ADR 0008, step 3): 8-bit colour-keyed texture surfaces drawn
  # through SetTexture and TEXTUREHANDLE, stage 0 ops, linear filtering, and
  # D3DRM decal transparency.
  add_executable(miniwin_textures_test ${PLATFORM_DIR}/native/test_miniwin_textures.cpp)
  target_link_libraries(miniwin_textures_test PRIVATE miniwin)
  add_test(NAME miniwin_textures COMMAND miniwin_textures_test)
  set_tests_properties(miniwin_textures PROPERTIES LABELS miniwin)

  # Lights (ADR 0008, step 4): spot cones, range, attenuation, light colour,
  # enable frames, and the material's emissive, ambient and specular terms.
  add_executable(miniwin_lights_test ${PLATFORM_DIR}/native/test_miniwin_lights.cpp)
  target_link_libraries(miniwin_lights_test PRIVATE miniwin)
  add_test(NAME miniwin_lights COMMAND miniwin_lights_test)
  set_tests_properties(miniwin_lights PROPERTIES LABELS miniwin)

  # Fog (ADR 0008, step 5): the scene's fog reaching the device's fog
  # states, linear, exponential and exponential-squared, per vertex and per
  # pixel, and Mesh.c's FOGENABLE off for its alpha-blended groups.
  add_executable(miniwin_fog_test ${PLATFORM_DIR}/native/test_miniwin_fog.cpp)
  target_link_libraries(miniwin_fog_test PRIVATE miniwin)
  add_test(NAME miniwin_fog COMMAND miniwin_fog_test)
  set_tests_properties(miniwin_fog PROPERTIES LABELS miniwin)

  # The rendering checks above again on the SDL GPU backend (ADR 0008,
  # step 6), and the same scene on both backends compared pixel by pixel.
  # They need an SDL GPU device that can draw on SDL's offscreen video
  # driver, such as Mesa's lavapipe; without one, as on CI, they are skipped.
  # Retained-mode checks run on the software backend only.
  foreach(name immediate viewports textures lights fog)
    add_test(NAME miniwin_${name}_sdlgpu COMMAND miniwin_${name}_test)
    set_tests_properties(miniwin_${name}_sdlgpu PROPERTIES LABELS "miniwin;sdlgpu"
      SKIP_RETURN_CODE 77
      ENVIRONMENT "MINIWIN_TEST_DEVICE=SDL3 GPU HAL;MINIWIN_TEST_VIDEO_DRIVER=offscreen")
  endforeach()
  add_executable(miniwin_backends_test ${PLATFORM_DIR}/native/test_miniwin_backends.cpp)
  target_link_libraries(miniwin_backends_test PRIVATE miniwin)
  add_test(NAME miniwin_backends COMMAND miniwin_backends_test)
  set_tests_properties(miniwin_backends PROPERTIES LABELS "miniwin;sdlgpu" SKIP_RETURN_CODE 77)

  # Main.c's window procedure under miniwin: SDL mouse events as client
  # coordinate messages, and SDL's quit event as WM_CLOSE.
  add_executable(miniwin_messages_test ${PLATFORM_DIR}/native/test_miniwin_messages.cpp)
  target_link_libraries(miniwin_messages_test PRIVATE miniwin)
  add_test(NAME miniwin_messages COMMAND miniwin_messages_test)
  set_tests_properties(miniwin_messages PROPERTIES LABELS miniwin)

  # miniwin's key messages: Windows virtual-key codes and lParam flags from
  # SDL key events, WM_SYSKEY* for Alt and F10, and Alt+F4 as WM_CLOSE.
  add_executable(miniwin_keys_test ${PLATFORM_DIR}/native/test_miniwin_keys.cpp)
  target_link_libraries(miniwin_keys_test PRIVATE miniwin)
  add_test(NAME miniwin_keys COMMAND miniwin_keys_test)
  set_tests_properties(miniwin_keys PROPERTIES LABELS miniwin)
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
