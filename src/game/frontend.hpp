// SPDX-License-Identifier: LGPL-3.0-or-later
/* Game layer — front end / main menu (finding 23).
 *
 * Reconstruction of the corpus's front-end machine so the port boots into
 * the main menu before any level loads:
 *
 *   MenuBuild @ 0x4140c0         — config-driven menu constructor. Reads the
 *                                  `Lego*` config's `Menu::<name>` groups
 *                                  (MenuCount, per-`Menu%i` keys, `Item%i`
 *                                  records) into the FeMenu model below.
 *   MenuRunLoop @ 0x413b80       — the modal loop (blocks while a menu is
 *                                  open). frontend_tick is its "one frame"
 *                                  face for the port's frame loop.
 *   FrontEndActive @ 0x416d00    — frontend_enabled(): the `Lego* Main
 *                                  FrontEnd` cfg gate (loader-mode <3 is a
 *                                  Win32-side notion; we keep the flag).
 *   FrontEndSelectedAction @ 0x416d70 / LevelChainBuild @ 0x4154b0 —
 *   the level pickers: StartLevel / TutorialStartLevel + NextLevel chains
 *   become the picker sub-menu rows (frontend_build grafts them).
 *
 * Item grammar (MenuItemType @ 0x413e60 + finding 23): an `Item%i` value is
 * "<Kind>:<args>" — Next:x:y:label:Menu%i, Trigger:x:y:label:flag,
 * Cycle/Slider/RealSlider/TextInput. The port drives selection with the
 * keyboard (the corpus's pointer selection comes later); rendering is a
 * software blit of the item labels with the port's own 3x5 font (the
 * shipped menus are bitmap-font BMP surfaces — our palette/font is ours,
 * README "Software level renderer").
 *
 * Discipline (ADR 0003/0004): plain C-shaped C++, no OS headers, no STL.
 */
#ifndef LEGORRR_GAME_FRONTEND_HPP
#define LEGORRR_GAME_FRONTEND_HPP

#include <cstdint>
#include <cstddef>

#include "gods98/block.hpp"
#include "gods98/texture.hpp"
#include "options.hpp"

