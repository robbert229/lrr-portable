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
#include <stdarg.h>

#include "gods98/block.hpp"
#include "gods98/texture.hpp"
#include "options.hpp"

struct EngineContext;

#ifdef __cplusplus
extern "C" {
#endif

struct Flic; /* gods98/flic.hpp — `.flh` overlay player (lazily opened) */
struct AudioPlayer; /* gods98/audio.hpp — optional front-end SFX output */
struct SavTreeNode;
struct SavListWindow;

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

/* FrontEndStateChange (FUN_004169c0) flow selectors. */
#define FE_STATE_MAIN       0
#define FE_STATE_MISSIONS   1
#define FE_STATE_TRAINING   2
#define FE_STATE_OPTIONS    3
#define FE_STATE_SAVE_REWARD 4

typedef struct FeItem {
    int   kind;
    int   x, y;      /* cfg anchor (or per-level FrontEndX/Y for picker rows) */
    char  label[FE_NAME_MAX];  /* display text (underscores shown as spaces) */
    char  image_normal[FE_PATH_MAX];  /* image-backed Next/Trigger item */
    char  image_selected[FE_PATH_MAX];
    char  status[FE_NAME_MAX]; /* hover/status text for save-list rows */
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
    char  full_name[FE_NAME_MAX]; /* FullName key (underscore-normalized) */
    char  title[FE_NAME_MAX];     /* Title key (underscored display text) */
    char  image[FE_PATH_MAX];     /* MenuImage */
    char  image_dark[FE_PATH_MAX];/* MenuImageDark */
    char  lofont[FE_PATH_MAX];    /* LoFont key — unselected items (Menu_Font_Lo.bmp) */
    char  hifont[FE_PATH_MAX];    /* HiFont key — selected items  (Menu_Font_Hi.bmp) */
    int   base_x, base_y;         /* Position key (x:y) */
    int   title_anchor_x;         /* Anchored x:y */
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
#define FE_SAVE_THUMBNAIL_COUNT 5 /* InterfaceResourceHandlesRelease slots */

/* A runtime save slot row (SaveMenuBuild @ 0x415a10 builds these from the
 * `Save_Game_%i` keys): the slot's label + whether it has any records. */
typedef struct FeSlot {
    int   id;
    char  label[FE_NAME_MAX];
    char  status[FE_NAME_MAX];
    int   present;
} FeSlot;

/* SaveMenuLayoutLoad (FUN_00415d00): the corpus stores these paths and
 * rectangles in a 0x728-byte resource record; this is its typed frontend
 * view. */
typedef struct FeSaveMenuLayout {
    char load[FE_PATH_MAX];
    char save[FE_PATH_MAX];
    char slot[FE_PATH_MAX];
    char save_sel[FE_PATH_MAX];
    char load_sel[FE_PATH_MAX];
    char level[FE_PATH_MAX];
    char tutorial[FE_PATH_MAX];
    int window_x, window_y, window_w, window_h;
    int has_window;
    char panel_path[FE_PATH_MAX];
    int panel_x, panel_y, panel_w, panel_h;
    int has_panel;
} FeSaveMenuLayout;

#define FE_LEVEL_MENU_LISTS 2
#define FE_LEVEL_MENU_ROWS  32
typedef struct FeLevelMenuRow {
    char level[FE_LEVEL_NAME_MAX];
    char full_name[FE_NAME_MAX];
    char menu_bmp[FE_PATH_MAX];
    int x, y;
    int open;
} FeLevelMenuRow;

typedef struct FeLevelMenuList {
    int native_id; /* SavMenuList selector 4 (campaign) or 5 (tutorial) */
    char title[FE_NAME_MAX];
    int row_count;
    FeLevelMenuRow rows[FE_LEVEL_MENU_ROWS];
} FeLevelMenuList;

#define FE_SURROUND_IMAGES_MAX 11
typedef struct FeSurroundImage {
    TextureSurface* image;
    int x, y, width, height;
    TextureSurface* image_without_back;
    int without_back_x, without_back_y;
} FeSurroundImage;

#define FE_INTERFACE_ACTION_COUNT 74
typedef struct FeInterfaceImage {
    TextureSurface* normal;
    TextureSurface* disabled;
    TextureSurface* pressed;
    char normal_tooltip[FE_PATH_MAX];
    char disabled_tooltip[FE_PATH_MAX];
    int normal_sound;
    int disabled_sound;
    char shortcut_name[FE_NAME_MAX];
    uint8_t shortcut_code;
    int has_shortcut_code;
    int optional_flag;
    int has_optional_flag;
} FeInterfaceImage;

typedef struct FeInterfaceBuildImage {
    TextureSurface* normal;
    TextureSurface* disabled;
    TextureSurface* pressed;
    char shortcut_name[FE_NAME_MAX];
    uint8_t shortcut_code;
    int has_shortcut_code;
} FeInterfaceBuildImage;

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
    TextureSurface* save_thumbnails[FE_SAVE_THUMBNAIL_COUNT];
    FeSaveMenuLayout save_text_layout;
    FeSaveMenuLayout level_text_layout;
    FeLevelMenuList level_menu_lists[FE_LEVEL_MENU_LISTS];

    /* --- runtime (modal menu) state ------------------------------------ */
    int  open;                      /* 1 while the menu loop is showing   */
    int  menu;                      /* FE_MENU_* (MAIN / PAUSED reachable) */
    int  flow_state;                /* FrontEndStateChange selector 0..4 */
    int  save_action;               /* mode 4: selecting a slot saves */
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
    uint32_t overlay_rng;           /* portable-only front-end LCG state    */
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
    FE_EVT_SAVE_GAME,     /* reward save: frontend.load_slot is target slot */
    FE_EVT_RESUME,        /* close a modal overlay (pause Continue / Esc)   */
    FE_EVT_CREDITS,       /* show the main-menu scrolling credits view      */
    FE_EVT_QUIT,          /* user quit the app from the front end */
} FeEvent;

/* --- lifecycle ---------------------------------------------------------- */

void frontend_init(FrontEnd* fe);
int save_menu_layout_load(FrontEnd* fe, const char* group,
                          FeSaveMenuLayout* layout); /* FUN_00415d00 */
int interface_back_button_config_load(BlockFile* cfg); /* FUN_0041b1b0 */
int interface_back_button_dimensions(int* width, int* height);
TextureSurface* interface_back_button_image(int pressed);
const char* interface_back_button_action(void);
int interface_surround_images_config_load(BlockFile* cfg); /* FUN_0041afd0 */
const FeSurroundImage* interface_surround_image_peek(int index);
int interface_images_config_load(BlockFile* cfg); /* FUN_0041ada0 */
const FeInterfaceImage* interface_image_peek(int action_index);
int interface_build_images_config_load(BlockFile* cfg); /* FUN_0041ab00 */
uint32_t interface_build_image_count(int group);
const FeInterfaceBuildImage* interface_build_image_peek(int group, int index);
TextureSurface* interface_build_image_binding_value_get(
    const struct EngineContext* engine, int kind, int index);
void* input_binding_default_value_get(int kind, int index, int mode);
void keyboard_key_names_init(void); /* FUN_00485a20 */
int named_option_index_find_case_insensitive(const char* name,
                                             uint8_t* out_index); /* FUN_00485e30 */
void main_menu_text_item(void); /* FUN_004103d0; requests scrolling credits */

/* FormattedTextPrintfWidth / MenuTextRecordMake: menu text measurement and
 * the 15-word text-record constructor used by legacy menu builders. */
uint32_t formatted_text_printf_width(const TextureSurface* font,
                                     const char* text); /* FUN_00401c50 */
void formatted_text_draw(const TextureSurface* font, void* px, int pitch,
                         int w, int h, int x, int y, const char* text);
uint32_t formatted_text_draw_and_measure(const TextureSurface* font,
                                         void* px, int pitch, int w, int h,
                                         int x, int y, const char* text,
                                         int* line_count);
void formatted_text_format_draw(const TextureSurface* font,
                                uint32_t* width_out, int* line_count,
                                void* px, int pitch, int w, int h,
                                int x, int y, const char* format,
                                va_list args);
void formatted_text_printf(const TextureSurface* font,
                           uint32_t* width_out, int* line_count,
                           void* px, int pitch, int w, int h,
                           int x, int y, const char* format, ...);
void reward_text_draw(const TextureSurface* font, void* px, int pitch,
                      int w, int h, int x, int y, const char* format, ...);
char* formatted_text_escape_decode_and_emit(const char* format, ...);
typedef const TextureSurface* (*FeFormattedTextTextureResolve)(uint32_t handle);
void formatted_text_texture_resolver_set(FeFormattedTextTextureResolve resolve);
void* menu_text_record_make(const char* text, const TextureSurface* font_a,
                            const TextureSurface* font_b, uint32_t f4,
                            uint32_t f5, uint32_t f6, int center,
                            uint32_t f8, uint32_t f9); /* FUN_00410d50 */
int case_insensitive_name_table_lookup(const char* name,
                                       int* out_index); /* FUN_0046b4d0 */
void ui_panel_name_tables_install(const char* const names[12],
                                  const char* const indexed_names[][24],
                                  unsigned int indexed_count);
void ui_panel_name_tables_init(void);
int ui_panel_name_table_lookup(const char* name, int* out_index);
int ui_panel_indexed_name_lookup(unsigned int table_index, const char* name,
                                 unsigned int* out_index);
typedef struct ToolTipIconConfigHooks {
    void* context;
    void* (*section_open)(void* config, const char* section, void* context);
    void* (*entry_first)(void* section, void* context);
    void* (*entry_next)(void* entry, void* context);
    const char* (*entry_name_get)(void* entry, void* context);
    const char* (*entry_value_get)(void* entry, void* context);
    void* (*image_load)(const char* name, void* context);
    void (*image_release)(void* image, void* context);
} ToolTipIconConfigHooks;
void tooltip_icon_config_hooks_set(const ToolTipIconConfigHooks* hooks,
                                   const char* const primary_names[11],
                                   const char* const secondary_names[6]);
void tooltip_icons_config_load(void* config); /* 0x437150 */
void* tooltip_icon_handle_get(unsigned int bank, unsigned int index);
void* menu_button_record_make(const char* text,
                              const TextureSurface* normal_font,
                              const TextureSurface* selected_font,
                              const char* normal_image,
                              const char* selected_image, uint32_t x,
                              uint32_t y, uint32_t flags, int centered,
                              const char* optional_name,
                              uint32_t extra); /* FUN_00410e20 */
int menu_mouse_coordinate_update(const void* root, const void* item,
                                 const void* row); /* FUN_00411360 */
int has_type6_item_at_index(const void* menu, int index); /* FUN_004114f0 */
int menu_widget_item_screen_rect(const void* menu, int index, int* x, int* y,
                                int* width, int* height); /* FUN_00411530 */
int menu_pick_widget_under_pointer(const void* menu,
                                   int* picked_index); /* FUN_00411790 */
int frontend_progress_value_changed(void); /* FUN_00411840 */
int option_music_default(void); /* FUN_00414e10 */

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
/* UiTextUnderscoresToSpaces (FUN_004102b0): return a shared display copy
 * with underscores changed to spaces for strings up to 255 bytes. */
const char* ui_text_underscores_to_spaces(const char* text);
/* InterfaceMenuActionLabelsInit / CaseInsensitiveStringTableIndexFind
 * (FUN_0041a300 / FUN_0041a9c0): initialize the 74 action-name keys and
 * selection region, then find a key case-insensitively (zero-based index). */
void interface_menu_action_labels_init(uint32_t x, uint32_t y,
                                       const void* context);
const char* interface_menu_action_label_get(int index);
int interface_menu_action_label_find(const char* name, int* out_index);
void interface_menu_action_layout_peek(float out_xy[2]);
const void* interface_menu_action_context_peek(void);

/* Checkpoint record <-> level mapping (the port's stand-in for the corpus's
 * sav-tree node record slots, SavTreeWalk2 / CheckpointStep's ListFindName
 * index): tutorials take records 0..tutorial_n-1, campaign levels follow
 * (tutorial_n..tutorial_n+campaign_n-1). Returns -1 / 0 for unknowns so
 * callers keep the record-0 default. */
int  fe_record_for_level(const FrontEnd* fe, const char* level);
int  fe_level_for_record(const FrontEnd* fe, int rec, char* out, size_t sz);

/* Enter the modal menu at `menu`/submenu 0, selection 0. */
int  frontend_open(FrontEnd* fe, int menu);
/* Prepare one native front-end flow (save-tree/global setup is port state). */
int  frontend_state_change(FrontEnd* fe, int state); /* FUN_004169c0 */
int  menu_enter(FrontEnd* fe, int mode); /* FUN_00416c80 */
int  save_menu_open_from_reward(FrontEnd* fe); /* FUN_00462030 */
void frontend_close(FrontEnd* fe);
void interface_resource_handles_release(FrontEnd* fe); /* FUN_00413b50 */
void frontend_save_thumbnail_invalidate(FrontEnd* fe, int slot);
void frontend_set_audio(FrontEnd* fe, struct AudioPlayer* audio);

/* Register the runtime save/load slot rows (from save_slot_state) and graft
 * them onto the SaveLoad submenus (MenuImage contains "SaveLoad" — the
 * shipped Load_A_Saved_Game screens), after the cfg items. Selecting a row
 * returns FE_EVT_LOAD_GAME with fe.load_slot = the slot id. */
int  frontend_build_slots(FrontEnd* fe, const FeSlot* slots, int n);
int  frontend_update_slot(FrontEnd* fe, const FeSlot* slot);
int  frontend_sav_menu_list(FrontEnd* fe, int list_id,
                            const struct SavTreeNode* root,
                            const struct SavListWindow* registry);
const FeLevelMenuList* frontend_level_menu_list_peek(const FrontEnd* fe,
                                                      int list_id);

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
void menu_render_ui_frame_cell(void* px, int pitch, int w, int h,
                               int x, int y, int text_row,
                               const char* text,
                               const TextureSurface* image);
typedef struct FePercentSplitSurface {
    const TextureSurface* right_fill;      /* packed record +0x18 */
    const TextureSurface* left_fill;       /* packed record +0x1c */
    const TextureSurface* left_overlay;    /* packed record +0x20 */
    const TextureSurface* right_overlay;   /* packed record +0x24 */
    const TextureSurface* right_idle;      /* packed record +0x28 */
    const TextureSurface* right_hover;     /* packed record +0x30 */
    const TextureSurface* left_idle;       /* packed record +0x2c */
    const TextureSurface* left_hover;      /* packed record +0x34 */
} FePercentSplitSurface;
void menu_percent_split_surface_blit(void* px, int pitch, int w, int h,
                                     const FePercentSplitSurface* surface,
                                     int x, int y, int numerator,
                                     int denominator, int mouse_x,
                                     int mouse_y);
typedef struct FeMenuPanelText {
    const char* text;
    int x, y, width;
    int centered;
} FeMenuPanelText;
typedef struct FeMenuOptionPanel {
    const TextureSurface* image;
    const TextureSurface* contrast;
    int contrast_x, contrast_y;
    const TextureSurface* font;
    const TextureSurface* hover_font;
    FeMenuPanelText title, body, ok, cancel;
} FeMenuOptionPanel;
int menu_option_panel_texture_update(const FeMenuOptionPanel* panel,
                                    void* px, int pitch, int w, int h,
                                    int mouse_x, int mouse_y);
typedef struct FeOverlayMenuEntry {
    int handle;
    int action_state;
} FeOverlayMenuEntry;
typedef struct FeOverlayMenuSet {
    FeOverlayMenuEntry entries[3];
} FeOverlayMenuSet;
typedef struct FeOverlayMenuGateState {
    FeOverlayMenuSet menus[2]; /* 0 = PausedMenu, 1 = OptionsMenu */
    int active_item;
    int cached_menu;
    int level_start_flag;
    int options_flag;
    int action_flag;
    int blocked;
} FeOverlayMenuGateState;
typedef void (*FeOverlayMenuScrollRefresh)(void* context);
typedef int (*FeOverlayMenuTick)(void* context, int active_item, int mode);
int overlay_menu_gate(FeOverlayMenuGateState* state, int mode,
                      FeOverlayMenuScrollRefresh refresh_scroll,
                      FeOverlayMenuTick menu_tick, void* context);
void menu_hover_status_text_draw(FrontEnd* fe, const TextureSurface* font,
                                 int w, int h, void* px, int pitch);
void interface_message_tab_info_text_load(const char* path); /* FUN_00419430 */
TextureSurface* interface_message_tab_info_text(void); /* port accessor */

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
 * reposition hook per found handle is the simtick-cited surface seam.
 * HelpWindowButtonHitTest (FUN_00418da0): the two-button inclusive rectangle
 * hit test and mode transition flow are ported; rectangle installation and
 * the legacy sound clear are exposed through explicit hooks. */
void timed_effect_update_request_set(int index);
uint32_t timed_effect_flags_peek(int index);           /* diagnostics */

/* TimedEffectCueRequestSet (FUN_0046b700): stores the cue request word in
 * the entry's band slot (+0 in the corpus's 0x27c stride, modeled as the
 * port's guarded cue window) and raises the entry's flag bits 0 and 1.
 * Deviation: the corpus strides any index and the stride band blindly. */
void timed_effect_cue_request_set(int index, uint32_t value);
uint32_t timed_effect_cue_peek(int index);             /* port hook */

/* TimedEffectPersistenceSet (FUN_0046b730): raises or drops the entry's
 * flag-word bit 4 (the persistence bit). Deviation: the corpus strides
 * the 0x27c-stride band word blindly. */
void timed_effect_persistence_set(int index, int set);
bool     ui_mode_transition_request(int index, int p2, int p3);
int      band_mode_peek(void);                        /* diagnostics */
void     band_mode_set(int mode);                     /* diagnostics */
void     selection_mode_set_by_index(int index);
void     ui_mode_resource_positions_apply(void);
void     ui_mode_resource_reposition_hook_set(
             int (*hook)(uint32_t handle, int x, int y));
void     help_window_button_feedback_update(int action, float amount);
void     help_window_feedback_state_set(float position, float limit);
float    help_window_feedback_position_get(void);
void     help_window_feedback_hooks_set(void (*reset)(int, int),
                                        void (*cleanup)(void),
                                        void (*finish)(int));
void     help_render_transition_clear_hook_set(void (*hook)(int, int));
void     help_render_transition_cleanup_if_active(void);
void     help_window_button_rect_set(int index, float x, float y,
                                     float width, float height);
typedef struct FeHelpWindowResourceOps {
    void* context;
    void (*info_strings_load)(void* context, BlockFile* cfg);
    const char* const* info_group_names[3];
    size_t info_group_counts[3];
    void (*info_string_store)(void* context, int group,
                              size_t catalog_index, int line_index,
                              const char* text);
    uint32_t (*resource_load)(void* context, const char* name, int x, int y);
    void (*background_apply)(void* context, uint32_t handle);
    uint32_t (*text_area_create)(void* context, uint32_t mode,
                                 const int rect[4], uint32_t flags);
    void (*text_load)(void* context, int slot, const char* name);
    void (*label_set)(void* context, int index, const char* text);
    void (*refresh)(void* context, int p1, int p2);
} FeHelpWindowResourceOps;
/* HelpWindowInfoConfigStringsLoad (FUN_00418450): loads the sixteen
 * HelpWindowInfo rows for each catalog name in the three corpus groups.
 * Catalog tables and destination storage are inputs because those globals
 * are not owned by the front-end model. */
void     help_window_info_config_strings_load(
             BlockFile* cfg, const char* const* const group_names[3],
             const size_t group_counts[3],
             void (*store)(void* context, int group, size_t catalog_index,
                           int line_index, const char* text),
             void* context);
/* ToolTipInfoStringsLoad (FUN_00427e00): load nine fixed labels from the
 * ToolTipInfo config group into module-owned strings. */
void tooltip_info_strings_load(BlockFile* cfg);
typedef void* (*UiTextImageLoadFn)(const char* path, void* context);
typedef struct UiTextEntryState {
    char text[256];
    void* image;
    uint32_t flags;
    int32_t counters[5];
} UiTextEntryState;
void ui_text_entry_load(UiTextEntryState* entry, const char* source,
                        UiTextImageLoadFn image_load, void* context);
typedef void* (*AdvisorTutorialIconLoadFn)(const char* path, void* context);
typedef void (*AdvisorTutorialIconInitializeFn)(void* icon, void* context);
void advisor_tutorial_icon_hooks_set(AdvisorTutorialIconLoadFn load,
                                     AdvisorTutorialIconInitializeFn initialize,
                                     void* context);
void advisor_tutorial_icon_image_load(BlockFile* cfg); /* FUN_00434a00 */
void* advisor_tutorial_icon_get(void);
typedef void (*AdvisorTextRequestActionFn)(uint32_t request, int p1, int p2,
                                           void* context);
void advisor_text_request_action_hook_set(AdvisorTextRequestActionFn hook,
                                          void* context);
void advisor_text_request_setup(uint32_t request, uint32_t flags);
uint32_t advisor_text_request_flags_get(void);
typedef struct AdvisorTextDisplayState {
    int current_slot;
    int hidden_slot;
    int message_id;
    uint32_t message_flags;
    uint32_t interface_flags;
    float message_timer;
    float x;
    float y;
} AdvisorTextDisplayState;
typedef struct AdvisorTextDisplayOps {
    void* context;
    int (*state_get)(void* context, AdvisorTextDisplayState* state);
    int (*slot_is_available)(void* context, int slot);
    int (*slot_text_draw)(void* context, int slot, int alternate,
                          float x, float y);
    int (*message_text_draw)(void* context, int message_id,
                             float x, float y);
    int (*overlay_active)(void* context);
    void (*overlay_close)(void* context);
    void (*current_slot_set)(void* context, int slot);
    void (*message_flags_set)(void* context, uint32_t flags);
    void (*message_timer_set)(void* context, float timer);
    void (*interface_finish)(void* context, const AdvisorTextDisplayState* state,
                             float delta);
} AdvisorTextDisplayOps;
void advisor_text_display_ops_set(const AdvisorTextDisplayOps* ops);
int advisor_text_display_worker(float delta); /* FUN_0046ad00 */
typedef struct SelectedGroundMarkerOps {
    void* context;
    int (*entity_renderable)(void* context, void* entity);
    uint32_t (*global_flags_get)(void* context);
    int (*marker_data_get)(void* context, void* entity,
                           float world_position[3], float* radius_out,
                           float* height_out, const char** label_out);
    int (*world_to_screen)(void* context, void* camera,
                           const float world_position[3],
                           float screen_position[3]);
    void (*viewport_get)(void* context, float* width_out,
                         float* height_out);
    void (*polyline_draw)(void* context, const float points_xy[16],
                          float red, float green, float blue);
    void (*label_update)(void* context, const char* label, float red,
                         float green, float blue);
} SelectedGroundMarkerOps;
void selected_ground_marker_ops_set(const SelectedGroundMarkerOps* ops);
int selected_entity_ground_marker_render(void* entity, void* camera,
                                         float red, float green, float blue);
typedef struct OverlayTextLine {
    void* entry;
    int kind;
    float height;
    float x;
    float y;
} OverlayTextLine;
typedef struct OverlayTextBufferOps {
    void* context;
    int (*ensure_initialized)(void* context);
    uint32_t (*interface_flags_get)(void* context);
    uint32_t (*input_flags_get)(void* context);
    void (*transition_begin)(void* context);
    int (*entry_count_get)(void* context);
    int (*entry_get)(void* context, int index, OverlayTextLine* line);
    int (*primary_text_draw)(void* context, float* bottom_out);
    void (*header_draw)(void* context);
    void (*header_dirty_clear)(void* context);
    float (*scroll_position_get)(void* context);
    int (*scroll_start_index_get)(void* context, int count);
    float (*scroll_clip_y_get)(void* context);
    int (*line_draw)(void* context, const OverlayTextLine* line,
                     float x, float y);
    void (*line_focus)(void* context, const OverlayTextLine* line);
    void (*scroll_limit_reached)(void* context);
    void (*finish)(void* context);
    void (*reset)(void* context);
} OverlayTextBufferOps;
void overlay_text_buffer_ops_set(const OverlayTextBufferOps* ops);
int overlay_text_buffer_refresh(void); /* FUN_00418a00 */
typedef struct AdvisorDialogStackEntry {
    void* record;
    uint32_t item_id;
    int has_special;
} AdvisorDialogStackEntry;
typedef struct AdvisorDialogStackOps {
    void* context;
    int (*active)(void* context);
    void (*layout_reset)(void* context, uint32_t width, uint32_t height);
    int (*entries_get)(void* context, uint32_t width, uint32_t height,
                       AdvisorDialogStackEntry** entries_out,
                       uint32_t* count_out);
    void* (*icon_get)(void* context, uint32_t index);
    int (*icon_height)(void* context, void* icon);
    void (*icon_place)(void* context, void* icon, float y);
    int (*entry_eligible)(void* context, void* record);
    void* (*item_resolve)(void* context, void* record, int eligible);
    int (*item_height)(void* context, void* item);
    void (*item_place)(void* context, void* item, float y);
    void (*special_item_apply)(void* context, uint32_t item_id);
} AdvisorDialogStackOps;
void advisor_dialog_stack_ops_set(const AdvisorDialogStackOps* ops);
void advisor_dialog_stack_layout(float y, uint32_t parameter,
                                 uint32_t width, uint32_t height); /* FUN_0041eca0 */
typedef struct DebugPanelTextOps {
    void* context;
    int (*origin_get)(void* context, int* x_out, int* y_out);
    int (*font_line_height)(void* context);
    int (*text_width)(void* context, const char* text);
    int (*scroll_width)(void* context, uint32_t token);
    const char* (*fallback_text_get)(void* context);
    void (*text_draw)(void* context, int x, int y, int alternate,
                      const char* text);
} DebugPanelTextOps;
void debug_panel_text_ops_set(const DebugPanelTextOps* ops);
void debug_panel_text_draw_update(float elapsed, int screen_width,
                                  const char* text); /* FUN_004230c0 */
typedef struct InterfaceSpawnRequestState {
    int type;
    uint32_t parameter;
    int facing;
} InterfaceSpawnRequestState;
typedef struct InterfaceSpawnRequestOps {
    void* context;
    void (*prepare)(void* context, int mode);
    int (*camera_direction_get)(void* context, float direction_out[2]);
    uint32_t (*request_flags_get)(void* context, int type,
                                  uint32_t parameter);
    void* (*saved_slot_get)(void* context, uint32_t category);
    int (*request_dispatch)(void* context, uint32_t category, void* slot,
                            int type, uint32_t parameter);
} InterfaceSpawnRequestOps;
void interface_spawn_request_ops_set(const InterfaceSpawnRequestOps* ops);
int interface_spawn_request_enqueue(int type, uint32_t parameter); /* FUN_00434e80 */
void interface_spawn_request_state_get(InterfaceSpawnRequestState* state_out);
typedef struct AdvisorOverlayRect {
    float x;
    float y;
    float width;
    float height;
    int enabled;
} AdvisorOverlayRect;
typedef int (*AdvisorOverlayFallbackHitFn)(
    void* context, float x, float y, float offset_x, float offset_y,
    uint32_t* result0, uint32_t* result1, uint32_t* result2);
int advisor_overlay_hit_resolve(
    float x, float y, const AdvisorOverlayRect* focused_rect,
    float offset_x, float offset_y, AdvisorOverlayFallbackHitFn fallback,
    void* context, uint32_t* result0, uint32_t* result1,
    uint32_t* result2); /* FUN_0041c1c0 */
typedef struct AdvisorTextAnimationOps {
    void* context;
    int (*is_paused)(void* context);
    int (*animation_find)(void* context, int slot, int* request_out);
    void (*animation_play)(void* context, int request, int mode);
} AdvisorTextAnimationOps;
void advisor_text_animation_ops_set(const AdvisorTextAnimationOps* ops,
                                    float reset_timer);
void advisor_text_animation_update(int slot, int force, int selected); /* FUN_0046ac60 */
typedef struct AdvisorTextBoxLayoutState {
    uint32_t object_id;
    uint32_t request_id;
    uint32_t mode;
    float value;
    float geometry[4];
    float bottom;
    float font_line_height;
    uint32_t flags;
} AdvisorTextBoxLayoutState;
typedef void (*AdvisorTextBoxExtentFn)(void* context, int width, int height);
void advisor_text_box_extent_hook_set(AdvisorTextBoxExtentFn hook,
                                      void* context);
void advisor_text_box_record_commit(AdvisorTextBoxLayoutState* state); /* FUN_0046a9e0 */
void text_layout_geometry_store_and_apply(
    AdvisorTextBoxLayoutState* state, uint32_t object_id,
    uint32_t request_id, float value, const float geometry[4],
    uint32_t mode); /* FUN_0046a950 */
const char* tooltip_info_string_peek(int index);
typedef struct TooltipOverlayState {
    const char* names[38];
    void* font;
    uint32_t font_metric;
    uint32_t values[5];
    float scale;
    float color_base[3];
    float color_high[3];
    float color_low[3];
} TooltipOverlayState;
typedef uint32_t (*TooltipFontMetricQueryFn)(void* font, void* context);
void tooltip_overlay_state_initialize(
    TooltipOverlayState* state, void* font,
    uint32_t value2, uint32_t value3, float scale,
    uint32_t value5, uint32_t value6, uint32_t value7,
    float color_r, float color_g, float color_b,
    TooltipFontMetricQueryFn metric_query, void* context); /* FUN_0046b1d0 */
void     help_window_resources_init(BlockFile* cfg,
                                    const FeHelpWindowResourceOps* ops);
void     help_window_button_layout_load(
             BlockFile* cfg, float offset_x, float offset_y,
             uint32_t (*resource_load)(void* context, const char* name,
                                       int x, int y),
             void (*text_load)(void* context, int slot, const char* name),
             void* context);
void     help_window_sound_clear_hook_set(void (*hook)(int));
int      help_window_button_hit_test(uint32_t x, uint32_t y, int mode,
                                     int transition_flags, int active,
                                     float feedback_amount);
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
void selection_region_parameters_set(uint32_t a, uint32_t b, uint32_t c,
                                     uint32_t d, float e);
/* selection_region_parameters_set (FUN_0041e9d0): four integer words
 * convert from unsigned and rotate across the region slots (a -> +08,
 * b -> +0c, c -> +00, d -> +04); the float scales by the corpus's .rdata
 * 0.04 in single precision into the +10 slot. */
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
/* PointerActionStateDispatch (FUN_0041c6e0): drive the pointer action
 * latch and matching input-entry/player flags. */
int pointer_action_state_dispatch(int action, int kind, int entry,
                                  int active, int suppress_initial);
typedef void (*FeAdvisorCallbackSchedule)(uint32_t selector, void* context);
void advisor_callback_schedule_enabled_set(int enabled); /* port hook */
void advisor_callback_schedule_gate(FeAdvisorCallbackSchedule schedule,
                                    void* context); /* FUN_0041c440 */
int advisor_animation_slot_id_resolve(uint32_t animation_kind,
                                      uint32_t position_index,
                                      uint32_t* slot_id); /* FUN_0045c6b0 */

/* CursorTileSelectionUpdateByType (FUN_0041cc80): the tile-selection pair
 * ride (DAT_004ddcd8/cdc role) keyed by the selection kind: 0x49 installs
 * the caller's pair, 5 and 0xe install the pair's fixed shapes, anything
 * else rides. The corpus readers sit in the unported UI/hit-test bands, so
 * the words ride own storage (the unowned-global deviation). */
void cursor_tile_selection_update_by_type(int kind, uint32_t first,
                                          uint32_t second);
uint32_t cursor_tile_pair_peek(int which);      /* port hook: 0/1 */

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
 * the shared random draw whether the word is a multiple of 400. It uses the
 * same RandomNext state as the level and sound paths.
 * GlobalStateGet_004ddd1c (FUN_0041b9b0): the status-banner toggle word
 * the corpus flips from its periodic status tick (finding 441) — that
 * feeder is unported, so the word lands zero-initialized with port-side
 * accessors standing in. */
bool menu_audio_tick_interval_reached(void); /* FUN_00412190 */
uint32_t sim_status_banner_word_get(void);  /* FUN_0041b9b0 */

/* PanelResourceLayoutStore (FUN_0045c340): the layout band store — both
 * surface loads ride the load seam (NULL answers), the glyph load's
 * transformed blit rides the blit seam when the load answered a surface,
 * then the size/extra/position words land.
 * PanelResourcePositionUpdateIfEnabled (FUN_0045c4b0): with the layout
 * glyph present, the indexed pair entry plus the stored offsets ride to
 * the blit seam, gated by the 0x50080c word and the status-banner word.
 * Deviation: the corpus loads are hook boundaries answering NULL and the
 * DAT words have no ported owner. */
void panel_resource_layout_store(void* face_resource, uint32_t width,
                                 uint32_t height, uint32_t extra,
                                 void* glyph_resource, uint32_t position_x,
                                 uint32_t position_y);
void panel_resource_position_update(int index);
void* panel_layout_surface_peek(void);    /* port hook */
void* panel_layout_glyph_peek(void);      /* port hook */
void  panel_layout_glyph_install(void* surface); /* test hook: the seam
                                                   * stands-in (deviation) */
uint32_t panel_layout_extra_peek(void);   /* port hook */
float panel_layout_size_peek(int which);  /* port hook: 0/1 */
float panel_layout_offset_peek(int which); /* port hook: 0/1 */
void     sim_status_banner_word_set(uint32_t value); /* port hook */

/* --- the UI/panel category-bounds band (2026-10-25) -------------------------
 * The corpus's DAT_00500110 record band (stride 0x30, eighteen windows)
 * carries the panel category rectangles and the per-entry draw bounds.
 * PanelCategoryBoundsSet (FUN_0045a650): stores the rectangle (ints cast
 * to floats), clears the record's word, and routes the pair behind — the
 * first corner while the modeled flag word's bit 2 rides, else the second.
 * The record's float fields have no ported owner — own storage; the pair/
 * flag windows ride the crt.* windows already modeled for them.
 * UiEntryBoundsGet (FUN_0045bf70): position plus derived size from the
 * panel's installed draw-record block (stride 0x2c, far minus near). */
void panel_category_bounds_set(int panel, int x1, int y1, int x2, int y2);
                                        /* FUN_0045a650 */
void panel_category_bounds_peek(int panel, float* out_x1, float* out_y1,
                                float* out_x2, float* out_y2); /* port hook */
void ui_entry_bounds_get(int panel, int item, float* out_bounds);
                                        /* FUN_0045bf70 */

/* PriorityImageSelectionStateUpdate (FUN_0045d300): the priority-image
 * selection state pair (a sentinel-parked state word plus a flag word —
 * no ported owner, own storage behind the install/peek hooks). */
int priority_image_selection_state_update(int candidate, int gate, int skip);
                                        /* FUN_0045d300 */
void priority_image_state_set(int32_t state, uint32_t flags); /* port hook */
void priority_image_state_peek(int32_t* state, uint32_t* flags);

/* UiRenderRecordCreate (FUN_0045da90): the 0x3c UI render record — the
 * source, its payload words copied from the caller's array, the extra
 * argument, and the source's float at byte offset +0x10 (the corpus's
 * float10 round trip is exact; the port copies). LP64-safe widened
 * fields; the corpus's uninitialized record tail stays absent here.
 * Paired with the heap module's bare free wrapper. */
typedef struct UiRenderRecord {
    void*     source;       /* corpus dword at +0 (widened, deviation) */
    uint32_t  payload[4];   /* corpus dwords +4..+16, from the input array */
    uint32_t  extra;        /* corpus dword +20 */
    float     float_at_10;  /* corpus dword +24: source's float at +0x10 */
} UiRenderRecord;
UiRenderRecord* ui_render_record_create(void* source,
                                        const uint32_t* payload,
                                        uint32_t extra);       /* FUN_0045da90 */

/* UiModeResourceBit40SetClear (FUN_00419200): with the first argument
 * set the corpus takes the current bit 0x40 as its second argument,
 * then installs the word with the bit set when that answer is clear and
 * cleared when it is not — the bit-0x40 rule over the modeled selection
 * word, faithful; the corpus function has no return value. */
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

/* TimedEffectTimestampReset (FUN_0046b7a0): stamps the corpus's global tick
 * word into the band entry's time field (stride-0x27c table — the port keeps
 * the fields the setters touch side by side). The corpus tick word
 * (DAT_0054bf3c) belongs to the unported update family, so it lands as a
 * zero-init module static with a set hook. Out-of-range indices are guarded
 * (documented deviation: the corpus strides blindly). */
void    timed_effect_timestamp_reset(int index);  /* FUN_0046b7a0 */
void    timed_effect_tick_word_set(uint32_t tick); /* port hook */
uint32_t timed_effect_tick_word_peek(void);
uint32_t timed_effect_time_peek(int index);        /* diagnostics */
/* ScreenRightEdgeCacheRefresh (FUN_0041a290): re-reads the index-0 text
 * metrics pair through the ported getter and caches x - y into the right-edge
 * word. The corpus's reader is the unported layout band, so the cache lands
 * as a module static with a peek hook. */
void     screen_right_edge_cache_refresh(void); /* FUN_0041a290 */
uint32_t screen_right_edge_cache_peek(void);    /* port hook */
/* BitmapFontGlyphMeasureWidth (FUN_0047a520): measures one glyph without
 * drawing. The decompiler labels the wrapper void, but its callers consume
 * the advance returned by FUN_0047a470; this port derives the same value from
 * the bitmap ink bounds used by its renderer. */
int bitmap_font_glyph_measure_width(void* font, unsigned char glyph);
                                                /* FUN_0047a520 */

/* --- panel resource + scroll leaf band (2026-10-24; findings 386/439/436/
 * 444/372) -----------------------------------------------------------------
 * PanelResourcePositionApplyIfEnabled (FUN_00459fd0): when the panel enable
 *   word's bit 0x10 is raised, the resource rides the surface blit seam at
 *   the supplied position plus the bubble offsets (words unowned; statics).
 * PanelResourceHandleAndDimensionsStore (FUN_0045c520): the surface-load
 *   seam's return lands as the panel handle word (the port has no surface
 *   layer, so the seam answers NULL — the view object-layer precedent),
 *   then the dimensions store.
 * ScrollOffsetFromCursorUpdate (FUN_0041a250): the panel UI record's state
 *   pair difference clamps at zero, lands in metrics slot 0 through the
 *   ported pair setter, and refreshes the right-edge cache.
 * All seams ride recorded hooks (the bobbin-drop precedent). */
void panel_resource_position_apply_if_enabled(void* resource, int x, int y);
                                                /* FUN_00459fd0 */
/* IndexedPanelResourcePositionApply (FUN_00459f50): with the panel enable
 * word's bit 0x1 raised, the record's screen float rides the corpus's two
 * float constant products through the x87 shape (modeled as exact double
 * products, truncated toward zero) to pick one of five panel resource
 * slots (the corpus's unsigned clamp puts every out-of-range answer,
 * negatives included, on slot 4) and a non-NULL slot rides the surface
 * blit seam at the position plus the offset pair. Slots and offsets are
 * own storage installed through the hooks below (the corpus
 * position-config loaders are recorded seams); the port reads the record
 * float's address instead of the corpus's blind record+0x344 read
 * (deviation). */
void indexed_panel_resource_position_apply(const float* record_position,
                                           int x, int y);
void panel_resource_handle_and_dimensions_store(void* resource, uint32_t w,
                                                uint32_t h, uint32_t extra);
                                                /* FUN_0045c520 */
void scroll_offset_from_cursor_update(void);    /* FUN_0041a250 */
void     panel_enable_word_set(uint32_t word);  /* port hook */
uint32_t panel_enable_word_peek(void);          /* port hook */
void     bubble_offsets_set(float x, float y);  /* port hook */
void     panel_surface_handle_set(void* h);     /* port hook */
void*    panel_surface_handle_peek(void);       /* port hook */
void     panel_dimensions_peek(float* w, float* h, uint32_t* extra);
                                                /* port hook */
void     panel_ui_record_set(void* record);     /* port hook */
void*    panel_ui_record_peek(void);            /* port hook */
void     panel_text_slot_set(void* slot);       /* port hook (DAT_005007dc) */
void*    panel_text_slot_peek(void);            /* port hook */
void     panel_position_slot_set(int which, void* res);  /* port hook */
void*    panel_position_slot_peek(int which);            /* port hook */
void     panel_position_offsets_set(float x, float y);   /* port hook */

/* InterfaceButtonImagesLoad (FUN_0041b270): load the two button images
 * through the surface-load seam and call the transform seam per non-zero
 * handle. The peek hook reads the DAT window roles (own storage). */
void     interface_button_images_load(const char* left_path,
                                      const char* right_path);
uint32_t interface_button_surface_peek(int which);   /* port hook */

/* EncyclopediaConfigFilesOpen (FUN_0040e490): walk the catalog name-table
 * groups (the corpus's counts 0x27c/0x280/0x284/0x288 ride the entity.*
 * window hooks), join `root::Encyclopedia::<name>`, read the config's
 * value token, and open the file the value names; the four handle windows
 * reallocate through the heap seam (the corpus leaks the superseded
 * arrays, and so does the port). The PowerCrystal/Ore singletons keep
 * their last handle across a config miss (the corpus's stale-global lane).
 * Deviations: the name tables and counts ride own storage (the corpus's
 * catalog loader is unported), NULL config/value answers where the corpus
 * strides, and the "r" mode maps to the port's AUTO open. */
void encyclopedia_config_files_open(BlockFile* cfg, const char* root_path);
void* encyclopedia_file_window_peek(int group);      /* port hook, 0..3 */
void* encyclopedia_crystal_file_peek(void);          /* port hook */
void* encyclopedia_ore_file_peek(void);              /* port hook */

/* ThreeSurfaceResourcesLoadAndInitialize (FUN_0045bfb0): the same fold
 * for three paths, with the corpus's slot quirk: param_2 lands at peek
 * index 2 and param_3 at index 1. */
void     three_surface_resources_load(const char* first, const char* second,
                                      const char* third);
uint32_t advisor_bar_surface_peek(int which);        /* port hook */

/* AdvisorProgressBarFillDraw (FUN_0045c3f0): gate on the layout face and
 * a positive percent, clamp at 100, and ride the seam with the row pair
 * (float bits) offset by the layout size pair and a {fill, face+8} size
 * witness; fill = extra * 0.01 * percent. Deviation: the face+8 read
 * needs panel_layout_surface_install (the load seam answers NULL). */
void     advisor_progress_bar_fill_draw(int row, float percent);

/* MenuSurfaceBlitScaled (FUN_00412af0): side-selectable face blit — flag
 * set takes the plain destination lane, unset the scaled lane through the
 * rect witness {-pan, -pan, 640, 480} at the origin. Own-storage record
 * window + plane-pan window (the corpus writers are per-submenu pan,
 * modeled apart in FeSubmenu.scroll). */
void     menu_surface_blit_scaled(int percent_side);
void     menu_blit_record_write(int index, uint32_t value);
uint32_t menu_blit_record_read(int index);
void     menu_blit_record_clear(void);
void     menu_plane_pan_set(float x, float y);
float    menu_plane_pan_peek(int axis);

/* PanelLayoutSurfaceInstall (port hook): stands in for the loaded face
 * record so the progress-bar fill's gate and face+8 read exercise. */
void     panel_layout_surface_install(void* surface);  /* test hook */

/* ScreenCoordNormalizeInit (FUN_0045d8b0): one pass over the corpus's
 * screen-normalization band — 90 color constants land scaled by the
 * 1/255 float constant, two stride-16 lanes fill 20 unit cells each,
 * seven coordinate-pair rows (primary + mirror) seed from the corpus's
 * constants and ride the display-dimension pair scaled by the 1/640 /
 * 1/480 float constants, the row-count word books 7, and two sentinel
 * words take all-bits. The band is own storage (its corpus owner is
 * unported) seeded through the reset hook; the peek hooks read it.
 * Quirks pinned: the last primary pair stays zero while its mirror
 * takes 320/40, and the first mirror row's pair differs from the
 * primary's. */
void     screen_coord_normalize_init(void);               /* FUN_0045d8b0 */
void     screen_coord_normalize_reset(void);              /* port hook */
float    screen_coord_normalize_color_peek(int index);    /* port hook */
float    screen_coord_normalize_pair_peek(int mirror, int row,
                                          int axis);      /* port hook */
float    screen_coord_normalize_cell_peek(int mirror, int index);
                                                          /* port hook */
uint32_t screen_coord_normalize_rows_peek(void);          /* port hook */
void     panel_seam_reset(void);                /* port hook */
int      panel_seam_calls(void);                /* port hook */
void     panel_seam_last(void** out_res, float* out_x, float* out_y);
                                                /* port hook */

/* ActiveObjectList (FUN_00416e00, findings 371/378): the active-object word
 * selector — in-game answers the DAT_00556fd0 word, otherwise the
 * DAT_00556fe4 word. Neither corpus word has a ported owner, so both land
 * here as zero-init statics; the selector returns the live address. */
int* active_object_list_word(void); /* FUN_00416e00 */

/* --- the sav visited-table band (2026-10-25) -------------------------------
 * The corpus's two "visited" tables run parallel to the modeled
 * active-object count words (out-of-game: DAT_00556fe4 count behind
 * DAT_00556ff0/ff4 item/word arrays; in-game: DAT_00556fd0 count behind
 * DAT_00556fdc/e0). Their installers live in the unported sav-tree
 * loaders, so the tables ride zero-init own storage behind the install/
 * peek hooks below (the unowned-global convention).
 * SavClearVisited (FUN_00416e70): walk both bands, out-of-game first,
 * clearing every non-null entry record's +0x10 flag word and the word
 * word alongside. Deviations: entry windows and count bounds guarded,
 * records touched through the modeled +0x10 word. */
void sav_clear_visited(void); /* FUN_00416e70 */
void sav_visited_entry_set(int which, int index, void* record,
                           uint32_t word);             /* port hook */
void sav_visited_entry_peek(int which, int index, void** out_record,
                            uint32_t* out_word);       /* port hook */

/* MenuListFindName (FUN_00414c90): walks the corpus's list header
 * {record-array pointer @+0, count @+4} and answers the index of the
 * first non-NULL entry whose name case-fold-equals the target (the
 * corpus's __strcmpi shape); misses and non-positive counts answer -1.
 * The unowned header rides an explicit pointer; its count and entry-array
 * fields are at +4 and +0, matching the corpus record. */
int menu_list_find_name(const void* list_header,
                        const char* name);                /* FUN_00414c90 */

/* --- screen-effect release (2026-10-24; findings 434/438) ------------------
 * ScreenEffectFlagsClearAndRelease (FUN_0040a3d0): walks the ten screen-
 *   effect entries (the corpus's DAT_004b8aa0 table stride) clearing active
 *   flag bits and sending each entry's handle through the visibility seam
 *   with mode 1. The table has no ported owner; it lands here as zero-init
 *   statics and the seam rides a recorded hook (the bobbin-drop precedent).
 * Deviation: the corpus blind-calls the seam even for cleared handle slots. */
void screen_effect_flags_clear_and_release(void);  /* FUN_0040a3d0 */
void  screen_effect_entry_set(int index, void* handle, uint32_t flags);
                                                /* port hook */
void* screen_effect_handle_peek(int index);       /* port hook */
uint32_t screen_effect_flag_peek(int index);      /* port hook */
void  screen_effect_seam_reset(void);             /* port hook */
int   screen_effect_seam_calls(void);             /* port hook */
void  screen_effect_seam_last(void** out_handle, int* out_flag);
                                                /* port hook */

/* StoreSelectedMapCoordinate (FUN_0041ea50): the selection-coordinate
 * store. A fresh kind — or the tracked word changing under the interaction
 * flag's bit 7 — takes the switch branch: bit 8 raises, bit 7 drops, and
 * the alias-0x16 cue fires. The tracked case refreshes the word, stores the
 * pair into the live slots through the non-NULL path, and always re-arms
 * the kind word.
 * Deviations: the corpus strides its DAT band blindly (the port's own
 * storage, the other owners unported); the pair rides as two words. */
void store_selected_map_coordinate(int kind, const uint32_t coord[2]);
                                                /* FUN_0041ea50 */
uint32_t map_store_pair_word(int which);        /* port hook: 0/1 live pair */
uint32_t map_store_last_word(int which);        /* port hook: 0/1 last pair */
uint32_t map_store_kind_peek(void);             /* port hook */

struct GameContext;

/* TextLayoutOffsetForCentering (FUN_00411320): the centered-widget pointer
 * test. With the center flag raised the widget rect shifts left by half
 * its width before the closed-edge hit test runs; the corpus discards the
 * hit result in the caller's EAX, so the port returns it (deviation). */
int text_layout_offset_for_centering(int x, int y, int rect_x, int rect_y,
                                     int rect_w, int rect_h, int center);
                                                /* FUN_00411320 */

/* SelectionEntityCodeResolve (FUN_0041b9c0): the selection entity code.
 * While the interaction word's bit 8 stands with bit 7 clear, the stored
 * code answers unchanged; otherwise the first selected record's high flag
 * bit picks 0x22 (raised) or 0x23 (clear).
 * Deviations: the corpus strides the interaction word and the context
 * global blindly (the port threads the state block); a NULL first
 * selection answers 0x23 where the corpus dereferences. */
int selection_entity_code_resolve(const struct GameContext* ctx,
                                  int* code_out); /* FUN_0041b9c0 */

/* ScrollRectResourceRecordCreate (FUN_00463770): allocate a 0x1c-float
 * rect record, fill it from the quad (x, y, x+dx, y+dy, then the seam-
 * derived dims where a texture handle rides), and store it in the view
 * window's rect half — A for mode 0, B otherwise (the corpus's param*8
 * stride maps to the port's view slots). Deviations: the window guards
 * and the surface-load seam answering NULL (the panel-load precedent),
 * so the dim halves stay zero. */
void scroll_rect_resource_record_create(int view, int mode,
                                        const float* quad, int tex_a,
                                        int tex_b);

/* --- overlay animation float (2026-10-25) ---------------------------------
 * UiOverlayAnimationAdvance (FUN_0045c580) steps the DAT_00500820
 * animation float through the corpus's convergence loop: mode 0 decays
 * by `(v - 0.05) * 20 * 0.05263158` per step, nonzero mode eases toward
 * 1.0 by `(1 - v) * 0.05` per step, and the negative guard lands after
 * the loop (clamping afterwards, not during). The overlay band is
 * reference state; the port models the animated float as guarded own
 * storage with install/peek hooks (the unowned-global deviation), and
 * the corpus's x87 float10 storage lands as plain float (deviation). */
void ui_overlay_animation_advance(int mode, int steps); /* FUN_0045c580 */
float overlay_anim_float_peek(void);           /* port hook */
void  overlay_anim_float_install(float value); /* port hook */

#endif /* LEGORRR_GAME_FRONTEND_HPP */
