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
 *   Slider/Cycle                 -> adjust the option (writes into fe->opts
 *                                  when attached; issue #85)
 *   TextInput                    -> display-only (parsed, not editable)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "frontend.hpp"
#include "gods98/audio.hpp"
#include "gods98/message.hpp"
#include "gods98/flic.hpp"
#include "gods98/crt.hpp"
#include "gods98/heap.hpp"
#include "gods98/record.hpp"
#include "gods98/texture.hpp"
#include "gods98/sound.hpp"
#include "gods98/crt.hpp"
#include "util.hpp"
#include "select.hpp"
#include "gods98/view.hpp"
#include "gods98/string.hpp"
#include "gods98/resource.hpp"
#include "entity.hpp"

static int s_lower(int c) { return (c >= 'A' && c <= 'Z') ? c + 0x20 : c; }
static int s_toupper(int c) { return (c >= 'a' && c <= 'z') ? c - 0x20 : c; }

/* Menu config identifiers use underscores where the original UI shows
 * spaces (for example Start_Game → Start Game). Keep the raw value in
 * FeItem for navigation and option lookup; normalize only at presentation.
 */
static void fe_display_label(const char* label, char* out, size_t out_size)
{
    if (out == NULL || out_size == 0) {
        return;
    }
    size_t i = 0;
    if (label != NULL) {
        for (; label[i] != '\0' && i + 1 < out_size; ++i) {
            out[i] = label[i] == '_' ? ' ' : label[i];
        }
    }
    out[i] = '\0';
}

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

/* Slider initial values per banner — the corpus's option defaults
 * (OptionsLoadOrDefault: speed 1, SFX 8, music 5, brightness 8). */
static int fe_option_default(const char* banner, int lo, int hi);

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
    it->load_slot = 0;
    it->min = 0; it->max = 0; it->value = 0; it->cycle_count = 0;
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
        if (it->kind == FE_ITEM_SLIDER) {
            /* Slider:x1:y1:x2:y2:banner:lo:hi:bar… */
            it->min = (n > 6) ? (int)strtol(fields[6], NULL, 10) : 0;
            it->max = (n > 7) ? (int)strtol(fields[7], NULL, 10) : 100;
            it->flag = it->min;
            it->value = fe_option_default(it->label, it->min, it->max);
        } else if (it->kind == FE_ITEM_REALSLIDER) {
            /* RealSlider:type:x1:y1:x2:y2:banner:lo:hi:step */
            it->min = (n > 6) ? (int)strtol(fields[6], NULL, 10) : 0;
            it->max = (n > 7) ? (int)strtol(fields[7], NULL, 10) : 100;
            it->value = fe_option_default(it->label, it->min, it->max);
        } else if (it->kind == FE_ITEM_CYCLE) {
            /* Cycle:x1:y1:x2:y2:banner:count:opt1:opt2:… */
            int count = (n > 6) ? (int)strtol(fields[6], NULL, 10) : 0;
            if (count > 4) {
                count = 4;
            }
            it->cycle_count = count;
            it->min = 0;
            it->max = count > 0 ? count - 1 : 0;
            it->value = 0;
            for (int k = 0; k < count && (7 + k) < n; k++) {
                snprintf(it->cycle_opts[k], sizeof(it->cycle_opts[0]), "%s",
                         fields[7 + k]);
            }
        }
        break;
    }
}

/* Slider initial values per banner — the corpus's option defaults
 * (OptionsLoadOrDefault: speed 1, SFX 8, music 5, brightness 8). With
 * `fe.opts` attached (issue #85) the *effective* store value wins —
 * fe_options_seed overrides these — and menu edits write back. */
static int fe_option_default(const char* banner, int lo, int hi)
{
    int d = lo;
    if (s_stristr(banner, "sfx")) { d = 8; }
    else if (s_stristr(banner, "music")) { d = 5; }
    else if (s_stristr(banner, "bright")) { d = 8; }
    else if (s_stristr(banner, "speed")) { d = 1; }
    if (d < lo) { d = lo; }
    if (d > hi) { d = hi; }
    return d;
}

/* --- functional options (issue #85) --------------------------------------
 * `fe->opts` (not owned) is the effective-value store: seeding puts the
 * store's value into the widget, adjustments write it back. Labels that map
 * to no option id (level names, plain triggers) are left display-only. */

static int fe_options_widget(int kind)
{
    return kind == FE_ITEM_SLIDER || kind == FE_ITEM_REALSLIDER
        || kind == FE_ITEM_CYCLE;
}

static void fe_options_seed_item(FrontEnd* fe, FeItem* it)
{
    int v = 0;
    if (fe->opts == NULL || !fe_options_widget(it->kind)) {
        return;
    }
    if (!options_get_label(fe->opts, it->label, &v)) {
        return;
    }
    if (v < it->min) {
        v = it->min;
    }
    if (v > it->max) {
        v = it->max;
    }
    it->value = v;
}

void frontend_options_sync(FrontEnd* fe)
{
    if (fe == NULL || fe->opts == NULL) {
        return;
    }
    for (int m = 0; m < FE_MENU_COUNT; m++) {
        for (int s = 0; s < fe->menus[m].submenu_count; s++) {
            FeSubmenu* sub = &fe->menus[m].submenus[s];
            for (int i = 0; i < sub->item_count; i++) {
                fe_options_seed_item(fe, &sub->items[i]);
            }
        }
    }
}

/* Write the widget's new value back into the store (issue #85): every option
 * adjustment lands here, so the frame loop's live consumers (dt scale,
 * brightness, help overlay, audio) read the effective value. */
static void fe_options_write(FrontEnd* fe, FeItem* it)
{
    if (fe->opts != NULL && fe_options_widget(it->kind)) {
        options_set_label(fe->opts, it->label, it->value);
    }
}

/* --- menu group building -------------------------------------------------- */

