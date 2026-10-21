// SPDX-License-Identifier: LGPL-3.0-or-later
// Entity table + `.ol` object list (finding 10 / 07).
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "game/entity.hpp"
#include "game/game.hpp"
#include "gods98/block.hpp"

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

int main(void)
{
    char rmcmd[64]; snprintf(rmcmd, sizeof(rmcmd), "rm -rf %s", FIX); system(rmcmd);
    mkdir(FIX, 0755);
    write_fixture("Lego.cfg",
        "Lego {\n"
        "  Level01 {\n"
        "    olistfile M01.ol\n"
        "  }\n"
        "}\n");
    write_fixture("M01.ol",
        "LegoOl {\n"
        "  Digger1 { Type 1 x 3 y 4 z 5 Rot 90 Model Data\\Models\\dig.lwo }\n"
        "  WallA   { Type 4 x 10 y 10 }\n"
        "  MinifigA { Type 2 x -2 y 0 }\n"
        "}\n");

    if (chdir(FIX) != 0) {
        fprintf(stderr, "chdir failed\n");
        return 1;
    }

    BlockFile* cfg = block_open("Lego.cfg");
    CHECK(cfg != NULL, "open cfg");
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
    CHECK(v != NULL && v->x == 3.0f && v->y == 4.0f && v->z == 5.0f, "vehicle pos");
    CHECK(v != NULL && v->rot == 90.0f, "vehicle rot");
    CHECK(v != NULL && v->type == ENTITY_TYPE_VEHICLE, "vehicle type");
    /* Optional Model key parsed into the slot (model loads lazily; the
     * fixture file does not exist so the handle stays NULL). */
    CHECK(strcmp(v->model_path, "Data\\Models\\dig.lwo") == 0,
          "vehicle model path from the .ol Model key");
    CHECK(entity_model(v) == NULL, "missing model file -> no handle");

    const EntitySlot* b = entity_at(&ctx, ENTITY_TYPE_BUILDING, 0);
    CHECK(b != NULL && strcmp(b->name, "WallA") == 0, "building name");
    CHECK(b != NULL && b->x == 10.0f && b->y == 10.0f, "building pos");

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
    if (s_failures != 0) {
        fprintf(stderr, "entity_test: %d failure(s)\n", s_failures);
        return 1;
    }
    printf("entity_test: OK\n");
    return 0;
}
