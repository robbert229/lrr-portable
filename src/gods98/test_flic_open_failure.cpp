// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_flic_open_failure — Flic_Setup on a .flh that cannot be opened. The
 * front end's menu overlays, the reward screen's flics, the pointers and the
 * panels hold the pointer Flic_Setup fills and animate it when it is not
 * NULL. The original (and the port) returned 0 with that pointer still the
 * half-built, zeroed FLICSTRUCT, so a game directory without its
 * Data/AVI/*.flh crashed in Flic_Animate's call through the struct's null
 * surface a few seconds after the main menu (issue #983; a wasm trap on the
 * web build). A failed open now leaves the pointer NULL, as Flic_Close and
 * every caller's NULL test expect. Synthetic data only. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <vector>

#include "platform/native/test_fixture.h"
#include "gods98/files.h"
/* Flic_Setup links under its port name (flic_link_names.h). */
#define Flic_Setup flic_setup
#include "gods98/flic_unit.h"

static const char* FIX = "flic_open_failure_fixture";
static int s_failures = 0;
static int s_finished = 0;

static void CHECK(int cond, const char* what)
{
    if (!cond) {
        printf("FAIL: %s\n", what);
        ++s_failures;
    }
}

static void exit_guard(void)
{
    if (!s_finished) {
        printf("FAIL: the test exited early\n");
        fflush(stdout);
        _Exit(1);
    }
}

/* A file of `size` bytes whose 128-byte header says `declared` bytes and
 * carries `magic`. */
static void write_flic(const char* path, uint32_t declared, uint16_t magic,
                       size_t size)
{
    std::vector<unsigned char> bytes(size, 0);
    FILE* fp = fopen(path, "wb");

    memcpy(&bytes[0], &declared, sizeof(declared));
    memcpy(&bytes[4], &magic, sizeof(magic));
    fwrite(&bytes[0], 1, bytes.size(), fp);
    fclose(fp);
}

/* Every way a caller opens a flic: the front end's overlays stream (flags
 * 0); the pointers and panels hold the file in memory and loop (3). */
static void check_open_fails(const char* name, const char* what)
{
    static const unsigned int FLAGS[] = { 0, FLICMEMORY | FLICLOOPINGON };
    size_t i;

    for (i = 0; i < sizeof(FLAGS) / sizeof(FLAGS[0]); i++) {
        LPFLICSTRUCT flic = (LPFLICSTRUCT)(uintptr_t)0xdeadbeef;
        char msg[160];

        snprintf(msg, sizeof(msg), "%s (flags %u): Flic_Setup reports failure", what, FLAGS[i]);
        CHECK(Flic_Setup(name, &flic, FLAGS[i]) == 0, msg);
        snprintf(msg, sizeof(msg), "%s (flags %u): the caller's pointer is NULL", what, FLAGS[i]);
        CHECK(flic == NULL, msg);
        /* What FrontEnd.c, Reward.c and Pointer.c then do with it. */
        snprintf(msg, sizeof(msg), "%s (flags %u): closing the NULL pointer is safe", what, FLAGS[i]);
        CHECK(Flic_Close(flic) == 0, msg);
    }
}

int main(void)
{
    atexit(exit_guard);
    if (!fixture_enter(FIX)) {
        printf("FAIL: chdir %s\n", FIX);
        s_finished = 1;
        return 1;
    }
    fixture_mkdir("Data");
    /* No WAD: the file layer's start-up finds the install by its Data
     * directory. */
    {
        FILE* fp = fopen("Data/delme.dat", "wb");
        fclose(fp);
    }
    file_initialise("flic_open_failure_test", 0, "");

    /* The file is not there, as a game directory without its .flh files. */
    check_open_fails("Data\\AVI\\Missing.flh", "missing file");

    /* The header's length is not the file's. */
    write_flic("Data/Short.flh", 4096, 0xaf43, 200);
    check_open_fails("Data\\Short.flh", "wrong length");

    /* The right length, but not a flic's magic. */
    write_flic("Data/Bad.flh", 200, 0x1111, 200);
    check_open_fails("Data\\Bad.flh", "wrong magic");

    s_finished = 1;
    if (s_failures != 0) {
        printf("flic_open_failure: %d failure(s)\n", s_failures);
        return 1;
    }
    printf("flic_open_failure: ok\n");
    return 0;
}
