// SPDX-License-Identifier: LGPL-3.0-or-later
#include <stddef.h>
#include <stdint.h>

#include "port_loop_ports.hpp"
#include "../gods98/block.hpp"
#include "util.hpp"
#include "../gods98/engine.hpp"
#include "../gods98/resource.hpp"
#include "startup_config_shared.hpp"
#include "startup_config_for_wad.hpp"

namespace unported_ledger_stubs {

// GLOBAL: LEGORR 0x00575ce0
GameFileHandle* s_startup_streams[4];
uint32_t s_startup_window_mode;
char s_startup_output_path[260];
char s_startup_token_list_path[260];
BufferedCharReadHook s_buffered_char_read_hook;
void* s_buffered_char_read_context;
static DependencyConfigEntryHook s_dependency_config_hook;
static void* s_dependency_config_context;

void startup_stream_handle_set(int slot, void* handle)
{
    if (slot < 0 || slot >= 4) return;
    s_startup_streams[slot] = (GameFileHandle*)handle;
}

void* startup_stream_handle_get(int slot)
{
    if (slot < 0 || slot >= 4) return NULL;
    return s_startup_streams[slot];
}

void startup_stream_window_mode_set(uint32_t value)
{
    s_startup_window_mode = value;
}

uint32_t startup_stream_window_mode_get(void)
{
    return s_startup_window_mode;
}

void startup_stream_state_reset(void)
{
    s_startup_streams[0] = NULL;
    s_startup_streams[1] = NULL;
    s_startup_streams[2] = NULL;
    s_startup_streams[3] = NULL;
    s_startup_window_mode = 0;
}

void startup_streams_close(void)
{
    int index;
    for (index = 0; index < 3; index++) {
        if (s_startup_streams[index] != NULL) {
            game_file_close(s_startup_streams[index]);
            s_startup_streams[index] = NULL;
        }
    }
}

static void startup_path_copy(char output[260], const char* input)
{
    size_t index;
    if (input == NULL) {
        output[0] = '\0';
        return;
    }
    for (index = 0u; index < 259u && input[index] != '\0'; ++index)
        output[index] = input[index];
    output[index] = '\0';
}

/* The target consumes two path globals whose initialization is outside the
 * recovered startup routine. Preserve that seam explicitly until a native
 * startup owner supplies the paths. */
void startup_config_stream_paths_set(const char* output_path,
                                     const char* token_list_path)
{
    startup_path_copy(s_startup_output_path, output_path);
    startup_path_copy(s_startup_token_list_path, token_list_path);
}

void buffered_char_stream_read_hook_set(BufferedCharReadHook read_char,
                                        void* context)
{
    s_buffered_char_read_hook = read_char;
    s_buffered_char_read_context = context;
}

void dependency_config_entry_hook_set(DependencyConfigEntryHook hook,
                                      void* context)
{
    s_dependency_config_hook = hook;
    s_dependency_config_context = context;
}

int dependency_grid_config_load(void* config, int level)
{
    BlockFile* file = (BlockFile*)config;
    BlockRec* group;
    BlockRec* entry;
    int processed = 0;
    if (file == NULL) return 0;
    group = block_subgroup(file, string_path_join(util_root_name(file),
                                                  "Dependencies",
                                                  (const char*)NULL));
    if (group == NULL) return 0;
    for (entry = group; entry != NULL;
         entry = block_next_sibling(entry)) {
        const char* name = block_name(entry);
        const char* value = block_value(entry);
        if (name == NULL || value == NULL) continue;
        if (!dependency_config_entry_should_load(level, name, value)) continue;
        ++processed;
    }
    return processed;
}

int dependency_config_entry_should_load(int level, const char* name,
                                        const char* value)
{
    return s_dependency_config_hook == NULL ||
        s_dependency_config_hook(s_dependency_config_context, level,
                                 name, value);
}

} // namespace unported_ledger_stubs
