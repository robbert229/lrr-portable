// SPDX-License-Identifier: LGPL-3.0-or-later
/* Game layer — front end / main menu (finding 23). See frontend.hpp.
 *
 * Reading the menu groups follows the shipped chip query shape exactly:
 * "<root> Menu::<name> <Menu%i> <key>" with the `::` group separator
 * (DAT_004aab18 — block.cpp tokenizes both ' ' and "::"). The level
 * pickers graft the StartLevel / TutorialStartLevel + NextLevel chains
 * (LevelChainBuild @ 0x4154b0 semantics) onto the picker sub-menus; each
 * row carries the level's FrontEndX/Y scatter position (the config's
 * per-level cavern coords) so the picker looks like a map.
 *
 * Actions (frontend_tick) — keyboard-first for this milestone:
 *   Enter on a picker row        -> FE_EVT_START_LEVEL (pending_level set)
 *   Enter on a Trigger, flag!=0  -> FE_EVT_QUIT       (the ship's "Yes" quit)
 *   Enter on a Next to "Menu%i"  -> navigate submenus
 *   Esc on a submenu             -> back to the root submenu
 *   Esc on the root submenu      -> quit
 *   Slider/Cycle/TextInput       -> display-only (parsed, not editable)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "frontend.hpp"
#include "gods98/flic.hpp"
#include "util.hpp"

static int s_lower(int c) { return (c >= 'A' && c <= 'Z') ? c + 0x20 : c; }
static int s_toupper(int c) { return (c >= 'a' && c <= 'z') ? c - 0x20 : c; }

static int s_strcaseeq(const char* a, const char* b)
{
    if (a == NULL || b == NULL) {
        return a == b;
    }
    while (*a && *b) {
        if (s_lower((unsigned char)*a) != s_lower((unsigned char)*b)) {
            return 0;
        }
        ++a; ++b;
    }
    return *a == *b;
}

/* Case-insensitive substring search ("LevelPick" in "Interface\LevelPicker\…"). */
static int s_stristr(const char* haystack, const char* needle)
{
    if (haystack == NULL || needle == NULL) {
        return 0;
    }
    for (size_t i = 0; haystack[i] != '\0'; i++) {
        size_t j = 0;
        while (needle[j] != '\0'
               && s_lower((unsigned char)haystack[i + j])
                  == s_lower((unsigned char)needle[j])) {
            ++j;
        }
        if (needle[j] == '\0') {
            return 1;
        }
    }
    return 0;
}

/* "Levels::Tutorial01" -> "Tutorial01" (the level group name). A name with
 * no "::" passes through. */
static const char* fe_level_group_name(const char* name)
{
    if (name == NULL) {
        return NULL;
    }
    const char* p = strstr(name, "::");
    return p != NULL ? p + 2 : name;
}

/* The cfg submenu record name ("Menu<i+1>"). */
static const char* fe_menu_name_idx(int idx)
{
    static char n[16];
    snprintf(n, sizeof(n), "Menu%d", idx + 1);
    return n;
}

/* --- config value helpers (Levels-container aware, like #37) ------------- */

static const char* fe_cfg_str(BlockFile* cfg, const char* level,
                              const char* key, char* out, size_t out_sz)
{
    if (cfg == NULL || level == NULL || key == NULL) {
        return NULL;
    }
    const char* root = util_root_name(cfg);
    char path[FE_PATH_MAX * 2];
    snprintf(path, sizeof(path), "%s Levels %s %s", root, level, key);
    const char* v = block_get_strptr(cfg, path);
    if (v == NULL) {
        snprintf(path, sizeof(path), "%s %s %s", root, level, key);
        v = block_get_strptr(cfg, path);
    }
    if (v == NULL) {
        return NULL;
    }
    snprintf(out, out_sz, "%s", v);
    return out;
}

static int fe_cfg_int(BlockFile* cfg, const char* level, const char* key,
                      int dflt)
{
    char buf[FE_PATH_MAX * 2];
    const char* v = fe_cfg_str(cfg, level, key, buf, sizeof(buf));
    if (v == NULL) {
        return dflt;
    }
    return (int)strtol(v, NULL, 10);
}

/* --- the StartLevel / TutorialStartLevel + NextLevel chains -------------- */

static int fe_build_chain(BlockFile* cfg, const char* start_key,
                          char names[][FE_LEVEL_NAME_MAX],
                          char labels[][FE_NAME_MAX], int max)
{
    if (cfg == NULL) {
        return 0;
    }
    const char* root = util_root_name(cfg);
    char path[FE_PATH_MAX * 2];
    snprintf(path, sizeof(path), "%s Main %s", root, start_key);
    const char* v = block_get_strptr(cfg, path);
    if (v == NULL) {
        return 0;
    }
    int n = 0;
    const char* group = fe_level_group_name(v);
    while (group != NULL && group[0] != '\0' && n < max) {
        snprintf(names[n], FE_LEVEL_NAME_MAX, "%s", group);
        char lbl[FE_NAME_MAX];
        if (fe_cfg_str(cfg, group, "FullName", lbl, sizeof(lbl)) != NULL) {
            snprintf(labels[n], FE_NAME_MAX, "%s", lbl);
        } else {
            snprintf(labels[n], FE_NAME_MAX, "%s", group);
        }
        char nl[FE_LEVEL_NAME_MAX];
        const char* next = fe_cfg_str(cfg, group, "NextLevel", nl,
                                      sizeof(nl));
        group = next != NULL ? fe_level_group_name(nl) : NULL;
        ++n;
    }
    return n;
}

/* --- Item%i value grammar ------------------------------------------------- */

static int fe_item_kind(const char* tok)
{
    if (s_strcaseeq(tok, "cycle"))       return FE_ITEM_CYCLE;
    if (s_strcaseeq(tok, "trigger"))     return FE_ITEM_TRIGGER;
    if (s_strcaseeq(tok, "textinput"))   return FE_ITEM_TEXTINPUT;
    if (s_strcaseeq(tok, "slider"))      return FE_ITEM_SLIDER;
    if (s_strcaseeq(tok, "realslider"))  return FE_ITEM_REALSLIDER;
    return FE_ITEM_NEXT;                 /* "Next" and anything unknown */
}

