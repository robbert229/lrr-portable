# SPDX-License-Identifier: LGPL-3.0-or-later
# The tests of the native platform layer and of miniwin below the seam
# (src/platform/native).
# CMakeLists.txt includes this file inside if(BUILD_TESTING), after
# cmake/LrrTesting.cmake; docs/building.md, "Writing a test".

# The web build's manifest of game files (emscripten/manifest.cpp, plain
# C++): which lines the parser keeps.
lrr_unit_test(web_manifest
  SOURCES
    ${PLATFORM_DIR}/native/emscripten/test_manifest.cpp
    ${PLATFORM_DIR}/native/emscripten/manifest.cpp
  INCLUDES ${SRC_ROOT}
  LINK
  LABELS config)
# Skipped (77) where the preference directory cannot be moved into the
# fixture.
lrr_unit_test(config SOURCES ${PLATFORM_DIR}/native/test_config.cpp
  LABELS config
  SKIP_RETURN_CODE 77)
# The host's suspend, resume and exit events (platform/native/lifecycle.h):
# what the game's main loop reads as its activity, and the exit request.
lrr_unit_test(lifecycle SOURCES ${PLATFORM_DIR}/native/test_lifecycle.cpp
  LABELS config)
# The frame rate the FPS overlay shows, over windows of half a second.
lrr_unit_test(fps_counter
  SOURCES
    ${PLATFORM_DIR}/native/test_fps_counter.cpp
    ${PLATFORM_DIR}/native/fps_counter.cpp
  INCLUDES ${SRC_ROOT} ${SRC_ROOT}/test
  LABELS config)
# The settings, set in-process: no file, so on every host.
lrr_unit_test(config_settings SOURCES ${PLATFORM_DIR}/native/test_config_settings.cpp
  LABELS config)
# The 3D device Init.c chooses, from synthetic device lists with each
# platform's default device.
lrr_unit_test(device_choice SOURCES ${PLATFORM_DIR}/native/test_device_choice.cpp
  LABELS config)
# The input latch and the touch gestures (issue #986), driven with
# synthetic events: plain C++, no SDL calls (SDL3 only for its headers).
lrr_unit_test(touch_input
  SOURCES
    ${PLATFORM_DIR}/native/test_touch_input.cpp
    ${PLATFORM_DIR}/native/input_latch.cpp
    ${PLATFORM_DIR}/native/touch_gestures.cpp
  LINK SDL3::SDL3
  INCLUDES ${SRC_ROOT}
  LABELS input)

# The DirectX 6 interface versions the units use, declared in miniwin
# (ADR 0007, decision 3): reachable, and failing rather than crashing
# where miniwin has only a stub.
lrr_unit_test(miniwin_dx6 SOURCES ${PLATFORM_DIR}/native/test_miniwin_dx6.cpp
  LINK miniwin
  LABELS miniwin)

# What the boot to the main menu needs from miniwin, on synthetic data:
# display modes, DirectDraw 4 surfaces and blits, the framebuffer, and
# Direct3DRM frame transforms.
lrr_unit_test(miniwin_ddraw4 SOURCES ${PLATFORM_DIR}/native/test_miniwin_ddraw4.cpp
  LINK miniwin
  LABELS miniwin)

# Immediate-mode drawing through a user visual on the software renderer
# (ADR 0008, step 1): coverage, z, blending and culling read back by Lock.
lrr_unit_test(miniwin_immediate SOURCES ${PLATFORM_DIR}/native/test_miniwin_immediate.cpp
  LINK miniwin
  LABELS miniwin)

# Viewports and clears (ADR 0008, step 2): two viewports on one target,
# Clear2 rectangles, the scissor, and z clears only on request.
lrr_unit_test(miniwin_viewports SOURCES ${PLATFORM_DIR}/native/test_miniwin_viewports.cpp
  LINK miniwin
  LABELS miniwin)

# Textures (ADR 0008, step 3): 8-bit colour-keyed texture surfaces drawn
# through SetTexture and TEXTUREHANDLE, stage 0 ops, linear filtering, and
# D3DRM decal transparency.
lrr_unit_test(miniwin_textures SOURCES ${PLATFORM_DIR}/native/test_miniwin_textures.cpp
  LINK miniwin
  LABELS miniwin)

# Lights (ADR 0008, step 4): spot cones, range, attenuation, light colour,
# enable frames, and the material's emissive, ambient and specular terms.
lrr_unit_test(miniwin_lights SOURCES ${PLATFORM_DIR}/native/test_miniwin_lights.cpp
  LINK miniwin
  LABELS miniwin)

# Fog (ADR 0008, step 5): the scene's fog reaching the device's fog
# states, linear, exponential and exponential-squared, per vertex and per
# pixel, and Mesh.c's FOGENABLE off for its alpha-blended groups.
lrr_unit_test(miniwin_fog SOURCES ${PLATFORM_DIR}/native/test_miniwin_fog.cpp
  LINK miniwin
  LABELS miniwin)

