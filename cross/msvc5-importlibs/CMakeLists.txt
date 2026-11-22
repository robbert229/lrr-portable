# SPDX-License-Identifier: LGPL-3.0-or-later
#
# The DirectX libraries the MSVC 5.0 toolchain lacks, rebuilt for the matching
# build from source (finding 574). No SDK binary is checked in.
#
#   dinput.lib  DINPUT.dll's DirectInputCreateA, imported by name (LIB /DEF)
#   dinput_data.lib  the SDK dinput.lib's data members dilib1.obj
#               (c_dfDIMouse) and dilib2.obj (c_dfDIKeyboard), whose data
#               lives in .text
#   dsound.lib  DSOUND.dll's DirectSoundCreate, imported by ordinal 1
#               (the toolchain's dsound.lib imports it by name)
#   ddraw.lib   DDRAW.dll's imports with DirectX 5's hints (the toolchain's
#               older ddraw.lib numbers them one lower)
#   amstrmid.lib  the DirectShow multimedia-stream GUIDs, one member
#   dxguid.lib  every DirectX 6.1 GUID, one member, so that the data
#               members' references pull the whole block (finding 688)
#
# Each library is an ordinary static-library target, one compile per member
# and one LIB call per library, so that ReproBit's CMake import records them
# as producers. LRR_IMPORTLIB_TARGETS maps each library name of the link
# order to its targets (joined by "+"); the program links them there.

# The members are compiled without optimisation or configuration defines;
# /Zl keeps default-library names out of them. /Zi, which ReproBit's compile
# model requires, adds only debug sections, which no link here keeps. These
# settings are this directory's own: the parent's directory-level compile
# options (LRR_ORIGINAL_UNIT_FLAGS adds /Zi /O2 there, finding 697) are cleared.
set_property(DIRECTORY PROPERTY COMPILE_OPTIONS "")
set(CMAKE_C_FLAGS "/W3 /Zl /Zi")
foreach(_config IN ITEMS DEBUG MINSIZEREL RELEASE RELWITHDEBINFO)
  set(CMAKE_C_FLAGS_${_config} "")
  set(CMAKE_STATIC_LINKER_FLAGS_${_config} "")
endforeach()
set(CMAKE_STATIC_LINKER_FLAGS "")

set(LRR_IMPORTLIB_DIR "${CMAKE_BINARY_DIR}/msvc5-importlibs")

function(lrr_importlib target name)
  add_library(${target} STATIC ${ARGN})
  set_target_properties(${target} PROPERTIES
    OUTPUT_NAME "${name}"
    ARCHIVE_OUTPUT_DIRECTORY "${LRR_IMPORTLIB_DIR}")
endfunction()

# LIB /DEF resolves each undecorated export against sdk_exports.c's object,
# so the members get the __stdcall symbol names while the imports stay
# undecorated. That object is never a member of the import library.
lrr_importlib(lrr_il_dinput dinput sdk_exports.c)
set_target_properties(lrr_il_dinput PROPERTIES STATIC_LIBRARY_OPTIONS
  "/machine:i386;/def:${CMAKE_CURRENT_SOURCE_DIR}/dinput.def")
lrr_importlib(lrr_il_dsound dsound sdk_exports.c)
set_target_properties(lrr_il_dsound PROPERTIES STATIC_LIBRARY_OPTIONS
  "/machine:i386;/def:${CMAKE_CURRENT_SOURCE_DIR}/dsound.def")

# ddraw.lib from DirectX 5's export list, for the original's import hints.
lrr_importlib(lrr_il_ddraw ddraw ddraw_exports.c)
set_target_properties(lrr_il_ddraw PROPERTIES STATIC_LIBRARY_OPTIONS
  "/machine:i386;/def:${CMAKE_CURRENT_SOURCE_DIR}/ddraw.def")

# The SDK's dinput.lib also held the two data members. A librarian step
# cannot take another library as input, so they are their own library,
# dinput_data.lib, which the link reads just before dinput.lib: the image is
# the same as with one combined library.
lrr_importlib(lrr_il_dinput_data dinput_data dilib1.c dilib2.c)

lrr_importlib(lrr_il_amstrmid amstrmid amstrmid.c)
lrr_importlib(lrr_il_dxguid dxguid dxguid.c)

set(LRR_IMPORTLIB_TARGETS
  dinput.lib lrr_il_dinput_data+lrr_il_dinput
  dsound.lib lrr_il_dsound
  ddraw.lib lrr_il_ddraw
  amstrmid.lib lrr_il_amstrmid
  dxguid.lib lrr_il_dxguid
  PARENT_SCOPE)
