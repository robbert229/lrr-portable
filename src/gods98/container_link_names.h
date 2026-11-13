// SPDX-License-Identifier: LGPL-3.0-or-later
/* G98-002 Container.c (finding 624): the port's link names of the unit's
 * functions, for the matching build only (as findings 593 and 616).
 *
 * In the matching build (MSVC 5.0, _MSC_VER 1100) every C caller of these
 * names passes the original's arguments: the C units (Lego.c, LegoObject.c,
 * Detail.c, Sound3D.c and the other game units) call Container.c through
 * them. So the unit's definitions take these names, and those callers reach
 * them with no caller edit. Most are the port's annotated names from
 * container_port.cpp; the others are original-ABI adapters the unit
 * replaces, in entity.cpp, surface.cpp, view.cpp, lego_port.cpp,
 * legoobject_port.cpp, vehicle_model_port.cpp and tile_variants_port.cpp.
 * Every file that defines one of these names compiles it out of the
 * matching build (LRR_CONTAINER_PORT_NAMES). Natively the port serves every
 * name and nothing calls the unit. ddiscene_animation_hook_create's port
 * contract has ten arguments; container_frame.h passes the original's
 * eleventh (0) in the matching build. The AnimClone.c names at the top are
 * that unit's own link names (animclone_link_names.h), which container.c
 * calls.
 *
 * Included only by container.c, before any other header. A macro renames a
 * declaration or a definition one for one, so MSVC's symbol numbering
 * (finding 538) does not change. */
#ifndef LEGORRR_GODS98_CONTAINER_LINK_NAMES_H
#define LEGORRR_GODS98_CONTAINER_LINK_NAMES_H

#if defined(_MSC_VER) && _MSC_VER < 1200
/* AnimClone.c's functions, by their matching-build names
 * (animclone_link_names.h). */
#define anim_clone_remove scene_object_owner_link_release /* 00489750 */
#define anim_clone_set_time ddiscene_timed_hook_dispatch /* 004897e0 */

