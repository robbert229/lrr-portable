// SPDX-License-Identifier: LGPL-3.0-or-later
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ai.hpp"
#include "entity.hpp"
#include "level.hpp"
#include "cmd.hpp"
#include "gods98/model.hpp"
#include "gods98/heap.hpp"
#include "util.hpp"
#include "gods98/media.hpp"
#include "gods98/resource.hpp"
#include "frontend.hpp"

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

/* --- entity record accessors (finding 457) --------------------------------
 * Every corpus reader resolves the entity's selected stats record and reads
 * a fixed field. The port mirrors the record on the slot (NULL = the zero
 * template, the Stats config loader being unported — see entity.hpp).
 * Names are offset-preserving: the decompile proves the offsets, not the
 * field semantics. */

static const uint8_t s_stats_zero_record[ENTITY_STATS_RECORD_SIZE] = { 0 };

static const uint8_t* stats_rec(const EntitySlot* e)
{
    const uint8_t* rec = (const uint8_t*)e->stats_record;
    return rec != NULL ? rec : s_stats_zero_record;
}

static float stats_rec_float(const EntitySlot* e, int off)
{
    float f = 0.0f;
    __builtin_memcpy(&f, stats_rec(e) + off, sizeof(f));
    return f;
}

static uint32_t stats_rec_word(const EntitySlot* e, int off)
{
    uint32_t w = 0;
    __builtin_memcpy(&w, stats_rec(e) + off, sizeof(w));
    return w;
}

float entity_stats_float_at20(const EntitySlot* e) { return stats_rec_float(e, 0x20); }
float entity_stats_float_at24_indexed(const EntitySlot* e, int index)
{
    return stats_rec_float(e, 0x24 + index * 4);
}
float entity_stats_float_atd4(const EntitySlot* e) { return stats_rec_float(e, 0xd4); }
float entity_stats_float_at9c(const EntitySlot* e) { return stats_rec_float(e, 0x9c); }
float entity_stats_float_atd8(const EntitySlot* e) { return stats_rec_float(e, 0xd8); }
float entity_stats_float_at6c(const EntitySlot* e) { return stats_rec_float(e, 0x6c); }
float entity_stats_float_at74(const EntitySlot* e) { return stats_rec_float(e, 0x74); }
float entity_stats_float_at78(const EntitySlot* e) { return stats_rec_float(e, 0x78); }
float entity_stats_float_ate8(const EntitySlot* e) { return stats_rec_float(e, 0xe8); }
float entity_stats_float_at70(const EntitySlot* e) { return stats_rec_float(e, 0x70); }
void* entity_stats_field_at_a0(const EntitySlot* e)
{
    /* the corpus returns the ADDRESS record+0xa0, not its contents */
    return (void*)(stats_rec(e) + 0xa0);
}
float entity_stats_float_at7c(const EntitySlot* e) { return stats_rec_float(e, 0x7c); }
float entity_stats_float_at80(const EntitySlot* e) { return stats_rec_float(e, 0x80); }
float entity_energy_decay_rate_get(const EntitySlot* e) { return stats_rec_float(e, 0x84); }
float entity_stats_float_atdc(const EntitySlot* e) { return stats_rec_float(e, 0xdc); }
uint32_t entity_stats_word_atac(const EntitySlot* e) { return stats_rec_word(e, 0xac); }
uint32_t entity_stats_word_get(const EntitySlot* e) { return stats_rec_word(e, 0x144); }
uint32_t entity_state_flags_get(const EntitySlot* e) { return stats_rec_word(e, 0x148); }
uint32_t entity_capability_flags_get(const EntitySlot* e) { return stats_rec_word(e, 0x14c); }
float entity_stats_float_at88(const EntitySlot* e) { return stats_rec_float(e, 0x88); }
uint32_t entity_stats_word_at_c0_or_c4(const EntitySlot* e, int selector)
{
    return stats_rec_word(e, selector != 0 ? 0xc4 : 0xc0);
}
float entity_stats_float_at8c(const EntitySlot* e) { return stats_rec_float(e, 0x8c); }
float entity_stats_float_at90(const EntitySlot* e) { return stats_rec_float(e, 0x90); }
float entity_stats_float_at98(const EntitySlot* e) { return stats_rec_float(e, 0x98); }
float entity_stats_float_at94(const EntitySlot* e) { return stats_rec_float(e, 0x94); }
float entity_stats_float_atf4(const EntitySlot* e) { return stats_rec_float(e, 0xf4); }
float entity_speed_component_get(const EntitySlot* e) { return stats_rec_float(e, 0xf8); }
float entity_stats_float_at130(const EntitySlot* e) { return stats_rec_float(e, 0x130); }
float entity_stats_float_at134(const EntitySlot* e) { return stats_rec_float(e, 0x134); }
float entity_stats_float_at120(const EntitySlot* e) { return stats_rec_float(e, 0x120); }
float entity_stats_float_at128(const EntitySlot* e) { return stats_rec_float(e, 0x128); }
float entity_stats_float_at124(const EntitySlot* e) { return stats_rec_float(e, 0x124); }
uint32_t entity_stats_word_at13c(const EntitySlot* e) { return stats_rec_word(e, 0x13c); }
float entity_stats_float_at12c(const EntitySlot* e) { return stats_rec_float(e, 0x12c); }
float entity_stats_float_at138(const EntitySlot* e) { return stats_rec_float(e, 0x138); }
float entity_collision_radius_get(const EntitySlot* e) { return stats_rec_float(e, 0xe4); }
float entity_stats_float_at114(const EntitySlot* e) { return stats_rec_float(e, 0x114); }
float entity_stats_float_at118(const EntitySlot* e) { return stats_rec_float(e, 0x118); }
float entity_stats_float_at11c(const EntitySlot* e) { return stats_rec_float(e, 0x11c); }
float entity_stats_float_at110(const EntitySlot* e) { return stats_rec_float(e, 0x110); }
float entity_stats_float_at10c(const EntitySlot* e) { return stats_rec_float(e, 0x10c); }


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

