// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-037 GAME-037: the unit's functions in original address order and the
 * state they own, relocated verbatim by tools/relocate_unit.py (ADR 0006).
 * Body hashes: docs/coverage/relocations/GAME-037.csv. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gods98/block.hpp"
#include "gods98/heap.hpp"
#include "gods98/message.hpp"
#include "gods98/string.hpp"
#include "crt/crt.hpp"
#include "entity.hpp"
#include "level.hpp"
#include "mission.hpp"
#include "select.hpp"
#include "frontend.hpp"
#include "objective.hpp"
#include "savegame.hpp"
#include "ui_list.hpp"
#include "effects.hpp"
#include "../gods98/sound.hpp"
#include "../gods98/texture.hpp"
#include "../gods98/surface.hpp"
#include "util.hpp"
#include "../gods98/input.hpp"
#include "../gods98/frameloop.hpp"
#include "../platform/platform.h"
#include "mission_shared.hpp"
#include <math.h>
#include <float.h>
#include <ctype.h>
#include <errno.h>
#include "cmd.hpp"
#include "ai.hpp"
#include "spawn.hpp"
#include "terrain_grid.hpp"
#include "gods98/engine.hpp"
#include "gods98/view.hpp"
#include "gods98/media.hpp"
#include "port_loop_ports.hpp"
#include <stdint.h>
#include "building_scene.hpp"
#include "actor_scene.hpp"
#include "type_config_ports.hpp"
#include "entity_pose_delta.hpp"
#include "entity_motion_ports.hpp"
#include "activity_timeout.hpp"
#include "entity_motion_probe.hpp"
#include "simulation_ui_ports.hpp"
#include "type_motion_scale.hpp"
#include "entity_quad_collision.hpp"
#include "weapon.hpp"
#include "gods98/model.hpp"
#include "gods98/resource.hpp"
#include "gods98/record.hpp"
#include "upgrades.hpp"
#include <stdarg.h>
#include <time.h>
#include "frontend.h"
#include "terrain_feedback.hpp"
#include "gods98/audio.hpp"
#include "gods98/simtick.hpp"
#include "gods98/flic.hpp"
#include "priority_config_ports.hpp"
#include "tutorial_hud_ports.hpp"
#include "ui_action_dispatch.hpp"
#include "camera_drag.hpp"
#include "screen_pick.hpp"
#include "pointer_cursor.hpp"
#include "gods98/scene_resources.hpp"
#include "game.hpp"
#include "options.hpp"
#include "game/render.hpp"
#include "game/tutorial_world_arrows.hpp"
#include "game/scene_render.hpp"
#include "movie.hpp"
#include "tickers.hpp"
#include "camera.hpp"
#include "ptrstack.hpp"
#include <limits.h>
#include "mission.h"
#include "mission_for_reward.hpp"
#include "crt/crt_for_reward.hpp"
#include "entity_for_reward.hpp"
#include "frontend_for_reward.hpp"
#include "objective_for_reward.hpp"
#include "savegame_for_reward.hpp"

/* The retail routine reads and writes fixed dword offsets in its temporary
 * checkpoint allocation. Use direct typed accesses inside this hot target
 * body; the memcpy helpers below remain for callers that need unaligned or
 * format-independent access. */
#define mission_blob_float_get_at(blob, offset) \
    (*(float*)((blob) + (offset)))
#define mission_blob_float_set_at(blob, offset, value) \
    (*(float*)((blob) + (offset)) = (value))
#define mission_blob_word_get_at(blob, offset) \
    (*(uint32_t*)((blob) + (offset)))
#define mission_blob_word_set_at(blob, offset, value) \
    (*(uint32_t*)((blob) + (offset)) = (value))

/* Reward/Timer default when the key is absent (finding 08): 76.9375. */
#define LVL_REWARD_TIMER_DEFAULT 76.9375f

/* The two add-one helpers share the corpus's .rdata subtrahend: both
 * 0x462836 and 0x462856 fsub dword ptr [0x0049e6bc], whose bytes are
 * verified 00 00 80 bf (-1.0f). A const static keeps the operand a
 * memory reference Reccmp resolves through the annotation (the
 * audio.cpp s_menu_volume_scale pattern). */
static const float s_minus_one_0049e6bc = -1.0f;

static const char s_task_name_score[] = "Score";
static const char s_task_name_timer[] = "Timer";
static const char s_task_name_oxygen[] = "Oxygen";
static const char s_task_name_rock_monsters[] = "RockMonsters";
static const char s_task_name_figures[] = "Figures";
static const char s_task_name_caverns[] = "Caverns";
static const char s_task_name_constructions[] = "Constructions";
static const char s_task_name_diggable[] = "Diggable";
static const char s_task_name_ore[] = "Ore";

static void reward_task_flics_close_all(RewardScreenState* state);

namespace unported_ledger_stubs {
void ledger_stub_00460290_EntityType4IndexListBuild(
    EntityType4IndexNameList* output);

extern "C" int ledger_stub_00435550_GlobalStateZeroTransitionDetect(void);
} // namespace unported_ledger_stubs

/* Defined below (forward declarations added by tools/relocate_unit.py). */
static void task_loader_boximages(BlockFile* cfg, const char* root, const char* level, GameTask* t);
static void task_loader_flics(BlockFile* cfg, const char* root, const char* level, GameTask* t);
static void task_loader_font(BlockFile* cfg, const char* root, const char* level, GameTask* t);
static void task_loader_images(GameTask* t);
static void task_loader_sound(BlockFile* cfg, const char* root, const char* level, GameTask* t);
static void task_loader_text(BlockFile* cfg, const char* root, const char* level, GameTask* t);
static const char* rw_str(BlockFile* cfg, const char* root, const char* level, const char* sub, const char* name);
static void task_put_str(GameTask* t, int off, const char* s, int cap);
static unsigned int task_slot_index(const GameTask* t);
static void task_add_flag(GameTask* t, uint32_t bits);
static int task_fields(const char* token, char out[UTIL_QUERY_MAX_FIELDS][256]);
static void mission_reward_button_config_load(BlockFile* cfg, const char* root, const char* level_name, const char* key, TextureSurface* images[4], float* x, float* y, float* width, float* height);
static void mission_reward_state_bind_for_level(GameContext* ctx);
static int rw_typed(BlockFile* cfg, const char* root, const char* level, const char* name);
static void checkpoint_reward_group(GameContext* ctx, const char* level_name);
namespace unported_ledger_stubs { int ledger_stub_00460310_EntityType4IndexListAppend( EntitySlot* entity, void* payload); }
static GameContext* mission_reward_game_get(void);
static void blob_count_up(char* blob, size_t guard_off, size_t add_off);
static float blob_float_get(const char* blob, size_t off);
static void blob_float_put(char* blob, size_t off, float v);
static float blob_percent_fold_store(char* blob, size_t den, size_t num, size_t out);
static int reward_endgame_avi1_configured(const GameContext* game);
static int reward_screen_entry_check(GameContext* game, const RewardScreenOps* ops);
static int reward_screen_fast_forward_get(const RewardScreenState* state, const RewardScreenOps* ops);
static void reward_screen_task_visual_draw(RewardScreenState* state, const RewardScreenOps* ops, int index, uint32_t mask);
static int reward_screen_zone_check(RewardScreenState* state, const RewardScreenOps* ops);
static void reward_task_image_draw_fallback(GameTask* task, int index, uint32_t mask);
static int reward_task_record_index(const RewardScreenState* state, const GameTask* task, int fallback);
static void reward_task_text_draw_fallback(GameTask* task, int index, uint32_t displayed_count);
static int zone_hit(float x, float y, float w, float h, int mx, int my);
static void reward_screen_task_timer_tick(RewardScreenState* state);
static uint32_t mission_blob_word_get(const uint8_t* blob, size_t offset);

int mission_task_init_target(void)
{
    GameContext* ctx = s_task_init_binding.game;
    BlockFile* cfg = s_task_init_binding.config;
    const char* level_name = s_task_init_binding.level_name;
    if (ctx == NULL) {
        return 0;
    }
    const char* root = cfg != NULL ? util_root_name(cfg) : "Lego*";
    GameTask* t;
    const char* names[GAME_TASK_COUNT];
    int i;
    /* TaskBlockInit clears its task block before the corpus's name pass. The
     * port keeps the same operation scoped to the context-owned task array. */
    reward_task_block_finish();
    memset(ctx->tasks, 0, sizeof(ctx->tasks));
    names[0] = s_task_name_crystals;
    names[1] = s_task_name_ore;
    names[2] = s_task_name_diggable;
    names[3] = s_task_name_constructions;
    names[4] = s_task_name_caverns;
    names[5] = s_task_name_figures;
    names[6] = s_task_name_rock_monsters;
    names[7] = s_task_name_oxygen;
    names[8] = s_task_name_timer;
    names[9] = s_task_name_score;
    for (i = 0, t = ctx->tasks; i < GAME_TASK_COUNT; ++i, ++t) {
        sprintf((char*)t->data + 4, s_task_copy_format, names[i]);
    }
    if (cfg == NULL || level_name == NULL) {
        return 1;
    }
    /* TaskBlockInit makes one pass per loader (Images, Text, Flics, Fonts,
     * Sounds, BoxImages). Keep that corpus order while targeting ctx's tasks;
     * each pass writes independent task fields, so the resulting records are
     * unchanged from the former per-task grouping. */
    for (t = ctx->tasks; t < ctx->tasks + GAME_TASK_COUNT; ++t) {
        task_loader_images(t);
    }
    for (t = ctx->tasks; t < ctx->tasks + GAME_TASK_COUNT; ++t) {
        task_loader_text(cfg, root, level_name, t);
    }
    for (t = ctx->tasks; t < ctx->tasks + GAME_TASK_COUNT; ++t) {
        task_loader_flics(cfg, root, level_name, t);
    }
    for (t = ctx->tasks; t < ctx->tasks + GAME_TASK_COUNT; ++t) {
        task_loader_font(cfg, root, level_name, t);
    }
    for (t = ctx->tasks; t < ctx->tasks + GAME_TASK_COUNT; ++t) {
        task_loader_sound(cfg, root, level_name, t);
    }
    for (t = ctx->tasks; t < ctx->tasks + GAME_TASK_COUNT; ++t) {
        task_loader_boximages(cfg, root, level_name, t);
    }
    return 1;
}
/* FUN_0045f1f0 — `Reward Sounds.<name>` at +0x208. */
static void task_loader_sound(BlockFile* cfg, const char* root,
                              const char* level, GameTask* t)
{
    const char* v = rw_str(cfg, root, level, "Sounds", task_name(t));
    task_put_str(t, TASK_OFF_SOUND, v, 64);
}
/* FUN_0045f230 — `Reward Fonts.<name>` at +0x204. The field stores the
 * created atlas handle (the default Reward.Font atlas when no task override
 * is configured), while the widened host pointer is retained in the task
 * resource sidecar for safe release. */
static void task_loader_font(BlockFile* cfg, const char* root,
                             const char* level, GameTask* t)
{
    const char* v = rw_str(cfg, root, level, "Fonts", task_name(t));
    const char* default_font = rw_str(cfg, root, level, NULL, "Font");
    void* record;
    unsigned int slot;
    if (v == NULL || v[0] == '\0') v = default_font;
    record = (v != NULL && v[0] != '\0') ? font_atlas_record_create(v) : NULL;
    if (record == NULL && default_font != NULL && default_font[0] != '\0' &&
        (v == NULL || strcmp(default_font, v) != 0)) {
        record = font_atlas_record_create(default_font);
    }
    task_put_int(t, TASK_OFF_FONT,
                 (int32_t)(uint32_t)(uintptr_t)record);
    slot = task_slot_index(t);
    if (slot < GAME_TASK_COUNT)
        reward_task_font_record_set(slot, record);
}

