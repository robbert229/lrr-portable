// SPDX-License-Identifier: LGPL-3.0-or-later
/* Port tail (ADR 0006) for G98-014 Files.c (finding 583). The original-shape
 * unit is gods98/files.c; this file keeps the port's file layer, which the
 * native port runs. None of it is annotated: the addresses belong to
 * files.c.
 *
 * - The port's models of Files.c's functions, under the port's names. They
 *   read the WADs through gods98/resource.cpp's Resource records, resolve
 *   host paths (POSIX separators, mounted roots, no _fullpath) and take
 *   their startup prompts from host hooks. Native callers keep calling them.
 * - In the matching build (MSVC 5.0) files.c defines 26 of those names
 *   itself, for the callers whose contracts are the original's (files.h).
 *   Their port definitions here are left out of it
 *   (FILES_PORT_DEFINES_UNIT_NAMES); the rest, whose port contracts differ
 *   (save_directory_ensure_exists, save_stream_length,
 *   wad_mount_path_cache_store, ares_cd_fallback_path, mount_wad_from_path,
 *   engine_heap_allocate, wad_entry_read_into_stream_slot,
 *   game_file_system_type_get, resource_directory_token_list_contains),
 *   stay in every build.
 * - At the end: the four Wad.c adapters gods98/wad.h describes, the no-op
 *   files.c calls natively for its debug messages, and the native stand-ins
 *   files_native.h declares. */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(_WIN32)
#include <windows.h>
#else
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
#include "resource.hpp"
#include "heap.hpp"
#include "record.hpp"
#include "sound.hpp"
#include "string.hpp"
#include "texture.hpp"
#include "view.hpp"
#include "scene_pool.hpp"
#include "resource_for_wad.hpp"
#include "resource_from_wad.hpp"
#include <math.h>
#include <stdint.h>
#include <limits.h>
#include "game.hpp"
#include "gods98/block.hpp"
#include "gods98/engine.hpp"
#include "gods98/media.hpp"
#include "effects.hpp"
#include "gods98/message.hpp"
#include "gods98/simtick.hpp"
#include "gods98/surface.hpp"
#include "crt/crt.hpp"
#include "gods98/camera.hpp"
#include "terrain_grid.hpp"
#include "game/entity.hpp"
#include "type_config_ports.hpp"
#include "ai.hpp"
#include "cmd.hpp"
#include "upgrades.hpp"
#include "spawn.hpp"
#include "port_loop_ports.hpp"
#include "frontend.hpp"
#include "level.hpp"
#include "objective.hpp"
#include "util.hpp"
#include "gods98/flic.hpp"
#include "select.hpp"
#include "scene_render.hpp"
#include "tutorial_hud_ports.hpp"
#include "mission.hpp"
#include "savegame.hpp"
#include "options.hpp"
#include <stddef.h>
#include "platform/platform.h"
#include "audio.hpp"
#include "frameloop.hpp"
#include <ctype.h>
#include "resource_for_files.hpp"
#include "game/level_for_files.hpp"

#include "wad.h"

/* The matching build, where files.h gives files.c the port's names. This
 * file cannot include files.h: its File declarations would then clash with
 * resource.hpp's under those names. */
#if defined(_MSC_VER) && _MSC_VER < 1200
#define FILES_PORT_DEFINES_UNIT_NAMES 0
#else
#define FILES_PORT_DEFINES_UNIT_NAMES 1
#endif

#if defined(_WIN32) && !defined(INVALID_FILE_ATTRIBUTES)
#define INVALID_FILE_ATTRIBUTES ((DWORD)-1)
#endif

typedef struct WadFileStreamState {
    int slot;
    int32_t cursor;
} WadFileStreamState;

#if defined(LEGORRR_BACKEND_WINAPI)
#  include <direct.h>
#  define LRR_MKDIR(p) _mkdir(p)
#else
#  include <sys/stat.h>
#  define LRR_MKDIR(p) mkdir(p, 0755)
#endif

static const char s_game_file_source_path[] =
    "C:\\Dev\\SourceSafe\\gods98_dx6\\gods98\\src\\Files.c";

static const char s_game_file_seek_mode_format[] = "Uknown seek mode (%i)";
/* Shared file-layer diagnostics used by the reference dispatchers. */
static const char s_game_file_unknown_type_format[] =
    "%s(%i) : Unknown file system in call to %s";

static const char s_game_file_seek_name[] = "File_Seek";

static const char s_game_file_tell_name[] = "File_Tell";

static const char s_game_file_getc_name[] = "File_GetC";
static const char s_game_file_gets_name[] = "File_GetS";

static const char s_game_file_printf_wad_error[] =
    "\"fprintf\" is unsupprted for wad files";
static const char s_game_file_printf_name[] = "File_PrintF";

static char      s_wad_mount_path_cache[0x400];

static char      s_wad_mount_path_rewrite[0x400];

/* LoaderProgressCallbackSet (FUN_004802b0) — see media.hpp. */
void* s_loader_progress_callback; /* DAT_005343ac role: unowned */
void* s_loader_progress_context;  /* DAT_005343b0 role: unowned */

static int       s_wad_mount_path_cache_valid;
static const char s_game_file_open_name[] = "File_Open";
static const char s_game_file_close_name[] = "File_Close";
static const char s_game_file_write_name[] = "File_Write";
static const char s_game_file_write_wad_error[] =
    "Cannot write to a file stored in a WAD";

static ResourceFileTokenList s_resource_file_tokens;

/* Defined below (forward declarations added by tools/relocate_unit.py). */
static int resource_cd_key_path_exists(const char* path);
static int resource_startup_cd_retry(const char* registry_key);
static int resource_startup_current_directory(char* path, size_t capacity);
static void resource_startup_delme_marker_remove(const char* path);
static void resource_startup_directory_create(const char* path);
static int resource_startup_directory_exists(const char* path);
static int resource_startup_missing_data_dialog(const char* registry_key);
void wad_mount_path_cache_store(const char* path);
static int resource_slot_length_get(int slot);
static void fixed_record_slot_release(int slot);

/* WadMountStartup @ 0x0047f130. The startup owner supplies the executable
 * prefix and registry key; the resource module owns numbered archive
 * mounting, Data-root derivation, CD-marker probing, and mount-path caching.
 * Localized prompts and modal UI are host operations; process exit remains
 * optional so portable callers can inspect the startup result. */
