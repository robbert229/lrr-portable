// SPDX-License-Identifier: LGPL-3.0-or-later
// Entity table + `.ol` object list (findings 10/07/12).
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "resource.hpp"
#include "game/ai.hpp"
#include "game/entity.hpp"
#include "game/level.hpp"
#include "gods98/media.hpp"
#include "game/game.hpp"
#include "gods98/block.hpp"
#include "gods98/resource.hpp"
#include "game/frontend.hpp"
#include "game/cmd.hpp"
#include "game/select.hpp"
#include "util.hpp"

static int s_failures = 0;

#define CHECK(cond, msg)                                                       \
    do {                                                                       \
        if (!(cond)) {                                                         \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, msg);      \
            ++s_failures;                                                      \
        }                                                                      \
    } while (0)

static const char* FIX = "entity_fixture";

static void write_fixture(const char* name, const char* body)
{
    char path[512];
    snprintf(path, sizeof(path), "%s/%s", FIX, name);
    FILE* fp = fopen(path, "w");
    if (fp != NULL) {
        fputs(body, fp);
        fclose(fp);
    }
}

static int lc(int c) { return (c >= 'A' && c <= 'Z') ? c + 0x20 : c; }
static int ci_eq(const char* a, const char* b)
{
    while (*a != '\0' && *b != '\0') {
        if (lc((int)(unsigned char)*a) != lc((int)(unsigned char)*b)) {
            return 0;
        }
        ++a;
        ++b;
    }
    return *a == '\0' && *b == '\0';
}

/* ------------------------------------------------------------------ *
 *  Synthetic fixture test — the SHIPPED .ol grammar (cross-checked):
 *
 *    Lego* {
 *      <name> {
 *        type       <name>
 *        xPos       <float>
 *        yPos       <float>
 *        heading    <float>
 *        driving    <ref>       (optional)
 *        Model      <path>      (port extension; absent in shipped data)
 *      }
 *    }
 * ------------------------------------------------------------------ */

#define FIX_DIM 64

/* A FIX_DIM x FIX_DIM all-zero surface map for the scratch level terrain. */
static void write_surface_map(const char* path)
{
    FILE* fp = fopen(path, "wb");
    if (fp == NULL) {
        return;
    }
    const char magic[4] = { 'M', 'A', 'P', ' ' };
    const uint32_t len = FIX_DIM * FIX_DIM * 2, w = FIX_DIM, h = FIX_DIM;
    static const uint16_t cells[FIX_DIM * FIX_DIM] = { 0 };
    fwrite(magic, 1, 4, fp);
    fwrite(&len, 1, 4, fp);
    fwrite(&w, 1, 4, fp);
    fwrite(&h, 1, 4, fp);
    fwrite(cells, 1, sizeof(cells), fp);
    fclose(fp);
}

static int fixture_roundtrip(void)
{
    BlockFile* cfg = block_open("Lego.cfg");
    CHECK(cfg != NULL, "open cfg");
    if (cfg == NULL) {
        return 1;
    }
    static GameContext ctx;
    game_context_init(&ctx);
    ctx.cfg = cfg;

    /* The faithful OListEntityRecordParse places every object through the
     * level's terrain (grid cell -> world centre) and only creates entities
     * on cells carrying the revealed/walk flag 0x8 (others are deferred), so
     * the context needs a level with a real terrain and open cells. File
     * opens resolve under the CD root (the fixture directory). */
    write_surface_map("L01_surface.map");
    void* const level_terrain = terrain_create("L01_surface.map", 1.0f);
    CHECK(level_terrain != NULL, "scratch terrain");
    static LevelState level;
    static uint8_t level_grid[FIX_DIM * FIX_DIM * LEVEL_VOXEL_STRIDE];
    memset(&level, 0, sizeof(level));
    level_set_int(&level, LEVEL_OFF_WIDTH, FIX_DIM);
    level_set_int(&level, LEVEL_OFF_HEIGHT, FIX_DIM);
    level.ptr_voxels = level_grid;
    level.ptr_terrain = level_terrain;
    for (int c = 0; c < FIX_DIM * FIX_DIM; ++c) {
        const uint32_t open = 0x8u;
        memcpy(level_grid + c * LEVEL_VOXEL_STRIDE + 8, &open, sizeof(open));
    }
    ctx.level = &level;

    /* object_list_load answers success (1); the entity count rides
     * ctx.entity_placed: 3 entities + the camera. */
    int placed = object_list_load(&ctx, cfg, "Level01");
    CHECK(placed == 1 && ctx.entity_placed == 3,
          "the list loads: 2 entities + camera placed (the building needs "
          "a type-config row)");

    /* World placement of an .ol xPos/yPos: grid = pos - 1, truncated to the
     * cell, centred through the terrain (blocksize 1: y is inverted). */
    auto ol_world = [&](float xpos, float ypos, float out[2]) {
        const float gx = xpos - 1.0f, gy = ypos - 1.0f;
        const int cx = (int)gx, cy = (int)gy;
        float center[2] = { 0.0f, 0.0f };
        terrain_cell_world_center_xy_transform(level_terrain, cx, cy, center);
        out[0] = center[0] + (gx - ((float)cx + 0.5f));
        out[1] = center[1] - (gy - ((float)cy + 0.5f));
    };
    float want[2];

    CHECK(entity_count(&ctx, ENTITY_TYPE_VEHICLE) == 1, "one vehicle");
    /* The building record needs a building archetype row (type-config
     * table with its shape data) to be created; the synthetic fixture has
     * none, so the faithful creator places no entity for it. The type name
     * still resolves to the building type. */
    CHECK(entity_count(&ctx, ENTITY_TYPE_BUILDING) == 0,
          "no building without a building type-config row");
    {
        int btype = 0, brow = 0;
        CHECK(entity_type_row_for_name("Toolstation", &btype, &brow) != 0 &&
                  btype == ENTITY_TYPE_BUILDING,
              "Toolstation resolves to the building type");
    }
    CHECK(entity_count(&ctx, ENTITY_TYPE_MINIFIG) == 1, "one minifigure");
    CHECK(entity_count(&ctx, ENTITY_TYPE_SPECIAL) == 0, "no special objects");

    const EntitySlot* v = entity_at(&ctx, ENTITY_TYPE_VEHICLE, 0);
    CHECK(v != NULL && strcmp(v->name, "Digger1") == 0, "vehicle name");
    ol_world(12.5f, 19.5f, want);
    CHECK(v != NULL && v->x == want[0] && v->y == want[1] && v->z == 0.0f,
          "vehicle xPos/yPos placed through the terrain (z is 0 in the .ol)");
    /* heading is degrees in the .ol, radians on the slot. */
    CHECK(v != NULL && v->rot == 45.0f * 0.0055555557f * 3.1415927f,
          "vehicle heading -> rot (radians)");
    CHECK(v != NULL && v->type == ENTITY_TYPE_VEHICLE, "SmallDigger -> vehicle");
    CHECK(v != NULL && strcmp(v->type_name, "SmallDigger") == 0,
          "type_name from the .ol `type` key");
    CHECK(v != NULL && v->driving[0] == '\0', "no driving ref on a vehicle");

    const EntitySlot* m = entity_at(&ctx, ENTITY_TYPE_MINIFIG, 0);
    CHECK(m != NULL && strcmp(m->name, "MinifigA") == 0, "minifig name");
    CHECK(m != NULL && strcmp(m->type_name, "Pilot") == 0,
          "Pilot -> minifig + type_name");
    CHECK(m != NULL && strcmp(m->driving, "Digger1") == 0,
          "driving ref captured");
    /* The faithful creator stores the parsed health in +0x340 (default 100). */
    CHECK(m != NULL && m->field_340 == 150.0f, ".ol `health` captured");
    CHECK(v != NULL && v->field_340 == 100.0f, "default .ol health is 100");
    /* Optional Model key parses into the slot (the file does not exist, so
     * the handle stays NULL — lazy load). */
    CHECK(m != NULL && strcmp(m->model_path, "Data\\Tools\\dig.lwo") == 0,
          "minifig model path from the .ol Model key");
    CHECK(m != NULL && entity_model(m) == NULL, "missing model file -> NULL");

    /* TVCamera — FUN_0042e020's type -1 branch drives the follow camera
     * instead of placing an entity. */
    CHECK(object_pointer_registry_count_peek() == 2u,
          "the loader fed the registry with the two created entities");
    CHECK(ctx.ol_camera_set == 1, "camera placed");
    ol_world(5.0f, 6.0f, want);
    CHECK(ctx.ol_camera_x == want[0] && ctx.ol_camera_y == want[1],
          "camera xPos/yPos placed through the terrain");
    CHECK(ctx.ol_camera_heading == 30.0f * 0.0055555557f * 3.1415927f,
          "camera heading (radians)");

    /* Unregistered type name (Test1) — the corpus aborts the whole parse
     * (the old port skipped just that record): the load answers 0, keeps the
     * records placed before it and creates nothing for Test1 or after. */
    {
        static GameContext bad;
        game_context_init(&bad);
        bad.level = &level;
        CHECK(object_list_load_file(&bad, "M02.ol") == 0 &&
                  bad.entity_placed == 1 &&
                  entity_count(&bad, ENTITY_TYPE_VEHICLE) == 1 &&
                  entity_count(&bad, ENTITY_TYPE_MINIFIG) == 0,
              "an unregistered type aborts the parse after the earlier records");
    }

    /* Corpus strides. */
    CHECK(entity_stride(ENTITY_TYPE_VEHICLE) == 0x1ec, "vehicle stride");
    CHECK(entity_stride(ENTITY_TYPE_MINIFIG) == 0x74, "minifig stride");
    CHECK(entity_stride(ENTITY_TYPE_BUILDING) == 0x14c, "building stride");
    CHECK(entity_stride(ENTITY_TYPE_SPECIAL) == 0x74, "special stride (grouped)");
    CHECK(entity_stride(20) == 0, "code 20 out of range stride 0");

    /* Table-exhaustion is guarded. */
    static GameContext cap;
    game_context_init(&cap);
    int filled = 0;
    while (entity_create(&cap, ENTITY_TYPE_VEHICLE, "x", 0, 0, 0, 0) >= 0) {
        ++filled;
    }
    CHECK(filled == ENTITY_CAP, "vehicle table caps at ENTITY_CAP");

    block_close(cfg);
    return 0;
}

/* ------------------------------------------------------------------ *
 *  Real-data probe (--real <wad-dir>): every level's .ol in the shipped
 *  Lego.cfg is loaded through object_list_load and cross-checked against an
 *  INDEPENDENT line/token parser over the raw file text (no block.hpp — the
 *  loader and the ground truth can't share a bug). Count, per-object
 *  grammar fields and an FNV-1a digest must all match.
 * ------------------------------------------------------------------ */

#define OL_PROBE_MAX 512

typedef struct OlRec {
    char  name[64];
    char  type_name[24];
    float x, y, heading;
    int   health;
    char  driving[40];
} OlRec;

