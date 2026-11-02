// SPDX-License-Identifier: LGPL-3.0-or-later
#include "camera.hpp"
#include "engine.hpp"
#include "frameloop.hpp"
#include "mainsim.hpp"
#include "message.hpp"
#include "tickers.hpp"
#include "view.hpp"
#include "record.hpp"
#include "crt/crt.hpp"
#include "sound.hpp"
#include "audio.hpp"
#include "media.hpp"
#include "resource.hpp"
#include "heap.hpp"
#include "scene_resources.hpp"
#include "view.hpp"
#include "game/frontend.hpp"
#include "game/entity.hpp"
#include "game/objective.hpp"
#include "game/port_loop_ports.hpp"
#include "heap.hpp"
#include <ctype.h>
#include <stdlib.h>

/* PanelSelectionHighlightToggle/EngineTick DAT globals (see the function
 * comments at their use sites): bit-0 flag word and the 250-ms cooldown in
 * the zero-filled .data virtual tail. Declared here because EngineTick owns
 * the corpus's scaled countdown of DAT_00556efc. */
// GLOBAL: LEGORR 0x00556ed0
static uint32_t s_panel_highlight_flags; /* DAT_00556ed0 bit 0 */
// GLOBAL: LEGORR 0x00556efc
static float s_panel_highlight_timer;    /* DAT_00556efc 250-ms cooldown */
static EngineContext* s_engine_pre_tick_context;
// GLOBAL: LEGORR 0x00556ec8
static int32_t s_context_mode;

/* PanelSelectionHighlightToggle compares against this file-backed .rdata
 * dword (the PE bytes at 0x0049e2b8 are 00 00 00 00, +0.0f). Volatile
 * preserves the corpus's explicit memory comparison instead of folding it
 * to an immediate floating-point constant. */
// GLOBAL: LEGORR 0x0049e2b8
static const volatile float s_panel_highlight_zero = 0.0f;

/* GlobalField505f88Get and CommandLineOptionParse share this app-mode word. */
// GLOBAL: LEGORR 0x00505f88
static int32_t s_application_mode;
static EngineContext* s_engine_current_context;
static EngineKeepRunningHook s_keep_running_hook;
static void* s_keep_running_user;
static EngineTutorialDispatchHook s_tutorial_dispatch_hook;
static void* s_tutorial_dispatch_context;

void engine_tutorial_dispatch_hook_set(EngineTutorialDispatchHook hook,
                                       void* context)
{
    s_tutorial_dispatch_hook = hook;
    s_tutorial_dispatch_context = context;
}
static unported_ledger_stubs::GlobalSubsystemResetState*
    s_global_subsystem_reset_state;

void unported_ledger_stubs::global_subsystem_reset_state_bind(
    unported_ledger_stubs::GlobalSubsystemResetState* state)
{
    s_global_subsystem_reset_state = state;
}

// FUNCTION: LEGORR 0x004263c0
void unported_ledger_stubs::ledger_stub_004263c0_GlobalSubsystemStateReset(
    void)
{
    static const int mode = 1;
    unported_ledger_stubs::GlobalSubsystemResetState* state =
        s_global_subsystem_reset_state;
    (void)engine_mode_set_target(&mode);
    if (state != NULL) {
        if (state->word_00503618 != NULL) *state->word_00503618 = 0u;
        text_buffer_reset(state->text_buffer);
    }
    engine_global_state_flags_bits6_and_9_set_target(0, 0);
    engine_global_state_flags_bit7_set_target(0);
}

uint32_t* engine_current_feature_word(void)
{
    return s_engine_current_context != NULL
        ? &s_engine_current_context->feature : NULL;
}

uint32_t* engine_current_state_word(void)
{
    return s_engine_current_context != NULL
        ? &s_engine_current_context->state : NULL;
}

/* The reference's initialization callback clears this state before storing
 * the executable basename at DAT_005560c4. The portable modules do not share
 * a byte-identical 0x3be-dword global block, so reset of their owned state is
 * supplied by an explicit hook instead of clearing unrelated module globals. */