#if FILES_PORT_DEFINES_UNIT_NAMES
void resource_mount_startup_target(const char* prefix, int require_cd,
                                   const char* registry_key)
{
    char working_directory[260];
    char data_directory[RESOURCE_PATH_MAX];
    char marker_path[RESOURCE_PATH_MAX];
    size_t length;
    int mounted;
    int cd_found;
    int data_directory_found;
    int data_missing;

    s_resource_startup_result = RESOURCE_STARTUP_OK;
    if (prefix == NULL || prefix[0] == '\0') prefix = s_wad_prefix;
    if (!resource_startup_current_directory(working_directory,
                                            sizeof(working_directory))) {
        s_resource_startup_result = RESOURCE_STARTUP_PATH_ERROR;
        return;
    }
    length = strlen(working_directory);
    if (length > 1u &&
        (working_directory[length - 1u] == '/' ||
         working_directory[length - 1u] == '\\')) {
        working_directory[length - 1u] = '\0';
    }
    if (snprintf(data_directory, sizeof(data_directory), "%s%cData",
                 working_directory, RES_PATH_SEP) < 0 ||
        strlen(data_directory) >= sizeof(data_directory)) {
        s_resource_startup_result = RESOURCE_STARTUP_PATH_ERROR;
        return;
    }

    resource_unmount_all();
    s_cd_root[0] = '\0';
    s_cd_drive = 'A';
    snprintf(s_cd_data_dir, sizeof(s_cd_data_dir), "%s", "Data");
    resource_set_wad_prefix(prefix);
    mounted = resource_mount(working_directory, prefix);
    /* resource_mount is also the explicit-root test API; restore the retail
     * startup Data directory after its modern-root cache setup. */
    resource_directory_scan_root_set(data_directory);
    wad_mount_path_cache_store(data_directory);

    cd_found = resource_find_cd_key_drive();
    while (!cd_found && require_cd != 0) {
        if (!resource_startup_cd_retry(registry_key)) return;
        cd_found = resource_find_cd_key_drive();
    }

    data_directory_found = resource_startup_directory_exists(data_directory);
    if (!cd_found && data_directory_found) {
        /* A loose Data tree is the retail fallback when neither a WAD nor a
         * CD key drive is available. Mounted CD paths remain drive-rooted. */
        snprintf(s_cd_root, sizeof(s_cd_root), "%s", working_directory);
    }
    data_missing = (!cd_found && mounted == 0 && !data_directory_found);
    if (require_cd != 0 &&
        snprintf(marker_path, sizeof(marker_path), "%s%cData%ccd.key",
                 working_directory, RES_PATH_SEP, RES_PATH_SEP) >= 0 &&
        resource_cd_key_path_exists(marker_path)) {
        data_missing = 1;
    }
    if (data_missing) {
        (void)resource_startup_missing_data_dialog(registry_key);
        return;
    }

    resource_startup_directory_create(data_directory);

    if (snprintf(marker_path, sizeof(marker_path), "%s%cData%cdelme.dat",
                 working_directory, RES_PATH_SEP, RES_PATH_SEP) >= 0 &&
        resource_cd_key_path_exists(marker_path)) {
        resource_startup_delme_marker_remove(marker_path);
    }
}
#endif
/* FindCdKeyDrive (FUN_0047f4f0): scan optical drive letters C through Z,
 * retaining the first drive with a readable cd.key marker. Platform hooks
 * supply drive type and file probes; a mounted root uses its file probe. */
#if FILES_PORT_DEFINES_UNIT_NAMES
int resource_find_cd_key_drive(void)
{
    char path[RESOURCE_PATH_MAX];
    char drive;
    if (s_cd_root[0] != '\0') {
        ares_cd_root_path("cd.key", path, sizeof(path));
        return resource_cd_key_path_exists(path);
    }
    for (drive = 'C'; drive <= 'Z'; ++drive) {
        int drive_type;
        if (s_cd_drive_type_query != NULL) {
            drive_type = s_cd_drive_type_query(drive, s_cd_probe_context);
        }
#if defined(_WIN32)
        else {
            char drive_root[4];
            drive_root[0] = drive;
            drive_root[1] = ':';
            drive_root[2] = '\\';
            drive_root[3] = '\0';
            drive_type = (int)GetDriveTypeA(drive_root);
        }
#else
        else {
            drive_type = 0;
        }
#endif
        if (drive_type != 5) {
            continue;
        }
        snprintf(path, sizeof(path), "%c:%c%s%ccd.key", drive,
                 RES_PATH_SEP, s_cd_data_dir, RES_PATH_SEP);
        if (resource_cd_key_path_exists(path)) {
            s_cd_drive = drive;
            return 1;
        }
    }
    return 0;
}
#endif

/* WadMountPathCacheStore (FUN_0047f590): keep a nonempty path shorter than
 * 0x401 bytes; invalid input clears the cache and its validity flag. */
void wad_mount_path_cache_store(const char* path)
{
    size_t length;
    if (path != NULL) {
        length = strlen(path);
        if (length != 0 && length < sizeof(s_wad_mount_path_cache)) {
            strncpy(s_wad_mount_path_cache, path, length + 1);
            s_wad_mount_path_cache_valid = 1;
            return;
        }
    }
    s_wad_mount_path_cache[0] = '\0';
    s_wad_mount_path_cache_valid = 0;
}

/* DebugOutputFormat (FUN_0047f600) formats one variadic message into the
 * same 1024-byte scratch size as the reference and emits it to the process
 * debug stream. The portable backend uses stderr for that stream. */
#if FILES_PORT_DEFINES_UNIT_NAMES
void resource_debug_output_format(const char* format, ...)
{
    char buffer[1024];
    va_list args;
    if (format == NULL) {
        return;
    }
    va_start(args, format);
    vsprintf(buffer, format, args);
    va_end(args);
    fputs(buffer, stderr);
}
#endif
/* MountWadFromPath (FUN_0047f640): reset mounted archive state and load the
 * requested archive path. */
void mount_wad_from_path(const char* path)
{
    resource_unmount_all();
    if (path != NULL) {
        char root[RESOURCE_PATH_MAX];
        char* slash;
        snprintf(root, sizeof(root), "%s", path);
        slash = strrchr(root, '/');
        {
            char* backslash = strrchr(root, '\\');
            if (backslash != NULL && (slash == NULL || backslash > slash))
                slash = backslash;
        }
        if (slash == NULL) {
            snprintf(root, sizeof(root), ".");
        }
        else if (slash == root) {
            root[1] = '\0';
        }
        else {
            *slash = '\0';
        }
        resource_directory_scan_root_set(root);
        (void)wwad_archive_index_load(path);
    }
}
/* SoundPathFallbackFormat (FUN_0047f660): only format the fallback when a
 * drive is configured. The port permits a mounted-root override and applies
 * host separators/bounds checks inside ares_cd_path. */
