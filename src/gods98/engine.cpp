// SPDX-License-Identifier: LGPL-3.0-or-later
#include "camera.hpp"
#include "engine.hpp"
#include "mainsim.hpp"
#include "message.hpp"
#include "tickers.hpp"
#include "view.hpp"
#include "record.hpp"
#include "crt.hpp"
#include "sound.hpp"
#include "heap.hpp"

/* Frame tick — corpus EngineTick @ 0x4232e0. The numbered structure below
 * mirrors finding 03 §"Tick structure" and the first 130 lines of the corpus.
 * Every subsystem ticker is a registered stub in tickers.cpp whose function
 * body is filled in from the corpus one at a time; the *order* here is the
 * observed call order and must stay stable.
 */
// FUNCTION: LEGORR 0x004232e0
int EngineTick(EngineContext* ctx, float dt)
{
    /* 1. Gate — loader/benchmark mode idles out of the frame. */
    if (ctx->app_mode == ENGINE_APP_LOADER && EngineKeepRunning(ctx) == 0) {
        return 0;
    }
    /* Frozen time zeroes dt (corpus: DAT_00556ecc & 0x100000). */
    if ((ctx->state & ENGINE_STATE_TIME_FROZEN) != 0) {
        dt = 0.0f;
    }

    /* 2. Per-frame housekeeping (EnginePreTick @ 0x4231f0). The text pump
     *    consumes the current frame's captured keys; the frame-loop drives
     *    the table lifecycle (capture before this, input_begin_frame at the
     *    frame end) — see frameloop.cpp. */
    EnginePreTick(ctx);

    /* 3. Scaled delta: subsystem tickers run scaled time, raw dt drives
     *    timers. Editor/preview (context_mode==0) runs raw dt. */
    float scale;
    if (ctx->context_mode == 0 && ctx->app_mode < 2) {
        scale = dt;
    } else {
        scale = EngineTimeScale(ctx) * dt;
    }
    /* One-shot "force scale 1.0 this frame" (corpus: DAT_00556ecc & 4). */
    if ((ctx->state & ENGINE_STATE_FORCE_SCALE_1) != 0) {
        ctx->state &= ~ENGINE_STATE_FORCE_SCALE_1;
        scale = 1.0f;
    }

    /* 4. Camera / move cluster (corpus lines 52-88): the camera
     *    interpolation block, screen-space → world projection, and the
     *    force-scale-1.0 one-shot are ported in camera.* (camera_cluster). */
    camera_cluster(ctx, dt, scale);

    /* 4b. The three view contexts (DAT_005560e4/e8/ec — ViewContextTick). */
    view_views_tick(ctx, dt, scale);

    /* 5. Main simulation step — 0 => quit frame (MainSimStep @ 0x4250c0,
     *    ported in mainsim.*). */
    uint32_t sim_out1 = 0, sim_out2 = 0, sim_out3 = 0;
    if (main_sim_step(ctx, scale, dt, &sim_out1, &sim_out2, &sim_out3) == 0) {
        return 0;
    }

    /* 6. The ordered subsystem-ticker sequence (finding 03; corpus lines
     *    93-119). Only the first entry is gated on !TIME_FROZEN at the call
     *    site; that gate lives in the table (see TICKERS[0].skip_when). */
    EngineSubsystemTickers(ctx, dt, scale);

    /* 7. Indexed manager dispatch (corpus ManagerTick FUN_0045a730) — enabled
     *    under ENGINE_STATE_INDEXED_SUBSYS; index list observed: 1,2,4,9,6,8,
     *    0xb,5,7,10 interleaved with the named manager tickers. Stub table. */
    if ((ctx->state & ENGINE_STATE_INDEXED_SUBSYS) != 0) {
        EngineManagerDispatch(ctx, dt);
    }

    /* 8. Render begin/end pairs, world-draw vs editor pass, and the
     *    &2/&0x2000-gated in-game render pass (corpus lines 130-215). Stub. */
    EngineRenderFrame(ctx, dt, scale, &sim_out1);

    /* 9. Air-supply beat mechanic (corpus tail): fires the air-low/empty
     *    notifications and loops SND_AirBeat; writes air_prev back. */
    EngineAirBeat(ctx, scale);

    /* Exit timer counts down in scaled time (corpus: DAT_00556efc -= scale). */
    ctx->exit_timer -= scale;

    return 1;
}