// GLOBAL: LEGORR 0x005560c4
static const char* s_engine_config_root;
static EngineContext* s_callback_registry_context;
static EngineConfigRegistryResetHook s_callback_registry_reset_hook;
static EngineGameConfigInitHook s_game_config_init_hook;
static void* s_callback_registry_user;

void engine_game_config_callback_registry_hooks_set(
    EngineContext* context,
    EngineConfigRegistryResetHook reset_hook,
    EngineGameConfigInitHook init_hook,
    void* user)
{
    s_callback_registry_context = context;
    s_callback_registry_reset_hook = reset_hook;
    s_game_config_init_hook = init_hook;
    s_callback_registry_user = user;
}

static int engine_game_config_init_callback(void)
{
    if (s_game_config_init_hook == NULL) return 0;
    return s_game_config_init_hook(s_engine_config_root,
                                   s_callback_registry_user);
}

static int engine_registered_tick_callback(float dt)
{
    if (s_callback_registry_context == NULL) return 0;
    return EngineTick(s_callback_registry_context, dt);
}

static void engine_registered_shutdown_normal_callback(void)
{
    if (s_callback_registry_context != NULL)
        EngineShutdown(s_callback_registry_context);
}

static void engine_registered_shutdown_loader_callback(void)
{
    if (s_callback_registry_context != NULL)
        EngineShutdown(s_callback_registry_context);
}

/* GameConfigCallbackRegistryInit (FUN_0041fa20) clears the reference's
 * 0x3be-dword state block, stores the root name, sets the window title, and
 * installs init/tick/shutdown callbacks. The reset and game-config callbacks
 * are explicit seams because their state and implementation are not owned by
 * this engine module. The adapters below match the callback signatures used
 * by the frame loop; the context-taking EngineTick/EngineShutdown functions
 * themselves are never published as incompatible raw callbacks. */
// FUNCTION: LEGORR 0x0041fa20
void engine_game_config_callback_registry_init(const char* root)
{
    size_t callbacks[3];
    const int loader_mode = s_application_mode == ENGINE_APP_LOADER;

    if (s_callback_registry_reset_hook != NULL)
        s_callback_registry_reset_hook(s_callback_registry_user);
    s_engine_config_root = root;
    window_title_set(root);

    callbacks[0] = reinterpret_cast<size_t>(
        &engine_game_config_init_callback);
    callbacks[1] = reinterpret_cast<size_t>(
        &engine_registered_tick_callback);
    callbacks[2] = reinterpret_cast<size_t>(loader_mode ?
        &engine_registered_shutdown_loader_callback :
        &engine_registered_shutdown_normal_callback);
    (void)window_runtime_callback_triplet_set(callbacks);
}

int engine_callback_triplet_init_dispatch(void)
{
    size_t callbacks[3];
    typedef int (*InitCallback)(void);
    if (!window_runtime_callback_triplet_get(callbacks) || callbacks[0] == 0u)
        return 0;
    return reinterpret_cast<InitCallback>(callbacks[0])();
}

int engine_callback_triplet_tick_dispatch(float dt)
{
    size_t callbacks[3];
    typedef int (*TickCallback)(float);
    if (!window_runtime_callback_triplet_get(callbacks) || callbacks[1] == 0u)
        return 0;
    return reinterpret_cast<TickCallback>(callbacks[1])(dt);
}

void engine_callback_triplet_shutdown_dispatch(void)
{
    size_t callbacks[3];
    typedef void (*ShutdownCallback)(void);
    if (!window_runtime_callback_triplet_get(callbacks) || callbacks[2] == 0u)
        return;
    reinterpret_cast<ShutdownCallback>(callbacks[2])();
}

// WindowModeFlagSet records the mode selected by renderer startup.
// GLOBAL: LEGORR 0x005764f4
static uint32_t s_window_mode_flag;
// FUNCTION: LEGORR 0x0048b280
void window_mode_flag_set(uint32_t mode)
{
    s_window_mode_flag = mode;
}

/* Frame tick — corpus EngineTick @ 0x4232e0. The numbered structure below
 * mirrors finding 03 §"Tick structure" and the first 130 lines of the corpus.
 * Every subsystem ticker is a registered stub in tickers.cpp whose function
 * body is filled in from the corpus one at a time; the *order* here is the
 * observed call order and must stay stable.
 */
