// SPDX-License-Identifier: LGPL-3.0-or-later
#include "creature_scene.hpp"

#if defined(LEGORRR_BACKEND_MINIWIN)

#include "animation_object.h"
#include "game.hpp"
#include "gods98/container_native.hpp"
#include "gods98/record.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct CreatureSceneType {
    int type;
    uint32_t row;
    char* path;
    int load_failed;
    int loaded;
    AnimationObject model;        /* the type row */
    struct CreatureSceneType* next;
} CreatureSceneType;

static CreatureSceneType* s_creature_types;

/* Creature_Load (0x406990) into the type row: the Container (a native
 * one, through container_load) and the null names from the .ae file. */
static int creature_scene_type_load(CreatureSceneType* type)
{
    if (type->loaded) return 1;
    if (type->load_failed) return 0;
    memset(&type->model, 0, sizeof(type->model));
    if (!entity_type_config_resource_row_load(&type->model, type->row, NULL,
                                              type->path, (void*)"Lego*",
                                              type->type == 2 ? 1u : 3u) ||
        type->model.container == NULL) {
        fprintf(stderr, "creature-scene: %s did not load\n", type->path);
        type->load_failed = 1;
        return 0;
    }
    /* Type rows are templates; only entity clones are drawn. */
    container_native_hidden_set(type->model.container, 1);
    type->loaded = 1;
    return 1;
}

static void creature_scene_type_free(CreatureSceneType* type)
{
    if (type->loaded) {
        /* Creature_Remove does not free the deposit name; the row owns it. */
        free(type->model.depositNullName);
        entity_payload_release_subslots((EntitySlot*)&type->model);
    }
    free(type->path);
    free(type);
}

int creature_scene_type_path_set(int type, uint32_t row, const char* path,
                                 uint32_t feature)
{
    CreatureSceneType* p;
    size_t length;
    (void)feature; /* container_load reads the engine's feature word */
    if ((type != 2 && type != 3) || path == NULL || path[0] == '\0') return 0;
    for (p = s_creature_types; p != NULL; p = p->next)
        if (p->type == type && p->row == row) return strcmp(p->path, path) == 0;
    p = (CreatureSceneType*)calloc(1u, sizeof(*p));
    if (p == NULL) return 0;
    length = strlen(path);
    p->path = (char*)malloc(length + 1u);
    if (p->path == NULL) {
        free(p);
        return 0;
    }
    memcpy(p->path, path, length + 1u);
    p->type = type;
    p->row = row;
    p->next = s_creature_types;
    s_creature_types = p;
    return 1;
}

void creature_scene_types_clear(int type)
{
    CreatureSceneType** link = &s_creature_types;
    while (*link != NULL) {
        CreatureSceneType* p = *link;
        if (type == 0 || p->type == type) {
            *link = p->next;
            creature_scene_type_free(p);
        }
        else {
            link = &p->next;
        }
    }
}

/* attached_direction_record_type2/3 are the reference's per-type model
 * pointers (entity +0x10, +0x14). */
static void** creature_scene_slot(EntitySlot* entity)
{
    if (entity == NULL) return NULL;
    if (entity->type == 2) return &entity->attached_direction_record_type2;
    if (entity->type == 3) return &entity->attached_direction_record_type3;
    return NULL;
}

AnimationObject* creature_scene_model(const EntitySlot* entity)
{
    void** slot = creature_scene_slot((EntitySlot*)entity);
    return slot != NULL ? (AnimationObject*)*slot : NULL;
}

int creature_scene_entity_create(EntitySlot* entity)
{
    void** slot = creature_scene_slot(entity);
    CreatureSceneType* type;
    AnimationObject* model;
    if (slot == NULL) return 0;
    if (*slot != NULL) return 1;
    for (type = s_creature_types; type != NULL; type = type->next)
        if (type->type == entity->type && type->row == (uint32_t)entity->etype)
            break;
    if (type == NULL || !creature_scene_type_load(type)) return 0;
    model = (AnimationObject*)calloc(1u, sizeof(*model));
    if (model == NULL) return 0;
    renderer_record_clone(&type->model, model);
    if (model->container == NULL) {
        free(model);
        return 0;
    }
    container_native_hidden_set(model->container, 0);
    /* The scene-end (TRIGGER) callback finds the entity by this word. */
    scene_object_owner_set(model->container, entity->ddiscene_owner_token);
    *slot = model;
    return 1;
}

void creature_scene_entity_release(EntitySlot* entity)
{
    void** slot = creature_scene_slot(entity);
    if (slot == NULL || *slot == NULL) return;
    entity_payload_release_subslots((EntitySlot*)*slot);
    free(*slot);
    *slot = NULL;
}

unsigned int creature_scene_meshes_visit(const EntitySlot* entity,
                                         CreatureSceneMeshVisitFn visit,
                                         void* context)
{
    AnimationObject* model = creature_scene_model(entity);
    if (model == NULL || model->container == NULL) return 0u;
    return container_native_meshes_visit(
        model->container, (ContainerNativeMeshVisitFn)visit, context);
}

#endif /* LEGORRR_BACKEND_MINIWIN */
