// SPDX-License-Identifier: LGPL-3.0-or-later
#ifndef LEGORRR_LWSC_SCENE_HPP
#define LEGORRR_LWSC_SCENE_HPP

#include <stddef.h>
#include <stdint.h>
#include "entity.hpp"
#include "model.hpp"

/* Host-owned LWSC1 data. None of these records has a retail memory layout.
 * Frames are real retained backend interfaces, never Model/EntitySlot casts.
 * Limits are rejection limits, not truncation limits. */
#define LWSC_MAX_NODES 4096u
#define LWSC_MAX_KEYS 131072u
#define LWSC_MAX_ACTIVITIES 128u
#define LWSC_MAX_TEXT (16u * 1024u * 1024u)
#define LWSC_PATH_CAPACITY 520u
#define LWSC_NAME_CAPACITY 1040u

typedef struct LwscError {
    unsigned int line;
    char message[160];
} LwscError;

typedef struct LwscKey {
    float channels[9];             /* authored XYZ, HPB degrees, XYZ scale */
    float radians[3];              /* 0x4869f0 conversion, before interpolation */
    uint16_t frame;
    int linear;                    /* retained; retail uses linear interpolation */
    float tension, continuity, bias;
} LwscKey;

typedef struct LwscDissolveKey {
    float dissolve;                /* authored value, not opacity */
    uint16_t frame;
    int linear;
    float tension, continuity, bias;
} LwscDissolveKey;

typedef struct LwscNode {
    char* authored_reference;       /* full LoadObject path / AddNullObject name */
    char* base_name;                /* mesh basename without .lwo, or full null */
    char native_name[LWSC_NAME_CAPACITY]; /* printable base_%0.2i (0x4887c0) */
    unsigned int is_null;
    unsigned int duplicate;        /* exact same-kind/name, case-sensitive */
    unsigned int parent_object;    /* ORIGINAL one-based object index, 0=root */
    unsigned int interface_index;  /* depth-first, distinct from source index */
    unsigned int first_child;      /* one-based source index, 0=none */
    unsigned int next_sibling;
    unsigned int flags;            /* 1=null, 4=UnseenByCamera */
    float pivot[3];                /* NEGATIVE authored PivotPoint, as retail */
    LwscKey* keys;
    unsigned int authored_key_count;
    unsigned int key_count;        /* includes retail terminal hold, if any */
    unsigned int motion_end_behavior; /* retained, not the ACT repeat flag */
    LwscDissolveKey* dissolve_keys;
    unsigned int authored_dissolve_count;
    unsigned int dissolve_count;
    unsigned int dissolve_end_behavior;
    int has_constant_dissolve;
    float constant_dissolve;
    Model* model;                  /* owned via lwo_model_entry_load/model_free */
    char model_path[LWSC_PATH_CAPACITY]; /* actual primary/fallback request */
} LwscNode;

typedef struct LwscSceneTemplate LwscSceneTemplate;
typedef struct LwscAssemblyTemplate LwscAssemblyTemplate;
typedef struct LwscAssemblyInstance LwscAssemblyInstance;

#define LWSC_CAPABILITY_POSE_QUERY 1u
#define LWSC_CAPABILITY_RENDERING 2u

/* Parse makes an owned immutable hierarchy, but does not open mesh assets.
 * open additionally loads EVERY LoadObject through the existing LWO owner.
 * path is the effective resource request, used for relative mesh resolution.
 * No raw buffer or BlockFile pointer is retained. */
LwscSceneTemplate* lwsc_scene_parse(const char* text, size_t length,
                                   const char* path, LwscError* error);
LwscSceneTemplate* lwsc_scene_open(const char* path, LwscError* error);
void lwsc_scene_retain(LwscSceneTemplate* scene);
void lwsc_scene_release(LwscSceneTemplate* scene);
unsigned int lwsc_scene_node_count(const LwscSceneTemplate* scene);
const LwscNode* lwsc_scene_node(const LwscSceneTemplate* scene,
                               unsigned int source_index);