/* FUN_0045f290 — `Reward BoxImages.<name>`: coords +0x59/0x5a, |0x40. */
static void task_loader_boximages(BlockFile* cfg, const char* root,
                                  const char* level, GameTask* t)
{
    const char* name = task_name(t);
    const char* v = rw_str(cfg, root, level, "BoxImages", name);
    char fallback_name[64];
    if (v == NULL) {
        if ((engine_feature_word_peek() & 0x2000000u) != 0u) return;
        size_t name_length = strlen(name);
        if (name_length > sizeof(fallback_name) - 2u)
            name_length = sizeof(fallback_name) - 2u;
        fallback_name[0] = '!';
        memcpy(fallback_name + 1, name, name_length);
        fallback_name[name_length + 1u] = '\0';
        v = rw_str(cfg, root, level, "Images", fallback_name);
        if (v == NULL) return;
    }
    char f[UTIL_QUERY_MAX_FIELDS][256];
    if (task_fields(v, f) < 3) {
        return;
    }
    {
        TextureSurface* image = image_surface_load(f[0], 0u, 0u);
        if (image == NULL) return;
        texture_source_color_key_set(image, 0u, 0u);
        image->resource_flags |= 2u;
        task_put_int(t, TASK_OFF_BOXIMG_XY, (int32_t)atoi(f[1]));
        task_put_int(t, TASK_OFF_BOXIMG_XY + 4, (int32_t)atoi(f[2]));
        task_put_int(t, TASK_OFF_BOX_IMAGE,
                     (int32_t)(uint32_t)(uintptr_t)image);
        reward_task_box_image_surface_set(task_slot_index(t), image);
    }
    task_add_flag(t, TASK_FLAG_BOXIMG);
}
/* TaskLoaderImages @ 0x0045f3e0 takes only the task record; the config,
 * root, and level belong to the active TaskBlockInit binding. */
static void task_loader_images(GameTask* t)
{
    BlockFile* cfg = s_task_init_binding.config;
    const char* level = s_task_init_binding.level_name;
    const char* root;
    const char* name;
    const char* v;
    if (cfg == NULL || level == NULL || t == NULL) return;
    root = util_root_name(cfg);
    name = task_name(t);
    v = rw_str(cfg, root, level, "Images", name);
    if (v == NULL && (engine_feature_word_peek() & 0x2000000u) == 0u) {
        char fallback_name[64];
        size_t name_length = strlen(name);
        if (name_length > sizeof(fallback_name) - 2u)
            name_length = sizeof(fallback_name) - 2u;
        fallback_name[0] = '!';
        memcpy(fallback_name + 1, name, name_length);
        fallback_name[name_length + 1u] = '\0';
        v = rw_str(cfg, root, level, "Images", fallback_name);
    }
    if (v == NULL) {
        return;
    }
    /* ResourceRecordParse (FUN_00477440) may read the tuple from a resource
     * file as well as from an inline config value. task_fields preserves that
     * lookup order through RESOURCE_AUTO before tokenizing the pipe tuple. */
    char fields[UTIL_QUERY_MAX_FIELDS][256];
    if (task_fields(v, fields) < 3 || fields[0][0] == '\0') return;
    {
        char image_path[128];
        size_t path_length = strlen(fields[0]);
        if (path_length >= sizeof(image_path)) return;
        memcpy(image_path, fields[0], path_length + 1u);
        task_put_int(t, TASK_OFF_IMG_XY, (int32_t)atoi(fields[1]));
        task_put_int(t, TASK_OFF_IMG_XY + 4, (int32_t)atoi(fields[2]));
        {
            TextureSurface* image = image_surface_load(image_path, 0u, 0u);
            if (image != NULL) {
                texture_source_color_key_set(image, 0u, 0u);
                image->resource_flags |= 2u;
                task_put_int(t, TASK_OFF_IMAGE,
                             (int32_t)(uint32_t)(uintptr_t)image);
                task_add_flag(t, TASK_FLAG_IMG);
                reward_task_resource_slot_set(task_slot_index(t), TASK_FLAG_IMG,
                                              image, NULL);
            }
        }
    }
}

/* FUN_0045f530 — `Reward Text.<name>` (key string lives at corpus
 * DAT_004a26b0 = "Text"): field 0 -> display text at +0x54 with '_'
 * mapped to ' ' over the first 0x20 bytes (corpus FUN_00413f00),
 * fields 1/2 -> text-draw x/y at +0x154/+0x158, flag |= 0x31. */
static void task_loader_text(BlockFile* cfg, const char* root,
                             const char* level, GameTask* t)
{
    const char* v = rw_str(cfg, root, level, "Text", task_name(t));
    if (v == NULL) {
        return;
    }
    char f[UTIL_QUERY_MAX_FIELDS][256];
    if (task_fields(v, f) == 0) {
        return;
    }
    task_put_str(t, TASK_OFF_TEXT, f[0], 0x100);
    mission_replace_char((char*)t->data + TASK_OFF_TEXT, '_', ' ');
    task_put_int(t, TASK_OFF_TEXT_XY, (int32_t)atoi(f[1]));
    task_put_int(t, TASK_OFF_TEXT_XY + 4, (int32_t)atoi(f[2]));
    task_add_flag(t, TASK_FLAG_MASK);
}

/* FUN_0045f5f0 — `Reward Flics.<name>`: path +0x5b, rect +0x7b..0x7e, |4. */
static void task_loader_flics(BlockFile* cfg, const char* root,
                              const char* level, GameTask* t)
{
    const char* name = task_name(t);
    const char* v = rw_str(cfg, root, level, "Flics", name);
    char fallback_name[64];
    if (v == NULL && (engine_feature_word_peek() & 0x1000000u) == 0u) {
        size_t name_length = strlen(name);
        if (name_length > sizeof(fallback_name) - 2u)
            name_length = sizeof(fallback_name) - 2u;
        fallback_name[0] = '!';
        memcpy(fallback_name + 1, name, name_length);
        fallback_name[name_length + 1u] = '\0';
        v = rw_str(cfg, root, level, "Flics", fallback_name);
    }
    if (v == NULL) {
        return;
    }
    char f[UTIL_QUERY_MAX_FIELDS][256];
    if (task_fields(v, f) == 0) {
        return;
    }
    task_put_str(t, TASK_OFF_FLICPATH2, f[0], 128);
    task_put_int(t, TASK_OFF_FLIC_RECT + 8, (int32_t)atoi(f[1]));
    task_put_int(t, TASK_OFF_FLIC_RECT + 12, (int32_t)atoi(f[2]));
    task_put_int(t, TASK_OFF_FLIC_RECT, (int32_t)atoi(f[3]));
    task_put_int(t, TASK_OFF_FLIC_RECT + 4, (int32_t)atoi(f[4]));
    task_add_flag(t, TASK_FLAG_FLIC);
}

/* TaskBlockFinish @ 0x0045f750: the ten 0x210-byte task records select
 * image release with flag bit 1 and font release with bit 0. */
void reward_task_block_finish(void)
{
    unsigned int i;
    for (i = 0; i < 10u; ++i) {
        RewardTaskResourceRecord* task = &s_reward_task_resources[i];
        if ((task->flags & 2u) != 0u) reward_task_flush_images(task);
        if ((task->flags & 1u) != 0u) reward_task_flush_font(task);
        if (task->box_image_surface != NULL)
            texture_release((TextureSurface*)task->box_image_surface);
        task->flags = 0;
        task->image_surface = NULL;
        task->font_record = NULL;
        task->box_image_surface = NULL;
    }
}
/* TaskFlushImages @ 0x0045f7b0: release the task's owned image surface. */
void reward_task_flush_images(RewardTaskResourceRecord* task)
{
    if (task != NULL) {
        texture_release((TextureSurface*)task->image_surface);
    }
}

/* TaskFlushFont @ 0x0045f7d0: release the task's font-atlas record. */
void reward_task_flush_font(RewardTaskResourceRecord* task)
{
    if (task != NULL) {
        font_atlas_record_release(task->font_record);
    }
}

int mission_reward_init(GameContext* ctx, BlockFile* cfg, const char* level_name)
{
    if (ctx == NULL) {
        return 0;
    }
    RewardScreen* rs = &ctx->reward_screen;
    /* A newly loaded level starts outside the prior level's reward overlay.
     * The retail RewardInit replaces its global presentation state here. */
    ctx->reward_active = 0;
    mission_reward_resources_release(ctx);
    /* The source RewardInit owns process-global font handles. If the active
     * portable session changed, retire the previous session's atlas records
     * before replacing those global slots. */
    if (s_reward_font_owner != NULL && s_reward_font_owner != ctx)
        mission_reward_font_records_release();
    __builtin_memset(rs, 0, sizeof(*rs));
    s_reward_success_words_valid = 0;
    memset(s_reward_success_words, 0, sizeof(s_reward_success_words));
    /* The reference reward ABI reads process-global state. */
    mission_reward_state_bind_for_level(ctx);
    if (cfg == NULL || level_name == NULL) {
        return 1;
    }
    const char* root = util_root_name(cfg);
    /* RewardInit @ 0x45f7f0 keys (corpus engine-region.c 6485-6557). */
    const char* v;
    if ((v = rw_str(cfg, root, level_name, NULL, "Wallpaper")) != NULL) {
        snprintf(rs->wallpaper, sizeof(rs->wallpaper), "%s", v);
    }
    rs->display = rw_typed(cfg, root, level_name, "Display");
    if (rs->display == 2) rs->display = 0;
    rs->centre_text = rw_typed(cfg, root, level_name, "CentreText");
    if (rs->centre_text == 2) rs->centre_text = 0;
    v = rw_str(cfg, root, level_name, NULL, "ScrollSpeed");
    rs->scroll_speed = (float)atof(v != NULL ? v : "");
    v = rw_str(cfg, root, level_name, NULL, "VertSpacing");
    rs->vert_spacing = (float)atof(v != NULL ? v : "");
    if ((v = rw_str(cfg, root, level_name, NULL, "Font")) != NULL) {
        snprintf(rs->font_base, sizeof(rs->font_base), "%s", v);
    }
    if ((v = rw_str(cfg, root, level_name, NULL, "TitleFont")) != NULL) {
        snprintf(rs->font_title, sizeof(rs->font_title), "%s", v);
    }
    if ((v = rw_str(cfg, root, level_name, NULL, "BackFont")) != NULL) {
        snprintf(rs->font_back, sizeof(rs->font_back), "%s", v);
    }
    /* RewardInit creates the three render records after resolving all font
     * names and before initializing either button zone. */
    s_reward_font_base_record = rs->font_base[0] != '\0'
        ? font_atlas_record_create(rs->font_base) : NULL;
    s_reward_font_title_record = rs->font_title[0] != '\0'
        ? font_atlas_record_create(rs->font_title) : NULL;
    s_reward_font_back_record = rs->font_back[0] != '\0'
        ? font_atlas_record_create(rs->font_back) : NULL;
    s_reward_font_owner = ctx;
    /* RewardSaveButtonZonesInit (0x45faf0) runs before the text strings and
     * text-position query in the reference routine. */
    mission_reward_button_config_load(cfg, root, level_name,
        "SaveButton", rs->save_button_images, &rs->save_x, &rs->save_y,
        &rs->save_w, &rs->save_h);
    mission_reward_button_config_load(cfg, root, level_name,
        "AdvanceButton", rs->advance_button_images, &rs->advance_x,
        &rs->advance_y, &rs->advance_w, &rs->advance_h);
    if ((v = rw_str(cfg, root, level_name, NULL, "CompleteText")) != NULL) {
        snprintf(rs->complete_text, sizeof(rs->complete_text), "%s", v);
    }
    if ((v = rw_str(cfg, root, level_name, NULL, "FailedText")) != NULL) {
        snprintf(rs->failed_text, sizeof(rs->failed_text), "%s", v);
    }
    if ((v = rw_str(cfg, root, level_name, NULL, "QuitText")) != NULL) {
        snprintf(rs->quit_text, sizeof(rs->quit_text), "%s", v);
    }
    if ((v = rw_str(cfg, root, level_name, NULL, "TextPos")) != NULL) {
        char f[UTIL_QUERY_MAX_FIELDS][256];
        if (task_fields(v, f) != 0) {
            rs->text_pos[0] = atoi(f[0]);
            rs->text_pos[1] = atoi(f[1]);
        }
    }
    return 1;
}

