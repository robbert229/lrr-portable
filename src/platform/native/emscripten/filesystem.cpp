// SPDX-License-Identifier: LGPL-3.0-or-later
/* The web build's filesystem (filesystem.h). Adapted from isle-portable's
 * ISLE/emscripten/filesystem.cpp (LGPL-3.0): the configuration and save
 * parts, and the streamed game data. isle-portable registers a compiled-in
 * list of its files on a WasmFS fetch backend, skipping those it bundled;
 * this registers the files the host's manifest lists (manifest.h), and
 * skips all of them when the data directory is already there. */
#include "platform/native/emscripten/filesystem.h"

#include "gods98/files_native.h"
#include "platform/native/emscripten/events.h"
#include "platform/native/emscripten/manifest.h"

#include <SDL3/SDL_error.h>
#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_iostream.h>
#include <SDL3/SDL_stdinc.h>
#include <emscripten.h>
#include <emscripten/wasmfs.h>

#include <set>
#include <string>
#include <vector>

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

static backend_t opfs = nullptr;
static backend_t fetchfs = nullptr;

/* Makes each directory along `p_path` on the OPFS backend. */
static void Emscripten_CreateOPFSDirectories(std::string p_path)
{
    char* parse = &p_path[0];
    while ((parse = SDL_strchr(++parse, '/'))) {
        *parse = '\0';
        wasmfs_create_directory(p_path.c_str(), 0644, opfs);
        *parse = '/';
    }
}

bool Emscripten_OPFSDisabled()
{
    return MAIN_THREAD_EM_ASM_INT({return !!Module["disableOpfs"]});
}

bool Emscripten_SetupConfig(const char* p_iniConfig)
{
    if (Emscripten_OPFSDisabled()) {
        SDL_Log("OPFS is disabled; ignoring .ini path");
        return false;
    }

    opfs = wasmfs_create_opfs_backend();
    Emscripten_CreateOPFSDirectories(p_iniConfig);
    return true;
}

/* Registers `p_path` (relative to the data directory) on the fetch backend,
 * making the directories above it. A fetch backend builds a file's URL from
 * the names of the directories above it, so each directory is made on the
 * backend (a file made in another backend's directory would fetch the base
 * URL itself). Returns whether it was registered. */
static bool Emscripten_RegisterDataFile(const std::string& p_path, std::set<std::string>& p_directories)
{
    std::string full = std::string(Emscripten_dataPath) + "/" + p_path;
    std::string directory;
    size_t slash = 0;

    while ((slash = full.find('/', slash + 1)) != std::string::npos) {
        directory = full.substr(0, slash);
        if (p_directories.insert(directory).second && directory != Emscripten_dataPath) {
            wasmfs_create_directory(&directory[0], 0755, fetchfs);
        }
    }

    int fd = wasmfs_create_file(full.c_str(), 0644, fetchfs);
    if (fd < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Cannot register \"%s\" for streaming", p_path.c_str());
        return false;
    }

    /* The file is only registered: it is opened, and fetched, when read. */
    close(fd);
    return true;
}

/* Reads `p_path` through to the end, which fetches all of it, sending the
 * page the share fetched. The game reads most of LegoRR0.wad before its
 * first menu, so fetching a WAD up front, a few chunks per request, costs
 * little over what the game fetches itself, and the page can show a progress
 * bar where the game, blocked in its first reads, shows nothing. It reads
 * with read(), not stdio: stdio splits a large read into a direct read and
 * one through its buffer, which fetches the chunk at the seam twice. */
