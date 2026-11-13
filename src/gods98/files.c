// SPDX-License-Identifier: LGPL-3.0-or-later
/* Files.c: original translation unit G98-014 (text 0047f130-004805b0, .data
 * 004aaf00, literals 004aaf04-004ab100, .bss 005339a0-005343bc; findings
 * 505 and 583).
 *
 * The gods98 file layer. File_Open picks the archive or the C library for
 * each name: a name under the data directory that a mounted WAD holds is read
 * from the WAD through Wad.c's stream handles; anything else, and every write,
 * goes to a C library stream, with the CD's data directory as the fallback.
 * File_Initialise mounts <program>0.wad to <program>9.wad, finds the CD by
 * its cd.key and checks the install; File_CheckRedundantFiles lists the files
 * under the data directory against a log.
 *
 * The native port does not run this unit: its file layer is the port tail,
 * gods98/files_port.cpp, which native callers reach by the port's names
 * (files.h explains the naming). */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#include <windows.h>
#include <direct.h>
#include <io.h>
#else
#include "files_native.h"
#endif

#include "crt/vc5names.h"

#include "files.h"
#include "wad.h"
#include "utils.h"
#include "g98_024.h"
#include "g98_029.h"

#ifdef __cplusplus
/* PORT-ONLY: the original calls an empty variadic function, folded into
 * 00484c90, with its debug messages. C++ cannot call the unprototyped
 * declaration below with arguments. */
extern "C" void files_debug_noop(const char* msg, ...);
#define flic_opcode27_fallback_noop files_debug_noop
#else
void flic_opcode27_fallback_noop();
#endif

#define FILE_MAXPATH 260
#define FILE_MAXWADS 10

typedef struct File_Globs {
    char basePath[1024];               /* 005339a0 File_SetBaseSearchPath */
    char wadName[1024];                /* 00533da0 _File_GetWadName */
    char fullName[FILE_MAXPATH];       /* 005341a0 File_VerifyFilename */
    unsigned int reserved;             /* 005342a4 */
    char dataDir[FILE_MAXPATH];        /* 005342a8 */
    FileLoadCallback loadCallback;     /* 005343ac */
    void* loadCallbackData;            /* 005343b0 */
    unsigned char cdLetter;            /* 005343b4 */
    int basePathSet;                   /* 005343b8 */
} File_Globs;

// GLOBAL: LEGORR 0x004aaf00
static int currWadHandle = -1;

// GLOBAL: LEGORR 0x005339a0
static File_Globs fileGlobs;

/* File_CheckRedundantFiles's file list: common symbols. */
// GLOBAL: LEGORR 0x005769e0
char fileRedundantList[2000][1024];
// GLOBAL: LEGORR 0x0076a9e0
unsigned int fileRedundantCount;