void checkpoint_blob_build(GameContext* ctx)
{
    if (ctx == NULL) {
        return;
    }
    uint8_t* blob = (uint8_t*)engine_heap_alloc(SAVE_BLOB_SIZE);
    if (blob == NULL) {
        return;
    }
    memset(blob, 0, SAVE_BLOB_SIZE);
    const char* level_name = ctx->level != NULL
                             ? (const char*)ctx->level->ptr_name : NULL;
    checkpoint_reward_group(ctx, level_name);

    int32_t enable = (int32_t)ctx->reward.enable;
    int32_t mod = (int32_t)ctx->reward.modifier;
    int32_t ten = GAME_TASK_COUNT;
    __builtin_memcpy(blob + 0x00, &enable, 4);          /* Reward/Enable   */
    __builtin_memcpy(blob + 0x1C0, &mod, 4);            /* Reward/Modifier  */
    __builtin_memcpy(blob + 0x1CE4, &ten, 4);           /* task count       */
    uint32_t self = (uint32_t)(uintptr_t)(blob + 0x1CECu);
    __builtin_memcpy(blob + 0x1CE8, &self, 4);          /* task-table ptr   */
    /* Copy the ten task records (stride 0x210) verbatim. */
    __builtin_memcpy(blob + 0x1CEC, ctx->tasks, GAME_TASK_COUNT * GAME_TASK_STRIDE);
    mission_task_snapshot_refresh(ctx, (GameTask*)(blob + 0x1CEC));

    /* Tutorial hard-code (CheckpointBlobBuild tail, corpus 0x460066-0x46008c):
     * when the level name matches "Levels::Level03", record_base + 0x868 —
     * i.e. task slot 4 ("Caverns") quota, record_base + 4*0x210 + 0x28 — is
     * forced to 1 as a dword write. */
    const char* base = level_name != NULL ? strrchr(level_name, ':') : NULL;
    if (base == NULL && level_name != NULL) {
        base = strrchr(level_name, '\\');
    }
    if (base != NULL) {
        ++base;
    } else {
        base = level_name;
    }
    /* FUN_0048ff10 is a case-folded strcmp, so match "Level03" the same way. */
    static const char LVL03[] = "Level03";
    int l03 = base != NULL && strcasecmp_lite(base, LVL03) == 0;
    if (l03) {
        int32_t one = 1;
        __builtin_memcpy(blob + 0x1CEC + 0x868, &one, 4);
    }

    engine_heap_free(ctx->checkpoint_blob);
    ctx->checkpoint_blob = blob;
}

/* TaskImportanceRefresh @ 0x4600a0 takes the task record; its config
 * context is the currently bound mission owner. */
void mission_task_importance_refresh(GameTask* t)
{
    MissionTaskInitBinding* binding = &s_task_init_binding;
    char path[512];
    const char* value;
    float imp;
    rw_path(path, sizeof(path), binding->config, binding->level_name,
            "Importance", task_name(t));
    value = path[0] != '\0' ? block_get_strptr(binding->config, path) : NULL;
    imp = (float)atof(value != NULL ? value : "");
    __builtin_memcpy(t->data + TASK_OFF_IMPORTANCE, &imp, 4);
}

/* TaskQuotaRefresh @ 0x460140: load one task's Reward Quota value into
 * record +0x28 from the currently bound mission config. */
void mission_task_quota_refresh(GameTask* t)
{
    MissionTaskInitBinding* binding = &s_task_init_binding;
    char path[512];
    const char* value;
    int32_t quota;
    rw_path(path, sizeof(path), binding->config, binding->level_name,
            "Quota", task_name(t));
    value = path[0] != '\0' ? block_get_strptr(binding->config, path) : NULL;
    quota = (int32_t)atoi(value != NULL ? value : "");
    __builtin_memcpy(t->data + TASK_OFF_QUOTA, &quota, 4);
}

/* CheckpointBlobFree (FUN_004601e0, finding 08): frees the blob. The
 * corpus frees its global and LEAVES the dangling pointer; the port
 * nulls it (documented deviation — port-owned storage). */
void checkpoint_blob_free(GameContext* ctx)
{
    if (ctx != NULL) {
        engine_heap_free(ctx->checkpoint_blob);
        ctx->checkpoint_blob = NULL;
    }
}

void* checkpoint_blob_get(const GameContext* ctx)
{
    return ctx != NULL ? ctx->checkpoint_blob : NULL;
}

#if !defined(LEGORRR_BACKEND_WINAPI) /* native backend and tests only (#576) */
/* SelectedLevelStartObjectUpdate (FUN_00460200) — see savegame.hpp. */
void selected_level_start_object_update(GameContext* ctx)
{
    if (ctx == NULL || ctx->level == NULL) {
        return; /* corpus: no level bound, nothing reads */
    }
    (void)checkpoint_blob_get(ctx); /* the corpus keeps no result */
}
#endif /* !LEGORRR_BACKEND_WINAPI */

namespace unported_ledger_stubs {
void ledger_stub_00460290_EntityType4IndexListBuild(
    EntityType4IndexNameList* output)
{
    EntityType4IndexNameList local;
    uint32_t row;
    uint32_t column;
    memset(&local, 0, sizeof(local));
    entity_pool_find_by_callback_filtered_current(
        ledger_stub_00460310_EntityType4IndexListAppend, &local);
    for (row = 0; row < 15u; ++row) {
        for (column = 0; column < 16u; ++column) {
            local.grid_count += grid_record_count_get(4, (int)row,
                                                      (int)column);
        }
    }
    if (output != NULL) {
        memcpy(output, &local, sizeof(local));
    }
}
int ledger_stub_00460310_EntityType4IndexListAppend(
    EntitySlot* entity, void* payload)
{
    EntityType4IndexNameList* list = (EntityType4IndexNameList*)payload;
    uint32_t kind;
    uint32_t index;
    if (list == NULL || entity == NULL) return 0;
    copy_two_dword_pair_to_outputs(entity, &kind, &index);
    if (kind == 4u && list->count < 100u) {
        list->indices[list->count] = index;
        list->names[list->count] = item_name_by_kind_lookup(4u, (int)index);
        list->count = list->count + 1u;
    }
    return 0;
}
} // namespace unported_ledger_stubs

/* CheckpointBlobTaskProgressRefresh prepares the per-level task snapshot.
 * Map counters stay local to the active GameContext, while the percentage
 * fold and CRT-backed score/stat reads use their existing helpers. The
 * retail DAT_005529c4 type-four list is represented by the selected-level
 * snapshot produced after object-list population. */
int mission_checkpoint_task_progress_refresh_bound(void)
{
    GameContext* ctx = mission_reward_game_get();
    LevelState* level;
    uint8_t* blob;
    int width;
    int height;
    uint32_t cell_count = 0u;
    float classified_a = 0.0f;
    float classified_b = 0.0f;
    float eligible_cells = 0.0f;
    float eligible_non_solid = 0.0f;
    float marked_cells = 0.0f;
    const EntityType4IndexNameList* type4_snapshot;
    uint32_t type4_name_count;
    uint32_t type4_grid_count;
    int x;

    if (ctx == NULL || ctx->level == NULL || ctx->checkpoint_blob == NULL)
        return 0;
    level = ctx->level;
    blob = (uint8_t*)ctx->checkpoint_blob;
    width = level_int(level, LEVEL_OFF_WIDTH);
    height = level_int(level, LEVEL_OFF_HEIGHT);
    if (width <= 0 || height <= 0 || level_voxel(level, 0, 0) == NULL) return 0;

    mission_blob_float_set_at(blob, 0x1d1c, 0.0f);
    mission_blob_float_set_at(blob, 0x1f2c, 0.0f);
    mission_blob_word_set_at(blob, 0x1c8, 0u);
    mission_blob_word_set_at(blob, 0x1cc, 0u);
    mission_blob_word_set_at(blob, 0x1d0, 0u);
    mission_blob_float_set_at(blob, 0x1d14, 0.0f);
    mission_blob_float_set_at(blob, 0x2140, crt_float_00554adc_peek());
    mission_blob_float_set_at(blob, 0x2560, crt_float_00554efc_peek());
    mission_blob_float_set_at(blob, 0x213c, 0.0f);
    mission_blob_float_set_at(blob, 0x1b04, 0.0f);
    mission_blob_float_set_at(blob, 0x1b08, 0.0f);
    {
        const uint32_t pilot_count = (uint32_t)
            resolved_entity_operation_dispatch("Pilot", 0, 0);
        const uint32_t pilot_grid_count = start_level_pilot_grid_count_get();
        mission_blob_word_set_at(blob, 0x2778, pilot_count);
        mission_blob_word_set_at(blob, 0x277c, pilot_grid_count);
        mission_blob_float_set_at(blob, 0x276c, (float)pilot_count);
        mission_blob_float_set_at(blob, 0x2770,
            (float)(pilot_count - pilot_grid_count));
    }
    mission_blob_float_set_at(blob, 0x2784, crt_float_00555120_peek());

    {
        const uint32_t count_a = crt_counter_00555324_peek();
        const uint32_t count_b = crt_counter_00555328_peek();
        const uint32_t count_c = crt_counter_0055532c_peek();
        const uint32_t score_percent_raw = count_a * 100u;
        float score_percent = (float)score_percent_raw;
        const float score_limit = crt_float_00555330_peek();
        if (score_percent > score_limit) score_percent = score_limit;
        mission_blob_word_set_at(blob, 0x2988, count_a);
        mission_blob_word_set_at(blob, 0x298c, count_b);
        mission_blob_word_set_at(blob, 0x2990, count_c);
        mission_blob_float_set_at(blob, 0x2994, score_percent);
        mission_blob_float_set_at(blob, 0x297c, (float)score_percent_raw);
        mission_blob_float_set_at(blob, 0x2980, score_percent);
    }
    {
        const uint32_t score_count = reward_score_count_peek();
        const float score_total = reward_score_total_peek();
        const float denominator = (float)score_count;
        mission_blob_float_set_at(blob, 0x2da0, denominator);
        mission_blob_float_set_at(blob, 0x2d9c, score_total / denominator);
    }
    /* The objective-success path stages these words in the temporary
     * DAT_0055299c block. The reference copies them only while its enable
     * dword is set; otherwise the checkpoint's prior result words survive. */
    if (s_reward_success_words_valid) {
        mission_blob_word_set_at(blob, 4u, 1u);
        memcpy(blob + 8u, s_reward_success_words,
               sizeof(s_reward_success_words));
    }
    memset(blob + 0x28, 0, 0x198u);

    /* The reference's 0x2c5b0 classifier reads voxel +5 and routes its
     * {1,3,5,10,25} weight to one of four accumulators. This caller adds the
     * first output to +0x1d1c and the third output to +0x1f2c. */
    /* FUN_00460360 copies the list prepared by StartLevelSelected rather
     * than rebuilding it during every refresh. Keep that snapshot stable
     * across reward ticks and use its saved grid count for the denominator. */
    type4_snapshot = start_level_entity_type4_list_get();
    type4_name_count = type4_snapshot != NULL
        ? type4_snapshot->count : 0u;
    type4_grid_count = type4_snapshot != NULL
        ? type4_snapshot->grid_count : 0u;
    if (type4_name_count > 100u) type4_name_count = 100u;
    if (type4_snapshot != NULL) {
        /* DAT_005529c4 is 100 type-4 selector dwords followed by the count
         * and terrain-grid count. CheckpointBlobTaskProgressRefresh copies
         * that 0x198-byte block verbatim to blob +0x28 before formatting the
         * names at +0x1d4. */
        memcpy(blob + 0x28u, type4_snapshot->indices, 100u * sizeof(uint32_t));
    }
    mission_blob_word_set_at(blob, 0x1b8, type4_name_count);
    mission_blob_word_set_at(blob, 0x1bc, type4_grid_count);
    mission_blob_word_set_at(blob, 0x1cc,
        mission_blob_word_get_at(blob, 0x1b8));
    mission_blob_word_set_at(blob, 0x1d0,
        mission_blob_word_get_at(blob, 0x1bc));
    {
        uint32_t i;
        for (i = 0u; i < type4_name_count; ++i) {
            char* row = (char*)blob + 0x1d4u + (size_t)i * 0x40u;
            snprintf(row, 0x40u, "%s", type4_snapshot->names[i] != NULL
                     ? type4_snapshot->names[i] : "");
        }
    }
    /* FUN_00460360 walks the grid with x as the outer index and y as the
     * inner index; preserve that order for the float accumulators. */
    for (x = 0; x < width; ++x) {
        int y;
        for (y = 0; y < height; ++y) {
            const uint8_t* voxel = level_voxel(level, x, y);
            uint32_t a = 0u, b = 0u, c = 0u, d = 0u;
            uint32_t flags;
            uint8_t terrain_type;
            if (voxel == NULL) continue;
            ++cell_count;
            level_cell_byte_state_classify(level, x, y, &a, &b, &c, &d);
            classified_a += (float)a;
            classified_b += (float)c;
            terrain_type = voxel[2];
            __builtin_memcpy(&flags, voxel + 8, sizeof(flags));
            if ((flags & 8u) == 0u) {
                if (terrain_type != 1u) {
                    eligible_cells += 1.0f;
                    eligible_non_solid += 1.0f;
                }
                if (terrain_type == 10u) classified_a += 4.0f;
            }
            else if (terrain_type != 7u && terrain_type != 6u) {
                eligible_non_solid += 1.0f;
            }
            if ((flags & 0x61100000u) != 0u) {
                marked_cells += 1.0f;
                eligible_cells += 1.0f;
            }
        }
    }

    mission_blob_word_set_at(blob, 0x1c8, cell_count);
    mission_blob_float_set_at(blob, 0x1d1c, classified_a);
    mission_blob_float_set_at(blob, 0x1f2c,
                           classified_b + eligible_cells * 4.0f);
    mission_blob_float_set_at(blob, 0x213c, eligible_cells);
    mission_blob_float_set_at(blob, 0x1b04, eligible_non_solid);
    mission_blob_float_set_at(blob, 0x1b08, marked_cells);
    mission_blob_float_set_at(blob, 0x1d20,
        (float)level_int(level, OBJ_OFF_CRYSTAL_LIVE));
    mission_blob_float_set_at(blob, 0x1f30,
        (float)level_int(level, OBJ_OFF_ORE_LIVE));
    mission_blob_float_set_at(blob, 0x2b8c, 100.0f);
    mission_blob_float_set_at(blob, 0x2b90,
        (float)level_int(level, LEVEL_OFF_AIR_SUPPLY));
    if (mission_blob_word_get_at(blob, 0x1d14) != 0u) {
        mission_blob_float_set_at(blob, 0x1d1c,
            (float)mission_blob_word_get_at(blob, 0x1d14));
    }
    mission_blob_float_set_at(blob, 0x234c, (float)type4_name_count);
    /* The retail calculation is uint arithmetic: (count - grid_count) + 1
     * wraps before it is converted to float for the task total. */
    mission_blob_float_set_at(blob, 0x2350,
        (float)(type4_name_count - type4_grid_count + 1u));
    {
        const uint32_t timer_quota = mission_blob_word_get_at(blob, 0x2d94);
        const float timer_count = mission_blob_float_get_at(blob, 0x2da0);
        float timer_progress = 0.0f;
        if (timer_quota != 0u) {
            const float quota_progress = timer_count * 0.04f;
            if ((float)timer_quota <= quota_progress) {
                timer_progress = 100.0f;
            }
            else {
                /* The decompile shows __ftol after comparing the Timer
                 * quota against four percent of the live count. The
                 * percentage numerator is inferred from that saturation
                 * threshold; __ftol supplies the observed truncation. */
                const int progress = crt_float_to_long(
                    (quota_progress / (float)timer_quota) * 100.0f);
                timer_progress = (float)progress;
                if (timer_progress > 100.0f) timer_progress = 100.0f;
                if (timer_progress < 0.0f) timer_progress = 0.0f;
            }
        }
        mission_blob_float_set_at(blob, 0x2da4, timer_progress);
    }
    if (mission_blob_word_get_at(blob, 0x2344) != 0u) {
        mission_blob_float_set_at(blob, 0x2354,
            (float)(100u / mission_blob_word_get_at(blob, 0x2344)) *
             mission_blob_float_get_at(blob, 0x2350));
    }
    if (mission_blob_word_get_at(blob, 0x2554) != 0u) {
        mission_blob_float_set_at(blob, 0x2564,
            (float)(100u / mission_blob_word_get_at(blob, 0x2554)) *
             mission_blob_float_get_at(blob, 0x2560));
    }

    /* Clamp every task's render percentage, except the Timer record whose
     * +0x38 field is owned by the timer task path in the original. */
    {
        uint32_t offset;
        for (offset = 0u; offset < 0x14a0u; offset += GAME_TASK_STRIDE) {
            float progress;
            if (offset == 0x1080u) continue;
            progress = mission_blob_float_get_at(blob, 0x1cec + offset + 0x38u);
            if (progress < 0.0f) progress = 0.0f;
            if (progress > 100.0f) progress = 100.0f;
            mission_blob_float_set_at(blob, 0x1cec + offset + 0x38u, progress);
        }
    }
    if (save_current_context_get() == ctx) {
        (void)checkpoint_blob_task_progress_recalculate_target();
    }
    else {
        (void)checkpoint_blob_task_progress_recalculate(ctx);
    }
    (void)reward_stats_panel_draw_bound();
    /* FUN_00460360 clears the whole temporary global block after the draw.
     * Keep the portable success-word sidecar one-shot for the same reason. */
    s_reward_success_words_valid = 0;
    memset(s_reward_success_words, 0, sizeof(s_reward_success_words));
    return 1;

}

