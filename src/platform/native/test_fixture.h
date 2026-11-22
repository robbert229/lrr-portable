// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_fixture — the tests' scratch directories on any host (issue #920):
 * a fresh fixture directory made the working directory, directories made
 * and checked, and the environment set, through std::filesystem and SDL
 * rather than POSIX calls, so the same tests build with MSVC. Paths come
 * back with '/' separators ("D:/a/..." on Windows), as the port's own
 * path mapping writes them (files_native.cpp). */
#ifndef LEGORRR_PLATFORM_NATIVE_TEST_FIXTURE_H
#define LEGORRR_PLATFORM_NATIVE_TEST_FIXTURE_H

#include <cstdint>
#include <filesystem>
#include <string>
#include <system_error>

#include <SDL3/SDL.h>

/* Removes `name` and everything in it, makes it again, and makes it the
 * working directory. False when any step fails. */
inline bool fixture_enter(const char* name)
{
    std::error_code ec;
    std::filesystem::remove_all(name, ec);
    if (ec) return false;
    std::filesystem::create_directory(name, ec);
    if (ec) return false;
    std::filesystem::current_path(name, ec);
    return !ec;
}

/* The working directory, with '/' separators; empty when it is unknown. */
inline std::string fixture_cwd(void)
{
    std::error_code ec;
    std::filesystem::path cwd = std::filesystem::current_path(ec);
    return ec ? std::string() : cwd.generic_string();
}

/* Makes the working directory `path`. False when it does not exist. */
inline bool fixture_chdir(const std::string& path)
{
    std::error_code ec;
    std::filesystem::current_path(path, ec);
    return !ec;
}

/* Makes the directory `path` (its parent must exist). True when it exists
 * afterwards. */
inline bool fixture_mkdir(const std::string& path)
{
    std::error_code ec;
    std::filesystem::create_directory(path, ec);
    return std::filesystem::is_directory(path, ec);
}

/* Removes `path` and everything in it, if it exists. */
inline void fixture_remove_all(const std::string& path)
{
    std::error_code ec;
    std::filesystem::remove_all(path, ec);
}

inline bool fixture_exists(const std::string& path)
{
    std::error_code ec;
    return std::filesystem::exists(path, ec);
}

inline bool fixture_is_directory(const std::string& path)
{
    std::error_code ec;
    return std::filesystem::is_directory(path, ec);
}

/* The file's size, or -1 when it is missing. */
inline long fixture_file_size(const std::string& path)
{
    std::error_code ec;
    std::uintmax_t size = std::filesystem::file_size(path, ec);
    return ec ? -1L : (long)size;
}

/* Whether '\\' separates path components on this host (Windows), so a
 * name spelled with one is a path, not a name containing a backslash. */
inline bool fixture_backslash_separates(void)
{
    return std::filesystem::path::preferred_separator == '\\';
}

/* Sets an environment variable for this process. */
inline void fixture_setenv(const char* name, const std::string& value)
{
    SDL_setenv_unsafe(name, value.c_str(), 1);
}

#endif /* LEGORRR_PLATFORM_NATIVE_TEST_FIXTURE_H */
