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

#ifdef __cplusplus
extern "C" {
#endif

#define FE_ITEMS_MAX      32   /* ItemCount cap — ≥ the 24-level campaign picker */
#define FE_SUBMENUS_MAX   8    /* MenuCount cap */
#define FE_NAME_MAX       48
#define FE_PATH_MAX       128
#define FE_LEVEL_NAME_MAX 48
#define FE_CHAIN_MAX      24   /* StartLevel/TutorialStartLevel + NextLevel */

/* Item kinds — MenuItemType @ 0x413e60 (values 0..5; 5 = the "Next" catch-
 * all, 1 = Trigger). Slider/Cycle/TextInput parse their banner/label into
 * FeItem.label but are display-only in this milestone. */
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
} FeItem;

typedef struct FeSubmenu {
    int   item_count;             /* cfg ItemCount + grafted chain rows */
    FeItem items[FE_ITEMS_MAX];
    char  title[FE_NAME_MAX];     /* Title key (underscored display text) */
    char  image[FE_PATH_MAX];     /* MenuImage / MenuImageDark (render lane) */
    int   base_x, base_y;         /* Position key (x:y) */
    int   auto_center;            /* AutoCenter TRUE|FALSE */
    int   display_title;          /* DisplayTitle TRUE|FALSE */
} FeSubmenu;

typedef struct FeMenu {
    char      name[FE_NAME_MAX];  /* "MainMenuFull" & co. */
    int       submenu_count;      /* MenuCount */
    FeSubmenu submenus[FE_SUBMENUS_MAX];
} FeMenu;

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

    /* --- runtime (modal menu) state ------------------------------------ */
    int  open;                      /* 1 while the menu loop is showing   */
    int  menu;                      /* FE_MENU_* (only MAIN is reachable) */
    int  sub;                       /* active submenu index               */
    int  sel;                       /* selected item index                */
    int  quit;                      /* user chose Quit (Yes on the confirm) */
    char pending_level[FE_LEVEL_NAME_MAX]; /* level group to load (no "Levels::") */
} FrontEnd;

/* One frame of modal-menu input: key *edges* this frame (the frame loop
 * derives these from input_key_edge; tests drive them directly). */
typedef struct FeInput {
    int up, down;     /* move selection */
    int enter;        /* activate selection */
    int esc;          /* back one submenu / close */
} FeInput;

typedef enum {
    FE_EVT_NONE = 0,
    FE_EVT_START_LEVEL,   /* frontend.pending_level holds the level group */
    FE_EVT_QUIT,          /* user quit the app from the front end */
} FeEvent;

/* --- lifecycle ---------------------------------------------------------- */

void frontend_init(FrontEnd* fe);

/* Parse the four menus from `cfg` (Menus FE_MENU_*), then graft the level
 * pickers (campaign/tutorial chains) onto the sub-menus whose MenuImage
 * names the level-picker art. Returns 1 when MainMenuFull parsed. The
 * level chain is populated from StartLevel/TutorialStartLevel + NextLevel. */
int  frontend_build(FrontEnd* fe, BlockFile* cfg);

/* The FrontEndActive gate: `Lego* Main FrontEnd` cfg flag (finding 23).
 * (The corpus ANDs the loader-mode check DAT_00505f88 < 3; the port only
 * tracks the flag — the loader-mode notion is Win32-side.) */
int  frontend_enabled(BlockFile* cfg);

/* Enter the modal menu at `menu`/submenu 0, selection 0. */
int  frontend_open(FrontEnd* fe, int menu);
void frontend_close(FrontEnd* fe);

/* Step the modal menu one frame. Returns the event the frame loop must act
 * on; FE_EVT_NONE while the menu stays up. See frontend_tick in the .cpp
 * for the action table (level picker -> start, Trigger flag -> quit, Next ->
 * navigate submenus, Esc -> back/close). */
FeEvent frontend_tick(FrontEnd* fe, const FeInput* in);

/* --- accessors ---------------------------------------------------------- */

const FeMenu* frontend_menu(const FrontEnd* fe, int menu);
const FeSubmenu* frontend_active_submenu(const FrontEnd* fe);
const FeItem* frontend_active_item(const FrontEnd* fe);

/* --- render (finding 22 §3 lane) ---------------------------------------- */

/* Draw the active submenu into a locked RGBA32 framebuffer (R,G,B,A bytes):
 * item labels at their anchors (AutoCenter → centered on base_x), a
 * selection highlight behind the selected row, and the Title when
 * display_title. The background / MenuImage blit is the caller's (it owns
 * the texture lane); this draws the menu layer only. Map labels replace
 * '_' with ' '. The font is the port's 3x5 (scale 2). */
void frontend_render(const FrontEnd* fe, int w, int h, void* px, int pitch);

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_FRONTEND_HPP */