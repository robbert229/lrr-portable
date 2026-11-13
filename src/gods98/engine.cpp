// SPDX-License-Identifier: LGPL-3.0-or-later
/* Matching build: Lego.c callers call lego_unit.c (#587). */
#include "game/lego_port_aliases.h"
#include "camera.hpp"
#include "engine.hpp"
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
#include "resource.hpp"
#include "heap.hpp"
#include "scene_resources.hpp"
#include "view.hpp"
#include "game/frontend.hpp"
#include "game/entity.hpp"
#include "game/building_scene.hpp"
#include "game/actor_scene.hpp"
#include "game/objective.hpp"
#include "game/effects.hpp"
#include "game/scene_render.hpp"
#include "game/util.hpp"
#include "scene_pool.hpp"
#include "string.hpp"
#include "game/port_loop_ports.hpp"
#include "heap.hpp"
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
extern "C" uint32_t s_overlay_word_03610, s_overlay_word_03614; /* text_port.cpp; natively also lego_unit.c and tutorial_script.c */
#include "engine_shared.hpp"
/* C linkage for the legoGlobs fields that GAME-021's lego_unit.c reads
 * (issue #544). The frozen header below redeclares them and inherits it. */
extern "C" {
extern const char* s_engine_config_root;
extern uint32_t s_triple_first;
extern uint32_t s_triple_second;
extern uint32_t s_triple_third;
extern float s_task_text_origin_x;
extern float s_task_text_origin_y;
extern float s_score_text_origin_x;
extern float s_score_text_origin_y;
extern uint32_t s_global_low_word;
extern void* s_refresh_record;
extern void* s_position_handle;
extern float s_scroll_scale;
extern float s_panel_highlight_timer; /* group B */
}
#include "engine_for_lego.hpp"
extern "C" int s_scroll_guard; /* lego_unit.c reads it (issue #544) */
#include "engine_from_lego.hpp"

/* PanelSelectionHighlightToggle/EngineTick DAT globals (see the function
 * comments at their use sites): bit-0 flag word and the 250-ms cooldown in
 * the zero-filled .data virtual tail. Declared here because EngineTick owns
 * the corpus's scaled countdown of DAT_00556efc. */
// GLOBAL: LEGORR 0x00556ed0
uint32_t s_panel_highlight_flags; /* DAT_00556ed0 bit 0 */
// GLOBAL: LEGORR 0x00556efc
float s_panel_highlight_timer;    /* DAT_00556efc 250-ms cooldown */
// GLOBAL: LEGORR 0x00556ec8
extern "C" { int32_t s_context_mode; } /* gods98/engine.h: Advisor.c reads it */

/* GlobalField505f88Get and CommandLineOptionParse share this app-mode word. */
// GLOBAL: LEGORR 0x00505f88
int32_t s_application_mode;
static EngineContext* s_engine_current_context;
static uint32_t s_engine_bootstrap_state;

#if defined(LEGORRR_BACKEND_WINAPI) && defined(_WIN32) && !defined(_WIN64)
static int s_engine_scene_live;
static SceneQuadGridRecord* s_engine_placement_preview;

static int engine_placement_preview_visibility(void*, const void* record,
                                               uint32_t hidden)
{
    return scene_placement_preview_visibility_apply(record, hidden);
}

static void engine_placement_preview_shutdown(void)
{
    if (s_engine_placement_preview == NULL) return;
    if (global_refresh_record_peek() == s_engine_placement_preview)
        global_refresh_record_set(NULL);
    (void)surface_visibility_adapter_remove(
        engine_placement_preview_visibility, NULL);
    (void)scene_placement_preview_destroy(s_engine_placement_preview);
    s_engine_placement_preview = NULL;
}

static int engine_placement_preview_initialize(void)
{
    if (s_engine_placement_preview != NULL ||
        global_refresh_record_peek() != NULL) return 0;
    /* GameConfigInit4210fd..421123 constructs this root-owned footprint
     * with binary32 extrusion5.0 before publishing and hiding it. */
    s_engine_placement_preview = scene_placement_preview_create(
        scene_root_object_get(), 5.0f);
    if (s_engine_placement_preview == NULL) return 0;
    if (!surface_visibility_adapter_install(
            engine_placement_preview_visibility, NULL)) {
        (void)scene_placement_preview_destroy(s_engine_placement_preview);
        s_engine_placement_preview = NULL;
        return 0;
    }
    global_refresh_record_set(s_engine_placement_preview);
    surface_visibility_set(s_engine_placement_preview, 1u);
    return 1;
}