int ares_cd_fallback_path(const char* name, char* out, size_t out_sz)
{
    if (s_cd_drive == 0) {
        return 0;
    }
    ares_cd_path(name, out, out_sz);
    return 1;
}

/* SaveDirectoryEnsureExists (FUN_0047f6a0): the engine's directory-create
 * wrapper. The port receives the resolved save path directly rather than
 * prefixing the corpus's shared `Data` directory. */
void save_directory_ensure_exists(const char* path)
{
    if (path != NULL && path[0] != '\0') {
        LRR_MKDIR(path);
    }
}

/* GameFileOpenDispatch (FUN_0047f6e0): resolve the path, select WAD versus
 * loose-file storage, then return the corresponding GameFileHandle. The WAD
 * stream owns a fixed resource slot plus a cursor record; loose streams keep
 * their CRT FILE handle directly. */
#if FILES_PORT_DEFINES_UNIT_NAMES
GameFileHandle* game_file_open_dispatch(const char* path, const char* mode)
{
    const char* resolved;
    GameFileHandle* handle;
    int route;
    if (path == NULL || mode == NULL) return NULL;
    resolved = resource_path_resolve_against_base(path);
    if (resolved == NULL) return NULL;
    route = game_file_open_route_select(resolved, mode);
    if (route == 0) {
        char* wad_name = game_file_mounted_path_prefix_rewrite(resolved);
        Resource* resource = resource_open(wad_name, RESOURCE_WAD_ONLY);
        WadFileStreamState* state;
        int slot;
        if (resource == NULL) return NULL;
        slot = wad_entry_slot_allocate();
        if (slot < 0) {
            resource_free(resource);
            return NULL;
        }
        handle = (GameFileHandle*)game_file_handle_allocate(0);
        if (handle == NULL) {
            resource_free(resource);
            return NULL;
        }
        state = (WadFileStreamState*)handle->stream;
        state->slot = slot;
        state->cursor = 0;
        fixed_record_slot_install(slot, resource);
        return handle;
    }
    if (route != 1) {
        resource_debug_output_format(s_game_file_unknown_type_format,
            s_game_file_source_path, 0x14a, s_game_file_open_name);
        return NULL;
    }

    handle = (GameFileHandle*)game_file_handle_allocate(1);
    if (handle == NULL) return NULL;
    handle->stream = fopen(resolved, mode);
    /* The corpus gates its CD fallback on CaseInsensitiveSubstringFind(mode,
     * "w"), rather than checking the first byte. That distinction matters
     * for update modes such as "r+w": fopen may accept the path, but the
     * fallback must not redirect that writable stream to the data directory. */
    if (handle->stream == NULL &&
        case_insensitive_substring_find(mode, "w") == NULL) {
        char fallback[RESOURCE_PATH_MAX];
        if (ares_cd_fallback_path(path, fallback, sizeof(fallback)))
            handle->stream = fopen(fallback, mode);
    }
#if !defined(_WIN32)
    /* Windows refuses to open a directory; glibc opens one for reading. An
     * empty level map name resolves to the data directory itself. */
    if (handle->stream != NULL) {
        struct stat info;
        if (fstat(fileno((FILE*)handle->stream), &info) != 0 ||
            !S_ISREG(info.st_mode)) {
            fclose((FILE*)handle->stream);
            handle->stream = NULL;
        }
    }
#endif
    if (handle->stream == NULL) {
        game_file_close(handle);
        return NULL;
    }
    return handle;
}
#endif
/* WadFileSeek (FUN_0047f850): update the WAD stream cursor for SET/CUR/END.
 * The END case preserves the corpus's length-plus-offset behavior. */
#if FILES_PORT_DEFINES_UNIT_NAMES
int wad_file_seek(GameFileHandle* handle, int offset, int origin)
{
    WadFileStreamState* state;
    int length;
    uint32_t type;
    if (handle == NULL) return 0;
    type = game_file_system_type_get(handle);
    if (type == 1u) {
        if (handle->stream == NULL) return 0;
        return fseek((FILE*)handle->stream, offset, origin);
    }
    if (type != 0u) {
        resource_debug_output_format(s_game_file_unknown_type_format,
            s_game_file_source_path, 0x170, s_game_file_seek_name);
        return 0;
    }
    if (handle->stream == NULL) return 0;
    state = (WadFileStreamState*)handle->stream;
    if (origin == 0) {
        state->cursor = offset;
        length = resource_slot_length_get(state->slot);
        if (length < state->cursor) state->cursor = length;
        if (state->cursor < 0) state->cursor = 0;
    }
    else if (origin == 1) {
        state->cursor += offset;
        length = resource_slot_length_get(state->slot);
        if (length < state->cursor) state->cursor = length;
        if (state->cursor < 0) state->cursor = 0;
    }
    else if (origin == 2) {
        state->cursor = resource_slot_length_get(state->slot) + offset;
    }
    else {
        resource_debug_output_format(s_game_file_seek_mode_format, origin);
    }
    return 0;
}
#endif

/* WadFileRead (FUN_0047f980): read complete elements from the slot resource,
 * clipping the byte count at its end and advancing the cursor. */
#if FILES_PORT_DEFINES_UNIT_NAMES
int wad_file_read(void* destination, int element_size, int element_count,
                  GameFileHandle* handle)
{
    WadFileStreamState* state;
    Resource* resource;
    int length;
    unsigned int bytes;
    uint32_t type;
    if (handle == NULL) {
        return 0;
    }
    type = game_file_system_type_get(handle);
    if (type == 1u) {
        if (destination == NULL || handle->stream == NULL || element_size <= 0
            || element_count <= 0) return 0;
        return (int)fread(destination, (size_t)element_size,
                          (size_t)element_count, (FILE*)handle->stream);
    }
    if (type != 0u) {
        resource_debug_output_format(s_game_file_unknown_type_format,
            s_game_file_source_path, 0x18b, "WadFileRead");
        return 0;
    }
    if (destination == NULL || handle->stream == NULL || element_size <= 0
        || element_count <= 0) return 0;
    state = (WadFileStreamState*)handle->stream;
    resource = (Resource*)fixed_record_slot_resource_get(state->slot);
    if (resource == NULL) return 0;
    length = resource_read(resource);
    if (length <= 0 || state->cursor < 0 || state->cursor >= length) return 0;
    bytes = (unsigned int)element_size * (unsigned int)element_count;
    if (bytes > (unsigned int)(length - state->cursor)) {
        bytes = (unsigned int)(length - state->cursor);
    }
    memcpy(destination, resource->data + state->cursor, bytes);
    state->cursor += (int32_t)bytes;
    return (int)(bytes / (unsigned int)element_size);
}
#endif

