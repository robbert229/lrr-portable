// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_files — Files.c's C library streams on SDL3 ([library:filesystem],
 * as isle-portable's LegoFile) and the path mapping every file call goes
 * through (files_native_host_path, isle-portable's
 * MxString::MapPathToFilesystem), on a fixture data directory. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <set>
#include <string>

#include "gods98/files.h"
#include "gods98/files_native.h"

/* Files.c's File_CheckRedundantFiles list (common symbols). */
extern "C" char fileRedundantList[2000][1024];
extern "C" unsigned int fileRedundantListCount;

static const char* FIX = "files_fixture";
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
        printf("FAIL: the test exited early (Files.c start-up found no Data directory?)\n");
        fflush(stdout);
        _exit(1);
    }
}

static void write_text(const char* path, const char* text)
{
    FILE* fp = fopen(path, "wb");
    fputs(text, fp);
    fclose(fp);
}

/* A Windows path maps to the host's: separators, and case per component. */
static void test_mapping(void)
{
    char host[512];

    mkdir("Data/Levels", 0755);
    write_text("Data/Levels/Map.txt", "x");
    /* A path that exists as spelled is kept, so on a disk that ignores case
     * (macOS's default) the units' spelling stays. */
    const char* expected = access("DATA/LEVELS/MAP.TXT", F_OK) == 0 ? "DATA/LEVELS/MAP.TXT" : "Data/Levels/Map.txt";
    CHECK(files_native_host_path(host, "DATA\\LEVELS\\MAP.TXT", sizeof(host)) != NULL &&
              strcmp(host, expected) == 0,
          "map: separators and case follow the disk");
    CHECK(files_native_host_path(host, "Data\\New\\File.txt", sizeof(host)) != NULL &&
              strcmp(host, "Data/New/File.txt") == 0,
          "map: a missing component keeps its spelling");
    CHECK(files_native_host_path(host, NULL, sizeof(host)) == NULL, "map: no path, no result");
    CHECK(files_native_host_path(host, "Data\\Levels\\Map.txt", 4) == NULL,
          "map: a result that does not fit is NULL");

    /* _mkdir maps too, and makes missing parents. */
    CHECK(files_native_mkdir("Data\\Made\\Deep") == 0, "map: _mkdir succeeds");
    struct stat st;
    CHECK(stat("Data/Made/Deep", &st) == 0 && S_ISDIR(st.st_mode), "map: _mkdir makes Data/Made/Deep");
}

/* _findfirst/_findnext over one directory, with Windows wildcards. */
static void test_find(void)
{
    struct _finddata_t entry;
    std::set<std::string> names;
    int dirs = 0;
    long handle;

    mkdir("Data/Find", 0755);
    mkdir("Data/Find/Sub", 0755);
    write_text("Data/Find/a.txt", "12345");
    write_text("Data/Find/B.TXT", "1");
    write_text("Data/Find/c.dat", "");

    handle = _findfirst("data\\find\\*.*", &entry);
    CHECK(handle != -1, "find: *.* finds the entries, the directory matched without case");
    if (handle != -1) {
        do {
            names.insert(entry.name);
            if (entry.attrib & _A_SUBDIR) dirs++;
            if (strcmp(entry.name, "a.txt") == 0) {
                CHECK(entry.size == 5, "find: the size is the file's");
            }
        } while (_findnext(handle, &entry) == 0);
        CHECK(_findclose(handle) == 0, "find: the search closes");
    }
    CHECK(names.size() == 4 && names.count("Sub") && names.count("a.txt") &&
              names.count("B.TXT") && names.count("c.dat"),
          "find: every entry, no . or ..");
    CHECK(dirs == 1, "find: the subdirectory has _A_SUBDIR");

    names.clear();
    handle = _findfirst("Data\\Find\\*.txt", &entry);
    if (handle != -1) {
        do names.insert(entry.name);
        while (_findnext(handle, &entry) == 0);
        _findclose(handle);
    }
    CHECK(names.size() == 2 && names.count("a.txt") && names.count("B.TXT"),
          "find: *.txt matches without case");
    CHECK(_findfirst("Data\\Find\\*.none", &entry) == -1, "find: no match is -1");
    CHECK(_findfirst("Data\\Missing\\*.*", &entry) == -1, "find: a missing directory is -1");
    CHECK(_findnext(12345, &entry) == -1 && _findclose(12345) == -1, "find: an unknown handle fails");
}

