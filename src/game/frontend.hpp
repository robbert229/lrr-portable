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
    int   tried;  /* 1 = open attempted (missing files are not retried) */
} FeOverlay;

typedef struct FeSubmenu {
    int   item_count;             /* cfg ItemCount + grafted chain rows */
    FeItem items[FE_ITEMS_MAX];
    char  title[FE_NAME_MAX];     /* Title key (underscored display text) */
    char  image[FE_PATH_MAX];     /* MenuImage / MenuImageDark (render lane) */
    char  lofont[FE_PATH_MAX];    /* LoFont key — unselected items (Menu_Font_Lo.bmp) */
    char  hifont[FE_PATH_MAX];    /* HiFont key — selected items  (Menu_Font_Hi.bmp) */
    int   base_x, base_y;         /* Position key (x:y) */
    int   auto_center;            /* AutoCenter TRUE|FALSE */
    int   display_title;          /* DisplayTitle TRUE|FALSE */
    int   play_random;            /* PlayRandom: one overlay plays per display */
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

    /* CanScroll plane pan lives on each FeSubmenu (corpus DAT_005574e0
     * is reset when a menu re-enters — frontend_open clears every sub). */
    int  view_w, view_h;            /* render viewport (set each render)  */
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
 * selection highlight behind the selected row, and the Title when
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

/* Public 3x5-face text blit (scale 2) for in-game overlays — the same
 * typeface frontend_render falls back to. */
void frontend_draw_text(void* px, int pitch, int w, int h, int x, int y,
                        const char* s, int r, int g, int b);

/* The in-game Help Window (issue #85): the panel the Help_Window option
 * toggles — drawn while the option is On. `text` (NULL for the demo key
 * lines) fills the info lines. */
void frontend_help_overlay(void* px, int pitch, int w, int h,
                           const char* text);

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_FRONTEND_HPP */