struct EngineSceneBuild {
    BlockFile* config;
    void* owners[4];
    void* default_owner;
    int failed;
};

static int engine_scene_rgb_read(const char* name, float rgb[3], void* user)
{
    EngineSceneBuild* build = (EngineSceneBuild*)user;
    const char* path = string_path_join(util_root_name(build->config),
        "Main", name, (char*)NULL);
    return config_rgb_vector_read(build->config, path, rgb, rgb + 1, rgb + 2);
}

static void* engine_scene_owner_get(int slot, void* user)
{
    return ((EngineSceneBuild*)user)->owners[slot];
}

static void* engine_scene_light_create(const SceneLightDefinition* def,
                                        void* user)
{
    void* light = scene_runtime_light_create(def->owner,
        (unsigned int)def->light_kind, def->rgb);
    if (light == NULL) ((EngineSceneBuild*)user)->failed = 1;
    return light;
}

static int engine_scene_parameter_set(void* light, uint32_t offset,
                                       float value, void* user)
{
    if (light == NULL) {
        ((EngineSceneBuild*)user)->failed = 1;
        return 0;
    }
    int result = scene_runtime_light_parameter_set(light, offset, value);
    if (!result) ((EngineSceneBuild*)user)->failed = 1;
    return result;
}

static int engine_scene_position_set(void* light, void* owner,
                                      const float position[3], void* user)
{
    int result = ddiscene_object_position_set(light, owner,
        position[0], position[1], position[2]);
    if (!result) ((EngineSceneBuild*)user)->failed = 1;
    return result;
}

static int engine_scene_pose_set(void* light, void* owner,
                                  const float pose[6], void* user)
{
    int result = ddiscene_object_pose_method_dispatch(light, owner,
        pose[0], pose[1], pose[2], pose[3], pose[4], pose[5]);
    if (!result) ((EngineSceneBuild*)user)->failed = 1;
    return result;
}

static int engine_scene_light_enable(void* light, int enabled, void*)
{
    if (light == NULL) return 0;
    scene_render_lock_toggle(light, enabled);
    return (int)((*(uint32_t*)((char*)light + 0x14) & 8u) != 0u) ==
        (enabled != 0);
}

static void* engine_scene_root_get(void*)
{
    return scene_root_object_get();
}

static void* engine_scene_object_allocate(void* root, void* user)
{
    EngineSceneBuild* build = (EngineSceneBuild*)user;
    build->default_owner = lego::ddiscene_object_allocate(
        (const lego::DDiSceneObjectDescriptor*)root);
    if (build->default_owner == NULL) build->failed = 1;
    return build->default_owner;
}

static int engine_scene_transform_init(void* light, void* root,
                                       const float rgb[3], void* user)
{
    EngineSceneBuild* build = (EngineSceneBuild*)user;
    (void)root;
    /* The reconstructed builder passes the root here. Retail 0x433770
     * passes DAT_00556108: the child just allocated under that root. */
    if (build->default_owner == NULL) return 0;
    scene_transform_state_initialize(light, build->default_owner,
        rgb[0], rgb[1], rgb[2]);
    return scene_transform_state_peek()->owner == build->default_owner;
}

void engine_scene_shutdown(void)
{
    actor_scene_require_stopped();
    building_scene_require_stopped();
    /* Concrete roof child/resource teardown must precede root-pool teardown
     * on normal, loader and rollback paths; the busy guard alone is not it. */
    graphics_overlay_resources_release();
    engine_placement_preview_shutdown();
    if (!s_engine_scene_live) return;
    motion_gameplay_scene_bind(NULL, 0u);
    motion_gameplay_scene_owner_bind(NULL, NULL);
    scene_runtime_shutdown();
    (void)shared_object_directory_root_set(NULL);
    s_engine_scene_live = 0;
}