unsigned int lwsc_scene_first_frame(const LwscSceneTemplate* scene);
unsigned int lwsc_scene_last_frame(const LwscSceneTemplate* scene);
float lwsc_scene_fps(const LwscSceneTemplate* scene);
const char* lwsc_scene_path(const LwscSceneTemplate* scene);

/* Uses EntityAssembly metadata from the SAME opened .ae as the requested
 * configured-name keys (e.g. ToolNullName). Missing names remain NULL.
 * Only genuine FILE + LWSFILE TRUE activities are supported here; other
 * formats fail explicitly, never substitute a similarly named scene.
 * The template owns metadata, strings, scene tracks and parsed mesh owners.
 * Reload = open a new template; existing instances retain the old one. */
LwscAssemblyTemplate* lwsc_assembly_open(const char* entity_path,
    const char* config_root, uint32_t feature,
    const char* const* name_keys, unsigned int name_key_count,
    LwscError* error);
void lwsc_assembly_retain(LwscAssemblyTemplate* assembly);
void lwsc_assembly_release(LwscAssemblyTemplate* assembly);
const EntityAssembly* lwsc_assembly_metadata(const LwscAssemblyTemplate* assembly);
const char* lwsc_assembly_configured_name(const LwscAssemblyTemplate* assembly,
                                         const char* key);
const LwscSceneTemplate* lwsc_assembly_scene(const LwscAssemblyTemplate* assembly,
                                            unsigned int activity);

/* Each non-NULL interface_pointer MUST be an actual retained RM/DDi frame
 * interface. This wrapper is NOT a DDiSceneObject. Consumers needing a DDi
 * object must ask their real scene owner to wrap/query the interface. */
typedef struct LwscFrameRef {
    void* interface_pointer;
} LwscFrameRef;
typedef struct LwscMeshRef {
    void* owner;                   /* real backend attachment, never a node id */
} LwscMeshRef;

/* All operations return 1 on success, 0 on failure (not HRESULT).
 * create returns one owned frame reference; destroy detaches from its
 * parent AND releases that reference. Parent references belong to backend.
 * model_attach must attach the supplied FULL Model through the actual mesh
 * owner and return an owned attachment; detach releases it. Empty named
 * frames are not a successful mesh attachment. The Model remains alive
 * until AFTER all attachments are detached.
 * activity_enable implements the real visible/hidden activity channel
 * operation while keeping the frame/query ownership valid.
 * world_position queries relative to the application's actual scene root,
 * including every ancestor. It must not return a cached guessed point.
 * Rendering operations are required by lwsc_instance_create. The explicit
 * pose-query factory does not call them or claim rendered attachments;
 * every authored Model/node and actual frame remains owned and associated.
 * Context/parent/ops must outlive the instance. No global installer. */
typedef struct LwscBackendOps {
    void* context;
    int (*frame_create)(const LwscFrameRef* parent, LwscFrameRef* result,
                         void* context);
    void (*frame_destroy)(LwscFrameRef* frame, void* context);
    int (*frame_name_set)(const LwscFrameRef* frame, const char* name,
                           void* context);
    int (*matrix_set)(const LwscFrameRef* frame, const float matrix[16],
                       void* context); /* REPLACE, relative to real parent */
    int (*matrix_get)(const LwscFrameRef* frame, float matrix[16],
                       void* context); /* same reference, temporary COM refs released */
    int (*world_position)(const LwscFrameRef* frame, float xyz[3], void* context);
    int (*activity_enable)(const LwscFrameRef* frame, int enabled, void* context);
    int (*model_attach)(const LwscFrameRef* frame, const Model* model,
                         const char* path, unsigned int flags,
                         LwscMeshRef* result, void* context);
    void (*model_detach)(LwscMeshRef* mesh, void* context);
    int (*model_opacity_set)(const LwscMeshRef* mesh, float opacity, void* context);
} LwscBackendOps;