// FUNCTION: LEGORR 0x004232e0
int EngineTick(EngineContext* ctx, float dt)
{
    s_engine_current_context = ctx;
    /* 1. Gate — loader/benchmark mode idles out of the frame. */
    if (ctx->app_mode == ENGINE_APP_LOADER && EngineKeepRunning(ctx) == 0) {
        return 0;
    }
    /* Frozen time zeroes dt (corpus: DAT_00556ecc & 0x100000). */
    if ((ctx->state & ENGINE_STATE_TIME_FROZEN) != 0) {
        dt = 0.0f;
    }

    /* 2. Per-frame housekeeping (EnginePreTick @ 0x4231f0). When text entry is
     *    active it consumes this frame's key table; the frame-loop drives the
     *    normal capture/input_begin_frame lifecycle — see frameloop.cpp. */
    EngineContext* previous_pre_tick_context = s_engine_pre_tick_context;
    s_engine_pre_tick_context = ctx;
    EnginePreTick();
    s_engine_pre_tick_context = previous_pre_tick_context;

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
    if (main_sim_step(ctx, scale, &sim_out1, &sim_out2, &sim_out3) == 0) {
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

    /* TutorialScriptCommandDispatch follows the manager/render tick cluster
     * in the corpus and is disabled by DAT_00556ecc bit 0x01000000. Keep its
     * game-owned interpreter behind a narrow callback so EngineTick does not
     * depend on the game layer. */
    if ((ctx->state & 0x01000000u) == 0u &&
        s_tutorial_dispatch_hook != NULL)
        s_tutorial_dispatch_hook(dt, s_tutorial_dispatch_context);

    /* Exit timer counts down in scaled time (corpus: DAT_00556efc -= scale). */
    s_panel_highlight_timer = s_panel_highlight_timer - scale;

    return 1;
}

static void engine_pre_tick_context_run(EngineContext* ctx)
{
    InputState* input;
    char* target;
    int length = 0;
    int scan_limit;
    int key;
    int cancel;
    if (ctx == NULL || ctx->input == NULL) return;
    input = ctx->input;
    target = input->text_target;
    if (target == NULL) return;

    /* The retail routine only edits strings shorter than eleven characters;
     * the explicit cap also protects this portable owner buffer. */
    scan_limit = input->text_cap > 0 ? input->text_cap : 12;
    if (scan_limit > 12) scan_limit = 12;
    while (length < scan_limit && target[length] != '\0') ++length;

    if (input->cur[0x8e] == 0u ||
        input->mirror[0x8e] == input->cur[0x8e]) {
        cancel = (input->cur[0x9c] == 0u &&
                  input->mirror[0x9c] != 0u) ||
                 (input->cur[0x01] == 0u &&
                  input->mirror[0x01] != 0u);
        if (cancel) {
            input->text_target = NULL;
        }
        else {
            int text_limit = input->text_cap > 0
                ? input->text_cap - 1 : 11;
            if (text_limit > 11) text_limit = 11;
            for (key = 0; key < INPUT_KEY_COUNT && length < text_limit;
                 ++key) {
                uint32_t mapped;
                if (input->cur[key] == 0u ||
                    input->mirror[key] == input->cur[key]) continue;
                mapped = input_key_context(input, (uint32_t)key);
                if (mapped == 0u) continue;
                target[length++] = (char)toupper((unsigned char)mapped);
                target[length] = '\0';
            }
        }
    }
    else if (length != 0) {
        target[length - 1] = '\0';
        --length;
    }
    input->text_len = length;
    /* Retail snapshots the current table into the text mirror and clears it
     * only while a text target is active. This prevents edited keys from
     * leaking into the following MainSimStep key-edge handling. */
    input_mirror_and_clear(input);
}

void engine_pre_tick_adapter(EngineContext* ctx)
{
    engine_pre_tick_context_run(ctx);
}

/* Retail-shaped EnginePreTick @ 0x4231f0 uses global input/text state. */
// FUNCTION: LEGORR 0x004231f0
void EnginePreTick(void)
{
    EngineContext* ctx = s_engine_pre_tick_context;
    InputState* input;
    char* target;
    int length = 0;
    int scan_limit;
    int key;
    int cancel;

    if (ctx == NULL || ctx->input == NULL) return;
    input = ctx->input;
    target = input->text_target;
    if (target == NULL) return;

    scan_limit = input->text_cap > 0 ? input->text_cap : 12;
    if (scan_limit > 12) scan_limit = 12;
    while (length < scan_limit && target[length] != '\0') ++length;

    if (input->cur[0x8e] == 0u ||
        input->mirror[0x8e] == input->cur[0x8e]) {
        cancel = (input->cur[0x9c] == 0u &&
                  input->mirror[0x9c] != 0u) ||
                 (input->cur[0x01] == 0u &&
                  input->mirror[0x01] != 0u);
        if (cancel) {
            input->text_target = NULL;
        }
        else {
            int text_limit = input->text_cap > 0
                ? input->text_cap - 1 : 11;
            if (text_limit > 11) text_limit = 11;
            for (key = 0; key < INPUT_KEY_COUNT && length < text_limit;
                 ++key) {
                uint32_t mapped;
                if (input->cur[key] == 0u ||
                    input->mirror[key] == input->cur[key]) continue;
                mapped = input_key_context(input, (uint32_t)key);
                if (mapped == 0u) continue;
                target[length++] = (char)toupper((unsigned char)mapped);
                target[length] = '\0';
            }
        }
    }
    else if (length != 0) {
        target[length - 1] = '\0';
        --length;
    }
    input->text_len = length;
    /* TransitionControl(0,0), called by the original cancel branch, is an
     * intentional no-op in the checked-in decompile. */
    input_mirror_and_clear(input);
}

/* KeepRunning's global app-state and resource predicates are owned by the
 * game/front-end layers. The engine performs its corpus-visible sound stop,
 * then delegates that state-dependent chain through this installable port
 * hook. An absent hook keeps the standalone engine loop alive. */
void engine_keep_running_hook_set(EngineKeepRunningHook hook, void* user)
{
    s_keep_running_hook = hook;
    s_keep_running_user = user;
}

/* Portable quit/liveness adapter. Retail KeepRunning @ 0x435940 is a global
 * no-argument state machine that also polls loader/resource interfaces; the
 * current EngineContext/hook API does not expose those owners. */
int EngineKeepRunning(EngineContext* ctx)
{
    if (ctx == NULL) return 0;
    sound_subsystem_stop_all();
    if (s_keep_running_hook != NULL)
        return s_keep_running_hook(ctx, s_keep_running_user) != 0;
    return 1;
}

/* Portable context adapter used by EngineTick and local tests. Retail
 * FUN_0041fb40 has no arguments and reads DAT_0055615c directly. */
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

/* ResetLevelAndExit @ 0x4250a0. The original normal-play callback does not
 * run the loader teardown: it disables the CD watchdog and exits through the
 * CRT. Keep that path distinct from EngineShutdown's loader-mode cleanup. */
static void engine_reset_level_and_exit(void)
{
    media_cd_watchdog_update(0);
    exit(0);
}

/* EngineShutdown @ 0x424d00. Several corpus owners (renderer globals and
 * five global entity arrays) have no corresponding EngineContext binding in
 * this port. Run the teardown operations whose owners/APIs are represented;
 * do not pass unrelated context fields to renderer cleanup routines. */
void EngineShutdown(EngineContext* ctx)
{
    if (ctx == NULL) {
        return;
    }

    if (ctx->app_mode != ENGINE_APP_LOADER) {
        engine_reset_level_and_exit();
        return;
    }

    /* Ordered leading subsystem teardown from the checked-in shutdown
     * decompile. FUN_0042f0c0's global pending-level owner is not bound here. */
    global_resource_handle_release();
    reward_task_block_finish();
    media_cd_watchdog_update(0);
    media_audio_playback_toggle(0);
    resource_handle_set_release();
    /* EntityTableMapsAllocate registers its six exact-width allocations in
     * the resource release band; clear the native owner after that band has
     * released them so a later config re-init cannot retain dangling maps. */
    entity_table_maps_current_release();

    /* The three view globals are represented by the context's view slots. */
    if (ctx->views[0] != NULL) ctx->views[0]->target_record = NULL;
    if (ctx->views[1] != NULL) ctx->views[1]->target_record = NULL;
    if (ctx->views[2] != NULL) ctx->views[2]->target_record = NULL;
    camera_view_context_close(ctx->views[0]);
    camera_view_context_close(ctx->views[1]);
    camera_view_context_close(ctx->views[2]);
    ctx->views[0] = NULL;
    ctx->views[1] = NULL;
    ctx->views[2] = NULL;
    memset(ctx->camera_targets, 0, sizeof(ctx->camera_targets));
    ctx->camera_target_count = 0u;

    /* Existing resource and entity-pool owners corresponding to later
     * teardown calls. Renderer-only release calls and the five unbound
     * entity-array destructor loops remain unavailable. */
    rock_fall_style_scene_resources_release();
    pooled_record_pool_resources_release();
    lego::scene_resource_slots_release_all();
    lego::scene_resource_string_tables_free();
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
 * writer is the unported renderer-side sim-interaction update). The triple is
 * three individually-addressed dwords in the corpus, so it carves into one
 * annotated static per dword — reccmp resolves whole objects only and an
 * array's tail elements would render as (OFFSET) fragments. */
// GLOBAL: LEGORR 0x004de548
static int s_scroll_guard;        /* DAT_004de548 */
// GLOBAL: LEGORR 0x0055651c
static uint32_t s_triple_first;  /* DAT_0055651c role: unowned */
// GLOBAL: LEGORR 0x00556520
static uint32_t s_triple_second; /* DAT_00556520 role: unowned */
// GLOBAL: LEGORR 0x00556524
static uint32_t s_triple_third;  /* DAT_00556524 role: unowned */
// GLOBAL: LEGORR 0x00556f24
static float s_task_text_origin_x;
// GLOBAL: LEGORR 0x00556f28
static float s_task_text_origin_y;

// FUNCTION: LEGORR 0x00425d80
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) bool current_engine_mode_is_zero(void)
{
    __asm {
        mov ecx, dword ptr [s_context_mode]
        xor eax, eax
        test ecx, ecx
        sete al
        ret
    }
}
#else
bool current_engine_mode_is_zero(void)
{
    return s_context_mode == 0;
}
#endif