/* The stream calls on a C library (SDL3) file. */
static void test_streams(void)
{
    File* f;
    char line[64];
    char buf[16];
    int i;

    f = file_open("Stream.txt", "w");
    CHECK(f != NULL && file_get_system(f) == FileSystem_Std, "streams: a write opens a std stream");
    if (f == NULL) return;
    file_printf(f, "first %d\n", 42);
    CHECK(file_write("second\nthird", 1, 12, f) == 12, "streams: fwrite counts items");
    CHECK(file_write("abcdef", 3, 2, f) == 2, "streams: fwrite counts whole items");
    CHECK(file_tell(f) == 9 + 12 + 6, "streams: the position follows the writes");
    file_close(f);

    f = file_open("STREAM.TXT", "r");
    CHECK(f != NULL, "streams: a read finds the file without case");
    if (f == NULL) return;
    CHECK(file_length(f) == 27 && file_tell(f) == 0, "streams: the length, the position kept");
    CHECK(file_gets(line, sizeof(line), f) != NULL && strcmp(line, "first 42\n") == 0,
          "streams: fgets keeps the newline");
    CHECK(file_getc(f) == 's', "streams: fgetc reads a byte");
    CHECK(file_seek(f, -1, SEEK_CUR) == 0 && file_getc(f) == 's', "streams: SEEK_CUR");
    CHECK(file_gets(line, 4, f) != NULL && strcmp(line, "eco") == 0,
          "streams: fgets stops at num - 1 bytes");
    CHECK(file_seek(f, 0, SEEK_END) == 0 && file_tell(f) == 27, "streams: SEEK_END");
    CHECK(file_getc(f) == EOF, "streams: fgetc at the end is EOF");
    CHECK(file_gets(line, sizeof(line), f) == NULL, "streams: fgets at the end is NULL");
    CHECK(file_seek(f, 21, SEEK_SET) == 0, "streams: SEEK_SET");
    CHECK(file_read(buf, 4, 3, f) == 1, "streams: fread counts whole items");
    CHECK(memcmp(buf, "abcdef", 6) == 0, "streams: the bytes read");
    file_close(f);

    /* File_Load reads a loose file whole. */
    unsigned int size = 0;
    char* data = (char*)file_load("Stream.txt", &size, 1);
    CHECK(data != NULL && size == 27 && memcmp(data, "first 42\n", 9) == 0, "streams: File_Load");
    free(data);

    CHECK(file_exists("stream.txt"), "streams: File_Exists without case");
    CHECK(!file_exists("Nothing.txt"), "streams: File_Exists of a missing file");
    CHECK(file_open("Nothing.txt", "r") == NULL, "streams: a missing file does not open");

    /* File_CheckRedundantFiles reads the log's words (fscanf "%s"). */
    write_text("Data/Redundant.log", "  one\ttwo\n\nthree  ");
    file_check_redundant_files("Redundant.log");
    CHECK(fileRedundantListCount == 3 && strcmp(fileRedundantList[0], "one") == 0 &&
              strcmp(fileRedundantList[1], "two") == 0 && strcmp(fileRedundantList[2], "three") == 0,
          "streams: the redundant-file log's words");
    for (i = 0; i < 3; i++) fileRedundantList[i][0] = '\0';
}


int main(void)
{
    char cmd[128];

    atexit(exit_guard);
    snprintf(cmd, sizeof(cmd), "rm -rf %s", FIX);
    if (system(cmd) != 0) return 1;
    mkdir(FIX, 0755);
    if (chdir(FIX) != 0) {
        printf("FAIL: chdir %s\n", FIX);
        s_finished = 1;
        return 1;
    }
    mkdir("Data", 0755);
    /* No WAD: the start-up finds the install by its Data directory
     * (_findfirst), then removes a stale delme.dat. */
    write_text("Data/delme.dat", "");
    file_initialise("files_test", 0, "");
    CHECK(access("Data/delme.dat", F_OK) != 0, "init: delme.dat is removed");

    test_mapping();
    test_find();
    test_streams();

    s_finished = 1;
    if (s_failures != 0) {
        printf("files_test: %d failure(s)\n", s_failures);
        return 1;
    }
    printf("files_test: OK\n");
    return 0;
}