/* Split an Item value ("Next:0:0:Start_Game:Menu2", "Trigger:…:Yes:1",
 * "Slider:x1:y1:x2:y2:banner:lo:hi:img…") into ':'-fields. Position x/y are
 * fields 1/2 for every kind; the display label differs per kind (finding 23):
 * Next/Trigger put it at field 3, Slider/Cycle/RealSlider at field 5
 * (x1:y1:x2:y2), and Trigger's image form carries images+tooltip before the
 * flag. */
static void fe_parse_item(const char* value, FeItem* it)
{
    it->kind = FE_ITEM_NEXT;
    it->x = 0; it->y = 0; it->flag = 0; it->level_start = 0;
    it->label[0] = '\0'; it->target[0] = '\0';
    if (value == NULL) {
        return;
    }
    char fields[12][64];
    int n = 0;
    const char* p = value;
    while (n < 12) {
        while (*p == ':') {
            ++p;
        }
        if (*p == '\0') {
            break;
        }
        const char* s = p;
        while (*p != '\0' && *p != ':') {
            ++p;
        }
        int len = (int)(p - s);
        if (len > 63) {
            len = 63;
        }
        memcpy(fields[n], s, (size_t)len);
        fields[n][len] = '\0';
        ++n;
    }
    if (n == 0) {
        return;
    }
    it->kind = fe_item_kind(fields[0]);
    int atoi1 = (n > 1) ? (int)strtol(fields[1], NULL, 10) : 0;
    int atoi2 = (n > 2) ? (int)strtol(fields[2], NULL, 10) : 0;
    switch (it->kind) {
    case FE_ITEM_NEXT:
        it->x = atoi1;
        it->y = atoi2;
        if (n > 3) {
            snprintf(it->label, FE_NAME_MAX, "%s", fields[3]);
        }
        if (n > 4) {
            snprintf(it->target, FE_NAME_MAX, "%s", fields[4]);
        }
        break;
    case FE_ITEM_TRIGGER: {
        it->x = atoi1;
        it->y = atoi2;
        /* image trigger form: Trigger:x:y:imgN:imgG:imgD:tooltip:1 */
        int image_form = (n > 3) && s_stristr(fields[3], ".bmp");
        if (image_form) {
            if (n > 6) {
                snprintf(it->label, FE_NAME_MAX, "%s", fields[6]);
            } else {
                snprintf(it->label, FE_NAME_MAX, "…");
            }
            if (n > 7) {
                it->flag = (int)strtol(fields[7], NULL, 10);
            }
        } else {
            if (n > 3) {
                snprintf(it->label, FE_NAME_MAX, "%s", fields[3]);
            }
            if (n > 4) {
                it->flag = (int)strtol(fields[4], NULL, 10);
            }
        }
        break;
    }
    default: /* slider / cycle / realslider / textinput: banner at field 5 */
        it->x = atoi1;
        it->y = atoi2;
        if (n > 5) {
            snprintf(it->label, FE_NAME_MAX, "%s", fields[5]);
        }
        if (it->label[0] == '\0' && n > 3) {
            snprintf(it->label, FE_NAME_MAX, "%s", fields[3]);
        }
        if (n > 6 && it->kind == FE_ITEM_SLIDER) {
            it->flag = (int)strtol(fields[6], NULL, 10);
        }
        break;
    }
}

/* --- menu group building -------------------------------------------------- */

