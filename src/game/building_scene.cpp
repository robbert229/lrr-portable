// SPDX-License-Identifier: LGPL-3.0-or-later
#include "building_scene.hpp"
#if defined(LEGORRR_BACKEND_WINAPI) && defined(_WIN32) && !defined(_WIN64)
#include "entity.hpp"
#include "game.hpp"
#include "level.hpp"
#include "cmd.hpp"
#include "gods98/lwsc_winapi.hpp"
#include "gods98/view.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <process.h>

struct BuildingSceneType {
    uint32_t row;
    uint32_t feature;
    char* path;
    LwscAssemblyTemplate* assembly;
    BuildingSceneType* next;
};
struct BuildingSceneInstance {
    EntitySlot* entity;
    BuildingSceneType* type;
    LwscAssemblyInstance* instance;
    void* tool_cache[5]; /* native numeric-tool map selects ordinals0..4 */
    float root_matrix[16];
    float clock;
    int failed;
    BuildingSceneInstance* next;
};
static BuildingSceneType* s_building_scene_types;
static BuildingSceneInstance* s_building_scene_instances;
static LwscWinapiContext* s_building_scene_backend;
static LwscBackendOps s_building_scene_ops;
static const LwscFrameRef* s_building_scene_parent;
static int s_building_scene_failed;

static BuildingSceneInstance* building_scene_find(const EntitySlot* e)
{
    BuildingSceneInstance* p;
    for (p = s_building_scene_instances; p != NULL; p = p->next)
        if (p->entity == e) return p;
    return NULL;
}
static void building_scene_fail(BuildingSceneInstance* p, const char* operation,
                                 const LwscError* error)
{
    if (p != NULL && !p->failed) {
        cmd_gettool_entity_removing(p->entity);
        memset(p->tool_cache, 0, sizeof(p->tool_cache));
        entity_ddiscene_scene_owner_bind(p->entity, NULL);
        p->failed = 1;
    }
    s_building_scene_failed = 1;
    fprintf(stderr, "building-scene: %s failed: %s\n", operation,
        error != NULL ? error->message : "native frame ownership failure");
    fflush(stderr);
}
static int building_scene_valid(BuildingSceneInstance* p)
{
    const char* error;
    if (p == NULL || p->failed || p->instance == NULL) return 0;
    error = lwsc_winapi_context_error(s_building_scene_backend);
    if (!(lwsc_instance_capabilities(p->instance) & LWSC_CAPABILITY_POSE_QUERY) ||
        (error != NULL && error[0] != '\0')) {
        building_scene_fail(p, "instance validity", NULL);
        return 0;
    }
    return 1;
}
static void* building_scene_tool_resolve(void*, EntitySlot* e, uint32_t ordinal)
{
    BuildingSceneInstance* p = building_scene_find(e);
    const LwscFrameRef* node;
    const char* name;
    void* scene;
    if (!building_scene_valid(p) || ordinal >= 5u) return NULL;
    if (p->tool_cache[ordinal] != NULL) return p->tool_cache[ordinal];
    name = lwsc_assembly_configured_name(p->type->assembly, "ToolNullName");
    if (name == NULL) return NULL;
    node = lwsc_instance_named_node(p->instance, name, ordinal);
    if (node == NULL) return NULL;
    scene = lwsc_winapi_scene_object(s_building_scene_backend, node);
    if (scene == NULL) { building_scene_fail(p, "same-frame node wrapper", NULL); return NULL; }
    p->tool_cache[ordinal] = scene;
    fprintf(stderr, "building-scene: tool entity=%p ordinal=%lu name=%s frame=%p ddi=%p\n",
        e, (unsigned long)ordinal, name, node->interface_pointer, scene);
    return scene;
}
int building_scene_initialize(void)
{
    LwscError error;
    if (s_building_scene_backend != NULL) return !s_building_scene_failed;
    s_building_scene_backend = lwsc_winapi_context_create(scene_root_object_get(), &error);
    if (s_building_scene_backend == NULL) {
        building_scene_fail(NULL, "context create", &error); return 0;
    }
    if (!lwsc_winapi_backend_get(s_building_scene_backend,
                                 &s_building_scene_ops, &s_building_scene_parent)) {
        building_scene_fail(NULL, "backend operations", NULL); return 0;
    }
    s_building_scene_failed = 0;
    unported_ledger_stubs::entity_object_position_scene_resolve_hook_set(
        building_scene_tool_resolve, NULL);
    return 1;
}
int building_scene_config_path_set(uint32_t row, const char* path, uint32_t feature)
{
    BuildingSceneType* t;
    size_t n;
    if (s_building_scene_backend == NULL || path == NULL || path[0] == '\0') return 0;
    n = strlen(path);
    if (n >= LWSC_PATH_CAPACITY) return 0;
    for (t = s_building_scene_types; t != NULL; t = t->next)
        if (t->row == row)
            return t->feature == feature && strcmp(t->path, path) == 0;
    t = (BuildingSceneType*)calloc(1, sizeof(*t));
    if (t == NULL) return 0;
    t->path = (char*)malloc(n + 1u);
    if (t->path == NULL) { free(t); return 0; }
    memcpy(t->path, path, n + 1u); t->row = row; t->feature = feature;
    t->next = s_building_scene_types; s_building_scene_types = t;
    return 1;
}
/* 0x4428a0 ->0x4086c0/0x475470: forward is [sin(h),cos(h),0],
 * up is [0,0,-1]. The Frame3 row-vector basis is right/up/forward.
 * 0x408710 obtains Z from the live0x44f940 terrain mesh. AE Scale is applied
 * ONCE below this root by lwsc_instance_create/root_set, not here. */