/* Independent .ol tokenizer. Returns the number of object records. */
static int ol_parse_indep(const char* text, OlRec* out, int max)
{
    int n = 0, depth = 0, cur = -1;
    const char* p = text;
    while (*p != '\0') {
        const char* nl = strchr(p, '\n');
        size_t len = nl != NULL ? (size_t)(nl - p) : strlen(p);
        char line[512];
        size_t l = len < sizeof(line) - 1 ? len : sizeof(line) - 1;
        memcpy(line, p, l);
        line[l] = '\0';
        while (l > 0 && (line[l - 1] == '\r' || line[l - 1] == ' '
                         || line[l - 1] == '\t')) {
            line[--l] = '\0';
        }
        if (l == 0) {
            p = nl ? nl + 1 : p + l;
            continue;
        }
        /* strip `;` comments to end of line */
        for (size_t i = 0; i < l; ++i) {
            if (line[i] == ';') {
                l = i;
                line[i] = '\0';
                break;
            }
        }
        /* tokenize on whitespace */
        char* toks[8];
        int nt = 0;
        for (size_t i = 0; i < l && nt < 8;) {
            while (i < l && (line[i] == ' ' || line[i] == '\t')) {
                ++i;
            }
            if (i >= l) {
                break;
            }
            toks[nt++] = &line[i];
            while (i < l && line[i] != ' ' && line[i] != '\t') {
                ++i;
            }
            if (i < l) {
                line[i++] = '\0';
            }
        }
        if (nt == 0) {
            p = nl ? nl + 1 : p + l;
            continue;
        }
        if (strcmp(toks[0], "}") == 0) {
            --depth;
            p = nl ? nl + 1 : p + l;
            continue;
        }
        if (strcmp(toks[0], "{") == 0) {
            ++depth;
            p = nl ? nl + 1 : p + l;
            continue;
        }
        if (nt >= 2 && strcmp(toks[1], "{") == 0) {
            /* a record open: at depth 1 the shipped lists place one record
             * per object (name is ObjectN or a special name like Camera);
             * the root record opened at depth 0, so this is the object. */
            if (depth == 1 && n < max) {
                cur = n;
                __builtin_memset(&out[cur], 0, sizeof(out[cur]));
                out[cur].health = 100; /* corpus default (FUN_0042e020) */
                snprintf(out[cur].name, sizeof(out[cur].name), "%s", toks[0]);
                ++n;
            }
            ++depth;
            p = nl ? nl + 1 : p + l;
            continue;
        }
        /* field line inside an object */
        if (depth >= 2 && cur >= 0 && nt >= 2) {
            if (ci_eq(toks[0], "type")) {
                snprintf(out[cur].type_name, sizeof(out[cur].type_name),
                         "%s", toks[1]);
            } else if (ci_eq(toks[0], "xPos")) {
                out[cur].x = (float)strtod(toks[1], NULL);
            } else if (ci_eq(toks[0], "yPos")) {
                out[cur].y = (float)strtod(toks[1], NULL);
            } else if (ci_eq(toks[0], "heading")) {
                out[cur].heading = (float)strtod(toks[1], NULL);
            } else if (ci_eq(toks[0], "health")) {
                out[cur].health = (int)strtod(toks[1], NULL);
            } else if (ci_eq(toks[0], "driving")) {
                snprintf(out[cur].driving, sizeof(out[cur].driving),
                         "%s", toks[1]);
            }
        }
        p = nl ? nl + 1 : p + l;
    }
    return n;
}

static void ol_sort(OlRec* a, int n)
{
    for (int i = 1; i < n; ++i) {
        OlRec t = a[i];
        int j = i - 1;
        while (j >= 0 && strcmp(a[j].name, t.name) > 0) {
            a[j + 1] = a[j];
            --j;
        }
        a[j + 1] = t;
    }
}

static uint32_t fnv1a32(uint32_t h, const char* s)
{
    for (int i = 0; s[i] != '\0'; ++i) {
        h ^= (uint8_t)s[i];
        h *= 16777619u;
    }
    return h;
}

static uint32_t ol_digest(OlRec* a, int n)
{
    uint32_t h = 2166136261u;
    for (int i = 0; i < n; ++i) {
        char buf[256];
        /* %g round-trips the same strtod float on both sides. */
        snprintf(buf, sizeof(buf), "%s|%s|%g|%g|%g|%d|%s", a[i].name,
                 a[i].type_name, a[i].x, a[i].y, a[i].heading, a[i].health,
                 a[i].driving);
        h = fnv1a32(h, buf);
    }
    return h;
}

/* The corpus registry (shipped Lego.cfg VehicleTypes/MiniFigureTypes/
 * RockMonsterTypes/BuildingTypes + FUN_0042e8b0 specials) — used by the
 * probe to decide which records the loader can place. */
static int ol_type_is_unknown(const char* name);

/* Filter records to the ones that become *placed entities*: TVCamera maps
 * to the follow camera (no entity — FUN_0042e020 type -1), and unregistered
 * type names (Test1…) are skipped by the port (the corpus aborts). Returns
 * the number of records kept; `*cameras`/`*unknown` tally the excluded
 * kinds, and the LAST camera record is captured in out_cam. */
static int ol_filter_mirrors(OlRec* a, int n, int* cameras, int* unknown,
                             OlRec* out_cam)
{
    int keep = 0;
    *cameras = 0;
    *unknown = 0;
    if (out_cam != NULL) {
        __builtin_memset(out_cam, 0, sizeof(*out_cam));
    }
    for (int i = 0; i < n; ++i) {
        if (ci_eq(a[i].type_name, "TVCamera")) {
            ++*cameras;
            if (out_cam != NULL) {
                *out_cam = a[i]; /* last one wins, like the loader */
            }
            continue;
        }
        /* unregistered names = not in the corpus ObjectTypeByName table —
         * mirror the loader's skip. */
        if (ol_type_is_unknown(a[i].type_name)) {
            ++*unknown;
            continue;
        }
        a[keep++] = a[i];
    }
    return keep;
}

static int ol_type_is_unknown(const char* name)
{
    if (name == NULL || name[0] == '\0') {
        return 1;
    }
    if (ci_eq(name, "TVCamera") || ci_eq(name, "PowerCrystal")
        || ci_eq(name, "Ore") || ci_eq(name, "ProcessedOre")
        || ci_eq(name, "Boulder") || ci_eq(name, "Dynamite")
        || ci_eq(name, "Barrier") || ci_eq(name, "ElectricFence")
        || ci_eq(name, "SpiderWeb") || ci_eq(name, "OohScary")
        || ci_eq(name, "Path") || ci_eq(name, "Pusher")
        || ci_eq(name, "Freezer") || ci_eq(name, "LaserShot")) {
        return 0;
    }
    static const char* const groups[][13] = {
        { "Hoverboard", "SmallDigger", "SmallTruck", "SmallCat", "SmallMLP",
          "SmallHeli", "BullDozer", "WalkerDigger", "LargeMLP",
          "LargeDigger", "LargeCat", "LargeHeli", NULL },
        { "Pilot", NULL },
        { "RockMonster", "TinyRM", "TinyIM", "SmallSpider", "Bat", "Slug",
          "LavaMonster", "IceMonster", NULL },
        { "Toolstation", "TeleportPad", "Docks", "Powerstation", "Barracks",
          "Upgrade", "Geo-dome", "OreRefinery", "Gunstation",
          "TeleportBIG", NULL },
    };
    for (size_t g = 0; g < 4; ++g) {
        for (size_t i = 0; groups[g][i] != NULL; ++i) {
            if (ci_eq(groups[g][i], name)) {
                return 0;
            }
        }
    }
    return 1;
}

/* Real-data Stats check (--real-stats <wad-dir>, finding 520): load the shipped
 * Lego.cfg Stats section through the startup loader, bind entities to their
 * per-level 0x150-byte records and read known values back through the
 * retail accessors. Values are the shipped Lego.cfg Stats entries; drill
 * and upgrade times are stored multiplied by 25 (reference ticks). */
static EntitySlot* stats_probe_bind(const char* name, uint32_t level)
{
    static EntitySlot slot;
    int type = 0, row = 0;
    memset(&slot, 0, sizeof(slot));
    if (!entity_type_row_for_name(name, &type, &row)) return NULL;
    slot.type = type;
    slot.etype = row;
    snprintf(slot.type_name, sizeof(slot.type_name), "%s", name);
    slot.task_state = AI_TASK_STATE_NONE;
    if (!entity_stats_record_bind(&slot) || slot.stats_record == NULL)
        return NULL;
    if (level != 0u && !entity_animation_state_set(&slot, level)) return NULL;
    return &slot;
}

static void stats_probe(const char* wad_dir)
{
    int mounted = resource_mount(wad_dir, "LegoRR");
    BlockFile* cfg = mounted > 0 ? block_open("Lego.cfg") : NULL;
    EntitySlot* e;
    CHECK(cfg != NULL, "--real: shipped Lego.cfg opens for the Stats probe");
    if (cfg == NULL) {
        resource_unmount_all();
        return;
    }
    entity_names_config_load(cfg);

    /* Pilot: RouteSpeed 1.1, SoilDrillTime 4.0 (+0x38 = 0x24 + 5*4),
     * EnergyDecayRate 0.25, AwarenessRange 1000, RepairValue 10:15:20:25. */
    e = stats_probe_bind("Pilot", 0u);
    CHECK(e != NULL, "--real: Pilot binds its level-0 Stats record");
    if (e != NULL) {
        CHECK(entity_stats_float_at20(e) == 1.1f, "--real: Pilot RouteSpeed");
        CHECK(entity_stats_float_at24_indexed(e, 5) == 4.0f * 25.0f,
              "--real: Pilot SoilDrillTime (x25)");
        CHECK(entity_energy_decay_rate_get(e) == 0.25f,
              "--real: Pilot EnergyDecayRate");
        CHECK(entity_collision_radius_get(e) == 1000.0f,
              "--real: Pilot AwarenessRange (+0xe4)");
        CHECK(entity_stats_float_at88(e) == 10.0f,
              "--real: Pilot level-0 RepairValue");
    }
    e = stats_probe_bind("Pilot", 2u);
    CHECK(e != NULL && entity_stats_float_at88(e) == 20.0f,
          "--real: Pilot level-2 RepairValue selects the third record");

    /* SmallDigger: RouteSpeed 2:2:4:4:..., HardDrillTime 180:150:... */
    e = stats_probe_bind("SmallDigger", 2u);
    CHECK(e != NULL && entity_stats_float_at20(e) == 4.0f,
          "--real: SmallDigger level-2 RouteSpeed");
    CHECK(e != NULL && entity_stats_float_at24_indexed(e, 2) == 180.0f * 25.0f,
          "--real: SmallDigger level-2 HardDrillTime (+0x2c, x25)");

    /* RockMonster weapons block: FreezerTime 25, PusherDist 20, LaserDamage
     * 110, AttackRadius 160, and CanFreeze|CanLaser|CanPush in +0x148. */
    e = stats_probe_bind("RockMonster", 0u);
    CHECK(e != NULL, "--real: RockMonster binds its Stats record");
    if (e != NULL) {
        CHECK(entity_stats_float_at10c(e) == 25.0f, "--real: FreezerTime");
        CHECK(entity_stats_float_at114(e) == 20.0f, "--real: PusherDist");
        CHECK(entity_stats_float_at11c(e) == 110.0f, "--real: LaserDamage");
        CHECK(entity_stats_float_at98(e) == 160.0f, "--real: AttackRadius");
        CHECK((entity_state_flags_get(e) & 0x07000000u) == 0x07000000u,
              "--real: CanFreeze/CanLaser/CanPush flags");
    }
    fprintf(stderr, "entity_test: --real Stats probe done\n");
    block_close(cfg);
    resource_unmount_all();
}

/* Real-data layout check (--real-layout <wad-dir>, findings 522 and 526):
 * create entities from the shipped Lego.cfg types and drive the members that
 * the 32-bit layout places at their original offsets (+0x28, +0x2f0..+0x308,
 * +0x330..+0x344, +0x36c, +0x3c0, +0x3e0..+0x3f0) through the retail readers
 * that read them. A layout move changes no native behaviour, so every reader
 * must keep its answer, and writing the moved members must leave the Stats
 * binding (+0x334) intact. */
static EntitySlot* layout_probe_create(GameContext* ctx, const char* name)
{
    int type = 0, row = 0;
    if (!entity_type_row_for_name(name, &type, &row)) return NULL;
    int index = entity_create(ctx, type, name, 0.0f, 0.0f, 0.0f, 0.0f);
    if (index < 0) return NULL;
    return (EntitySlot*)entity_at(ctx, type, index);
}

