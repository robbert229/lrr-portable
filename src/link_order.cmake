# SPDX-License-Identifier: LGPL-3.0-or-later
#
# The link-order list (ADR 0006 decision 1; GLOSSARY.md). It is the only
# authority on the order in which the matching build links its objects, and
# it decides which library each source belongs to in the native and
# reference builds (ADR 0003 as amended), where order does not matter.
#
# One entry per object, in link order:
#
#   lrr_link(<kind> <key> <library> <source> [<platform>])
#
#   kind      unit    the file holds exactly one original unit (converted to
#                     original shape as <stem>.c, or relocated as <stem>.cpp);
#                     key is its unit id in translation-unit-ranges.csv
#             legacy  a mixed file that still holds original functions; key is
#                     the lower median original address of its FUNCTION and LIBRARY
#                     markers (for an even count, use the lower middle), which
#                     places it among the units
#             tail    port-only code: the port tail (ADR 0006 decision 4);
#                     key is "-"
#   key       see kind; units and legacy files are in ascending key order
#   library   game, gods98, platform_glue or program (the executable's own
#             sources)
#   source    path under SRC_ROOT
#   platform  native or winapi when the object belongs to one backend only
#
# Order: the game segment, then the gods98 segment, then the port tail, then
# the legacy files whose functions lie in the CRT range. Libraries follow, in
# LRR_LINK_LIBRARIES. Fields are separated by blanks; one entry per line, so
# tools read this file without CMake (tools/link_order.py).
#
# tools/convert_unit.py inserts a converted unit's entry here. A converted
# unit's .c file is compiled as C in the matching build and as C++ elsewhere.

# Ordered game and gods98 segments, port tail, then CRT-range legacy files.
lrr_link(unit   GAME-001 game          game/advisor.cpp)
lrr_link(legacy 00403a50 game          game/cmd.cpp)
lrr_link(unit   GAME-003 game          game/bezier.c)
lrr_link(unit   GAME-004 game          game/animation_object.cpp)
lrr_link(unit   GAME-006 game          game/building_model.cpp)
lrr_link(unit   GAME-007 game          game/placement.cpp)
lrr_link(unit   GAME-008 game          game/entity_cell_effects.cpp)
lrr_link(unit   GAME-009 game          game/screen_effect_quads.cpp)
lrr_link(unit   GAME-010 game          game/dependencies.cpp)
lrr_link(unit   GAME-014 game          game/encyclopedia.cpp)
lrr_link(unit   GAME-015 game          game/lava_erosion.cpp)
lrr_link(unit   GAME-016 game          game/steering.cpp)
lrr_link(legacy 00416910 game          game/options.cpp)
lrr_link(legacy 00417eb0 game          game/savegame.cpp)
lrr_link(legacy 00418920 game          game/dependency_grid.cpp)
lrr_link(legacy 00419b20 gods98        gods98/message.cpp)
lrr_link(legacy 00419f10 game          game/util.cpp)
lrr_link(legacy 0041a920 game          game/upgrades.cpp)
lrr_link(legacy 0041baa0 game          game/frontend.cpp)
lrr_link(legacy 0041e820 gods98        gods98/input.cpp)
lrr_link(legacy 0041f420 game          game/select.cpp)
lrr_link(legacy 004250c0 gods98        gods98/mainsim.cpp)
lrr_link(legacy 004263a0 gods98        gods98/engine.cpp)
lrr_link(legacy 0042cb70 gods98        gods98/camera.cpp)
lrr_link(legacy 0042d100 game          game/entity_config.cpp)
lrr_link(legacy 004329d0 game          game/level.cpp)
lrr_link(legacy 00433770 game          game/scene_render.cpp)
lrr_link(legacy 004351a0 gods98        gods98/media.cpp)
lrr_link(legacy 0043c280 game          game/ai.cpp)
lrr_link(legacy 004410f0 game          game/terrain_grid.cpp)
lrr_link(legacy 00444fb0 game          game/entity.cpp)
lrr_link(legacy 00447210 game          game/spawn.cpp)
lrr_link(legacy 00449c00 gods98        gods98/simtick.cpp)
lrr_link(legacy 004502e0 game          game/screen_pick.cpp)
lrr_link(legacy 00458580 game          game/objective.cpp)
lrr_link(legacy 00459560 game          game/tutorial_hud.cpp)
lrr_link(legacy 0045b740 game          game/ui_list.cpp)
lrr_link(unit   GAME-034 game          game/priorities.c)
lrr_link(legacy 00460360 game          game/mission.cpp)
lrr_link(legacy 004628e0 gods98        gods98/ptrstack.cpp)
lrr_link(legacy 004652d0 gods98        gods98/sound.cpp)
lrr_link(legacy 00465ac0 game          game/effects.cpp)
lrr_link(legacy 0046f140 game          game/weapon.cpp)
lrr_link(legacy 004712c0 gods98        gods98/record.cpp)