// FUNCTION: LEGORR 0x0047f130
void file_initialise(const char* programName, int insistOnCD,
                     const char* registryLocation)
{
    char cwd[FILE_MAXPATH];
    char dirBuf[FILE_MAXPATH];
    char path[1024];
    unsigned int i;
    int foundWad = 0;
    int foundCD;
    int failed = 0;
    FILE* fp;

    _getcwd(cwd, FILE_MAXPATH);
    if (cwd[strlen(cwd) - 1] == '\\') cwd[strlen(cwd) - 1] = '\0';
    // STRING: LEGORR 0x004a1cbc
    sprintf(fileGlobs.dataDir, "%s\\%s", cwd,
            // STRING: LEGORR 0x004aaf5c
            "Data");

    for (i = 0; i < FILE_MAXWADS; i++) {
        char wadFile[FILE_MAXPATH];
        // STRING: LEGORR 0x004aaf50
        sprintf(wadFile, "%s%i.wad", programName, i);
        if (file_load_wad(wadFile) != -1) foundWad = 1;
    }

    _getcwd(dirBuf, FILE_MAXPATH);
    // STRING: LEGORR 0x004aaf48
    strcat(dirBuf, "\\Data");
    file_set_base_search_path(dirBuf);

    foundCD = file_find_data_cd();
    while (!foundCD && insistOnCD) {
        char err[1024];
        // STRING: LEGORR 0x004aaf40
        char title[1024] = "Error";
        // STRING: LEGORR 0x004aaf34
        if (registry_path_accessor(registryLocation, "CDMissing", 0, err,
                                   1024)) {
            // STRING: LEGORR 0x004aaf28
            registry_path_accessor(registryLocation, "SetupError", 0, title,
                                   1024);
            if (MessageBoxA(NULL, err, title, MB_OKCANCEL) == IDCANCEL)
                exit(0);
        } else {
            exit(0);
        }
        foundCD = file_find_data_cd();
    }

    if (!foundCD && !foundWad) {
        struct _finddata_t c_file;
        long hFile;
        failed = 1;
        // STRING: LEGORR 0x004aaf24
        if ((hFile = _findfirst("*.*", &c_file)) != -1) {
            do {
                if ((c_file.attrib & _A_SUBDIR) &&
                    _strcmpi(c_file.name, "Data") == 0) {
                    failed = 0;
                    break;
                }
            } while (_findnext(hFile, &c_file) == 0);
            _findclose(hFile);
        }
    }

    if (insistOnCD) {
        sprintf(path, "%s\\%s", "Data",
                // STRING: LEGORR 0x004aaf1c
                "cd.key");
        // STRING: LEGORR 0x004a1eac
        if ((fp = fopen(path, "r")) != NULL) {
            fclose(fp);
            failed = 1;
        }
    }

    if (!failed) {
        _mkdir("Data");
        sprintf(path, "%s\\%s", "Data",
                // STRING: LEGORR 0x004aaf10
                "delme.dat");
        if ((fp = fopen(path, "r")) != NULL) {
            fclose(fp);
            _chmod(path, 0x80);
            remove(path);
        }
    } else {
        char err[1024];
        char title[1024] = "Error";
        // STRING: LEGORR 0x004aaf04
        if (registry_path_accessor(registryLocation, "DataMissing", 0, err,
                                   1024)) {
            registry_path_accessor(registryLocation, "SetupError", 0, title,
                                   1024);
            MessageBoxA(NULL, err, title, MB_OK);
        }
        exit(0);
    }
}

// FUNCTION: LEGORR 0x0047f4f0
int file_find_data_cd(void)
{
    // STRING: LEGORR 0x004aaf70
    char drive[] = "A:\\";
    char path[FILE_MAXPATH];
    unsigned char letter;
    FILE* fp;

    for (letter = 'C'; letter <= 'Z'; letter++) {
        drive[0] = letter;
        if (GetDriveTypeA(drive) == DRIVE_CDROM) {
            // STRING: LEGORR 0x004aaf64
            sprintf(path, "%c:\\%s\\%s", letter, "Data", "cd.key");
            if ((fp = fopen(path, "r")) != NULL) {
                fclose(fp);
                fileGlobs.cdLetter = letter;
                return 1;
            }
        }
    }
    return 0;
}

// FUNCTION: LEGORR 0x0047f590
int file_set_base_search_path(const char* basePath)
{
    int len;

    if (basePath) {
        len = strlen(basePath);
        if (len <= 1024 && len != 0) {
            strcpy(fileGlobs.basePath, basePath);
            return fileGlobs.basePathSet = 1;
        }
    }
    memset(fileGlobs.basePath, 0, sizeof(fileGlobs.basePath));
    return fileGlobs.basePathSet = 0;
}

// FUNCTION: LEGORR 0x0047f600
void file_error_file(const char* msg, ...)
{
    char buffer[1024];
    va_list args;

    va_start(args, msg);
    vsprintf(buffer, msg, args);
    OutputDebugStringA(buffer);
    va_end(args);
}

// FUNCTION: LEGORR 0x0047f640
int file_load_wad(const char* fName)
{
    flic_opcode27_fallback_noop(0);
    return wad_load(fName);
}

// FUNCTION: LEGORR 0x0047f660
int file_get_cd_file_path(char* path, const char* fname)
{
    if (fileGlobs.cdLetter) {
        sprintf(path, "%c:\\%s\\%s", fileGlobs.cdLetter, "Data", fname);
        return 1;
    }
    return 0;
}

// FUNCTION: LEGORR 0x0047f6a0
void file_make_dir(const char* path)
{
    char buffer[FILE_MAXPATH];

    sprintf(buffer, "%s\\%s", "Data", path);
    _mkdir(buffer);
}