/* Per-frame housekeeping — corpus EnginePreTick @ 0x4231f0. The real body is
 * text/entity-name input handling (keyboard echo through DAT_00556fa0); the
 * input core (findings 04/05) supplies the text pump. cur is NOT cleared here
 * (the frame-loop clears it at the frame boundary; the corpus's in-place
 * clear is incompatible with per-frame device capture).
 */
// FUNCTION: LEGORR 0x004231f0
void EnginePreTick(EngineContext* ctx)
{
    if (ctx->input == NULL) {
        return;
    }
    char*  target = ctx->input->text_target;
    int    cap    = ctx->input->text_cap;
    int    len    = ctx->input->text_len;
    if (target != NULL) {
        input_text_pump(ctx->input, target, cap, &len, 0);
        ctx->input->text_len = len;
    } else {
        input_mirror(ctx->input);
    }
}

/* Quit-poll probe — corpus KeepRunning @ 0x435940. The real body chains the
 * splash/quit checks and the app-state test (FUN_00431560, FUN_00416d00 …);
 * a quit request ends up here. For the scaffold the frame never asks to quit.
 */
// STUB: LEGORR 0x00435940
int EngineKeepRunning(EngineContext* ctx)
{
    (void)ctx;
    /* TODO(engine): wire the corpus quit-poll chain here (finding 06). */
    return 1;
}

/* Time-scale factor — corpus FUN_0041fb40, read once per frame in the tick.
 * Multiplies dt into the *scaled* delta passed to subsystem tickers.
 */
// FUNCTION: LEGORR 0x0041fb40
float EngineTimeScale(EngineContext* ctx)
{
    /* Corpus FUN_0041fb40: the config-derived speed factor. Issue #85 wires
     * the Options Game_Speed slider through it — the frame loop writes
     * options_time_scale(&game.options) into ctx->time_scale every frame
     * (1.0 while no option has been raised). 0 keeps the scaffold default. */
    if (ctx == NULL || ctx->time_scale == 0.0f) {
        return 1.0f;
    }
    return ctx->time_scale;
}

/* Engine teardown — triplet slot DAT_00505934 (finding 20). The corpus
 * selected LAB_00424cf0 (normal play) or LAB_00424d00 (loader mode) at
 * registration time (FUN_0041fa20); that choice is reproduced here.
 */
// FUNCTION: LEGORR 0x0041fa20
void EngineShutdown(EngineContext* ctx)
{
    if (ctx->app_mode == ENGINE_APP_LOADER) {
        /* corpus EngineShutdown @ 0x424d00 — subsystem closes, entity-array
         * destructors, LoaderProfile load. TODO(engine). */
        return;
    }
    /* corpus LAB_00424cf0 — stub forwarding to FUN_004250a0. TODO(engine). */
}
/* RuntimeFlag40000000Get (FUN_0041f970): the faithful masked read of the
 * port's engine-state word. */
// FUNCTION: LEGORR 0x0041f970
uint32_t runtime_flag40000000_get(const EngineContext* ctx)
{
    return ctx != NULL ? (ctx->state & 0x40000000u) : 0u;
}

/* ModeDependentField24Get (FUN_00423080): mode-selected follow-span read.
 * The corpus indexes views[ctx->context_mode == 1 ? 0 : 2]; other modes
 * return the mode word verbatim, and the port reads the ViewContext's
 * follow_span slot (the corpus's word 0x24) through the threaded view
 * table (deviation: NULL slots answer zero, corpus dereferences blindly).
 */
// FUNCTION: LEGORR 0x00423080
int mode_dependent_field24_get(const EngineContext* ctx)
{
    if (ctx == NULL) {
        return 0; /* deviation: the corpus dereferences its global blindly */
    }
    int which;
    if (ctx->context_mode == 1) {
        which = 0;
    } else if (ctx->context_mode == 0) {
        which = 2;
    } else {
        return ctx->context_mode;
    }
    const ViewContext* v = ctx->views[which];
    return v != NULL ? (int)v->follow_span : 0;
}

/* --- the 0x425-0x426 engine-state band (see engine.hpp) ---------------------
 * The two threaded one-liners are the corpus's own reads of the context-mode
 * and cheat-word globals; the two DAT-global shapes land as zero-initialized
 * module storage per the unowned-global convention (the triple's only corpus
 * writer is the unported renderer-side sim-interaction update). */
