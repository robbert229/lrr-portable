# SPDX-License-Identifier: LGPL-3.0-or-later
#
# The source list: every source of the native build (ADR 0007). The order
# of entries does not matter to the build; it keeps the original's unit
# order (ADR 0006 on the decomp branch) so the units can be read in the order the
# original linked them.
#
# One entry per source:
#
#   lrr_link(<kind> <key> <library> <source>)
#
#   kind      unit      the file holds exactly one original unit (ADR 0005;
#                       ADR 0007 decision 1); key is its unit id in
#                       docs/coverage/translation-unit-ranges.csv
#             platform  port-only code below the seam: the process entry
#                       and the seam's stubs; key is "-"
#             library   port-only library code that a unit's library
#                       substitution calls (ADR 0007 decision 3), such as
#                       the FFmpeg video library; key is "-"
#   library   game, gods98, platform or library
#   source    path under SRC_ROOT
#
# Fields are separated by blanks, one entry per line.

# The game units, then the gods98 units.
lrr_link(unit   GAME-001 game          game/advisor.c)
lrr_link(unit   GAME-002 game          game/aitask.c)
lrr_link(unit   GAME-003 game          game/bezier.c)
lrr_link(unit   GAME-004 game          game/animation_object.c)
lrr_link(unit   GAME-005 game          game/bubbles.c)
lrr_link(unit   GAME-006 game          game/building_model.c)
lrr_link(unit   GAME-007 game          game/collision.c)
lrr_link(unit   GAME-052 game          game/construction.c)
lrr_link(unit   GAME-008 game          game/entity_cell_effects.c)
lrr_link(unit   GAME-009 game          game/screen_effect_quads.c)
lrr_link(unit   GAME-010 game          game/dependencies.c)
lrr_link(unit   GAME-011 game          game/detail.c)
lrr_link(unit   GAME-012 game          game/rock_fall_styles.c)
lrr_link(unit   GAME-013 game          game/terrain_cell_objects.c)
lrr_link(unit   GAME-014 game          game/encyclopedia.c)
lrr_link(unit   GAME-015 game          game/erode.c)
lrr_link(unit   GAME-051 game          game/fallin.c)
lrr_link(unit   GAME-016 game          game/steering.c)
lrr_link(unit   GAME-017 game          game/frontend_unit.c)
lrr_link(unit   GAME-018 game          game/helpwindow.c)
lrr_link(unit   GAME-019 game          game/info_unit.c)
lrr_link(unit   GAME-020 game          game/interface_unit.c)
lrr_link(unit   GAME-021 game          game/lego_unit.c)
lrr_link(unit   GAME-022 game          game/camera_control.c)
lrr_link(unit   GAME-023 game          game/legoobject_unit.c)
lrr_link(unit   GAME-024 game          game/lighteffects.c)
lrr_link(unit   GAME-025 game          game/loader.c)
lrr_link(unit   GAME-026 game          game/terrain_map.c)
lrr_link(unit   GAME-027 game          game/message_unit.c)
lrr_link(unit   GAME-028 game          game/tutorial_script.c)
lrr_link(unit   GAME-029 game          game/objective_unit.c)
lrr_link(unit   GAME-030 game          game/objectrecall.c)
lrr_link(unit   GAME-031 game          game/objinfo.c)
lrr_link(unit   GAME-032 game          game/panel_unit.c)
lrr_link(unit   GAME-033 game          game/pointer.c)
lrr_link(unit   GAME-034 game          game/priorities.c)
lrr_link(unit   GAME-035 game          game/ptl.c)
lrr_link(unit   GAME-036 game          game/radarmap.c)
lrr_link(unit   GAME-037 game          game/reward.c)
lrr_link(unit   GAME-038 game          game/roof.c)
lrr_link(unit   GAME-039 game          game/scrollinfo.c)
lrr_link(unit   GAME-040 game          game/selectplace.c)
lrr_link(unit   GAME-041 game          game/sfx.c)
lrr_link(unit   GAME-042 game          game/spiderweb.c)
lrr_link(unit   GAME-043 game          game/entity_stats.c)
lrr_link(unit   GAME-044 game          game/teleporter.c)
lrr_link(unit   GAME-045 game          game/text.c)
lrr_link(unit   GAME-046 game          game/tooltip.c)
lrr_link(unit   GAME-047 game          game/upgrade.c)
lrr_link(unit   GAME-048 game          game/vehicle_model.c)
lrr_link(unit   GAME-049 game          game/water.c)
lrr_link(unit   GAME-050 game          game/weapons.c)