static void layout_probe_defaults(const EntitySlot* e, const char* name)
{
    char msg[96];
    snprintf(msg, sizeof msg, "--real-layout: %s binds its Stats record",
             name);
    CHECK(e != NULL && e->stats_record != NULL, msg);
    if (e == NULL) return;
    /* EntityCreateByType's +0x3e8 arms: 0x40100 for type 4 and the Pilot
     * scheduler mask 0x4481f5 for type 2. Later creation steps add bits (a
     * building's 0x20000000), so check that each arm's bits are present. */
    const uint32_t want = e->type == ENTITY_TYPE_BUILDING ? 0x00040100u
                        : e->type == ENTITY_TYPE_MINIFIG  ? 0x004481f5u
                                                          : 0u;
    if (want == 0u) return;
    snprintf(msg, sizeof msg, "--real-layout: %s +0x3e8 creation word", name);
    CHECK((e->flags_3e8 & want) == want, msg);
}

/* Finding 526: +0x28 path count, the +0x2f0..+0x308 window, +0x330,
 * +0x344 and +0x3c0, each driven through a reader that the move aligned. */
static void layout_probe_window_2f0(GameContext* ctx, EntitySlot* building,
                                    EntitySlot* monster, EntitySlot* pilot)
{
    if (building == NULL || monster == NULL || pilot == NULL) return;

    /* EntityRouteAdjacentStepAppend (0x4468c0) reads and bumps +0x28. */
    pilot->flags_3e0 |= 1u;
    pilot->wp_count = 1;
    pilot->wp_index = 0;
    ENTITY_WAYPOINTS(pilot)[0].x = 5.0f;
    ENTITY_WAYPOINTS(pilot)[0].y = 5.0f;
    CHECK(entity_route_adjacent_step_append(pilot, 6, 5) == 1
              && pilot->wp_count == 2
              && ENTITY_WAYPOINTS(pilot)[1].x == 6.0f,
          "--real-layout: adjacent step appends at +0x28");
    CHECK(entity_route_adjacent_step_append(pilot, 9, 9) == 0
              && pilot->wp_count == 2,
          "--real-layout: non-adjacent step leaves +0x28");
    pilot->wp_count = 0;
    pilot->flags_3e0 &= ~1u;

    /* CmdCreateTarget (0x4036c0) installs the +0x2f0 chain head. */
    CHECK(pilot->cmd_head == NULL && cmd_chain_walk(pilot) == NULL,
          "--real-layout: new Pilot has no +0x2f0 command chain");
    cmd_create_target(pilot, monster);
    CHECK(pilot->cmd_head != NULL && cmd_chain_walk(pilot) != NULL,
          "--real-layout: target command lands at +0x2f0");

    /* EntityProgressAdvanceBy25 (0x447730): +0x344 against the +0x340
     * bound, clearing the +0x2fc link. */
    pilot->progress_344 = 0.0f;
    pilot->field_340 = 40.0f;
    pilot->link_2fc = (uintptr_t)monster;
    CHECK(entity_progress_advance_by_25(pilot) == 0
              && pilot->progress_344 == 25.0f && pilot->link_2fc == 0u,
          "--real-layout: +0x344 advances and +0x2fc clears");
    CHECK(entity_progress_advance_by_25(pilot) == 1
              && pilot->progress_344 == 40.0f,
          "--real-layout: +0x344 clamps to +0x340");
    pilot->progress_344 = 0.0f;
    pilot->field_340 = 0.0f;

    /* LinkedEntityRouteFlagClear (0x43f100) follows +0x2fc. */
    pilot->link_2fc = (uintptr_t)monster;
    pilot->flags_3ec = 0x40u;
    monster->flags_3ec = 0x42u;
    entity_linked_route_flag_clear(ctx, pilot);
    CHECK(pilot->flags_3ec == 0u && monster->flags_3ec == 2u,
          "--real-layout: +0x2fc link has its route bit cleared");
    pilot->link_2fc = 0u;
    monster->flags_3ec = 0u;

    /* SelectionCandidateFilterActive (0x4526e0) tests the +0x304 object in
     * place of the candidate: an ineligible one rejects the Toolstation. */
    building->field_340 = 100.0f;
    CHECK(selection_candidate_eligibility_check(building) == 1,
          "--real-layout: Toolstation eligible before the +0x304 link");
    monster->flags_3e8 &= ~0x100u;
    building->attach_304 = monster;
    CHECK(selection_candidate_filter_active(building, 1, 0) == 0,
          "--real-layout: +0x304 object replaces the candidate");
    building->attach_304 = NULL;

    /* EntityClearNestedState8Fields (0x4487c0): the +0x308 carried type-8
     * object has its +0x2f4/+0x2f8 pair cleared. */
    static EntitySlot nested;
    memset(&nested, 0, sizeof nested);
    nested.type = 8;
    nested.pos_pair_2f4 = -1.0f;
    nested.pos_pair_2f8 = -1.0f;
    pilot->carry_link_308 = &nested;
    entity_clear_nested_state_8_fields(pilot);
    CHECK(nested.pos_pair_2f4 == -1.0f && nested.pos_pair_2f8 == -1.0f,
          "--real-layout: +0x308 pair untouched without +0x3e0 bit 0x400");
    pilot->flags_3e0 |= 0x400u;
    entity_clear_nested_state_8_fields(pilot);
    CHECK(nested.pos_pair_2f4 == 0.0f && nested.pos_pair_2f8 == 0.0f,
          "--real-layout: +0x308 carried object's +0x2f4/+0x2f8 cleared");
    pilot->flags_3e0 &= ~0x400u;
    pilot->carry_link_308 = NULL;

    /* EntityAnimationStateSet (0x439cd0 -> 0x469c10) writes +0x330 and
     * selects that Stats row (0x150 bytes per level). */
    const uint8_t* level0 = (const uint8_t*)building->stats_record;
    CHECK(building->indexed_record_slot_330 == 0 && level0 != NULL,
          "--real-layout: Toolstation starts at +0x330 level 0");
    CHECK(entity_animation_state_set(building, 1u) == 1
              && building->indexed_record_slot_330 == 1
              && (const uint8_t*)building->stats_record == level0 + 0x150,
          "--real-layout: +0x330 level 1 selects the next Stats row");
    CHECK(entity_animation_state_set(building, 0u) == 1
              && building->indexed_record_slot_330 == 0
              && building->stats_record == level0,
          "--real-layout: +0x330 level 0 restores the Stats row");

    /* EntityDirectionalActivityStart (0x447950) redirects through the
     * +0x3c0 pair link while +0x3e4 bit 2 is set. */
    pilot->flags_3e4 |= 2u;
    pilot->pair_link_3c0 = (uintptr_t)monster;
    pilot->flags_3e0 &= ~0x1000u;
    monster->flags_3e0 &= ~0x1000u;
    monster->field_340 = 100.0f;
    entity_directional_activity_start(pilot, NULL, 0);
    CHECK((monster->flags_3e0 & 0x1000u) != 0u
              && (pilot->flags_3e0 & 0x1000u) == 0u,
          "--real-layout: +0x3c0 pair link receives the activity");
    pilot->flags_3e4 &= ~2u;
    pilot->pair_link_3c0 = 0u;

    /* Every placed member holds distinct storage beside the Stats binding. */
    const void* record = pilot->stats_record;
    pilot->wp_count = 0x28;
    pilot->pos_pair_2f4 = 2.0f;
    pilot->pos_pair_2f8 = 3.0f;
    pilot->link_2fc = 0x2fcu;
    pilot->attach_304 = building;
    pilot->carry_link_308 = monster;
    pilot->indexed_record_slot_330 = 0x330;
    pilot->progress_344 = 4.0f;
    pilot->pair_link_3c0 = 0x3c0u;
    CHECK(pilot->stats_record == record && entity_stats_float_at20(pilot) == 1.1f
              && pilot->cmd_head != NULL && pilot->wp_count == 0x28
              && pilot->pos_pair_2f4 == 2.0f && pilot->pos_pair_2f8 == 3.0f
              && pilot->link_2fc == 0x2fcu && pilot->attach_304 == building
              && pilot->carry_link_308 == monster
              && pilot->indexed_record_slot_330 == 0x330
              && pilot->progress_344 == 4.0f && pilot->pair_link_3c0 == 0x3c0u,
          "--real-layout: finding 526 members hold distinct storage");
    pilot->wp_count = 0;
    pilot->attach_304 = NULL;
    pilot->carry_link_308 = NULL;
    pilot->link_2fc = 0u;
    pilot->pair_link_3c0 = 0u;
    pilot->indexed_record_slot_330 = 0;
}

static void layout_probe(const char* wad_dir)
{
    int mounted = resource_mount(wad_dir, "LegoRR");
    BlockFile* cfg = mounted > 0 ? block_open("Lego.cfg") : NULL;
    CHECK(cfg != NULL, "--real-layout: shipped Lego.cfg opens");
    if (cfg == NULL) {
        resource_unmount_all();
        return;
    }
    entity_names_config_load(cfg);
    static GameContext ctx;
    game_context_init(&ctx);

    EntitySlot* building = layout_probe_create(&ctx, "Toolstation");
    EntitySlot* monster = layout_probe_create(&ctx, "RockMonster");
    EntitySlot* pilot = layout_probe_create(&ctx, "Pilot");
    layout_probe_defaults(building, "Toolstation");
    layout_probe_defaults(monster, "RockMonster");
    layout_probe_defaults(pilot, "Pilot");

    /* SelectionCandidateEligibilityCheck (0x452870): +0x3e8 bit 0x100 from
     * the type-4 creation word, then the shared gate (+0x3e0, +0x3e4,
     * +0x340) and the +0x3e4/+0x3e0 clear-bit band. */
    if (building != NULL) {
        /* EntityCreateByType seeds +0x340 with 100.0 (0x42c80000). */
        building->field_340 = 100.0f;
        CHECK(selection_candidate_eligibility_check(building) == 1,
              "--real-layout: Toolstation is a selection candidate");
        building->field_340 = -1.0f;
        CHECK(selection_candidate_eligibility_check(building) == 0,
              "--real-layout: negative +0x340 rejects the candidate");
        building->field_340 = 100.0f;
        building->flags_3e0 = 0x04000000u;
        CHECK(selection_candidate_eligibility_check(building) == 0,
              "--real-layout: +0x3e0 bit 0x04000000 rejects the candidate");
        building->flags_3e0 = 0u;
        building->flags_3e4 = 0x2u;
        CHECK(selection_candidate_eligibility_check(building) == 0,
              "--real-layout: +0x3e4 bit 2 rejects the candidate");
        building->flags_3e4 = 0u;
        building->flags_3e8 &= ~0x100u;
        CHECK(selection_candidate_eligibility_check(building) == 0,
              "--real-layout: +0x3e8 bit 0x100 is required");
        building->flags_3e8 |= 0x100u;
        CHECK(selection_candidate_eligibility_check(building) == 1,
              "--real-layout: Toolstation eligible again once restored");
    }

    /* 0x4436f0 (+0x3e8), 0x43baf0 (+0x3e0/+0x3e4) and 0x452690 (+0x3ec). */
    if (monster != NULL) {
        monster->flags_3e0 = 0u;
        monster->flags_3e4 = 0u;
        monster->flags_3e8 &= ~0x04000000u;
        monster->flags_3ec = 0u;
        CHECK(entity_type3_flag04000000_test(monster) == 0,
              "--real-layout: RockMonster +0x3e8 bit 0x04000000 clear");
        monster->flags_3e8 |= 0x04000000u;
        CHECK(entity_type3_flag04000000_test(monster) == 1,
              "--real-layout: RockMonster +0x3e8 bit 0x04000000 set");
        CHECK(entity_flags_mask107c33de_or_1c00203_any_test(monster) == 0,
              "--real-layout: RockMonster flag masks clear");
        monster->flags_3e4 = 0x00000200u;
        CHECK(entity_flags_mask107c33de_or_1c00203_any_test(monster) == 1,
              "--real-layout: +0x3e4 mask bit seen");
        monster->flags_3e4 = 0u;
        monster->flags_3e0 = 0x10000000u;
        CHECK(entity_flags_mask107c33de_or_1c00203_any_test(monster) == 1,
              "--real-layout: +0x3e0 mask bit seen");
        monster->flags_3e0 = 0u;
        CHECK(selection_active_candidate_already_selected_or_marked(monster)
                  == 0,
              "--real-layout: unselected RockMonster is not marked");
        monster->flags_3ec = 2u;
        CHECK(selection_active_candidate_already_selected_or_marked(monster)
                  == 1,
              "--real-layout: +0x3ec bit 2 marks the candidate");
        monster->flags_3ec = 0u;
    }

    /* EntityFlag40000ThresholdCompare (0x437cc0): Stats flag 0x40000 and
     * PainThreshold (+0xe8) against the entity's +0x340. */
    EntitySlot* probes[3] = { building, monster, pilot };
    for (int i = 0; i < 3; ++i) {
        EntitySlot* e = probes[i];
        if (e == NULL) continue;
        const int shot = (entity_state_flags_get(e) & 0x40000u) != 0u;
        e->field_340 = entity_stats_float_ate8(e) + 1.0f;
        CHECK(entity_flag40000_threshold_compare(e) == shot,
              "--real-layout: +0x340 above PainThreshold follows flag 0x40000");
        e->field_340 = entity_stats_float_ate8(e) - 1.0f;
        CHECK(entity_flag40000_threshold_compare(e) == 0,
              "--real-layout: +0x340 below PainThreshold never compares");
        e->field_340 = 0.0f;
    }

    /* Writing every moved member leaves the Stats binding and each other
     * member intact (no overlap on any host). */
    if (pilot != NULL) {
        const void* record = pilot->stats_record;
        const float route_speed = entity_stats_float_at20(pilot);
        pilot->field_33c = 33.0f;
        pilot->field_340 = 34.0f;
        pilot->owner_36c = monster;
        pilot->flags_3e0 = 0x3e0u;
        pilot->flags_3e4 = 0x3e4u;
        pilot->flags_3e8 = 0x3e8u;
        pilot->flags_3ec = 0x3ecu;
        pilot->flags_3f0 = 0x3f0u;
        CHECK(pilot->stats_record == record
                  && entity_stats_float_at20(pilot) == route_speed
                  && route_speed == 1.1f,
              "--real-layout: moved members leave the Pilot Stats binding");
        CHECK(pilot->field_33c == 33.0f && pilot->field_340 == 34.0f
                  && pilot->owner_36c == monster
                  && pilot->flags_3e0 == 0x3e0u && pilot->flags_3e4 == 0x3e4u
                  && pilot->flags_3e8 == 0x3e8u && pilot->flags_3ec == 0x3ecu
                  && pilot->flags_3f0 == 0x3f0u,
              "--real-layout: moved members hold distinct storage");
        pilot->field_33c = 0.0f;
        pilot->field_340 = 0.0f;
        pilot->owner_36c = NULL;
        pilot->flags_3e0 = 0u;
        pilot->flags_3e4 = 0u;
        pilot->flags_3e8 = 0x004481f5u;
        pilot->flags_3ec = 0u;
        pilot->flags_3f0 = 0u;
    }
    layout_probe_window_2f0(&ctx, building, monster, pilot);
    fprintf(stderr, "entity_test: --real layout probe done\n");
    block_close(cfg);
    resource_unmount_all();
}

