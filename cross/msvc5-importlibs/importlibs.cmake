# SPDX-License-Identifier: LGPL-3.0-or-later
#
# The DirectX libraries the MSVC 5.0 toolchain lacks, rebuilt for the matching
# build from source (finding 574). No SDK binary is checked in.
#
#   dinput.lib  DINPUT.dll's DirectInputCreateA, imported by name (LIB /DEF),
#               plus the SDK's data members dilib1.obj (c_dfDIMouse) and
#               dilib2.obj (c_dfDIKeyboard), whose data lives in .text
#   dsound.lib  DSOUND.dll's DirectSoundCreate, imported by ordinal 1
#               (the toolchain's dsound.lib imports it by name)
#   amstrmid.lib  the DirectShow multimedia-stream GUIDs, one member
#   dxguid.lib  every DirectX 6.1 GUID, one member, so that the data
#               members' references pull the whole block (finding 688)
#
# The libraries are written to LRR_IMPORTLIB_DIR, which the matching link
# searches before the toolchain's lib directory. The target
# lrr_msvc5_importlibs builds them.

set(LRR_IMPORTLIB_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}")
set(LRR_IMPORTLIB_DIR "${CMAKE_BINARY_DIR}/msvc5-importlibs")
file(MAKE_DIRECTORY "${LRR_IMPORTLIB_DIR}")

set(_lrr_il_src "${LRR_IMPORTLIB_SOURCE_DIR}")
set(_lrr_il_out "${LRR_IMPORTLIB_DIR}")
set(_lrr_il_cl "${CMAKE_C_COMPILER}" /nologo /c /Zl /W3)
set(_lrr_il_lib "${CMAKE_AR}" /nologo)

# One object per SDK member. /Zl keeps default-library names out of them.
foreach(_member IN ITEMS sdk_exports dilib1 dilib2 amstrmid dxguid)
  add_custom_command(OUTPUT "${_lrr_il_out}/${_member}.obj"
    COMMAND ${_lrr_il_cl} "${_lrr_il_src}/${_member}.c" "/Fo${_member}.obj"
    DEPENDS "${_lrr_il_src}/${_member}.c" "${_lrr_il_src}/dinput_formats.h"
      "${_lrr_il_src}/sdk_guid.h"
    WORKING_DIRECTORY "${_lrr_il_out}" VERBATIM)
endforeach()

# LIB /DEF resolves each undecorated export against sdk_exports.obj, so the
# members get the __stdcall symbol names while the imports stay undecorated.
add_custom_command(OUTPUT "${_lrr_il_out}/dinput.lib"
  COMMAND ${_lrr_il_lib} /machine:ix86 "/def:${_lrr_il_src}/dinput.def"
    /out:dinput_imp.lib sdk_exports.obj
  COMMAND ${_lrr_il_lib} /out:dinput.lib dilib1.obj dilib2.obj dinput_imp.lib
  DEPENDS "${_lrr_il_src}/dinput.def" "${_lrr_il_out}/sdk_exports.obj"
    "${_lrr_il_out}/dilib1.obj" "${_lrr_il_out}/dilib2.obj"
  WORKING_DIRECTORY "${_lrr_il_out}" VERBATIM)
add_custom_command(OUTPUT "${_lrr_il_out}/dsound.lib"
  COMMAND ${_lrr_il_lib} /machine:ix86 "/def:${_lrr_il_src}/dsound.def"
    /out:dsound.lib sdk_exports.obj
  DEPENDS "${_lrr_il_src}/dsound.def" "${_lrr_il_out}/sdk_exports.obj"
  WORKING_DIRECTORY "${_lrr_il_out}" VERBATIM)
add_custom_command(OUTPUT "${_lrr_il_out}/amstrmid.lib"
  COMMAND ${_lrr_il_lib} /out:amstrmid.lib amstrmid.obj
  DEPENDS "${_lrr_il_out}/amstrmid.obj"
  WORKING_DIRECTORY "${_lrr_il_out}" VERBATIM)
add_custom_command(OUTPUT "${_lrr_il_out}/dxguid.lib"
  COMMAND ${_lrr_il_lib} /out:dxguid.lib dxguid.obj
  DEPENDS "${_lrr_il_out}/dxguid.obj"
  WORKING_DIRECTORY "${_lrr_il_out}" VERBATIM)

set(LRR_IMPORTLIB_FILES "${_lrr_il_out}/dinput.lib" "${_lrr_il_out}/dsound.lib"
  "${_lrr_il_out}/amstrmid.lib" "${_lrr_il_out}/dxguid.lib")
add_custom_target(lrr_msvc5_importlibs DEPENDS ${LRR_IMPORTLIB_FILES})