void engine_context_mode_global_set(int32_t mode)
{
    s_context_mode = mode;
}

/* EngineModeSet (FUN_004565f0) clears the two advisor-text request globals;
 * their normal writers/readers are in the advisor text band. */
// GLOBAL: LEGORR 0x00503610
static uint32_t s_overlay_word_03610; /* DAT_00503610 role */
// GLOBAL: LEGORR 0x00503614
static uint32_t s_overlay_word_03614; /* DAT_00503614 role */

void engine_global_state_flags_bit7_set_target(int value);
void engine_global_state_flags_bits6_and_9_set_target(
    int value, int set_bit9);

/* The mapped body takes only the mode pointer; keep the portable context
 * adapter separate to preserve that x86 argument layout. */
// FUNCTION: LEGORR 0x004565f0
int engine_mode_set_target(const int* mode)
{
    global_flag_004ff978_install((uint32_t)*mode);
    if (*mode == 1) {
        engine_global_state_flags_bits6_and_9_set_target(0, 1);
        engine_global_state_flags_bit7_set_target(0);
    }
    s_overlay_word_03610 = 0u;
    s_overlay_word_03614 = 0u;
    return *mode;
}

int engine_mode_set(EngineContext* ctx, const int* mode)
{
    int result;
    if (mode == NULL) {
        return 0; /* portable adapter guard; mapped body dereferences */
    }
    result = engine_mode_set_target(mode);
    if (ctx != NULL && *mode == 1) {
        ctx->debug &= ~(0x40u | 0x80u);
        ctx->debug |= 0x200u;
    }
    return result;
}