#ifdef __cplusplus
extern "C" {
#endif

struct Flic; /* gods98/flic.hpp — `.flh` overlay player (lazily opened) */
struct AudioPlayer; /* gods98/audio.hpp — optional front-end SFX output */

#define FE_ITEMS_MAX      32   /* ItemCount cap — ≥ the 24-level campaign picker */
#define FE_SUBMENUS_MAX   8    /* MenuCount cap */
#define FE_OVERLAYS_MAX   5    /* per-submenu `!Overlay%i` cap */
#define FE_NAME_MAX       48
#define FE_PATH_MAX       128
#define FE_LEVEL_NAME_MAX 48
#define FE_CHAIN_MAX      24   /* StartLevel/TutorialStartLevel + NextLevel */

/* Bitmap-font layout (FUN_0047a470): a 10-column x 19-row ASCII grid
 * starting at 0x20 (" ") — glyph g = ch-0x20 at (col = g%10, row = g/10),
 * cell = image width/10 x height/19. Menu_Font_Hi.bmp is 380x817 -> 38x43. */
#define FE_FONT_GRID_COLS 10
#define FE_FONT_GRID_ROWS 19
/* The colour keys are per font — the corpus keys BOTH palette entries 0 and
 * 255 of each 8bpp face (FUN_0047d9d0 -> FUN_0047e050; Menu_Font* pal-255 =
 * pure red, MbriefFont* = white). texture_decode records them on the surface
 * (keys[]/key_count) and fe_font_keys() passes them through; surfaces with
 * no palette info fall back to the dominant-colour sample of glyph 0's cell
 * (port-side stand-in). FE_FONT_KEY is the generic default. Glyphs blit as
 * their measured ink box (non-key pixels only), which keeps both key
 * colours and the cells' frame strips out of the render. */
#define FE_FONT_KEY       0xffff0000u

/* Item kinds — MenuItemType @ 0x413e60 (values 0..5; 5 = the "Next" catch-
 * all, 1 = Trigger). Slider/Cycle adjust the option store behind `fe.opts`
 * when one is attached (issue #85); TextInput parses its label only. */
#define FE_ITEM_CYCLE      0
#define FE_ITEM_TRIGGER    1
#define FE_ITEM_TEXTINPUT  2
#define FE_ITEM_SLIDER     3
#define FE_ITEM_REALSLIDER 4
#define FE_ITEM_NEXT       5

/* The four top-level menu groups (LoadMenuBuild @ 0x4161f0 addresses them
 * as Menu::MainMenuFull / Menu::SaveMenu / Menu::PausedMenu / Menu::OptionsMenu). */
#define FE_MENU_MAIN    0
#define FE_MENU_SAVE    1
#define FE_MENU_PAUSED  2
#define FE_MENU_OPTIONS 3
#define FE_MENU_COUNT   4

typedef struct FeItem {
    int   kind;
    int   x, y;      /* cfg anchor (or per-level FrontEndX/Y for picker rows) */
    char  label[FE_NAME_MAX];  /* display text (underscores shown as spaces) */
    char  target[FE_NAME_MAX]; /* "Menu%i" target, or the level group name    */
    int   flag;                /* Trigger flag / Cycle count / slider lo      */
    int   level_start;         /* 1 = grafted picker row: Enter starts a level */
    int   load_slot;           /* 1 = grafted save-slot row: Enter loads it    */
    /* Slider / Cycle interaction state (Options menus). `value` lives in
     * [min, max]; cycles min=0, max=count-1 and carry their option texts. */
    int   min, max, value;
    int   cycle_count;
    char  cycle_opts[4][24];
} FeItem;

/* A front-end FLIC overlay (`!Overlay%i <avi\X.flh>:<SFX>:x:y`, finding
 * 23 / 14 §5): a small .flh animation blitted onto the menu backdrop. */
typedef struct FeOverlay {
    char  path[FE_PATH_MAX];
    char  sfx[FE_NAME_MAX];
    int   x, y;
    Flic* flic;   /* opened player, or NULL (missing file / not started) */
    int   tried;  /* non-random overlays are only opened once per menu visit */
} FeOverlay;

typedef struct FeSubmenu {
    int   item_count;             /* cfg ItemCount + grafted chain rows */
    FeItem items[FE_ITEMS_MAX];
    char  title[FE_NAME_MAX];     /* Title key (underscored display text) */
    char  image[FE_PATH_MAX];     /* MenuImage / MenuImageDark (render lane) */
    char  lofont[FE_PATH_MAX];    /* LoFont key — unselected items (Menu_Font_Lo.bmp) */
    char  hifont[FE_PATH_MAX];    /* HiFont key — selected items  (Menu_Font_Hi.bmp) */
    int   base_x, base_y;         /* Position key (x:y) */
    int   title_anchor_y;         /* Anchored x:y; title adds y to Position.y */
    int   auto_center;            /* AutoCenter TRUE|FALSE */
    int   display_title;          /* DisplayTitle TRUE|FALSE */
    int   play_random;            /* PlayRandom: random sequential FLIC playback */
    FeOverlay overlays[FE_OVERLAYS_MAX];
    int   overlay_count;
    int   can_scroll;             /* CanScroll TRUE — item plane pans (0x413970) */
    int   scroll;                 /* plane offset in px (<= 0; reset on entry) */
    int   scroll_extent;          /* content height = tallest item + row height */
} FeSubmenu;

typedef struct FeMenu {
    char      name[FE_NAME_MAX];  /* "MainMenuFull" & co. */
    int       submenu_count;      /* MenuCount */
    FeSubmenu submenus[FE_SUBMENUS_MAX];
} FeMenu;

#define FE_SLOT_COUNT 6    /* SAVE_SLOTS */

/* A runtime save slot row (SaveMenuBuild @ 0x415a10 builds these from the
 * `Save_Game_%i` keys): the slot's label + whether it has any records. */
typedef struct FeSlot {
    int   id;
    char  label[FE_NAME_MAX];
    int   present;
} FeSlot;

typedef struct FrontEnd {
    BlockFile* cfg;               /* Lego.cfg DB (not owned) */
    FeMenu  menus[FE_MENU_COUNT];   /* main / save / paused / options */
    int     built;                  /* 1 = frontend_build succeeded      */

    /* StartLevel / TutorialStartLevel chains (LevelChainBuild @ 0x4154b0).
     * campaign  = StartLevel + NextLevel; tutorial = TutorialStartLevel. */
    int  campaign_n, tutorial_n;
    char campaign[FE_CHAIN_MAX][FE_LEVEL_NAME_MAX];
    char tutorial[FE_CHAIN_MAX][FE_LEVEL_NAME_MAX];
    char campaign_label[FE_CHAIN_MAX][FE_NAME_MAX];
    char tutorial_label[FE_CHAIN_MAX][FE_NAME_MAX];

    /* Runtime save/load slot rows (grafted onto the SaveLoad submenus by
     * frontend_build_slots). */
    int    slot_count;
    FeSlot slots[FE_SLOT_COUNT];

    /* --- runtime (modal menu) state ------------------------------------ */
    int  open;                      /* 1 while the menu loop is showing   */
    int  menu;                      /* FE_MENU_* (MAIN / PAUSED reachable) */
    int  sub;                       /* active submenu index               */
    int  sel;                       /* selected item index                */
    int  quit;                      /* user chose Quit (Yes on the confirm) */
    int  load_slot;                 /* FE_EVT_LOAD_GAME target slot id     */
    int  option_changed;            /* 1: a Slider/Cycle value changed this tick */
    char pending_level[FE_LEVEL_NAME_MAX]; /* level group to load (no "Levels::") */
    char resume_level[FE_LEVEL_NAME_MAX];  /* current level (pause Restart)   */

    /* The functional option store (issue #85). When set, every option
     * Slider/Cycle's displayed value seeds from it (build/open) and every
     * adjustment writes back into it — the menu shows the *effective* value,
     * never a reset default. NULL = classic display-only menu (values keep
     * the per-banner fe_option_default). */
    GameOptions* opts;

    /* Overlay players open for the currently active submenu (lazily). */
    const FeSubmenu* overlays_for;  /* which submenu's flics are open     */
    int  overlay_active;            /* selected overlay index, or -1       */
    uint32_t overlay_rng;           /* per-front-end LCG state              */
    struct AudioPlayer* audio;      /* optional SFX player                  */

    /* CanScroll plane pan lives on each FeSubmenu (corpus DAT_005574e0
     * is reset when a menu re-enters — frontend_open clears every sub). */
    int  view_w, view_h;            /* render viewport (set each render)  */
    int  pointer_x, pointer_y;      /* last menu pointer position         */
} FrontEnd;

/* One frame of modal-menu input: key *edges* this frame (the frame loop
 * derives these from input_key_edge; tests drive them directly) plus the
 * pointer. When mouse coords are >= 0 the cursor follows the items (the
 * hovered row becomes the selection); `click` activates it like Enter. */
typedef struct FeInput {
    int up, down;     /* move selection */
    int left, right;  /* adjust a Slider/Cycle value */
    int enter;        /* activate selection (keyboard) */
    int esc;          /* back one submenu / close */
    int mouse_x;      /* pointer x (>= 0 = mouse in use) */
    int mouse_y;      /* pointer y */
    int click;        /* left-button press edge this frame */
    int mdown;        /* left button held (level state) — slider drag */
} FeInput;

typedef enum {
    FE_EVT_NONE = 0,
    FE_EVT_START_LEVEL,   /* frontend.pending_level holds the level group */
    FE_EVT_LOAD_GAME,     /* frontend.load_slot holds the save slot id    */
    FE_EVT_RESUME,        /* close a modal overlay (pause Continue / Esc)   */
    FE_EVT_CREDITS,       /* show the main-menu scrolling credits view      */
    FE_EVT_QUIT,          /* user quit the app from the front end */
} FeEvent;

/* --- lifecycle ---------------------------------------------------------- */

void frontend_init(FrontEnd* fe);

/* Parse the four menus from `cfg` (Menus FE_MENU_*), then graft the level
 * pickers (campaign/tutorial chains) onto the sub-menus whose MenuImage
 * names the level-picker art. Returns 1 when MainMenuFull parsed. The
 * level chain is populated from StartLevel/TutorialStartLevel + NextLevel. */
int  frontend_build(FrontEnd* fe, BlockFile* cfg);

/* Sync the option widgets with `fe->opts` (issue #85): seed every option
 * Slider/Cycle value from the store, clamped to the item's cfg range. Called
 * on build (when `opts` is already attached) and on every frontend_open, so
 * re-entering Options always shows the effective value. */
void frontend_options_sync(FrontEnd* fe);

/* The FrontEndActive gate: `Lego* Main FrontEnd` cfg flag (finding 23).
 * (The corpus ANDs the loader-mode check DAT_00505f88 < 3; the port only
 * tracks the flag — the loader-mode notion is Win32-side.) */
int  frontend_enabled(BlockFile* cfg);

/* Checkpoint record <-> level mapping (the port's stand-in for the corpus's
 * sav-tree node record slots, SavTreeWalk2 / CheckpointStep's ListFindName
 * index): tutorials take records 0..tutorial_n-1, campaign levels follow
 * (tutorial_n..tutorial_n+campaign_n-1). Returns -1 / 0 for unknowns so
 * callers keep the record-0 default. */
int  fe_record_for_level(const FrontEnd* fe, const char* level);
int  fe_level_for_record(const FrontEnd* fe, int rec, char* out, size_t sz);

/* Enter the modal menu at `menu`/submenu 0, selection 0. */
int  frontend_open(FrontEnd* fe, int menu);
void frontend_close(FrontEnd* fe);
void frontend_set_audio(FrontEnd* fe, struct AudioPlayer* audio);

/* Register the runtime save/load slot rows (from save_slot_state) and graft
 * them onto the SaveLoad submenus (MenuImage contains "SaveLoad" — the
 * shipped Load_A_Saved_Game screens), after the cfg items. Selecting a row
 * returns FE_EVT_LOAD_GAME with fe.load_slot = the slot id. */
int  frontend_build_slots(FrontEnd* fe, const FeSlot* slots, int n);

/* Step the modal menu one frame. Returns the event the frame loop must act
 * on; FE_EVT_NONE while the menu stays up. `measure` is the submenu's
 * HiFont texture (or NULL for the 3x5 fallback width) used for hit-testing
 * the pointer. See frontend_tick in the .cpp for the action table
 * (level picker -> start, Trigger flag -> quit, Next -> navigate submenus,
 * Esc -> back/close). Pointer hover sets the selection; a click activates
 * the hovered row like Enter. */
FeEvent frontend_tick(FrontEnd* fe, const FeInput* in,
                      const TextureSurface* measure);

/* --- accessors ---------------------------------------------------------- */

const FeMenu* frontend_menu(const FrontEnd* fe, int menu);
const FeSubmenu* frontend_active_submenu(const FrontEnd* fe);
const FeItem* frontend_active_item(const FrontEnd* fe);

/* The active submenu's MenuImage path (Menu::<name> <Menu%i> MenuImage,
 * ":0:0:1" trimmed), or NULL when the submenu has none — the frame loop
 * uses it to blit the background art behind the menu layer. */
const char* frontend_active_image(const FrontEnd* fe);

/* --- render (finding 22 §3 lane) ---------------------------------------- */

/* Draw the active submenu into a locked RGBA32 framebuffer (R,G,B,A bytes):
 * item labels at their anchors (AutoCenter → centered on base_x), a
 * selected font for the active row, the pointer graphic, and the Title when
 * display_title. The background / MenuImage blit is the caller's (it owns
 * the texture lane). `hi_font`/`lo_font` are the shipped bitmap fonts
 * (HiFont = selected, LoFont = unselected — the engine keys palette 255
 * pure red transparent); when NULL the port's own 3x5 typeface is used as
 * a fallback. Map labels replace '_' with ' '.
 *
 * Bitmap fonts (FUN_0047a470, finding-23 session): a 10-column x 19-row
 * ASCII grid starting at 0x20 — glyph g = ch-0x20 at row g/10, col g%10;
 * cell size = font width/10 x font height/19. Advance width is measured
 * from each glyph's ink bounding box (the engine stores per-glyph float
 * advances in the font object; we re-derive them from the image). */
void frontend_render(FrontEnd* fe, const TextureSurface* hi_font,
                     const TextureSurface* lo_font,
                     int w, int h, void* px, int pitch);

/* Draw one frame of the scrolling credits lane over the caller's backdrop.
 * scroll_px advances one pixel per frame; the text cycles from below the
 * screen back to the bottom after the final line leaves the top. */
void frontend_render_credits(const TextureSurface* font,
                             const char* const* lines, int line_count,
                             int scroll_px, int w, int h, void* px, int pitch);

/* The in-game Help Window (issue #85): the panel the Help_Window option
 * toggles. The option is checked here — the display gate — so every caller
 * shares it and the panel cannot draw while the store says Off. `text`
 * (NULL for the demo key lines) fills the info lines. */
void frontend_help_overlay(void* px, int pitch, int w, int h,
                           const char* text, const GameOptions* opts);

/* CurrentSelectionContextGet / CurrentSelectionContextSet (FUN_00417fe0 /
 * FUN_00417ff0, finding 376): get / set accessors over the corpus's
 * current-selection-context global. Its corpus writers (the frontend
 * state-change leaf — unported — and the menu-build leaf, modeled without
 * storing) leave it unset, so the context answers NULL in port cases
 * (documented deviation). */
void* current_selection_context_get(void);
void current_selection_context_set(void* context);

#ifdef __cplusplus
}
#endif

