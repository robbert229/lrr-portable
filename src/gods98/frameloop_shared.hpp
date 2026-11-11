// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private to src/gods98/frameloop.cpp and the units relocated out of it (ADR 0006
 * decision 4; tools/relocate_unit.py). Statics both sides use have
 * external linkage through the declarations below. Include it only from
 * those files: it is not a shared header. */
#ifndef LRR_GODS98_FRAMELOOP_SHARED_HPP
#define LRR_GODS98_FRAMELOOP_SHARED_HPP

#define WINDOW_W 640

#define WINDOW_H 480

#define FRAME_MS 33 /* ~30 fps */

typedef struct MenuSoundTickClock {
    uint32_t last_ms;
    float dt;
} MenuSoundTickClock;

typedef struct RewardFramePumpContext {
    PlatformWindow* window;
    DisplaySurface* surface;
    GameContext* game;
    InputState* input;
    AudioPlayer* audio;
    FrontEnd* frontend;
    FILE* log;
    long* frames;
    long frame_cap;
    int* quit;
    const char** stop;
    uint32_t task_draw_masks[GAME_TASK_COUNT];
    Flic* task_flics[GAME_TASK_COUNT];
    int score_text_requested;
    int stats_result_requested;
    int zone_action_consumed;
} RewardFramePumpContext;

/* Storage for state that belongs to the application lifetime, rather than to
 * the C stack frame of LegorrrRun. The reference WinMain keeps this state in
 * globals while repeatedly dispatching its callback triplet. Keeping the
 * reconstruction's corresponding records here is the first step toward the
 * same init/tick/shutdown boundary. */
typedef struct FrameloopApplicationState {
    int initialized;
    int phase;
    int gameplay_prepared;
    float frame_dt;
    PlatformEvent frame_events[32];
    int frame_event_count;
    const char* wad_dir;
    long frame_cap;
    long autopause;
    FILE* log;
    int log_owned;
    GameContext* game_storage;
    FrontEnd* frontend_storage;
    EngineState engine_state;
    InputState input;
    int pause_escape_down;
    int pause_mouse_down;
    int gameplay_mouse_suppressed;
    int gameplay_clock_rebase;
    uint32_t gameplay_resume_ms;
    ViewContext views[3];
    EngineContext engine_context;
    PlatformWindow* window;
    DisplaySurface surface;
    TextureSurface* rock_texture;
    TextureSurface* ui_texture;
    AudioPlayer audio;
    int audio_status;
    long frames;
    int thumbnail_slot;
    int quit;
    const char* stop_reason;
    RewardFramePumpContext reward_frame;
    RewardScreenOps reward_ops;
    MenuSoundTickClock menu_sound_clock;
    int play_movies;
    char level_name[FE_LEVEL_NAME_MAX];
    int boot_menu;
    int frontend_built;
    int pending_load_slot;
    int pending_load_record;
    char mission_brief[1024];
    int mission_brief_active;
    int mission_brief_kind;
    int mission_brief_page;
    unsigned int mission_brief_keys_held;
    TextureSurface* briefing_panel;
    TextureSurface* briefing_title_font;
    TextureSurface* briefing_body_font;
    TextureSurface* briefing_next;
    TextureSurface* briefing_back;
    TextureSurface* briefing_back_arrow;
    int briefing_x;
    int briefing_y;
    char briefing_title[128];
    LevelState* pointer_dispatch_level;
    int pointer_dispatch_previous_down; /* retained for pause patch composition */
    SimulationPointerCapture pointer_captures[2];
    uint32_t pointer_capture_serial;
    LevelState* reward_tick_level;
    int reward_tick_started;
    LevelState* picker_return_level;
    int picker_return_pending;
    int picker_return_origin;
    int rock_x;
    int rock_y;
} FrameloopApplicationState;

extern FrameloopApplicationState s_frameloop_application;
int frameloop_application_events_track(FrameloopApplicationState* app, const PlatformEvent* events, int count);
int64_t now_ms(void);
void frameloop_credits(PlatformWindow* win, DisplaySurface* surf, BlockFile* cfg, const char* wad_dir, AudioPlayer* ap, int audio_status, int* quit, FILE* log, long* frame_number);

#endif /* LRR_GODS98_FRAMELOOP_SHARED_HPP */