static void fe_parse_submenu(FrontEnd* fe, FeMenu* m, const char* name,
                             int idx)
{
    FeSubmenu* sub = &m->submenus[idx];
    sub->item_count = 0;
    sub->base_x = sub->base_y = 0;
    sub->auto_center = 0;
    sub->display_title = 0;
    sub->title[0] = sub->image[0] = '\0';
    memset(sub->items, 0, sizeof(sub->items));
    char pre[FE_PATH_MAX * 2];
    snprintf(pre, sizeof(pre), "%s Menu::%s %s", util_root_name(fe->cfg),
             name, fe_menu_name_idx(idx));
    char buf[FE_PATH_MAX * 2];

    snprintf(buf, sizeof(buf), "%s Title", pre);
    if (block_get_strptr(fe->cfg, buf) != NULL) {
        snprintf(sub->title, FE_NAME_MAX, "%s", block_get_strptr(fe->cfg, buf));
    }

    /* Position x:y */
    snprintf(buf, sizeof(buf), "%s Position", pre);
    const char* v = block_get_strptr(fe->cfg, buf);
    if (v != NULL) {
        const char* c = strchr(v, ':');
        if (c != NULL) {
            char xb[16], yb[16];
            int xl = (int)(c - v) < 15 ? (int)(c - v) : 15;
            memcpy(xb, v, (size_t)xl); xb[xl] = '\0';
            snprintf(yb, sizeof(yb), "%s", c + 1);
            sub->base_x = (int)strtol(xb, NULL, 10);
            sub->base_y = (int)strtol(yb, NULL, 10);
        }
    }

    snprintf(buf, sizeof(buf), "%s AutoCenter", pre);
    sub->auto_center = block_get_bool(fe->cfg, buf) == 1;
    snprintf(buf, sizeof(buf), "%s DisplayTitle", pre);
    sub->display_title = block_get_bool(fe->cfg, buf) == 1;
    snprintf(buf, sizeof(buf), "%s MenuImage", pre);
    v = block_get_strptr(fe->cfg, buf);
    if (v != NULL) {
        /* "img.bmp:0:0:1" -> keep only the path part (temp first:
         * snprintf with overlapping src/dst would be UB) */
        char raw[FE_PATH_MAX];
        snprintf(raw, sizeof(raw), "%s", v);
        const char* c = strchr(raw, ':');
        snprintf(sub->image, FE_PATH_MAX, "%.*s",
                 c != NULL ? (int)(c - raw) : (int)strlen(raw), raw);
    }

    /* Bitmap fonts: LoFont = unselected, HiFont = selected (MenuFont is
     * the measurement face — fall back to it for both when the pair is
     * absent). */
    snprintf(buf, sizeof(buf), "%s LoFont", pre);
    if (block_get_strptr(fe->cfg, buf) != NULL) {
        snprintf(sub->lofont, FE_PATH_MAX, "%s",
                 block_get_strptr(fe->cfg, buf));
    }
    snprintf(buf, sizeof(buf), "%s HiFont", pre);
    if (block_get_strptr(fe->cfg, buf) != NULL) {
        snprintf(sub->hifont, FE_PATH_MAX, "%s",
                 block_get_strptr(fe->cfg, buf));
    }
    if (sub->lofont[0] == '\0' || sub->hifont[0] == '\0') {
        snprintf(buf, sizeof(buf), "%s MenuFont", pre);
        const char* mf = block_get_strptr(fe->cfg, buf);
        if (mf != NULL) {
            if (sub->lofont[0] == '\0') {
                snprintf(sub->lofont, FE_PATH_MAX, "%s", mf);
            }
            if (sub->hifont[0] == '\0') {
                snprintf(sub->hifont, FE_PATH_MAX, "%s", mf);
            }
        }
    }

    snprintf(buf, sizeof(buf), "%s ItemCount", pre);
    int ic = block_get_int(fe->cfg, buf, 0);
    if (ic > FE_ITEMS_MAX) {
        ic = FE_ITEMS_MAX;
    }
    sub->item_count = ic;
    for (int j = 0; j < ic; j++) {
        snprintf(buf, sizeof(buf), "%s Item%d", pre, j + 1);
        const char* item = block_get_strptr(fe->cfg, buf);
        if (item == NULL) {
            continue;
        }
        fe_parse_item(item, &sub->items[j]);
    }

    /* FLIC overlays (`!Overlay%i <avi\X.flh>:<SFX>:x:y`) + PlayRandom —
     * finding 23's decoration lane; the `.flh` players open lazily at
     * display time (the paths may be loose Data/AVI copies). */
    snprintf(buf, sizeof(buf), "%s PlayRandom", pre);
    sub->play_random = block_get_bool(fe->cfg, buf) == 1;
    sub->overlay_count = 0;
    for (int j = 1; j <= FE_OVERLAYS_MAX && sub->overlay_count < FE_OVERLAYS_MAX;
         ++j) {
        snprintf(buf, sizeof(buf), "%s !Overlay%d", pre, j);
        const char* ov = block_get_strptr(fe->cfg, buf);
        if (ov == NULL || ov[0] == '\0') {
            continue;
        }
        FeOverlay* o = &sub->overlays[sub->overlay_count];
        o->path[0] = o->sfx[0] = '\0';
        o->x = o->y = 0;
        o->flic = NULL;
        /* value = path:sfx:x:y (path may itself contain ':' — take the
         * first token as path, then sfx, x, y). */
        const char* p = ov;
        const char* c1 = strchr(p, ':');
        size_t pl = c1 != NULL ? (size_t)(c1 - p) : strlen(p);
        if (pl >= sizeof(o->path)) {
            pl = sizeof(o->path) - 1;
        }
        memcpy(o->path, p, pl);
        o->path[pl] = '\0';
        if (c1 != NULL) {
            const char* sfx = c1 + 1;
            const char* c2 = strchr(sfx, ':');
            size_t sl = c2 != NULL ? (size_t)(c2 - sfx) : strlen(sfx);
            if (sl >= sizeof(o->sfx)) {
                sl = sizeof(o->sfx) - 1;
            }
            memcpy(o->sfx, sfx, sl);
            o->sfx[sl] = '\0';
            if (c2 != NULL) {
                o->x = (int)strtol(c2 + 1, NULL, 10);
                const char* c3 = strchr(c2 + 1, ':');
                if (c3 != NULL && c3[1] != '\0') {
                    o->y = (int)strtol(c3 + 1, NULL, 10);
                }
            }
        }
        ++sub->overlay_count;
    }
}

static int fe_parse_menu(FrontEnd* fe, int mi, const char* name)
{
    FeMenu* m = &fe->menus[mi];
    m->submenu_count = 0;
    snprintf(m->name, FE_NAME_MAX, "%s", name);
    if (fe->cfg == NULL) {
        return 0;
    }
    char buf[FE_PATH_MAX * 2];
    snprintf(buf, sizeof(buf), "%s Menu::%s MenuCount", util_root_name(fe->cfg),
             name);
    int count = block_get_int(fe->cfg, buf, 0);
    if (count > FE_SUBMENUS_MAX) {
        count = FE_SUBMENUS_MAX;
    }
    m->submenu_count = count;
    for (int i = 0; i < count; i++) {
        fe_parse_submenu(fe, m, name, i);
    }
    return count > 0;
}

/* Graft a level chain onto the picker sub-menus: each row becomes a Next
 * item with x/y = the level's FrontEndX/Y scatter position and
 * level_start = 1. Campaign -> "LevelPick" art, tutorial -> "LevelPickT". */
static void fe_graft_picker(FrontEnd* fe, FeMenu* m, int tutorial)
{
    for (int i = 0; i < m->submenu_count; i++) {
        FeSubmenu* sub = &m->submenus[i];
        if (tutorial) {
            if (!s_stristr(sub->image, "LevelPickT")) {
                continue;
            }
        } else {
            if (!s_stristr(sub->image, "LevelPick")
                || s_stristr(sub->image, "LevelPickT")) {
                continue;
            }
        }
        int base = sub->item_count;
        int n = tutorial ? fe->tutorial_n : fe->campaign_n;
        for (int k = 0; k < n && base + k < FE_ITEMS_MAX; k++) {
            FeItem* it = &sub->items[base + k];
            it->kind = FE_ITEM_NEXT;
            it->level_start = 1;
            const char* lvl = tutorial ? fe->tutorial[k] : fe->campaign[k];
            snprintf(it->target, FE_NAME_MAX, "%s", lvl);
            snprintf(it->label, FE_NAME_MAX, "%s",
                     tutorial ? fe->tutorial_label[k] : fe->campaign_label[k]);
            it->x = fe_cfg_int(fe->cfg, lvl, "FrontEndX", 0);
            it->y = fe_cfg_int(fe->cfg, lvl, "FrontEndY", 0);
            sub->item_count = base + k + 1;
        }
    }
}