/* --- selection-state band (2026-10-24) ------------------------------------
 * SelectionStateFlagClearBit0 / SelectionStateHandleGet (FUN_004181a0 /
 * FUN_00419150, findings 380/478): the corpus's selection-state word
 * (DAT_004dc650) — its bit-0 clear leaf and the mode/index-gated handle
 * leaf over the strided handle tables (DAT_004db914/918, stride 0x1c) and
 * the extra handle (DAT_004db934). No ported code feeds this band — the
 * UI-mode resource positions / hit-test leaves that write the tables,
 * hit index, and state bits are unported — so everything lands
 * zero-initialized and the corpus's blind table indexing is guarded to a
 * documented 16-entry window (deviation). The set/peek functions are the
 * tests/diagnostics view (the message_active_count precedent); tests also
 * inject through them. */
void selection_state_flag_clear_bit0(void);
uint32_t selection_state_handle_get(int index);

uint32_t selection_state_flags_peek(void);

/* SelectionModeStateSet (FUN_00418190, finding 478): the corpus stores the
 * selection-mode word (DAT_004db8e8) beside the selection-state word this
 * band models; no ported feeder, so zero-init storage with a peek. */
void     selection_mode_state_set(uint32_t value);
uint32_t selection_mode_state_peek(void);

/* UiModeResourceBit40Get (FUN_004191f0, findings 443/474): bit 0x40 of the
 * selection-state word — the same word the band above models (this leaf
 * reads it through the corpus's mask). */
