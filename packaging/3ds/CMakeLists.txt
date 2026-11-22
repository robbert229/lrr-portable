# SPDX-License-Identifier: LGPL-3.0-or-later
#
# The Nintendo 3DS packages (issue #931), as isle-portable's CMakeLists.txt
# makes them (LGPL-3.0-or-later):
#   - LegoRR.3dsx, for the Homebrew Launcher: devkitPro's ctr_generate_smdh
#     and ctr_create_3dsx, always.
#   - LegoRR.cia, installable from FBI and the like: bannertool and makerom,
#     when both are on the PATH (CI downloads them).
# The icon, banner and banner sound are original (regenerate.py). Neither
# package holds game data: the game reads Data and the WADs from
# sdmc:/3ds/LegoRR (src/platform/native/3ds/config.cpp).

set(LRR_3DS_TITLE "LRR Portable")
set(LRR_3DS_DESCRIPTION "Portable reconstruction of Lego Rock Raiders")
set(LRR_3DS_AUTHOR "robbert229/lrr-portable")
set(LRR_3DS_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

ctr_generate_smdh(LegoRR.smdh
  NAME "${LRR_3DS_TITLE}"
  DESCRIPTION "${LRR_3DS_DESCRIPTION}"
  AUTHOR "${LRR_3DS_AUTHOR}"
  ICON "${LRR_3DS_DIR}/icon.png")
ctr_create_3dsx(program
  SMDH "${CMAKE_CURRENT_BINARY_DIR}/LegoRR.smdh"
  OUTPUT "${CMAKE_BINARY_DIR}/LegoRR.3dsx")

find_program(BANNERTOOL bannertool)
find_program(MAKEROM makerom)
if(BANNERTOOL AND MAKEROM)
  add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/LegoRR.bnr"
    COMMAND "${BANNERTOOL}" makebanner
      -i "${LRR_3DS_DIR}/banner.png"
      -a "${LRR_3DS_DIR}/banner.wav"
      -o "${CMAKE_CURRENT_BINARY_DIR}/LegoRR.bnr"
    DEPENDS "${LRR_3DS_DIR}/banner.png" "${LRR_3DS_DIR}/banner.wav"
    VERBATIM)
  add_custom_command(
    OUTPUT "${CMAKE_BINARY_DIR}/LegoRR.cia"
    COMMAND "${MAKEROM}"
      -f cia
      -exefslogo
      -o "${CMAKE_BINARY_DIR}/LegoRR.cia"
      -rsf "${LRR_3DS_DIR}/template.rsf"
      -major "${PROJECT_VERSION_MAJOR}"
      -minor "${PROJECT_VERSION_MINOR}"
      -micro "${PROJECT_VERSION_PATCH}"
      -icon "${CMAKE_CURRENT_BINARY_DIR}/LegoRR.smdh"
      -banner "${CMAKE_CURRENT_BINARY_DIR}/LegoRR.bnr"
      -elf "$<TARGET_FILE:program>"
    DEPENDS program "${LRR_3DS_DIR}/template.rsf"
      "${CMAKE_CURRENT_BINARY_DIR}/LegoRR.smdh" "${CMAKE_CURRENT_BINARY_DIR}/LegoRR.bnr"
    COMMENT "Building LegoRR.cia"
    VERBATIM)
  add_custom_target(program_cia ALL DEPENDS "${CMAKE_BINARY_DIR}/LegoRR.cia")
else()
  message(STATUS "bannertool or makerom not found: building LegoRR.3dsx only, no LegoRR.cia")
endif()