/* --- public API ----------------------------------------------------------- */

/* Close every open overlay flic (menu switch / close). */
static void fe_overlay_close_all(FrontEnd* fe)
{
    for (int m = 0; m < FE_MENU_COUNT; ++m) {
        for (int s = 0; s < fe->menus[m].submenu_count; ++s) {
            FeSubmenu* sub = &fe->menus[m].submenus[s];
            for (int i = 0; i < sub->overlay_count; ++i) {
                if (sub->overlays[i].flic != NULL) {
                    flic_close(sub->overlays[i].flic);
                    sub->overlays[i].flic = NULL;
                }
            }
        }
    }
    fe->overlays_for = NULL;
}

/* Open (and tick-start) the active submenu's overlay flics. PlayRandom
 * submenus play one chosen overlay; others play all listed. Missing files
 * (paths that don't resolve) leave the overlay's flic NULL — a no-op. */
const FeSubmenu* frontend_active_submenu(const FrontEnd* fe);

static void fe_overlay_open(FrontEnd* fe, const FeSubmenu* sub, int picked)
{
    if (fe == NULL || sub == NULL) {
        return;
    }
    for (int i = 0; i < sub->overlay_count; ++i) {
        FeOverlay* o = (FeOverlay*)&sub->overlays[i];
        if ((sub->play_random && i != picked) || o->tried) {
            continue;
        }
        o->tried = 1;
        o->flic = flic_open(o->path);
    }
    fe->overlays_for = sub;
}

/* Ensure the current submenu's overlays are open (re-opening on menu
 * switches); advances each open flic one frame. */
static void fe_overlay_step(FrontEnd* fe)
{
    if (fe == NULL || !fe->open) {
        return;
    }
    const FeSubmenu* sub = frontend_active_submenu(fe);
    if (sub == NULL || sub->overlay_count == 0) {
        if (fe->overlays_for != NULL) {
            fe_overlay_close_all(fe);
        }
        return;
    }
    if (fe->overlays_for != sub) {
        fe_overlay_close_all(fe);
        /* PlayRandom: pick one overlay per display; the frame counter is a
         * decent shake. */
        int picked = sub->play_random
                         ? (int)(fe->sel * 7919u % (unsigned)sub->overlay_count)
                         : -1;
        fe_overlay_open(fe, sub, picked);
    }
    for (int i = 0; i < sub->overlay_count; ++i) {
        if (sub->overlays[i].flic != NULL) {
            flic_tick_loop(sub->overlays[i].flic, 1);
        }
    }
}

void frontend_init(FrontEnd* fe)
{
    if (fe == NULL) {
        return;
    }
    memset(fe, 0, sizeof(*fe));
}

int frontend_build(FrontEnd* fe, BlockFile* cfg)
{
    if (fe == NULL) {
        return 0;
    }
    frontend_init(fe);
    fe->cfg = cfg;
    if (cfg == NULL) {
        return 0;
    }

    fe->campaign_n = fe_build_chain(cfg, "StartLevel",
                                    fe->campaign, fe->campaign_label,
                                    FE_CHAIN_MAX);
    fe->tutorial_n = fe_build_chain(cfg, "TutorialStartLevel",
                                    fe->tutorial, fe->tutorial_label,
                                    FE_CHAIN_MAX);

    int main_ok = fe_parse_menu(fe, FE_MENU_MAIN, "MainMenuFull");
    fe_parse_menu(fe, FE_MENU_SAVE, "SaveMenu");
    fe_parse_menu(fe, FE_MENU_PAUSED, "PausedMenu");
    fe_parse_menu(fe, FE_MENU_OPTIONS, "OptionsMenu");

    /* graft the level pickers onto the main menu's picker sub-menus */
    fe_graft_picker(fe, &fe->menus[FE_MENU_MAIN], 0);
    fe_graft_picker(fe, &fe->menus[FE_MENU_MAIN], 1);

    fe->built = main_ok;
    return main_ok;
}

int frontend_enabled(BlockFile* cfg)
{
    if (cfg == NULL) {
        return 0;
    }
    /* FrontEndActive @ 0x416d00: the cfg flag (loader-mode <3 is Win32). */
    return block_get_bool(cfg, "Lego* Main FrontEnd") == 1;
}

int frontend_open(FrontEnd* fe, int menu)
{
    if (fe == NULL || !fe->built) {
        return 0;
    }
    fe->menu = menu;
    fe->sub = 0;
    fe->sel = 0;
    fe->open = 1;
    fe->quit = 0;
    fe->pending_level[0] = '\0';
    fe_overlay_step(fe); /* open the active submenu's flic overlays */
    return 1;
}

void frontend_close(FrontEnd* fe)
{
    if (fe != NULL) {
        fe->open = 0;
        fe_overlay_close_all(fe);
    }
}

/* Pointer hit-testing (defined with the render helpers below) uses the same
 * placement arithmetic + measure-font widths as drawing. */
static int fe_item_pick(const FrontEnd* fe, int mx, int my,
                        const TextureSurface* measure, int* out_idx);

/* --- activation (Enter or a pointer click resolves to this) -------------- */

