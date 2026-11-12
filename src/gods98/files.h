// SPDX-License-Identifier: LGPL-3.0-or-later
/* Files.c (original translation unit G98-014, finding 583): the gods98 file
 * layer over the WAD archives (Wad.c) and the C library.
 *
 * C-compatible header (ADR 0005). files.c is compiled as C in the matching
 * build and as C++ in the native port. Shared C++ headers do not include this
 * file: every include and declaration a header adds shifts MSVC's symbol
 * numbering (finding 506).
 *
 * Names. The native port keeps its own file layer (gods98/files_port.cpp,
 * the port tail), which reads the WADs through gods98/resource.cpp and
 * resolves host paths; native code calls it by the port's names. The unit's
 * functions are declared here as file_* names, which is what they are called
 * natively, where nothing calls them. In the matching build (MSVC 5.0) the
 * functions whose every port caller already has the original's contract take
 * the port's names instead (the block at the end), and files_port.cpp leaves
 * its own definitions of those names out. Port callers then call the
 * original functions there, as the original's callers did (finding 579). */
#ifndef LEGORRR_GODS98_FILES_H
#define LEGORRR_GODS98_FILES_H

#include <stdio.h>

#if defined(_MSC_VER) && _MSC_VER < 1200
/* The matching build: the unit owns these port names (finding 583). */
#define LEGORRR_FILES_UNIT_OWNS_PORT_NAMES 1
#define file_initialise resource_mount_startup_target
#define file_find_data_cd resource_find_cd_key_drive
#define file_error_file resource_debug_output_format
#define file_get_cd_file_path resource_cd_file_path_get
#define file_open game_file_open_dispatch
#define file_seek wad_file_seek
#define file_read wad_file_read
#define file_write game_file_write
#define file_close game_file_close
#define file_tell wad_file_cursor_get
#define file_exists sound_sample_file_open_by_name
#define file_getc game_file_read_byte
#define file_internal_fgets game_file_read_line
#define file_gets game_file_read_line_dispatch
#define file_printf game_file_print_format
#define file_check_system game_file_open_route_select
#define file_alloc game_file_handle_allocate
#define file_free engine_heap_free_wrapper
#define file_dealloc game_file_handle_release
#define file_get_wad_name game_file_mounted_path_prefix_rewrite
#define file_get_line game_file_read_crlf_line
#define file_load_binary_handle map_open2
#define file_verify_filename resource_path_resolve_against_base
#define file_set_load_callback loader_progress_callback_set
#define file_check_redundant_files resource_file_token_list_load_and_tree_scan_target
#define file_check_directory resource_directory_tree_scan_target
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* The file system behind a File (_File_GetSystem). */
typedef enum FileSys {
    FileSystem_Wad,
    FileSystem_Std,
    FileSystem_Error
} FileSys;

/* An open WAD entry: its Wad.c handle and the read position. */
typedef struct WADFILE {
    int hFile;
    int streamPos;
    int eof;
} WADFILE;

/* An open file: a C library stream or a WAD entry. 8 bytes in the original;
 * the pointer widens on 64-bit hosts. */
typedef struct File {
    FileSys type;
    union {
        FILE* std;
        WADFILE* wad;
    } stream;
} File;

/* File_SetLoadCallback's callback: the file name and its size. */
typedef void (*FileLoadCallback)(const char* filename, unsigned int fileSize,
                                 void* data);

void file_initialise(const char* programName, int insistOnCD,
                     const char* registryLocation);           /* 0047f130 */
int file_find_data_cd(void);                                   /* 0047f4f0 */
int file_set_base_search_path(const char* basePath);           /* 0047f590 */
void file_error_file(const char* msg, ...);                    /* 0047f600 */
int file_load_wad(const char* fName);                          /* 0047f640 */
int file_get_cd_file_path(char* path, const char* fname);      /* 0047f660 */
void file_make_dir(const char* path);                          /* 0047f6a0 */
File* file_open(const char* fName, const char* mode);          /* 0047f6e0 */
int file_seek(File* f, int pos, int mode);                     /* 0047f850 */
int file_read(void* buffer, int size, int count, File* f);     /* 0047f980 */
int file_write(const void* buffer, int size, int count,
               File* f);                                       /* 0047fa50 */
int file_close(File* f);                                       /* 0047fac0 */
int file_tell(File* f);                                        /* 0047fb10 */
int file_exists(const char* fName);                            /* 0047fb60 */
int file_getc(File* f);                                        /* 0047fc20 */
int file_length(File* f);                                      /* 0047fca0 */
char* file_internal_fgets(char* fgetsBuffer, int num, File* f); /* 0047fce0 */
char* file_gets(char* fgetsBuffer, int num, File* f);          /* 0047fd40 */
int file_printf(File* f, const char* msg, ...);                /* 0047fdb0 */
FileSys file_get_system(File* f);                              /* 0047fe20 */
FileSys file_check_system(const char* fName, const char* mode); /* 0047fe30 */
int file_open_wad(WADFILE* wad, const char* fName);            /* 0047fea0 */
File* file_alloc(FileSys fType);                               /* 0047fed0 */
void* file_malloc(int size);                                   /* 0047ff30 */
void file_free(void* ptr);                                     /* 0047ff40 */
void file_dealloc(File* file);                                 /* 0047ff50 */
char* file_get_wad_name(char* fName);                          /* 0047ffc0 */
char* file_get_line(char* buffer, int size, File* file);       /* 00480050 */
void* file_load_binary(const char* filename,
                       unsigned int* sizeptr);                 /* 004800a0 */
void* file_load(const char* filename, unsigned int* sizeptr,
                int binary);                                   /* 004800c0 */
int file_load_binary_handle(const char* filename,
                            unsigned int* sizeptr);            /* 00480170 */
char* file_verify_filename(const char* filename);              /* 00480220 */
void file_set_load_callback(FileLoadCallback callback,
                            void* data);                       /* 004802b0 */
void file_check_redundant_files(const char* logName);          /* 004802d0 */
void file_check_directory(const char* dir);                    /* 00480390 */
void file_check_file(const char* fName);                       /* 00480570 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GODS98_FILES_H */
