// SPDX-License-Identifier: LGPL-3.0-or-later
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ai.hpp"
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
    default:
        /* Codes 5..19 are the special-object kinds (FUN_0042e8b0); the
         * corpus keeps one singleton slot each (DAT_00556368..b0), which the
         * port groups under the shared special table with the object
         * stride. */
        return (type >= 5 && type <= 19) ? 0x74 : 0;
    }
}

/* Table index for a corpus type code (FUN_0042e8b0):
 * 1..4 map to the four typed tables, 5..19 map to the special table. */
static int entity_tab(int type)
{
    return (type >= 1 && type <= ENTITY_TYPES - 1) ? type - 1
                                                   : ENTITY_TYPES - 1;
}

int entity_count(GameContext* ctx, int type)
{
    if (ctx == NULL || type < 1 || type > 19) {
        return 0;
    }
    return ctx->entity_count[entity_tab(type)];
}

const EntitySlot* entity_at(GameContext* ctx, int type, int index)
{
    if (ctx == NULL || type < 1 || type > 19) {
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
    if (ctx == NULL || type < 1 || type > 19) {
        return -1;
    }
    /* Codes 1..4 map to the four typed tables; the special kinds (5..19)
     * share the last table (the corpus's per-kind singletons). */
    int ti = entity_tab(type);
    int* cnt = &ctx->entity_count[ti];
    if (*cnt >= ENTITY_CAP) {
        return -1;
    }
    EntitySlot* slot = &ctx->entity[ti][*cnt];
    __builtin_memset(slot, 0, sizeof(*slot));
    slot->type = type;
    slot->etype = type;
    slot->activity = -1;       /* corpus +0x2e8 NULL pointer = no activity */
    slot->activity_last = -1;
    slot->x = x;
    slot->y = y;
    slot->z = z;
    slot->rot = rot;
    /* Corpus spawn dispatch (FUN_00437d00): the type-2 minifigure arm
     * (at 0x437eb6) seeds its permitted-activity list with Activity_Stand
     * through the FUN_0044b370 appender — the first grant is value 0. */
    if (type == 2) {
        ai_activity_grant(ctx, slot, AI_ACT_STAND);
    }
    /* Corpus spawn dispatch default arm: the 0x438150 switch assigns the
     * minion task-state (+0x334 stats block) to the static-template types —
     * type 8 -> DAT_004a98f0, 9 -> DAT_004a9a40, 0xc..0xe and 0x10..0x13
     * -> DAT_004a9b90; every other type falls through to the instance
     * stats attacher (FUN_00469c10 at 0x43818a). The task state carries the
     * permitted-activity capacity (state +0xcc, FUN_0046a060) that the
     * appender reads per entity, so minion spawns set their own cap here
     * (the shipped templates are zero — the real capacity is filled from
     * config at runtime, TODO(corpus); the port defaults it to the span). */
    slot->task_state = ai_minion_state_for(type);
    if (slot->task_state != AI_TASK_STATE_NONE) {
        slot->activity_cap = AI_ACTIVITY_SLOTS;
    }
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

/* `.ol` `type` name -> entity table code — DECOMPILE-VERIFIED (finding 07/10).
 *
 * The corpus resolver `FUN_0042e8b0` (ObjectTypeByName) matches the name
 * case-insensitively (__strcmpi) against the four runtime name registries —
 * populated from the shipped `Lego.cfg` `VehicleTypes` (FUN_0042cda0),
 * `RockMonsterTypes` (FUN_0042d100, the type-3 general/creature table),
 * `MiniFigureTypes` and `BuildingTypes` (parallel inits) — and then against
 * a hardcoded special-object table:
 *
 *   1 = VehicleTypes      (Hoverboard, SmallDigger, … 12)
 *   2 = MiniFigureTypes   (Pilot)
 *   3 = RockMonsterTypes  (RockMonster, TinyRM, TinyIM, SmallSpider, Bat,
 *                          Slug, LavaMonster, IceMonster)
 *   4 = BuildingTypes     (Toolstation, TeleportPad, … Geo-dome, … 10)
 *   5..19 = special objects (hardcoded in FUN_0042e8b0):
 *          Boulder=5, PowerCrystal=6, Ore=7(idx 0), ProcessedOre=7(idx 1),
 *          Dynamite=8, Barrier=9, ElectricFence=11, SpiderWeb=12, OohScary=13,
 *          Path=15, Pusher=16, Freezer=17, LaserShot=19
 *   -1 = TVCamera (the .ol loader's camera branch, no entity)
 *   0  = not registered (the corpus aborts the load; the port skips, see
 *        object_list_load_file)
 *
 * The special-entity codes are stored in the port's shared table index 4
 * (ENTITY_TYPE_SPECIAL); the corpus keeps one singleton slot each
 * (DAT_00556368..b0). */
typedef struct NameCode { const char* name; int code; } NameCode;

static const NameCode s_vehicle_names[] = {
    { "hoverboard", 1 }, { "smalldigger", 1 }, { "smalltruck", 1 },
    { "smallcat", 1 },   { "smallmlp", 1 },    { "smallheli", 1 },
    { "bulldozer", 1 },  { "walkerdigger", 1 }, { "largemlp", 1 },
    { "largedigger", 1 }, { "largecat", 1 },   { "largeheli", 1 },
};
static const NameCode s_minifig_names[] = {
    { "pilot", 2 },
};
static const NameCode s_creature_names[] = {
    { "rockmonster", 3 }, { "tinyrm", 3 }, { "tinyim", 3 },
    { "smallspider", 3 }, { "bat", 3 },    { "slug", 3 },
    { "lavamonster", 3 }, { "icemonster", 3 },
};
static const NameCode s_building_names[] = {
    { "toolstation", 4 }, { "teleportpad", 4 }, { "docks", 4 },
    { "powerstation", 4 }, { "barracks", 4 },   { "upgrade", 4 },
    { "geo-dome", 4 },    { "orerefinery", 4 }, { "gunstation", 4 },
    { "teleportbig", 4 },
};
static const NameCode s_special_names[] = {
    { "boulder", 5 },      { "powercrystal", 6 },  { "ore", 7 },
    { "processedore", 7 }, { "dynamite", 8 },      { "barrier", 9 },
    { "electricfence", 11 }, { "spiderweb", 12 },  { "oohscary", 13 },
    { "path", 15 },        { "pusher", 16 },       { "freezer", 17 },
    { "lasershot", 19 },
};

static int nc_find(const NameCode* t, size_t n, const char* folded)
{
    for (size_t i = 0; i < n; ++i) {
        if (strcmp(t[i].name, folded) == 0) {
            return t[i].code;
        }
    }
    return 0;
}

/* ASCII case-insensitive compare (the corpus uses __strcmpi for the .ol
 * type names and driving refs). */
static int ci_strcmp(const char* a, const char* b);

static int entity_type_for_name(const char* name)
{
    if (name == NULL || name[0] == '\0') {
        return 0; /* unregistered — the corpus aborts, the port skips */
    }
    if (ci_strcmp(name, "tvcamera") == 0) {
        return -1; /* follow-camera branch in FUN_0042e020 */
    }
    char c[24];
    size_t i = 0;
    for (; i < sizeof(c) - 1 && name[i] != '\0'; ++i) {
        char ch = name[i];
        c[i] = (ch >= 'A' && ch <= 'Z') ? (char)(ch - 'A' + 'a') : ch;
    }
    c[i] = '\0';

    int code = nc_find(s_special_names, 13, c);
    if (code != 0) {
        return code;
    }
    code = nc_find(s_vehicle_names, 12, c);
    if (code != 0) {
        return code;
    }
    code = nc_find(s_minifig_names, 1, c);
    if (code != 0) {
        return code;
    }
    code = nc_find(s_creature_names, 8, c);
    if (code != 0) {
        return code;
    }
    return nc_find(s_building_names, 10, c);
}

static void ol_strncpy(char* dst, size_t n, const char* src)
{
    size_t i = 0;
    for (; i + 1 < n && src != NULL && src[i] != '\0'; ++i) {
        dst[i] = src[i];
    }
    dst[i] = '\0';
}

/* ASCII case-insensitive compare (the corpus uses __strcmpi for the .ol
 * type names and driving refs). */
static int ci_strcmp(const char* a, const char* b)
{
    while (*a != '\0' && *b != '\0') {
        int ca = (int)(unsigned char)*a;
        int cb = (int)(unsigned char)*b;
        if (ca >= 'A' && ca <= 'Z') {
            ca += 0x20;
        }
        if (cb >= 'A' && cb <= 'Z') {
            cb += 0x20;
        }
        if (ca != cb) {
            return ca - cb;
        }
        ++a;
        ++b;
    }
    return (int)(unsigned char)*a - (int)(unsigned char)*b;
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

        /* Shipped grammar (decompile-verified, FUN_0042e020): `type <name>`,
         * `xpos` / `ypos`, `heading`, optional `health` (default 100) and
         * `driving <ref>`. `Model` is the port's extension key (absent from
         * all 44 shipped lists). */
        const char* t = obj_val(ol, rname, name, "type");
        ol_strncpy(op.type_name, sizeof(op.type_name), t);
        op.type = entity_type_for_name(op.type_name);
        op.x = util_atof(obj_val(ol, rname, name, "xPos"));
        op.y = util_atof(obj_val(ol, rname, name, "yPos"));
        op.heading = util_atof(obj_val(ol, rname, name, "heading"));
        op.z = 0.0f;
        op.rot = op.heading;
        op.health = (int)util_atof(obj_val(ol, rname, name, "health"));
        ol_strncpy(op.driving, sizeof(op.driving),
                   obj_val(ol, rname, name, "driving"));
        ol_strncpy(op.model, sizeof(op.model), obj_val(ol, rname, name, "Model"));

        if (op.type == -1) {
            /* TVCamera — FUN_0042e020's type -1 branch drives the follow
             * camera instead of creating an entity (last one wins). */
            ctx->ol_camera_set = 1;
            ctx->ol_camera_x = op.x;
            ctx->ol_camera_y = op.y;
            ctx->ol_camera_heading = op.heading;
            ++placed;
            continue;
        }
        if (op.type == 0) {
            /* Unregistered type name — the corpus aborts the whole load
             * (FUN_0042e020 breaks and returns 0); the port skips the
             * record and keeps the rest (dev leftovers like Paul.ol's
             * Test1 would otherwise kill every load). */
            continue;
        }

        int idx = entity_create(ctx, op.type, op.name, op.x, op.y, op.z,
                                op.heading);
        if (idx >= 0) {
            EntitySlot* slot = (EntitySlot*)entity_at(ctx, op.type, idx);
            ol_strncpy(slot->type_name, sizeof(slot->type_name), op.type_name);
            ol_strncpy(slot->driving, sizeof(slot->driving), op.driving);
            slot->rot = op.heading;
            slot->health = op.health > 0 ? op.health : 100;
            entity_set_model(slot, op.model[0] != '\0' ? op.model : NULL);
            ++placed;
        }
    }
    block_close(ol);
    ctx->entity_placed = placed;
    return placed;
}