/* ResolvedEntityOperationDispatch (FUN_00460910) — see entity.hpp. */
int resolved_entity_operation_dispatch(const char* name, int slot,
                                       int subtract)
{
    /* Corpus shape: one local (the type slot, `push ecx`) and the resolver's
     * row output aliased onto this function's first-parameter stack slot
     * (`lea eax,[esp+0x8]` at 0x460915), so the resolver's entry zeroing
     * clears it for special names while table/ore matches write the row
     * there, and the post-resolver count scan reads its column word back
     * out of the name parameter (`mov edx,[esp+0x8]` at 0x460936). The
     * corpus's fourth resolver word is the immediate 0 (its NULL-guarded
     * tvcamera store never fires from here). */
    int type;
    if (entity_type_pair_for_name(name, &type, (int*)&name, NULL)) {
        return grid_record_count_difference_get(type, *(int*)&name, slot,
                                                subtract);
    }
    return 0;
}

int checkpoint_blob_task_progress_recalculate_target(void)
{
    GameContext* ctx = s_save_current_context;
    if (ctx == NULL || ctx->level == NULL) {
        return 0; /* corpus: the CurrentLevelGet guard; deviation: threaded */
    }
    char* blob = (char*)checkpoint_blob_get(ctx);
    if (blob == NULL) {
        return 0; /* corpus: the blob getter guard */
    }
    blob_float_put(blob, 0x2fb0, 0.0f);
    blob_float_put(blob, 0x2fac, 0.0f);
    blob_float_put(blob, 0x2fb4, 0.0f);
    (void)blob_percent_fold_store(blob, 0x1d1c, 0x1d20, 0x1d24);
    (void)blob_percent_fold_store(blob, 0x1f2c, 0x1f30, 0x1f34);
    (void)blob_percent_fold_store(blob, 0x213c, 0x2140, 0x2144);
    (void)blob_percent_fold_store(blob, 0x1b04, 0x1b08, 0x1b0c);
    (void)blob_percent_fold_store(blob, 0x2b8c, 0x2b90, 0x2b94);
    (void)blob_percent_fold_store(blob, 0x234c, 0x2350, 0x2354);
    (void)blob_percent_fold_store(blob, 0x276c, 0x2770, 0x2774);
    (void)blob_percent_fold_store(blob, 0x297c, 0x2980, 0x2984);
    blob_count_up(blob, 0x2760, 0x2774);
    blob_count_up(blob, 0x1d10, 0x1d24);
    blob_count_up(blob, 0x2b80, 0x2b94);
    blob_count_up(blob, 0x2340, 0x2354);
    blob_count_up(blob, 0x2550, 0x2564); /* the raw-window twin */
    if (0.0f < blob_float_get(blob, 0x2d90)) {
        /* Corpus shape: this loop keeps both accumulators in registers
         * and writes them back only when it ends. */
        float sum = blob_float_get(blob, 0x2fac);
        float carry = blob_float_get(blob, 0x2fb0);
        const float each = 100.0f - blob_float_get(blob, 0x2da4);
        for (uint32_t n = 0u;
             (float)n < blob_float_get(blob, 0x2d90); ++n) {
            sum = sum + 100.0f;
            carry = each + carry;
        }
        blob_float_put(blob, 0x2fb0, carry);
        blob_float_put(blob, 0x2fac, sum);
    }
    blob_count_up(blob, 0x2970, 0x2984);
    if (blob_float_get(blob, 0x2fac) < blob_float_get(blob, 0x2fb0)) {
        blob_float_put(blob, 0x2fb0, blob_float_get(blob, 0x2fac));
    }
    if (blob_float_get(blob, 0x2fb0) < 0.0f) {
        blob_float_put(blob, 0x2fb0, 0.0f);
    }
    float v = blob_percent_fold_store(blob, 0x2fac, 0x2fb0, 0x2fb4);
    uint32_t bonus = 0u; /* the corpus's converted uint word */
    __builtin_memcpy(&bonus, blob + 0x1c0, sizeof bonus);
    v = (float)bonus + blob_float_get(blob, 0x2fb4);
    blob_float_put(blob, 0x2fb4, v);
    if (100.0f < v) {
        v = 100.0f;
    }
    blob_float_put(blob, 0x2fb4, v);
    return 1;
}

/* RewardStatsPanelDraw (FUN_00460f00) formats stats and selects the result
 * label from the active level before issuing renderer-owned draw operations. */
int reward_stats_panel_draw_bound(void)
{
    GameContext* game = mission_reward_game_get();
    if (game == NULL || game->level == NULL ||
        game->checkpoint_blob == NULL ||
        !reward_stats_text_format_bound()) return 0;
    {
        RewardScreen* config = &game->reward_screen;
        const char* configured_text = NULL;
        char* decoded_text;
        int result = level_int(game->level, LEVEL_OFF_OBJ_RESULT);
        config->stats_result_text[0] = '\0';
        if (result == 1) configured_text = config->complete_text;
        else if (result == 2) configured_text = config->failed_text;
        else if (result == 3) configured_text = config->quit_text;
        if (configured_text != NULL && configured_text[0] != '\0') {
            /* Retail passes the configured label through
             * FormattedTextEscapeDecodeAndEmit before adding it to the
             * reward text context. */
            decoded_text = formatted_text_escape_decode_and_emit(
                "%s", configured_text);
            if (decoded_text != NULL) {
                snprintf(config->stats_result_text,
                         sizeof(config->stats_result_text), "%s",
                         decoded_text);
                engine_heap_free(decoded_text);
            }
        }
    }
    if (s_reward_screen_ops != NULL &&
        s_reward_screen_ops->reward_stats_draw != NULL) {
        s_reward_screen_ops->reward_stats_draw(
            s_reward_screen_ops->context, game);
    }
    else {
        static const int text_offsets[] = {
            0x1dc0, 0x1fd0, 0x21e0, 0x1ba8, 0x23f0,
            0x2600, 0x2810, 0x2a20, 0x2c30, 0x3050
        };
        DisplaySurface* target = surface_active_render_target_get();
        void* font_record = s_reward_font_title_record != NULL
            ? s_reward_font_title_record : s_reward_font_base_record;
        TextureSurface* font =
            font_atlas_record_surface_get(font_record);
        if (target != NULL && font != NULL) {
            void* pixels = NULL;
            int pitch = 0;
            if (surface_lock(target, &pixels, &pitch) >= 0) {
                if (pixels != NULL) {
                    int line_height = font_atlas_record_line_height_get(
                        font_record);
                    int x = game->reward_screen.text_pos[0];
                    int y = game->reward_screen.text_pos[1];
                    int i;
                    if (line_height <= 0) line_height = 19;
                    if (game->reward_screen.stats_result_text[0] != '\0') {
                        reward_text_draw(font, pixels, pitch, target->w,
                            target->h, x, y, "%s",
                            game->reward_screen.stats_result_text);
                        y += line_height;
                    }
                    for (i = 0; i < 10; ++i) {
                        const char* text = (const char*)game->checkpoint_blob +
                            text_offsets[i];
                        if (text[0] != '\0')
                            reward_text_draw(font, pixels, pitch, target->w,
                                target->h, x, y + i * line_height, "%s", text);
                    }
                }
                surface_unlock(target, pixels);
            }
        }
    }
    return 1;
}
/* RewardStatsTextFormat (FUN_00461070). The reference's `%d%%%%` formatter
 * emits two percent characters; construction and cavern counts use `%d`.
 * Elapsed time is assembled from the literals at 0x4a949c..0x4a94c8 and
 * stored as HH:MM.SS. */
