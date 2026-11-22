// SPDX-License-Identifier: LGPL-3.0-or-later
/* The filesystem substitution (ADR 0007, [library:filesystem]): the VC5
 * LIBC directory routines and the KERNEL32 calls outside miniwin's scope
 * that gods98/files.c makes, the Windows-to-host path mapping every file
 * call goes through, and the save directory (gods98/files_native.h).
 *
 * The mapping plays the part of isle-portable's
 * MxString::MapPathToFilesystem: the units build Windows paths, '\\'
 * separators and names whose case need not match the files on disk, and
 * each file call maps its path here first. */
#include <SDL3/SDL.h>

#include "gods98/files_native.h"
#include "platform/native/emscripten/events.h"

#include <cstring>
#include <map>
#include <string>
#include <vector>

namespace {

std::string g_savePath;
std::string g_diskPath;

/* A directory as configured, without the trailing separator that would
 * double up once a name is added. */
std::string TrimDirectory(const char* path)
{
    std::string dir = path != NULL ? path : "";
    while (dir.size() > 1 && (dir.back() == '/' || dir.back() == '\\')) {
        dir.pop_back();
    }
    return dir;
}

/* '\\' separators become '/'. */
void ToHostSeparators(std::string& path)
{
    for (char& c : path) {
        if (c == '\\') c = '/';
    }
}

/* Whether a path with host separators is absolute: rooted ("/..."), or
 * with a drive or scheme as its first component ("C:/...", the 3DS and
 * Switch "sdmc:/..."). */
bool IsAbsolute(const std::string& path)
{
    if (!path.empty() && path[0] == '/') return true;
    size_t colon = path.find(':');
    return colon != std::string::npos && colon > 0 && colon < path.find('/');
}

/* `path` taken from the working directory when relative, with its "." and
 * ".." components resolved, so it can prefix a relative path once. */
std::string AbsoluteDirectory(const std::string& path)
{
    std::string text = path;
    ToHostSeparators(text);
    if (!IsAbsolute(text)) {
        char* cwd = SDL_GetCurrentDirectory();
        if (cwd != NULL) {
            std::string base = cwd;
            SDL_free(cwd);
            ToHostSeparators(base);
            if (!base.empty() && base.back() != '/') base += '/';
            text = base + text;
        }
    }
    /* The root: "/", or a drive or scheme with its separator ("sdmc:/"). */
    std::string root;
    size_t start = 0;
    if (text[0] == '/') {
        root = "/";
        start = 1;
    }
    else if (IsAbsolute(text)) {
        size_t slash = text.find('/');
        start = slash == std::string::npos ? text.size() : slash + 1;
        root = text.substr(0, start);
    }
    std::vector<std::string> parts;
    while (start < text.size()) {
        size_t end = text.find('/', start);
        if (end == std::string::npos) end = text.size();
        std::string part = text.substr(start, end - start);
        start = end + 1;
        if (part.empty() || part == ".") continue;
        if (part == ".." && !parts.empty() && parts.back() != "..") parts.pop_back();
        else if (part != ".." || root.empty()) parts.push_back(part);
    }
    std::string result = root;
    for (size_t i = 0; i < parts.size(); i++) {
        if (i > 0) result += '/';
        result += parts[i];
    }
    return result.empty() ? "." : result;
}

struct CaseMatch {
    const char* want;
    std::string found;
};

SDL_EnumerationResult SDLCALL MatchEntryCase(void* userdata, const char* dirname,
                                             const char* fname)
{
    (void)dirname;
    CaseMatch* match = static_cast<CaseMatch*>(userdata);
    if (SDL_strcasecmp(fname, match->want) == 0) {
        match->found = fname;
        return SDL_ENUM_SUCCESS;
    }
    return SDL_ENUM_CONTINUE;
}

/* Whether `path` is components joined by single separators, none of them
 * "." or "..": the walk below would spell it the same. */
bool IsPlainRelative(const std::string& path)
{
    size_t start = 0;
    if (path.empty()) return false;
    while (start <= path.size()) {
        size_t end = path.find('/', start);
        if (end == std::string::npos) end = path.size();
        const size_t length = end - start;
        if (length == 0 || (length == 1 && path[start] == '.') ||
            (length == 2 && path[start] == '.' && path[start + 1] == '.')) {
            return false;
        }
        start = end + 1;
    }
    return true;
}

/* Whether HostPath matches case from listings it keeps, one per directory
 * read once, rather than asking the disk for every component of every
 * path. The 3DS reads its SD card through FS:USER, where each lookup is a
 * slow service call, and the game probes the disk for many files that only
 * its WADs hold: matching their case on the disk made the start-up take
 * minutes. A listing is not read again, so a file made after it keeps the
 * spelling asked for, which the 3DS's FAT card opens in any case. */
#if defined(__3DS__)
constexpr bool kKeepListings = true;
#else
constexpr bool kKeepListings = false;
#endif

std::map<std::string, std::vector<std::string> > g_listings;

SDL_EnumerationResult SDLCALL CollectEntry(void* userdata, const char* dirname, const char* fname)
{
    (void)dirname;
    static_cast<std::vector<std::string>*>(userdata)->push_back(fname);
    return SDL_ENUM_CONTINUE;
}

/* The spelling of `part` in `dir`: as spelled when it is there, else the
 * entry that matches it without case, else as spelled. */
std::string MatchCase(const std::string& dir, const std::string& part, const std::string& candidate)
{
    if (kKeepListings) {
        auto listing = g_listings.find(dir);
        if (listing == g_listings.end()) {
            listing = g_listings.emplace(dir, std::vector<std::string>()).first;
            SDL_EnumerateDirectory(dir.c_str(), CollectEntry, &listing->second);
        }
        const std::string* folded = NULL;
        for (const std::string& name : listing->second) {
            if (name == part) return part;
            if (folded == NULL && SDL_strcasecmp(name.c_str(), part.c_str()) == 0) folded = &name;
        }
        return folded != NULL ? *folded : part;
    }
    SDL_PathInfo info;
    if (SDL_GetPathInfo(candidate.c_str(), &info)) return part;
    CaseMatch match = { part.c_str(), std::string() };
    SDL_EnumerateDirectory(dir.c_str(), MatchEntryCase, &match);
    return match.found.empty() ? part : match.found;
}

/* `part` under `base` ("" for the working directory; "/" or "sdmc:/" for
 * a root). */
std::string JoinPath(const std::string& base, const std::string& part)
{
    if (base.empty()) return part;
    return base.back() == '/' ? base + part : base + "/" + part;
}

/* The host path for a Windows one: each component that does not exist as
 * spelled takes the spelling of an entry that matches it without case.
 * Each lookup is a filesystem call, so a path that exists as spelled is
 * taken at once, and the diskpath, which exists, is not walked again. */
std::string HostPath(const char* path)
{
    std::string text(path);
    ToHostSeparators(text);
    std::string resolved;
    size_t start = 0;
    if (!g_diskPath.empty() && text.compare(0, g_diskPath.size(), g_diskPath) == 0 &&
        text.size() > g_diskPath.size() && text[g_diskPath.size()] == '/') {
        /* Inside the diskpath, as File_Initialise's paths are: it exists. */
        resolved = g_diskPath;
        start = g_diskPath.size() + 1;
    }
    else if (!text.empty() && text[0] == '/') {
        resolved = "/";
        start = 1;
    }
    else if (IsAbsolute(text)) {
        /* A drive or device root, "C:/" or "sdmc:/". */
        start = text.find('/');
        start = start == std::string::npos ? text.size() : start + 1;
        resolved = text.substr(0, start);
    }
    else if (!g_diskPath.empty()) {
        /* [library:config] A relative path is taken from the diskpath, as
         * the original takes it from the working directory. */
        resolved = g_diskPath;
    }
    if (!kKeepListings && IsPlainRelative(text.substr(start))) {
        SDL_PathInfo info;
        const std::string whole = JoinPath(resolved, text.substr(start));
        if (SDL_GetPathInfo(whole.c_str(), &info)) return whole;
    }
    while (start <= text.size()) {
        size_t end = text.find('/', start);
        if (end == std::string::npos) end = text.size();
        std::string part = text.substr(start, end - start);
        start = end + 1;
        if (part.empty() || part == ".") continue;
        const std::string dir = resolved.empty() ? "." : resolved;
        if (part != "..") part = MatchCase(dir, part, JoinPath(resolved, part));
        resolved = JoinPath(resolved, part);
    }
    return resolved;
}

/* An open _findfirst search: the matching names and the next to return. */
struct FindSearch {
    std::string dir;
    std::vector<std::string> names;
    size_t next;
};

std::map<long, FindSearch> g_searches;
long g_nextSearch = 1;

/* Fills one _finddata_t entry from the directory entry `name`. */
void FillFindData(const FindSearch& search, const std::string& name,
                  struct files_native_finddata* fileinfo)
{
    SDL_PathInfo info;
    std::string full = search.dir == "." ? name : search.dir + "/" + name;

