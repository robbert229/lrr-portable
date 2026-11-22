# SPDX-License-Identifier: LGPL-3.0-or-later
#
# Packages (issue #922): `cpack` in the build directory writes
# lrr-portable-<version>-<system>-<arch> to build/dist, from `install()`:
#   - Linux:   a TGZ with lrr-portable, the desktop entry, the AppStream metadata and
#              the icon; packaging/linux also holds the AppImage and Flatpak
#              recipes the CI packages workflow runs.
#   - macOS:   a DragNDrop disk image holding lrr-portable.app (packaging/macos).
#   - Windows: a ZIP with lrr-portable.exe, and SDL3.dll beside it when SDL is shared.
# The layout follows isle-portable's packaging/ (LGPL-3.0-or-later; keep the
# attribution when adapting more of it). No package holds game data: every one
# expects the owner's own LegoRR0.wad and LegoRR1.wad, found through `diskpath`
# in LegoRR.ini (README.md).

# The reverse-DNS application id, shared by the desktop entry, the AppStream
# component, the Flatpak and the macOS bundle identifier. It follows Flathub's
# convention for a project hosted on GitHub: io.github.<owner>.<name>.
set(LRR_APP_ID "io.github.robbert229.lrr_portable")
set(LRR_APP_NAME "LRR Portable")
set(LRR_APP_SUMMARY "Portable reconstruction of Lego Rock Raiders")
set(LRR_APP_SPDX "LGPL-3.0-or-later")
set(LRR_APP_URL "https://github.com/robbert229/lrr-portable")

# The AppStream release date. SOURCE_DATE_EPOCH keeps it reproducible.
string(TIMESTAMP LRR_BUILD_DATE "%Y-%m-%d" UTC)
# CI numbers the version recorded in the package metadata and the AppStream
# release by run, as isle-portable does, so that two runs of one version sort
# apart. File names carry the plain version.
if("$ENV{GITHUB_ACTIONS}" STREQUAL "true" AND NOT "$ENV{GITHUB_RUN_NUMBER}" STREQUAL "")
  set(LRR_SEMANTIC_VERSION "${PROJECT_VERSION}~build$ENV{GITHUB_RUN_NUMBER}")
else()
  set(LRR_SEMANTIC_VERSION "${PROJECT_VERSION}")
endif()

# Windows puts lrr-portable.exe and the licence at the root of its ZIP; elsewhere
# they go under bin/ and share/doc. These are plain variables, not the cache
# ones of GNUInstallDirs, which another directory may already have set.
if(WIN32)
  set(LRR_INSTALL_BINDIR ".")
  set(LRR_INSTALL_DOCDIR ".")
else()
  include(GNUInstallDirs)
  set(LRR_INSTALL_BINDIR "${CMAKE_INSTALL_BINDIR}")
  set(LRR_INSTALL_DOCDIR "${CMAKE_INSTALL_DOCDIR}")
endif()

# The bundle properties come before install(TARGETS), which reads them.
if(APPLE AND NOT IOS)
  add_subdirectory(macos)
endif()

install(TARGETS program
  RUNTIME DESTINATION "${LRR_INSTALL_BINDIR}"
  BUNDLE DESTINATION ".")

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  add_subdirectory(linux)
endif()
if(APPLE AND NOT IOS)
  # Here rather than in macos/, so that it runs after the install() of
  # lrr-portable.app above.
  # Pull in any shared library the bundle links (none with the fetched, static
  # SDL3), then sign, so that the signature covers the final contents.
  install(CODE "
    include(BundleUtilities)
    fixup_bundle(\"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/lrr-portable.app\" \"\" \"\")
    execute_process(COMMAND /usr/bin/codesign --force --deep --sign -
      \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/lrr-portable.app\"
      COMMAND_ERROR_IS_FATAL ANY)
    ")
endif()
if(WIN32)
  # A shared SDL3 (an installed package; the fetched one is static) travels
  # with the exe, as the post-build step copies it beside the build's.
  install(FILES $<TARGET_RUNTIME_DLLS:program> DESTINATION "${LRR_INSTALL_BINDIR}")
endif()
if(NOT APPLE)
  install(FILES "${PROJECT_SOURCE_DIR}/LICENSE" DESTINATION "${LRR_INSTALL_DOCDIR}")
endif()

# The package name: lrr-portable-0.1.0-Linux-x86_64, -Darwin-arm64,
# -Windows-x64 ...
if(MSVC)
  string(TOLOWER "${MSVC_CXX_ARCHITECTURE_ID}" _lrr_arch)
elseif(APPLE AND CMAKE_OSX_ARCHITECTURES)
  string(REPLACE ";" "-" _lrr_arch "${CMAKE_OSX_ARCHITECTURES}")
else()
  string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" _lrr_arch)
endif()
set(LRR_PACKAGE_NAME "${CMAKE_SYSTEM_NAME}-${_lrr_arch}"
  CACHE STRING "Platform name of the package")

set(CPACK_PACKAGE_NAME "lrr-portable")
set(CPACK_PACKAGE_VENDOR "lrr-portable")
set(CPACK_PACKAGE_VERSION "${LRR_SEMANTIC_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${LRR_APP_SUMMARY}")
set(CPACK_PACKAGE_HOMEPAGE_URL "${LRR_APP_URL}")
# No CPACK_RESOURCE_FILE_LICENSE: DragNDrop would embed it as a licence
# agreement that a disk image cannot be mounted without accepting. The licence
# text is installed beside lrr-portable in the Linux and Windows packages.
set(CPACK_PACKAGE_DIRECTORY "dist")
set(CPACK_PACKAGE_FILE_NAME "lrr-portable-${PROJECT_VERSION}-${LRR_PACKAGE_NAME}")
if(WIN32)
  set(CPACK_GENERATOR ZIP)
elseif(APPLE)
  set(CPACK_GENERATOR DragNDrop)
  set(CPACK_DMG_VOLUME_NAME "${LRR_APP_NAME}")
else()
  set(CPACK_GENERATOR TGZ)
endif()
include(CPack)