/* GameFileWrite (FUN_0047fa50): writes are supported by loose-file streams;
 * WAD entries are immutable and report a zero item count. */
#if FILES_PORT_DEFINES_UNIT_NAMES
uint32_t game_file_write(const void* data, uint32_t size,
                         uint32_t count, GameFileHandle* handle)
{
    uint32_t type;
    if (handle == NULL) {
        return 0u;
    }
    type = game_file_system_type_get(handle);
    if (type == 0u) {
        resource_debug_output_format(s_game_file_write_wad_error);
        return 0u;
    }
    if (type != 1u) {
        resource_debug_output_format(s_game_file_unknown_type_format,
            s_game_file_source_path, 0x19e, s_game_file_write_name);
        return 0u;
    }
    if (handle->stream == NULL || data == NULL || size == 0u || count == 0u)
        return 0u;
    return (uint32_t)fwrite(data, (size_t)size, (size_t)count,
                            (FILE*)handle->stream);
}
#endif
#if FILES_PORT_DEFINES_UNIT_NAMES
int game_file_close(GameFileHandle* handle)
{
    if (handle == NULL) return 0;
    if (handle->flag == 0 || handle->flag == 1) {
        game_file_handle_release(handle);
        return 0;
    }
    resource_debug_output_format(s_game_file_unknown_type_format,
        s_game_file_source_path, 0x1af, s_game_file_close_name);
    return 0;
}
#endif
/* WadFileCursorGet (FUN_0047fb10): return the current WAD stream cursor. */
#if FILES_PORT_DEFINES_UNIT_NAMES
int wad_file_cursor_get(GameFileHandle* handle)
{
    WadFileStreamState* state;
    uint32_t type;
    if (handle == NULL) return 0;
    type = game_file_system_type_get(handle);
    if (type == 0u) {
        if (handle->stream == NULL) return 0;
        state = (WadFileStreamState*)handle->stream;
        return state->cursor;
    }
    if (type == 1u) {
        if (handle->stream == NULL) return 0;
        return (int)ftell((FILE*)handle->stream);
    }
    resource_debug_output_format(s_game_file_unknown_type_format,
        s_game_file_source_path, 0x1d5, s_game_file_tell_name);
    return 0;
}
#endif

/* SoundSampleFileOpenByName (FUN_0047fb60) is a presence query: the retail
 * code opens and closes loose files, but returns the matching WAD entry
 * index for an archive-backed sample. Keep the decoded Sound loader below
 * this probe so the portable audio API can still consume sample data. */
#if FILES_PORT_DEFINES_UNIT_NAMES
int sound_sample_file_open_by_name(const char* name)
{
    const char* resolved;
    char* wad_name;
    int route;
    if (name == NULL || name[0] == '\0') return 0;
    resolved = resource_path_resolve_against_base(name);
    if (resolved == NULL) return 0;
    route = game_file_open_route_select(resolved, "r");
    if (route == 1) {
        GameFileHandle* handle = game_file_open_dispatch(name, "r");
        if (handle == NULL) return 0;
        game_file_close(handle);
        return 1;
    }
    if (route == 0) {
        wad_name = game_file_mounted_path_prefix_rewrite(resolved);
        return wad_entry_find_across_mounted_archives(wad_name, -1);
    }
    return 0;
}
#endif

/* GameFileReadByte (FUN_0047fc20) and its line readers (FUN_0047fce0,
 * FUN_0047fd40): type 0 dispatches to the WAD cursor buffer and type 1 to
 * the FILE-backed CRT stream. */
#if FILES_PORT_DEFINES_UNIT_NAMES
int game_file_read_byte(GameFileHandle* handle)
{
    WadFileStreamState* state;
    Resource* resource;
    int length;
    int value;
    uint32_t type;
    if (handle == NULL) return -1;
    type = game_file_system_type_get(handle);
    if (type == 1u) {
        return handle->stream != NULL ? fgetc((FILE*)handle->stream) : -1;
    }
    if (type != 0u) {
        resource_debug_output_format(s_game_file_unknown_type_format,
            s_game_file_source_path, 0x221, s_game_file_getc_name);
        return 0;
    }
    if (handle->stream == NULL) return -1;
    state = (WadFileStreamState*)handle->stream;
    resource = (Resource*)fixed_record_slot_resource_get(state->slot);
    if (resource == NULL) return -1;
    length = resource_read(resource);
    if (length <= 0 || state->cursor < 0 || state->cursor >= length) return -1;
    value = resource->data[state->cursor++];
    return value;
}
#endif

/* GameFileLengthGet (FUN_0047fca0): query the stream length and restore its
 * original position, matching the original's save/seek-end/read-position /
 * seek-back sequence. */
long save_stream_length(FILE* fp)
{
    long position = ftell(fp);
    fseek(fp, 0, SEEK_END);
    long length = ftell(fp);
    fseek(fp, position, SEEK_SET);
    return length;
}

#if FILES_PORT_DEFINES_UNIT_NAMES
char* game_file_read_line(char* buffer, int capacity,
                          GameFileHandle* handle)
{
    int count = 0;
    int c;
    if (buffer == NULL || capacity <= 1) return NULL;
    while (count < capacity - 1) {
        c = game_file_read_byte(handle);
        if (c < 0) break;
        buffer[count++] = (char)c;
        if (c == '\0' || c == '\n') break;
    }
    if (count == 0) return NULL;
    buffer[count] = '\0';
    return buffer;
}
#endif

#if FILES_PORT_DEFINES_UNIT_NAMES
char* game_file_read_line_dispatch(char* buffer, int capacity,
                                   GameFileHandle* handle)
{
    uint32_t type;
    if (handle == NULL) return NULL;
    type = game_file_system_type_get(handle);
    if (type == 0u) {
        return game_file_read_line(buffer, capacity, handle);
    }
    if (type == 1u) {
        if (buffer == NULL || capacity <= 0 || handle->stream == NULL) {
            return NULL;
        }
        return fgets(buffer, capacity, (FILE*)handle->stream);
    }
    resource_debug_output_format(s_game_file_unknown_type_format,
        s_game_file_source_path, 0x24b, s_game_file_gets_name);
    return NULL;
}
#endif