    memset(fileinfo, 0, sizeof(*fileinfo));
    SDL_strlcpy(fileinfo->name, name.c_str(), sizeof(fileinfo->name));
    if (SDL_GetPathInfo(full.c_str(), &info)) {
        if (info.type == SDL_PATHTYPE_DIRECTORY) fileinfo->attrib = _A_SUBDIR;
        fileinfo->size = (unsigned long)info.size;
        /* SDL_Time is nanoseconds; the CRT's time_t fields are seconds. */
        fileinfo->time_create = (long)(info.create_time / SDL_NS_PER_SECOND);
        fileinfo->time_access = (long)(info.access_time / SDL_NS_PER_SECOND);
        fileinfo->time_write = (long)(info.modify_time / SDL_NS_PER_SECOND);
    }
}

} // namespace

extern "C" {

/* --- path mapping ------------------------------------------------------ */

char* files_native_host_path(char* hostPath, const char* path,
                             unsigned int maxLength)
{
    if (path == NULL) return NULL;
    std::string host = HostPath(path);
    if (host.size() + 1 > maxLength) return NULL;
    memcpy(hostPath, host.c_str(), host.size() + 1);
    return hostPath;
}

FILE* files_native_fopen(const char* path, const char* mode)
{
    if (path == NULL) return NULL;
    return fopen(HostPath(path).c_str(), mode);
}

/* --- the save directory ------------------------------------------------ */

void files_native_set_save_path(const char* path)
{
    g_savePath = TrimDirectory(path);
    if (!g_savePath.empty()) {
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Saving games in \"%s\"", g_savePath.c_str());
    }
}

