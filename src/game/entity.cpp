// SPDX-License-Identifier: LGPL-3.0-or-later
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "entity.hpp"
#include "util.hpp"

int entity_stride(int type)
{
    switch (type) {
    case ENTITY_TYPE_VEHICLE:  return 0x1ec; /* corpus 005561d4 stride */
    case ENTITY_TYPE_MINIFIG:  return 0x74;  /* corpus 005561d8 stride */
    case ENTITY_TYPE_OBJ:      return 0x74;  /* corpus 005561dc stride */
    case ENTITY_TYPE_BUILDING: return 0x14c; /* corpus 005561e0 stride */
    default:                   return 0;
    }
}

/* Table index for a corpus entity-type code (placement kinds 1..4 map to
 * the matching table; 5..9 share the last). */
static int entity_tab(int type)
{
    return (type <= ENTITY_TYPES) ? type - 1 : ENTITY_TYPES - 1;
}

int entity_count(GameContext* ctx, int type)
{
    if (ctx == NULL || type < 1 || type > 9) {
        return 0;
    }
    return ctx->entity_count[entity_tab(type)];
}

const EntitySlot* entity_at(GameContext* ctx, int type, int index)
{
    if (ctx == NULL || type < 1 || type > 9) {
        return NULL;
    }
    int tab = entity_tab(type);
    if (index < 0 || index >= ctx->entity_count[tab]) {
        return NULL;
    }
    return &ctx->entity[tab][index];
}

int entity_create(GameContext* ctx, int type, const char* name,
                  float x, float y, float z, float rot)
{
    if (ctx == NULL || type < 1 || type > 9) {
        return -1;
    }
    /* Corpus placement kinds 1..4 map to the four tables; other entity-type
     * codes (6 minion, 7/8/9) share the last table. */
    int ti = (type <= ENTITY_TYPES) ? type - 1 : ENTITY_TYPES - 1;
    int* cnt = &ctx->entity_count[ti];
    if (*cnt >= ENTITY_CAP) {
        return -1;
    }
    EntitySlot* slot = &ctx->entity[ti][*cnt];
    __builtin_memset(slot, 0, sizeof(*slot));
    slot->type = type;
    slot->etype = type;
    slot->x = x;
    slot->y = y;
    slot->z = z;
    slot->rot = rot;
    if (name != NULL) {
        int i = 0;
        for (; i < 31 && name[i] != '\0'; ++i) {
            slot->name[i] = name[i];
        }
        slot->name[i] = '\0';
    }
    return (*cnt)++;
}

/* Parse an optional child value under "<root> <obj> <key>". */
static const char* obj_val(BlockFile* bf, const char* root, const char* obj,
                           const char* key)
{
    char path[512];
    snprintf(path, sizeof(path), "%s %s %s", root, obj, key);
    return block_get_strptr(bf, path);
}

int object_list_load(GameContext* ctx, BlockFile* cfg, const char* level_name)
{
    if (ctx == NULL || cfg == NULL || level_name == NULL) {
        return -1;
    }
    char olist[256] = { 0 };
    char path[512];
    snprintf(path, sizeof(path), "%s %s %s", util_root_name(cfg), level_name,
             "olistfile");
    const char* v = block_get_strptr(cfg, path);
    if (v == NULL) {
        return -1; /* no object list for this level */
    }
    snprintf(olist, sizeof(olist), "%s", v);

    BlockFile* ol = block_open(olist);
    if (ol == NULL) {
        return -1; /* missing/unparsable .ol resource */
    }

    int placed = 0;
    /* Root = the file's first record; objects are its children (document
     * order, like the entity list the corpus walks). */
    BlockRec* rootrec = block_record_at(ol, 0);
    const char* rname = rootrec ? block_name(rootrec) : NULL;
    if (rname == NULL) {
        block_close(ol);
        return -1;
    }
    for (BlockRec* rec = block_first_child(rootrec); rec != NULL;
         rec = block_next_sibling(rec)) {
        const char* name = block_name(rec);
        if (name == NULL) {
            continue;
        }
        ObjectPlacement op;
        __builtin_memset(&op, 0, sizeof(op));
        snprintf(op.name, sizeof(op.name), "%s", name);
        op.type = ENTITY_TYPE_VEHICLE; /* default */

        const char* t = obj_val(ol, rname, name, "Type");
        if (t != NULL) {
            op.type = (int)util_atoi(t);
            if (op.type < 1) {
                op.type = ENTITY_TYPE_VEHICLE;
            }
        }
        const char* x = obj_val(ol, rname, name, "x");
        const char* y = obj_val(ol, rname, name, "y");
        const char* z = obj_val(ol, rname, name, "z");
        const char* r = obj_val(ol, rname, name, "Rot");
        op.x = x ? util_atof(x) : 0.0f;
        op.y = y ? util_atof(y) : 0.0f;
        op.z = z ? util_atof(z) : 0.0f;
        op.rot = r ? util_atof(r) : 0.0f;

        if (entity_create(ctx, op.type, op.name, op.x, op.y, op.z, op.rot) >= 0) {
            ++placed;
        }
    }
    block_close(ol);
    ctx->entity_placed = placed;
    return placed;
}