int reward_stats_text_format_bound(void)
{
    static const int float_value_offsets[] = {
        0x1d3c, 0x1f4c, 0x215c, 0x1b24,
        0x257c, 0x278c, 0x299c, 0x2bac, 0x2fcc
    };
    static const int text_offsets[] = {
        0x1dc0, 0x1fd0, 0x21e0, 0x1ba8, 0x23f0,
        0x2600, 0x2810, 0x2a20, 0x2c30, 0x3050
    };
    static const char* const formats[] = {
        /* DAT_004a94c8 is "%d%%%%"; the CRT emits two percent signs. */
        "%d%%%%", "%d%%%%", "%d%%%%", "%d", "%d",
        "%d%%%%", "%d%%%%", "%d%%%%", "%d%%%%", "%d%%%%"
    };
    GameContext* game = mission_reward_game_get();
    uint8_t* blob;
    int i;
    uint32_t elapsed;
    uint32_t remaining;
    uint32_t hours;
    uint32_t minutes;
    uint32_t seconds;
    float value;
    int32_t integer;
    char time_text[128];
    char time_part[32];

    if (game == NULL || game->level == NULL || game->checkpoint_blob == NULL)
        return 0;
    blob = game->checkpoint_blob;
    /* Four task percentages, then four map-derived percentages, are stored as
     * floats in the snapshot. Convert through the port's __ftol-compatible
     * helper, then write the integer back before formatting the display. */
    for (i = 0; i < 8; ++i) {
        int text_index = i < 4 ? i : i + 1;
        __builtin_memcpy(&value, blob + float_value_offsets[i], 4);
        integer = (int32_t)crt_float_to_long(value);
        __builtin_memcpy(blob + float_value_offsets[i], &integer, 4);
        sprintf((char*)blob + text_offsets[text_index], formats[text_index],
                (int)integer);
    }

    /* Cavern count is already an integer. The reference mirrors it into the
     * task record at +0x236c, then formats the copy. */
    __builtin_memcpy(&integer, blob + 0x1cc, 4);
    __builtin_memcpy(blob + 0x236c, &integer, 4);
    sprintf((char*)blob + text_offsets[4], formats[4], (int)integer);

    /* Convert and retain the unwrapped elapsed value at +0x2dbc, then reduce
     * a local copy modulo 359999 for the display. The target assembles hours,
     * minutes and seconds with the exact zero-padding and separators below. */
    __builtin_memcpy(&value, blob + 0x2dbc, 4);
    integer = (int32_t)crt_float_to_long(value);
    elapsed = (uint32_t)integer;
    __builtin_memcpy(blob + 0x2dbc, &elapsed, 4);
    remaining = elapsed % 359999u;
    time_text[0] = '\0';

    if (remaining < 0xe11u) {
        strcat(time_text, "00:");
    }
    else {
        hours = (remaining - 1u) / 3600u;
        remaining -= hours * 3600u;
        if (hours < 10u) sprintf(time_part, "0%d:", (int)hours);
        else sprintf(time_part, "%d:", (int)hours);
        strcat(time_text, time_part);
    }

    if (remaining < 0x3du) {
        strcat(time_text, "00.");
    }
    else {
        minutes = (remaining - 1u) / 60u;
        remaining -= minutes * 60u;
        if (minutes < 10u) sprintf(time_part, "0%d.", (int)minutes);
        else sprintf(time_part, "%d.", (int)minutes);
        strcat(time_text, time_part);
    }

    if (remaining < 2u) {
        strcat(time_text, "00");
    }
    else {
        seconds = remaining - 1u;
        if (seconds < 10u) sprintf(time_part, "0%d", (int)seconds);
        else sprintf(time_part, "%d", (int)seconds);
        strcat(time_text, time_part);
    }
    sprintf((char*)blob + 0x2e40, "%s", time_text);

    /* Score is the final float-to-integer conversion in the target body. */
    __builtin_memcpy(&value, blob + float_value_offsets[8], 4);
    integer = (int32_t)crt_float_to_long(value);
    __builtin_memcpy(blob + float_value_offsets[8], &integer, 4);
    sprintf((char*)blob + text_offsets[9], formats[9], (int)integer);
    return 1;
}

int reward_screen_tick(void)
{
    RewardScreenState* state = s_reward_screen_state;
    const RewardScreenOps* ops = s_reward_screen_ops;
    GameContext* game;
    uint32_t now;
    int completed;
    int i;
    if (state == NULL || state->game == NULL) return 0;
    game = state->game;
    if (!reward_screen_entry_check(game, ops)) {
        /* A disabled/missing reward gate follows CheckpointStep directly in
         * retail. Keep the portable render overlay from surviving that
         * mission transition. */
        mission_reward_set_active(game, 0);
        return 0;
    }
    /* The task-card loop is modal in retail and needs the owning frame pump.
     * Without that renderer/input seam, the active reward remains owned by
     * the outer game loop (which draws the live overlay) instead of spinning
     * here without advancing time or input. */
    if (ops == NULL || ops->frame_pump == NULL) return 1;
    now = ops->time_get_ms != NULL
        ? ops->time_get_ms(ops->context) : platform_time_milliseconds();
    state->last_clock_ms = now;
    completed = level_int(game->level, OBJ_OFF_RESULT_INDEX) == 1;

    while ((state->transition_stage < 4 ||
            renderer_word_c_get() != 0u ||
            (ops->loop_should_continue != NULL &&
             ops->loop_should_continue(ops->context, state))) &&
           (ops->loop_should_abort == NULL ||
            !ops->loop_should_abort(ops->context))) {
        uint32_t delta_ms_time = ops->time_get_ms != NULL
            ? ops->time_get_ms(ops->context) : platform_time_milliseconds();
        float delta = ((float)delta_ms_time -
                       (float)state->last_clock_ms) * 0.001f;
        uint32_t current_ms;
        int task_limit;
        int fast_forward;
        int score_text_drawn = 0;
        current_ms = ops->time_get_ms != NULL
            ? ops->time_get_ms(ops->context) : platform_time_milliseconds();
        state->last_clock_ms = current_ms;
        state->elapsed += delta;
        reward_flic_context_tick_abi(&state->displayed_task_count,
                                     &state->transition_stage,
                                     &state->elapsed);
        fast_forward = reward_screen_fast_forward_get(state, ops);
        if (fast_forward && state->transition_stage < 3) {
            ++state->transition_stage;
            if (state->transition_stage == 3)
                state->transition_stage = 2;
            if (state->transition_stage < 2 || !completed)
                state->displayed_task_count = 9;
            else state->displayed_task_count = 10;
        }
        for (i = 0; i < state->displayed_task_count; ++i)
            reward_screen_task_visual_draw(state, ops, i, 2u);
        task_limit = state->displayed_task_count < 9
            ? state->displayed_task_count : 9;
        for (i = 0; i < task_limit; ++i)
            reward_screen_task_visual_draw(state, ops, i, 0x40u);
        for (i = 0; i < state->displayed_task_count; ++i)
            reward_screen_task_visual_draw(state, ops, i, 4u);

        reward_task_text_draw_and_score_gate_abi(
            &state->displayed_task_count, &state->score_text_gate,
            state->elapsed);
        if (state->score_text_gate && game->reward.display_text != 0) {
            score_text_drawn = reward_score_task_text_draw(
                state->displayed_task_count, state->elapsed);
            if (score_text_drawn && state->transition_stage < 3 &&
                state->game->reward_screen.display != 0)
                (void)reward_screen_zone_check(state, ops);
        }
        /* RewardScreenTick draws the task/text passes and runs RewardZoneCheck
         * before advancing the scrolling banner (FUN_00462ad0). */
        if (state->scroll_ticker != NULL)
            reward_scroll_ticker_advance_and_draw(state->scroll_ticker);
        if (ops->reward_stats_draw != NULL)
            ops->reward_stats_draw(ops->context, game);
        if (state->transition_stage == 3) state->transition_stage = 4;
        reward_screen_transition_tick(state, ops);
        reward_pointer_override_expiry_tick(state, delta, ops);
    }

    if (ops->reward_scroll_cleanup != NULL)
        ops->reward_scroll_cleanup(ops->context);
    if (ops->render_finish != NULL) ops->render_finish(ops->context);
    reward_task_flics_close_all(state);
    sound_stream_stop_and_release(1);
    if (ops->input_restore != NULL) ops->input_restore(ops->context, 1);
    if (completed && reward_endgame_avi1_configured(game)) {
        if (ops->completion_audio != NULL)
            ops->completion_audio(ops->context, game);
        sound_request_queue_submit(0x2c, 1);
        if (ops->frame_pump != NULL) ops->frame_pump(ops->context, 1);
        sound_stream_stop_and_release(1);
    }
    return 1;
}
/* RewardDisplayTick (FUN_00461790) takes the task pointer, draw mask, and
 * displayed-task count. The elapsed value is read from the bound loop state. */
void reward_display_task_abi(GameTask* task, uint32_t draw_mask,
                             uint32_t displayed_count)
{
    RewardScreenState* state = s_reward_screen_state;
    const RewardScreenOps* ops = s_reward_screen_ops;
    float elapsed;
    int index;
    uint32_t flags;
    Flic* flic;
    const char* path;

    if (state == NULL || state->game == NULL || task == NULL)
        return;
    elapsed = state->elapsed;
    index = reward_task_record_index(state, task, (int)displayed_count);
    if (index < 0 || index >= GAME_TASK_COUNT) return;
    flags = task_flags(task);
    if (draw_mask == 2u &&
        (state->game->reward.display_images == 0 ||
         (flags & TASK_FLAG_IMG) == 0u)) return;
    if (draw_mask == 0x40u &&
        (state->game->reward.display_images == 0 ||
         (flags & TASK_FLAG_BOXIMG) == 0u)) return;
    if (draw_mask == 4u &&
        (state->game->reward.display_flics == 0 ||
         (flags & TASK_FLAG_FLIC) == 0u)) return;
    if ((draw_mask == 1u || draw_mask == 0x10u || draw_mask == 0x20u) &&
        state->game->reward.display_text == 0) return;
    if (draw_mask == 1u && (flags & TASK_FLAG_COMPLETED) == 0u) return;
    if (draw_mask == 0x10u && (flags & TASK_FLAG_TEXT) == 0u) return;
    if (draw_mask == 0x20u && (flags & TASK_FLAG_BOXTEXT) == 0u) return;
    if (draw_mask != 1u && draw_mask != 2u && draw_mask != 4u &&
        draw_mask != 0x10u && draw_mask != 0x20u && draw_mask != 0x40u)
        return;

    if (draw_mask == 1u) {
        if (ops != NULL && ops->task_text_draw != NULL)
            ops->task_text_draw(ops->context, task, index,
                                (int)displayed_count, elapsed);
        else
            reward_task_text_draw_fallback(task, index, displayed_count);
        return;
    }

    flic = state->task_flics[index];
    if (draw_mask == 4u) {
        if (flic == NULL) {
            path = (const char*)task->data + TASK_OFF_FLICPATH2;
            if (path[0] != '\0') {
                flic = flic_open(path);
                state->task_flics[index] = flic;
                task_put_int(task, TASK_OFF_FLIC_HANDLE,
                             (int32_t)(uint32_t)(uintptr_t)flic);
            }
        }
        if (flic != NULL) {
            /* FUN_00461790 passes FlicFrameAdvanceAndBlit's frame-index
             * gate: frames advance while +0x624 (the current index) is
             * below 0x23, using the non-loop stepper. At index 0x23 and
             * later, retain the last decoded image without advancing. */
            if (flic_frame(flic) < 0x23)
                (void)flic_tick_loop(flic, 0);
        }
    }
    /* The FLIC is decoded above, but FUN_00484070 presents it through a
     * renderer-owned rectangle/blit interface that DisplaySurface does not
     * model. FLIC presentation therefore remains behind this renderer op. */
    if (ops != NULL && ops->task_visual_draw != NULL) {
        ops->task_visual_draw(ops->context, task, index, draw_mask,
                              (int)displayed_count, elapsed, flic);
    }
    else if (draw_mask == 2u || draw_mask == 0x40u) {
        reward_task_image_draw_fallback(task, index, draw_mask);
    }
    if ((draw_mask == 0x10u || draw_mask == 0x20u) &&
        ops != NULL && ops->task_text_draw != NULL) {
        ops->task_text_draw(ops->context, task, index,
                            (int)displayed_count, elapsed);
    }
    else if (draw_mask == 0x10u || draw_mask == 0x20u) {
        reward_task_text_draw_fallback(task, index, displayed_count);
    }
}

