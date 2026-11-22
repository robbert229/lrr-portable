// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_diskpath — Files.c's start-up and file calls with the data directory
 * taken from the configuration's diskpath ([library:config], as
 * isle-portable's isle:diskpath) instead of the working directory: the
 * WADs mount from it, its Data directory is the base, and a name opens
 * there whatever the working directory is. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <string>

#include "gods98/files.h"
#include "gods98/files_native.h"
#include "platform/native/test_fixture.h"

static const char* FIX = "diskpath_fixture";
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
        printf("FAIL: the test exited early (Files.c start-up found no data in diskpath?)\n");
        fflush(stdout);
        _Exit(1);
    }
}

static void write_text(const std::string& path, const char* text)
{
    FILE* fp = fopen(path.c_str(), "wb");
    fputs(text, fp);
    fclose(fp);
}

static void put32(FILE* fp, uint32_t v)
{
    unsigned char b[4] = { (unsigned char)v, (unsigned char)(v >> 8),
                           (unsigned char)(v >> 16), (unsigned char)(v >> 24) };
    fwrite(b, 1, 4, fp);
}

/* A WAD holding one stored (uncompressed) entry, `name` -> `text`. */
static void write_wad(const std::string& path, const char* name, const char* text)
{
    FILE* fp = fopen(path.c_str(), "wb");
    uint32_t len = (uint32_t)strlen(text);
    uint32_t names = (uint32_t)strlen(name) + 1;
    uint32_t addr = 4 + 4 + names * 2 + 16;

    fwrite("WWAD", 1, 4, fp);
    put32(fp, 1);
    fwrite(name, 1, names, fp);  /* the archive-relative name */
    fwrite(name, 1, names, fp);  /* the source file name */
    put32(fp, 0);                /* compression: stored */
    put32(fp, len);
    put32(fp, len);
    put32(fp, addr);
    fwrite(text, 1, len, fp);
    fclose(fp);
}

int main(void)
{
    char buf[64];
    File* f;

    atexit(exit_guard);
    if (!fixture_enter(FIX)) {
        printf("FAIL: chdir %s\n", FIX);
        s_finished = 1;
        return 1;
    }
    const std::string root = fixture_cwd();
    const std::string install = root + "/Install";

    /* The install: a Data directory and a WAD, neither in the working
     * directory (Run), which holds nothing. */
    fixture_mkdir(install);
    fixture_mkdir(install + "/Data");
    fixture_mkdir(root + "/Run");
    write_text(install + "/Data/Loose.txt", "loose");
    write_text(install + "/Data/delme.dat", "");
    write_wad(install + "/diskpath_test0.wad", "Packed.txt", "packed");
    if (!fixture_chdir("Run")) {
        printf("FAIL: chdir Run\n");
        s_finished = 1;
        return 1;
    }

    /* A trailing separator is dropped, as for the save path. */
    files_native_set_disk_path((install + "/").c_str());
    CHECK(files_native_disk_path() != NULL && install == files_native_disk_path(),
          "diskpath: the directory is kept without its trailing separator");

    /* A path with a scheme or drive (3DS and Switch "sdmc:/...") is
     * absolute: the diskpath is not added to it a second time. */
    char host[1024];
    files_native_set_disk_path("sdmc:/3ds/LegoRR");
    CHECK(files_native_host_path(host, "sdmc:/3ds/LegoRR\\Data\\Lego.cfg", sizeof(host)) != NULL &&
              strcmp(host, "sdmc:/3ds/LegoRR/Data/Lego.cfg") == 0,
          "map: a scheme path is absolute");
    CHECK(files_native_host_path(host, "Data\\Lego.cfg", sizeof(host)) != NULL &&
              strcmp(host, "sdmc:/3ds/LegoRR/Data/Lego.cfg") == 0,
          "map: a relative path goes under a scheme diskpath");

    /* A diskpath in another case takes the disk's spelling when it is set,
     * on a disk that tells cases apart, so that file paths inside it need
     * not be matched again. */
    if (!fixture_exists(root + "/INSTALL")) {
        files_native_set_disk_path((root + "/INSTALL").c_str());
        CHECK(files_native_disk_path() != NULL && install == files_native_disk_path(),
              "diskpath: the directory takes the disk's spelling");
        CHECK(files_native_host_path(host, (install + "\\DATA\\loose.TXT").c_str(), sizeof(host)) != NULL &&
                  install + "/Data/Loose.txt" == host,
              "map: a path inside the diskpath still takes the disk's spelling");
    }

    /* A relative diskpath is taken from the working directory once, when
     * it is set: the start-up below runs from Run with "../Install". */
    files_native_set_disk_path("../Install");
    CHECK(files_native_disk_path() != NULL && install == files_native_disk_path(),
          "diskpath: a relative directory is made absolute");

    file_initialise("diskpath_test", 0, "");
    CHECK(!fixture_exists(install + "/Data/delme.dat"),
          "init: delme.dat is removed from the diskpath's Data");
    CHECK(!fixture_exists("Data"), "init: no Data directory is made in the working directory");

    f = file_open("Loose.txt", "r");
    CHECK(f != NULL && file_get_system(f) == FileSystem_Std,
          "open: a loose file opens from the diskpath's Data");
    if (f != NULL) {
        memset(buf, 0, sizeof(buf));
        CHECK(file_read(buf, 1, 5, f) == 5 && strcmp(buf, "loose") == 0, "open: the loose file's bytes");
        file_close(f);
    }

    f = file_open("Packed.txt", "r");
    CHECK(f != NULL && file_get_system(f) == FileSystem_Wad,
          "open: the diskpath's WAD is mounted");
    if (f != NULL) {
        memset(buf, 0, sizeof(buf));
        CHECK(file_read(buf, 1, 6, f) == 6 && strcmp(buf, "packed") == 0, "open: the packed file's bytes");
        file_close(f);
    }

    /* A unit's own relative Windows path ("Data\\..."; Images.c, the CD
     * key check) maps into the diskpath too; on a disk that ignores case
     * the units' spelling stays (test_files). */
    const std::string loose = fixture_exists(install + "/DATA/LOOSE.TXT") ?
        install + "/DATA/LOOSE.TXT" : install + "/Data/Loose.txt";
    CHECK(files_native_host_path(host, "DATA\\LOOSE.TXT", sizeof(host)) != NULL &&
              loose == host,
          "map: a relative path is taken from the diskpath");

    /* An empty diskpath keeps the working directory. */
    files_native_set_disk_path("");
    CHECK(files_native_disk_path() == NULL, "diskpath: empty keeps the working directory");
    CHECK(files_native_host_path(host, "Data\\Loose.txt", sizeof(host)) != NULL &&
              strcmp(host, "Data/Loose.txt") == 0,
          "map: without a diskpath a relative path stays relative");

    s_finished = 1;
    if (s_failures != 0) {
        printf("diskpath_test: %d failure(s)\n", s_failures);
        return 1;
    }
    printf("diskpath_test: OK\n");
    return 0;
}
