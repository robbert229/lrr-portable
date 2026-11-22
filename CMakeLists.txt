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
  # The toolchain file's LINK.EXE on the toolchain's own runtime (finding 694).
  if(LRR_MSVC5_LINK)
    set(CMAKE_LINKER "${LRR_MSVC5_LINK}")
  endif()
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
option(LRR_ORIGINAL_UNIT_FLAGS
  "Compile each unit with the original's debug-info and /Gy choice and link with /OPT:ICF (finding 697)" OFF)

if(LRR_RECCMP AND NOT LRR_CLASSIC_MSVC5)
  message(FATAL_ERROR "LRR_RECCMP requires LRR_CLASSIC_MSVC5")
endif()
if(LRR_ORIGINAL_UNIT_FLAGS AND NOT LRR_CLASSIC_MSVC5)
  message(FATAL_ERROR "LRR_ORIGINAL_UNIT_FLAGS requires LRR_CLASSIC_MSVC5")
endif()
# Reccmp maps each annotation through the PDB's line numbers, and a unit
# without /Zi has none: its functions leave the report (finding 697). The
# gate therefore keeps /Zi and /OPT:NOICF; only the identity build (ReproBit)
# takes the original's per-unit choices.
if(LRR_ORIGINAL_UNIT_FLAGS AND LRR_RECCMP)
  message(FATAL_ERROR "LRR_ORIGINAL_UNIT_FLAGS drops /Zi, which Reccmp needs")
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
  # "portable": every build but the MSVC 5.0 matching build (link_order.cmake).
  if("${ARGN}" STREQUAL "" OR "${ARGN}" STREQUAL "${PLATFORM}"
      OR ("${ARGN}" STREQUAL "portable" AND NOT LRR_CLASSIC_MSVC5))
    list(APPEND LRR_LINK_SOURCES "${SRC_ROOT}/${source}")
    list(APPEND LRR_LINK_TARGETS "${library}")
    list(APPEND LRR_LINK_KINDS "${kind}")
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
  # Units the original compiled without optimisation: their code keeps the
  # frame pointer and reloads every operand, and the functions are packed
  # with int3 fill (finding 505 rule 5; finding 587). They take /Od after the
  # configuration's /O2, and /Ob0 after its /Ob1, as plain /Od gives: no
  # inline expansion, so Maths.h's inline functions are called (finding 692).
  set(LRR_UNOPTIMISED_UNITS "${SRC_ROOT}/game/camera_control.c")
  # The original's per-unit compile choices (finding 697). LINK 5.0 folds two
  # identical COMDATs only when their COFF line-number counts agree too, so a
  # function of a unit without debug information (no line numbers) folds
  # only with others like it, LIBC's among them, and never with a /Zi unit's.
  # - Without /Zi: gods98's units except Files.c, and NERPs.c, RadarMap.c and
  #   Water.c. LIBC's _matherr, operator delete and _fpclear fold into
  #   00456210, 0045daf0 and 00484c90, and Flic_GetWidth into 004897d0;
  #   Water.c's and Maths.c's code is the no-debug code.
  # - Without /Gy (/O2 spelled out without it; MSVC 5.0 ignores /Gy-):
  #   AITask.c and Container.c, which keep their identical small functions
  #   apart from Files.c's. Files.c keeps /Gy: /OPT:REF removed its
  #   unreferenced File_Flush (finding 667), and Mesh.c's mesh_parse_lwo
  #   changes without it.
  set(LRR_NO_DEBUG_INFO_UNITS "")
  foreach(_rel IN ITEMS
      gods98/movie_unit.cpp gods98/container.c gods98/viewports.c
      gods98/utils.c gods98/main.c gods98/config.c gods98/maths.c
      gods98/font.c gods98/sound3d.c gods98/directdraw.c
      gods98/images_unit.c gods98/avi_player.cpp gods98/input_unit.c
      gods98/mesh_unit.c gods98/flic_unit.c gods98/textwindow.c gods98/keys.c
      gods98/draw.c gods98/lws.c gods98/soundsys.c gods98/g98_024.c
      gods98/animclone.c gods98/g98_026.c gods98/dxerror.c
      gods98/file_list_streams.c gods98/g98_029.c gods98/wad.c gods98/lwo.c
      gods98/rnc.c gods98/init.c
      game/tutorial_script.c game/radarmap.c game/water.c)
    list(APPEND LRR_NO_DEBUG_INFO_UNITS "${SRC_ROOT}/${_rel}")
  endforeach()
  set(LRR_NO_GY_UNITS "${SRC_ROOT}/game/aitask.c" "${SRC_ROOT}/gods98/container.c")
  if(LRR_ORIGINAL_UNIT_FLAGS)
    # /Zi and /O2 move from the configuration's flags to each target's
    # COMPILE_OPTIONS, which lrr_original_unit_flags() replaces per unit.
    # Their place after /Ob1 /DNDEBUG does not change the code.
    foreach(_lang IN ITEMS C CXX)
      foreach(_flag IN ITEMS /Zi /O2)
        string(REPLACE "${_flag}" "" CMAKE_${_lang}_FLAGS_RELWITHDEBINFO
          "${CMAKE_${_lang}_FLAGS_RELWITHDEBINFO}")
      endforeach()
    endforeach()
    add_compile_options(/Zi /O2)
  endif()
  function(lrr_original_unit_flags target src)
    if(NOT LRR_ORIGINAL_UNIT_FLAGS)
      return()
    endif()
    set(_opts "")
    if(NOT src IN_LIST LRR_NO_DEBUG_INFO_UNITS)
      list(APPEND _opts /Zi)
    endif()
    if(src IN_LIST LRR_NO_GY_UNITS)
      list(APPEND _opts /Og /Oi /Ot /Oy /Ob1 /Gs /Gf)
    else()
      list(APPEND _opts /O2)
    endif()
    set_property(TARGET ${target} PROPERTY COMPILE_OPTIONS ${_opts})
  endfunction()
  set(LRR_LINK_OBJECTS "")
  set(LRR_LINK_LATE_SOURCES "")
  set(LRR_LINK_LATE_TARGETS "")
  set(LRR_LINK_TAIL_OBJECTS "")
  set(LRR_LINK_MEMBER_OBJECTS "")
  set(LRR_LINK_MEMBER_RECORDS "")
  set(_lrr_place explicit)
  set(_lrr_index 0)
  foreach(_src _lib _kind IN ZIP_LISTS LRR_LINK_SOURCES LRR_LINK_TARGETS LRR_LINK_KINDS)
    # The list's late members go to their own libraries, after the others
    # (LRR_LINK_LATE_MEMBERS, link_order.cmake; finding 657).
    string(REPLACE "${SRC_ROOT}/" "" _rel "${_src}")
    if(_rel IN_LIST LRR_LINK_LATE_MEMBERS)
      list(APPEND LRR_LINK_LATE_SOURCES "${_src}")
      list(APPEND LRR_LINK_LATE_TARGETS "${_lib}")
      continue()
    endif()
    math(EXPR _lrr_index "${_lrr_index} + 1")
    get_filename_component(_stem "${_src}" NAME_WE)
    string(LENGTH "00${_lrr_index}" _len)
    math(EXPR _len "${_len} - 3")
    string(SUBSTRING "00${_lrr_index}" ${_len} 3 _pos)
    set(_target "link_${_pos}_${_stem}")
    add_library(${_target} OBJECT ${_src})
    target_include_directories(${_target} PRIVATE ${LRR_LINK_INCLUDES_${_lib}})
    lrr_original_unit_flags(${_target} "${_src}")
    if(_src IN_LIST LRR_UNOPTIMISED_UNITS)
      target_compile_options(${_target} PRIVATE /Od /Ob0)
    endif()
    # Where the object goes (finding 658). Each keeps its own target, so it
    # compiles the same wherever it is linked from:
    # - explicit: the game units and gods98's first, up to
    #   LRR_LINK_EXPLICIT_LAST (link_order.cmake);
    # - member: each later gods98 unit, archived in list order into one
    #   library, unit_gods98.lib, searched before LRR_LINK_LIBRARIES
    #   (finding 690);
    # - tail: from the first port-tail entry on, every entry (the tail and
    #   the CRT-range legacy files) goes to lrr_tail.lib, below.
    if(_kind STREQUAL "tail")
      set(_lrr_place tail)
    endif()
    if(_lrr_place STREQUAL "tail")
      list(APPEND LRR_LINK_TAIL_OBJECTS "$<TARGET_OBJECTS:${_target}>")
      string(APPEND LRR_LINK_MEMBER_RECORDS "${LRR_LINK_TAIL_LIBRARY} ${_rel} $<TARGET_OBJECTS:${_target}>\n")
    elseif(_lrr_place STREQUAL "member")
      if(NOT _kind STREQUAL "unit")
        message(FATAL_ERROR "${_rel}: only unit entries may follow LRR_LINK_EXPLICIT_LAST before the tail")
      endif()
      list(APPEND LRR_LINK_MEMBER_OBJECTS "$<TARGET_OBJECTS:${_target}>")
      string(APPEND LRR_LINK_MEMBER_RECORDS "unit_gods98 ${_rel} $<TARGET_OBJECTS:${_target}>\n")
    else()
      list(APPEND LRR_LINK_OBJECTS "$<TARGET_OBJECTS:${_target}>")
    endif()
    if(_rel STREQUAL LRR_LINK_EXPLICIT_LAST)
      set(_lrr_place member)
    endif()
  endforeach()
  if(_lrr_place STREQUAL "explicit")
    message(FATAL_ERROR "LRR_LINK_EXPLICIT_LAST names a source the list lacks: "
      "${LRR_LINK_EXPLICIT_LAST}")
  endif()
  list(LENGTH LRR_LINK_LATE_MEMBERS _lrr_late_wanted)
  list(LENGTH LRR_LINK_LATE_SOURCES _lrr_late_found)
  if(NOT _lrr_late_wanted EQUAL _lrr_late_found)
    message(FATAL_ERROR "LRR_LINK_LATE_MEMBERS names a source the list lacks: "
      "${LRR_LINK_LATE_MEMBERS}")
  endif()
  # One library per late member, in list order: LINK searches each library
  # in turn, so the members come out in list order. In one library, init.c
  # came out before rnc.c (finding 657). tools/link_order.py's late_library()
  # mirrors the late_<stem> name.
  set(LRR_LINK_LATE_LIBRARIES "")
  foreach(_src _lib IN ZIP_LISTS LRR_LINK_LATE_SOURCES LRR_LINK_LATE_TARGETS)
    get_filename_component(_stem "${_src}" NAME_WE)
    add_library(late_${_stem} STATIC ${_src})
    target_include_directories(late_${_stem} PRIVATE ${LRR_LINK_INCLUDES_${_lib}})
    lrr_original_unit_flags(late_${_stem} "${_src}")
    list(APPEND LRR_LINK_LATE_LIBRARIES late_${_stem})
  endforeach()
  # The port tail as one library (ADR 0006 addendum of 2026-11-13, finding
  # 658). LINK pulls a LIBC member when the first unresolved reference to it
  # comes up, so explicit tail objects pulled members before LIBC's own
  # references to them, out of the original's order. From a library searched
  # after LIBC, the tail is pulled in LINK's first pass and the LIBC members
  # only it needs in the second, after the import thunks, where the original
  # has nothing of its own. Tail objects that nothing references are not
  # linked; under /OPT:REF they contributed nothing anyway.
  add_library(${LRR_LINK_TAIL_LIBRARY} STATIC ${LRR_LINK_TAIL_OBJECTS})
  set_target_properties(${LRR_LINK_TAIL_LIBRARY} PROPERTIES LINKER_LANGUAGE CXX)
  # gods98's later units as one library, as the original linked them
  # (finding 690). LINK allocates the commons library by library in
  # command-line order, and within a library in reverse order of the members
  # it pulled, so one library gives gods98's commons the original's reverse
  # unit order; a library per unit gave list order. LRR_LINK_MEMBER_ORDER
  # (link_order.cmake) keeps the members' pull order, and so their code, in
  # list order.
  add_library(unit_gods98 STATIC ${LRR_LINK_MEMBER_OBJECTS})
  set_target_properties(unit_gods98 PROPERTIES LINKER_LANGUAGE CXX)
  list(LENGTH LRR_LINK_MEMBER_OBJECTS _lrr_members)
  list(LENGTH LRR_LINK_MEMBER_ORDER _lrr_member_symbols)
  if(NOT _lrr_members EQUAL _lrr_member_symbols)
    message(FATAL_ERROR "LRR_LINK_MEMBER_ORDER names ${_lrr_member_symbols} "
      "symbols for ${_lrr_members} gods98 member units; name one per unit")
  endif()
  # Which library each archived object is in, for the tools that read the
  # objects (tools/link_order.py's member_objects()).
  file(GENERATE OUTPUT "${CMAKE_BINARY_DIR}/link-members.txt"
    CONTENT "${LRR_LINK_MEMBER_RECORDS}")
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
    ${GAME_DIR}/dependency_grid.cpp ${GAME_DIR}/screen_pick.cpp)