uint32_t engine_overlay_word_peek(int which)
{
    return which == 0 ? s_overlay_word_03610 : s_overlay_word_03614;
}

void engine_overlay_words_set(uint32_t flags, uint32_t request)
{
    s_overlay_word_03610 = flags;
    s_overlay_word_03614 = request;
}

/* ObjectiveScoreTextOriginSet (FUN_004262e0) — see engine.hpp. The pair
 * rides unowned-global module storage (the corpus's writer side is this
 * setter; the reader is the unported score-text draw band — deviation). */
// GLOBAL: LEGORR 0x00556f1c
static float s_score_text_origin_x;
// GLOBAL: LEGORR 0x00556f20
static float s_score_text_origin_y;

// FUNCTION: LEGORR 0x004262e0
void objective_score_text_origin_set(const float* pair)
{
    if (pair != NULL) {
        s_score_text_origin_x = pair[0];
        s_score_text_origin_y = pair[1];
        /* Corpus 0x4262f3..0x426301: mov eax,ds:0x556ed0 / or ah,0x40 /
         * mov ds:0x556ed0,eax — the shared debug/cheat word (the panel
         * highlight flag word), not the caller's context. */
        s_panel_highlight_flags = s_panel_highlight_flags | 0x4000u;
        return;
    }
    /* Corpus 0x426307..0x42630f: mov / and ah,0xbf / mov. */
    s_panel_highlight_flags = s_panel_highlight_flags & ~0x4000u;
}

