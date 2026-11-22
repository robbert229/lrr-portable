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
 * [library:filesystem] (ADR 0007), as isle-portable's LegoFile: the C
 * library streams are SDL3 streams, every path goes through
 * files_native_host_path (Windows separators and case, isle-portable's
 * MxString::MapPathToFilesystem), and a name under "Saves" opens in the save
 * directory the configuration names (files_native_save_path), not in the
 * data directory. */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_iostream.h>
#include <miniwin/windows.h>

#include "files_native.h"

#include "crt/vc5names.h"

#include "files.h"
#include "wad.h"
#include "utils.h"
#include "g98_024.h"
#include "g98_029.h"

/* The original calls an empty variadic function, folded into 00484c90
 * (Flic_Nothing), with its debug messages. C++ cannot call 00484c90's
 * prototype with arguments, so the unit keeps an empty variadic one. */
static void files_nothing(const char* msg, ...)
{
}
#define flic_opcode27_fallback_noop files_nothing

#define FILE_MAXPATH 260
#define FILE_MAXWADS 10

/* [library:filesystem] The original's C library calls on SDL3 streams, as
 * isle-portable's LegoFile::Open: the Windows path maps to the host's
 * first. SDL_IOFromFile fails cleanly on a NULL path (one too long). */
static SDL_IOStream* files_io_open(const char* path, const char* mode)
{
    char hostPath[FILE_MAXPATH * 4];

    return SDL_IOFromFile(files_native_host_path(hostPath, path, sizeof(hostPath)), mode);
}

/* [library:filesystem] fgets on an SDL3 stream: at most num - 1 bytes, up
 * to and including a newline; NULL at the end with nothing read. */
static char* files_io_gets(char* buffer, int num, SDL_IOStream* io)
{
    int pos = 0;
    char c;

    if (num <= 0) return NULL;
    while (pos < num - 1 && SDL_ReadIO(io, &c, 1) == 1) {
        buffer[pos++] = c;
        if (c == '\n') break;
    }
    buffer[pos] = '\0';
    return pos ? buffer : NULL;
}

/* [library:filesystem] fscanf(stream, "%<size-1>s", word) on an SDL3
 * stream: skips blanks, then reads one word; 0 at the end of the stream. */
static int files_io_scan_word(SDL_IOStream* io, char* word, int size)
{
    int len = 0;
    char c;

    do {
        if (SDL_ReadIO(io, &c, 1) != 1) return 0;
    } while (isspace((unsigned char)c));
    for (;;) {
        if (len == size - 1) {
            SDL_SeekIO(io, -1, SDL_IO_SEEK_CUR); /* fscanf leaves it unread */
            break;
        }
        word[len++] = c;
        if (SDL_ReadIO(io, &c, 1) != 1) break;
        if (isspace((unsigned char)c)) break;
    }
    word[len] = '\0';
    return 1;
}

/* [library:filesystem] The name under the save directory of a name under
 * the original's "Saves" ("Saves\\0.sav" -> "0.sav", "Saves" -> ""), or
 * NULL when no save directory is configured or the name is elsewhere. */
static const char* files_save_name(const char* name)
{
    const char* saves = "Saves";
    size_t len = strlen(saves);

    if (files_native_save_path() == NULL || name == NULL) return NULL;
    if (*name == '\\' || *name == '/') name++;
    if (strnicmp(name, saves, len) != 0) return NULL;
    if (name[len] == '\0') return name + len;
    if (name[len] == '\\' || name[len] == '/') return name + len + 1;
    return NULL;
}

/* [library:config] _getcwd, or the data directory the configuration's
 * diskpath names (files_native_disk_path): File_Initialise's install. A
 * diskpath too long for the buffer keeps the working directory. */
