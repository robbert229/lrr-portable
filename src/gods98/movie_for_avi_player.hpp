// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what avi_player uses from movie.cpp.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by both files. */
#ifndef LRR_GODS98_MOVIE_FOR_AVI_PLAYER_HPP
#define LRR_GODS98_MOVIE_FOR_AVI_PLAYER_HPP

namespace unported_ledger_stubs {
struct AviFrameSeekRebaseMethod {
    void seek(uint32_t frame_index);
};
} // namespace unported_ledger_stubs

void movie_avi_file_initialize(void);
extern AviTimeHook s_avi_time_hook;
extern void* s_avi_time_context;
extern AviFrameUpdateHook s_avi_frame_update_hook;
extern AviFrameLoopHook s_avi_frame_loop_hook;
extern void* s_avi_frame_hook_context;
extern AviSurfaceLockHook s_avi_surface_lock_hook;
extern void* s_avi_surface_lock_context;
extern AviDisplayModeHook s_avi_display_mode_hook;
extern void* s_avi_display_mode_context;
extern AviReleaseHook s_avi_release_hook;
extern AviDisplayReleaseHook s_avi_display_release_hook;
extern void* s_avi_release_context;

#endif /* LRR_GODS98_MOVIE_FOR_AVI_PLAYER_HPP */
