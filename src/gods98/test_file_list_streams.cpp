// SPDX-License-Identifier: LGPL-3.0-or-later
/* file_list_streams.c — reset start-up state, close existing input
 * streams, and scan into the output only if it can be opened. */
#include <string>

#include "gods98/files_open.h"
#include "test/lrr_test.h"

extern "C" void ledger_stub_0048b260_StartupStreamStateReset(void);
extern "C" void ledger_stub_0048b290_StartupStreamsClose(void);
extern "C" void window_mode_flag_set(unsigned int);
extern "C" void startup_config_stream_load(void);

/* This state record is private to file_list_streams.c. */
struct FileListStreams_Globs {
    GameFileHandle* streams[4];
    char tokenListPath[1024];
    char outputPath[1024];
    unsigned int field_810;
    unsigned int windowMode;
};
extern FileListStreams_Globs fileListStreamsGlobs;

struct Stream {
    bool open = false;
    std::string contents;
};

static Stream output;

extern "C" GameFileHandle* __wrap_game_file_open_dispatch(const char* path, const char* mode)
{
    if (std::strcmp(path, "listing.txt") != 0 || std::strcmp(mode, "w") != 0) return nullptr;
    output.open = true;
    output.contents.clear();
    return reinterpret_cast<GameFileHandle*>(&output);
}

extern "C" int __wrap_game_file_close(GameFileHandle* handle)
{
    auto* stream = reinterpret_cast<Stream*>(handle);
    stream->open = false;
    return 0;
}

extern "C" void __wrap_resource_file_token_list_load_and_tree_scan_target(const char* path)
{
    if (output.open && std::strcmp(path, "tokens.txt") == 0) output.contents = "missing.txt\n";
}

int main()
{
    auto& state = fileListStreamsGlobs;
    std::strcpy(state.tokenListPath, "tokens.txt");
    std::strcpy(state.outputPath, "listing.txt");
    window_mode_flag_set(7);
    CHECK_EQ(state.windowMode, 7u);
    ledger_stub_0048b260_StartupStreamStateReset();
    CHECK_EQ(state.windowMode, 0u);
    for (auto* stream : state.streams) CHECK_EQ(stream, nullptr);
    CHECK_STREQ(state.tokenListPath, "tokens.txt");
    CHECK_STREQ(state.outputPath, "listing.txt");

    Stream input[3];
    for (int i = 0; i < 3; ++i) {
        input[i].open = true;
        state.streams[i] = reinterpret_cast<GameFileHandle*>(&input[i]);
    }
    startup_config_stream_load();
    for (int i = 0; i < 3; ++i) {
        CHECK_EQ(state.streams[i], nullptr);
        CHECK_EQ(input[i].open, false);
    }
    CHECK_EQ(output.contents, std::string("missing.txt\n"));
    CHECK_EQ(output.open, false);

    output.contents.clear();
    std::strcpy(state.outputPath, "unavailable.txt");
    startup_config_stream_load();
    CHECK_EQ(state.streams[3], nullptr);
    CHECK(output.contents.empty(), "failed output does not receive scan results");
    CHECK_EQ(output.open, false);
    ledger_stub_0048b290_StartupStreamsClose();
    return lrr_test_result();
}
