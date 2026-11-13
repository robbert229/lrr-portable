// SPDX-License-Identifier: LGPL-3.0-or-later
/* Wad.c: original translation unit G98-030 (text 0048b4a0-0048c040,
 * literals 004af9e8-004af9f0, commons 00575500 and 005756a0; findings 505
 * and 591).
 *
 * The WWAD archive layer under Files.c. Wad_Load mounts an archive into the
 * first free of ten archive records: the "WWAD" magic, the entry count, the
 * archive-relative names, the data-directory names and a 16-byte record per
 * entry. Wad_FileOpen reads one entry into the first free of 100 handles,
 * expanding it when its RNC bit is set; Files.c's WAD streams read the
 * handle's data in memory.
 *
 * The native port does not run this unit: its WAD reader is the port tail,
 * gods98/wad_port.cpp and gods98/resource.cpp, under the port's names
 * (wad.h). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "crt/vc5names.h"

#include "files.h"
#include "wad.h"
#include "wad_payload.h"

#define MAX_WADS 10
#define MAX_FILES 100
#define WAD_ERROR -1

/* An entry's record in the archive's table. */
typedef struct WadEntry {
    unsigned int compression;        /* bit 1: RNC-packed */
    unsigned int fileLength;         /* bytes in the archive */
    unsigned int decompressedLength;
    unsigned int addr;               /* file offset */
} WadEntry;

/* A mounted archive. 0x24 bytes in the original; the three words that
 * nothing reads are kept for the layout. */
typedef struct Wad {
    int unused00;
    int active;
    int unused08;
    int unused0c;
    FILE* fWad;
    char** fileNames;      /* data-directory names */
    char** wadNames;       /* archive-relative names, searched by name */
    WadEntry* wadEntries;
    int numFiles;
} Wad;

/* An open entry: its data in memory. */
typedef struct WadFile {
    void* data;
    int active;
    int wadNo;
    int fileNo;
} WadFile;

int wad_get_file_name(FILE* f, char* buffer);
Wad* wad_get(int wadNo);
int wad_get_file_length(int wadNo, int fileNo);
int wad_get_file_compressed_length(int wadNo, int fileNo);
int wad_find_free_file_handle(void);
int wad_get_file_index(const char* fName, int wadNo);
int wad_get_free_wad(void);
int wad_file_open_in_wad(const char* fName, int wadNo);
void* wad_file_get_pointer(int handle);

/* Commons, as in the original. */
// GLOBAL: LEGORR 0x00575500
Wad wads[MAX_WADS];
// GLOBAL: LEGORR 0x005756a0
WadFile wadFiles[MAX_FILES];

/* GetFileName: a NUL-terminated name from the archive's name table. */
// FUNCTION: LEGORR 0x0048b4a0
int wad_get_file_name(FILE* f, char* buffer)
{
    char c;

    while ((c = (char)fgetc(f)) != 0) {
        *buffer++ = c;
    }
    *buffer = 0;
    return 1;
}

