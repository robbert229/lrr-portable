// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-021 Lego.c: the unit's functions in original address order and the
 * state they own, relocated verbatim by tools/relocate_unit.py (ADR 0006).
 * Body hashes: docs/coverage/relocations/GAME-021.csv. */
/* Matching build: LegoObject.c callers call legoobject_unit.c (#575). */
#define LEGORRR_LEGOOBJECT_ALIASES_NO_ADAPTERS
#include "legoobject_port_aliases.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include <limits.h>
#include "game.hpp"
#include "gods98/block.hpp"
#include "gods98/engine.hpp"
#include "gods98/media.hpp"
#include "effects.hpp"
#include "gods98/heap.hpp"
#include "gods98/message.hpp"
#include "gods98/record.hpp"
#include "gods98/resource.hpp"
#include "gods98/sound.hpp"
#include "gods98/simtick.hpp"
#include "gods98/string.hpp"
#include "gods98/surface.hpp"
#include "crt/crt.hpp"
#include "gods98/view.hpp"
#include "gods98/camera.hpp"
#include "terrain_grid.hpp"
#include "entity.hpp"
#include "type_config_ports.hpp"
#include "ai.hpp"
#include "cmd.hpp"
#include "upgrades.hpp"
#include "spawn.hpp"
#include "port_loop_ports.hpp"
#include "frontend.hpp"
#include "level.hpp"
#include "objective.hpp"
#include "util.hpp"
#include "gods98/flic.hpp"
#include "select.hpp"
#include "scene_render.hpp"
#include "tutorial_hud_ports.hpp"
/* GAME-026's Map3D_IsInsideDimensions (FUN_004502c0) is defined in the C
 * unit terrain_map.c. This first declaration gives it C linkage; the frozen
 * level headers below redeclare it and inherit that linkage (finding 556). */
extern "C" int level_coord_pair_within_bounds_target(
    const uint32_t* dimensions, uint32_t cx, uint32_t cy);
#include "level_shared.hpp"
#include <float.h>
#include <ctype.h>
#include <errno.h>
#include "gods98/texture.hpp"
#include "building_scene.hpp"
#include "actor_scene.hpp"
#include "mission.hpp"
#include "cmd_shared.hpp"
#include "entity_pose_delta.hpp"
#include "entity_motion_ports.hpp"
#include "activity_timeout.hpp"
#include "entity_motion_probe.hpp"
#include "simulation_ui_ports.hpp"
#include "type_motion_scale.hpp"
#include "entity_quad_collision.hpp"
#include "weapon.hpp"
#include "gods98/model.hpp"
#include "gods98/entity.hpp"
#include "entity_config_shared.hpp"
#include <stdarg.h>
#include <time.h>
#include "frontend.h"
#include "terrain_feedback.hpp"
#include "savegame.hpp"
#include "gods98/audio.hpp"
#include "gods98/input.hpp"
#include "priority_config_ports.hpp"
#include "ui_action_dispatch.hpp"
#include "camera_drag.hpp"
#include "screen_pick.hpp"
#include "pointer_cursor.hpp"
#include "ui_list.hpp"
#include "frontend_shared.hpp"
#include "render.hpp"
#include "gods98/scene_resources.hpp"
#include <locale.h>
#include "frameloop.hpp"
#include "mainsim.hpp"
#include "tickers.hpp"
#include "scene_pool.hpp"
#include "engine_shared.hpp"
#include <stddef.h>
#include "platform/platform.h"
#include "message_shared.hpp"
#include "movie.hpp"
#include "simtick_shared.hpp"
#include "view_shared.hpp"
/* C linkage for the legoGlobs block-update lists that GAME-021's
 * lego_unit.c reads (issue #574, defined in level.cpp). The frozen header
 * below redeclares them and inherits it. */
extern "C" int32_t s_cell_refresh_track[][2];
extern "C" int s_cell_refresh_track_count;
extern "C" int32_t s_cell_previous_track[][2];
extern "C" int s_cell_previous_track_count;
extern "C" int32_t s_cell_visited_track[][2];
extern "C" int s_cell_visited_track_count;
/* GAME-021's lego_unit.c reads the surface type descriptions (issue #587):
 * C linkage, which the frozen header below and the definitions inherit. */
extern "C" const char* s_surface_type_names[];
extern "C" uint32_t s_surface_type_values[];
#include "level_for_lego.hpp"
/* C linkage for a legoGlobs field that GAME-021's lego_unit.c reads (issue
 * #544). The frozen header below redeclares it and inherits it. */
extern "C" float s_global_float_00556f00;
#include "crt/crt_for_lego.hpp"
extern "C" void* s_upgrade_records; /* lego_unit.c (C) reads it */
#include "cmd_for_lego.hpp"
#include "entity_for_lego.hpp"
#include "entity_config_for_lego.hpp"
#include "frontend_for_lego.hpp"
/* GAME-021 lego_unit.c writes this word (C linkage, ADR 0005 step 6). */
namespace unported_ledger_stubs {
extern "C" uint32_t s_start_level_pilot_grid_count;
}
#include "spawn_for_lego.hpp"
#include "terrain_grid_for_lego.hpp"
#include "upgrades_for_lego.hpp"
#include "util_for_lego.hpp"
#include "gods98/camera_for_lego.hpp"
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
extern float s_panel_highlight_timer;
}
#include "gods98/engine_for_lego.hpp"
#include "gods98/media_for_lego.hpp"
#include "gods98/message_for_lego.hpp"
#include "gods98/resource_for_lego.hpp"
#include "gods98/simtick_for_lego.hpp"
#include "gods98/view_for_lego.hpp"
#include "ai_from_lego.hpp"
#include "entity_from_lego.hpp"

#define LVL_OXYGEN_SCALE  0.001f      /* LevelLoad converts OxygenRate to a per-tick rate. */

/* Cheat-word bits (DAT_00556ed0) set by LevelLoad (finding 07/config): kept
 * accessible for the frame loop but not written by the load itself (the
 * corpus's DAT_00556ed0 is modelled in GameContext). */
#define LVL_OFF_BOULDER_ANIM 0x274   /* +0x9d */
#define LVL_OFF_INIT_CRYS    0x130   /* +0x4c scratch from post-load init */
#define LVL_OFF_INIT_ORE     0x134   /* +0x4d scratch from post-load init */
#define LVL_OFF_UPGRADE_TIMES 0x210  /* +0x84, twenty float slots */
#define LVL_OFF_VEHICLE_UPGRADE_TIME 0x214 /* +0x85 */
#define LVL_OFF_MINIFIG_UPGRADE_TIME 0x218 /* +0x86 */
#define LVL_OFF_BUILDING_UPGRADE_TIME 0x220 /* +0x88 */
#define LVL_OFF_TRAIN_TIME 0x260     /* +0x98 */

/* `.ol` `type` name -> entity table code — DECOMPILE-VERIFIED (finding 07/10).
 *
 * The corpus resolver `FUN_0042e8b0` (ObjectTypeByName) matches the name
 * case-insensitively (__strcmpi) against the four runtime name registries —
 * populated from the shipped `Lego.cfg` `VehicleTypes` (FUN_0042cda0),
 * `RockMonsterTypes` (FUN_0042d100, the type-3 general/creature table),
 * `MiniFigureTypes` and `BuildingTypes` (parallel inits) — and then against
 * a hardcoded special-object table:
 *
 *   1 = VehicleTypes      (Hoverboard, SmallDigger, … 12)
 *   2 = MiniFigureTypes   (Pilot)
 *   3 = RockMonsterTypes  (RockMonster, TinyRM, TinyIM, SmallSpider, Bat,
 *                          Slug, LavaMonster, IceMonster)
 *   4 = BuildingTypes     (Toolstation, TeleportPad, … Geo-dome, … 10)
 *   5..19 = special objects (hardcoded in FUN_0042e8b0):
 *          Boulder=5, PowerCrystal=6, Ore=7(idx 0), ProcessedOre=7(idx 1),
 *          Dynamite=8, Barrier=9, ElectricFence=11, SpiderWeb=12, OohScary=13,
 *          Path=15, Pusher=16, Freezer=17, LaserShot=19
 *   -1 = TVCamera (the .ol loader's camera branch, no entity)
 *   0  = not registered (the corpus aborts the load; the port skips, see
 *        object_list_load_file)
 *
 * The special-entity codes are stored in the port's shared table index 4
 * (ENTITY_TYPE_SPECIAL); the corpus keeps one singleton slot each
 * (DAT_00556368..b0). */
typedef struct NameCode { const char* name; int code; } NameCode;

namespace unported_ledger_stubs {
typedef struct MiscResourceSpec {
    const char* name;
    int kind;
    int fallback_kind;
    int load_flag;
} MiscResourceSpec;
} // namespace unported_ledger_stubs

/* PanelSelectionHighlightToggle compares against this file-backed .rdata
 * dword (the PE bytes at 0x0049e2b8 are 00 00 00 00, +0.0f). Volatile
 * preserves the corpus's explicit memory comparison instead of folding it
 * to an immediate floating-point constant. */
/* Not annotated: 0049e2b8 is the original's pooled 0.0f constant, which
 * the original-shape functions in lego_unit.c reach as a literal. */
static const volatile float s_panel_highlight_zero = 0.0f;

/* Not annotated: 004a3f40 is Lego.c's "SpiderWeb" literal, which
 * lego_unit.c marks and SpiderWeb.c (spiderweb.c) reuses (finding 620). */
const char s_item_name_spider_web[] = "SpiderWeb";
/* Not annotated: 004a3f90, 004a3fa0, 004a3fb0, 004a3fb8, 004a3fc4,
 * 004a46dc and 004a46f8 are Lego.c's own literals ("ElectricFence",
 * "ProcessedOre", "Barrier", "OohScary", "Dynamite", "tvcamera",
 * "Activity_Stand"). lego_unit.c marks them, so every unit's use of them
 * is labelled as the literal (finding 626). */
const char s_item_name_electric_fence[] = "ElectricFence";
const char s_item_name_processed_ore[] = "ProcessedOre";
const char s_item_name_barrier[] = "Barrier";
const char s_item_name_ooh_scary[] = "OohScary";
const char s_item_name_dynamite[] = "Dynamite";
const char s_item_name_tv_camera[] = "tvcamera";

const char s_activity_name_activity_stand[] = "Activity_Stand";

/* 004de424 (the last left-button state) is lego_unit.c's s_lego_mslbLast. */

/* --- the 0x425-0x426 engine-state band (see engine.hpp) ---------------------
 * The two threaded one-liners are the corpus's own reads of the context-mode
 * and cheat-word globals; the two DAT-global shapes land as zero-initialized
 * module storage per the unowned-global convention (the triple's only corpus
 * writer is the unported renderer-side sim-interaction update). The triple is
 * three individually-addressed dwords in the corpus, so it carves into one
 * annotated static per dword — reccmp resolves whole objects only and an
 * array's tail elements would render as (OFFSET) fragments. */
extern "C" int s_scroll_guard; /* DAT_004de548, defined in lego_unit.c */

extern "C" uint32_t s_neighbor_pick_rotation; /* DAT_004de588, defined in lego_unit.c */
/* Config key names (labels confirmed in finding 07/22 or the corpus). */
static const char* K_BOULDER_ANIM   = "BoulderAnimation";
static const char* K_FALLIN_MULT    = "FallinMultiplier";
static const char* K_BLOCKSIZE      = "blocksize";
static const char* K_DIGDEPTH       = "digdepth";
static const char* K_USEROOF        = "useroof";
static const char* K_ROOFHEIGHT     = "roofheight";
static const char* K_ROUGHLEVEL     = "roughlevel";
static const char* K_SURFACEMAP     = "SurfaceMap";
static const char* K_PREDUGMAP      = "PredugMap";
static const char* K_TERRAINMAP     = "TerrainMap";
static const char* K_BLOCKPTRMAP    = "BlockPointersMap";
static const char* K_CRYOREMAP      = "CryOreMap";
static const char* K_ERODEMAP       = "ErodeMap";
static const char* K_EMERGEMAP      = "EmergeMap";
static const char* K_AIMAP          = "AIMap";
static const char* K_PATHMAP        = "PathMap";
static const char* K_FALLINMAP      = "FallinMap";
static const char* K_TEXTURESET     = "textureset";
static const char* K_EMERGE_TIMEOUT = "EmergeTimeOut";
static const char* K_EMERGE_CREATURE = "EmergeCreature";
static const char* K_SELBOXH        = "selboxheight";

static const char* K_SAFECAVERNS    = "SafeCaverns";
static const char* K_STARTFP        = "StartFP";
static const char* K_NODRAIN        = "NoDrain";
static const char* K_OXYGENRATE     = "OxygenRate";
static const char* K_UPGRADETIME    = "UpgradeTime";
static const char* K_MINIFIG_UPGRADE_TIME = "MinifigureUpgradeTime";
static const char* K_VEHICLE_UPGRADE_TIME = "VehicleUpgradeTime";
static const char* K_BUILDING_UPGRADE_TIME = "BuildingUpgradeTime";
static const char* K_NO_MULTI_SELECT = "NoMultiSelect";
static const char* K_NO_AUTO_EAT     = "NoAutoEat";
static const char* K_RECALL_OL_OBJECTS = "RecallOLObjects";
static const char* K_GENERATE_SPIDERS = "GenerateSpiders";
static const char* K_DISABLE_TOOLTIP_SOUND = "DisableToolTipSound";
static const char* K_DISABLE_START_TELEPORT = "DisableStartTeleport";
static const char* K_TRAIN_TIME     = "TrainTime";
static const char* K_BUILDTOL       = "BuildingTolerance";
static const char* K_BUILDMAXVAR    = "BuildingMaxVariation";
static const char* K_NEXTLEVEL      = "nextlevel";
static const char* K_FULLNAME       = "FullName";
static const char* K_INITCRYSTALS   = "InitialCrystals";
static const char* K_INITORE        = "InitialOre";
static const char* K_ERODETRIGGER   = "ErodeTriggerTime";
static const char* K_ERODETIME      = "ErodeErodeTime";
static const char* K_ERODELOCK      = "ErodeLockTime";
static const char* K_LANDSLIDECOUNT = "NumberOfLandSlidesTillCaveIn";

/* These are the keys initialized by GameConfigInit at DAT_00556290. The
 * second token in each SurfaceTypeDescriptions value is a sound alias; the
 * first token is escape-decoded localized text. */
static const char* const s_surface_type_config_keys[SURFACE_TYPE_SLOTS] = {
    "Lego_SurfaceType_Tunnel", "Lego_SurfaceType_Immovable",
    "Lego_SurfaceType_Hard", "Lego_SurfaceType_Medium",
    "Lego_SurfaceType_Loose", "Lego_SurfaceType_Soil",
    "Lego_SurfaceType_Lava", "Lego_SurfaceType_Water",
    "Lego_SurfaceType_OreSeam", "Lego_SurfaceType_Lake",
    "Lego_SurfaceType_CrystalSeam", "Lego_SurfaceType_RechargeSeam",
    "Lego_SurfaceType_Rubble", "Lego_SurfaceType_Reinforcement",
    "Lego_SurfaceType_Path", "Lego_SurfaceType_SlugHole",
    "Lego_SurfaceType_Undiscovered", "Lego_SurfaceType_Cavern"
};

static const NameCode s_vehicle_names[] = {
    { "hoverboard", 1 }, { "smalldigger", 1 }, { "smalltruck", 1 },
    { "smallcat", 1 },   { "smallmlp", 1 },    { "smallheli", 1 },
    { "bulldozer", 1 },  { "walkerdigger", 1 }, { "largemlp", 1 },
    { "largedigger", 1 }, { "largecat", 1 },   { "largeheli", 1 },
};
static const NameCode s_minifig_names[] = {
    { "pilot", 2 },
};
static const NameCode s_creature_names[] = {
    { "rockmonster", 3 }, { "tinyrm", 3 }, { "tinyim", 3 },
    { "smallspider", 3 }, { "bat", 3 },    { "slug", 3 },
    { "lavamonster", 3 }, { "icemonster", 3 },
};
static const NameCode s_building_names[] = {
    { "toolstation", 4 }, { "teleportpad", 4 }, { "docks", 4 },
    { "powerstation", 4 }, { "barracks", 4 },   { "upgrade", 4 },
    { "geo-dome", 4 },    { "orerefinery", 4 }, { "gunstation", 4 },
    { "teleportbig", 4 },
};
static const NameCode s_special_names[] = {
    { "boulder", 5 },      { "powercrystal", 6 },  { "ore", 7 },
    { "processedore", 7 }, { "dynamite", 8 },      { "barrier", 9 },
    { "electricfence", 11 }, { "spiderweb", 12 },  { "oohscary", 13 },
    { "path", 15 },        { "pusher", 16 },       { "freezer", 17 },
    { "lasershot", 19 },
};

#if defined(_MSC_VER) && defined(_M_IX86)
static const char s_entity_rock_fall_in_name[] = "RockFallIn";
#endif

static float s_debug_panel_text_timer;
static int s_debug_panel_text_alternate;

static EngineContext* s_engine_pre_tick_context;

/* VC5 rand.obj supplies the shared stream in the classic x86 build; other
 * builds keep this subsystem's local LCG model. */
#if !defined(_MSC_VER) || (_MSC_VER >= 1200) || !defined(_M_IX86)
static uint32_t s_cd_rand_state = 1;
#endif

/* shared tranche-10 module state */
static int   s_mouse_anchor_x = 0, s_mouse_anchor_y = 0;

LevelState* level_current_owner_get(void);

namespace frontend_config {
void panels_config_entry_apply(BlockFile* config, int display_width,
                               int display_height);
void panel_button_layout_config_load(BlockFile* config, int display_width,
                                     int display_height);
void text_messages_config_load(BlockFile* config);
} // namespace frontend_config

namespace unported_ledger_stubs {
int ledger_stub_00449700_RouteStepCommandIssue(EntitySlot* entity);
} // namespace unported_ledger_stubs

void engine_global_state_flags_bit7_set_target(int value);
void engine_global_state_flags_bits6_and_9_set_target(
    int value, int set_bit9);

/* Defined below (forward declarations added by tools/relocate_unit.py). */
static int engine_game_config_init_callback(void);
static void engine_registered_shutdown_loader_callback(void);
static void engine_registered_shutdown_normal_callback(void);
static int engine_registered_tick_callback(float dt);
static int m_edge(EngineContext* ctx, int key);
static int m_held(EngineContext* ctx, int key);
static void m_toggle_hud(EngineContext* ctx, uint32_t* flag, uint32_t bit, const char* label);
static int periodic_entity_callback_adapter(EntitySlot* entity, void* payload);
static void spawn_issue(int source_object, uint32_t type);
static int spawn_probe(int first, int second);
namespace lego { namespace { int prefix_equal(const char* value, const char* prefix); } }
namespace unported_ledger_stubs { static uint32_t terrain_feedback_message_cue_get(void* context, int message_id); }
namespace unported_ledger_stubs { static void terrain_feedback_message_cue_set(void* context, int message_group, uint32_t cue); }
namespace unported_ledger_stubs { static void terrain_feedback_message_update(void* context, int message_group); }
static uint32_t media_float_bits(float value);
static unsigned s_cd_draw_slot(void);
static void s_cd_rearm(void);
static void camera_predug_spawn_state_seed(LevelState* level);
static float cfg_float(BlockFile* cfg, const char* level, const char* key, float dflt);
static long cfg_int(BlockFile* cfg, const char* level, const char* key, long dflt);
static const char* cfg_string(BlockFile* cfg, const char* level, const char* key, char* out, size_t out_sz);
static BlockRec* level_config_block_find(BlockFile* cfg, const char* level);
static int level_identifier_remap_table_load(const char* path, const char* root_name);
static int map_bias_name(BlockFile* cfg, const char* level, const char* key, char* out, size_t out_sz);
static int map_pipeline(LevelState* s, const char** names, const int* biases, const char* texture_set_name);
static void cell_init_all(LevelState* s);
static void level_field_grid_buffer_replace(LevelState* level);
static int load_block_pointers(LevelState* s, const char* name);
static int load_emerge(LevelState* s, const char* name);
static int load_erode_cryore(LevelState* s, const char* name, int bias);
static int load_fallin(LevelState* s, const char* name, int bias);
static int load_layer_l(LevelState* s, const char* name);
static int load_layer_m(LevelState* s, const char* name);
static int load_path_ai(LevelState* s, const char* name, int bias);
static int load_predug(LevelState* s, const char* name, int bias);
static int load_terrain_map(LevelState* s, const char* name, int bias);
static int dims_ok(const MapFile* m, const LevelState* s);
static void predug_wall_cell(LevelState* s, int x, int y);
static void terrain_activity_build_record_update(LevelState* s, int x, int y, int enable);
static int grid_spawn(uint32_t* coords, uint32_t hint);
namespace lego { namespace { void upgrade_type_tables_release(void); } }
static const char* obj_val(BlockFile* bf, const char* root, const char* obj, const char* key);
static void ol_strncpy(char* dst, size_t n, const char* src);
static int entity_type_config_name_find(uint32_t table_kind, const char* name, int type, int* row_out);
static CellTransientEffectPort* cell_transient_effect_port_ensure( LevelState* level, void* record);
static void cell_transient_effect_set(CellTransientEffectPort* port, void* emitter);
static void terrain_cell_orientation_set_level(LevelState* level, int x, int y, int flipped);
static void terrain_level_vertex_offset(LevelState* level, int vertex_x, int vertex_y, float delta);
static uint32_t cell_secondary_word_unchecked(const LevelState* s, int cx, int cy);
static float entity_cell_speed_indexed_scale_get(int type, const void* entity_record);
static uint32_t entity_cell_speed_rock_fall_name_find(const char* name);
static int terrain_cell_mutation_progress_level(LevelState* level, TerrainSurfaceGrid* surface, int x, int y);
namespace frontend_config { static int ledger_config_split_limited(char* text, char** fields, unsigned int field_limit, const char* separator); }
static void advisor_tooltip_text_apply(int index, const char* value);
static void surface_type_config_path_build(char* path, size_t capacity, const char* root, const char* key);
static void fe_menu_panel_text_draw(void* px, int pitch, int w, int h, const FeMenuPanelText* item, const TextureSurface* font);
static int control_probe(uint32_t mask);

/* The retail entries below operate on DAT_005560d4 directly. Keep the
 * explicit-level variants separate for portable callers with owned state. */




/* FUN_0041f8e0: empty body; the corpus's callable no-op (finding 381). */

/* UpgradeResourceCheck (FUN_0041f900, finding 475): the level's resource
 * counters — +0xa0 for selector 0, +0xac for any nonzero selector; and
 * zero when no level object is active (the corpus's own DAT-null rule,
 * which doubles as the port's NULL guard). */
/* Port model of 0x0041f900 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x0041f900, retires to the tail in GAME-021 part 3
uint32_t upgrade_resource_check(const LevelState* s, int selector)
{
    if (s == NULL) {
        return 0u;
    }
    return (uint32_t)level_int(s, selector != 0 ? LEVEL_OFF_RES_B
                                               : LEVEL_OFF_RES_A);
}


/* RuntimeFlag40000000Get (FUN_0041f970): the faithful masked read of the
 * port's engine-state word. */
/* Port model of 0x0041f970 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x0041f970, retires to the tail in GAME-021 part 3
uint32_t runtime_flag40000000_get(const EngineContext* ctx)
{
    return ctx != NULL ? (ctx->state & 0x40000000u) : 0u;
}



/* --- resource/ore counter pair (2026-10-25) --------------------------------
 * The counter half runs under the corpus's current-level gate; the
 * overlay advance half runs outside it (corpus order), so the guarded
 * no-level path keeps stepping the float. */
/* Port model of 0x0041f9a0 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x0041f9a0, retires to the tail in GAME-021 part 3
void level_resource_balance_add_and_sound(int selector)
{
    if (s_ctx != NULL && s_ctx->level != NULL) {
        LevelState* lvl = (LevelState*)s_ctx->level;
        if (selector == 0) {
            level_set_int(lvl, LEVEL_OFF_RES_A,
                          level_int(lvl, LEVEL_OFF_RES_A) + 1);
        } else {
            level_set_int(lvl, LEVEL_OFF_RES_B,
                          level_int(lvl, LEVEL_OFF_RES_B) + 1);
        }
    }
    ui_overlay_animation_advance(1, selector != 0 ? 5 : 1);
}

/* UpgradeResourceDebit (FUN_0041f9e0) — the debit half of the counter pair.
 * Corpus order: the counter store runs under the level gate, then the
 * overlay advance runs outside it. Pin: the debit half steps the advance
 * with mode 0 (the add half is mode 1), and a nonzero selector steps the
 * constant 5 regardless of the amount — selector 0 steps the amount.
 * Deviation: the corpus's DAT-null gate is the port's ctx/level guard. */
/* Port model of 0x0041f9e0 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x0041f9e0, retires to the tail in GAME-021 part 3
void upgrade_resource_debit(int selector, int amount)
{
    if (s_ctx != NULL && s_ctx->level != NULL) {
        LevelState* lvl = (LevelState*)s_ctx->level;
        if (selector == 0) {
            level_set_int(lvl, LEVEL_OFF_RES_A,
                          level_int(lvl, LEVEL_OFF_RES_A) - amount);
        } else {
            level_set_int(lvl, LEVEL_OFF_RES_B,
                          level_int(lvl, LEVEL_OFF_RES_B) - amount);
        }
    }
    ui_overlay_animation_advance(0, selector != 0 ? 5 : amount);
}

/* GameConfigCallbackRegistryInit (FUN_0041fa20) clears the reference's
 * 0x3be-dword state block, stores the root name, sets the window title, and
 * installs init/tick/shutdown callbacks. The reset and game-config callbacks
 * are explicit seams because their state and implementation are not owned by
 * this engine module. The adapters below match the callback signatures used
 * by the frame loop; the context-taking EngineTick/EngineShutdown functions
 * themselves are never published as incompatible raw callbacks. */
/* Port model of 0x0041fa20 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x0041fa20, retires to the tail in GAME-021 part 3
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

namespace unported_ledger_stubs {
/* Port model of 0x0041fa80 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x0041fa80, retires to the tail in GAME-021 part 3
void ledger_stub_0041fa80_StartLevelPilots(void)
{
    GameContext* ctx = entity_pool_callback_context_get();
    uint32_t* engine_state = engine_current_state_word();
    uint32_t selector[4];
    uint32_t panel_flags;

    /* DAT_00556ecc is an engine-owned global in the reference. If the port
     * has no active engine context, there is no corresponding state word to
     * test or update, so this initialization cannot run faithfully. */
    if (engine_state == NULL) return;

    /* FUN_00431560's current-level getter is unconditional and its returned
     * pointer is discarded.  The port has no side effect to reproduce. */
    if ((*engine_state & 0x08000000u) != 0u) return;

    /* FUN_00455300("Pilot", 0) resolves the catalog row and reads its
     * slot-zero grid count.  GridRecordCountGet is the same state table in
     * the port; the type/row resolver is intentionally not exposed here. */
    s_start_level_pilot_grid_count =
        entity_type_grid_record_count_get("Pilot", 0);
    *engine_state |= 0x08000000u;
    level_state_stored_value_set(1u);
    lego::scatter_spawn_gate_set(1);

    /* The decompile passes selector, mode=1, flags=1 for each of these.
     * Its per-entity callback body remains unresolved in the checked-in
     * corpus; the dispatch helper's known state path is still invoked. */
    selector[0] = 1u;
    selector[1] = 2u;
    selector[2] = 8u;
    selector[3] = 0x400u;
    scene_object_state_dispatch(ctx, selector[0], 1u, 1u, NULL, NULL);
    scene_object_state_dispatch(ctx, selector[1], 1u, 1u, NULL, NULL);
    scene_object_state_dispatch(ctx, selector[2], 1u, 1u, NULL, NULL);
    scene_object_state_dispatch(ctx, selector[3], 1u, 1u, NULL, NULL);
    ledger_stub_00425c30_TransitionControl(0, 0u);
    selection_clear_request_and_hud_reset();

    /* DAT_0050013c is the first request/panel record in the portable CRT
     * window.  Only its observed bit-1 gate controls the two toggles. */
    panel_flags = request_record_flags_peek(0);
    if ((panel_flags & 2u) != 0u) {
        panel_state_flags_toggle(0);
        panel_state_flags_toggle(1);
    }
}
} // namespace unported_ledger_stubs

/* TimeScaleGet @ 0x0041fb40 returns the shared scroll/time scale global. */

namespace unported_ledger_stubs {
/* Port model of 0x00422850 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x00422850, retires to the tail in GAME-021 part 3
void misc_object_resources_load(void* config)
{
    static const MiscResourceSpec specs[22] = {
        { "Boulder", 1, 2, 1 },
        { "BoulderExplodeIce", 3, -1, 1 },
        { "BoulderExplode", 3, -1, 1 },
        { "SmashPath", 3, -1, 1 },
        { "Explosion", 5, -1, 1 },
        { "Crystal", 1, 2, 1 },
        { "Dynamite", 4, -1, 1 },
        { "OohScary", 4, -1, 1 },
        { "Barrier", 4, -1, 1 },
        { "Ore", 1, 2, 1 },
        { "ProcessedOre", 1, 2, 1 },
        { "ElectricFence", 1, -1, 1 },
        { "ElectricFenceStud", 3, -1, 1 },
        { "ShortElectricFenceBeam", 6, -1, 0 },
        { "LongElectricFenceBeam", 7, -1, 1 },
        { "SpiderWeb", 4, -1, 1 },
        { "RechargeSparkle", 3, -1, 1 },
        { "MiniTeleportUp", 3, -1, 0 },
        { "IceCube", 4, -1, 1 },
        { "Pusher", 3, -1, 1 },
        { "Freezer", 3, -1, 1 },
        { "LaserShot", 3, -1, 1 }
    };
    unsigned int i;
    memset(s_misc_resource_handles, 0, sizeof(s_misc_resource_handles));
    if (s_misc_resource_hooks.load != NULL) {
        for (i = 0u; i < 22u; ++i) {
            void* handle = s_misc_resource_hooks.load(config, "MiscObjects",
                specs[i].name, specs[i].kind, specs[i].load_flag,
                s_misc_resource_hooks.user);
            if (handle == NULL && specs[i].fallback_kind >= 0) {
                handle = s_misc_resource_hooks.load(config, "MiscObjects",
                    specs[i].name, specs[i].fallback_kind,
                    specs[i].load_flag, s_misc_resource_hooks.user);
            }
            s_misc_resource_handles[i] = handle;
            if (handle != NULL && s_misc_resource_hooks.enable != NULL)
                s_misc_resource_hooks.enable(handle,
                                             s_misc_resource_hooks.user);
        }
    }
    if (s_misc_resource_hooks.post_load != NULL)
        s_misc_resource_hooks.post_load(config, s_misc_resource_hooks.user);
}
} // namespace unported_ledger_stubs

/* ModeDependentField24Get (FUN_00423080): mode-selected follow-span read.
 * The corpus indexes views[ctx->context_mode == 1 ? 0 : 2]; other modes
 * return the mode word verbatim, and the port reads the ViewContext's
 * follow_span slot (the corpus's word 0x24) through the threaded view
 * table (deviation: NULL slots answer zero, corpus dereferences blindly).
 */
/* Port model of 0x00423080 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x00423080, retires to the tail in GAME-021 part 3
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


/* DebugPanelTextDrawUpdate (FUN_004230c0): pulse the alternate palette every
 * 12.5 seconds, resolve an empty string from the active selection, clamp its
 * horizontal placement, and draw it centered vertically on its font row. */
/* Port model of 0x004230c0 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x004230c0, retires to the tail in GAME-021 part 3
void debug_panel_text_draw_update(float elapsed, int screen_width,
                                  const char* text)
{
    int x = 0;
    int y = 0;
    int font_height = 0;
    int text_width = 0;
    int scroll_width = 0;
    s_debug_panel_text_timer -= elapsed;
    if (s_debug_panel_text_timer <= 0.0f) {
        s_debug_panel_text_timer += 12.5f;
        s_debug_panel_text_alternate =
            (s_debug_panel_text_alternate == 0) ? 1 : 0;
    }
    if (text == NULL) return;
    if (text[0] == '\0' &&
        s_debug_panel_text_ops.fallback_text_get != NULL) {
        text = s_debug_panel_text_ops.fallback_text_get(
            s_debug_panel_text_ops.context);
        if (text == NULL) return;
    }
    if (s_debug_panel_text_ops.origin_get != NULL) {
        s_debug_panel_text_ops.origin_get(s_debug_panel_text_ops.context,
                                          &x, &y);
    }
    if (s_debug_panel_text_ops.font_line_height != NULL) {
        font_height = s_debug_panel_text_ops.font_line_height(
            s_debug_panel_text_ops.context);
    }
    if (s_debug_panel_text_ops.text_width != NULL) {
        text_width = s_debug_panel_text_ops.text_width(
            s_debug_panel_text_ops.context, text);
    }
    if (s_debug_panel_text_ops.scroll_width != NULL) {
        scroll_width = s_debug_panel_text_ops.scroll_width(
            s_debug_panel_text_ops.context, 0x22u);
    }
    if (screen_width - 40 < x + text_width) x -= text_width;
    if (s_debug_panel_text_ops.text_draw != NULL) {
        int offset = (s_debug_panel_text_alternate == 0) ? scroll_width : 0;
        s_debug_panel_text_ops.text_draw(
            s_debug_panel_text_ops.context, x + offset,
            y - font_height / 2, s_debug_panel_text_alternate, text);
    }
}

/* Retail-shaped EnginePreTick @ 0x4231f0 uses global input/text state. */
/* Port model of 0x004231f0 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x004231f0, retires to the tail in GAME-021 part 3
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
/* Frame tick — corpus EngineTick @ 0x4232e0. The numbered structure below
 * mirrors finding 03 §"Tick structure" and the first 130 lines of the corpus.
 * Every subsystem ticker is a registered stub in tickers.cpp whose function
 * body is filled in from the corpus one at a time; the *order* here is the
 * observed call order and must stay stable.
 */
/* Port model of 0x004232e0 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x004232e0, retires to the tail in GAME-021 part 3
int EngineTick(EngineContext* ctx, float dt)
{
    engine_current_context_bind(ctx);
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
    const int camera_force_scale_one =
        (ctx->state & ENGINE_STATE_FORCE_SCALE_1) != 0;
    if (camera_force_scale_one) {
        ctx->state &= ~ENGINE_STATE_FORCE_SCALE_1;
        scale = 1.0f;
    }

    /* 4. Movement/grid cluster; the primary camera's world-target consumer
     * follows below, independently of these entity/grid operations. */
    camera_cluster(ctx, dt, scale);

    /* EngineTick's primary world-target consumer uses reference frame
     * units. AI and the remaining simulation continue to receive seconds. */
    float camera_delta = view_camera_delta_from_seconds(dt);
    view_primary_target_tick(ctx, camera_delta);

    /* 5. Main simulation step — 0 => quit frame (MainSimStep @ 0x4250c0,
     *    ported in mainsim.*). */
    uint32_t sim_out1 = 0, sim_out2 = 0, sim_out3 = 0;
    if (main_sim_step(ctx, scale, &sim_out1, &sim_out2, &sim_out3) == 0) {
        return 0;
    }

    /* 6. The ordered subsystem-ticker sequence (finding 03; corpus lines
     *    93-119). Only the first entry is gated on !TIME_FROZEN at the call
     *    site; that gate lives in the table (see TICKERS[0].skip_when). */
    if (!EngineSubsystemTickersWithViews(ctx, dt, scale, camera_delta,
            dt > 0.0f ? (camera_force_scale_one ? 1.0f :
                (scale / dt) * camera_delta) : 0.0f)) {
        fprintf(stderr, "engine: camera terrain collection owner unavailable\n");
        terrain_camera_detail_restore((LevelState*)ctx->level_block);
        return 0;
    }

    /* 7. Indexed manager dispatch (corpus ManagerTick FUN_0045a730) — enabled
     *    under ENGINE_STATE_INDEXED_SUBSYS; index list observed: 1,2,4,9,6,8,
     *    0xb,5,7,10 interleaved with the named manager tickers. Stub table. */
    if ((ctx->state & ENGINE_STATE_INDEXED_SUBSYS) != 0) {
        /* Indexed UI timers use the reference 25-Hz frame unit. */
        EngineManagerDispatch(ctx, dt * 25.0f);
    }

    /* 8. Render begin/end pairs, world-draw vs editor pass, and the
     *    &2/&0x2000-gated in-game render pass (corpus lines 130-215). Stub. */
    EngineRenderFrame(ctx, dt, scale, &sim_out1);
    if (!simtick_camera_detail_collection_ready()) {
        fprintf(stderr, "engine: secondary terrain collection owner unavailable\n");
        terrain_camera_detail_restore((LevelState*)ctx->level_block);
        return 0;
    }
    if (s_world_arrow_capture_hook != NULL)
        s_world_arrow_capture_hook(ctx, dt, s_world_arrow_capture_context);

    /* 9. Air-supply beat mechanic (corpus tail): fires the air-low/empty
     *    notifications and loops SND_AirBeat; writes air_prev back. */
    EngineAirBeat(ctx, scale);

    media_cd_audio_watchdog_tick((int)(ctx->state & MEDIA_CD_WATCHDOG_BIT));

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

/* AirSupplyAmountUpdate (FUN_00424560, finding 317): see level.hpp. The
 * stats read lands before the gate (corpus order); the corpus's float10
 * accumulation is modelled with the port's double — the documented
 * extended-precision deviation. */
/* Port model of 0x00424560 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x00424560, retires to the tail in GAME-021 part 3
void air_supply_amount_update(LevelState* s, const struct EntitySlot* e,
                              float delta)
{
    if (s == NULL || e == NULL) {
        return; /* deviation: the corpus strides both blindly */
    }
    const double stats = (double)entity_stats_float_atdc(e);
    if (e->type == 4 && (e->flags_3e8 & 0x20000000u) == 0u) {
        return;
    }
    const double rate = (double)level_float(s, LEVEL_OFF_OXYGEN_RATE);
    const double acc = (double)level_float(s, LEVEL_OFF_AIR_SUPPLY);
    const double next = (double)delta * 0.04 * rate * stats + acc;
    float stored = (float)next;
    if (next > 100.0) {
        stored = 100.0f;
    }
    level_set_float(s, LEVEL_OFF_AIR_SUPPLY, stored);
    if (stored <= 0.0f) {
        level_set_float(s, LEVEL_OFF_AIR_SUPPLY, 0.0f);
        if ((system_state_flags_get() & 0x08000000u) == 0u) {
            objective_announce(s, 2);
        }
    }
}

/* Port model of 0x00424730 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x00424730, retires to the tail in GAME-021 part 3
void sine_phase_midpoint_blend_dispatch(void* object, float delta)
{
    SinePhaseBlendState* state = &s_sine_phase_blend_state;
    SinePhaseBlendHooks* hooks = &s_sine_phase_blend_hooks;
    float color[3];
    float half_difference;
    float oscillation;
    unsigned int i;
    if ((state->flags & 0x8000u) == 0u) return;
    if (hooks->object_prepare != NULL) {
        hooks->object_prepare(hooks->context, object);
    }
    if (object == NULL || (state->flags & 0x10000u) == 0u ||
        hooks->color_apply == NULL || state->period == 0.0f) return;
    state->phase += (6.2831855f / state->period) * delta;
    oscillation = (float)sin((double)state->phase);
    for (i = 0; i < 3; ++i) {
        half_difference = (state->to[i] - state->from[i]) * 0.5f;
        color[i] = state->from[i] + half_difference +
                   half_difference * oscillation;
    }
    hooks->color_apply(hooks->context, color[0], color[1], color[2]);
}

/* Port model of 0x004247d0 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x004247d0, retires to the tail in GAME-021 part 3
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) int entity_default_transform_apply_if_flagged(
    EntitySlot* entity, void* argument)
{
    __asm {
        push ebp
        mov ebp, esp
        push ecx
        mov eax, dword ptr [ebp + 0ch]
        mov dword ptr [ebp - 4], eax
        mov ecx, dword ptr [ebp + 8]
        mov edx, dword ptr [ecx + 3ech]
        and edx, 1
        test edx, edx
        je default_transform_done
        push 0
        push 0
        push 3f800000h
        mov eax, dword ptr [ebp - 4]
        push eax
        mov ecx, dword ptr [ebp + 8]
        push ecx
        call selected_entity_ground_marker_render
        add esp, 14h
    default_transform_done:
        xor eax, eax
        mov esp, ebp
        pop ebp
        ret
    }
}
#else
int entity_default_transform_apply_if_flagged(EntitySlot* entity,
                                              void* argument)
{
    if (entity != NULL && (entity->flags_3ec & 1u) != 0u)
        selected_entity_ground_marker_render(entity, argument, 1.0f, 0.0f,
                                             0.0f);
    return 0;
}
#endif

/* Port model of 0x00424810 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x00424810, retires to the tail in GAME-021 part 3
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) void entity_default_transform_apply_all(void* argument)
{
    __asm {
        push ebp
        mov ebp, esp
        mov eax, dword ptr [ebp + 8]
        push eax
        push offset entity_default_transform_apply_if_flagged
        call entity_pool_find_by_callback_filtered_current
        add esp, 8
        pop ebp
        ret
    }
}
#else
void entity_default_transform_apply_all(void* argument)
{
    entity_pool_find_by_callback_filtered_current(
        entity_default_transform_apply_if_flagged, argument);
}
#endif

/* SelectedObjectOutlinesDraw (FUN_00424830): draw the first selected entry
 * green, then the rest yellow. The scene-marker builder is renderer-owned. */
/* Port model of 0x00424830 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x00424830, retires to the tail in GAME-021 part 3
void selected_object_outlines_draw(
    const GameContext* ctx, SelectedObjectOutlineDrawFn draw,
    void* context)
{
    const int count = selection_count(ctx);
    EntitySlot* const* entries = selection_record_array(ctx);
    int i;
    if (draw == NULL || entries == NULL) return;
    for (i = 0; i < count; ++i) {
        if (i == 0) {
            draw(context, entries[i], 0.0f, 1.0f, 0.0f);
        }
        else {
            draw(context, entries[i], 1.0f, 1.0f, 0.0f);
        }
    }
}

/* SelectedEntityGroundMarkerRender (FUN_004248b0): project the entity's
 * ground point, reject off-screen markers, build the beveled eight-point
 * ground outline, then update its optional label tint. */
/* Port model of 0x004248b0 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x004248b0, retires to the tail in GAME-021 part 3
int selected_entity_ground_marker_render(void* entity, void* camera,
                                         float red, float green, float blue)
{
    SelectedGroundMarkerOps* ops = &s_selected_ground_marker_ops;
    float world[3];
    float screen[3];
    float edge_world[3];
    float edge_screen[3];
    float width = 0.0f;
    float viewport_height = 0.0f;
    float entity_height = 0.0f;
    float radius = 0.0f;
    float pixel_radius;
    float bevel;
    float points[16];
    const char* label = NULL;
    uint32_t global_flags;
    float dx;
    float dy;
    float dz;
    if (entity == NULL ||
        entity_type_operation_release((const EntitySlot*)entity) != 0u ||
        ops->entity_renderable == NULL ||
        !ops->entity_renderable(ops->context, entity) ||
        ops->marker_data_get == NULL || ops->world_to_screen == NULL ||
        ops->viewport_get == NULL) return 0;
    global_flags = ops->global_flags_get != NULL ?
        ops->global_flags_get(ops->context) : 0u;
    if ((global_flags & 0x100000u) != 0u) {
        red *= 0.3f;
        green *= 0.3f;
        blue *= 0.3f;
    }
    if (!ops->marker_data_get(ops->context, entity, world, &radius,
                              &entity_height, &label)) return 0;
    world[2] -= entity_height * 0.5f;
    if (!ops->world_to_screen(ops->context, camera, world, screen)) return 0;
    ops->viewport_get(ops->context, &width, &viewport_height);
    if (screen[0] < 0.0f || screen[1] < 0.0f ||
        screen[0] > width || screen[1] > viewport_height) return 0;
    edge_world[0] = world[0] + radius;
    edge_world[1] = world[1];
    edge_world[2] = world[2];
    if (!ops->world_to_screen(ops->context, camera, edge_world, edge_screen))
        return 0;
    dx = screen[0] - edge_screen[0];
    dy = screen[1] - edge_screen[1];
    dz = screen[2] - edge_screen[2];
    pixel_radius = (float)sqrt(dx * dx + dy * dy + dz * dz) / 2.5f;
    bevel = pixel_radius / 5.0f + 3.0f;
    points[0] = screen[0] - pixel_radius;
    points[1] = screen[1] - pixel_radius + bevel;
    points[2] = screen[0] - pixel_radius + 1.0f;
    points[3] = screen[1] - pixel_radius;
    points[4] = screen[0] - pixel_radius + bevel;
    points[5] = screen[1] - pixel_radius;
    points[6] = screen[0] + pixel_radius - bevel;
    points[7] = screen[1] - pixel_radius;
    points[8] = screen[0] + pixel_radius;
    points[9] = screen[1] - pixel_radius + bevel;
    points[10] = screen[0] + pixel_radius;
    points[11] = screen[1] + pixel_radius - bevel;
    points[12] = screen[0] + pixel_radius - bevel;
    points[13] = screen[1] + pixel_radius;
    points[14] = screen[0] - pixel_radius + bevel;
    points[15] = screen[1] + pixel_radius;
    if (ops->polyline_draw != NULL)
        ops->polyline_draw(ops->context, points, red, green, blue);
    if ((global_flags & 0x100000u) == 0u && label != NULL && *label != '\0' &&
        ops->label_update != NULL) {
        ops->label_update(ops->context, label, red, green, blue);
    }
    return 1;
}

/* Port model of 0x004250c0 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x004250c0, retires to the tail in GAME-021 part 3
int main_sim_step(EngineContext* ctx, float scale,
                  uint32_t* o1, uint32_t* o2, uint32_t* o3)
{
    if (ctx == NULL) {
        return 0;
    }
    if (o1 != NULL) {
        *o1 = 0;
    }
    if (o2 != NULL) {
        *o2 = 0;
    }
    if (o3 != NULL) {
        *o3 = 0;
    }

    /* 425637..425708: capture each held movement key before later tickers. */
    entity_motion_inputs_capture(ctx->input);

    /* Debug/cheat toggles — gated by the debug word bit 0x10 (finding 04). */
    if ((ctx->debug & 0x10u) != 0) {
        if (m_edge(ctx, MNSIM_K_FRAME_RATE)) {
            m_toggle_hud(ctx, &ctx->state, 0x40u, "Frame Rate Monitor");
        }
        if (m_edge(ctx, MNSIM_K_MEMORY)) {
            m_toggle_hud(ctx, &ctx->state, 0x80u, "Memory Monitor");
        }
        if (m_edge(ctx, MNSIM_K_SOUNDFX)) {
            m_toggle_hud(ctx, &ctx->state, 0x10u, "Sound Effects");
        }
        if (m_edge(ctx, MNSIM_K_MUSIC)) {
            m_toggle_hud(ctx, &ctx->state, 0x8u, "Music");
        }
        /* render-pass toggle (corpus state 0x2/0x1000/0x2000 cluster). */
        if (m_edge(ctx, MNSIM_K_SAYFB_RENDER)) {
            if ((ctx->state & 0x2u) == 0) {
                ctx->state |= 0x2u;
            } else {
                ctx->state &= ~0x2u;
            }
        }
    }

    /* The retail air cheat is enabled by feature bit 0x8000 and decrements
     * the live LevelState float at +0x1fc by 1.0 while its key is held.
     * EngineState::air_beat is only a reconstruction-side ticker counter;
     * the level block is the source used by the game logic. */
    if ((ctx->feature & 0x8000u) != 0u && ctx->level_block != NULL &&
        m_held(ctx, MNSIM_K_AIRCHEAT)) {
        float air;
        memcpy(&air, (const char*)ctx->level_block + 0x1fc, sizeof(air));
        air -= 1.0f;
        memcpy((char*)ctx->level_block + 0x1fc, &air, sizeof(air));
    }
    else if (ctx->level_block == NULL && ctx->state_block != NULL &&
             m_held(ctx, MNSIM_K_AIRCHEAT) &&
             ctx->state_block->air_beat != 0u) {
        /* Isolated engine tests and preview contexts may have only the
         * ticker sidecar bound. Keep that adapter useful without changing
         * the live level-block path above. */
        ctx->state_block->air_beat--;
    }

    /* 4252e5..425360 (Finding 101): held camera keys, gated on the outer
     * activity mask, in-game context mode, and system-state bit 0x1000.
     * Zoom drives the bounded controller distance at +0x90, which the
     * primary follow view reads. The corpus delta is scale * 3.0 in 25-Hz
     * frame units; the port's scale is seconds, hence the * 25.
     * Pan (+0x78) and tilt (+0x84) remain deferred until the follow view
     * consumes those controller fields. */
    if ((ctx->state & 0x08100000u) == 0u && ctx->context_mode == 1 &&
        (system_state_flags_get() & 0x1000u) == 0u) {
        const float frames = scale * 25.0f;
        if (m_held(ctx, MNSIM_K_ZOOM_OUT)) {
            camera_zoom_update(ctx->camera_controller, frames * 3.0f);
        }
        if (m_held(ctx, MNSIM_K_ZOOM_IN)) {
            camera_zoom_update(ctx->camera_controller, frames * -3.0f);
        }
    }

    /* The three edge outputs EngineTick's later tickers consume. */
    if (m_edge(ctx, '1')) {
        *o1 = 1;
    }
    if (m_edge(ctx, '2')) {
        *o2 = 1;
    }
    if (m_edge(ctx, '3')) {
        *o3 = 1;
    }

    /* Quit gate: the corpus ends the frame through QuitPoll + the
     * keep-running probe; ours keys it to the named key + EngineKeepRunning. */
    if (m_edge(ctx, MNSIM_K_QUIT)) {
        return 0;
    }
    if (EngineKeepRunning(ctx) == 0) {
        return 0;
    }
    return 1;
}

/* PeriodicCallbackDispatch (FUN_00425b40): run the installed periodic
 * callback over live entity records, skipping the corpus's +0x3e8 bit
 * 0x80000 flagged entries and stopping when a callback reports a match. */
/* Port model of 0x00425b40 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x00425b40, retires to the tail in GAME-021 part 3
void periodic_entity_callback_dispatch(void* payload)
{
    entity_pool_find_by_callback_filtered_current(
        periodic_entity_callback_adapter, payload);
}

namespace unported_ledger_stubs {
/* Port model of 0x00425c30 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x00425c30, retires to the tail in GAME-021 part 3
void ledger_stub_00425c30_TransitionControl(int enabled, uint32_t parameter)
{
    uint32_t* engine_state = engine_current_state_word();

    if ((engine_debug_word_peek() & 2u) != 0u || engine_state == NULL)
        return;

    if (enabled != 0)
        parameter = (~*engine_state >> 20) & 1u;

    if (parameter != 0u) {
        if ((*engine_state & 0x100000u) == 0u) {
            sound_subsystem_stop_all();
        }
        engine_state = engine_current_state_word();
        if (engine_state != NULL) *engine_state |= 0x100000u;
        engine_debug_word_bit_set(0x1000u, 1);
        return;
    }

    periodic_entity_callback_dispatch(NULL);
    sound_alias_play(7, 1);
    engine_state = engine_current_state_word();
    if (engine_state != NULL) *engine_state &= ~0x100000u;
    engine_debug_word_bit_set(0x1000u, 0);
}
} // namespace unported_ledger_stubs


/* Port model of 0x00425ce0 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x00425ce0, retires to the tail in GAME-021 part 3
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

/* Port model of 0x00425d90 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x00425d90, retires to the tail in GAME-021 part 3
void simtick_mouse_world(EngineContext* c, float dt)
{
    (void)dt;
    /* gate: state & 2 (in-game render pass), &0x1000 required, &0x100 blocks. */
    if (c == NULL) return;
    if ((c->state & 2u) == 0) return;
    if ((c->state & 0x1000u) == 0) return;
    if ((c->state & 0x100u) != 0) return;
    if (c->input == NULL) return;
    /* mouse delta vs stored anchor; accumulate the distance. */
    static float acc = 0.0f;
    float dx = (float)c->input->mouse_x - (float)s_mouse_anchor_x;
    float dy = (float)c->input->mouse_y - (float)s_mouse_anchor_y;
    acc += sqrtf(dx * dx + dy * dy);
    if (acc >= 4.0f) {
        acc = 0.0f;
        if (s_pick != NULL) {
            s_pick(c->input->mouse_x, c->input->mouse_y);
        }
        ++s_picks;
    }
}

/* Port model of 0x004261c0 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x004261c0, retires to the tail in GAME-021 part 3
void camera_spawn_countdown(float dt)
{
    const uint8_t* level = s_spawn_level_block;
    float spawn_interval;
    uint32_t spawn_type;
    if (level == NULL) return;
    /* The reference reads the re-arm interval and type from the current
     * level block at +0x268/+0x26c. */
    memcpy(&spawn_interval, level + 0x268, sizeof(spawn_interval));
    memcpy(&spawn_type, level + 0x26c, sizeof(spawn_type));
    if (spawn_type == 0x14) {
        return;
    }
    g_camera_countdown -= dt;
    if (g_camera_countdown < 0.0f) {
        if (spawn_probe(0, 0)) {
            spawn_issue(0, (uint32_t)spawn_type);
        }
        g_camera_countdown = spawn_interval;
    }
}

/* Port model of 0x00426230 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x00426230, retires to the tail in GAME-021 part 3
void global_triple_copy_out(uint32_t* out)
{
    if (out == NULL) {
        return; /* deviation: the corpus writes through blindly */
    }
    out[0] = s_triple_first;
    out[1] = s_triple_second;
    out[2] = s_triple_third;
}

/* ObjectiveTaskTextOriginSet (FUN_00426320) — see engine.hpp. The corpus's
 * pair has no ported reader outside the corpus's own draw band (the only
 * writers are this setter), so it rides module storage per the unowned-
 * global convention; the enable bit rides the bound debug word. */
/* Port model of 0x00426320 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x00426320, retires to the tail in GAME-021 part 3
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
/* Port model of 0x004263c0 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x004263c0, retires to the tail in GAME-021 part 3
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

/* MainSimInteractionAndCellUpdate: retain this subsystem and its real
 * admission gates. Ordinary pointer actions share the frame UI arbiter's
 * ordered samples and central world owner; raw aggregate presses cannot
 * bypass UI or emit a second command after the operation queue has drained. */
/* Port model of 0x00426520 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x00426520, retires to the tail in GAME-021 part 3
void simtick_vu_meter(EngineContext* c, float dt)
{
#if defined(LEGORRR_BACKEND_WINAPI)
    GameContext* game;
    LevelState* level;
    int width, height, i;
    (void)dt;
    if (c == NULL) return;
    c->debug &= ~0x400u;
    if ((c->state & 2u) == 0u || (c->state & 0x100u) != 0u ||
        c->context_mode != 1 || c->input == NULL) return;
    game = entity_pool_callback_context_get();
    level = (LevelState*)c->level_block;
    if (game == NULL || level == NULL || game->level != level) return;
    width = display_width_get();
    height = display_height_get();
    for (i = 0; i < c->input->pointer_count; ++i)
        (void)selection_pointer_world_dispatch(game, c,
            &c->input->pointer_samples[i], width, height);
#else
    GameContext* game;
    LevelState* level;
    float cell_x, cell_y;
    EntitySlot* picked;
    int width, height;
    (void)dt;
    if (c == NULL) return;
    c->debug &= ~0x400u; /* corpus's first statement after loading state */

    if ((c->state & 2u) == 0u || (c->state & 0x100u) != 0u ||
        c->context_mode != 1 || c->input == NULL ||
        c->input->mouse_press == 0) return;

    game = entity_pool_callback_context_get();
    level = (LevelState*)c->level_block;
    if (game == NULL || level == NULL || game->level != level) return;
    width = display_width_get();
    height = display_height_get();
    if (width <= 0 || height <= 0 ||
        !level_scene_mouse_cell(game, level, width, height,
                               c->input->mouse_x, c->input->mouse_y,
                               &cell_x, &cell_y)) return;

    picked = level_scene_pick_entity(game, level, width, height,
                                     c->input->mouse_x, c->input->mouse_y,
                                     18.0f);
    if (picked == NULL) {
        /* The renderer's precise screen picker currently covers minifigures.
         * Keep other selectable entity kinds reachable through the shared
         * cell-space selection owner. */
        picked = selection_pick(game, cell_x, cell_y, 0.75f);
    }
    if (picked != NULL) {
        (void)selection_add(game, picked);
    }
    else if (selection_count(game) != 0) {
        (void)selection_cmd_move(game, cell_x, cell_y);
    }
#endif
}

/* ToolTipInfoStringsLoad (FUN_00427e00) loads the nine fixed labels from
 * the ToolTipInfo group. The corpus heap-copies each value into a global;
 * this model keeps bounded module-owned copies so they survive block_close. */
/* Port model of 0x00427e00 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x00427e00, retires to the tail in GAME-021 part 3
void tooltip_info_strings_load(BlockFile* cfg)
{
    static const char* const paths[9] = {
        "ToolTipInfo::HealthText",
        "ToolTipInfo::EnergyText",
        "ToolTipInfo::CrystalsText",
        "ToolTipInfo::OreText",
        "ToolTipInfo::StudsText",
        "ToolTipInfo::ToolsText",
        "ToolTipInfo::CarryObjectText",
        "ToolTipInfo::DrivenByText",
        "ToolTipInfo::OreRequiredText"
    };
    int i;
    for (i = 0; i < 9; ++i) {
        s_tooltip_info_strings[i][0] = '\0';
        (void)block_get_str(cfg, paths[i], s_tooltip_info_strings[i],
                            sizeof(s_tooltip_info_strings[i]));
    }
}

namespace lego {
/* Port model of 0x00427f80 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x00427f80, retires to the tail in GAME-021 part 3
void level_upgrade_name_strings_load(void* config)
{
    uint32_t i;
    uint32_t count;
    char key[128];
    uint32_t value;
    if (s_upgrade_entry_hook != 0 && s_upgrade_entry_count_hook != 0) {
        count = s_upgrade_entry_count_hook(config);
        if (count > 4096) count = 4096;
        for (i = 0; i < count; ++i) {
            const char* suffix;
            uint32_t index;
            key[0] = '\0';
            value = 0;
            if (!s_upgrade_entry_hook(config, i, key, sizeof(key), &value))
                continue;
            key[sizeof(key) - 1] = '\0';
            if (!prefix_equal(key, "Level")) continue;
            suffix = key + 5;
            /* FUN_00427f80 passes the suffix straight to FUN_0048db70/atoi.
             * Keep atoi's prefix parsing and empty/non-numeric result of 0. */
            index = (uint32_t)atoi(suffix);
            if (index < UPGRADE_NAME_SLOT_COUNT) {
                if (s_level_upgrade_name_strings[index] != 0) {
                    engine_heap_free(s_level_upgrade_name_strings[index]);
                    s_level_upgrade_name_strings[index] = 0;
                }
                s_level_upgrade_names[index] = value;
            }
        }
        return;
    }

    /* GameConfigInit passes the live Lego.cfg block to FUN_00427f80. The
     * reference resolves UpgradeNames from its config root, then duplicates
     * each matching value string into the global 16-entry pointer table. */
    {
        BlockRec* group = config != 0
            ? block_subgroup((BlockFile*)config,
                  string_path_join(util_root_name((BlockFile*)config),
                                   "UpgradeNames", (const char*)NULL)) : 0;
        BlockRec* entry;
        for (entry = group; entry != 0;
             entry = block_next_equal_key(entry)) {
            const char* suffix;
            const char* entry_name = entry->key;
            const char* entry_value = entry->value;
            uint32_t index;
            char* copy;
            if (strnicmp_lite("Level", entry_name, 5) != 0) continue;
            suffix = entry_name + 5;
            index = (uint32_t)atoi(suffix);
            if (index >= UPGRADE_NAME_SLOT_COUNT) continue;
            copy = string_duplicate(entry_value);
            if (s_level_upgrade_name_strings[index] != 0) {
                engine_heap_free(s_level_upgrade_name_strings[index]);
                s_level_upgrade_name_strings[index] = 0;
            }
            s_level_upgrade_name_strings[index] = copy;
            /* The retail global is a 32-bit pointer table. Keep that image
             * on x86; on wider hosts callers use the pointer sidecar getter. */
            s_level_upgrade_names[index] = sizeof(void*) == sizeof(uint32_t)
                ? (uint32_t)(uintptr_t)copy : 0u;
        }
    }
}
} // namespace lego

/* EntityCommandStatusFeedbackDispatch (FUN_00428020): suppresses the first
 * observed entity after a selection switch, clears stale empty command text,
 * and routes changed command/status text through the UI adapter. */
/* Port model of 0x00428020 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x00428020, retires to the tail in GAME-021 part 3
void entity_command_status_feedback_dispatch(EntitySlot* entity)
{
    const char* command;
    const char* status_text;
    char text[256];
    unsigned int i;
    int status;
    EntityCommandStatusFeedbackHooks* hooks =
        &s_entity_command_feedback_hooks;
    if (entity == NULL || hooks->command_text_get == NULL) return;
    command = hooks->command_text_get(entity, hooks->context);
    if (command == NULL) return;
    if (s_entity_command_feedback_last_entity != entity) {
        s_entity_command_feedback_last_entity = entity;
        return;
    }
    if (command[0] == '\0' && hooks->empty_command_clear != NULL) {
        hooks->empty_command_clear(entity, hooks->context);
        s_entity_command_feedback_last_text[0] = '\0';
        return;
    }
    if (command[0] == '\0') return;
    for (i = 0; i + 1 < sizeof(text) && command[i] != '\0'; ++i)
        text[i] = command[i];
    text[i] = '\0';
    status = hooks->status_get != NULL ?
        hooks->status_get(entity, hooks->context) : 0;
    status_text = hooks->status_text_get != NULL ?
        hooks->status_text_get(entity, status, hooks->context) : NULL;
    if (status_text != NULL && status_text[0] != '\0') {
        size_t j = strlen(text);
        if (j < sizeof(text) - 1) text[j++] = ' ';
        for (i = 0; j + 1 < sizeof(text) && status_text[i] != '\0'; ++i)
            text[j++] = status_text[i];
        text[j] = '\0';
    }
    if (strcmp(text, s_entity_command_feedback_last_text) == 0) return;
    strcpy(s_entity_command_feedback_last_text, text);
    if (hooks->feedback_dispatch != NULL)
        hooks->feedback_dispatch(entity, text, hooks->context);
}

namespace unported_ledger_stubs {
/* Port model of 0x00428330 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x00428330, retires to the tail in GAME-021 part 3
void ledger_stub_00428330_TerrainCommandFeedbackDispatch(
    TerrainCommandFeedbackState* state, int details_requested,
    int parameter_three, int parameter_four,
    const TerrainCommandFeedbackHooks* hooks)
{
    TerrainCommandFeedbackHooks runtime_hooks;
    if (hooks != NULL) runtime_hooks = *hooks;
    else memset(&runtime_hooks, 0, sizeof(runtime_hooks));
    if (runtime_hooks.message_cue_get == NULL)
        runtime_hooks.message_cue_get = terrain_feedback_message_cue_get;
    if (runtime_hooks.message_cue_set == NULL)
        runtime_hooks.message_cue_set = terrain_feedback_message_cue_set;
    if (runtime_hooks.message_update == NULL)
        runtime_hooks.message_update = terrain_feedback_message_update;
    terrain_command_feedback_dispatch(state, details_requested,
                                      parameter_three, parameter_four,
                                      &runtime_hooks);
}
} // namespace unported_ledger_stubs

/* TerrainCellCommandPredicate (FUN_00428780) — see engine.hpp. The corpus
 * re-reads its system-state word through FUN_00456240 before each gate and
 * strides the voxel cell blindly; the port answers through the
 * bounds-checking voxel accessor for that latter path. CurrentLevelGet's
 * block is the bound level_block here. */
/* Port model of 0x00428780 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x00428780, retires to the tail in GAME-021 part 3
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


/* TerrainCellPaletteResourceAtGridPosition returns the cell's palette
 * texture handle from the current level's +0x7c resource grid. */
/* Port model of 0x00429160 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x00429160, retires to the tail in GAME-021 part 3
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) void* terrain_cell_palette_resource_at_grid_position_current(
    const int cell_xy[2])
{
    __asm {
        mov ecx, dword ptr [g_active_level_retail]
        mov edx, dword ptr [esp + 4]
        push esi
        mov eax, dword ptr [ecx + 14h]
        mov esi, dword ptr [edx]
        imul eax, dword ptr [edx + 4]
        mov edx, dword ptr [ecx + 0b8h]
        mov ecx, dword ptr [ecx + 7ch]
        add eax, esi
        lea eax, dword ptr [eax + eax*8]
        mov al, byte ptr [edx + eax*8 + 1]
        push eax
        push ecx
        call packed_grid_cell_value_get
        add esp, 8
        pop esi
        ret
    }
}
#else
void* terrain_cell_palette_resource_at_grid_position_current(
    const int cell_xy[2])
{
    LevelState* level = level_current_owner_get();
    if (level == NULL || cell_xy == NULL) return NULL;
#if defined(_WIN32) && !defined(_WIN64)
    /* The retail target reads the active level's +0x14 width, +0xb8 cell
     * table, and +0x7c LoadTileVariants descriptor directly. Its callers
     * supply in-bounds coordinates; preserve the same row-major selector and
     * let PackedGridCellValueGet decode the table coordinates. */
    {
        uint32_t width = level->word[LEVEL_OFF_WIDTH / 4];
        uint32_t index = width * (uint32_t)cell_xy[1] +
                         (uint32_t)cell_xy[0];
        uint32_t packed = index * 9u;
        const uint8_t* voxels = (const uint8_t*)
            level->word[LEVEL_OFF_VOXELS / 4];
        const void* resources = (const void*)
            level->word[LEVEL_OFF_CELL_RESOURCES / 4];
        packed = (packed & 0xffffff00u) |
            voxels[index * LEVEL_VOXEL_STRIDE + 1u];
        return (void*)packed_grid_cell_value_get(resources, packed);
    }
#else
    {
        TerrainCellResourceGrid* resources;
        const uint8_t* cell;
        uint32_t x, y, width, height, index, packed, row, column;
        uint32_t resource_index;
        width = (uint32_t)level_width(level);
        height = (uint32_t)level_height(level);
        if (cell_xy[0] < 0 || cell_xy[1] < 0 ||
            (uint32_t)cell_xy[0] >= width ||
            (uint32_t)cell_xy[1] >= height) return NULL;
        x = (uint32_t)cell_xy[0];
        y = (uint32_t)cell_xy[1];
        cell = level_voxel(level, (int)x, (int)y);
        resources = level->ptr_cell_resources;
        if (cell == NULL || resources == NULL || resources->handles == NULL)
            return NULL;
        index = y * width + x;
        packed = (index * 9u & 0xffffff00u) | cell[1];
        /* FUN_0040b850 indexes low-nibble * width + high-nibble. */
        row = packed & 0x0fu;
        column = (packed & 0xffu) >> 4;
        if (row >= resources->height || column >= resources->width)
            return NULL;
        resource_index = row * resources->width + column;
        if (resource_index >= resources->width * resources->height)
            return NULL;
        return resources->handles[resource_index];
    }
#endif
}
#endif

/* Port model of 0x004291a0 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x004291a0, retires to the tail in GAME-021 part 3
void simtick_render_pass(EngineContext* c, float dt, float scale, uint32_t o1)
{
    (void)o1;
    if (c == NULL) return;
    if (scale == 0.0f) s_camera_detail_collection_ready = 1;
#if defined(LEGORRR_BACKEND_WINAPI) && defined(_WIN32) && !defined(_WIN64)
    if (c->level_block != NULL &&
        ((scale == 0.0f && c->context_mode == 1) ||
         (scale != 0.0f && (c->state & 2u) != 0u &&
          (c->state & 0x2000u) != 0u))) {
        const int primary = scale == 0.0f;
        float position[3], forward[3];
        if (!scene_runtime_collection_pose_get(c->views[primary ? 0 : 1],
                primary, position, forward)) {
            fprintf(stderr, "camera-diag: pose failed primary=%d view=%p level=%p\n",
                primary, c->views[primary ? 0 : 1], c->level_block);
            fflush(stderr);
            s_camera_detail_collection_ready = 0;
        } else if (!terrain_camera_detail_collect((LevelState*)c->level_block,
                position, primary ? forward : NULL, primary)) {
            fprintf(stderr, "camera-diag: collection failed primary=%d level=%p pos=%g,%g,%g\n",
                primary, c->level_block, position[0], position[1], position[2]);
            fflush(stderr);
            s_camera_detail_collection_ready = 0;
        }
    }
#endif
    if (scale == 0.0f) {
        if (c->context_mode == 1 || c->context_mode == 0) {
            if (s_render_pass_hook != NULL)
                s_render_pass_hook(c, dt, scale, s_render_pass_user);
            ++s_post_n;
        }
    } else if ((c->state & 2u) != 0 && (c->state & 0x2000u) != 0) {
        if (s_render_pass_hook != NULL)
            s_render_pass_hook(c, dt, scale, s_render_pass_user);
        ++s_post_n;
    }
}

/* Port model of 0x004295a0 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x004295a0, retires to the tail in GAME-021 part 3
uint32_t context_selected_global_pointer_get(void)
{
    return (media_min_distance_mode() != 0) ? s_ctx_sel_ptr_second
                                           : s_ctx_sel_ptr_first;
}

/* Port model of 0x004295c0 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x004295c0, retires to the tail in GAME-021 part 3
int entity_matches_current_selection_or_owner(const void* entity)
{
    if (entity == NULL) {
        return 0;
    }
    const void* selected = media_min_distance_context();
    const int mode = media_min_distance_mode();
    void* owner = NULL;
    __builtin_memcpy(&owner, (const char*)entity + 0x36c, sizeof(owner));
    /* corpus shape: match (mode-clear && entity == selection) or
     * (owner != NULL && owner == selection); reject otherwise. */
    const int direct_ok = (mode == 0) && (entity == selected);
    const int owner_ok = (owner != NULL) && (owner == selected);
    return (direct_ok || owner_ok) ? 1 : 0;
}

/* Port model of 0x004295f0 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x004295f0, retires to the tail in GAME-021 part 3
void media_min_distance_update_mode(int mode, void* context,
                                    int scalar_selector)
{
    if (mode == 0) {
        /* Scalar selector: 0 -> 0.9, nonzero -> 0.6 (corpus float
         * constants 0x3f666666 / 0x3f19999a). */
        s_mind_listener = scalar_selector != 0 ? 0.6f : 0.9f;
        s_mind_clip = s_mind_dist_base * s_mind_cell_scale;
        s_mind_ref = 50.0f; /* 0x42480000 */
        sound_tracked_buffers_set_min_distance(s_mind_ref);
        /* Context release / entity-kind hooks land at the entity layer
         * (unported seams); storing is the observable half. The corpus
         * dereferences the context unchecked; the port guards NULL. */
        s_mind_context = context;
    } else if (mode == 1) {
        if (s_mind_context != NULL) {
            /* Corpus: context release (entity layer seam). */
            s_mind_context = NULL;
        } else {
            s_mind_context = NULL;
        }
        s_mind_listener = 0.5f; /* 0x3f000000 */
        s_mind_clip = s_mind_clip_base;
        s_mind_ref = s_mind_ref_base;
        sound_tracked_buffers_set_min_distance(s_mind_ref);
    }
    /* Other modes update only the mode global, like the corpus. */
    s_mind_mode = mode;
}
/* Port model of 0x004297b0 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x004297b0, retires to the tail in GAME-021 part 3
void media_cd_watchdog_update(int enable)
{
    uint32_t* flags = engine_shared_state_word();
    if (enable != 0) {
        s_cd_slot = s_cd_draw_slot();
        *flags |= MEDIA_CD_WATCHDOG_BIT;
        media_cd_audio_track_start((int)s_cd_slot, 0, s_cd_rearm);
    } else {
        *flags &= ~MEDIA_CD_WATCHDOG_BIT;
        media_cd_audio_stop_and_close_if_active();
        media_cd_audio_watchdog_tick(0);
    }
}

/* Port model of 0x00429810 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x00429810, retires to the tail in GAME-021 part 3
void media_audio_playback_toggle(int enable)
{
    if (enable != 0) {
        *engine_shared_state_word() |= MEDIA_FLAG_SOUND_ON;
        sound_subsystem_enable_set_and_stop(enable);
        return;
    }
    *engine_shared_state_word() &= ~MEDIA_FLAG_SOUND_ON;
    sound_subsystem_enable_set_and_stop(0);
}



/* EngineFloatField260Get (FUN_00429880, findings 295/392): the float at
 * active engine-state +0x260. */
/* Port model of 0x00429890 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x00429890, retires to the tail in GAME-021 part 3
LevelState* level_load(BlockFile* cfg, const char* level_name)
{
    GameContext* game;
    LevelState* previous_level;
    if (cfg == NULL || level_name == NULL) {
        return NULL;
    }
    /* GameConfigInit loads SurfaceTypeDescriptions from the same Lego.cfg
     * root before a retail level is entered. The portable level-load path
     * owns the parsed config tree and is the earliest shared production
     * seam available for those process-wide description tables. */
    surface_type_descriptions_config_load(cfg, util_root_name(cfg));
    /* The object-type SFX lookup table is process-global in retail. The
     * config loader has no startup caller in this checkout, so seed it from
     * the same live Lego.cfg root used by the level's terrain descriptions. */
    terrain_object_sfx_grid_config_load(cfg, util_root_name(cfg));
    game = entity_pool_callback_context_get();
    previous_level = game != NULL ? game->level : NULL;

    /* PendingLevelName clears the cross-level pools before loading the new
     * level (FUN_004372a0); this is the shared production level-load path. */
    spawn_coordinate_pool_clear();

    LevelState* s = (LevelState*)calloc(1, sizeof(LevelState));
    if (s == NULL) {
        return NULL;
    }
    /* DAT_005560d4 is current throughout LevelLoad, including the map
     * loaders that register EmergeMap stages. Mirror that lifetime in the
     * GameContext instead of waiting for the caller's assignment. */
    if (game != NULL) game->level = s;
    /* The erosion tables are process globals in the reference, but their
     * active coordinates belong to this level's EmergeMap. Bind and clear
     * them before loading any sparse map that can register a stage. */
    lava_erosion_level_context_set(s);
    {
        float trigger_time = cfg_float(cfg, level_name,
                                       K_ERODETRIGGER, 0.0f);
        float erosion_time = cfg_float(cfg, level_name,
                                        K_ERODETIME, 0.0f);
        float lock_time = cfg_float(cfg, level_name, K_ERODELOCK, 0.0f);
        long mutation_count = cfg_int(cfg, level_name,
                                      K_LANDSLIDECOUNT, 0);
        if (trigger_time == 0.0f) trigger_time = 10.0f;
        if (erosion_time == 0.0f) erosion_time = 5.0f;
        if (lock_time == 0.0f) lock_time = 60.0f;
        if (mutation_count == 0) mutation_count = 3;
        erosion_timing_state_init(trigger_time, erosion_time, lock_time);
        lava_erosion_mutation_threshold_set((unsigned int)mutation_count);
    }

    /* --- scalar config keys (finding 07 §"What a level is") ------------- */
    level_store_ptr(s, LEVEL_OFF_LEVEL_NAME, level_name);
    /* LevelLoad 0x42ad16 initializes the real air and comparison values. */
    level_set_float(s, LEVEL_OFF_AIR_SUPPLY, 100.0f);
    level_set_float(s, 0x204, 100.0f);
    /* DependencyGridConfigLoad builds its 20x15 rule table from the global
     * Dependencies records.  Bind those records to this LevelState before
     * the map's later dependency/upgrade queries can run. */
    if (game != NULL && game->level == s) {
        unported_ledger_stubs::dependency_grid_config_load(
            level_config_block_find(cfg, level_name),
            game->cfg != NULL ? game->cfg : cfg);
    }
    else {
        unported_ledger_stubs::dependency_grid_level_load(
            s, level_config_block_find(cfg, level_name), cfg);
    }

    float blocksize = cfg_float(cfg, level_name, K_BLOCKSIZE, 0.0f);
    level_set_float(s, LEVEL_OFF_BLOCKSIZE, blocksize);
    level_set_float(s, LEVEL_OFF_DIGDEPTH, cfg_float(cfg, level_name, K_DIGDEPTH, 0.0f));
    level_set_int(s, LEVEL_OFF_USEROOF, (int32_t)cfg_typed(cfg, level_name, K_USEROOF, 0));
    level_set_float(s, LEVEL_OFF_ROOFHEIGHT, cfg_float(cfg, level_name, K_ROOFHEIGHT, 0.0f));
    level_set_float(s, LEVEL_OFF_ROUGHLEVEL, cfg_float(cfg, level_name, K_ROUGHLEVEL, 0.0f));
    level_set_int(s, LEVEL_OFF_SAFECAVERNS, (int32_t)cfg_typed(cfg, level_name, K_SAFECAVERNS, 0));
    level_set_float(s, LEVEL_OFF_SELBOXHEIGHT, cfg_float(cfg, level_name, K_SELBOXH, 0.0f));

    /* LevelLoad maps Rock/default to 0, Lava to 1, and Ice to 2. */
    {
        char boulder[64] = { 0 };
        if (cfg_string(cfg, level_name, K_BOULDER_ANIM, boulder, sizeof(boulder)) != NULL) {
            int mode = 0;
            if (strcasecmp_lite(boulder, "Lava") == 0) mode = 1;
            else if (strcasecmp_lite(boulder, "Ice") == 0) mode = 2;
            level_set_int(s, LVL_OFF_BOULDER_ANIM, mode);
        }
    }

    /* LevelLoad stores rates and upgrade durations before opening map data.
     * Its twenty default upgrade slots are overwritten by the three
     * specialized values below when those keys are nonzero. */
    level_set_int(s, LEVEL_OFF_STARTP,
        cfg_typed(cfg, level_name, K_STARTFP, 0) == 1 ? 1 : 0);
    level_set_int(s, LEVEL_OFF_NODRAIN,
        cfg_typed(cfg, level_name, K_NODRAIN, 0) == 1 ? 1 : 0);
    level_set_float(s, LEVEL_OFF_OXYGEN_RATE,
                    cfg_float(cfg, level_name, K_OXYGENRATE, 0.0f) * LVL_OXYGEN_SCALE);
    {
        float base_ticks = cfg_float(cfg, level_name, K_UPGRADETIME,
                                     0.0f) * 25.0f;
        float vehicle_ticks;
        float minifigure_ticks;
        float building_ticks;
        int slot;
        for (slot = 0; slot < 20; ++slot)
            level_set_float(s, LVL_OFF_UPGRADE_TIMES + slot * 4,
                            base_ticks);

        vehicle_ticks = cfg_float(cfg, level_name,
                                   K_VEHICLE_UPGRADE_TIME, 0.0f);
        if (vehicle_ticks == 0.0f) vehicle_ticks = base_ticks;
        else vehicle_ticks *= 25.0f;
        level_set_float(s, LVL_OFF_VEHICLE_UPGRADE_TIME, vehicle_ticks);

        minifigure_ticks = cfg_float(cfg, level_name,
                                     K_MINIFIG_UPGRADE_TIME, 0.0f);
        if (minifigure_ticks == 0.0f) minifigure_ticks = base_ticks;
        else minifigure_ticks *= 25.0f;
        level_set_float(s, LVL_OFF_MINIFIG_UPGRADE_TIME,
                        minifigure_ticks);

        building_ticks = cfg_float(cfg, level_name,
                                   K_BUILDING_UPGRADE_TIME, 0.0f);
        if (building_ticks == 0.0f) building_ticks = base_ticks;
        else building_ticks *= 25.0f;
        level_set_float(s, LVL_OFF_BUILDING_UPGRADE_TIME, building_ticks);

        level_set_float(s, LVL_OFF_TRAIN_TIME,
            cfg_float(cfg, level_name, K_TRAIN_TIME, 0.0f) * 25.0f);
    }

    /* LevelLoad applies these configuration flags to DAT_00556ed0 after
     * the upgrade-time fields are initialized. Preserve unrelated bits in
     * the live GameContext word. NoMultiSelect is intentionally set-only in
     * the reference; the other four flags explicitly set or clear their bit. */
    if (game != NULL) {
        if (cfg_typed(cfg, level_name, K_NO_MULTI_SELECT, 0) == 1)
            game->cheat |= 0x00002000u;
        if (cfg_typed(cfg, level_name, K_NO_AUTO_EAT, 0) == 1)
            game->cheat |= 0x00200000u;
        else
            game->cheat &= ~0x00200000u;
        if (cfg_typed(cfg, level_name, K_RECALL_OL_OBJECTS, 0) == 1)
            game->cheat |= 0x00040000u;
        else
            game->cheat &= ~0x00040000u;
        if (cfg_typed(cfg, level_name, K_GENERATE_SPIDERS, 0) == 1)
            game->cheat |= 0x00080000u;
        else
            game->cheat &= ~0x00080000u;
        if (cfg_typed(cfg, level_name, K_DISABLE_TOOLTIP_SOUND, 0) == 1)
            game->cheat |= 0x00100000u;
        else
            game->cheat &= ~0x00100000u;
    }

    level_set_float(s, LEVEL_OFF_TOLERANCE,
                    cfg_float(cfg, level_name, K_BUILDTOL, 0.0f));
    level_set_float(s, LEVEL_OFF_MAXVARIATION,
                    cfg_float(cfg, level_name, K_BUILDMAXVAR, 0.0f));
    /* The target stores this inverted config boolean in its +0xa0 word
     * before the map pipeline begins. That word is reused for the live
     * resource balance later in the level lifetime. */
    level_set_int(s, LEVEL_OFF_DISABLE_START_TELEPORT,
        cfg_typed(cfg, level_name, K_DISABLE_START_TELEPORT, 0) == 1 ? 0 : 1);

    /* --- the ten `.map` keys + their header (bias) words ------------------ */
    char surface[256], predug[256], terrain[256], blockptrs[256];
    char cryore[256], erode[256], emerge[256], aimap[256];
    char pathmap[256], fallin[256];
    const char* names[10] = { surface, predug, terrain, blockptrs,
                              cryore, erode, pathmap, emerge, aimap, fallin };
    const char* keys[10]  = { K_SURFACEMAP, K_PREDUGMAP, K_TERRAINMAP,
                              K_BLOCKPTRMAP, K_CRYOREMAP, K_ERODEMAP,
                              K_PATHMAP, K_EMERGEMAP, K_AIMAP, K_FALLINMAP };
    int biases[10];
    for (int i = 0; i < 10; ++i) {
        char* dst = (char*)names[i];
        biases[i] = map_bias_name(cfg, level_name, keys[i], dst, 256);
    }
    /* Only the three mandatory layers abort the load if absent (the corpus's
     * pipeline gate); the sparse layers (erode/emerge/path/ai/fallin) may be
     * missing — their loaders treat a missing name as an optional layer. */

    /* --- mission-layer helper keys -------------------------------------- */
    char texture_set[256] = { 0 };
    const int has_texture_set = cfg_string(cfg, level_name, K_TEXTURESET,
                                            texture_set,
                                            sizeof(texture_set)) != NULL;
    char ptl_file[256] = { 0 };
    const char* ptl_path = cfg_string(cfg, level_name, "PTLFile", ptl_file,
                                      sizeof(ptl_file));
    long fallin_mult = cfg_int(cfg, level_name, K_FALLIN_MULT, 1);
    if (fallin_mult == 0) {
        fallin_mult = 1;
    }
    /* LevelLoad stores the configured multiplier in DAT_00556f14 before the
     * map pipeline; LoadBlockPointers and the camera grid timer share it. */
    g_camera_scale_14 = (int)fallin_mult;
    {
        float emerge_timeout = cfg_float(cfg, level_name,
                                         K_EMERGE_TIMEOUT, 0.0f);
        if (emerge_timeout == 0.0f) emerge_timeout = 1500.0f;
        level_set_float(s, LEVEL_OFF_EMERGE_TIMEOUT, emerge_timeout);
    }
    {
        char emerge_creature[256] = { 0 };
        int type = 0;
        int row = 0;
        (void)cfg_string(cfg, level_name, K_EMERGE_CREATURE,
                         emerge_creature, sizeof(emerge_creature));
        (void)entity_type_row_for_name(emerge_creature, &type, &row);
        /* LevelLoad stores the resolver's row output at +0xb0; the temporary
         * type output is used only to validate/resolve the configured name. */
        level_set_int(s, LEVEL_OFF_ENGINE_B0, row);
    }

    /* Retail LevelLoad initializes objectives, NERP state, and the advisor
     * token table before invoking MapLoadPipeline. Keep those owners ready
     * for map/cell initialization callbacks that can consult level state. */
    objective_load(cfg, level_name, s);
    {
        char nerp_file[256] = { 0 };
        char nerp_message_file[256] = { 0 };
        const char* script_path = cfg_string(
            cfg, level_name, "NERPFile", nerp_file, sizeof(nerp_file));
        const char* message_path = cfg_string(
            cfg, level_name, "NERPMessageFile", nerp_message_file,
            sizeof(nerp_message_file));
        (void)tutorial_script_file_load(script_path);
        (void)tutorial_objective_token_load(message_path);
    }

    /* These level-owned display/navigation strings are installed before
     * the map pipeline in LevelLoad and are released by level_destroy if a
     * later mandatory map stage fails. */
    {
        char next[256] = { 0 };
        if (cfg_string(cfg, level_name, K_NEXTLEVEL, next,
                       sizeof(next)) != NULL) {
#if defined(_MSC_VER)
            char* dup = _strdup(next);
#else
            char* dup = (char*)malloc(strlen(next) + 1u);
            if (dup != NULL) strcpy(dup, next);
#endif
            if (dup != NULL) level_store_ptr(s, LEVEL_OFF_NEXTLEVEL, dup);
        }
    }
    {
        char full_name[256] = { 0 };
        if (cfg_string(cfg, level_name, K_FULLNAME, full_name,
                       sizeof(full_name)) != NULL) {
            size_t i;
            char* dup;
#if defined(_MSC_VER)
            dup = _strdup(full_name);
#else
            dup = (char*)malloc(strlen(full_name) + 1u);
            if (dup != NULL) strcpy(dup, full_name);
#endif
            if (dup != NULL) {
                for (i = 0; dup[i] != '\0'; ++i) {
                    if (dup[i] == '_') dup[i] = ' ';
                }
                level_store_ptr(s, LEVEL_OFF_FULLNAME, dup);
            }
        }
    }

    {
        int load_ok = level_identifier_remap_table_load(
            ptl_path, util_root_name(cfg));
        if (load_ok) {
            /* TextureSetConfigLoad uses this temporary config context while
             * MapLoadPipeline resolves the named texture set. Keep the
             * borrowed BlockFile pointer out of the global slot during all
             * earlier failure paths and clear it as soon as that load ends. */
            terrain_texture_set_config_context_bind(cfg);
            load_ok = map_pipeline(s, names, biases,
                                   has_texture_set ? texture_set : NULL);
            terrain_texture_set_config_context_bind(NULL);
        }
        if (!load_ok) {
            level_destroy(s);
            if (game != NULL) game->level = previous_level;
            return NULL;
        }
    }
    camera_predug_spawn_state_seed(s);

    /* LevelLoad calls ObjectiveVoiceAliasPrepare(0) then ObjectiveTick with
     * checker/frame deltas of 1 before parsing the object list. The level
     * state machine and alias bookkeeping are represented; DAT_00556140's
     * renderer owner is not, so use its explicit-level adapter with NULL. */
    objective_announce_current(0);
    objective_tick_target(NULL, s, 1.0f, 1.0f);

    /* LevelLoad calls OListEntityRecordParse after MapLoadPipeline and its
     * viewport/no-gather setup, before initial resource counters and the
     * level intro. At this point the active GameContext, Lego.cfg tree, and
     * loaded terrain are all available to the target parser. */
    if (game == NULL || object_list_load(game, cfg, level_name) <= 0) {
        level_destroy(s);
        if (game != NULL) game->level = previous_level;
        return NULL;
    }

    /* Initial-collection counters read by the entity layer (post-load). */
    level_set_int(s, LVL_OFF_INIT_CRYS, (int32_t)cfg_int(cfg, level_name, K_INITCRYSTALS, 0));
    level_set_int(s, LVL_OFF_INIT_ORE, (int32_t)cfg_int(cfg, level_name, K_INITORE, 0));

    /* LevelLoad (0x429890) activates the coordinate lists after OList and
     * initial counters, before the first ordinary tutorial dispatch. */
    tutorial_path_coordinate_index(s);

    return s;
}

/* EntityEffectCountersApplyFromRecord (FUN_0042b2f0): when state bit 8 is
 * active, route the record words at +4/+8 to the global update and resource
 * counter helpers, in decompiled order. */
/* Port model of 0x0042b2f0 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x0042b2f0, retires to the tail in GAME-021 part 3
void entity_effect_counters_apply_from_record(EntitySlot* e,
                                              const void* record)
{
    if (e == NULL || record == NULL
        || (entity_state_flags_get(e) & 8u) == 0u) {
        return;
    }
    uint32_t count_a_bits = 0u;
    uint32_t count_b_bits = 0u;
    int count_b;
    GameContext* context = e->owner_ctx;
    __builtin_memcpy(&count_a_bits, (const char*)record + 4,
                     sizeof(count_a_bits));
    __builtin_memcpy(&count_b_bits, (const char*)record + 8,
                     sizeof(count_b_bits));
    if (context == NULL) context = entity_pool_callback_context_get();

    /* FUN_0042b2f0 forwards +4 to SimulationUpdateCounterAdd, whose tail
     * also performs EntityActivitySceneStateUpdate. */
    simulation_update_counter_add(context, (int)count_a_bits);

    /* FUN_0043a910 calls LevelResourceBalanceAddAndSound(0) once per +8
     * count. Keep that operation on the command module's active context so
     * its global resource counter and overlay notification stay paired. */
    count_b = (int)count_b_bits;
    while (count_b != 0) {
        level_resource_balance_add_and_sound(0);
        --count_b;
    }
}

/* LoadTileMeshes (FUN_0042b330): the level owns a row-major pointer table of
 * TileModelEntityRegister records. The editor path leaves it zeroed; normal
 * play registers one paired visual record for every surface-text cell. */
/* Port model of 0x0042b330 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x0042b330, retires to the tail in GAME-021 part 3
int terrain_tile_mesh_grid_load_level(LevelState* level,
                                      const char* bitmap_prefix)
{
    TerrainTileMeshGrid* grid;
    size_t count;
    uint32_t y;
    if (level == NULL) return 0;
    grid = level->ptr_tile_mesh_grid;
    if (grid == NULL) return 0;
    grid->owner_level = level;
    if (grid->width == 0u || grid->height == 0u ||
        grid->width > 65535u || grid->height > 65535u ||
        (size_t)grid->width > ((size_t)-1) / (size_t)grid->height ||
        (size_t)grid->width * (size_t)grid->height >
            ((size_t)-1) / sizeof(void*)) return 0;
    grid->bitmap_prefix = bitmap_prefix;
    terrain_tile_mesh_grid_release(grid);
    count = (size_t)grid->width * (size_t)grid->height;
    grid->meshes = (SceneCellVisualRelayRecord**)calloc(
        count, sizeof(SceneCellVisualRelayRecord*));
    if (grid->meshes == NULL) return 0;
    if (grid->editor_mode) return 1;
    if (grid->bitmap_prefix == NULL) {
        terrain_tile_mesh_grid_release(grid);
        return 0;
    }
    /* The retail loop indexes the pointer table by y * width + x. Its
     * registration failure is nonfatal: a failed scene resource leaves a
     * null slot while the rest of the cell grid is still initialized. */
    if (grid->register_mesh == NULL) return 1;
    for (y = 0u; y < grid->height; ++y) {
        uint32_t x;
        for (x = 0u; x < grid->width; ++x) {
            char bitmap_a[256];
            char bitmap_b[256];
            size_t prefix_len = strlen(grid->bitmap_prefix);
            size_t index = (size_t)y * (size_t)grid->width + x;
            if (prefix_len + 16u >= sizeof(bitmap_a)) {
                terrain_tile_mesh_grid_release(grid);
                return 0;
            }
            strcpy(bitmap_a, grid->bitmap_prefix);
            strcpy(bitmap_b, grid->bitmap_prefix);
            sprintf(bitmap_a + prefix_len, "%x%xa", (unsigned int)x,
                    (unsigned int)y);
            sprintf(bitmap_b + prefix_len, "%x%xb", (unsigned int)x,
                    (unsigned int)y);
            grid->meshes[index] = grid->register_mesh(
                grid->scene, bitmap_a, bitmap_b, grid->material,
                grid->options, grid->context);
            if (grid->meshes[index] != NULL) {
                SceneCellVisualRelayRecord* visual = grid->meshes[index];
                visual->lookup_table =
                    (SceneCellVisualLookupTable*)grid->visual_lookup_table;
                visual->projection_scale = grid->cell_projection_scale;
            }
        }
    }
    return 1;
}

/* Port model of 0x0042b480 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x0042b480, retires to the tail in GAME-021 part 3
void rectangular_resource_table_destroy(RectangularResourceTable* table)
{
    unsigned long i;
    unsigned long total;
    if (table == NULL || table->resources == NULL) return;
    total = (unsigned long)table->width * (unsigned long)table->height;
    for (i = 0; i < total; ++i) {
        if (s_resource_entry_release_hook != NULL)
            s_resource_entry_release_hook(table->resources[i]);
        else
            unported_ledger_stubs::cell_visual_channel_table_dispose(
                (unported_ledger_stubs::CellVisualChannelTable*)
                    table->resources[i]);
    }
    if (s_resource_array_release_hook != NULL)
        s_resource_array_release_hook(table->resources);
    else
        engine_heap_free(table->resources);
    table->resources = NULL;
}

/* GlobalStateField84Get (FUN_0042b500, findings 324/392): the retail getter
 * reads +0x84 from the active level global and takes no arguments. */
/* Port model of 0x0042b510 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x0042b510, retires to the tail in GAME-021 part 3
static int map_pipeline(LevelState* s, const char** names, const int* biases,
                        const char* texture_set_name)
{
    /* `names` follows the ten config keys in LevelLoad. The corpus then
     * forwards them in a different positional order: Predug, Terrain, Path,
     * Erode, Fallin, Emerge, AI, BlockPointers, then the TerrainMap pass. */
    TerrainState* terrain = (TerrainState*)terrain_create_retail(
        scene_root_object_get(), names[0], level_float(s, LEVEL_OFF_BLOCKSIZE),
        level_float(s, LEVEL_OFF_ROUGHLEVEL));
    if (terrain == NULL) {
        return 0;
    }
    terrain->owner_level = s;
    level_store_ptr(s, LEVEL_OFF_TERRAIN, terrain);
    {
        uint32_t slot_index;
        /* TerrainCreate initializes all ten transition mesh handles to -1.
         * MapLoadPipeline's six IndexedFlagValueSetOne(0x70) calls all set
         * this same material suppression entry; their result is idempotent. */
        for (slot_index = 0u; slot_index < 10u; ++slot_index)
            s->material_transition_slots[slot_index].mesh_token = 0xffffffffu;
        s->material_transition_suppress_flags[0x70u] = 1u;
    }
    level_set_int(s, LEVEL_OFF_WIDTH, terrain->width);
    level_set_int(s, LEVEL_OFF_HEIGHT, terrain->height);
#if defined(LEGORRR_BACKEND_WINAPI) && defined(_WIN32) && !defined(_WIN64)
    if (level_int(s, LEVEL_OFF_USEROOF) != 0 &&
        !terrain_roof_owner_initialize(s)) return 0;
#endif
    /* TerrainCreate's geometry buffer is represented by TerrainState's CPU
     * quad records. Seed the native scene cache from those records before
     * CellInitPass can mutate orientations or heights. */
    if (terrain->cell_vertices != NULL) {
        int cell_x, cell_y;
        for (cell_y = 0; cell_y + 1 < terrain->height; ++cell_y) {
            for (cell_x = 0; cell_x + 1 < terrain->width; ++cell_x) {
                size_t index = ((size_t)cell_y *
                                (size_t)(terrain->width - 1) +
                                (size_t)cell_x) * 12u;
                float cached[12];
                uint8_t* cell_record = terrain->blocks +
                    ((size_t)cell_y * (size_t)terrain->width +
                     (size_t)cell_x) * TERRAIN_BLOCK_STRIDE;
                /* Retain geometry Y = -height byte * RoughLevel, exactly as
                 * the later CPU-quad mutation producer does. */
                memcpy(cached, terrain->cell_vertices + index,
                       sizeof(cached));
                scene_render_terrain_cell_vertices_update(
                    s, (uint32_t)cell_x, (uint32_t)cell_y, cached);
                if ((cell_record[0x19] & 1u) != 0u)
                    scene_render_terrain_cell_orientation_apply(
                        s, (uint32_t)cell_x, (uint32_t)cell_y, 1);
            }
        }
    }
    /* TerrainCellMutation's 0x46e220 step records the live cell's four
     * geometry-space heights in SceneRender's row-major CPU quad cache. */
    terrain_cell_overlay_height_fill_hook_set(
        scene_render_terrain_cell_overlay_height_fill, NULL);
    /* MapCellStateBufferRebuild @ 0x40ce80 is called as soon as the terrain
     * dimensions are known; own its per-cell links and pool on this level. */
    map_cell_state_buffer_rebuild(s);
    /* GridCellRecordBufferReplace @ 0x466230 follows the map-cell pool
     * rebuild in MapLoadPipeline and owns a distinct entity/timer/flag row
     * for every map coordinate. */
    level_field_grid_buffer_replace(s);

    /* TextureSetConfigLoad is a required pipeline gate: the reference does
     * not allocate cell records until config dimensions, texture resources,
     * tile meshes, and the roof texture have all been installed. */
    if (texture_set_name == NULL || texture_set_name[0] == '\0') return 0;
    {
        TerrainTileMeshGrid* tile_grid = (TerrainTileMeshGrid*)calloc(
            1, sizeof(TerrainTileMeshGrid));
        if (tile_grid == NULL) return 0;
        tile_grid->owner_level = s;
        /* LoadTileMeshes receives DAT_005560d8 as its scene manager. Terrain
         * creation captured that same root before the texture-set pass. */
        tile_grid->scene = terrain->scene_context;
        tile_grid->width = terrain->width;
        tile_grid->height = terrain->height;
        tile_grid->editor_mode =
            (engine_feature_word_peek() & 0x800000u) != 0u;
        tile_grid->cell_projection_scale =
            level_float(s, LEVEL_OFF_BLOCKSIZE);
        s->ptr_tile_mesh_grid = tile_grid;
        if (!terrain_texture_set_config_load_level(s, texture_set_name))
            return 0;
    }

    size_t w = (size_t)terrain->width, h = (size_t)terrain->height;
    uint8_t* vox = (uint8_t*)calloc(w * h, LEVEL_VOXEL_STRIDE);
    if (vox == NULL) {
        terrain_destroy(terrain);
        level_store_ptr(s, LEVEL_OFF_TERRAIN, NULL);
        return 0;
    }
    level_store_ptr(s, LEVEL_OFF_VOXELS, vox);
    for (size_t y = 0; y < h; ++y) {
        for (size_t x = 0; x < w; ++x) {
            uint16_t seed = level_rand_u16();
            __builtin_memcpy(vox + (y * w + x) * LEVEL_VOXEL_STRIDE + 0x20, &seed, 2);
        }
    }

    /* Layer order is the corpus call order; a header-carrying or mandatory
     * layer failing aborts the pipeline (0) exactly as the corpus does. */
    int ok = load_predug(s, names[1], biases[1]);
    if (!ok) { return 0; }
    ok = load_erode_cryore(s, names[2], biases[2]);
    if (!ok) { return 0; }
    load_path_ai(s, names[4], biases[4]);
    load_emerge(s, names[5]);
    load_fallin(s, names[6], biases[6]);
    load_layer_l(s, names[7]);
    load_layer_m(s, names[8]);
    load_block_pointers(s, names[9]);
    ok = load_terrain_map(s, names[3], biases[3]);
    if (ok) {
        level_set_int(s, LEVEL_OFF_TERRAIN_LOADED, 1);
    }
    cell_init_all(s);
    {
        TerrainSurfaceGrid* surface = level_surface_grid_get(s);
        if (surface != NULL)
            terrain_grid_cells_initialize(surface);
    }
    terrain_cell_quad_run_stage_level(NULL, s);
    return 1;
}

/* Retail TextureSetConfigLoad receives the level record and resolves its
 * +0x74/+0x78 grid and +0x04 terrain owner before loading resources. */
/* Port model of 0x0042bb60 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x0042bb60, retires to the tail in GAME-021 part 3
int terrain_texture_set_config_load_level(LevelState* level,
                                          const char* group_name)
{
    if (level == NULL || level->ptr_tile_mesh_grid == NULL) return 0;
    level->ptr_tile_mesh_grid->owner_level = level;
    return terrain_texture_set_config_load_grid(
        level->ptr_tile_mesh_grid, group_name);
}

/* LoadPredugLayer @ 0x42bd20 */
/* Port model of 0x0042bd20 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x0042bd20, retires to the tail in GAME-021 part 3
static int load_predug(LevelState* s, const char* name, int bias)
{
    s_predug_coordinate_count = 0;
    MapFile* m = level_map_open(name);
    if (m == NULL) {
        return 0;
    }
    if (!dims_ok(m, s)) {
        level_map_close(m);
        return 0;
    }
    for (int y = 1; y + 1 < m->height; ++y) {
        for (int x = 1; x + 1 < m->width; ++x) {
            int v = (int)level_map_cell(m, x, y) - bias;
            uint8_t* rec = voxel_at(s, x, y);
            uint32_t f8 = 0, fC = 0;
            if (v == 1 || v == 3) {
                /* Corpus cell update first (sets cave/traversable marks),
                 * then the edge-wall OR. */
                predug_wall_cell(s, x, y);
                /* Edge-wall: the corpus ORs 0x4000000 into the 3×3
                 * neighbourhood's +0x08 words. */
                for (int dy = -1; dy <= 1; ++dy) {
                    for (int dx = -1; dx <= 1; ++dx) {
                        int qx = x + dx, qy = y + dy;
                        if (qx >= 0 && qy >= 0 && qx < m->width && qy < m->height) {
                            uint32_t* f = (uint32_t*)(voxel_at(s, qx, qy) + 0x08);
                            *f |= 0x4000000u;
                        }
                    }
                }
            } else if (v == 2 || v == 4) {
                f8 |= 0x20000u;
            }
            if (v == 3) {
                fC |= 0x20u;
                const int cell_xy[2] = {x, y};
                predug_coordinate_pool_append(cell_xy);
            }
            if (v == 4) {
                fC |= 0x200u;
            }
            if (f8) *(uint32_t*)(rec + 0x08) |= f8;
            if (fC) *(uint32_t*)(rec + 0x0C) |= fC;
        }
    }
    level_map_close(m);
    return 1;
}

/* LoadEmergeLayer @ 0x42bf40 — even nonzero values seed an erosion stage;
 * zero and odd values are rounded up before the voxel's +0x06 write. */
/* Port model of 0x0042bf40 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x0042bf40, retires to the tail in GAME-021 part 3
static int load_emerge(LevelState* s, const char* name)
{
    uint32_t width;
    uint32_t height;
    int32_t image_size;
    int slot = map_open2(name, &image_size);
    uint8_t* voxels;
    if (slot == -1) {
        return 0;
    }
    level_map_dimensions_get((uint32_t)slot, &width, &height);
    if (width != level_cptr(s)[LEVEL_OFF_WIDTH / 4] ||
        height != level_cptr(s)[LEVEL_OFF_HEIGHT / 4]) {
        level_map_slot_release((uint32_t)slot);
        return 0;
    }
    voxels = (uint8_t*)level_load_ptr(s, LEVEL_OFF_VOXELS);
    for (uint32_t y = 0; y < height; ++y) {
        for (uint32_t x = 0; x < width; ++x) {
            uint32_t index = y * width + x;
            uint16_t value = level_map_cell_get((uint32_t)slot,
                                                (int)x, (int)y);
            int seed_stage = 0;
            if (value == 0u || (value & 1u) != 0u) {
                ++value;
            } else {
                seed_stage = 1;
            }
            voxels[index * LEVEL_VOXEL_STRIDE + 0x06] =
                (uint8_t)(value >> 1);
            if (seed_stage != 0) {
                int cell_xy[2];
                int16_t random_value = (int16_t)level_rand_u16();
                int sign = random_value >> 31;
                int magnitude = ((int)random_value ^ sign) - sign;
                int phase = ((magnitude & 3) ^ sign) - sign;
                cell_xy[0] = (int)x;
                cell_xy[1] = (int)y;
                lava_erosion_stage_register(cell_xy, phase);
            }
        }
    }
    (void)image_size;
    level_map_slot_release((uint32_t)slot);
    return 1;
}

/* LoadLayerM @ 0x42c060 — reads the AIMap, writes raw byte at voxel +0x34. */
/* Port model of 0x0042c060 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x0042c060, retires to the tail in GAME-021 part 3
static int load_layer_m(LevelState* s, const char* name)
{
    uint32_t width;
    uint32_t height;
    int32_t image_size;
    int slot = map_open2(name, &image_size);
    uint8_t* voxels;
    if (slot == -1) {
        return 0;
    }
    level_map_dimensions_get((uint32_t)slot, &width, &height);
    if (width != level_cptr(s)[LEVEL_OFF_WIDTH / 4] ||
        height != level_cptr(s)[LEVEL_OFF_HEIGHT / 4]) {
        level_map_slot_release((uint32_t)slot);
        return 0;
    }
    voxels = (uint8_t*)level_load_ptr(s, LEVEL_OFF_VOXELS);
    for (uint32_t y = 0; y < height; ++y) {
        for (uint32_t x = 0; x < width; ++x) {
            uint32_t index = y * width + x;
            voxels[index * LEVEL_VOXEL_STRIDE + 0x34] =
                (uint8_t)level_map_cell_get((uint32_t)slot, (int)x, (int)y);
        }
    }
    (void)image_size;
    level_map_slot_release((uint32_t)slot);
    return 1;
}
/* LoadLayerL @ 0x42c120 — reads the EmergeMap; ORs `0x40`/`0x80` flag bits
 * at voxel +0x0C and appends 0x4C-stride records into level+0x08. */
/* Port model of 0x0042c120 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x0042c120, retires to the tail in GAME-021 part 3
static int load_layer_l(LevelState* s, const char* name)
{
    uint32_t width;
    uint32_t height;
    int32_t image_size;
    int slot;
    uint8_t* voxels;
    uint8_t* records = (uint8_t*)level_load_ptr(s, LEVEL_OFF_LAYERL_RECORDS);
    int      count   = level_int(s, LEVEL_OFF_LAYERL_COUNT);

    /* MapOpen2's retail entry takes only (path, size_out), with the map-slot
     * owner installed separately. Layer L then uses the same slot helpers as
     * the decompile instead of allocating a host MapFile wrapper. */
    slot = map_open2(name, &image_size);
    if (slot == -1) {
        return 0;
    }
    level_map_dimensions_get((uint32_t)slot, &width, &height);
    if (width != level_cptr(s)[LEVEL_OFF_WIDTH / 4] ||
        height != level_cptr(s)[LEVEL_OFF_HEIGHT / 4]) {
        level_map_slot_release((uint32_t)slot);
        return 0;
    }
    voxels = (uint8_t*)level_load_ptr(s, LEVEL_OFF_VOXELS);
    for (uint32_t y = 0; y < height; ++y) {
        for (uint32_t x = 0; x < width; ++x) {
            uint16_t c = level_map_cell_get((uint32_t)slot, (int)x, (int)y);
            if (c == 0) {
                continue;
            }
            int record_slot = ((int)c - 1) % 6;
            int rec  = ((int)c - 1) / 6;
            if (rec >= count) {
                int next_count = rec + 1;
                size_t old_size = (size_t)count * 0x4Cu;
                size_t new_size = (size_t)next_count * 0x4Cu;
                uint8_t* grown;
                /* The array is released with free() by level_destroy and
                 * grown with realloc(); on native builds engine_heap_alloc
                 * is the CRT pool allocator, whose blocks free()/realloc()
                 * cannot take, so allocate with malloc (the same call on the
                 * reference build, where engine_heap_alloc is a macro). */
                if (records == NULL)
                    grown = (uint8_t*)malloc(new_size);
                else
                    grown = (uint8_t*)realloc(records, new_size);
                if (grown == NULL) {
                    level_store_ptr(s, LEVEL_OFF_LAYERL_RECORDS, records);
                    level_set_int(s, LEVEL_OFF_LAYERL_COUNT, count);
                    level_map_slot_release((uint32_t)slot);
                    return 0;
                }
                records = grown;
                __builtin_memset(records + old_size, 0, new_size - old_size);
                count = next_count;
                level_store_ptr(s, LEVEL_OFF_LAYERL_RECORDS, records);
                level_set_int(s, LEVEL_OFF_LAYERL_COUNT, count);
            }
            uint8_t* recp = records + (size_t)rec * 0x4C;
            uint8_t* vox = voxels + ((size_t)y * width + x) * 0x48u;
            if (record_slot == 0) {
                *(uint32_t*)(vox + 0x0C) |= 0x80u;
                *(uint32_t*)(recp + 4) = x;
                *(uint32_t*)(recp + 8) = y;
            } else {
                *(uint32_t*)(vox + 0x0C) |= 0x40u;
                *(uint32_t*)(recp + (size_t)record_slot * 0xC + 4) = x;
                *(uint32_t*)(recp + (size_t)record_slot * 0xC + 8) = y;
                *(uint32_t*)(recp +
                    (size_t)(record_slot + 1) * 0xC) = 1u;
            }
        }
    }
    level_store_ptr(s, LEVEL_OFF_LAYERL_RECORDS, records);
    level_set_int(s, LEVEL_OFF_LAYERL_COUNT, count);
    (void)image_size;
    level_map_slot_release((uint32_t)slot);
    return 1;
}

namespace unported_ledger_stubs {
/* Port model of 0x0042c330 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x0042c330, retires to the tail in GAME-021 part 3
int ledger_stub_0042c330_EmergeMapTriggerSpawn(
    LevelState* level, const int cell_xy[2], int output_xy[2])
{
    unsigned char* records;
    uint32_t record_count;
    uint32_t owner_index;
    GameContext* context;
    const EntitySlot* base_entity;
    uint32_t record_index;
    if (level == NULL || cell_xy == NULL || level->ptr_layerl == NULL)
        return 0;
    records = (unsigned char*)level->ptr_layerl;
    record_count = level->word[LEVEL_OFF_LAYERL_COUNT / 4];
    owner_index = level->word[0xb0 / 4];
    context = entity_pool_callback_context_get();
    if (context == NULL || context->level != level) return 0;
    base_entity = entity_at(context, 3, (int)owner_index);
    if (emergence_type_name_get((int)owner_index) == NULL &&
        base_entity == NULL) return 0;
    for (record_index = 0u; record_index < record_count; ++record_index) {
        unsigned char* record = records + record_index * 0x4cu;
        uint32_t slot_index;
        int record_x, record_y;
        float timer;
        memcpy(&record_x, record + 4, sizeof(record_x));
        memcpy(&record_y, record + 8, sizeof(record_y));
        memcpy(&timer, record + 0x0c, sizeof(timer));
        /* The retail branch is timer <= 0.0. Express the negation directly
         * so unordered NaN values remain ineligible, as they are there. */
        if (record_x != cell_xy[0] || record_y != cell_xy[1] ||
            !(timer <= 0.0f)) continue;
        for (slot_index = 0u; slot_index < 5u; ++slot_index) {
            unsigned char* entry = record + 0x18u + slot_index * 0x0cu;
            int spawn_x, spawn_y;
            uint32_t populated;
            EntitySlot* spawned;
            memcpy(&populated, entry, sizeof(populated));
            if (populated == 0u) continue;
            memcpy(&spawn_x, entry - 8, sizeof(spawn_x));
            memcpy(&spawn_y, entry - 4, sizeof(spawn_y));
            spawned = creature_spawn_from_emergence(
                (void*)base_entity, 3, (int)owner_index, spawn_x, spawn_y);
            if (spawned == NULL) continue;
            memcpy(record + 0x0c, &level->word[0x264 / 4], sizeof(float));
            if (output_xy != NULL) {
                output_xy[0] = spawn_x;
                output_xy[1] = spawn_y;
            }
            return 1;
        }
    }
    return 0;
}
} // namespace unported_ledger_stubs

/* Port model of 0x0042c440 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x0042c440, retires to the tail in GAME-021 part 3
void camera_grid_timers(EngineContext* ctx, float dt)
{
    EngineState* st = ctx != NULL ? ctx->state_block : NULL;
    if (st == NULL || st->move_list == NULL || st->move_list_count == 0) {
        return;
    }
    uint8_t* base = (uint8_t*)st->move_list;
    for (uint32_t i = 0; i < st->move_list_count; ++i) {
        float* f = (float*)(base + 0xc + i * 0x4c);
        if (*f > 0.0f) {
            *f -= dt;
        }
    }
}

/* LoadErodeCryoreLayers @ 0x42c480 — reads the TerrainMap (Surf materials),
 * writes the erosion/cryo byte at voxel +0x02. */
/* Port model of 0x0042c480 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x0042c480, retires to the tail in GAME-021 part 3
static int load_erode_cryore(LevelState* s, const char* name, int bias)
{
    uint32_t width;
    uint32_t height;
    int32_t image_size;
    int slot = map_open2(name, &image_size);
    uint8_t* voxels;
    if (slot == -1) {
        return 0;
    }
    level_map_dimensions_get((uint32_t)slot, &width, &height);
    if (width != level_cptr(s)[LEVEL_OFF_WIDTH / 4] ||
        height != level_cptr(s)[LEVEL_OFF_HEIGHT / 4]) {
        level_map_slot_release((uint32_t)slot);
        return 0;
    }
    voxels = (uint8_t*)level_load_ptr(s, LEVEL_OFF_VOXELS);
    for (uint32_t y = 0; y < height; ++y) {
        for (uint32_t x = 0; x < width; ++x) {
            uint32_t index = y * width + x;
            int value = (int)level_map_cell_get((uint32_t)slot,
                                                (int)x, (int)y) - bias;
            uint8_t* record = voxels + index * LEVEL_VOXEL_STRIDE;
            if (value == 5) {
                value = 4;
            }
            record[0x02] = (uint8_t)value;
            if ((int8_t)record[0x02] == 6) {
                record[0x07] = 4;
            } else if ((int8_t)record[0x02] == 11) {
                /* rock-monster / cavern spawn point */
                int cell_xy[2];
                cell_xy[0] = (int)x;
                cell_xy[1] = (int)y;
                unported_ledger_stubs::type11_cell_coordinate_append(
                    cell_xy);
                terrain_activity_build_record_update(s, (int)x, (int)y, 1);
            }
        }
    }
    (void)image_size;
    level_map_slot_release((uint32_t)slot);
    return 1;
}

/* Retail ABI: coordinate pair plus four output words, with level ownership
 * obtained through DAT_005560d4. Existing game code uses the explicit-level
 * helper above so noncurrent loads can classify cells without changing the
 * target entry's ownership contract. */
/* Port model of 0x0042c5b0 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x0042c5b0, retires to the tail in GAME-021 part 3
int terrain_cell_byte_state_classify_current(const int xy[2],
    uint32_t* out_a, uint32_t* out_b, uint32_t* out_c, uint32_t* out_d)
{
    uint32_t values[5];
    uint32_t value;
    uint32_t index;
    LevelState* level = g_active_level_retail;
    uint32_t width = level->word[LEVEL_OFF_WIDTH / 4u];
    uint32_t cell_index = (uint32_t)xy[1] * width + (uint32_t)xy[0];
    const uint8_t* voxels;
#if defined(_WIN32) && defined(_M_IX86)
    voxels = (const uint8_t*)(uintptr_t)
        level->word[LEVEL_OFF_VOXELS / 4u];
#else
    voxels = (const uint8_t*)level->ptr_voxels;
#endif
    value = (uint32_t)voxels[(size_t)cell_index * 0x48u + 5u];
    values[0] = 1u;
    values[1] = 3u;
    values[2] = 5u;
    values[3] = 10u;
    values[4] = 25u;
    index = value - 1u;
    *out_d = 0u;
    *out_c = 0u;
    *out_b = 0u;
    *out_a = 0u;
    if (value == 0u) return 0;
    value = *(uint32_t*)((uint8_t*)values + (index & 0xfffffffcu));
    switch (index & 3u) {
        case 0u: *out_a = value; return 1;
        case 1u: *out_c = value; return 1;
        case 2u: *out_b = value; return 1;
        default: *out_d = value; return 1;
    }
}
/* LoadPathAiLayers @ 0x42c6a0 — reads the CryOreMap, writes path/AI byte
 * (value − bias) at voxel +0x05. */
/* Port model of 0x0042c6a0 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x0042c6a0, retires to the tail in GAME-021 part 3
static int load_path_ai(LevelState* s, const char* name, int bias)
{
    uint32_t width;
    uint32_t height;
    int32_t image_size;
    int slot = map_open2(name, &image_size);
    uint8_t* voxels;
    int8_t bias_byte = (int8_t)bias;
    if (slot == -1) {
        return 0;
    }
    level_map_dimensions_get((uint32_t)slot, &width, &height);
    if (width != level_cptr(s)[LEVEL_OFF_WIDTH / 4] ||
        height != level_cptr(s)[LEVEL_OFF_HEIGHT / 4]) {
        level_map_slot_release((uint32_t)slot);
        return 0;
    }
    voxels = (uint8_t*)level_load_ptr(s, LEVEL_OFF_VOXELS);
    for (uint32_t y = 0; y < height; ++y) {
        for (uint32_t x = 0; x < width; ++x) {
            uint32_t index = y * width + x;
            int8_t value = (int8_t)(uint8_t)
                level_map_cell_get((uint32_t)slot, (int)x, (int)y);
            voxels[index * LEVEL_VOXEL_STRIDE + 0x05] =
                (uint8_t)(value - bias_byte);
        }
    }
    (void)image_size;
    level_map_slot_release((uint32_t)slot);
    return 1;
}

/* LoadFallinLayer @ 0x42c760 — reads the PathMap, ORs fallin flag bits into
 * voxel +0x08 over an existing cave flag (the corpus's `(flags & 8)` gate). */
/* Port model of 0x0042c760 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x0042c760, retires to the tail in GAME-021 part 3
static int load_fallin(LevelState* s, const char* name, int bias)
{
    MapFile* m = level_map_open(name);
    if (m == NULL) {
        return 0;
    }
    if (!dims_ok(m, s)) {
        level_map_close(m);
        return 0;
    }
    for (int y = 0; y < m->height; ++y) {
        for (int x = 0; x < m->width; ++x) {
            int v = (int)level_map_cell(m, x, y) - bias;
            uint8_t* rec = voxel_at(s, x, y);
            uint32_t flags;
            __builtin_memcpy(&flags, rec + 0x08, 4);
            if ((flags & 8) != 0) {
                if (v == 2) {
                    flags |= 0x20000000u;
                } else if (v == 1) {
                    /* LoadFallinLayer runs TerrainCellWorkFlagClear with
                     * selector 0x3d for this branch. That helper owns the
                     * descriptor veto, activity update, coordinate callback,
                     * and four queued events before restoring flag bits 0/1. */
                    const int cell_xy[2] = { x, y };
                    terrain_cell_work_flag_clear_current(cell_xy, 0x3du);
                    continue;
                }
                __builtin_memcpy(rec + 0x08, &flags, 4);
            }
        }
    }
    level_map_close(m);
    return 1;
}

/* LoadBlockPointers @ 0x42c9d0 — reads the FallinMap, writes the block
 * pointer count/flag at voxel +0x38/+0x3C and the hash mod at +0x40.
 * (Key→loader roles are the code's mapping, see level.hpp.) */
/* Port model of 0x0042c9d0 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x0042c9d0, retires to the tail in GAME-021 part 3
static int load_block_pointers(LevelState* s, const char* name)
{
    /* DAT_00556f18 is also the camera's block-pointer-grid enable word. */
    uint32_t width;
    uint32_t height;
    int32_t image_size;
    int slot;
    uint8_t* voxels;
    g_camera_grid_enabled = 0;
    if (name == NULL) {
        return 0;
    }
    slot = map_open2(name, &image_size);
    if (slot == -1) {
        return 0;
    }
    level_map_dimensions_get((uint32_t)slot, &width, &height);
    if (width != level_cptr(s)[LEVEL_OFF_WIDTH / 4] ||
        height != level_cptr(s)[LEVEL_OFF_HEIGHT / 4]) {
        level_map_slot_release((uint32_t)slot);
        return 0;
    }
#if defined(_MSC_VER) && defined(_M_IX86)
    voxels = (uint8_t*)(uintptr_t)
        level_cptr(s)[LEVEL_OFF_VOXELS / 4];
#else
    voxels = (uint8_t*)level_load_ptr(s, LEVEL_OFF_VOXELS);
#endif
    for (uint32_t y = 0; y < height; ++y) {
        for (uint32_t x = 0; x < width; ++x) {
            uint32_t index = y * width + x;
            uint8_t* record = voxels + index * LEVEL_VOXEL_STRIDE;
            uint32_t v = level_map_cell_get((uint32_t)slot,
                                            (int)x, (int)y);
            *(uint32_t*)(record + 0x3C) = 0u;
            if (v != 0) {
                int16_t seed = *(int16_t*)(record + 0x20);
                uint32_t hash = (uint32_t)(int32_t)seed %
                    (v * (uint32_t)g_camera_scale_14 * 0x19u);
                *(uint32_t*)(record + 0x3C) = v;
                *(float*)(record + 0x40) = (float)hash;
                *(uint32_t*)(record + 0x38) = 0u;
                if (v > 4) {
                    *(uint32_t*)(record + 0x3C) = v - 4u;
                    *(uint32_t*)(record + 0x38) = 1u;
                }
            }
        }
    }
    (void)image_size;
    level_map_slot_release((uint32_t)slot);
    g_camera_grid_enabled = 1;
    return 1;
}

/* Port model of 0x0042cb70 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x0042cb70, retires to the tail in GAME-021 part 3
void camera_grid_expiry(EngineContext* ctx, float dt)
{
    EngineState* st = ctx != NULL ? ctx->state_block : NULL;
    /* Prefer the bound level's voxel grid (width/height +0x14/0x18, voxels
     * via the resolved pointer — the raw +0xb8 word truncates on 64-bit). */
    const uint8_t* cells = NULL;
    uint32_t gw = 0, gh = 0;
    if (ctx != NULL && ctx->level_block != NULL) {
        gw = (uint32_t)engine_level_width(ctx);
        gh = (uint32_t)engine_level_height(ctx);
        cells = ctx->level_voxels;
        if (cells == NULL) {
            cells = (const uint8_t*)engine_level_ptr(ctx, 0xb8);
        }
    } else if (st != NULL) {
        gw = st->grid_w;
        gh = st->grid_h;
        cells = (const uint8_t*)st->grid_cells;
    }
    if (!g_camera_grid_enabled || gw == 0 || gh == 0 || cells == NULL) {
        return;
    }
    for (uint32_t y = 0; y < gh; ++y) {
        for (uint32_t x = 0; x < gw; ++x) {
            uint32_t idx = y * gw + x;
            uint8_t* cell = (uint8_t*)cells + idx * 0x48;
            uint32_t busy = *(uint32_t*)(cell + 0x3c);
            if (busy == 0) {
                continue;
            }
            float* timer = (float*)(cell + 0x40);
            *timer += dt;
            uint32_t spent = busy * (uint32_t)g_camera_scale_14 * 0x19;
            if ((float)spent < *timer) {
                *timer = 0.0f;
                uint32_t coords[2] = { x, y };
                uint32_t hint = *(uint32_t*)(cell + 0x38);
                if (grid_spawn(coords, hint)) {
                    message_dispatch(6, 0, 0, coords); /* corpus FUN_00419b80 */
                }
            }
        }
    }
}

/* LoadTerrainMap @ 0x42cc90 — reads the BlockPointersMap, writes surface
 * type byte (value − bias) at voxel +0x04 (mapping note: level.hpp). */
/* Port model of 0x0042cc90 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x0042cc90, retires to the tail in GAME-021 part 3
static int load_terrain_map(LevelState* s, const char* name, int bias)
{
    uint32_t width;
    uint32_t height;
    int32_t image_size;
    int slot = map_open2(name, &image_size);
    uint8_t* voxels;
    int8_t bias_byte = (int8_t)bias;
    if (slot == -1) {
        return 0;
    }
    level_map_dimensions_get((uint32_t)slot, &width, &height);
    if (width != level_cptr(s)[LEVEL_OFF_WIDTH / 4] ||
        height != level_cptr(s)[LEVEL_OFF_HEIGHT / 4]) {
        level_map_slot_release((uint32_t)slot);
        return 0;
    }
    voxels = (uint8_t*)level_load_ptr(s, LEVEL_OFF_VOXELS);
    for (uint32_t y = 0; y < height; ++y) {
        for (uint32_t x = 0; x < width; ++x) {
            uint32_t index = y * width + x;
            int8_t value = (int8_t)(uint8_t)
                level_map_cell_get((uint32_t)slot, (int)x, (int)y);
            voxels[index * LEVEL_VOXEL_STRIDE + 0x04] =
                (uint8_t)(value - bias_byte);
        }
    }
    (void)image_size;
    level_map_slot_release((uint32_t)slot);
    return 1;
}

/* Port model of 0x0042cd50 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x0042cd50, retires to the tail in GAME-021 part 3
const void* upgrade_name_entry_find(const char* name)
{
    /* Install clamps negatives to zero. Corpus 0x42cd5b/0x42cd7f uses
     * unsigned count gates and an unsigned index bound. */
    if (name == NULL || s_upgrade_count == 0u || s_upgrade_records == NULL) {
        /* deviation: the corpus strides a dangling record base blindly */
        return NULL;
    }
    for (unsigned i = 0; i < s_upgrade_count; ++i) {
        const char* entry = s_upgrade_names[i];
        /* Corpus 0x42cd6b calls the annotated strcasecmp_lite twin at
         * 0x48ff10; this caller observes equality only. */
        if (entry != NULL && strcasecmp_lite(name, entry) == 0) {
            return (const char*)s_upgrade_records + (size_t)i * 16;
        }
    }
    return NULL;
}

extern "C" int entity_vehicle_type_table_load(void);
/* VehicleTypesLoad (FUN_0042cda0) likewise loads 0x74-byte visual rows. */
/* Port model of 0x0042cda0 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x0042cda0, retires to the tail in GAME-021 part 3
int entity_vehicle_type_table_load(void)
{
    ENTITY_TYPE_CONFIG_TABLE_LOAD(&s_vehicle_type_config,
                                  "VehicleTypes", 4u, 0x74u);
}
extern "C" int entity_minifigure_type_table_load(void);
/* MiniFigureTypeTableLoad (FUN_0042cf50) reads the active engine config,
 * builds 0x74-byte rows, finalizes each parsed row, and stores copied names. */
/* Port model of 0x0042cf50 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x0042cf50, retires to the tail in GAME-021 part 3
int entity_minifigure_type_table_load(void)
{
    ENTITY_TYPE_CONFIG_TABLE_LOAD(&s_minifigure_type_config,
                                  "MiniFigureTypes", 1u, 0x74u);
}

extern "C" int entity_rockmonster_type_table_load(void);
/* RockMonsterTypesLoad (FUN_0042d100) uses the same 0x74-byte visual row
 * reader as the vehicle and minifigure tables (FUN_00406990). */
/* Port model of 0x0042d100 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x0042d100, retires to the tail in GAME-021 part 3
int entity_rockmonster_type_table_load(void)
{
    ENTITY_TYPE_CONFIG_TABLE_LOAD(&s_rockmonster_type_config,
                                  "RockMonsterTypes", 3u, 0x74u);
}

extern "C" int entity_building_types_load(void);
/* BuildingTypesLoad (FUN_0042d2b0) uses the same config walk with 0x14c-byte
 * rows and the building-specific `.ae` parser/finalizer. */
/* Port model of 0x0042d2b0 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x0042d2b0, retires to the tail in GAME-021 part 3
int entity_building_types_load(void)
{
    ENTITY_TYPE_CONFIG_TABLE_LOAD(&s_building_type_config,
                                  "BuildingTypes", 2u, 0x14cu);
}

namespace lego {
/* Port model of 0x0042d460 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x0042d460, retires to the tail in GAME-021 part 3
int upgrade_types_load(void* config)
{
    BlockRec* group;
    BlockRec* entry;
    uint32_t count;
    uint32_t i;
    upgrade_type_tables_release();

    /* Retain the explicit adapter for hosts that provide complete retail
     * records. The production config path below owns the table and names. */
    if (s_upgrade_type_load_hook != 0) {
        UpgradeTypeRecord loaded[UPGRADE_TYPE_CAPACITY];
        memset(loaded, 0, sizeof(loaded));
        count = s_upgrade_type_load_hook(loaded, UPGRADE_TYPE_CAPACITY);
        if (count == 0u || count > UPGRADE_TYPE_CAPACITY) return 0;
        s_upgrade_types = (UpgradeTypeRecord*)engine_heap_alloc(
            (long)(count * sizeof(UpgradeTypeRecord)));
        s_upgrade_type_names = (char**)engine_heap_alloc(
            (long)(count * sizeof(char*)));
        if (s_upgrade_types == 0 || s_upgrade_type_names == 0) {
            engine_heap_free(s_upgrade_types);
            engine_heap_free(s_upgrade_type_names);
            s_upgrade_types = 0;
            s_upgrade_type_names = 0;
            return 0;
        }
        memset(s_upgrade_type_names, 0, count * sizeof(char*));
        memcpy(s_upgrade_types, loaded, count * sizeof(UpgradeTypeRecord));
        s_upgrade_type_count = count;
        upgrade_name_table_install(s_upgrade_types,
            (const char* const*)s_upgrade_type_names, (int)count);
        return 1;
    }

    group = config != 0
        ? block_subgroup((BlockFile*)config,
              string_path_join(util_root_name((BlockFile*)config),
                               "UpgradeTypes", (const char*)NULL)) : 0;
    count = 0u;
    for (entry = group; entry != 0;
         entry = block_next_equal_key(entry)) {
        if (count == 0xffffffffu) return 0;
        ++count;
    }
    if (count == 0u) return 0;
    if (count > (uint32_t)(LONG_MAX / (long)sizeof(UpgradeTypeRecord)) ||
        count > (uint32_t)(LONG_MAX / (long)sizeof(char*))) return 0;

    s_upgrade_types = (UpgradeTypeRecord*)engine_heap_alloc(
        (long)(count * sizeof(UpgradeTypeRecord)));
    if (s_upgrade_types == 0) return 0;
    memset(s_upgrade_types, 0, count * sizeof(UpgradeTypeRecord));
    s_upgrade_type_names = (char**)engine_heap_alloc(
        (long)(count * sizeof(char*)));
    if (s_upgrade_type_names == 0) {
        engine_heap_free(s_upgrade_types);
        s_upgrade_types = 0;
        return 0;
    }
    memset(s_upgrade_type_names, 0, count * sizeof(char*));

    i = 0u;
    for (entry = group; entry != 0;
         entry = block_next_equal_key(entry), ++i) {
        const char* entry_name = block_name(entry);
        char* name_copy;
        if (entry_name == 0) goto load_failed;
        name_copy = string_duplicate(entry_name);
        if (name_copy == 0) goto load_failed;
        /* FUN_0046c030 zeroes the 0x10-byte row, writes the zero-based
         * ordinal at +0 and attempts LWO, ACT, then MESH resource creation
         * into +4. The renderer factory has no portable scene-object
         * equivalent, so preserve the row and name while leaving +4 null. */
        s_upgrade_types[i].index = i;
        s_upgrade_type_names[i] = name_copy;
        s_upgrade_type_count = i + 1u;
    }
    upgrade_name_table_install(s_upgrade_types,
        (const char* const*)s_upgrade_type_names, (int)s_upgrade_type_count);
    return 1;

load_failed:
    upgrade_type_tables_release();
    return 0;
}
} // namespace lego

extern "C" void entity_object_names_load(BlockFile*);
/* ObjectNamesLoad (FUN_0042d600) receives the parent config handle. The
 * decompile builds ObjectNames-qualified keys and queries them against that
 * root; entity_config_section_find provides the equivalent scope. */
/* Port model of 0x0042d600 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x0042d600, retires to the tail in GAME-021 part 3
void entity_object_names_load(BlockFile* config)
{
    ENTITY_OBJECT_NAMES_LOAD_BODY("ObjectNames");
}

extern "C" void entity_object_the_names_load(BlockFile*);
/* ObjectTheNamesLoad (FUN_0042da20) has the same parent-config ABI and uses
 * the separate ObjectTheNames-qualified lookup namespace. */
/* Port model of 0x0042da20 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x0042da20, retires to the tail in GAME-021 part 3
void entity_object_the_names_load(BlockFile* config)
{
    ENTITY_OBJECT_NAMES_LOAD_BODY("ObjectTheNames");
}

/* Retail-shaped accessors share the OList-fed registry and its host-pointer
 * sidecar above; do not keep a second table at DAT_005563b8. */
/* Port model of 0x0042dfc0 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x0042dfc0, retires to the tail in GAME-021 part 3
void object_pointer_registry_clear_match(int32_t pointer)
{
    for (uint32_t i = 0; i < s_object_pointer_registry_count &&
         i < OBJECT_POINTER_REGISTRY_CAPACITY; ++i) {
        if ((int32_t)s_object_pointer_registry_words[i] == pointer) {
            s_object_pointer_registry_words[i] = 0u;
            s_object_pointer_registry_entities[i] = NULL;
        }
    }
}

/* Port model of 0x0042dff0 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x0042dff0, retires to the tail in GAME-021 part 3
int object_pointer_registry_get_slot(uint32_t index, int32_t* out)
{
    if (index >= s_object_pointer_registry_count) {
        return 0; /* deviation: the corpus reads blindly past the count */
    }
    const int32_t v = (int32_t)s_object_pointer_registry_words[index];
    if (out != NULL) {
        *out = v;
    }
    return v != 0 ? 1 : 0;
}
/* Port model of 0x0042e020 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x0042e020, retires to the tail in GAME-021 part 3
int object_list_load_file(GameContext* ctx, const char* olist)
{
    if (ctx == NULL || olist == NULL || olist[0] == '\0') {
        return 0;
    }

    BlockFile* ol = block_open(olist);
    if (ol == NULL) {
        return 0;
    }

    /* OListEntityRecordParse clears DAT_005563b8 only after the resource
     * opened successfully, then appends created entities in file order. */
    object_pointer_registry_reset();
    deferred_olist_entity_level_begin(ctx);

    int placed = 0;
    int parse_ok = 1;
    size_t placed_slot_count = 0u;
    size_t placed_slot_capacity = block_record_count(ol);
    EntitySlot** placed_slots = (EntitySlot**)calloc(
        placed_slot_capacity, sizeof(EntitySlot*));
    if (placed_slot_capacity != 0u && placed_slots == NULL) {
        block_close(ol);
        return 0;
    }
    /* Root = the file's first record; objects are its children (document
     * order, like the entity list the corpus walks). */
    BlockRec* rootrec = block_record_at(ol, 0);
    const char* rname = rootrec ? block_name(rootrec) : NULL;
    if (rname == NULL) {
        free(placed_slots);
        block_close(ol);
        return 0;
    }
    for (BlockRec* rec = block_first_child(rootrec); rec != NULL;
         rec = block_next_sibling(rec)) {
        const char* name = block_name(rec);
        if (name == NULL) {
            continue;
        }
        ObjectPlacement op;
        __builtin_memset(&op, 0, sizeof(op));
        snprintf(op.name, sizeof(op.name), "%s", name);

        /* Shipped grammar (decompile-verified, FUN_0042e020): `type <name>`,
         * `xpos` / `ypos`, `heading`, optional `health` (default 100) and
         * `driving <ref>`. `Model` is the port's extension key (absent from
         * all 44 shipped lists). */
        const char* t = obj_val(ol, rname, name, "type");
        const char* x_value = obj_val(ol, rname, name, "xPos");
        const char* y_value = obj_val(ol, rname, name, "yPos");
        const char* heading_value = obj_val(ol, rname, name, "heading");
        const char* health_value = obj_val(ol, rname, name, "health");
        int type_row = 0;
        float grid_x;
        float grid_y;
        float center[2];
        int cell_x;
        int cell_y;
        float blocksize;
        if (t == NULL || x_value == NULL || y_value == NULL ||
            heading_value == NULL) {
            parse_ok = 0;
            break;
        }
        ol_strncpy(op.type_name, sizeof(op.type_name), t);
        if (!entity_type_row_for_name(op.type_name, &op.type, &type_row)) {
            parse_ok = 0;
            break;
        }
        grid_x = (float)atof(x_value) - 1.0f;
        grid_y = (float)atof(y_value) - 1.0f;
        cell_x = crt_float_to_long(grid_x);
        cell_y = crt_float_to_long(grid_y);
        if (ctx->level == NULL || ctx->level->ptr_terrain == NULL ||
            !terrain_cell_world_center_xy_transform(
                ctx->level->ptr_terrain,
                cell_x, cell_y, center)) {
            parse_ok = 0;
            break;
        }
        blocksize = terrain_blocksize_get(ctx->level->ptr_terrain);
        op.x = center[0] + (grid_x - ((float)cell_x + 0.5f)) * blocksize;
        op.y = center[1] - (grid_y - ((float)cell_y + 0.5f)) * blocksize;
        op.heading = (float)atof(heading_value) *
            0.0055555557f * 3.1415927f;
        op.z = 0.0f;
        op.rot = op.heading;
        {
            const ParsedEntityHealthValue* parsed_health =
                parse_entity_health_value(
                    health_value != NULL ? health_value : "", 0, 0, 0);
            op.health = parsed_health != NULL
                ? (float)parsed_health->value : 0.0f;
        }
        ol_strncpy(op.driving, sizeof(op.driving),
                   obj_val(ol, rname, name, "driving"));
        ol_strncpy(op.model, sizeof(op.model), obj_val(ol, rname, name, "Model"));

        if (op.type == -1) {
            /* TVCamera — FUN_0042e020's type -1 branch drives the follow
             * camera instead of creating an entity (last one wins). */
            ctx->ol_camera_set = 1;
            ctx->ol_camera_x = op.x;
            ctx->ol_camera_y = op.y;
            ctx->ol_camera_heading = op.heading;
            ++placed;
            continue;
        }
        {
            const uint8_t* voxel = level_voxel(ctx->level, cell_x, cell_y);
            uint32_t flags = 0u;
            if (voxel != NULL) memcpy(&flags, voxel + 8, 4);
            if ((flags & 8u) == 0u) {
                uint32_t world[2];
                int heading_bits, health_bits;
                memcpy(world, &op.x, 4);
                memcpy(world + 1, &op.y, 4);
                memcpy(&heading_bits, &op.heading, 4);
                memcpy(&health_bits, &op.health, 4);
                /* Resolve the configured archetype by kind/row when the
                 * reveal consumer runs; keep instance/link names owned. */
                deferred_olist_entity_queue_add(ctx, world, 0, op.type,
                    type_row, heading_bits, health_bits, op.name,
                    op.driving[0] != '\0' ? op.driving : NULL);
                continue;
            }
        }
        {
            /* The creator resolves its name argument as a configured type
             * and binds Stats before kind-four post-create callbacks. The
             * OList instance label is separate from that type identity. */
            EntitySlot* slot;
            if (op.type == 4) {
                DeferredOListEntityRecord record;
                memset(&record, 0, sizeof(record));
                record.cell_x = cell_x;
                record.cell_y = cell_y;
                record.entity_type = op.type;
                record.variant = type_row;
                record.heading = op.heading;
                record.name = op.name;
                slot = deferred_olist_live_building_create(&record,
                                                            0u, ctx);
            }
            else slot = entity_create_with_position(ctx, op.type,
                op.type_name, type_row, op.x, op.y, op.heading);
            if (slot != NULL) {
                ol_strncpy(slot->name, sizeof(slot->name), op.name);
                ol_strncpy(slot->type_name, sizeof(slot->type_name),
                           op.type_name);
                slot->etype = type_row;
                (void)entity_stats_record_bind(slot);
                (void)entity_activity_runner_type_config_bind(slot);
                (void)entity_type_config_scene_slot_nodes_load(slot);
                ol_strncpy(slot->driving, sizeof(slot->driving), op.driving);
                slot->rot = op.heading;
                slot->field_340 = op.health != 0.0f
                    ? op.health : 100.0f;
                entity_set_model(slot,
                    op.model[0] != '\0' ? op.model : NULL);
                entity_scene_slot_table_rebuild(slot, 0);
                if (op.type == 6 || op.type == 7 || op.type == 8 ||
                    op.type == 9) {
                    cmd_create_task(slot);
                }
                object_pointer_registry_append(slot);
                slot->flags_3ec |= 0x200u;
                lego::building_upgrade_state_changed_dispatch(
                    op.type, type_row, 0, 1);
                entity_queue_for_rockfall_processing_if_eligible(slot);
                if (op.type == 12) {
                    slot->flags_3e8 |= 0x10000u;
                    /* Retail also sets scene action "FlapInWind" and frame 0.
                     * The portable scene owner has no action/frame setter,
                     * and its expansion-cell table has no represented owner. */
                }
                if (slot->type == 11) {
                    /* The .ol loader's type-0xb branch installs its instance
                     * in the current level's per-cell object registry. */
                    register_object_at_grid_position(slot);
                }
                if (placed_slots != NULL &&
                    placed_slot_count < placed_slot_capacity) {
                    placed_slots[placed_slot_count++] = slot;
                }
                ++placed;
            }
        }
    }

    if (!parse_ok) {
        free(placed_slots);
        block_close(ol);
        ctx->entity_placed = placed;
        return 0;
    }

    /* The corpus resolves `driving` references after every object has been
     * created, so forward references work. It links the source entity to the
     * named object, raises +0x3e4 bit 2, dispatches the capability-derived
     * activity event, then clears route bit 0x40 on both linked entities. */
    if (placed_slots != NULL) {
        size_t source_index;
        for (source_index = 0u; source_index < placed_slot_count;
             ++source_index) {
            EntitySlot* source = placed_slots[source_index];
            size_t target_index;
            if (source == NULL || source->driving[0] == '\0') {
                continue;
            }
            for (target_index = 0u; target_index < placed_slot_count;
                 ++target_index) {
                EntitySlot* target = placed_slots[target_index];
                uint32_t events;
                uint32_t capabilities;
                if (target == NULL ||
                    strcasecmp_lite(source->driving, target->name) != 0) {
                    continue;
                }
                source->link_2fc = target;
                source->flags_3e4 |= 4u;
                capabilities = entity_capability_flags_get(target);
                if ((capabilities & 0x100u) != 0u) {
                    events = 1u;
                }
                else if ((target->flags_3e8 & 0x01000000u) == 0u) {
                    events = 4u;
                }
                else if ((target->flags_3e8 & 0x02000000u) == 0u) {
                    events = 2u;
                }
                else {
                    events = 1u;
                }
                entity_linked_activity_event_dispatch(source, events);
                entity_linked_route_flag_clear(ctx, source);
                break;
            }
        }
    }
    free(placed_slots);
    block_close(ol);
    ctx->entity_placed = placed;
    return 1;
}
/* Resolver pair (FUN_0042e8b0 shape): answers known-1 with the corpus's
 * (type, registry-row) outputs. The corpus zeroes the row slot at entry
 * (`mov ebx,[esp+0x10]` / `mov [ebx],0` at 0x42e8b8, before the name load
 * and on every path, even the failed ones); registry matches then write
 * both, the special names write the dedicated ore-pair rows (0/1) only,
 * so everything else answers a zero row (the dispatch aliases that slot
 * onto its first argument's word — a corpus quirk, pinned in the tests). */
/* Port model of 0x0042e8b0 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x0042e8b0, retires to the tail in GAME-021 part 3
extern "C" int entity_type_pair_lookup(const char* name, int* type_out,
                                     int* row_out, int* zero_out)
{
    /* The corpus's entry zeroing, blind (mov ebx,[esp+0x10] / mov [ebx],0
     * at 0x42e8b8): every in-tree caller threads a live row slot
     * (entity_type_for_name carries a sink; the wrapper's two callers pass
     * &barracks_row / &obj_y; the dispatch aliases its name slot). */
    *row_out = 0;
    if (name == NULL || name[0] == '\0') {
        return 0; /* unregistered; ObjectListLoadFile aborts on this result. */
    }
    if (strcasecmp_lite(name, "tvcamera") == 0) {
        if (type_out != NULL) {
            *type_out = -1; /* follow-camera branch in FUN_0042e020 */
        }
        /* corpus: the special-name answer also zeroes the fourth word
         * (test a3 / store) — every port caller threads NULL. */
        if (zero_out != NULL) {
            *zero_out = 0;
        }
        return 1;
    }
    int code;
    int row = 0;
    size_t i;
    code = entity_type_config_name_find(3u, name, 3, &row);
    if (code != 0) {
        if (code < 0) code = 0;
        else {
            if (type_out != NULL) *type_out = code;
            if (row_out != NULL) *row_out = row;
            return 1;
        }
    }
    code = 0;
    for (i = 0; i < 13; ++i) {
        if (strcasecmp_lite(s_special_names[i].name, name) == 0) {
            code = s_special_names[i].code;
            row = (int)i;
            break;
        }
    }
    if (code != 0) {
        if (type_out != NULL) {
            *type_out = code;
        }
        if (row_out != NULL) {
            if (code == 7) {
                *row_out = row - 2; /* Ore and ProcessedOre occupy rows 2 and 3. */
            }
        }
        return 1;
    }
    code = entity_type_config_name_find(4u, name, 1, &row);
    if (code < 0) code = 0;
    if (code == 0 && (entity_type_config_table_get(4u) == NULL ||
        entity_type_config_table_get(4u)->names == NULL))
    for (i = 0; i < 12; ++i) {
        if (strcasecmp_lite(s_vehicle_names[i].name, name) == 0) {
            code = s_vehicle_names[i].code; row = (int)i; break;
        }
    }
    if (code != 0) {
        if (type_out != NULL) {
            *type_out = code;
        }
        if (row_out != NULL) {
            *row_out = row;
        }
        return 1;
    }
    code = entity_type_config_name_find(1u, name, 2, &row);
    if (code < 0) code = 0;
    if (code == 0 && (entity_type_config_table_get(1u) == NULL ||
        entity_type_config_table_get(1u)->names == NULL))
    for (i = 0; i < 1; ++i) {
        if (strcasecmp_lite(s_minifig_names[i].name, name) == 0) {
            code = s_minifig_names[i].code; row = (int)i; break;
        }
    }
    if (code != 0) {
        if (type_out != NULL) {
            *type_out = code;
        }
        if (row_out != NULL) {
            *row_out = row;
        }
        return 1;
    }
    if (entity_type_config_table_get(3u) == NULL ||
        entity_type_config_table_get(3u)->names == NULL) for (i = 0; i < 8; ++i) {
        if (strcasecmp_lite(s_creature_names[i].name, name) == 0) {
            code = s_creature_names[i].code; row = (int)i; break;
        }
    }
    if (code != 0) {
        if (type_out != NULL) {
            *type_out = code;
        }
        if (row_out != NULL) {
            *row_out = row;
        }
        return 1;
    }
    code = entity_type_config_name_find(2u, name, 4, &row);
    if (code < 0) code = 0;
    if (code == 0 && (entity_type_config_table_get(2u) == NULL ||
        entity_type_config_table_get(2u)->names == NULL))
    for (i = 0; i < 10; ++i) {
        if (strcasecmp_lite(s_building_names[i].name, name) == 0) {
            code = s_building_names[i].code; row = (int)i; break;
        }
    }
    if (code != 0) {
        if (type_out != NULL) {
            *type_out = code;
        }
        if (row_out != NULL) {
            *row_out = row;
        }
        return 1;
    }
    return 0;
}
/* EntityPointer's types 1..4 table lookup (FUN_0042ed70). The port adds
 * bounds/null guards and groups the original special singleton types into
 * one table, so only the regular typed-array branch is represented here. */
/* Port model of 0x0042ed70 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x0042ed70, retires to the tail in GAME-021 part 3
const EntitySlot* entity_at(GameContext* ctx, int type, int index)
{
    if (ctx == NULL || type < 1 || type > 19) {
        return NULL;
    }
    int tab = entity_tab(type);
    if (index < 0 || index >= ctx->entity_count[tab]) {
        return NULL;
    }
    return &ctx->entity[tab][index];
}

/* Port model of 0x0042ef40 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x0042ef40, retires to the tail in GAME-021 part 3
int stats_level_record_count_for_stat_type(int stat_type)
{
    switch (stat_type) {
    case 1:
        return (int)s_catalog_counts[0];
    case 2:
        return (int)s_catalog_counts[1];
    case 3:
        return (int)s_catalog_counts[2];
    case 4:
        return (int)s_catalog_counts[3];
    case 5:
    case 6:
    case 8:
    case 9:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
    case 16:
    case 17:
    case 18:
    case 19:
        return 1;
    case 7:
        return 2;
    case 10:
        return (int)s_catalog_counts[4];
    default:
        return stat_type - 1;
    }
}

/* Port model of 0x0042efc0 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x0042efc0, retires to the tail in GAME-021 part 3
int movie_video_playback_loop(const MoviePump* pump)
{
    if (pump == NULL || pump->next == NULL || pump->present == NULL
        || pump->info == NULL) {
        return 0;
    }
    /* Corpus: the rect branch restores the render surface from the
     * primary once, before the pump starts (the corpus's
     * RenderSurfaceRestoreFromPrimary role); the null branch starts
     * immediately. */
    if (pump->rect != NULL && pump->restore != NULL) {
        pump->restore(pump->restore_user);
    }
    int frames = 0;
    for (;;) {
        MovieOut out;
        memset(&out, 0, sizeof(out));
        if (pump->next(&out, pump->next_user) != 0) {
            break;                    /* EOF or error ends the branch */
        }
        if (pump->present(&out, pump->info, pump->rect,
                          pump->present_user) != 0) {
            break;
        }
        ++frames;
        if (pump->quit != NULL && pump->quit(pump->quit_user) != 0) {
            break;                    /* message-pump quit between frames */
        }
    }
    return frames;
}

/* Port model of 0x0042f2e0 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x0042f2e0, retires to the tail in GAME-021 part 3
void terrain_cell_edge_flag_set_and_update(LevelState* s, int cx, int cy,
                                           int set)
{
    uint32_t index = s->word[LEVEL_OFF_WIDTH / 4u] * (uint32_t)cy +
                     (uint32_t)cx;
    uint32_t* flags = (uint32_t*)((uint8_t*)s->ptr_voxels +
                                  (size_t)index * 0x48u + 8u);
    if (set != 0)
        *flags |= 0x200u;
    else
        *flags &= 0xfffffdffu;
    cell_init_pass(s, (float)cx, (uint32_t)cy, 0);
}

/* The retail entry uses the active level and returns the classification via
 * a four-argument ABI. Keep the explicit-level table helper above for tools
 * and callers operating on an inactive level. */
/* Port model of 0x0042f350 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x0042f350, retires to the tail in GAME-021 part 3
uint8_t terrain_neighbor_coordinate_check_current_target(
    int x, int y, uint8_t* kind, int mode)
{
    int offsets[8];
    LevelState* level = g_active_level_retail;
    uint32_t width = level->word[LEVEL_OFF_WIDTH / 4u];
    uint32_t height = level->word[LEVEL_OFF_HEIGHT / 4u];
    uint8_t* voxels = (uint8_t*)level->ptr_voxels;
#if defined(_WIN32) && defined(_M_IX86)
    voxels = (uint8_t*)(uintptr_t)
        level->word[LEVEL_OFF_VOXELS / 4u];
#endif
    offsets[0] = 0;
    offsets[1] = -1;
    offsets[2] = 1;
    offsets[3] = 0;
    offsets[4] = 0;
    offsets[5] = 1;
    offsets[6] = -1;
    offsets[7] = 0;
    uint32_t mask = 0u;
    uint32_t i;
    for (i = 0u; i < 4u; ++i) {
        const int nx = x + offsets[i * 2u];
        const int ny = y + offsets[i * 2u + 1u];
        int in_bounds = 0;
        int occupied = 0;
        size_t cell_index = 0u;
        if (nx >= 0 && ny >= 0 && (uint32_t)nx < width &&
            (uint32_t)ny < height) {
            in_bounds = 1;
            cell_index = (size_t)(uint32_t)ny * width + (uint32_t)nx;
            uint32_t flags;
            __builtin_memcpy(&flags,
                voxels + cell_index * LEVEL_VOXEL_STRIDE + 8u,
                sizeof(flags));
            occupied = (flags & 0x20000000u) != 0u;
        }
        /* Retail performs this second flag probe even after the first probe
         * rejects an out-of-map neighbor. Keep the port's edge guard. */
        if (!occupied && in_bounds) {
            uint32_t flags;
            __builtin_memcpy(&flags,
                voxels + cell_index * LEVEL_VOXEL_STRIDE + 8u,
                sizeof(flags));
            if ((flags & 0x400u) != 0u) occupied = 1;
        }
        if (occupied) mask |= 1u << i;
    }

    if (mode == 0) {
        switch (mask) {
        case 0: *kind = 0; return 0x65;
        case 1: *kind = 2; return 0x65;
        case 2: *kind = 3; return 0x65;
        case 3: *kind = 3; return 99;
        case 4: *kind = 0; return 0x65;
        case 5: *kind = 0; return 0x62;
        case 6: *kind = 0; return 99;
        case 7: *kind = 0; return 100;
        case 8: *kind = 1; return 0x65;
        case 9: *kind = 2; return 99;
        case 10: *kind = 1; return 0x62;
        case 11: *kind = 3; return 100;
        case 12: *kind = 1; return 99;
        case 13: *kind = 2; return 100;
        case 14: *kind = 1; return 100;
        default: *kind = 0; return 0x60;
        }
    }
    switch (mask) {
    case 0: *kind = 0; return 0x75;
    case 1: *kind = 2; return 0x75;
    case 2: *kind = 3; return 0x75;
    case 3: *kind = 3; return 0x73;
    case 4: *kind = 0; return 0x75;
    case 5: *kind = 0; return 0x72;
    case 6: *kind = 0; return 0x73;
    case 7: *kind = 0; return 0x74;
    case 8: *kind = 1; return 0x75;
    case 9: *kind = 2; return 0x73;
    case 10: *kind = 1; return 0x72;
    case 11: *kind = 3; return 0x74;
    case 12: *kind = 1; return 0x73;
    case 13: *kind = 2; return 0x74;
    case 14: *kind = 1; return 0x74;
    default: *kind = 0; return 0x71;
    }
}
/* CellInitPass @ 0x42f6f0. The target processes one cell coordinate at a
 * time. The port applies record flags/types, task-counter transitions,
 * descriptor veto, erosion queue state, terrain dirty/material state,
 * callbacks, and diagonal updates to the installed per-level surface grid
 * and CPU terrain cache. Cell-kind records are level-owned; only their DDi
 * object factory and scene attachment remain behind renderer hooks. */
/* Port model of 0x0042f6f0 (GAME-021); the original-shape function is in lego_unit.c. */
/* Callers in the reference often push a fourth zero that FUN_0042f6f0 does
 * not inspect. Keep the worker variadic so those target-shaped call sites can
 * preserve that stack shape while older portable callers omit it. */
// PORT-ONLY: port model of 0x0042f6f0, retires to the tail in GAME-021 part 3
void cell_init_pass(LevelState* s, float cell_x, uint32_t cell_y, ...)
{
    static const int neighbour_x[4] = { 0, 1, 1, 0 };
    static const int neighbour_y[4] = { 0, 0, 1, 1 };
    int x = (int)cell_x;
    int width;
    int height;
    uint8_t* rec;
    uint32_t primary;
    uint32_t original_primary;
    uint8_t empty_neighbours[4];
    int empty_count = 0;
    uint32_t secondary;
    int solid_count = 0;
    int last_solid = 0;
    int i;
    if (s == NULL) return;
    map_cell_record_grid_activate(s);
    width = level_int(s, LEVEL_OFF_WIDTH);
    height = level_int(s, LEVEL_OFF_HEIGHT);
    if ((uint32_t)x >= (uint32_t)(width - 1) ||
        cell_y >= (uint32_t)(height - 1)) return;
    rec = voxel_at(s, x, (int)cell_y);
    if (rec == NULL) return;
    primary = *(uint32_t*)(rec + 0x08);
    original_primary = primary;
    secondary = *(uint32_t*)(rec + 0x0c);
    if ((primary & 0x20000000u) != 0u) {
        if ((secondary & 0x400u) == 0u) {
            entity_task_counter_increment_by_branch(1);
            secondary |= 0x400u;
            *(uint32_t*)(rec + 0x0c) = secondary;
        }
    }
    else if ((secondary & 0x400u) != 0u) {
        entity_task_counter_increment_by_branch(0);
        secondary &= ~0x400u;
        *(uint32_t*)(rec + 0x0c) = secondary;
    }
    for (i = 0; i < 4; ++i) {
        uint8_t* neighbour = voxel_at(s, x + neighbour_x[i],
                                      (int)cell_y + neighbour_y[i]);
        if (neighbour != NULL && neighbour[0] == 1) {
            ++solid_count;
            last_solid = i;
        }
        else {
            empty_neighbours[empty_count++] = (uint8_t)i;
        }
    }
    primary &= 0xffffdf27u;
    if (solid_count == 4) {
        primary = (primary & 0xffffdf07u) | 0x0cu;
        rec[3] = 0;
    } else if (solid_count == 1 || solid_count == 3) {
        primary = (primary & 0xffffdf07u) | 0x14u;
        if (solid_count == 1)
            rec[3] = (uint8_t)((last_solid + 2) & 3);
        switch (rec[2]) {
        case 1:
        case 7:
            rec[1] = (uint8_t)(solid_count == 1 ? 0x35 : 0x55);
            break;
        case 2:
            rec[1] = (uint8_t)(solid_count == 1 ? 0x34 : 0x54);
            break;
        case 3:
            rec[1] = (uint8_t)(solid_count == 1 ? 0x33 : 0x53);
            break;
        case 4:
            rec[1] = (uint8_t)(solid_count == 1 ? 0x32 : 0x52);
            break;
        case 5:
            rec[1] = (uint8_t)(solid_count == 1 ? 0x31 : 0x51);
            break;
        default:
            break;
        }
        if (solid_count == 1)
            primary = (primary & 0xffffdf07u) | 0x54u;
        else
            primary |= 0x80u;
    } else {
        if (solid_count == 0) {
            if ((primary & 4u) == 0u ||
                (rec[2] >= 1u && rec[2] <= 5u)) {
                rec[1] = 0x70u;
            }
            rec[3] = 0u;
        }
        else {
            static const uint8_t empty_pairs[6][2] = {
                { 0, 1 }, { 1, 2 }, { 2, 3 },
                { 0, 3 }, { 0, 2 }, { 1, 3 }
            };
            static const uint8_t partial_shapes[5] = {
                0x25, 0x24, 0x23, 0x22, 0x21
            };
            uint8_t orientation = (uint8_t)x;
            int pair;
            if (solid_count == 3) {
                /* The retail loop stores each non-solid corner into the
                 * first byte of local_38; with three solid corners that is
                 * the sole empty index, copied directly to record +3. */
                orientation = empty_neighbours[0];
            }
            else {
                for (pair = 0; pair < 6; ++pair) {
                    if ((empty_neighbours[0] == empty_pairs[pair][0] &&
                         empty_neighbours[1] == empty_pairs[pair][1]) ||
                        (empty_neighbours[1] == empty_pairs[pair][0] &&
                         empty_neighbours[0] == empty_pairs[pair][1])) {
                        orientation = (uint8_t)pair;
                        break;
                    }
                }
            }
            /* The two-solid branch preserves bit 0x20 from uVar6; the
             * one/three-solid branches use the stricter 0xffffdf07 mask. */
            primary |= 0x14u;
            if (orientation > 3u) {
                if (rec[2] >= 1 && rec[2] <= 5)
                    rec[1] = 0x77;
                primary |= 0x2000u;
                rec[3] = (uint8_t)(orientation - 4u);
                terrain_cell_orientation_set_level(s, x, (int)cell_y,
                    (int)((orientation - 4u) & 1u));
            }
            else if ((original_primary & 0x20u) != 0u) {
                if (rec[2] >= 1u && rec[2] <= 5u) {
                    rec[1] = partial_shapes[rec[2] - 1];
                }
                /* The retail switch defaults directly to the common label,
                 * which writes the orientation even for unsupported types. */
                rec[3] = orientation;
            }
            else {
                switch (rec[2]) {
                case 1:
                case 7: rec[1] = 5; break;
                case 2: rec[1] = 4; break;
                case 3: rec[1] = 3; break;
                case 4: rec[1] = 2; break;
                case 5: rec[1] = 1; break;
                case 8: rec[1] = 0x40; break;
                case 10: rec[1] = 0x20; break;
                case 11: rec[1] = 0x67; break;
                default: break;
                }
                rec[3] = orientation;
            }
        }
    }
    *(uint32_t*)(rec + 0x08) = primary;
    if (solid_count == 4 && (original_primary & 8u) == 0u) {
        int cell_xy[2];
        cell_xy[0] = x;
        cell_xy[1] = (int)cell_y;
        terrain_cell_refresh_dispatch_current(cell_xy, 0);
        terrain_neighbor_flag_refresh_xy(s, x, (int)cell_y, 1);
        if ((render_feature_flags_get() & 0x800000u) != 0u) {
            int cell_xy[2];
            cell_xy[0] = x;
            cell_xy[1] = (int)cell_y;
            cmd_produce(0x0e, NULL, rec[3], cell_xy);
        }
    }
    else if (solid_count == 1 || solid_count == 3) {
        terrain_neighbor_flag_refresh_xy(s, x, (int)cell_y, 1);
    }
    if (solid_count == 3) {
        int cell_xy[2];
        cell_xy[0] = x;
        cell_xy[1] = (int)cell_y;
        coordinate_callback_dispatch_current(cell_xy);
    }

    /* FUN_0040e1e0 / FUN_0040e350 use this level's 0x40cdc0 sidecar. The
     * eligibility and table state are native; only scene-object creation is
     * delegated to the typed renderer hook. */
    if (solid_count == 4 &&
        (*(uint32_t*)(rec + 0x08) & 0x400000u) == 0u) {
        int cell_xy[2];
        uint32_t* current_flags = (uint32_t*)(rec + 0x08);
        int descriptor_match = 0;
        void* descriptor;
        cell_xy[0] = x;
        cell_xy[1] = (int)cell_y;
        if ((*current_flags & 0x400u) == 0u &&
            (secondary & 4u) == 0u && (rec[9] & 0x80u) == 0u) {
            /* DAT_004b8a24 shares the path-record list root. Its pair keys
             * are at +0x0c/+0x10 and its next link is at +0x24. */
            descriptor = path_record_last_peek();
            while (descriptor != NULL) {
                const uint32_t* words = (const uint32_t*)descriptor;
                if (words[3] == (uint32_t)x &&
                    words[4] == (uint32_t)cell_y) {
                    descriptor_match = 1;
                    break;
                }
                descriptor = *(void**)((uint8_t*)descriptor + 0x24);
            }
            if (!descriptor_match) {
                GameContext* game = entity_pool_callback_context_get();
                *current_flags &= 0xffdfffffu;
                *current_flags &= 0xdfffffffu;
                *current_flags &= 0xbfffffffu;
                map_cell_tracking_activity_state_update(game, s);
                coordinate_pair_callback_dispatch(cell_xy);
                flic_opcode27_fallback_noop();
                coordinate_pair_event_queue(cell_xy, (void*)0x28);
                coordinate_pair_event_queue(cell_xy, (void*)0x28);
                coordinate_pair_event_queue(cell_xy, (void*)0x28);
                coordinate_pair_event_queue(cell_xy, (void*)0x28);
                *current_flags |= 3u;
            }
        }
        *current_flags |= 0x400000u;
        selection_mode7_cancel_at_point((const uint32_t*)cell_xy);
    }

    if (solid_count == 4) {
        uint32_t has_special_neighbor =
            unported_ledger_stubs::ledger_stub_0040e1e0_CellCornerOccupancyScan(
                s, x, (int)cell_y);
        terrain_cell_scene_object_update(s, (uint32_t)x, cell_y,
                                         has_special_neighbor != 0u);
    }

    /* The target selects erosion and flag-driven material variants only in
     * its four-solid-cell branch. Other neighbor counts retain the shape
     * variant selected above for the common terrain update below. */
    if (solid_count == 4) {
        primary = *(uint32_t*)(rec + 0x08);
        secondary = *(uint32_t*)(rec + 0x0c);
        if (rec[2] == 6 || (primary & 0x80000000u) != 0u) {
            int cell_xy[2];
            cell_xy[0] = x;
            cell_xy[1] = (int)cell_y;
            lava_erosion_pending_cell_add(cell_xy, 1);
            if ((secondary & 0x200u) == 0u) {
                if (rec[7] == 4u) {
                    rec[1] = 0x46u;
                    terrain_cell_dirty_queue_push_and_clear(
                        s->ptr_terrain, x, (int)cell_y, 1);
                    if ((primary & 0x4000u) == 0u) {
                        if ((int)(int16_t)*(uint16_t*)(rec + 0x20) % 3 == 0) {
                            /* CellInitPass passes these raw words directly
                             * through the emitter's int* ABI. Preserve the
                             * exact stack representation used by the target. */
                            uint32_t direction_words[3];
                            CellTransientEffectPort* effect_port;
                            void* emitter;
                            direction_words[0] = 0x3dcccccdU;
                            direction_words[1] = 0x3dcccccdU;
                            direction_words[2] = 0xbf800000U;
                            effect_port =
                                cell_transient_effect_port_ensure(s, rec);
                            emitter = NULL;
                            if (effect_port != NULL) {
                                emitter = transient_effect_emitter_create(
                                    NULL, (uint32_t)x, cell_y,
                                    (int*)direction_words,
                                    0.6f, 0.4f, 0.0f, 1.0f,
                                    0x3e99999a, 0,
                                    (uint32_t)(int)(int16_t)
                                        *(uint16_t*)(rec + 0x20), 0x26);
                                cell_transient_effect_set(effect_port,
                                                          emitter);
                            }
                            primary |= 0x4000u;
                            *(uint32_t*)(rec + 0x08) = primary;
                        }
                    }
                    else {
                        transient_effect_enable_set(
                            cell_transient_effect_get(s, rec), 0);
                    }
                }
                else {
                    terrain_cell_dirty_queue_push_and_clear(
                        s->ptr_terrain, x, (int)cell_y, 0);
                    switch (rec[7]) {
                    case 0: rec[1] = 6u; break;
                    case 1: rec[1] = 0x16u; break;
                    case 2: rec[1] = 0x26u; break;
                    case 3: rec[1] = 0x36u; break;
                    default: break;
                    }
                }
            }
            else {
                terrain_cell_dirty_queue_push_and_clear(
                    s->ptr_terrain, x, (int)cell_y, 0);
                rec[1] = 0x47u;
                if ((primary & 0x4000u) != 0u) {
                    transient_effect_enable_set(
                        cell_transient_effect_get(s, rec), 1);
                }
            }
        }
        else if (rec[2] == 9u) {
            rec[1] = 0x45u;
            terrain_cell_dirty_queue_push_and_clear(
                s->ptr_terrain, x, (int)cell_y, 1);
        }
        else if ((primary & 0x100000u) == 0u) {
            if ((primary & 0x20000000u) != 0u) {
                uint8_t kind = rec[3];
                rec[1] = (uint8_t)terrain_neighbor_coordinate_check(
                    s, x, (int)cell_y, &kind, (secondary & 0x100u) != 0u);
                rec[3] = kind;
            }
            else if ((secondary & 0x20u) != 0u) {
                rec[1] = 0x30u;
            }
            else if ((primary & 0x200000u) != 0u) {
                rec[1] = (uint8_t)(((primary & 0x40000000u) != 0u)
                                   ? 0x61u : 0u);
            }
            else {
                rec[1] = (uint8_t)(0x13u - (primary & 3u));
            }
        }
        else {
            rec[1] = (uint8_t)(((secondary & 0x100u) != 0u)
                               ? 0x66u : 0x76u);
        }
    }

    if (solid_count == 1 || solid_count == 3) {
        TerrainSurfaceGrid* surface = level_surface_grid_get(s);
        {
            float v00[3], v11[3], v10[3], v01[3];
            int have_vertices = 0;
            uint32_t height_component = 0u;
            if (surface != NULL &&
                terrain_cell_edge_vector_query(surface, (uint32_t)x,
                                               cell_y, v00) &&
                terrain_cell_edge_vector_query(surface,
                    (uint32_t)x + 1u, cell_y + 1u, v11) &&
                terrain_cell_edge_vector_query(surface,
                    (uint32_t)x + 1u, cell_y, v10) &&
                terrain_cell_edge_vector_query(surface, (uint32_t)x,
                    cell_y + 1u, v01)) {
                have_vertices = 1;
                height_component = 2u;
            }
            else {
                float vertices[4][3];
                if (scene_render_terrain_cell_vertices_get(
                        s, (uint32_t)x, cell_y, vertices)) {
                    memcpy(v00, vertices[0], sizeof(v00));
                    memcpy(v10, vertices[1], sizeof(v10));
                    memcpy(v11, vertices[2], sizeof(v11));
                    memcpy(v01, vertices[3], sizeof(v01));
                    have_vertices = 1;
                    height_component = 1u;
                }
            }
            if (have_vertices) {
                /* FUN_004507d0's third result is the raw surface height.
                 * SceneRender presents height in component one instead. */
                terrain_cell_orientation_set_level(s, x, (int)cell_y,
                    fabs((double)(v10[height_component] -
                                  v01[height_component])) <
                    fabs((double)(v00[height_component] -
                                  v11[height_component])));
            }
        }
    }

    /* CellInitPass clears the temporary color override for these cell
     * families before asking the renderer to rebuild the final surface. */
    if ((primary & 0x800004u) == 0x800004u &&
        (rec[2] == 1 || rec[2] == 7 || (primary & 8u) != 0u)) {
        primary &= ~0x800000u;
        *(uint32_t*)(rec + 0x08) = primary;
        if (s->ptr_terrain != NULL)
            terrain_cell_surface_type_reset(s->ptr_terrain, x, (int)cell_y);
    }

    /* CellInitPass selects the ten-slot transition path when render flag
     * 0x8000 is enabled; both paths finish through TerrainCellMaterialSet
     * (FUN_0044f230), which updates the cell material and corner rotation. */
    if (s->ptr_terrain != NULL) {
        TerrainSurfaceGrid* surface = level_surface_grid_get(s);
        TerrainState* terrain = (TerrainState*)s->ptr_terrain;
        if ((render_feature_flags_get() & 0x8000u) != 0u) {
            terrain_cell_material_transition_begin(
                surface, (uint32_t)x, cell_y, (int)rec[1], (int)rec[3]);
        }
        else if (surface != NULL) {
            terrain_cell_material_set(surface, (uint32_t)x, cell_y,
                                      rec[1], rec[3]);
        }
        /* TerrainCreate's per-cell CPU record mirrors the renderer's +0x18
         * material byte even when no native surface owner is installed. */
        if (terrain->blocks != NULL && x >= 0 && cell_y <
            (uint32_t)terrain->height) {
            const size_t index = ((size_t)cell_y * (size_t)terrain->width +
                                  (size_t)x) * TERRAIN_BLOCK_STRIDE;
            terrain->blocks[index + 0x18u] = rec[1];
        }
    }

    /* Target invalidates the 4x4 terrain patch around the updated cell. */
    {
        int dirty_x, dirty_y;
        for (dirty_y = (int)cell_y - 1; dirty_y <= (int)cell_y + 2;
             ++dirty_y) {
            for (dirty_x = x - 1; dirty_x <= x + 2; ++dirty_x) {
                terrain_cell_changed_flag_set(s->ptr_terrain, dirty_x,
                                              dirty_y);
            }
        }
    }

    /* FUN_0042f6f0 ends with this gate after the material setter/transition:
     * LevelState +0x2c is the loaded `useroof` field. The progress routine
     * applies the level's dig-depth/roof-height displacement to the live
     * cell vertices, so keep it conditional on that retail field. */
    if (level_int(s, LEVEL_OFF_USEROOF) != 0) {
        terrain_cell_mutation_progress(s, (uint32_t)x, cell_y);
    }
}

/* MapCellTrackingFlagsClearAndRefresh (FUN_004302b0) reads the active level
 * global and receives only the coordinate pair in the retail ABI. */
/* Port model of 0x004302b0 (GAME-021) for the targets other than MSVC x86; on MSVC
 * x86 the shared original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x004302b0 off MSVC x86, retires to the tail in GAME-021 part 3
#if !defined(_MSC_VER) || !defined(_M_IX86)
void map_cell_tracking_flags_clear_and_refresh_current(
    const int cell_xy[2])
{
    LevelState* level = g_active_level_retail;
    const int width = (int)level->word[LEVEL_OFF_WIDTH / 4];
#if defined(_WIN32) && defined(_M_IX86)
    uint8_t* voxels = (uint8_t*)(uintptr_t)
        level->word[LEVEL_OFF_VOXELS / 4];
#else
    uint8_t* voxels = (uint8_t*)level->ptr_voxels;
#endif
    int cell_index = width * cell_xy[1] + cell_xy[0];
    uint8_t* record = voxels + cell_index * 0x48;
    volatile uint32_t* flags = (volatile uint32_t*)(record + 8);
    uint32_t value = *flags;
    value &= 0xdfffffffu;
    *flags = value;

    cell_index = width * cell_xy[1] + cell_xy[0];
    record = voxels + cell_index * 0x48;
    flags = (volatile uint32_t*)(record + 8);
    value = *flags;
    value &= 0xbfffffffu;
    *flags = value;

    entity_activity_scene_state_update_retail();
    coordinate_pair_callback_dispatch(cell_xy);
    ((void (*)(const int*))flic_opcode27_fallback_noop)(cell_xy);
}
#endif
/* TerrainCellWorkFlagClear @ 0x00430320 takes only the coordinate pair and
 * event selector; DAT_005560d4 supplies its level. */
/* Port model of 0x00430320 (GAME-021) for the targets other than MSVC x86; on MSVC
 * x86 the shared original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x00430320 off MSVC x86, retires to the tail in GAME-021 part 3
#if !defined(_MSC_VER) || !defined(_M_IX86)
void terrain_cell_work_flag_clear_current(const int coord[2],
                                          uint32_t selector)
{
    GameContext* game = entity_pool_callback_context_get();
    LevelState* s;
    uint8_t* rec;
    uint32_t* flags;
    if (coord == NULL) return;
    /* Retail reads DAT_005560d4 directly. The active map-cell registry is the
     * portable owner when entity-pool context is not installed. */
    s = game != NULL ? game->level : NULL;
    if (s == NULL) s = level_current_owner_get();
    if (s == NULL) return;
    /* The retail entry has no coordinate bounds gate. */
    rec = cell_record_unchecked(s, coord[0], coord[1]);
    if (rec == NULL) return;
    flags = (uint32_t*)(rec + 0x08);
    if ((*flags & 0x400u) != 0u || (rec[0x0c] & 4u) != 0u ||
        (*flags & 0x8000u) != 0u) return;
    if (terrain_cell_predicate_check_current(
            (const uint32_t*)coord) != 0) return;
    *flags &= 0xffdfffffu;
    *flags &= 0xdfffffffu;
    *flags &= 0xbfffffffu;
    map_cell_tracking_activity_state_update(game, s);
    coordinate_pair_callback_dispatch(coord);
    flic_opcode27_fallback_noop();
    coordinate_pair_event_queue(coord, (void*)(uintptr_t)selector);
    coordinate_pair_event_queue(coord, (void*)(uintptr_t)selector);
    coordinate_pair_event_queue(coord, (void*)(uintptr_t)selector);
    coordinate_pair_event_queue(coord, (void*)(uintptr_t)selector);
    *flags |= 3u;
}
#endif

/* TerrainNeighborFlagSet (FUN_00430470): for an interior, flagged cell,
 * require a staged spatial record at that coordinate, move the primary flag
 * from bit 3 to bit 4, run CellInitPass, and clear its staged render slot. */
/* Port model of 0x00430470 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x00430470, retires to the tail in GAME-021 part 3
void terrain_neighbor_flag_set(LevelState* s, int manager,
                               uint32_t x, uint32_t y)
{
#if defined(_WIN32) && defined(_M_IX86)
    uint32_t width;
    uint32_t height;
    uint32_t index;
    uint8_t* voxels;
    uint8_t* rec;
    (void)manager;
    if (x != 0u && y != 0u) {
        width = s->word[LEVEL_OFF_WIDTH / 4];
        if (x < width - 2u) {
            height = s->word[LEVEL_OFF_HEIGHT / 4];
            if (y < height - 2u) {
                index = y * width + x;
                voxels = (uint8_t*)(uintptr_t)
                    s->word[LEVEL_OFF_VOXELS / 4];
                rec = voxels + index * 0x48u;
                if ((rec[8] & 8u) != 0u &&
                    terrain_cell_quad_run_slot_state_get(
                        s, (int)x, (int)y, NULL, NULL) != 0) {
                    *(uint32_t*)(rec + 8) =
                        *(uint32_t*)(rec + 8) & 0xfffffff7u;
                    *(uint32_t*)(rec + 8) =
                        *(uint32_t*)(rec + 8) | 0x10u;
                    cell_init_pass(s, (float)x, y, 0);
                    terrain_cell_surface_quad_reset_current(
                        (int)x, (int)y);
                }
            }
        }
    }
#else
    uint8_t* rec;
    uint32_t* flags;
    (void)manager;
    if (s == NULL || x == 0u || y == 0u || level_width(s) < 2 ||
        level_height(s) < 2 || x >= (uint32_t)level_width(s) - 2u ||
        y >= (uint32_t)level_height(s) - 2u) {
        return;
    }
    rec = level_voxel(s, (int)x, (int)y);
    if (rec == NULL) {
        return;
    }
    flags = (uint32_t*)(rec + 0x08);
    /* Retail checks for a staged cell quad at this coordinate through
     * WorldSpatialRecordFindAtGridCoordinate (0x46e8a0). The LevelState
     * sidecar is that same per-level table; the old hook-only gate prevented
     * the production table from participating when no renderer adapter was
     * installed. */
    if ((*flags & 8u) == 0u ||
        !terrain_cell_quad_run_slot_state_get(s, (int)x, (int)y,
                                               NULL, NULL)) {
        return;
    }
    *flags &= ~8u;
    *flags |= 0x10u;
    /* The retail path runs CellInitPass here, then clears the staged run's
     * selected overlay slot. A dirty-bit mark alone misses its terrain and
     * material-state updates. */
    cell_init_pass(s, (float)x, y);
    terrain_cell_surface_quad_reset_level(s, (int)x, (int)y);
    if (s_terrain_neighbor_render_hook != NULL) {
        s_terrain_neighbor_render_hook(x, y,
                                       s_terrain_neighbor_hook_context);
    }
#endif
}
/* TerrainCellMutation's mode-1 path is used by the tutorial ground visitor as
 * well as the predug-cell sweep. CPU-visible state from both modes is owned
 * here; the indexed DDi height-overlay slot remains a renderer operation. */
/* Port model of 0x00430530 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x00430530, retires to the tail in GAME-021 part 3
int terrain_cell_mutate(LevelState* level, uint32_t x, uint32_t y, int mode)
{
    static const int corner_offset[4][2] = {
        { 0, 0 }, { 1, 0 }, { 1, 1 }, { 0, 1 }
    };
    static const int neighbor_offset[8][2] = {
        { -1, -1 }, { 0, -1 }, { 1, -1 }, { -1, 0 },
        { 1, 0 }, { -1, 1 }, { 0, 1 }, { 1, 1 }
    };
    uint8_t* target;
    uint32_t* target_flags;
    int cell_xy[2];
    int width, height;
    int index;
    if (level == NULL || x > 0x7fffffffu || y > 0x7fffffffu) return 0;
    width = level_width(level);
    height = level_height(level);
    if (width < 3 || height < 3) return 0;
    if (x == 0u || y == 0u || x >= (uint32_t)(width - 2) ||
        y >= (uint32_t)(height - 2)) return 0;
    target = level_voxel(level, (int)x, (int)y);
    if (target == NULL) return 0;
    target_flags = (uint32_t*)(target + 0x08);
    if ((*target_flags & 8u) != 0u) return 0;
    cell_xy[0] = (int)x;
    cell_xy[1] = (int)y;

    if (target[2] == 8u || target[2] == 10u) {
        uint16_t* steps = (uint16_t*)(target + 0x22);
        memset(target + 0x14, 0, 4u);
        for (index = 0; index < 4; ++index) {
            const int nx = (int)x + corner_offset[index][0];
            const int ny = (int)y + corner_offset[index][1];
            uint8_t* neighbor = level_voxel(level, nx, ny);
            if (neighbor == NULL) continue;
            if (neighbor[0] == 0u) {
                neighbor[0] = 5u;
                message_dispatch(0x25, 0u, 0u, cell_xy);
                if (target[2] == 8u)
                    message_dispatch(1, 0u, 0u, cell_xy);
                else
                    message_dispatch(0x24, 0u, 0u, cell_xy);
            }
            if (neighbor[0] != 1u) {
                --neighbor[0];
                terrain_level_vertex_offset(level, nx, ny,
                    level_float(level, LEVEL_OFF_DIGDEPTH) * 0.25f);
            }
        }
        terrain_neighbor_refresh(level, cell_xy);
        ++*steps;
        if (*steps != 4u) {
            terrain_cell_mutation_progress(level, x, y);
            for (index = 0; index < 8; ++index) {
                terrain_cell_mutation_progress(
                    level,
                    (uint32_t)((int)x + neighbor_offset[index][0]),
                    (uint32_t)((int)y + neighbor_offset[index][1]));
            }
            return 0;
        }
        target[2] = 5u;
        for (index = 0; index < 4; ++index) {
            uint8_t* neighbor = level_voxel(
                level, (int)x + corner_offset[index][0],
                (int)y + corner_offset[index][1]);
            if (neighbor != NULL) neighbor[0] = 1u;
        }
        return terrain_cell_mutate(level, x, y, mode);
    }

    for (index = 0; index < 4; ++index) {
        const int vx = (int)x + corner_offset[index][0];
        const int vy = (int)y + corner_offset[index][1];
        uint8_t* corner_cell = level_voxel(level, vx, vy);
        if (corner_cell != NULL && corner_cell[0] == 0u) {
            terrain_level_vertex_offset(level, vx, vy,
                level_float(level, LEVEL_OFF_DIGDEPTH));
            corner_cell[0] = 1u;
        }
    }
    for (index = 0; index < 8; ++index) {
        uint8_t* neighbor = level_voxel(
            level, (int)x + neighbor_offset[index][0],
            (int)y + neighbor_offset[index][1]);
        if (neighbor != NULL) {
            uint32_t* flags = (uint32_t*)(neighbor + 0x08);
            if ((*flags & 8u) == 0u) {
                *flags |= 0x10u;
                *flags &= ~0x40000u;
            }
        }
    }
    *target_flags |= 8u;
    terrain_neighbor_flag_refresh(level, cell_xy, 1);
    *target_flags &= ~0x20u;
    *target_flags |= 0x400000u;
    if (((*target_flags & 0x4000000u) == 0u ||
         (*target_flags & 0x20000u) != 0u) &&
        (*target_flags & 0x800000u) != 0u)
        global_float_at_00554adc_add_one();
    if (mode == 0) {
        int scan_y;
        for (scan_y = (int)y - 1; scan_y <= (int)y + 1; ++scan_y) {
            int scan_x;
            for (scan_x = (int)x - 1; scan_x <= (int)x + 1; ++scan_x) {
                uint8_t* scan_cell = level_voxel(level, scan_x, scan_y);
                if (scan_cell != NULL && (scan_cell[0x0a] & 2u) != 0u) {
                    GameContext* game =
                        entity_pool_callback_context_get();
                    if (game != NULL && game->level == level)
                        terrain_neighbor_work_flag_scan_current(
                            (unsigned int)scan_x, (unsigned int)scan_y);
                    else
                        terrain_neighbor_work_flag_scan(level,
                            (unsigned int)scan_x, (unsigned int)scan_y);
                    if (system_state_flags_get() == 0u) {
                        message_dispatch(9, 0u, 0u, cell_xy);
                        advisor_text_animation_update(0x12, 1, 0);
                        global_float_at_00554efc_add_one();
                    }
                }
            }
        }
        *target_flags |= 0x800u;
        cmd_produce(0x0e, NULL, target[3], cell_xy);
        terrain_cell_surface_quad_height_fill_current(level, (int)x, (int)y);
        /* Mirror the live cell heights into SceneRender's row-major CPU
         * record. The separate staged DDi object/edge-slot mutation still has
         * no native scene-resource owner. */
        if (s_terrain_cell_overlay_height_fill != NULL) {
            s_terrain_cell_overlay_height_fill(
                s_terrain_cell_overlay_height_fill_context, level, x, y);
        }
        if (((*target_flags & 0x400u) == 0u) &&
            ((target[0x0c] & 4u) == 0u) &&
            ((*target_flags & 0x8000u) == 0u)) {
            terrain_cell_work_flag_clear(level, cell_xy, 0x28u);
            selection_mode7_cancel_at_point((const uint32_t*)cell_xy);
        }
    }
    else {
        *target_flags |= 0x200000u;
    }
    /* Retail's TerrainCellRefreshDispatch always walks the live command pool
     * before resetting the terrain surface type. Keep that state update even
     * when this explicit LevelState is active without a GameContext; the
     * callback walk itself is owned by CmdRec and does not require one. */
    terrain_cell_refresh_dispatch_level(level, cell_xy, 0);
    for (index = -1; index <= 1; ++index) {
        int dx;
        for (dx = -1; dx <= 1; ++dx)
            cell_init_pass(level, (float)((int)x + dx),
                           (uint32_t)((int)y + index));
    }
    *target_flags &= ~0x800000u;
    for (index = -1; index <= 2; ++index) {
        int dx;
        for (dx = -1; dx <= 2; ++dx)
            terrain_cell_changed_flag_set(level->ptr_terrain,
                                           (int)x + dx, (int)y + index);
    }
    if (mode == 0) {
        GameContext* game = entity_pool_callback_context_get();
        if (game != NULL && (game->cheat & 0x80000u) != 0u) {
            const int16_t seed = *(const int16_t*)(target + 0x20);
            const unsigned int magnitude = seed < 0
                ? (unsigned int)(-(int)seed) : (unsigned int)seed;
            if ((magnitude & 3u) == 0u) {
                int variation = (int)seed % 0x549;
                unported_ledger_stubs::
                    ledger_stub_0044b140_SmallSpiderCellSwarmSpawn(
                    (int)x, (int)y, (uint32_t)variation);
            }
        }
    }
    {
        GameContext* game = entity_pool_callback_context_get();
        if (game != NULL && game->level == level)
            deferred_olist_entity_create_and_link(cell_xy);
    }
    {
        uint32_t* secondary = (uint32_t*)(target + 0x0c);
        if ((*secondary & 0x200u) != 0u) {
            *secondary &= ~0x200u;
            *secondary |= 0x20u;
            predug_coordinate_pool_append(cell_xy);
            selection_mode7_cancel_at_point((const uint32_t*)cell_xy);
            cell_init_pass(level, (float)x, y);
        }
    }
    if (mode == 0) {
        message_dispatch(0x1b, 0u, 0u, cell_xy);
        return 1;
    }
    return 1;
}

/* TerrainNeighborRefresh (FUN_00430df0): inspect the first eligible
 * bit-3 neighbor in north/east/south/west order. Current state 8 requests
 * the build action; state 10 requests the clear action. */
/* Port model of 0x00430df0 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x00430df0, retires to the tail in GAME-021 part 3
void terrain_neighbor_refresh(LevelState* s, const int cell_xy[2])
{
    static const int offsets[4][2] = {
        {0, -1}, {1, 0}, {0, 1}, {-1, 0}
    };
    uint8_t* current;
    int x, y, i;
    int neighbor[2];
    uint8_t* found = NULL;
    if (s == NULL || cell_xy == NULL) {
        return;
    }
    x = cell_xy[0];
    y = cell_xy[1];
    for (i = 0; i < 4; ++i) {
        neighbor[0] = x + offsets[i][0];
        neighbor[1] = y + offsets[i][1];
        found = level_voxel(s, neighbor[0], neighbor[1]);
        if (found != NULL &&
            ((*(uint32_t*)(found + 0x08) & 8u) != 0u) &&
            found[2] != 9 && found[2] != 6) {
            break;
        }
        found = NULL;
    }
    if (found == NULL) {
        return;
    }
    current = level_voxel(s, x, y);
    if (current == NULL) {
        return;
    }
    if (current[2] == 8) {
        if (s_terrain_neighbor_build_action != NULL) {
            s_terrain_neighbor_build_action(
                neighbor, s_terrain_neighbor_action_context);
        } else {
            /* Retail FUN_00430df0 uses the alternate kind-7 placement setup
             * for a type-8 target: FUN_004394b0(cell, 0, 0, 1). */
            (void)entity_build_placement_command_setup(
                neighbor, 0, NULL, 1, 1);
        }
    } else if (current[2] == 10) {
        if (s_terrain_neighbor_clear_action != NULL) {
            s_terrain_neighbor_clear_action(
                neighbor, s_terrain_neighbor_action_context);
        } else {
            /* Type 10 uses FUN_00439370(cell, 0, 0, 1), the kind-6 form. */
            (void)entity_build_placement_command_setup(
                neighbor, 0, NULL, 1, 0);
        }
    }
}
/* 0x430ee0: initialize one actual level cell, including terrain refresh. */
/* Port model of 0x00430ee0 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x00430ee0, retires to the tail in GAME-021 part 3
int terrain_cell_initialize(LevelState* level, uint32_t x, uint32_t y)
{
    static const int cardinal[4][2] = {{0,-1},{1,0},{0,1},{-1,0}};
    uint8_t* cell;
    int ix, iy, i, count, last;
    int xy[2];
    int width, height;
    if (level == NULL) return 0;
    width = level_width(level); height = level_height(level);
    if (width < 3 || height < 3 || x == 0u || y == 0u ||
        x >= (uint32_t)(width - 2) || y >= (uint32_t)(height - 2))
        return 0;
    cell = level_voxel(level, (int)x, (int)y);
    if (cell == NULL) return 0;
    if (cell[0] != 0u) return 1;
    terrain_level_vertex_offset(level, (int)x, (int)y,
                                level_float(level, LEVEL_OFF_DIGDEPTH));
    cell[0] = 1u;
    for (iy = (int)y - 1; iy <= (int)y; ++iy) {
        for (ix = (int)x - 1; ix <= (int)x; ++ix) {
            uint8_t* neighbor = level_voxel(level, ix, iy);
            uint32_t flags;
            __builtin_memcpy(&flags, neighbor + 8, sizeof(flags));
            flags |= 0x40000u;
            __builtin_memcpy(neighbor + 8, &flags, sizeof(flags));
        }
    }
    for (iy = (int)y - 1; iy <= (int)y; ++iy)
        for (ix = (int)x - 1; ix <= (int)x; ++ix)
            cell_init_pass(level, (float)ix, (uint32_t)iy);
    for (iy = (int)y - 1; iy <= (int)y + 1; ++iy)
        for (ix = (int)x - 1; ix <= (int)x + 1; ++ix)
            terrain_cell_changed_flag_set(level->ptr_terrain, ix, iy);
    count = 0; last = 0;
    for (i = 0; i < 4; ++i) {
        const uint8_t* neighbor = level_voxel(level,
            (int)x + cardinal[i][0], (int)y + cardinal[i][1]);
        if (neighbor[0] == 1u) { ++count; last = i; }
    }
    xy[0] = (int)x; xy[1] = (int)y;
    if (count == 1)
        cmd_produce(0x0e, NULL, ((last - 2) & 3) | 0x10000, xy);
    terrain_cell_refresh_dispatch_level(level, xy, 1);
    return 1;
}

/* Port model of 0x004310f0 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x004310f0, retires to the tail in GAME-021 part 3
void terrain_cell_flag20_clear_and_refresh(LevelState* s, int cx, int cy)
{
    terrain_neighbor_flag_refresh_xy(s, cx, cy, 0);
    uint8_t* rec = level_voxel(s, cx, cy);
    if (rec != NULL) {
        uint32_t* word = (uint32_t*)(rec + 0x08);
        *word &= 0xffffffdfu;
    }
    /* FUN_004310f0 finishes with CellInitPass(level, x, y, 0). The level
     * module owns that transition, so a dirty-bit mark alone would omit the
     * cell flags, task-counter transitions, and material/variant updates. */
    cell_init_pass(s, (float)cx, (uint32_t)cy);
}

/* Port model of 0x00431140 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x00431140, retires to the tail in GAME-021 part 3
void terrain_cell_queued_work_transition_current(int x, int y)
{
    LevelState* lvl = level_current_owner_get();
    uint8_t* rec;
    uint32_t* word;
    int pair[2];
    uint32_t flags;
    if (lvl == NULL) return;
    rec = cell_record_unchecked(lvl, x, y);
    if (rec == NULL) return;
    word = (uint32_t*)(rec + 0x08);
    flags = *word;
    if ((flags & 0x10u) == 0u || (flags & 0xe0u) != 0u) return;
    *word = flags | 0x20u;
    cell_init_pass(lvl, (float)x, (uint32_t)y);
    /* FUN_004311d0(lvl, pair, 0) has no state effect on this branch. */
    pair[0] = x;
    pair[1] = y;
    coordinate_callback_dispatch_current(pair);
    message_dispatch(0x1c, 0, 0, pair);
}

/* Port model of 0x004313b0 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x004313b0, retires to the tail in GAME-021 part 3
void simtick_minion_states(EngineContext* c, float dt, float scale)
{
    (void)dt;
    EngineState* st = c != NULL ? c->state_block : NULL;
    uintptr_t* node = (uintptr_t*)(st != NULL && st->minion_list != NULL
                                       ? st->minion_list : s_minion_list);
    s_minion_actions = s_minion_fails = 0;
    while (node != NULL) {
        uintptr_t* next = (uintptr_t*)node[5];
        float dist = 0.0f;
        if (s_minion_dist != NULL) {
            dist = (float)s_minion_dist((int)(uint32_t)node[0], scale);
        }
        if (node[6] == 0) {
            uint8_t* b = (uint8_t*)&node[3];
            if (((*b & 1u) == 0) && (dist != 0.0f)) {
                *b = (uint8_t)(*b | 1u);
                if (s_minion_act != NULL) {
                    s_minion_act((int)(uint32_t)node[0]);
                }
                ++s_minion_actions;
            } else if ((*b & 2u) == 0) {
                if (((*b & 4u) != 0) && (dist != 0.0f)) {
                    *b = (uint8_t)(*b | 2u);
                    ++s_minion_actions;
                }
            } else if (s_minion_fail != NULL) {
                s_minion_fail(node);
                ++s_minion_fails;
            }
        } else if (s_minion_fail != NULL) {
            s_minion_fail(node);
            ++s_minion_fails;
        }
        node = next;
    }
}

/* Port model of 0x00431450 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x00431450, retires to the tail in GAME-021 part 3
void terrain_neighbor_flag_refresh(LevelState* s, const int cell_xy[2],
                                   int mode)
{
    TerrainActivityBuildRecordModel* build;
    int cx;
    int cy;
    cx = cell_xy[0];
    cy = cell_xy[1];
    if (level_voxel(s, cx, cy) == NULL) {
        return; /* bounds deviation (as across this module) */
    }
    build = (TerrainActivityBuildRecordModel*)
        terrain_activity_build_record_get(s, cx, cy);
    if (build == NULL) {
        return;
    }
    if (mode == 0 && build->enabled == 0) {
        /* SceneActionNameSet("Activity_Destroy") and SceneFrameSet(0)
         * require the scene-object handle owned by the corpus record.
         * The sidecar retains the decompiled record byte at +0x0c, but the
         * portable level has no scene object to receive those calls. */
        build->scene_flags_0c |= 4u;
        return;
    }
    build->scene_flags_0c |= 2u;
}
/* Port model of 0x00431560 (GAME-021) for the targets other than MSVC x86; on MSVC
 * x86 the shared original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x00431560 off MSVC x86, retires to the tail in GAME-021 part 3
#if !defined(_MSC_VER) || !defined(_M_IX86)
LevelState* level_current_owner_get(void)
{
    GameContext* game = entity_pool_callback_context_get();
    LevelState* level = game != NULL ? game->level : NULL;
    if (level == NULL)
        level = (LevelState*)map_cell_record_grid_level_get();
    return level;
}
#endif

/* Port model of 0x00431570 (GAME-021) for the targets other than MSVC x86; on MSVC
 * x86 the shared original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x00431570 off MSVC x86, retires to the tail in GAME-021 part 3
#if !defined(_MSC_VER) || !defined(_M_IX86)
void* level_current_terrain_get(int cell_x, int cell_y)
{
    LevelState* level = level_current_owner_get();
    (void)cell_x;
    (void)cell_y;
    return level != NULL ? level->ptr_terrain : NULL;
}
#endif

/* Retail ABI is two unsigned coordinates. The level is the live global, not
 * an explicit first parameter; keep the three-argument form above as the
 * portable adapter for isolated level callers. */
/* Port model of 0x00431580 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x00431580, retires to the tail in GAME-021 part 3
void terrain_neighbor_work_flag_scan_current(unsigned int seed_x,
                                             unsigned int seed_y)
{
    GameContext* game = entity_pool_callback_context_get();
    LevelState* s;
    int w;
    int h;
    int span;
    float* buf;
    float* page[2];
    unsigned int count[2];
    int cur;
    s = game != NULL ? game->level : NULL;
    if (s == NULL) s = (LevelState*)map_cell_record_grid_level_get();
    if (s == NULL) return;
    w = level_width(s);
    h = level_height(s);
    span = w + h;
    if (span <= 0) return;
    buf = (float*)engine_heap_alloc((long)span * 0x20);
    if (buf == NULL) return;
    page[0] = buf;
    page[1] = buf + span * 4;
    count[0] = 1u;
    count[1] = 0u;
    page[0][0] = (float)seed_x;
    page[0][1] = (float)seed_y;
    cur = 0;
    do {
        if (count[cur] != 0u) {
            const float* pf = page[cur];
            unsigned int idx;
            for (idx = 0; idx < count[cur]; ++idx, pf += 2) {
                const int col0 = level_cell_of(pf[0]);
                const int row0 = level_cell_of(pf[1]);
                int row;
                for (row = row0 - 1; row <= row0 + 1; ++row) {
                    int col;
                    for (col = col0 - 1; col <= col0 + 1; ++col) {
                        uint8_t* rec = level_voxel(s, col, row);
                        uint32_t word = 0;
                        if (rec != NULL)
                            __builtin_memcpy(&word, rec + 0x08, sizeof(word));
                        if ((word & 0x20000u) != 0u) {
                            terrain_cell_mutate(s, (uint32_t)col,
                                                (uint32_t)row, 1);
                            /* Preserve flags published by mutation,
                             * including a deferred building footprint. */
                            rec = level_voxel(s, col, row);
                            if (rec != NULL) {
                                __builtin_memcpy(&word, rec + 0x08,
                                                 sizeof(word));
                                word &= 0xfffdffffu;
                                __builtin_memcpy(rec + 0x08, &word,
                                                 sizeof(word));
                            }
                            if (col != col0 || row != row0) {
                                const unsigned int n = count[1 - cur];
                                float* dst = page[1 - cur];
                                dst[2 * n] = (float)col;
                                dst[2 * n + 1] = (float)row;
                                count[1 - cur] = n + 1u;
                            }
                        }
                    }
                }
            }
        }
        cur = (cur == 0) ? 1 : 0;
        count[1 - cur] = 0u;
    } while (count[cur] != 0u);
    engine_heap_free(buf);
}
/* TerrainStampImpactHandler (FUN_00431780) has four arguments and reads the
 * current level plus renderer-owned slot tables from globals. This target
 * entry uses the live LevelState, terrain, sound, and scene-slot owners that
 * the portable runtime exposes. */
/* Port model of 0x00431780 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x00431780, retires to the tail in GAME-021 part 3
int terrain_stamp_impact_handle_current(uint32_t x, uint32_t y,
                                        int direction, int mode)
{
    static const int neighbor_x[4] = { 1, 0, -1, 0 };
    static const int neighbor_y[4] = { 0, 1, 0, -1 };
    LevelState* level = level_current_owner_get();
    TerrainSurfaceGrid* surface;
    float position_xy[2];
    float position[3];
    uint32_t neighbors = 0u;
    uint32_t flags;
    uint8_t* cell;
    int i;
    if (direction < 0 || direction >= 4) return 0;
    if (level == NULL || level->ptr_terrain == NULL ||
        x >= (uint32_t)level_int(level, LEVEL_OFF_WIDTH) ||
        y >= (uint32_t)level_int(level, LEVEL_OFF_HEIGHT)) return 0;
    surface = level_surface_grid_get(level);
    if (mode == 0) {
        if (!terrain_cell_world_center_xy_get(level->ptr_terrain,
                (int)x, (int)y, position_xy)) return 0;
    }
    else {
        float edge[3];
        if (surface == NULL ||
            !terrain_cell_edge_vector_query(surface, x, y, edge)) return 0;
        position_xy[0] = edge[0];
        position_xy[1] = edge[2];
    }
    for (i = 0; i < 4; ++i) {
        if ((cell_flag_word_unchecked(level,
                (int)x + neighbor_x[i], (int)y + neighbor_y[i]) & 8u)
            != 0u) ++neighbors;
    }
    position[0] = position_xy[0];
    position[1] = position_xy[1];
    position[2] = terrain_surface_height_interpolate(
        level->ptr_terrain, position_xy[0], position_xy[1]);
    sound_positional_sound_submit(0, 4, 0, 0, position);
    {
        const float effect_height = terrain_surface_height_interpolate(
            level->ptr_terrain, position_xy[0], position_xy[1]);
        /* The decompile's FUN_0040c230 call omits its last two pose slots;
         * the adapter requires them, so retain the neutral pose. */
        if (!scene_slot_pose_scale_register(
                neighbors > 1u, x, y, position_xy[0], position_xy[1],
                effect_height, 0.0f, 0.0f)) return 0;
    }
    flags = cell_flag_word_unchecked(level, (int)x, (int)y);
    flags |= 0x800u;
    cell = cell_record_unchecked(level, (int)x, (int)y);
    if (cell != NULL) __builtin_memcpy(cell + 0x08, &flags, sizeof(flags));
    return 1;
}
/* TerrainCellMaterialGet (FUN_004319b0) reads byte +2 from the active
 * level's 0x48-byte row-major cell record. Like retail it has no coordinate
 * or active-level validation; callers must have an active level and in-range
 * cell coordinates. */
/* Port model of 0x004319b0 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x004319b0, retires to the tail in GAME-021 part 3
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) uint8_t terrain_cell_material_get_current(int x, int y)
{
    __asm {
        mov ecx, dword ptr [g_active_level_retail]
        mov edx, dword ptr [esp + 4]
        mov eax, dword ptr [ecx + 14h]
        mov ecx, dword ptr [ecx + 0b8h]
        imul eax, dword ptr [esp + 8]
        add eax, edx
        xor edx, edx
        lea eax, dword ptr [eax + eax*8]
        mov dl, byte ptr [ecx + eax*8 + 2]
        mov eax, edx
        ret
    }
}
#else
uint8_t terrain_cell_material_get_current(int x, int y)
{
    LevelState* level = g_active_level_retail;
    const uint32_t width = level->word[LEVEL_OFF_WIDTH / 4u];
    const uint8_t* voxels = (const uint8_t*)level->ptr_voxels;
    const uint32_t index = (uint32_t)y * width + (uint32_t)x;
    const uint8_t* record = voxels + index * LEVEL_VOXEL_STRIDE;
    return record[2];
}
#endif

/* MapFileCellGet (FUN_004319e0) and SelectedEntitiesAnyPredicateMatch
 * (FUN_00431a30) are defined in GAME-021's lego_unit.c (shared). */

/* Port model of 0x00431ab0 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x00431ab0, retires to the tail in GAME-021 part 3
int any_object_can_occupy_terrain_cell(int cx, int cy)
{
    int count = 0;
    const void* recs = active_context_record_get(&count);
    if (recs == NULL || count <= 0) {
        return 0;
    }
    const LevelState* lvl = (s_ctx != NULL)
        ? (const LevelState*)s_ctx->level : NULL;
    const EntitySlot* const* arr = (const EntitySlot* const*)recs;
    /* Corpus 0x431aef..0x431af5 advances the array pointer and uses an
     * unsigned loop bound. Keep the signed nonpositive guard above so a
     * negative port count still answers zero before the unsigned walk. */
    const EntitySlot* const* current = arr;
    for (unsigned i = 0; i < (unsigned)count; ++i, ++current) {
        if (terrain_cell_allows_placement(*current, lvl, cx, cy) != 0) {
            return 1;
        }
    }
    return 0;
}

namespace unported_ledger_stubs {
/* Port model of 0x00431b20 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x00431b20, retires to the tail in GAME-021 part 3
uint32_t terrain_placement_blocked_adjacent_cell_scan(
    int x, int y, int mode, int duplicate_pair)
{
    static const int offset[4][2] = {
        { 0, -1 }, { 1, 0 }, { 0, 1 }, { -1, 0 }
    };
    GameContext* game = entity_pool_callback_context_get();
    LevelState* level = game != NULL ? game->level : NULL;
    uint8_t* cell;
    uint32_t flags;
    uint8_t type;
    void* descriptor;
    uint32_t index;
    if (level == NULL)
        level = (LevelState*)map_cell_record_grid_level_get();
    if (level == NULL) return 0u;
    cell = level_voxel(level, x, y);
    if (cell == NULL || (cell[0x0c] & 0x20u) != 0u) return 0u;
    flags = *(uint32_t*)(cell + 0x08);
    type = cell[0x02];

    descriptor = path_record_last_peek();
    while (descriptor != NULL) {
        const uint32_t* words = (const uint32_t*)descriptor;
        if (words[3] == (uint32_t)x && words[4] == (uint32_t)y)
            return 0u;
        descriptor = *(void**)((uint8_t*)descriptor + 0x24);
    }
    if ((flags & 0x20000000u) != 0u) return 0u;
    if (mode != 0 && duplicate_pair != 0 && type != 9u) return 0u;
    if (mode != 0 && (flags & 0x8000u) != 0u) return 0u;
    if ((duplicate_pair == 0 && type == 9u) || type == 6u) return 0u;
    if ((flags & 8u) == 0u || (flags & 0x200000u) == 0u ||
        (flags & 0x108400u) != 0u) return 0u;
    if (grid_cell_record_present_check(x, y) != 0) return 0u;

    for (index = 0u; index < 4u; ++index) {
        uint8_t* neighbor = level_voxel(
            level, x + offset[index][0], y + offset[index][1]);
        if (neighbor == NULL || neighbor[0x02] == 11u) return 0u;
    }
    return 1u;
}
} // namespace unported_ledger_stubs

/* Port model of 0x00431c70 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x00431c70, retires to the tail in GAME-021 part 3
int first_free_neighbor_cell_pick_current(const struct EntitySlot* entity,
                                          const int* cell_in,
                                          int* cell_out, int mode)
{
#if defined(_MSC_VER) && defined(_M_IX86)
    int local_20[8];
    uint32_t local_24;
    uint32_t direction;
    uint32_t width;
    uint32_t height;
    uint32_t index;
    uint32_t flags;
    int x;
    int y;
    int result;
    LevelState* level;
    uint8_t* voxels;

    local_20[1] = -1;
    local_20[6] = -1;
    ++s_neighbor_pick_rotation;
    local_20[0] = 0;
    local_20[2] = 1;
    local_20[3] = 0;
    local_20[4] = 0;
    local_20[5] = 1;
    local_20[7] = 0;
    local_24 = 0u;
    level = g_active_level_retail;
    do {
        direction = (s_neighbor_pick_rotation + local_24) & 3u;
        x = local_20[direction * 2u] + cell_in[0];
        y = local_20[direction * 2u + 1u] + cell_in[1];
        if (mode != 0) {
            width = level->word[LEVEL_OFF_WIDTH / 4u];
            height = level->word[LEVEL_OFF_HEIGHT / 4u];
            if ((uint32_t)x < width - 1u &&
                (uint32_t)y < height - 1u) {
                voxels = (uint8_t*)(uintptr_t)
                    level->word[LEVEL_OFF_VOXELS / 4u];
                index = (uint32_t)y * width + (uint32_t)x;
                flags = *(uint32_t*)(voxels + 8u + index * 0x48u);
                if ((flags & 0x10u) != 0u && (flags & 8u) == 0u) {
                    ++local_24;
                    if (local_24 > 3u)
                        return 0;
                    continue;
                }
            }
        }
        result = terrain_cell_route_and_occupancy_check_current(
            entity, x, y, x, y, 0);
        level = g_active_level_retail;
        if (result != 0) {
            cell_out[0] = x;
            cell_out[1] = y;
            return 1;
        }
        ++local_24;
        if (local_24 > 3u)
            return 0;
    } while (1);
#else
    LevelState* level;
    int probe;

    /* Keep the retail entry's four-neighbor walk in this mapped body. The
     * reference increments DAT_004de588 once per call, then indexes the
     * cardinal table with (counter + probe) & 3. */
    ++s_neighbor_pick_rotation;
    level = level_current_owner_get();
    for (probe = 0; probe < 4; ++probe) {
        const uint32_t direction =
            (s_neighbor_pick_rotation + (uint32_t)probe) & 3u;
        const int x = cell_in[0] + s_cardinal_deltas[direction][0];
        const int y = cell_in[1] + s_cardinal_deltas[direction][1];

        if (mode != 0 && level != NULL &&
            (unsigned)x < (unsigned)level_width(level) - 1u &&
            (unsigned)y < (unsigned)level_height(level) - 1u) {
            const uint32_t flags = route_cell_flags(level, x, y);
            if ((flags & 0x10u) != 0u && (flags & 8u) == 0u) {
                continue;
            }
        }

        /* FUN_00431da0 resolves DAT_005560d4 itself, so reacquire the active
         * level for every probe just as the retail caller does. */
        if (terrain_cell_route_and_occupancy_check(
                entity, level_current_owner_get(), x, y, x, y, 0) != 0) {
            cell_out[0] = x;
            cell_out[1] = y;
            return 1;
        }
    }
    return 0;
#endif
}
/* Port model of 0x00431da0 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x00431da0, retires to the tail in GAME-021 part 3
int terrain_cell_route_and_occupancy_check_current(
    const struct EntitySlot* entity, int from_x, int from_y,
    int to_x, int to_y, int strict)
{
#if defined(_MSC_VER) && defined(_M_IX86)
    int local_20[8];
    int delta_index;
    int delta_slot;
    int entity_cell_x;
    int entity_cell_y;
    uint32_t width;
    uint32_t source_index;
    uint32_t dest_index;
    uint32_t flags;
    uint32_t block_relief;
    uint32_t special;
    uint32_t idle_gate;
    uint32_t walk_gate;
    uint32_t state_mask;
    uint32_t answer;
    uint8_t* voxels;
    uint8_t* source;
    uint8_t* dest;
    LevelState* level;

    level = g_active_level_retail;
    width = level->word[LEVEL_OFF_WIDTH / 4u];
    voxels = (uint8_t*)(uintptr_t)
        level->word[LEVEL_OFF_VOXELS / 4u];
    source_index = (uint32_t)from_y * width + (uint32_t)from_x;
    source = voxels + source_index * 0x48u;
    dest_index = (uint32_t)to_y * width + (uint32_t)to_x;
    dest = voxels + dest_index * 0x48u;
    special = 0u;
    walk_gate = 1u;
    idle_gate = 0u;
    state_mask = 0u;
    answer = 1u;
    local_20[0] = 0;
    local_20[3] = 0;
    local_20[1] = -1;
    local_20[2] = 1;
    local_20[4] = 0;
    local_20[5] = 1;
    local_20[6] = -1;
    local_20[7] = 0;
    if (entity != NULL &&
        (entity_stats_word_get(entity) & 0x20000u) != 0u &&
        entity->owner_36c == NULL) {
        return 0;
    }
    delta_slot = 0;
    delta_index = 0;
    for (;;) {
        if (local_20[delta_slot * 2] + to_x == from_x &&
            to_y + local_20[delta_slot * 2 + 1] == from_y) {
            break;
        }
        ++delta_slot;
        ++delta_index;
        if (delta_slot >= 4)
            break;
    }
    if (entity != NULL) {
        if (entity->type == 0x0b)
            return 0;
        block_relief = entity_capability_flags_get(entity) & 0x200u;
        special = entity_stats_word_get(entity) & 1u;
        idle_gate = entity_stats_word_get(entity) & 0x1000000u;
        walk_gate = entity_stats_word_get(entity) & 0x2000000u;
        state_mask = entity_state_flags_get(entity) & 0x200000u;
        if (strict == 0) {
            entity_cell_x = 0;
            entity_cell_y = 0;
            entity_position_to_terrain_cell(entity, level, &entity_cell_x,
                                            &entity_cell_y);
            if (from_x == entity_cell_x && from_y == entity_cell_y)
                return 1;
        }
    }
    else {
        block_relief = 0u;
    }

    if (source[2] == 9) {
        if (idle_gate == 0u)
            return 0;
    }
    else if (walk_gate == 0u) {
        return 0;
    }
    if (source[2] == 6 && state_mask == 0u &&
        (*(uint32_t*)(source + 8) & 0x200u) == 0u) {
        return 0;
    }
    flags = *(uint32_t*)(source + 8);
    if ((flags & 0x400u) != 0u)
        return 0;
    if (strict != 0 && (flags & 0x40u) != 0u)
        return 0;
    if (block_relief == 0u && (source[0x0c] & 4u) != 0u)
        return 0;
    if (special == 0u) {
        if ((flags & 8u) == 0u)
            return 0;
    }
    else {
        if ((flags & 0x10u) != 0u)
            answer = 3u;
        if (source[0] == 0u)
            return 0;
        if ((flags & 0x10u) != 0u) {
            uint32_t above_index = ((uint32_t)from_y - 1u) * width +
                                   (uint32_t)from_x;
            uint8_t* above = voxels + above_index * 0x48u;
            if (source[2] == 6 || above[2] == 6)
                return 0;
            if (source[2] == 7 || above[2] == 7)
                return 0;
        }
        if (delta_index == 0) {
            if ((flags & 0x2000u) != 0u ||
                (*(uint32_t*)(dest + 8) & 0x2000u) != 0u) {
                answer = 2u;
            }
        }
    }
    if (to_x != from_x && to_y != from_y) {
        uint32_t cross_a = (uint32_t)to_y * width + (uint32_t)from_x;
        uint32_t cross_b = (uint32_t)from_y * width + (uint32_t)to_x;
        uint8_t* cell_a = voxels + cross_a * 0x48u;
        uint8_t* cell_b = voxels + cross_b * 0x48u;
        if ((*(uint32_t*)(cell_a + 8) & 8u) != 0u &&
            (*(uint32_t*)(cell_b + 8) & 8u) != 0u) {
            return 2;
        }
    }
    return (int)answer;
#else
    return terrain_cell_route_and_occupancy_check(
        entity, level_current_owner_get(), from_x, from_y, to_x, to_y,
        strict);
#endif
}
/* The target sets the secondary +0x0c flag, appends the coordinate to the
 * refresh queue, and invokes the one-cell initializer. */
/* Port model of 0x00432100 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x00432100, retires to the tail in GAME-021 part 3
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) void terrain_cell_flag100_set_and_track_current(
    const int cell_xy[2])
{
    __asm {
        mov edx, dword ptr [g_active_level_retail]
        mov ecx, dword ptr [esp + 4]
        push esi
        push 0
        mov eax, dword ptr [edx + 14h]
        mov esi, dword ptr [ecx]
        imul eax, dword ptr [ecx + 4]
        add eax, esi
        mov esi, dword ptr [edx + 0b8h]
        lea eax, dword ptr [eax + eax*8]
        lea eax, dword ptr [esi + eax*8 + 0ch]
        mov esi, dword ptr [eax]
        or esi, 100h
        mov dword ptr [eax], esi
        mov esi, dword ptr [s_cell_refresh_track_count]
        mov eax, dword ptr [ecx]
        mov dword ptr [esi*8 + s_cell_refresh_track], eax
        mov esi, dword ptr [s_cell_refresh_track_count]
        mov eax, dword ptr [ecx + 4]
        mov dword ptr [esi*8 + s_cell_refresh_track + 4], eax
        mov esi, dword ptr [s_cell_refresh_track_count]
        inc esi
        mov dword ptr [s_cell_refresh_track_count], esi
        mov eax, dword ptr [ecx + 4]
        mov ecx, dword ptr [ecx]
        push eax
        push ecx
        push edx
        call cell_init_pass
        add esp, 10h
        pop esi
        ret
    }
}
#else
void terrain_cell_flag100_set_and_track_current(const int cell_xy[2])
{
    LevelState* level = terrain_current_level_resolve();
    const uint32_t width = level->word[LEVEL_OFF_WIDTH / 4u];
    const size_t index = (size_t)cell_xy[1] * width +
                         (size_t)cell_xy[0];
    uint8_t* record = (uint8_t*)level->ptr_voxels +
                      index * LEVEL_VOXEL_STRIDE;
    uint32_t* word = (uint32_t*)(record + 0x0cu);
    *word |= 0x100u;
    /* Retail appends directly at DAT_0055684c[count] and increments the
     * global count without a capacity test. Keep this target entry's write
     * sequence direct; the explicit-LevelState adapter keeps its bounded
     * host-side helper. */
    s_cell_refresh_track[s_cell_refresh_track_count][0] = cell_xy[0];
    s_cell_refresh_track[s_cell_refresh_track_count][1] = cell_xy[1];
    ++s_cell_refresh_track_count;
    cell_init_pass(level, (float)cell_xy[0], (uint32_t)cell_xy[1]);
}
#endif
/* Port model of 0x00432170 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x00432170, retires to the tail in GAME-021 part 3
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) uint32_t terrain_cell_secondary_flag_100_current(
    const int cell_xy[2])
{
    __asm {
        mov edx, dword ptr [esp + 4]
        mov ecx, dword ptr [g_active_level_retail]
        push esi
        mov eax, dword ptr [edx + 4]
        mov esi, dword ptr [edx]
        imul eax, dword ptr [ecx + 14h]
        mov ecx, dword ptr [ecx + 0b8h]
        add eax, esi
        pop esi
        lea eax, dword ptr [eax + eax*8]
        mov eax, dword ptr [ecx + eax*8 + 0ch]
        and eax, 100h
        ret
    }
}
#else
uint32_t terrain_cell_secondary_flag_100_current(const int cell_xy[2])
{
    LevelState* level = g_active_level_retail;
    int width = (int)level->word[LEVEL_OFF_WIDTH / 4];
    uint8_t* voxels = (uint8_t*)level->ptr_voxels;
    int cell_index = cell_xy[1] * width + cell_xy[0];
    uint32_t flags = *(uint32_t*)(voxels + 0x0c + cell_index * 0x48);
    return flags & 0x100u;
}
#endif

/* TerrainCellUpdateQueueFlush (FUN_004321a0): after the entity walk, run
 * CellInitPass for each snapshotted cell whose 0x100 secondary mark remains
 * clear, then discard the snapshot count. */
/* Port model of 0x004321a0 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x004321a0, retires to the tail in GAME-021 part 3
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) void terrain_cell_update_queue_flush_current(void)
{
    __asm {
        mov eax, dword ptr [s_cell_previous_track_count]
        push ebx
        xor ebx, ebx
        push esi
        mov esi, dword ptr [g_active_level_retail]
        test eax, eax
        jbe terrain_cell_update_queue_flush_done
        push edi
        push ebp
        mov edi, offset s_cell_previous_track
    terrain_cell_update_queue_flush_loop:
        mov ecx, dword ptr [edi + 4]
        mov eax, dword ptr [esi + 14h]
        imul eax, ecx
        mov edx, dword ptr [edi]
        mov ebp, dword ptr [esi + 0b8h]
        add eax, edx
        lea eax, dword ptr [eax + eax*8]
        test byte ptr [ebp + eax*8 + 0dh], 1
        jne terrain_cell_update_queue_flush_next
        push 0
        push ecx
        push edx
        push esi
        call cell_init_pass
        add esp, 10h
    terrain_cell_update_queue_flush_next:
        mov eax, dword ptr [s_cell_previous_track_count]
        inc ebx
        add edi, 8
        cmp ebx, eax
        jb terrain_cell_update_queue_flush_loop
        pop ebp
        pop edi
    terrain_cell_update_queue_flush_done:
        pop esi
        mov dword ptr [s_cell_previous_track_count], 0
        pop ebx
        ret
    }
}
#else
void terrain_cell_update_queue_flush_current(void)
{
    LevelState* level = terrain_current_level_resolve();
    uint32_t i;
    if (level == NULL) return;

    for (i = 0u; i < (uint32_t)s_cell_previous_track_count; ++i) {
        const int x = s_cell_previous_track[i][0];
        const int y = s_cell_previous_track[i][1];
        const uint8_t* record = cell_record_unchecked_const(level, x, y);
        if (record != NULL && (record[0x0d] & 1u) == 0u) {
            cell_init_pass(level, (float)x, (uint32_t)y);
        }
    }
    s_cell_previous_track_count = 0;
}
#endif
/* TerrainCellPreviousStateTrack (FUN_00432200): copy the just-collected
 * coordinates into the second 100-pair buffer, clear each cell's secondary
 * 0x100 mark, then publish that buffer as the post-walk update queue. The
 * order is significant: the entity walk runs after these marks are cleared
 * and may set them again to suppress the later CellInitPass. */
/* Port model of 0x00432200 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x00432200, retires to the tail in GAME-021 part 3
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) void terrain_cell_previous_state_track_current(void)
{
    __asm {
        mov eax, dword ptr [s_cell_refresh_track_count]
        xor edx, edx
        push esi
        mov esi, dword ptr [g_active_level_retail]
        test eax, eax
        jbe terrain_cell_previous_state_track_done
        push edi
        mov ecx, offset s_cell_refresh_track
    terrain_cell_previous_state_track_loop:
        mov eax, dword ptr [ecx]
        mov dword ptr [ecx + 320h], eax
        mov eax, dword ptr [ecx + 4]
        mov dword ptr [ecx + 324h], eax
        mov eax, dword ptr [ecx + 4]
        imul eax, dword ptr [esi + 14h]
        mov edi, dword ptr [ecx]
        add ecx, 8
        add eax, edi
        mov edi, dword ptr [esi + 0b8h]
        lea eax, dword ptr [eax + eax*8]
        lea eax, dword ptr [edi + eax*8 + 0ch]
        mov edi, dword ptr [eax]
        and edi, 0fffffeffh
        inc edx
        mov dword ptr [eax], edi
        mov eax, dword ptr [s_cell_refresh_track_count]
        cmp edx, eax
        jb terrain_cell_previous_state_track_loop
        pop edi
    terrain_cell_previous_state_track_done:
        mov dword ptr [s_cell_previous_track_count], eax
        mov dword ptr [s_cell_refresh_track_count], 0
        pop esi
        ret
    }
}
#else
void terrain_cell_previous_state_track_current(void)
{
    LevelState* level = terrain_current_level_resolve();
    uint32_t i;
    if (level == NULL) return;

    for (i = 0u; i < (uint32_t)s_cell_refresh_track_count; ++i) {
        const int x = s_cell_refresh_track[i][0];
        const int y = s_cell_refresh_track[i][1];
        uint8_t* record = cell_record_unchecked(level, x, y);
        s_cell_previous_track[i][0] = x;
        s_cell_previous_track[i][1] = y;
        if (record != NULL) {
            uint32_t* secondary = (uint32_t*)(record + 0x0c);
            *secondary &= ~0x100u;
        }
    }
    s_cell_previous_track_count = s_cell_refresh_track_count;
    s_cell_refresh_track_count = 0;
}
#endif

/* The retail entry point reads DAT_005560d4 and appends without checking the
 * 100-pair table's capacity. Keep the explicit-level adapter bounded. */
/* Port model of 0x00432270 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x00432270, retires to the tail in GAME-021 part 3
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) void terrain_cell_visited_flag_set_and_track_current(
    const int cell_xy[2])
{
    __asm {
        mov edx, dword ptr [g_active_level_retail]
        mov ecx, dword ptr [esp + 4]
        push esi
        mov eax, dword ptr [edx + 14h]
        mov esi, dword ptr [ecx]
        imul eax, dword ptr [ecx + 4]
        mov edx, dword ptr [edx + 0b8h]
        add eax, esi
        pop esi
        lea eax, dword ptr [eax + eax*8]
        lea eax, dword ptr [edx + eax*8 + 0ch]
        mov edx, dword ptr [eax]
        or edx, 2
        mov dword ptr [eax], edx
        mov edx, dword ptr [s_cell_visited_track_count]
        mov eax, dword ptr [ecx]
        mov dword ptr [edx*8 + s_cell_visited_track], eax
        mov eax, dword ptr [ecx + 4]
        mov ecx, dword ptr [s_cell_visited_track_count]
        mov dword ptr [ecx*8 + s_cell_visited_track + 4], eax
        mov eax, dword ptr [s_cell_visited_track_count]
        inc eax
        mov dword ptr [s_cell_visited_track_count], eax
        ret
    }
}
#else
void terrain_cell_visited_flag_set_and_track_current(const int cell_xy[2])
{
    LevelState* s = terrain_current_level_resolve();
    uint8_t* rec;
    uint32_t* word;
    if (s == NULL || cell_xy == NULL) return;
    rec = cell_record_unchecked(s, cell_xy[0], cell_xy[1]);
    if (rec == NULL) return;
    word = (uint32_t*)(rec + 0x0c);
    *word |= 2u;
    s_cell_visited_track[s_cell_visited_track_count][0] = cell_xy[0];
    s_cell_visited_track[s_cell_visited_track_count][1] = cell_xy[1];
    ++s_cell_visited_track_count;
}
#endif
/* Port model of 0x004322d0 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x004322d0, retires to the tail in GAME-021 part 3
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) uint32_t terrain_cell_visited_flag_current(
    const int cell_xy[2])
{
    __asm {
        mov ecx, dword ptr [g_active_level_retail]
        mov edx, dword ptr [esp + 4]
        push esi
        mov eax, dword ptr [ecx + 14h]
        mov esi, dword ptr [edx]
        imul eax, dword ptr [edx + 4]
        mov ecx, dword ptr [ecx + 0b8h]
        add eax, esi
        pop esi
        lea eax, dword ptr [eax + eax*8]
        mov eax, dword ptr [ecx + eax*8 + 0ch]
        and eax, 2
        ret
    }
}
#else
uint32_t terrain_cell_visited_flag_current(const int cell_xy[2])
{
    LevelState* level = g_active_level_retail;
    int width = (int)level->word[LEVEL_OFF_WIDTH / 4];
    uint8_t* voxels = (uint8_t*)level->ptr_voxels;
    int cell_index = width * cell_xy[1] + cell_xy[0];
    uint32_t flags = *(uint32_t*)(voxels + 0x0c + cell_index * 0x48);
    return flags & 2u;
}
#endif

/* The retail entry point is parameterless and reads the current level from
 * DAT_005560d4. Keep the LevelState-taking form above as a portable adapter
 * for callers that already own an explicit level. */
/* Port model of 0x00432300 (GAME-021) for the targets other than MSVC x86.
 * Under MSVC x86, lego_unit.c defines terrain_cell_visited_flags_clear_all_current
 * (shared: same contract and the same globals). */
// PORT-ONLY: port model of 0x00432300 for non-MSVC targets; lego_unit.c defines it under MSVC x86
#if !(defined(_MSC_VER) && defined(_M_IX86))
void terrain_cell_visited_flags_clear_all_current(void)
{
    LevelState* s = terrain_current_level_resolve();
    int i;
    if (s == NULL) return;
    for (i = 0; i < s_cell_visited_track_count; ++i) {
        uint8_t* rec = cell_record_unchecked(s, s_cell_visited_track[i][0],
                                             s_cell_visited_track[i][1]);
        if (rec != NULL) {
            uint32_t* word = (uint32_t*)(rec + 0x0C);
            *word &= ~2u;
        }
    }
    s_cell_visited_track_count = 0;
}
#endif

/* Port model of 0x00432360 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x00432360, retires to the tail in GAME-021 part 3
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) void terrain_cell_primary_flags_mask_clear_and_refresh_current(
    const int cell_xy[2])
{
    __asm {
        mov ecx, dword ptr [g_active_level_retail]
        mov edx, dword ptr [esp + 4]
        push esi
        push 0
        mov eax, dword ptr [ecx + 14h]
        mov esi, dword ptr [edx]
        imul eax, dword ptr [edx + 4]
        add eax, esi
        mov esi, dword ptr [ecx + 0b8h]
        lea eax, dword ptr [eax + eax*8]
        lea eax, dword ptr [esi + eax*8 + 8]
        mov esi, dword ptr [eax]
        and esi, 0dfef7bffh
        mov dword ptr [eax], esi
        mov eax, dword ptr [ecx + 14h]
        imul eax, dword ptr [edx + 4]
        mov esi, dword ptr [edx]
        add eax, esi
        mov esi, dword ptr [ecx + 0b8h]
        lea eax, dword ptr [eax + eax*8]
        lea eax, dword ptr [esi + eax*8 + 0ch]
        mov esi, dword ptr [eax]
        and esi, 0fffffffbh
        mov dword ptr [eax], esi
        mov eax, dword ptr [edx + 4]
        mov edx, dword ptr [edx]
        push eax
        push edx
        push ecx
        call cell_init_pass
        add esp, 10h
        pop esi
        ret
    }
}
#else
void terrain_cell_primary_flags_mask_clear_and_refresh_current(
    const int cell_xy[2])
{
    LevelState* s = terrain_current_level_resolve();
    uint8_t* rec;
    uint32_t* primary;
    uint32_t* secondary;
    uint32_t value;
    if (s == NULL || cell_xy == NULL) return;
    rec = cell_record_unchecked(s, cell_xy[0], cell_xy[1]);
    if (rec == NULL) {
        return;
    }
    primary = (uint32_t*)(rec + 0x08);
    value = *primary;
    value &= 0xdfef7bffu;
    *primary = value;
    secondary = (uint32_t*)(rec + 0x0C);
    value = *secondary;
    value &= 0xfffffffbu;
    *secondary = value;
    cell_init_pass(s, (float)cell_xy[0], (uint32_t)cell_xy[1]);
}
#endif
/* Port model of 0x004323c0 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x004323c0, retires to the tail in GAME-021 part 3
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) void terrain_cell_secondary_bit0_clear_current(
    const int cell_xy[2])
{
    __asm {
        mov edx, dword ptr [esp + 4]
        mov ecx, dword ptr [g_active_level_retail]
        push esi
        mov eax, dword ptr [edx + 4]
        mov esi, dword ptr [edx]
        imul eax, dword ptr [ecx + 14h]
        mov ecx, dword ptr [ecx + 0b8h]
        add eax, esi
        pop esi
        lea eax, dword ptr [eax + eax*8]
        lea eax, dword ptr [ecx + eax*8 + 0ch]
        and dword ptr [eax], 0fffffffeh
        ret
    }
}
#else
void terrain_cell_secondary_bit0_clear_current(const int cell_xy[2])
{
    LevelState* level = g_active_level_retail;
    int width = (int)level->word[LEVEL_OFF_WIDTH / 4];
    uint8_t* voxels = (uint8_t*)level->ptr_voxels;
    int cell_index = cell_xy[1] * width + cell_xy[0];
    uint32_t* flags = (uint32_t*)(voxels + 0x0c + cell_index * 0x48);
    *flags = *flags & 0xfffffffeu;
}
#endif

/* Port model of 0x004323f0 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x004323f0, retires to the tail in GAME-021 part 3
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) void terrain_cell_flags_set_and_refresh_current(
    const int cell_xy[2])
{
    __asm {
        mov ecx, dword ptr [g_active_level_retail]
        mov edx, dword ptr [esp + 4]
        push esi
        push 0
        mov eax, dword ptr [ecx + 14h]
        mov esi, dword ptr [edx]
        imul eax, dword ptr [edx + 4]
        add eax, esi
        mov esi, dword ptr [ecx + 0b8h]
        lea eax, dword ptr [eax + eax*8]
        lea eax, dword ptr [esi + eax*8 + 8]
        mov esi, dword ptr [eax]
        or esi, 108000h
        mov dword ptr [eax], esi
        mov eax, dword ptr [ecx + 14h]
        imul eax, dword ptr [edx + 4]
        mov esi, dword ptr [edx]
        add eax, esi
        mov esi, dword ptr [ecx + 0b8h]
        lea eax, dword ptr [eax + eax*8]
        lea eax, dword ptr [esi + eax*8 + 0ch]
        mov esi, dword ptr [eax]
        or esi, 4
        mov dword ptr [eax], esi
        mov eax, dword ptr [edx + 4]
        mov edx, dword ptr [edx]
        push eax
        push edx
        push ecx
        call cell_init_pass
        add esp, 10h
        pop esi
        ret
    }
}
#else
void terrain_cell_flags_set_and_refresh_current(const int cell_xy[2])
{
    LevelState* s = terrain_current_level_resolve();
    uint8_t* rec;
    uint32_t* primary;
    uint32_t* secondary;
    uint32_t value;
    if (s == NULL || cell_xy == NULL) return;
    rec = cell_record_unchecked(s, cell_xy[0], cell_xy[1]);
    if (rec == NULL) {
        return;
    }
    primary = (uint32_t*)(rec + 0x08);
    value = *primary;
    value |= 0x108000u;
    *primary = value;
    secondary = (uint32_t*)(rec + 0x0C);
    value = *secondary;
    value |= 4u;
    *secondary = value;
    cell_init_pass(s, (float)cell_xy[0], (uint32_t)cell_xy[1]);
}
#endif

/* Port model of 0x00432450 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x00432450, retires to the tail in GAME-021 part 3
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) void terrain_cell_flags_set_mask100400_current(int x,
                                                                  int y)
{
    __asm {
        mov ecx, dword ptr [g_active_level_retail]
        mov edx, dword ptr [esp + 8]
        push esi
        mov esi, dword ptr [esp + 8]
        mov eax, dword ptr [ecx + 14h]
        push edi
        imul eax, edx
        mov edi, dword ptr [ecx + 0b8h]
        add eax, esi
        push 0
        push edx
        lea eax, dword ptr [eax + eax*8]
        push esi
        push ecx
        lea eax, dword ptr [edi + eax*8 + 8]
        or dword ptr [eax], 100400h
        call cell_init_pass
        add esp, 10h
        pop edi
        pop esi
        ret
    }
}
#else
void terrain_cell_flags_set_mask100400_current(int x, int y)
{
    LevelState* s = level_current_owner_get();
    uint8_t* rec;
    if (s == NULL) return;
    rec = cell_record_unchecked(s, x, y);
    if (rec == NULL) return;
    *(uint32_t*)(rec + 0x08) |= 0x100400u;
    cell_init_pass(s, (float)x, (uint32_t)y);
}
#endif

/* Port model of 0x00432490 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x00432490, retires to the tail in GAME-021 part 3
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) void terrain_cell_flags_set_mask20108000_current(int x,
                                                                     int y)
{
    __asm {
        mov ecx, dword ptr [g_active_level_retail]
        mov edx, dword ptr [esp + 8]
        push esi
        mov esi, dword ptr [esp + 8]
        mov eax, dword ptr [ecx + 14h]
        push edi
        imul eax, edx
        mov edi, dword ptr [ecx + 0b8h]
        add eax, esi
        push 0
        push edx
        lea eax, dword ptr [eax + eax*8]
        push esi
        push ecx
        lea eax, dword ptr [edi + eax*8 + 8]
        or dword ptr [eax], 20108000h
        call cell_init_pass
        add esp, 10h
        pop edi
        pop esi
        ret
    }
}
#else
void terrain_cell_flags_set_mask20108000_current(int x, int y)
{
    LevelState* s = level_current_owner_get();
    uint8_t* rec;
    if (s == NULL) return;
    rec = cell_record_unchecked(s, x, y);
    if (rec == NULL) return;
    *(uint32_t*)(rec + 0x08) |= 0x20108000u;
    cell_init_pass(s, (float)x, (uint32_t)y);
}
#endif
/* Port model of 0x004324d0 (GAME-021). The original-shape function in lego_unit.c
 * shares its name in the matching build (MSVC x86). */
// PORT-ONLY: port model of 0x004324d0, retires to the tail in GAME-021 part 3
#if !(defined(_MSC_VER) && defined(_M_IX86))
void terrain_cell_secondary_flag_8_set_current(int cx, int cy, int set)
{
    if (set != 0) {
        LevelState* level = g_active_level_retail;
        int width = (int)level->word[LEVEL_OFF_WIDTH / 4];
#if defined(_WIN32) && defined(_M_IX86)
        volatile uint32_t* words = (volatile uint32_t*)level->word;
        uint8_t* voxels = (uint8_t*)(uintptr_t)
            words[LEVEL_OFF_VOXELS / 4];
#else
        uint8_t* voxels = (uint8_t*)level->ptr_voxels;
#endif
        int cell_index = width * cy + cx;
        uint32_t* flags = (uint32_t*)(voxels + 0x0c + cell_index * 0x48);
        *flags |= 8u;
        return;
    }
    {
        LevelState* level = g_active_level_retail;
        int width = (int)level->word[LEVEL_OFF_WIDTH / 4];
#if defined(_WIN32) && defined(_M_IX86)
        volatile uint32_t* words = (volatile uint32_t*)level->word;
        uint8_t* voxels = (uint8_t*)(uintptr_t)
            words[LEVEL_OFF_VOXELS / 4];
#else
        uint8_t* voxels = (uint8_t*)level->ptr_voxels;
#endif
        int cell_index = width * cy + cx;
        uint32_t* flags = (uint32_t*)(voxels + 0x0c + cell_index * 0x48);
        *flags &= ~8u;
    }
}
#endif

/* Port model of 0x00432520 (GAME-021). The original-shape function in lego_unit.c
 * shares its name in the matching build (MSVC x86). */
// PORT-ONLY: port model of 0x00432520, retires to the tail in GAME-021 part 3
#if !(defined(_MSC_VER) && defined(_M_IX86))
uint32_t terrain_cell_secondary_flag_8_current(int cx, int cy)
{
    LevelState* level = g_active_level_retail;
    int width = (int)level->word[LEVEL_OFF_WIDTH / 4];
#if defined(_WIN32) && defined(_M_IX86)
    uint8_t* voxels = (uint8_t*)(uintptr_t)
        level->word[LEVEL_OFF_VOXELS / 4];
#else
    uint8_t* voxels = (uint8_t*)level->ptr_voxels;
#endif
    int cell_index = width * cy + cx;
    uint32_t flags = *(uint32_t*)(voxels + 0x0c + cell_index * 0x48);
    return flags & 8u;
}
#endif

/* AlwaysAcceptPlaceCommandCell (FUN_00432550, finding 455): always yes. */
/* Port model of 0x00432550 (GAME-021). The original-shape function in lego_unit.c
 * shares its name in the matching build (MSVC x86). */
// PORT-ONLY: port model of 0x00432550, retires to the tail in GAME-021 part 3
#if !(defined(_MSC_VER) && defined(_M_IX86))
int always_accept_place_command_cell(void)
{
    return 1;
}
#endif

/* Level_BlockUpdateSurface (0042f6f0) with the original's ABI, in lego_unit.c.
 * The two naked bodies below push its four integer arguments. */
extern "C" void lego_cell_init_pass(struct Lego_Level* level, unsigned int bx,
                                    unsigned int by, int reserved);

/* Port model of 0x00432560 (GAME-021). The original-shape function in lego_unit.c
 * shares its name in the matching build (MSVC x86). */
// PORT-ONLY: port model of 0x00432560, retires to the tail in GAME-021 part 3
#if !(defined(_MSC_VER) && defined(_M_IX86))
void terrain_cell_flag40000000_clear_or_set_current(const int cell_xy[2],
                                                     int set)
{
    LevelState* s = level_current_owner_get();
    uint8_t* rec;
    uint32_t* primary;
    uint32_t value;
    if (s == NULL || cell_xy == NULL) return;
    /* The retail entry addresses the cell without a grid-bounds check. */
    rec = cell_record_unchecked(s, cell_xy[0], cell_xy[1]);
    if (rec == NULL) {
        return;
    }
    primary = (uint32_t*)(rec + 0x08);
    value = *primary;
    if (set == 0) {
        value &= 0xbfffffffu;
    } else {
        value |= 0x40000000u;
    }
    *primary = value;
    cell_init_pass(s, (float)cell_xy[0], (uint32_t)cell_xy[1], 0u);
}
#endif
/* Port model of 0x004325d0 (GAME-021). The original-shape function in lego_unit.c
 * shares its name in the matching build (MSVC x86). */
// PORT-ONLY: port model of 0x004325d0, retires to the tail in GAME-021 part 3
#if !(defined(_MSC_VER) && defined(_M_IX86))
void terrain_cell_secondary_bit0_set_current(const int cell_xy[2])
{
    LevelState* level = g_active_level_retail;
    int width = (int)level->word[LEVEL_OFF_WIDTH / 4];
    uint8_t* voxels = (uint8_t*)level->ptr_voxels;
    int cell_index = cell_xy[1] * width + cell_xy[0];
    uint32_t* flags = (uint32_t*)(voxels + 0x0c + cell_index * 0x48);
    *flags = *flags | 1u;
}
#endif

/* Port model of 0x00432600 (GAME-021). The original-shape function in lego_unit.c
 * shares its name in the matching build (MSVC x86). */
// PORT-ONLY: port model of 0x00432600, retires to the tail in GAME-021 part 3
#if !(defined(_MSC_VER) && defined(_M_IX86))
int terrain_cell_variant_flag_propagate_current(const int cell_xy[2])
{
    static const int offsets[5][2] = {
        {0, 0}, {0, -1}, {1, 0}, {0, 1}, {-1, 0}
    };
    LevelState* s;
    uint8_t* rec;
    uint32_t* flags;
    uint32_t old_flags;
    int i;
    if (cell_xy == NULL) return 0;
    s = level_current_owner_get();
    if (s == NULL) return 0;
    rec = level_voxel(s, cell_xy[0], cell_xy[1]);
    if (rec == NULL) return 0;
    flags = (uint32_t*)(rec + 0x08);
    old_flags = *flags;
    if ((old_flags & 0x200000u) == 0u || rec[2] == 6u) return 0;
    if ((old_flags & 0x80000000u) == 0u) {
        *flags |= 0x20000000u;
    }
    else {
        lava_erosion_pending_cell_add(cell_xy, 0);
        rec[7] = 0;
    }
    *flags &= 0xbfffffffu;
    for (i = 0; i < 5; ++i) {
        cell_init_pass(s,
            (float)(cell_xy[0] + offsets[i][0]),
            (uint32_t)(cell_xy[1] + offsets[i][1]));
    }
    map_cell_tracking_activity_state_update(
        entity_pool_callback_context_get(), s);
    return 1;
}
#endif
/* Retail entry uses an unchecked coordinate-pair ABI; retain the explicit
 * LevelState helper above for portable callers. */
/* Port model of 0x00432710 (GAME-021). The original-shape function in lego_unit.c
 * shares its name in the matching build (MSVC x86). */
// PORT-ONLY: port model of 0x00432710, retires to the tail in GAME-021 part 3
#if !(defined(_MSC_VER) && defined(_M_IX86))
void terrain_cell_flag_80000_clear_or_set_current(const int cell_xy[2],
                                                   int set)
{
    LevelState* level = g_active_level_retail;
    uint8_t* record = (uint8_t*)level->ptr_voxels +
        (cell_xy[1] * (int)level->word[LEVEL_OFF_WIDTH / 4] + cell_xy[0]) *
            LEVEL_VOXEL_STRIDE;
    uint32_t* flags = (uint32_t*)(record + 8);
    if (set != 0) *flags |= 0x80000u;
    else *flags &= ~0x80000u;
}
#endif
/* The mapped retail predicate carries an entity and two mode arguments and
 * reads the active level.  Keep level_walkable above as the simpler grid
 * convenience used by path-search code; this is the full address-level
 * behavior from FUN_00432770. */
/* Port model of 0x00432770 (GAME-021). The original-shape function in lego_unit.c
 * shares its name in the matching build (MSVC x86). */
// PORT-ONLY: port model of 0x00432770, retires to the tail in GAME-021 part 3
#if !(defined(_MSC_VER) && defined(_M_IX86))
int terrain_cell_walkability_query_current(const struct EntitySlot* e,
                                            uint32_t cx, uint32_t cy,
                                            int require_stat_bit,
                                            int gated_mode)
{
    LevelState* s = level_current_owner_get();
    const uint8_t* rec;
    uint32_t stats;
    int width, height;
    if (s == NULL || e == NULL) return 0;
    width = level_width(s);
    height = level_height(s);
    rec = level_voxel(s, (int)cx, (int)cy);
    if (rec == NULL) return 0;
    stats = entity_stats_word_get(e);
    if ((stats & 0x20000u) != 0u && e->owner_36c == NULL) return 0;
    if (require_stat_bit != 0 && (entity_stats_word_get(e) & 1u) == 0u)
        return 0;
    if (gated_mode == 0 || (e->flags_3e0 & 0x400u) == 0u) {
        if ((e->flags_3e8 & 2u) != 0u &&
            entity_stats_float_at24_indexed(e, (int)(signed char)rec[2]) != 0.0f &&
            cx < (uint32_t)width - 2u && cy < (uint32_t)height - 2u) {
            uint32_t flags;
            const int type = (int)(signed char)rec[2];
            __builtin_memcpy(&flags, rec + 8, sizeof flags);
            if (require_stat_bit == 0) {
                return (flags & 0x10u) != 0u && (flags & 0x40u) == 0u &&
                       type != 1 && type != 7 && type != 11;
            }
            if (rec[0] == 0) {
                static const int offsets[16] = {
                    0,-1, 1,0, 0,1, -1,0,
                    -1,-1, 0,-1, 0,0, -1,0
                };
                unsigned i;
                int any_cell = 0;
                for (i = 0; i < 4; ++i) {
                    const uint8_t* near_rec = level_voxel(
                        s, (int)cx + offsets[i * 2],
                        (int)cy + offsets[i * 2 + 1]);
                    if (near_rec != NULL && near_rec[0] != 0) {
                        any_cell = 1;
                        break;
                    }
                }
                if (any_cell) {
                    for (i = 0; i < 4; ++i) {
                        const uint8_t* near_rec = level_voxel(
                            s, (int)cx + offsets[8 + i * 2],
                            (int)cy + offsets[9 + i * 2]);
                        if (near_rec != NULL) {
                            const int near_type = (int)(signed char)near_rec[2];
                            if (near_type != 1 && near_type != 7) return 1;
                        }
                    }
                }
            }
        }
    }
    return 0;
}
#endif

/* FUN_00432950 — EntityMoveDestinationCellIsUsable. The corpus reads the
 * cell address without a bounds check past the destination gate (the gate
 * keeps every read in range); the port routes through level_voxel, whose
 * NULL for an unreachable cell reports unusable (documented deviation). */
/* Port model of 0x00432950 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x00432950, retires to the tail in GAME-021 part 3
int entity_move_destination_cell_usable(const struct EntitySlot* e,
                                       const LevelState* s, int cx, int cy)
{
    /* Corpus: *(uint *)(param_1 + 1000) & 0x8000 — +1000 decimal is
     * +0x3e8, the port's flags_3e8 mirror. */
    if (e == NULL || (e->flags_3e8 & 0x8000u) == 0u) {
        return 0;
    }
    if (s == NULL) s = level_current_owner_get();
    const int w = level_width(s), h = level_height(s);
    /* Corpus unsigned gates: cx != 0, cy != 0, and both < dim - 2. */
    if (cx == 0 || cy == 0
        || (unsigned)cx >= (unsigned)w - 2u
        || (unsigned)cy >= (unsigned)h - 2u) {
        return 0;
    }
    const uint8_t* rec = level_voxel(s, cx, cy);
    if (rec == NULL) {
        return 0;
    }
    uint32_t flags;
    __builtin_memcpy(&flags, rec + 0x08, sizeof flags);
    const int type = (int)(signed char)rec[0x02];
    return (flags & TILE_FLAG_WALKABLE) != 0
           && (flags & TILE_FLAG_BLOCKING) == 0
           && type != 1 && type != 7 && type != 11;
}

/* FUN_004329d0 ignores its first argument and reads the active level and its
 * terrain dimensions, then tests the voxel's +8 flag word. */
/* Port model of 0x004329d0 (GAME-021). The original-shape function in lego_unit.c
 * shares its name in the matching build (MSVC x86). */
// PORT-ONLY: port model of 0x004329d0, retires to the tail in GAME-021 part 3
#if !(defined(_MSC_VER) && defined(_M_IX86))
extern "C" int level_cell_flag8_check_target(const LevelState* unused,
                                         int cx, int cy)
{
    LevelState* level = g_active_level_retail;
    TerrainState* terrain;
    uint8_t* voxels;
    uint32_t index;
    (void)unused;
    if (level == NULL) return 0;
    terrain = (TerrainState*)level->ptr_terrain;
    if (terrain == NULL ||
        !level_coord_pair_within_bounds_target(terrain->dim,
            (uint32_t)cx, (uint32_t)cy)) return 0;
    voxels = (uint8_t*)level->ptr_voxels;
    if (voxels == NULL) return 0;
    index = (uint32_t)cy * level->word[LEVEL_OFF_WIDTH / 4u] +
            (uint32_t)cx;
    return (voxels[index * LEVEL_VOXEL_STRIDE + 8u] & 8u) != 0u;
}
#endif
/* FUN_00432a20 — TerrainCellAllowsPlacement. The gate is the corpus's
 * byte read at +1000: the port's flags_3e8 low byte (little-endian), bit
 * 0x4; the record-NULL case passes, matching the config-driven NULL call. */
/* Port model of 0x00432a20 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x00432a20, retires to the tail in GAME-021 part 3
int terrain_cell_allows_placement(const struct EntitySlot* e,
                                  const LevelState* s, int cx, int cy)
{
    if (e != NULL && (e->flags_3e8 & 0x04u) == 0u) {
        return 0;
    }
    if (s == NULL) s = level_current_owner_get();
    const int w = level_width(s), h = level_height(s);
    if (cx == 0 || cy == 0
        || (unsigned)cx >= (unsigned)w - 2u
        || (unsigned)cy >= (unsigned)h - 2u) {
        return 0;
    }
    const uint8_t* rec = level_voxel(s, cx, cy);
    if (rec == NULL) {
        return 0;
    }
    uint32_t flags;
    __builtin_memcpy(&flags, rec + 0x08, sizeof flags);
    const int type = (int)(signed char)rec[0x02];
    return (flags & TILE_FLAG_WALKABLE) != 0
           && (flags & 0x20u) == 0
           && type != 8 && type != 10 && type != 11;
}

/* Port model of 0x00432aa0 (GAME-021). The original-shape function in lego_unit.c
 * shares its name in the matching build (MSVC x86). */
// PORT-ONLY: port model of 0x00432aa0, retires to the tail in GAME-021 part 3
#if !(defined(_MSC_VER) && defined(_M_IX86))
uint32_t terrain_cell_occupied_or_flag_current(int cx, int cy, int mode)
{
    LevelState* level = g_active_level_retail;
    if (mode != 0) {
        int width = (int)level->word[LEVEL_OFF_WIDTH / 4];
#if defined(_WIN32) && defined(_M_IX86)
        volatile uint32_t* words = (volatile uint32_t*)level->word;
        uint8_t* voxels = (uint8_t*)(uintptr_t)
            words[LEVEL_OFF_VOXELS / 4];
#else
        uint8_t* voxels = (uint8_t*)level->ptr_voxels;
#endif
        int cell_index = width * cy + cx;
        uint8_t* record = voxels + cell_index * 0x48;
        uint32_t flags = *(uint32_t*)(record + 8);
        if ((flags & 0x400u) == 0u && (record[0x0c] & 4u) == 0u) {
            return 0u;
        }
        return 1u;
    }
    {
        int width = (int)level->word[LEVEL_OFF_WIDTH / 4];
#if defined(_WIN32) && defined(_M_IX86)
        uint8_t* voxels = (uint8_t*)(uintptr_t)
            level->word[LEVEL_OFF_VOXELS / 4];
#else
        uint8_t* voxels = (uint8_t*)level->ptr_voxels;
#endif
        int cell_index = width * cy + cx;
        uint8_t* record = voxels + cell_index * 0x48;
        uint32_t flags = *(uint32_t*)(record + 8);
        return flags & 0x400u;
    }
}
#endif

/* Port model of 0x00432b00 (GAME-021). The original-shape function in lego_unit.c
 * shares its name in the matching build (MSVC x86). */
// PORT-ONLY: port model of 0x00432b00, retires to the tail in GAME-021 part 3
#if !(defined(_MSC_VER) && defined(_M_IX86))
uint32_t terrain_cell_flag_800_bounded_current(uint32_t cx, uint32_t cy)
{
    LevelState* level = g_active_level_retail;
    uint32_t width = level->word[LEVEL_OFF_WIDTH / 4];
    if (cx < width - 1u &&
        cy < level->word[LEVEL_OFF_HEIGHT / 4] - 1u) {
        uint32_t index = cy * width + cx;
#if defined(_WIN32) && defined(_M_IX86)
        uint8_t* voxels = (uint8_t*)(uintptr_t)
            level->word[LEVEL_OFF_VOXELS / 4];
#else
        uint8_t* voxels = (uint8_t*)level->ptr_voxels;
#endif
        uint32_t flags = *(uint32_t*)(voxels + 8u + index * 0x48u);
        return flags & 0x800u;
    }
    return 0u;
}
#endif

/* Port model of 0x00432b50 (GAME-021). The original-shape function in lego_unit.c
 * shares its name in the matching build (MSVC x86). */
// PORT-ONLY: port model of 0x00432b50, retires to the tail in GAME-021 part 3
#if !(defined(_MSC_VER) && defined(_M_IX86))
uint32_t terrain_cell_flag_8_bounded_current(uint32_t cx, uint32_t cy)
{
    LevelState* level = g_active_level_retail;
    uint32_t width = level->word[LEVEL_OFF_WIDTH / 4];
    if (cx < width - 1u &&
        cy < level->word[LEVEL_OFF_HEIGHT / 4] - 1u) {
        uint32_t index = cy * width + cx;
#if defined(_WIN32) && defined(_M_IX86)
        uint8_t* voxels = (uint8_t*)(uintptr_t)
            level->word[LEVEL_OFF_VOXELS / 4];
#else
        uint8_t* voxels = (uint8_t*)level->ptr_voxels;
#endif
        uint32_t flags = *(uint32_t*)(voxels + 8u + index * 0x48u);
        return flags & 8u;
    }
    return 0u;
}
#endif

/* Port model of 0x00432b90 (GAME-021). The original-shape function in lego_unit.c
 * shares its name in the matching build (MSVC x86). */
// PORT-ONLY: port model of 0x00432b90, retires to the tail in GAME-021 part 3
#if !(defined(_MSC_VER) && defined(_M_IX86))
int terrain_cell_type_is_8_or_10_or_11_current(int cx, int cy)
{
    LevelState* level = g_active_level_retail;
    int width = (int)level->word[LEVEL_OFF_WIDTH / 4];
#if defined(_WIN32) && defined(_M_IX86)
    uint8_t* voxels = (uint8_t*)(uintptr_t)
        level->word[LEVEL_OFF_VOXELS / 4];
#else
    uint8_t* voxels = (uint8_t*)level->ptr_voxels;
#endif
    int cell_index = width * cy + cx;
    signed char type = *(signed char*)(voxels + 2 + cell_index * 0x48);
    if (type != 8 && type != 10 && type != 11) {
        return 0;
    }
    return 1;
}
#endif
/* Port model of 0x00432bd0 (GAME-021). The original-shape function in lego_unit.c
 * shares its name in the matching build (MSVC x86). */
// PORT-ONLY: port model of 0x00432bd0, retires to the tail in GAME-021 part 3
#if !(defined(_MSC_VER) && defined(_M_IX86))
int terrain_cell_walkable_flag_current(uint32_t cx, uint32_t cy)
{
    LevelState* level = g_active_level_retail;
    uint32_t width = level->word[LEVEL_OFF_WIDTH / 4];
    if (cx >= width - 1u) {
        return 0;
    }
    uint32_t height = level->word[LEVEL_OFF_HEIGHT / 4];
    if (cy >= height - 1u) {
        return 0;
    }
    uint32_t index = cy * width + cx;
#if defined(_WIN32) && defined(_M_IX86)
    uint8_t* voxels = (uint8_t*)(uintptr_t)
        level->word[LEVEL_OFF_VOXELS / 4];
#else
    uint8_t* voxels = (uint8_t*)level->ptr_voxels;
#endif
    uint32_t flags = *(uint32_t*)(voxels + 8u + index * 0x48u);
    if ((flags & 0x10u) != 0u && (flags & 8u) == 0u) {
        return 1;
    }
    return 0;
}
#endif

/* Port model of 0x00432c20 (GAME-021). The original-shape function in lego_unit.c
 * shares its name in the matching build (MSVC x86). */
// PORT-ONLY: port model of 0x00432c20, retires to the tail in GAME-021 part 3
#if !(defined(_MSC_VER) && defined(_M_IX86))
uint32_t terrain_cell_flag_wave_current(int cx, int cy)
{
    LevelState* level = g_active_level_retail;
    int width = (int)level->word[LEVEL_OFF_WIDTH / 4];
#if defined(_WIN32) && defined(_M_IX86)
    uint8_t* voxels = (uint8_t*)(uintptr_t)
        level->word[LEVEL_OFF_VOXELS / 4];
#else
    uint8_t* voxels = (uint8_t*)level->ptr_voxels;
#endif
    int cell_index = width * cy + cx;
    uint32_t flags = *(uint32_t*)(voxels + 8 + cell_index * 0x48);
    return flags & 0x40000u;
}
#endif

/* Port model of 0x00432c50 (GAME-021). The original-shape function in lego_unit.c
 * shares its name in the matching build (MSVC x86). */
// PORT-ONLY: port model of 0x00432c50, retires to the tail in GAME-021 part 3
#if !(defined(_MSC_VER) && defined(_M_IX86))
int terrain_cell_speed_class_current(const int cell_xy[2])
{
    LevelState* level = g_active_level_retail;
    int width = (int)level->word[LEVEL_OFF_WIDTH / 4];
#if defined(_WIN32) && defined(_M_IX86)
    uint8_t* voxels = (uint8_t*)(uintptr_t)
        level->word[LEVEL_OFF_VOXELS / 4];
#else
    uint8_t* voxels = (uint8_t*)level->ptr_voxels;
#endif
    int cell_index = width * cell_xy[1] + cell_xy[0];
    uint32_t flags = *(uint32_t*)(voxels + 8 + cell_index * 0x48);
    if ((flags & 0x200000u) != 0u) {
        return 0;
    }
    return (int)(flags & 3u) + 1;
}
#endif

/* Port model of 0x00432c90 (GAME-021). The original-shape function in lego_unit.c
 * shares its name in the matching build (MSVC x86). */
// PORT-ONLY: port model of 0x00432c90, retires to the tail in GAME-021 part 3
#if !(defined(_MSC_VER) && defined(_M_IX86))
int terrain_cell_state_update_current(const int cell_xy[2])
{
    LevelState* s = level_current_owner_get();
    uint8_t* rec;
    uint32_t* flags;
    uint32_t value;
    uint32_t state;
    if (s == NULL) s = (LevelState*)map_cell_record_grid_level_get();
    if (s == NULL || cell_xy == NULL) return 0;
    rec = cell_record_unchecked(s, cell_xy[0], cell_xy[1]);
    if (rec == NULL) return 0;
    flags = (uint32_t*)(rec + 0x08);
    value = *flags;
    if ((value & 0x200000u) != 0u) return 1;
    state = value & 3u;
    if (state != 0u) {
        *flags = (*flags & ~3u) | ((state - 1u) & 3u);
    }
    else {
        *flags |= 0x200000u;
        if (s_terrain_cell_linked_entry_remove_hook != NULL) {
            s_terrain_cell_linked_entry_remove_hook(
                cell_xy, s_terrain_cell_linked_entry_remove_context);
        }
        selection_mode5_cancel_at_point((const uint32_t*)cell_xy);
        coordinate_callback_dispatch_by_mode(cell_xy, 0);
        terrain_cell_state_effect_spawn(s, cell_xy);
    }
    cell_init_pass(s, (float)cell_xy[0], (uint32_t)cell_xy[1]);
    return state == 0u;
}
#endif
/* Port model of 0x00432d90 (GAME-021). The original-shape function in lego_unit.c
 * shares its name in the matching build (MSVC x86). */
// PORT-ONLY: port model of 0x00432d90, retires to the tail in GAME-021 part 3
#if !(defined(_MSC_VER) && defined(_M_IX86))
uint32_t terrain_cell_flag_20_bounded_current(uint32_t cx, uint32_t cy)
{
    LevelState* level = g_active_level_retail;
    uint32_t width = level->word[LEVEL_OFF_WIDTH / 4];
    if (cx < width - 1u &&
        cy < level->word[LEVEL_OFF_HEIGHT / 4] - 1u) {
        uint32_t index = cy * width + cx;
#if defined(_WIN32) && defined(_M_IX86)
        uint8_t* voxels = (uint8_t*)(uintptr_t)
            level->word[LEVEL_OFF_VOXELS / 4];
#else
        uint8_t* voxels = (uint8_t*)level->ptr_voxels;
#endif
        uint32_t flags = *(uint32_t*)(voxels + 8u + index * 0x48u);
        return flags & 0x20u;
    }
    return 0u;
}
#endif

/* Port model of 0x00432dd0 (GAME-021). The original-shape function in lego_unit.c
 * shares its name in the matching build (MSVC x86). */
// PORT-ONLY: port model of 0x00432dd0, retires to the tail in GAME-021 part 3
#if !(defined(_MSC_VER) && defined(_M_IX86))
uint32_t terrain_cell_flag_reserved_current(const int cell_xy[2])
{
    LevelState* level = g_active_level_retail;
    int width = (int)level->word[LEVEL_OFF_WIDTH / 4];
    uint8_t* voxels = (uint8_t*)level->ptr_voxels;
    int cell_index = cell_xy[1] * width + cell_xy[0];
    uint32_t flags = *(uint32_t*)(voxels + 8 + cell_index * 0x48);
    return flags & 0x8000000u;
}
#endif
/* Retail ABI: cell-coordinate pair plus the set/clear selector. */
/* Port model of 0x00432e00 (GAME-021). The original-shape function in lego_unit.c
 * shares its name in the matching build (MSVC x86). */
// PORT-ONLY: port model of 0x00432e00, retires to the tail in GAME-021 part 3
#if !(defined(_MSC_VER) && defined(_M_IX86))
void level_tile_mark_current_target(const int cell_xy[2], int set)
{
    LevelState* level = g_active_level_retail;
    if (set != 0) {
        int width = (int)level->word[LEVEL_OFF_WIDTH / 4];
#if defined(_WIN32) && defined(_M_IX86)
        uint8_t* voxels = (uint8_t*)(uintptr_t)
            level->word[LEVEL_OFF_VOXELS / 4];
#else
        uint8_t* voxels = (uint8_t*)level->ptr_voxels;
#endif
        int cell_index = width * cell_xy[1] + cell_xy[0];
        uint32_t* flags = (uint32_t*)(voxels + 8 + cell_index * 0x48);
        *flags |= 0x8000000u;
        return;
    }
    {
        int width = (int)level->word[LEVEL_OFF_WIDTH / 4];
#if defined(_WIN32) && defined(_M_IX86)
        uint8_t* voxels = (uint8_t*)(uintptr_t)
            level->word[LEVEL_OFF_VOXELS / 4];
#else
        uint8_t* voxels = (uint8_t*)level->ptr_voxels;
#endif
        int cell_index = width * cell_xy[1] + cell_xy[0];
        uint32_t* flags = (uint32_t*)(voxels + 8 + cell_index * 0x48);
        *flags &= 0xf7ffffffu;
    }
}
#endif

/* Retail target reads the active level and normalizes either flag to 1. */
/* Port model of 0x00432e60 (GAME-021). The original-shape function in lego_unit.c
 * shares its name in the matching build (MSVC x86). */
// PORT-ONLY: port model of 0x00432e60, retires to the tail in GAME-021 part 3
#if !(defined(_MSC_VER) && defined(_M_IX86))
uint32_t terrain_cell_mask_c0_current(int cx, int cy)
{
    LevelState* level = g_active_level_retail;
    int width = (int)level->word[LEVEL_OFF_WIDTH / 4];
#if defined(_WIN32) && defined(_M_IX86)
    uint8_t* voxels = (uint8_t*)(uintptr_t)
        level->word[LEVEL_OFF_VOXELS / 4];
#else
    uint8_t* voxels = (uint8_t*)level->ptr_voxels;
#endif
    int cell_index = width * cy + cx;
    uint8_t flags = voxels[8 + cell_index * 0x48];
    return (flags & 0xc0u) != 0u ? 1u : 0u;
}
#endif

/* Port model of 0x00432e90 (GAME-021). The original-shape function in lego_unit.c
 * shares its name in the matching build (MSVC x86). */
// PORT-ONLY: port model of 0x00432e90, retires to the tail in GAME-021 part 3
#if !(defined(_MSC_VER) && defined(_M_IX86))
uint32_t terrain_cell_edge_wall_current(const int cell_xy[2])
{
    LevelState* level = g_active_level_retail;
    int width = (int)level->word[LEVEL_OFF_WIDTH / 4];
    uint8_t* voxels = (uint8_t*)level->ptr_voxels;
    int cell_index = cell_xy[1] * width + cell_xy[0];
    uint32_t flags = *(uint32_t*)(voxels + 8 + cell_index * 0x48);
    return flags & 0x4000000u;
}
#endif

/* Retail predicates use the current level and one coordinate-pair argument. */
/* Port model of 0x00432ec0 (GAME-021). The original-shape function in lego_unit.c
 * shares its name in the matching build (MSVC x86). */
// PORT-ONLY: port model of 0x00432ec0, retires to the tail in GAME-021 part 3
#if !(defined(_MSC_VER) && defined(_M_IX86))
int terrain_cell_state_equals_one_current(const int cell_xy[2])
{
    LevelState* level = g_active_level_retail;
    int width = (int)level->word[LEVEL_OFF_WIDTH / 4];
#if defined(_WIN32) && defined(_M_IX86)
    uint8_t* voxels = (uint8_t*)(uintptr_t)
        level->word[LEVEL_OFF_VOXELS / 4];
#else
    uint8_t* voxels = (uint8_t*)level->ptr_voxels;
#endif
    int cell_index = cell_xy[1] * width + cell_xy[0];
    return voxels[2 + cell_index * 0x48] == 1;
}
#endif

/* Port model of 0x00432f00 (GAME-021). The original-shape function in lego_unit.c
 * shares its name in the matching build (MSVC x86). */
// PORT-ONLY: port model of 0x00432f00, retires to the tail in GAME-021 part 3
#if !(defined(_MSC_VER) && defined(_M_IX86))
int terrain_cell_state_equals_six_current(const int cell_xy[2])
{
    LevelState* level = g_active_level_retail;
    int width = (int)level->word[LEVEL_OFF_WIDTH / 4];
#if defined(_WIN32) && defined(_M_IX86)
    uint8_t* voxels = (uint8_t*)(uintptr_t)
        level->word[LEVEL_OFF_VOXELS / 4];
#else
    uint8_t* voxels = (uint8_t*)level->ptr_voxels;
#endif
    int cell_index = cell_xy[1] * width + cell_xy[0];
    return voxels[2 + cell_index * 0x48] == 6;
}
#endif

/* Port model of 0x00432f40 (GAME-021). The original-shape function in lego_unit.c
 * shares its name in the matching build (MSVC x86). */
// PORT-ONLY: port model of 0x00432f40, retires to the tail in GAME-021 part 3
#if !(defined(_MSC_VER) && defined(_M_IX86))
int terrain_cell_mask18_clear_current(uint32_t cx, uint32_t cy)
{
    LevelState* level = terrain_current_level_resolve();
    uint32_t width;
    uint32_t height;
    const uint8_t* voxels;
    uint32_t index;
    uint32_t flags;
    if (level == NULL) return 1;
    width = level->word[LEVEL_OFF_WIDTH / 4u];
    height = level->word[LEVEL_OFF_HEIGHT / 4u];
    if (cx >= width - 1u || cy >= height - 1u) return 1;
    voxels = (const uint8_t*)level->ptr_voxels;
    if (voxels == NULL) return 1;
    index = cy * width + cx;
    __builtin_memcpy(&flags, voxels + 8u + index * 0x48u, sizeof(flags));
    return (flags & 0x18u) == 0u;
}
#endif

/* TerrainCellFlag4TestInBounds (FUN_00432f90) reads the active level. */
/* Port model of 0x00432f90 (GAME-021). The original-shape function in lego_unit.c
 * shares its name in the matching build (MSVC x86). */
// PORT-ONLY: port model of 0x00432f90, retires to the tail in GAME-021 part 3
#if !(defined(_MSC_VER) && defined(_M_IX86))
uint32_t level_cell_flag_4_current(uint32_t cx, uint32_t cy)
{
    const LevelState* s;
    int w;
    int h;
    const uint8_t* voxels;
    uint32_t index;
    uint32_t flags;
    /* The target reads DAT_005560d4 directly. The map-cell registry is the
     * same retained current-level owner when no GameContext is installed. */
    s = terrain_current_level_resolve();
    if (s == NULL) return 0u;
    w = level_int(s, LEVEL_OFF_WIDTH);
    h = level_int(s, LEVEL_OFF_HEIGHT);
    if (cx >= ((uint32_t)w - 1u) ||
        cy >= ((uint32_t)h - 1u)) {
        return 0u;
    }
    voxels = (const uint8_t*)level_load_ptr(s, LEVEL_OFF_VOXELS);
    if (voxels == NULL) return 0u;
    index = (uint32_t)cy * (uint32_t)w + (uint32_t)cx;
    flags = *(const uint32_t*)(voxels + index * LEVEL_VOXEL_STRIDE + 8u);
    return flags & 4u;
}
#endif

/* Port model of 0x00432fd0 (GAME-021). The original-shape function in lego_unit.c
 * shares its name in the matching build (MSVC x86). */
// PORT-ONLY: port model of 0x00432fd0, retires to the tail in GAME-021 part 3
#if !(defined(_MSC_VER) && defined(_M_IX86))
uint32_t terrain_cell_flag_2000_current(int cx, int cy)
{
#if defined(_WIN32) && defined(_M_IX86)
    LevelState* level = g_active_level_retail;
    int width = (int)level->word[LEVEL_OFF_WIDTH / 4];
    uint8_t* voxels = (uint8_t*)(uintptr_t)
        level->word[LEVEL_OFF_VOXELS / 4];
    int index = width * cy + cx;
    uint32_t flags = *(uint32_t*)(voxels + 8 + index * 0x48);
    return flags & 0x2000u;
#else
    LevelState* level = terrain_current_level_resolve();
    const uint8_t* voxels;
    uint32_t index;
    uint32_t flags;
    if (level == NULL) return 0u;
    voxels = (const uint8_t*)level_load_ptr(level, LEVEL_OFF_VOXELS);
    if (voxels == NULL) return 0u;
    index = (uint32_t)cy * (uint32_t)level_int(level, LEVEL_OFF_WIDTH) +
            (uint32_t)cx;
    __builtin_memcpy(&flags,
        voxels + index * (uint32_t)LEVEL_VOXEL_STRIDE + 8u,
        sizeof(flags));
    return flags & 0x2000u;
#endif
}
#endif

/* Port model of 0x00433000 (GAME-021). The original-shape function in lego_unit.c
 * shares its name in the matching build (MSVC x86). */
// PORT-ONLY: port model of 0x00433000, retires to the tail in GAME-021 part 3
#if !(defined(_MSC_VER) && defined(_M_IX86))
uint32_t terrain_cell_flag_80_current(int cx, int cy)
{
#if defined(_WIN32) && defined(_M_IX86)
    LevelState* level = g_active_level_retail;
    int width = (int)level->word[LEVEL_OFF_WIDTH / 4];
    uint8_t* voxels = (uint8_t*)(uintptr_t)
        level->word[LEVEL_OFF_VOXELS / 4];
    int index = width * cy + cx;
    uint32_t flags = *(uint32_t*)(voxels + 8 + index * 0x48);
    return flags & 0x80u;
#else
    LevelState* level = terrain_current_level_resolve();
    const uint8_t* voxels;
    uint32_t index;
    uint32_t flags;
    if (level == NULL) return 0u;
    voxels = (const uint8_t*)level_load_ptr(level, LEVEL_OFF_VOXELS);
    if (voxels == NULL) return 0u;
    index = (uint32_t)cy * (uint32_t)level_int(level, LEVEL_OFF_WIDTH) +
            (uint32_t)cx;
    __builtin_memcpy(&flags,
        voxels + index * (uint32_t)LEVEL_VOXEL_STRIDE + 8u,
        sizeof(flags));
    return flags & 0x80u;
#endif
}
#endif

/* Retail pair-pointer ABI; this target is an unchecked masked-word read. */
/* Port model of 0x00433030 (GAME-021). The original-shape function in lego_unit.c
 * shares its name in the matching build (MSVC x86). */
// PORT-ONLY: port model of 0x00433030, retires to the tail in GAME-021 part 3
#if !(defined(_MSC_VER) && defined(_M_IX86))
uint32_t terrain_cell_flag_8000_current(const int cell_xy[2])
{
    LevelState* level = level_current_owner_get();
    if (cell_xy == NULL || level == NULL) return 0u;
    return cell_flag_word_unchecked(level, cell_xy[0], cell_xy[1]) & 0x8000u;
}
#endif

/* Port model of 0x00433060 (GAME-021). The original-shape function in lego_unit.c
 * shares its name in the matching build (MSVC x86). */
// PORT-ONLY: port model of 0x00433060, retires to the tail in GAME-021 part 3
#if !(defined(_MSC_VER) && defined(_M_IX86))
uint32_t terrain_cell_secondary_bit0_current(const int cell_xy[2])
{
    LevelState* level = level_current_owner_get();
    if (cell_xy == NULL || level == NULL) return 0u;
    return cell_secondary_word_unchecked(level, cell_xy[0], cell_xy[1]) & 1u;
}
#endif

/* TerrainCoordinateAndOccupancyCheck (FUN_00433090) uses the current level
 * global and returns the masked flag word (not a normalized boolean). */
/* Port model of 0x00433090 (GAME-021). The original-shape function in lego_unit.c
 * shares its name in the matching build (MSVC x86). */
// PORT-ONLY: port model of 0x00433090, retires to the tail in GAME-021 part 3
#if !(defined(_MSC_VER) && defined(_M_IX86))
uint32_t level_cell_occupied_at_current(const int cell_xy[2])
{
    LevelState* level;
    int x;
    int y;
    if (cell_xy == NULL) return 0u;
    /* FUN_00433090 reads the process-wide active level directly. The portable
     * owner may be the map-cell registry when no entity-pool GameContext is
     * installed, so use the same current-owner resolution as the other
     * coordinate-pair flag readers. */
    level = level_current_owner_get();
    if (level == NULL) return 0u;
    x = cell_xy[0];
    y = cell_xy[1];
    return cell_flag_word(level, x, y) & 0x20000000u;
}
#endif

/* Port model of 0x004330e0 (GAME-021). The original-shape function in lego_unit.c
 * shares its name in the matching build (MSVC x86). */
// PORT-ONLY: port model of 0x004330e0, retires to the tail in GAME-021 part 3
#if !(defined(_MSC_VER) && defined(_M_IX86))
int terrain_cell_mask_180000_current(const int cell_xy[2])
{
    LevelState* level = level_current_owner_get();
    if (cell_xy == NULL || level == NULL) return 0;
    return (cell_flag_word_unchecked(level, cell_xy[0], cell_xy[1]) &
            0x180000u) != 0u;
}
#endif

/* Port model of 0x00433120 (GAME-021). The original-shape function in lego_unit.c
 * shares its name in the matching build (MSVC x86). */
// PORT-ONLY: port model of 0x00433120, retires to the tail in GAME-021 part 3
#if !(defined(_MSC_VER) && defined(_M_IX86))
void terrain_cell_flag_10000000_set_current(const int cell_xy[2], int set)
{
    LevelState* level = level_current_owner_get();
    if (level != NULL && cell_xy != NULL) {
        level_cell_flag_10000000_set(level, cell_xy[0], cell_xy[1], set);
    }
}
#endif

/* Port model of 0x00433180 (GAME-021). The original-shape function in lego_unit.c
 * shares its name in the matching build (MSVC x86). */
// PORT-ONLY: port model of 0x00433180, retires to the tail in GAME-021 part 3
#if !(defined(_MSC_VER) && defined(_M_IX86))
int entity_cell_type_speed_multiplier_resolve_current(
    const EntitySlot* entity, int x, int y, float scale, float* out_scale)
{
    LevelState* level = g_active_level_retail;
    if (entity->type == 6 || entity->type == 7 || entity->type == 5 ||
        entity->type == 8 || entity->type == 11 || entity->type == 13 ||
        entity->type == 14 || entity->type == 15 || entity->type == 16 ||
        entity->type == 17 || entity->type == 18 || entity->type == 19) {
        return 0;
    }
    if (entity->owner_36c != NULL &&
        (entity_stats_word_get(entity) & 0x20000u) == 0u) return 0;

    {
        int width = (int)level->word[LEVEL_OFF_WIDTH / 4];
#if defined(_WIN32) && defined(_M_IX86)
        uint8_t* voxels = (uint8_t*)(uintptr_t)
            level->word[LEVEL_OFF_VOXELS / 4];
#else
        uint8_t* voxels = (uint8_t*)level->ptr_voxels;
#endif
        int index = width * y + x;
        uint8_t* cell = voxels + index * 0x48;
        if (cell[2] == 6) {
            uint32_t cell_flags = *(uint32_t*)(cell + 8);
            if ((cell_flags & 0x200u) == 0u &&
                (entity_state_flags_get(entity) & 0x200000u) == 0u) {
                if (out_scale == NULL) return 1;
                *out_scale = scale;
                return 1;
            }
        }
        if ((*(uint32_t*)(cell + 8) & 0x1000u) != 0u) {
            if (out_scale != NULL) {
                uint32_t rock_fall_type =
                    lego::object_type_name_index_find("RockFallIn");
                float motion_scale =
                    lego::entity_indexed_float_lookup_with_minus_one_fallback(
                        (int)rock_fall_type, entity);
                *out_scale = motion_scale * scale;
            }
            return 1;
        }
    }
    return 0;
}
#endif

/* Port model of 0x004332c0 (GAME-021). The original-shape function in lego_unit.c
 * shares its name in the matching build (MSVC x86). */
// PORT-ONLY: port model of 0x004332c0, retires to the tail in GAME-021 part 3
#if !(defined(_MSC_VER) && defined(_M_IX86))
uint32_t terrain_cell_record_byte3_current(int x, int y)
{
    LevelState* level = level_current_owner_get();
    const uint8_t* rec;
    if (level == NULL) return 0u;
    rec = cell_record_unchecked_const(level, x, y);
    return rec != NULL ? rec[3] : 0u;
}
#endif

/* TerrainCellWorkFlagSet @ 0x004332f0 reads the current level global and
 * raises bit 2 only for coordinates strictly inside the final row/column. */
/* Port model of 0x004332f0 (GAME-021). The original-shape function in lego_unit.c
 * shares its name in the matching build (MSVC x86). */
// PORT-ONLY: port model of 0x004332f0, retires to the tail in GAME-021 part 3
#if !(defined(_MSC_VER) && defined(_M_IX86))
void terrain_cell_work_flag_set_current(uint32_t cx, uint32_t cy)
{
    GameContext* game = entity_pool_callback_context_get();
    LevelState* s = game != NULL ? game->level : NULL;
    uint32_t width, height;
    uint8_t* rec;
    uint32_t* flags;
    /* FUN_00431560 is the authoritative current-level source; the map-cell
     * registry retains that owner when no entity-pool context is active. */
    if (s == NULL) s = (LevelState*)map_cell_record_grid_level_get();
    if (s == NULL) return;
    width = (uint32_t)level_int(s, LEVEL_OFF_WIDTH);
    height = (uint32_t)level_int(s, LEVEL_OFF_HEIGHT);
    if (cx < width - 1u && cy < height - 1u) {
        rec = (uint8_t*)level_load_ptr(s, LEVEL_OFF_VOXELS)
            + ((size_t)cy * (size_t)width + (size_t)cx)
                * LEVEL_VOXEL_STRIDE;
        flags = (uint32_t*)(rec + 0x08);
        *flags = *flags | 4u;
    }
}
#endif

/* Port model of 0x00433330 (GAME-021). The original-shape function in lego_unit.c
 * shares its name in the matching build (MSVC x86). */
// PORT-ONLY: port model of 0x00433330, retires to the tail in GAME-021 part 3
#if !(defined(_MSC_VER) && defined(_M_IX86))
int terrain_cell_type_get_at_current(uint32_t cx, uint32_t cy,
                                     uint32_t* out_type)
{
    LevelState* level = level_current_owner_get();
    const int width = level_int(level, LEVEL_OFF_WIDTH);
    const int height = level_int(level, LEVEL_OFF_HEIGHT);
    if (cx < ((unsigned)width - 1u) && cy < ((unsigned)height - 1u)) {
        const uint8_t* record = level_voxel(level, (int)cx, (int)cy);
        if (record != NULL) *out_type = (uint32_t)record[0x02];
        return 1;
    }
    return 0;
}
#endif

/* TerrainCellMutationProgress has the retail level/x/y ABI. It samples and
 * derives its separate SceneRender quad without writing back into the base. */
/* Port model of 0x00433380 (GAME-021) for the targets other than MSVC x86; on MSVC
 * x86 the shared original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x00433380 off MSVC x86, retires to the tail in GAME-021 part 3
#if !defined(_MSC_VER) || !defined(_M_IX86)
void terrain_cell_mutation_progress(LevelState* level, uint32_t x,
                                    uint32_t y)
{
    TerrainSurfaceGrid* surface = level_surface_grid_get(level);
    if (level == NULL || x > 0x7fffffffu || y > 0x7fffffffu) return;
    (void)terrain_cell_mutation_progress_level(level, surface,
                                               (int)x, (int)y);
}
#endif
/* FUN_004334c0 (level_map_dimensions_get) is shared: lego_unit.c defines it. */

/* Port model of 0x00433770 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x00433770, retires to the tail in GAME-021 part 3
int scene_lights_build_from_config(const SceneLightBuildOps* ops)
{
    SceneLightDefinition definition;
    void* top_spot;
    void* track_spot;
    void* ambient;
    void* first_person;
    void* rotated_first_person = NULL;
    void* red_auxiliary;
    void* root;
    void* default_owner;
    void* default_light;
    float pose[6];
    float position[3];
    float rgb[3];
    float scale[3];

    if (ops == NULL || ops->rgb_read == NULL || ops->owner_get == NULL ||
        ops->light_create == NULL || ops->light_parameter_set == NULL ||
        ops->light_position_set == NULL || ops->light_pose_set == NULL ||
        ops->light_enable == NULL || ops->scene_root_get == NULL ||
        ops->scene_object_allocate == NULL ||
        ops->scene_object_transform_init == NULL) {
        return 0;
    }

    /* TopSpotRGB: config lookup and light creation are required. */
    definition.config_name = "topspotrgb";
    definition.owner_slot = SCENE_LIGHT_OWNER_TOP_SPOT;
    definition.owner = ops->owner_get(definition.owner_slot, ops->user);
    definition.light_kind = 2;
    if (!ops->rgb_read(definition.config_name, definition.rgb, ops->user))
        return 0;
    top_spot = ops->light_create(&definition, ops->user);
    if (top_spot == NULL) return 0;
    (void)ops->light_parameter_set(top_spot, 0x40u, 0.85f, ops->user);
    (void)ops->light_parameter_set(top_spot, 0x3cu, 0.4f, ops->user);
    position[0] = 200.0f; position[1] = 140.0f; position[2] = -130.0f;
    (void)ops->light_position_set(top_spot, definition.owner, position,
                                  ops->user);
    pose[0] = -1.0f; pose[1] = -0.8f; pose[2] = 0.75f;
    pose[3] = 0.0f; pose[4] = 1.0f; pose[5] = 0.0f;
    (void)ops->light_pose_set(top_spot, definition.owner, pose, ops->user);

    /* TrackSpotRGB: required config/light, then position and pose. */
    definition.config_name = "trackspotrgb";
    definition.owner_slot = SCENE_LIGHT_OWNER_TRACK_SPOT;
    definition.owner = ops->owner_get(definition.owner_slot, ops->user);
    definition.light_kind = 2;
    if (!ops->rgb_read(definition.config_name, definition.rgb, ops->user))
        return 0;
    track_spot = ops->light_create(&definition, ops->user);
    if (track_spot == NULL) return 0;
    position[0] = 0.0f; position[1] = 0.0f; position[2] = -150.0f;
    (void)ops->light_position_set(track_spot, NULL, position, ops->user);
    pose[0] = 0.0f; pose[1] = 0.0f; pose[2] = 1.0f;
    pose[3] = 0.0f; pose[4] = 1.0f; pose[5] = 0.0f;
    (void)ops->light_pose_set(track_spot, NULL, pose, ops->user);

    /* AmbientRGB and FPLightRGB are required, with no post-create setters. */
    definition.config_name = "ambientrgb";
    definition.owner_slot = SCENE_LIGHT_OWNER_AMBIENT;
    definition.owner = ops->owner_get(definition.owner_slot, ops->user);
    definition.light_kind = 0;
    if (!ops->rgb_read(definition.config_name, definition.rgb, ops->user))
        return 0;
    ambient = ops->light_create(&definition, ops->user);
    if (ambient == NULL) return 0;

    definition.config_name = "fplightrgb";
    definition.owner_slot = SCENE_LIGHT_OWNER_FIRST_PERSON;
    definition.owner = ops->owner_get(definition.owner_slot, ops->user);
    definition.light_kind = 1;
    if (!ops->rgb_read(definition.config_name, definition.rgb, ops->user))
        return 0;
    first_person = ops->light_create(&definition, ops->user);
    if (first_person == NULL) return 0;

    /* FPRotLightRGB is the only optional config light. */
    definition.config_name = "fprotlightrgb";
    definition.light_kind = 3;
    if (ops->rgb_read(definition.config_name, definition.rgb, ops->user))
        rotated_first_person = ops->light_create(&definition, ops->user);

    /* The auxiliary red type-3 light is attempted regardless of the
     * optional rotated light and is not a required-success gate in retail. */
    definition.config_name = NULL;
    definition.rgb[0] = 1.0f;
    definition.rgb[1] = 0.0f;
    definition.rgb[2] = 0.0f;
    red_auxiliary = ops->light_create(&definition, ops->user);

    /* Retail creates a scene object under the current root, then places a
     * default white type-2 light on that new object. */
    root = ops->scene_root_get(ops->user);
    default_owner = ops->scene_object_allocate(root, ops->user);
    definition.config_name = NULL;
    definition.owner_slot = SCENE_LIGHT_OWNER_DEFAULT_ROOT;
    definition.owner = default_owner;
    definition.light_kind = 2;
    definition.rgb[0] = 1.0f;
    definition.rgb[1] = 1.0f;
    definition.rgb[2] = 1.0f;
    default_light = ops->light_create(&definition, ops->user);
    if (default_light == NULL) return 0;
    scale[0] = 0.8f; scale[1] = 0.8f; scale[2] = 0.8f;
    (void)ops->scene_object_transform_init(default_light, root, scale,
                                           ops->user);
    pose[0] = 0.0f; pose[1] = 0.0f; pose[2] = 1.0f;
    pose[3] = 0.0f; pose[4] = 1.0f; pose[5] = 0.0f;
    (void)ops->light_pose_set(default_light, NULL, pose, ops->user);
    (void)ops->light_parameter_set(default_light, 0x40u, 0.85f, ops->user);
    (void)ops->light_parameter_set(default_light, 0x3cu, 0.0f, ops->user);

    /* Retail enables in this order, then assigns the three far ranges. */
    (void)ops->light_enable(first_person, 1, ops->user);
    if (rotated_first_person != NULL)
        (void)ops->light_enable(rotated_first_person, 1, ops->user);
    (void)ops->light_enable(top_spot, 1, ops->user);
    (void)ops->light_enable(track_spot, 1, ops->user);
    (void)ops->light_enable(ambient, 1, ops->user);
    (void)ops->light_enable(default_light, 1, ops->user);
    (void)ops->light_enable(red_auxiliary, 1, ops->user);
    (void)ops->light_parameter_set(top_spot, 0x38u, 2000.0f, ops->user);
    (void)ops->light_parameter_set(track_spot, 0x38u, 2000.0f, ops->user);
    (void)ops->light_parameter_set(default_light, 0x38u, 2000.0f, ops->user);
    return 1;
}

/* CurrentLevelTerrainGridPick (FUN_00433be0) — see view.hpp. */
/* Port model of 0x00433be0 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x00433be0, retires to the tail in GAME-021 part 3
void current_level_terrain_grid_pick(EngineContext* ctx, float x, float y,
                                     int* gx, int* gy)
{
    const float pos[2] = { x, y };
    int tx = 0, ty = 0;
    int* px = gx != NULL ? gx : &tx;
    int* py = gy != NULL ? gy : &ty;
    (void)engine_context_grid_pick(ctx, pos, px, py);
}

/* EntityNeighborBlockedCellStepProbe (FUN_00433c10): the retail ABI is an
 * entity, a float cadence divisor, and a mode word. The first branch tests
 * the four adjacent voxel flag words and raises the route-step response on
 * the first cell with bit 0x10 set and bit 8 clear. */
/* Port model of 0x00433c10 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x00433c10, retires to the tail in GAME-021 part 3
int entity_neighbor_blocked_cell_step_probe(EntitySlot* entity,
                                             float cadence, int mode)
{
    static const int offset[4][2] = {
        { -1, 0 }, { 1, 0 }, { 0, -1 }, { 0, 1 }
    };
    GameContext* game;
    LevelState* level;
    int cell_x = -1;
    int cell_y = -1;
    uint32_t state_flags;
    int i;
    if (entity == NULL) return 0;
    state_flags = entity_state_flags_get(entity);
    game = entity_pool_callback_context_get();
    level = game != NULL ? game->level : NULL;
    if (level == NULL)
        level = (LevelState*)map_cell_record_grid_level_get();
    if (level == NULL) return 0;

    if ((state_flags & 0x1000u) != 0u &&
        (entity->flags_3e0 & 1u) == 0u &&
        !entity_flags_mask107c33de_or_1c00203_any_test(entity) &&
        cadence != 0.0f) {
        uint32_t divisor = (uint32_t)(int)cadence;
        if (divisor != 0u &&
            (uint32_t)level_random_draw15() % divisor == 0u) {
            entity_position_to_terrain_cell(entity, level, &cell_x, &cell_y);
            if (!level_coord_pair_within_bounds(level, cell_x, cell_y))
                return 0;
            for (i = 0; i < 4; ++i) {
                int x = cell_x + offset[i][0];
                int y = cell_y + offset[i][1];
                uint8_t* cell;
                uint32_t flags;
                if (x < 0 || y < 0 ||
                    (uint32_t)x >= level->word[5] - 1u ||
                    (uint32_t)y >= level->word[6] - 1u) continue;
                cell = level_voxel(level, x, y);
                if (cell == NULL) continue;
                memcpy(&flags, cell + 8, sizeof(flags));
                if ((flags & 0x10u) != 0u && (flags & 8u) == 0u) {
                    entity->field_340 = -1.0f;
                    unported_ledger_stubs::
                        ledger_stub_00449700_RouteStepCommandIssue(entity);
                    return 1;
                }
            }
        }
    }

    /* The other retail branch calls EntityRandomCellStepRouteIssue after its
     * own cadence gate. Its route search needs the renderer-owned surface
     * sampler and path-marker allocator, neither of which has a live adapter
     * in this module; leave that action inert until those owners are bound. */
    if ((mode != 0 || (state_flags & 0x400u) != 0u) &&
        (entity->flags_3e0 & 1u) == 0u &&
        !entity_flags_mask107c33de_or_1c00203_any_test(entity) &&
        cadence != 0.0f) {
        uint32_t divisor = (uint32_t)(int)cadence;
        if (mode != 0 || (divisor != 0u &&
                          (uint32_t)level_random_draw15() % divisor == 0u)) {
            /* See the dependency note above. */
        }
    }
    return 0;
}

/* Port model of 0x00433e30 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x00433e30, retires to the tail in GAME-021 part 3
int grid_cell_entity_eligibility_check(int x, int y)
{
    GridCellEligibilityHooks* hooks = &s_grid_cell_eligibility_hooks;
    GameContext* game = entity_pool_callback_context_get();
    LevelState* level = game != NULL ? game->level : NULL;
    uint32_t flags = 0;
    if (level == NULL)
        level = (LevelState*)map_cell_record_grid_level_get();
    if (hooks->cell_flags_get != NULL) {
        flags = hooks->cell_flags_get(x, y, hooks->context);
    }
    else {
        const uint8_t* cell;
        if (level == NULL) return 0;
        cell = level_voxel(level, x, y);
        if (cell == NULL) return 0;
        memcpy(&flags, cell + 8, sizeof(flags));
    }
    if ((flags & 0x100u) != 0u) return 0;
    if (hooks->surface_entity_eligible != NULL)
        return hooks->surface_entity_eligible(x, y, hooks->context) != 0;
    return level != NULL && grid_cell_detail_enabled_get(
        level->ptr_terrain, x, y) != 0;
}

/* CellSurfaceUpdateDispatch (FUN_00434480): resolve the packed surface's
 * per-level tile-model record, sample the live four-corner geometry, then
 * run the two persistent visual channels through the scene resource API. */
/* Port model of 0x00434480 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x00434480, retires to the tail in GAME-021 part 3
int terrain_cell_surface_update_dispatch_level(LevelState* level,
                                                int x, int y,
                                                float scale,
                                                float uniform_scale)
{
    uint8_t* cell;
    uint32_t flags;
    TerrainTileMeshGrid* tile_grid;
    SceneCellVisualRelayRecord* visual;
    TerrainSurfaceGrid* surface;
    float vertices[12];
    if (level == NULL) return 0;
    cell = level_voxel(level, x, y);
    if (cell == NULL) return 0;
    memcpy(&flags, cell + 8, sizeof(flags));
    if ((flags & 0x18u) == 0u) return 0;
    tile_grid = level->ptr_tile_mesh_grid;
    visual = terrain_tile_mesh_visual_channels_get(tile_grid, cell[1]);
    if (visual == NULL) return 0;
    surface = level_surface_grid_get(level);
    if (surface == NULL || surface->surface_vectors == NULL || x < 0 ||
        y < 0 || (uint32_t)x >= surface->width ||
        (uint32_t)y >= surface->height) return 0;
    /* FUN_004500d0 returns the cell's diagonal orientation, not a success
     * flag: zero is the normal (unflipped) cell and still has valid vertices. */
    (void)terrain_cell_surface_vertices_get(surface, (uint32_t)x,
                                             (uint32_t)y, vertices);
    /* The record's lookup pointer is the +0x7c texture-variant owner passed
     * by TextureSetConfigLoad into TileModelEntityRegister. */
    if (visual->lookup_table == NULL &&
        tile_grid->visual_lookup_table != NULL)
        visual->lookup_table = (SceneCellVisualLookupTable*)
            tile_grid->visual_lookup_table;
    scene_cell_visual_channels_apply(visual, vertices, cell[1], cell[3],
                                     scale, uniform_scale);
    return 1;
}

/* TerrainGridEntryResourcesReleaseAll (FUN_00434530): the retail level's
 * +0x80 pointer array contains visual-channel records. Clear their two
 * 100-entry resource channels and reset +0x32c; retain the records and roots. */
/* Port model of 0x00434530 (GAME-021) for the targets other than MSVC x86; on MSVC
 * x86 the shared original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x00434530 off MSVC x86, retires to the tail in GAME-021 part 3
#if !defined(_MSC_VER) || !defined(_M_IX86)
void terrain_grid_entry_resources_release_all_current(void)
{
    GameContext* game = entity_pool_callback_context_get();
    LevelState* level = game != NULL ? game->level : NULL;
    TerrainTileMeshGrid* grid;
    size_t count;
    size_t i;
    if (level == NULL)
        level = (LevelState*)map_cell_record_grid_level_get();
    if (level == NULL) return;
    grid = level->ptr_tile_mesh_grid;
    if (grid == NULL || grid->meshes == NULL) return;
    if (grid->height != 0u && (size_t)grid->width >
        ((size_t)-1) / (size_t)grid->height) return;
    count = (size_t)grid->width * (size_t)grid->height;
    for (i = 0u; i < count; ++i) {
        SceneCellVisualRelayRecord* visual = grid->meshes[i];
        uint32_t channel;
        uint32_t item;
        if (visual == NULL) continue;
        visual->channel_cursor = 0u;
        for (channel = 0u; channel < 2u; ++channel) {
            for (item = 0u; item < 100u; ++item) {
                void* resource = visual->channel_resources[channel][item];
                if (resource != NULL) ddiscene_object_destroy(resource);
                visual->channel_resources[channel][item] = NULL;
            }
        }
    }
}
#endif

/* Port model of 0x00434570 (GAME-021); the matching build uses lego_unit.c's
 * (it reads the level's pointer words). */
#if !defined(_MSC_VER) || !defined(_M_IX86)
// PORT-ONLY: port model of 0x00434570, retires to the tail in GAME-021 part 3
int terrain_cell_drill_work_add_current(int x, int y, float divisor,
                                        float elapsed)
{
    if (divisor == 0.0f || divisor != divisor) return 0;
    return terrain_cell_drill_work_add(level_current_owner_get(), x, y,
                                       divisor, elapsed);
}
#endif

namespace frontend_config {
/* Port model of 0x004345f0 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x004345f0, retires to the tail in GAME-021 part 3
void panels_config_entry_apply(BlockFile* config, int display_width,
                               int display_height)
{
    char path[256];
    BlockRec* section;
    BlockRec* entry;
    if (config == NULL) return;
    sprintf(path, "Panels%ix%i", display_width, display_height);
    section = block_subgroup(config,
        string_path_join(util_root_name(config), path, (const char*)NULL));
    if (section == NULL) return;
    for (entry = section; entry != NULL;
         entry = block_next_sibling(entry)) {
        int panel;
        const char* value = block_value(entry);
        char copy[256];
        char* fields[6];
        int count;
        if (!ui_panel_name_table_lookup(block_name(entry), &panel) ||
            value == NULL) continue;
        strncpy(copy, value, sizeof(copy) - 1u);
        copy[sizeof(copy) - 1u] = '\0';
        count = ledger_config_split_limited(copy, fields, 6u, ",");
        if (count != 5) continue;
        legacy_overlay_image_record_load(fields[0], panel, 4u);
        panel_category_bounds_set(panel, atoi(fields[1]), atoi(fields[2]),
                                  atoi(fields[3]), atoi(fields[4]));
    }
}

/* Port model of 0x00434710 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x00434710, retires to the tail in GAME-021 part 3
void panel_button_layout_config_load(BlockFile* config, int display_width,
                                     int display_height)
{
    enum { PANEL_COUNT = 12, BUTTONS_PER_PANEL = 24, LAYOUT_FIELDS = 9 };
    int counts[PANEL_COUNT];
    int slots[PANEL_COUNT][BUTTONS_PER_PANEL];
    char* textures[3][PANEL_COUNT][BUTTONS_PER_PANEL];
    int ids[PANEL_COUNT][BUTTONS_PER_PANEL];
    float geometry[PANEL_COUNT][BUTTONS_PER_PANEL][4];
    char path[256];
    BlockRec* group;
    BlockRec* entry;
    int panel;

    if (config == NULL) return;
    sprintf(path, "Buttons%ix%i", display_width, display_height);
    group = block_subgroup(config,
        string_path_join(util_root_name(config), path, (const char*)NULL));
    if (group == NULL) return;
    memset(counts, 0, sizeof(counts));
    memset(textures, 0, sizeof(textures));
    memset(ids, 0, sizeof(ids));

    for (entry = group; entry != NULL;
         entry = block_next_sibling(entry)) {
        const char* panel_name = block_name(entry);
        const char* value = block_value(entry);
        char value_copy[512];
        char* fields[LAYOUT_FIELDS];
        int count;
        unsigned int button;
        int row;

        if (panel_name == NULL || value == NULL ||
            !ui_panel_name_table_lookup(panel_name, &panel) ||
            panel < 0 || panel >= PANEL_COUNT ||
            counts[panel] >= BUTTONS_PER_PANEL) continue;
        strncpy(value_copy, value, sizeof(value_copy) - 1u);
        value_copy[sizeof(value_copy) - 1u] = '\0';
        count = ledger_config_split_limited(value_copy, fields,
                                            LAYOUT_FIELDS, ",");
        if (count != LAYOUT_FIELDS ||
            !ui_panel_indexed_name_lookup((unsigned int)panel, fields[0],
                                          &button) ||
            button >= BUTTONS_PER_PANEL) continue;

        row = counts[panel];
        slots[panel][row] = (int)button;
        if (strcasecmp_lite(fields[1], "NULL") != 0)
            textures[0][panel][row] = string_duplicate(fields[1]);
        if (strcasecmp_lite(fields[2], "NULL") != 0)
            textures[1][panel][row] = string_duplicate(fields[2]);
        if (strcasecmp_lite(fields[3], "NULL") != 0)
            textures[2][panel][row] = string_duplicate(fields[3]);
        geometry[panel][row][0] = (float)atoi(fields[4]);
        geometry[panel][row][1] = (float)atoi(fields[5]);
        geometry[panel][row][2] = (float)atoi(fields[6]);
        geometry[panel][row][3] = (float)atoi(fields[7]);
        if (!case_insensitive_name_table_lookup(fields[8], &ids[panel][row]))
            ids[panel][row] = 0;
        ++counts[panel];
    }

    for (panel = 0; panel < PANEL_COUNT; ++panel) {
        panel_texture_slots_allocate_named(
            panel, counts[panel], slots[panel], &geometry[panel][0][0],
            (const char* const*)textures[0][panel],
            (const char* const*)textures[1][panel],
            (const char* const*)textures[2][panel],
            ids[panel]);
    }
    for (panel = 0; panel < PANEL_COUNT; ++panel) {
        int row;
        for (row = 0; row < counts[panel]; ++row) {
            int texture;
            for (texture = 0; texture < 3; ++texture) {
                engine_heap_free(textures[texture][panel][row]);
                textures[texture][panel][row] = NULL;
            }
        }
    }
}
} // namespace frontend_config

/* AdvisorTutorialIconImageLoad (FUN_00434a00): root/Main/TutorialIcon
 * selects the authored bitmap. Production owns one texture reference. */
/* Port model of 0x00434a00 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x00434a00, retires to the tail in GAME-021 part 3
void advisor_tutorial_icon_image_load(BlockFile* cfg)
{
    const char* icon_path;
    void* icon;
    advisor_tutorial_icon_image_release();
    if (cfg == NULL || s_tutorial_icon_load == NULL) return;
    icon_path = block_get_strptr(cfg, string_path_join(util_root_name(cfg),
        "Main", "TutorialIcon", (char*)NULL));
    if (icon_path == NULL) return;
    icon = s_tutorial_icon_load(icon_path, s_tutorial_icon_context);
    if (icon == NULL) return;
    s_tutorial_icon = icon;
    if (s_tutorial_icon_initialize != NULL) {
        s_tutorial_icon_initialize(icon, s_tutorial_icon_context);
    }
}

/* Port model of 0x00434a50 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x00434a50, retires to the tail in GAME-021 part 3
void sound_samples_config_load(BlockFile* config, int allow_marked)
{
    BlockRec* group;
    BlockRec* entry;
    if (config == NULL) return;
    group = block_subgroup(config,
        string_path_join(util_root_name(config), "Samples", (const char*)NULL));
    if (group == NULL) return;
    sound_alias_registration_set(1);
    for (entry = group; entry != NULL;
         entry = block_next_sibling(entry)) {
        const char* value = block_value(entry);
        const char* alias_name = value;
        int alias;
        if (alias_name == NULL) continue;
        if (alias_name[0] == '!' && allow_marked) ++alias_name;
        else if (alias_name[0] == '!' && !allow_marked) continue;
        if (sound_alias_lookup(alias_name, &alias))
            (void)sound_sample_marker_parse(block_name(entry), alias);
    }
    sound_alias_registration_set(0);
}

namespace frontend_config {
// TextMessagesConfigLoad
/* Port model of 0x00434af0 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x00434af0, retires to the tail in GAME-021 part 3
void text_messages_config_load(BlockFile* config)
{
    BlockRec* group;
    BlockRec* entry;
    char path[256];
    if (config == NULL) return;
    group = block_subgroup(config, string_path_join(util_root_name(config),
        "TextMessages", (const char*)NULL));
    for (entry = group; entry != NULL;
         entry = block_next_sibling(entry)) {
        int slot;
        const char* label = block_name(entry);
        const char* text = block_value(entry);
        if (ui_message_label_index_find(label, &slot))
            ui_message_text_slot_assign(slot, text != NULL ? text : "");
    }
    group = block_subgroup(config, string_path_join(util_root_name(config),
        "TextMessagesWithImages", (const char*)NULL));
    for (entry = group; entry != NULL;
         entry = block_next_sibling(entry)) {
        int slot;
        const char* label = block_name(entry);
        const char* value = block_value(entry);
        char copy[512];
        char* fields[3];
        int count;
        if (!ui_message_label_index_find(label, &slot) || value == NULL)
            continue;
        strncpy(copy, value, sizeof(copy) - 1u);
        copy[sizeof(copy) - 1u] = '\0';
        fields[0] = copy;
        fields[1] = NULL;
        fields[2] = NULL;
        /* TextMessagesWithImages uses the colon delimiter at DAT_004a1cd4;
         * Panels_* and Buttons* use the separate comma delimiter. */
        count = ledger_config_split_limited(copy, fields, 3u, ":");
        (void)count;
        ui_message_text_slot_assign_with_image(slot, fields[0], fields[1],
                                                fields[2]);
    }
}
} // namespace frontend_config

/* Port model of 0x00434c10 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x00434c10, retires to the tail in GAME-021 part 3
void message_load_infomessages(BlockFile* cfg)
{
    static const char separators[] = ":";
    static const char slow_mode[] = "SLOW";
    static const char always_play[] = "ALWAYSPLAYSFX";
    if (cfg == NULL) {
        return;
    }
    /* Find any `InfoMessages` record (nested under Lego*; the parser keeps
     * document order) and configure each child by its Info name. */
    size_t n = block_record_count(cfg);
    for (size_t i = 0; i < n; ++i) {
        BlockRec* rec = block_record_at(cfg, i);
        if (rec == NULL || block_name(rec) == NULL
            || strcasecmp_lite(block_name(rec), "InfoMessages") != 0) {
            continue;
        }
        for (BlockRec* kid = block_first_child(rec); kid != NULL;
             kid = block_next_sibling(kid)) {
            const char* nm = block_name(kid);
            const char* val = block_value(kid);
            if (nm == NULL) {
                continue;
            }
            /* Entries may carry the Info_ prefix (finding 22's stored
             * strings are `Info_*`); the catalog is bare. */
            if (strncmp(nm, "Info_", 5) == 0) {
                nm += 5;
            }
            int id = -1;
            for (int k = 0; k < MESSAGE_MAX_ID; ++k) {
                if (strcasecmp_lite(nm, s_catalog[k]) == 0) {
                    id = k;
                    break;
                }
            }
            if (id < 0) {
                continue;
            }
            char value[1024];
            char* fields[1024];
            int field_count;
            size_t value_length;
            int sound_alias;
            char* sound_name = NULL;
            if (val == NULL) continue;
            value_length = strlen(val);
            if (value_length >= sizeof(value))
                value_length = sizeof(value) - 1u;
            memcpy(value, val, value_length);
            value[value_length] = '\0';
            field_count = sound_variant_split(value, fields, separators);
            if (field_count < 2) continue;

            if (field_count > 2 && fields[2][0] != '\0') {
                size_t sound_length = strlen(fields[2]);
                sound_name = (char*)engine_heap_alloc(
                    (long)(sound_length + 1u));
                if (sound_name != NULL) {
                    memcpy(sound_name, fields[2], sound_length + 1u);
                }
            }
            if (s_info_message_sound_names[id] != NULL)
                engine_heap_free(s_info_message_sound_names[id]);
            s_info_message_sound_names[id] = sound_name;

            /* InfoMessages stores display text, image resource, optional
             * sound alias, and two independent mode markers as colon fields. */
            message_config_set(id, sound_name, 0.0f, 0u);
            incident_message_template_replace(id, fields[0]);
            info_message_surface_load(id, fields[1]);

            if (field_count > 2 &&
                sound_alias_lookup(fields[2], &sound_alias) != 0) {
                info_message_secondary_sound_set(id,
                                                   (uint32_t)sound_alias);
            }
            if (field_count < 4) {
                info_message_always_play_flag_set(id, 1);
                indexed_selection_flag_20000_set_clear(id, 0);
            }
            else {
                info_message_always_play_flag_set(
                    id, strcasecmp_lite(fields[3], slow_mode) == 0);
                indexed_selection_flag_20000_set_clear(id,
                    strcasecmp_lite(fields[3], always_play) == 0 ||
                    (field_count >= 5 &&
                     strcasecmp_lite(fields[4], always_play) == 0));
            }
        }
    }
}

/* AdvisorTooltipAndTooltipSoundConfigLoad (FUN_00434da0): the two config
 * groups are keyed by the 38 named tooltip slots. Keep their text and sound
 * alias in the same slot used by case_insensitive_name_table_lookup. */
/* Port model of 0x00434da0 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x00434da0, retires to the tail in GAME-021 part 3
void advisor_tooltip_and_sound_config_load(BlockFile* config)
{
    BlockRec* entry;
    int index;
    int alias;
    for (entry = block_subgroup(config, string_path_join(
             util_root_name(config), "ToolTips", (const char*)NULL));
         entry != NULL; entry = block_next_equal_key(entry)) {
        if (case_insensitive_name_table_lookup(entry->key, &index))
            advisor_tooltip_text_apply(index, entry->value);
    }
    for (entry = block_subgroup(config, string_path_join(
             util_root_name(config), "ToolTipsSFXs", (const char*)NULL));
         entry != NULL; entry = block_next_equal_key(entry)) {
        if (case_insensitive_name_table_lookup(entry->key, &index) &&
            sound_alias_lookup(entry->value, &alias)) {
            timed_effect_cue_request_set(index, (uint32_t)alias);
        }
    }
}

/* InterfaceSpawnRequestEnqueue (FUN_00434e80): clear the interface pending
 * bit, update the directional request state for the two special types, or
 * dispatch to the first matching category from the object flags. */
/* Port model of 0x00434e80 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x00434e80, retires to the tail in GAME-021 part 3
int interface_spawn_request_enqueue(int type, uint32_t parameter)
{
    uint32_t flags;
    uint32_t category;
    uint32_t state_flags = global_state_low_word_get();
    global_state_low_word_set(state_flags & ~0x80u);
    if (s_interface_spawn_request_ops.prepare != NULL) {
        s_interface_spawn_request_ops.prepare(
            s_interface_spawn_request_ops.context, 1);
    }
    if (type == 4 || type == 0xb) {
        float direction[2] = { 0.0f, 0.0f };
        state_flags = global_state_low_word_get();
        global_state_low_word_set(state_flags | 0x80u);
        s_interface_spawn_request_state.type = type;
        s_interface_spawn_request_state.parameter = parameter;
        if (s_interface_spawn_request_ops.camera_direction_get != NULL) {
            s_interface_spawn_request_ops.camera_direction_get(
                s_interface_spawn_request_ops.context, direction);
        }
        if (fabs((double)direction[0]) <= fabs((double)direction[1])) {
            s_interface_spawn_request_state.facing =
                (direction[1] > 0.0f) ? 2 : 0;
        }
        else {
            s_interface_spawn_request_state.facing =
                (direction[0] > 0.0f) ? 3 : 1;
        }
        return 1;
    }
    if (s_interface_spawn_request_ops.request_flags_get == NULL ||
        s_interface_spawn_request_ops.request_dispatch == NULL) return 0;
    flags = s_interface_spawn_request_ops.request_flags_get(
        s_interface_spawn_request_ops.context, type, parameter);
    if ((flags & 0x1000u) != 0u) category = 0u;
    else if ((flags & 0x800u) != 0u) category = 1u;
    else if ((flags & 0x2000u) != 0u) category = 2u;
    else if ((flags & 0x400000u) != 0u) category = 3u;
    else return 0;
    return s_interface_spawn_request_ops.request_dispatch(
        s_interface_spawn_request_ops.context, category,
        category < 3u && s_interface_spawn_request_ops.saved_slot_get != NULL
            ? s_interface_spawn_request_ops.saved_slot_get(
                s_interface_spawn_request_ops.context, category)
            : NULL,
        type, parameter);
}

/* Retail ABI: pointer to center pair, then radius; level comes from the
 * process-global current level. */
/* Port model of 0x00435010 (GAME-021); the matching build uses lego_unit.c's
 * (it reads the level's pointer words). */
#if !defined(_MSC_VER) || !defined(_M_IX86)
// PORT-ONLY: port model of 0x00435010, retires to the tail in GAME-021 part 3
void terrain_cell_work_flag_circle_set_current(const int center_xy[2],
                                               int radius)
{
    GameContext* game = entity_pool_callback_context_get();
    LevelState* level;
    if (center_xy == NULL) return;
    level = game != NULL ? game->level : NULL;
    if (level == NULL)
        level = (LevelState*)map_cell_record_grid_level_get();
    if (level == NULL) return;
    const int width = (int)level->word[LEVEL_OFF_WIDTH / 4u];
    const int height = (int)level->word[LEVEL_OFF_HEIGHT / 4u];
    const int radius_squared = radius * radius;
    int row;
    for (row = -radius; row <= radius; ++row) {
        int column;
        for (column = -radius; column <= radius; ++column) {
            const int x = column + center_xy[0];
            const int y = center_xy[1] + row;
            if (column * column + row * row <= radius_squared &&
                -1 < x && x < width && -1 < y && y < height) {
                terrain_cell_work_flag_set_current((uint32_t)x,
                                                   (uint32_t)y);
            }
        }
    }
}
#endif
/* SurfaceTypeDescriptionsConfigLoad @ 0x4350a0. The caller supplies the
 * loaded Lego.cfg tree and its config-root prefix (DAT_005560c4 in retail).
 * Missing entries leave their corresponding zero/null global values alone. */
/* Port model of 0x004350a0 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x004350a0, retires to the tail in GAME-021 part 3
void surface_type_descriptions_config_load(BlockFile* config,
                                           const char* config_root)
{
    unsigned int i;
    if (config == NULL) return;
    for (i = 0u; i < SURFACE_TYPE_SLOTS; ++i) {
        char path[512];
        const char* value;
        char value_copy[1024];
        char* alias_name;
        char* value_end;
        char* split;
        char* decoded;
        int alias = 0;
        size_t length = 0u;
        surface_type_config_path_build(path, sizeof(path), config_root,
                                       s_surface_type_config_keys[i]);
        value = block_value(block_find(config, path));
        if (value == NULL) continue;
        while (length < sizeof(value_copy) - 1u && value[length] != '\0') {
            value_copy[length] = value[length];
            ++length;
        }
        value_copy[length] = '\0';
        split = strchr(value_copy, ',');
        if (split == NULL) continue;
        *split++ = '\0';
        alias_name = value_copy;
        while (*alias_name == ' ' || *alias_name == '\t') ++alias_name;
        value_end = alias_name + strlen(alias_name);
        while (value_end > alias_name &&
               (value_end[-1] == ' ' || value_end[-1] == '\t'))
            *--value_end = '\0';
        while (*split == ' ' || *split == '\t') ++split;
        value_end = split + strlen(split);
        while (value_end > split &&
               (value_end[-1] == ' ' || value_end[-1] == '\t'))
            *--value_end = '\0';
        decoded = formatted_text_escape_decode_and_emit("%s", alias_name);
        if (decoded != NULL) s_surface_type_names[i] = decoded;
        if (sound_alias_lookup(split, &alias) == 0) alias = 0;
        s_surface_type_values[i] = (uint32_t)alias;
    }
}

/* Port model of 0x00435170 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x00435170, retires to the tail in GAME-021 part 3
const char* terrain_surface_type_name_get(int index)
{
    static const char missing[] = "Need config for surface type";
    if (index < 0 || index >= SURFACE_TYPE_SLOTS
        || s_surface_type_names[index] == NULL) {
        return missing;
    }
    return s_surface_type_names[index];
}

/* Port model of 0x00435190 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x00435190, retires to the tail in GAME-021 part 3
uint32_t surface_type_configured_value_at_index(int index)
{
    if (index < 0 || index >= SURFACE_TYPE_SLOTS) {
        return 0; /* bounds deviation (corpus indexes blindly) */
    }
    return s_surface_type_values[index];
}

/* 0x004351a0 (media_action_code_to_sound_alias) is defined in lego_unit.c. */

/* RandomVariantCellScatter (FUN_00435230) — see level.hpp. Draws ride the
 * module's corpus-RNG model (the shared 15-bit word thunk). */
/* Port model of 0x00435230 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x00435230, retires to the tail in GAME-021 part 3
void random_variant_cell_scatter(int center_x, int center_y, int radius,
                                 int mode)
{
    int dedup[100];
    for (int i = 0; i < 100; ++i) {
        dedup[i] = 0;
    }
    if (radius <= 0) {
        return; /* deviation: the corpus would zero-modulo every draw */
    }
    const unsigned int diam = (unsigned int)(radius * 2);
    unsigned int limit = diam * diam;
    if (mode == 0) {
        limit >>= 2;
    }
    for (unsigned int n = 0; n < limit; ++n) {
        const unsigned int dx =
            (unsigned int)level_random_draw15() % diam;
        const unsigned int dy =
            (unsigned int)level_random_draw15() % diam;
        const unsigned int key = dy + dx * 10u;
        if (key < 100u && dedup[key] == 0) {
            const int cx = (int)((unsigned)center_x + dx) - radius;
            const int cy = (int)((unsigned)center_y + dy) - radius;
            const int candidate[2] = { cx, cy };
            /* FUN_0040f190 is the live erosion candidate/mutation path. */
            const int spawn = lava_erosion_cell_eligible_and_mutate(
                candidate, 1);
            if (spawn != 0 && mode != 0) {
                return;
            }
            dedup[key] = 1;
        }
    }
}

/* 0x00435550 (ledger_stub_00435550_GlobalStateZeroTransitionDetect) is
 * defined in lego_unit.c. */

/* Port model of 0x00435580 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x00435580, retires to the tail in GAME-021 part 3
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

/* MenuOptionPanelTextureUpdate (FUN_004355c0): software-framebuffer face of
 * the dialog panel updater. `FeMenuPanelText` carries the position and
 * centering data that the original text records hold in their packed words.
 * The two return values preserve the native OK/Cancel hover results. */
/* Port model of 0x004355c0 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x004355c0, retires to the tail in GAME-021 part 3
int menu_option_panel_texture_update(const FeMenuOptionPanel* panel,
                                    void* px, int pitch, int w, int h,
                                    int mouse_x, int mouse_y)
{
    if (panel == NULL || px == NULL || panel->image == NULL ||
        panel->font == NULL || panel->title.text == NULL ||
        panel->body.text == NULL || w <= 0 || h <= 0) {
        return -1;
    }

    if (panel->contrast != NULL) {
        surface_blit_texture(px, pitch, w, h, panel->contrast,
                             panel->contrast_x, panel->contrast_y);
    }
    surface_blit_texture(px, pitch, w, h, panel->image,
                         (w - panel->image->width) / 2,
                         (h - panel->image->height) / 2);
    fe_menu_panel_text_draw(px, pitch, w, h, &panel->title, panel->font);
    fe_menu_panel_text_draw(px, pitch, w, h, &panel->body, panel->font);

    int result = -1;
    int cell_width = 0, cell_height = 0;
    fe_font_cell(panel->font, &cell_width, &cell_height);
    (void)cell_width;
    const FeMenuPanelText* buttons[2] = { &panel->ok, &panel->cancel };
    for (int i = 0; i < 2; ++i) {
        const FeMenuPanelText* button = buttons[i];
        if (button->text == NULL) continue;
        const int text_width = fe_font_text_width(button->text, panel->font);
        int text_x = button->x;
        if (button->centered) {
            text_x += (button->width - text_width) / 2;
        }
        const int hover = mouse_x >= text_x && mouse_x <= text_x + text_width &&
                          mouse_y >= button->y && mouse_y <= button->y + cell_height;
        const TextureSurface* font = hover && panel->hover_font != NULL
                                         ? panel->hover_font : panel->font;
        formatted_text_draw(font, px, pitch, w, h, text_x, button->y,
                            button->text);
        if (hover) result = (i == 0) ? 1 : 0;
    }
    return result;
}

/* 0x00435a20 (global_state_low_byte_clear_and_refresh) is defined in
 * lego_unit.c. */

/* MainSimFeatureFlagLatch (FUN_00435a50): probe control 0x40b when state
 * bit 0x08000000 is set, then latch debug bit 0x2. Context-owned state is
 * the port adaptation of the original globals. */
/* Port model of 0x00435a50 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x00435a50, retires to the tail in GAME-021 part 3
void camera_registry_probe(EngineContext* ctx)
{
    if (ctx == NULL) {
        return;
    }
    if ((ctx->state & 0x8000000u) != 0) {
        if (control_probe(0x40bu)) {
            ctx->debug |= 2u;
        }
    }
}

/* Port model of 0x00435aa0 (GAME-021); the original-shape function is in lego_unit.c. */
// PORT-ONLY: port model of 0x00435aa0, retires to the tail in GAME-021 part 3
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

static CellTransientEffectPort* cell_transient_effect_port_ensure(
    LevelState* level, void* record)
{
    CellTransientEffectPort* port =
        cell_transient_effect_port_find(level, record);
    if (port != NULL) return port;
    port = (CellTransientEffectPort*)malloc(sizeof(*port));
    if (port == NULL) return NULL;
    port->level = level;
    port->record = record;
    port->emitter = NULL;
    port->next = s_cell_transient_effect_ports;
    s_cell_transient_effect_ports = port;
    return port;
}

static void cell_transient_effect_set(CellTransientEffectPort* port,
                                      void* emitter)
{
    if (port == NULL) return;
    port->emitter = emitter;
    /* Keep the retail pointer word for x86 builds and layout observability;
     * the sidecar holds the native pointer on wider hosts. */
    *(uint32_t*)((uint8_t*)port->record + 0x1c) =
        (uint32_t)(uintptr_t)emitter;
}

/* MapLoadPipeline calls GridCellRecordBufferReplace(level) after rebuilding
 * the separate 20-byte map-cell pool. EngineState is a portable view, so
 * keep its dimensions in persistent storage instead of casting LevelState
 * to the non-layout-compatible EngineState struct. */
static void level_field_grid_buffer_replace(LevelState* level)
{
    if (level == NULL) return;
    memset(&s_level_field_grid_state, 0, sizeof(s_level_field_grid_state));
    s_level_field_grid_state.grid_w = (uint32_t)level_int(
        level, LEVEL_OFF_WIDTH);
    s_level_field_grid_state.grid_h = (uint32_t)level_int(
        level, LEVEL_OFF_HEIGHT);
    s_level_field_grid_owner = level;
    grid_cell_record_buffer_replace(&s_level_field_grid_state);
}

/* Map header helper — corpus FUN_00477440. Returns the bias word the level
 * loaders subtract per layer (0 for the shipped maps, whose cells start at
 * +0x10; see level.hpp header note). */
static int map_header_value(const char* name)
{
    char fields[UTIL_QUERY_MAX_FIELDS][256];
    int status = util_query_fields(name, fields, sizeof(fields[0]));
    if (status == 0) {
        return 0;
    }
    return atoi(fields[1]);
}

static void terrain_cell_orientation_set_level(LevelState* level, int x, int y,
                                              int flipped)
{
    TerrainState* terrain;
    TerrainSurfaceGrid* surface;
    if (level == NULL || x < 0 || y < 0) return;
    terrain = (TerrainState*)level->ptr_terrain;
    surface = level_surface_grid_get(level);
    if (surface != NULL)
        terrain_cell_surface_orientation_set(surface, (uint32_t)x,
                                             (uint32_t)y, flipped);
    if (terrain != NULL && terrain->blocks != NULL && x < terrain->width &&
        y < terrain->height) {
        uint8_t* record = terrain->blocks +
            ((size_t)y * (size_t)terrain->width + (size_t)x) *
                TERRAIN_BLOCK_STRIDE;
        record[0x19] = (uint8_t)((record[0x19] & (uint8_t)~1u) |
                                 (flipped != 0 ? 1u : 0u));
        if (x + 1 < terrain->width && y + 1 < terrain->height) {
            size_t quad_index = (size_t)y * (size_t)terrain->dim[0] +
                                (size_t)x;
            terrain_indexed_mesh_cell_sync(terrain, quad_index);
            if (terrain->derived_mesh.records != NULL &&
                (terrain->derived_mesh.records[((size_t)y *
                    terrain->derived_mesh.width + (uint32_t)x) * 8u + 4u] &
                 1u) == 0u) {
                terrain_cell_mutation_progress(level, (uint32_t)x,
                                                (uint32_t)y);
            }
        }
    }
    scene_render_terrain_cell_orientation_apply(level, (uint32_t)x,
                                               (uint32_t)y, flipped);
}

static int dims_ok(const MapFile* m, const LevelState* s)
{
    return m->width == level_int(s, LEVEL_OFF_WIDTH)
           && m->height == level_int(s, LEVEL_OFF_HEIGHT);
}

/* The predug edge-wall cell update — the flag side of the cell update
 * (FUN_00430530) the corpus runs per `v == 1/3` predug cell *before* the
 * 3x3 edge-wall OR (LoadPredugLayer 0x42bd20). Corpus shape, cell-update
 * branch (the type byte is still 0 this early in the pipeline, so the
 * `type == 8/10` branch cannot run):
 *
 *   - bounds gate: x/y == 0 or x >= w-2 / y >= h-2 (unsigned) no-ops;
 *   - already-cave cells (`+0x08` bit 8 set) return untouched;
 *   - the wave-seed marks the northwest 2x2 footprint {cell, x+1,
 *     y+1, x+1/y+1} wave-state bytes (`record+0` = 1, the corpus's
 *     flooded mark of a seeded handle);
 *   - the eight neighbours without the cave flag take the traversable
 *     mark `+0x08 |= 0x10` and lose the `0x400000` open flag
 *     (`& ~0x400000`, the corpus's `& 0xfffbffff`);
 *   - the cell itself raises cave `8`, keeps `+0x40` clear (the corpus
 *     `& ~0x40`), and raises `0x400000` open plus `0x200000` — the
 *     param-4 (load-time) wave flag of the `else` branch.
 *
 * Renderer, wave-handle (`FUN_00431450`, `FUN_0044efb0`), and message
 * calls stay stubbed like the rest of the loading passes. This is the
 * *only* place the shipped load pipeline marks cells traversable
 * (`+0x08` bit `0x10`, the reader of `FUN_00432770` simple mode): the
 * later full-map cell-init never raises it, so without this update a
 * shipped level has no walkable cells at all — found by the T3
 * real-data probe (wave-flood paths must cross real terrain). */
static void predug_wall_cell(LevelState* s, int x, int y)
{
    int w = level_int(s, LEVEL_OFF_WIDTH);
    int h = level_int(s, LEVEL_OFF_HEIGHT);
    int dx, dy;
    if (x == 0 || y == 0 || (unsigned)x >= (unsigned)w - 2u ||
        (unsigned)y >= (unsigned)h - 2u) {
        return;
    }
    uint8_t* rec = voxel_at(s, x, y);
    uint32_t flags;
    __builtin_memcpy(&flags, rec + 0x08, sizeof(flags));
    if ((flags & 8u) != 0) {
        return; /* the corpus's cave gate: already cave, untouched */
    }

    /* Wave seed over the northwest 2x2 footprint (record+0 = flooded). */
    for (dy = 0; dy <= 1; ++dy) {
        for (dx = 0; dx <= 1; ++dx) {
            uint8_t* f = voxel_at(s, x + dx, y + dy);
            if (f[0x00] == 0) {
                f[0x00] = 1;
            }
        }
    }

    /* The eight neighbours: traversable mark over non-cave cells. */
    for (dy = -1; dy <= 1; ++dy) {
        for (dx = -1; dx <= 1; ++dx) {
            uint8_t* n = voxel_at(s, x + dx, y + dy);
            uint32_t nf;
            __builtin_memcpy(&nf, n + 0x08, sizeof(nf));
            if ((nf & 8u) == 0) {
                nf |= TILE_FLAG_WALKABLE;
                nf &= ~0x400000u;
                __builtin_memcpy(n + 0x08, &nf, sizeof(nf));
            }
        }
    }

    /* The cell itself: cave + open + wave flag, blocking bit kept clear. */
    flags |= 8u | 0x400000u | 0x200000u;
    flags &= ~0x40u;
    __builtin_memcpy(rec + 0x08, &flags, sizeof(flags));
}

/* FUNCTION: LEGORR 0x004311d0. The scene object and linked record family
 * are not ported, so preserve the observable per-level cell membership and
 * enable state in a bounded sidecar table. */
static void terrain_activity_build_record_update(LevelState* s, int x, int y,
                                                  int enable)
{
    if (s == NULL) return;
    int free_slot = -1;
    for (int i = 0; i < 256; ++i) {
        TerrainActivityBuildRecordModel* record =
            &s_terrain_activity_build_records[i];
        if (record->level == s && record->x == x && record->y == y) {
            if (enable == 0) {
                record->level = NULL;
                record->enabled = 0;
                record->scene_flags_0c = 0u;
            } else {
                record->enabled = enable;
            }
            return;
        }
        if (record->level == NULL && free_slot < 0) free_slot = i;
    }
    if (enable != 0 && free_slot >= 0) {
        s_terrain_activity_build_records[free_slot].level = s;
        s_terrain_activity_build_records[free_slot].x = x;
        s_terrain_activity_build_records[free_slot].y = y;
        s_terrain_activity_build_records[free_slot].enabled = enable;
        s_terrain_activity_build_records[free_slot].scene_flags_0c = 0u;
    }
}

/* The level loader sweeps cells by calling the target-shaped per-cell entry.
 * Keep the sweep separate so it cannot be mistaken for the mapped function. */
static void cell_init_all(LevelState* s)
{
    int x, y;
    int width = level_int(s, LEVEL_OFF_WIDTH);
    int height = level_int(s, LEVEL_OFF_HEIGHT);
    for (y = 0; y < height; ++y)
        for (x = 0; x < width; ++x)
            cell_init_pass(s, (float)x, (uint32_t)y);
}

/* The source level loader's predug pool is the production source for the
 * camera selector's bounded copy. Keep the engine sidecar in step with the
 * just-loaded level before simulation starts. */
static int camera_predug_coordinate_resolve(int x, int y, float* world_x,
                                           float* world_y, void* context)
{
    LevelState* level = (LevelState*)context;
    float position[2];
    if (level == NULL || world_x == NULL || world_y == NULL ||
        !terrain_cell_world_center_xy_get(
            level_load_ptr(level, LEVEL_OFF_TERRAIN), x, y, position)) {
        return 0;
    }
    *world_x = position[0];
    *world_y = position[1];
    return 1;
}

static void camera_predug_spawn_state_seed(LevelState* level)
{
    int cell_xy[20 * 2];
    unsigned int i;
    unsigned int count = level_predug_coordinate_count_get();
    if (count > 20u) count = 20u;
    for (i = 0u; i < count; ++i) {
        int cell[2];
        if (!level_predug_coordinate_get(i, cell)) break;
        cell_xy[i * 2u] = cell[0];
        cell_xy[i * 2u + 1u] = cell[1];
    }
    camera_predug_spawn_state_install(cell_xy, i, i != 0u);
    camera_spawn_level_block_bind(level);
    camera_spawn_coordinate_hook_set(camera_predug_coordinate_resolve, level);
}

static const char* cfg_string(BlockFile* cfg, const char* level, const char* key,
                              char* out, size_t out_sz)
{
    if (cfg == NULL || level == NULL || key == NULL) {
        return NULL;
    }
    char path[512];
    if (cfg_resolve_path(cfg, level, key, path, sizeof(path)) == NULL) {
        return NULL;
    }
    const char* v = block_get_strptr(cfg, path);
    if (v == NULL) {
        return NULL;
    }
    snprintf(out, out_sz, "%s", v);
    return out;
}

static long cfg_int(BlockFile* cfg, const char* level, const char* key, long dflt)
{
    char path[512];
    if (cfg_resolve_path(cfg, level, key, path, sizeof(path)) == NULL) {
        return dflt;
    }
    return block_get_int(cfg, path, (int)dflt);
}

static float cfg_float(BlockFile* cfg, const char* level, const char* key, float dflt)
{
    char path[512];
    if (cfg_resolve_path(cfg, level, key, path, sizeof(path)) == NULL) {
        return dflt;
    }
    const char* value = block_get_strptr(cfg, path);
    return value != NULL ? (float)atof(value) : dflt;
}

static int map_bias_name(BlockFile* cfg, const char* level, const char* key,
                         char* out, size_t out_sz)
{
    out[0] = '\0';
    if (cfg_string(cfg, level, key, out, out_sz) == NULL) {
        return 0;
    }
    return map_header_value(out);
}

static BlockRec* level_config_block_find(BlockFile* cfg, const char* level)
{
    char path[512];
    const char* root;
    BlockRec* record;
    if (cfg == NULL || level == NULL) return NULL;
    root = util_root_name(cfg);
    /* The level's container record (NULL when missing or empty). */
    snprintf(path, sizeof(path), "%s::Levels::%s", root, level);
    record = block_find(cfg, path);
    if (block_has_children(record)) return record;
    snprintf(path, sizeof(path), "%s::%s", root, level);
    record = block_find(cfg, path);
    return block_has_children(record) ? record : NULL;
}

/* LevelIdentifierRemapTableLoad @ 0x45d7e0 opens PTLFile, selects the
 * executable-root subgroup, and stores each child name/value as a pair of
 * message identifiers. Unknown names retain the final case-insensitive
 * comparison result returned by the retail lookup. */
static int level_identifier_remap_table_load(const char* path,
                                              const char* root_name)
{
    enum { LEVEL_REMAP_LOCAL_CAPACITY = 128 };
    uint32_t pairs[LEVEL_REMAP_LOCAL_CAPACITY * 2];
    BlockFile* file;
    BlockRec* group;
    BlockRec* entry;
    unsigned int count = 0u;
    unsigned int i;
    if (path == NULL || path[0] == '\0' || root_name == NULL) {
        level_identifier_remap_table_install(NULL, 0u);
        return 0;
    }
    file = block_open(path);
    if (file == NULL) {
        level_identifier_remap_table_install(NULL, 0u);
        return 0;
    }
    group = block_subgroup(file, root_name);
    if (group == NULL) {
        level_identifier_remap_table_install(NULL, 0u);
        block_close(file);
        return 1;
    }
    message_string_table_initialize();
    for (entry = group; entry != NULL;
         entry = block_next_sibling(entry)) {
        const char* name = block_name(entry);
        const char* value = block_value(entry);
        int resolved[2] = { -1, -1 };
        const char* tokens[2];
        unsigned int token;
        if (name == NULL || value == NULL) continue;
        tokens[0] = name;
        tokens[1] = value;
        for (token = 0u; token < 2u; ++token) {
            int comparison = -1;
            for (i = 0u; i < 65u; ++i) {
                const unsigned char* left = (const unsigned char*)
                    message_string_table_get(i);
                const unsigned char* right =
                    (const unsigned char*)tokens[token];
                if (left == NULL) {
                    comparison = 1;
                    continue;
                }
                /* MessageIdentifierFind compares the authored name with
                 * the table entry after its eight-byte Message_ prefix. */
                left += 8;
                do {
                    unsigned char a = *left++;
                    unsigned char b = *right++;
                    if (a >= 'A' && a <= 'Z') a += 'a' - 'A';
                    if (b >= 'A' && b <= 'Z') b += 'a' - 'A';
                    comparison = (int)a - (int)b;
                } while (comparison == 0 && left[-1] != 0u);
                if (comparison == 0) {
                    resolved[token] = (int)i;
                    break;
                }
            }
            if (resolved[token] < 0) resolved[token] = comparison;
        }
        if (count < LEVEL_REMAP_LOCAL_CAPACITY) {
            pairs[count * 2u] = (uint32_t)resolved[0];
            pairs[count * 2u + 1u] = (uint32_t)resolved[1];
            ++count;
        }
    }
    level_identifier_remap_table_install(pairs, count);
    block_close(file);
    return 1;
}

static uint32_t cell_secondary_word_unchecked(const LevelState* s,
                                              int cx, int cy)
{
    const uint8_t* rec = cell_record_unchecked_const(s, cx, cy);
    uint32_t flags = 0u;
    if (rec != NULL) __builtin_memcpy(&flags, rec + 0x0c, sizeof(flags));
    return flags;
}

/* 0x433380 reads the mutable base mesh through 0x4500d0, adjusts a
 * local copy, then publishes it to the distinct 0x462f20 quad owner. */
static int terrain_cell_mutation_progress_level(LevelState* level,
                                                TerrainSurfaceGrid* surface,
                                                int x, int y)
{
    static const int corner_x[4] = { 0, 1, 1, 0 };
    static const int corner_y[4] = { 0, 0, 1, 1 };
    TerrainState* terrain;
    uint8_t* cell;
    uint32_t flags;
    float base[12];
    float derived[4][3];
    float impact;
    int corner;
    (void)surface;
    if (level == NULL || x < 0 || y < 0 ||
        x >= level_width(level) - 1 || y >= level_height(level) - 1)
        return 0;
    terrain = (TerrainState*)level->ptr_terrain;
    cell = level_voxel(level, x, y);
    if (cell == NULL || terrain == NULL ||
        terrain->derived_mesh.records == NULL ||
        !terrain_cell_base_vertices_get(terrain, (uint32_t)x, (uint32_t)y,
                                         base)) return 0;
    flags = *(uint32_t*)(cell + 8);
    if ((flags & (8u | 0x10u)) == 0u) return 0;
    impact = level_float(level, LEVEL_OFF_DIGDEPTH) +
             level_float(level, LEVEL_OFF_ROOFHEIGHT);
    for (corner = 0; corner < 4; ++corner) {
        uint8_t* vertex = level_voxel(level, x + corner_x[corner],
                                      y + corner_y[corner]);
        derived[corner][0] = base[corner * 3];
        derived[corner][1] = base[corner * 3 + 2];
        derived[corner][2] = base[corner * 3 + 1];
        if ((flags & 8u) != 0u || (vertex != NULL && vertex[0] == 1u))
            derived[corner][2] -= impact;
    }
    terrain_cell_quad_geometry_emit(&terrain->derived_mesh,
        (uint32_t)x, (uint32_t)y, derived);
    return 1;
}

static void terrain_level_vertex_offset(LevelState* level, int vertex_x,
                                        int vertex_y, float delta)
{
    static const int cell_x[4] = { 0, -1, -1, 0 };
    static const int cell_y[4] = { 0, 0, -1, -1 };
    static const uint8_t corner[4] = { 0u, 1u, 2u, 3u };
    TerrainState* terrain;
    uint32_t index;
    if (level == NULL || vertex_x < 0 || vertex_y < 0) return;
    terrain = (TerrainState*)level->ptr_terrain;
    if (terrain == NULL || terrain->cell_vertices == NULL ||
        terrain->width < 2 || terrain->height < 2) return;
    for (index = 0u; index < 4u; ++index) {
        int x = vertex_x + cell_x[index];
        int y = vertex_y + cell_y[index];
        uint32_t cell_corner = corner[index];
        size_t cell_index;
        if (x < 0 || y < 0 || x >= terrain->width - 1 ||
            y >= terrain->height - 1) continue;
        cell_index = (size_t)y * (size_t)(terrain->width - 1) +
                     (size_t)x;
        /* cell_vertices is canonical. The indexed mesh applies the retail
         * rotation; applying it here again selects the wrong shared corner.
         * 0x44f380 supplies (0,0,delta) in retail X,Z,height order. */
        terrain->cell_vertices[cell_index * 12u + cell_corner * 3u + 1u]
            += delta;
        terrain_indexed_mesh_cell_sync(terrain, cell_index);
        terrain_create_cell_surface_vectors_rebuild(terrain,
            (uint32_t)cell_index, (uint32_t)y * terrain->dim[2] +
                (uint32_t)x,
            (terrain->blocks[((size_t)y * terrain->dim[2] +
                (size_t)x) * TERRAIN_BLOCK_STRIDE + 0x19u] & 1u) != 0u);
    }
    scene_render_terrain_vertex_offset(level, vertex_x, vertex_y, delta);
}

static void surface_type_config_path_build(char* path, size_t capacity,
                                           const char* root,
                                           const char* key)
{
    static const char section[] = "::SurfaceTypeDescriptions::";
    size_t used = 0u;
    size_t i;
    if (path == NULL || capacity == 0u) return;
    path[0] = '\0';
    if (root != NULL) {
        for (i = 0u; root[i] != '\0' && used + 1u < capacity; ++i)
            path[used++] = root[i];
    }
    for (i = 0u; section[i] != '\0' && used + 1u < capacity; ++i)
        path[used++] = section[i];
    if (key != NULL) {
        for (i = 0u; key[i] != '\0' && used + 1u < capacity; ++i)
            path[used++] = key[i];
    }
    path[used] = '\0';
}

static int periodic_entity_callback_adapter(EntitySlot* entity,
                                            void* payload)
{
    if (s_periodic_entity_callback == NULL) {
        return 0;
    }
    return s_periodic_entity_callback(entity, payload,
                                      s_periodic_entity_callback_context);
}

/* Parse an optional child value under "<root>::<obj>::<key>". */
static const char* obj_val(BlockFile* bf, const char* root, const char* obj,
                           const char* key)
{
    char path[512];
    snprintf(path, sizeof(path), "%s::%s::%s", root, obj, key);
    return block_get_strptr(bf, path);
}

/* ObjectListEntityTypeResolve scans the loaded RockMonster, Vehicle,
 * MiniFigure, and Building name tables in that order. Keep the fixed names
 * below as a pre-config fallback for callers that run before GameConfigInit;
 * once a table is installed, its rows and names are authoritative. */
static int entity_type_config_name_find(uint32_t table_kind,
                                       const char* name, int type,
                                       int* row_out)
{
    const TypeConfigTable* table = entity_type_config_table_get(table_kind);
    uint32_t row;
    if (table == NULL || table->names == NULL) return 0;
    for (row = 0u; row < table->count; ++row) {
        if (table->names[row] != NULL &&
            strcasecmp_lite(table->names[row], name) == 0) {
            if (row_out != NULL) *row_out = (int)row;
            return type;
        }
    }
    return -1;
}

static void ol_strncpy(char* dst, size_t n, const char* src)
{
    if (n == 0) {
        dst[0] = '\0';
        return;
    }
    if (src == NULL) {
        dst[0] = '\0';
        return;
    }
    strncpy(dst, src, n - 1);
    dst[n - 1] = '\0';
}

static uint32_t entity_cell_speed_rock_fall_name_find(const char* name)
{
    return lego::object_type_name_index_find(name);
}

static float entity_cell_speed_indexed_scale_get(int type,
                                                  const void* entity_record)
{
    return lego::entity_indexed_float_lookup_with_minus_one_fallback(
        type, entity_record);
}

static void fe_menu_panel_text_draw(void* px, int pitch, int w, int h,
                                    const FeMenuPanelText* item,
                                    const TextureSurface* font)
{
    if (item == NULL || item->text == NULL) return;
    int text_x = item->x;
    if (item->centered) {
        text_x += (item->width - fe_font_text_width(item->text, font)) / 2;
    }
    formatted_text_draw(font, px, pitch, w, h, text_x, item->y, item->text);
}

/* The text half of ToolTip_SetContent (FUN_0046b520) for the advisor
 * tool tips: copy the value into the slot's record, turning "\n" into a
 * newline and '_' into a space, measure it and arm the record. Image
 * entries ('@') require the indexed resource owner; do not turn their file
 * names into visible tooltip text. */
static void advisor_tooltip_text_apply(int index, const char* value)
{
    TimedEffectRecord* record;
    char* out;
    size_t used = 0;
    int lines = 0;
    uint32_t width;
    if (value == NULL || *value == '@') return;
    record = timed_effect_record_get((unsigned int)index);
    out = (char*)record->payload;
    while (*value != '\0' && used < 511u) {
        if (*value == '\\' && value[1] == 'n') {
            out[used++] = '\n'; value += 2;
        } else {
            out[used++] = *value == '_' ? ' ' : *value;
            ++value;
        }
    }
    out[used] = '\0';
    width = formatted_text_draw_and_measure(s_advisor_tooltip_font,
        NULL, 0, 0, 0, 0, 0, out, &lines);
    memcpy((char*)record + 0x204, &width, sizeof(width));
    memcpy((char*)record + 0x208, &lines, sizeof(lines));
    memset((char*)record + 0x20c, 0, 0x64);
    record->flags = (record->flags & ~8u) | 3u;
}

namespace frontend_config {
static int ledger_config_split_limited(char* text, char** fields,
                                       unsigned int field_limit,
                                       const char* separator)
{
    size_t separator_length;
    unsigned int count;
    char* p;
    if (text == NULL || fields == NULL || field_limit == 0u ||
        separator == NULL || separator[0] == '\0' || text[0] == '\0')
        return 0;
    separator_length = strlen(separator);
    fields[0] = text;
    count = 1u;
    /* BlockTextTokenSplit tests the character after the candidate before
     * comparing the separator, so a trailing delimiter stays in the token. */
    for (p = text; p[1] != '\0' && count < field_limit; ++p) {
        if (strncmp(separator, p, separator_length) == 0) {
            *p = '\0';
            fields[count++] = p + separator_length;
        }
    }
    return (int)count;
}
} // namespace frontend_config

namespace unported_ledger_stubs {
// TerrainCommandFeedbackDispatch
static uint32_t terrain_feedback_message_cue_get(void* context,
                                                  int message_id)
{
    (void)context;
    return surface_type_configured_value_at_index(message_id);
}

static void terrain_feedback_message_cue_set(void* context, int message_group,
                                              uint32_t cue)
{
    (void)context;
    timed_effect_cue_request_set(message_group, cue);
}

static void terrain_feedback_message_update(void* context, int message_group)
{
    (void)context;
    timed_effect_update_request_set(message_group);
}
} // namespace unported_ledger_stubs

namespace lego {
namespace {
int ascii_lower(int c)
{
    return (c >= 'A' && c <= 'Z') ? c + ('a' - 'A') : c;
}

void upgrade_type_tables_release(void)
{
    uint32_t i;
    for (i = 0u; i < s_upgrade_type_count; ++i)
        engine_heap_free(s_upgrade_type_names != 0
            ? s_upgrade_type_names[i] : 0);
    engine_heap_free(s_upgrade_type_names);
    engine_heap_free(s_upgrade_types);
    s_upgrade_type_names = 0;
    s_upgrade_types = 0;
    s_upgrade_type_count = 0u;
    upgrade_name_table_install(0, 0, 0);
}

int prefix_equal(const char* value, const char* prefix)
{
    if (value == 0 || prefix == 0) return 0;
    while (*prefix != '\0') {
        if (*value == '\0' || ascii_lower((unsigned char)*value) !=
                               ascii_lower((unsigned char)*prefix)) return 0;
        ++value;
        ++prefix;
    }
    return 1;
}
} // namespace
} // namespace lego

/* FUN_0046a4c0's bit ordering and all-pass aggregation are implemented by
 * the game entity layer. Only LAB_0046a490's entity predicate is unknown. */
static int control_registry_predicate_dispatch(EntitySlot* entity,
                                               uint32_t bit, void* user)
{
    (void)user;
    if (s_control_registry_predicate == NULL) {
        return 0;
    }
    return s_control_registry_predicate(entity, bit,
                                        s_control_registry_user);
}

/* Spawn one pending grid cell through the installed game entity factory. */
static int grid_spawn(uint32_t* coords, uint32_t hint)
{
    if (coords == NULL || s_grid_spawn_hook == NULL) return 0;
    return s_grid_spawn_hook(coords, hint, s_grid_spawn_context);
}

/* Query the game-owned registry before issuing a delayed spawn. */
static int spawn_probe(int first, int second)
{
    if (s_spawn_registry_probe_hook != NULL) {
        return s_spawn_registry_probe_hook(first, second,
                                           s_spawn_registry_probe_context);
    }
    /* The level-backed pool is the supported eligibility signal until the
     * entity registry callback is connected by the game layer. */
    return s_predug_spawn_enabled != 0 && s_predug_spawn_count != 0u;
}

static void spawn_issue(int source_object, uint32_t type)
{
    predug_spawn_coordinate_select_and_create(source_object, (int)type);
}

static int control_probe(uint32_t mask)
{
    GameContext* game = entity_pool_callback_context_get();
    if (s_control_registry_predicate == NULL) {
        return 0;
    }
    return entity_flag_mask_callbacks_predicate_all_pass(
        game, mask, control_registry_predicate_dispatch, NULL);
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

static int m_edge(EngineContext* ctx, int key)
{
    return ctx->input != NULL ? input_key_edge(ctx->input, key) : 0;
}

static int m_held(EngineContext* ctx, int key)
{
    return ctx->input != NULL ? input_key_down(ctx->input, key) : 0;
}

static void m_toggle_hud(EngineContext* ctx, uint32_t* flag, uint32_t bit,
                         const char* label)
{
    *flag ^= bit;
    char buf[96];
    snprintf(buf, sizeof(buf), "%s : %s", label,
             (*flag & bit) != 0 ? "on" : "off");
    main_hud_text(ctx, buf);
}

static int s_cd_rand15(void)
{
#if defined(_MSC_VER) && (_MSC_VER < 1200) && defined(_M_IX86)
    return rand();
#else
    s_cd_rand_state = s_cd_rand_state * 214013u + 2531011u;
    return (int)((s_cd_rand_state >> 16) & 0x7fffu);
#endif
}

/* Retail track selection uses unsigned remainder + base - 1. */
static unsigned s_cd_draw_slot(void)
{
    unsigned draw = (unsigned)s_cd_rand15();
    return draw % (unsigned)s_cd_window - 1u + (unsigned)s_cd_base;
}

static void s_cd_rearm(void)
{
    media_cd_audio_stop_and_close_if_active();
    media_cd_watchdog_update(1);
}

static uint32_t media_float_bits(float value)
{
    union FloatBits {
        float value;
        uint32_t bits;
    } converted;
    converted.value = value;
    return converted.bits;
}

/* --- legoGlobs fields Panel.c reads (game/lego.h) ---------------------------
 * The two interface fonts (DAT_00556124, DAT_00556128) and the radar zoom
 * (DAT_005563f4). The port has no owner of these legoGlobs fields yet; these
 * port globals stand in for them (finding 554). */
#include "lego.h"

/* s_lego_text_window_font (00556124), s_lego_crystal_count_font (00556128)
 * and s_lego_radar_zoom (005563f4) are defined in lego_unit.c. */

/* Lego_GetObjectByName (0042e8b0) has C linkage under the name
 * entity_type_pair_lookup, for Reward.c (GAME-037, finding 560). entity.cpp
 * reaches it through its frozen private header under the port's name, so
 * this unannotated forwarder keeps that name. */
// PORT-ONLY: adapter or forwarder for the port contract
int entity_type_pair_for_name(const char* name, int* type_out, int* row_out,
                              int* zero_out)
{
    return entity_type_pair_lookup(name, type_out, row_out, zero_out);
}

/* ---- GAME-021 part 1 adapters, group G ---- */

/* Original-ABI adapter (ADR 0005 step 6) for GAME-021's lego_unit.c: the
 * original FUN_00431570 (Lego_GetMap) takes no argument; the port's entry
 * takes two cell coordinates it ignores. */
// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (level_current_terrain_get)
extern "C" void* level_current_terrain_get_original(void)
{
    return level_current_terrain_get(0, 0);
}

/* Original-ABI adapter (ADR 0005 step 6) for GAME-021's lego_unit.c:
 * FUN_00433e80 (Lego.c part 2) takes the first-person camera's container,
 * the main viewport, the clip distance and a count. The port's
 * TerrainProjectedCellStateRefresh takes a state/hooks model instead; there
 * is no port entry with the original's contract, so this does nothing
 * (natively nothing calls it). */
// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (none: FUN_00433e80 has no port entry)
extern "C" void terrain_projected_cell_state_refresh_original(void* container,
                                                              void* view,
                                                              float distance,
                                                              unsigned int count)
{
    (void)container;
    (void)view;
    (void)distance;
    (void)count;
}

/* ---- GAME-021 part 1 adapters, group C ---- */

/* Original-ABI adapters (ADR 0005 step 6) for GAME-021's lego_unit.c. */

/* RadarMap_Draw (FUN_0045dbc0): the port has no function for it.
 * Natively nothing calls this. In the matching build radarmap.c's
 * RadarMap_Draw takes this name (radarmap_link_names.h; finding 606). */
#if !(defined(_MSC_VER) && _MSC_VER < 1200)
// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (RadarMap_Draw 0045dbc0, no port function)
extern "C" void radar_map_tv_position_set_adapter(void* radarMap, const float* pos)
{
    (void)radarMap;
    (void)pos;
}
#endif

/* The building placement update (FUN_0044a910): the port's entry takes a
 * hook table. Natively nothing calls this; it reports no placement. */
// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (0044a910, port entry takes a hook table)
extern "C" int building_placement_update_adapter(int type, int index, int leftReleased,
                                                 int rightReleased, const float* pos,
                                                 unsigned int bx, unsigned int by,
                                                 int show, void* selectPlace)
{
    (void)type; (void)index; (void)leftReleased; (void)rightReleased;
    (void)pos; (void)bx; (void)by; (void)show; (void)selectPlace;
    return 0;
}

/* LegoObject_GetBlockPos (FUN_004424f0): the port's entry takes the level
 * as well; the original reads the current level. */
// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (LegoObject_GetBlockPos 004424f0 -> entity_position_to_terrain_cell)
extern "C" void entity_position_to_terrain_cell_adapter(const struct EntitySlot* entity,
                                                        int* cell_x, int* cell_y)
{
    entity_position_to_terrain_cell(entity, g_active_level_retail, cell_x, cell_y);
}

/* ---- GAME-021 part 1 adapters, group B ---- */
/* Original-ABI adapters (ADR 0005 step 6) for GAME-021's lego_unit.c
 * (Lego_MainLoop, 0x004232e0). The port's entries take its engine or game
 * context, other arguments, or do not exist. Natively nothing calls these:
 * the port runs its own frame tick. Where a context is needed they pass the
 * main loop's engine context or the bound game context. */
// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (EngineKeepRunning)
extern "C" int keep_running_original(void) /* KeepRunning 0x00435940 */
{
    return EngineKeepRunning(s_callback_registry_context);
}

// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (camera_move_pipeline)
extern "C" void camera_move_pipeline_original(float elapsedWorld) /* 0x0046a730 */
{
    camera_move_pipeline(s_callback_registry_context, elapsedWorld);
}

// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (camera_registry_probe)
extern "C" void camera_registry_probe_original(void) /* 0x00435a50 */
{
    camera_registry_probe(s_callback_registry_context);
}

/* In the matching build GAME-009's DamageFont_UpdateAll takes this name
 * itself (screen_effect_quads_link_names.h, finding 609). */
// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (simtick_table_dispatch)
#if !(defined(_MSC_VER) && _MSC_VER < 1200)
extern "C" void simtick_table_dispatch_original(float elapsed) /* 0x0040aa10 */
{
    simtick_table_dispatch(s_callback_registry_context, elapsed, elapsed);
}
#endif

// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (simtick_scene_pass)
extern "C" void simtick_scene_pass_original(float elapsedWorld) /* 0x0043b270 */
{
    simtick_scene_pass(s_callback_registry_context, elapsedWorld);
}

// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (simtick_sfx_bank)
extern "C" void simtick_sfx_bank_original(float elapsedWorld) /* 0x0046f550 */
{
    simtick_sfx_bank(s_callback_registry_context, elapsedWorld);
}

/* 0x00435f5c LegoCamera_Update(camera, level, elapsed, elapsedWorld). */
// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (view_tick)
extern "C" void view_tick_original(void* camera, struct Lego_Level* level,
                                   float elapsed, float elapsedWorld)
{
    (void)level;
    if (camera != NULL)
        view_tick((ViewContext*)camera, s_callback_registry_context, elapsed,
                  elapsedWorld);
}

// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (simtick_render_step)
extern "C" void simtick_render_step_original(void) /* 0x00449c00 */
{
    simtick_render_step(s_callback_registry_context);
}

// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (simtick_post_cluster)
extern "C" void simtick_post_cluster_original(float elapsedWorld) /* 0x0040d450 */
{
    (void)elapsedWorld;
    simtick_post_cluster(s_callback_registry_context);
}

/* 0x0046e390 takes the level; the port reads it from the context. */
// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (simtick_fx_bank_tick)
extern "C" void simtick_fx_bank_tick_original(struct Lego_Level* level,
                                              float elapsedWorld)
{
    (void)level;
    simtick_fx_bank_tick(s_callback_registry_context, elapsedWorld);
}

// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (simtick_sound_keepalive)
extern "C" void simtick_sound_keepalive_original(void) /* 0x0047aba0 */
{
    simtick_sound_keepalive(s_callback_registry_context);
}

/* 0x00463230 walks global dirty-quad state; the port's entry takes a pass
 * record the main loop does not own. Nothing to do here. */
// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (terrain_dirty_quad_normal_emit_pass, no-op)
extern "C" void terrain_dirty_quad_normal_emit_pass_original(void)
{
}

/* 0x0040e8d0 takes the main viewport; the port's entry takes the effect
 * record and flags, which the viewport does not give. Nothing to do here. */
// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (screen_space_effect_dispatch_if_enabled, no-op)
extern "C" void screen_space_effect_dispatch_original(void* view)
{
    (void)view;
}

/* 0x00435300 and 0x00454600, the world render passes: no port entry. */
// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (0x00435300, no port entry)
extern "C" void world_render_pass_a_original(struct Lego_Level* level, void* view,
                                             float elapsedWorld, float elapsed)
{
    (void)level;
    (void)view;
    (void)elapsedWorld;
    (void)elapsed;
}

// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (0x00454600, no port entry)
extern "C" void world_render_pass_b_original(struct Lego_Level* level, void* view,
                                             float elapsedWorld, float elapsed)
{
    (void)level;
    (void)view;
    (void)elapsedWorld;
    (void)elapsed;
}

/* 0x0043c2b0 takes the frame time and whether the track view renders. */
// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (periodic_timer25_advance)
extern "C" void periodic_timer25_advance_original(float elapsedWorld,
                                                  int renderTrack)
{
    (void)renderTrack;
    periodic_timer25_advance(entity_pool_callback_context_get(),
                             elapsedWorld);
}

/* 0x0040e7f0 and 0x0041b490 (AdvisorOverlayUpdate): no port entry. */
// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (0x0040e7f0, no port entry)
extern "C" void effects_update_original(float elapsed)
{
    (void)elapsed;
}

/* 0x0045c0d0 Panel_Crystals_Draw also takes the frame time, unused. The
 * port's entry is in panel_unit.c (panel_unit.h). */
extern "C" void panel_crystals_draw(unsigned int crystals,
                                    unsigned int usedCrystals);
// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (panel_crystals_draw)
extern "C" void panel_crystals_draw_original(unsigned int crystals,
                                             unsigned int usedCrystals,
                                             float elapsedWorld)
{
    (void)elapsedWorld;
    panel_crystals_draw(crystals, usedCrystals);
}

/* 0x004589a0 Objective_IsEnded reads the current level. */
// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (global_option_low_bits_enabled)
extern "C" int objective_is_ended_original(void)
{
    return global_option_low_bits_enabled(g_active_level_retail);
}

/* 0x00415370 runs the in-game menu with the original's globals; the port's
 * gate takes its state and callbacks from the frame loop. */
// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (overlay_menu_gate, no-op)
extern "C" int overlay_menu_gate_original(float elapsed, int mode)
{
    (void)elapsed;
    (void)mode;
    return 0;
}

/* 0x00415220 starts the selected level from the front end's globals; the
 * port's entry takes the front end and the level. */
// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (frontend_start_level, no-op)
extern "C" void frontend_start_level_original(void)
{
}

/* ---- GAME-021 part 1 adapters, group D ---- */

/* Original-ABI adapters (ADR 0005 step 6) for GAME-021's lego_unit.c:
 * LevelLoad (0x00429890) and EntityEffectCountersApplyFromRecord
 * (0x0042b2f0) call these with the original's arguments. Natively nothing
 * calls them; the port loads levels through level_load above. */
struct Lego_Level;
void renderer_fog_color_set(float red, float green, float blue);
void renderer_fog_mode_set(unsigned int mode);
void renderer_fog_parameters_set(unsigned int start, unsigned int end,
                                 unsigned int density);

/* EntityCreateByType (FUN_00437d00): no port entry with the original's
 * contract (the port spawns through its GameContext); returns NULL. */
// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (no port equivalent of 0x00437d00)
extern "C" void* lego_object_create_adapter(void* objectData, int type,
                                            void* extra)
{
    (void)objectData;
    (void)type;
    (void)extra;
    return NULL;
}

/* EntityPoolFindByCallbackFiltered (FUN_004377b0): same contract; frozen
 * headers declare it with C++ linkage. */
// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (entity_pool_find_by_callback_filtered_current, 0x004377b0)
extern "C" int entity_pool_find_by_callback_filtered_current_original(
    int (*callback)(EntitySlot*, void*), void* payload)
{
    return entity_pool_find_by_callback_filtered_current(callback, payload);
}

/* FUN_0043a8f0 takes the object; the port's entry takes its context. */
// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (simulation_update_counter_add, 0x0043a8f0)
extern "C" void simulation_update_counter_add_original(EntitySlot* object,
                                                       int count)
{
    GameContext* context = object != NULL ? object->owner_ctx : NULL;
    if (context == NULL) context = entity_pool_callback_context_get();
    simulation_update_counter_add(context, count);
}

/* FUN_0043a910 takes the object; the port's entry takes an ignored mode
 * word in its place. */
// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (simulation_mode_counter_add, 0x0043a910)
extern "C" void simulation_mode_counter_add_original(EntitySlot* object,
                                                     int count)
{
    (void)object;
    simulation_mode_counter_add(0, count);
}

/* LevelIdentifierRemapTableLoad (FUN_0045d7e0): the port's loader is the
 * static level_identifier_remap_table_load above. */
// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (level_identifier_remap_table_load, 0x0045d7e0)
extern "C" int level_identifier_remap_table_load_original(
    const char* fileName, const char* gameName)
{
    return level_identifier_remap_table_load(fileName, gameName);
}

/* Container_SetFogColour/FogMode/FogParams (FUN_00473e10/70/a0) are in
 * view.cpp, which has no C declaration of them; the parameters entry takes
 * the floats' bit images. */
// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (renderer_fog_color_set, 0x00473e10)
extern "C" void renderer_fog_color_set_original(float r, float g, float b)
{
    renderer_fog_color_set(r, g, b);
}

/* container.c (Container.c, finding 624) serves these port names in the
 * matching build (container_link_names.h): every build but MSVC 5.0's
 * compiles the port's definitions. */
#if defined(_MSC_VER) && _MSC_VER < 1200
#define LRR_CONTAINER_PORT_NAMES 0
#else
#define LRR_CONTAINER_PORT_NAMES 1
#endif

// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (renderer_fog_mode_set, 0x00473e70)
#if LRR_CONTAINER_PORT_NAMES /* container.c serves renderer_fog_mode_set_original in the matching build */
extern "C" void renderer_fog_mode_set_original(unsigned int mode)
{
    renderer_fog_mode_set(mode);
}
#endif /* LRR_CONTAINER_PORT_NAMES */

// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (renderer_fog_parameters_set, 0x00473ea0)
#if LRR_CONTAINER_PORT_NAMES /* container.c serves renderer_fog_parameters_set_original in the matching build */
extern "C" void renderer_fog_parameters_set_original(float start, float end,
                                                     float density)
{
    renderer_fog_parameters_set(media_float_bits(start),
                                media_float_bits(end),
                                media_float_bits(density));
}
#endif /* LRR_CONTAINER_PORT_NAMES */

/* ---- GAME-021 part 1 adapters, group F ---- */

/* Original-ABI adapter (ADR 0005 step 6) for GAME-021's lego_unit.c: the
 * original FUN_00438960 (LegoObject.c) has no port entry; the port's
 * delayed-spawn probe stands in for it. */
// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (FUN_00438960 spawn probe)
extern "C" int spawn_registry_probe_adapter(int first, int second)
{
    return spawn_probe(first, second);
}

/* Original-ABI adapter (ADR 0005 step 6) for GAME-021's lego_unit.c: the
 * original FUN_00437880 (LegoObject_SetCustomName) has no port entry
 * (mainsim.cpp keeps it as a stub boundary). Nothing calls this natively,
 * so it does nothing. */
// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (LegoObject_SetCustomName 00437880)
extern "C" void entity_custom_name_set_adapter(EntitySlot* object,
                                               const char* name)
{
    (void)object;
    (void)name;
}

/* Original-ABI adapters (ADR 0005 step 6) for GAME-021's lego_unit.c debug
 * keys (0x004288e0). LegoObject.c's 0044a5d0, 00449f50 and 004403d0 are
 * namespaced ledger stubs in legoobject_port.cpp; 004471b0, 0044c150 and
 * 0043f6a0 take other parameter types or fewer parameters there; 00437d00
 * has no port entry. Nothing calls these natively. */
namespace unported_ledger_stubs {
int ledger_stub_0044a5d0_NearestOpenCellCoordSearch(
    EntitySlot* entity, int* output_x, int* output_y,
    int require_minimum_distance, int allow_maskc0, int allow_flag20);
void ledger_stub_00449f50_EntityTypeOperationDispatch(
    EntitySlot* entity, uint32_t operation);
} // namespace unported_ledger_stubs

// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (ledger stub 0044a5d0)
extern "C" int nearest_open_cell_search_adapter(EntitySlot* entity, int* x,
                                                int* y, int requireDistance,
                                                int allowMaskC0,
                                                int allowFlag20)
{
    return unported_ledger_stubs::
        ledger_stub_0044a5d0_NearestOpenCellCoordSearch(
            entity, x, y, requireDistance, allowMaskC0, allowFlag20);
}

/* LegoObject_Create (00437d00): no port entry; answers no object. */
// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (LegoObject_Create 00437d00)
extern "C" EntitySlot* object_create_from_container_adapter(void* container,
                                                            int type, int id)
{
    (void)container;
    (void)type;
    (void)id;
    return NULL;
}

// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (ledger stub 00449f50)
extern "C" void entity_type_operation_dispatch_adapter(EntitySlot* entity,
                                                       int operation)
{
    unported_ledger_stubs::ledger_stub_00449f50_EntityTypeOperationDispatch(
        entity, (uint32_t)operation);
}

/* The original takes the block coordinates as integers. */
// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (entity_try_create_linked_at_position 004471b0)
extern "C" int entity_try_create_linked_at_position_original(
    EntitySlot* entity, int x, int y, EntitySlot* linked)
{
    return lego::entity_try_create_linked_at_position(entity, (float)x, (float)y,
                                                      linked);
}

/* The original passes a float pair and a float scalar; the port's entry
 * stores the same 32-bit words. */
// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (entity_pair_scalar_fields_set 0044c150)
extern "C" int entity_pair_scalar_fields_set_original(EntitySlot* entity,
                                                      const float* pair,
                                                      float scalar)
{
    int32_t pair_words[2];
    int32_t scalar_word;
    memcpy(pair_words, pair, sizeof(pair_words));
    memcpy(&scalar_word, &scalar, sizeof(scalar_word));
    return entity_pair_scalar_fields_set(entity, pair_words, scalar_word);
}

/* The original takes the elapsed time as a fourth argument. */
// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (entity_damage_apply 0043f6a0)
extern "C" void entity_damage_apply_original(EntitySlot* entity, float damage,
                                             int emitEffects, float elapsed)
{
    (void)elapsed;
    entity_damage_apply(entity, damage, emitEffects);
}

/* The port's ledger stub takes a table of host operations; pass none. */
// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (ledger stub 004403d0)
extern "C" int level_debug_cell_retarget_adapter(EntitySlot* entity,
                                                 EntitySlot* target)
{
    return unported_ledger_stubs::
        ledger_stub_004403d0_LevelDebugCellRetargetDispatch(entity, target,
                                                            NULL);
}

/* ---- GAME-021 part 1 adapters, group E ---- */
/* Original-ABI adapters (ADR 0005 step 6) for GAME-021's lego_unit.c. The
 * callees live in legoobject/view.cpp, in Lego.c's parts 2-3, or have no
 * port entry at all; natively nothing calls these. */
void renderer_fog_enable_and_color_apply(int enabled);           /* view.cpp */
void renderer_fog_color_set(float red, float green, float blue); /* view.cpp */

// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (LegoObject_SetCustomName 0x00437880, no port entry)
extern "C" void lego_object_custom_name_set_adapter(struct EntitySlot* liveObj,
                                                    const char* name)
{
    /* No port entry with the original's contract: does nothing. */
    (void)liveObj;
    (void)name;
}

// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (entity_motion_input_accumulate, 0x0044a310)
extern "C" void lego_object_movement_input_adapter(struct EntitySlot* liveObj,
                                                   int forward, int strafe,
                                                   float rotate)
{
    entity_motion_input_accumulate(liveObj, forward, strafe, rotate);
}

// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (KeepRunning 0x00435940, no port entry)
extern "C" int lego_keep_running_adapter(void)
{
    /* No port entry with the original's contract (the port's
     * EngineKeepRunning takes the EngineContext): keep running. */
    return 1;
}

// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (QuitPoll 0x004589c0, no port entry)
extern "C" int lego_quit_poll_adapter(int a, int b, int* quit)
{
    /* No port entry: never quits. */
    (void)a;
    (void)b;
    (void)quit;
    return 0;
}

// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (renderer_fog_enable_and_color_apply, 0x00473dc0)
#if LRR_CONTAINER_PORT_NAMES /* container.c serves lego_fog_enable_adapter in the matching build */
extern "C" void lego_fog_enable_adapter(int on)
{
    renderer_fog_enable_and_color_apply(on);
}
#endif /* LRR_CONTAINER_PORT_NAMES */

// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (renderer_fog_color_set, 0x00473e10)
#if LRR_CONTAINER_PORT_NAMES /* container.c serves lego_fog_colour_set_adapter in the matching build */
extern "C" void lego_fog_colour_set_adapter(float r, float g, float b)
{
    renderer_fog_color_set(r, g, b);
}
#endif /* LRR_CONTAINER_PORT_NAMES */

// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (light_interface_call_slot_40, 0x00473460)
extern "C" void lego_light_penumbra_set_adapter(void* light, float value)
{
    uint32_t bits;
    memcpy(&bits, &value, sizeof(bits));
    light_interface_call_slot_40(light, bits);
}

// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (light_interface_call_slot_3c, 0x00473480)
extern "C" void lego_light_umbra_set_adapter(void* light, float value)
{
    uint32_t bits;
    memcpy(&bits, &value, sizeof(bits));
    light_interface_call_slot_3c(light, bits);
}

/* ---- GAME-021 part 1 adapters, group A ---- */
/* Original-ABI adapters (ADR 0005 step 6) for GAME-021's lego_unit.c:
 * Lego_Initialise (0x0041fb50) calls these. Each has the original's
 * signature. Where the port has no entry with this contract, the body is
 * minimal: natively nothing calls lego_game_config_init. */
#include "lego_unit.h"
#include "gods98/images.h" /* Area2F; not lego_unit_imports.h, whose Win32 Sound3D declaration clashes with simtick.hpp here */

// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (Container_Initialise 00472710, no port entry)
#if LRR_CONTAINER_PORT_NAMES /* container.c serves container_initialise_original in the matching build */
extern "C" void* container_initialise_original(const char* gameName)
{
    (void)gameName;
    return NULL;
}
#endif /* LRR_CONTAINER_PORT_NAMES */

// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (LegoObject_TriggerFrameCallback 0043f130, no port entry)
extern "C" void lego_object_trigger_frame_callback_adapter(void* cont, void* data)
{
    (void)cont;
    (void)data;
}

/* Text_Initialise (0046a950): in the matching build text.c defines this
 * name itself (text_link_names.h, finding 607). */
// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (message panel set-up 0046a950, no port entry)
#if !(defined(_MSC_VER) && _MSC_VER < 1200)
extern "C" void text_message_panel_initialise_adapter(const char* imageFile,
                                                      unsigned int x, unsigned int y,
                                                      unsigned int openY,
                                                      Area2F* messageWindow,
                                                      Area2F* imageWindow,
                                                      float timer)
{
    (void)imageFile; (void)x; (void)y; (void)openY;
    (void)messageWindow; (void)imageWindow; (void)timer;
}
#endif

// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (Front_PlayIntroMovie 00415910, no port entry with this contract)
extern "C" void front_play_intro_movie_adapter(const char* movieKey, int skippable)
{
    (void)movieKey;
    (void)skippable;
}

// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (Front_PlayIntroSplash 004157c0, no port entry)
extern "C" void front_play_intro_splash_adapter(const char* imageKey, int skippable,
                                                const char* timeKey)
{
    (void)imageKey; (void)skippable; (void)timeKey;
}

// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (Loader_Initialise 0044db50, no port entry with this contract)
extern "C" void loader_initialise_adapter(const char* loadScreenName,
                                          const char* shutdownScreenName,
                                          struct TextureSurface* font,
                                          const char* profileName,
                                          int progressDirection,
                                          const char* progressBarName,
                                          Area2F* progressWindow,
                                          const char* loadingText)
{
    (void)loadScreenName; (void)shutdownScreenName; (void)font;
    (void)profileName; (void)progressDirection; (void)progressBarName;
    (void)progressWindow; (void)loadingText;
}

// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (Lego_LoadLighting 00433770, scene_lights_build_from_config without host ops)
extern "C" int scene_lights_build_from_config_original(void)
{
    return scene_lights_build_from_config(NULL);
}

// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (Lego_LoadGraphicsSettings 004334f0; the port applies its settings elsewhere)
extern "C" int render_quality_config_apply_original(void)
{
    return 1;
}

// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (Stats_Initialise 004667e0, no port entry)
extern "C" void stats_initialise_adapter(struct BlockRec* config, const char* gameName)
{
    (void)config;
    (void)gameName;
}

/* dependencies.c serves this name in the matching build
 * (dependencies_link_names.h, finding 612). */
#if !(defined(_MSC_VER) && _MSC_VER < 1200)
// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (Dependencies_Initialise 0040ab70; the port's entry takes the level block)
extern "C" void dependencies_initialise_adapter(struct BlockRec* config,
                                                const char* gameName)
{
    (void)config;
    (void)gameName;
}
#endif

// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (the empty function at 00484c90, called with two arguments)
extern "C" void lego_noop_load_adapter(void* root, const char* name)
{
    (void)root;
    (void)name;
}

/* The matching build links GAME-012's Effect load (rock_fall_styles.c)
 * under this name (rock_fall_styles_link_names.h, finding 589). */
// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (RockFall_Initialise 0040bfd0; the port's entry takes a style table)
#if !(defined(_MSC_VER) && _MSC_VER < 1200)
extern "C" void rock_fall_initialise_adapter(struct BlockRec* config,
                                             const char* gameName, void* root)
{
    (void)config; (void)gameName; (void)root;
}
#endif

// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (Lego_LoadPanels 004345f0, frontend_config::panels_config_entry_apply)
extern "C" void panels_config_entry_apply_original(struct BlockRec* config, int width,
                                                   int height)
{
    frontend_config::panels_config_entry_apply(config, width, height);
}

// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (Lego_LoadPanelButtons 00434710, frontend_config::panel_button_layout_config_load)
extern "C" void panel_button_layout_config_load_original(struct BlockRec* config,
                                                         int width, int height)
{
    frontend_config::panel_button_layout_config_load(config, width, height);
}

// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (Lego_LoadTextMessages 00434af0, frontend_config::text_messages_config_load)
extern "C" void text_messages_config_load_original(struct BlockRec* config)
{
    frontend_config::text_messages_config_load(config);
}

// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (Front_LoadMenus 004161f0, no port entry)
extern "C" void front_load_menus_adapter(struct BlockRec* config)
{
    (void)config;
}

// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (Front_RunScreenMenuType 00416c80; the port's menu_enter takes the front end)
extern "C" void front_run_screen_menu_type_adapter(int menuType)
{
    (void)menuType;
}

// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (Front_GetSelectedLevel 00416d70; the port's entry takes the front end)
extern "C" const char* front_get_selected_level_adapter(void)
{
    return NULL;
}

// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (Main_GetStartLevel 00401c00; the buffer is the Win32 entry point's)
extern "C" const char* main_get_start_level_adapter(void)
{
    return NULL;
}

/* ==== GAME-021 part 2 (issue #574) adapters ==== */

/* ---- part 2 adapters, group 2A (0042b510-0042c440): begin ---- */

/* ---- part 2 adapters, group 2A: end ---- */

/* ---- part 2 adapters, group 2B (0042c480-0042d460): begin ---- */

// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (an empty function the linker folded to 00484c90, called with the block position)
extern "C" void lego_noop_block_adapter(const int* blockPos)
{
    (void)blockPos;
}

/* In the matching build vehicle_model.c's Vehicle_Load takes this name
 * (vehicle_model_link_names.h; finding 590). */
// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (Vehicle_Load 0046c6f0; no port entry has its contract, natively nothing calls this)
#if !(defined(_MSC_VER) && _MSC_VER < 1200)
extern "C" int lego_vehicle_load_adapter(void* vehicle, unsigned int objID,
                                         void* root, const char* filename,
                                         const char* gameName)
{
    (void)vehicle;
    (void)objID;
    (void)root;
    (void)filename;
    (void)gameName;
    return 0;
}
#endif

// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (Creature_Load 00406990; animation_object.c's entry adds a row kind, which it ignores)
extern "C" int lego_creature_load_adapter(void* creature, unsigned int objID,
                                          void* root, const char* filename,
                                          const char* gameName)
{
    return entity_type_config_resource_row_load(creature, objID, root,
                                                 filename, (void*)gameName,
                                                 0u);
}

/* ---- part 2 adapters, group 2B: end ---- */

/* ---- part 2 adapters, group 2C (0042d600-0042dff0): begin ---- */

/* ---- part 2 adapters, group 2C: end ---- */

/* ---- part 2 adapters, group 2D (0042e020-0042ef40): begin ---- */

/* Original-ABI adapters (ADR 0005 step 6) for GAME-021 part 2's
 * lego_object_list_load_file (0x0042e020). Nothing calls them natively. */

/* LegoObject_RegisterHidden (FUN_00437c20) takes the object model, type,
 * index, world position, heading, health and the two object-list names; the
 * port's queue takes the game context and the position and floats as words. */
// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (deferred_olist_entity_queue_add 00437c20; the port takes the GameContext and word-typed floats)
extern "C" void deferred_olist_entity_queue_add_original(
    void* objModel, int objType, unsigned int objID, const float* position,
    float heading, float health, const char* olName,
    const char* drivingName)
{
    uint32_t world[2];
    int heading_bits;
    int health_bits;
    (void)objModel;
    memcpy(world, position, sizeof(world));
    memcpy(&heading_bits, &heading, sizeof(heading_bits));
    memcpy(&health_bits, &health, sizeof(health_bits));
    deferred_olist_entity_queue_add(entity_pool_callback_context_get(), world,
                                    0, objType, (int)objID, heading_bits,
                                    health_bits, olName, drivingName);
}

/* FUN_0043f100 takes (object, driven object) and clears the route bit on
 * the second and its link; the port's entry takes the game context first. */
// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (entity_linked_route_flag_clear 0043f100; the port takes the GameContext first)
extern "C" void entity_linked_route_flag_clear_original(EntitySlot* object,
                                                        EntitySlot* driven)
{
    (void)object;
    entity_linked_route_flag_clear(entity_pool_callback_context_get(), driven);
}

/* ---- part 2 adapters, group 2D: end ---- */

/* ---- part 2 adapters, group 2E (0042efc0-0042f6f0): begin ---- */

/* 00435aa0 (Lego.c part 3) takes the flag alone; the port's entry also
 * takes the EngineContext, which it does not read. */
// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (00435aa0; the port's entry takes the EngineContext)
extern "C" void lego_panel_selection_highlight_toggle_adapter(int on)
{
    panel_selection_highlight_toggle(NULL, on);
}

/* 00456c60 takes no argument (it frees the global pool lists); the port's
 * object_pool_lists_free_all takes the pool table, which the port's level
 * owner keeps. No port entry frees the original's: does nothing. */
// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (00456c60; the port's entry takes the pool table)
extern "C" void lego_object_pool_lists_free_all_adapter(void)
{
}

/* 00484c90 is an empty function; the original also calls it with a block
 * position. */
// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (00484c90 called with an argument)
extern "C" void lego_noop_point_adapter(const int* blockPos)
{
    (void)blockPos;
}

/* ---- part 2 adapters, group 2E: end ---- */

/* ---- part 2 adapters, group 2F (004302b0-00431570): begin ---- */

/* 00439370 and 004394b0 are declared only in legoobject_port.cpp's ledger
 * namespace, with C++ linkage; these forward the original's arguments. */
namespace unported_ledger_stubs {
void ledger_stub_00439370_EntityBuildPlacementCommandSetup(
    const int cell_xy[2], int entity_id, const float world_xy[2],
    int update_state);
void ledger_stub_004394b0_EntityBuildPlacementAlternateCommandSetup(
    const int cell_xy[2], int entity_id, const float world_xy[2],
    int update_state);
} // namespace unported_ledger_stubs

// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (EntityBuildPlacementCommandSetup 00439370; C++ linkage in legoobject_port.cpp's ledger namespace)
extern "C" void entity_build_placement_setup_original(
    const int cell_xy[2], int entity_id, const float world_xy[2],
    int update_state)
{
    unported_ledger_stubs::ledger_stub_00439370_EntityBuildPlacementCommandSetup(
        cell_xy, entity_id, world_xy, update_state);
}

// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (EntityBuildPlacementAlternateCommandSetup 004394b0; C++ linkage in legoobject_port.cpp's ledger namespace)
extern "C" void entity_build_placement_alt_setup_original(
    const int cell_xy[2], int entity_id, const float world_xy[2],
    int update_state)
{
    unported_ledger_stubs::ledger_stub_004394b0_EntityBuildPlacementAlternateCommandSetup(
        cell_xy, entity_id, world_xy, update_state);
}

/* ---- part 2 adapters, group 2F: end ---- */

/* ---- part 2 adapters, group 2G (00431580-00432490): begin ---- */
/* Original-ABI adapters (ADR 0005 step 6) for GAME-021's lego_unit.c,
 * functions 00431580-00432490. Natively nothing calls them: the unit
 * functions that do run only in the matching build. */

/* LegoObject_GetBlockPos (FUN_004424f0) returns whether the object's
 * position is on the map; the port's entity_position_to_terrain_cell returns
 * nothing, so this asks the terrain picker itself. */
// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (LegoObject_GetBlockPos 004424f0; the port's entry takes the level and returns nothing)
extern "C" int entity_block_pos_get_adapter(const struct EntitySlot* entity,
                                            int* bx, int* by)
{
    if (entity == NULL || g_active_level_retail == NULL) return 0;
    return terrain_world_xy_to_cell(g_active_level_retail->ptr_terrain,
                                    entity->x, entity->y, bx, by) != 0;
}

/* ---- part 2 adapters, group 2G: end ---- */

/* ---- part 3 adapters, group 3A (004324d0-00433330): begin ---- */

/* ---- part 3 adapters, group 3A: end ---- */

/* ---- part 3 adapters, group 3B (00433380-00434530): begin ---- */

/* Original-ABI adapters (ADR 0005 step 6) for GAME-021's lego_unit.c,
 * functions 00433380-00434530. Natively nothing calls them: the unit
 * functions that do run only in the matching build. */

void render_surface_get_value_pair_80_and_84(void* surface,
                                             uint32_t* out_80,
                                             uint32_t* out_84); /* viewports.cpp, 00476f10 */
namespace lego {
struct DDiSceneObjectDescriptor;
void* ddiscene_object_allocate(const DDiSceneObjectDescriptor* descriptor); /* container_unit.cpp, 00472950 */
}
namespace unported_ledger_stubs {
int ledger_stub_00444260_EntityRandomCellStepRouteIssue(EntitySlot* entity); /* legoobject_port.cpp */
}

/* FUN_00462f20 (Roof_SetBlockRoofVertices) takes the block and its four
 * corners; the port's TerrainCellQuadGeometryEmit takes a grid and a packed
 * array, with no grid for the current level. This does nothing. */
// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (00462f20; no port entry with the original's contract)
extern "C" void lego_roof_block_vertices_set_adapter(unsigned int bx,
                                                     unsigned int by,
                                                     struct TerrainMapVector3* v0,
                                                     struct TerrainMapVector3* v1,
                                                     struct TerrainMapVector3* v2,
                                                     struct TerrainMapVector3* v3)
{
    (void)bx; (void)by; (void)v0; (void)v1; (void)v2; (void)v3;
}

/* FUN_00478330 (Main_Setup3D) takes the render quality and six switches;
 * the port's direct3d_render_preferences_apply takes another set of five.
 * This does nothing. */
// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (00478330; the port's entry takes other parameters)
extern "C" void lego_main_setup3d_adapter(int quality, int dither, int filter,
                                          int mipmap, int linearMipMap,
                                          int blend, int sort)
{
    (void)quality; (void)dither; (void)filter; (void)mipmap;
    (void)linearMipMap; (void)blend; (void)sort;
}

/* FUN_00472950 (Container_Create): the parent container is the port's
 * scene-object descriptor (its +0/+8 frames). */
// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (00472950; C++ linkage in namespace lego)
extern "C" void* lego_container_create_adapter(void* parent)
{
    return lego::ddiscene_object_allocate(
        (const lego::DDiSceneObjectDescriptor*)parent);
}

/* FUN_00477270 (Viewport_GetField) returns the field of view; the port's
 * naked entry is declared void. This returns 0. */
// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (00477270; the port's entry returns nothing)
extern "C" float lego_viewport_field_get_adapter(void* view)
{
    (void)view;
    return 0.0f;
}

// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (00476f10; C++ linkage in viewports.cpp)
extern "C" void lego_viewport_size_get_adapter(void* view, unsigned int* width,
                                               unsigned int* height)
{
    render_surface_get_value_pair_80_and_84(view, width, height);
}

/* FUN_0040b470 (Detail's projected block area) takes the corners, the
 * viewport, the screen size and the no-clip switch; the port's entry takes a
 * projection adapter first. This returns 0 (no area). In the matching build
 * detail.c defines this name (detail_link_names.h, finding 615). */
#if !(defined(_MSC_VER) && _MSC_VER < 1200)
// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (0040b470; the port's entry takes a projection adapter)
extern "C" float lego_projected_quad_area_adapter(struct TerrainMapVector3* vertices,
                                                  void* view,
                                                  const float screen[2],
                                                  int noClip)
{
    (void)vertices; (void)view; (void)screen; (void)noClip;
    return 0.0f;
}
#endif

// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (00444260; C++ linkage in legoobject_port.cpp's ledger namespace)
extern "C" int lego_entity_random_cell_step_route_adapter(struct EntitySlot* object)
{
    return unported_ledger_stubs::ledger_stub_00444260_EntityRandomCellStepRouteIssue(
        object);
}

/* In the matching build detail.c defines this name (detail_link_names.h,
 * finding 615). */
#if !(defined(_MSC_VER) && _MSC_VER < 1200)
// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (0040b810; the port's entry takes the ledger namespace's channel table)
extern "C" void lego_detail_mesh_resources_clear_adapter(void* mesh)
{
    unported_ledger_stubs::cell_visual_channel_table_resources_clear(
        (unported_ledger_stubs::CellVisualChannelTable*)mesh);
}
#endif

/* ---- part 3 adapters, group 3B: end ---- */

/* ---- part 3 adapters, group 3C (00434570-00435aa0): begin ---- */

/* ---- part 3 adapters, group 3C: end ---- */