/* A per-building instance has its own real frames, clock and pose. Rendering
 * instances additionally own mesh attachments; pose-query instances retain
 * the complete template's real Models and their source/frame associations.
 * Every activity is instantiated once and retained until destruction, so
 * borrowed named-node refs survive activity changes (retail +0x50 cache is
 * NOT blanket-cleared by 0x408570). No selector mapping or GetTool movement
 * occurs in this module. Caller maps a numeric selector ONCE, separately.
 * Initial activity is metadata row 0, matching first successful ACT load.
 * Building placement/heading/height belong to the caller's ordinary owner;
 * root_matrix is a full local matrix relative to the supplied parent.
 * Assembly Scale is applied once below that pose, as 0x472cd0 does. */
LwscAssemblyInstance* lwsc_instance_create(LwscAssemblyTemplate* assembly,
    const LwscBackendOps* ops, const LwscFrameRef* parent,
    const float root_matrix[16], LwscError* error);
/* Genuine transform-query capability only: retain ALL authored LoadObject
 * and null nodes, real loaded Models and real frames, with the same root,
 * activity/time, ancestry and borrowed-node lifetimes. Model ownership is
 * associated by activity/source index; no mesh attachment is reported.
 * activity_enable/model_attach/model_detach/model_opacity_set are unused.
 * This creates no rendered geometry or visibility/opacity capability and
 * does not supply DDi wrappers. The caller must bind the same real frames
 * to genuine DDi owners before returning a node to a DDi consumer. */
LwscAssemblyInstance* lwsc_instance_create_for_pose(LwscAssemblyTemplate* assembly,
    const LwscBackendOps* ops, const LwscFrameRef* parent,
    const float root_matrix[16], LwscError* error);
/* clone allocates fresh frames/attachments from retained template metadata,
 * preserves its capabilities, selects source's activity and starts at time0
 * (0x487a00). No shallow COM copy. Pose-query clones claim no attachments. */
LwscAssemblyInstance* lwsc_instance_clone(const LwscAssemblyInstance* source,
    const LwscFrameRef* parent, const float root_matrix[16], LwscError* error);
void lwsc_instance_destroy(LwscAssemblyInstance* instance);
int lwsc_instance_activity_set(LwscAssemblyInstance* instance,
                               const char* activity_state, LwscError* error);
/* Absolute authored frame units, not seconds. FPS/TRANSCOEF are retained
 * for the external ACT time owner. Repeating uses remainder(time,LastFrame),
 * not LastFrame+1; nonrepeat only clamps above LastFrame (0x487ba0).
 * Backend mutation failure poisons queries; destroy/recreate that instance. */
int lwsc_instance_time_set(LwscAssemblyInstance* instance, float frame_time,
                           LwscError* error);
int lwsc_instance_root_set(LwscAssemblyInstance* instance,
                           const float root_matrix[16], LwscError* error);
unsigned int lwsc_instance_activity(const LwscAssemblyInstance* instance);
/* Zero for invalid instances; query-only instances never report RENDERING. */
unsigned int lwsc_instance_capabilities(const LwscAssemblyInstance* instance);
float lwsc_instance_time(const LwscAssemblyInstance* instance);
const LwscFrameRef* lwsc_instance_root(const LwscAssemblyInstance* instance);
/* source_index is zero-based document order; ordinal is already mapped.
 * Named lookup follows native depth-first, ASCII case-insensitive matching.
 * A borrowed ref is valid through activity changes until instance destroy;
 * do not release it. No successful fallback to the root is supplied. */
const LwscFrameRef* lwsc_instance_node(const LwscAssemblyInstance* instance,
                                      unsigned int activity,
                                      unsigned int source_index);
/* Borrowed real Model associated with that exact authored frame; NULL for
 * authored null nodes or invalid arguments. Retained until instance destroy. */
const Model* lwsc_instance_node_model(const LwscAssemblyInstance* instance,
    unsigned int activity, unsigned int source_index);
const LwscFrameRef* lwsc_instance_named_node(const LwscAssemblyInstance* instance,
    const char* configured_base, unsigned int ordinal);
int lwsc_instance_node_world_position(const LwscAssemblyInstance* instance,
    const LwscFrameRef* frame, float xyz[3]);

#endif /* LEGORRR_LWSC_SCENE_HPP */