// FUNCTION: LEGORR 0x0047f6e0
File* file_open(const char* fName, const char* mode)
{
    char* fullName;
    FileSys fs;
    File* file;
    char cdName[FILE_MAXPATH];

    fullName = file_verify_filename(fName);
    fs = file_check_system(fullName, mode);
    switch (fs) {
    case FileSystem_Wad:
        if ((file = file_alloc(fs)) != NULL) {
            if (file_open_wad(file->stream.wad, file_get_wad_name(fullName))) {
                // STRING: LEGORR 0x004aaf84
                flic_opcode27_fallback_noop("WAD Load %s\n",
                                            file_get_wad_name(fullName));
                return file;
            }
            // STRING: LEGORR 0x004aaf74
            flic_opcode27_fallback_noop("WAD Fail %s\n",
                                        file_get_wad_name(fullName));
            file_dealloc(file);
        }
        break;
    case FileSystem_Std:
        if ((file = file_alloc(fs)) != NULL) {
            if ((file->stream.std = fopen(fullName, mode)) != NULL) {
                // STRING: LEGORR 0x004aafa4
                flic_opcode27_fallback_noop("STD Load %s\n", fullName);
                return file;
            }
            // STRING: LEGORR 0x004a52b8
            if (case_insensitive_substring_find(mode, "w") == NULL &&
                file_get_cd_file_path(cdName, fName) &&
                (file->stream.std = fopen(cdName, mode)) != NULL) {
                return file;
            }
            // STRING: LEGORR 0x004aaf94
            flic_opcode27_fallback_noop("STD Fail %s\n", fullName);
            file_dealloc(file);
        }
        break;
    default:
        // STRING: LEGORR 0x004aafb4
        file_error_file("%s(%i) : Error in call to %s\n",
                        // STRING: LEGORR 0x004aafd4
                        "C:\\Dev\\SourceSafe\\gods98_dx6\\gods98\\src\\Files.c",
                        // STRING: LEGORR 0x004ab004
                        330, "File_Open");
        break;
    }
    return NULL;
}

// FUNCTION: LEGORR 0x0047f850
int file_seek(File* f, int pos, int mode)
{
    FileSys fs;

    fs = file_get_system(f);
    switch (fs) {
    case FileSystem_Wad:
        switch (mode) {
        case SEEK_SET:
            f->stream.wad->streamPos = pos;
            if (f->stream.wad->streamPos > wad_file_length(f->stream.wad->hFile))
                f->stream.wad->streamPos = wad_file_length(f->stream.wad->hFile);
            if (f->stream.wad->streamPos < 0) f->stream.wad->streamPos = 0;
            break;
        case SEEK_CUR:
            f->stream.wad->streamPos += pos;
            if (f->stream.wad->streamPos > wad_file_length(f->stream.wad->hFile))
                f->stream.wad->streamPos = wad_file_length(f->stream.wad->hFile);
            if (f->stream.wad->streamPos < 0) f->stream.wad->streamPos = 0;
            break;
        case SEEK_END:
            f->stream.wad->streamPos =
                wad_file_length(f->stream.wad->hFile) + pos;
            break;
        default:
            // STRING: LEGORR 0x004ab010
            file_error_file("Uknown seek mode (%i)", mode);
            break;
        }
        return 0;
    case FileSystem_Std:
        return fseek(f->stream.std, pos, mode);
    default:
        // STRING: LEGORR 0x004ab028
        file_error_file("%s(%i) : Unknown file system in call to %s",
                        "C:\\Dev\\SourceSafe\\gods98_dx6\\gods98\\src\\Files.c",
                        // STRING: LEGORR 0x004ab054
                        368, "File_Seek");
        break;
    }
    return 0;
}

// FUNCTION: LEGORR 0x0047f980
int file_read(void* buffer, int size, int count, File* f)
{
    FileSys fs;
    int length;
    int read;
    int amount;

    fs = file_get_system(f);
    switch (fs) {
    case FileSystem_Wad:
        length = wad_file_length(f->stream.wad->hFile);
        read = size * count;
        if (read + f->stream.wad->streamPos > length)
            amount = length - f->stream.wad->streamPos;
        else
            amount = read;
        memcpy(buffer,
               (char*)wad_file_data(f->stream.wad->hFile) +
                   f->stream.wad->streamPos,
               amount);
        f->stream.wad->streamPos += amount;
        return amount / size;
    case FileSystem_Std:
        return fread(buffer, size, count, f->stream.std);
    default:
        file_error_file("%s(%i) : Unknown file system in call to %s",
                        "C:\\Dev\\SourceSafe\\gods98_dx6\\gods98\\src\\Files.c",
                        395, "");
        break;
    }
    return 0;
}