int engine_scene_initialize(EngineContext* context, BlockFile* config)
{
    EngineSceneBuild build;
    SceneLightBuildOps ops;
    if (s_engine_scene_live || context == NULL || config == NULL ||
        context->views[0] == NULL || context->views[1] == NULL ||
        context->views[2] == NULL) return 0;
    if (!scene_runtime_initialize()) return 0;
    s_engine_scene_live = 1;
    if (!scene_runtime_primary_camera_bind(context->views[0],
                                           context->camera_controller)) {
        engine_scene_shutdown();
        return 0;
    }
    /* GameConfigInit0x41fb50 supplies Main SharedObjects to0x4876c0.
     * The real LWSC/LWO owner retries this configured prefix after the
     * scene directory. Sound callbacks remain with their existing owner. */
    {
        const char* path = string_path_join(util_root_name(config),
                                            "Main", "SharedObjects", NULL);
        const char* directory = block_get_strptr(config, path);
        if (!shared_object_directory_root_set(directory)) {
            engine_scene_shutdown();
            return 0;
        }
    }
    memset(&build, 0, sizeof(build));
    memset(&ops, 0, sizeof(ops));
    build.config = config;
    /* Explicit layout bridge: the retail view stores frame pointers at
     * +0x24/28/2c, while ViewContext embeds three CameraHandle values. */
    build.owners[SCENE_LIGHT_OWNER_TOP_SPOT] =
        scene_runtime_camera_bind(&context->views[0]->cam_facing);
    build.owners[SCENE_LIGHT_OWNER_TRACK_SPOT] =
        scene_runtime_camera_bind(&context->views[1]->cam_follow);
    build.owners[SCENE_LIGHT_OWNER_AMBIENT] = scene_root_object_get();
    build.owners[SCENE_LIGHT_OWNER_FIRST_PERSON] =
        scene_runtime_camera_bind(&context->views[2]->cam_world);
    if (build.owners[0] == NULL || build.owners[1] == NULL ||
        build.owners[3] == NULL || !scene_runtime_cameras_sync()) {
        engine_scene_shutdown();
        return 0;
    }
    ops.user = &build;
    ops.rgb_read = engine_scene_rgb_read;
    ops.owner_get = engine_scene_owner_get;
    ops.light_create = engine_scene_light_create;
    ops.light_parameter_set = engine_scene_parameter_set;
    ops.light_position_set = engine_scene_position_set;
    ops.light_pose_set = engine_scene_pose_set;
    ops.light_enable = engine_scene_light_enable;
    ops.scene_root_get = engine_scene_root_get;
    ops.scene_object_allocate = engine_scene_object_allocate;
    ops.scene_object_transform_init = engine_scene_transform_init;
    if (!scene_lights_build_from_config(&ops) || build.failed) {
        engine_scene_shutdown();
        return 0;
    }
    if (!engine_placement_preview_initialize()) {
        engine_scene_shutdown();
        return 0;
    }
    return 1;
}
#endif
static EngineKeepRunningHook s_keep_running_hook;
static void* s_keep_running_user;
EngineWorldArrowCaptureHook s_world_arrow_capture_hook;
void* s_world_arrow_capture_context;
void engine_world_arrow_capture_hook_set(EngineWorldArrowCaptureHook hook,
                                         void* context)
{
    s_world_arrow_capture_hook = hook;
    s_world_arrow_capture_context = context;
}
EngineTutorialDispatchHook s_tutorial_dispatch_hook;
void* s_tutorial_dispatch_context;

void engine_tutorial_dispatch_hook_set(EngineTutorialDispatchHook hook,
                                       void* context)
{
    s_tutorial_dispatch_hook = hook;
    s_tutorial_dispatch_context = context;
}
unported_ledger_stubs::GlobalSubsystemResetState*
    s_global_subsystem_reset_state;

void unported_ledger_stubs::global_subsystem_reset_state_bind(
    unported_ledger_stubs::GlobalSubsystemResetState* state)
{
    s_global_subsystem_reset_state = state;
}

uint32_t* engine_current_feature_word(void)
{
    return s_engine_current_context != NULL
        ? &s_engine_current_context->feature : NULL;
}