static FeEvent fe_activate(FrontEnd* fe)
{
    FeMenu* m = &fe->menus[fe->menu];
    FeSubmenu* sub = &m->submenus[fe->sub];
    FeItem* it = &sub->items[fe->sel];

    /* picker row: start the selected level */
    if (it->level_start) {
        snprintf(fe->pending_level, FE_LEVEL_NAME_MAX, "%s", it->target);
        fe->open = 0;
        return FE_EVT_START_LEVEL;
    }

    /* positive Trigger flags resolve by menu context (the shipped buttons):
     * on the pause overlay they are Continue / the quit-confirm / the
     * restart-confirm; on the main menu the only one is the Quit confirm. */
    if (it->kind == FE_ITEM_TRIGGER && it->flag != 0) {
        if (fe->menu == FE_MENU_PAUSED) {
            switch (fe->sub) {
            case 0:   /* pause Menu1: Continue_Game / Replay_Objective */
                if (s_stristr(it->label, "continue")) {
                    fe->open = 0;
                    return FE_EVT_RESUME;
                }
                return FE_EVT_NONE;   /* Replay_Objective: stub */
            case 2:   /* pause Menu3 = quit confirm ("Yes") */
                fe->quit = 1;
                fe->open = 0;
                return FE_EVT_QUIT;
            case 3:   /* pause Menu4 = restart confirm ("Yes") */
                snprintf(fe->pending_level, FE_LEVEL_NAME_MAX, "%s",
                         fe->resume_level[0] != '\0' ? fe->resume_level
                                                     : it->target);
                fe->open = 0;
                return FE_EVT_START_LEVEL;
            default:  /* options submenus: Replay_Objective etc. — stub */
                return FE_EVT_NONE;
            }
        }
        fe->quit = 1;
        fe->open = 0;
        return FE_EVT_QUIT;
    }

    /* Next -> navigate submenus ("Menu%i" target) */
    if (it->kind == FE_ITEM_NEXT && it->target[0] != '\0'
        && s_stristr(it->target, "menu") != 0) {
        int idx = (int)strtol(it->target + 4, NULL, 10) - 1; /* "MenuN" */
        if (idx >= 0 && idx < m->submenu_count) {
            fe->sub = idx;
            fe->sel = 0;
        }
        return FE_EVT_NONE;
    }

    /* Slider / Cycle / TextInput / plain Trigger (flag 0): no-op this
     * milestone (parsed + rendered, not editable). */
    return FE_EVT_NONE;
}

FeEvent frontend_tick(FrontEnd* fe, const FeInput* in,
                      const TextureSurface* measure)
{
    if (fe == NULL || in == NULL || !fe->open) {
        return FE_EVT_NONE;
    }
    FeMenu* m = &fe->menus[fe->menu];
    if (fe->sub < 0 || fe->sub >= m->submenu_count) {
        fe->sub = 0;
    }
    FeSubmenu* sub = &m->submenus[fe->sub];
    if (sub->item_count <= 0) {
        fe_overlay_step(fe); /* still advance any overlay animations */
        return FE_EVT_NONE;
    }
    if (fe->sel >= sub->item_count) {
        fe->sel = sub->item_count - 1;
    }
    if (fe->sel < 0) {
        fe->sel = 0;
    }
    /* .flh overlay lane (finding 23): open + advance the active submenu's
     * flics each frame (PlayRandom picks one at open). */
    fe_overlay_step(fe);

    if (in->up) {
        if (fe->sel > 0) {
            --fe->sel;
        }
    }
    if (in->down) {
        if (fe->sel < sub->item_count - 1) {
            ++fe->sel;
        }
    }

    /* pointer: hover selects; a click activates the hovered row (Enter).
     * When the pointer is in use but outside every item, a click is a
     * no-op (the menu stays up). */
    if (in->mouse_x >= 0 && in->mouse_y >= 0) {
        int idx = 0;
        int hit = fe_item_pick(fe, in->mouse_x, in->mouse_y, measure, &idx);
        if (hit) {
            fe->sel = idx;
        }
        if (in->click) {
            if (hit) {
                return fe_activate(fe);
            }
            return FE_EVT_NONE;
        }
    }

    if (in->esc) {
        if (fe->sub != 0) {
            fe->sub = 0;
            fe->sel = 0;
        } else if (fe->menu == FE_MENU_PAUSED) {
            fe->open = 0;             /* pause Esc = resume */
            return FE_EVT_RESUME;
        } else {
            fe->quit = 1;
            fe->open = 0;
            return FE_EVT_QUIT;
        }
        return FE_EVT_NONE;
    }
    if (!in->enter) {
        return FE_EVT_NONE;
    }
    return fe_activate(fe);
}

const FeMenu* frontend_menu(const FrontEnd* fe, int menu)
{
    static const FeMenu s_empty = {};
    if (fe == NULL || menu < 0 || menu >= FE_MENU_COUNT) {
        return &s_empty;
    }
    return &fe->menus[menu];
}

const FeSubmenu* frontend_active_submenu(const FrontEnd* fe)
{
    static const FeSubmenu s_empty = {};
    if (fe == NULL || !fe->open || fe->menu < 0 || fe->menu >= FE_MENU_COUNT) {
        return &s_empty;
    }
    const FeMenu* m = &fe->menus[fe->menu];
    if (fe->sub < 0 || fe->sub >= m->submenu_count) {
        return &s_empty;
    }
    return &m->submenus[fe->sub];
}

const FeItem* frontend_active_item(const FrontEnd* fe)
{
    static const FeItem s_empty = { 0, 0, 0, {0}, {0}, 0, 0 };
    const FeSubmenu* s = frontend_active_submenu(fe);
    if (s == NULL || fe->sel < 0 || fe->sel >= s->item_count) {
        return &s_empty;
    }
    return &s->items[fe->sel];
}

const char* frontend_active_image(const FrontEnd* fe)
{
    const FeSubmenu* s = frontend_active_submenu(fe);
    return (s != NULL && s->image[0] != '\0') ? s->image : NULL;
}

