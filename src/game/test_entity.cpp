// SPDX-License-Identifier: LGPL-3.0-or-later
// Entity table + `.ol` object list (findings 10/07/12).
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "game/entity.hpp"
#include "gods98/media.hpp"
#include "game/game.hpp"
#include "gods98/block.hpp"
#include "gods98/resource.hpp"
#include "game/frontend.hpp"
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

static int fixture_roundtrip(void)
{
    BlockFile* cfg = block_open("Lego.cfg");
    CHECK(cfg != NULL, "open cfg");
    if (cfg == NULL) {
        return 1;
    }
    GameContext ctx;
    game_context_init(&ctx);
    ctx.cfg = cfg;

    int placed = object_list_load(&ctx, cfg, "Level01");
    CHECK(placed == 4, "four .ol records handled (3 entities + camera; Test1 skipped)");

    CHECK(entity_count(&ctx, ENTITY_TYPE_VEHICLE) == 1, "one vehicle");
    CHECK(entity_count(&ctx, ENTITY_TYPE_BUILDING) == 1, "one building");
    CHECK(entity_count(&ctx, ENTITY_TYPE_MINIFIG) == 1, "one minifigure");
    CHECK(entity_count(&ctx, ENTITY_TYPE_SPECIAL) == 0, "no special objects");

    const EntitySlot* v = entity_at(&ctx, ENTITY_TYPE_VEHICLE, 0);
    CHECK(v != NULL && strcmp(v->name, "Digger1") == 0, "vehicle name");
    CHECK(v != NULL && v->x == 12.5f && v->y == 19.5f && v->z == 0.0f,
          "vehicle xPos/yPos placed (z is 0 in the .ol)");
    CHECK(v != NULL && v->rot == 45.0f, "vehicle heading -> rot");
    CHECK(v != NULL && v->type == ENTITY_TYPE_VEHICLE, "SmallDigger -> vehicle");
    CHECK(v != NULL && strcmp(v->type_name, "SmallDigger") == 0,
          "type_name from the .ol `type` key");
    CHECK(v != NULL && v->driving[0] == '\0', "no driving ref on a vehicle");

    const EntitySlot* b = entity_at(&ctx, ENTITY_TYPE_BUILDING, 0);
    CHECK(b != NULL && strcmp(b->name, "WallA") == 0, "building name");
    CHECK(b != NULL && strcmp(b->type_name, "Toolstation") == 0,
          "Toolstation -> building + type_name");
    CHECK(b != NULL && b->x == 12.5f && b->y == 13.5f, "building xPos/yPos");

    const EntitySlot* m = entity_at(&ctx, ENTITY_TYPE_MINIFIG, 0);
    CHECK(m != NULL && strcmp(m->name, "MinifigA") == 0, "minifig name");
    CHECK(m != NULL && strcmp(m->type_name, "Pilot") == 0,
          "Pilot -> minifig + type_name");
    CHECK(m != NULL && strcmp(m->driving, "Digger1") == 0,
          "driving ref captured");
    CHECK(m != NULL && m->health == 150, ".ol `health` captured");
    /* Optional Model key parses into the slot (the file does not exist, so
     * the handle stays NULL — lazy load). */
    CHECK(m != NULL && strcmp(m->model_path, "Data\\Tools\\dig.lwo") == 0,
          "minifig model path from the .ol Model key");
    CHECK(m != NULL && entity_model(m) == NULL, "missing model file -> NULL");

    /* TVCamera — FUN_0042e020's type -1 branch drives the follow camera
     * instead of placing an entity. */
    CHECK(ctx.ol_camera_set == 1, "camera placed");
    CHECK(ctx.ol_camera_x == 5.0f && ctx.ol_camera_y == 6.0f,
          "camera xPos/yPos");
    CHECK(ctx.ol_camera_heading == 30.0f, "camera heading");

    /* Unregistered type name (Test1) — corpus aborts; the port skips. */
    CHECK(entity_count(&ctx, ENTITY_TYPE_MINIFIG) == 1,
          "Test1 not placed as a minifig (unregistered)");

    /* Corpus strides. */
    CHECK(entity_stride(ENTITY_TYPE_VEHICLE) == 0x1ec, "vehicle stride");
    CHECK(entity_stride(ENTITY_TYPE_MINIFIG) == 0x74, "minifig stride");
    CHECK(entity_stride(ENTITY_TYPE_BUILDING) == 0x14c, "building stride");
    CHECK(entity_stride(ENTITY_TYPE_SPECIAL) == 0x74, "special stride (grouped)");
    CHECK(entity_stride(20) == 0, "code 20 out of range stride 0");

    /* Table-exhaustion is guarded. */
    GameContext cap;
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

            GameContext ctx;
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
        snprintf(path, sizeof(path), "%s Levels %s OListFile", root,
                 block_name(g));
        const char* olist = block_get_strptr(cfg, path);
        if (olist == NULL) {
            snprintf(path, sizeof(path), "%s %s OListFile", root,
                     block_name(g));
            olist = block_get_strptr(cfg, path);
        }
        if (olist == NULL) {
            continue;
        }
        GameContext ctx;
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

    GameContext ctx;
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
    GameContext ctx;
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

int main(int argc, char** argv)
{
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
        "  UnknownX {\n"
        "    type    Test1\n"
        "    xPos    1\n"
        "    yPos    2\n"
        "  }\n"
        "}\n");

    if (chdir(FIX) != 0) {
        fprintf(stderr, "chdir failed\n");
        return 1;
    }
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

    /* The object pointer registry (2026-10-24): no feeder lands, so the
     * count stays zero and the slot lookup never fires. */
    {
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

    if (s_failures != 0) {
        fprintf(stderr, "entity_test: %d failure(s)\n", s_failures);
        return 1;
    }
    printf("entity_test: all pass\n");
    return 0;
}