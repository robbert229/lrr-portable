# SPDX-License-Identifier: LGPL-3.0-or-later
#
# Packages (issue #922): `cpack` in the build directory writes
# lrr-portable-<version>-<system>-<arch> to build/dist, from `install()`:
#   - Linux:   a TGZ with LegoRR, the desktop entry, the AppStream metadata and
#              the icon; packaging/linux also holds the AppImage and Flatpak
#              recipes the CI packages workflow runs.
#   - macOS:   a DragNDrop disk image holding LegoRR.app (packaging/macos),
#              and a ZIP of the same for RecompLauncher; with LRR_FFMPEG_DIR,
#              the app bundles FFmpeg for the movies.
#   - Windows: a ZIP with LegoRR.exe, SDL3.dll beside it when SDL is shared,
#              and the Visual C++ runtime DLLs in an MSVC build.
# Every package carries LICENSE and THIRD-PARTY-NOTICES.txt.
# The layout follows isle-portable's packaging/ (LGPL-3.0-or-later; keep the
# attribution when adapting more of it). No package holds game data: every one
# expects the owner's own LegoRR0.wad and LegoRR1.wad, found through `diskpath`
# in LegoRR.ini (README.md).

# The reverse-DNS application id, shared by the desktop entry, the AppStream
# component, the Flatpak and the macOS bundle identifier. It follows Flathub's
# convention for a project hosted on GitHub: io.github.<owner>.<name>.
set(LRR_APP_ID "io.github.robbert229.LegoRR")
set(LRR_APP_NAME "LRR Portable")
set(LRR_APP_SUMMARY "Portable reconstruction of Lego Rock Raiders")
set(LRR_APP_SPDX "LGPL-3.0-or-later")
set(LRR_APP_URL "https://github.com/robbert229/lrr-portable")

# The AppStream release date. SOURCE_DATE_EPOCH keeps it reproducible.
string(TIMESTAMP LRR_BUILD_DATE "%Y-%m-%d" UTC)
# CI numbers the version recorded in the package metadata and the AppStream
# release by run, as isle-portable does, so that two runs of one version sort
# apart. File names carry the plain version. A run on a tag (the release
# workflow, docs/releasing.md) records the plain version as a stable release,
# or as a development release when the version carries a pre-release label.
if("$ENV{GITHUB_ACTIONS}" STREQUAL "true" AND NOT "$ENV{GITHUB_RUN_NUMBER}" STREQUAL ""
    AND NOT "$ENV{GITHUB_REF_TYPE}" STREQUAL "tag")
  set(LRR_SEMANTIC_VERSION "${LRR_VERSION}~build$ENV{GITHUB_RUN_NUMBER}")
  set(LRR_RELEASE_TYPE "development")
elseif("$ENV{GITHUB_REF_TYPE}" STREQUAL "tag")
  set(LRR_SEMANTIC_VERSION "${LRR_VERSION}")
  if(LRR_VERSION_PRERELEASE STREQUAL "")
    set(LRR_RELEASE_TYPE "stable")
  else()
    set(LRR_RELEASE_TYPE "development")
  endif()
else()
  set(LRR_SEMANTIC_VERSION "${LRR_VERSION}")
  set(LRR_RELEASE_TYPE "development")
endif()

# The licence and the third-party notices travel in every package
# (docs/releasing.md); tools/check_package.py fails a package without them.
set(LRR_NOTICE_FILES
  "${PROJECT_SOURCE_DIR}/LICENSE"
  "${PROJECT_SOURCE_DIR}/THIRD-PARTY-NOTICES.txt")

