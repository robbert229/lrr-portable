// SPDX-License-Identifier: LGPL-3.0-or-later
/* G98-028: the startup file-list streams (findings 505, 680 and 684), in
 * the original's shape as C (ADR 0005). The original's unit is C: its state
 * is a common (00575ce0), which only a C tentative definition gives; a C++
 * global goes to .bss. The functions are in original address order.
 * Natively the file builds as C++ against the port's names. */
#include <stddef.h>

#include "files_open.h"

extern "C" void resource_file_token_list_load_and_tree_scan_target(const char* token_file); /* 004802d0 */

/* The unit's state, the original's whole 0x818-byte block (findings 680
 * and 684): the four stream handles (00575ce0-00575cec), the token-list
 * path that 0048b2f0 scans (00575cf0) and the output path it opens
 * (005760f0), each 1024 bytes, a word no retail code references
 * (005764f0) and the window-mode flag (005764f4) that 0048b260 clears and
 * 0048b280 sets. */
typedef struct FileListStreams_Globs {
    struct GameFileHandle* streams[4]; /* +0x000 */
    char tokenListPath[1024];          /* +0x010 */
    char outputPath[1024];             /* +0x410 */
    unsigned int field_810;            /* +0x810 unreferenced in retail */
    unsigned int windowMode;           /* +0x814 */
} FileListStreams_Globs;

/* A tentative definition: a common, as in the original. */
// GLOBAL: LEGORR 0x00575ce0
FileListStreams_Globs fileListStreamsGlobs;

#define s_startup_streams fileListStreamsGlobs.streams
#define s_startup_token_list_path fileListStreamsGlobs.tokenListPath
#define s_startup_output_path fileListStreamsGlobs.outputPath
#define s_window_mode_flag fileListStreamsGlobs.windowMode

#ifdef __cplusplus
extern "C" {
#endif

// FUNCTION: LEGORR 0x0048b260
void ledger_stub_0048b260_StartupStreamStateReset(void)
{
    s_startup_streams[0] = NULL;
    s_startup_streams[1] = NULL;
    s_startup_streams[2] = NULL;
    s_startup_streams[3] = NULL;
    s_window_mode_flag = 0u;
}

// FUNCTION: LEGORR 0x0048b280
void window_mode_flag_set(unsigned int mode)
{
    s_window_mode_flag = mode;
}

// FUNCTION: LEGORR 0x0048b290
void ledger_stub_0048b290_StartupStreamsClose(void)
{
    if (s_startup_streams[0] != NULL)
        game_file_close(s_startup_streams[0]);
    if (s_startup_streams[1] != NULL)
        game_file_close(s_startup_streams[1]);
    if (s_startup_streams[2] != NULL)
        game_file_close(s_startup_streams[2]);
    s_startup_streams[2] = NULL;
    s_startup_streams[1] = NULL;
    s_startup_streams[0] = NULL;
}

/* The original calls 0048b290 here, not startup_config.cpp's copy
 * (finding 680). */
#define startup_streams_close ledger_stub_0048b290_StartupStreamsClose

// FUNCTION: LEGORR 0x0048b2f0
void startup_config_stream_load(void)
{
    startup_streams_close();
    if ((s_startup_streams[3] = game_file_open_dispatch(s_startup_output_path, "w")) != NULL) {
        resource_file_token_list_load_and_tree_scan_target(s_startup_token_list_path);
        game_file_close(s_startup_streams[3]);
    }
    /* The retail routine leaves the just-closed value in DAT_00575cec. */
}

#ifdef __cplusplus
}
#endif
