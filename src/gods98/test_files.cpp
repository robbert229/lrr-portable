// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_files — Files.c's C library streams on SDL3 ([library:filesystem],
 * as isle-portable's LegoFile) and the path mapping every file call goes
 * through (files_native_host_path, isle-portable's
 * MxString::MapPathToFilesystem), on a fixture data directory; and Wad.c's
 * archives, read through the same streams and mapping. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <filesystem>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "gods98/files.h"
#include "gods98/files_native.h"
#include "platform/native/test_fixture.h"

/* Files.c's File_CheckRedundantFiles list (common symbols; files.c builds
 * as C++, so they have C++ linkage). */
extern char fileRedundantList[2000][1024];
extern unsigned int fileRedundantListCount;

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
        _Exit(1);
    }
}

static void write_text(const char* path, const char* text)
{
    FILE* fp = fopen(path, "wb");
    fputs(text, fp);
    fclose(fp);
}

static void put32(FILE* fp, uint32_t v)
{
    unsigned char b[4] = { (unsigned char)v, (unsigned char)(v >> 8),
                           (unsigned char)(v >> 16), (unsigned char)(v >> 24) };
    fwrite(b, 1, 4, fp);
}

/* A WAD of stored (uncompressed) entries, name -> text. */
static void write_wad(const char* path, const std::vector<std::pair<std::string, std::string> >& entries)
{
    FILE* fp = fopen(path, "wb");
    uint32_t addr = 4 + 4 + 16 * (uint32_t)entries.size();
    size_t i;

    for (i = 0; i < entries.size(); i++) addr += 2 * (uint32_t)(entries[i].first.size() + 1);
    fwrite("WWAD", 1, 4, fp);
    put32(fp, (uint32_t)entries.size());
    for (i = 0; i < entries.size(); i++) fwrite(entries[i].first.c_str(), 1, entries[i].first.size() + 1, fp);
    for (i = 0; i < entries.size(); i++) fwrite(entries[i].first.c_str(), 1, entries[i].first.size() + 1, fp);
    for (i = 0; i < entries.size(); i++) {
        uint32_t len = (uint32_t)entries[i].second.size();
        put32(fp, 0); /* compression: stored */
        put32(fp, len);
        put32(fp, len);
        put32(fp, addr);
        addr += len;
    }
    for (i = 0; i < entries.size(); i++) fwrite(entries[i].second.data(), 1, entries[i].second.size(), fp);
    fclose(fp);
}

/* A Windows path maps to the host's: separators, and case per component. */
static void test_mapping(void)
{
    char host[512];

    fixture_mkdir("Data/Levels");
    write_text("Data/Levels/Map.txt", "x");
    /* A path that exists as spelled is kept, so on a disk that ignores case
     * (macOS's default) the units' spelling stays. */
    const char* expected =
        fixture_exists("DATA/LEVELS/MAP.TXT") ? "DATA/LEVELS/MAP.TXT" : "Data/Levels/Map.txt";
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
    CHECK(fixture_is_directory("Data/Made/Deep"), "map: _mkdir makes Data/Made/Deep");
}

/* _findfirst/_findnext over one directory, with Windows wildcards. */
static void test_find(void)
{
    struct _finddata_t entry;
    std::set<std::string> names;
    int dirs = 0;
    long handle;

    fixture_mkdir("Data/Find");
    fixture_mkdir("Data/Find/Sub");
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

/* Wad.c reads its archives through SDL3 streams on the host path: a name in
 * another case than the file's mounts, a corrupt archive is refused without
 * taking a record, and the entries read back from their offsets. */
static void test_wad(void)
{
    typedef std::vector<std::pair<std::string, std::string> > Entries;
    char buf[32];
    File* f;
    FILE* fp;
    unsigned int size = 0;

    /* Refused: no such file, not a WAD, too short, a count the file cannot
     * hold, names that end with the archive, a table cut short. */
    CHECK(file_load_wad("wad_missing0.wad") == -1, "wad: a missing archive is refused");
    write_text("wad_magic0.wad", "NOPEnope");
    CHECK(file_load_wad("wad_magic0.wad") == -1, "wad: a bad magic is refused");
    write_text("wad_short0.wad", "WWA");
    CHECK(file_load_wad("wad_short0.wad") == -1, "wad: a short header is refused");
    fp = fopen("wad_count0.wad", "wb");
    fwrite("WWAD", 1, 4, fp);
    put32(fp, 0x7fffffffu);
    fclose(fp);
    CHECK(file_load_wad("wad_count0.wad") == -1, "wad: an impossible entry count is refused");
    fp = fopen("wad_names0.wad", "wb");
    fwrite("WWAD", 1, 4, fp);
    put32(fp, 1);
    fputs("Packed.txt", fp); /* no NUL, nothing after */
    fclose(fp);
    CHECK(file_load_wad("wad_names0.wad") == -1, "wad: a name cut off by the end is refused");
    write_wad("wad_table0.wad", Entries(1, std::make_pair(std::string("Cut.txt"), std::string("cut"))));
    {
        std::error_code ec; /* cut the data and part of the record */
        std::filesystem::resize_file("wad_table0.wad", 4 + 4 + 8 * 2 + 8, ec);
        CHECK(!ec, "wad: the table's fixture is cut");
    }
    CHECK(file_load_wad("wad_table0.wad") == -1, "wad: a record cut short is refused");

    /* A refused archive takes no record: the good one mounts, by a name in
     * another case than the file's. */
    Entries entries;
    entries.push_back(std::make_pair(std::string("Wadded.txt"), std::string("wadded")));
    entries.push_back(std::make_pair(std::string("Sub\\Deep.txt"), std::string("deep down")));
    write_wad("WadCase0.WAD", entries);
    CHECK(file_load_wad("wadcase0.wad") != -1, "wad: the archive mounts by a name in another case");

    f = file_open("Wadded.txt", "r");
    CHECK(f != NULL && file_get_system(f) == FileSystem_Wad, "wad: the first entry opens from the archive");
    if (f != NULL) {
        memset(buf, 0, sizeof(buf));
        CHECK(file_read(buf, 1, 6, f) == 6 && strcmp(buf, "wadded") == 0, "wad: the first entry's bytes");
        file_close(f);
    }
    char* data = (char*)file_load("Sub\\Deep.txt", &size, 1);
    CHECK(data != NULL && size == 9 && memcmp(data, "deep down", 9) == 0,
          "wad: the second entry's bytes, from its offset");
    free(data);
    CHECK(file_open("Nothing.txt", "r") == NULL, "wad: a name no archive holds is not found");
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
    /* No WAD: the start-up finds the install by its Data directory
     * (_findfirst), then removes a stale delme.dat. */
    write_text("Data/delme.dat", "");
    file_initialise("files_test", 0, "");
    CHECK(!fixture_exists("Data/delme.dat"), "init: delme.dat is removed");

    test_mapping();
    test_find();
    test_streams();
    test_wad();

    s_finished = 1;
    if (s_failures != 0) {
        printf("files_test: %d failure(s)\n", s_failures);
        return 1;
    }
    printf("files_test: OK\n");
    return 0;
}