/* RewardScoreTaskTextDraw (FUN_00461c90). The native caller supplies the
 * current displayed-task count and elapsed time; the score record is the
 * final task slot in the active reward context. Its ordinary task text is
 * submitted through RewardDisplayTick before the separate score string. */
int reward_score_task_text_draw(int displayed_task_count, float elapsed)
{
    RewardScreenState* state = s_reward_screen_state;
    const RewardScreenOps* ops = s_reward_screen_ops;
    GameContext* game;
    int drawn = 0;
    if (state == NULL || state->game == NULL || state->game->level == NULL)
        return 0;
    game = state->game;
    if (game->level->objective_result != 1) return 0;

    reward_display_task_abi(reward_task_get(state, GAME_TASK_COUNT - 1), 1u,
                            (uint32_t)displayed_task_count);
    if (ops != NULL && ops->score_text_draw != NULL) {
        drawn = ops->score_text_draw(ops->context, game,
                                     displayed_task_count, elapsed);
    }
    return drawn;
}
int mission_reward_zone(GameContext* ctx, int mx, int my,
                        int* hit_save, int* hit_advance)
{
    if (ctx == NULL || hit_save == NULL || hit_advance == NULL) {
        return 0;
    }
    *hit_save = 0;
    *hit_advance = 0;
    if (!ctx->reward_active || ctx->level == NULL) {
        return 0;
    }
    int result = level_int(ctx->level, LEVEL_OFF_OBJ_RESULT);
    if (result == 1) {
        /* achieved: the SaveButton zone is live (RewardZoneCheck branch 1) */
        if (zone_hit(ctx->reward_screen.save_x, ctx->reward_screen.save_y,
                     ctx->reward_screen.save_w, ctx->reward_screen.save_h,
                     mx, my)) {
            *hit_save = 1;
            return 1;
        }
    } else if (zone_hit(ctx->reward_screen.advance_x, ctx->reward_screen.advance_y,
                        ctx->reward_screen.advance_w, ctx->reward_screen.advance_h,
                        mx, my)) {
        *hit_advance = 1;
        return 1;
    }
    return 0;
}

namespace reference_target {
void save_menu_open_from_reward(void)
{
    FrontEnd* fe = s_reward_save_frontend;
    if (fe == NULL)
        return;
    options_write_to_slot_target();
    if (!frontend_state_change(fe, FE_STATE_SAVE_REWARD))
        return;
    /* SaveMenuOpenFromReward selects submenu zero in the separate save-menu
     * set. The legacy routine then runs a blocking input loop; the portable
     * frame loop owns that loop, so this entry prepares the same live state
     * and returns to its caller. */
    if (fe->menus[FE_MENU_SAVE].submenu_count <= 0 ||
        !frontend_open(fe, FE_MENU_SAVE))
        return;
    fe->save_action = 1;
}
} // namespace reference_target

void checkpoint_step_target(void)
{
    GameContext* ctx = s_save_current_context;
    if (ctx == NULL) {
        return;
    }
    int orig = ctx->save_slot;
    int slot = orig == -1 ? SAVE_DEFAULT_SLOT : orig;

    if (ctx->records == NULL || ctx->active_slot != slot) {
        /* Bootstrap the slot context (`SavSetSlot(5)` path in the corpus). */
        (void)save_read_slot_target((uint32_t)slot, NULL, 0);
    }
    if (orig == -1) {
        /* SavSetSlot selects the fallback even when its records are already
         * active; the commit uses the selected slot, not active_slot. */
        save_slot_set(ctx, slot);
    }
    int rec = ctx->current_record;
    /* The corpus marks the record dirty when the level's objective-result
     * word (+0x27c) reads 1 ("achieved"), then copies the blob in. */
    int result = 0;
    if (ctx->level != NULL) {
        result = level_int(ctx->level, LEVEL_OFF_OBJ_RESULT);
    }
    if (ctx->checkpoint_blob == NULL) {
        checkpoint_blob_build(ctx);
    }
    /* Write before marking: the write grows the record array if `rec` is
     * past the bootstrap count, so the mark (a flags-word write) can't
     * miss. The corpus marks first, but both touch the same record — the
     * observable order is the same. */
    save_write_record(ctx, rec, ctx->checkpoint_blob);
    if (result == 1) {
        save_mark_record(ctx, rec);
    }
    if (orig == -1) {
        /* FUN_00462270 clears every default-slot level record after the
         * first one, then reapplies the current checkpoint before commit.
         * The initial write above ensures the table has grown far enough for
         * this active level's record. */
        if (ctx->records != NULL && ctx->record_count > 1) {
            memset(ctx->records + SAVE_RECORD_SIZE, 0,
                   (size_t)(ctx->record_count - 1) * SAVE_RECORD_SIZE);
        }
        (void)save_write_record(ctx, rec, ctx->checkpoint_blob);
        if (result == 1) {
            save_mark_record(ctx, rec);
        }
        /* The corpus commits the cleared/reapplied default slot, then
         * reloads slot-5 metadata. */
        save_commit(ctx);
        (void)save_read_slot_target(SAVE_DEFAULT_SLOT, NULL, 0);

    }
}

void reward_screen_transition_tick_abi(int* displayed_task_count,
                                       int* transition_stage,
                                       int* score_text_gate, float* elapsed)
{
    RewardScreenState* state = s_reward_screen_state;
    if (state == NULL || displayed_task_count == NULL ||
        transition_stage == NULL || score_text_gate == NULL ||
        elapsed == NULL) return;
    state->displayed_task_count = *displayed_task_count;
    state->transition_stage = *transition_stage;
    state->score_text_gate = *score_text_gate;
    state->elapsed = *elapsed;
    reward_screen_transition_tick(state, s_reward_screen_ops);
    *displayed_task_count = state->displayed_task_count;
    *transition_stage = state->transition_stage;
    *score_text_gate = state->score_text_gate;
    *elapsed = state->elapsed;
}
/* RewardFlhContextReload @ 0x00462460 closes and reopens one task's FLIC
 * context only when its record has the FLIC flag. Resolve that record to the
 * active task index so the host-width Flic pointer stays in the sidecar. */
void reward_flic_context_reload_abi(void* task_context)
{
#if defined(_WIN64) || defined(__LP64__)
    RewardScreenState* state = s_reward_screen_state;
    GameTask* task = (GameTask*)task_context;
    int index;
    if (task == NULL || (task_flags(task) & TASK_FLAG_FLIC) == 0u ||
        state == NULL || state->game == NULL) return;
    for (index = 0; index < GAME_TASK_COUNT; ++index) {
        if (task == &state->game->tasks[index] ||
            task == reward_task_get(state, index)) {
            reward_task_flic_reload_record(state, task, index);
            return;
        }
        if (state->game->checkpoint_blob != NULL &&
            task == (GameTask*)(state->game->checkpoint_blob + 0x1cecu +
                (size_t)index * GAME_TASK_STRIDE)) {
            reward_task_flic_reload_record(state, task, index);
            return;
        }
    }
#else
    uint8_t* task = (uint8_t*)task_context;
    if (task != NULL && (task[0] & TASK_FLAG_FLIC) != 0u) {
        Flic** flic = (Flic**)(task + TASK_OFF_FLIC_HANDLE);
        flic_close(*flic);
        *flic = flic_open((const char*)(task + TASK_OFF_FLICPATH2));
    }
#endif
}

/* RewardFlhContextTick (FUN_004624a0) updates the timer/index tuple supplied
 * by the reward loop, and performs task audio and transition cleanup. */
void reward_flic_context_tick_abi(int* displayed_task_count,
                                  int* transition_stage, float* elapsed)
{
    RewardScreenState* state = s_reward_screen_state;
    if (state == NULL || state->game == NULL || displayed_task_count == NULL ||
        state->game->checkpoint_blob == NULL || transition_stage == NULL ||
        elapsed == NULL) return;
    state->displayed_task_count = *displayed_task_count;
    state->transition_stage = *transition_stage;
    state->elapsed = *elapsed;
    reward_screen_task_timer_tick(state);
    *displayed_task_count = state->displayed_task_count;
    *transition_stage = state->transition_stage;
    *elapsed = state->elapsed;
}
/* RewardTaskTextDrawAndScoreGate (FUN_00462600) applies the completion-side
 * score-FLIC gate, draws text for the revealed task rows, then submits the
 * level/result labels through the renderer-owned reward text context. The
 * retail caller already owns the displayed count and elapsed value. */
void reward_task_text_draw_and_score_gate_abi(int* displayed_task_count,
                                             int* score_text_gate,
                                             float elapsed)
{
    RewardScreenState* state = s_reward_screen_state;
    const RewardScreenOps* ops = s_reward_screen_ops;
    GameContext* game;
    int count;
    int objective_result;
    int i;
    int task_offset;
    if (state == NULL || state->game == NULL ||
        state->game->level == NULL ||
        state->game->checkpoint_blob == NULL) return;
    game = state->game;
    count = *displayed_task_count;
    objective_result = level_objective_result_get(game->level);

    if ((count == 10 || (objective_result != 1 && count == 9)) &&
        (mission_blob_word_get(game->checkpoint_blob, 0x3178u) == 0u ||
         game->reward.display_flics == 0 || objective_result != 1 ||
         (state->task_flics[GAME_TASK_COUNT - 1] != NULL &&
          flic_frame(state->task_flics[GAME_TASK_COUNT - 1]) > 0x21))) {
        *score_text_gate = 1;
    }

    i = 0;
    task_offset = 0;
    for (;;) {
        int current_count = *displayed_task_count;
        int task_limit = current_count;
        GameTask* task;
        if (task_limit > 9) task_limit = 9;
        if (task_limit <= i) break;
        task = (GameTask*)(game->checkpoint_blob + 0x1cecu +
                           (size_t)task_offset);
        reward_display_task_abi(task, 1u, (uint32_t)current_count);
        ++i;
        task_offset += GAME_TASK_STRIDE;
    }

    if (ops != NULL && ops->task_result_text_draw != NULL)
        ops->task_result_text_draw(ops->context, game, elapsed);
}

/* PointerOverrideExpiryTick (FUN_00462700): expire overrides, draw the
 * current cursor, advance the sound/effect work, then pump one frame. */
void pointer_override_expiry_tick(float delta)
{
    media_pointer_cursor_override_tick(1.0f);
    media_pointer_cursor_select(0);
    pointer_cursor_draw_target(s_renderer_word_a, s_renderer_word_b);
    (void)unported_ledger_stubs::
        ledger_stub_00435550_GlobalStateZeroTransitionDetect();
    s_window_mouse_word_4c = 0u;
    sound_subsystem_frame_tick(delta);
    timed_effect_runtime_update(s_renderer_word_a, s_renderer_word_b,
                                (delta * 0.04f) * 1000.0f);
    main_loop_frame_pump(1);
}

#if !defined(LEGORRR_BACKEND_WINAPI) /* native backend and tests only (#576) */
int mission_reward_click(GameContext* ctx, int hit_save)
{
    if (ctx == NULL || ctx->level == NULL || ctx->checkpoint_blob == NULL)
        return 0;

    if (hit_save) {
        /* RewardZoneCheck's SaveButton route enters SaveMenuOpenFromReward;
         * the save menu owns its selection and cancellation. A failed open
         * leaves the reward screen and checkpoint untouched. */
        return save_menu_open_from_reward_bound();
    }

    /* The AdvanceButton route commits through CheckpointStep, then follows
     * the configured nextlevel. mission_reward_advance performs both steps
     * against the active GameContext. At campaign end it closes the reward
     * screen without replacing the level; a missing config/level is failure. */
    if (ctx->cfg == NULL) return 0;
    {
        const char* next = mission_reward_advance(ctx);
        if (next != NULL || !ctx->reward_active) return 1;
    }
    return 0;
}
#endif /* !LEGORRR_BACKEND_WINAPI */

