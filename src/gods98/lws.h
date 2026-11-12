// SPDX-License-Identifier: LGPL-3.0-or-later
/* lws.c (original translation unit G98-020): the LightWave scene loader.
 * It parses an "LWSC" .lws file into a node tree with position, rotation
 * and scale keys, dissolve keys and sound triggers, builds a Direct3DRM
 * frame per node with its mesh, clones scenes, and plays them back.
 *
 * C-compatible header (ADR 0005). lws.c is compiled as C in the matching
 * build and as C++ in the native port. Shared C++ headers do not include
 * this file: every declaration a header adds shifts MSVC's symbol numbering
 * (finding 538). src/game/test_c_boundaries.cpp checks it against the C++
 * declarations of the same functions.
 *
 * The six functions that port code outside the unit calls keep the port's
 * names and contracts here: resource_entry_id_get (record.hpp),
 * animation_track_find_keyframe_and_fraction (view.hpp),
 * shared_object_sample_time_matches and keyframe_time_segment_fraction
 * (scene_resources.hpp), pair_table_find and pair_table_append (crt.hpp).
 * Their `const void*` arguments are the original's scene, node and mesh
 * path list, which lws.c's definitions take. The other functions take the original's arguments; the native
 * port never calls them (it loads scenes through lwsc_scene.cpp), and the
 * port's own models of them stay, unannotated, in lws_port.cpp. The names
 * are descriptive; the comments give the gods98 names. */
#ifndef LEGORRR_GODS98_LWS_H
#define LEGORRR_GODS98_LWS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct LwsInfo LwsInfo;
typedef struct LwsNode LwsNode;
struct LwsFrame;

/* Lws_Initialise's callbacks: find a sample by name, play a sample on a
 * frame, and whether sound is on. */
typedef int (*LwsFindSFXIDFunc)(const char* name, unsigned long* sfxID);
typedef unsigned long (*LwsPlaySample3DFunc)(void* frame, unsigned long type,
                                             int loop, int onCont,
                                             void* wPos);
typedef int (*LwsSoundEnabledFunc)(void);

/* The six functions with port contracts. lws.c defines them with the
 * original's argument types (a scene, a node, a mesh path list) and its
 * own prototypes; the calls link by name. */
#if !defined(LEGORRR_LWS_UNIT)

/* Lws_GetFrameCount (FUN_004877d0): the scene's last frame. */
uint32_t resource_entry_id_get(const void* entry);

/* Lws_FindPrevKey (FUN_00487cb0): the key before `time` and the fraction
 * of the way to the next. */
float animation_track_find_keyframe_and_fraction(const void* track,
                                                 float time,
                                                 uint16_t* keyframe_out);

/* Lws_KeyPassed (FUN_00487ed0): 1 when the last time step passed `time`. */
int shared_object_sample_time_matches(const void* record, uint32_t time);

/* Lws_FindPrevDissolve (FUN_00487fc0). */
float keyframe_time_segment_fraction(const void* track, float time,
                                     uint16_t* segment_out);

/* Lws_SearchMeshPathList (FUN_00488730). */
void* pair_table_find(const void* table, uint32_t count, const char* key);

/* Lws_AddMeshPathEntry (FUN_00488790). */
void pair_table_append(void* table, int* count, const char* key,
                       void* value);

#endif /* !LEGORRR_LWS_UNIT */

/* Lws_Parse (FUN_004869f0): load "<fname>.lws"; NULL on failure. */
LwsInfo* lws_parse(const char* fname, int looping);

/* Lws_Initialise (FUN_004876c0): the shared mesh directory and the sound
 * callbacks. */
void lws_initialise(const char* sharedDir, LwsFindSFXIDFunc FindSFXIDFunc,
                    LwsPlaySample3DFunc PlaySample3DFunc,
                    LwsSoundEnabledFunc SoundEnabledFunc);

/* Lws_Shutdown (FUN_00487760): free the mesh path lists' names. */
void lws_shutdown(void);

/* Lws_SetupSoundTriggers (FUN_004877e0). */
void lws_sound_triggers_setup(LwsInfo* scene);

/* Lws_LoadMeshes (FUN_00487990): create the frames under `parent` and load
 * their meshes, then show frame 0. */
void lws_meshes_load(LwsInfo* scene, struct LwsFrame* parent);

/* Lws_Clone (FUN_00487a00): a scene sharing `scene`'s nodes, with its own
 * frames under `parent`. */
LwsInfo* lws_clone(LwsInfo* scene, struct LwsFrame* parent);

/* Lws_SetTime (FUN_00487ba0). */
void lws_time_set(LwsInfo* scene, float time);

/* Lws_AnimateTextures (FUN_00487d50). */
void lws_textures_animate(LwsInfo* scene, LwsNode* node);

/* Lws_HandleTrigger (FUN_00487de0). */
void lws_trigger_handle(LwsInfo* scene, LwsNode* node);

/* Lws_InterpolateDissolve (FUN_00488070). */
void lws_dissolve_interpolate(LwsInfo* scene, LwsNode* node,
                              unsigned short prev, float delta);

/* Lws_SetDissolveLevel (FUN_004880d0). */
void lws_dissolve_level_set(LwsInfo* scene, LwsNode* node, float level);

/* Lws_Interpolate (FUN_00488170). */
void lws_interpolate(LwsInfo* scene, LwsNode* node, unsigned short key,
                     float delta);

/* Lws_SetAbsoluteKey (FUN_004882e0). */
struct LwsVector3;
void lws_absolute_key_set(LwsInfo* scene, LwsNode* node,
                          struct LwsVector3* pos, struct LwsVector3* hpb,
                          struct LwsVector3* scale);

/* Lws_LoadMesh (FUN_004885c0). */
void* lws_mesh_load(const char* baseDir, const char* fname,
                    struct LwsFrame* frame, int noTextures);

/* Lws_CreateFrames (FUN_004887c0). */
void lws_frames_create(LwsInfo* scene, LwsNode* node,
                       struct LwsFrame* parent, unsigned short* frameCount);

/* Lws_LoadNodes (FUN_00488900). */
void lws_nodes_load(LwsInfo* scene, LwsNode* node);

/* Lws_SetupNodeTransform (FUN_004889a0). */
void lws_node_transform_setup(LwsInfo* scene, LwsNode* node,
                              unsigned short key);

/* Lws_Free (FUN_004889d0). */
void lws_free(LwsInfo* scene);

/* Lws_FreeNode (FUN_00488a70). */
void lws_node_free(LwsInfo* scene, LwsNode* node);

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GODS98_LWS_H */