/* --- render -----------------------------------------------------------------
 * Software blit of the active submenu (finding 22 §3 lane). Background and
 * MenuImage blits are the caller's; this draws the item layer with the
 * shipped bitmap fonts (HiFont selected / LoFont unselected, FUN_0047a470
 * 10x19 grid), falling back to the port's own 3x5 typeface when the frame
 * loop supplies none. Selection = the classic highlight bar + bright face.
 * ------------------------------------------------------------------------- */

/* 3x5 glyphs (5 rows, bit2 = left column). Uppercase + digits + the
 * punctuation the shipped labels use. Our typeface ("palette is ours" —
 * README "Software level renderer"), not a clone. */
static const int8_t s_font_caps[26][5] = {
    {5,7,5,5,5}, {7,5,7,5,7}, {7,4,4,4,7}, {6,5,5,5,6},
    {7,4,7,4,7}, {7,4,7,4,4}, {7,5,4,5,7}, {5,5,7,5,5},
    {2,2,7,2,2}, {1,1,2,3,7}, {5,6,4,6,5}, {4,4,4,4,7},
    {7,5,5,5,7}, {6,5,5,5,6}, {7,5,5,5,7}, {7,5,7,4,4},
    {7,5,4,5,7}, {7,5,7,4,7}, {7,4,1,2,7}, {7,4,7,4,4},
    {7,5,5,5,7}, {5,5,5,5,7}, {5,5,5,5,7}, {5,2,0,2,5},
    {5,5,5,5,7}, {7,1,2,4,7},
};
static const int8_t s_font_digits[10][5] = {
    {7,5,5,5,7}, {2,6,2,2,7}, {7,4,2,1,7}, {7,1,2,3,7},
    {6,6,7,2,2}, {7,4,1,2,7}, {7,4,6,4,7}, {7,1,1,1,7},
    {7,5,7,5,7}, {7,5,7,4,7},
};
static const int8_t s_font_fallback[5] = { 7, 7, 7, 7, 7 };

static const int8_t* fe_glyph(int ch)
{
    if (ch >= 'A' && ch <= 'Z') {
        return s_font_caps[ch - 'A'];
    }
    if (ch >= 'a' && ch <= 'z') {
        return s_font_caps[s_toupper(ch) - 'A'];
    }
    if (ch >= '0' && ch <= '9') {
        return s_font_digits[ch - '0'];
    }
    switch (ch) {
    case ' ':  return NULL;              /* zero-width */
    case '!':  { static const int8_t g[5] = {2,2,0,2,2}; return g; }
    case '?':  { static const int8_t g[5] = {3,1,2,5,7}; return g; }
    case '-':  { static const int8_t g[5] = {0,0,7,0,0}; return g; }
    case '_':  { static const int8_t g[5] = {0,0,0,0,7}; return g; }
    case '.':  { static const int8_t g[5] = {0,0,0,0,1}; return g; }
    case ',':  { static const int8_t g[5] = {0,0,0,1,1}; return g; }
    case ':':  { static const int8_t g[5] = {0,2,0,2,0}; return g; }
    case '\'': { static const int8_t g[5] = {1,1,0,0,0}; return g; }
    case '/':  { static const int8_t g[5] = {1,2,2,2,4}; return g; }
    case '"':  { static const int8_t g[5] = {5,5,0,0,0}; return g; }
    default:
        return s_font_fallback;
    }
}

#define FE_FONT_SCALE 2

static void fe_set_px(void* px, int pitch, int w, int h, int x, int y,
                      uint8_t r, uint8_t g, uint8_t b)
{
    if (x < 0 || y < 0 || x >= w || y >= h) {
        return;
    }
    uint8_t* row = (uint8_t*)px + (size_t)y * (size_t)pitch + (size_t)x * 4;
    row[0] = r; row[1] = g; row[2] = b; row[3] = 255;
}

static int fe_text_width(const char* s)
{
    return (int)strlen(s) * (3 * FE_FONT_SCALE + 1);
}

static void fe_draw_text(void* px, int pitch, int w, int h, int x, int y,
                         const char* s, int r, int g, int b)
{
    for (int i = 0; s[i] != '\0' && x < w; ++i) {
        int c = (unsigned char)s[i];
        int sc = (c == '_') ? ' ' : s_toupper(c);
        const int8_t* glyph = fe_glyph(sc);
        if (glyph == NULL) {              /* space: no ink, just advance */
            x += 3 * FE_FONT_SCALE + 1;
            continue;
        }
        for (int row = 0; row < 5; row++) {
            int bits = glyph[row];
            for (int col = 0; col < 3; col++) {
                if (bits & (1 << (2 - col))) {
                    for (int sy = 0; sy < FE_FONT_SCALE; sy++) {
                        for (int sx = 0; sx < FE_FONT_SCALE; sx++) {
                            fe_set_px(px, pitch, w, h,
                                      x + col * FE_FONT_SCALE + sx,
                                      y + row * FE_FONT_SCALE + sy,
                                      (uint8_t)r, (uint8_t)g, (uint8_t)b);
                        }
                    }
                }
            }
        }
        x += 3 * FE_FONT_SCALE + 1;
    }
}

static void fe_fill_rect(void* px, int pitch, int w, int h,
                         int x, int y, int rw, int rh, int r, int g, int b)
{
    for (int yy = 0; yy < rh; yy++) {
        for (int xx = 0; xx < rw; xx++) {
            fe_set_px(px, pitch, w, h, x + xx, y + yy,
                      (uint8_t)r, (uint8_t)g, (uint8_t)b);
        }
    }
}

/* --- shipped bitmap-font lane (FUN_0047a470) ------------------------------ */

/* Advance is measured from each cell's ink bounding box (the engine stores
 * per-glyph float advances in the font object at load; we re-derive the
 * same numbers from the image). */

static int fe_font_cell(const TextureSurface* f, int* cw, int* ch)
{
    if (f == NULL || f->pixels == NULL || f->width < FE_FONT_GRID_COLS
        || f->height < FE_FONT_GRID_ROWS) {
        return 0;
    }
    *cw = f->width / FE_FONT_GRID_COLS;
    *ch = f->height / FE_FONT_GRID_ROWS;
    return 1;
}