uint32_t engine_debug_word_peek(void)
{
    return s_panel_highlight_flags;
}

void engine_debug_word_bit_set(uint32_t mask, int enabled)
{
    if (enabled != 0) s_panel_highlight_flags |= mask;
    else s_panel_highlight_flags &= ~mask;
}

void objective_score_text_origin_peek(float out[2])
{
    if (out != NULL) {
        out[0] = s_score_text_origin_x;
        out[1] = s_score_text_origin_y;
    }
}

// FUNCTION: LEGORR 0x004263a0
void engine_global_state_flags_bit7_set_target(int value)
{
    if (value != 0) {
        s_panel_highlight_flags |= 0x80u;
        return;
    }
    s_panel_highlight_flags &= ~0x80u;
}

void global_state_flags_bit7_set(EngineContext* ctx, int value)
{
    if (ctx == NULL) {
        return; /* deviation: the corpus writes its global blindly */
    }
    if (value != 0) {
        ctx->debug |= 0x80u;
        return;
    }
    ctx->debug &= ~0x80u;
}

// FUNCTION: LEGORR 0x00426360
void engine_global_state_flags_bits6_and_9_set_target(
    int value, int set_bit9)
{
    /* FUN_00426360 commits the bit-6 update before testing its second
     * argument, then performs the bit-9 update against that stored word. */
    if (value != 0) s_panel_highlight_flags |= 0x40u;
    else s_panel_highlight_flags &= ~0x40u;
    if (set_bit9 != 0) {
        s_panel_highlight_flags |= 0x200u;
        return;
    }
    s_panel_highlight_flags &= ~0x200u;
}

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
        s_task_text_origin_x = pair[0];
        s_task_text_origin_y = pair[1];
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
        out[0] = s_task_text_origin_x;
        out[1] = s_task_text_origin_y;
    }
}

// FUNCTION: LEGORR 0x00426230
void global_triple_copy_out(uint32_t* out)
{
    if (out == NULL) {
        return; /* deviation: the corpus writes through blindly */
    }
    out[0] = s_triple_first;
    out[1] = s_triple_second;
    out[2] = s_triple_third;
}

/* GlobalField505f88Get (FUN_00401bf0): app-mode global read. */
// FUNCTION: LEGORR 0x00401bf0
int engine_app_mode_get(void)
{
    return s_application_mode;
}

void engine_app_mode_global_set(int32_t mode)
{
    s_application_mode = mode;
}

/* --- the grid-cell record buffer (see engine.hpp) ---------------------------- */
// GLOBAL: LEGORR 0x005520e8
static void*        s_field_grid_buf;   /* DAT_005520e8 role */
// GLOBAL: LEGORR 0x005520ec
static EngineState* s_field_grid_desc;  /* DAT_005520ec role */