uint32_t ui_mode_resource_bit40_get(void);
void selection_state_flags_set(uint32_t flags);       /* diagnostics */
int selection_state_hit_index_peek(void);
void selection_state_hit_index_set(int index);        /* diagnostics */
uint32_t selection_state_handle_extra_peek(void);
void selection_state_handle_extra_set(uint32_t value);/* diagnostics */
uint32_t selection_state_handle_slot_peek(int table, int index);
void selection_state_handle_slot_set(int table, int index, uint32_t value);

/* --- band-mode / timed-effect / positions (2026-10-24) ----------------------
 * TimedEffectUpdateRequestSet (FUN_0046b770, findings 228/05): raises the
 * update flag (bit 2) of a timed-effect entry. The band storage lives
 * with the unported timed-effect update family — the port models each
 * entry's flag word inside a guarded 16-entry window (documented bounds
 * deviation), with a peek view for tests.
 * UiModeTransitionRequest (FUN_00418fc0, finding 250): the band mode word
 * (DAT_004dc64c, modeled with the selection-state band) transition —
 * the zero-second argument arms mode 4, mode 4 hands over to the requested
 * index (raising the band's bit 5), and a same-index request just raises
 * the bit. The answer reports whether the requested index is now active.
 * SelectionModeSetByIndex (FUN_00418f80, finding 250): the two-entry
 * switch — raises the timed-effect update flags for the band modes.
 * UiModeResourcePositionsApply (FUN_00419100, finding 250): walks the
 * two-entry position table (the corpus writes the table from the
 * unported help-window layout loader — zero-init statics here), and the
 * reposition hook per found handle is the simtick-cited surface seam. */
