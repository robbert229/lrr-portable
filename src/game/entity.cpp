// SPDX-License-Identifier: LGPL-3.0-or-later
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "entity.hpp"
#include "gods98/model.hpp"
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

/* --- model cache (finding 30) --------------------------------------------
 * One parsed LWO model per resource path, process-lifetime for the demo —
 * models are never freed (documented; the renderer only reads them). */
#define MODEL_CACHE_MAX 16
static struct {
    const char* path;
    const void* model;
} s_model_cache[MODEL_CACHE_MAX];
static int s_model_cache_n = 0;

static char* model_dup(const char* s)
{
    size_t n = strlen(s);
    char* d = (char*)malloc(n + 1);
    if (d != NULL) {
        memcpy(d, s, n);
        d[n] = '\0';
    }
    return d;
}

static const void* model_for_path(const char* path)
{
    if (path == NULL || path[0] == '\0') {
        return NULL;
    }
    for (int i = 0; i < s_model_cache_n; ++i) {
        if (s_model_cache[i].path != NULL
            && strcmp(s_model_cache[i].path, path) == 0) {
            return s_model_cache[i].model;
        }
    }
    const void* m = NULL;
    if (s_model_cache_n < MODEL_CACHE_MAX) {
        m = model_open(path);
        char* kept = m != NULL ? model_dup(path) : NULL;
        if (kept != NULL) {
            s_model_cache[s_model_cache_n].path = kept;
            s_model_cache[s_model_cache_n].model = m;
            ++s_model_cache_n;
        }
    }
    return m;
}

void entity_set_model(EntitySlot* e, const char* path)
{
    if (e == NULL) {
        return;
    }
    e->model_path[0] = '\0';
    e->model = NULL;
    if (path != NULL && path[0] != '\0') {
        int i = 0;
        for (; i < 63 && path[i] != '\0'; ++i) {
            e->model_path[i] = path[i];
        }
        e->model_path[i] = '\0';
        e->model = model_for_path(path);
    }
}

const void* entity_model(const EntitySlot* e)
{
    return e != NULL ? e->model : NULL;
}

/* `.ol` `type` name -> entity table code (provisional seam). The corpus's
 * .ol consumer (FUN_0042e020) is not decompiled (finding 07 §"Still open"),
 * so the numeric code is inferred from the shipped object vocabulary and
 * the four-table semantics of EntityPointer (finding 10), NOT pinned by
 * decompile evidence. The grammar the code rides on (name, type name,
 * xPos/yPos/heading, driving) IS corpus-verified (entity_test --real over
 * all 44 shipped lists); only the code is the seam. */
static int entity_type_for_name(const char* name)
{
    if (name == NULL || name[0] == '\0') {
        return ENTITY_TYPE_VEHICLE;
    }
    char c[24];
    size_t i = 0;
    for (; i < sizeof(c) - 1 && name[i] != '\0'; ++i) {
        char ch = name[i];
        c[i] = (ch >= 'A' && ch <= 'Z') ? (char)(ch - 'A' + 'a') : ch;
    }
    c[i] = '\0';

    if (strcmp(c, "pilot") == 0 || strcmp(c, "test1") == 0) {
        return ENTITY_TYPE_MINIFIG;
    }
    if (strcmp(c, "hoverboard") == 0 || strcmp(c, "smalldigger") == 0
        || strcmp(c, "smalltruck") == 0 || strcmp(c, "bulldozer") == 0
        || strcmp(c, "smallheli") == 0 || strcmp(c, "largeheli") == 0
        || strcmp(c, "largedigger") == 0 || strcmp(c, "walkerdigger") == 0
        || strcmp(c, "smallcat") == 0 || strcmp(c, "largecat") == 0) {
        return ENTITY_TYPE_VEHICLE;
    }
    if (strcmp(c, "toolstation") == 0 || strcmp(c, "powerstation") == 0
        || strcmp(c, "barracks") == 0 || strcmp(c, "teleportpad") == 0
        || strcmp(c, "teleportbig") == 0 || strcmp(c, "gunstation") == 0
        || strcmp(c, "docks") == 0 || strcmp(c, "geo-dome") == 0
        || strcmp(c, "upgrade") == 0) {
        return ENTITY_TYPE_BUILDING;
    }
    /* Cameras/crystals (TVCamera, PowerCrystal) and the monster family
     * (LavaMonster, IceMonster, RockMonster, Bat, SmallSpider) land in the
     * general-object table provisionally — monsters may carry their own
     * code in the corpus (not decompiled). */
    return ENTITY_TYPE_OBJ;
}

static void ol_strncpy(char* dst, size_t n, const char* src)
{
    size_t i = 0;
    for (; i + 1 < n && src != NULL && src[i] != '\0'; ++i) {
        dst[i] = src[i];
    }
    dst[i] = '\0';
}

int object_list_load(GameContext* ctx, BlockFile* cfg, const char* level_name)
{
    if (ctx == NULL || cfg == NULL || level_name == NULL) {
        return -1;
    }
    char olist[256] = { 0 };
    char path[512];
    if (util_level_cfg_path(cfg, level_name, "OListFile", path, sizeof(path))
        == NULL) {
        return -1; /* no object list for this level */
    }
    const char* v = block_get_strptr(cfg, path);
    if (v == NULL) {
        return -1; /* no object list for this level */
    }
    snprintf(olist, sizeof(olist), "%s", v);
    return object_list_load_file(ctx, olist);
}

int object_list_load_file(GameContext* ctx, const char* olist)
{
    if (ctx == NULL || olist == NULL || olist[0] == '\0') {
        return -1;
    }

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

        /* Shipped grammar (cross-checked, finding 07/12): `type <name>`,
         * `xPos`, `yPos`, `heading`, optional `driving <ref>`. `Model` is
         * the port's extension key (absent from all 44 shipped lists). */
        const char* t = obj_val(ol, rname, name, "type");
        ol_strncpy(op.type_name, sizeof(op.type_name), t);
        op.type = entity_type_for_name(op.type_name);
        op.x = util_atof(obj_val(ol, rname, name, "xPos"));
        op.y = util_atof(obj_val(ol, rname, name, "yPos"));
        op.heading = util_atof(obj_val(ol, rname, name, "heading"));
        op.z = 0.0f;
        op.rot = op.heading;
        ol_strncpy(op.driving, sizeof(op.driving),
                   obj_val(ol, rname, name, "driving"));
        ol_strncpy(op.model, sizeof(op.model), obj_val(ol, rname, name, "Model"));

        int idx = entity_create(ctx, op.type, op.name, op.x, op.y, op.z,
                                op.heading);
        if (idx >= 0) {
            EntitySlot* slot = (EntitySlot*)entity_at(ctx, op.type, idx);
            ol_strncpy(slot->type_name, sizeof(slot->type_name), op.type_name);
            ol_strncpy(slot->driving, sizeof(slot->driving), op.driving);
            slot->rot = op.heading;
            entity_set_model(slot, op.model[0] != '\0' ? op.model : NULL);
            ++placed;
        }
    }
    block_close(ol);
    ctx->entity_placed = placed;
    return placed;
}