uint32_t* engine_shared_state_word(void)
{
    return s_engine_current_context != NULL
        ? &s_engine_current_context->state : &s_engine_bootstrap_state;
}

void engine_current_context_bind(EngineContext* ctx)
{
    if (ctx == s_engine_current_context) return;
    if (ctx != NULL && s_engine_current_context == NULL) {
        ctx->state |= s_engine_bootstrap_state;
    }
    s_engine_bootstrap_state = 0;
    s_engine_current_context = ctx;
}

uint32_t* engine_current_state_word(void)
{
    return s_engine_current_context != NULL
        ? &s_engine_current_context->state : NULL;
}

ViewContext* engine_current_primary_view_get(void)
{
    return s_engine_current_context != NULL
        ? s_engine_current_context->views[0] : NULL;
}

/* The reference's initialization callback clears this state before storing
 * the executable basename at DAT_005560c4. The portable modules do not share
 * a byte-identical 0x3be-dword global block, so reset of their owned state is
 * supplied by an explicit hook instead of clearing unrelated module globals. */
// GLOBAL: LEGORR 0x005560c4
const char* s_engine_config_root;
EngineContext* s_callback_registry_context;
EngineConfigRegistryResetHook s_callback_registry_reset_hook;
EngineGameConfigInitHook s_game_config_init_hook;
void* s_callback_registry_user;

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
uint32_t s_window_mode_flag;

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

/* Preserve the shared owner in the portable context-shaped adapter too.
 * Retail FUN_0041fb40 has no arguments; authored zero is a valid scale. */