static void Emscripten_PreloadDataFile(const std::string& p_name)
{
    std::string path = std::string(Emscripten_dataPath) + "/" + p_name;
    int fd = open(path.c_str(), O_RDONLY);
    struct stat info;
    if (fd < 0 || fstat(fd, &info) != 0) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Cannot preload %s", path.c_str());
        if (fd >= 0) {
            close(fd);
        }
        return;
    }

    /* A few chunks at a time, which one range request fetches. */
    const size_t slice = SDL_min(8 * (size_t) LRR_EMSCRIPTEN_CHUNK_SIZE, (size_t) 4 * 1024 * 1024);
    std::vector<char> buffer(slice);
    off_t size = info.st_size;
    off_t done = 0;
    int progress = -1;

    while (done < size) {
        ssize_t read = ::read(fd, buffer.data(), buffer.size());
        if (read <= 0) {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Preloading %s stopped at %lld of %lld bytes", path.c_str(), (long long) done, (long long) size);
            break;
        }

        done += read;
        int now = (int) (done * 100 / size);
        if (now != progress) {
            progress = now;
            Emscripten_SendDataProgress(p_name.c_str(), progress);
        }
    }
    close(fd);
}

/* Streams the game data: a fetch backend rooted at <host>/LegoRR, mounted
 * at Emscripten_dataPath (the diskpath), with each file of the manifest
 * registered. A file is fetched when the game first reads or sizes it, with
 * range requests for a file over twice the chunk size (the WADs). */
static void Emscripten_SetupDataFilesystem()
{
    /* Data the page bundled (--preload-file) is already there; the fetch
     * backend cannot add the files it lacks, as it can only make a file in
     * one of its own directories. */
    if (SDL_GetPathInfo(Emscripten_dataPath, NULL)) {
        SDL_Log("%s is bundled and won't be streamed", Emscripten_dataPath);
        return;
    }

    std::string baseUrl = std::string(Emscripten_streamHost) + Emscripten_dataPath;
    fetchfs = wasmfs_create_fetch_backend(baseUrl.c_str(), LRR_EMSCRIPTEN_CHUNK_SIZE);
    std::set<std::string> directories;

    std::string root = Emscripten_dataPath;
    wasmfs_create_directory(&root[0], 0755, fetchfs);

    /* The manifest is the first file, which tells which others there are. */
    std::string manifestPath = root + "/" LRR_DATA_MANIFEST_NAME;
    if (!Emscripten_RegisterDataFile(LRR_DATA_MANIFEST_NAME, directories)) {
        return;
    }

    size_t length = 0;
    void* data = SDL_LoadFile(manifestPath.c_str(), &length);
    if (!data || length == 0) {
        /* A host that answers 404 reads as an empty file. */
        SDL_free(data);
        std::string message = "Cannot read the game data manifest " + baseUrl + "/" LRR_DATA_MANIFEST_NAME;
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", message.c_str());
        Emscripten_SendDataError(message.c_str());
        return;
    }

    std::vector<std::string> files;
    size_t dropped = DataManifest_Parse(static_cast<const char*>(data), length, files);
    SDL_free(data);
    SDL_RemovePath(manifestPath.c_str());

    if (dropped > 0) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "The game data manifest has %zu unsafe paths; ignored", dropped);
    }

    size_t registered = 0;
    for (const std::string& file : files) {
        if (Emscripten_RegisterDataFile(file, directories)) {
            registered++;
        }
    }
    SDL_Log("%zu of %zu game data files set up for streaming from %s", registered, files.size(), baseUrl.c_str());
    if (registered == 0) {
        Emscripten_SendDataError("The game data manifest lists no files");
        return;
    }

#if LRR_EMSCRIPTEN_PRELOAD_WADS
    for (const std::string& file : files) {
        if (file.size() > 4 && SDL_strcasecmp(file.c_str() + file.size() - 4, ".wad") == 0) {
            Emscripten_PreloadDataFile(file);
        }
    }
#endif
    Emscripten_SendDataReady();
}

void Emscripten_SetupFilesystem()
{
    Emscripten_SetupDataFilesystem();

    const char* savePath = files_native_save_path();

    if (savePath && *savePath && !Emscripten_OPFSDisabled()) {
        if (!opfs) {
            opfs = wasmfs_create_opfs_backend();
        }

        std::string path = savePath;
        if (path[path.length() - 1] != '/') {
            path += "/";
        }

        Emscripten_CreateOPFSDirectories(path);
    }
}