static void fe_parse_submenu(FrontEnd* fe, FeMenu* m, const char* name,
                             int idx)
{
    FeSubmenu* sub = &m->submenus[idx];
    sub->item_count = 0;
    sub->base_x = sub->base_y = 0;
    sub->title_anchor_y = 0;
    sub->auto_center = 0;
    sub->display_title = 0;
    sub->can_scroll = 0;
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
    /* The original menu renderer centers DisplayTitle on the screen and
     * draws it at Position.y + Anchored.y. Anchored.x is not used for the
     * title's horizontal placement. */
    snprintf(buf, sizeof(buf), "%s Anchored", pre);
    v = block_get_strptr(fe->cfg, buf);
    if (v != NULL) {
        const char* c = strchr(v, ':');
        if (c != NULL) {
            sub->title_anchor_y = (int)strtol(c + 1, NULL, 10);
        }
    }
    snprintf(buf, sizeof(buf), "%s CanScroll", pre);
    sub->can_scroll = block_get_bool(fe->cfg, buf) == 1;
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
        /* The campaign/tutorial picker plane is taller than any window
         * (FrontEndY scatters to ~2100), so the receiving sub scrolls —
         * the corpus gates its rows with the DAT_005574e0 content clamp;
         * flagging the grafted sub is the port-side equivalent. */
        if (sub->item_count > base) {
            sub->can_scroll = 1;
            sub->scroll_extent = 0;
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
                sub->overlays[i].tried = 0;
            }
        }
    }
    fe->overlays_for = NULL;
    fe->overlay_active = -1;
}

/* Keep the existing all-overlay handling for submenus without PlayRandom. */
const FeSubmenu* frontend_active_submenu(const FrontEnd* fe);

static void fe_overlay_open_all(FrontEnd* fe, const FeSubmenu* sub)
{
    if (fe == NULL || sub == NULL) {
        return;
    }
    for (int i = 0; i < sub->overlay_count; ++i) {
        FeOverlay* o = (FeOverlay*)&sub->overlays[i];
        if (o->tried) {
            continue;
        }
        o->tried = 1;
        o->flic = flic_open(o->path);
    }
    fe->overlays_for = sub;
}

/* FUN_0048e170: the game's 15-bit LCG. Seed this front-end instance from
 * wall-clock seconds and its address so menu sequences vary between runs
 * without consuming the gameplay random stream. */
static uint32_t fe_overlay_random(FrontEnd* fe)
{
    fe->overlay_rng = fe->overlay_rng * 0x343fdu + 0x269ec3u;
    return (fe->overlay_rng >> 16) & 0x7fffu;
}

static void fe_overlay_play_sfx(FrontEnd* fe, const FeOverlay* o)
{
    if (fe == NULL || o == NULL || fe->audio == NULL
        || o->sfx[0] == '\0' || s_strcaseeq(o->sfx, "SFX_NULL")) {
        return;
    }

    /* Overlay values name an SFX alias, while Lego.cfg maps that alias under
     * Samples to a WAVE resource (often prefixed by '@' for streamed sounds). */
    char key[FE_NAME_MAX + 32];
    snprintf(key, sizeof(key), "%s Samples !%s", util_root_name(fe->cfg),
             o->sfx);
    const char* resource = block_get_strptr(fe->cfg, key);
    if (resource == NULL || resource[0] == '\0') {
        return;
    }
    if (resource[0] == '@') {
        ++resource;
    }
    audio_play_named(fe->audio, resource);
}

static void fe_overlay_start_random(FrontEnd* fe, const FeSubmenu* sub)
{
    if (sub->overlay_count <= 0 || fe_overlay_random(fe) % 400u != 0) {
        return;
    }

    int picked = (int)(fe_overlay_random(fe) % (uint32_t)sub->overlay_count);
    FeOverlay* o = (FeOverlay*)&sub->overlays[picked];
    o->flic = flic_open(o->path);
    if (o->flic == NULL) {
        return;
    }
    fe->overlay_active = picked;
    fe_overlay_play_sfx(fe, o);
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
        fe->overlays_for = sub;
        if (!sub->play_random) {
            fe_overlay_open_all(fe, sub);
        }
    }
    if (sub->play_random) {
        if (fe->overlay_active >= 0
            && fe->overlay_active < sub->overlay_count) {
            FeOverlay* o = (FeOverlay*)&sub->overlays[fe->overlay_active];
            flic_tick_loop(o->flic, 0);
            if (flic_eof(o->flic)) {
                flic_close(o->flic);
                o->flic = NULL;
                o->tried = 0;
                fe->overlay_active = -1;
            }
        } else {
            fe_overlay_start_random(fe, sub);
        }
        return;
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
    fe->pointer_x = fe->pointer_y = -1;
    fe->overlay_active = -1;
    fe->overlay_rng = (uint32_t)time(NULL) ^ (uint32_t)(uintptr_t)fe;
    if (fe->overlay_rng == 0) {
        fe->overlay_rng = 1;
    }
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
    /* Options widgets seed from the store when one is already attached. */
    frontend_options_sync(fe);
    return main_ok;
}

int fe_record_for_level(const FrontEnd* fe, const char* level)
{
    if (fe == NULL || !fe->built || level == NULL || level[0] == '\0') {
        return -1;
    }
    for (int i = 0; i < fe->tutorial_n; i++) {
        if (s_strcaseeq(level, fe->tutorial[i])) {
            return i;
        }
    }
    for (int i = 0; i < fe->campaign_n; i++) {
        if (s_strcaseeq(level, fe->campaign[i])) {
            return fe->tutorial_n + i;
        }
    }
    return -1;
}