/* Table match with its row (the resolver's registry-index shape). */
static int nc_find_indexed(const NameCode* t, size_t n, const char* folded,
                           int* row)
{
    for (size_t i = 0; i < n; ++i) {
        if (strcmp(t[i].name, folded) == 0) {
            if (row != NULL) {
                *row = (int)i;
            }
            return t[i].code;
        }
    }
    return 0;
}

/* ASCII case-insensitive compare (the corpus uses __strcmpi for the .ol
 * type names and driving refs). */
static int ci_strcmp(const char* a, const char* b);

/* Resolver pair (FUN_0042e8b0 shape): answers known-1 with the corpus's
 * (type, registry-row) outputs. Registry matches write both; the corpus's
 * special names write the dedicated ore-pair rows (0/1) only, leaving the
 * row slot for everything else (the dispatch aliases that slot onto its
 * first argument's word — a corpus quirk, pinned in the tests). */
static int entity_type_pair_for_name(const char* name, int* type_out,
                                     int* row_out)
{
    if (name == NULL || name[0] == '\0') {
        return 0; /* unregistered — the corpus aborts, the port skips */
    }
    if (ci_strcmp(name, "tvcamera") == 0) {
        if (type_out != NULL) {
            *type_out = -1; /* follow-camera branch in FUN_0042e020 */
        }
        return 1;
    }
    char c[24];
    size_t i = 0;
    for (; i < sizeof(c) - 1 && name[i] != '\0'; ++i) {
        char ch = name[i];
        c[i] = (ch >= 'A' && ch <= 'Z') ? (char)(ch - 'A' + 'a') : ch;
    }
    c[i] = '\0';

    int code;
    int row = 0;
    code = nc_find_indexed(s_special_names, 13, c, &row);
    if (code != 0) {
        if (type_out != NULL) {
            *type_out = code;
        }
        if (row_out != NULL) {
            if (strcmp(c, "ore") == 0) {
                *row_out = 0; /* the corpus's dedicated index stores */
            } else if (strcmp(c, "processedore") == 0) {
                *row_out = 1;
            }
        }
        return 1;
    }
    code = nc_find_indexed(s_vehicle_names, 12, c, &row);
    if (code != 0) {
        if (type_out != NULL) {
            *type_out = code;
        }
        if (row_out != NULL) {
            *row_out = row;
        }
        return 1;
    }
    code = nc_find_indexed(s_minifig_names, 1, c, &row);
    if (code != 0) {
        if (type_out != NULL) {
            *type_out = code;
        }
        if (row_out != NULL) {
            *row_out = row;
        }
        return 1;
    }
    code = nc_find_indexed(s_creature_names, 8, c, &row);
    if (code != 0) {
        if (type_out != NULL) {
            *type_out = code;
        }
        if (row_out != NULL) {
            *row_out = row;
        }
        return 1;
    }
    code = nc_find_indexed(s_building_names, 10, c, &row);
    if (code != 0) {
        if (type_out != NULL) {
            *type_out = code;
        }
        if (row_out != NULL) {
            *row_out = row;
        }
        return 1;
    }
    return 0;
}

static int entity_type_for_name(const char* name)
{
    int type = 0;
    if (!entity_type_pair_for_name(name, &type, NULL)) {
        return 0;
    }
    return type;
}

/* Public (type, row) shape of the resolver — the corpus call form with a
 * NULL fourth output (FUN_0042e8b0(name, &type, &row, 0)): table matches
 * write both, the special names write only their dedicated rows, leaving
 * the row slot for the caller to carry. */