static int ol_probe(const char* wad_dir)
{
    int mounted = resource_mount(wad_dir, "LegoRR");
    BlockFile* cfg = mounted > 0 ? block_open("Lego.cfg") : NULL;
    if (cfg == NULL) {
        fprintf(stderr,
                "entity_test: --real probe skipped (no Lego.cfg from %s)\n",
                wad_dir);
        resource_unmount_all();
        return 0;
    }

    /* 1. Full-corpus grammar check: every `.ol` entry in the mounted wads
     *    is parsed independently and placed through the loader; count +
     *    per-object FNV digest must be identical. The corpus ships 44 .ol
     *    entries, only 34 of which the shipped Lego.cfg references (the
     *    other 10 are dev/test leftovers — TestLevels/Paul*.ol, Test.ol,
     *    GameLevels/Level25/Copy of 25.ol, TutorialLevels/GeoTuto_01/G01.ol
     *    — never named by an OListFile, so the engine never loads them
     *    either; the probe still validates their grammar). */
    int files = 0, objects = 0, mismatches = 0, total_unknown = 0;
    int total_cameras = 0;
    for (int w = 0; w < resource_mounted_count(); ++w) {
        uint32_t entries = resource_wad_entry_count(w);
        for (uint32_t e = 0; e < entries; ++e) {
            const char* ename = resource_wad_entry_name(w, e);
            if (ename == NULL) {
                continue;
            }
            size_t elen = strlen(ename);
            if (elen < 4 || ename[elen - 3] != '.'
                || lc((int)(unsigned char)ename[elen - 2]) != 'o'
                || lc((int)(unsigned char)ename[elen - 1]) != 'l') {
                continue; /* not a `.ol` entry */
            }

            Resource* r = resource_open(ename, RESOURCE_AUTO);
            if (r == NULL || resource_read(r) <= 0) {
                fprintf(stderr, "entity_test: .ol not readable: %s\n", ename);
                ++mismatches;
                if (r != NULL) {
                    resource_free(r);
                }
                continue;
            }
            char* text = (char*)malloc((size_t)r->size + 1);
            if (text == NULL) {
                resource_free(r);
                ++mismatches;
                continue;
            }
            memcpy(text, r->data, (size_t)r->size);
            text[r->size] = '\0';
            resource_free(r);

            OlRec want[OL_PROBE_MAX];
            int wn = ol_parse_indep(text, want, OL_PROBE_MAX);
            int cameras = 0, unknown = 0;
            OlRec want_cam;
            int wn2 = ol_filter_mirrors(want, wn, &cameras, &unknown,
                                        &want_cam);
            ol_sort(want, wn2);

            static GameContext ctx;
            game_context_init(&ctx);
            ctx.cfg = cfg;
            int placed = object_list_load_file(&ctx, ename);
            (void)placed; /* compared via tables below */

            /* The placed-entity records: types 1..4 map to the four typed
             * tables, 5..19 to the shared special table (table index 4). */
            OlRec got[OL_PROBE_MAX];
            int gn = 0;
            for (int t = ENTITY_TYPE_VEHICLE; t <= ENTITY_TYPE_SPECIAL; ++t) {
                for (int k = 0; k < entity_count(&ctx, t); ++k) {
                    const EntitySlot* s = entity_at(&ctx, t, k);
                    if (s == NULL || gn >= OL_PROBE_MAX) {
                        continue;
                    }
                    snprintf(got[gn].name, sizeof(got[gn].name), "%s", s->name);
                    snprintf(got[gn].type_name, sizeof(got[gn].type_name),
                             "%s", s->type_name);
                    got[gn].x = s->x;
                    got[gn].y = s->y;
                    got[gn].heading = s->rot;
                    got[gn].health = s->health;
                    snprintf(got[gn].driving, sizeof(got[gn].driving),
                             "%s", s->driving);
                    ++gn;
                }
            }
            ol_sort(got, gn);

            /* The loader counts camera records in `placed` but places no
             * entity for them (FUN_0042e020 type -1); the digest is over
             * the entity records only. */
            int ok = gn == wn2
                     && ol_digest(got, gn) == ol_digest(want, wn2);
            if (cameras > 0) {
                ok = ok && ctx.ol_camera_set == 1
                     && ctx.ol_camera_x == want_cam.x
                     && ctx.ol_camera_y == want_cam.y
                     && ctx.ol_camera_heading == want_cam.heading;
            } else {
                ok = ok && ctx.ol_camera_set == 0;
            }
            if (!ok) {
                ++mismatches;
                fprintf(stderr,
                        "entity_test: %s: .ol mismatch (entities got=%d "
                        "want=%d cameras=%d unknown=%d)\n",
                        ename, gn, wn2, cameras, unknown);
                for (int d = 0; d < wn2 && d < 3; ++d) {
                    fprintf(stderr,
                            "  want %-16s type=%-16s x=%.4f y=%.4f h=%.4f "
                            "hp=%d d=%s\n",
                            want[d].name, want[d].type_name, want[d].x,
                            want[d].y, want[d].heading, want[d].health,
                            want[d].driving);
                }
            }
            /* unregistered type names may only exist in the orphaned dev
             * files (Test1…); none in the 34 cfg-referenced lists. */
            total_unknown += unknown;
            total_cameras += cameras;
            ++files;
            objects += wn;
            free(text);
        }
    }

    /* 2. Config wiring: every OListFile the shipped Lego.cfg names must
     *    resolve through the level wrapper too. */
    int wired = 0, wired_fail = 0;
    const char* root = util_root_name(cfg);
    for (size_t i = 0; i < block_record_count(cfg); ++i) {
        BlockRec* g = block_record_at(cfg, i);
        if (g == NULL) {
            continue;
        }
        char path[512];
        snprintf(path, sizeof(path), "%s::Levels::%s::OListFile", root,
                 block_name(g));
        const char* olist = block_get_strptr(cfg, path);
        if (olist == NULL) {
            snprintf(path, sizeof(path), "%s::%s::OListFile", root,
                     block_name(g));
            olist = block_get_strptr(cfg, path);
        }
        if (olist == NULL) {
            continue;
        }
        static GameContext ctx;
        game_context_init(&ctx);
        ctx.cfg = cfg;
        int placed = object_list_load(&ctx, cfg, block_name(g));
        int tables = 0;
        for (int t = ENTITY_TYPE_VEHICLE; t <= ENTITY_TYPE_SPECIAL; ++t) {
            tables += entity_count(&ctx, t);
        }
        /* `placed` counts camera records too, so it is >= the table count;
         * a level's camera count is bounded by its few TVCameras. */
        if (placed < 0 || placed < tables || placed - tables > 2) {
            ++wired_fail;
            fprintf(stderr, "entity_test: %s -> %s wired load mismatch "
                            "(placed=%d tables=%d)\n",
                    block_name(g), olist, placed, tables);
        }
        ++wired;
    }

    block_close(cfg);
    resource_unmount_all();

    CHECK(files == 44, "all 44 shipped .ol entries cross-checked");
    CHECK(mismatches == 0, "every .ol grammar/placement digest-identical");
    CHECK(wired_fail == 0, "every referenced OListFile wired (>= 34 levels)");
    CHECK(total_unknown == 90, "the 90 Test1 records are all unregistered "
                               "(dev orphans — mirror of the corpus abort)");
    fprintf(stderr, "entity_test: --real probe: %d corpus .ol files, %d "
                    "object records, %d TVCameras (follow-camera), %d "
                    "unregistered skips (Test1…), %d cfg-wired loads, %s\n",
            files, objects, total_cameras, total_unknown, wired,
            mismatches == 0 && wired_fail == 0 ? "all match" : "MISMATCH");
    return files;
}

/* --- entity record accessors (the +0x334 stats record family) ----------- *
 * Drive every offset getter over a synthetic 0x150-byte record image and
 * check the read lands where the corpus reads it. */