int building_scene_pose_set(EntitySlot* e)
{
    BuildingSceneInstance* p = building_scene_find(e);
    LwscError error;
    float m[16], z, sn, cs;
    if (p == NULL) return 0;
    if (!building_scene_valid(p)) return 0;
    if (e->owner_ctx == NULL || e->owner_ctx->level == NULL ||
        e->owner_ctx->level->ptr_terrain == NULL || !_finite(e->x) ||
        !_finite(e->y) || !_finite(e->rot)) {
        building_scene_fail(p, "invalid ordinary root pose", NULL); return 0;
    }
    z = terrain_surface_height_interpolate(e->owner_ctx->level->ptr_terrain, e->x, e->y);
    if (!_finite(z)) { building_scene_fail(p, "nonfinite terrain height", NULL); return 0; }
    sn = (float)sin((double)e->rot); cs = (float)cos((double)e->rot);
    memset(m, 0, sizeof(m)); m[0] = cs; m[1] = -sn; m[6] = -1.0f;
    m[8] = sn; m[9] = cs; m[12] = e->x; m[13] = e->y; m[14] = z; m[15] = 1.0f;
    if (!lwsc_instance_root_set(p->instance, m, &error)) {
        building_scene_fail(p, "ordinary root pose", &error); return 0;
    }
    memcpy(p->root_matrix, m, sizeof(m)); e->z = z; return 1;
}
int building_scene_entity_bind(EntitySlot* e)
{
    BuildingSceneType* t;
    BuildingSceneInstance* p;
    LwscError error;
    float identity[16];
    static const char* keys[] = { "ToolNullName", "DepositNullName", "CarryNullName",
        "CameraNullName", "EntranceNullName", "FireNullName" };
    void* root_scene;
    if (e == NULL || e->type != 4 || e->etype < 0 ||
        s_building_scene_backend == NULL || s_building_scene_failed) return 0;
    if (building_scene_find(e) != NULL) return 1;
    for (t = s_building_scene_types; t != NULL; t = t->next)
        if (t->row == (uint32_t)e->etype) break;
    if (t == NULL) return 0;
    if (t->assembly == NULL) {
        t->assembly = lwsc_assembly_open(t->path, "Lego*", t->feature, keys, 6u, &error);
        if (t->assembly == NULL) { building_scene_fail(NULL, "authored template", &error); return 0; }
        fprintf(stderr, "building-scene: template row=%lu path=%s\n", (unsigned long)t->row, t->path);
    }
    p = (BuildingSceneInstance*)calloc(1, sizeof(*p));
    if (p == NULL) return 0;
    memset(identity, 0, sizeof(identity));
    identity[0] = identity[5] = identity[10] = identity[15] = 1.0f;
    p->instance = lwsc_instance_create_for_pose(t->assembly, &s_building_scene_ops,
                                                s_building_scene_parent, identity, &error);
    if (p->instance == NULL) { free(p); building_scene_fail(NULL, "instance create", &error); return 0; }
    p->type = t; p->entity = e; p->next = s_building_scene_instances;
    s_building_scene_instances = p;
    if (!building_scene_pose_set(e)) { building_scene_fail(p, "initial root pose", NULL); return 0; }
    root_scene = lwsc_winapi_scene_object(s_building_scene_backend, lwsc_instance_root(p->instance));
    if (root_scene == NULL) { building_scene_fail(p, "root wrapper", NULL); return 0; }
    entity_ddiscene_scene_owner_bind(e, root_scene);
    fprintf(stderr, "building-scene: instance entity=%p instance=%p root=%p xyz=%g,%g,%g heading=%g\n",
        e, p->instance, root_scene, e->x, e->y, e->z, e->rot);
    return 1;
}
int building_scene_activity_set(EntitySlot* e, const char* state)
{
    BuildingSceneInstance* p = building_scene_find(e);
    LwscError error;
    const EntityAssembly* a;
    unsigned int i;
    if (!building_scene_valid(p) || state == NULL) return 0;
    a = lwsc_assembly_metadata(p->type->assembly);
    for (i = 0; i < a->count; ++i) if (_stricmp(a->acts[i].state, state) == 0) break;
    if (i == a->count) return 0; /* unavailable activity does not poison a live Stand */
    if (i == lwsc_instance_activity(p->instance)) return 1;
    if (!lwsc_instance_activity_set(p->instance, state, &error)) {
        building_scene_fail(p, "ACT activity", &error); return 0;
    }
    p->clock = 0.0f; /* retained ToolNull cache remains backed by its old frames */
    return 1;
}
void building_scene_tick(GameContext* game, float seconds)
{
    BuildingSceneInstance* p;
    if (!(seconds > 0.0f) || !_finite(seconds)) return;
    for (p = s_building_scene_instances; p != NULL; p = p->next) {
        LwscError error;
        float next;
        if (p->entity->owner_ctx != game || !building_scene_valid(p) ||
            (p->entity->flags_3e0 & 0x4000u)) continue;
        /* 0x475120 adds authored frame delta to current time. TRANSCOEF is
         * retained movement metadata, NOT a clock multiplier (Stand has0).
         * The app caller supplies pause-gated/scaled seconds; reference25Hz. */
        next = lwsc_instance_time(p->instance) + seconds * 25.0f;
        if (!lwsc_instance_time_set(p->instance, next, &error)) {
            building_scene_fail(p, "ACT frame time", &error); continue;
        }
        p->clock = lwsc_instance_time(p->instance);
    }
}
void building_scene_entity_release(EntitySlot* e)
{
    BuildingSceneInstance** link = &s_building_scene_instances;
    BuildingSceneInstance* p;
    while (*link != NULL && (*link)->entity != e) link = &(*link)->next;
    p = *link; if (p == NULL) return;
    cmd_gettool_entity_removing(e);
    memset(p->tool_cache, 0, sizeof(p->tool_cache));
    entity_ddiscene_scene_owner_bind(e, NULL);
    lwsc_instance_destroy(p->instance); p->instance = NULL;
    *link = p->next; free(p);
}
int building_scene_types_clear(void)
{
    BuildingSceneType* t;
    while (s_building_scene_instances != NULL)
        building_scene_entity_release(s_building_scene_instances->entity);
    while (s_building_scene_types != NULL) {
        t = s_building_scene_types; s_building_scene_types = t->next;
        lwsc_assembly_release(t->assembly); free(t->path); free(t);
    }
    return s_building_scene_backend == NULL || lwsc_winapi_frame_count(s_building_scene_backend) == 0u;
}
int building_scene_shutdown(void)
{
    unported_ledger_stubs::entity_object_position_scene_resolve_hook_set(NULL, NULL);
    if (!building_scene_types_clear()) return 0;
    if (!lwsc_winapi_context_destroy(s_building_scene_backend)) return 0;
    s_building_scene_backend = NULL; s_building_scene_parent = NULL;
    memset(&s_building_scene_ops, 0, sizeof(s_building_scene_ops));
    s_building_scene_failed = 0;
    return lwsc_winapi_can_shutdown();
}
void building_scene_require_stopped(void)
{
    if (building_scene_shutdown()) return;
    fprintf(stderr, "building-scene: native context remains busy; refusing global/CRT teardown\n");
    fflush(stderr);
    _exit(2); /* do not call global scene release or later CRT destructors */
}
#endif
