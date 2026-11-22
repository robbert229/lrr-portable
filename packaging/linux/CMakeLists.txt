# SPDX-License-Identifier: LGPL-3.0-or-later
# Adapted from isle-portable's packaging/linux/CMakeLists.txt.
#
# Injects the application id and the build into the desktop entry and the
# AppStream metadata, and installs them with the icon beside lrr-portable.
configure_file(lrr-portable.desktop.in "${LRR_APP_ID}.desktop" @ONLY)
configure_file(lrr-portable.metainfo.xml.in "${LRR_APP_ID}.metainfo.xml" @ONLY)

install(FILES "../icons/lrr-portable.svg"
  RENAME "${LRR_APP_ID}.svg"
  DESTINATION "share/icons/hicolor/scalable/apps")
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${LRR_APP_ID}.desktop"
  DESTINATION "share/applications")
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${LRR_APP_ID}.metainfo.xml"
  DESTINATION "share/metainfo")