void* callback_record_allocate_initialize(void** out_record, uint32_t param_2,
                                          uint32_t param_3, uint32_t param_4,
                                          uint32_t param_5, uint32_t param_6)
{
    void* record = engine_heap_alloc(0x28);
    if (record != NULL) {
        memset(record, 0, 0x28);
        void** slot = (void**)engine_heap_alloc(sizeof(void*));
        __builtin_memcpy((char*)record + 0x20, &slot,
                         sizeof slot); /* corpus dword 8 */
        if (slot != NULL) {
            /* deviation: the corpus strides when this alloc fails */
            void* buffer = engine_heap_alloc(0x20);
            if (buffer != NULL) {
                memset(buffer, 0, 0x20);
            }
            *slot = buffer;
        }
        uint32_t* d = (uint32_t*)record;
        d[5] = param_6;
        d[1] = param_2;
        d[2] = param_3;
        d[3] = param_4;
        d[4] = param_5;
        d[0] = param_3; /* pinned: param_3 rides dwords 0 and 2 */
    }
    if (out_record != NULL) {
        /* pinned quirk: the corpus writes the slot even when the record
         * allocation failed (NULL lands in the caller's slot) */
        *out_record = record;
    }
    return record;
}

#if !defined(LEGORRR_BACKEND_WINAPI) /* native backend and tests only (#576) */
/* TaskTableContentsRelease (FUN_00462960) — see cmd.hpp. */
int task_table_contents_release(void* record)
{
    if (record == NULL) {
        return 1; /* deviation: the corpus strides the handle blindly */
    }
    unsigned char* rec = (unsigned char*)record;
    uint32_t count;
    __builtin_memcpy(&count, rec + 0x1c, sizeof count);
    if (count != 0u) {
        uint32_t i = 0;
        for (;;) {
            void* entries = NULL;
            void* entry = NULL;
            void* sub = NULL;
            __builtin_memcpy(&entries, rec + 0x20, sizeof entries);
            if (entries != NULL) {
                entry = ((void**)entries)[i];
            }
            if (entry != NULL) {
                /* Corpus: the entry's +0x14 slot is read blindly; a NULL
                 * entry or NULL sub-slot answers "nothing to free" here. */
                __builtin_memcpy(&sub, (unsigned char*)entry + 0x14,
                                 sizeof sub);
                if (sub != NULL) {
                    engine_heap_free(sub);
                }
                engine_heap_free(entry);
            }
            ++i;
            __builtin_memcpy(&count, rec + 0x1c, sizeof count);
            if (count <= i) {
                break;
            }
        }
    }
    void* entries = NULL;
    __builtin_memcpy(&entries, rec + 0x20, sizeof entries);
    if (entries != NULL) {
        engine_heap_free(entries);
    }
    engine_heap_free(rec);
    return 1;
}
#endif /* !LEGORRR_BACKEND_WINAPI */

/* --- the record-field write band (see record.hpp) --------------------------- */
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) void record_field18_set(void* record, uint32_t value)
{
    __asm {
        fld dword ptr [esp + 8]
        mov eax, dword ptr [esp + 4]
        fstp dword ptr [eax + 18h]
        ret
    }
}
#else
void record_field18_set(void* record, uint32_t value)
{
    if (record == NULL) {
        return; /* deviation: the corpus writes blindly */
    }
    __builtin_memcpy((char*)record + 0x18, &value, sizeof(value));
}
#endif

#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) void record_flags_24_or(void* record, uint32_t flags)
{
    __asm {
        mov eax, dword ptr [esp + 4]
        mov ecx, dword ptr [esp + 8]
        or dword ptr [eax + 24h], ecx
        ret
    }
}
#else
void record_flags_24_or(void* record, uint32_t flags)
{
    if (record == NULL) {
        return; /* deviation: the corpus reads blindly */
    }
    *(uint32_t*)((char*)record + 0x24) |= flags;
}
#endif

extern "C" int reward_scroll_ticker_advance_and_draw(
    RewardScrollTickerState* state)
{
    uint32_t i;
    if (state == NULL || state->entry_get == NULL || state->entry_draw == NULL)
        return 0;
    for (i = 0; i < state->entry_count; ++i) {
        RewardScrollTickerEntry entry;
        int y;
        int upper;
        int clipped;
        if (!state->entry_get(state->context, i, &entry)) continue;
        y = entry.y;
        if ((state->flags & 1u) != 0u && entry.line_height != 0) {
            while ((float)y < state->minimum_y)
                y += entry.line_height;
            upper = (int)(state->scroll_height + state->minimum_y +
                          state->lower_clip);
            while ((float)y > (float)upper) y -= entry.line_height;
        }
        clipped = (entry.flags & 2u) == 0u &&
            ((float)y > state->minimum_y + state->lower_clip ||
             (float)y < state->minimum_y);
        if (!clipped || (entry.flags & 2u) != 0u)
            state->entry_draw(state->context, &entry, entry.x, y, clipped);
    }
    state->current_y -= state->scroll_speed;
    if ((state->flags & 0x10u) != 0u && state->debug_key_get != NULL) {
        if (state->debug_key_get(state->context, 0u)) state->scroll_speed = 0.0f;
        if (state->debug_key_get(state->context, 1u))
            state->current_y = state->minimum_y;
        if (state->debug_key_get(state->context, 2u))
            state->scroll_speed += 0.01f;
        if (state->debug_key_get(state->context, 3u))
            state->scroll_speed -= 0.01f;
        for (i = 0; i < 10u; ++i) {
            if (state->debug_key_get(state->context, i + 4u)) {
                float magnitude = (float)i * 0.1f;
                state->scroll_speed = state->scroll_speed < 0.0f
                    ? -magnitude : magnitude;
            }
        }
    }
    return 1;
}
static void reward_task_flics_close_all(RewardScreenState* state)
{
    int i;
    if (state == NULL) return;
    for (i = 0; i < GAME_TASK_COUNT; ++i) {
        GameTask* task = reward_task_get(state, i);
        if (task != NULL && (task_flags(task) & TASK_FLAG_FLIC) != 0u) {
            if (state->task_flics[i] != NULL) {
                flic_close(state->task_flics[i]);
                state->task_flics[i] = NULL;
            }
            task_put_int(task, TASK_OFF_FLIC_HANDLE, 0);
        }
    }
}
static const char* rw_str(BlockFile* cfg, const char* root, const char* level,
                          const char* sub, const char* name)
{
    (void)root;
    char path[512];
    rw_path(path, sizeof(path), cfg, level, sub, name);
    return block_get_strptr(cfg, path);
}

/* Corpus FUN_004790d0 delegates to ConfigBooleanGet @ 0x477710. */
static int rw_typed(BlockFile* cfg, const char* root, const char* level,
                    const char* name)
{
    (void)root;
    char path[512];
    rw_path(path, sizeof(path), cfg, level, NULL, name);
    return block_get_bool_path(cfg, path);
}

/* Store a short token string at a task payload offset (bounded write). */
static void task_put_str(GameTask* t, int off, const char* s, int cap)
{
    char* dst = (char*)t->data + off;
    dst[0] = '\0';
    if (s == NULL || cap <= 1) {
        return;
    }
    size_t copy_length = strlen(s);
    if (copy_length >= (size_t)cap) copy_length = (size_t)cap - 1u;
    strncpy(dst, s, copy_length);
    dst[copy_length] = '\0';
}

static void task_add_flag(GameTask* t, uint32_t bits)
{
    task_set_flags(t, task_flags(t) | bits);
}

static unsigned int task_slot_index(const GameTask* t)
{
    GameContext* game = s_task_init_binding.game;
    if (game == NULL || t < game->tasks || t >= game->tasks + GAME_TASK_COUNT)
        return GAME_TASK_COUNT;
    return (unsigned int)(t - game->tasks);
}

/* Helper used by the field loaders: resolve a pipe-delimited
 * `file|n|n…` config token to its fields (corpus FUN_00477440). */
static int task_fields(const char* token, char out[UTIL_QUERY_MAX_FIELDS][256])
{
    return util_query_fields(token, out, sizeof(out[0]));
}

static uint32_t mission_blob_word_get(const uint8_t* blob, size_t offset)
{
    uint32_t value;
    __builtin_memcpy(&value, blob + offset, sizeof(value));
    return value;
}

static GameContext* mission_reward_game_get(void)
{
    if (s_checkpoint_refresh_context_override != NULL)
        return s_checkpoint_refresh_context_override;
    if (s_reward_screen_state != NULL && s_reward_screen_state->game != NULL)
        return s_reward_screen_state->game;
    if (s_reward_screen_default_state.game != NULL)
        return s_reward_screen_default_state.game;
    return save_current_context_get();
}

/* Bind the portable reward loop to the newly initialized mission while
 * keeping this owner-side work out of RewardInit's config/resource body. */
static void mission_reward_state_bind_for_level(GameContext* ctx)
{
    RewardScreenState* state = s_reward_screen_state;
    if (ctx == NULL || (state != NULL &&
        state != &s_reward_screen_default_state && state->game != ctx)) {
        return;
    }
    if (state == NULL) state = &s_reward_screen_default_state;
    if (state->game != NULL) reward_task_flics_close_all(state);
    {
        PointerCursorState* pointer_cursor = state->pointer_cursor;
        const PointerCursorHooks* pointer_hooks = state->pointer_cursor_hooks;
        RewardScrollTickerState* scroll_ticker = state->scroll_ticker;
        reward_screen_state_init(state, ctx, pointer_cursor, pointer_hooks);
        state->scroll_ticker = scroll_ticker;
    }
    reward_screen_runtime_bind(state, s_reward_screen_ops);
}

static void mission_reward_button_config_load(BlockFile* cfg,
                                               const char* root,
                                               const char* level_name,
                                               const char* key,
                                               TextureSurface* images[4],
                                               float* x, float* y,
                                               float* width, float* height)
{
    const char* value = rw_str(cfg, root, level_name, NULL, key);
    char fields[UTIL_QUERY_MAX_FIELDS][256];
    int count;
    int i;
    if (value == NULL) return;
    count = task_fields(value, fields);
    if (count < 6) return;
    if (fields[0][0] == '\0') return;
    images[0] = texture_get_or_load(NULL, fields[0]);
    if (images[0] == NULL) return;
    for (i = 1; i < 4; ++i) {
        if (fields[i][0] != '\0')
            images[i] = texture_get_or_load(NULL, fields[i]);
    }
    /* RewardSaveButtonZonesInit applies a black source key to the normal,
     * hover, and pressed surfaces; the fourth state is left unkeyed. */
    for (i = 0; i < 3; ++i) {
        if (images[i] != NULL)
            texture_source_color_key_set(images[i], 0u, 0u);
    }
    *x = (float)atoi(fields[4]);
    *y = (float)atoi(fields[5]);
    *width = (float)images[0]->width;
    *height = (float)images[0]->height;
}

/* Rect hit-test helper (the corpus's widget zones +w/h, RewardZoneCheck). */
static int zone_hit(float x, float y, float w, float h, int mx, int my)
{
    return mx >= x && mx < x + w && my >= y && my < y + h;
}

static int reward_task_record_index(const RewardScreenState* state,
                                    const GameTask* task, int fallback)
{
    int i;
    if (state != NULL && state->game != NULL) {
        for (i = 0; i < GAME_TASK_COUNT; ++i) {
            if (state->game->checkpoint_blob != NULL && task ==
                (const GameTask*)(state->game->checkpoint_blob + 0x1cecu +
                    (size_t)i * GAME_TASK_STRIDE)) {
                return i;
            }
            if (task == &state->game->tasks[i]) return i;
        }
    }
    if (fallback == 0) return 0;
    return fallback < GAME_TASK_COUNT ? fallback - 1 : GAME_TASK_COUNT - 1;
}