/* Colour key derivation — the loader samples the key from the image pixels
 * (FUN_0047d9d0), and each shipped face keys its own background: Menu_Font*
 * uses palette-255 pure red, MbriefFont* pure white. The dominant colour of
 * the space cell (glyph 0) is the background in every observed font.
 * FE_FONT_KEY stays as the generic default when a font decodes to a blank
 * cell 0. */
static uint32_t fe_font_key(const TextureSurface* f, int cw, int ch)
{
    uint32_t colors[8];
    unsigned count[8] = { 0 };
    int n = 0;
    for (int yy = 0; yy < ch; yy += (ch > 4 ? 2 : 1)) {
        for (int xx = 0; xx < cw; xx += (cw > 4 ? 2 : 1)) {
            uint32_t p = f->pixels[(size_t)yy * (size_t)f->width + xx];
            int i = 0;
            for (; i < n; i++) {
                if (colors[i] == p) {
                    break;
                }
            }
            if (i == n) {
                if (n == 8) {
                    continue;
                }
                colors[n] = p;
                count[n] = 1;
                n++;
            } else {
                count[i]++;
            }
        }
    }
    if (n == 0) {
        return FE_FONT_KEY;
    }
    unsigned bestc = 0;
    uint32_t best = colors[0];
    for (int i = 1; i < n; i++) {
        if (count[i] > bestc) {
            bestc = count[i];
            best = colors[i];
        }
    }
    return best;
}

/* Ink bounding box of glyph g — the non-key pixels only. The engine blits
 * exactly this box (FUN_00479ee0's right-edge scan / per-glyph rect), which
 * keeps each face's background AND frame strips out while retaining the
 * glyph's dark outline when it is inside the box. */
static int fe_font_ink_box(const TextureSurface* f, int cw, int ch, int g,
                           uint32_t key, int* minx, int* miny, int* maxx,
                           int* maxy)
{
    int col = g % FE_FONT_GRID_COLS, row = g / FE_FONT_GRID_COLS;
    int x0 = col * cw, y0 = row * ch;
    *minx = cw; *miny = ch; *maxx = -1; *maxy = -1;
    for (int yy = 0; yy < ch; yy++) {
        for (int xx = 0; xx < cw; xx++) {
            if (f->pixels[(size_t)(y0 + yy) * (size_t)f->width + (x0 + xx)]
                == key) {
                continue;
            }
            if (xx < *minx) { *minx = xx; }
            if (xx > *maxx) { *maxx = xx; }
            if (yy < *miny) { *miny = yy; }
            if (yy > *maxy) { *maxy = yy; }
        }
    }
    return *maxx >= 0;
}

/* Advance width of glyph g = its ink width from the cell's left edge
 * (the engine's right-edge scan), 0 when blank. */
static int fe_font_advance(const TextureSurface* f, int cw, int ch, int g,
                           uint32_t key)
{
    int mx, my, ax, ay;
    if (!fe_font_ink_box(f, cw, ch, g, key, &mx, &my, &ax, &ay)) {
        return 0;
    }
    return ax + 1;
}

static void fe_blit_glyph(void* px, int pitch, int w, int h,
                          int dx, int dy, const TextureSurface* f,
                          int cw, int ch, int g, uint32_t key)
{
    int col = g % FE_FONT_GRID_COLS, row = g / FE_FONT_GRID_COLS;
    int x0 = col * cw, y0 = row * ch;
    int minx, miny, maxx, maxy;
    if (!fe_font_ink_box(f, cw, ch, g, key, &minx, &miny, &maxx, &maxy)) {
        return;
    }
    for (int yy = miny; yy <= maxy; yy++) {
        int dy2 = dy + yy;
        if (dy2 < 0 || dy2 >= h) {
            continue;
        }
        for (int xx = minx; xx <= maxx; xx++) {
            uint32_t p = f->pixels[(size_t)(y0 + yy) * (size_t)f->width
                                   + (x0 + xx)];
            if (p == key) {
                continue;                              /* key *inside* the box */
            }
            int dx2 = dx + xx;
            if (dx2 < 0 || dx2 >= w) {
                continue;
            }
            uint8_t* o = (uint8_t*)px + (size_t)dy2 * (size_t)pitch
                         + (size_t)dx2 * 4;
            o[0] = (uint8_t)(p >> 16);
            o[1] = (uint8_t)(p >> 8);
            o[2] = (uint8_t)(p);
            o[3] = (uint8_t)(p >> 24);
        }
    }
}

/* Draw `s` with a bitmap font at (x,y). `_` renders as a space, matching
 * the shipped menu labels. Returns the advance. */
static int fe_draw_font_text(void* px, int pitch, int w, int h,
                             int x, int y, const char* s,
                             const TextureSurface* f)
{
    int cw = 0, chh = 0;
    if (!fe_font_cell(f, &cw, &chh)) {
        return 0;
    }
    uint32_t key = fe_font_key(f, cw, chh);
    int cx = x;
    for (int i = 0; s[i] != '\0' && cx < w; ++i) {
        int c = (unsigned char)s[i];
        int g = c - 0x20;
        if (g < 0 || g >= FE_FONT_GRID_COLS * FE_FONT_GRID_ROWS) {
            cx += cw / 2;
            continue;
        }
        int adv = fe_font_advance(f, cw, chh, g, key);
        if (adv <= 0) {
            adv = (c == ' ') ? cw / 2 : cw;        /* blank cell fallback */
        }
        fe_blit_glyph(px, pitch, w, h, cx, y, f, cw, chh, g, key);
        cx += adv;
    }
    return cx - x;
}