int fe_level_for_record(const FrontEnd* fe, int rec, char* out, size_t sz)
{
    if (fe == NULL || !fe->built || out == NULL || rec < 0) {
        return 0;
    }
    if (rec < fe->tutorial_n) {
        snprintf(out, sz, "%s", fe->tutorial[rec]);
        return 1;
    }
    rec -= fe->tutorial_n;
    if (rec < fe->campaign_n) {
        snprintf(out, sz, "%s", fe->campaign[rec]);
        return 1;
    }
    return 0;
}

int frontend_enabled(BlockFile* cfg)
{    if (cfg == NULL) {
        return 0;
    }
    /* FrontEndActive @ 0x416d00: the cfg flag (loader-mode <3 is Win32). */
    return block_get_bool(cfg, "Lego* Main FrontEnd") == 1;
}

int frontend_build_slots(FrontEnd* fe, const FeSlot* slots, int n)
{
    if (fe == NULL || slots == NULL || n <= 0) {
        return 0;
    }
    if (n > FE_SLOT_COUNT) {
        n = FE_SLOT_COUNT;
    }
    fe->slot_count = n;
    for (int i = 0; i < n; i++) {
        fe->slots[i] = slots[i];
    }
    int grafted = 0;
    /* both the main menu's Load_A_Saved_Game screen and the SaveMenu carry
     * the slot rows (SaveMenuBuild @ 0x415a10's runtime rows; the screens
     * are identified by their SaveLoad art). */
    for (int mi = 0; mi < FE_MENU_COUNT; mi++) {
        FeMenu* m = &fe->menus[mi];
        for (int si = 0; si < m->submenu_count; si++) {
            FeSubmenu* sub = &m->submenus[si];
            if (!s_stristr(sub->image, "SaveLoad")) {
                continue;
            }
            int base = sub->item_count;
            for (int k = 0; k < n && base + k < FE_ITEMS_MAX; k++) {
                FeItem* it = &sub->items[base + k];
                it->kind = FE_ITEM_NEXT;
                it->load_slot = slots[k].id;
                it->level_start = 0;
                it->flag = 0;
                it->target[0] = '\0';
                snprintf(it->label, FE_NAME_MAX, "%s", slots[k].label);
                it->x = 48;
                /* SaveMenuBuild constructs the slot list with a 0x3c-pixel
                 * row interval; MenuTick advances each list row by it. */
                it->y = 96 + k * 60;
                sub->item_count = base + k + 1;
            }
            grafted = 1;
        }
    }
    return grafted;
}

int frontend_open(FrontEnd* fe, int menu)
{
    if (fe == NULL || !fe->built) {
        return 0;
    }
    fe_overlay_close_all(fe);
    fe->menu = menu;
    fe->sub = 0;
    fe->sel = 0;
    fe->open = 1;
    fe->quit = 0;
    fe->pending_level[0] = '\0';
    /* Issue #85: re-entry shows the effective option values, never the
     * build-time defaults — sync every widget from the store. */
    frontend_options_sync(fe);
    fe_overlay_step(fe); /* open the active submenu's flic overlays */
    /* corpus DAT_005574e0 reset @ 0x413d29: the plane returns to the top
     * whenever the menu (re-)enters. */
    for (int i = 0; i < FE_SUBMENUS_MAX; i++) {
        fe->menus[menu].submenus[i].scroll = 0;
    }
    return 1;
}