static int stats_record_test(void)
{
    uint8_t rec[ENTITY_STATS_RECORD_SIZE];
    __builtin_memset(rec, 0, sizeof rec);
    for (int i = 0; i < ENTITY_STATS_RECORD_SIZE; ++i) {
        /* Small bytes: keeps the float high byte away from the 0x7F/0xFF
         * exponent patterns so `==` never hits a NaN bit pattern. */
        rec[i] = (uint8_t)(1 + i % 100);
    }

    static GameContext ctx;
    game_context_init(&ctx);
    int idx = entity_create(&ctx, 1, "StatsProbe", 0.0f, 0.0f, 0.0f, 0.0f);
    CHECK(idx >= 0, "probe slot created");
    if (idx < 0) {
        return 1;
    }

    /* Zero-template fallback: an unselected record reads zero everywhere. */
    CHECK(entity_stats_float_at20(&ctx.entity[0][idx]) == 0.0f,
          "unset record reads zero float");
    CHECK(entity_stats_word_get(&ctx.entity[0][idx]) == 0u,
          "unset record reads zero word");
    CHECK(entity_state_flags_get(&ctx.entity[0][idx]) == 0u,
          "unset record reads zero state flags");

    EntitySlot* e = &ctx.entity[0][idx];
    e->stats_record = rec;

    /* Float fields: rec[off] is i%256 at offset off, so the expected float
     * is the raw pattern little-endian at that offset. */
#define FLOAT_AT(fn, off)                                                      \
    do {                                                                       \
        float want, got;                                                       \
        __builtin_memcpy(&want, rec + (off), 4);                               \
        got = (fn)(e);                                                         \
        CHECK(got == want, #fn " reads record offset");                        \
    } while (0)

    FLOAT_AT(entity_stats_float_at20, 0x20);
    FLOAT_AT(entity_stats_float_atd4, 0xd4);
    FLOAT_AT(entity_stats_float_at9c, 0x9c);
    FLOAT_AT(entity_stats_float_atd8, 0xd8);
    FLOAT_AT(entity_stats_float_at6c, 0x6c);
    FLOAT_AT(entity_stats_float_at74, 0x74);
    FLOAT_AT(entity_stats_float_at78, 0x78);
    FLOAT_AT(entity_stats_float_ate8, 0xe8);
    FLOAT_AT(entity_stats_float_at70, 0x70);
    FLOAT_AT(entity_stats_float_at7c, 0x7c);
    FLOAT_AT(entity_stats_float_at80, 0x80);
    FLOAT_AT(entity_energy_decay_rate_get, 0x84);
    FLOAT_AT(entity_stats_float_atdc, 0xdc);
    FLOAT_AT(entity_stats_float_at88, 0x88);
    FLOAT_AT(entity_stats_float_at8c, 0x8c);
    FLOAT_AT(entity_stats_float_at90, 0x90);
    FLOAT_AT(entity_stats_float_at98, 0x98);
    FLOAT_AT(entity_stats_float_at94, 0x94);
    FLOAT_AT(entity_stats_float_atf4, 0xf4);
    FLOAT_AT(entity_speed_component_get, 0xf8);
    FLOAT_AT(entity_stats_float_at130, 0x130);
    FLOAT_AT(entity_stats_float_at134, 0x134);
    FLOAT_AT(entity_stats_float_at120, 0x120);
    FLOAT_AT(entity_stats_float_at128, 0x128);
    FLOAT_AT(entity_stats_float_at124, 0x124);
    FLOAT_AT(entity_stats_float_at12c, 0x12c);
    FLOAT_AT(entity_stats_float_at138, 0x138);
    FLOAT_AT(entity_collision_radius_get, 0xe4);
    FLOAT_AT(entity_stats_float_at114, 0x114);
    FLOAT_AT(entity_stats_float_at118, 0x118);
    FLOAT_AT(entity_stats_float_at11c, 0x11c);
    FLOAT_AT(entity_stats_float_at110, 0x110);
    FLOAT_AT(entity_stats_float_at10c, 0x10c);

    /* Indexed terrain-speed floats: +24 + index*4. */
    {
        float want2, want5, want;
        __builtin_memcpy(&want2, rec + 0x24 + 2 * 4, 4);
        __builtin_memcpy(&want5, rec + 0x24 + 5 * 4, 4);
        want = entity_stats_float_at24_indexed(e, 2);
        CHECK(want == want2, "indexed speed reads +24+index*4 (2)");
        want = entity_stats_float_at24_indexed(e, 5);
        CHECK(want == want5, "indexed speed reads +24+index*4 (5)");
    }

    /* Word fields. */
    uint32_t w;
#define WORD_AT(fn, off, ...)                                                  \
    do {                                                                       \
        __builtin_memcpy(&w, rec + (off), 4);                                  \
        CHECK((fn)(__VA_ARGS__) == w, #fn " reads record offset");             \
    } while (0)
    WORD_AT(entity_stats_word_atac, 0xac, e);
    WORD_AT(entity_stats_word_get, 0x144, e);
    WORD_AT(entity_state_flags_get, 0x148, e);
    WORD_AT(entity_capability_flags_get, 0x14c, e);
    WORD_AT(entity_stats_word_at13c, 0x13c, e);

    /* Selector pair: 0 -> +c0, nonzero -> +c4. */
    __builtin_memcpy(&w, rec + 0xc0, 4);
    CHECK(entity_stats_word_at_c0_or_c4(e, 0) == w, "selector 0 reads +c0");
    __builtin_memcpy(&w, rec + 0xc4, 4);
    CHECK(entity_stats_word_at_c0_or_c4(e, 1) == w, "selector 1 reads +c4");

    /* The address getter hands back record+0xa0 itself. */
    CHECK(entity_stats_field_at_a0(e) == (void*)(rec + 0xa0),
          "the field getter returns the record address");

    /* Clear back to the zero template and check both flag words. */
    e->stats_record = NULL;
    CHECK(entity_stats_word_get(e) == 0u && entity_stats_field_at_a0(e) != NULL,
          "unset record falls back to the zero template");

    ctx.cfg = NULL; /* no resources owned by this probe */
    return 0;
}

/* Eligibility predicates over the flag mirrors (FUN_004385b0 /
 * FUN_0043baf0 / FUN_004436f0) — stack EntitySlot probes. */
static void eligibility_test(void)
{
    EntitySlot e;
    memset(&e, 0, sizeof(e));
    e.type = 3;

    /* Mask-any test: the +3e0 and +3e4 masks are disjoint from zero. */
    CHECK(entity_flags_mask107c33de_or_1c00203_any_test(&e) == false,
          "clean flags mask nothing");
    e.flags_3e0 = 0x00000100u; /* inside 0x107c33de */
    CHECK(entity_flags_mask107c33de_or_1c00203_any_test(&e) == true,
          "the +3e0 mask bites");
    e.flags_3e0 = 0u;
    e.flags_3e4 = 0x01c00203u; /* the exact +3e4 mask */
    CHECK(entity_flags_mask107c33de_or_1c00203_any_test(&e) == true,
          "the +3e4 mask bites");
    e.flags_3e4 = 0u;

    /* Shared eligibility: the clean entity is eligible only through the
     * relaxed form — the strict form needs the +3e8 pair. */
    e.field_340 = 100.0f;
    CHECK(entity_shared_eligibility_check(&e, 1) == 1,
          "relaxed form passes a clean entity");
    CHECK(entity_shared_eligibility_check(&e, 0) == 0,
          "strict form needs the +3e8 bit pair");
    e.flags_3e8 = 0x20000000u;
    CHECK(entity_shared_eligibility_check(&e, 0) == 1,
          "the +3e8 pair opens the strict form");
    e.flags_3e8 |= 0x80000000u;
    CHECK(entity_shared_eligibility_check(&e, 0) == 0,
          "the high bit closes the strict form again");
    e.flags_3e8 = 0u;

    /* Each gate independently. */
    e.flags_3e0 = 0x00000100u; /* bit in 0x700100 */
    CHECK(entity_shared_eligibility_check(&e, 1) == 0,
          "the +3e0 mask 0x700100 gates");
    e.flags_3e0 = 0x00020000u;
    CHECK(entity_shared_eligibility_check(&e, 1) == 0,
          "+3e0 bit 0x20000 gates");
    e.flags_3e0 = 0u;
    e.flags_3e4 = 0x00100000u;
    CHECK(entity_shared_eligibility_check(&e, 1) == 0,
          "+3e4 bit 0x100000 gates");
    e.flags_3e4 = 0u;
    e.field_340 = -0.001f;
    CHECK(entity_shared_eligibility_check(&e, 1) == 0,
          "a negative +340 float gates");
    e.field_340 = 0.0f;
    CHECK(entity_shared_eligibility_check(&e, 1) == 1,
          "zero is not negative");

    /* Kind-3 test. */
    CHECK(entity_type3_flag04000000_test(&e) == 0,
          "kind 3 without the bit is false");
    e.flags_3e8 = 0x04000000u;
    CHECK(entity_type3_flag04000000_test(&e) == 1,
          "kind 3 with the bit is true");
    e.type = 6;
    CHECK(entity_type3_flag04000000_test(&e) == 0,
          "the wrong kind is false with the bit set");
}

/* Routing-progress advance (FUN_00447730) — stack EntitySlot probe. */
static void progress_advance_test(void)
{
    EntitySlot e;
    memset(&e, 0, sizeof(e));

    /* Clamping bound: +0x340. */
    e.field_340 = 40.0f;
    e.progress_344 = 10.0f;
    e.link_2fc = 0xcafebabeu;
    CHECK(entity_progress_advance_by_25(&e) == 0, "under the bound stays put");
    CHECK(e.progress_344 == 35.0f, "one step adds 25");
    CHECK(e.link_2fc == 0u, "the advance clears the link field");

    CHECK(entity_progress_advance_by_25(&e) == 1, "crossing the bound reports");
    CHECK(e.progress_344 == 40.0f, "the value clamps to the +0x340 bound");

    /* The step itself advances: already at the bound, adding 25 still
     * clamps right back and reports. */
    CHECK(entity_progress_advance_by_25(&e) == 1, "bound is sticky");
    CHECK(e.progress_344 == 40.0f, "the clamp is idempotent");

    /* Zero bound: any step reports and pins at zero. */
    memset(&e, 0, sizeof(e));
    e.progress_344 = 7.0f;
    CHECK(entity_progress_advance_by_25(&e) == 1, "zero bound crosses");
    CHECK(e.progress_344 == 0.0f, "zero bound pins the progress");
}

static int stats_flag_test(void)
{
    /* EntityStatsWordFlag10/20/40Test (2026-10-24): the bit tests mask the
     * stats reader's word exactly. */
    uint8_t rec[ENTITY_STATS_RECORD_SIZE];
    memset(rec, 0, sizeof rec);
    static GameContext ctx;
    game_context_init(&ctx);
    int a = entity_create(&ctx, 1, "FlagA", 0.0f, 0.0f, 0.0f, 0.0f);
    int b = entity_create(&ctx, 1, "FlagB", 0.0f, 0.0f, 0.0f, 0.0f);
    if (a < 0 || b < 0) {
        CHECK(0, "probe slots created");
        return 1;
    }
    EntitySlot* e = &ctx.entity[0][a];
    e->stats_record = rec;
    CHECK(entity_stats_flag10_test(e) == 0u
              && entity_stats_flag20_test(e) == 0u
              && entity_stats_flag40_test(e) == 0u,
          "a clear record answers no bits");
    const uint32_t w1 = 0x133u;
    memcpy(rec + 0x144, &w1, 4);
    CHECK(entity_stats_flag10_test(e) == 0x10u, "bit 0x10 comes masked");
    CHECK(entity_stats_flag20_test(e) == 0x20u, "bit 0x20 comes masked");
    CHECK(entity_stats_flag40_test(e) == 0u, "a clear bit 0x40 answers 0");
    const uint32_t w2 = 0x40u;
    memcpy(rec + 0x144, &w2, 4);
    CHECK(entity_stats_flag40_test(e) == 0x40u, "bit 0x40 comes masked");
    CHECK(entity_stats_flag10_test(e) == 0u, "the other bits stay clear");
    EntitySlot* n = &ctx.entity[0][b];
    CHECK(entity_stats_flag10_test(n) == 0u
              && entity_stats_flag20_test(n) == 0u
              && entity_stats_flag40_test(n) == 0u,
          "an unset record answers no bits");
    return 0;
}

/* CatalogCallbackByIdDispatch recorder: books the callback arguments so
 * the lanes' (kind, lane, row) shapes pin; optional early stop (1-based
 * call index) and one-shot band-count grow (the corpus's mid-sweep hop
 * re-read) ride as hooks. */
static int s_disp_calls;
static int s_disp_stop_at;
static int s_disp_grow;
static int s_disp_kind[8];
static int s_disp_lane[8];
static int s_disp_row[8];

static int disp_recorder(int kind, int lane, int row, void* payload)
{
    (void)payload;
    int n = s_disp_calls++;
    if (n < 8) {
        s_disp_kind[n] = kind;
        s_disp_lane[n] = lane;
        s_disp_row[n] = row;
    }
    if (s_disp_grow != 0) {
        resource_handle_set_band_word_install(s_disp_grow, 2u);
        s_disp_grow = 0;
    }
    return s_disp_stop_at != 0 && n + 1 == s_disp_stop_at ? 1 : 0;
}

int main(int argc, char** argv)
{
    /* The Stats probe loads the full shipped config into process-wide
     * tables, so it runs alone (ctest: entity_stats_real). */
    if (argc >= 3 && strcmp(argv[1], "--real-stats") == 0) {
        stats_probe(argv[2]);
        if (s_failures != 0) {
            fprintf(stderr, "entity_test: %d failure(s)\n", s_failures);
            return 1;
        }
        printf("entity_test: real Stats pass\n");
        return 0;
    }
    /* Real-data layout probe (ctest: entity_layout_real, finding 522). */
    if (argc >= 3 && strcmp(argv[1], "--real-layout") == 0) {
        layout_probe(argv[2]);
        if (s_failures != 0) {
            fprintf(stderr, "entity_test: %d failure(s)\n", s_failures);
            return 1;
        }
        printf("entity_test: real layout pass\n");
        return 0;
    }
    char rmcmd[64];
    snprintf(rmcmd, sizeof(rmcmd), "rm -rf %s", FIX);
    system(rmcmd);
    mkdir(FIX, 0755);
    write_fixture("Lego.cfg",
        "Lego {\n"
        "  Level01 {\n"
        "    OListFile M01.ol\n"
        "  }\n"
        "}\n");
    write_fixture("M01.ol",
        "LegoOl {\n"
        "  Digger1 {\n"
        "    type    SmallDigger\n"
        "    xPos    12.5\n"
        "    yPos    19.5\n"
        "    heading 45\n"
        "  }\n"
        "  WallA {\n"
        "    type    Toolstation\n"
        "    xPos    12.500000\n"
        "    yPos    13.500000\n"
        "    heading 90\n"
        "  }\n"
        "  MinifigA {\n"
        "    type    Pilot\n"
        "    xPos    11.5\n"
        "    yPos    14.0\n"
        "    heading 0\n"
        "    driving Digger1\n"
        "    health  150\n"
        "    Model   Data\\Tools\\dig.lwo\n"
        "  }\n"
        "  Cam {\n"
        "    type    TVCamera\n"
        "    xPos    5\n"
        "    yPos    6\n"
        "    heading 30\n"
        "  }\n"
        "}\n");
    write_fixture("M02.ol",
        "LegoOl {\n"
        "  Digger1 {\n"
        "    type    SmallDigger\n"
        "    xPos    12.5\n"
        "    yPos    19.5\n"
        "    heading 45\n"
        "  }\n"
        "  UnknownX {\n"
        "    type    Test1\n"
        "    xPos    1\n"
        "    yPos    2\n"
        "    heading 0\n"
        "  }\n"
        "  MinifigA {\n"
        "    type    Pilot\n"
        "    xPos    11.5\n"
        "    yPos    14.0\n"
        "    heading 0\n"
        "  }\n"
        "}\n");

    if (chdir(FIX) != 0) {
        fprintf(stderr, "chdir failed\n");
        return 1;
    }
    /* File opens resolve under the CD root; the fixture is the CWD. */
    resource_set_cd_root(".");
    resource_set_cd_data_dir(".");
    fixture_roundtrip(); /* s_failures drives the exit code below */
    stats_record_test();
    stats_flag_test();
    eligibility_test();
    progress_advance_test();

    /* Back out of the fixture dir so the probe sees the shipped Lego.cfg
     * (and its wad mount) rather than the loose fixture. */
    chdir("..");

    for (int a = 1; a < argc; ++a) {
        if (strcmp(argv[a], "--real") == 0 && a + 1 < argc) {
            ol_probe(argv[a + 1]);
            ++a;
        }
    }

    /* The object pointer registry (2026-10-24): with no feeder landing, the
     * count stays zero and the slot lookup never fires. The faithful .ol
     * loader IS a feeder (it appends every created entity), so the fixture
     * load above left entries behind: clear them first. */
    {
        object_pointer_registry_reset();
        int32_t out = 0x5a5a5a5a;
        CHECK(object_pointer_registry_count_peek() == 0u,
              "the registry count starts zero");
        CHECK(object_pointer_registry_get_slot(0, &out) == 0,
              "the first slot never fires");
        CHECK(out == 0x5a5a5a5a, "the output stays untouched out of range");
        CHECK(object_pointer_registry_get_slot(0, NULL) == 0,
              "the NULL output is safe (deviation)");
        object_pointer_registry_clear_match(1234);
        CHECK(object_pointer_registry_count_peek() == 0u,
              "the clear-match is a no-op with no entries");
    }

    /* The context-selected globals (2026-10-24): zero-pair start, the
     * mode-selected read, and the selection/owner predicate via the
     * media models. */
    {
        CHECK(context_selected_pointer_pair_peek(0) == 0u
                  && context_selected_pointer_pair_peek(1) == 0u,
              "the pair starts zero (no feeder)");
        context_selected_pointer_pair_set(0xAAu, 0xBBu);
        /* Mode 1 (media default? no: media mode is 0 until fed) — first. */
        CHECK(context_selected_global_pointer_get() == 0xAAu,
              "mode 0 reads the first pointer");

        /* The predicate: no selection stored yet, everything rejects. */
        int buf[256];
        memset(buf, 0, sizeof(buf));
        void* owner_slot = NULL;
        memcpy((char*)buf + 0x36c, &owner_slot, sizeof(owner_slot));
        CHECK(entity_matches_current_selection_or_owner(buf) == 0,
              "no selection rejects");
        CHECK(entity_matches_current_selection_or_owner(NULL) == 0,
              "NULL entity rejects (deviation: corpus derefs)");

        /* Store a selection through the media model (mode 0 path). */
        media_min_distance_update_mode(0, buf, 0);
        CHECK(entity_matches_current_selection_or_owner(buf) == 1,
              "the entity itself matches the selection");
        void* owner = buf;
        memcpy((char*)buf + 0x36c, &owner, sizeof(owner));
        CHECK(entity_matches_current_selection_or_owner(buf) == 1,
              "the owner match satisfies it too");

        /* Mode 1 rejects the direct match (corpus shape). */
        int other[256];
        memset(other, 0, sizeof(other));
        media_min_distance_update_mode(1, buf, 0);
        /* mode 1 clears the context in the corpus shape. */
        CHECK(context_selected_global_pointer_get() == 0xBBu,
              "mode 1 reads the second pointer");
        media_min_distance_update_mode(0, other, 0);
        CHECK(entity_matches_current_selection_or_owner(buf) == 0,
              "another selection rejects");

        media_min_distance_update_mode(1, NULL, 0);
        context_selected_pointer_pair_set(0u, 0u);
    }

    /* GlobalAnimationStateSet (2026-10-24): the counter word store. */
    {
        entity_global_animation_state_set(0u);
        CHECK(entity_global_animation_state_peek() == 0u,
              "the counter clears on store");
        entity_global_animation_state_set(7u);
        CHECK(entity_global_animation_state_peek() == 7u,
              "the store reads back");
    }

    /* The eligibility + admission leaves (2026-10-24). */
    {
        static GameContext ctx;
        game_context_init(&ctx);
        int idx = entity_create(&ctx, 1, "EligLeaf", 0.0f, 0.0f, 0.0f, 0.0f);
        CHECK(idx >= 0, "eligibility leaf slot created");
        if (idx >= 0) {
            EntitySlot* e = &ctx.entity[0][idx];
            e->flags_3e0 = 0u;
            e->flags_3e4 = 0u;
            e->flags_3e8 = 0u;
            e->field_340 = 0.0f;
            char rec[ENTITY_STATS_RECORD_SIZE];
            memset(rec, 0, sizeof rec);
            e->stats_record = rec;
            /* quad-mesh admission: clear stats, type 1 answers allowed. */
            CHECK(entity_quad_mesh_render_record_allowed(e) == 1,
                  "a clear stats word allows the record");
            const uint32_t hi = 0x08000000u;
            memcpy(rec + 0x148, &hi, 4);
            CHECK(entity_quad_mesh_render_record_allowed(e) == 0,
                  "the stats bit admits nothing");
            const uint32_t zero = 0u;
            memcpy(rec + 0x148, &zero, 4);
            e->type = 8;
            CHECK(entity_quad_mesh_render_record_allowed(e) == 0,
                  "type 8 is excluded");
            e->type = 13;
            CHECK(entity_quad_mesh_render_record_allowed(e) == 0,
                  "type 13 is excluded");
            e->type = 1;
            /* flag-100 + shared eligibility: gate follows stats bit 0x100. */
            CHECK(entity_flag100_and_shared_eligibility_check(e) == 0,
                  "a clear stats word answers no");
            const uint32_t f100 = 0x100u;
            memcpy(rec + 0x148, &f100, 4);
            CHECK(entity_flag100_and_shared_eligibility_check(e) == 1,
                  "bit 100 rides through the relaxed gate");
            e->type = 4; /* the gate takes the strict clause */
            e->flags_3e8 = 0x20000000u;
            CHECK(entity_flag100_and_shared_eligibility_check(e) == 1,
                  "the strict clause answers through its word");
            e->flags_3e8 = 0u;
            CHECK(entity_flag100_and_shared_eligibility_check(e) == 0,
                  "the strict clause fails its word");
            /* task type + condition: the gate and the caller's condition. */
            CHECK(task_type_and_condition_check(NULL, e, 1) == 1,
                  "the relaxed gate carries the condition");
            CHECK(task_type_and_condition_check(NULL, e, 0) == 0,
                  "a clear condition answers no");
            e->flags_3e0 = 0x100u; /* blocks the gate */
            CHECK(task_type_and_condition_check(NULL, e, 1) == 0,
                  "the blocked gate answers no");
        }
    }

    /* The shared eligibility gate's own negative field (2026-10-24). */
    {
        static GameContext ctx;
        game_context_init(&ctx);
        int idx = entity_create(&ctx, 1, "EligNeg", 0.0f, 0.0f, 0.0f, 0.0f);
        if (idx >= 0) {
            EntitySlot* e = &ctx.entity[0][idx];
            e->flags_3e0 = 0u;
            e->flags_3e4 = 0u;
            e->field_340 = -1.0f;
            CHECK(entity_flag100_and_shared_eligibility_check(e) == 0,
                  "the negative bound fails the gate");
        }
    }

    /* EntityTypeAndStateFlagPredicate (2026-10-24): set + flag gate. */
    {
        EntitySlot e;
        memset(&e, 0, sizeof e);
        CHECK(entity_type_and_state_flag_predicate(&e) == 1,
              "an included kind with clear flags answers one");
        static char rec[0x150];
        memset(rec, 0, sizeof rec);
        const uint32_t bit = 0x20000000u;
        __builtin_memcpy(rec + 0x148, &bit, sizeof bit);
        e.stats_record = rec;
        CHECK(entity_type_and_state_flag_predicate(&e) == 0,
              "the raised flag bit silences the predicate");
        e.type = 0xe;
        CHECK(entity_type_and_state_flag_predicate(&e) == 0,
              "an excluded kind answers zero");
        e.type = 1;
        memset(rec + 0x148, 0, 4);
        CHECK(entity_type_and_state_flag_predicate(&e) == 1,
              "the cleared flag re-arms the predicate");
        CHECK(entity_type_and_state_flag_predicate(NULL) == 0,
              "the null entity answers zero");
    }

    /* ObjectFilterMatchesActionTarget (2026-10-24): kinds + identity. */
    {
        uint32_t filt[12];
        uint32_t targ[4];
        memset(filt, 0, sizeof filt);
        memset(targ, 0, sizeof targ);

        filt[0] = 3u;
        CHECK(object_filter_matches_action_target(filt, targ) == 0,
              "an off-list kind answers zero");
        filt[0] = 5u;
        CHECK(object_filter_matches_action_target(filt, targ) == 1,
              "the bare kind-5 matches");
        filt[8] = 0x1111u; /* +0x20 name target */
        CHECK(object_filter_matches_action_target(filt, targ) == 0,
              "a name pair needs the target word");
        targ[0] = 0x1111u;
        CHECK(object_filter_matches_action_target(filt, targ) == 1,
              "the name word matches through");
        filt[10] = 0x77u; /* +0x28 companion */
        CHECK(object_filter_matches_action_target(filt, targ) == 0,
              "the companion word must agree too");
        targ[2] = 0x77u;
        CHECK(object_filter_matches_action_target(filt, targ) == 1,
              "the full pair matches");
        filt[9] = 5u; /* +0x24 selector */
        CHECK(object_filter_matches_action_target(filt, targ) == 0,
              "a set selector gates word 1");
        targ[1] = 5u;
        CHECK(object_filter_matches_action_target(filt, targ) == 1,
              "the selector agreement lands");

        static EntitySlot eslot;
        memset(&eslot, 0, sizeof eslot);
        eslot.flags_3e8 = 0x20000000u;
        filt[0] = 7u;
        uintptr_t handle = (uintptr_t)&eslot;
        memcpy((char*)filt + 0x10, &handle, sizeof handle);
        CHECK(object_filter_matches_action_target(filt, targ) == 1,
              "the eligible handle lets kind 7 through");
        eslot.flags_3e8 = 0u;
        CHECK(object_filter_matches_action_target(filt, targ) == 0,
              "the ineligible handle blocks kind 7");
        void* zero_handle = NULL;
        memcpy((char*)filt + 0x10, &zero_handle, sizeof zero_handle);
        CHECK(object_filter_matches_action_target(filt, targ) == 1,
              "the handleless kind 7 skips the check (quirk)");
        CHECK(object_filter_matches_action_target(NULL, targ) == 0
                  && object_filter_matches_action_target(filt, NULL) == 0,
              "the null records answer zero");
    }

    /* EntityDetachCarriedObjectAndResetState (2026-10-24): the carrier
     * detach gates, the pair clear, and the field-0x9c fold. */
    {
        static EntitySlot carrier, carried, stranger;
        static LevelState dlvl;
        int ref_value = 7;
        memset(&carrier, 0, sizeof carrier);
        memset(&carried, 0, sizeof carried);
        memset(&stranger, 0, sizeof stranger);
        memset(&dlvl, 0, sizeof dlvl);

        carrier.type = 2;
        carrier.field_340 = 5.0f;
        entity_detach_carried_object_and_reset_state(&carrier, &dlvl);
        CHECK(carrier.field_340 == 5.0f && carrier.flags_3e8 == 0u,
              "the type gate keeps non-type-3 slots untouched");

        carrier.type = 3;
        entity_detach_carried_object_and_reset_state(&carrier, &dlvl);
        CHECK(carrier.field_340 == -1.0f && carrier.flags_3e8 == 0x800000u,
              "the bare type-3 reset lands");

        carried.type = 6;
        carried.attach_304 = &carrier;
        carried.field_340 = 42.0f;
        carrier.carry_link_308 = &carried;
        carrier.carry_state_324 = 9;
        carrier.flags_3e0 = 0x400u;
        entity_detach_carried_object_and_reset_state(&carrier, &dlvl);
        CHECK(carrier.carry_link_308 == NULL
                  && carrier.carry_state_324 == 0
                  && carried.attach_304 == NULL
                  && (carrier.flags_3e0 & 0x400u) == 0u
                  && carried.field_340 == -1.0f
                  && carrier.field_340 == -1.0f,
              "the detach clears the carried half and the pair");

        carrier.carry_link_308 = &stranger;
        stranger.type = 5;
        stranger.field_340 = 3.0f;
        entity_detach_carried_object_and_reset_state(&carrier, &dlvl);
        CHECK(carrier.carry_link_308 == &stranger
                  && stranger.field_340 == 3.0f,
              "the non-6 carry stays attached (the corpus type test)");

        carrier.carry_link_308 = NULL;
        carrier.carry_ref_348 = &ref_value;
        entity_detach_carried_object_and_reset_state(&carrier, &dlvl);
        CHECK((int)level_field9c_get(&dlvl) == 7,
              "the reference dword folds into the level's 9c");
        carrier.carry_ref_348 = NULL;
        entity_detach_carried_object_and_reset_state(&carrier, NULL);
        level_state_field9c_add_for_level(NULL, 9); /* the null guard is a no-op */
        CHECK((int)level_field9c_get(&dlvl) == 7,
              "the absent reference keeps the fold quiet");
    }

    /* EntityClearLinkedObjectState (2026-10-24): the gate-400 clear. */
    {
        static EntitySlot gate, linked;
        memset(&gate, 0, sizeof gate);
        memset(&linked, 0, sizeof linked);
        gate.flags_3e0 = 0x100u;
        gate.carry_link_308 = &linked;
        gate.carry_state_324 = 5;
        entity_clear_linked_object_state(&gate);
        CHECK(gate.carry_link_308 == &linked && gate.carry_state_324 == 5,
              "the clear stays behind the gate bit");
        gate.flags_3e0 = 0x400u;
        linked.flags_3e8 = 1u;
        entity_clear_linked_object_state(&gate);
        CHECK(gate.carry_link_308 == NULL && gate.carry_state_324 == 0
                  && (gate.flags_3e0 & 0x400u) == 0u
                  && linked.flags_3e8 == 0x2001u,
              "the clear drops the pair and raises the carried 0x2000");
        gate.flags_3e0 = 0x400u;
        gate.carry_link_308 = NULL;
        entity_clear_linked_object_state(&gate);
        CHECK(gate.flags_3e0 == 0u,
              "the missing carried slot rides the guard (deviation)");
    }

    /* Pooled-record chunk window (2026-10-25): stride links and the
     * chunk-tail join onto the previous head. */
    {
        const uint32_t chunks_before = pooled_record_chunk_count_peek();
        pooled_record_chunk_allocate_and_link();
        CHECK(pooled_record_chunk_count_peek() == chunks_before + 1u,
              "the first chunk raises the counter");
        void* head1 = pooled_record_head_peek();
        CHECK(head1 != NULL, "the chunk lands on the free head");
        void* link = (void*)0x1;
        memcpy(&link, (char*)head1 + 0x408, sizeof link);
        CHECK(link == NULL, "the lone record closes to the old head");
        pooled_record_chunk_allocate_and_link(); /* counter 1 -> two records */
        void* head2 = pooled_record_head_peek();
        CHECK(head2 != NULL && head2 != head1, "the second chunk moves the head");
        memcpy(&link, (char*)head2 + 0x408, sizeof link);
        CHECK(link == (char*)head2 + 0x410, "the first link rides the stride");
        memcpy(&link, (char*)head2 + 0x410 + 0x408, sizeof link);
        CHECK(link == head1, "the chunk tail joins the previous head");
        CHECK(pooled_record_chunk_count_peek() == chunks_before + 2u,
              "the counter grows with the chunk table");
    }

    /* Pooled record allocate-and-reset (2026-10-25): the pop, the
     * 0x103-dword clear, the busy self-link, and the seeded floats. */
    {
        const uint32_t chunks_before = pooled_record_chunk_count_peek();
        void* head = pooled_record_head_peek();
        CHECK(head != NULL, "the chunk window leaves records on the head");
        void* rec1 = pooled_record_allocate_and_reset();
        CHECK(rec1 == head, "the pop answers the head record");
        void* link = NULL;
        memcpy(&link, (char*)rec1 + 0x408, sizeof link);
        CHECK(link == rec1, "the busy mark self-links");
        uint32_t word = 0xffffffffu;
        memcpy(&word, (char*)rec1 + 0x14, 4);
        CHECK(word == 0u, "the clear zeroes the record head");
        memcpy(&word, (char*)rec1 + 0x2f4, 4);
        const uint32_t seeded = word;
        memcpy(&word, (char*)rec1 + 0x2f8, 4);
        CHECK(seeded == 0xbf800000u && word == 0xbf800000u,
              "the float pair seeds");
        CHECK(pooled_record_head_peek() != rec1, "the head advanced past it");

        /* Drain the window, then the empty-head grow half. */
        int drained = 0;
        while (pooled_record_head_peek() != NULL) {
            CHECK(pooled_record_allocate_and_reset() != NULL,
                  "the drain pops records (rounds)");
            if (++drained > 64) {
                break; /* guard the test if the head never empties */
            }
        }
        CHECK(drained > 0, "the window drained to empty");
        const void* grown = pooled_record_allocate_and_reset();
        CHECK(grown != NULL && pooled_record_head_peek() != NULL,
              "the empty head grows a chunk (deviation-pinned)");
        CHECK(pooled_record_chunk_count_peek() > chunks_before,
              "the grow raises the chunk counter");
    }

    /* Resolved entity operation dispatch (2026-10-25): the resolver's
     * (type, row) pair feeds the count-difference scan. */
    {
        grid_record_count_tables_clear();
        CHECK(resolved_entity_operation_dispatch("NotAThing", 0, 0) == 0,
              "the unknown name answers zero");
        /* "hoverboard" = table type 1, row 0 -> cell 15; slot 3. */
        grid_count_table_slot_poke(1, 0, 3, 1, 1234u);
        grid_count_table_slot_poke(1, 0, 3, 0, 22u);
        CHECK(resolved_entity_operation_dispatch("hoverboard", 3, 1) == 1212,
              "the vehicle pair lands (type 1, row 0)");
        /* "tinyrm" = creature type 3, row 1 -> cell 46; subtract-free. */
        grid_count_table_slot_poke(3, 1, 1, 1, 55u);
        CHECK(resolved_entity_operation_dispatch("tinyrm", 1, 0) == 55,
              "the creature pair lands (type 3, row 1)");
        /* The ore pair: dedicated index stores 0 and 1. */
        grid_count_table_slot_poke(7, 0, 0, 1, 77u);
        grid_count_table_slot_poke(7, 1, 0, 1, 99u);
        CHECK(resolved_entity_operation_dispatch("Ore", 0, 0) == 77
                  && resolved_entity_operation_dispatch("ProcessedOre", 0, 0)
                         == 99,
              "the ore pair lands through the case-fold");
        /* Special names ride the zeroed row slot. */
        grid_count_table_slot_poke(5, 0, 2, 1, 7u);
        CHECK(resolved_entity_operation_dispatch("Boulder", 2, 0) == 7
                  && resolved_entity_operation_dispatch("Boulder", 0, 0) == 0,
              "the special kinds ride the zero row");
        grid_record_count_tables_clear();
    }

    /* DAT_00502bd8 indexed stats-type tables (2026-10-25): window reads,
     * install spans, instance strides and guards. Type 9 / row 7 belongs
     * to this band. */
    {
        uint8_t tmpl[2 * 0x150];
        for (int i = 0; i < (int)sizeof(tmpl); ++i) {
            tmpl[i] = (uint8_t)(i % 256);
        }
        uint8_t cfg[0x334];
        memset(cfg, 0, sizeof(cfg));

        CHECK(stats_table_word_at_b8(9, 7) == 0u
                  && stats_table_float_at_dc(9, 7) == 0.0f
                  && stats_table_word_at_144(9, 7) == 0u
                  && stats_table_word_at_148(9, 7) == 0u
                  && stats_table_word_at_14c(9, 7) == 0u,
              "uninstalled cells read zero");

        stats_type_table_install(9, 7, tmpl, 2);
        {
            uint32_t w = 0;
            float f = 0.0f;
            __builtin_memcpy(&w, tmpl + 0xb8, 4);
            CHECK(stats_table_word_at_b8(9, 7) == w, "the b8 window read");
            __builtin_memcpy(&f, tmpl + 0xdc, 4);
            CHECK(stats_table_float_at_dc(9, 7) == f, "the dc float window read");
            __builtin_memcpy(&w, tmpl + 0x144, 4);
            CHECK(stats_table_word_at_144(9, 7) == w, "the 144 window read");
            __builtin_memcpy(&w, tmpl + 0x148, 4);
            CHECK(stats_table_word_at_148(9, 7) == w, "the 148 window read");
            __builtin_memcpy(&w, tmpl + 0x14c, 4);
            CHECK(stats_table_word_at_14c(9, 7) == w, "the 14c window read");
        }

        {
            int type = 9, row = 7;
            __builtin_memcpy(cfg + 0x0, &type, 4);
            __builtin_memcpy(cfg + 0x4, &row, 4);
            int idx1 = 1;
            __builtin_memcpy(cfg + 0x330, &idx1, 4);
            uint32_t w = 0;
            __builtin_memcpy(&w, tmpl + 0xbc + 0x150, 4);
            CHECK(stats_instance_word_at_bc(cfg) == w, "the bc instance read");
            __builtin_memcpy(&w, tmpl + 0xe0 + 0x150, 4);
            CHECK(stats_instance_word_at_e0(cfg) == w, "the e0 instance read");
            __builtin_memcpy(&w, tmpl + 0xec + 0x150, 4);
            CHECK(stats_instance_word_at_ec(cfg) == w, "the ec instance read");
            __builtin_memcpy(&w, tmpl + 0xf0 + 0x150, 4);
            CHECK(stats_instance_word_at_f0(cfg) == w, "the f0 instance read");

            int idx2 = 2; /* strides past the installed two-stride template */
            __builtin_memcpy(cfg + 0x330, &idx2, 4);
            CHECK(stats_instance_word_at_e0(cfg) == 0u, "the stride guard answers zero");
            CHECK(stats_instance_word_at_bc(NULL) == 0u, "the NULL block guard holds");
        }

        {
            uint32_t w = 0;
            __builtin_memcpy(&w, tmpl + 0xb4 + 0x150, 4);
            CHECK(stats_table_word_at_b4(9, 7, 1) == w, "the b4 stride read");
            __builtin_memcpy(&w, tmpl + 0xd0 + 0x150, 4);
            CHECK(stats_table_word_at_d0(9, 7, 1) == w, "the d0 stride read");
            __builtin_memcpy(&w, tmpl + 0x100 + 0x150, 4);
            CHECK(stats_table_word_at_100(9, 7, 1) == w, "the 100 stride read");
            CHECK(stats_table_word_at_b4(9, 7, 2) == 0u
                      && stats_table_word_at_b4(9, 7, -1) == 0u,
                  "the stride guards answer zero");
        }

        CHECK(stats_table_word_at_b8(-1, 0) == 0u
                  && stats_type_table_cell(9, 18) == NULL
                  && stats_table_word_at_b8(16, 7) == 0u,
              "the window guards hold");
        stats_type_table_install(9, 7, NULL, 0); /* release the cell */
    }

    /* EntityQueryFlagCount (2026-10-25): catalog counts, the 0x14/0x15
     * flag folds over the type-1 stats rows, and the pair-table default. */
    {
        static uint8_t rows[3][2 * 0x150];
        memset(rows, 0, sizeof rows);
        for (int i = 0; i < 3; ++i) {
            stats_type_table_install(1, i, rows[i], 2);
        }

        CHECK(entity_query_flag_count(0x13) == 0
                  && catalog_count_peek(1) == 0u
                  && catalog_count_peek(0) == 0u,
              "the fresh catalog counts answer zero");
        catalog_count_install(1, 7u);
        CHECK(entity_query_flag_count(0x13) == 7, "the 0x13 query answers count b");
        catalog_count_install(1, 0u);

        /* Zero-count fold: no rows in range, both folds answer zero. */
        CHECK(entity_query_flag_count(0x14) == 0 && entity_query_flag_count(0x15) == 0,
              "the empty catalog folds zero");

        /* Row 0: w148 bit 0x1000, no 14c bit — counts in 0x14, skips 0x15. */
        uint32_t v = 0x1000u;
        __builtin_memcpy(rows[0] + 0x148, &v, 4);
        catalog_count_install(0, 1u);
        CHECK(entity_query_flag_count(0x14) == 1, "the 0x1000 flag counts in 0x14");
        CHECK(entity_query_flag_count(0x15) == 0,
              "the 0x1000-only row skips the 0x15 fold");

        /* Row 1: 0x2000 raised but the 14c bit 8 — 0x14 skips, 0x15 counts. */
        v = 0x2000u;
        __builtin_memcpy(rows[1] + 0x148, &v, 4);
        v = 8u;
        __builtin_memcpy(rows[1] + 0x14c, &v, 4);
        catalog_count_install(0, 2u);
        CHECK(entity_query_flag_count(0x14) == 1, "the raised bit 8 drops 0x14");
        CHECK(entity_query_flag_count(0x15) == 1, "the raised bit 8 counts in 0x15");

        /* Row 2: 0x800 — the 0x15 fold counts it through the other half.
         * Past-count rows would stay out too: row 2 rides in only now. */
        v = 0x800u;
        __builtin_memcpy(rows[2] + 0x148, &v, 4);
        catalog_count_install(0, 3u);
        CHECK(entity_query_flag_count(0x15) == 2, "the 0x800 flag counts in 0x15");
        CHECK(entity_query_flag_count(0x14) == 1,
              "the 0x800-only row skips the 0x14 fold");

        /* The pair-table default: own storage, guarded window. */
        CHECK(entity_query_flag_count(2) == 0,
              "the default query answers the empty pair table");
        catalog_id_pair_install(2, 42u);
        CHECK(entity_query_flag_count(2) == 42, "the installed pair answers");
        catalog_id_pair_install(8, 2u);
        catalog_id_pair_install(18, 4u);
        CHECK(entity_query_flag_count(8) == 2
                  && entity_query_flag_count(18) == 4,
              "registered action modes above seven expose their counts");
        catalog_id_pair_install(8, 0u);
        catalog_id_pair_install(18, 0u);
        CHECK(entity_query_flag_count(-1) == 0 && entity_query_flag_count(24) == 0,
              "the pair-window guards answer zero");

        catalog_count_install(0, 0u);
        catalog_count_install(1, 0u);
        for (int i = 0; i < 3; ++i) {
            stats_type_table_install(1, i, NULL, 0); /* release */
        }
    }

    /* CatalogCallbackByIdDispatch (FUN_0041b680; 2026-10-25): the three
     * catalog lanes hand the callback their gated rows, and the generic
     * lane sweeps a key group's code array through the shared band. */
    {
        static GameContext dctx;
        game_context_init(&dctx);
        static uint8_t drows[3][2 * 0x150];
        memset(drows, 0, sizeof drows);
        for (int i = 0; i < 3; ++i) {
            stats_type_table_install(1, i, drows[i], 2);
        }
        uint32_t v;
        v = 0x1000u;
        __builtin_memcpy(drows[0] + 0x148, &v, 4);
        v = 0x2000u;
        __builtin_memcpy(drows[1] + 0x148, &v, 4);
        v = 8u;
        __builtin_memcpy(drows[1] + 0x14c, &v, 4);
        v = 0x800u;
        __builtin_memcpy(drows[2] + 0x148, &v, 4);

        /* 0x13: count_b rows, each handed over as (0x49, 4, row). */
        catalog_count_install(1, 3u);
        s_disp_calls = 0;
        s_disp_stop_at = 0;
        s_disp_grow = 0;
        CHECK(catalog_callback_by_id_dispatch(&dctx, 0x13, disp_recorder, NULL)
                  == 0 && s_disp_calls == 3
                  && s_disp_kind[0] == 0x49 && s_disp_lane[0] == 4
                  && s_disp_row[0] == 0 && s_disp_row[2] == 2,
              "the 0x13 lane hands the callback every count_b row");

        /* The first nonzero callback answer ends the dispatch. */
        s_disp_calls = 0;
        s_disp_stop_at = 2;
        CHECK(catalog_callback_by_id_dispatch(&dctx, 0x13, disp_recorder, NULL)
                  == 1 && s_disp_calls == 2,
              "the first nonzero callback answer wins the fold");

        /* 0x14: rows raising +148 bit 0x1000 or 0x2000 while the +14c bit 8
         * stays clear; row 1's raised bit 8 drops it out. */
        catalog_count_install(0, 3u);
        s_disp_calls = 0;
        s_disp_stop_at = 0;
        CHECK(catalog_callback_by_id_dispatch(&dctx, 0x14, disp_recorder, NULL)
                  == 0 && s_disp_calls == 1
                  && s_disp_kind[0] == 0x49 && s_disp_lane[0] == 1
                  && s_disp_row[0] == 0,
              "the 0x14 gate hands over the raised-bit clear-word rows");

        /* 0x15: rows raising +148 bit 0x800 or the +14c bit 8. */
        s_disp_calls = 0;
        CHECK(catalog_callback_by_id_dispatch(&dctx, 0x15, disp_recorder, NULL)
                  == 0 && s_disp_calls == 2
                  && s_disp_row[0] == 1 && s_disp_row[1] == 2,
              "the 0x15 gate folds the other flag halves");
        catalog_count_install(0, 0u);
        catalog_count_install(1, 0u);
        for (int i = 0; i < 3; ++i) {
            stats_type_table_install(1, i, NULL, 0); /* release */
        }

        /* Generic lane, pick branch: the empty selection rewrites the
         * cached 0x22 element in place through the selection resolve. */
        static int codes_a[3] = { 1, 0x22, 5 };
        objective_selection_state_reset(); /* inter bit 7: the pick branch */
        resource_handle_set_band_word_install(0, 3);
        resource_handle_set_slot_install(0, codes_a);
        s_disp_calls = 0;
        CHECK(catalog_callback_by_id_dispatch(&dctx, 0, disp_recorder, NULL)
                  == 0 && s_disp_calls == 3
                  && s_disp_lane[0] == 0 && s_disp_lane[1] == 0
                  && s_disp_kind[0] == 1 && s_disp_kind[1] == 0x23
                  && codes_a[1] == 0x23 && s_disp_kind[2] == 5,
              "the sweep rewrites the cached 0x22 pick through the resolve");
        resource_handle_set_band_word_install(0, 0);

        /* The high-bit selection rewrites the 0x23 element back up. */
        static EntitySlot picked;
        memset(&picked, 0, sizeof picked);
        picked.type = 1;
        picked.flags_3e8 = 0x80000000u;
        selection_add(&dctx, &picked);
        static int codes_b[2] = { 2, 0x23 };
        resource_handle_set_band_word_install(1, 2);
        resource_handle_set_slot_install(1, codes_b);
        s_disp_calls = 0;
        CHECK(catalog_callback_by_id_dispatch(&dctx, 1, disp_recorder, NULL)
                  == 0 && s_disp_calls == 2
                  && s_disp_kind[1] == 0x22 && codes_b[1] == 0x22,
              "the high-bit selection rewrites the 0x23 pick in the array");
        selection_clear(&dctx);
        resource_handle_set_band_word_install(1, 0);
        resource_handle_set_slot_install(1, NULL);

        /* The corpus re-reads the band word every hop: a callback that
         * regrows the count retargets the walk mid-sweep. */
        static int codes_c[2] = { 6, 7 };
        resource_handle_set_band_word_install(3, 1);
        resource_handle_set_slot_install(3, codes_c);
        s_disp_calls = 0;
        s_disp_grow = 3;
        CHECK(catalog_callback_by_id_dispatch(&dctx, 3, disp_recorder, NULL)
                  == 0 && s_disp_calls == 2 && s_disp_kind[1] == 7,
              "the sweep re-reads the band count every hop");
        s_disp_grow = 0;
        resource_handle_set_band_word_install(3, 0);
        resource_handle_set_slot_install(3, NULL);

        /* Guards: the NULL callback and the missing pair array answer zero
         * (the corpus strides both blindly). */
        resource_handle_set_band_word_install(4, 1);
        s_disp_calls = 0;
        CHECK(catalog_callback_by_id_dispatch(&dctx, 4, disp_recorder, NULL) == 0
                  && s_disp_calls == 0,
              "the pair-array guard answers zero");
        CHECK(catalog_callback_by_id_dispatch(&dctx, 9, NULL, NULL) == 0,
              "the NULL callback guard answers zero");
        resource_handle_set_band_word_install(4, 0);
    }

    if (s_failures != 0) {
        fprintf(stderr, "entity_test: %d failure(s)\n", s_failures);
        return 1;
    }
    printf("entity_test: all pass\n");
    return 0;
}