static int reward_endgame_avi1_configured(const GameContext* game)
{
    const char* level_name;
    const char* root;
    if (game == NULL || game->cfg == NULL || game->level == NULL) return 0;
    level_name = (const char*)game->level->ptr_name;
    if (level_name == NULL) return 0;
    root = util_root_name(game->cfg);
    return rw_str(game->cfg, root, level_name, NULL, "EndGameAVI1") != NULL;
}

/* RewardSaveButtonZoneCheck @ 0x00462780 is the entry gate for the modal
 * reward screen. The display switch is Reward.Display; blob word zero is
 * Reward.Enable. With either gate off, retail commits CheckpointStep instead
 * of entering the display loop. */
static int reward_screen_entry_check(GameContext* game,
                                     const RewardScreenOps* ops)
{
    if (game == NULL || game->level == NULL || game->checkpoint_blob == NULL)
        return 0;
    if (ops != NULL && ops->save_button_check != NULL) {
        if (ops->save_button_check(ops->context, game)) {
            sound_request_queue_submit(0x29, 1);
            return 1;
        }
        checkpoint_step(game);
        return 0;
    }
    if (game->reward_screen.display != 0 &&
        mission_blob_word_get(game->checkpoint_blob, 0u) != 0u) {
        sound_request_queue_submit(0x29, 1);
        return 1;
    }
    checkpoint_step(game);
    return 0;
}

/* RewardFlhContextTick (FUN_004624a0): advance one task when the display
 * interval expires, or skip the first interval on the slow/normal path. */
static void reward_screen_task_timer_tick(RewardScreenState* state)
{
    int advance = 0;
    int stage_is_initial;
    if (state == NULL || state->game == NULL ||
        state->game->checkpoint_blob == NULL) return;
    const int completed = state->game != NULL && state->game->level != NULL &&
        level_int(state->game->level, OBJ_OFF_RESULT_INDEX) == 1;
    stage_is_initial = state->transition_stage == 0;
    if (state->task_duration <= state->elapsed) {
        if (stage_is_initial) advance = 1;
    }
    /* The retail second gate is a separate test after the first interval
     * check, so the completed-level two-interval path must still run when
     * elapsed is already greater than one interval. */
    if (!stage_is_initial &&
             state->task_duration * 2.0f <= state->elapsed &&
             completed &&
             state->displayed_task_count < GAME_TASK_COUNT) {
        advance = 1;
    }
    if (!advance) return;

    state->elapsed -= state->task_duration;
    if (state->displayed_task_count < GAME_TASK_COUNT)
        ++state->displayed_task_count;
    if (state->displayed_task_count > 0) {
        GameTask* task = reward_task_get(state,
                                          state->displayed_task_count - 1);
        if (task != NULL) {
            const char* sound_name = (const char*)task->data + TASK_OFF_SOUND;
            int sound_alias;
            if (sound_name[0] != '\0' &&
                sound_alias_lookup(sound_name, &sound_alias))
                sound_request_queue_submit(sound_alias, 0);
        }
    }
    if (state->displayed_task_count > 9) {
        if (state->transition_stage == 0) {
            state->displayed_task_count = 9;
            ++state->transition_stage;
            return;
        }
        if (state->transition_stage > 1) {
            if (state->displayed_task_count > 10) {
                state->displayed_task_count = 10;
                ++state->transition_stage;
                return;
            }
            reward_task_flics_reload_all(state);
        }
    }
}

static void reward_screen_task_visual_draw(RewardScreenState* state,
                                          const RewardScreenOps* ops,
                                          int index, uint32_t mask)
{
    GameTask* task = reward_task_get(state, index);
    (void)ops;
    if (task == NULL) return;
    reward_display_task_abi(task, mask,
                            (uint32_t)state->displayed_task_count);
}

/* The renderer callback is optional in the portable runtime. Task image
 * resources are already owned by the task sidecars, and the active display
 * target exposes the same lock/blit operations used by the pointer renderer. */
static void reward_task_image_draw_fallback(GameTask* task, int index,
                                            uint32_t mask)
{
    DisplaySurface* target = surface_active_render_target_get();
    TextureSurface* image;
    int x;
    int y;
    void* pixels = NULL;
    int pitch = 0;
    if (task == NULL || target == NULL) return;
    if (mask == 2u) {
        image = (TextureSurface*)reward_task_image_surface_get(
            (unsigned int)index);
        x = task_get_int(task, TASK_OFF_IMG_XY);
        y = task_get_int(task, TASK_OFF_IMG_XY + 4);
    }
    else if (mask == 0x40u) {
        image = (TextureSurface*)reward_task_box_image_surface_get(
            (unsigned int)index);
        x = task_get_int(task, TASK_OFF_BOXIMG_XY);
        y = task_get_int(task, TASK_OFF_BOXIMG_XY + 4);
    }
    else {
        return;
    }
    if (image == NULL || surface_lock(target, &pixels, &pitch) < 0)
        return;
    if (pixels != NULL)
        surface_blit_texture(pixels, pitch, target->w, target->h, image,
                             x, y);
    surface_unlock(target, pixels);
}

/* +0xd4 is the staging string consumed by the retail draw routine. It has no
 * config writer in the shipped corpus; task names use the configured reward
 * text position and the task's already loaded atlas when that slot is empty. */
static void reward_task_text_draw_fallback(GameTask* task, int index,
                                           uint32_t displayed_count)
{
    DisplaySurface* target = surface_active_render_target_get();
    void* font_record;
    TextureSurface* font;
    const char* text;
    const char* name;
    void* pixels = NULL;
    int pitch = 0;
    int x;
    int y;
    if (task == NULL || target == NULL || s_reward_screen_state == NULL ||
        s_reward_screen_state->game == NULL) return;
    font_record = reward_task_font_record_get((unsigned int)index);
    if (font_record == NULL) font_record = s_reward_font_base_record;
    font = font_atlas_record_surface_get(font_record);
    if (font == NULL || surface_lock(target, &pixels, &pitch) < 0) return;
    if (pixels == NULL) {
        surface_unlock(target, pixels);
        return;
    }
    x = task_get_int(task, TASK_OFF_TEXT_XY);
    y = task_get_int(task, TASK_OFF_TEXT_XY + 4);
    text = (const char*)task->data + 0xd4;
    if (text[0] != '\0') {
        uint32_t width = formatted_text_printf_width(font, "%s", text);
        reward_text_draw(font, pixels, pitch, target->w, target->h,
                         x - (int)(width / 2u), y, "%s", text);
    }
    if (displayed_count < 10u) {
        name = (const char*)task->data + TASK_OFF_NAME;
        if (name[0] != '\0') {
            int label_x = s_reward_screen_state->game->reward_screen.text_pos[0];
            int label_y = s_reward_screen_state->game->reward_screen.text_pos[1];
            uint32_t width = formatted_text_printf_width(font, "%s", name);
            reward_text_draw(font, pixels, pitch, target->w, target->h,
                             label_x - (int)(width / 2u), label_y,
                             "%s", name);
        }
    }
    surface_unlock(target, pixels);
}

static int reward_screen_fast_forward_get(const RewardScreenState* state,
                                          const RewardScreenOps* ops)
{
    const InputState* input;
    if (ops != NULL && ops->fast_forward_pressed != NULL)
        return ops->fast_forward_pressed(ops->context);
    input = input_direct_state_get();
    if (input == NULL) return 0;
    /* DAT_0076abb9/DAT_0076aa39 are the current/previous DIK_SPACE bytes. */
    return (input->cur[0x39] == 0u && input->prev[0x39] != 0u) ||
           (state != NULL && state->pointer_override_active != 0);
}

static int reward_screen_zone_check(RewardScreenState* state,
                                    const RewardScreenOps* ops)
{
    const InputState* input;
    int hit_save = 0;
    int hit_advance = 0;
    if (state == NULL || state->game == NULL) return 0;
    if (ops != NULL && ops->reward_zone_check != NULL)
        return ops->reward_zone_check(ops->context, state);
    input = input_direct_state_get();
    if (input == NULL || input->mouse_press == 0 ||
        !mission_reward_zone(state->game, input->mouse_x, input->mouse_y,
                             &hit_save, &hit_advance)) return 0;
    if (hit_save) {
        (void)save_menu_open_from_reward_bound();
        state->transition_state = 1;
        sound_request_queue_submit(0x29, 1);
        return 1;
    }
    if (hit_advance) {
        checkpoint_step(state->game);
        state->transition_stage = 3;
        return 1;
    }
    return 0;
}

static void checkpoint_reward_group(GameContext* ctx, const char* level_name)
{
    /* FUN_0045fda0 reads every key on each build. Missing values therefore
     * take the retail empty/default parse instead of retaining a previous
     * level's reward configuration. */
    ctx->reward.enable = 0;
    ctx->reward.modifier = 0;
    ctx->reward.timer = LVL_REWARD_TIMER_DEFAULT;
    ctx->reward.display_text = 0;
    ctx->reward.display_images = 0;
    ctx->reward.display_flics = 0;
    if (ctx->cfg == NULL || level_name == NULL) {
        return;
    }
    char path[512];
    /* Reward/Enable tri-state (2 -> 0). Shipped config nests the level
     * under root -> Levels, so resolve each key through the container. */
    if (util_level_cfg_path(ctx->cfg, level_name, "Reward::Enable", path,
                            sizeof(path)) != NULL) {
        int en = block_get_bool_path(ctx->cfg, path);
        if (en == 2) en = 0;
        ctx->reward.enable = en;
    }
    if (util_level_cfg_path(ctx->cfg, level_name, "Reward::Modifier", path,
                            sizeof(path)) != NULL) {
        ctx->reward.modifier = block_get_int(ctx->cfg, path, 0);
    }
    if (util_level_cfg_path(ctx->cfg, level_name, "Reward::Timer", path,
                            sizeof(path)) != NULL) {
        ctx->reward.timer =
            block_get_float(ctx->cfg, path, LVL_REWARD_TIMER_DEFAULT);
    }
    if (util_level_cfg_path(ctx->cfg, level_name, "Reward::DisplayText", path,
                            sizeof(path)) != NULL) {
        int dt = block_get_bool_path(ctx->cfg, path);
        ctx->reward.display_text = dt == 2 ? 1 : dt;
    }
    if (util_level_cfg_path(ctx->cfg, level_name, "Reward::DisplayImages", path,
                            sizeof(path)) != NULL) {
        int di = block_get_bool_path(ctx->cfg, path);
        ctx->reward.display_images = di == 2 ? 1 : di;
    }
    if (util_level_cfg_path(ctx->cfg, level_name, "Reward::DisplayFlics", path,
                            sizeof(path)) != NULL) {
        int df = block_get_bool_path(ctx->cfg, path);
        ctx->reward.display_flics = df == 2 ? 1 : df;
    }
}

static float blob_float_get(const char* blob, size_t off)
{
    float v = 0.0f;
    __builtin_memcpy(&v, blob + off, sizeof v);
    return v;
}

static void blob_float_put(char* blob, size_t off, float v)
{
    __builtin_memcpy(blob + off, &v, sizeof v);
}

/* Corpus fold shape: percent = 100 / den * num rides through only when
 * the denominator reads non-zero. It stores the raw result before the
 * upper clamp, then stores the clamped result; zero denominator stores 100. */
static float blob_percent_fold_store(char* blob, size_t den, size_t num,
                                     size_t out)
{
    float v = 100.0f;
    const float d = blob_float_get(blob, den);
    if (d != 0.0f) {
        v = (100.0f / d) * blob_float_get(blob, num);
    }
    blob_float_put(blob, out, v);
    if (100.0f < v) {
        v = 100.0f;
    }
    blob_float_put(blob, out, v);
    return v;
}

/* Corpus count-up: every round adds 100.0 to the +0x2fac sum (the
 * corpus's subtracted negative — an addition) and the window's last
 * folded percent to the +0x2fb0 carry; the guard float is re-read every
 * round and the counter rides as a converted uint. */
static void blob_count_up(char* blob, size_t guard_off, size_t add_off)
{
    for (uint32_t n = 0u; (float)n < blob_float_get(blob, guard_off); ++n) {
        blob_float_put(blob, 0x2fac, blob_float_get(blob, 0x2fac) + 100.0f);
        blob_float_put(blob, 0x2fb0,
                       blob_float_get(blob, add_off)
                           + blob_float_get(blob, 0x2fb0));
    }
}