// FUNCTION: LEGORR 0x0047fa50
int file_write(const void* buffer, int size, int count, File* f)
{
    FileSys fs;

    fs = file_get_system(f);
    switch (fs) {
    case FileSystem_Wad:
        // STRING: LEGORR 0x004ab060
        file_error_file("Cannot write to a file stored in a Wad!");
        return 0;
    case FileSystem_Std:
        return fwrite(buffer, size, count, f->stream.std);
    default:
        file_error_file("%s(%i) : Unknown file system in call to %s",
                        "C:\\Dev\\SourceSafe\\gods98_dx6\\gods98\\src\\Files.c",
                        // STRING: LEGORR 0x004ab088
                        414, "File_Write");
        break;
    }
    return 0;
}

// FUNCTION: LEGORR 0x0047fac0
int file_close(File* f)
{
    FileSys fs;

    fs = file_get_system(f);
    switch (fs) {
    case FileSystem_Wad:
    case FileSystem_Std:
        file_dealloc(f);
        break;
    default:
        file_error_file("%s(%i) : Unknown file system in call to %s",
                        "C:\\Dev\\SourceSafe\\gods98_dx6\\gods98\\src\\Files.c",
                        // STRING: LEGORR 0x004ab094
                        431, "File_Close");
        break;
    }
    return 0;
}

// FUNCTION: LEGORR 0x0047fb10
int file_tell(File* f)
{
    FileSys fs;

    fs = file_get_system(f);
    switch (fs) {
    case FileSystem_Wad:
        return f->stream.wad->streamPos;
    case FileSystem_Std:
        return ftell(f->stream.std);
    default:
        file_error_file("%s(%i) : Unknown file system in call to %s",
                        "C:\\Dev\\SourceSafe\\gods98_dx6\\gods98\\src\\Files.c",
                        // STRING: LEGORR 0x004ab0a0
                        469, "File_Tell");
        break;
    }
    return 0;
}

// FUNCTION: LEGORR 0x0047fb60
int file_exists(const char* fName)
{
    char* fullName;
    FileSys fs;
    FILE* f;
    char cdName[FILE_MAXPATH];

    fullName = file_verify_filename(fName);
    fs = file_check_system(fullName, "r");
    if (fs == FileSystem_Std) {
        if ((f = fopen(fullName, "r")) != NULL) {
            fclose(f);
            return 1;
        }
        if (file_get_cd_file_path(cdName, fName)) {
            if ((f = fopen(cdName, "r")) != NULL) {
                fclose(f);
                return 1;
            }
        }
    } else if (fs == FileSystem_Wad) {
        return wad_is_file_in_wad(fullName, currWadHandle);
    }
    return 0;
}

// FUNCTION: LEGORR 0x0047fc20
int file_getc(File* f)
{
    FileSys fs;
    int len;

    fs = file_get_system(f);
    switch (fs) {
    case FileSystem_Wad:
        len = wad_file_length(f->stream.wad->hFile) - 1;
        if (f->stream.wad->streamPos >= len) return -1;
        return *(int*)((char*)wad_file_data(f->stream.wad->hFile) +
                       f->stream.wad->streamPos++);
    case FileSystem_Std:
        return fgetc(f->stream.std);
    default:
        file_error_file("%s(%i) : Unknown file system in call to %s",
                        "C:\\Dev\\SourceSafe\\gods98_dx6\\gods98\\src\\Files.c",
                        // STRING: LEGORR 0x004ab0ac
                        545, "File_GetC");
        break;
    }
    return 0;
}

// FUNCTION: LEGORR 0x0047fca0
int file_length(File* f)
{
    int pos;
    int len;

    pos = file_tell(f);
    file_seek(f, 0, SEEK_END);
    len = file_tell(f);
    file_seek(f, pos, SEEK_SET);
    return len;
}

// FUNCTION: LEGORR 0x0047fce0
char* file_internal_fgets(char* fgetsBuffer, int num, File* f)
{
    int pos = 0;

    fgetsBuffer[0] = (char)file_getc(f);
    while (fgetsBuffer[pos] != '\0' && fgetsBuffer[pos] != '\n' &&
           fgetsBuffer[pos] != (char)-1 && pos != num) {
        pos++;
        fgetsBuffer[pos] = (char)file_getc(f);
    }
    fgetsBuffer[pos + 1] = '\0';
    if (pos) return fgetsBuffer;
    return NULL;
}