// FUNCTION: LEGORR 0x004661c0
void grid_cell_record_buffer_allocate(EngineState* block)
{
    if (block == NULL) {
        return; /* deviation: the corpus strides the block blindly */
    }
    const int w = (int)block->grid_w;
    const int h = (int)block->grid_h;
    /* The corpus uses 12-byte records on x86 (entity, timer, flags). The
     * portable build widens the entity slot with sizeof(void*), retaining a
     * 16-byte record on LP64 while naturally returning to 12 bytes on x86. */
    const int cell_size = (int)sizeof(void*) + 8;
    void* buf = engine_heap_alloc((long)(w * h * cell_size));
    if (buf != NULL) {
        uint32_t* p = (uint32_t*)buf;
        for (uint32_t n = (uint32_t)(w * h * (cell_size / 4)) & 0x3fffffffu;
             n != 0u; --n) {
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
// GLOBAL: LEGORR 0x00556ed4
static uint32_t s_global_low_word; /* DAT_00556ed4 role */
// GLOBAL: LEGORR 0x005563b4
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
// GLOBAL: LEGORR 0x00556f3c
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
// GLOBAL: LEGORR 0x0055615c
static float s_scroll_scale = 1.0f; /* DAT_0055615c; port default (ref is zero-filled) */

/* TimeScaleGet @ 0x0041fb40 returns the shared scroll/time scale global. */
// FUNCTION: LEGORR 0x0041fb40
float EngineTimeScale(void)
{
    return s_scroll_scale;
}

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

/* PanelSelectionHighlightToggle (FUN_00435aa0) — see engine.hpp. The corpus
 * rides two DAT globals, not the context: bit 0 of the shared debug/cheat
 * flag word DAT_00556ed0 (25 further corpus claimants — docs/dat/globals.csv)
 * and the cooldown timer DAT_00556efc, both in the zero-filled .data virtual
 * tail. Its fld/fcomp/fnstsw, test ah,0x41, je gate is MSVC's `<=` shape
 * against the .rdata +0.0f at 0x0049e2b8: only a strictly greater timer
 * skips the sound; lesser and equal timers play and re-arm it. (The
 * `!(timer > zero)` spelling folds to `zero >= timer` and emits the
 * swapped-operand C0-only test instead — measured 84.4 vs 87.5.) The ctx
 * parameter is a port-only API carry-over (callers/tests use it); the
 * corpus has one cdecl argument, whereas this declaration's select is its
 * second argument. */
/* (The DAT objects live above EngineTick — single definition, two sites.) */

// FUNCTION: LEGORR 0x00435aa0
void panel_selection_highlight_toggle(EngineContext* ctx, int on)
{
    (void)ctx; /* the corpus dereferences nothing; it only tests the arg */
    if (on != 0) {
        s_panel_highlight_flags = s_panel_highlight_flags | 1u;
        if (s_panel_highlight_timer <= s_panel_highlight_zero) {
            sound_alias_play(0x19, 0);
            s_panel_highlight_timer = 250.0f;
        }
        panel_item_flag8_set_if_enabled(6, 2, 1);
    } else {
        s_panel_highlight_flags = s_panel_highlight_flags & ~1u;
        s_panel_highlight_timer = 0.0f;
        panel_item_flag8_set_if_enabled(6, 2, 0);
    }
}

/* TerrainCellCommandPredicate (FUN_00428780) — see engine.hpp. The corpus
 * re-reads its system-state word through FUN_00456240 before each gate and
 * strides the voxel cell blindly; the port answers through the
 * bounds-checking voxel accessor for that latter path. CurrentLevelGet's
 * block is the bound level_block here. */
// FUNCTION: LEGORR 0x00428780
int terrain_cell_command_predicate(const EngineContext* ctx, int x, int y)
{
    if (system_state_flags_get() != 0u) {
        if ((system_state_flags_get() & 8u) == 0u) {
            if ((system_state_flags_get() & 4u) == 0u) {
                return 1;
            }
        }
    }
    if ((system_state_flags_get() & 2u) != 0u) {
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
    return buf + ((size_t)((int)desc->grid_w * y + x) *
                   ((size_t)sizeof(void*) + 8u));
}

int entity_field_grid_cell_get(int x, int y, void** entity_out,
                               uint32_t* flags_out)
{
    const EngineState* desc = s_field_grid_desc;
    char* cell;
    void* entity = NULL;
    uint32_t flags = 0u;
    if (desc == NULL || x < 0 || y < 0 ||
        (uint32_t)x >= desc->grid_w || (uint32_t)y >= desc->grid_h)
        return 0;
    cell = field_grid_cell(x, y);
    if (cell == NULL) return 0;
    __builtin_memcpy(&entity, cell, sizeof(entity));
    __builtin_memcpy(&flags, cell + sizeof(void*) + 4, sizeof(flags));
    if (entity_out != NULL) *entity_out = entity;
    if (flags_out != NULL) *flags_out = flags;
    return 1;
}

// FUNCTION: LEGORR 0x00466380
void set_expansion_entity_at_grid_cell(int x, int y, void* entity)
{
    if (s_field_grid_buf != NULL && s_field_grid_desc != NULL) {
        /* Preserve the corpus's repeated index calculation and write order.
         * On x86 these expressions use stride 0x0c, flags at +8, and timer
         * at +4: imul width*y; add x; lea index*3; then scale by four. */
        *(void**)((char*)s_field_grid_buf +
                  ((int)s_field_grid_desc->grid_w * y + x) *
                      ((int)sizeof(void*) + 8)) = entity;
        uint32_t* flags = (uint32_t*)((char*)s_field_grid_buf +
                  ((int)s_field_grid_desc->grid_w * y + x) *
                      ((int)sizeof(void*) + 8) + (int)sizeof(void*) + 4);
        *flags = *flags | 0x100u;
        *(uint32_t*)((char*)s_field_grid_buf +
                  ((int)s_field_grid_desc->grid_w * y + x) *
                      ((int)sizeof(void*) + 8) + (int)sizeof(void*)) =
            0x42c80000u;
    }
}

// FUNCTION: LEGORR 0x00466750
void grid_cell_invalidate_and_clear_flags(int x, int y)
{
    const EngineState* desc = s_field_grid_desc;
    char* cell;
    if (s_field_grid_buf == NULL || desc == NULL) {
        return; /* deviation: the corpus strides the missing grid blindly */
    }
    cell = (char*)s_field_grid_buf +
           (size_t)((int)desc->grid_w * y + x) *
               ((size_t)sizeof(void*) + 8u);
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
    __builtin_memcpy(&flags, cell + sizeof(void*) + 4, sizeof flags);
    flags &= 0xfffffeffu;
    __builtin_memcpy(cell + sizeof(void*) + 4, &flags, sizeof flags);
}

// FUNCTION: LEGORR 0x004663f0
int can_spawn_expansion_entity_at_grid_cell(int x, int y, float* out)
{
    const EngineState* desc = s_field_grid_desc;
    const int w = (int)desc->grid_w;
    const char* vox = (const char*)desc->grid_cells;
    const long i = (long)y * w + x;
    const long s = i * 9; /* the corpus's lea [eax+eax*8], *8 rides the scale */
    const char* base = vox + s * 8;
    if ((base[8] & 8) == 0) {
        return 0;
    }
    if ((base[0x50] & 0x10) == 0 || (base[-0x40] & 0x10) == 0) {
        const long iu = (long)(y + 1) * w + x;
        const long su = iu * 9;
        const long id = (long)(y - 1) * w + x;
        const long sd = id * 9;
        if ((vox[su * 8 + 8] & 0x10) == 0 || (vox[sd * 8 + 8] & 0x10) == 0) {
            return 0;
        }
        if (out != NULL) {
            *(uint32_t*)out = 0u; /* the corpus's zero immediate dword */
        }
    } else if (out != NULL) {
        *(uint32_t*)out = 0x3fc90fdbu; /* the corpus's immediate dword */
        return 1;
    }
    return 1;
}

namespace unported_ledger_stubs {
EngineTaskCallbackRecord* ledger_stub_00480870_TaskCallbackRecordAllocateAndInitialize(
    void* owner, void* callback, uint32_t mode, void* context,
    uint32_t flag_value)
{
    return engine_task_callback_record_allocate_and_initialize(
        owner, callback, mode, context, flag_value);
}


} // namespace unported_ledger_stubs