void timed_effect_update_request_set(int index);
uint32_t timed_effect_flags_peek(int index);           /* diagnostics */
bool     ui_mode_transition_request(int index, int p2, int p3);
int      band_mode_peek(void);                        /* diagnostics */
void     band_mode_set(int mode);                     /* diagnostics */
void     selection_mode_set_by_index(int index);
void     ui_mode_resource_positions_apply(void);
void     layout_position_set(int entry, int x, int y); /* diagnostics */
int      layout_position_get(int entry, int* x, int* y);

/* --- the front-end flow words (2026-10-24; findings 23/438) -----------------
 * The corpus's five DAT_0055701c/20/24/48/4c menu-flow words, modeled as
 * zero-initialized module statics per the unowned-global convention (the
 * corpus's writers — OptionsLoadOrDefault's reset and the menu-callback
 * bindings — are unported). At the corpus's load image all five read zero,
 * and SaveSlotOpen/IsInGame test `!= -1`, so both report true exactly as
 * the corpus does before the save/tutorial selections initialize (the
 * assigned IsInGame label is a selection sentinel per finding 438).
 * The port-side setter stands in for the unported callback bindings. */
int save_slot_open(void);       /* FUN_00416d50: DAT_00557020 != -1 */
int is_in_game(void);           /* FUN_00416d60: DAT_00557024 != -1 */
int quit_menu_probe(void);      /* FUN_00416dd0: DAT_0055701c verbatim */
int level_start_flag(void);     /* FUN_00416de0: DAT_00557048 != 0 */
int menu_options_flag(void);    /* FUN_00416df0: DAT_0055704c != 0 */
void frontend_flow_word_set(int which, int value); /* port-side setter */