void frontend_set_audio(FrontEnd* fe, struct AudioPlayer* audio)
{
    if (fe != NULL) {
        fe->audio = audio;
    }
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
static int fe_scroll_measure(const FeSubmenu* sub);
static void fe_item_pos(const FrontEnd* fe, const FeItem* it, int i,
                        int w, const TextureSurface* measure, int* x, int* y);

/* --- activation (Enter or a pointer click resolves to this) -------------- */

static FeEvent fe_activate(FrontEnd* fe)
{
    FeMenu* m = &fe->menus[fe->menu];
    FeSubmenu* sub = &m->submenus[fe->sub];
    FeItem* it = &sub->items[fe->sel];

    /* save-slot row: load the slot (FE_EVT_LOAD_GAME, fe.load_slot set) */
    if (it->load_slot) {
        fe->load_slot = it->load_slot;
        frontend_close(fe);
        return FE_EVT_LOAD_GAME;
    }

    /* picker row: start the selected level */
    if (it->level_start) {
        snprintf(fe->pending_level, FE_LEVEL_NAME_MAX, "%s", it->target);
        frontend_close(fe);
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
                    frontend_close(fe);
                    return FE_EVT_RESUME;
                }
                return FE_EVT_NONE;   /* Replay_Objective: stub */
            case 2:   /* pause Menu3 = quit confirm ("Yes") */
                fe->quit = 1;
                frontend_close(fe);
                return FE_EVT_QUIT;
            case 3:   /* pause Menu4 = restart confirm ("Yes") */
                snprintf(fe->pending_level, FE_LEVEL_NAME_MAX, "%s",
                         fe->resume_level[0] != '\0' ? fe->resume_level
                                                     : it->target);
                frontend_close(fe);
                return FE_EVT_START_LEVEL;
            default:  /* options submenus: Replay_Objective etc. — stub */
                return FE_EVT_NONE;
            }
        }
        fe->quit = 1;
        frontend_close(fe);
        return FE_EVT_QUIT;
    }

    /* MainMenuFull's Credits text row is a trigger with flag 0. In the
     * original menu builder it has a callback that opens the scrolling
     * credits display; the other plain triggers remain no-ops here. */
    if (it->kind == FE_ITEM_TRIGGER && fe->menu == FE_MENU_MAIN
        && s_stristr(it->label, "credits")) {
        return FE_EVT_CREDITS;
    }

    /* Next -> navigate submenus ("Menu%i" target) */
    if (it->kind == FE_ITEM_NEXT && it->target[0] != '\0'
        && s_stristr(it->target, "menu") != 0) {
        int idx = (int)strtol(it->target + 4, NULL, 10) - 1; /* "MenuN" */
        if (idx >= 0 && idx < m->submenu_count) {
            fe->sub = idx;
            fe->sel = 0;
            /* corpus resets DAT_005574e0 (the plane offset) when a menu
             * (re-)enters — the port keeps it per submenu. */
            m->submenus[idx].scroll = 0;
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
    /* .flh overlay lane (finding 23): advance the active clip, or let
     * PlayRandom's per-tick gate start one configured overlay. */
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

    /* Slider / Cycle adjustment: left/right steps the value within
     * [min,max] and flags option_changed for the frame loop to apply. */
    {
        FeItem* dit = &sub->items[fe->sel];
        if (dit->kind == FE_ITEM_SLIDER || dit->kind == FE_ITEM_REALSLIDER
            || dit->kind == FE_ITEM_CYCLE) {
            if (in->right && dit->value < dit->max) {
                dit->value++;
                fe_options_write(fe, dit);
                fe->option_changed = 1;
            }
            if (in->left && dit->value > dit->min) {
                dit->value--;
                fe_options_write(fe, dit);
                fe->option_changed = 1;
            }
        }
    }

    /* CanScroll edge pan (corpus FUN_00413970 @ 0x413970 + DAT_005574e0
     * in FUN_00412c00's item placement): while the cursor is in the outer
     * bands past the deadzone the item plane scrolls, velocity proportional
     * to distance from the band edge, clamped to [view_h - content, 0].
     * The corpus gates y against absolute bands for a 480-high screen
     * (< 200 scrolls, 200..280 deadzone) and velocity ~(50 - y/4)^2; this
     * port keeps the 5/12-h + h/12 band shape and linear distance velocity
     * capped at the corpus's ~50 px/frame. */
    if (sub->can_scroll) {
        if (sub->scroll_extent <= 0) {
            sub->scroll_extent = fe_scroll_measure(sub);
        }
        int vh = (fe->view_h > 0) ? fe->view_h : 480;
        int band = vh / 12;
        int cy = vh / 2;
        int lo = vh - sub->scroll_extent;
        if (lo > 0) {
            lo = 0;
        }
        if (sub->scroll < lo) {
            sub->scroll = lo;
        }
        if (sub->scroll > 0) {
            sub->scroll = 0;
        }
        if (in->mouse_y >= 0) {
            int d = 0, dir = 0;
            if (in->mouse_y < cy - band) {
                d = (cy - band) - in->mouse_y;
                dir = 1;                       /* show earlier rows */
            } else if (in->mouse_y > cy + band) {
                d = in->mouse_y - (cy + band);
                dir = -1;                      /* show later rows */
            }
            if (d > 0) {
                sub->scroll += dir * (d > 50 ? 50 : d);
                if (sub->scroll > 0) {
                    sub->scroll = 0;
                }
                if (sub->scroll < lo) {
                    sub->scroll = lo;
                }
            }
        }
    }

    /* pointer: hover selects; a click activates the hovered row (Enter).
     * When the pointer is in use but outside every item, a click is a
     * no-op (the menu stays up). */
    if (in->mouse_x >= 0 && in->mouse_y >= 0) {
        fe->pointer_x = in->mouse_x;
        fe->pointer_y = in->mouse_y;
        int idx = 0;
        int hit = fe_item_pick(fe, in->mouse_x, in->mouse_y, measure, &idx);
        if (hit) {
            fe->sel = idx;
        }

        /* Slider pointer drag (corpus FUN_00411a00 case 3 routes a held
         * button through FUN_00411870 every frame): with the pointer held
         * inside the bar's 100x20 rect anchored at the selected item's
         * origin, the value continuously tracks pointer x —
         *   v = round(clamp((mx - x) * 0.01, 0.0, 1.0) * (max - min))
         * (constants 0x49e170/174/178 = 0.0 / 1.0 / 0.01, rect w/h =
         * 0x64/0x14). The corpus mouse path omits the +min offset that the
         * keyboard path keeps; every shipped slider is min 0, and the port
         * clamps to [min,max] so a hypothetical min>0 row cannot sink below
         * its floor. */
        if (in->mdown && fe->sel < sub->item_count) {
            FeItem* dit = &sub->items[fe->sel];
            if (dit->kind == FE_ITEM_SLIDER || dit->kind == FE_ITEM_REALSLIDER) {
                int bx = 0, by = 0;
                fe_item_pos(fe, dit, fe->sel, 0, measure, &bx, &by);
                if (in->mouse_x >= bx && in->mouse_x <= bx + 100
                    && in->mouse_y >= by && in->mouse_y <= by + 20) {
                    float f = (float)(in->mouse_x - bx) * 0.01f;
                    if (f < 0.0f) {
                        f = 0.0f;
                    }
                    if (f > 1.0f) {
                        f = 1.0f;
                    }
                    int v = (int)(f * (float)(dit->max - dit->min) + 0.5f);
                    if (v < dit->min) {
                        v = dit->min;
                    }
                    if (v > dit->max) {
                        v = dit->max;
                    }
                    if (v != dit->value) {
                        dit->value = v;
                        fe_options_write(fe, dit);
                        fe->option_changed = 1;
                    }
                }
            }
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
            frontend_close(fe);       /* pause Esc = resume */
            return FE_EVT_RESUME;
        } else {
            fe->quit = 1;
            frontend_close(fe);
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
    static const FeItem s_empty = { 0, 0, 0, {0}, {0}, 0, 0, 0,
                                        0, 0, 0, 0, {{0},{0},{0},{0}} };
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

/* The in-game Help Window panel (issue #85). Corpus model: HelpWindow's
 * BIG-screen backdrop with info text; the Help_Window option is the
 * display gate — it lives here, so the frame loop and every caller share
 * one gate and the option can never draw without the store saying On.
 * The port's face is a bottom-left panel: title line plus `text` (the demo
 * key lines when NULL). */
void frontend_help_overlay(void* px, int pitch, int w, int h,
                           const char* text, const GameOptions* opts)
{
    if (opts != NULL && opts->help_window == 0) {
        return; /* Help_Window Off: the option gates the display. */
    }
    if (px == NULL || w <= 0 || h <= 0) {
        return;
    }
    int x0 = 16, y0 = (h > 96) ? h - 92 : 4;
    int pw = (w > 360) ? 360 : (w > 40 ? w - 32 : w);
    int ph = 80;
    if (y0 + ph > h) {
        ph = h - y0 - 4;
    }
    if (ph <= 0) {
        return;
    }
    fe_fill_rect(px, pitch, w, h, x0, y0, pw, ph, 10, 14, 34);
    /* 1-px frame */
    fe_fill_rect(px, pitch, w, h, x0, y0, pw, 1, 150, 160, 200);
    fe_fill_rect(px, pitch, w, h, x0, y0 + ph - 1, pw, 1, 150, 160, 200);
    fe_fill_rect(px, pitch, w, h, x0, y0, 1, ph, 150, 160, 200);
    fe_fill_rect(px, pitch, w, h, x0 + pw - 1, y0, 1, ph, 150, 160, 200);
    fe_draw_text(px, pitch, w, h, x0 + 8, y0 + 6, "HELP", 235, 225, 150);
    fe_draw_text(px, pitch, w, h, x0 + 8, y0 + 26,
                 (text != NULL && text[0] != '\0')
                     ? text : "ESC PAUSE  ENTER SELECT  SPACE SFX",
                 200, 205, 220);
    fe_draw_text(px, pitch, w, h, x0 + 8, y0 + 46,
                 "C ORE   CRYSTALS   B SAVE   Y REWARD",
                 170, 175, 195);
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

/* Fallback key derivation (port-side stand-in, documented deviation): when
 * a surface carries no palette key info — synthetic fonts, 24-bit sources
 * — the key is sampled as the dominant colour of the grid (glyph 0's cell
 * is background in every observed font). FE_FONT_KEY stays as the generic
 * default for a fully blank sample. */
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

/* Face keys — the corpus path (FUN_0047d9d0 + FUN_0047e050): every 8bpp
 * font surface carries palette[0] and palette[255] as colour keys, both
 * blitted transparent. Surfaces without a decoded palette (key_count 0)
 * keep the sampled stand-in for both slots. */
static void fe_font_keys(const TextureSurface* f, int cw, int ch,
                         uint32_t keys[2])
{
    if (f->key_count == 2) {
        keys[0] = f->keys[0];
        keys[1] = f->keys[1];
        return;
    }
    keys[0] = keys[1] = fe_font_key(f, cw, ch);
}

static int fe_font_key_hit(uint32_t p, const uint32_t keys[2])
{
    return p == keys[0] || p == keys[1];
}

/* Ink bounding box of glyph g — the non-key pixels only. The engine blits
 * exactly this box (FUN_00479ee0's right-edge scan / per-glyph rect), which
 * keeps each face's background AND frame strips out while retaining the
 * glyph's dark outline when it is inside the box. */
static int fe_font_ink_box(const TextureSurface* f, int cw, int ch, int g,
                           const uint32_t keys[2], int* minx, int* miny,
                           int* maxx, int* maxy)
{
    int col = g % FE_FONT_GRID_COLS, row = g / FE_FONT_GRID_COLS;
    int x0 = col * cw, y0 = row * ch;
    *minx = cw; *miny = ch; *maxx = -1; *maxy = -1;
    for (int yy = 0; yy < ch; yy++) {
        for (int xx = 0; xx < cw; xx++) {
            if (fe_font_key_hit(
                    f->pixels[(size_t)(y0 + yy) * (size_t)f->width + (x0 + xx)],
                    keys)) {
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
                           const uint32_t keys[2])
{
    int mx, my, ax, ay;
    if (!fe_font_ink_box(f, cw, ch, g, keys, &mx, &my, &ax, &ay)) {
        return 0;
    }
    return ax + 1;
}

static void fe_blit_glyph(void* px, int pitch, int w, int h,
                          int dx, int dy, const TextureSurface* f,
                          int cw, int ch, int g, const uint32_t keys[2])
{
    int col = g % FE_FONT_GRID_COLS, row = g / FE_FONT_GRID_COLS;
    int x0 = col * cw, y0 = row * ch;
    int minx, miny, maxx, maxy;
    if (!fe_font_ink_box(f, cw, ch, g, keys, &minx, &miny, &maxx, &maxy)) {
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
            if (fe_font_key_hit(p, keys)) {
                continue;               /* key *inside* the box */
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
    uint32_t keys[2];
    fe_font_keys(f, cw, chh, keys);
    int cx = x;
    for (int i = 0; s[i] != '\0' && cx < w; ++i) {
        int c = (unsigned char)s[i];
        int g = c - 0x20;
        if (g < 0 || g >= FE_FONT_GRID_COLS * FE_FONT_GRID_ROWS) {
            cx += cw / 2;
            continue;
        }
        int adv = fe_font_advance(f, cw, chh, g, keys);
        if (adv <= 0) {
            adv = (c == ' ') ? cw / 2 : cw;        /* blank cell fallback */
        }
        fe_blit_glyph(px, pitch, w, h, cx, y, f, cw, chh, g, keys);
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
    uint32_t keys[2];
    fe_font_keys(f, cw, chh, keys);
    int cx = 0;
    for (int i = 0; s[i] != '\0'; ++i) {
        int c = (unsigned char)s[i];
        int g = c - 0x20;
        if (g < 0 || g >= FE_FONT_GRID_COLS * FE_FONT_GRID_ROWS) {
            cx += cw / 2;
            continue;
        }
        int adv = fe_font_advance(f, cw, chh, g, keys);
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
    /* Runtime save rows use absolute coordinates just like level picker
     * rows. Their SaveLoad panel is already positioned by the submenu, so
     * adding its centered menu anchor would push the list off the panel. */
    if (it->level_start || it->load_slot) {
        *x = it->x;
        *y = it->y;
    } else {
        int bx = (sub != NULL) ? sub->base_x : 0;
        int by = (sub != NULL) ? sub->base_y : 0;
        if (sub != NULL && sub->auto_center) {
            char label[FE_NAME_MAX];
            fe_display_label(it->label, label, sizeof(label));
            int tw = measure != NULL ? fe_font_text_width(label, measure)
                                     : fe_text_width(label);
            *x = bx + it->x - tw / 2;
        } else {
            *x = bx + it->x;
        }
        *y = by + it->y;
    }
    /* CanScroll plane pan (FUN_00412c00 adds DAT_005574e0 to the item y
     * in BOTH draw and hit-test paths). */
    if (sub != NULL && sub->can_scroll) {
        *y += sub->scroll;
    }
}

/* CanScroll content extent (the corpus's menu+8 "plane height"): the
 * deepest item row of the submenu plus one row height. */
static int fe_scroll_measure(const FeSubmenu* sub)
{
    int maxy = 0;
    for (int i = 0; i < sub->item_count; i++) {
        int top = sub->items[i].level_start
                      ? sub->items[i].y
                      : sub->base_y + sub->items[i].y;
        if (top > maxy) {
            maxy = top;
        }
    }
    return maxy + 16;               /* last row's height (port chrome) */
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
        char label[FE_NAME_MAX];
        fe_display_label(it->label, label, sizeof(label));
        int tw = measure != NULL ? fe_font_text_width(label, measure)
                                 : fe_text_width(label);
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

void frontend_render(FrontEnd* fe, const TextureSurface* hi_font,
                     const TextureSurface* lo_font,
                     int w, int h, void* px, int pitch)
{
    if (fe == NULL || px == NULL || !fe->open) {
        return;
    }
    /* viewport bookkeeping for the CanScroll bands/clamp (the corpus keeps
     * its own screen-absolute band constants; we track the window here). */
    if (fe->view_w != w || fe->view_h != h) {
        fe->view_w = w;
        fe->view_h = h;
        FeSubmenu* act = &fe->menus[fe->menu].submenus[fe->sub];
        if (act->can_scroll) {
            act->scroll_extent = 0;            /* re-measure next tick */
        }
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
        const uint16_t* frm = flic_frame555(o->flic);
        if (frm == NULL) {
            continue;
        }
        int fw = flic_width(o->flic), fh = flic_height(o->flic);
        for (int yy = 0; yy < fh; ++yy) {
            int dy = o->y + yy;
            if (dy < 0 || dy >= h) {
                continue;
            }
            int sx = o->x < 0 ? -o->x : 0;
            int ex = fw;
            if (o->x + ex > w) {
                ex = w - o->x;
            }
            if (sx >= ex) {
                continue;
            }
            uint8_t* d = (uint8_t*)px + (size_t)dy * (size_t)pitch
                         + (size_t)(o->x + sx) * 4;
            for (int xx = sx; xx < ex; ++xx) {
                uint16_t c = frm[(size_t)yy * fw + xx];
                /* FLIC-backed sprites use RGB555 zero as the transparent
                 * key. Treating every pixel as opaque paints the decoder's
                 * empty canvas over the menu backdrop. */
                if (c != 0) {
                    uint32_t rgba = flic_rgb555_to_rgba(c);
                    d[0] = (uint8_t)rgba;
                    d[1] = (uint8_t)(rgba >> 8);
                    d[2] = (uint8_t)(rgba >> 16);
                    d[3] = 0xff;
                }
                d += 4;
            }
        }
    }

    if (sub->display_title && sub->title[0] != '\0') {
        char title[FE_NAME_MAX];
        fe_display_label(sub->title, title, sizeof(title));
        int title_w = hi_font != NULL ? fe_font_text_width(title, hi_font)
                                      : fe_text_width(title);
        int title_x = (w - title_w) / 2;
        int title_y = sub->base_y + sub->title_anchor_y;
        if (hi_font != NULL) {
            fe_draw_font_text(px, pitch, w, h, title_x, title_y, title,
                              hi_font);
        } else {
            fe_draw_text(px, pitch, w, h, title_x, title_y, title,
                         230, 210, 120);
        }
    }

    for (int i = 0; i < sub->item_count; i++) {
        const FeItem* it = &sub->items[i];
        int x, y;
        fe_item_pos(fe, it, i, w, hi_font, &x, &y);
        if (x < 0) {
            x = 8;
        }
        if (y < 0 && !(sub->can_scroll)) {
            y = 8;
        }
        if (sub->can_scroll && (y + 16 < 0 || y >= h)) {
            continue;   /* scrolled row: outside the plane window */
        }

        int selected = (i == fe->sel);
        const TextureSurface* font = selected ? hi_font : lo_font;
        char label[FE_NAME_MAX];
        fe_display_label(it->label, label, sizeof(label));
        int lw = font != NULL ? fe_font_text_width(label, font)
                              : fe_text_width(label);
        if (font != NULL) {
            fe_draw_font_text(px, pitch, w, h, x, y, label, font);
        } else {
            fe_draw_text(px, pitch, w, h, x, y, label,
                         selected ? 255 : 200, selected ? 255 : 200,
                         selected ? 255 : 160);
        }
        /* Slider / Cycle widget: a bar + knob for sliders, the current
         * option text for cycles (the port's own chrome, not the shipped
         * Vol_* bar art). */
        if (it->kind == FE_ITEM_SLIDER || it->kind == FE_ITEM_REALSLIDER) {
            int chh2 = 12;
            if (font != NULL) {
                int cw2 = 0;
                fe_font_cell(font, &cw2, &chh2);
            }
            int bx = x + lw + 10, by = y + chh2 / 2 - 2;
            int bw = 72;
            int frac = bw;
            if (it->max > it->min) {
                frac = (it->value - it->min) * bw / (it->max - it->min);
            }
            fe_fill_rect(px, pitch, w, h, bx, by, bw, 6, 36, 36, 40);
            fe_fill_rect(px, pitch, w, h, bx, by, frac, 6, 210, 180, 70);
            fe_fill_rect(px, pitch, w, h, bx + frac, by - 1, 3, 8, 255, 255, 255);
        } else if (it->kind == FE_ITEM_CYCLE && it->cycle_count > 0) {
            int oi = it->value < it->cycle_count ? it->value : 0;
            const char* opt = it->cycle_opts[oi];
            if (font != NULL) {
                fe_draw_font_text(px, pitch, w, h, x + lw + 10, y, opt, font);
            } else {
                fe_draw_text(px, pitch, w, h, x + lw + 10, y, opt,
                             selected ? 255 : 200, selected ? 255 : 200,
                             selected ? 255 : 160);
            }
        }
    }

    /* Pointer_Standard is the shipped Aclosed.bmp cursor from Lego.cfg's
     * Pointers block. Its red tip is the top-left hotspot. */
    if (fe->pointer_x >= 0 && fe->pointer_y >= 0
        && fe->pointer_x < w && fe->pointer_y < h) {
        char key[128];
        snprintf(key, sizeof(key), "%s Pointers Pointer_Standard",
                 util_root_name(fe->cfg));
        const char* pointer = fe->cfg != NULL
                                  ? block_get_strptr(fe->cfg, key) : NULL;
        if (pointer == NULL || pointer[0] == '\0') {
            pointer = "Interface\\Pointers\\Aclosed.bmp";
        }
        TextureSurface* cursor = texture_get_or_load(NULL, pointer);
        if (cursor != NULL) {
            int ox = fe->pointer_x, oy = fe->pointer_y;
            for (int yy = 0; yy < cursor->height; ++yy) {
                int dy = oy + yy;
                if (dy < 0 || dy >= h) {
                    continue;
                }
                uint8_t* d = (uint8_t*)px + (size_t)dy * pitch;
                for (int xx = 0; xx < cursor->width; ++xx) {
                    int dx = ox + xx;
                    if (dx < 0 || dx >= w) {
                        continue;
                    }
                    uint32_t c = cursor->pixels[(size_t)yy * cursor->width + xx];
                    int transparent = 0;
                    for (int k = 0; k < cursor->key_count; ++k) {
                        if (c == cursor->keys[k]) {
                            transparent = 1;
                            break;
                        }
                    }
                    if (!transparent) {
                        uint8_t* p = d + (size_t)dx * 4;
                        p[0] = (uint8_t)(c >> 16);
                        p[1] = (uint8_t)(c >> 8);
                        p[2] = (uint8_t)c;
                        p[3] = 0xff;
                    }
                }
            }
            texture_release(cursor);
        }
    }
}

void frontend_render_credits(const TextureSurface* font,
                             const char* const* lines, int line_count,
                             int scroll_px, int w, int h, void* px, int pitch)
{
    if (px == NULL || lines == NULL || line_count <= 0 || w <= 0 || h <= 0) {
        return;
    }
    int cw = 0, ch = 0;
    if (font != NULL && fe_font_cell(font, &cw, &ch)) {
        /* use the shipped RSFont bitmap grid */
    } else {
        font = NULL;
        cw = 4;
        ch = 8;
    }
    const int line_h = ch;
    const int visible_rows = h / line_h + 1;
    const int period = (line_count + visible_rows) * line_h;
    int phase = scroll_px % period;
    if (phase < 0) {
        phase += period;
    }
    const int first_row = -visible_rows + phase / line_h;
    const int partial = phase % line_h;
    for (int row = first_row < 0 ? 0 : first_row;
         row < line_count && row < first_row + visible_rows + 1; ++row) {
        const char* text = lines[row] != NULL ? lines[row] : "";
        int tw = font != NULL ? fe_font_text_width(text, font)
                              : fe_text_width(text);
        int x = (w - tw) / 2;
        int y = (row - first_row) * line_h - partial;
        if (font != NULL) {
            fe_draw_font_text(px, pitch, w, h, x, y, text, font);
        } else {
            fe_draw_text(px, pitch, w, h, x, y, text, 230, 230, 220);
        }
    }
}

/* --- current selection context (see frontend.hpp) -------------------------- */
static void* s_current_selection_context;

void* current_selection_context_get(void)
{
    return s_current_selection_context;
}

void current_selection_context_set(void* context)
{
    s_current_selection_context = context;
}

/* --- selection-state band (see frontend.hpp) -------------------------------
 * Zero-initialized module statics: the corpus's table writers (the
 * UI-mode resource positions apply (UiModeResourcePositionsApply) and the
 * button hit test (HelpWindowButtonHitTest) are unported, so state, hit
 * index, and handles land at
 * zero. The corpus's 0x1c-stride handle tables index blindly; the port
 * guards a 16-entry window per table (index * 7 dwords). */
#define SEL_HANDLE_ENTRIES 16
#define SEL_HANDLE_STRIDE 7 /* dwords per 0x1c-stride entry */

static uint32_t s_sel_state;                    /* DAT_004dc650 */
static int      s_sel_hit_index;                /* DAT_004dc648 */
static uint32_t s_sel_handles_a[SEL_HANDLE_ENTRIES * SEL_HANDLE_STRIDE];
static uint32_t s_sel_handles_b[SEL_HANDLE_ENTRIES * SEL_HANDLE_STRIDE];
static uint32_t s_sel_handle_extra;             /* DAT_004db934 */

void selection_state_flag_clear_bit0(void)
{
    s_sel_state = s_sel_state & ~1u;
}

uint32_t selection_state_handle_get(int index)
{
    if (index == 1 && (s_sel_state & 4u) == 0) {
        return s_sel_handle_extra;
    }
    if ((s_sel_state & 0x10u) != 0 && s_sel_hit_index == index) {
        if (index < 0 || index >= SEL_HANDLE_ENTRIES) {
            return 0u; /* deviation: the corpus indexes blindly */
        }
        const size_t off = (size_t)index * SEL_HANDLE_STRIDE;
        if ((s_sel_state & 0x20u) != 0) {
            return s_sel_handles_b[off];
        }
        return s_sel_handles_a[off];
    }
    return 0u;
}

uint32_t selection_state_flags_peek(void) { return s_sel_state; }
void selection_state_flags_set(uint32_t flags) { s_sel_state = flags; }
int selection_state_hit_index_peek(void) { return s_sel_hit_index; }
void selection_state_hit_index_set(int index) { s_sel_hit_index = index; }
uint32_t selection_state_handle_extra_peek(void) { return s_sel_handle_extra; }
void selection_state_handle_extra_set(uint32_t value)
{
    s_sel_handle_extra = value;
}

uint32_t selection_state_handle_slot_peek(int table, int index)
{
    if (index < 0 || index >= SEL_HANDLE_ENTRIES) {
        return 0u;
    }
    const size_t off = (size_t)index * SEL_HANDLE_STRIDE;
    return (table != 0) ? s_sel_handles_b[off] : s_sel_handles_a[off];
}

void selection_state_handle_slot_set(int table, int index, uint32_t value)
{
    if (index < 0 || index >= SEL_HANDLE_ENTRIES) {
        return;
    }
    const size_t off = (size_t)index * SEL_HANDLE_STRIDE;
    if (table != 0) {
        s_sel_handles_b[off] = value;
    } else {
        s_sel_handles_a[off] = value;
    }
}

/* --- band-mode / timed-effect / positions (see frontend.hpp) -----------------
 * The band mode word joins the selection-state statics (the corpus's
 * DAT_004dc64c, read by the transition leaf and raised in passing by the
 * state-bit gates). The timed-effect flag words land as a guarded window:
 * the corpus's 0x27c-stride band entries live with the unported
 * update/timestamp family, so only the per-entry flag word is modeled. */
#define TIMED_EFFECT_ENTRIES 16
#define LAYOUT_POSITION_ENTRIES 2   /* corpus: two help-window buttons */

static int      s_band_mode;                    /* DAT_004dc64c */
static uint32_t s_timed_effect_flags[TIMED_EFFECT_ENTRIES];
static int32_t  s_layout_position[LAYOUT_POSITION_ENTRIES][2];

void timed_effect_update_request_set(int index)
{
    if (index >= 0 && index < TIMED_EFFECT_ENTRIES) {
        s_timed_effect_flags[index] |= 4u;
    }
    /* deviation: the corpus raises blindly at any index. */
}

uint32_t timed_effect_flags_peek(int index)
{
    if (index < 0 || index >= TIMED_EFFECT_ENTRIES) {
        return 0u;
    }
    return s_timed_effect_flags[index];
}

bool ui_mode_transition_request(int index, int p2, int p3)
{
    const int mode = s_band_mode;
    if (p2 == 0) {
        s_band_mode = 4;
    } else if (s_band_mode == 4) {
        if (p3 == 0) {
            s_band_mode = index;
            s_sel_state |= 0x20u;
            return index == 4;
        }
    } else if (s_band_mode == index) {
        s_sel_state |= 0x20u;
        return true;
    }
    return mode == index;
}

int band_mode_peek(void) { return s_band_mode; }
void band_mode_set(int mode) { s_band_mode = mode; }

void selection_mode_set_by_index(int index)
{
    switch (index) {
    case 0:
        timed_effect_update_request_set(8);
        break;
    case 1:
        timed_effect_update_request_set(9);
        break;
    default:
        break;
    }
}

void ui_mode_resource_positions_apply(void)
{
    for (int i = 0; i < LAYOUT_POSITION_ENTRIES; ++i) {
        const uint32_t handle = selection_state_handle_get(i);
        if (handle != 0u) {
            /* Corpus: the reposition leaf with (handle, 0, &position, 0) —
             * the surface reposition hook, cited as a seam in simtick;
             * the position table itself lands zero (the help-window layout
             * loader is unported). */
        }
    }
}

void layout_position_set(int entry, int x, int y)
{
    if (entry >= 0 && entry < LAYOUT_POSITION_ENTRIES) {
        s_layout_position[entry][0] = x;
        s_layout_position[entry][1] = y;
    }
}

int layout_position_get(int entry, int* x, int* y)
{
    if (entry < 0 || entry >= LAYOUT_POSITION_ENTRIES) {
        return 0;
    }
    if (x != NULL) {
        *x = s_layout_position[entry][0];
    }
    if (y != NULL) {
        *y = s_layout_position[entry][1];
    }
    return 1;
}

/* --- the front-end flow words (see frontend.hpp) ----------------------------
 * Zero-initialized module statics stand in for the corpus's DAT words; the
 * load image's zero state is the faithful initial state (finding 23/438). */
static int s_fe_flow[5]; /* DAT_0055701c / 20 / 24 / 48 / 4c, in order */

int save_slot_open(void)
{
    return s_fe_flow[1] != -1;
}

int is_in_game(void)
{
    return s_fe_flow[2] != -1;
}

int quit_menu_probe(void)
{
    return s_fe_flow[0];
}

int level_start_flag(void)
{
    return s_fe_flow[3] != 0;
}

int menu_options_flag(void)
{
    return s_fe_flow[4] != 0;
}

void frontend_flow_word_set(int which, int value)
{
    if (which >= 0 && which < 5) {
        s_fe_flow[which] = value;
    }
}