static int s_scroll_guard;        /* DAT_004de548 */
static uint32_t s_triple[3];      /* DAT_0055651c / DAT_00556520 / DAT_00556524 */
static float s_task_text_origin[2]; /* _DAT_00556f24 / _DAT_00556f28 */

// FUNCTION: LEGORR 0x00425d80
bool current_engine_mode_is_zero(const EngineContext* ctx)
{
    return ctx != NULL && ctx->context_mode == 0;
}

/* EngineModeSet (FUN_004565f0) — see engine.hpp. The corpus's DAT_00503610/
 * DAT_00503614 overlay pair lands as own storage (the corpus writers and
 * readers ride the unported advisor/message bands — deviation). */
static uint32_t s_overlay_word_03610; /* DAT_00503610 role */
static uint32_t s_overlay_word_03614; /* DAT_00503614 role */

// FUNCTION: LEGORR 0x004565f0
int engine_mode_set(EngineContext* ctx, const int* mode)
{
    if (mode == NULL) {
        return 0; /* deviation: the corpus strides the pointer blindly */
    }
    global_flag_004ff978_install((uint32_t)*mode);
    if (*mode == 1) {
        global_state_flags_bits6_and_9_set(ctx, 0, 1);
        global_state_flags_bit7_set(ctx, 0);
    }
    s_overlay_word_03610 = 0u;
    s_overlay_word_03614 = 0u;
    return *mode;
}

uint32_t engine_overlay_word_peek(int which)
{
    return which == 0 ? s_overlay_word_03610 : s_overlay_word_03614;
}

/* ObjectiveScoreTextOriginSet (FUN_004262e0) — see engine.hpp. The pair
 * rides unowned-global module storage (the corpus's writer side is this
 * setter; the reader is the unported score-text draw band — deviation). */
static float s_score_text_origin[2];

// FUNCTION: LEGORR 0x004262e0
void objective_score_text_origin_set(EngineContext* ctx, const float* pair)
{
    if (pair != NULL) {
        s_score_text_origin[0] = pair[0];
        s_score_text_origin[1] = pair[1];
        if (ctx != NULL) {
            ctx->debug |= 0x4000u;
        }
        return; /* deviation: the corpus writes the word blindly */
    }
    if (ctx != NULL) {
        ctx->debug &= ~0x4000u;
    }
}

void objective_score_text_origin_peek(float out[2])
{
    if (out != NULL) {
        out[0] = s_score_text_origin[0];
        out[1] = s_score_text_origin[1];
    }
}

// FUNCTION: LEGORR 0x004263a0
void global_state_flags_bit7_set(EngineContext* ctx, int value)
{
    if (ctx == NULL) {
        return; /* deviation: the corpus writes its global blindly */
    }
    if (value != 0) {
        ctx->debug |= 0x80u;
    } else {
        ctx->debug &= ~0x80u;
    }
}

// FUNCTION: LEGORR 0x00426360
void global_state_flags_bits6_and_9_set(EngineContext* ctx, int value,
                                        int set_bit9)
{
    if (ctx == NULL) {
        return; /* deviation: the corpus writes its global blindly */
    }
    /* Corpus shape: bit 6 (and only bit 6) lands first through value, then
     * bit 9 is raised or cleared through set_bit9 — bit 6 is decided before
     * bit 9's mask applies, and bit 9 never touches bit 6's half. */
    const uint32_t v = (value != 0) ? (ctx->debug | 0x40u)
                                    : (ctx->debug & ~0x40u);
    ctx->debug = (set_bit9 != 0) ? (v | 0x200u) : (v & ~0x200u);
}

// FUNCTION: LEGORR 0x00425cd0
void global_scroll_guard_set(int value)
{
    s_scroll_guard = value;
}

int global_scroll_guard_peek(void)
{
    return s_scroll_guard;
}

/* ObjectiveTaskTextOriginSet (FUN_00426320) — see engine.hpp. The corpus's
 * pair has no ported reader outside the corpus's own draw band (the only
 * writers are this setter), so it rides module storage per the unowned-
 * global convention; the enable bit rides the bound debug word. */