# gods98 segment.
lrr_link(unit   G98-001  gods98        gods98/movie_unit.cpp)
lrr_link(unit   G98-002  gods98        gods98/container.c)
lrr_link(unit   G98-003  gods98        gods98/viewports.c)
lrr_link(unit   G98-004  gods98        gods98/utils.c)
lrr_link(unit   G98-005  gods98        gods98/main.c)
lrr_link(unit   G98-006  gods98        gods98/config.c)
lrr_link(unit   G98-007  gods98        gods98/maths.c)
lrr_link(unit   G98-008  gods98        gods98/font.c)
lrr_link(unit   G98-009  gods98        gods98/sound3d.c)
lrr_link(unit   G98-010  gods98        gods98/directdraw.c)
lrr_link(unit   G98-011  gods98        gods98/images_unit.c)
lrr_link(unit   G98-012  gods98        gods98/avi_player.cpp)
lrr_link(unit   G98-013  gods98        gods98/input_unit.c)
lrr_link(unit   G98-014  gods98        gods98/files.c)
lrr_link(unit   G98-015  gods98        gods98/mesh_unit.c)
lrr_link(unit   G98-016  gods98        gods98/flic_unit.c)
lrr_link(unit   G98-017  gods98        gods98/textwindow.c)
lrr_link(unit   G98-018  gods98        gods98/keys.c)
lrr_link(unit   G98-019  gods98        gods98/draw.c)
lrr_link(unit   G98-020  gods98        gods98/lws.c)
lrr_link(unit   G98-021  gods98        gods98/soundsys.c)
lrr_link(unit   G98-024  gods98        gods98/g98_024.c)
lrr_link(unit   G98-025  gods98        gods98/animclone.c)
lrr_link(unit   G98-026  gods98        gods98/g98_026.c)
lrr_link(unit   G98-027  gods98        gods98/dxerror.c)
lrr_link(unit   G98-028  gods98        gods98/file_list_streams.c)
lrr_link(unit   G98-029  gods98        gods98/g98_029.c)
lrr_link(unit   G98-030  gods98        gods98/wad.c)
lrr_link(unit   G98-031  gods98        gods98/lwo.c)
lrr_link(unit   LATE-001 gods98        gods98/rnc.c)
lrr_link(unit   LATE-002 gods98        gods98/init.c)

# The native platform: the process entry, which calls the units' WinMain,
# the seam's remaining stubs, the filesystem substitution, the
# configuration file and the input layer under the keyboard substitution,
# with its latch and touch gestures (ADR 0007). The hosts' own sources are
# added in CMakeLists.txt.
lrr_link(platform -     platform      platform/native/entry.cpp)
lrr_link(platform -     platform      platform/native/seam_stubs.cpp)
lrr_link(platform -     platform      platform/native/files_native.cpp)
lrr_link(platform -     platform      platform/native/config.cpp)
lrr_link(platform -     platform      platform/native/lifecycle.cpp)
lrr_link(platform -     platform      platform/native/fps_counter.cpp)
lrr_link(platform -     platform      platform/native/input.cpp)
lrr_link(platform -     platform      platform/native/input_latch.cpp)
lrr_link(platform -     platform      platform/native/touch_gestures.cpp)

# The libraries the units' substitutions call (ADR 0007).
lrr_link(library  -     library       library/video_ffmpeg.cpp)
lrr_link(library  -     library       library/audio_sdl.cpp)
lrr_link(library  -     library       library/miniaudio_impl.c)