// FUNCTION: LEGORR 0x0047fd40
char* file_gets(char* fgetsBuffer, int num, File* f)
{
    FileSys fs;

    fs = file_get_system(f);
    switch (fs) {
    case FileSystem_Wad:
        return file_internal_fgets(fgetsBuffer, num, f);
    case FileSystem_Std:
        return fgets(fgetsBuffer, num, f->stream.std);
    default:
        file_error_file("%s(%i) : Unknown file system in call to %s",
                        "C:\\Dev\\SourceSafe\\gods98_dx6\\gods98\\src\\Files.c",
                        // STRING: LEGORR 0x004ab0b8
                        587, "File_GetS");
        break;
    }
    return NULL;
}

// FUNCTION: LEGORR 0x0047fdb0
int file_printf(File* f, const char* msg, ...)
{
    FileSys fs;
    va_list args;

    fs = file_get_system(f);
    switch (fs) {
    case FileSystem_Wad:
        // STRING: LEGORR 0x004ab0c4
        file_error_file("\"fprintf\" is unsupprted for wad files");
        return 0;
    case FileSystem_Std:
        va_start(args, msg);
        vfprintf(f->stream.std, msg, args);
        va_end(args);
        return 0;
    default:
        file_error_file("%s(%i) : Unknown file system in call to %s",
                        "C:\\Dev\\SourceSafe\\gods98_dx6\\gods98\\src\\Files.c",
                        // STRING: LEGORR 0x004ab0ec
                        612, "File_PrintF");
        break;
    }
    return 0;
}

// FUNCTION: LEGORR 0x0047fe20
FileSys file_get_system(File* f)
{
    return f->type;
}

// FUNCTION: LEGORR 0x0047fe30
FileSys file_check_system(const char* fName, const char* mode)
{
    if (fName && mode && strlen(fName) && strlen(mode)) {
        if (mode[0] == 'w' || mode[0] == 'W') return FileSystem_Std;
        return (FileSys)(wad_is_file_in_wad(
                             file_get_wad_name((char*)fName),
                             currWadHandle) == -1);
    }
    return FileSystem_Error;
}

// FUNCTION: LEGORR 0x0047fea0
int file_open_wad(WADFILE* wad, const char* fName)
{
    wad->eof = 0;
    wad->streamPos = 0;
    wad->hFile = wad_file_open(fName, currWadHandle);
    return wad->hFile != -1;
}

// FUNCTION: LEGORR 0x0047fed0
File* file_alloc(FileSys fType)
{
    File* file;

    if (fType == FileSystem_Std) {
        file = (File*)file_malloc(sizeof(File));
        file->type = fType;
        return file;
    } else if (fType == FileSystem_Wad) {
        if ((file = (File*)file_malloc(sizeof(File))) != NULL) {
            file->type = fType;
            if ((file->stream.wad = (WADFILE*)file_malloc(sizeof(WADFILE))) != NULL)
                return file;
            file_free(file);
        }
    }
    return NULL;
}

// FUNCTION: LEGORR 0x0047ff30
void* file_malloc(int size)
{
    return malloc(size);
}

// FUNCTION: LEGORR 0x0047ff40
void file_free(void* ptr)
{
    free(ptr);
}

// FUNCTION: LEGORR 0x0047ff50
void file_dealloc(File* file)
{
    if (file) {
        if (file_get_system(file) == FileSystem_Std) {
            if (file->stream.std) {
                fclose(file->stream.std);
            }
        } else if (file_get_system(file) == FileSystem_Wad) {
            if (file->stream.wad) {
                wad_file_close(file->stream.wad->hFile);
                file_free(file->stream.wad);
            }
        } else {
            return;
        }
        file_free(file);
    }
}

// FUNCTION: LEGORR 0x0047ffc0
char* file_get_wad_name(char* fName)
{
    int len;
    int baseLen;
    char c;

    if (fileGlobs.basePathSet) {
        len = strlen(fName);
        baseLen = strlen(fileGlobs.basePath);
        if (len <= baseLen) return fName;
        c = fName[baseLen];
        fName[baseLen] = '\0';
        if (_strcmpi(fName, fileGlobs.basePath) == 0) {
            // STRING: LEGORR 0x004a1d60
            sprintf(fileGlobs.wadName, "%s", &fName[baseLen + 1]);
            fName[baseLen] = c;
            return fileGlobs.wadName;
        }
        fName[baseLen] = c;
        return fName;
    }
    return fName;
}

// FUNCTION: LEGORR 0x00480050
char* file_get_line(char* buffer, int size, File* file)
{
    char* res;
    int len;

    res = file_gets(buffer, size, file);
    len = strlen(buffer) - 1;
    if (buffer[len] == '\n') buffer[len] = '\0';
    if (len && buffer[len - 1] == '\r') buffer[len - 1] = '\0';
    return res;
}

