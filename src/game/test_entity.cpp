// SPDX-License-Identifier: LGPL-3.0-or-later
// Entity table + `.ol` object list (findings 10/07/12).
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "game/entity.hpp"
#include "game/game.hpp"
#include "gods98/block.hpp"
#include "gods98/resource.hpp"
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
    CHECK(placed == 3, "three objects placed from M01.ol");

    CHECK(entity_count(&ctx, ENTITY_TYPE_VEHICLE) == 1, "one vehicle");
    CHECK(entity_count(&ctx, ENTITY_TYPE_BUILDING) == 1, "one building");
    CHECK(entity_count(&ctx, ENTITY_TYPE_MINIFIG) == 1, "one minifigure");

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
    /* Optional Model key parses into the slot (the file does not exist, so
     * the handle stays NULL — lazy load). */
    CHECK(m != NULL && strcmp(m->model_path, "Data\\Tools\\dig.lwo") == 0,
          "minifig model path from the .ol Model key");
    CHECK(m != NULL && entity_model(m) == NULL, "missing model file -> NULL");

    /* Corpus strides. */
    CHECK(entity_stride(ENTITY_TYPE_VEHICLE) == 0x1ec, "vehicle stride");
    CHECK(entity_stride(ENTITY_TYPE_MINIFIG) == 0x74, "minifig stride");
    CHECK(entity_stride(ENTITY_TYPE_BUILDING) == 0x14c, "building stride");
    CHECK(entity_stride(9) == 0, "bad type stride 0");

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
        snprintf(buf, sizeof(buf), "%s|%s|%g|%g|%g|%s", a[i].name,
                 a[i].type_name, a[i].x, a[i].y, a[i].heading, a[i].driving);
        h = fnv1a32(h, buf);
    }
    return h;
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
    int files = 0, objects = 0, mismatches = 0;
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
            ol_sort(want, wn);

            GameContext ctx;
            game_context_init(&ctx);
            ctx.cfg = cfg;
            int placed = object_list_load_file(&ctx, ename);

            OlRec got[OL_PROBE_MAX];
            int gn = 0;
            for (int t = ENTITY_TYPE_VEHICLE; t <= ENTITY_TYPE_BUILDING; ++t) {
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
                    snprintf(got[gn].driving, sizeof(got[gn].driving),
                             "%s", s->driving);
                    ++gn;
                }
            }
            ol_sort(got, gn);

            int ok = placed == wn && gn == wn
                     && ol_digest(got, gn) == ol_digest(want, wn);
            if (!ok) {
                ++mismatches;
                fprintf(stderr,
                        "entity_test: %s: .ol mismatch (placed=%d want=%d "
                        "got=%d)\n", ename, placed, wn, gn);
                for (int d = 0; d < wn && d < 3; ++d) {
                    fprintf(stderr,
                            "  want %-16s type=%-16s x=%.4f y=%.4f h=%.4f "
                            "d=%s\n",
                            want[d].name, want[d].type_name, want[d].x,
                            want[d].y, want[d].heading, want[d].driving);
                }
            }
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
        for (int t = ENTITY_TYPE_VEHICLE; t <= ENTITY_TYPE_BUILDING; ++t) {
            tables += entity_count(&ctx, t);
        }
        if (placed < 0 || placed != tables) {
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
    fprintf(stderr, "entity_test: --real probe: %d corpus .ol files, %d "
                    "objects, %d cfg-wired loads, %s\n",
            files, objects, wired, mismatches == 0 && wired_fail == 0
                                       ? "all match" : "MISMATCH");
    return files;
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
        "    Model   Data\\Tools\\dig.lwo\n"
        "  }\n"
        "}\n");

    if (chdir(FIX) != 0) {
        fprintf(stderr, "chdir failed\n");
        return 1;
    }
    fixture_roundtrip(); /* s_failures drives the exit code below */

    /* Back out of the fixture dir so the probe sees the shipped Lego.cfg
     * (and its wad mount) rather than the loose fixture. */
    chdir("..");

    for (int a = 1; a < argc; ++a) {
        if (strcmp(argv[a], "--real") == 0 && a + 1 < argc) {
            ol_probe(argv[a + 1]);
            ++a;
        }
    }

    if (s_failures != 0) {
        fprintf(stderr, "entity_test: %d failure(s)\n", s_failures);
        return 1;
    }
    printf("entity_test: all pass\n");
    return 0;
}