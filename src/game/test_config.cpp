// SPDX-License-Identifier: LGPL-3.0-or-later
// Block-text config DB (finding 12) — synthetic fixture coverage.
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "gods98/config.hpp"

static int s_failures = 0;

#define CHECK(cond, msg)                                                       \
    do {                                                                       \
        if (!(cond)) {                                                         \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, msg);       \
            ++s_failures;                                                      \
        }                                                                      \
    } while (0)

static void write_fixture(const char* dir, const char* name, const char* body)
{
    char path[512];
    snprintf(path, sizeof(path), "%s/%s", dir, name);
    FILE* fp = fopen(path, "w");
    if (fp == NULL) {
        fprintf(stderr, "cannot write %s\n", path);
        return;
    }
    fputs(body, fp);
    fclose(fp);
}

int main(void)
{
    char dir[512];
    snprintf(dir, sizeof(dir), "config_fixture");
    mkdir(dir, 0755);

    write_fixture(dir, "Lego.cfg",
        "Lego {\n"
        "  ; comment line\n"
        "  Level01 {\n"
        "    blocksize 4\n"
        "    digdepth 3\n"
        "    useroof 1\n"
        "    truth TRUE\n"
        "    f FALSE\n"
        "    SurfaceMap Levels\\Level01.map\n"
        "    Reward {\n"
        "      Enable TRUE\n"
        "      Modifier 2\n"
        "      Timer 60\n"
        "    }\n"
        "  }\n"
        "  VehicleTypes {\n"
        "    SmallDigger { name VD1 }\n"
        "    BigTruck   { name VT2 }\n"
        "  }\n"
        "}\n");

    if (chdir(dir) != 0) {
        fprintf(stderr, "chdir %s failed\n", dir);
        return 1;
    }

    ConfigDb* cfg = config_open("Lego.cfg");
    CHECK(cfg != NULL, "open Lego.cfg");
    if (cfg == NULL) {
        return 1;
    }

    /* Wrapper unwrap: no root called "Lego" is exposed; Level01 is one. */
    CHECK(config_roots(cfg) == 2, "roots = Level01, VehicleTypes");
    const ConfigNode* lvl = config_find(cfg, "Level01");
    CHECK(lvl != NULL, "find Level01");
    CHECK(strcmp(config_node_name(lvl), "Level01") == 0, "Level01 name");

    /* Typed getters. */
    CHECK(config_int(cfg, "Level01 blocksize", 0) == 4, "blocksize int");
    CHECK(config_float(cfg, "Level01 blocksize", 0.0f) == 4.0f, "blocksize float");
    CHECK(config_int(cfg, "Level01 useroof", 9) == 1, "useroof int");
    CHECK(config_int(cfg, "Level01 truth", 0) == 1, "TRUE -> 1");
    CHECK(config_int(cfg, "Level01 f", 1) == 0, "FALSE -> 0");
    CHECK(config_int(cfg, "Level01 missing", 42) == 42, "missing default");
    CHECK(strcmp(config_value(cfg, "Level01 SurfaceMap"), "Levels\\Level01.map") == 0,
          "string value");

    /* Multilevel path + children iteration. */
    const ConfigNode* reward = config_find(cfg, "Level01 Reward");
    CHECK(reward != NULL, "find Reward");
    CHECK(config_int(cfg, "Level01 Reward Enable", 0) == 1, "Reward Enable");
    CHECK(config_int(cfg, "Level01 Reward Modifier", 0) == 2, "Reward Modifier");
    CHECK(config_float(cfg, "Level01 Reward Timer", 0.0f) == 60.0f, "Reward Timer");

    const ConfigNode* vt = config_find(cfg, "VehicleTypes");
    CHECK(vt != NULL, "find VehicleTypes");
    CHECK(config_node_child_count(vt) == 2, "two vehicle groups");
    CHECK(strcmp(config_node_name(config_node_child(vt, 0)), "SmallDigger") == 0,
          "first vehicle name");
    CHECK(strcmp(config_value(cfg, "VehicleTypes BigTruck name"), "VT2") == 0,
          "nested value");

    /* Case-insensitive keys. */
    CHECK(config_int(cfg, "LEVEL01 BLOCKSIZE", 0) == 4, "case-insensitive path");

    config_close(cfg);

    if (s_failures != 0) {
        fprintf(stderr, "config_test: %d failure(s)\n", s_failures);
        return 1;
    }
    printf("config_test: OK\n");
    return 0;
}