int entity_type_row_for_name(const char* name, int* type_out, int* row_out)
{
    return entity_type_pair_for_name(name, type_out, row_out);
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

/* EntityFlagsMask107C33DEOr1C00203AnyTest (FUN_0043baf0, finding 319):
 * either flag word intersects its literal mask. */
bool entity_flags_mask107c33de_or_1c00203_any_test(const EntitySlot* e)
{
    if ((e->flags_3e0 & 0x107c33deu) != 0u) {
        return true;
    }
    return (e->flags_3e4 & 0x01c00203u) != 0u;
}

/* EntitySharedEligibilityCheck (FUN_004385b0, finding 471/472 context):
 * the command and activity layers' shared gate. Corpus order: +0x3e0 bits
 * 0x700100 clear, +0x3e4 bit 0x100000 clear, +0x340 float not negative,
 * +0x3e0 bit 0x20000 clear; a nonzero `relaxed` skips the +0x3e8 clause,
 * otherwise bit 0x20000000 must be set and bit 0x80000000 clear.
 * (+1000 decimal is +0x3e8 — the flags_3e8 mirror.) */
int entity_shared_eligibility_check(const EntitySlot* e, int relaxed)
{
    if ((e->flags_3e0 & 0x00700100u) != 0u) {
        return 0;
    }
    if ((e->flags_3e4 & 0x00100000u) != 0u) {
        return 0;
    }
    if (!(0.0f <= e->field_340)) {
        return 0;
    }
    if ((e->flags_3e0 & 0x00020000u) != 0u) {
        return 0;
    }
    if (relaxed != 0) {
        return 1;
    }
    if ((e->flags_3e8 & 0x20000000u) != 0u
        && (e->flags_3e8 & 0x80000000u) == 0u) {
        return 1;
    }
    return 0;
}

/* EntityType3Flag04000000Test (FUN_004436f0, finding 318): kind 3 with
 * +0x3e8 bit 0x04000000. The corpus reads the flag word as
 * entity[0xfa] (dword-indexed +0x3e8). */
int entity_type3_flag04000000_test(const EntitySlot* e)
{
    return (e->type == 3 && (e->flags_3e8 & 0x04000000u) != 0u) ? 1 : 0;
}

/* EntityProgressAdvanceBy25 (FUN_00447730, findings 377/448): the corpus
 * routing-progress step — add 25 (the corpus writes `- -25.0`), clear
 * the link field, clamp to the entity's own +0x340 bound, and report
 * when the bound was reached. Field meanings stay with the routing
 * layer per finding 448. */
int entity_progress_advance_by_25(EntitySlot* e)
{
    const float advanced = e->progress_344 + 25.0f;
    e->link_2fc = 0u;
    e->progress_344 = advanced;
    if (e->field_340 < advanced) {
        e->progress_344 = e->field_340;
        return 1;
    }
    return 0;
}

/* --- object pointer registry (see entity.hpp) ------------------------------
 * The corpus's ten-slot table (DAT pair at 0x5563b8 / 0x5563e0 as the
 * count): zero-initialized statics, no ported feeder. */
static int32_t  s_obj_ptr_slots[10];
static uint32_t s_obj_ptr_count = 0u;

void object_pointer_registry_clear_match(int32_t pointer)
{
    for (uint32_t i = 0; i < s_obj_ptr_count && i < 10u; ++i) {
        if (s_obj_ptr_slots[i] == pointer) {
            s_obj_ptr_slots[i] = 0;
        }
    }
}

int object_pointer_registry_get_slot(uint32_t index, int32_t* out)
{
    if (index >= s_obj_ptr_count) {
        return 0; /* deviation: the corpus reads blindly past the count */
    }
    const int32_t v = s_obj_ptr_slots[index];
    if (out != NULL) {
        *out = v;
    }
    return v != 0 ? 1 : 0;
}

uint32_t object_pointer_registry_count_peek(void)
{
    return s_obj_ptr_count;
}

/* --- context-selected globals (see entity.hpp) -----------------------------
 * The corpus reads the context-mode global (DAT_00556ec8) and the
 * current-selection pointer (DAT_00556160); both are modeled in the media
 * layer (the port's own models, fed by SoundMinimumDistanceUpdateMode).
 * The DAT_005560f8 / DAT_00556104 pair has no ported feeder — two
 * zero-initialized statics (documented deviation). */
static uint32_t s_ctx_sel_ptr_first  = 0u; /* DAT_005560f8 */
static uint32_t s_ctx_sel_ptr_second = 0u; /* DAT_00556104 */

void context_selected_pointer_pair_set(uint32_t first, uint32_t second)
{
    s_ctx_sel_ptr_first = first;
    s_ctx_sel_ptr_second = second;
}

uint32_t context_selected_pointer_pair_peek(int second)
{
    return (second != 0) ? s_ctx_sel_ptr_second : s_ctx_sel_ptr_first;
}

uint32_t context_selected_global_pointer_get(void)
{
    return (media_min_distance_mode() != 0) ? s_ctx_sel_ptr_second
                                           : s_ctx_sel_ptr_first;
}

int entity_matches_current_selection_or_owner(const void* entity)
{
    if (entity == NULL) {
        return 0;
    }
    const void* selected = media_min_distance_context();
    const int mode = media_min_distance_mode();
    void* owner = NULL;
    __builtin_memcpy(&owner, (const char*)entity + 0x36c, sizeof(owner));
    /* corpus shape: match (mode-clear && entity == selection) or
     * (owner != NULL && owner == selection); reject otherwise. */
    const int direct_ok = (mode == 0) && (entity == selected);
    const int owner_ok = (owner != NULL) && (owner == selected);
    return (direct_ok || owner_ok) ? 1 : 0;
}

/* SelectionCandidateEligibilityCheck (FUN_00452870, findings 77/85/468):
 * corpus order kept — state gate, shared predicate, then the clear-bit
 * band; the byte-read of +0x3e4's low byte (& 10 decimal) maps to the
 * flag-word mask 0xA. */
int selection_candidate_eligibility_check(const EntitySlot* e)
{
    if (e == NULL) {
        return 0; /* deviation: the corpus dereferences blindly */
    }
    if ((e->flags_3e8 & 0x100u) == 0u) {
        return 0;
    }
    if (entity_shared_eligibility_check(e, 1) == 0) {
        return 0;
    }
    if ((e->flags_3e4 & 0xAu) != 0u) { /* corpus: low byte & 10 (decimal) */
        return 0;
    }
    if ((e->flags_3e0 & 0x24000000u) != 0u) {
        return 0;
    }
    if (e->type == 1 && (e->flags_3e0 & 0x00040000u) != 0u) {
        return 0; /* corpus: `*param_1 != 1 || (word & 0x40000)` — type 1
                   * has the extra +0x3e0 bit-0x40000 rejection (finding
                   * 468); every other type ignores the branch */
    }
    return 1;
}

/* EntityFlag40000ThresholdCompare (see entity.hpp). */
int entity_flag40000_threshold_compare(const EntitySlot* e)
{
    if ((entity_state_flags_get(e) & 0x40000u) != 0u) {
        if (entity_stats_float_ate8(e) < e->field_340) {
            return 1;
        }
    }
    return 0;
}

/* EntityFlagsContainRequestedSingleBit (FUN_00438ed0, findings 327/449).
 * The corpus re-reads the word at each chain rung; the selector arms and
 * the flag bits pair one-to-one, so one read through the switch is the
 * identical answer. */
int entity_flags_contain_requested_single_bit(const EntitySlot* e,
                                              int selector)
{
    const uint32_t flags = entity_state_flags_get(e);
    switch (selector) {
    case 1:  return (flags & 0x04u) != 0u;
    case 2:  return (flags & 0x08u) != 0u;
    case 4:  return (flags & 0x10u) != 0u;
    case 8:  return (flags & 0x20u) != 0u;
    case 0x10: return (flags & 0x40u) != 0u;
    case 0x20: return (flags & 0x80u) != 0u;
    default: return 0;
    }
}

uint32_t entity_stats_flag10_test(const EntitySlot* e)
{
    return entity_stats_word_get(e) & 0x10u;
}

uint32_t entity_stats_flag20_test(const EntitySlot* e)
{
    return entity_stats_word_get(e) & 0x20u;
}

uint32_t entity_stats_flag40_test(const EntitySlot* e)
{
    return entity_stats_word_get(e) & 0x40u;
}

/* GlobalAnimationStateSet (FUN_004383a0) — see entity.hpp. */
static uint32_t s_global_animation_state; /* DAT_004eadc4 role: unowned */

void entity_global_animation_state_set(uint32_t value)
{
    s_global_animation_state = value;
}

uint32_t entity_global_animation_state_peek(void)
{
    return s_global_animation_state;
}

/* EntityFlag100AndSharedEligibilityCheck (FUN_00471ce0) — see entity.hpp. */
int entity_flag100_and_shared_eligibility_check(const EntitySlot* e)
{
    if ((entity_state_flags_get(e) & 0x100u) == 0u) {
        return 0;
    }
    return entity_shared_eligibility_check(e, e->type != 4) != 0 ? 1 : 0;
}

/* EntityQuadMeshRenderRecordAllowed (FUN_0040aae0) — see entity.hpp. */
int entity_quad_mesh_render_record_allowed(const EntitySlot* e)
{
    if ((entity_state_flags_get(e) & 0x08000000u) == 0u && e->type != 8
        && e->type != 13) {
        return 1;
    }
    return 0;
}

/* TaskTypeAndConditionCheck (FUN_0043c490) — see entity.hpp. */
int task_type_and_condition_check(const EntitySlot* target, int condition)
{
    if (entity_shared_eligibility_check(target, 1) == 0 || condition == 0) {
        return 0;
    }
    return 1;
}

/* EntityTypeAndStateFlagPredicate (FUN_0045e400) — see entity.hpp. */
int entity_type_and_state_flag_predicate(const EntitySlot* e)
{
    if (e == NULL) {
        return 0; /* deviation: the corpus strides blindly */
    }
    switch (e->type) {
    case 4: case 5: case 8: case 9: case 0xc: case 0xd:
    case 0xe: case 0x10: case 0x11: case 0x12: case 0x13:
        return 0;
    default:
        break;
    }
    return ((~entity_state_flags_get(e)) >> 29) & 1u; /* ~flags >> 0x1d & 1 */
}

/* ObjectFilterMatchesActionTarget (FUN_00404f50) — see entity.hpp. The
 * identity pair (+0x20/+0x24/+0x28) stays dword; only the +0x10 entity
 * handle widens for the host pointer it dereferences. */
static uint32_t filt_dword(const void* rec, size_t off)
{
    uint32_t v = 0;
    if (rec != NULL) {
        __builtin_memcpy(&v, (const char*)rec + off, sizeof v);
    }
    return v;
}

int object_filter_matches_action_target(const void* filter_rec,
                                        const void* target_rec)
{
    if (filter_rec == NULL || target_rec == NULL) {
        return 0; /* deviation: the corpus strides both records blindly */
    }
    const uint32_t type = filt_dword(filter_rec, 0x00);
    if (type != 5u) {
        if (type != 7u && type != 9u && type != 0xfu) {
            return 0;
        }
        uintptr_t handle = 0;
        __builtin_memcpy(&handle, (const char*)filter_rec + 0x10,
                         sizeof handle);
        if (handle != 0
            && entity_shared_eligibility_check((const EntitySlot*)handle, 0)
                   == 0) {
            return 0;
        }
    }
    if (filt_dword(filter_rec, 0x20) != 0u) {
        const uint32_t selector = filt_dword(filter_rec, 0x24);
        if (filt_dword(target_rec, 0x00) != filt_dword(filter_rec, 0x20)
            || (selector != 0xFFFFFFFFu
                && filt_dword(target_rec, 0x04) != selector)
            || filt_dword(target_rec, 0x08) != filt_dword(filter_rec, 0x28)) {
            return 0;
        }
    }
    return 1;
}

/* EntityDetachCarriedObjectAndResetState (FUN_0043bbc0) — see entity.hpp.
 * The carried half is read through the link as a native slot pointer; the
 * corpus strides its dword words blindly. */
void entity_detach_carried_object_and_reset_state(EntitySlot* e,
                                                  LevelState* lvl)
{
    if (e == NULL) {
        return; /* deviation: the corpus dereferences the slot blindly */
    }
    if (e->type != 3) {
        return;
    }
    EntitySlot* carried = (EntitySlot*)e->carry_link_308;
    if (carried != NULL && carried->type == 6) {
        e->carry_link_308 = NULL;
        e->carry_state_324 = 0;
        carried->attach_304 = NULL;
        e->flags_3e0 &= ~0x400u;
        carried->field_340 = -1.0f;
    }
    if (e->carry_ref_348 != NULL) {
        level_state_field9c_add(lvl, *(const int*)e->carry_ref_348);
    }
    e->field_340 = -1.0f;
    e->flags_3e8 |= 0x800000u;
}

/* EntityClearLinkedObjectState (FUN_00447780) — see entity.hpp. */
void entity_clear_linked_object_state(EntitySlot* e)
{
    if (e == NULL) {
        return; /* deviation: the corpus strides the slot blindly */
    }
    if ((e->flags_3e0 & 0x400u) == 0u) {
        return;
    }
    EntitySlot* carried = (EntitySlot*)e->carry_link_308;
    e->flags_3e0 &= ~0x400u;
    e->carry_state_324 = 0;
    e->carry_link_308 = NULL;
    if (carried != NULL) {
        carried->flags_3e8 |= 0x2000u;
    }
    cmd_chain_drop(e, 4);
}

/* --- the pooled-record chunk window + resolved dispatch band (2026-10-25) -- */

/* PooledRecordChunkAllocateAndLink (FUN_00438310) — see entity.hpp. The
 * corpus grows a power-of-two chunk of 0x40c-stride entity records, threads
 * them as a free list whose last link joins the previous head, and tracks
 * the chunk table with a byte counter. Deviations: the port records widen
 * the link slot (stride 0x410 where the corpus strides 0x40c, the
 * port-pointer convention) and the chunk table is bounded 32 entries where
 * the corpus strides it blindly. */

#define POOL_CHUNK_COUNT_SLOTS 32 /* deviation: bounded chunk table */
#define POOL_RECORD_STRIDE 0x410  /* corpus 0x40c widened for the link */
#define POOL_RECORD_LINK_OFF 0x408

static uint32_t s_pool_chunk_count;             /* DAT_004dee10 role */
static void* s_pool_chunks[POOL_CHUNK_COUNT_SLOTS]; /* DAT_004de790 role */
static void* s_pool_record_head;                /* DAT_004de810 role */

void pooled_record_chunk_allocate_and_link(void)
{
    const uint32_t n =
        1u << ((uint32_t)(unsigned char)s_pool_chunk_count & 0x1fu);
    void* base = engine_heap_alloc((long)n * POOL_RECORD_STRIDE);
    if (base == NULL) {
        return; /* deviation: the corpus strides the allocator's word */
    }
    if (s_pool_chunk_count < POOL_CHUNK_COUNT_SLOTS) {
        s_pool_chunks[s_pool_chunk_count] = base;
    }
    s_pool_chunk_count = s_pool_chunk_count + 1u;
    char* cur = (char*)base;
    for (uint32_t k = 1; k < n; ++k) {
        void* next = cur + POOL_RECORD_STRIDE;
        __builtin_memcpy(cur + POOL_RECORD_LINK_OFF, &next, sizeof next);
        cur += POOL_RECORD_STRIDE;
    }
    __builtin_memcpy(cur + POOL_RECORD_LINK_OFF, &s_pool_record_head,
                     sizeof(s_pool_record_head));
    s_pool_record_head = base;
}

void* pooled_record_head_peek(void) { return s_pool_record_head; }
uint32_t pooled_record_chunk_count_peek(void) { return s_pool_chunk_count; }

/* PooledRecordAllocateAndReset (FUN_004382c0) — see entity.hpp. The
 * corpus pops the free-list head, clears 0x103 dwords, self-links the
 * link slot as its busy mark, and seeds the two 0xbf800000 float words
 * (the corpus's -1.5 pair). Deviations: the port's link slot is
 * native-size (the pool-stride convention), and the grow half answers
 * NULL when the allocator is empty where the corpus strides the word. */
void* pooled_record_allocate_and_reset(void)
{
    if (s_pool_record_head == NULL) {
        pooled_record_chunk_allocate_and_link();
    }
    char* record = (char*)s_pool_record_head;
    if (record == NULL) {
        return NULL; /* deviation: empty allocator (corpus strides) */
    }
    void* link = NULL;
    __builtin_memcpy(&link, record + POOL_RECORD_LINK_OFF, sizeof link);
    s_pool_record_head = link;
    memset(record, 0, 0x40cu); /* the corpus's 0x103-dword clear */
    __builtin_memcpy(record + POOL_RECORD_LINK_OFF, &record,
                     sizeof record); /* the busy self-link */
    const uint32_t seeded = 0xbf800000u;
    __builtin_memcpy(record + 0x2f4, &seeded, 4);
    __builtin_memcpy(record + 0x2f8, &seeded, 4);
    return record;
}

/* ResolvedEntityOperationDispatch (FUN_00460910) — see entity.hpp. */
int resolved_entity_operation_dispatch(const char* name, int slot,
                                       int subtract)
{
    int column = 0;
    /* Corpus shape: the resolver zeroes its row slot at entry and only
     * table-row matches (and the ore pair) write it, so special names
     * ride the column slot with zero. */
    int type = 0;
    if (!entity_type_pair_for_name(name, &type, &column)) {
        return 0;
    }
    return grid_record_count_difference_get(type, column, slot, subtract);
}

/* --- DAT_00502bd8 indexed stats-type tables (2026-10-25) ------------------
 * The corpus reads `*(row_table[type][row])` through DAT_00502bd8's pointer
 * tables and strides the template record blindly; the config loader that
 * fills the tables is unported, so the port keeps guarded own storage and
 * guards every stride (deviations documented per the DAT-window convention).
 * The corpus float fields read as x87 float10 of the stored float; the port
 * hands back the plain float. */

static const void* s_stats_cell_ptr[STATS_TYPE_TABLE_TYPES][STATS_TYPE_TABLE_ROWS];
static int s_stats_cell_span[STATS_TYPE_TABLE_TYPES][STATS_TYPE_TABLE_ROWS];

void stats_type_table_install(int type, int row, const void* record,
                              int stride_count)
{
    if (type < 0 || type >= STATS_TYPE_TABLE_TYPES
        || row < 0 || row >= STATS_TYPE_TABLE_ROWS) {
        return; /* deviation: the corpus strides its tables blindly */
    }
    if (stride_count < 0) {
        stride_count = 0;
    }
    s_stats_cell_ptr[type][row] = stride_count > 0 ? record : NULL;
    s_stats_cell_span[type][row] = stride_count;
}

const void* stats_type_table_cell(int type, int row)
{
    if (type < 0 || type >= STATS_TYPE_TABLE_TYPES
        || row < 0 || row >= STATS_TYPE_TABLE_ROWS) {
        return NULL; /* deviation: the corpus strides blindly */
    }
    return s_stats_cell_ptr[type][row];
}

static uint32_t stats_table_read(int type, int row, int off, int stride_index)
{
    if (type < 0 || type >= STATS_TYPE_TABLE_TYPES
        || row < 0 || row >= STATS_TYPE_TABLE_ROWS || stride_index < 0) {
        return 0u; /* deviation: the corpus strides the index blindly */
    }
    const void* base = s_stats_cell_ptr[type][row];
    if (base == NULL) {
        return 0u; /* empty cell = the zero template */
    }
    const long long bound = (long long)s_stats_cell_span[type][row] * 0x150;
    const long long off64 = (long long)off + (long long)stride_index * 0x150;
    if (off64 + 4 > bound) {
        return 0u; /* deviation: the corpus strides past the template */
    }
    uint32_t word = 0u;
    __builtin_memcpy(&word, (const uint8_t*)base + off64, sizeof(word));
    return word;
}

uint32_t stats_table_word_at_b8(int type, int row) { return stats_table_read(type, row, 0xb8, 0); }
/* FUN_00469b20 */

uint32_t stats_table_word_at_b4(int type, int row, int stride_index)
{
    return stats_table_read(type, row, 0xb4, stride_index); /* FUN_00469af0 */
}
uint32_t stats_table_word_at_d0(int type, int row, int stride_index)
{
    return stats_table_read(type, row, 0xd0, stride_index); /* FUN_00469f60 */
}
uint32_t stats_table_word_at_100(int type, int row, int stride_index)
{
    return stats_table_read(type, row, 0x100, stride_index); /* FUN_0046a0f0 */
}

float stats_table_float_at_dc(int type, int row)
{
    /* FUN_00469e00 */
    const uint32_t bits = stats_table_read(type, row, 0xdc, 0);
    float f = 0.0f;
    __builtin_memcpy(&f, &bits, sizeof(f));
    return f;
}

uint32_t stats_table_word_at_144(int type, int row) { return stats_table_read(type, row, 0x144, 0); }
/* FUN_00469ec0 */
uint32_t stats_table_word_at_148(int type, int row) { return stats_table_read(type, row, 0x148, 0); }
/* FUN_00469ee0 */
uint32_t stats_table_word_at_14c(int type, int row) { return stats_table_read(type, row, 0x14c, 0); }
/* FUN_00469f00 */

static uint32_t stats_instance_read(const void* config_block, int off)
{
    if (config_block == NULL) {
        return 0u; /* deviation: the corpus dereferences blindly */
    }
    int type = 0, row = 0, index = 0;
    __builtin_memcpy(&type, (const uint8_t*)config_block + 0x0, sizeof(type));
    __builtin_memcpy(&row, (const uint8_t*)config_block + 0x4, sizeof(row));
    __builtin_memcpy(&index, (const uint8_t*)config_block + 0x330, sizeof(index));
    return stats_table_read(type, row, off, index);
}

uint32_t stats_instance_word_at_bc(const void* config_block) { return stats_instance_read(config_block, 0xbc); }
/* FUN_00469b50 */
uint32_t stats_instance_word_at_e0(const void* config_block) { return stats_instance_read(config_block, 0xe0); }
/* FUN_00469b80 */
uint32_t stats_instance_word_at_ec(const void* config_block) { return stats_instance_read(config_block, 0xec); }
/* FUN_00469bb0 */
uint32_t stats_instance_word_at_f0(const void* config_block) { return stats_instance_read(config_block, 0xf0); }
/* FUN_00469be0 */

/* --- entity query flag count (2026-10-25) ----------------------------------
 * EntityQueryFlagCount (FUN_0041b3c0): catalog-kind query. 0x13 answers the
 * second catalog count word; 0x14 counts the first catalog's rows whose
 * +148 word raises bit 0x1000 or 0x2000 while the +14c bit 8 stays clear;
 * 0x15 counts rows with bit 0x800 raised or the +14c bit 8 raised; any
 * other query strides the 8-byte id-pair table and answers that pair's
 * size. Deviations: the count globals and the id table have no ported
 * corpus owner (their loader band is unported), so they ride own storage
 * zero-init behind install/peek hooks (the unowned-DAT convention); rows
 * past the guarded stats table read the zero template through the ported
 * stats getters, and the pair window is guarded (the corpus strides
 * blindly). */

/* The catalog window band: the four count words and the four name tables
 * the encyclopedia loader indexes (the corpus's loader band is unported,
 * so the window rides own storage behind the hooks below; counts 0/1 are
 * the DAT_0055627c/88 roles the query lanes already used). */
static uint32_t s_catalog_counts[4]; /* DAT_0055627c/80/84/88 roles */
static const void* s_catalog_name_tables[4]; /* DAT_005561e8/ec/f0/f4 roles */
#define CATALOG_ID_PAIRS 8u

int entity_query_flag_count(int query)
{
    if (query == 0x13) {
        return (int)s_catalog_counts[1];
    }
    if (query == 0x14 || query == 0x15) {
        int count = 0;
        for (uint32_t row = 0; row < s_catalog_counts[0]; ++row) {
            const uint32_t w148 = stats_table_word_at_148(1, (int)row);
            const uint32_t w14c = stats_table_word_at_14c(1, (int)row);
            if (query == 0x14) {
                if (((w148 & 0x1000u) != 0u || (w148 & 0x2000u) != 0u)
                    && (w14c & 8u) == 0u) {
                    count = count + 1;
                }
            } else if ((w148 & 0x800u) != 0u || (w14c & 8u) != 0u) {
                count = count + 1;
            }
        }
        return count;
    }
    if (query < 0 || (unsigned)query >= CATALOG_ID_PAIRS) {
        return 0; /* deviation: the corpus strides blindly */
    }
    /* The pair word is the stride-8 band's count word (the corpus reads
     * one and the same window for both roles); the merged model lives
     * behind the resource band accessor. */
    return (int)resource_handle_set_band_word_peek(query);
}

void catalog_count_install(int which, uint32_t value)
{
    if (which >= 0 && which < 4) {
        s_catalog_counts[which] = value;
    }
}

uint32_t catalog_count_peek(int which)
{
    return (which >= 0 && which < 4) ? s_catalog_counts[which] : 0u;
}

/* Port hooks over the encyclopedia loader's name-table windows: kind 0..3
 * ride the DAT_005561e8/ec/f0/f4 arrays in address order. */
void catalog_name_table_install(int kind, const void* names)
{
    if (kind >= 0 && kind < 4) {
        s_catalog_name_tables[kind] = names;
    }
}

const void* catalog_name_table_peek(int kind)
{
    return (kind >= 0 && kind < 4) ? s_catalog_name_tables[kind] : NULL;
}

/* CatalogCallbackByIdDispatch (FUN_0041b680): dispatch over the shared
 * stride-8 band. Query 0x13 walks the second catalog count, handing the
 * callback the row lane; 0x14 walks the first catalog's rows and calls
 * the callback on the rows whose +148 word raises bit 0x1000 or 0x2000
 * while the +14c bit 8 stays clear (the first callback that answers
 * nonzero wins); 0x15 hands over rows raising bit 0x800 or the +14c bit
 * 8; any other query needs a nonzero pair count and walks the group's
 * code array through the callback — 0x22/0x23 codes ride through the
 * selection-code resolve, which rewrites the array element in place
 * (the corpus's cached pick). Deviations: the query lanes thread the
 * context block (the resolve's state window is not a corpus argument),
 * NULL callback/pair arrays answer zero where the corpus strides
 * blindly, and the guarded reads fold the corpus's blind strides. */
int catalog_callback_by_id_dispatch(const GameContext* ctx, int query,
                                    int (*callback)(int, int, int, void*),
                                    void* payload)
{
    if (callback == NULL) {
        return 0; /* deviation: the corpus calls the pointer blindly */
    }
    if (query == 0x13) {
        for (uint32_t row = 0; row < s_catalog_counts[1]; ++row) {
            if (callback(0x49, 4, (int)row, payload) != 0) {
                return 1;
            }
        }
        return 0;
    }
    if (query == 0x14 || query == 0x15) {
        for (uint32_t row = 0; row < s_catalog_counts[0]; ++row) {
            int answer = 0;
            if (query == 0x14) {
                const uint32_t w148 = stats_table_word_at_148(1, (int)row);
                if ((w148 & 0x1000u) != 0u || (w148 & 0x2000u) != 0u) {
                    const uint32_t w14c = stats_table_word_at_14c(1, (int)row);
                    if ((w14c & 8u) == 0u) {
                        answer = callback(0x49, 1, (int)row, payload);
                    }
                }
            } else {
                const uint32_t w148 = stats_table_word_at_148(1, (int)row);
                const uint32_t w14c = stats_table_word_at_14c(1, (int)row);
                if ((w148 & 0x800u) != 0u || (w14c & 8u) != 0u) {
                    answer = callback(0x49, 1, (int)row, payload);
                }
            }
            if (answer != 0) {
                return 1;
            }
        }
        return 0;
    }
    if (resource_handle_set_band_word_peek(query) == 0u) {
        return 0;
    }
    int* codes = (int*)resource_handle_set_slot_peek(query);
    if (codes == NULL) {
        return 0; /* deviation: the corpus strides the pair blindly */
    }
    for (uint32_t i = 0;;) {
        if (codes[i] == 0x22 || codes[i] == 0x23) {
            /* the corpus rewrites the cached element through the resolve */
            selection_entity_code_resolve(ctx, &codes[i]);
        }
        if (callback(codes[i], 0, 0, payload) != 0) {
            return 1;
        }
        /* corpus order: the index increments and the band word re-reads
         * every hop (a callback that re-registers the group retargets the
         * walk mid-sweep), and the walk stops before the first index the
         * count no longer covers */
        if (++i == 0u
                || resource_handle_set_band_word_peek(query) <= i) {
            return 0;
        }
    }
}

void catalog_id_pair_install(int entry, uint32_t size)
{
    /* The pair word rides the merged stride-8 band window (see above),
     * guarded to the corpus's strideable range. */
    if (entry >= 0 && (unsigned)entry < CATALOG_ID_PAIRS) {
        resource_handle_set_band_word_install(entry, size);
    }
}