static char* files_disk_dir(char* buffer, int maxlen)
{
    const char* disk = files_native_disk_path();

    if (disk == NULL || (int)strlen(disk) + 1 > maxlen) return _getcwd(buffer, maxlen);
    strcpy(buffer, disk);
    return buffer;
}

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
unsigned int fileRedundantListCount;

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
    SDL_IOStream* fp;

    files_disk_dir(cwd, FILE_MAXPATH); // [library:config]
    if (cwd[strlen(cwd) - 1] == '\\') cwd[strlen(cwd) - 1] = '\0';
    // STRING: LEGORR 0x004a1cbc
    snprintf(fileGlobs.dataDir, sizeof(fileGlobs.dataDir), "%s\\%s", cwd,
            // STRING: LEGORR 0x004aaf5c
            "Data");

    for (i = 0; i < FILE_MAXWADS; i++) {
        char wadFile[FILE_MAXPATH];
        // STRING: LEGORR 0x004aaf50
        snprintf(wadFile, sizeof(wadFile), "%s%i.wad", programName, i);
        /* The WAD in the diskpath, by its host path: Wad.c opens it with
         * the C library's fopen. */
        files_native_host_path(wadFile, wadFile, FILE_MAXPATH); // [library:config]
        if (file_load_wad(wadFile) != -1) foundWad = 1;
    }

    files_disk_dir(dirBuf, FILE_MAXPATH); // [library:config]
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
                    stricmp(c_file.name, "Data") == 0) {
                    failed = 0;
                    break;
                }
            } while (_findnext(hFile, &c_file) == 0);
            _findclose(hFile);
        }
    }

    if (insistOnCD) {
        snprintf(path, sizeof(path), "%s\\%s", "Data",
                // STRING: LEGORR 0x004aaf1c
                "cd.key");
        // STRING: LEGORR 0x004a1eac
        if ((fp = files_io_open(path, "r")) != NULL) { // [library:filesystem]
            SDL_CloseIO(fp); // [library:filesystem]
            failed = 1;
        }
    }

    if (!failed) {
        char hostPath[FILE_MAXPATH * 4];

        SDL_CreateDirectory(files_native_host_path(hostPath, "Data", sizeof(hostPath))); // [library:filesystem]
        snprintf(path, sizeof(path), "%s\\%s", "Data",
                // STRING: LEGORR 0x004aaf10
                "delme.dat");
        if ((fp = files_io_open(path, "r")) != NULL) { // [library:filesystem]
            SDL_CloseIO(fp); // [library:filesystem]
            /* SDL_RemovePath needs no read-only bit cleared first (_chmod). */
            SDL_RemovePath(files_native_host_path(hostPath, path, sizeof(hostPath))); // [library:filesystem]
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
    SDL_IOStream* fp;

    for (letter = 'C'; letter <= 'Z'; letter++) {
        drive[0] = letter;
        if (GetDriveTypeA(drive) == DRIVE_CDROM) {
            // STRING: LEGORR 0x004aaf64
            snprintf(path, sizeof(path), "%c:\\%s\\%s", letter, "Data", "cd.key");
            if ((fp = files_io_open(path, "r")) != NULL) { // [library:filesystem]
                SDL_CloseIO(fp); // [library:filesystem]
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
    vsnprintf(buffer, sizeof(buffer), msg, args);
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
        /* Every caller passes a buffer of at least FILE_MAXPATH. */
        snprintf(path, FILE_MAXPATH, "%c:\\%s\\%s", fileGlobs.cdLetter, "Data", fname);
        return 1;
    }
    return 0;
}

// FUNCTION: LEGORR 0x0047f6a0
void file_make_dir(const char* path)
{
    char buffer[FILE_MAXPATH * 2];
    char hostPath[FILE_MAXPATH * 4];
    const char* saveName;

    if ((saveName = files_save_name(path)) != NULL) { // [library:filesystem]
        snprintf(buffer, sizeof(buffer), "%s\\%s", files_native_save_path(), saveName);
    } else {
        snprintf(buffer, sizeof(buffer), "%s\\%s", "Data", path);
    }
    /* SDL_CreateDirectory also makes any missing parent, which a save
     * directory outside the data directory may need. */
    SDL_CreateDirectory(files_native_host_path(hostPath, buffer, sizeof(hostPath))); // [library:filesystem]
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
            if ((file->stream.std = files_io_open(fullName, mode)) != NULL) { // [library:filesystem]
                // STRING: LEGORR 0x004aafa4
                flic_opcode27_fallback_noop("STD Load %s\n", fullName);
                return file;
            }
            // STRING: LEGORR 0x004a52b8
            if (case_insensitive_substring_find(mode, "w") == NULL &&
                file_get_cd_file_path(cdName, fName) &&
                (file->stream.std = files_io_open(cdName, mode)) != NULL) { // [library:filesystem]
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
        /* SEEK_SET, SEEK_CUR and SEEK_END are SDL_IO_SEEK_SET, _CUR and
         * _END; fseek's 0 is success, SDL_SeekIO's the new position. */
        return SDL_SeekIO(f->stream.std, pos, (SDL_IOWhence)mode) < 0 ? -1 : 0; // [library:filesystem]
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
        /* fread's count of whole items. */
        if (size <= 0 || count <= 0) return 0;
        return (int)(SDL_ReadIO(f->stream.std, buffer, (size_t)size * count) / size); // [library:filesystem]
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
        /* fwrite's count of whole items. */
        if (size <= 0 || count <= 0) return 0;
        return (int)(SDL_WriteIO(f->stream.std, buffer, (size_t)size * count) / size); // [library:filesystem]
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

/* File_Flush. Nothing calls it, so the original's /OPT:REF removed it, but
 * its fflush reference had already pulled LIBC's fflush.obj between fread
 * and fgetc (finding 667). The original's line numbers leave room for it:
 * File_Close's error is line 431 and File_Tell's 469. Its shape is
 * inferred; only the reference matters. */
// PORT-ONLY: the original's unreferenced File_Flush, which /OPT:REF removed; kept for its fflush reference (finding 667)
int file_flush(File* f)
{
    if (file_get_system(f) == FileSystem_Std) return SDL_FlushIO(f->stream.std) ? 0 : EOF; // [library:filesystem]
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
        return (int)SDL_TellIO(f->stream.std); // [library:filesystem]
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
    SDL_IOStream* f;
    char cdName[FILE_MAXPATH];

    fullName = file_verify_filename(fName);
    fs = file_check_system(fullName, "r");
    if (fs == FileSystem_Std) {
        if ((f = files_io_open(fullName, "r")) != NULL) { // [library:filesystem]
            SDL_CloseIO(f); // [library:filesystem]
            return 1;
        }
        if (file_get_cd_file_path(cdName, fName)) {
            if ((f = files_io_open(cdName, "r")) != NULL) { // [library:filesystem]
                SDL_CloseIO(f); // [library:filesystem]
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
        /* The original loads 4 bytes and its callers keep the low one;
         * reading the byte alone does not run past the WAD buffer. */
        return *((const char*)wad_file_data(f->stream.wad->hFile) +
                 f->stream.wad->streamPos++);
    case FileSystem_Std: {
        unsigned char c;

        return SDL_ReadIO(f->stream.std, &c, 1) == 1 ? c : EOF; // [library:filesystem]
    }
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
        return files_io_gets(fgetsBuffer, num, f->stream.std); // [library:filesystem]
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
        SDL_IOvprintf(f->stream.std, msg, args); // [library:filesystem]
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
                SDL_CloseIO(file->stream.std); // [library:filesystem]
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
            snprintf(fileGlobs.wadName, sizeof(fileGlobs.wadName), "%s", &fName[baseLen + 1]);
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
    const char* saveName;

    if (filename) {
        if (*filename == '\\') filename++;
        if ((saveName = files_save_name(filename)) != NULL) { // [library:filesystem]
            /* A save opens in the save directory, and must stay inside it
             * as a data file stays inside the data directory. */
            const char* savePath = files_native_save_path();

            if ((unsigned)snprintf(buff, sizeof(buff), "%s\\%s", savePath, saveName) < sizeof(buff) &&
                _fullpath(fileGlobs.fullName, buff, FILE_MAXPATH) &&
                strncmp(fileGlobs.fullName, savePath, strlen(savePath)) == 0)
                return fileGlobs.fullName;
            return NULL;
        }
        snprintf(buff, sizeof(buff), "%s\\%s", fileGlobs.dataDir, filename);
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
        /* The original hands the File record itself to fscanf and fclose
         * as a C library stream; read the record's own stream instead (a
         * WAD stream has none) and close the record. */
        SDL_IOStream* stream = (f->type == FileSystem_Std) ? f->stream.std : NULL;
        fileRedundantListCount = 0;
        while (stream && fileRedundantListCount < 2000 &&
               files_io_scan_word(stream, line, sizeof(line))) { // [library:filesystem]
            strcpy(fileRedundantList[fileRedundantListCount++], line);
        }
        file_check_directory(fileGlobs.dataDir);
        file_close(f);
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

    for (i = 0; i < fileRedundantListCount; i++) {
        if (_strcmpi(fName, fileRedundantList[i]) == 0) return;
    }
}