/* MainSimStateAdmissionPredicate (FUN_004191a0): the corpus's sim-admission
 * gate — selection-state bits 1 and 2 set, bit 0x40 raised through the
 * leaf above, and the message module's system-state word clear. All three
 * models are the port's own (the same s_sel_state the selection band
 * reads and the message_system_state model), so no new storage lands. */
int main_sim_state_admission_predicate(void);

/* MenuWipeEnd (FUN_00410440): clears the wipe flag's low two bits — the
 * corpus's word DAT_005574f0, whose setter half (wipe start, setting 3)
 * lives in the unported menu-wipe driver. Zero-init static with a peek
 * stands in (deviation, documented in frontend.cpp). */
void     menu_wipe_end(void);
uint32_t menu_wipe_flags_peek(void);
void     menu_wipe_flags_set(uint32_t flags); /* port hook */

/* --- the selection-region band (2026-10-24; findings 06/250/478) ------------
 * The corpus's selection/UI-state words beside the selection-state word
 * the band above models: the coordinate quad (DAT_004ddd0c/10/14/18), the
 * region parameters (DAT_004ddce0…cf0), the objective-selection pair
 * (DAT_004ddccc/d0 — reset mirrors DAT_004ddce8) and the interaction
 * timer + flag word (DAT_004de1ec/1f8). No ported feeder owns them, so
 * everything lands zero-initialized (deviation) with port-side peeks.
 * ModeFlagPairApplyIfBit2 (0x418800) applies the corpus's mode rule to
 * the selection-state word this band already models. */
void selection_coordinates_set(const uint32_t* pair, uint32_t x, uint32_t y);
/* selection_coordinates_set (FUN_0041cce0): the pointer's two dwords go
 * to the +0c/+10 slots; x/y arrive as integers and convert to floats for
 * the +14/+18 slots — the corpus's int->float casts, faithful. */
void selection_region_parameters_set(float a, float b, float c, float d,
                                     float e);
/* selection_region_parameters_set (FUN_0041e9d0): five region slots, the
 * last scaled by 0.04 (the corpus's float multiply). */
void objective_selection_state_reset(void);
/* objective_selection_state_reset (FUN_0041a990): clears the selection
 * word, mirrors the region's +08 float into the tracking word, and raises
 * interaction-flag bits 0x84. */