// FUNCTION: LEGORR 0x00426320
void objective_task_text_origin_set(EngineContext* ctx, const float* pair)
{
    if (pair != NULL) {
        s_task_text_origin[0] = pair[0];
        s_task_text_origin[1] = pair[1];
        if (ctx != NULL) {
            ctx->debug |= 0x8000u;
        }
        return; /* deviation: the corpus writes the word blindly */
    }
    if (ctx != NULL) {
        ctx->debug &= ~0x8000u;
    }
}

void objective_task_text_origin_peek(float out[2])
{
    if (out != NULL) {
        out[0] = s_task_text_origin[0];
        out[1] = s_task_text_origin[1];
    }
}

// FUNCTION: LEGORR 0x00426230
void global_triple_copy_out(uint32_t* out)
{
    if (out == NULL) {
        return; /* deviation: the corpus writes through blindly */
    }
    out[0] = s_triple[0];
    out[1] = s_triple[1];
    out[2] = s_triple[2];
}

/* GlobalField505f88Get (FUN_00401bf0): threaded app-mode read. */
// FUNCTION: LEGORR 0x00401bf0
int engine_app_mode_get(const EngineContext* ctx)
{
    return ctx != NULL ? ctx->app_mode : 0;
}

/* --- the grid-cell record buffer (see engine.hpp) ---------------------------- */
static void*        s_field_grid_buf;   /* DAT_005520e8 role */
static EngineState* s_field_grid_desc;  /* DAT_005520ec role */

// FUNCTION: LEGORR 0x004661c0
void grid_cell_record_buffer_allocate(EngineState* block)
{
    if (block == NULL) {
        return; /* deviation: the corpus strides the block blindly */
    }
    const int w = (int)block->grid_w;
    const int h = (int)block->grid_h;
    /* Deviation: the port's cell record widens the entity slot, so the
     * image strides 0x10 where the corpus strides 0x0c (see engine.hpp). */
    void* buf = engine_heap_alloc((long)(w * h * 0x10));
    if (buf != NULL) {
        uint32_t* p = (uint32_t*)buf;
        for (uint32_t n = (uint32_t)(w * h * 4) & 0x3fffffffu; n != 0u; --n) {
            *p++ = 0u;
        }
    }
    s_field_grid_buf = buf;
    s_field_grid_desc = block;
}

// FUNCTION: LEGORR 0x00466210
void entity_field_grid_buffer_release(void)
{
    if (s_field_grid_buf != NULL) {
        engine_heap_free(s_field_grid_buf);
        s_field_grid_buf = NULL;
    }
}

// FUNCTION: LEGORR 0x00466230
void grid_cell_record_buffer_replace(EngineState* block)
{
    entity_field_grid_buffer_release();
    grid_cell_record_buffer_allocate(block);
}

void* entity_field_grid_buffer_peek(void) { return s_field_grid_buf; }

EngineState* entity_field_grid_descriptor_peek(void) { return s_field_grid_desc; }

/* --- the global-state refresh leaf (see engine.hpp) ------------------------
 * FUN_00435a20's two corpus words (state word, refresh record pointer) have
 * no ported owner; they land here as zero-init statics with hooks. */
static uint32_t s_global_low_word; /* DAT_00556ed4 role */
static void* s_refresh_record;     /* DAT_005563b4 role */

// FUNCTION: LEGORR 0x00435a20
void global_state_low_byte_clear_and_refresh(void)
{
    s_global_low_word &= 0xffffff00u;
    surface_visibility_set(s_refresh_record, 1u);
}

uint32_t global_state_low_word_get(void) { return s_global_low_word; }
void global_state_low_word_set(uint32_t value) { s_global_low_word = value; }
void  global_refresh_record_set(void* record) { s_refresh_record = record; }
void* global_refresh_record_peek(void) { return s_refresh_record; }

/* --- the global position handle leaf (see engine.hpp) ----------------------
 * DAT_00556f3c has no ported owner; the reposition seam (FUN_0047dcb0) is
 * a hook boundary, so the call rides a recorded seam here. */
static void* s_position_handle;   /* DAT_00556f3c role */
static int    s_position_seam_calls;
static void*  s_position_seam_last_handle;
static int    s_position_seam_last_x;
static int    s_position_seam_last_y;

// FUNCTION: LEGORR 0x00435580
int global_handle_position_apply_if_present(void)
{
    if (s_position_handle == NULL) {
        return 0;
    }
    ++s_position_seam_calls;
    s_position_seam_last_handle = s_position_handle;
    s_position_seam_last_x = 0;
    s_position_seam_last_y = 0;
    return 1;
}