static int fe_font_text_width(const char* s, const TextureSurface* f)
{
    int cw = 0, chh = 0;
    if (!fe_font_cell(f, &cw, &chh)) {
        return 0;
    }
    uint32_t key = fe_font_key(f, cw, chh);
    int cx = 0;
    for (int i = 0; s[i] != '\0'; ++i) {
        int c = (unsigned char)s[i];
        int g = c - 0x20;
        if (g < 0 || g >= FE_FONT_GRID_COLS * FE_FONT_GRID_ROWS) {
            cx += cw / 2;
            continue;
        }
        int adv = fe_font_advance(f, cw, chh, g, key);
        cx += adv > 0 ? adv : (c == ' ' ? cw / 2 : cw);
    }
    return cx;
}

/* Item display position — the MenuTick @ 0x412c00 arithmetic: each item
 * draws at (menu.Position.x + item.x [+ centering], menu.Position.y +
 * item.y); AutoCenter centres the label on the Position column (base_x -
 * text_width/2), otherwise the item's own x anchor is added (the shipped
 * Back buttons are e.g. x=-314 on Position 320:200 -> left edge). Picker
 * rows carry absolute FrontEndX/Y scatter coords (their submenu Position is
 * 0:0). */
static void fe_item_pos(const FrontEnd* fe, const FeItem* it, int i,
                        int w, const TextureSurface* measure, int* x, int* y)
{
    (void)i;
    (void)w;
    const FeSubmenu* sub = frontend_active_submenu(fe);
    if (it->level_start) {
        *x = it->x;
        *y = it->y;
        return;
    }
    int bx = (sub != NULL) ? sub->base_x : 0;
    int by = (sub != NULL) ? sub->base_y : 0;
    if (sub != NULL && sub->auto_center) {
        int tw = measure != NULL ? fe_font_text_width(it->label, measure)
                                 : fe_text_width(it->label);
        *x = bx + it->x - tw / 2;
    } else {
        *x = bx + it->x;
    }
    *y = by + it->y;
}

/* Pointer hit-test over the active submenu's items, using the same
 * placement arithmetic (fe_item_pos) and measure-font widths as drawing.
 * Returns 1 and writes the hovered index. */
static int fe_item_pick(const FrontEnd* fe, int mx, int my,
                        const TextureSurface* measure, int* out_idx)
{
    const FeSubmenu* sub = frontend_active_submenu(fe);
    if (sub == NULL || sub->item_count <= 0) {
        return 0;
    }
    int cw = 0, chh = 0;
    if (measure != NULL) {
        fe_font_cell(measure, &cw, &chh);
    }
    int rh = measure != NULL ? chh + 8 : 12;
    for (int i = 0; i < sub->item_count; i++) {
        const FeItem* it = &sub->items[i];
        int x, y;
        fe_item_pos(fe, it, i, 0, measure, &x, &y);
        int tw = measure != NULL ? fe_font_text_width(it->label, measure)
                                 : fe_text_width(it->label);
        if (mx >= x - 4 && mx <= x + tw + 4
            && my >= y - 2 && my <= y + rh) {
            if (out_idx != NULL) {
                *out_idx = i;
            }
            return 1;
        }
    }
    return 0;
}

void frontend_render(const FrontEnd* fe, const TextureSurface* hi_font,
                     const TextureSurface* lo_font,
                     int w, int h, void* px, int pitch)
{
    if (fe == NULL || px == NULL || !fe->open) {
        return;
    }
    const FeSubmenu* sub = frontend_active_submenu(fe);
    if (sub == NULL || sub->item_count <= 0) {
        return;
    }

    /* .flh overlay lane (finding 23): blit each open overlay's current
     * frame at its configured (x,y), native size, over the menu backdrop
     * and under the items. */
    for (int i = 0; i < sub->overlay_count; ++i) {
        const FeOverlay* o = &sub->overlays[i];
        if (o == NULL || o->flic == NULL) {
            continue;
        }
        const uint8_t* frm = flic_frame_rgba(o->flic);
        if (frm == NULL) {
            continue;
        }
        int fw = flic_width(o->flic), fh = flic_height(o->flic);
        for (int yy = 0; yy < fh; ++yy) {
            int dy = o->y + yy;
            if (dy < 0 || dy >= h) {
                continue;
            }
            const uint8_t* s = frm + (size_t)yy * fw * 4;
            uint8_t* d = (uint8_t*)px + (size_t)dy * (size_t)pitch
                         + (size_t)o->x * 4;
            for (int xx = 0; xx < fw; ++xx) {
                int dx = o->x + xx;
                if (dx < 0 || dx >= w) {
                    continue;
                }
                const int oi = xx * 4;
                d[0] = s[oi + 0];
                d[1] = s[oi + 1];
                d[2] = s[oi + 2];
                d[3] = s[oi + 3];
                d += 4;
            }
        }
    }

    if (sub->display_title && sub->title[0] != '\0') {
        if (hi_font != NULL) {
            fe_draw_font_text(px, pitch, w, h, 8, 8, sub->title, hi_font);
        } else {
            fe_draw_text(px, pitch, w, h, 8, 8, sub->title, 230, 210, 120);
        }
    }

    for (int i = 0; i < sub->item_count; i++) {
        const FeItem* it = &sub->items[i];
        int x, y;
        fe_item_pos(fe, it, i, w, hi_font, &x, &y);
        if (x < 0) {
            x = 8;
        }
        if (y < 0) {
            y = 8;
        }

        int selected = (i == fe->sel);
        const TextureSurface* font = selected ? hi_font : lo_font;
        int lw = font != NULL ? fe_font_text_width(it->label, font)
                              : fe_text_width(it->label);
        int chh = 0;
        if (font != NULL) {
            int cw = 0;
            fe_font_cell(font, &cw, &chh);
        }
        int hh = font != NULL ? chh + 8 : 12;

        if (selected) {
            fe_fill_rect(px, pitch, w, h, x - 4, y - 2, lw + 8, hh,
                         60, 80, 200);
        }
        if (font != NULL) {
            fe_draw_font_text(px, pitch, w, h, x, y, it->label, font);
        } else {
            fe_draw_text(px, pitch, w, h, x, y, it->label,
                         selected ? 255 : 200, selected ? 255 : 200,
                         selected ? 255 : 160);
        }
    }
}