/* GameFilePrintFormat (FUN_0047fdb0): forward formatted text to a loose
 * stream, returning the corpus's fixed zero status. */
#if FILES_PORT_DEFINES_UNIT_NAMES
int game_file_print_format(GameFileHandle* handle, const char* format, ...)
{
    uint32_t type;
    va_list args;
    if (handle == NULL) return 0;
    type = game_file_system_type_get(handle);
    if (type == 0u) {
        resource_debug_output_format(s_game_file_printf_wad_error);
        return 0;
    }
    if (type != 1u) {
        resource_debug_output_format(s_game_file_unknown_type_format,
            s_game_file_source_path, 0x264, s_game_file_printf_name);
        return 0;
    }
    if (handle->stream == NULL || format == NULL) return 0;
    va_start(args, format);
    (void)vfprintf((FILE*)handle->stream, format, args);
    va_end(args);
    return 0;
}
#endif

/* GameFileSystemTypeGet (FUN_0047fe20, findings 234/404): the handle's
 * leading type dword. */
uint32_t game_file_system_type_get(const void* handle)
{
    if (handle == NULL) {
        return 0u; /* pinned deviation: the corpus loads blindly */
    }
    return *(const uint32_t*)handle;
}

/* FileOpenRouteSelect (FUN_0047fe30): choose loose-file handling for
 * non-write modes only when the rewritten path is not present in a mounted
 * archive. */
#if FILES_PORT_DEFINES_UNIT_NAMES
int game_file_open_route_select(const char* path, const char* mode)
{
    char* rewritten;
    if (path == NULL || path[0] == '\0' || mode == NULL || mode[0] == '\0')
        return 2;
    if (mode[0] == 'w' || mode[0] == 'W') return 1;
    rewritten = game_file_mounted_path_prefix_rewrite(path);
    return wad_entry_find_across_mounted_archives(rewritten, -1) == -1;
}
#endif

/* WadEntryReadIntoStreamSlot (FUN_0047fea0): zero the slot pair behind the
 * handle, resolve the entry through the corpus's wad path opener with the
 * search-all selector, and answer whether the entry exists. Deviations:
 * the opener's record-slot handle (its slot table is modeled in
 * fixed_record_slot_resource_get) rides as the opened resource pointer and
 * the corpus's -1 sentinel becomes NULL; the search-all index arrives as
 * the wad-only open mode with the corpus's global at -1 inlined. */
int wad_entry_read_into_stream_slot(WadStreamSlots* slots, const char* name)
{
    if (slots == NULL) {
        return 0; /* deviation: the corpus strides the slots blindly */
    }
    slots->word2 = 0u;
    slots->stream = NULL;
    const Resource* handle = resource_open(name, RESOURCE_WAD_ONLY);
    slots->handle = handle;
    return handle != NULL;
}
/* GameFileHandleAllocate (FUN_0047fed0): the file-layer handle pair. Kind 1
 * (wads) is the 8-byte record flagged 1 with no stream buffer; kind 0 (the
 * loose-file route) flags 0 and carries the 12-byte stream buffer behind,
 * releasing the record if the buffer alloc fails. Deviations: the record
 * fields widen to (int32_t flag; void* stream) for LP64 (the corpus's
 * dwords, 8 bytes, alias this record's front half) and the corpus's
 * blind-first store for kind 1 is guarded. The corpus's wrapper chain
 * 0x47ff30 is a bare call into the heap's alloc entry. */
#if FILES_PORT_DEFINES_UNIT_NAMES
void* game_file_handle_allocate(int kind)
{
    GameFileHandle* h;
    if (kind == 1) {
        h = (GameFileHandle*)engine_heap_alloc((long)sizeof(GameFileHandle));
        if (h == NULL) {
            return NULL;
        }
        h->flag = 1;
        /* Test-pinned deviation: unlike the corpus, the port leaves the
         * kind-1 stream slot deterministically NULL. */
        h->stream = NULL;
        return h;
    }
    if (kind != 0) {
        return NULL;
    }
    h = (GameFileHandle*)engine_heap_alloc((long)sizeof(GameFileHandle));
    if (h == NULL) {
        return NULL;
    }
    h->flag = 0;
    h->stream = engine_heap_alloc(12);
    if (h->stream != NULL) {
        return h;
    }
    engine_heap_free_wrapper(h);
    return NULL;
}
#endif

void engine_heap_allocate(long size)
{
    (void)engine_heap_alloc(size);
}
#if FILES_PORT_DEFINES_UNIT_NAMES
void engine_heap_free_wrapper(void* ptr)
{
    engine_heap_free(ptr);
}
#endif

#if FILES_PORT_DEFINES_UNIT_NAMES
void game_file_handle_release(GameFileHandle* handle)
{
    if (handle == NULL) return;
    if (handle->flag == 1) {
        if (handle->stream != NULL) fclose((FILE*)handle->stream);
        engine_heap_free_wrapper(handle);
        return;
    }
    else if (handle->flag == 0) {
        WadFileStreamState* state = (WadFileStreamState*)handle->stream;
        if (state != NULL) {
            fixed_record_slot_release(state->slot);
            engine_heap_free_wrapper(state);
        }
    }
    else {
        return;
    }
    engine_heap_free_wrapper(handle);
}
#endif
#if FILES_PORT_DEFINES_UNIT_NAMES
char* game_file_mounted_path_prefix_rewrite(const char* path)
{
    size_t path_length;
    size_t prefix_length;
    const char* relative_path;
    char prefix[0x400];
    if (!s_wad_mount_path_cache_valid || path == NULL) return (char*)path;
    path_length = strlen(path);
    prefix_length = strlen(s_wad_mount_path_cache);
    if (path_length <= prefix_length || prefix_length == 0u
        || prefix_length >= sizeof(prefix)) return (char*)path;
    memcpy(prefix, path, prefix_length);
    prefix[prefix_length] = '\0';
    if (strcasecmp_lite(prefix, s_wad_mount_path_cache) != 0)
        return (char*)path;
    relative_path = path + prefix_length;
    /* PathResolveAgainstBase forms `<Data root><separator><name>`, while the
     * WAD directory stores names without a leading separator. Strip that
     * join separator at the root boundary before looking up the archive
     * entry; otherwise a valid `Lego.cfg` is treated as a loose file. */
    while (*relative_path == '/' || *relative_path == '\\') {
        ++relative_path;
    }
    snprintf(s_wad_mount_path_rewrite, sizeof(s_wad_mount_path_rewrite),
             "%s", relative_path);
    return s_wad_mount_path_rewrite;
}
#endif

