# SPDX-License-Identifier: LGPL-3.0-or-later
# Adapted from isle-portable's packaging/macos/CMakeLists.txt.
#
# Turns LegoRR into LegoRR.app: Info.plist, the icon, and (when installing) an
# ad-hoc signature, which Apple silicon needs before it will run the binary.
# The bundle is neither Developer ID signed nor notarized; Gatekeeper asks
# the first time (docs/building.md).
set(_lrr_icon AppIcon)
set(MACOSX_BUNDLE_GUI_IDENTIFIER "${LRR_APP_ID}")
set(MACOSX_BUNDLE_COPYRIGHT "${LRR_APP_SPDX}")
set(MACOSX_BUNDLE_NAME "${LRR_APP_NAME}")
set(MACOSX_BUNDLE_DISPLAY_NAME "${LRR_APP_NAME}")
set(MACOSX_BUNDLE_INFO_STRING "${PROJECT_VERSION}")
set(MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}")
set(MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}")
set(MACOSX_BUNDLE_LONG_VERSION_STRING "Version ${PROJECT_VERSION}")
# The plist names the minimum system only when the build sets one.
if(CMAKE_OSX_DEPLOYMENT_TARGET)
  set(LRR_PLIST_MINIMUM_SYSTEM
    "<key>LSMinimumSystemVersion</key>\n\t<string>${CMAKE_OSX_DEPLOYMENT_TARGET}</string>")
else()
  set(LRR_PLIST_MINIMUM_SYSTEM "")
endif()

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in"
  "${CMAKE_CURRENT_BINARY_DIR}/Info.plist" @ONLY)

set(_lrr_icon_file "${CMAKE_CURRENT_SOURCE_DIR}/${_lrr_icon}.icns")
# Source file properties are per directory, and LegoRR is defined in the
# top-level one.
set_source_files_properties("${_lrr_icon_file}"
  TARGET_DIRECTORY program
  PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
target_sources(program PRIVATE "${_lrr_icon_file}")
set_target_properties(program PROPERTIES
  MACOSX_BUNDLE TRUE
  MACOSX_BUNDLE_ICON_FILE "${_lrr_icon}.icns"
  MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_BINARY_DIR}/Info.plist")