# Windows puts LegoRR.exe and the licence at the root of its ZIP; elsewhere
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
  # LegoRR.app above.
  # Pull in any shared library the bundle links (none with the fetched, static
  # SDL3), add the bundled FFmpeg, which the game loads at run time and so
  # fixup_bundle never sees, then sign, so that the signature covers the
  # final contents.
  set(_lrr_ffmpeg_install "")
  foreach(_lrr_lib IN LISTS LRR_BUNDLED_FFMPEG_LIBS)
    # The library itself, under the name the others link it by.
    get_filename_component(_lrr_name "${_lrr_lib}" NAME)
    get_filename_component(_lrr_real "${_lrr_lib}" REALPATH)
    string(APPEND _lrr_ffmpeg_install "
    file(INSTALL \"${_lrr_real}\"
      DESTINATION \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/LegoRR.app/Contents/Frameworks\"
      RENAME \"${_lrr_name}\")")
  endforeach()
  if(LRR_BUNDLED_FFMPEG_LIBS)
    string(APPEND _lrr_ffmpeg_install "
    file(INSTALL \"${LRR_FFMPEG_DIR}/share/ffmpeg/COPYING.LGPLv2.1\"
      DESTINATION \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/LegoRR.app/Contents/Resources\"
      RENAME \"FFmpeg-COPYING.LGPLv2.1.txt\")")
  endif()
  install(CODE "
    include(BundleUtilities)
    fixup_bundle(\"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/LegoRR.app\" \"\" \"\")
    ${_lrr_ffmpeg_install}
    execute_process(COMMAND /usr/bin/codesign --force --deep --sign -
      \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/LegoRR.app\"
      COMMAND_ERROR_IS_FATAL ANY)
    ")
endif()
if(WIN32)
  # A shared SDL3 (an installed package; the fetched one is static) travels
  # with the exe, as the post-build step copies it beside the build's.
  install(FILES $<TARGET_RUNTIME_DLLS:program> DESTINATION "${LRR_INSTALL_BINDIR}")
  # MSVC links the C++ runtime as DLLs (msvcp140, vcruntime140), which a
  # Windows without the Visual C++ Redistributable lacks; they go beside
  # LegoRR.exe. The universal CRT is part of Windows 10 and later.
  if(MSVC)
    set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION "${LRR_INSTALL_BINDIR}")
    set(CMAKE_INSTALL_UCRT_LIBRARIES OFF)
    include(InstallRequiredSystemLibraries)
  endif()
  # Windows opens files by extension.
  install(FILES "${PROJECT_SOURCE_DIR}/LICENSE" RENAME "LICENSE.txt"
    DESTINATION "${LRR_INSTALL_DOCDIR}")
  install(FILES "${PROJECT_SOURCE_DIR}/THIRD-PARTY-NOTICES.txt"
    DESTINATION "${LRR_INSTALL_DOCDIR}")
elseif(APPLE)
  # Inside the bundle (macos/CMakeLists.txt) and beside it in the disk image.
  install(FILES ${LRR_NOTICE_FILES} DESTINATION ".")
else()
  install(FILES ${LRR_NOTICE_FILES} DESTINATION "${LRR_INSTALL_DOCDIR}")
endif()

# RecompLauncher reads recomplauncher.json from the top of the ZIP or TGZ (the
# AppImage's copy goes to its root: appimage/build.sh). On macOS it installs
# the ZIP, whose name says the system, over the disk image; the disk image
# carries the same copy beside LegoRR.app, harmlessly.
if(WIN32 OR APPLE OR CMAKE_SYSTEM_NAME STREQUAL "Linux")
  install(FILES recomplauncher.json DESTINATION ".")
endif()

# The package name: lrr-portable-<version>-Linux-x86_64, -Darwin-arm64,
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
# and notices are installed beside LegoRR instead (above).
set(CPACK_PACKAGE_DIRECTORY "dist")
set(CPACK_PACKAGE_FILE_NAME "lrr-portable-${LRR_VERSION}-${LRR_PACKAGE_NAME}")
if(WIN32)
  set(CPACK_GENERATOR ZIP)
elseif(APPLE)
  # The disk image is for players; RecompLauncher installs the ZIP, a .app
  # at its top with recomplauncher.json beside it.
  set(CPACK_GENERATOR DragNDrop ZIP)
  set(CPACK_DMG_VOLUME_NAME "${LRR_APP_NAME}")
else()
  set(CPACK_GENERATOR TGZ)
endif()
include(CPack)