void global_position_handle_set(void* handle) { s_position_handle = handle; }
void* global_position_handle_peek(void) { return s_position_handle; }
void global_position_seam_reset(void)
{
    s_position_seam_calls = 0;
    s_position_seam_last_handle = NULL;
    s_position_seam_last_x = 0;
    s_position_seam_last_y = 0;
}
int global_position_seam_calls(void) { return s_position_seam_calls; }
void global_position_seam_last(void** out_handle, int* out_x, int* out_y)
{
    if (out_handle != NULL) {
        *out_handle = s_position_seam_last_handle;
    }
    if (out_x != NULL) {
        *out_x = s_position_seam_last_x;
    }
    if (out_y != NULL) {
        *out_y = s_position_seam_last_y;
    }
}

/* ScrollScaleSetAndClamp (FUN_00425ce0) — see engine.hpp. The corpus reads
 * its speed word through the overlapping _DAT_0055615c alias; the port keeps
 * one float in this module's storage. */
static float s_scroll_scale = 1.0f; /* DAT_0055615c role; corpus default 1.0 */

float scroll_scale_peek(void) { return s_scroll_scale; }
void  scroll_scale_set(float value) { s_scroll_scale = value; }

int scroll_speed_index_get(void)
{
    const float v = s_scroll_scale;
    if (v < 0.6666667f) { return 0; }
    if (v < 1.0f)       { return 1; }
    if (v < 1.3333334f) { return 2; }
    if (v < 1.6666666f) { return 3; }
    if (v < 2.0f)       { return 4; }
    return 5;
}

// FUNCTION: LEGORR 0x00425ce0
void scroll_scale_set_and_clamp(float scale)
{
    if (s_scroll_scale <= scale || s_scroll_guard == 0) {
        s_scroll_scale = scale;
        if (s_scroll_scale < 0.0f) {
            s_scroll_scale = 0.0f;
        }
        if (s_scroll_scale > 3.0f) {
            s_scroll_scale = 3.0f;
        }
    }
}

/* CameraFocusScrollScaleCommit (FUN_004538a0) — see engine.hpp. The corpus
 * reads the level-state root global and strides both windows blindly. */
// FUNCTION: LEGORR 0x004538a0
int camera_focus_scroll_scale_commit(const EngineContext* ctx,
                                     const void* record)
{
    if (record == NULL) {
        return 0; /* deviation: the corpus strides the record blindly */
    }
    uint32_t focus = 0, guard = 0;
    __builtin_memcpy(&focus, (const char*)record + 0x00, sizeof focus);
    __builtin_memcpy(&guard, (const char*)record + 0x04, sizeof guard);
    if (focus == 0) {
        const EngineState* st = ctx != NULL ? ctx->state_block : NULL;
        if (st != NULL && st->focus_gate_27c != 0) {
            return 0;
        }
    }
    global_scroll_guard_set((int)guard);
    scroll_scale_set_and_clamp((float)focus * 0.01f);
    global_scroll_guard_set((int)guard);
    return 0;
}

/* PanelSelectionHighlightToggle (FUN_00435aa0) — see engine.hpp. */
// FUNCTION: LEGORR 0x00435aa0
void panel_selection_highlight_toggle(EngineContext* ctx, int on)
{
    if (on != 0) {
        if (ctx != NULL) {
            ctx->debug |= 1u;
            if (ctx->exit_timer <= 0.0f) {
                sound_alias_play(0x19, 0);
                ctx->exit_timer = 250.0f;
            }
        }
        panel_item_flag8_set_if_enabled(6, 2, 1);
    } else {
        if (ctx != NULL) {
            ctx->debug &= ~1u;
            ctx->exit_timer = 0.0f;
        }
        panel_item_flag8_set_if_enabled(6, 2, 0);
    }
}

/* TerrainCellCommandPredicate (FUN_00428780) — see engine.hpp. The corpus
 * re-reads its system-state word through FUN_00456240 and strides the
 * voxel cell blindly; the port reads it once and answers through the
 * bounds-checking voxel accessor (deviations). CurrentLevelGet's block is
 * the bound level_block here. */