#if FILES_PORT_DEFINES_UNIT_NAMES
char* game_file_read_crlf_line(char* buffer, int capacity,
                               GameFileHandle* handle)
{
    char* result = game_file_read_line_dispatch(buffer, capacity, handle);
    size_t length;
    if (result == NULL) return NULL;
    length = strlen(buffer);
    if (length != 0u && buffer[length - 1u] == '\n') {
        buffer[--length] = '\0';
    }
    if (length != 0u && buffer[length - 1u] == '\r') {
        buffer[length - 1u] = '\0';
    }
    return result;
}
#endif

#if FILES_PORT_DEFINES_UNIT_NAMES
int map_open2(const char* path, int32_t* size_out)
{
    GameFileHandle* handle = game_file_open_dispatch(
        path, s_map_open_binary_mode);
    if (handle != NULL) {
        int size;
        int slot;
        void* map_data;
        (void)wad_file_seek(handle, 0, 2);
        size = wad_file_cursor_get(handle);
        if (s_loader_progress_callback != NULL) {
            typedef void (*ProgressProc)(const char*, uint32_t, void*);
            ((ProgressProc)s_loader_progress_callback)(
                path, (uint32_t)size, s_loader_progress_context);
        }
        slot = level_map_slot_allocate((size_t)size);
        if (slot != -1) {
            map_data = level_map_slot_get((uint32_t)slot);
            (void)wad_file_seek(handle, 0, 0);
            (void)wad_file_read(map_data, 1, size, handle);
            if (size_out != NULL) *size_out = size;
            (void)game_file_close(handle);
            return slot;
        }
        (void)game_file_close(handle);
    }
    return -1;
}
#endif

/* PathResolveAgainstBase (FUN_00480220): form a path under the configured
 * data root and reject parent-directory components that escape that root. */
#if FILES_PORT_DEFINES_UNIT_NAMES
const char* resource_path_resolve_against_base(const char* path)
{
    static char resolved[RESOURCE_PATH_MAX];
    char base[RESOURCE_PATH_MAX];
    const char* part;
    size_t length;
    if (path == NULL) return NULL;
    if (*path == '\\') ++path;
    length = strlen(path);
    if (length >= sizeof(resolved)) return NULL;
    if (*path == '/' || strchr(path, ':') != NULL) return NULL;
    part = path;
    while (*part != '\0') {
        const char* end = part;
        size_t segment_length;
        while (*end != '\0' && *end != '/' && *end != '\\') ++end;
        segment_length = (size_t)(end - part);
        if (segment_length == 2 && part[0] == '.' && part[1] == '.') {
            return NULL;
        }
        part = (*end == '\0') ? end : end + 1;
    }
    ares_cd_path("", base, sizeof(base));
    if (strlen(base) + length >= sizeof(resolved)) return NULL;
    snprintf(resolved, sizeof(resolved), "%s%s", base, path);
    return resolved;
}
#endif

#if FILES_PORT_DEFINES_UNIT_NAMES
void loader_progress_callback_set(void* callback, void* context)
{
    s_loader_progress_callback = callback;
    s_loader_progress_context = context;
}
#endif

/* FileTokenListLoadAndTreeScan (FUN_004802d0): retail ABI takes only the
 * token filename; root and token table are module-owned loader state. */
#if FILES_PORT_DEFINES_UNIT_NAMES
void resource_file_token_list_load_and_tree_scan_target(
    const char* token_file)
{
    int result;
    if (token_file == NULL) return;
    result = resource_file_token_list_parse(token_file,
                                            &s_resource_file_tokens, 0);
    if (result < 0) return;
    if (s_resource_directory_root[0] != '\0')
        resource_directory_tree_scan_target(s_resource_directory_root);
}
#endif

/* DirectoryTreeScan (FUN_00480390): retail ABI takes one root and reads the
 * process-wide token state populated by FileTokenListLoadAndTreeScan. */
#if FILES_PORT_DEFINES_UNIT_NAMES
void resource_directory_tree_scan_target(const char* root)
{
    ResourceDirectoryScanHandle* scan = NULL;
    char pattern[1024];
    char name[260];
    char child_path[1024];
    unsigned int root_length;
    unsigned int offset;
    if (root == NULL) return;

    /* Keep the target's local root\*.* pattern construction here instead
     * of forwarding through the portable explicit-state walker. */
    root_length = (unsigned int)strlen(root);
    if (root_length + 5u >= sizeof(pattern)) return;
    memcpy(pattern, root, root_length);
    offset = root_length;
    if (offset != 0u && pattern[offset - 1u] != '/' &&
        pattern[offset - 1u] != '\\') {
        pattern[offset++] = '\\';
    }
    memcpy(pattern + offset, "*.*", 4u);
    if (!resource_directory_scan_default_open(NULL, pattern,
                                               (void**)&scan) ||
        scan == NULL) return;

    for (;;) {
        int is_directory = 0;
        unsigned int name_length;
        int next;
        name[0] = '\0';
        next = resource_directory_scan_default_next(NULL, scan, name,
                                                     &is_directory);
        if (next <= 0) break;
        name[259] = '\0';
        if (name[0] == '\0' || strcmp(name, ".") == 0 ||
            strcmp(name, "..") == 0) continue;
        root_length = (unsigned int)strlen(root);
        name_length = (unsigned int)strlen(name);
        if (root_length + name_length + 2u >= sizeof(child_path)) continue;
        memcpy(child_path, root, root_length);
        offset = root_length;
        if (offset != 0u && child_path[offset - 1u] != '/' &&
            child_path[offset - 1u] != '\\') {
#if defined(_WIN32)
            child_path[offset++] = '\\';
#else
            child_path[offset++] = '/';
#endif
        }
        memcpy(child_path + offset, name, name_length + 1u);
        if (is_directory != 0) {
            resource_directory_tree_scan_target(child_path);
        }
        else {
            /* Retail calls the comparator for its side effect and ignores
             * the membership result. */
            (void)resource_directory_token_list_contains(
                &s_resource_file_tokens, child_path);
        }
    }
    resource_directory_scan_default_close(NULL, scan);
}
#endif
/* DirectoryTokenListCompare (FUN_00480570): find a token with the same
 * ASCII case-folded spelling. The original discards the comparison result;
 * this API exposes it to the reconstructed directory walker. */