void interaction_timer_reset_and_flag_clear(void);
/* interaction_timer_reset_and_flag_clear (FUN_0041ee60): restocks the
 * interaction timer (25.0) and clears bit 0x800. */
void mode_flag_pair_apply_if_bit2(int value);
/* mode_flag_pair_apply_if_bit2 (FUN_00418800): with bit 2 of the
 * selection word set, nonzero arms bits (1 set, 0 clear) and zero drops
 * both. */
float    selection_coordinates_peek(int which); /* 0..3 */
void     selection_region_parameters_peek(float out[5]);
uint32_t objective_selection_word_peek(void);   /* port hook */
float    interaction_timer_peek(void);          /* port hook */
uint32_t interaction_flags_peek(void);          /* port hook */

/* --- the cell-action permission window (2026-10-24; findings 250/419) --------
 * CellActionPermissionGet (FUN_0041e990): while the message module's
 * system-state word has bit 4 clear, every cell action is permitted (1);
 * with bit 4 set the corpus reads its 300-entry level-permission table
 * (dword row + column*15, zeroed at level start by the unported
 * counters-clear path). The table lands as zero-init storage with
 * port-side accessors standing in for the unported cell writer. */
int cell_action_permission_get(int column, int row);
void cell_action_permission_table_set(int index, uint32_t value);
                                       /* port hook */
uint32_t cell_action_permission_table_peek(int index);

/* --- the menu-tick gate + status banner word (2026-10-24; findings 37/441) --
 * MenuAudioTickIntervalReached (FUN_00412190): the corpus's menu tick asks
 * the shared random draw whether the word is a multiple of 400. The port
 * models the draw module-locally the way the overlay rng and the level/
 * sound models do (same LCG constants, same 15-bit result).
 * GlobalStateGet_004ddd1c (FUN_0041b9b0): the status-banner toggle word
 * the corpus flips from its periodic status tick (finding 441) — that
 * feeder is unported, so the word lands zero-initialized with port-side
 * accessors standing in. */
int menu_audio_tick_interval_reached(void); /* FUN_00412190 */
uint32_t sim_status_banner_word_get(void);  /* FUN_0041b9b0 */
void     sim_status_banner_word_set(uint32_t value); /* port hook */

/* UiModeResourceBit40SetClear (FUN_00419200): with the first argument
 * set the corpus takes the current bit 0x40 as its second argument,
 * then installs the word with the bit set when that answer is clear and
 * cleared when it is not — the bit-0x40 rule over the modeled selection
 * word, faithful. */
void ui_mode_resource_bit40_set_clear(int probe, uint32_t value);

/* MenuWipeStart (FUN_004103f0, finding 37): when the wipe-arm word is
 * set, stop the running wipe clock, raise the start/pending bits in the
 * modeled wipe flags, stamp the clock word with the (float) tick, and
 * submit sound alias 0x27. The arm/clock words have no ported owner —
 * zero-init statics with port hooks (the corpus's timeGetTime stamp is
 * modeled by the tick-word hook, a documented deviation). */
void menu_wipe_start(void);             /* FUN_004103f0 */
void menu_wipe_arm_set(uint32_t value);
uint32_t menu_wipe_arm_peek(void);
float menu_wipe_clock_peek(void);
void menu_clock_millis_set(uint32_t millis); /* port tick hook */

/* --- the panel texture-slot blit (2026-10-24; findings 250/252) --------------
 * PanelTextureSlotsBlit (FUN_0045a9c0): walks the panel's draw-record block
 * (stride 0x2c, count `panel_draw_count_peek`) with the panel's view-offset
 * pair added to each record's position, picks the texture slot the flag
 * word's bits 2/4/8/0x10/0x40 select — with the status-banner word as the
 * corpus's alternate-look gate — and blits through the modeled surface
 * seam. Bit 0x10 hides the item; the picker otherwise consults the banner
 * word (`sim_status_banner_word_get`) exactly where the corpus does. */
void panel_texture_slots_blit(int panel); /* FUN_0045a9c0 */

#endif /* LEGORRR_GAME_FRONTEND_HPP */