// FUNCTION: LEGORR 0x00428780
int terrain_cell_command_predicate(const EngineContext* ctx, int x, int y)
{
    const uint32_t s = system_state_flags_get();
    if (s != 0u) {
        if ((s & 8u) == 0u) {
            if ((s & 4u) == 0u) {
                return 1;
            }
        }
    }
    if ((s & 2u) != 0u) {
        const uint8_t* cell = engine_level_voxel(ctx, x, y);
        if (cell != NULL && cell[0x4] == 0u) {
            return 1; /* deviation: an absent/occupied cell blocks */
        }
    }
    return 0;
}

/* --- the field-grid cell window band (see engine.hpp) --------------------- */

static char* field_grid_cell(int x, int y)
{
    char* buf = (char*)entity_field_grid_buffer_peek();
    const EngineState* desc = entity_field_grid_descriptor_peek();
    if (buf == NULL || desc == NULL) {
        return NULL; /* deviation: the corpus strides missing words blindly */
    }
    return buf + ((size_t)((int)desc->grid_w * y + x) * 0x10u);
}

// FUNCTION: LEGORR 0x00466380
void set_expansion_entity_at_grid_cell(int x, int y, void* entity)
{
    char* cell = field_grid_cell(x, y);
    if (cell == NULL) {
        return; /* deviation: the corpus strides the missing grid blindly */
    }
    __builtin_memcpy(cell + 0x00, &entity, sizeof entity);
    uint32_t flags = 0u;
    __builtin_memcpy(&flags, cell + 0x0c, sizeof flags);
    flags |= 0x100u;
    __builtin_memcpy(cell + 0x0c, &flags, sizeof flags);
    const float timer = 100.0f; /* the corpus's 0x42c80000 store */
    __builtin_memcpy(cell + 0x08, &timer, sizeof timer);
}

// FUNCTION: LEGORR 0x00466750
void grid_cell_invalidate_and_clear_flags(int x, int y)
{
    char* cell = field_grid_cell(x, y);
    if (cell == NULL) {
        return; /* deviation: the corpus strides the missing grid blindly */
    }
    /* Corpus shape: the entity is re-read from the slot before each flag
     * window it touches (the aliasing reloads). */
    void* entity = NULL;
    __builtin_memcpy(&entity, cell + 0x00, sizeof entity);
    if (entity != NULL) {
        uint32_t w = 0u;
        __builtin_memcpy(&w, (char*)entity + 0x3e8, sizeof w);
        w |= 0x800000u;
        __builtin_memcpy((char*)entity + 0x3e8, &w, sizeof w);
        entity = NULL;
        __builtin_memcpy(&entity, cell + 0x00, sizeof entity);
        if (entity != NULL) {
            __builtin_memcpy(&w, (char*)entity + 0x3e0, sizeof w);
            w &= 0xfdffffffu;
            __builtin_memcpy((char*)entity + 0x3e0, &w, sizeof w);
        }
    }
    uint32_t flags = 0u;
    __builtin_memcpy(&flags, cell + 0x0c, sizeof flags);
    flags &= 0xfffffeffu;
    __builtin_memcpy(cell + 0x0c, &flags, sizeof flags);
}

// FUNCTION: LEGORR 0x004663f0
int can_spawn_expansion_entity_at_grid_cell(int x, int y, float* out)
{
    const EngineState* desc = entity_field_grid_descriptor_peek();
    if (desc == NULL || desc->grid_cells == NULL) {
        return 0; /* deviation: the corpus strides both words blindly */
    }
    const int w = (int)desc->grid_w;
    const char* vox = (const char*)desc->grid_cells;
    const ptrdiff_t idx = (ptrdiff_t)y * w + x;
    const char* f = vox + 8 + idx * 0x48;
    if ((f[0] & 8) == 0) {
        return 0;
    }
    if ((f[0x50] & 0x10) == 0 || (f[-0x40] & 0x10) == 0) {
        const char* up = vox + 8 + ((ptrdiff_t)(y + 1) * w + x) * 0x48;
        const char* dn = vox + 8 + ((ptrdiff_t)(y - 1) * w + x) * 0x48;
        if ((up[0] & 0x10) == 0 || (dn[0] & 0x10) == 0) {
            return 0;
        }
        if (out != NULL) {
            *out = 0.0f;
        }
    } else if (out != NULL) {
        const unsigned bits = 0x3fc90fdbu; /* the corpus's float word */
        __builtin_memcpy(out, &bits, sizeof *out);
        return 1;
    }
    return 1;
}