// FUNCTION: LEGORR 0x0048b4e0
int wad_load(const char* fName)
{
    FILE* f;
    char header[4];
    char buffer[4096];
    int i = 0;
    int j;
    int newWad = wad_get_free_wad();

    if (newWad != WAD_ERROR) {
        f = fopen(fName, "rb");
        wad_get(newWad)->fWad = f;
        if (f) {
            wad_get(newWad)->active = 1;
            if (fread(header, 1, 4, f) != 4) {
                fclose(f);
                return WAD_ERROR;
            }
            // STRING: LEGORR 0x004af9ec
            if (memcmp(header, "WWAD", 4)) {
                fclose(f);
                return WAD_ERROR;
            }
            if (fread(&wad_get(newWad)->numFiles, 1, 4, f) != 4) {
                fclose(f);
                return WAD_ERROR;
            }
            wad_get(newWad)->wadNames = (char**)malloc(sizeof(char*) * wad_get(newWad)->numFiles);
            wad_get(newWad)->fileNames = (char**)malloc(sizeof(char*) * wad_get(newWad)->numFiles);
            if (wad_get(newWad)->wadNames && wad_get(newWad)->fileNames) {
                memset(wad_get(newWad)->wadNames, 0, sizeof(char*) * wad_get(newWad)->numFiles);
                memset(wad_get(newWad)->fileNames, 0, sizeof(char*) * wad_get(newWad)->numFiles);

                for (; i < wad_get(newWad)->numFiles; i++) {
                    if (!wad_get_file_name(f, buffer)) {
                        if (wad_get(newWad)->wadNames) {
                            for (j = 0; j < wad_get(newWad)->numFiles; j++) {
                                if (wad_get(newWad)->wadNames[j])
                                    free(wad_get(newWad)->wadNames[j]);
                            }
                            free(wad_get(newWad)->wadNames);
                        }
                        if (wad_get(newWad)->fileNames)
                            free(wad_get(newWad)->fileNames);
                        fclose(f);
                        return WAD_ERROR;
                    }
                    wad_get(newWad)->wadNames[i] = (char*)malloc(strlen(buffer) + 1);
                    strcpy(wad_get(newWad)->wadNames[i], buffer);
                    // STRING: LEGORR 0x004af9e8
                    file_error_file("%s\n", wad_get(newWad)->wadNames[i]);
                }

                for (i = 0; i < wad_get(newWad)->numFiles; i++) {
                    if (!wad_get_file_name(f, buffer)) {
                        if (wad_get(newWad)->fileNames) {
                            for (j = 0; j < wad_get(newWad)->numFiles; j++) {
                                if (wad_get(newWad)->wadNames[j])
                                    free(wad_get(newWad)->wadNames[j]);
                            }
                            free(wad_get(newWad)->wadNames);
                        }
                        if (wad_get(newWad)->fileNames) {
                            for (j = 0; j < wad_get(newWad)->numFiles; j++) {
                                if (wad_get(newWad)->fileNames[j])
                                    free(wad_get(newWad)->fileNames[j]);
                            }
                            free(wad_get(newWad)->fileNames);
                        }
                        fclose(f);
                        return WAD_ERROR;
                    }
                    wad_get(newWad)->fileNames[i] = (char*)malloc(strlen(buffer) + 1);
                    strcpy(wad_get(newWad)->fileNames[i], buffer);
                    file_error_file("%s\n", wad_get(newWad)->fileNames[i]);
                }

                wad_get(newWad)->wadEntries = (WadEntry*)malloc(sizeof(WadEntry) * wad_get(newWad)->numFiles);
                if (!wad_get(newWad)->wadEntries) {
                    if (wad_get(newWad)->fileNames) {
                        for (j = 0; j < wad_get(newWad)->numFiles; j++) {
                            if (wad_get(newWad)->wadNames[j])
                                free(wad_get(newWad)->wadNames[j]);
                        }
                        free(wad_get(newWad)->wadNames);
                    }
                    if (wad_get(newWad)->fileNames) {
                        for (j = 0; j < wad_get(newWad)->numFiles; j++) {
                            if (wad_get(newWad)->fileNames[j])
                                free(wad_get(newWad)->fileNames[j]);
                        }
                        free(wad_get(newWad)->fileNames);
                    }
                    fclose(f);
                    return WAD_ERROR;
                }
                if (fread(wad_get(newWad)->wadEntries, 1, sizeof(WadEntry) * wad_get(newWad)->numFiles, f) !=
                    sizeof(WadEntry) * wad_get(newWad)->numFiles) {
                    if (wad_get(newWad)->fileNames) {
                        for (j = 0; j < wad_get(newWad)->numFiles; j++) {
                            if (wad_get(newWad)->wadNames[j])
                                free(wad_get(newWad)->wadNames[j]);
                        }
                        free(wad_get(newWad)->wadNames);
                    }
                    if (wad_get(newWad)->fileNames) {
                        for (j = 0; j < wad_get(newWad)->numFiles; j++) {
                            if (wad_get(newWad)->fileNames[j])
                                free(wad_get(newWad)->fileNames[j]);
                        }
                        free(wad_get(newWad)->fileNames);
                    }
                    if (wad_get(newWad)->wadEntries)
                        free(wad_get(newWad)->wadEntries);
                    fclose(f);
                    return WAD_ERROR;
                }
                return newWad;
            } else {
                fclose(f);
                if (wad_get(newWad)->wadNames)
                    free(wad_get(newWad)->wadNames);
                if (wad_get(newWad)->fileNames)
                    free(wad_get(newWad)->fileNames);
            }
        }
    }
    return WAD_ERROR;
}

/* GetWad. */
// FUNCTION: LEGORR 0x0048bce0
Wad* wad_get(int wadNo)
{
    return &wads[wadNo];
}

/* An entry's uncompressed length. */
// FUNCTION: LEGORR 0x0048bcf0
int wad_get_file_length(int wadNo, int fileNo)
{
    return wad_get(wadNo)->wadEntries[fileNo].decompressedLength;
}

