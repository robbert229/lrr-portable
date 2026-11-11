// SPDX-License-Identifier: LGPL-3.0-or-later
#include "actor_scene.hpp"
#if defined(LEGORRR_BACKEND_WINAPI) && defined(_WIN32) && !defined(_WIN64)
#include "entity.hpp"
#include "ai.hpp"
#include "cmd.hpp"
#include "game.hpp"
#include "level.hpp"
#include "gods98/lwsc_winapi.hpp"
#include "gods98/view.hpp"
#include "gods98/string.hpp"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <process.h>

struct ActorSceneType {
    int type;
    uint32_t row, feature;
    char* path;
    LwscAssemblyTemplate* assembly;
    ActorSceneType* next;
};
struct ActorSceneInstance {
    EntitySlot* entity;
    ActorSceneType* type;
    LwscAssemblyInstance* instance;
    void* root_scene;
    int failed;
    unsigned int diagnostic_calls, diagnostic_lines;
    int diagnostic_activity, diagnostic_positive, diagnostic_head;
    ActorSceneInstance* next;
};
static ActorSceneType* s_actor_types;
static ActorSceneInstance* s_actor_instances;
static LwscWinapiContext* s_actor_backend;
static LwscBackendOps s_actor_ops;
static const LwscFrameRef* s_actor_parent;
static int s_actor_failed;

static int actor_finite(float value)
{ return value == value && value >= -FLT_MAX && value <= FLT_MAX; }
static ActorSceneInstance* actor_find(const EntitySlot* entity)
{
    ActorSceneInstance* p;
    for (p = s_actor_instances; p != NULL; p = p->next)
        if (p->entity == entity) return p;
    return NULL;
}
static void actor_fail(ActorSceneInstance* p, const char* operation,
                        const LwscError* error)
{
    if (p != NULL) p->failed = 1;
    s_actor_failed = 1;
    fprintf(stderr, "actor-scene: %s failed: %s\n", operation,
        error != NULL ? error->message : "authored resource/frame owner unavailable");
    fflush(stderr);
}
static int actor_valid(ActorSceneInstance* p)
{
    const char* error;
    if (p == NULL || p->failed || p->instance == NULL) return 0;
    error = lwsc_winapi_context_error(s_actor_backend);
    if ((lwsc_instance_capabilities(p->instance) & LWSC_CAPABILITY_POSE_QUERY) == 0u ||
        (error != NULL && error[0] != '\0')) {
        actor_fail(p, "instance validity", NULL);
        return 0;
    }
    return 1;
}
static void actor_sample(void* context, const char* name)
{
    ActorSceneInstance* p = (ActorSceneInstance*)context;
    ddiscene_activity_sample_dispatch(name, p->root_scene);
}
static void actor_trigger(void* context)
{
    ActorSceneInstance* p = (ActorSceneInstance*)context;
    ddiscene_activity_trigger_dispatch(p->root_scene);
}
static void actor_callbacks(ActorSceneInstance* p, LwscClockCallbacks* callbacks)
{
    callbacks->context = p;
    callbacks->sample = actor_sample;
    callbacks->trigger = actor_trigger;
}

