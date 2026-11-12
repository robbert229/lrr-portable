// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-041 SFX.c (finding 585): the port's link names of the unit's
 * functions, for the matching build only (as finding 581's
 * entity_stats_link_names.h).
 *
 * In the matching build (MSVC 5.0, _MSC_VER 1100) every caller of these
 * names passes the original's arguments: sample and SFX ids, sound
 * handles, flags, the container or frame a sound plays on, the smoke
 * emitter records this unit creates, and the Sound3D.c handles sound3d.c
 * returns (the port's sound bank is that unit too). So the unit's
 * definitions take the port's names: C and C++ callers then reach them
 * without an alias header or a caller edit, and the port tail sfx_port.cpp
 * compiles its own definitions of these names out of the matching build.
 * Natively the port tail keeps serving them: it hashes aliases its own
 * way, keeps its own chain pool and drives smoke through the port's
 * renderer hooks.
 *
 * Included only by sfx.c, before any other header. A macro renames a
 * declaration or a definition one for one, so MSVC's symbol numbering
 * (finding 538) does not change. The three functions the port never
 * modelled (00464c20, 00465250, 00465ee0) keep the unit's names. */
#ifndef LEGORRR_GAME_SFX_LINK_NAMES_H
#define LEGORRR_GAME_SFX_LINK_NAMES_H

#if defined(_MSC_VER) && _MSC_VER < 1200
#define sfx_initialise sound_alias_registry_init /* 00464740 */
#define sfx_set_sample_populate_mode sound_alias_registration_set /* 00464c50 */
#define sfx_get_type sound_alias_lookup /* 00464c70 */
#define sfx_load_sample_property sound_sample_marker_parse /* 00464d00 */
#define sfx_random_get_sound3d_handle sound_variant_select /* 00464e20 */
#define sfx_stop_global_sample sound_deferred_play_expire /* 00464e80 */
#define sfx_set_global_sample_duration_if_le0 sound_deferred_timer_set_if_idle /* 00464ec0 */
#define sfx_random_set_and_play_global_sample sound_submit_with_global_cooldown /* 00464ef0 */
#define sfx_add_to_queue sound_request_queue_submit /* 00464f60 */
#define sfx_random_play_sound_normal sound_alias_play /* 00464fa0 */
#define sfx_random_set_buffer_volume sound_variant_property_set /* 00465010 */
#define sfx_random_get_buffer_volume sound_variant_property_read /* 00465030 */
#define sfx_random_play_sound3d_on_container sound_submission_route_wrapper /* 00465050 */
#define sfx_random_play_sound3d_on_frame_internal sound_positional_sound_submit /* 00465090 */
#define sfx_random_get_sample_play_time sound_object_duration_get /* 00465160 */
#define sfx_sound3d_stop_sound sound_handle_stop_forward /* 00465190 */
#define sfx_update sound_subsystem_frame_tick /* 004651a0 */
#define sfx_set_sound_on sound_subsystem_enable_set /* 00465260 */
#define sfx_set_sound_on_and_stop_all sound_subsystem_enable_set_and_stop /* 004652a0 */
#define sfx_is_queue_mode sound_batch_mode_get /* 004652b0 */
#define sfx_set_queue_mode_and_flush sound_deferred_queue_drain /* 004652c0 */
#define sfx_set_queue_mode sound_batch_queue_drain /* 004652d0 */
#define sfx_is_sound_on sound_subsystem_enabled /* 00465370 */
#define smoke_initialise scene_effect_registry_reset /* 00465380 */
#define smoke_load_textures transient_effect_textures_preload /* 004653a0 */
#define smoke_create_smoke_area transient_effect_emitter_create /* 00465430 */
#define smoke_remove_all transient_effect_registry_release_all /* 00465970 */
#define smoke_remove transient_effect_instance_release /* 004659b0 */
#define smoke_hide_all transient_effect_registry_enable_set /* 00465a90 */
#define smoke_hide transient_effect_enable_set /* 00465ac0 */
#define smoke_update transient_effect_instances_tick /* 00465b00 */
#define smoke_group_show transient_effect_particle_slot_initialize /* 00465c50 */
#define smoke_group_update transient_effect_segment_advance /* 00465e00 */
#define smoke_group_set_vertices transient_effect_instance_quad_vertices_build /* 00465f40 */
#endif

#endif /* LEGORRR_GAME_SFX_LINK_NAMES_H */