const char* files_native_save_path(void)
{
    return g_savePath.empty() ? NULL : g_savePath.c_str();
}

/* --- the data directory ----------------------------------------------- */

void files_native_set_disk_path(const char* path)
{
    g_diskPath = TrimDirectory(path);
    /* Absolute, so File_Initialise's paths, which start with it, are not
     * taken from it a second time, and spelled as on the disk, so that
     * HostPath need not match its case again for every file. */
    if (!g_diskPath.empty()) {
        const std::string absolute = AbsoluteDirectory(g_diskPath);
        g_diskPath.clear();
        g_diskPath = HostPath(absolute.c_str());
    }
    if (!g_diskPath.empty()) {
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Reading game data from \"%s\"", g_diskPath.c_str());
    }
}

const char* files_native_disk_path(void)
{
    return g_diskPath.empty() ? NULL : g_diskPath.c_str();
}

void files_native_save_slot_written(int slot)
{
    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "save slot %d written", slot);
#ifdef __EMSCRIPTEN__
    Emscripten_SendSaveSlotWritten(slot);
#endif
}

void files_native_save_state_changed(void)
{
    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "save slots changed");
#ifdef __EMSCRIPTEN__
    Emscripten_SendSaveStateChanged();
#endif
}

/* --- LIBC directory routines ------------------------------------------ */

/* Working. A Windows wildcard over one directory: "*.*" matches every
 * name, as on Windows; SDL_GlobDirectory lists no "." or "..". */
