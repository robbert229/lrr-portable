// SPDX-License-Identifier: LGPL-3.0-or-later
/* The web build's list of game files (manifest.h). */
#include "platform/native/emscripten/manifest.h"

#include <set>

bool DataManifest_IsSafePath(const std::string& p_path)
{
    if (p_path.empty() || p_path[0] == '/' || p_path.back() == '/') {
        return false;
    }

    size_t start = 0;
    while (start <= p_path.size()) {
        size_t end = p_path.find('/', start);
        if (end == std::string::npos) {
            end = p_path.size();
        }
        std::string part = p_path.substr(start, end - start);
        if (part.empty() || part == "." || part == "..") {
            return false;
        }
        start = end + 1;
    }

    for (char c : p_path) {
        if (c == '\\' || (unsigned char) c < 0x20 || c == 0x7f) {
            return false;
        }
    }
    return true;
}

size_t DataManifest_Parse(const char* p_text, size_t p_length, std::vector<std::string>& p_files)
{
    std::set<std::string> seen(p_files.begin(), p_files.end());
    size_t dropped = 0;
    size_t pos = 0;

    while (pos < p_length) {
        size_t end = pos;
        while (end < p_length && p_text[end] != '\n') {
            end++;
        }

        std::string line(p_text + pos, end - pos);
        pos = end + 1;
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (line.empty() || line[0] == '#') {
            continue;
        }

        if (!DataManifest_IsSafePath(line)) {
            dropped++;
        }
        else if (seen.insert(line).second) {
            p_files.push_back(line);
        }
    }
    return dropped;
}