endif()
if(PLATFORM STREQUAL "winapi")
  list(APPEND LRR_SOURCES ${SRC_ROOT}/version.rc)
endif()
if(LRR_CLASSIC_MSVC5)
  # The original's resources (#560, finding 651): its icon and group icon
  # (src/LegoRR.ico), and no VERSIONINFO.
  set_source_files_properties(${SRC_ROOT}/version.rc PROPERTIES
    COMPILE_DEFINITIONS LRR_ORIGINAL_RESOURCES
    OBJECT_DEPENDS "${LRR_SRC_ROOT_ABS}/LegoRR.ico")
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
  # The original's operator delete is 0045daf0, which C code also calls as a
  # plain function (ui_render_record_free). LINK 5.0 has no /ALTERNATENAME,
  # so a section-less object gives that function the C++ name too, as a weak
  # external (finding 696). It adds nothing to the image. The alias
  # characteristic resolves references from command-line objects such as
  # movie_unit.cpp's to the default too; with nolibrary, LINK would search
  # the libraries for them and take LIBC's delete.obj.
  find_package(Python3 REQUIRED COMPONENTS Interpreter)
  set(_lrr_weak_delete "${CMAKE_BINARY_DIR}/weak_operator_delete.obj")
  add_custom_command(OUTPUT "${_lrr_weak_delete}"
    COMMAND "${Python3_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/tools/coff_weak_external.py" --search=alias
      "${_lrr_weak_delete}" "??3@YAXPAX@Z=_ui_render_record_free"
      # Flic.c's do-nothing functions, which the original's linker folded
      # into one body each (flic_unit.h).
      "_Flic_Palette64=_flic_opcode25_success_stub"
      "_Flic_DeltaByte=_flic_opcode25_success_stub"
      "_Flic_Black=_flic_opcode25_success_stub"
      "_FlicBRunDepackBytePerPixel=_flic_opcode25_success_stub"
      "_FlicBRunDepackHiColorFlic=_flic_opcode25_success_stub"
      "_FlicCopyHiColorFlic=_flic_opcode27_fallback_noop"
      "_FlicCopyBytePerPixel=_flic_opcode27_fallback_noop"
      "_FlicCopyHiColor=_flic_opcode27_fallback_noop"
      "_FlicDeltaWordBytePerPixel=_flic_opcode27_fallback_noop"
      "_FlicDeltaWordHiColorDZ=_flic_opcode27_fallback_noop"
      "_FlicDeltaWordHiColorFlic=_flic_opcode27_fallback_noop"
      "_Flic_Unpack=_flh_magic_uu_noop"
      # FrontEnd.c's four data frees, folded into steering.c's
      # optional_resource_destroy (frontend_unit_imports.h).
      "_front_menu_item_free_trigger=_optional_resource_destroy"
      "_front_menu_item_free_text_input=_optional_resource_destroy"
      "_front_menu_item_free_slider=_optional_resource_destroy"
      "_front_menu_item_free_real_slider=_optional_resource_destroy"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/tools/coff_weak_external.py"
    VERBATIM)
  set_source_files_properties("${_lrr_weak_delete}" PROPERTIES
    EXTERNAL_OBJECT TRUE GENERATED TRUE)
  target_sources(program PRIVATE "${_lrr_weak_delete}")
  # The DirectX libraries the toolchain lacks, rebuilt from source (finding
  # 574). Each target's full path takes the place the link order gives the
  # library's name, so LINK reads them in the same order and never searches
  # the toolchain's own dinput.lib or dsound.lib.
  add_subdirectory(cross/msvc5-importlibs)
  set(_lrr_link_libraries "")
  # The toolchain root for the libraries searched twice: the toolchain file's
  # LRR_MSVC5_ROOT, or, where a caller configures with its own toolchain file
  # (ReproBit's import), the root above the compiler's wine/x86/cl.
  set(_lrr_toolchain_root "${LRR_MSVC5_ROOT}")
  if(NOT _lrr_toolchain_root)
    get_filename_component(_lrr_cl_dir "${CMAKE_C_COMPILER}" DIRECTORY)
    get_filename_component(_lrr_cl_root "${_lrr_cl_dir}/../.." ABSOLUTE)
    if(EXISTS "${_lrr_cl_root}/lib/kernel32.lib")
      set(_lrr_toolchain_root "${_lrr_cl_root}")
    endif()
  endif()
  foreach(_lib IN LISTS LRR_LINK_LIBRARIES)
    list(FIND LRR_IMPORTLIB_TARGETS "${_lib}" _index)
    if(_index GREATER -1)
      math(EXPR _index "${_index} + 1")
      list(GET LRR_IMPORTLIB_TARGETS ${_index} _lib)
      string(REPLACE "+" ";" _lib "${_lib}")
    endif()
    # A library also searched earlier (LRR_LINK_FIRST_LIBRARIES): LINK
    # searches a name once, so the second search names the file (finding
    # 694).
    list(FIND LRR_LINK_FIRST_LIBRARIES "${_lib}" _index)
    if(_index GREATER -1 AND _lrr_toolchain_root)
      set(_lib "${_lrr_toolchain_root}/lib/${_lib}")
    endif()
    list(APPEND _lrr_link_libraries "${_lib}")
  endforeach()
  set(LRR_LINK_LIBRARIES ${_lrr_link_libraries})
  target_link_options(program PRIVATE ${LRR_LINK_MEMBER_ORDER} ${LRR_LINK_SYMBOLS})
  # The late units' libraries, then the gods98 member library, come first:
  # LINK infers the entry point after the explicit objects, and the
  # original's wincrt0.obj follows Movie.cpp's LIBC references but precedes
  # Container.c's (finding 658). Nothing references the late units before
  # gods98's members do, so LINK pulls them in its second pass; listed
  # first, their commons come before gods98's, as in the original (finding
  # 690).
  # kernel32.lib's first search comes before the gods98 members load, so it
  # pulls only the explicit objects' imports (finding 694).
  target_link_libraries(program PRIVATE ${LRR_LINK_LATE_LIBRARIES}
    ${LRR_LINK_FIRST_LIBRARIES} unit_gods98)
  target_link_libraries(program PRIVATE ${LRR_LINK_LIBRARIES})
  target_link_libraries(program PRIVATE ${LRR_LINK_TAIL_LIBRARY})
  # No /ENTRY: LINK infers WinMainCRTStartup from WinMain and pulls
  # wincrt0.obj where the original has it, after exsup.obj. /ENTRY made it
  # LIBC's first member, so its own LIBC references came before the units'
  # (finding 658).
  # The original's link mode (#562, finding 642): it was linked without
  # /DEBUG, so LINK 5.0 removed unreferenced COMDATs (/OPT:REF), and it folded
  # no identical functions. The matching build states both, because /DEBUG
  # (kept for Reccmp's PDB) would otherwise select /OPT:NOREF. The original
  # kept every function of its objects, which had no /Gy; the matching
  # build's objects do, so /INCLUDE keeps the original's annotated function
  # that nothing references: LegoObject.c's 00438390, unreferenced in the
  # original as well. (004907c0 is LIBC cvt.obj's static __shift, which LIBC
  # links and calls; tools/reccmp_map_statics.py names it, finding 643.)
  # The identity build folds identical COMDATs as the original's link did
  # (finding 697); the gate keeps them apart, since every unit there has /Zi.
  if(LRR_ORIGINAL_UNIT_FLAGS)
    set(_lrr_icf "/OPT:ICF")
  else()
    set(_lrr_icf "/OPT:NOICF")
  endif()
  target_link_options(program PRIVATE "/OPT:REF" "${_lrr_icf}"
    "/INCLUDE:_legoobject_global_animation_state_get")
else()
  # The frame-loop object in gods98 calls into game, while game itself uses
  # gods98 APIs. Repeat the archive so GNU ld resolves this static-library
  # cycle.
  target_link_libraries(program PRIVATE game gods98 game platform_glue)
  if(PLATFORM STREQUAL "winapi")
    # ddraw: DirectDraw.c (gods98/directdraw.c) imports DirectDrawCreate and
    # DirectDrawEnumerateA, as the original does (finding 596); vfw32: the
    # AVI player (gods98/avi_player.cpp) imports AVIFIL32 (finding 604);
    # ole32: the movie unit (gods98/movie_unit.cpp) imports CoInitialize and
    # CoCreateInstance (finding 618); dinput and dxguid: Input.c
    # (gods98/input_unit.c) imports DirectInputCreateA and links
    # c_dfDIKeyboard and GUID_SysKeyboard (finding 683).
    target_link_libraries(program PRIVATE winmm d3drm ddraw vfw32 ole32 advapi32
      dinput dxguid)
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