int resource_directory_token_list_contains(
    const ResourceFileTokenList* list, const char* token)
{
    uint32_t index;
    if (list == NULL || token == NULL) return 0;
    for (index = 0u; index < list->count && index < 512u; ++index) {
        if (strcasecmp_lite(token, list->tokens[index]) == 0) return 1;
    }
    return 0;
}
static int resource_cd_key_path_exists(const char* path)
{
    FILE* fp;
    if (s_cd_path_exists != NULL) {
        return s_cd_path_exists(path, s_cd_probe_context);
    }
    fp = fopen(path, "rb");
    if (fp == NULL) return 0;
    fclose(fp);
    return 1;
}

static int resource_startup_current_directory(char* path, size_t capacity)
{
    if (path == NULL || capacity == 0u) return 0;
#if defined(_WIN32)
    {
        DWORD length = GetCurrentDirectoryA((DWORD)capacity, path);
        return length != 0u && length < capacity;
    }
#else
    return getcwd(path, capacity) != NULL;
#endif
}

static int resource_startup_directory_exists(const char* path)
{
#if defined(_WIN32)
    DWORD attributes = GetFileAttributesA(path);
    return attributes != INVALID_FILE_ATTRIBUTES
        && (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0u;
#else
    struct stat status;
    return stat(path, &status) == 0 && S_ISDIR(status.st_mode);
#endif
}

static void resource_startup_directory_create(const char* path)
{
#if defined(_WIN32)
    (void)CreateDirectoryA(path, NULL);
#else
    (void)mkdir(path, 0755);
#endif
}

static int resource_startup_localized_text(const char* registry_key,
                                           const char* text_key,
                                           char* output, size_t capacity)
{
    int loaded;
    if (output == NULL || capacity == 0u) return 0;
    output[0] = '\0';
    if (s_resource_startup_ops.text_lookup == NULL) return 0;
    loaded = s_resource_startup_ops.text_lookup(
        s_resource_startup_ops.context, registry_key, text_key,
        output, capacity);
    output[capacity - 1u] = '\0';
    return loaded != 0 && output[0] != '\0';
}

static void resource_startup_exit(int result)
{
    s_resource_startup_result = result;
    if (s_resource_startup_ops.exit_process != NULL) {
        s_resource_startup_ops.exit_process(s_resource_startup_ops.context, 0);
    }
}

static int resource_startup_missing_data_dialog(const char* registry_key)
{
    char message[1024];
    char caption[128];
    if (s_resource_startup_ops.message_box == NULL ||
        !resource_startup_localized_text(registry_key, "DataMissing",
                                         message, sizeof(message)) ||
        !resource_startup_localized_text(registry_key, "SetupError",
                                         caption, sizeof(caption))) {
        s_resource_startup_result = RESOURCE_STARTUP_DEPENDENCY_MISSING;
        return 0;
    }
    (void)s_resource_startup_ops.message_box(s_resource_startup_ops.context,
                                             message, caption, 0u);
    resource_startup_exit(RESOURCE_STARTUP_MISSING_DATA);
    return 1;
}

static int resource_startup_cd_retry(const char* registry_key)
{
    char message[1024];
    char caption[128];
    if (s_resource_startup_ops.message_box == NULL ||
        !resource_startup_localized_text(registry_key, "CDMissing",
                                         message, sizeof(message)) ||
        !resource_startup_localized_text(registry_key, "SetupError",
                                         caption, sizeof(caption))) {
        s_resource_startup_result = RESOURCE_STARTUP_DEPENDENCY_MISSING;
        return 0;
    }
    if (s_resource_startup_ops.message_box(s_resource_startup_ops.context,
                                           message, caption, 1u) == 2) {
        resource_startup_exit(RESOURCE_STARTUP_CD_CANCELLED);
        return 0;
    }
    return 1;
}

static void resource_startup_delme_marker_remove(const char* path)
{
#if defined(_WIN32)
    DWORD attributes = GetFileAttributesA(path);
    if (attributes != INVALID_FILE_ATTRIBUTES) {
        (void)SetFileAttributesA(path, attributes & ~FILE_ATTRIBUTE_READONLY);
        (void)DeleteFileA(path);
    }
#else
    struct stat status;
    if (stat(path, &status) == 0) {
        (void)chmod(path, status.st_mode | S_IWUSR);
        (void)remove(path);
    }
#endif
}

/* Port-side payload length used by the file seek implementation. */
static int resource_slot_length_get(int slot)
{
    Resource* resource;
    int32_t length;
    resource = (Resource*)fixed_record_slot_resource_get(slot);
    if (resource == NULL) {
        return 0;
    }
    length = resource_read(resource);
    return length > 0 ? (int)length : 0;
}

static void fixed_record_slot_release(int slot)
{
    if (slot < 0 || slot >= FIXED_RECORD_SLOTS) return;
    if (fixed_record_slot_pointer_get(slot) != NULL) {
        resource_free((Resource*)fixed_record_slot_pointer_get(slot));
    }
#if defined(_WIN64) || defined(__x86_64__) || defined(__aarch64__)
    s_fixed_slot_resources[slot] = NULL;
#endif
    s_fixed_slots[slot].resource = 0u;
    s_fixed_slots[slot].in_use = 0;
}

/* File_GetCDFilePath (FUN_0047f660) with the original's (out, name)
 * argument order, for gods98/sound3d.c natively (ADR 0005 step 6); files.c
 * defines it in the matching build. The out buffer is MAX_PATH bytes
 * there. */
#if FILES_PORT_DEFINES_UNIT_NAMES
extern "C" int resource_cd_file_path_get(char* out, const char* name)
{
    return ares_cd_fallback_path(name, out, 260);
}
#endif

/* --- Wad.c adapters (gods98/wad.h) -----------------------------------------
 * The original-contract Wad.c entries files.c calls where the port's
 * function at that address has another contract. Wad.c's conversion
 * replaces them. A stream handle is a slot of the port's fixed-record table
 * (resource.cpp) holding the entry's data, allocated with the C library
 * heap, so that fixed_record_slot_resource_release (Wad_FileClose, 0048bfc0)
 * frees it as the original does. */
static int s_files_wad_file_lengths[FIXED_RECORD_SLOTS];

/* Wad_Load (0048b4e0): the port's wwad_archive_index_load has the contract,
 * but its only declaration (resource_from_wad.hpp, frozen) has C++
 * linkage. */
extern "C" int wad_load(const char* fName)
{
    return wwad_archive_index_load(fName);
}

/* Wad_FileOpen (0048bf70): the port's ares_open_wad_path returns a Resource
 * record. Archive selection follows wad_entry_find_across_mounted_archives;
 * files.c always passes -1 (every archive, newest first). */
extern "C" int wad_file_open(const char* fName, int wadNo)
{
    Resource* resource;
    int32_t length;
    void* data;
    int slot;
    (void)wadNo;
    resource = ares_open_wad_path(fName, RESOURCE_WAD_ONLY);
    if (resource == NULL) return -1;
    length = resource_read(resource);
    slot = wad_entry_slot_allocate();
    if (length < 0 || slot < 0) {
        resource_free(resource);
        return -1;
    }
    data = engine_heap_alloc(length > 0 ? (long)length : 1L);
    if (data == NULL) {
        resource_free(resource);
        return -1;
    }
    if (length > 0) memcpy(data, resource->data, (size_t)length);
    resource_free(resource);
    fixed_record_slot_install(slot, data);
    s_files_wad_file_lengths[slot] = (int)length;
    return slot;
}

/* Wad_FileLength (0048c010): the port's wad_stream_length_get returns
 * nothing. */
extern "C" int wad_file_length(int handle)
{
    if ((unsigned int)handle >= FIXED_RECORD_SLOTS) return 0;
    return s_files_wad_file_lengths[handle];
}

/* Wad_FileData (0048c030): the port's wad_memory_stream_buffer_get returns
 * nothing. */
extern "C" void* wad_file_data(int handle)
{
    return fixed_record_slot_resource_get(handle);
}

/* The original's debug-message calls go to an empty variadic function
 * (00484c90). files.c calls this one when it is compiled as C++. */
extern "C" void files_debug_noop(const char* msg, ...)
{
    (void)msg;
}

#if !defined(_WIN32)
/* --- Native stand-ins (gods98/files_native.h) ------------------------------
 * The VC5 LIBC directory routines and the Win32 calls files.c makes, over
 * POSIX. Backslashes in the paths files.c builds are host separators here. */
#include "files_native.h"

static void files_native_host_path(const char* path, char* out, size_t size)
{
    size_t i;
    for (i = 0; path[i] != '\0' && i + 1 < size; ++i)
        out[i] = path[i] == '\\' ? '/' : path[i];
    out[i] = '\0';
}

typedef struct FilesNativeFind {
    DIR* directory;
    char root[1024];
} FilesNativeFind;
static FilesNativeFind s_files_native_finds[16];

static int files_native_find_fill(FilesNativeFind* find,
                                  struct files_native_finddata* info)
{
    struct dirent* entry;
    struct stat status;
    char path[2048];
    if ((entry = readdir(find->directory)) == NULL) return -1;
    memset(info, 0, sizeof(*info));
    snprintf(info->name, sizeof(info->name), "%s", entry->d_name);
    snprintf(path, sizeof(path), "%s/%s", find->root, entry->d_name);
    if (stat(path, &status) == 0) {
        if (S_ISDIR(status.st_mode)) info->attrib |= _A_SUBDIR;
        info->size = (unsigned long)status.st_size;
        info->time_write = (long)status.st_mtime;
    }
    return 0;
}

extern "C" long files_native_findfirst(const char* filespec,
                                       struct files_native_finddata* fileinfo)
{
    char path[1024];
    char* slash;
    long handle;
    files_native_host_path(filespec, path, sizeof(path));
    /* The pattern is always "*.*": every entry. */
    slash = strrchr(path, '/');
    if (slash != NULL) *slash = '\0';
    else snprintf(path, sizeof(path), ".");
    for (handle = 0; handle < 16; ++handle) {
        FilesNativeFind* find = &s_files_native_finds[handle];
        if (find->directory != NULL) continue;
        if ((find->directory = opendir(path)) == NULL) return -1;
        snprintf(find->root, sizeof(find->root), "%s", path);
        if (files_native_find_fill(find, fileinfo) != 0) {
            closedir(find->directory);
            find->directory = NULL;
            return -1;
        }
        return handle;
    }
    return -1;
}

extern "C" int files_native_findnext(long handle,
                                     struct files_native_finddata* fileinfo)
{
    if (handle < 0 || handle >= 16 ||
        s_files_native_finds[handle].directory == NULL)
        return -1;
    return files_native_find_fill(&s_files_native_finds[handle], fileinfo);
}

extern "C" int files_native_findclose(long handle)
{
    if (handle < 0 || handle >= 16 ||
        s_files_native_finds[handle].directory == NULL)
        return -1;
    closedir(s_files_native_finds[handle].directory);
    s_files_native_finds[handle].directory = NULL;
    return 0;
}

extern "C" int files_native_chmod(const char* path, int mode)
{
    char host[1024];
    struct stat status;
    files_native_host_path(path, host, sizeof(host));
    if (stat(host, &status) != 0) return -1;
    return chmod(host, (mode & 0x80) != 0 ? (status.st_mode | S_IWUSR)
                                          : (status.st_mode & ~S_IWUSR));
}

extern "C" char* files_native_getcwd(char* buffer, int maxlen)
{
    return getcwd(buffer, (size_t)maxlen);
}

extern "C" int files_native_mkdir(const char* path)
{
    char host[1024];
    files_native_host_path(path, host, sizeof(host));
    return mkdir(host, 0755);
}

extern "C" char* files_native_fullpath(char* absPath, const char* relPath,
                                       unsigned int maxLength)
{
    char cwd[1024];
    int written;
    if (relPath[0] == '/' || relPath[0] == '\\') {
        written = snprintf(absPath, maxLength, "%s", relPath);
    } else {
        if (getcwd(cwd, sizeof(cwd)) == NULL) return NULL;
        written = snprintf(absPath, maxLength, "%s/%s", cwd, relPath);
    }
    if (written < 0 || (unsigned int)written >= maxLength) return NULL;
    return absPath;
}

extern "C" unsigned int files_native_get_drive_type(const char* rootPathName)
{
    (void)rootPathName;
    return 1; /* DRIVE_NO_ROOT_DIR: no drive letters */
}

extern "C" int files_native_message_box(void* window, const char* text,
                                        const char* caption, unsigned int type)
{
    (void)window;
    (void)type;
    fprintf(stderr, "%s: %s\n", caption, text);
    return 1; /* IDOK */
}

extern "C" void files_native_output_debug_string(const char* text)
{
    fputs(text, stderr);
}
#endif