int actor_scene_initialize(void)
{
    LwscError error;
    if (s_actor_backend != NULL) return !s_actor_failed;
    s_actor_backend = lwsc_winapi_context_create(scene_root_object_get(), &error);
    if (s_actor_backend == NULL ||
        !lwsc_winapi_backend_get(s_actor_backend, &s_actor_ops, &s_actor_parent)) {
        actor_fail(NULL, "context create", &error);
        return 0;
    }
    s_actor_failed = 0;
    return 1;
}
int actor_scene_config_path_set(int type, uint32_t row, const char* path,
                                 uint32_t feature)
{
    ActorSceneType* p;
    size_t length;
    if ((type != 2 && type != 3) || s_actor_backend == NULL ||
        path == NULL || path[0] == '\0') return 0;
    length = strlen(path);
    if (length >= LWSC_PATH_CAPACITY) return 0;
    for (p = s_actor_types; p != NULL; p = p->next)
        if (p->type == type && p->row == row)
            return p->feature == feature && strcmp(p->path, path) == 0;
    p = (ActorSceneType*)calloc(1u, sizeof(*p));
    if (p == NULL) return 0;
    p->path = (char*)malloc(length + 1u);
    if (p->path == NULL) { free(p); return 0; }
    memcpy(p->path, path, length + 1u);
    p->type = type; p->row = row; p->feature = feature;
    p->next = s_actor_types; s_actor_types = p;
    return 1;
}
int actor_scene_pose_set(EntitySlot* entity)
{
    ActorSceneInstance* p = actor_find(entity);
    LwscError error;
    float matrix[16], height, sn, cs;
    if (p == NULL) return 0;
    if (!actor_valid(p) || entity->owner_ctx == NULL ||
        entity->owner_ctx->level == NULL ||
        entity->owner_ctx->level->ptr_terrain == NULL ||
        !actor_finite(entity->x) || !actor_finite(entity->y) ||
        !actor_finite(entity->rot)) return 0;
    height = terrain_surface_height_interpolate(
        entity->owner_ctx->level->ptr_terrain, entity->x, entity->y);
    if (!actor_finite(height)) { actor_fail(p, "terrain height", NULL); return 0; }
    sn = (float)sin((double)entity->rot);
    cs = (float)cos((double)entity->rot);
    memset(matrix, 0, sizeof(matrix));
    matrix[0] = cs; matrix[1] = -sn; matrix[6] = -1.0f;
    matrix[8] = sn; matrix[9] = cs;
    matrix[12] = entity->x; matrix[13] = entity->y;
    matrix[14] = height; matrix[15] = 1.0f;
    if (!lwsc_instance_root_set(p->instance, matrix, &error)) {
        actor_fail(p, "root pose", &error); return 0;
    }
    entity->z = height;
    return 1;
}
void* actor_scene_motion_component(EntitySlot* entity)
{
    ActorSceneInstance* p = actor_find(entity);
    if (!actor_valid(p) || p->root_scene == NULL ||
        entity->attached_scene_object != p->root_scene ||
        scene_root_object_get() == NULL) return NULL;
    return p->root_scene;
}
int actor_scene_entity_bind(EntitySlot* entity)
{
    static const char* const keys[] = {
        "CarryNullName", "CameraNullName", "DrillNullName",
        "DepositNullName", "FootStepNullName", "ThrowNullName"
    };
    ActorSceneType* type;
    ActorSceneInstance* p;
    LwscError error;
    float identity[16];
    if (entity == NULL || (entity->type != 2 && entity->type != 3) ||
        s_actor_backend == NULL || s_actor_failed) return 0;
    if (actor_find(entity) != NULL) return actor_valid(actor_find(entity));
    for (type = s_actor_types; type != NULL; type = type->next)
        if (type->type == entity->type && type->row == (uint32_t)entity->etype) break;
    if (type == NULL) return 0;
    if (type->assembly == NULL) {
        type->assembly = lwsc_assembly_open(type->path, "Lego*", type->feature,
                                            keys, sizeof(keys) / sizeof(keys[0]), &error);
        if (type->assembly == NULL) { actor_fail(NULL, "authored template", &error); return 0; }
    }
    p = (ActorSceneInstance*)calloc(1u, sizeof(*p));
    if (p == NULL) return 0;
    memset(identity, 0, sizeof(identity));
    identity[0] = identity[5] = identity[10] = identity[15] = 1.0f;
    p->instance = lwsc_instance_create_for_pose(type->assembly, &s_actor_ops,
                                                s_actor_parent, identity, &error);
    if (p->instance == NULL) { free(p); actor_fail(NULL, "instance create", &error); return 0; }
    p->entity = entity; p->type = type;
    p->next = s_actor_instances; s_actor_instances = p;
    if (!actor_scene_pose_set(entity)) { actor_fail(p, "initial pose", NULL); return 0; }
    p->root_scene = lwsc_winapi_scene_object(s_actor_backend,
                                             lwsc_instance_root(p->instance));
    if (p->root_scene == NULL) { actor_fail(p, "root wrapper", NULL); return 0; }
    entity_ddiscene_scene_owner_bind(entity, p->root_scene);
    fprintf(stderr, "actor-scene: entity=%p type=%d row=%lu path=%s instance=%p root=%p\n",
        entity, entity->type, (unsigned long)type->row, type->path, p->instance, p->root_scene);
    return 1;
}
int actor_scene_activity_set(EntitySlot* entity, const char* state,
                              float initial_time)
{
    ActorSceneInstance* p = actor_find(entity);
    LwscError error;
    LwscClockCallbacks callbacks;
    const EntityAssembly* metadata;
    size_t i;
    float ignored;
    if (!actor_valid(p) || state == NULL || !actor_finite(initial_time)) return 0;
    metadata = lwsc_assembly_metadata(p->type->assembly);
    for (i = 0u; i < metadata->count; ++i)
        if (strcasecmp_lite(metadata->acts[i].state, state) == 0) break;
    if (i == metadata->count) return 0;
    if (!lwsc_instance_activity_set(p->instance, state, &error)) {
        actor_fail(p, "activity change", &error); return 0;
    }
    actor_callbacks(p, &callbacks);
    if (!lwsc_instance_clock_set(p->instance, initial_time,
                                  &callbacks, &ignored, &error)) {
        actor_fail(p, "initial ACT time", &error); return 0;
    }
    return 1;
}
int actor_scene_clock_reset(EntitySlot* entity)
{
    ActorSceneInstance* p = actor_find(entity);
    LwscClockCallbacks callbacks;
    LwscError error;
    float ignored;
    if (!actor_valid(p)) return 0;
    {
        const EntityAssembly* metadata = lwsc_assembly_metadata(p->type->assembly);
        const char* expected = ai_activity_name(entity->activity);
        unsigned int index = lwsc_instance_activity(p->instance);
        if (expected == NULL || metadata == NULL || index >= metadata->count ||
            strcasecmp_lite(expected, metadata->acts[index].state) != 0) {
            actor_fail(p, "ACT clock reset identity", NULL); return 0;
        }
    }
    actor_callbacks(p, &callbacks);
    if (!lwsc_instance_clock_set(p->instance, 0.0f, &callbacks, &ignored, &error)) {
        actor_fail(p, "ACT clock reset", &error); return 0;
    }
    return 1;
}
int actor_scene_advance(EntitySlot* entity, float delta, uint32_t count,
                         float* overflow)
{
    ActorSceneInstance* p = actor_find(entity);
    LwscClockCallbacks callbacks;
    LwscError error;
    if (!actor_valid(p) || overflow == NULL) return 0;
    {
        const EntityAssembly* metadata = lwsc_assembly_metadata(p->type->assembly);
        const char* expected = ai_activity_name(entity->activity);
        unsigned int index = lwsc_instance_activity(p->instance);
        if (expected == NULL || metadata == NULL || index >= metadata->count ||
            strcasecmp_lite(expected, metadata->acts[index].state) != 0) return 0;
    }
    actor_callbacks(p, &callbacks);
    if (!lwsc_instance_advance(p->instance, delta, count,
                                &callbacks, overflow, &error)) {
        actor_fail(p, "ACT advance", &error); return 0;
    }
    return 1;
}
void* actor_scene_named_node(EntitySlot* entity, const char* key,
                              uint32_t ordinal)
{
    ActorSceneInstance* p = actor_find(entity);
    const char* name;
    const LwscFrameRef* node;
    if (!actor_valid(p) || key == NULL) return NULL;
    name = lwsc_assembly_configured_name(p->type->assembly, key);
    if (name == NULL) return NULL;
    node = lwsc_instance_named_node(p->instance, name, ordinal);
    return node != NULL ? lwsc_winapi_scene_object(s_actor_backend, node) : NULL;
}
void actor_scene_entity_release(EntitySlot* entity)
{
    ActorSceneInstance** link = &s_actor_instances;
    ActorSceneInstance* p;
    while (*link != NULL && (*link)->entity != entity) link = &(*link)->next;
    p = *link;
    if (p == NULL) return;
    entity_ddiscene_scene_owner_bind(entity, NULL);
    lwsc_instance_destroy(p->instance);
    *link = p->next;
    free(p);
}
int actor_scene_types_clear(int type)
{
    ActorSceneInstance* p = s_actor_instances;
    ActorSceneType** link = &s_actor_types;
    while (p != NULL) {
        ActorSceneInstance* next = p->next;
        if (type == 0 || p->type->type == type) actor_scene_entity_release(p->entity);
        p = next;
    }
    while (*link != NULL) {
        ActorSceneType* old = *link;
        if (type == 0 || old->type == type) {
            lwsc_assembly_release(old->assembly);
            free(old->path); *link = old->next; free(old);
        }
        else link = &old->next;
    }
    return s_actor_backend == NULL || s_actor_instances != NULL ||
        lwsc_winapi_frame_count(s_actor_backend) == 0u;
}
void actor_scene_require_stopped(void)
{
    if (!actor_scene_types_clear(0) ||
        !lwsc_winapi_context_destroy(s_actor_backend)) {
        fprintf(stderr, "actor-scene: native context remains busy; refusing global/CRT teardown\n");
        fflush(stderr); _exit(2);
    }
    s_actor_backend = NULL; s_actor_parent = NULL; s_actor_failed = 0;
    memset(&s_actor_ops, 0, sizeof(s_actor_ops));
}
#endif