#define Container_Initialise container_initialise_original /* 00472710 */
#define Container_Hide2 legoobject_gs_container_hidden_set_adapter /* 00473b20 */
#define Container_SetFogMode renderer_fog_mode_set_original /* 00473e70 */
#define Container_SetFogParams renderer_fog_parameters_set_original /* 00473ea0 */
#define Container_SetPerspectiveCorrection vehicle_container_enable_sound_triggers_adapter /* 00473ec0 */
#define Container_Mesh_GetGroup detail_container_mesh_get_group_adapter /* 00474c40 */
#define Container_Mesh_Scale detail_container_mesh_scale_adapter /* 00474e90 */
#define Container_InverseTransform detail_container_inverse_transform_adapter /* 00475090 */
#define Container_AddTransform legoobject_gt_container_matrix_set_adapter /* 004756b0 */
#define Container_Shutdown ddiscene_object_pool_shutdown /* 00472800 */
#define Container_SetSharedTextureDirectory shared_texture_directory_set /* 004728c0 */
#define Container_EnableSoftwareRenderer renderer_flag_bit6_set /* 004728e0 */
#define Container_SetTriggerFrameCallback scene_end_hook_register /* 00472900 */
#define Container_SetSoundTriggerCallback scene_sample_hook_register /* 00472920 */
#define Container_GetRoot scene_root_object_get /* 00472940 */
#define Container_Create ddiscene_object_allocate /* 00472950 */
#define Container_Remove ledger_stub_00472a40_DDiSceneObjectDestroy /* 00472a40 */
#define Container_Load container_load /* 00472cd0 */
#define Container_IsCurrentActivity ledger_stub_00473340_SceneActionNamePresent /* 00473340 */
#define Container_SetActivity scene_action_name_set /* 00473370 */
#define Container_Light_SetSpotPenumbra light_interface_call_slot_40 /* 00473460 */
#define Container_Light_SetSpotUmbra light_interface_call_slot_3c /* 00473480 */
#define Container_Light_SetSpotRange light_interface_call_slot_38 /* 004734a0 */
#define Container_Light_SetEnableContainer advisor_companion_transform_apply /* 004734c0 */
#define Container_MakeLight ddiscene_light_create /* 004734f0 */
#define Container_MakeMesh2 container_make_mesh2 /* 00473560 */
#define Container_GetMasterFrame scene_object_wrapped_pointer_get /* 00473680 */
#define Container_Clone entity_visual_resource_load /* 00473690 */
#define Container_Hide scene_render_lock_toggle /* 00473b40 */
#define Container_IsHidden scene_object_flag_bit3_get_target /* 00473ba0 */
#define Container_SearchTree resource_name_register_or_query_target /* 00473bc0 */
#define Container_FormatPartName xf_texture_name_format /* 00473c60 */
#define Container_SetUserData scene_object_owner_set /* 00473da0 */
#define Container_GetUserData scene_object_owner_get /* 00473db0 */
#define Container_EnableFog lego_fog_enable_adapter /* 00473dc0 */
#define Container_SetFogColour lego_fog_colour_set_adapter /* 00473e10 */
#define Container_LoadTexture2 detail_container_load_texture2_adapter /* 004744f0 */
#define Container_FreeTexture detail_container_free_texture_adapter /* 00474710 */
#define Container_Mesh_Swap interface_array_forward_sync /* 00474760 */
#define Container_Mesh_AddGroup mesh_record_allocate_or_append /* 004748f0 */
#define Container_Mesh_GetGroupCount animated_texture_frame_count /* 00474a20 */
#define Container_Mesh_SetQuality animated_texture_frame_bind /* 00474a60 */
#define Container_Mesh_IsGroupHidden terrain_detail_entry_get_at_index /* 00474ae0 */
#define Container_Mesh_HideGroup container_mesh_hide_group /* 00474b30 */
#define Container_Mesh_GetVertices lwo_layer_polygon_vertex_data_read /* 00474cc0 */
#define Container_Mesh_SetVertices container_mesh_set_vertices /* 00474d30 */
#define Container_Mesh_SetTexture animated_texture_frame_set /* 00474da0 */
#define Container_Mesh_SetPerspectiveCorrection container_mesh_perspective_correction_set /* 00474e30 */
#define Container_Mesh_GetBox scene_object_bounds_union /* 00474f10 */
#define Container_Mesh_SetEmissive scene_mesh_color_set /* 00474ff0 */
#define Container_Mesh_SetColourAlpha scene_mesh_color_and_alpha_set /* 00475020 */
#define Container_Transform scene_object_coordinate_transform_in_place /* 00475070 */
#define Container_SetColourAlpha render_object_set_packed_rgba_from_float_channels /* 004750b0 */
#define Container_MoveAnimation ddiscene_advance_by_time_target /* 00475120 */
#define Container_SetAnimationTime ddiscene_frame_update_and_callbacks /* 00475140 */
#define Container_ForceAnimationUpdate ddiscene_apply_act_frame_timing /* 00475300 */
#define Container_GetAnimationTime ddiscene_object_length_get /* 00475390 */
#define Container_GetAnimationFrames ddiscene_object_frame_position_get /* 004753f0 */
#define Container_SetPosition ddiscene_object_position_set /* 00475430 */
#define Container_SetOrientation ddiscene_object_pose_method_dispatch /* 00475470 */
#define Container_GetPosition ddiscene_object_position_get /* 004754c0 */
#define Container_GetOrientation ddiscene_object_facing_pair_get /* 00475500 */
#define Container_AddRotation render_object_rotate_axis /* 00475580 */
#define Container_AddScale scene_frame_scale_set /* 004755b0 */
#define Container_AddTranslation scene_object_camera_teleport_or_offset /* 004755e0 */
#define Container_ClearTransform render_object_set_identity_transform /* 00475610 */
#define Container_GetZXRatio ledger_stub_004756d0_VideoObjectAspectRatioGet /* 004756d0 */
#define Container_SetParent ddiscene_scene_object_attach_or_unlink /* 00475710 */
#define Container_GetTransCoef typed_child_resource_scalar_get /* 004757f0 */
#define Container_Frame_SetAppData ddiscene_animation_hook_create /* 00475f70 */
#endif

#endif /* LEGORRR_GODS98_CONTAINER_LINK_NAMES_H */