# gods98 segment. G98-001 is still in src/game and the game library;
# it moves with its relocation.
lrr_link(unit   G98-001  game          game/video_graph.cpp)
lrr_link(legacy 00472950 gods98        gods98/scene_pool.cpp)
lrr_link(legacy 00472cd0 gods98        gods98/entity.cpp)
lrr_link(legacy 00474710 gods98        gods98/texture.cpp)
lrr_link(legacy 00475140 gods98        gods98/view.cpp)
lrr_link(legacy 00476fd0 gods98        gods98/scene_resources.cpp)
lrr_link(legacy 00477220 gods98        gods98/resource.cpp)
lrr_link(unit   G98-004  gods98        gods98/utils.c)
lrr_link(legacy 004777a0 program       platform/winapi/entry.cpp winapi)
lrr_link(legacy 00478010 gods98        gods98/frameloop.cpp)
lrr_link(legacy 004783f0 platform_glue platform/winapi/frameloop.cpp winapi)
lrr_link(unit   G98-006  gods98        gods98/config.c)
lrr_link(unit   G98-007  gods98        gods98/maths.c)
lrr_link(legacy 0047a800 gods98        gods98/audio.cpp)
lrr_link(legacy 0047d2d0 gods98        gods98/surface.cpp)
lrr_link(legacy 0047e9e0 gods98        gods98/movie.cpp)
lrr_link(legacy 00484880 gods98        gods98/flic.cpp)
lrr_link(legacy 00485160 gods98        gods98/heap.cpp)
lrr_link(unit   G98-018  gods98        gods98/keys.c)
lrr_link(unit   G98-019  gods98        gods98/draw.c)
lrr_link(unit   G98-021  gods98        gods98/soundsys.c)
lrr_link(unit   G98-024  gods98        gods98/g98_024.c)
lrr_link(unit   G98-026  gods98        gods98/g98_026.c)
lrr_link(unit   G98-027  gods98        gods98/dxerror.c)
lrr_link(legacy 0048b290 game          game/startup_config.cpp)
lrr_link(unit   G98-029  gods98        gods98/g98_029.c)
lrr_link(legacy 0048c040 gods98        gods98/string.cpp)
lrr_link(legacy 0048c210 gods98        gods98/model.cpp)

# Port tail
lrr_link(tail   -        game          game/actor_scene.cpp winapi)
lrr_link(tail   -        game          game/building_scene.cpp winapi)
lrr_link(tail   -        game          game/entity_motion_owner.cpp)
lrr_link(tail   -        game          game/game.cpp)
lrr_link(tail   -        game          game/opgroup.cpp)
lrr_link(tail   -        game          game/render.cpp)
lrr_link(tail   -        game          game/tutorial_world_arrows.cpp)
lrr_link(tail   -        gods98        gods98/block.cpp)
lrr_link(tail   -        gods98        gods98/draw_port.cpp)
lrr_link(tail   -        gods98        gods98/gods98.cpp)
lrr_link(tail   -        gods98        gods98/lwsc_scene.cpp)
lrr_link(tail   -        gods98        gods98/lwsc_winapi.cpp winapi)
lrr_link(tail   -        gods98        gods98/registry_port.cpp)
lrr_link(tail   -        gods98        gods98/sound_wave.cpp)
lrr_link(tail   -        gods98        gods98/tickers.cpp)
lrr_link(tail   -        gods98        gods98/winmm_port.cpp)
lrr_link(tail   -        gods98        crt/crt_runtime_aliases.cpp)
lrr_link(tail   -        gods98        crt/crt_runtime_calls.cpp)
lrr_link(tail   -        program       main.cpp)
lrr_link(tail   -        program       platform/native/entry.cpp native)
lrr_link(tail   -        platform_glue platform/native/frameloop.cpp native)

# Legacy files whose functions lie in the CRT range, just before LIBC
lrr_link(legacy 0048ed00 game          game/seh_runtime.cpp)
lrr_link(legacy 00491710 gods98        crt/crt_heap.cpp)
lrr_link(legacy 00491fe0 gods98        crt/crt.cpp)

# Libraries of the matching build, after the objects: LIBC (the /ML runtime),
# oldnames.lib, then the import libraries in the original's import-thunk
# order (finding 505 rule 7), then the others in the order of the original's
# import directory. The toolchain has no dinput.lib; vfw32.lib holds the
# avifil32 imports.
set(LRR_LINK_LIBRARIES libc.lib oldnames.lib
  d3drm.lib ddraw.lib vfw32.lib dsound.lib kernel32.lib
  ole32.lib winmm.lib user32.lib gdi32.lib advapi32.lib)