float EngineTimeScale(EngineContext* ctx)
{
    (void)ctx;
    return EngineTimeScale();
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
#if defined(LEGORRR_BACKEND_WINAPI) && defined(_WIN32) && !defined(_WIN64)
    actor_scene_require_stopped();
    building_scene_require_stopped();
#endif
    if (ctx == NULL) {
        return;
    }

    if (ctx->app_mode != ENGINE_APP_LOADER) {
#if defined(LEGORRR_BACKEND_WINAPI) && defined(_WIN32) && !defined(_WIN64)
        engine_scene_shutdown();
#endif
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
#if defined(LEGORRR_BACKEND_WINAPI) && defined(_WIN32) && !defined(_WIN64)
    engine_scene_shutdown();
#endif
    lego::scene_resource_slots_release_all();
    lego::scene_resource_string_tables_free();
}

// GLOBAL: LEGORR 0x0055651c
uint32_t s_triple_first;  /* DAT_0055651c role: unowned */
// GLOBAL: LEGORR 0x00556520
uint32_t s_triple_second; /* DAT_00556520 role: unowned */
// GLOBAL: LEGORR 0x00556524
uint32_t s_triple_third;  /* DAT_00556524 role: unowned */
// GLOBAL: LEGORR 0x00556f24
float s_task_text_origin_x;
// GLOBAL: LEGORR 0x00556f28
float s_task_text_origin_y;

void engine_context_mode_global_set(int32_t mode)
{
    s_context_mode = mode;
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
float s_score_text_origin_x;
// GLOBAL: LEGORR 0x00556f20
float s_score_text_origin_y;

uint32_t engine_debug_word_peek(void)
{
    return s_panel_highlight_flags;
}

void engine_debug_word_bit_set(uint32_t mask, int enabled)
{
    if (enabled != 0) s_panel_highlight_flags |= mask;
    else s_panel_highlight_flags &= ~mask;
}

#if !defined(LEGORRR_BACKEND_WINAPI) /* native backend and tests only (#576) */
void objective_score_text_origin_peek(float out[2])
{
    if (out != NULL) {
        out[0] = s_score_text_origin_x;
        out[1] = s_score_text_origin_y;
    }
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

int global_scroll_guard_peek(void)
{
    return s_scroll_guard;
}

void objective_task_text_origin_peek(float out[2])
{
    if (out != NULL) {
        out[0] = s_task_text_origin_x;
        out[1] = s_task_text_origin_y;
    }
}

void engine_app_mode_global_set(int32_t mode)
{
    s_application_mode = mode;
}
#endif /* !LEGORRR_BACKEND_WINAPI */

/* --- the grid-cell record buffer (see engine.hpp) ---------------------------- */
// GLOBAL: LEGORR 0x005520e8
void*        s_field_grid_buf;   /* DAT_005520e8 role */
// GLOBAL: LEGORR 0x005520ec
EngineState* s_field_grid_desc;  /* DAT_005520ec role */

void* entity_field_grid_buffer_peek(void) { return s_field_grid_buf; }

EngineState* entity_field_grid_descriptor_peek(void) { return s_field_grid_desc; }

/* --- the global-state refresh leaf (see engine.hpp) ------------------------
 * FUN_00435a20's two corpus words (state word, refresh record pointer) have
 * no ported owner; they land here as zero-init statics with hooks. */
// GLOBAL: LEGORR 0x00556ed4
uint32_t s_global_low_word; /* DAT_00556ed4 role */
// GLOBAL: LEGORR 0x005563b4
void* s_refresh_record;     /* DAT_005563b4 role */

uint32_t global_state_low_word_get(void) { return s_global_low_word; }
void global_state_low_word_set(uint32_t value) { s_global_low_word = value; }
void  global_refresh_record_set(void* record) { s_refresh_record = record; }
void* global_refresh_record_peek(void) { return s_refresh_record; }

/* --- the global position handle leaf (see engine.hpp) ----------------------
 * DAT_00556f3c has no ported owner; the reposition seam (FUN_0047dcb0) is
 * a hook boundary, so the call rides a recorded seam here. */
// GLOBAL: LEGORR 0x00556f3c
void* s_position_handle;   /* DAT_00556f3c role */
int    s_position_seam_calls;
void*  s_position_seam_last_handle;
int    s_position_seam_last_x;
int    s_position_seam_last_y;

#if !defined(LEGORRR_BACKEND_WINAPI) /* native backend and tests only (#576) */
void global_position_handle_set(void* handle) { s_position_handle = handle; }
#endif /* !LEGORRR_BACKEND_WINAPI */
void* global_position_handle_peek(void) { return s_position_handle; }
#if !defined(LEGORRR_BACKEND_WINAPI) /* native backend and tests only (#576) */
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
#endif /* !LEGORRR_BACKEND_WINAPI */

/* ScrollScaleSetAndClamp (FUN_00425ce0) — see engine.hpp. The corpus reads
 * its speed word through the overlapping _DAT_0055615c alias; the port keeps
 * one float in this module's storage. */
// GLOBAL: LEGORR 0x0055615c
float s_scroll_scale = 1.0f; /* DAT_0055615c; port default (ref is zero-filled) */

float scroll_scale_peek(void) { return s_scroll_scale; }
#if !defined(LEGORRR_BACKEND_WINAPI) /* native backend and tests only (#576) */
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
#endif /* !LEGORRR_BACKEND_WINAPI */

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

namespace unported_ledger_stubs {
EngineTaskCallbackRecord* ledger_stub_00480870_TaskCallbackRecordAllocateAndInitialize(
    void* owner, void* callback, uint32_t mode, void* context,
    uint32_t flag_value)
{
    return engine_task_callback_record_allocate_and_initialize(
        owner, callback, mode, context, flag_value);
}


} // namespace unported_ledger_stubs

/* ResetLevelAndExit (004250a0) in the original ABI, for Reward.c (GAME-037,
 * finding 560); a thin unannotated adapter (ADR 0005 step 6). */
#include "game/lego_api.h"

void lego_reset_level_and_exit(void)
{
    engine_reset_level_and_exit();
}

/* Lego_GotoInfo (0042de40) in the original ABI, for Info.c (GAME-019,
 * finding 567): move the view to a message's object or block. A thin
 * unannotated adapter (ADR 0005 step 6). The port's model at that address
 * (ledger_stub_0042de40_ViewportTargetPositionClamp) takes a clamp state,
 * not an object, and the port has no camera goto, so this does nothing. */
#include "game/info_api.h"

void lego_goto_info(void* object, const int* blockPos, int force)
{
    (void)object;
    (void)blockPos;
    (void)force;
}