// FUNCTION: LEGORR 0x004800a0
void* file_load_binary(const char* filename, unsigned int* sizeptr)
{
    return file_load(filename, sizeptr, 1);
}

// FUNCTION: LEGORR 0x004800c0
void* file_load(const char* filename, unsigned int* sizeptr, int binary)
{
    File* ifp;
    char* buffer;
    unsigned int size;

    // STRING: LEGORR 0x004a2770
    if ((ifp = file_open(filename, binary ? "rb" : "r")) != NULL) {
        file_seek(ifp, 0, SEEK_END);
        size = file_tell(ifp);
        if (fileGlobs.loadCallback)
            fileGlobs.loadCallback(filename, size, fileGlobs.loadCallbackData);
        if ((buffer = (char*)malloc(size)) != NULL) {
            file_seek(ifp, 0, SEEK_SET);
            file_read(buffer, 1, size, ifp);
            if (sizeptr) *sizeptr = size;
            file_close(ifp);
            return buffer;
        }
        file_close(ifp);
    }
    return NULL;
}

// FUNCTION: LEGORR 0x00480170
int file_load_binary_handle(const char* filename, unsigned int* sizeptr)
{
    File* ifp;
    unsigned char* buffer;
    unsigned int size;
    int handle;

    if ((ifp = file_open(filename, "rb")) != NULL) {
        file_seek(ifp, 0, SEEK_END);
        size = file_tell(ifp);
        if (fileGlobs.loadCallback)
            fileGlobs.loadCallback(filename, size, fileGlobs.loadCallbackData);
        if ((handle = level_map_slot_allocate(size)) != -1) {
            buffer = level_map_slot_get(handle);
            file_seek(ifp, 0, SEEK_SET);
            file_read(buffer, 1, size, ifp);
            if (sizeptr) *sizeptr = size;
            file_close(ifp);
            return handle;
        }
        file_close(ifp);
    }
    return -1;
}

// FUNCTION: LEGORR 0x00480220
char* file_verify_filename(const char* filename)
{
    char buff[FILE_MAXPATH];

    if (filename) {
        if (*filename == '\\') filename++;
        sprintf(buff, "%s\\%s", fileGlobs.dataDir, filename);
        if (_fullpath(fileGlobs.fullName, buff, FILE_MAXPATH)) {
            if (strncmp(fileGlobs.fullName, fileGlobs.dataDir,
                        strlen(fileGlobs.dataDir)) == 0)
                return fileGlobs.fullName;
        }
    }
    return NULL;
}

// FUNCTION: LEGORR 0x004802b0
void file_set_load_callback(FileLoadCallback callback, void* data)
{
    fileGlobs.loadCallback = callback;
    fileGlobs.loadCallbackData = data;
}

// FUNCTION: LEGORR 0x004802d0
void file_check_redundant_files(const char* logName)
{
    File* f;
    char line[1024];

    if ((f = file_open(logName, "r")) != NULL) {
        fileRedundantCount = 0;
        /* The original reads the File record as a C library stream. */
        while (fscanf((FILE*)f, "%s", line) != EOF) {
            strcpy(fileRedundantList[fileRedundantCount++], line);
        }
        file_check_directory(fileGlobs.dataDir);
        fclose((FILE*)f);
    }
}

// FUNCTION: LEGORR 0x00480390
void file_check_directory(const char* dir)
{
    struct _finddata_t c_file;
    long hFile;
    char path[1024];

    strcpy(path, dir);
    // STRING: LEGORR 0x004ab100
    strcat(path, "\\*.*");
    if ((hFile = _findfirst(path, &c_file)) != -1) {
        do {
            // STRING: LEGORR 0x004ab0fc
            if (strcmp(c_file.name, ".") &&
                // STRING: LEGORR 0x004ab0f8
                strcmp(c_file.name, "..")) {
                strcpy(path, dir);
                strcat(path, "\\");
                strcat(path, c_file.name);
                if (c_file.attrib & _A_SUBDIR) file_check_directory(path);
                else file_check_file(path);
            }
        } while (_findnext(hFile, &c_file) == 0);
        _findclose(hFile);
    }
}

// FUNCTION: LEGORR 0x00480570
void file_check_file(const char* fName)
{
    unsigned int i;

    for (i = 0; i < fileRedundantCount; i++) {
        if (_strcmpi(fName, fileRedundantList[i]) == 0) return;
    }
}