long files_native_findfirst(const char* filespec,
                            struct files_native_finddata* fileinfo)
{
    if (filespec == NULL || fileinfo == NULL) return -1;
    std::string spec(filespec);
    for (char& c : spec) {
        if (c == '\\') c = '/';
    }
    size_t slash = spec.rfind('/');
    std::string dir = slash == std::string::npos ? "." : spec.substr(0, slash);
    std::string pattern = slash == std::string::npos ? spec : spec.substr(slash + 1);
    if (dir.empty()) dir = "/";
    if (pattern == "*.*") pattern = "*";

    FindSearch search;
    /* [library:config] The working directory's listing is the diskpath's. */
    if (dir == "." && !g_diskPath.empty()) search.dir = HostPath(".");
    else search.dir = dir == "." || dir == "/" ? dir : HostPath(dir.c_str());
    search.next = 0;
    int count = 0;
    char** names = SDL_GlobDirectory(search.dir.c_str(), pattern.c_str(),
                                     SDL_GLOB_CASEINSENSITIVE, &count);
    if (names == NULL) return -1;
    for (int i = 0; i < count; i++) search.names.push_back(names[i]);
    SDL_free(names);
    if (search.names.empty()) return -1;

    FillFindData(search, search.names[search.next++], fileinfo);
    long handle = g_nextSearch++;
    g_searches[handle] = search;
    return handle;
}

int files_native_findnext(long handle, struct files_native_finddata* fileinfo)
{
    auto it = g_searches.find(handle);
    if (it == g_searches.end() || fileinfo == NULL) return -1;
    FindSearch& search = it->second;
    if (search.next >= search.names.size()) return -1;
    FillFindData(search, search.names[search.next++], fileinfo);
    return 0;
}

int files_native_findclose(long handle)
{
    return g_searches.erase(handle) != 0 ? 0 : -1;
}

/* Stub: attributes are not changed (SDL has no chmod). files.c's one
 * caller clears read-only before it removes a file, which SDL_RemovePath
 * does not need. */
int files_native_chmod(const char* path, int mode)
{
    (void)path;
    (void)mode;
    return 0;
}

/* Working. */
char* files_native_getcwd(char* buffer, int maxlen)
{
    char* cwd = SDL_GetCurrentDirectory();
    if (!cwd) return NULL;
    size_t length = strlen(cwd);
    /* SDL ends the directory with a separator; _getcwd does not. */
    if (length > 1 && (cwd[length - 1] == '/' || cwd[length - 1] == '\\')) cwd[--length] = '\0';
    if (length + 1 > (size_t)maxlen) {
        SDL_free(cwd);
        return NULL;
    }
    memcpy(buffer, cwd, length + 1);
    SDL_free(cwd);
    return buffer;
}

/* Working: the Windows path is mapped first, so "Data\\Saves" is a
 * directory inside Data, not one named with a backslash. */
int files_native_mkdir(const char* path)
{
    if (path == NULL) return -1;
    return SDL_CreateDirectory(HostPath(path).c_str()) ? 0 : -1;
}

/* Working: a relative path is taken from the working directory. A path
 * is absolute when it starts with a separator or with a drive or device
 * prefix: a name and a colon before any separator, as C: on Windows and
 * the consoles' sdmc: (3DS, Switch) and ux0: (Vita). */
char* files_native_fullpath(char* absPath, const char* relPath,
                            unsigned int maxLength)
{
    char cwd[1024];
    const size_t prefix = strcspn(relPath, ":/\\");
    if (relPath[0] == '/' || relPath[0] == '\\' || (prefix > 0 && relPath[prefix] == ':')) {
        if (strlen(relPath) + 1 > maxLength) return NULL;
        strcpy(absPath, relPath);
        return absPath;
    }
    if (!files_native_getcwd(cwd, sizeof(cwd))) return NULL;
    if ((unsigned)snprintf(absPath, maxLength, "%s/%s", cwd, relPath) >= maxLength) return NULL;
    return absPath;
}

/* --- KERNEL32 ---------------------------------------------------------- */

/* No drive is a CD-ROM drive (DRIVE_UNKNOWN). */
unsigned int files_native_get_drive_type(const char* rootPathName)
{
    (void)rootPathName;
    return 0;
}

/* Working. */
void files_native_output_debug_string(const char* text)
{
    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "%s", text);
}

} /* extern "C" */
