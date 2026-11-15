// SPDX-License-Identifier: LGPL-3.0-or-later
/* G98-028 G98-028: the unit's functions in original address order and the
 * state they own, relocated verbatim by tools/relocate_unit.py (ADR 0006).
 * Body hashes: docs/coverage/relocations/G98-028.csv. */
#include <stddef.h>
#include <stdint.h>
#include "port_loop_ports.hpp"
#include "../gods98/block.hpp"
#include "util.hpp"
#include "../gods98/engine.hpp"
#include "../gods98/resource.hpp"
#include "camera.hpp"
#include "frameloop.hpp"
#include "mainsim.hpp"
#include "message.hpp"
#include "tickers.hpp"
#include "simtick.hpp"
#include "view.hpp"
#include "record.hpp"
#include "crt/crt.hpp"
#include "sound.hpp"
#include "audio.hpp"
#include "media.hpp"
#include "heap.hpp"
#include "scene_resources.hpp"
#include "game/frontend.hpp"
#include "game/entity.hpp"
#include "game/building_scene.hpp"
#include "game/actor_scene.hpp"
#include "game/objective.hpp"
#include "game/effects.hpp"
#include "game/scene_render.hpp"
#include "scene_pool.hpp"
#include "string.hpp"
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
/* GAME-021 lego_unit.c reads this word (C linkage, ADR 0005 step 6). */
extern "C" uint32_t s_overlay_word_03610;
#include "engine_for_file_list_streams.hpp"
#include "game/startup_config_for_file_list_streams.hpp"
#include "engine_for_file_list_streams.hpp"

#if defined(_MSC_VER) && _MSC_VER < 1200
/* The unit's stream state, at the original's addresses (finding 680): the
 * four stream handles (00575ce0-00575cec), the token-list path that
 * 0048b2f0 scans (00575cf0) and the output path it opens (005760f0), each
 * 1024 bytes. The original's block goes on with an unreferenced word
 * (005764f0) and the window-mode flag (005764f4), which engine.cpp still
 * defines as s_window_mode_flag. Natively startup_config.cpp keeps the
 * port's globals under the old names. */
struct FileListStreams_Globs {
    GameFileHandle* streams[4];     /* +0x000 */
    char tokenListPath[1024];       /* +0x010 */
    char outputPath[1024];          /* +0x410 */
};
// GLOBAL: LEGORR 0x00575ce0
FileListStreams_Globs fileListStreamsGlobs;
#define s_startup_streams fileListStreamsGlobs.streams
#define s_startup_token_list_path fileListStreamsGlobs.tokenListPath
#define s_startup_output_path fileListStreamsGlobs.outputPath
#endif

namespace unported_ledger_stubs {
// FUNCTION: LEGORR 0x0048b260
extern "C" void ledger_stub_0048b260_StartupStreamStateReset()
{
    s_startup_streams[0] = NULL;
    s_startup_streams[1] = NULL;
    s_startup_streams[2] = NULL;
    s_startup_streams[3] = NULL;
#if !(defined(_MSC_VER) && _MSC_VER < 1200)
    /* The port's own word; the original stores only the four handles and
     * the window-mode flag (finding 680). */
    s_startup_window_mode = 0u;
#endif
    window_mode_flag_set(0u);
}
} // namespace unported_ledger_stubs

// FUNCTION: LEGORR 0x0048b280
void window_mode_flag_set(uint32_t mode)
{
    s_window_mode_flag = mode;
}

namespace unported_ledger_stubs {
// FUNCTION: LEGORR 0x0048b290
extern "C" void ledger_stub_0048b290_StartupStreamsClose()
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
#if defined(_MSC_VER) && _MSC_VER < 1200
/* The original calls 0048b290 here, not startup_config.cpp's copy
 * (finding 680). */
#define startup_streams_close ledger_stub_0048b290_StartupStreamsClose
#endif
// FUNCTION: LEGORR 0x0048b2f0
extern "C" void startup_config_stream_load(void)
{
    GameFileHandle* stream;
    startup_streams_close();
    if (s_startup_output_path[0] == '\0') return;
    stream = game_file_open_dispatch(s_startup_output_path, "w");
    s_startup_streams[3] = stream;
    if (stream == NULL) return;
    if (s_startup_token_list_path[0] != '\0')
        resource_file_token_list_load_and_tree_scan_target(
            s_startup_token_list_path);
    game_file_close(stream);
    /* The retail routine leaves the just-closed value in DAT_00575cec. */
}
} // namespace unported_ledger_stubs