/* An entry's length in the archive. */
// FUNCTION: LEGORR 0x0048bd10
int wad_get_file_compressed_length(int wadNo, int fileNo)
{
    return wad_get(wadNo)->wadEntries[fileNo].fileLength;
}

// FUNCTION: LEGORR 0x0048bd30
int wad_find_free_file_handle(void)
{
    int i;

    for (i = 0; i < MAX_FILES; i++) {
        if (!wadFiles[i].active)
            return i;
    }
    return WAD_ERROR;
}

// FUNCTION: LEGORR 0x0048bd50
int wad_is_file_in_wad(const char* fName, int wadNo)
{
    int i, index;

    if (wadNo == WAD_ERROR) {
        for (i = MAX_WADS - 1; i >= 0; i--) {
            if (wad_get(i)->active) {
                if ((index = wad_get_file_index(fName, i)) != WAD_ERROR)
                    return index;
            }
        }
        return WAD_ERROR;
    }
    return wad_get_file_index(fName, wadNo);
}

/* The entry's index in one archive, by archive-relative name. */
// FUNCTION: LEGORR 0x0048bda0
int wad_get_file_index(const char* fName, int wadNo)
{
    int i;

    for (i = 0; i < wads[wadNo].numFiles; i++) {
        if (!_strcmpi(wads[wadNo].wadNames[i], fName))
            return i;
    }
    return WAD_ERROR;
}

// FUNCTION: LEGORR 0x0048be00
int wad_get_free_wad(void)
{
    int i;

    for (i = 0; i < MAX_WADS; i++) {
        if (!wads[i].active) {
            memset(&wads[i], 0, sizeof(Wad));
            return i;
        }
    }
    return WAD_ERROR;
}

/* _Wad_FileOpen: read the entry from one archive. */
// FUNCTION: LEGORR 0x0048be40
int wad_file_open_in_wad(const char* fName, int wadNo)
{
    int handle, fileNo;
    void* data;

    if ((handle = wad_find_free_file_handle()) == WAD_ERROR)
        return WAD_ERROR;
    if ((fileNo = wad_is_file_in_wad(fName, wadNo)) == WAD_ERROR)
        return WAD_ERROR;
    if ((data = malloc(wad_get_file_compressed_length(wadNo, fileNo))) == NULL)
        return WAD_ERROR;
    fseek(wad_get(wadNo)->fWad, wad_get(wadNo)->wadEntries[fileNo].addr, SEEK_SET);
    if (fread(data, wad_get_file_compressed_length(wadNo, fileNo), 1, wad_get(wadNo)->fWad) != 1) {
        free(data);
        return WAD_ERROR;
    }
    if (wad_get(wadNo)->wadEntries[fileNo].compression & 2) {
        void* dest = NULL;
        wad_expand_payload(data, &dest);
        free(data);
        data = dest;
    }
    wadFiles[handle].data = data;
    wadFiles[handle].active = 1;
    wadFiles[handle].wadNo = wadNo;
    wadFiles[handle].fileNo = fileNo;
    return handle;
}

// FUNCTION: LEGORR 0x0048bf70
int wad_file_open(const char* fName, int wadNo)
{
    int i, handle;

    if (wadNo == WAD_ERROR) {
        for (i = MAX_WADS - 1; i >= 0; i--) {
            if (wad_get(i)->active) {
                if ((handle = wad_file_open_in_wad(fName, i)) != WAD_ERROR)
                    return handle;
            }
        }
        return WAD_ERROR;
    }
    return wad_file_open_in_wad(fName, wadNo);
}

// FUNCTION: LEGORR 0x0048bfc0
void wad_file_close(int handle)
{
    if (wadFiles[handle].active) {
        free(wadFiles[handle].data);
        wadFiles[handle].active = 0;
    }
}

// FUNCTION: LEGORR 0x0048bff0
void* wad_file_get_pointer(int handle)
{
    if (wadFiles[handle].active)
        return wadFiles[handle].data;
    return NULL;
}

/* Wad_hLength. */
// FUNCTION: LEGORR 0x0048c010
int wad_file_length(int handle)
{
    return wad_get_file_length(wadFiles[handle].wadNo, wadFiles[handle].fileNo);
}

/* Wad_hData. */
// FUNCTION: LEGORR 0x0048c030
void* wad_file_data(int handle)
{
    return wad_file_get_pointer(handle);
}