# The rendering checks above again on the SDL GPU backend (ADR 0008,
# step 6), and the same scene on both backends compared pixel by pixel.
# They need an SDL GPU device that can draw on SDL's offscreen video
# driver, such as Mesa's lavapipe; without one, as on CI, they are skipped.
# Retained-mode checks run on the software backend only.
foreach(name immediate viewports textures lights fog)
  lrr_unit_test_variant(miniwin_${name}_sdlgpu OF miniwin_${name}
    LABELS miniwin sdlgpu
    SKIP_RETURN_CODE 77
    ENVIRONMENT "MINIWIN_TEST_DEVICE=SDL3 GPU HAL" "MINIWIN_TEST_VIDEO_DRIVER=offscreen")
endforeach()
lrr_unit_test(miniwin_backends SOURCES ${PLATFORM_DIR}/native/test_miniwin_backends.cpp
  LINK miniwin
  LABELS miniwin sdlgpu
  SKIP_RETURN_CODE 77)

# The same checks, and the same comparison with the software backend, on
# the OpenGL ES 3 backend (#925). They need an OpenGL ES 3 context on an
# OpenGL window on MINIWIN_TEST_VIDEO_DRIVER (offscreen, through EGL);
# without one, as on CI, they are skipped. Override the driver with
# LRR_MINIWIN_GLES3_VIDEO_DRIVER (x11 or wayland on a desktop).
set(LRR_MINIWIN_GLES3_VIDEO_DRIVER "offscreen" CACHE STRING
  "SDL video driver the miniwin OpenGL ES 3 tests draw on")
foreach(name immediate viewports textures lights fog backends)
  lrr_unit_test_variant(miniwin_${name}_gles3 OF miniwin_${name}
    LABELS miniwin gles3
    SKIP_RETURN_CODE 77
    ENVIRONMENT "MINIWIN_TEST_DEVICE=OpenGL ES 3.0 HAL"
      "MINIWIN_TEST_VIDEO_DRIVER=${LRR_MINIWIN_GLES3_VIDEO_DRIVER}")
endforeach()

# Main.c's window procedure under miniwin: SDL mouse events as client
# coordinate messages, and SDL's quit event as WM_CLOSE.
lrr_unit_test(miniwin_messages SOURCES ${PLATFORM_DIR}/native/test_miniwin_messages.cpp
  LINK miniwin
  LABELS miniwin)

# The desktop window's rectangle: the primary display's bounds, so that
# Main.c's fullscreen window goes fullscreen on the primary display.
lrr_unit_test(miniwin_desktop SOURCES ${PLATFORM_DIR}/native/test_miniwin_desktop.cpp
  LINK miniwin
  LABELS miniwin)

# The virtual display fitted into a window of another size: the fit, the
# render size and filtering rule, the mouse through the fit, and a frame
# buffer lock that draws only the pixels it changed
# (miniwin/miniwinpresentation.h).
lrr_unit_test(miniwin_presentation SOURCES ${PLATFORM_DIR}/native/test_miniwin_presentation.cpp
  LINK miniwin
  LABELS miniwin
  SKIP_RETURN_CODE 77)

# The arithmetic of miniwin's Citro3D immediate mode, which runs only on a
# 3DS: its header (src/internal/citro3dimmediate.h) builds on any host.
lrr_unit_test(miniwin_citro3d SOURCES ${PLATFORM_DIR}/native/test_miniwin_citro3d.cpp
  INCLUDES "${MINIWIN}/src/internal"
  LINK miniwin
  LABELS miniwin)

# miniwin's key messages: Windows virtual-key codes and lParam flags from
# SDL key events, WM_SYSKEY* for Alt and F10, and Alt+F4 as WM_CLOSE.
lrr_unit_test(miniwin_keys SOURCES ${PLATFORM_DIR}/native/test_miniwin_keys.cpp
  LINK miniwin
  LABELS miniwin)

lrr_unit_test(startup_options SOURCES ${PLATFORM_DIR}/native/test_startup_options.cpp
  LABELS config)
foreach(scenario IN ITEMS fit original best)
  lrr_unit_test_variant(startup_device_${scenario} OF startup_options
    ARGS ${scenario} LABELS config ENVIRONMENT "SDL_VIDEO_DRIVER=dummy" SKIP_RETURN_CODE 77)
endforeach()

# Only the external device availability/setup boundary is controlled here.
lrr_unit_test(startup_device_errors SOURCES ${PLATFORM_DIR}/native/test_startup_device_errors.cpp
  WRAP directdraw_enum_drivers directdraw_enum_devices directdraw_setup
  ARGS no-driver LABELS config ENVIRONMENT "SDL_VIDEO_DRIVER=dummy")
foreach(scenario IN ITEMS no-hardware setup-fails)
  lrr_unit_test_variant(startup_device_${scenario} OF startup_device_errors
    ARGS ${scenario} LABELS config ENVIRONMENT "SDL_VIDEO_DRIVER=dummy")
endforeach()
