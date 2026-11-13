// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-017 FrontEnd.c: the unit's functions in original address order and the
 * state they own, relocated verbatim by tools/relocate_unit.py (ADR 0006).
 * Body hashes: docs/coverage/relocations/GAME-017.csv. */
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <time.h>
#include "frontend.hpp"
#include "frontend.h"
#include "objective.hpp"
#include "level.hpp"
#include "entity.hpp"
#include "terrain_feedback.hpp"
#include "port_loop_ports.hpp"
#include "effects.hpp"
#include "savegame.hpp"
#include "gods98/audio.hpp"
#include "gods98/engine.hpp"
#include "gods98/simtick.hpp"
#include "gods98/message.hpp"
#include "gods98/flic.hpp"
#include "gods98/media.hpp"
#include "crt/crt.hpp"
#include "gods98/heap.hpp"
#include "gods98/record.hpp"
#include "gods98/texture.hpp"
#include "gods98/surface.hpp"
#include "gods98/sound.hpp"
#include "gods98/block.hpp"
#include "gods98/input.hpp"
#include "util.hpp"
#include "select.hpp"
#include "cmd.hpp"
#include "priority_config_ports.hpp"
#include "tutorial_hud_ports.hpp"
#include "ui_action_dispatch.hpp"
#include "simulation_ui_ports.hpp"
#include "camera_drag.hpp"
#include "screen_pick.hpp"
#include "pointer_cursor.hpp"
#include "ui_list.hpp"
#include "gods98/view.hpp"
#include "gods98/string.hpp"
#include "frontend_for_interface.hpp"
#include "gods98/resource.hpp"
#include "frontend_for_lego.hpp"
#include <float.h>
#include <errno.h>
#include "crt/crt_for_panel_unit.hpp"
#include <stdint.h>
#include <limits.h>
#include "game.hpp"
#include "gods98/camera.hpp"
#include "terrain_grid.hpp"
#include "type_config_ports.hpp"
#include "ai.hpp"
#include "upgrades.hpp"
#include "spawn.hpp"
#include "scene_render.hpp"
#include "level_for_files.hpp"
#include "mission.hpp"
#include "../gods98/frameloop.hpp"
#include "../platform/platform.h"
#include <stddef.h>
#include "options.hpp"
#include "savegame_shared.hpp"
#include "savegame_for_reward.hpp"
#include <locale.h>
#include "audio_for_sound3d.hpp"
#include "game/building_scene.hpp"
#include "game/actor_scene.hpp"
#include "game/render.hpp"
#include "game/tutorial_world_arrows.hpp"
#include "movie.hpp"
#include "tickers.hpp"
#if defined(LEGORRR_BACKEND_WINAPI)
#include <windows.h>
#endif
#if defined(LEGORRR_BACKEND_MINIWIN)

#include <dlfcn.h>
#include <dirent.h>
#include <sys/stat.h>

#endif
#include "record_shared.hpp"
#if defined(_WIN32)
#include <windows.h>
#else
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
#include "scene_pool.hpp"
#include "scene_resources.hpp"
#include "model.hpp"
extern "C" void* DAT_0055613c;
extern "C" void* DAT_00556f50;
extern "C" void* DAT_00556f54;
#include "frontend_for_frontend_unit.hpp"
#include "crt/crt_for_frontend_unit.hpp"
#include "options_for_frontend_unit.hpp"
#include "savegame_for_frontend_unit.hpp"
#include "upgrades_for_frontend_unit.hpp"
#include "util_for_frontend_unit.hpp"
#include "gods98/frameloop_for_frontend_unit.hpp"

typedef struct FeSavMenuListBuild {
    FrontEnd* fe;
    FeLevelMenuList* list;
    const SavListWindow* registry;
    const SavTreeNode* seen[FE_LEVEL_MENU_ROWS * 4];
    int seen_count;
} FeSavMenuListBuild;

/* RegistryValueLookupWithLock (FUN_00417280) — see savegame.hpp. The
 * user block is the corpus's four stack dwords: found node, the probe
 * index, the visit counter, and the hit-index write. */
typedef struct SavTreeNthState {
    void*    found;
    uint32_t want;
    uint32_t seen;
    uint32_t found_at;
} SavTreeNthState;

#define STRING_QUEUE_CAPACITY 15
#define STRING_QUEUE_TEXT_CAPACITY 1024
struct StringQueueEntry {
    uint32_t first;
    uint32_t second;
    uint32_t third;
    char text[STRING_QUEUE_TEXT_CAPACITY];
};

/* Resource rows keep the corpus's 0x20-byte shape on x86. On a 64-bit
 * host, widen pointer fields and the row stride so none of the handles are
 * truncated. Menu resource producers must use this same layout. */
#if UINTPTR_MAX == UINT32_MAX
typedef struct MenuResourceEntry {
    uint32_t flags;
    void* owned_string;
    TextureSurface* textures[3];
    uint32_t reserved[3];
} MenuResourceEntry;
#else
typedef struct MenuResourceEntry {
    uint32_t flags;
    uint32_t reserved0;
    void* owned_string;
    TextureSurface* textures[3];
} MenuResourceEntry;
#endif

/* The corpus menu-volume gain is a standalone .rdata dword (verified
 * 0x49e1e0 = 0.11111111f). */
// GLOBAL: LEGORR 0x0049e1e0
static const float s_menu_volume_scale = 0.11111111f; /* DAT_0049e1e0 */

/* DAT_004a1f60 is a file-backed 256-entry scan-code-to-character table.
 * These bytes were read from reference/LegoRR.exe at file offset 0xa0d60;
 * the table has no runtime writer in the observed input paths. */
// GLOBAL: LEGORR 0x004A1F60
static const uint32_t s_key_context_table[INPUT_KEY_COUNT] = {
    0x00u, 0x00u, 0x31u, 0x32u, 0x33u, 0x34u, 0x35u, 0x36u,
    0x37u, 0x38u, 0x39u, 0x30u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x71u, 0x77u, 0x65u, 0x72u, 0x74u, 0x79u, 0x75u, 0x69u,
    0x6fu, 0x70u, 0x00u, 0x00u, 0x00u, 0x00u, 0x61u, 0x73u,
    0x64u, 0x66u, 0x67u, 0x68u, 0x6au, 0x6bu, 0x6cu, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x7au, 0x78u, 0x63u, 0x76u,
    0x62u, 0x6eu, 0x6du, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x20u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x37u,
    0x38u, 0x39u, 0x00u, 0x34u, 0x35u, 0x36u, 0x00u, 0x31u,
    0x32u, 0x33u, 0x30u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u
};

// GLOBAL: LEGORR 0x004a2360
static const char s_filename_bitmap_suffix[5] = ".bmp";

/* Item-kind matcher. The corpus compares all six names through the CRT's
 * case-folded compare (six `call 0x48ff10` sites, literals pushed second:
 * `push 0x4a23dc /*"cycle" / push esi / call 0x48ff10 / add esp,8 / test
 * eax,eax / jne`), answers 0..4 from mov-immediates, and folds the tail
 * match arithmetically (`neg eax / sbb eax,eax / and al,0xfa / add eax,5`
 * — MSVC's `? 5 : -1` idiom). Unknown tokens answer -1, which the caller's
 * switch routes to the generic banner arm; the earlier local spelling
 * (s_strcaseeq, no "next" case, unknown folded to Next) predates the
 * corpus read. */
/* The corpus compares against its .data token strings at 0x4a23ac..0x4a23e3
 * (eight/12-byte strides, image bytes verified against the reference PE:
 * next, realslider, slider, textinput, trigger, cycle). Annotating them as
 * DATs lets each `push` land on the corpus's immediate address — the six
 * `push imm` vs `push STRING` pairs were this function's entire residual. */
// GLOBAL: LEGORR 0x004a23ac
static const char s_item_token_next[5] = "next";
// GLOBAL: LEGORR 0x004a23b4
static const char s_item_token_realslider[11] = "realslider";
// GLOBAL: LEGORR 0x004a23c0
static const char s_item_token_slider[7] = "slider";
// GLOBAL: LEGORR 0x004a23c8
static const char s_item_token_textinput[10] = "textinput";
// GLOBAL: LEGORR 0x004a23d4
static const char s_item_token_trigger[8] = "trigger";
// GLOBAL: LEGORR 0x004a23dc
static const char s_item_token_cycle[6] = "cycle";

// GLOBAL: LEGORR 0x004a23e4
static const char s_filename_lightwave_suffix[5] = ".lws";
/* FilenameExtensionClassify compares these exact .data strings in order;
 * their mapped addresses and bytes are verified in the data annotation
 * inventory. */
// GLOBAL: LEGORR 0x004a23ec
static const char s_filename_avi_suffix[5] = ".avi";

/* The corpus compares names against the .data "Null" literal (objdump-
 * verified bytes 4E 75 6C 6C 00 at 0x4a23f4); a named const array keeps
 * the push operand resolvable by Reccmp. */
// GLOBAL: LEGORR 0x004a23f4
static const char s_menu_null_label[5] = "Null";

/* MapOpen2 (FUN_00480170): preserve the retail open, seek/tell, progress,
 * allocate, rewind/read, and close order against the level-owned map slots. */
/* 004a2770 is the pooled "rb" literal, marked in gods98/files.c (finding
 * 583); this copy serves the port's map_open2 (files_port.cpp). */
const char s_map_open_binary_mode[] = "rb";

// GLOBAL: LEGORR 0x004db748
static char s_ui_text_underscore_buffer[256];

/* --- the string registry (see crt.hpp) ------------------------------------- */
// GLOBAL: LEGORR 0x004db8cc
void* s_string_registry_head; /* DAT_004db8cc role: unowned */

/* The previous-renderer-word memory is corpus-owned (DAT_004db8d4, the
 * zero-filled tail); the current word is crt's g_renderer_word_c — the
 * corpus loads both directly, so the port does too (the getter call was
 * the only operand mismatch). */
// GLOBAL: LEGORR 0x004db8d4
static uint32_t s_progress_previous;

static StringQueueEntry s_string_queue[STRING_QUEUE_CAPACITY];

/* Slider initial values per banner — the corpus's option defaults
 * (OptionsLoadOrDefault: speed 1, SFX 8, music 5, brightness 8). */
static int fe_option_default(const char* banner, int lo, int hi);

void fe_item_pos(const FrontEnd* fe, const FeItem* it, int i,
                        int w, const TextureSurface* measure, int* x, int* y);

int save_write_record_target(int index, const uint8_t* blob);
void* sav_build_ctx_target(void);
int save_slot_get_target(void);
void save_slot_set_target(int slot);
void save_slot_selection_clear_target(void);
void* sav_slot_ctx_target(int slot);
void* sav_record_ptr_target(int index);

void menu_save_image_config_rect_blit(FrontEnd* fe,
                                             GameContext* game,
                                             int item_index, void* pixels,
                                             int pitch, int width, int height);

/* Defined below (forward declarations added by tools/relocate_unit.py). */
/* In the matching build ScrollingTextDisplay is GAME-009's Credits_Play
 * (screen_effect_quads.c, finding 609), which takes this name. */
#if defined(_MSC_VER) && _MSC_VER < 1200
extern "C" void scrolling_text_display(void* text, void* font, void* background);
#else
static void scrolling_text_display(void* text, void* font, void* background);
#endif
static void menu_resource_entry_release(MenuResourceEntry* entry);
static void record_window_put(void* rec, int field, const void* src);
static void fe_menu_record_put_word(void* record, int field, uint32_t value);
static void record_window_put_ptr(void* rec, int field, const void* value);
static void record_window_put_u32(void* rec, int field, uint32_t value);
static uint32_t fe_legacy_dword(const void* record, size_t field);
static int fe_legacy_hit(uint32_t mouse_x, uint32_t mouse_y, int32_t x, int32_t y, int32_t width, int32_t height);
static void* fe_legacy_pointer(const void* record, size_t field);
static void fe_overlay_start_random(FrontEnd* fe, const FeSubmenu* sub);
static int frontend_open_prepare(FrontEnd* fe, int menu, int submenu_index);
static void fe_parse_submenu(FrontEnd* fe, FeMenu* m, const char* name, int idx);
static void* sav_current_slot_record_window(void);
static const char* fe_level_group_name(const char* name);
static int fe_parse_int_tuple_bar(const char* text, int* out, int count);
static void fe_sav_menu_list_walk(FeSavMenuListBuild* build, const SavTreeNode* node);
static int save_slot_write_fresh_file(GameContext* ctx, int slot, uint32_t record_count);

/* The mapped target assumes a valid mutable input pointer, returns it
 * unchanged at 256 bytes or more, and otherwise copies to shared storage. */
// FUNCTION: LEGORR 0x004102b0
char* ui_text_underscores_to_spaces_target(char* text)
{
    char* const display = s_ui_text_underscore_buffer;
    const size_t length = strlen(text);
    if (length >= 0x100u) {
        return text;
    }
    strcpy(display, text);
    /* The original scans the full 256-byte scratch region after copying;
     * bytes after the terminator are not visible as part of the returned
     * C string, but retaining the full scan also matches its loop shape. */
    for (size_t i = 0; i < 0x100u; ++i) {
        if (display[i] == '_') {
            display[i] = ' ';
        }
    }
    return display;
}

// FUNCTION: LEGORR 0x00410320
void string_queue_record_or_flush(uint32_t first, uint32_t second,
                                  uint32_t third, const char* text,
                                  int record)
{
    unsigned int i;
    if (!record) {
        if (s_string_queue_flush_hook != NULL) {
            for (i = 0; i < s_string_queue_count; ++i) {
                s_string_queue_flush_hook(s_string_queue[i].first,
                                          s_string_queue[i].second,
                                          s_string_queue[i].third,
                                          s_string_queue[i].text);
            }
        }
        s_string_queue_count = 0;
        return;
    }
    if (s_string_queue_count >= STRING_QUEUE_CAPACITY || text == NULL) return;
    s_string_queue[s_string_queue_count].first = first;
    s_string_queue[s_string_queue_count].second = second;
    s_string_queue[s_string_queue_count].third = third;
    strncpy(s_string_queue[s_string_queue_count].text, text,
            STRING_QUEUE_TEXT_CAPACITY - 1);
    s_string_queue[s_string_queue_count].text[STRING_QUEUE_TEXT_CAPACITY - 1] = '\0';
    ++s_string_queue_count;
}

// FUNCTION: LEGORR 0x004103d0
#if defined(LEGORRR_BACKEND_WINAPI)
__declspec(naked) void main_menu_text_item(void)
{
    __asm {
        mov eax, DAT_00556f54
        mov ecx, DAT_0055613c
        mov edx, DAT_00556f50
        push eax
        push ecx
        push edx
        call scrolling_text_display
        add esp, 0x0c
        ret
    }
}
#else
void main_menu_text_item(void)
{
    scrolling_text_display(DAT_00556f50, DAT_0055613c, DAT_00556f54);
}
#endif

// FUNCTION: LEGORR 0x004103f0
#if defined(LEGORRR_BACKEND_WINAPI)
__declspec(naked) void menu_wipe_start(void)
{
    __asm {
        mov eax, s_menu_wipe_arm
        sub esp, 8
        test eax, eax
        je menu_wipe_start_done
        mov eax, s_menu_wipe
        mov dword ptr s_menu_wipe_stop, 0
        or al, 1
        or al, 2
        mov s_menu_wipe, eax
        call timeGetTime
        mov dword ptr [esp], eax
        mov dword ptr [esp + 4], 0
        fild qword ptr [esp]
        push 0
        push 0x27
        fstp dword ptr s_menu_wipe_clock
        call sound_request_queue_submit
        add esp, 8
    menu_wipe_start_done:
        add esp, 8
        ret
    }
}
#else
void menu_wipe_start(void)
{
    if (s_menu_wipe_arm == 0u) {
        return;
    }
    s_menu_wipe_stop = 0u;
    s_menu_wipe |= 3u;
    s_menu_wipe_clock = (float)s_menu_clock_millis;
    sound_request_queue_submit(0x27, 0);
}
#endif
/* MenuWipeEnd (FUN_00410440) — DAT_005574f0 as a zero-init static; the
 * corpus's MenuWipeStart-side setter is unported. */
// FUNCTION: LEGORR 0x410440
#if defined(LEGORRR_BACKEND_WINAPI)
__declspec(naked) void menu_wipe_end(void)
{
    /* Symbolic operands (like menu_wipe_start): __emit-encoded operand
     * dwords carry no relocation, so reccmp resolved them as an OFFSET
     * into the old .data$zzzzzz pad and scored the two movs as
     * mismatches (measured 50.0%); the symbolic form resolves through
     * the 0x005574f0 annotation as DATA. */
    __asm {
        mov eax, s_menu_wipe
        and al, 0xfc
        mov s_menu_wipe, eax
        ret
    }
}
#else
void menu_wipe_end(void)
{
    s_menu_wipe &= ~3u;
}
#endif

// FUNCTION: LEGORR 0x00410450
#if defined(_MSC_VER) && defined(_M_IX86)
/* Corpus 0x410450..0x410485 keeps the empty-head exit and list walk in this
 * order, including the annotated strcasecmp_lite call. */
__declspec(naked) void* string_keyed_cache_entry_find(const char* key)
{
    __asm {
        push esi
        mov esi, dword ptr [s_string_registry_head]
        test esi, esi
        push edi
        je string_cache_empty
        mov edi, dword ptr [esp + 0ch]
    string_cache_next:
        mov eax, dword ptr [esi]
        push eax
        push edi
        call _strcmpi
        add esp, 8
        test eax, eax
        je string_cache_match
        mov esi, dword ptr [esi + 0ch]
        test esi, esi
        jne string_cache_next
        xor eax, eax
        pop edi
        pop esi
        ret
    string_cache_match:
        mov eax, esi
        pop edi
        pop esi
        ret
    string_cache_empty:
        pop edi
        xor eax, eax
        pop esi
        ret
    }
}
#else
void* string_keyed_cache_entry_find(const char* key)
{
    /* Corpus walk: the head gates once, the name rides strcasecmp_lite
     * untested (intern only ever links an entry whose duplicated name
     * landed), and the +0x0c link advances through a bottom-tested
     * test/jne. */
    void* entry = s_string_registry_head;
    if (entry == NULL) {
        return NULL;
    }
    for (;;) {
        if (strcasecmp_lite(*(char**)entry, key) == 0) {
            return entry;
        }
        /* corpus mov esi,[esi+0xc]: link is field 3 (0xc on the 32-bit
         * target; native entries are widened, see string_registry_intern) */
        entry = *(void**)((char*)entry + 3 * sizeof(void*));
        if (entry == NULL) {
            return NULL;
        }
    }
}
#endif

// FUNCTION: LEGORR 0x00410490
void* string_registry_intern(const char* key)
{
    if (key == NULL || *key == '\0') {
        return NULL; /* corpus's null-pointer and empty-string exits */
    }
    void* entry = string_keyed_cache_entry_find(key);
    if (entry != NULL) {
        return entry;
    }
    /* Corpus strdup idiom: both allocations are unchecked (the corpus
     * stores blind — a NULL allocator answer writes through it), the
     * length rides the inlined strlen intrinsic into the alloc push, and
     * the copy pointer lands in field 0 before the inlined strcpy runs. */
    void* record = engine_heap_alloc(4 * sizeof(void*)); /* 0x10 on x86-32 */
    char** slots = (char**)record;
    char* copy = (char*)engine_heap_alloc((long)strlen(key) + 1);
    slots[0] = copy; /* the corpus stores the copy pointer before the copy */
    strcpy(copy, key);
    void* head = s_string_registry_head; /* the head is loaded once, here */
    slots[1] = NULL;
    slots[3] = (char*)head;
    slots[2] = NULL;
    s_string_registry_head = record;
    return record;
}

// FUNCTION: LEGORR 0x00410520
void* string_cache_lookup_or_load_corpus(const char* key)
{
    if (key != NULL && *key != '\0') {
        void* entry = string_registry_intern(key);
        void** value_slot = (void**)((char*)entry + sizeof(void*));
        if (*value_slot == NULL) {
            *value_slot = image_surface_load(key, 0u, 0u);
        }
        return *value_slot;
    }
    return NULL;
}

// FUNCTION: LEGORR 0x00410560
void* resource_name_cache_get_or_load(const char* key)
{
    if (key != NULL && *key != '\0') {
        void* entry = string_registry_intern(key);
        void** value_slot = (void**)((char*)entry + 2 * sizeof(void*));
        if (*value_slot == NULL) {
            *value_slot = (s_font_resource_loader != NULL)
                ? s_font_resource_loader(key) : NULL;
        }
        return *value_slot;
    }
    return NULL;
}

// FUNCTION: LEGORR 0x004105a0
char* campaign_level_fallback_label(const char* label)
{
    if (label == NULL) {
        return NULL;
    }
    /* Corpus strdup idiom: engine_heap_alloc(strlen+1) then the inlined
     * strcpy intrinsic (rep scas + rep movs). The port's 0x7fffffff
     * length bound was a deviation — the corpus bounds nothing (audited:
     * no test and both callers pass ordinary config labels). */
    char* copy = (char*)engine_heap_alloc((long)strlen(label) + 1);
    if (copy == NULL) {
        return NULL;
    }
    strcpy(copy, label);
    return copy;
}

// FUNCTION: LEGORR 0x004105f0
#if defined(LEGORRR_BACKEND_WINAPI)
__declspec(naked) void* menu_param_record_create(
    uint32_t mode, const char* first, const char* second, uint32_t f4,
    uint32_t f5, uint32_t f6, uint32_t f7, uint32_t f8, uint32_t f9,
    uint32_t f10, uint32_t f11, uint32_t f12, uint32_t f13, uint32_t f14)
{
    __asm {
        push esi
        push edi
        push 0x5c
        call malloc
        mov esi, eax
        mov ecx, 0x17
        xor eax, eax
        mov edi, esi
        add esp, 4
        rep stosd
        mov eax, dword ptr [esp + 0x10]
        push eax
        call campaign_level_fallback_label
        mov ecx, dword ptr [esp + 0x18]
        add esp, 4
        mov dword ptr [esi + 0x20], eax
        push ecx
        call campaign_level_fallback_label
        mov edx, dword ptr [esp + 0x10]
        mov ecx, dword ptr [esp + 0x20]
        mov dword ptr [esi + 0x24], eax
        mov eax, dword ptr [esp + 0x1c]
        mov dword ptr [esi + 0x2c], eax
        mov eax, dword ptr [esp + 0x28]
        mov dword ptr [esi + 0x28], edx
        mov edx, dword ptr [esp + 0x24]
        mov dword ptr [esi + 0x30], ecx
        mov ecx, dword ptr [esp + 0x2c]
        mov dword ptr [esi + 0x38], eax
        mov eax, dword ptr [esp + 0x34]
        mov dword ptr [esi + 0x34], edx
        mov edx, dword ptr [esp + 0x30]
        mov dword ptr [esi + 0x3c], ecx
        mov ecx, dword ptr [esp + 0x38]
        mov dword ptr [esi + 0x44], eax
        mov eax, dword ptr [esp + 0x40]
        add esp, 4
        mov dword ptr [esi + 0x40], edx
        mov edx, dword ptr [esp + 0x38]
        mov dword ptr [esi + 0x48], ecx
        mov ecx, dword ptr [esp + 0x40]
        mov dword ptr [esi + 0x54], eax
        mov dword ptr [esi + 0x50], edx
        mov dword ptr [esi + 0x4c], 0
        mov dword ptr [esi + 0x58], ecx
        mov eax, esi
        pop edi
        pop esi
        ret
    }
}
#else
void* menu_param_record_create(uint32_t mode, const char* first,
                              const char* second, uint32_t f4,
                              uint32_t f5, uint32_t f6, uint32_t f7,
                              uint32_t f8, uint32_t f9, uint32_t f10,
                              uint32_t f11, uint32_t f12, uint32_t f13,
                              uint32_t f14)
{
    void* rec = engine_heap_alloc(23 * sizeof(void*));
    if (rec == NULL) return NULL;
    memset(rec, 0, 23 * sizeof(void*));
    char* first_copy = campaign_level_fallback_label(first);
    char* second_copy = campaign_level_fallback_label(second);
    if ((first != NULL && first_copy == NULL)
        || (second != NULL && second_copy == NULL)) {
        engine_heap_free(first_copy);
        engine_heap_free(second_copy);
        engine_heap_free(rec);
        return NULL;
    }
    char* bytes = (char*)rec;
    *(void**)(bytes + 8 * sizeof(void*)) = first_copy;
    *(void**)(bytes + 9 * sizeof(void*)) = second_copy;
    *(uint32_t*)(bytes + 10 * sizeof(void*)) = mode;
    *(uint32_t*)(bytes + 11 * sizeof(void*)) = f4;
    *(uint32_t*)(bytes + 12 * sizeof(void*)) = f5;
    *(uint32_t*)(bytes + 13 * sizeof(void*)) = f6;
    *(uint32_t*)(bytes + 14 * sizeof(void*)) = f7;
    *(uint32_t*)(bytes + 15 * sizeof(void*)) = f8;
    *(uint32_t*)(bytes + 16 * sizeof(void*)) = f9;
    *(uint32_t*)(bytes + 17 * sizeof(void*)) = f10;
    *(uint32_t*)(bytes + 18 * sizeof(void*)) = f11;
    *(uint32_t*)(bytes + 20 * sizeof(void*)) = f12;
    *(uint32_t*)(bytes + 21 * sizeof(void*)) = f13;
    *(uint32_t*)(bytes + 22 * sizeof(void*)) = f14;
    return rec;
}
#endif

// FUNCTION: LEGORR 0x00410690
#if defined(LEGORRR_BACKEND_WINAPI)
__declspec(naked) void menu_record_resources_release(void* record)
{
    __asm {
        push ebp
        push edi
        mov edi, dword ptr [esp + 0x0c]
        xor ebp, ebp
        cmp edi, ebp
        je release_done
        mov eax, dword ptr [edi + 0x20]
        push esi
        cmp eax, ebp
        push ebx
        je release_second
        push eax
        call free
        add esp, 4
    release_second:
        mov eax, dword ptr [edi + 0x24]
        cmp eax, ebp
        je release_entries
        push eax
        call free
        add esp, 4
    release_entries:
        cmp dword ptr [edi], ebp
        je release_record
        mov eax, dword ptr [edi + 0x1c]
        mov dword ptr [esp + 0x14], ebp
        cmp eax, ebp
        jle release_array
    release_entry_loop:
        mov eax, dword ptr [edi]
        lea ecx, [eax + ebp]
        mov eax, dword ptr [eax + ebp]
        test al, 1
        je release_textures_test
        mov ecx, dword ptr [ecx + 4]
        push ecx
        call free
        add esp, 4
        jmp release_entry_next
    release_textures_test:
        test al, 2
        je release_entry_next
        lea esi, [ebp + 8]
        mov ebx, 3
    release_texture_loop:
        mov edx, dword ptr [edi]
        mov eax, dword ptr [edx + esi]
        push eax
        call image_surface_release
        add esp, 4
        add esi, 4
        dec ebx
        jne release_texture_loop
    release_entry_next:
        mov eax, dword ptr [esp + 0x14]
        mov ecx, dword ptr [edi + 0x1c]
        inc eax
        add ebp, 0x20
        cmp eax, ecx
        mov dword ptr [esp + 0x14], eax
        jl release_entry_loop
    release_array:
        mov ecx, dword ptr [edi]
        push ecx
        call free
        add esp, 4
    release_record:
        push edi
        call free
        add esp, 4
        pop ebx
        pop esi
    release_done:
        pop edi
        pop ebp
        ret
    }
}
#else
void menu_record_resources_release(void* record)
{
    if (record == NULL) return;
    engine_heap_free(rec_ptr(record, 8 * sizeof(void*)));
    engine_heap_free(rec_ptr(record, 9 * sizeof(void*)));
    void* resources = rec_ptr(record, 0);
    const uint32_t count = rec_dword(record, 7 * sizeof(void*));
    if (resources != NULL) {
        for (uint32_t i = 0; i < count; ++i) {
            menu_resource_entry_release((MenuResourceEntry*)resources + i);
        }
        engine_heap_free(resources);
    }
    engine_heap_free(record);
}
#endif

/* SaveMenuListEntryAppend (FUN_00410740), adapted to FeItem instead of the
 * corpus's packed 0x20-byte resource row. */
// FUNCTION: LEGORR 0x00410740
int fe_save_menu_list_entry_append(FeSubmenu* submenu,
                                          const FeSlot* slot, int row)
{
    if (submenu == NULL || slot == NULL || submenu->item_count < 0
        || submenu->item_count >= FE_ITEMS_MAX || row < 0) {
        return 0;
    }
    FeItem* item = &submenu->items[submenu->item_count];
    memset(item, 0, sizeof(*item));
    item->kind = FE_ITEM_NEXT;
    item->load_slot = slot->id;
    snprintf(item->label, FE_NAME_MAX, "%s", slot->label);
    snprintf(item->status, FE_NAME_MAX, "%s", slot->status);
    /* SaveMenuBuild supplies list offsets relative to the submenu anchor. */
    item->x = submenu->base_x - 200;
    item->y = submenu->base_y - 100 + row * 60;
    ++submenu->item_count;
    return 1;
}

// FUNCTION: LEGORR 0x00410a10
#if defined(LEGORRR_BACKEND_WINAPI)
__declspec(naked) void* menu_item_choice_strings_allocate(
    uint32_t count, uint32_t f3, uint32_t f4, uint32_t f5, uint32_t f6)
{
    __asm {
        push ebx
        push ebp
        push esi
        push edi
        push 0x1c
        call malloc
        mov esi, eax
        add esp, 4
        test esi, esi
        jne have_record
        pop edi
        pop esi
        pop ebp
        pop ebx
        ret
    have_record:
        mov ebp, dword ptr [esp + 0x14]
        lea ebx, [ebp * 4]
        push ebx
        call malloc
        mov edi, eax
        add esp, 4
        test edi, edi
        mov dword ptr [esi], edi
        jne have_strings
        push esi
        call resource_handle_list_dispose
        add esp, 4
        xor eax, eax
        pop edi
        pop esi
        pop ebp
        pop ebx
        ret
    have_strings:
        mov ecx, ebx
        xor eax, eax
        mov edx, ecx
        shr ecx, 2
        rep stosd
        mov ecx, edx
        mov edx, dword ptr [esp + 0x20]
        and ecx, 3
        rep stosb
        mov eax, dword ptr [esp + 0x18]
        mov ecx, dword ptr [esp + 0x1c]
        mov dword ptr [esi + 0x0c], eax
        mov eax, dword ptr [esp + 0x24]
        mov dword ptr [esi + 0x18], eax
        mov dword ptr [esi + 4], ebp
        mov dword ptr [esi + 8], 0
        mov dword ptr [esi + 0x10], ecx
        mov dword ptr [esi + 0x14], edx
        mov eax, esi
        pop edi
        pop esi
        pop ebp
        pop ebx
        ret
    }
}
#else
void* menu_item_choice_strings_allocate(uint32_t count, uint32_t f3,
                                        uint32_t f4, uint32_t f5,
                                        uint32_t f6)
{
    if ((size_t)count > (size_t)LONG_MAX / sizeof(void*)) return NULL;
    void* rec = engine_heap_alloc(7 * sizeof(void*));
    if (rec == NULL) return NULL;
    memset(rec, 0, 7 * sizeof(void*));
    void* strings = engine_heap_alloc((long)((size_t)count * sizeof(void*)));
    if (strings == NULL) {
        engine_heap_free(rec);
        return NULL;
    }
    memset(strings, 0, (size_t)count * sizeof(void*));
    *(void**)((char*)rec + 0 * sizeof(void*)) = strings;
    *(uint32_t*)((char*)rec + 1 * sizeof(void*)) = count;
    *(uint32_t*)((char*)rec + 2 * sizeof(void*)) = 0;
    *(uint32_t*)((char*)rec + 3 * sizeof(void*)) = f3;
    *(uint32_t*)((char*)rec + 4 * sizeof(void*)) = f4;
    *(uint32_t*)((char*)rec + 5 * sizeof(void*)) = f5;
    *(uint32_t*)((char*)rec + 6 * sizeof(void*)) = f6;
    return rec;
}
#endif

/* ResourceHandleListAppend (FUN_00410aa0): append a heap-owned string only
 * while the preallocated handle array has capacity. */
// FUNCTION: LEGORR 0x00410aa0
void resource_handle_list_append(ResourceHandleList* list, const char* value)
{
    if (list == NULL || list->handles == NULL
        || list->count >= (int32_t)(uint32_t)list->word1 || value == NULL) {
        return;
    }
    const size_t length = strlen(value);
    if (length >= 0x7fffffffU) return;
    char* copy = (char*)engine_heap_alloc((long)length + 1);
    if (copy == NULL) return;
    strncpy(copy, value, length + 1);
    list->handles[list->count] = copy;
    ++list->count;
}
/* ResourceHandleListDispose (FUN_00410ae0): free the list's non-zero
 * handles (the count lives behind), then the array, then the record — the
 * record is freed even when its array word is zero. Deviations: the record
 * fields widen LP64-safe and the NULL record is guarded (the corpus strides
 * blindly). */
// FUNCTION: LEGORR 0x00410ae0
void resource_handle_list_dispose(ResourceHandleList* list)
{
    if (list == NULL) {
        return; /* deviation: the corpus derefs blindly */
    }
    if (list->handles != NULL) {
        for (int32_t i = 0; i < list->count; ++i) {
            if (list->handles[i] != NULL) {
                engine_heap_free(list->handles[i]); /* FUN_0048db80 */
            }
        }
        engine_heap_free(list->handles);
    }
    engine_heap_free(list);
}

// FUNCTION: LEGORR 0x00410b30
uint32_t renderer_word_a_get(void) { return s_renderer_word_a; } /* FUN_00410b30 */
// FUNCTION: LEGORR 0x00410b40
uint32_t renderer_word_b_get(void) { return s_renderer_word_b; } /* FUN_00410b40 */
// FUNCTION: LEGORR 0x00410b50
uint32_t renderer_word_c_get(void) { return g_renderer_word_c; } /* FUN_00410b50 */

/* ThreeWordRecordAllocateAndInitialize (FUN_00410b60) — see heap.hpp. */
// FUNCTION: LEGORR 0x00410b60
void* three_word_record_allocate_and_initialize(uint32_t a, uint32_t b,
                                                uint32_t c)
{
    uint32_t* rec = (uint32_t*)engine_heap_alloc(12);
    if (rec == NULL) {
        return NULL; /* deviation: the corpus strides the record blindly */
    }
    rec[0] = a;
    rec[1] = b;
    rec[2] = c;
    return rec;
}

/* BoundedFloatRecordCreate (FUN_00410b90) — see record.hpp. */
// FUNCTION: LEGORR 0x00410b90
void* bounded_float_record_create(int kind, float min_v, float max_v,
                                  uint32_t f3, uint32_t f4, uint32_t f5,
                                  uint32_t f6)
{
    if (!(min_v < max_v)) {
        return NULL;
    }
    void* rec = engine_heap_alloc(7 * sizeof(void*));
    if (rec == NULL) {
        return NULL;
    }
    const uint32_t kind_w = (uint32_t)kind;
    record_window_put(rec, 0, &kind_w);
    record_window_put(rec, 1, &min_v);
    record_window_put(rec, 2, &max_v);
    record_window_put(rec, 3, &f3);
    record_window_put(rec, 4, &f4);
    record_window_put(rec, 5, &f5);
    record_window_put(rec, 6, &f6);
    return rec;
}

/* MenuItemNumericRangeRecordAllocate (FUN_00410bf0) — see record.hpp.
 * The record has fourteen dword fields: kind, bounds, then eleven payload
 * words. Bounds are stored after the first payload fields in corpus order. */
// FUNCTION: LEGORR 0x00410bf0
#if defined(LEGORRR_BACKEND_WINAPI)
__declspec(naked) void* menu_item_numeric_range_record_allocate(
    int kind, int min_v, int max_v, uint32_t f3, uint32_t f4, uint32_t f5,
    uint32_t f6, uint32_t f7, uint32_t f8, uint32_t f9, uint32_t f10,
    uint32_t f11, uint32_t f12, uint32_t f13)
{
    __asm {
        push esi
        mov esi, dword ptr [esp + 0x10]
        push edi
        mov edi, dword ptr [esp + 0x10]
        cmp esi, edi
        jg menu_item_numeric_range_record_allocate_alloc
        xor eax, eax
        pop edi
        pop esi
        ret
    menu_item_numeric_range_record_allocate_alloc:
        push 0x38
        call malloc
        add esp, 4
        test eax, eax
        jne menu_item_numeric_range_record_allocate_store
        pop edi
        pop esi
        ret
    menu_item_numeric_range_record_allocate_store:
        mov ecx, dword ptr [esp + 0x0c]
        mov edx, dword ptr [esp + 0x18]
        mov dword ptr [eax], ecx
        mov ecx, dword ptr [esp + 0x1c]
        mov dword ptr [eax + 0x0c], edx
        mov edx, dword ptr [esp + 0x20]
        mov dword ptr [eax + 0x10], ecx
        mov ecx, dword ptr [esp + 0x24]
        mov dword ptr [eax + 0x14], edx
        mov edx, dword ptr [esp + 0x28]
        mov dword ptr [eax + 0x18], ecx
        mov ecx, dword ptr [esp + 0x2c]
        mov dword ptr [eax + 0x1c], edx
        mov edx, dword ptr [esp + 0x30]
        mov dword ptr [eax + 0x20], ecx
        mov ecx, dword ptr [esp + 0x34]
        mov dword ptr [eax + 0x24], edx
        mov edx, dword ptr [esp + 0x38]
        mov dword ptr [eax + 0x28], ecx
        mov ecx, dword ptr [esp + 0x3c]
        mov dword ptr [eax + 0x2c], edx
        mov edx, dword ptr [esp + 0x40]
        mov dword ptr [eax + 4], edi
        mov dword ptr [eax + 8], esi
        pop edi
        mov dword ptr [eax + 0x30], ecx
        mov dword ptr [eax + 0x34], edx
        pop esi
        ret
    }
}
#else
void* menu_item_numeric_range_record_allocate(int kind, int min_v, int max_v,
                                              uint32_t f3, uint32_t f4,
                                              uint32_t f5, uint32_t f6,
                                              uint32_t f7, uint32_t f8,
                                              uint32_t f9, uint32_t f10,
                                              uint32_t f11, uint32_t f12,
                                              uint32_t f13)
{
    if (!(min_v < max_v)) return NULL;
    void* rec = engine_heap_alloc(14 * sizeof(void*));
    if (rec == NULL) return NULL;
    char* bytes = (char*)rec;
    *(uint32_t*)(bytes + 0 * sizeof(void*)) = (uint32_t)kind;
    *(uint32_t*)(bytes + 3 * sizeof(void*)) = f3;
    *(uint32_t*)(bytes + 4 * sizeof(void*)) = f4;
    *(uint32_t*)(bytes + 5 * sizeof(void*)) = f5;
    *(uint32_t*)(bytes + 6 * sizeof(void*)) = f6;
    *(uint32_t*)(bytes + 7 * sizeof(void*)) = f7;
    *(uint32_t*)(bytes + 8 * sizeof(void*)) = f8;
    *(uint32_t*)(bytes + 9 * sizeof(void*)) = f9;
    *(uint32_t*)(bytes + 10 * sizeof(void*)) = f10;
    *(uint32_t*)(bytes + 11 * sizeof(void*)) = f11;
    *(uint32_t*)(bytes + 12 * sizeof(void*)) = f12;
    *(uint32_t*)(bytes + 1 * sizeof(void*)) = (uint32_t)min_v;
    *(uint32_t*)(bytes + 2 * sizeof(void*)) = (uint32_t)max_v;
    *(uint32_t*)(bytes + 13 * sizeof(void*)) = f13;
    return rec;
}
#endif
// FUNCTION: LEGORR 0x00410c70
void resource_variant_record_release(void* record)
{
    if (record == NULL) return;
    engine_heap_free(rec_ptr(record, 0));
    const uint32_t kind = rec_dword(record, 5 * sizeof(void*));
    void* payload = rec_ptr(record, 4 * sizeof(void*));
    switch (kind) {
    case 0:
        resource_handle_list_dispose((ResourceHandleList*)payload);
        break;
    case 1:
    case 2:
    case 3:
    case 4:
        optional_resource_destroy(payload);
        break;
    case 6:
        menu_record_resources_release(payload);
        break;
    default:
        break;
    }
    engine_heap_free(record);
}

/* MenuTextRecordMake (FUN_00410d50): stores a copied label, layout words,
 * font handles and optional negative half-width offsets. */
// FUNCTION: LEGORR 0x00410d50
#if defined(LEGORRR_BACKEND_WINAPI)
__declspec(naked) void* menu_text_record_make(
    const char* text, const TextureSurface* font_a,
    const TextureSurface* font_b, uint32_t f4, uint32_t f5,
    uint32_t f6, int center, uint32_t f8, uint32_t f9)
{
    __asm {
        push ebx
        mov ebx, dword ptr [esp + 8]
        push esi
        push edi
        test ebx, ebx
        jne text_record_allocate
        xor eax, eax
        pop edi
        pop esi
        pop ebx
        ret
    text_record_allocate:
        push 0x3c
        call malloc
        mov esi, eax
        add esp, 4
        test esi, esi
        jne text_record_initialize
        pop edi
        pop esi
        pop ebx
        ret
    text_record_initialize:
        mov ecx, 0x0f
        xor eax, eax
        mov edi, esi
        push ebx
        rep stosd
        mov edi, ebx
        or ecx, 0xffffffff
        repnz scasb
        not ecx
        dec ecx
        mov dword ptr [esi + 4], ecx
        call campaign_level_fallback_label
        mov ecx, dword ptr [esp + 0x28]
        mov edx, dword ptr [esp + 0x30]
        mov edi, dword ptr [esp + 0x1c]
        mov dword ptr [esi + 0x14], ecx
        mov ecx, dword ptr [esp + 0x20]
        mov dword ptr [esi], eax
        mov eax, dword ptr [esp + 0x18]
        mov dword ptr [esi + 0x18], ecx
        mov ecx, dword ptr [esp + 0x34]
        mov dword ptr [esi + 0x10], edx
        mov edx, dword ptr [esp + 0x24]
        mov dword ptr [esi + 0x38], ecx
        mov ecx, dword ptr [esp + 0x2c]
        add esp, 4
        test ecx, ecx
        mov dword ptr [esi + 0x0c], eax
        mov dword ptr [esi + 8], edi
        mov dword ptr [esi + 0x1c], edx
        je text_record_zero_offsets
        test eax, eax
        je text_record_zero_offsets
        test edi, edi
        je text_record_zero_offsets
        push ebx
        push eax
        call formatted_text_printf_width
        add esp, 8
        shr eax, 1
        neg eax
        push ebx
        push edi
        mov dword ptr [esi + 0x20], eax
        call formatted_text_printf_width
        shr eax, 1
        add esp, 8
        neg eax
        mov dword ptr [esi + 0x24], eax
        mov eax, esi
        pop edi
        pop esi
        pop ebx
        ret
    text_record_zero_offsets:
        mov dword ptr [esi + 0x20], 0
        mov dword ptr [esi + 0x24], 0
        mov eax, esi
        pop edi
        pop esi
        pop ebx
        ret
    }
}
#else
void* menu_text_record_make(const char* text, const TextureSurface* font_a,
                            const TextureSurface* font_b, uint32_t f4,
                            uint32_t f5, uint32_t f6, int center,
                            uint32_t f8, uint32_t f9)
{
    if (text == NULL) return NULL;
    void* record = engine_heap_alloc(15 * sizeof(void*));
    if (record == NULL) return NULL;
    memset(record, 0, 15 * sizeof(void*));
    char* copy = campaign_level_fallback_label(text);
    if (copy == NULL) {
        engine_heap_free(record);
        return NULL;
    }
    const size_t length = strlen(text);
    void* font_a_slot = (void*)font_a;
    void* font_b_slot = (void*)font_b;
    __builtin_memcpy((char*)record, &copy, sizeof(copy));
    fe_menu_record_put_word(record, 1, (uint32_t)length);
    __builtin_memcpy((char*)record + 2 * sizeof(void*), &font_b_slot,
                     sizeof(font_b_slot));
    __builtin_memcpy((char*)record + 3 * sizeof(void*), &font_a_slot,
                     sizeof(font_a_slot));
    fe_menu_record_put_word(record, 4, f8);
    fe_menu_record_put_word(record, 5, f6);
    fe_menu_record_put_word(record, 6, f4);
    fe_menu_record_put_word(record, 7, f5);
    fe_menu_record_put_word(record, 14, f9);
    if (center != 0 && font_a != NULL && font_b != NULL) {
        fe_menu_record_put_word(record, 8,
            0u - (formatted_text_printf_width(font_a, text) >> 1));
        fe_menu_record_put_word(record, 9,
            0u - (formatted_text_printf_width(font_b, text) >> 1));
    }
    return record;
}
#endif

/* MenuButtonRecordMake (FUN_00410e20): create a 15-slot image-backed menu
 * item, including its optional tooltip-name index. */
// FUNCTION: LEGORR 0x00410e20
void* menu_button_record_make(const char* text,
                              const TextureSurface* normal_font,
                              const TextureSurface* selected_font,
                              const char* normal_image,
                              const char* selected_image, uint32_t x,
                              uint32_t y, uint32_t flags, int centered,
                              const char* optional_name, uint32_t extra)
{
    if (text == NULL || normal_image == NULL) return NULL;
    void* record = engine_heap_alloc(15 * sizeof(void*));
    if (record == NULL) return NULL;
    memset(record, 0, 15 * sizeof(void*));

    char* label = campaign_level_fallback_label(text);
    if (label == NULL) {
        engine_heap_free(record);
        return NULL;
    }
    const TextureSurface* normal = image_surface_load(normal_image, 0, 0);
    if (normal == NULL) {
        engine_heap_free(label);
        engine_heap_free(record);
        return NULL;
    }
    const TextureSurface* selected = selected_image != NULL
        ? image_surface_load(selected_image, 0, 0) : NULL;

    void* label_slot = label;
    void* selected_slot = (void*)selected;
    void* normal_slot = (void*)normal;
    __builtin_memcpy((char*)record, &label_slot, sizeof(label_slot));
    fe_menu_record_put_word(record, 1, (uint32_t)strlen(text));
    void* selected_font_slot = (void*)selected_font;
    void* normal_font_slot = (void*)normal_font;
    __builtin_memcpy((char*)record + 2 * sizeof(void*), &selected_font_slot,
                     sizeof(selected_font_slot));
    __builtin_memcpy((char*)record + 3 * sizeof(void*), &normal_font_slot,
                     sizeof(normal_font_slot));
    __builtin_memcpy((char*)record + 11 * sizeof(void*), &normal_slot,
                     sizeof(normal_slot));
    __builtin_memcpy((char*)record + 12 * sizeof(void*), &selected_slot,
                     sizeof(selected_slot));
    if (selected != NULL) fe_menu_record_put_word(record, 10, 1);
    fe_menu_record_put_word(record, 4, extra);
    fe_menu_record_put_word(record, 5, flags);
    fe_menu_record_put_word(record, 6, x);
    fe_menu_record_put_word(record, 7, y);
    if (centered && selected != NULL) {
        fe_menu_record_put_word(record, 8,
            0u - ((uint32_t)normal->width >> 1));
        fe_menu_record_put_word(record, 9,
            0u - ((uint32_t)selected->width >> 1));
    }
    int name_index = 0;
    if (case_insensitive_name_table_lookup(optional_name, &name_index)) {
        fe_menu_record_put_word(record, 13, (uint32_t)name_index);
    }
    return record;
}

// FUNCTION: LEGORR 0x00410f30
void resource_record_and_children_release(void* record)
{
    if (record == NULL) return;
    engine_heap_free(rec_ptr(record, 0));
    engine_heap_free(rec_ptr(record, sizeof(void*)));
    void* children = rec_ptr(record, 6 * sizeof(void*));
    const uint32_t count = rec_dword(record, 7 * sizeof(void*));
    /* The original frees the pointer array before walking it. Keep the
     * same ownership result while releasing children before their array. */
    for (uint32_t i = 0; children != NULL && i < count; ++i) {
        void* child = NULL;
        __builtin_memcpy(&child, (char*)children + (size_t)i * sizeof(void*),
                         sizeof(child));
        resource_variant_record_release(child);
    }
    engine_heap_free(children);
    texture_release((TextureSurface*)rec_ptr(record, 4 * sizeof(void*)));
    engine_heap_free(record);
}

// FUNCTION: LEGORR 0x00410fb0
int menu_item_image_set(void* item, const char* image_config, int primary)
{
    if (item == NULL || image_config == NULL) return 0;
    char text[1024];
    size_t length = strlen(image_config);
    if (length >= sizeof(text)) return 0;
    strncpy(text, image_config, length + 1);
    char* tokens[4] = { NULL, NULL, NULL, NULL };
    const int count = sound_variant_split(text, tokens, ",");
    if (count == 0) return 0;
    if (count > 2) {
        const size_t flags_offset = 13 * sizeof(void*);
        const uint32_t flags = rec_dword(item, flags_offset) | 1u;
        __builtin_memcpy((char*)item + flags_offset, &flags, sizeof(flags));
        const float x = (float)atof(tokens[1]);
        const float y = (float)atof(tokens[2]);
        __builtin_memcpy((char*)item + 14 * sizeof(void*), &x, sizeof(x));
        __builtin_memcpy((char*)item + 15 * sizeof(void*), &y, sizeof(y));
    }
    TextureSurface* surface = (TextureSurface*)string_cache_lookup_or_load(tokens[0]);
    const int field = primary ? 4 : 5;
    *(void**)((char*)item + (size_t)field * sizeof(void*)) = surface;
    if (surface == NULL) return 0;
    if (count == 4 && atoi(tokens[3]) == 1) {
        if (primary) {
            resource_record_add_ref(surface);
        } else {
            /* The corpus calls ImageSurfaceBlitTransformed with zero
             * transform/color arguments on this branch. */
            texture_source_color_key_set(surface, 0, 0);
            surface->resource_flags |= 2u;
        }
    }
    return 1;
}
// FUNCTION: LEGORR 0x00411100
void* menu_section_record_create(const char* name, const char* title,
                                 uint32_t f3, uint32_t f4, uint32_t f5,
                                 uint32_t f6, uint32_t f7, uint32_t f8,
                                 uint32_t f9, const char* integer_pair)
{
    if (name == NULL || title == NULL) return NULL;
    void* rec = engine_heap_alloc(40 * sizeof(void*));
    if (rec == NULL) return NULL;
    memset(rec, 0, 40 * sizeof(void*));
    char* name_copy = campaign_level_fallback_label(name);
    char* title_copy = campaign_level_fallback_label(title);
    if (name_copy == NULL || title_copy == NULL) {
        engine_heap_free(name_copy);
        engine_heap_free(title_copy);
        engine_heap_free(rec);
        return NULL;
    }
    void* child = engine_heap_alloc(15 * sizeof(void*));
    if (child == NULL) {
        engine_heap_free(name_copy);
        engine_heap_free(title_copy);
        engine_heap_free(rec);
        return NULL;
    }
    memset(child, 0, 15 * sizeof(void*));
    record_window_put_ptr(rec, 0, name_copy);
    record_window_put_ptr(rec, 1, title_copy);
    record_window_put_u32(rec, 2, (uint32_t)strlen(name));
    record_window_put_ptr(rec, 6, child);
    record_window_put_u32(rec, 3, f3);
    record_window_put_u32(rec, 11, f4);
    record_window_put_u32(rec, 12, f5);
    record_window_put_u32(rec, 17, f6);
    record_window_put_u32(rec, 18, f7);
    record_window_put_u32(rec, 22, f8);
    record_window_put_u32(rec, 8, 15);
    record_window_put_u32(rec, 13, f9 != 0 ? 2u : 0u);
    if (integer_pair != NULL && integer_pair[0] != '\0') {
        char copy[1024];
        size_t length = strlen(integer_pair);
        if (length >= sizeof(copy)) length = sizeof(copy) - 1;
        memcpy(copy, integer_pair, length);
        copy[length] = '\0';
        char* first = copy;
        char* second = NULL;
        for (size_t i = 0; i + 1 < length; ++i) {
            if (copy[i] == ',') {
                copy[i] = '\0';
                second = copy + i + 1;
                break;
            }
        }
        if (second != NULL) {
            record_window_put_u32(rec, 19, 1);
            record_window_put_u32(rec, 20, (uint32_t)atoi(first));
            record_window_put_u32(rec, 21, (uint32_t)atoi(second));
        }
    }
    return rec;
}

// FUNCTION: LEGORR 0x00411260
int menu_list_append(void* list, void* value)
{
    if (list == NULL || value == NULL) return 0;
    void* items = rec_ptr(list, 6 * sizeof(void*));
    const uint32_t count = rec_dword(list, 7 * sizeof(void*));
    const uint32_t capacity = rec_dword(list, 8 * sizeof(void*));
    if (count == capacity) {
        if (capacity > (uint32_t)(LONG_MAX / (long)sizeof(void*) - 15)) {
            return 0;
        }
        const uint32_t new_capacity = capacity + 15;
#if defined(_MSC_VER) && defined(_M_IX86)
        void* grown;
        const size_t new_bytes = (size_t)new_capacity * sizeof(void*);
        if (items == NULL) {
            grown = malloc(new_bytes);
            if (grown != NULL) {
                memset(grown, 0, new_bytes);
            }
        } else {
            grown = realloc(items, new_bytes);
            if (grown != NULL) {
                memset((char*)grown + (size_t)capacity * sizeof(void*), 0,
                       (size_t)(new_capacity - capacity) * sizeof(void*));
            }
        }
        if (grown == NULL) return 0;
#else
        void* grown = engine_heap_alloc((long)((size_t)new_capacity * sizeof(void*)));
        if (grown == NULL) return 0;
        memset(grown, 0, (size_t)new_capacity * sizeof(void*));
        if (items != NULL) {
            memcpy(grown, items, (size_t)count * sizeof(void*));
            engine_heap_free(items);
        }
#endif
        items = grown;
        record_window_put_ptr(list, 6, items);
        record_window_put_u32(list, 8, new_capacity);
    }
    __builtin_memcpy((char*)items + (size_t)count * sizeof(void*), &value,
                     sizeof(value));
    record_window_put_u32(list, 7, count + 1);
    return 1;
}

/* MenuPointerRectangleHitTest (FUN_004112e0): the corpus's widget test,
 * reproduced clause-for-clause — inclusive lower corners, inclusive
 * upper corners via origin+size (finding 23: the widget rectangle is
 * the 100x20 row band this routine hit-tests). */
// FUNCTION: LEGORR 0x004112e0
#if defined(LEGORRR_BACKEND_WINAPI)
__declspec(naked) int menu_pointer_rectangle_hit_test(
    int x, int y, int rect_x, int rect_y, int rect_w, int rect_h)
{
    __asm {
        mov eax, dword ptr [esp + 4]
        mov ecx, dword ptr [esp + 0x0c]
        cmp eax, ecx
        jl menu_pointer_rectangle_hit_test_miss
        mov edx, dword ptr [esp + 0x14]
        add edx, ecx
        cmp eax, edx
        jg menu_pointer_rectangle_hit_test_miss
        mov eax, dword ptr [esp + 8]
        mov ecx, dword ptr [esp + 0x10]
        cmp eax, ecx
        jl menu_pointer_rectangle_hit_test_miss
        mov edx, dword ptr [esp + 0x18]
        add edx, ecx
        cmp eax, edx
        jg menu_pointer_rectangle_hit_test_miss
        mov eax, 1
        ret
    menu_pointer_rectangle_hit_test_miss:
        xor eax, eax
        ret
    }
}
#else
int menu_pointer_rectangle_hit_test(int x, int y, int rect_x, int rect_y,
                                    int rect_w, int rect_h)
{
    if (x < rect_x || x > rect_w + rect_x) return 0;
    if (y < rect_y || y > rect_h + rect_y) return 0;
    return 1;
}
#endif

/* TextLayoutOffsetForCentering (FUN_00411320) — see frontend.hpp. */
// FUNCTION: LEGORR 0x411320
int text_layout_offset_for_centering(int x, int y, int rect_x, int rect_y,
                                     int rect_w, int rect_h, int center)
{
    const int xoff = (center != 0) ? -(rect_w / 2) : 0;
    return menu_pointer_rectangle_hit_test(x, y, xoff + rect_x, rect_y,
                                           rect_w, rect_h);
}
/* MenuMouseCoordinateUpdate (FUN_00411360): test the two row labels first,
 * then the visible type-6 resource entries. */
// FUNCTION: LEGORR 0x00411360
int menu_mouse_coordinate_update(const void* root, const void* item,
                                 const void* row)
{
    if (root == NULL || item == NULL || row == NULL) return -1;
    const TextureSurface* face = (const TextureSurface*)
        fe_legacy_pointer(item, 2);
    const char* label = (const char*)fe_legacy_pointer(row, 8);
    uint32_t first_width = 0, first_height = 0;
    if (face != NULL) {
        first_width = formatted_text_printf_width(face, label);
        first_height = (uint32_t)face->height;
    }
    const int32_t mouse_x = (int32_t)renderer_word_a_get();
    const int32_t mouse_y = (int32_t)renderer_word_b_get();
    int32_t x = (int32_t)fe_legacy_dword(root, 11)
        + (int32_t)fe_legacy_dword(row, 15);
    int32_t y = (int32_t)fe_legacy_dword(root, 12)
        + (int32_t)fe_legacy_dword(row, 16);
    if (fe_legacy_hit((uint32_t)mouse_x, (uint32_t)mouse_y, x, y,
                      (int32_t)first_width, (int32_t)first_height)) {
        return -2;
    }

    label = (const char*)fe_legacy_pointer(row, 9);
    uint32_t second_width = 0, second_height = 0;
    if (face != NULL) {
        second_width = formatted_text_printf_width(face, label);
        second_height = (uint32_t)face->height;
    }
    x = (int32_t)fe_legacy_dword(root, 11)
        + (int32_t)fe_legacy_dword(row, 17);
    y = (int32_t)fe_legacy_dword(root, 12)
        + (int32_t)fe_legacy_dword(row, 18);
    if (fe_legacy_hit((uint32_t)mouse_x, (uint32_t)mouse_y, x, y,
                      (int32_t)second_width, (int32_t)second_height)) {
        return -3;
    }

    const uint32_t visible = fe_legacy_dword(row, 14);
    const uint32_t start = fe_legacy_dword(row, 19);
    const uint32_t total = fe_legacy_dword(row, 7);
    const uint8_t* entries = (const uint8_t*)fe_legacy_pointer(row, 0);
    const uint32_t* widths = (const uint32_t*)fe_legacy_pointer(row, 1);
    const uint32_t* heights = (const uint32_t*)fe_legacy_pointer(row, 4);
    if (entries == NULL || widths == NULL || heights == NULL) return -1;
    for (uint32_t i = 0; i < visible; ++i) {
        const uint32_t index = start + i;
        if (index >= total) break;
        const uint8_t* entry = entries + (size_t)index * 0x20;
        uint32_t flags = 0, offset_x = 0, offset_y = 0;
        __builtin_memcpy(&flags, entry, sizeof(flags));
        if ((flags & 4u) == 0) continue;
        __builtin_memcpy(&offset_x, entry + 0x14, sizeof(offset_x));
        __builtin_memcpy(&offset_y, entry + 0x18, sizeof(offset_y));
        x = (int32_t)fe_legacy_dword(root, 11)
            + (int32_t)fe_legacy_dword(item, 6)
            + (int32_t)fe_legacy_dword(row, 11)
            + (int32_t)offset_x + (int32_t)s_menu_plane_pan[0] + mouse_x;
        y = (int32_t)fe_legacy_dword(root, 12)
            + (int32_t)fe_legacy_dword(item, 7)
            + (int32_t)fe_legacy_dword(row, 12)
            + (int32_t)offset_y + (int32_t)s_menu_plane_pan[1]
            + (int32_t)fe_legacy_dword(row, 13) * (int32_t)i + mouse_y;
        if (text_layout_offset_for_centering(mouse_x, mouse_y, x, y,
                (int32_t)widths[index], (int32_t)heights[index],
                (int)fe_legacy_dword(root, 17))) {
            return (int)(start + i);
        }
    }
    return -1;
}

/* HasType6ItemAtIndex (FUN_004114f0): the item is active when its row
 * renderer reports any hit result other than the no-hit sentinel. */
// FUNCTION: LEGORR 0x004114f0
int has_type6_item_at_index(const void* menu, int index)
{
    if (menu == NULL || index < 0
        || (uint32_t)index >= fe_legacy_dword(menu, 7)) {
        return 0;
    }
    void* items = fe_legacy_pointer(menu, 6);
    if (items == NULL) return 0;
    void* item = NULL;
    __builtin_memcpy(&item, (char*)items + (size_t)index * sizeof(void*),
                     sizeof(item));
    if (item == NULL || fe_legacy_dword(item, 5) != 6) return 0;
    return menu_mouse_coordinate_update(menu, item,
        fe_legacy_pointer(item, 4)) != -1;
}

/* MenuWidgetItemScreenRect (FUN_00411530): calculate a menu item's clickable
 * screen rectangle. Legacy menu records store scalar values in the low dword
 * of each native-width slot, which keeps this layout usable in 64-bit builds. */
// FUNCTION: LEGORR 0x00411530
int menu_widget_item_screen_rect(const void* menu, int index, int* x, int* y,
                                 int* width, int* height)
{
    if (menu == NULL || index < 0
        || (uint32_t)index >= fe_legacy_dword(menu, 7)) return 0;
    void* items = fe_legacy_pointer(menu, 6);
    if (items == NULL) return 0;
    void* item = NULL;
    __builtin_memcpy(&item, (char*)items + (size_t)index * sizeof(void*),
                     sizeof(item));
    if (item == NULL) return 0;

    const uint32_t type = fe_legacy_dword(item, 5);
    if (type == 6) return 0;
    const int32_t origin_x = (int32_t)fe_legacy_dword(menu, 11);
    const int32_t origin_y = (int32_t)fe_legacy_dword(menu, 12);
    const int32_t item_x = (int32_t)fe_legacy_dword(item, 6);
    const int32_t item_y = (int32_t)fe_legacy_dword(item, 7);
    const bool selected = (int32_t)fe_legacy_dword(menu, 9) == index;

    int32_t left = origin_x + (int32_t)fe_legacy_dword(item, 9) + item_x;
    int32_t top = origin_y + item_y;
    int32_t rect_width = 0;
    int32_t rect_height = 0;
    if (type == 3) {
        const char* label = (const char*)fe_legacy_pointer(item, 0);
        const TextureSurface* font = (const TextureSurface*)
            fe_legacy_pointer(item, selected ? 2 : 3);
        const int32_t label_width = (int32_t)
            formatted_text_printf_width(font, label);
        const int32_t label_height = font != NULL ? font->height : 0;
        const int32_t label_x = left;
        const int32_t label_y = top;

        const void* slider = fe_legacy_pointer(item, 4);
        const int32_t slider_x = origin_x + item_x
            + (int32_t)fe_legacy_dword(slider, 3);
        const int32_t slider_y = origin_y + item_y
            + (int32_t)fe_legacy_dword(slider, 4);
        const TextureSurface* left_cap = (const TextureSurface*)
            fe_legacy_pointer(slider, 9);
        const TextureSurface* right_cap = (const TextureSurface*)
            fe_legacy_pointer(slider, 10);
        const int32_t extra_left = left_cap != NULL ? left_cap->width : 0;
        const int32_t extra_right = right_cap != NULL ? right_cap->width : 0;

        left = label_x < slider_x ? label_x : slider_x;
        top = label_y < slider_y ? label_y : slider_y;
        const int32_t label_right = label_x + label_width;
        const int32_t slider_right = slider_x + extra_left + 0x68 + extra_right;
        const int32_t bottom_label = label_y + label_height;
        const int32_t bottom_slider = slider_y + 0x14;
        const int32_t right = label_right > slider_right ? label_right : slider_right;
        const int32_t bottom = bottom_label > bottom_slider
            ? bottom_label : bottom_slider;
        rect_width = right - left;
        rect_height = bottom - top;
    }
    else if (fe_legacy_dword(item, 10) != 0) {
        const TextureSurface* surface = (const TextureSurface*)
            fe_legacy_pointer(item, selected ? 12 : 11);
        rect_width = surface != NULL ? surface->width : 0;
        rect_height = surface != NULL ? surface->height : 0;
    }
    else {
        const char* label = (const char*)fe_legacy_pointer(item, 0);
        const TextureSurface* font = (const TextureSurface*)
            fe_legacy_pointer(item, selected ? 2 : 3);
        rect_width = (int32_t)formatted_text_printf_width(font, label);
        rect_height = font != NULL ? font->height : 0;
    }

    if (x != NULL) *x = left;
    if (y != NULL) *y = top;
    if (width != NULL) *width = rect_width;
    if (height != NULL) *height = rect_height;
    return 1;
}

/* MenuPickWidgetUnderPointer (FUN_00411790): the regular-rect helper gates
 * the walk; a failed rect returns unless the type-6 helper hits. */
// FUNCTION: LEGORR 0x00411790
int menu_pick_widget_under_pointer(const void* menu, int* picked_index)
{
    int x = 0, y = 0, width = 0, height = 0;
    int index = 0;
    int type6_hit = 0;
    for (;;) {
        const int has_rect = menu_widget_item_screen_rect(menu, index, &x, &y,
                                                          &width, &height);
        if (!has_rect) {
            type6_hit = has_type6_item_at_index(menu, index);
            if (!type6_hit) return 0;
        }
        if (type6_hit) break;
        if (menu_pointer_rectangle_hit_test(
                (int32_t)renderer_word_a_get(),
                (int32_t)renderer_word_b_get(), x, y, width, height)) {
            break;
        }
        ++index;
    }
    if (picked_index != NULL) *picked_index = index;
    return 1;
}

/* FrontendProgressValueChanged (FUN_00411840): detect the transition from
 * progress-active (1) back to idle (0) in the renderer's third word. */
// FUNCTION: LEGORR 0x00411840
int frontend_progress_value_changed(void)
{
    const uint32_t current = g_renderer_word_c;
    const int changed_to_idle = current == 0u && s_progress_previous == 1u;
    s_progress_previous = current;
    return changed_to_idle;
}
/* MenuSliderPointerHandler (FUN_00411870): the track drag plus the two
 * resource-sized arrow hit zones from the retail numeric-range record. */
// FUNCTION: LEGORR 0x00411870
void fe_slider_pointer_handle(FrontEnd* fe, FeSubmenu* sub,
                                     const FeInput* in,
                                     const TextureSurface* measure)
{
    if (!in->mdown || fe->sel < 0 || fe->sel >= sub->item_count) {
        return;
    }
    FeItem* dit = &sub->items[fe->sel];
    if (dit->kind != FE_ITEM_SLIDER && dit->kind != FE_ITEM_REALSLIDER) {
        return;
    }
    (void)measure;
    int bx = sub->base_x + dit->x + dit->slider_x;
    int by = sub->base_y + dit->y + dit->slider_y;
    if (sub->can_scroll) {
        by += sub->scroll;
    }
    if (in->mouse_x < bx || in->mouse_x > bx + 100
        || in->mouse_y < by || in->mouse_y > by + 20) {
        TextureSurface* right_anchor = dit->slider_images[3];
        TextureSurface* right_arrow = dit->slider_images[4];
        TextureSurface* left_arrow = dit->slider_images[5];
        int hit = 0;
        int changed = 0;
        if (right_arrow != NULL && left_arrow != NULL) {
            int left_w = left_arrow->width;
            int left_h = left_arrow->height;
            hit = menu_pointer_rectangle_hit_test(
                in->mouse_x, in->mouse_y, bx - left_w - 4, by,
                left_w, left_h);
            if (!hit && right_anchor != NULL) {
                hit = menu_pointer_rectangle_hit_test(
                    in->mouse_x, in->mouse_y,
                    bx + right_anchor->width + 0x68, by,
                    right_arrow->width, right_arrow->height);
                if (hit && dit->value < dit->max) {
                    ++dit->value;
                    changed = 1;
                }
            } else if (hit && dit->value > dit->min) {
                --dit->value;
                changed = 1;
            }
            if (changed) {
                fe_options_write(fe, dit);
                fe->option_changed = 1;
            }
        }
        return;
    }
    float f = (float)(in->mouse_x - bx) * 0.01f;
    if (f < 0.0f) {
        f = 0.0f;
    }
    if (f > 1.0f) {
        f = 1.0f;
    }
    int v = (int)(f * (float)(dit->max - dit->min) + 0.5f);
    /* Retail writes the scaled delta directly; unlike the keyboard path, it
     * does not add the configured minimum. */
    if (v != dit->value) {
        dit->value = v;
        fe_options_write(fe, dit);
        fe->option_changed = 1;
    }
}

/* HasActiveResource (FUN_004119d0) — see message.hpp. */
// FUNCTION: LEGORR 0x004119d0
int has_active_resource(const void* record)
{
    uint32_t handle = 0u;
    if (record != NULL) {
        __builtin_memcpy(&handle, (const char*)record + 0x38, sizeof handle);
    }
    if (record == NULL || handle == 0u) {
        return 0;
    }
    if (system_state_flags_get() == 0u) {
        return 0;
    }
    return 1;
}

/* KeyContextCheck (FUN_00411f00) returns the character mapped to a DirectInput
 * scan-code index. The retail caller supplies an index in the 256-byte
 * keyboard state domain, and the original performs no range check. */
// FUNCTION: LEGORR 0x00411F00
uint32_t input_key_context_check(int key)
{
    return s_key_context_table[key];
}

/* PercentSplitSurfaceBlit (FUN_00411f10): two ten-pixel-high source lanes
 * are split at (numerator * 100) / denominator. The original packed record
 * stores the images in the same order represented by FePercentSplitSurface;
 * optional overlays and two independently hoverable handles are layered on
 * top. The handle hit rectangles use the normal image dimensions, matching
 * MenuPointerRectangleHitTest's inclusive edges. */
// FUNCTION: LEGORR 0x00411F10
void menu_percent_split_surface_blit(void* px, int pitch, int w, int h,
                                     const FePercentSplitSurface* surface,
                                     int x, int y, int numerator,
                                     int denominator, int mouse_x,
                                     int mouse_y)
{
    if (px == NULL || surface == NULL || denominator == 0) {
        return;
    }
    const int percent = (int)(((int64_t)numerator * 100) / denominator);
    fe_blit_texture_crop(px, pitch, w, h, surface->left_fill,
                         0, percent, 10, x, y);
    fe_blit_texture_crop(px, pitch, w, h, surface->right_fill,
                         percent, 100 - percent, 10, x + percent, y);
    if (surface->left_overlay != NULL) {
        surface_blit_texture(px, pitch, w, h, surface->left_overlay, x, y);
    }
    if (surface->right_overlay != NULL) {
        surface_blit_texture(px, pitch, w, h, surface->right_overlay,
                             x + 100, y);
    }

    if (surface->left_idle != NULL && surface->left_hover != NULL) {
        const int handle_x = x - (surface->left_idle->width + 4);
        const int hot = menu_pointer_rectangle_hit_test(
            mouse_x, mouse_y, handle_x, y, surface->left_idle->width,
            surface->left_idle->height);
        surface_blit_texture(px, pitch, w, h,
                             hot ? surface->left_hover : surface->left_idle,
                             handle_x, y);
    }
    if (surface->right_idle != NULL && surface->right_hover != NULL) {
        const int right_overlay_width = surface->right_overlay != NULL
                                            ? surface->right_overlay->width : 0;
        const int handle_x = x + right_overlay_width + 104;
        const int hot = menu_pointer_rectangle_hit_test(
            mouse_x, mouse_y, handle_x, y, surface->right_idle->width,
            surface->right_idle->height);
        surface_blit_texture(px, pitch, w, h,
                             hot ? surface->right_hover : surface->right_idle,
                             handle_x, y);
    }
}

/* MenuOverlayListCount (FUN_00412170): head slot + record+0x40, next
 * slot + node+0x18, both native-size (pointer-slot deviation; see the
 * header). */
// FUNCTION: LEGORR 0x00412170
int menu_overlay_list_count(const void* menu)
{
    int count = 0;
    void* head = *(void* const*)((const char*)menu + 0x40);
    while (head != NULL) {
        ++count;
        head = *(void**)((char*)head + 0x18);
    }
    return count;
}

// FUNCTION: LEGORR 0x00412190
#if defined(LEGORRR_BACKEND_WINAPI)
__declspec(naked) bool menu_audio_tick_interval_reached(void)
{
    __asm {
        call rand
        cdq
        mov ecx, 0x190
        idiv ecx
        xor eax, eax
        test edx, edx
        sete al
        ret
    }
}
#else
bool menu_audio_tick_interval_reached(void)
{
    return level_rand_u16() % 400 == 0;
}
#endif
/* MenuOverlayFlicPlaybackController (FUN_004121b0): the retail path has one
 * active FLIC player. PlayRandom starts one configured entry on the rand()%400
 * gate; non-random hover FLIC selection needs the renderer's type-6 menu rows. */
// FUNCTION: LEGORR 0x004121b0
void fe_overlay_step(FrontEnd* fe)
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
    }
    if (sub->play_random && fe->overlay_active < 0) {
        fe_overlay_start_random(fe, sub);
        return;
    }
    if (fe->overlay_active >= 0 &&
        fe->overlay_active < sub->overlay_count) {
        FeOverlay* o = (FeOverlay*)&sub->overlays[fe->overlay_active];
        if (o->flic == NULL) {
            fe->overlay_active = -1;
            return;
        }
        flic_tick_loop(o->flic, 0);
        if (flic_eof(o->flic)) {
            flic_close(o->flic);
            o->flic = NULL;
            fe->overlay_active = -1;
        }
    }
}

/* RenderUiFrameCell @ 0x412450 selects either the supplied direct surface
 * or one of eight animation frames in the renderer-owned frame table. */
// FUNCTION: LEGORR 0x00412450
void menu_render_ui_frame_cell(void* px, int pitch, int w, int h,
                               int x, int y,
                               const TextureSurface* direct_surface,
                               const FeUiFrameCellTable* table,
                               int item_index, int frame_index)
{
    if (px == NULL || w <= 0 || h <= 0 || table == NULL ||
        table->entries == NULL || item_index < 0 || frame_index < 0 ||
        (size_t)item_index >= table->entry_count) {
        return;
    }
    if ((table->entries[item_index].flags & 1u) != 0u) {
        if (direct_surface != NULL && direct_surface->pixels != NULL &&
            direct_surface->width > 0 && direct_surface->height > 0) {
            surface_blit_texture(px, pitch, w, h, direct_surface, x, y);
        }
        return;
    }
    if (frame_index >= 8 || table->frame_surfaces == NULL) {
        return;
    }
    size_t surface_index = (size_t)item_index * 8u +
                           (size_t)frame_index;
    if (surface_index >= table->frame_surface_count) return;
    const TextureSurface* image = table->frame_surfaces[surface_index];
    if (image == NULL || image->width <= 0 || image->height <= 0 ||
        x >= 640 || -image->width >= x ||
        -image->height >= y || y >= 480) return;
    surface_blit_texture(px, pitch, w, h, image, x, y);
}

/* MenuSaveImageConfigRectBlit (FUN_004124f0): load the selected slot's
 * framebuffer thumbnail and draw it in the configured SaveImage rectangle.
 * The port's TextureSurface replaces the corpus's cached DirectDraw handle. */
// FUNCTION: LEGORR 0x004124f0
void menu_save_image_config_rect_blit(FrontEnd* fe,
                                             GameContext* game,
                                             int item_index, void* pixels,
                                             int pitch, int width, int height)
{
    if (fe == NULL || game == NULL || pixels == NULL || fe->cfg == NULL
        || item_index < 0) {
        return;
    }
    const FeSubmenu* sub = frontend_active_submenu(fe);
    const char* image = frontend_active_image(fe);
    if (sub == NULL || image == NULL || !s_icontains(image, "SaveLoad")
        || item_index >= sub->item_count) {
        return;
    }
    const FeItem* item = &sub->items[item_index];
    if (item->load_slot <= 0) {
        return;
    }

    char key[160];
    snprintf(key, sizeof(key), "%s::Menu::SaveImage::pos%d",
             util_root_name(fe->cfg), item->load_slot);
    const char* position = block_get_strptr(fe->cfg, key);
    if (position == NULL) {
        return;
    }
    const char* separator = strchr(position, ':');
    if (separator == NULL || separator == position || separator[1] == '\0') {
        return;
    }
    int x = atoi(position);
    int y = atoi(separator + 1);
    snprintf(key, sizeof(key), "%s::Menu::SaveImage::Width",
             util_root_name(fe->cfg));
    const char* width_value = block_get_strptr(fe->cfg, key);
    int draw_w = atoi(width_value != NULL ? width_value : "");
    snprintf(key, sizeof(key), "%s::Menu::SaveImage::Height",
             util_root_name(fe->cfg));
    const char* height_value = block_get_strptr(fe->cfg, key);
    int draw_h = atoi(height_value != NULL ? height_value : "");
    if (draw_w <= 0 || draw_h <= 0 || draw_w > width * 2
        || draw_h > height * 2) {
        return;
    }

    char thumb_path[320];
    save_slot_path(game, thumb_path, sizeof(thumb_path), frontend_slot_raw_index(item->load_slot),
                   "dat");
    const int cache_index = item->load_slot - 1;
    if (cache_index < 0 || cache_index >= FE_SAVE_THUMBNAIL_COUNT) {
        return;
    }
    if (fe->save_thumbnails[cache_index] == NULL) {
        fe->save_thumbnails[cache_index] =
            texture_get_or_load(NULL, thumb_path);
    }
    TextureSurface* thumb = fe->save_thumbnails[cache_index];
    if (thumb == NULL || thumb->pixels == NULL || thumb->width <= 0
        || thumb->height <= 0) {
        return;
    }
    frameloop_blit_texture_scaled(thumb, pixels, pitch, width, height,
                                  (int)x, (int)y, draw_w, draw_h);
}

/* MenuHoverStatusTextDraw (FUN_00412750) runs once per MenuTick after input
 * selection has been updated. In this single-window software frontend the
 * active FeItem is the focused row; its status is the port-side text payload
 * where the native menu's type-6 child records are not available. */
// FUNCTION: LEGORR 0x00412750
void menu_hover_status_text_draw(FrontEnd* fe, const TextureSurface* font,
                                 int w, int h, void* px, int pitch)
{
    if (fe == NULL || px == NULL || fe->menu < 0 || fe->menu >= FE_MENU_COUNT
        || h < 16) {
        return;
    }
    const FeSubmenu* sub = frontend_active_submenu(fe);
    const char* image = frontend_active_image(fe);
    const FeItem* item = frontend_active_item(fe);
    if (sub == NULL || item == NULL) return;
    const int save_load_menu = image != NULL &&
                               s_stristr(image, "SaveLoad") != NULL;

    if (save_load_menu && fe->save_text_layout.has_window) {
        const FeSaveMenuLayout* layout = &fe->save_text_layout;
        const char* prompt = fe->save_action ? layout->save : layout->load;
        char instruction[FE_SAVE_LAYOUT_TEXT_MAX];
        char progress[FE_SAVE_LAYOUT_TEXT_MAX];
        char normalized_progress[FE_SAVE_LAYOUT_TEXT_MAX];
        float rect[4];
        uint32_t header[4];
        TextBufferDrawOps saved_ops;
        void* text_font;
        void* buffer;
        int completed = 0;
        int total_levels = fe->campaign_n + fe->tutorial_n;
        int percent;
        int slot_index;
        for (slot_index = 0; slot_index < fe->slot_count; ++slot_index) {
            if (fe->slots[slot_index].id == item->load_slot) {
                completed = fe->slots[slot_index].completed;
                break;
            }
        }
        percent = total_levels > 0 ? (completed * 100) / total_levels : 0;
        if (percent < 0) percent = 0;
        if (percent > 100) percent = 100;
        snprintf(instruction, sizeof(instruction), "%s",
                 ui_text_underscores_to_spaces(prompt));
        snprintf(progress, sizeof(progress), layout->slot, percent);
        snprintf(normalized_progress, sizeof(normalized_progress), "%s",
                 ui_text_underscores_to_spaces(progress));
        rect[0] = (float)layout->window_x;
        rect[1] = (float)layout->window_y;
        rect[2] = (float)layout->window_w;
        rect[3] = (float)layout->window_h;
        memcpy(header, rect, sizeof(header));
        /* SaveMenuLayoutLoad creates this window with FONT5_HI. */
        text_font = font_atlas_record_create("Interface\\Fonts\\FONT5_HI.bmp");
        buffer = text_font != NULL
                     ? text_buffer_create((uintptr_t)text_font, header, 512)
                     : NULL;
        if (buffer != NULL) {
            text_buffer_append_format(buffer, "\n%s\n%s", instruction,
                                      normalized_progress);
            text_buffer_draw_ops_get(&saved_ops);
            text_buffer_draw_framebuffer_install(px, pitch, w, h);
            text_buffer_draw_wrapped(buffer, 0u, 1.0f, NULL);
            text_buffer_draw_ops_set(&saved_ops);
            text_buffer_destroy(buffer);
        }
        if (text_font != NULL) font_atlas_record_release(text_font);
        return;
    }
    if (item->status[0] != '\0') {
        const char* status = ui_text_underscores_to_spaces(item->status);
        TextureSurface* status_font = texture_get_or_load(
            NULL, "Interface\\Fonts\\MBriefFONT2.bmp");
        const TextureSurface* text_font = status_font != NULL ? status_font : font;
        int text_w = text_font != NULL ? fe_font_text_width(status, text_font)
                                       : fe_text_width(status);
        if (text_font != NULL)
            formatted_text_draw(text_font, px, pitch, w, h, (w - text_w) / 2,
                                h - 41, status);
        else
            fe_draw_text(px, pitch, w, h, (w - text_w) / 2, h - 41,
                         status, 150, 255, 80);
        if (status_font != NULL) texture_release(status_font);
    }

}

/* MenuOverlaySurfaceAndTextSetup (FUN_004129d0): the corpus resets the
 * save/load panel surface and its text resources when entering these menu
 * records. The frame-driven adapter composes the typed panel layout and the
 * current menu text on the software surface. */
// FUNCTION: LEGORR 0x004129d0
void menu_overlay_surface_and_text_setup(
    FrontEnd* fe, const TextureSurface* hi_font, const TextureSurface* lo_font,
    void* pixels, int pitch, int width, int height, int draw_cursor)
{
    if (fe == NULL || pixels == NULL) {
        return;
    }
    const char* image = frontend_active_image(fe);
    const FeSubmenu* active = frontend_active_submenu(fe);
    const int picker = frontend_picker_active(fe);
    if (picker) {
        frontend_render_menu_pass(fe, hi_font, lo_font, width, height,
                                  pixels, pitch, 1, draw_cursor);
    }
    if (active != NULL
        && (frontend_picker_active(fe) || fe->menu == FE_MENU_SAVE
            || (fe->menu == FE_MENU_MAIN && image != NULL
                && s_icontains(image, "SaveLoad")))) {
        const FeSaveMenuLayout* layout = frontend_picker_active(fe)
                                             ? &fe->level_text_layout
                                             : &fe->save_text_layout;
        if (layout->has_panel && layout->panel_path[0] != '\0') {
            TextureSurface* panel = texture_get_or_load(NULL,
                                                        layout->panel_path);
            if (panel != NULL) {
                frameloop_blit_texture_scaled(panel, pixels, pitch, width,
                                              height, layout->panel_x,
                                              layout->panel_y, layout->panel_w,
                                              layout->panel_h);
                texture_release(panel);
            }
        }
    }
    frontend_picker_panel_text(fe, hi_font, pixels, pitch, width, height);
    if (picker) {
        frontend_render_menu_pass(fe, hi_font, lo_font, width, height,
                                  pixels, pitch, 2, draw_cursor);
    } else {
        frontend_render_menu_pass(fe, hi_font, lo_font, width, height,
                                  pixels, pitch, 0, draw_cursor);
    }
}

// FUNCTION: LEGORR 0x00412af0
void menu_surface_blit_scaled(int percent_side)
{
    const uint32_t handle
        = s_menu_blit_record[percent_side == 0 ? 5 : 4]; /* +0x14 / +0x10 */
    if (handle == 0u) {
        return;
    }
    if ((s_menu_blit_record[13] & 1u) != 0u) { /* record +0x34 flag byte */
        crt_surface_blit_record(handle,
                                crt_float_bits(s_menu_blit_record[14]),
                                crt_float_bits(s_menu_blit_record[15]));
        return;
    }
    const float rect1[4] = { -s_menu_plane_pan[0], -s_menu_plane_pan[1],
                             640.0f, 480.0f };
    crt_surface_blit_record_rect(handle, 0.0f, 0.0f, rect1, NULL);
}
// FUNCTION: LEGORR 0x00412c00
void fe_item_pos(const FrontEnd* fe, const FeItem* it, int i,
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
    if (sub != NULL && sub->can_scroll &&
        (it->level_start || !fe_picker_submenu(sub))) {
        *y += sub->scroll;
    }
}
/* InterfaceResourceHandlesRelease (FUN_00413b50): release the five cached
 * save-thumbnail surfaces when the menu loop exits. The corpus stores them
 * in a fixed global handle band; the frontend keeps those handles per instance. */
// FUNCTION: LEGORR 0x00413b50
void interface_resource_handles_release(FrontEnd* fe)
{
    if (fe == NULL) {
        return;
    }
    for (int i = 0; i < FE_SAVE_THUMBNAIL_COUNT; ++i) {
        if (fe->save_thumbnails[i] != NULL) {
            texture_release(fe->save_thumbnails[i]);
            fe->save_thumbnails[i] = NULL;
        }
    }
    for (int mi = 0; mi < FE_MENU_COUNT; ++mi) {
        FeMenu* menu = &fe->menus[mi];
        for (int si = 0; si < menu->submenu_count; ++si) {
            FeSubmenu* sub = &menu->submenus[si];
            for (int ii = 0; ii < sub->item_count; ++ii) {
                FeItem* item = &sub->items[ii];
                for (int ri = 0; ri < 8; ++ri) {
                    if (item->slider_images[ri] != NULL) {
                        texture_release(item->slider_images[ri]);
                        item->slider_images[ri] = NULL;
                    }
                }
            }
        }
    }
}
/* MenuShow (FUN_00413e20): clear every submenu's run flag, then enter the
 * requested submenu. The native call blocks in MenuRunLoop; the portable
 * frontend leaves that selected state open for the frame loop to advance. */
// FUNCTION: LEGORR 0x00413e20
int frontend_open_submenu(FrontEnd* fe, int menu, int submenu_index)
{
    if (fe == NULL || !fe->built || menu < 0 || menu >= FE_MENU_COUNT ||
        submenu_index < 0 ||
        submenu_index >= fe->menus[menu].submenu_count) {
        return 0;
    }
    for (int i = 0; i < fe->menus[menu].submenu_count; ++i) {
        fe->menus[menu].submenus[i].run_flags = 0u;
    }
    return frontend_open_prepare(fe, menu, submenu_index);
}
// FUNCTION: LEGORR 0x00413e60
static int fe_item_kind(const char* tok)
{
    if (strcasecmp_lite(tok, s_item_token_cycle) == 0)      return FE_ITEM_CYCLE;
    if (strcasecmp_lite(tok, s_item_token_trigger) == 0)    return FE_ITEM_TRIGGER;
    if (strcasecmp_lite(tok, s_item_token_textinput) == 0)  return FE_ITEM_TEXTINPUT;
    if (strcasecmp_lite(tok, s_item_token_slider) == 0)     return FE_ITEM_SLIDER;
    if (strcasecmp_lite(tok, s_item_token_realslider) == 0) return FE_ITEM_REALSLIDER;
    return strcasecmp_lite(tok, s_item_token_next) == 0 ? FE_ITEM_NEXT : -1;
}

/* StrReplace (FUN_00413f00): replace one byte throughout a NUL-terminated
 * string and return the original pointer. */
/* C linkage for GAME-021 lego_unit.c (the frozen header declares it without). */
extern "C" char* mission_replace_char(char* text, char from, char to);
// FUNCTION: LEGORR 0x00413f00
extern "C" char* mission_replace_char(char* text, char from, char to)
{
    char* const original = text;
    while (*text != '\0') {
        if (*text == from) {
            *text = to;
        }
        ++text;
    }
    return original;
}

// FUNCTION: LEGORR 0x00413f30
int filename_extension_classify(const void* record)
{
    const char* suffix = *(const char* const*)record;
    while (*suffix != '.') ++suffix;
    if (strcmp(suffix, s_filename_avi_suffix) == 0) return 1;
    if (strcmp(suffix, s_filename_bitmap_suffix) == 0) return 2;
    if (strcmp(suffix, s_filename_lightwave_suffix) == 0) return 3;
    return 0;
}

// FUNCTION: LEGORR 0x00413f90
#if defined(LEGORRR_BACKEND_WINAPI)
__declspec(naked) void* menu_item_record_alloc(const char* name,
                                               void** list_head,
                                               uint32_t f3, uint32_t f4,
                                               uint32_t f5)
{
    __asm {
        push esi
        push edi
        push 0x20
        call malloc
        mov esi, eax
        mov ecx, 8
        xor eax, eax
        mov edi, esi
        add esp, 4
        rep stosd
        mov edi, dword ptr [esp + 0x0c]
        push 4
        push offset s_menu_null_label
        push edi
        call _strnicmp
        add esp, 0x0c
        test eax, eax
        je item_record_done
        test edi, edi
        je item_record_link
        push edi
        call campaign_level_fallback_label
        add esp, 4
        mov dword ptr [esi], eax
    item_record_link:
        push esi
        call filename_extension_classify
        mov edx, dword ptr [esp + 0x18]
        mov dword ptr [esi + 4], eax
        mov eax, dword ptr [esp + 0x14]
        mov dword ptr [esi + 0x1c], 0
        add esp, 4
        mov ecx, dword ptr [eax]
        mov dword ptr [esi + 0x0c], edx
        mov edx, dword ptr [esp + 0x1c]
        mov dword ptr [esi + 0x18], ecx
        mov ecx, dword ptr [esp + 0x18]
        mov dword ptr [esi + 0x14], edx
        mov dword ptr [esi + 0x10], ecx
        mov dword ptr [eax], esi
    item_record_done:
        mov eax, esi
        pop edi
        pop esi
        ret
    }
}
#else
void* menu_item_record_alloc(const char* name, void** list_head,
                             uint32_t f3, uint32_t f4, uint32_t f5)
{
    void* rec = engine_heap_alloc(8 * sizeof(void*));
    if (rec == NULL) return NULL;
    memset(rec, 0, 8 * sizeof(void*));
    if (name != NULL && strnicmp_lite(name, s_menu_null_label, 4) != 0) {
        char* name_copy = campaign_level_fallback_label(name);
        *(void**)((char*)rec + 0x00) = name_copy;
        *(uint32_t*)((char*)rec + 0x04) =
            (uint32_t)filename_extension_classify(rec);
        *(uint32_t*)((char*)rec + 0x0c) = f3;
        *(uint32_t*)((char*)rec + 0x10) = f4;
        *(uint32_t*)((char*)rec + 0x14) = f5;
        *(void**)((char*)rec + 0x18) =
            list_head != NULL ? *list_head : NULL;
        if (list_head != NULL) *list_head = rec;
    }
    return rec;
}
#endif
/* LoadItemStrings (FUN_00414010): the corpus uses eight dword resource
 * pointers at byte offset 0x20. This port reads the equivalent eight
 * native-width pointer slots beginning at slot 4, preserving handles on
 * 64-bit hosts. */
// FUNCTION: LEGORR 0x00414010
void load_item_strings(uint32_t record_type, const void* record,
                       void* out_resources[8])
{
    if (out_resources == NULL) {
        return;
    }
    for (int i = 0; i < 8; ++i) {
        out_resources[i] = NULL;
    }
    if (record_type != 0x10 || record == NULL) {
        return;
    }
    for (int resource_index = 0; resource_index < 8; ++resource_index) {
        const char* key = (const char*)rec_ptr(
            record, (size_t)(4 + resource_index) * sizeof(void*));
        void* resource = string_cache_lookup_or_load(key);
        out_resources[resource_index] = resource;
        if (resource != NULL) {
            resource_record_add_ref(resource);
        }
    }
}

/* DwordListAllocateAndZero (FUN_00414070) — see heap.hpp. */
// FUNCTION: LEGORR 0x00414070
void** dword_list_allocate_and_zero(uint32_t count)
{
    void** hdr = (void**)engine_heap_alloc(2 * sizeof(void*));
    if (hdr == NULL) {
        return NULL; /* deviation: the corpus strides both halves blindly */
    }
    hdr[0] = NULL;
    hdr[1] = NULL;
    void* data = engine_heap_alloc((long)(count * sizeof(void*)));
    hdr[0] = data;
    /* Corpus shape: the zero-pass sweeps `count & 0x3fffffff` slots and the
     * second pass is dead code (loop seed 0) — kept faithfully. */
    void** p = (void**)data;
    const uint32_t n = count & 0x3fffffffu;
    for (uint32_t i = 0; i != n; ++i) {
        p[i] = NULL;
    }
    if (data == NULL && count != 0) {
        engine_heap_free(hdr); /* deviation: the corpus leaves both behind */
        return NULL;
    }
    hdr[1] = (void*)(size_t)count;
    return hdr;
}

/* Per-menu config constructor (the surrounding frontend_build composes the
 * main/save/pause/options menus; submenu/item parsing is delegated below). */
// FUNCTION: LEGORR 0x004140c0
int fe_parse_menu(FrontEnd* fe, int mi, const char* name)
{
    if (fe == NULL || mi < 0 || mi >= FE_MENU_COUNT || name == NULL) {
        return 0;
    }
    FeMenu* m = &fe->menus[mi];
    m->submenu_count = 0;
    snprintf(m->name, FE_NAME_MAX, "%s", name);
    if (fe->cfg == NULL) {
        return 0;
    }
    char buf[FE_PATH_MAX * 2];
    snprintf(buf, sizeof(buf), "%s::Menu::%s::MenuCount", util_root_name(fe->cfg),
             name);
    const char* menu_count_value = block_get_strptr(fe->cfg, buf);
    int count = atoi(menu_count_value != NULL ? menu_count_value : "");
    if (count < 0) {
        count = 0;
    }
    if (count > FE_SUBMENUS_MAX) {
        count = FE_SUBMENUS_MAX;
    }
    m->submenu_count = count;
    for (int i = 0; i < count; i++) {
        fe_parse_submenu(fe, m, name, i);
    }
    return count > 0;
}

// FUNCTION: LEGORR 0x00414c90
#if defined(LEGORRR_BACKEND_WINAPI)
__declspec(naked) int menu_list_find_name(const void* list, const char* name)
{
    __asm {
        push ebx
        push esi
        push edi
        mov edi, dword ptr [esp + 0x10]
        xor esi, esi
        mov eax, dword ptr [edi + 4]
        test eax, eax
        jle menu_list_find_name_no_entries
        mov ebx, dword ptr [esp + 0x14]
    menu_list_find_name_loop:
        mov eax, dword ptr [edi]
        mov eax, dword ptr [eax + esi*4]
        test eax, eax
        je menu_list_find_name_next
        mov ecx, dword ptr [eax]
        push ebx
        push ecx
        call _strcmpi
        add esp, 8
        test eax, eax
        je menu_list_find_name_found
    menu_list_find_name_next:
        mov eax, dword ptr [edi + 4]
        inc esi
        cmp esi, eax
        jl menu_list_find_name_loop
        or eax, 0xffffffff
        pop edi
        pop esi
        pop ebx
        ret
    menu_list_find_name_found:
        mov eax, esi
        pop edi
        pop esi
        pop ebx
        ret
    menu_list_find_name_no_entries:
        pop edi
        pop esi
        or eax, 0xffffffff
        pop ebx
        ret
    }
}
#else
int menu_list_find_name(const void* list, const char* name)
{
    if (list == NULL) return -1;
    void* const* entries = *(void* const* const*)list;
    const int count = *(const int*)((const char*)list + sizeof(void*));
    for (int i = 0; i < count; ++i) {
        void* entry = entries[i];
        if (entry != NULL
            && strcasecmp_lite(*(const char**)entry, name) == 0) {
            return i;
        }
    }
    return -1;
}
#endif

// FUNCTION: LEGORR 0x00414ce0
int movie_avis_enabled(BlockFile* cfg)
{
    int dont_play_avis = 0;
    if (cfg != NULL) {
        dont_play_avis = block_get_bool_path(cfg, "Lego*::Main::DontPlayAvis");
    }
    /* AvisEnabled @ 0x414ce0 also suppresses playback in app modes >= 3. */
    return dont_play_avis != 1 && engine_app_mode_get() < 3;
}

/* GameSpeedSelect (FUN_00414de0): the ledger name is misleading; this is
 * the SFX slider callback and simply forwards its index. */
// FUNCTION: LEGORR 0x00414de0
void audio_menu_sfx_volume_set(int index)
{
    audio_sound_volume_index_set(index);
}

/* VolumeSet (FUN_00414df0): the corpus sends param_1 / 9 to both mixer
 * channels. AudioPlayer exposes one master output gain, so apply the same
 * normalized value once; audio_set_volume provides the device-side clamp. */
// FUNCTION: LEGORR 0x00414df0
#if defined(LEGORRR_BACKEND_WINAPI)
__declspec(naked) void audio_menu_volume_set(int value)
{
    __asm {
        fild dword ptr [esp + 4]
        fmul dword ptr [s_menu_volume_scale]
        fstp dword ptr [esp + 4]
        mov eax, dword ptr [esp + 4]
        push eax
        push eax
        call music_mixer_volume_set
        add esp, 8
        ret
    }
}
#else
void audio_menu_volume_set(int value)
{
    audio_set_volume(s_active_audio_player, (float)value * 0.11111111f);
}
#endif

/* OptionMusicDefault (FUN_00414e10) queries a WinMM audio-CD mixer source
 * and returns the rounded mean channel level on the original 0..9 scale,
 * or zero when no such source is available. Backends without a hardware CD
 * mixer return unavailable rather than substituting streamed-output gain. */
// FUNCTION: LEGORR 0x00414e10
int option_music_default(void)
{
    float left;
    float right;
    if (!audio_cd_mixer_volume_get(&left, &right)) return 0;
    return crt_float_to_long((left + right) * 4.5f);
}

/* ScrollToSliderIndexStore (FUN_00414f90) stores the same speed band in the
 * context-owned option word; the corpus writes its module global. */
// FUNCTION: LEGORR 0x414f90
void options_scroll_index_store(struct GameContext* ctx)
{
    if (ctx == NULL) {
        return; /* deviation: the corpus stores the global blindly */
    }
    const double v = (double)scroll_scale_peek();
    if (v < 0.6666667) {
        ctx->options.game_speed = 0;
        return;
    }
    if (v < 1.0) {
        ctx->options.game_speed = 1;
        return;
    }
    if (v < 1.3333334) {
        ctx->options.game_speed = 2;
        return;
    }
    if (v < 1.6666666) {
        ctx->options.game_speed = 3;
        return;
    }
    ctx->options.game_speed = 4;
    if (2.0 <= v) {
        ctx->options.game_speed = 5;
    }
}
/* --- the scroll speed index band (see options.hpp) ----------------------- */
// FUNCTION: LEGORR 0x00415030
int scroll_to_slider_index(void)
{
    const double v = (double)scroll_scale_peek(); /* corpus reads the _DAT
                                                   * alias of its scale word */
    if (v < 0.6666667) {
        return 0;
    }
    if (v < 1.0) {
        return 1;
    }
    if (v < 1.3333334) {
        return 2;
    }
    if (v < 1.6666666) {
        return 3;
    }
    return (2.0 <= v) ? 5 : 4; /* corpus shape: NaN skips both tests, 4 */
}

/* SaveMenuRefresh @ 0x004150b0 takes the old context word and new slot; the
 * first word is unused by the recovered body, while save context is global. */
// FUNCTION: LEGORR 0x004150b0
void save_menu_refresh_target(uint32_t unused, int slot)
{
    void* rec;
    void* base = NULL;
    uint32_t flag = 1u;
    (void)unused;
    save_slot_set_target(slot);
    rec = sav_build_ctx_target();
    if (rec != NULL) {
        flag = 0u;
        base = sav_current_slot_record_window(); /* corpus: rec + 0x34 */
    }
    sav_tree_load(sav_tree_root_peek(0), base, sav_menu_window_peek(0),
                  0u, flag, NULL);
    sav_tree_load(sav_tree_root_peek(1), NULL, sav_menu_window_peek(1),
                  0u, 0u, NULL);
}

/* FrontEndStartLevel (FUN_00415220): prepare transition state and record the
 * selected level; the frame loop owns the pending-level teardown and load. */
// FUNCTION: LEGORR 0x00415220
FeEvent frontend_start_level(FrontEnd* fe, const char* level)
{
    if (fe == NULL || level == NULL || level[0] == '\0') {
        return FE_EVT_NONE;
    }
    snprintf(fe->pending_level, FE_LEVEL_NAME_MAX, "%s", level);
    /* The reference clears transition state before it tears down the active
     * level and begins LevelLoad. The frame loop owns that load/teardown. */
    unported_ledger_stubs::ledger_stub_00425c30_TransitionControl(0, 0u);
    frontend_flow_word_set(4, 0);
    frontend_close(fe);
    return FE_EVT_START_LEVEL;
}

// FUNCTION: LEGORR 0x00415360
void scroll_slider_refresh(struct GameContext* ctx)
{
    if (ctx != NULL) {
        ctx->options.game_speed = scroll_to_slider_index();
    }
}

/* OverlayMenuGate (FUN_00415370): keep the native open transition separate
 * from steady-state ticking. Entry slots 0 and 2 carry the action latch that
 * MenuTick sets; the adapter returns and clears those latches in the same
 * mode-specific order as the original routine. */
// FUNCTION: LEGORR 0x00415370
int overlay_menu_gate(FeOverlayMenuGateState* state, int mode,
                      FeOverlayMenuScrollRefresh refresh_scroll,
                      FeOverlayMenuTick menu_tick, void* context)
{
    if (refresh_scroll != NULL) {
        refresh_scroll(context);
    }
    if (state == NULL || (mode != 0 && mode != 1)) {
        return mode != 0 && mode != 1;
    }

    FeOverlayMenuSet* menu = &state->menus[mode];
    if (state->active_item != 0 && state->options_flag == 0 &&
        state->level_start_flag == 0 && state->action_flag == 0 &&
        state->cached_menu == mode && state->blocked == 0) {
        if (menu_tick != NULL) {
            state->active_item = menu_tick(context, state->active_item, 0);
        }
        if (mode == 1) {
            const int activated = menu->entries[0].action_state;
            menu->entries[0].action_state = 0;
            return activated == 1;
        }
        const int activated = menu->entries[0].action_state == 1 ||
                              menu->entries[2].action_state == 1;
        menu->entries[0].action_state = 0;
        menu->entries[2].action_state = 0;
        return activated;
    }

    state->level_start_flag = 0;
    state->active_item = menu->entries[0].handle;
    state->cached_menu = mode;
    if (state->action_flag != 0) {
        if (menu->entries[0].handle != 0) {
            menu->entries[0].action_state = 1;
        }
        if (menu->entries[2].handle != 0) {
            menu->entries[2].action_state = 1;
        }
        if (mode == 0) {
            state->action_flag = 0;
            return 0;
        }
        return 0;
    }
    if (state->options_flag == 1) {
        return 0;
    }

    if (state->blocked == 0 && menu_tick != NULL) {
        state->active_item = menu_tick(context, state->active_item, 0);
    }
    if (mode == 1) {
        const int activated = menu->entries[0].action_state;
        menu->entries[0].action_state = 0;
        return activated == 1;
    }
    const int activated = menu->entries[0].action_state == 1 ||
                          menu->entries[2].action_state == 1;
    menu->entries[0].action_state = 0;
    menu->entries[2].action_state = 0;
    return activated;
}
// FUNCTION: LEGORR 0x004154b0
int fe_build_chain(BlockFile* cfg, const char* start_key,
                          char names[][FE_LEVEL_NAME_MAX],
                          char labels[][FE_NAME_MAX], int max)
{
    if (cfg == NULL) {
        return 0;
    }

    const char* root = util_root_name(cfg);
    char path[FE_PATH_MAX * 2];
    snprintf(path, sizeof(path), "%s::Main::%s", root, start_key);
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

/* Partial MoviePlayNamed (FUN_00415910): extracts and normalizes the named
 * Lego*::Main path; opening and playback are dispatched by the frame loop. */
// FUNCTION: LEGORR 0x00415910
int movie_key_path(BlockFile* cfg, const char* key, char* out, size_t sz)
{
    if (cfg == NULL || key == NULL || out == NULL || sz == 0) {
        return 0;
    }
    char path[256];
    snprintf(path, sizeof(path), "Lego*::Main::%s", key);
    const char* v = block_get_strptr(cfg, path);
    if (v == NULL || v[0] == '\0') {
        return 0;
    }
    /* normalize '\' -> '/' (the config spells `avi\intro.avi`) */
    size_t i = 0;
    for (; i < sz - 1 && v[i] != '\0'; ++i) {
        out[i] = (v[i] == '\\') ? '/' : v[i];
    }
    out[i] = '\0';
    return 1;
}

/* Partial MoviePlayLevelVideo (FUN_00415990): reads and normalizes the
 * level's Video config path; opening and playback happen in the caller. */
// FUNCTION: LEGORR 0x00415990
int movie_level_video_path(BlockFile* cfg, const char* level,
                           char* out, size_t sz)
{
    if (cfg == NULL || level == NULL || out == NULL || sz == 0) {
        return 0;
    }
    /* shipped layout: root { Levels { <level> { Video … } } } */
    char path[256];
    snprintf(path, sizeof(path), "Lego*::Levels::%s::Video", level);
    const char* v = block_get_strptr(cfg, path);
    if (v == NULL) {
        /* fixture/legacy flat shape */
        snprintf(path, sizeof(path), "Lego*::%s::Video", level);
        v = block_get_strptr(cfg, path);
    }
    if (v == NULL || v[0] == '\0') {
        return 0;
    }
    size_t i = 0;
    for (; i < sz - 1 && v[i] != '\0'; ++i) {
        out[i] = (v[i] == '\\') ? '/' : v[i];
    }
    out[i] = '\0';
    return 1;
}

/* Partial SaveMenuBuild (FUN_00415a10): populate save/load menus with
 * runtime slot rows; the corpus also resolves config keys and slot summaries. */
// FUNCTION: LEGORR 0x00415a10
int frontend_slot_raw_index(int ordinal)
{
    return ordinal >= 1 && ordinal < SAVE_SLOTS ? ordinal - 1 : -1;
}

/* SaveSlotSelectionClear @ 0x00415cf0 is the same process-global reset. */
// FUNCTION: LEGORR 0x00415cf0
void save_slot_selection_clear_target(void)
{
    s_save_current_slot = -1;
}

/* SaveMenuLayoutLoad (FUN_00415d00): resolve the seven named resource paths
 * plus the Window rectangle and Panel resource tuple. The corpus writes these
 * into fixed offsets in its resource record; FeSaveMenuLayout keeps the same
 * data in bounded, typed fields. LoadMenuBuild zeroes the record before these
 * calls; this helper only overwrites fields whose config entries are present. */
// FUNCTION: LEGORR 0x00415d00
int save_menu_layout_load(FrontEnd* fe, const char* group,
                          FeSaveMenuLayout* layout)
{
    if (fe == NULL || fe->cfg == NULL || group == NULL || layout == NULL) {
        return 0;
    }
    static const char* const keys[7] = {
        "Load", "Save", "Slot", "SaveSel", "LoadSel", "Level", "Tutorial"
    };
    char* const fields[7] = {
        layout->load, layout->save, layout->slot, layout->save_sel,
        layout->load_sel, layout->level, layout->tutorial
    };
    char path[FE_PATH_MAX * 2];
    int found = 0;
    for (int i = 0; i < 7; ++i) {
        snprintf(path, sizeof(path), "%s::%s::%s", util_root_name(fe->cfg),
                 group, keys[i]);
        const char* value = block_get_strptr(fe->cfg, path);
        if (value != NULL) {
            snprintf(fields[i], FE_SAVE_LAYOUT_TEXT_MAX, "%s", value);
            found = 1;
        }
    }

    snprintf(path, sizeof(path), "%s::%s::Window", util_root_name(fe->cfg),
             group);
    int window[4];
    if (fe_parse_int_tuple_bar(block_get_strptr(fe->cfg, path), window, 4)) {
        layout->window_x = window[0];
        layout->window_y = window[1];
        layout->window_w = window[2];
        layout->window_h = window[3];
        layout->has_window = 1;
        found = 1;
    }

    snprintf(path, sizeof(path), "%s::%s::Panel", util_root_name(fe->cfg),
             group);
    const char* panel = block_get_strptr(fe->cfg, path);
    if (panel != NULL) {
        const char* separator = strchr(panel, '|');
        const char* end = panel;
        int rect[4];
        int i;
        if (separator != NULL && separator != panel) {
            size_t name_len = (size_t)(separator - panel);
            if (name_len < sizeof(layout->panel_path)) {
                for (i = 0; i < 4; ++i) {
                    end = strchr(end, '|');
                    if (end == NULL) {
                        break;
                    }
                    ++end;
                }
                if (i == 4 && end != NULL && strchr(end, '|') == NULL
                    && fe_parse_int_tuple_bar(separator + 1, rect, 4)) {
                    memcpy(layout->panel_path, panel, name_len);
                    layout->panel_path[name_len] = '\0';
                    /* The reference loads the configured bitmap directly,
                     * then applies a black source color key before storing
                     * its four panel coordinates. */
                    /* The frame renderer's SaveLoad panel path uses the
                     * shared texture cache; retain that same cached surface
                     * here so layout loading and drawing share one resource. */
                    layout->panel_surface = texture_get_or_load(
                        NULL, layout->panel_path);
                    if (layout->panel_surface != NULL) {
                        texture_source_color_key_set(layout->panel_surface,
                                                     0u, 0u);
                        layout->panel_x = rect[0];
                        layout->panel_y = rect[1];
                        layout->panel_w = rect[2];
                        layout->panel_h = rect[3];
                        layout->has_panel = 1;
                    }
                    found = 1;
                }
            }
        }
    }
    return found;
}

// FUNCTION: LEGORR 0x00416150
int sound_stream_play_level(int level)
{
    /* StreamPlayLevel @ 0x416150: the Stream_LevelName_Level%d alias at
     * 0x4a25c8, submitted through the global cooldown when it resolves. */
    char name[128];
    snprintf(name, sizeof(name), "Stream_LevelName_Level%d", level);
    int alias = 0;
    if (sound_alias_lookup(name, &alias) != 0) {
        return sound_submit_with_global_cooldown(alias, NULL);
    }
    return 0;
}

// FUNCTION: LEGORR 0x004161a0
int sound_stream_play_tutorial(int level)
{
    /* StreamPlayTutorial @ 0x4161a0. */
    char name[128];
    snprintf(name, sizeof(name), "Stream_LevelName_TLevel%d", level);
    int alias = 0;
    if (sound_alias_lookup(name, &alias) != 0) {
        return sound_submit_with_global_cooldown(alias, NULL);
    }
    return 0;
}

/* OptionsWriteToSlot @ 0x00416910 reads the current game and save contexts. */
// FUNCTION: LEGORR 0x00416910
void options_write_to_slot_target(void)
{
    GameContext* ctx = (GameContext*)sav_build_ctx_target();
    if (ctx == NULL) return;
    /* The host-width sidecar carries retail's SFX/speed/brightness words. */
    s_options_slot_window[2] = (uint32_t)ctx->options.sfx_volume;
    s_options_slot_window[3] = (uint32_t)ctx->options.game_speed;
    s_options_slot_window[4] = (uint32_t)ctx->options.brightness;
    ctx->saved_sfx_volume = ctx->options.sfx_volume;
    ctx->saved_scroll_index = ctx->options.game_speed;
    ctx->saved_brightness = ctx->options.brightness;
}
/* OptionsLoadOrDefault @ 0x00416940 takes (load_options, reset_frontend).
 * The frontend supplies the separate DAT_00557028 selection gate. */
// FUNCTION: LEGORR 0x00416940
void options_load_or_default_target(int load_options, int reset_frontend)
{
    GameContext* ctx = save_current_context_get();
    if (reset_frontend != 0) {
        frontend_flow_word_set(0, 0);
        frontend_flow_word_set(1, -1);
        frontend_flow_word_set(2, -1);
        frontend_flow_word_set(3, 0);
    }
    if (load_options != 0 && ctx != NULL) {
        if (s_options_frontend_selection == -1 ||
            sav_build_ctx_target() == NULL ||
            ctx->active_slot != ctx->save_slot) {
            options_load_corpus_defaults(&ctx->options);
        } else {
            ctx->options.sfx_volume = ctx->saved_sfx_volume;
            ctx->options.game_speed = ctx->saved_scroll_index;
            ctx->options.brightness = ctx->saved_brightness;
        }
    }
}

/* FrontEndStateChange (FUN_004169c0): prepare state before MenuEnter chooses
 * a menu. The live option and save-event words are applied here; the
 * corpus's tree walks still need the EngineContext passed to its callbacks. */
// FUNCTION: LEGORR 0x004169c0
int frontend_state_change(FrontEnd* fe, int state)
{
    if (fe == NULL || !fe->built) {
        return 0;
    }
    if (state < FE_STATE_MAIN || state > FE_STATE_SAVE_REWARD) {
        return 0;
    }
    /* FrontEndStateChange calls OptionsLoadOrDefault for every valid flow.
     * Main/new-game/load retain the active save context's option triple;
     * options/quit only clear the frontend selection words. */
    options_frontend_selection_set(state == FE_STATE_MAIN
                                       ? -1 : fe->selected_save_slot);
    options_load_or_default(save_current_context_get(),
        state <= FE_STATE_TRAINING ? 1 : 0, 1);
    /* Save flow case 4 raises DAT_00557500 only when there is no active
     * save-slot context and the save-state event flag is already set. */
    if (state == FE_STATE_SAVE_REWARD && sav_build_ctx_target() == NULL &&
        save_state_event_context_peek() != 0u) {
        current_selection_context_set((void*)(uintptr_t)1u);
    }
    /* Every switch arm ends by raising the save-state event context. */
    save_state_event_context_set(1u);
    fe->flow_state = state;
    frontend_options_sync(fe);
    return 1;
}

/* MenuEnter (FUN_00416c80): FrontEndStateChange prepares the flow, then the
 * native routine selects submenu 0/1/2 in MainMenuFull for modes 0/1/2;
 * mode 4 selects submenu 0 in the separate save-menu set. Mode 3 performs
 * cleanup without showing a menu. */
// FUNCTION: LEGORR 0x00416c80
int menu_enter(FrontEnd* fe, int mode)
{
    if (!frontend_state_change(fe, mode)) {
        return 0;
    }
    if (mode == FE_STATE_OPTIONS) {
        frontend_close(fe);
        return 1;
    }

    const int menu = mode == FE_STATE_SAVE_REWARD ? FE_MENU_SAVE : FE_MENU_MAIN;
    const int submenu = mode == FE_STATE_SAVE_REWARD ? 0 : mode;
    if (submenu >= fe->menus[menu].submenu_count ||
        !frontend_open_submenu(fe, menu, submenu)) {
        return 0;
    }
    if (mode == FE_STATE_SAVE_REWARD) {
        fe->save_action = 1;
    }
    return 1;
}
// FUNCTION: LEGORR 0x00416d00
int frontend_enabled(BlockFile* cfg)
{
    if (cfg == NULL) {
        return 0;
    }
    /* FrontEndActive @ 0x416d00: the cfg flag (loader-mode <3 is Win32). */
    return block_get_bool_path(cfg, "Lego*::Main::FrontEnd") == 1;
}

/* --- the front-end flow words (see frontend.hpp) ----------------------------
 * Each word is its own zero-initialized static; the winapi build annotates
 * them individually at their corpus DAT addresses, whose zero-filled .data
 * tail makes that the faithful initial state (finding 23/438). */
// FUNCTION: LEGORR 0x00416d50
int save_slot_open(void)
{
    return FE_FLOW_SAVE_SLOT != -1;
}

// FUNCTION: LEGORR 0x00416d60
int is_in_game(void)
{
    return FE_FLOW_INGAME_SLOT != -1;
}
/* Partial FrontEndSelectedAction (FUN_00416d70): resolve the selected
 * campaign/tutorial picker row to its level key. Save-slot list resolution
 * remains in the frame-loop load path. */
// FUNCTION: LEGORR 0x00416d70
const char* fe_selected_level_action(const FrontEnd* fe)
{
    const FeItem* it = frontend_active_item(fe);
    return it->level_start ? it->target : NULL;
}

// FUNCTION: LEGORR 0x00416dd0
int quit_menu_probe(void)
{
    return FE_FLOW_QUIT_WORD;
}

// FUNCTION: LEGORR 0x00416de0
int level_start_flag(void)
{
    return FE_FLOW_LEVEL_START != 0;
}

// FUNCTION: LEGORR 0x00416df0
int menu_options_flag(void)
{
    return FE_FLOW_MENU_OPTIONS != 0;
}

// FUNCTION: LEGORR 0x00416e00
int* active_object_list_word(void)
{
    return is_in_game() != 0 ? &s_active_object_ingame : &s_active_object_outgame;
}

#if defined(_MSC_VER) && defined(_M_IX86)
/* The mapped ListFindName body follows the unchecked retail list walk. */
// FUNCTION: LEGORR 0x00416e20
extern "C" __declspec(naked) int front_level_set_index_of(
    const SavListWindow* win, const char* name)
{
    __asm {
        push ebx
        push esi
        push edi
        mov edi, dword ptr [esp + 10h]
        xor esi, esi
        cmp dword ptr [edi], 0
        jle list_find_name_not_found
        mov ebx, dword ptr [esp + 14h]
    list_find_name_loop:
        mov eax, dword ptr [edi + 4]
        mov ecx, dword ptr [eax + esi*4]
        push ecx
        push ebx
        call _strcmpi
        add esp, 8
        test eax, eax
        je list_find_name_found
        mov eax, dword ptr [edi]
        inc esi
        cmp esi, eax
        jl list_find_name_loop
        or eax, 0ffffffffh
        pop edi
        pop esi
        pop ebx
        ret
    list_find_name_found:
        mov eax, esi
        pop edi
        pop esi
        pop ebx
        ret
    list_find_name_not_found:
        pop edi
        pop esi
        or eax, 0ffffffffh
        pop ebx
        ret
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
    }
}
#else
// PORT-ONLY: adapter or forwarder for the port contract
extern "C" int front_level_set_index_of(const SavListWindow* win, const char* name)
{
    int i;
    if (win->count <= 0) return -1;
    for (i = 0; i < win->count; ++i) {
        if (strcasecmp_lite(name, (const char*)win->entries[i]) == 0)
            return i;
    }
    return -1;
}
#endif

/* SavClearVisited (FUN_00416e70): clear each installed record marker and its
 * paired per-index word, out-of-game band first and in-game band second. The
 * two active counts are set from the same root sizes used to allocate these
 * sidecar arrays. */
// FUNCTION: LEGORR 0x00416e70
void sav_clear_visited(void)
{
    int iVar2;

    iVar2 = 0;
    if (0 < s_active_object_outgame) {
        do {
            void* record = s_sav_visited_outgame_items[iVar2];
            if (record != NULL) {
                ((uint32_t*)record)[4] = 0u;
                s_sav_visited_outgame_words[iVar2] = 0u;
            }
            iVar2 = iVar2 + 1;
        } while (iVar2 < s_active_object_outgame);
    }
    iVar2 = 0;
    if (0 < s_active_object_ingame) {
        do {
            void* record = s_sav_visited_ingame_items[iVar2];
            if (record != NULL) {
                ((uint32_t*)record)[4] = 0u;
                s_sav_visited_ingame_words[iVar2] = 0u;
            }
            iVar2 = iVar2 + 1;
        } while (iVar2 < s_active_object_ingame);
    }
}
// FUNCTION: LEGORR 0x00416ed0
void list_add_name(SavListWindow* win, const void* wide_key, void* value)
{
    if (win == NULL || wide_key == NULL) {
        return; /* tested port guard; the corpus dereferences both */
    }
    const int count = win->count;
    if (count <= 0) {
        return;
    }
    void** entry = win->entries;
    const unsigned char* probe = (const unsigned char*)wide_key;
    int index = 0;
    while (index < count) {
        const unsigned char* key = (const unsigned char*)*entry;
        const unsigned char* other = probe;
        int compare;
        for (;;) {
            unsigned char ch = *key;
            if (ch != *other) {
                compare = ch < *other ? -1 : 1;
                break;
            }
            if (ch == 0u) {
                compare = 0;
                break;
            }
            ch = key[1];
            if (ch != other[1]) {
                compare = ch < other[1] ? -1 : 1;
                break;
            }
            key += 2;
            other += 2;
            if (ch == 0u) {
                compare = 0;
                break;
            }
        }
        if (compare == 0) {
            win->values_b[index] = value; /* corpus: blind store through +16 */
            return;
        }
        ++index;
        ++entry;
    }
}

// FUNCTION: LEGORR 0x00416f40
void* list_has_item(const SavListWindow* win, const void* wide_key)
{
    if (win == NULL || wide_key == NULL) {
        return NULL;
    }
    const int count = win->count;
    if (count <= 0) {
        return NULL;
    }
    void* const* entry = win->entries;
    const unsigned char* probe = (const unsigned char*)wide_key;
    int index = 0;
    while (index < count) {
        const unsigned char* key = (const unsigned char*)*entry;
        const unsigned char* other = probe;
        int compare;
        for (;;) {
            unsigned char ch = *key;
            if (ch != *other) {
                compare = ch < *other ? -1 : 1;
                break;
            }
            if (ch == 0u) {
                compare = 0;
                break;
            }
            ch = key[1];
            if (ch != other[1]) {
                compare = ch < other[1] ? -1 : 1;
                break;
            }
            key += 2;
            other += 2;
            if (ch == 0u) {
                compare = 0;
                break;
            }
        }
        if (compare == 0) {
            return win->values_b[index];
        }
        ++index;
        ++entry;
    }
    return NULL;
}

// FUNCTION: LEGORR 0x00416fb0
void list_set_node(SavListWindow* win, const void* wide_key, void* value)
{
    if (win == NULL || wide_key == NULL) {
        return; /* tested port guard; the corpus dereferences both */
    }
    const int count = win->count;
    if (count <= 0) {
        return;
    }
    void** entry = win->entries;
    const unsigned char* probe = (const unsigned char*)wide_key;
    int index = 0;
    while (index < count) {
        const unsigned char* key = (const unsigned char*)*entry;
        const unsigned char* other = probe;
        int compare;
        for (;;) {
            unsigned char ch = *key;
            if (ch != *other) {
                compare = ch < *other ? -1 : 1;
                break;
            }
            if (ch == 0u) {
                compare = 0;
                break;
            }
            ch = key[1];
            if (ch != other[1]) {
                compare = ch < other[1] ? -1 : 1;
                break;
            }
            key += 2;
            other += 2;
            if (ch == 0u) {
                compare = 0;
                break;
            }
        }
        if (compare == 0) {
            win->values_a[index] = value; /* corpus: blind store through +12 */
            return;
        }
        ++index;
        ++entry;
    }
}

// FUNCTION: LEGORR 0x00417020
void* list_get_node(const SavListWindow* win, const void* wide_key)
{
    if (win == NULL || wide_key == NULL) {
        return NULL;
    }
    const int count = win->count;
    if (count <= 0) {
        return NULL;
    }
    void* const* entry = win->entries;
    const unsigned char* probe = (const unsigned char*)wide_key;
    int index = 0;
    while (index < count) {
        const unsigned char* key = (const unsigned char*)*entry;
        const unsigned char* other = probe;
        int compare;
        for (;;) {
            unsigned char ch = *key;
            if (ch != *other) {
                compare = ch < *other ? -1 : 1;
                break;
            }
            if (ch == 0u) {
                compare = 0;
                break;
            }
            ch = key[1];
            if (ch != other[1]) {
                compare = ch < other[1] ? -1 : 1;
                break;
            }
            key += 2;
            other += 2;
            if (ch == 0u) {
                compare = 0;
                break;
            }
        }
        if (compare == 0) {
            return win->values_a[index];
        }
        ++index;
        ++entry;
    }
    return NULL;
}

/* LevelNodeGet (FUN_00417090): recursively materialize the LevelLinks tree.
 * The native routine gets its config from globals and caches both an
 * in-progress marker and the node in the two registry value arrays; this
 * adapter takes the config explicitly and uses the same cache slots. */
// FUNCTION: LEGORR 0x00417090
SavTreeNode* level_node_get(SavListWindow* win, BlockFile* cfg,
                            const char* level_name)
{
    if (win == NULL || cfg == NULL || level_name == NULL) {
        return NULL;
    }
    if (win->values_a == NULL || win->values_b == NULL) {
        return NULL;
    }
    if (list_has_item(win, level_name) != NULL) {
        return (SavTreeNode*)list_get_node(win, level_name);
    }

    SavTreeNode* node = (SavTreeNode*)engine_heap_alloc(sizeof(*node));
    list_add_name(win, level_name, (void*)1); /* break LevelLinks cycles */
    list_set_node(win, level_name, node);
    if (node == NULL) {
        return NULL;
    }
    memset(node, 0, sizeof(*node));
    {
        const int index = front_level_set_index_of(win, level_name);
        if (index < 0) {
            engine_heap_free(node);
            return NULL;
        }
        node->window = (uint32_t)index;
    }

    const char* group = strstr(level_name, "::");
    group = group != NULL ? group + 2 : level_name;
    char path[256];
    const char* root = util_root_name(cfg);
    snprintf(path, sizeof(path), "%s::Levels::%s::LevelLinks", root, group);
    const char* links = block_get_strptr(cfg, path);
    if (links == NULL) {
        snprintf(path, sizeof(path), "%s::%s::LevelLinks", root, group);
        links = block_get_strptr(cfg, path);
    }
    if (links == NULL) {
        return node;
    }

    char names[15][FE_LEVEL_NAME_MAX];
    uint32_t count = 0;
    const char* p = links;
    while (*p != '\0' && count < 15) {
        p += strspn(p, ", \t");
        if (*p == '\0') {
            break;
        }
        const char* end = p + strcspn(p, ",");
        size_t len = (size_t)(end - p);
        while (len > 0 && (p[len - 1] == ' ' || p[len - 1] == '\t')) {
            --len;
        }
        if (len >= FE_LEVEL_NAME_MAX) {
            len = FE_LEVEL_NAME_MAX - 1;
        }
        if (len != 0) {
            memcpy(names[count], p, len);
            names[count][len] = '\0';
            ++count;
        }
        p = *end == ',' ? end + 1 : end;
    }
    if (count == 0) {
        list_set_node(win, level_name, NULL);
        engine_heap_free(node);
        return NULL;
    }

    SavTreeNode** children = (SavTreeNode**)engine_heap_alloc(
        (long)(count * sizeof(*children)));
    if (children == NULL) {
        list_set_node(win, level_name, NULL);
        list_add_name(win, level_name, NULL);
        engine_heap_free(node);
        return NULL;
    }
    memset(children, 0, count * sizeof(*children));
    node->children = children;
    node->count = count;
    for (uint32_t i = 0; i < count; ++i) {
        children[i] = level_node_get(win, cfg, names[i]);
    }
    return node;
}

/* SavTreeWalk (FUN_004171c0) — see savegame.hpp. */
// FUNCTION: LEGORR 0x004171c0
/* On x86, retain the corpus's push-user/push-node/flag-store/call order.
 * The callback-null guard is kept because test_savegame.cpp pins it. */
#if defined(LEGORRR_BACKEND_WINAPI)
__declspec(naked) int sav_tree_walk(void* node, sav_tree_visit visit,
                                    void* user)
{
    __asm {
        push ebx
        push ebp
        push esi
        mov esi, dword ptr [esp + 10h]
        test esi, esi
        push edi
        je sav_tree_walk_empty
        mov eax, dword ptr [esi + 10h]
        test eax, eax
        jne sav_tree_walk_empty
        mov ebx, dword ptr [esp + 1ch]
        mov ebp, dword ptr [esp + 18h]
        test ebp, ebp
        je sav_tree_walk_no_callback
        push ebx
        push esi
        mov dword ptr [esi + 10h], 1
        call ebp
        add esp, 8
        test eax, eax
        je sav_tree_walk_children
        mov eax, 1
        pop edi
        pop esi
        pop ebp
        pop ebx
        ret
    sav_tree_walk_no_callback:
        mov dword ptr [esi + 10h], 1
    sav_tree_walk_children:
        mov eax, dword ptr [esi + 4]
        test eax, eax
        je sav_tree_walk_empty
        mov eax, dword ptr [esi + 8]
        xor edi, edi
        test eax, eax
        jbe sav_tree_walk_empty
    sav_tree_walk_loop:
        mov eax, dword ptr [esi + 4]
        push ebx
        push ebp
        mov ecx, dword ptr [eax + edi * 4]
        push ecx
        call sav_tree_walk
        add esp, 0ch
        test eax, eax
        jne sav_tree_walk_child_found
        mov eax, dword ptr [esi + 8]
        inc edi
        cmp edi, eax
        jb sav_tree_walk_loop
        xor eax, eax
        pop edi
        pop esi
        pop ebp
        pop ebx
        ret
    sav_tree_walk_child_found:
        mov eax, 1
        pop edi
        pop esi
        pop ebp
        pop ebx
        ret
    sav_tree_walk_empty:
        pop edi
        pop esi
        pop ebp
        xor eax, eax
        pop ebx
        ret
    }
}
#else
int sav_tree_walk(void* node, sav_tree_visit visit, void* user)
{
    SavTreeNode* n = (SavTreeNode*)node;
    if (n == NULL) {
        return 0;
    }
    /* SavTreeNode preserves the corpus's reserved +0x0c word so the walk
     * flag remains at +0x10 on x86; the pointer slot widens on the host. */
    if (n->visited != 0u) {
        return 0;
    }
    int visit_result = 0;
    if (visit != NULL) {
        n->visited = 1u;
        visit_result = visit(node, user);
    } else {
        n->visited = 1u; /* test-pinned guard; corpus calls callback blind */
    }
    if (visit_result != 0) {
        return 1;
    }
    if (n->children == NULL) {
        return 0;
    }
    /* the corpus re-reads the children slot every pass (widened slots) */
    for (uint32_t i = 0; i < n->count; ++i) {
        if (sav_tree_walk(((void**)n->children)[i], visit, user) != 0) {
            return 1;
        }
    }
    return 0;
}
#endif

// FUNCTION: LEGORR 0x00417250
static int sav_tree_nth_visit(void* node, void* user)
{
    SavTreeNthState* st = (SavTreeNthState*)user;
    if (st->want == st->seen) {
        st->found_at = st->seen;
        st->found = node;
        return 1;
    }
    st->seen = st->seen + 1u;
    return 0;
}

// FUNCTION: LEGORR 0x00417280
void* registry_value_lookup(void* root, uint32_t index)
{
    SavTreeNthState st = { NULL, index, 0u, 0u };
    if (sav_tree_walk(root, sav_tree_nth_visit, &st) != 0) {
        sav_clear_visited(); /* corpus call site 1: the hit path */
        return st.found;
    }
    sav_clear_visited(); /* corpus call site 2: the miss path */
    return NULL;
}

// FUNCTION: LEGORR 0x004172d0
uint32_t registry_value_index_lookup_or_not_found(void* root, uint32_t index)
{
    void* node = registry_value_lookup(root, index);
    if (node != NULL) {
        uint32_t word = 0u;
        __builtin_memcpy(&word, node, sizeof word); /* corpus reads blind */
        return word;
    }
    return 0xffffffffu;
}
/* SavTreeWalk2 (FUN_004172f0) — see savegame.hpp. */
// FUNCTION: LEGORR 0x004172f0
void sav_tree_walk2(void* node, SavTree2State* state, int carry,
                    const EngineContext* eng)
{
    SavTreeNode* n = (SavTreeNode*)node;
    if (n == NULL || n->visited != 0u) {
        return;
    }
    const int extended = 1 < engine_app_mode_get()
                      || (engine_feature_word_peek() & 0x20000u) != 0u;
    if (extended) {
        state->flag = 0u;
    }
    n->visited = 1u;
    uint32_t need;
    if (state->window_base == NULL || extended) {
        need = state->flag == 0u ? 1u : 0u;
    } else {
        const char* w = (const char*)state->window_base
                      + (ptrdiff_t)((int32_t)n->window) * 0x3190;
        uint32_t word = 0u;
        __builtin_memcpy(&word, w, sizeof word);
        need = word & 1u;
    }
    if (state->blocks != NULL && *state->blocks != NULL) {
        char* blk = (char*)*state->blocks + (size_t)state->index * 0x20;
        uint32_t flags = 0u;
        uint32_t probe = 0u;
        __builtin_memcpy(&flags, blk, sizeof flags);
        __builtin_memcpy(&probe, blk + 0x1c, sizeof probe);
        flags = (need == 0u && carry == 0 && probe == 0u)
                    ? (flags & ~4u)
                    : (flags | 4u);
        __builtin_memcpy(blk, &flags, sizeof flags);
    }
    state->index = state->index + 1u; /* the corpus advances blindly */
    if (n->children != NULL) {
        void** kids = (void**)n->children; /* widened slots */
        for (uint32_t i = 0; i < n->count; ++i) {
            sav_tree_walk2(kids[i], state, need != 0u ? 1 : 0, eng);
        }
    }
}

/* SavTreeLoad (FUN_004173e0) — see savegame.hpp. */
// FUNCTION: LEGORR 0x004173e0
void sav_tree_load(void* node, void* window_base, void** blocks,
                   uint32_t window, uint32_t flag,
                   const EngineContext* eng)
{
    SavTree2State st; /* the corpus's five stack dwords */
    st.window_base = window_base;
    st.window = window;
    st.blocks = blocks;
    st.index = 0u;
    st.flag = flag;
    if (node == s_sav_tree_root[0]) sav_tree_owned_visit_flags_clear(0);
    if (node == s_sav_tree_root[1]) sav_tree_owned_visit_flags_clear(1);
    sav_tree_walk2(node, &st, 1, eng);
    sav_clear_visited();
    if (node == s_sav_tree_root[0]) sav_tree_owned_visit_flags_clear(0);
    if (node == s_sav_tree_root[1]) sav_tree_owned_visit_flags_clear(1);
}

// FUNCTION: LEGORR 0x00417430
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) int float_to_long_x87(unsigned int index, const void* record)
{
    __asm {
        mov ecx, dword ptr [esp + 4]
        mov edx, dword ptr [esp + 8]
        mov eax, ecx
        shl eax, 5
        add eax, ecx
        lea eax, [eax + eax * 2]
        lea ecx, [ecx + eax * 8]
        shl ecx, 4
        mov eax, dword ptr [edx + 0x34]
        fld dword ptr [ecx + eax + 0x2fb8]
        jmp _ftol
    }
}
#else
int float_to_long_x87(unsigned int index, const void* record)
{
    uintptr_t values = 0;
    uint32_t offset = index * 0x3190u;
    float value;
    memcpy(&values, (const char*)record + 0x34, sizeof(uintptr_t));
    memcpy(&value, (const char*)values + offset + 0x2fb8, sizeof(value));
    return crt_float_to_long(value);
}
#endif

/* SavMenuList (FUN_004179b0): snapshot the selected campaign/tutorial tree
 * into the port's typed level-row list. The native function emits packed
 * menu records and loads MenuBMP resources; this model preserves tree order,
 * level coordinates, Open state, and image keys for the frontend renderer. */
// FUNCTION: LEGORR 0x004179b0
int frontend_sav_menu_list(FrontEnd* fe, int list_id,
                           const SavTreeNode* root,
                           const SavListWindow* registry)
{
    if (fe == NULL || !fe->built || root == NULL || registry == NULL
        || (list_id != 4 && list_id != 5)) {
        return 0;
    }
    FeLevelMenuList* list = &fe->level_menu_lists[list_id - 4];
    memset(list, 0, sizeof(*list));
    list->native_id = list_id;
    snprintf(list->title, sizeof(list->title), "Levels...");
    FeSavMenuListBuild build;
    memset(&build, 0, sizeof(build));
    build.fe = fe;
    build.list = list;
    build.registry = registry;
    fe_sav_menu_list_walk(&build, root);
    return list->row_count;
}

/* SavReadSlot's retail ABI is (slot, slot-context, mode). The portable
 * slot block is not layout-compatible with the retail 0xb8-byte context;
 * use the GameContext bound by save_init or the typed adapter. */
// FUNCTION: LEGORR 0x00417a90
int save_read_slot_target(uint32_t slot, void* slot_context, int mode)
{
    GameContext* ctx = s_save_current_context;
    const int per_slot_context = slot_context != NULL &&
                                 slot_context != (void*)ctx;
    uint32_t tree_counts[2] = { 0u, 0u };
    int have_tree_counts = 0;
    if (ctx == NULL || slot >= SAVE_SLOTS) {
        return 0;
    }
    save_directory_ensure_exists(ctx->saves_dir[0] != '\0'
                                     ? ctx->saves_dir : "Saves");
    char path[280];
    save_slot_path(ctx, path, sizeof(path), slot, "sav");
    FILE* fp = fopen(path, MODE_RB);
    if (fp == NULL) {
        if (mode == 0) {
            uint32_t count = SAVE_DEFAULT_RECORDS;
            (void)save_tree_level_count(0, &count);
            if (per_slot_context) {
                (void)save_slot_write_fresh_file(ctx, (int)slot, count);
            } else {
                /* Preserve the typed port's historical empty-slot bootstrap
                 * used by checkpoint and direct-launch callers. */
                save_slot_set_target((int)slot);
                ctx->save_slot = (int)slot;
                if (save_bootstrap_records(ctx, SAVE_DEFAULT_RECORDS)) {
                    ctx->active_slot = (int)slot;
                    ctx->saved_music_volume = OPT_DFLT_MUSIC_VOLUME;
                    ctx->saved_sfx_volume = OPT_RETAIL_FRESH_SFX;
                    ctx->saved_scroll_index = OPT_DFLT_GAME_SPEED;
                    ctx->saved_brightness = OPT_RETAIL_FRESH_BRIGHTNESS;
                    (void)save_slot_write_fresh_file(ctx, (int)slot,
                                                     SAVE_DEFAULT_RECORDS);
                }
            }
        }
        return 0;
    }

    uint8_t header[SAVE_HEADER_SIZE];
    size_t got = fread(header, 1, SAVE_HEADER_SIZE, fp);
    if (got != SAVE_HEADER_SIZE) {
        fclose(fp);
        return 0;
    }
    /* GOPS identifies the earlier portable format unambiguously. Its
     * +0x10 word was music, not retail brightness; normalize the numeric
     * words before either the typed or per-slot reader consumes them. */
    {
        GameOptions legacy_options;
        options_defaults(&legacy_options);
        if (options_load_header(&legacy_options, header + 0x20)) {
            __builtin_memcpy(header + 0x04, &legacy_options.music_volume, 4);
            __builtin_memcpy(header + 0x08, &legacy_options.sfx_volume, 4);
            __builtin_memcpy(header + 0x0c, &legacy_options.game_speed, 4);
            __builtin_memcpy(header + 0x10, &legacy_options.brightness, 4);
        }
    }
    int32_t count;
    __builtin_memcpy(&count, header + 0x18, 4);
    have_tree_counts = save_current_level_counts(&tree_counts[0],
                                                 &tree_counts[1]);
    if (per_slot_context) {
        memcpy(slot_context, header, SAVE_HEADER_SIZE);
    }
    if (mode == 0 && have_tree_counts &&
        (uint32_t)count != tree_counts[0]) {
        /* Native SaveWriteSlot initializes this slot, then SavReadSlot clears
         * exactly the supplied 0xb8-byte slot context and returns a miss. */
        (void)save_slot_write_fresh_file(ctx, (int)slot, tree_counts[0]);
        if (per_slot_context) {
            save_slot_sidecar_release((int)slot);
            memset(slot_context, 0, 0xb8);
        } else {
            save_release_records(ctx);
        }
        fclose(fp);
        return 0;
    }

    if (count <= 0 || count > 0x1000) {
        fclose(fp);
        if (mode == 0 && per_slot_context) {
            save_slot_sidecar_release((int)slot);
            memset(slot_context, 0, 0xb8);
        } else if (mode == 0) {
            save_release_records(ctx);
        }
        return 0;
    }

    uint8_t* rec = (uint8_t*)malloc((size_t)count * SAVE_RECORD_SIZE);
    if (rec == NULL) {
        fclose(fp);
        return 0;
    }
    got = fread(rec, 1, (size_t)count * SAVE_RECORD_SIZE, fp);
    fclose(fp);
    if (got != (size_t)count * SAVE_RECORD_SIZE) {
        free(rec);
        return 0;
    }
    if (per_slot_context) {
        uint32_t retail_record_pointer = (uint32_t)(uintptr_t)rec;
        save_slot_sidecar_release((int)slot);
        s_sav_slot_records[slot] = rec;
        s_sav_slot_record_counts[slot] = count;
        __builtin_memcpy((uint8_t*)slot_context + 0x34,
                         &retail_record_pointer, 4);
        return 1;
    }

    /* Retail stores the three option words directly in the slot-context
     * header. Accept the former GOPS extension only for saves written by
     * earlier portable builds. */
    __builtin_memcpy(&ctx->saved_music_volume, header + 0x04, 4);
    __builtin_memcpy(&ctx->saved_sfx_volume, header + 0x08, 4);
    __builtin_memcpy(&ctx->saved_scroll_index, header + 0x0c, 4);
    __builtin_memcpy(&ctx->saved_brightness, header + 0x10, 4);
    GameOptions legacy_options;
    options_defaults(&legacy_options);
    if (options_load_header(&legacy_options, header + 0x20)) {
        ctx->options = legacy_options;
    } else {
        /* The retail header contains numeric settings, not the live
         * Advanced cycles. A checkpoint's default-slot reload must retain
         * those cycles, just as an ordinary slot read does. */
        ctx->options.music_volume = ctx->saved_music_volume;
        ctx->options.sfx_volume = ctx->saved_sfx_volume;
        ctx->options.game_speed = ctx->saved_scroll_index;
        ctx->options.brightness = ctx->saved_brightness;
    }
    save_release_records(ctx);
    ctx->active_slot = slot;
    ctx->record_count = count;
    ctx->records = rec;
    save_slot_set_target(slot);
    ctx->save_slot = slot;
    return 1;
}
/* Serialise the active slot to `<dir>\<slot>.sav` (SavWriteSlot). */
// FUNCTION: LEGORR 0x00417bd0
int sav_write_slot_file(GameContext* ctx)
{
    /* The typed save entry points also serve callers outside the menu's
     * close callback. Snapshot their effective settings before serializing. */
    ctx->saved_music_volume = ctx->options.music_volume;
    options_write_to_slot(ctx);
    save_directory_ensure_exists(ctx->saves_dir[0] != '\0'
                                     ? ctx->saves_dir : "Saves");
    char path[280];
    save_slot_path(ctx, path, sizeof(path), ctx->save_slot, "sav");
    FILE* fp = fopen(path, MODE_WB);
    if (fp == NULL) {
        return 0;
    }
    uint8_t header[SAVE_HEADER_SIZE] = { 0 };
    int32_t count = (int32_t)ctx->record_count;
    __builtin_memcpy(header + 0x18, &count, 4);
    /* The retail 0xb0-byte header is the slot context's raw prefix. The
     * tracked option words occupy +0x04/+0x08/+0x0c/+0x10; +0x20 is ordinary
     * context data, not a port-specific options extension. */
    __builtin_memcpy(header + 0x04, &ctx->saved_music_volume, 4);
    __builtin_memcpy(header + 0x08, &ctx->saved_sfx_volume, 4);
    __builtin_memcpy(header + 0x0c, &ctx->saved_scroll_index, 4);
    __builtin_memcpy(header + 0x10, &ctx->saved_brightness, 4);
    size_t wrote = fwrite(header, 1, SAVE_HEADER_SIZE, fp);
    if (wrote == SAVE_HEADER_SIZE && count > 0) {
        wrote = fwrite(ctx->records, 1, (size_t)count * SAVE_RECORD_SIZE, fp);
    }
    int ok = (wrote != 0);
    fclose(fp);
    if (!ok) {
        return 0;
    }
    /* After a write, refresh the shadow OSF table too (.osf sidecar). */
    (void)osf_write_path(ctx, ctx->save_slot);
    return 1;
}

// FUNCTION: LEGORR 0x00417df0
void sav_refresh_slots_target(void)
{
    GameContext* ctx = s_save_current_context;
    uint8_t* slot_context = s_sav_slot_context_blocks;
    uint8_t* const slot_context_end =
        s_sav_slot_context_blocks + sizeof(s_sav_slot_context_blocks);
    int slot = 0;
    if (ctx == NULL) return;
    const int write_first =
        (engine_feature_word_peek() & 0x400u) != 0u;
    do {
        if (!write_first) {
            if (save_read_slot_target((uint32_t)slot, slot_context, 0) != 0)
                goto next_slot;
        } else {
            uint32_t record_count = SAVE_DEFAULT_RECORDS;
            (void)save_tree_level_count(0, &record_count);
            (void)save_slot_write_fresh_file(ctx, slot, record_count);
        }
        (void)save_read_slot_target((uint32_t)slot, slot_context, 0);
next_slot:
        slot_context += 0xb8;
        ++slot;
    } while (slot_context < slot_context_end);
}
/* SavSlotCtx @ 0x00417e50 receives the slot index and returns its context.
 * The portable module has one active GameContext in place of six retail
 * context records, so valid indices resolve to that bound owner. */
// FUNCTION: LEGORR 0x00417e50
void* sav_slot_ctx_target(int slot)
{
    if (slot >= 0 && slot < SAVE_SLOTS && s_save_current_context != NULL)
        return s_save_current_context;
    return NULL;
}
/* SavBuildCtx @ 0x00417e70 reads the process-global slot and returns its
 * context block. The portable owner is bound by save_init and typed APIs. */
// FUNCTION: LEGORR 0x00417e70
void* sav_build_ctx_target(void)
{
    if (s_save_current_slot != -1)
        return sav_slot_ctx_target(s_save_current_slot);
    return NULL;
}

/* SavGetSlot @ 0x00417e90 is the no-argument read of the selected slot. */
// FUNCTION: LEGORR 0x00417e90
int save_slot_get_target(void)
{
    return s_save_current_slot;
}

/* SavSetSlot @ 0x00417ea0 writes the global slot and the bound context view. */
// FUNCTION: LEGORR 0x00417ea0
void save_slot_set_target(int slot)
{
    s_save_current_slot = slot;
}
// FUNCTION: LEGORR 0x00417eb0
void save_mark_record(GameContext* ctx, int index)
{
    uint32_t* flags = save_record_flags(ctx, index);
    if (flags == NULL) {
        return;
    }
    *flags |= 1u;   /* dirty */
    /* `|2` marks an empty-name/present record (corpus SavMarkRecord). */
    *flags |= 2u;
}

/* OptionsContextSixFieldSet @ 0x00417f20 has one source pointer argument. */
// FUNCTION: LEGORR 0x00417f20
void options_context_six_field_set_target(const uint32_t* src)
{
    if (sav_build_ctx_target() == NULL) return;
    uint32_t* dst = s_options_slot_window + 7;
    memcpy(dst, src, 6u * sizeof(*src));
}

/* SavWriteRecord @ 0x00417f40 takes only the record index and blob pointer;
 * the current save-slot context is process-global in the corpus. The typed
 * save_write_record adapter above grows the portable record table before
 * entering this target-shaped body. */
// FUNCTION: LEGORR 0x00417f40
int save_write_record_target(int index, const uint8_t* blob)
{
    GameContext* slot_ctx = (GameContext*)sav_build_ctx_target();
    uint32_t* destination;
    const uint32_t* source = (const uint32_t*)blob;
    uint32_t count;
    if (slot_ctx != NULL && index < slot_ctx->record_count) {
        destination = (uint32_t*)(slot_ctx->records
            + index * SAVE_RECORD_SIZE + 4);
        for (count = 0xC63u; count != 0u; --count)
            *destination++ = *source++;
        return 1;
    }
    return 0;
}

/* SavRecordPtr @ 0x00417f90 receives one index and reads the bound context. */
// FUNCTION: LEGORR 0x00417f90
void* sav_record_ptr_target(int index)
{
    GameContext* slot_ctx = (GameContext*)sav_build_ctx_target();
    if (slot_ctx != NULL && index < slot_ctx->record_count)
        return slot_ctx->records + index * SAVE_RECORD_SIZE + 4;
    return NULL;
}
/* SavCommit (FUN_00417fc0): the corpus commits the selected slot by
 * writing the open slot context — DAT_005574d0's slot through the
 * port's active-slot model, bootstrap-and-write faithful. */
// FUNCTION: LEGORR 0x00417fc0
void save_commit(GameContext* ctx)
{
    if (ctx == NULL || ctx->save_slot < 0) {
        return;
    }
    if (ctx->records == NULL) {
        save_bootstrap_records(ctx, SAVE_DEFAULT_RECORDS);
    }
    (void)sav_write_slot_file(ctx);

}

// FUNCTION: LEGORR 0x00417fe0
void* current_selection_context_get(void)
{
    return s_current_selection_context;
}

// FUNCTION: LEGORR 0x00417ff0
void current_selection_context_set(void* context)
{
    s_current_selection_context = context;
}

/* ShowLevelOverlayForCurrentSelection @ 0x00418000 is a no-argument global
 * entry. It commits the active slot before rebuilding the save views. */
// FUNCTION: LEGORR 0x00418000
void show_level_overlay_target(void)
{
    GameContext* ctx = save_current_context_get();
    const int slot = save_slot_get_target();
    if (ctx == NULL || slot < 0 || slot >= SAVE_SLOTS) return;
    (void)save_write_slot(ctx, slot); /* FUN_00417bd0 */
    save_menu_refresh_target(0u, slot);
}

namespace unported_ledger_stubs {
// FUNCTION: LEGORR 0x00418040
int ledger_stub_00418040_CampaignChainSnapshotCreate(
    const GameContext* ctx, void* snapshot, size_t capacity)
{
    unsigned char* bytes = (unsigned char*)snapshot;
    size_t records_size;
    uint32_t record_count;
    uint32_t records_offset;
    uint32_t header_word;
    uint32_t word_index;
    if (ctx == NULL || bytes == NULL ||
        sav_build_ctx((GameContext*)ctx) == NULL || ctx->record_count < 0)
        return 0;
    records_size = (size_t)ctx->record_count * SAVE_RECORD_SIZE;
    if ((size_t)ctx->record_count != 0u
        && records_size / (size_t)ctx->record_count != SAVE_RECORD_SIZE)
        return 0;
    if (capacity < 0xb8u + records_size
        || (records_size != 0u && ctx->records == NULL)) return 0;
    /* Copy the active 46-dword slot-context window, matching the decompile's
     * first 0xB8-byte copy. The active GameContext owns the live record count
     * and record bytes; encode their in-buffer location in the copied +0x34
     * pointer slot because the portable snapshot is a caller-owned buffer. */
    for (word_index = 0u; word_index < 0x2eu; ++word_index) {
        header_word = options_slot_window_dword_peek((int)word_index);
        memcpy(bytes + word_index * sizeof(header_word), &header_word,
               sizeof(header_word));
    }
    record_count = (uint32_t)ctx->record_count;
    records_offset = records_size != 0u ? 0xb8u : 0u;
    memcpy(bytes + 0x18, &record_count, sizeof(record_count));
    memcpy(bytes + 0x34, &records_offset, sizeof(records_offset));
    if (records_size != 0u)
        memcpy(bytes + 0xb8u, ctx->records, records_size);
    return 1;
}
} // namespace unported_ledger_stubs

/* OptionsContextCopyAndReplaceResource @ 0x004180c0 copies the 0xb8-byte
 * active slot window and transfers ownership of its resource word. */
// FUNCTION: LEGORR 0x004180c0
void options_context_copy_and_replace_resource_target(const uint32_t* source)
{
    if (sav_build_ctx_target() == NULL) return;
    if (s_options_slot_resource != NULL) {
        engine_heap_free(s_options_slot_resource);
    }
    __builtin_memcpy(s_options_slot_window, source,
                     sizeof s_options_slot_window);
    s_options_slot_resource = (void*)(uintptr_t)source[13];
    s_options_slot_window[13] = source[13];
}

// FUNCTION: LEGORR 0x00418110
void save_state_event_context_set(uint32_t value)
{
    s_save_state_event_context = value;
}

namespace lego {
// FUNCTION: LEGORR 0x00418120
int upgrade_average_is_above_99(void)
{
    uint32_t count;
    uint32_t i;
    uint32_t total = 0;
    float value;
    if (s_average_count_hook == 0 || s_average_value_hook == 0) return 0;
    count = (uint32_t)s_average_count_hook();
    if (count <= 8) return 0;
    for (i = 8; i < count; ++i) {
        if (!s_average_value_hook(i, &value)) return 0;
        if (value > 0.0f) total += (uint32_t)value;
        else if (value < 0.0f) total -= (uint32_t)(-value);
    }
    return total / (count - 8) > 99;
}
} // namespace lego

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
/* Split an Item value ("Next:0:0:Start_Game:Menu2", "Trigger:…:Yes:1",
 * "Slider:x1:y1:x2:y2:banner:lo:hi:img…") into ':'-fields. Position x/y are
 * fields 1/2 for every kind; the display label differs per kind (finding 23):
 * Next/Trigger put it at field 3, Slider/Cycle/RealSlider at field 5
 * (x1:y1:x2:y2). Image-backed Next/Trigger entries carry normal and
 * selected images plus a tooltip before their target/flag. */
static void fe_parse_item(const char* value, FeItem* it)
{
    it->kind = FE_ITEM_NEXT;
    it->x = 0; it->y = 0; it->flag = 0; it->level_start = 0;
    it->load_slot = 0;
    it->state_gated = 0;
    it->min = 0; it->max = 0; it->value = 0; it->cycle_count = 0;
    it->slider_x = 0; it->slider_y = 0;
    it->slider_config = NULL;
    memset(it->slider_images, 0, sizeof(it->slider_images));
    it->label[0] = '\0'; it->target[0] = '\0';
    it->image_normal[0] = it->image_selected[0] = it->image_disabled[0] = '\0';
    if (value == NULL) {
        return;
    }
    /* A Slider has eight resource paths after its seven numeric/text
     * fields. Preserve the complete retail tuple (16 fields total). */
    char fields[16][FE_PATH_MAX];
    int n = 0;
    const char* p = value;
    while (n < 16) {
        p += strspn(p, ":");
        if (*p == '\0') {
            break;
        }
        const char* s = p;
        size_t length = strcspn(p, ":");
        p += length;
        size_t copy_length = length;
        if (copy_length >= FE_PATH_MAX) {
            copy_length = FE_PATH_MAX - 1;
        }
        memcpy(fields[n], s, copy_length);
        fields[n][copy_length] = '\0';
        ++n;
    }
    if (n == 0) {
        return;
    }
    it->kind = fe_item_kind(fields[0]);
    int atoi1 = (n > 1) ? atoi(fields[1]) : 0;
    int atoi2 = (n > 2) ? atoi(fields[2]) : 0;
    switch (it->kind) {
    case FE_ITEM_NEXT: {
        it->x = atoi1;
        it->y = atoi2;
        const int image_form = n > 7 && s_stristr(fields[3], ".bmp");
        if (image_form) {
            snprintf(it->image_normal, sizeof(it->image_normal), "%s",
                     fields[3]);
            snprintf(it->image_selected, sizeof(it->image_selected), "%s",
                     fields[4]);
            snprintf(it->label, FE_NAME_MAX, "%s", fields[6]);
            snprintf(it->target, FE_NAME_MAX, "%s", fields[7]);
        } else if (n > 3) {
            it->state_gated = (n == 6);
            snprintf(it->label, FE_NAME_MAX, "%s", fields[3]);
            if (n > 4) {
                snprintf(it->target, FE_NAME_MAX, "%s", fields[4]);
            }
        }
        break;
    }
    case FE_ITEM_TRIGGER: {
        it->x = atoi1;
        it->y = atoi2;
        /* image trigger form: Trigger:x:y:imgN:imgG:imgD:tooltip:1 */
        int image_form = (n > 3) && s_stristr(fields[3], ".bmp");
        if (image_form) {
            snprintf(it->image_normal, sizeof(it->image_normal), "%s",
                     fields[3]);
            if (n > 4) {
                snprintf(it->image_selected, sizeof(it->image_selected),
                         "%s", fields[4]);
            }
            if (n > 6) {
                snprintf(it->label, FE_NAME_MAX, "%s", fields[6]);
            } else {
                snprintf(it->label, FE_NAME_MAX, "…");
            }
            if (n > 7) {
                it->flag = atoi(fields[7]);
            }
        } else {
            if (n > 3) {
                snprintf(it->label, FE_NAME_MAX, "%s", fields[3]);
            }
            if (n > 4) {
                it->flag = atoi(fields[4]);
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
            /* Slider:x1:y1:x2:y2:banner:lo:hi:off:on:leftcap:rightcap:
             * plus:minus:plus-highlight:minus-highlight. Resource paths are
             * still owned by the BlockFile; runtime handles are cached here. */
            it->slider_x = (n > 3) ? atoi(fields[3]) : 0;
            it->slider_y = (n > 4) ? atoi(fields[4]) : 0;
            if (n > 8) {
                it->slider_config = value;
            }
            it->min = (n > 6) ? atoi(fields[6]) : 0;
            it->max = (n > 7) ? atoi(fields[7]) : 100;
            it->flag = it->min;
            it->value = fe_option_default(it->label, it->min, it->max);
        } else if (it->kind == FE_ITEM_REALSLIDER) {
            /* RealSlider:type:x1:y1:x2:y2:banner:lo:hi:step */
            it->min = (n > 6) ? atoi(fields[6]) : 0;
            it->max = (n > 7) ? atoi(fields[7]) : 100;
            it->value = fe_option_default(it->label, it->min, it->max);
        } else if (it->kind == FE_ITEM_CYCLE) {
            /* Cycle:x1:y1:x2:y2:banner:count:opt1:opt2:… */
            it->cycle_x = (n > 3) ? atoi(fields[3]) : 0;
            it->cycle_y = (n > 4) ? atoi(fields[4]) : 0;
            int count = (n > 6) ? atoi(fields[6]) : 0;
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

/* MenuImageLoad (0x410fb0): normal and dark images share the optional
 * floating-point origin. Only a four-field tuple ending in 1 enables a key. */
static void fe_parse_menu_image(FeSubmenu* sub, const char* value, int dark)
{
    char fields[5][FE_PATH_MAX];
    const char* p = value;
    int count = 0;
    if (value == NULL) return;
    while (count < 5) {
        p += strspn(p, ":");
        if (*p == '\0') break;
        size_t length = strcspn(p, ":");
        size_t copied = length < FE_PATH_MAX ? length : FE_PATH_MAX - 1;
        memcpy(fields[count], p, copied);
        fields[count][copied] = '\0';
        ++count;
        p += length;
    }
    if (count == 0) return;
    snprintf(dark ? sub->image_dark : sub->image, FE_PATH_MAX, "%s", fields[0]);
    if (count >= 3) {
        sub->image_has_origin = 1;
        sub->image_x = (float)atof(fields[1]);
        sub->image_y = (float)atof(fields[2]);
    }
    int keyed = count == 4 && atoi(fields[3]) == 1;
    if (dark) sub->image_dark_keyed = keyed;
    else sub->image_keyed = keyed;
}

static void fe_parse_submenu(FrontEnd* fe, FeMenu* m, const char* name,
                             int idx)
{
    if (fe == NULL || m == NULL || name == NULL || idx < 0
        || idx >= FE_SUBMENUS_MAX || fe->cfg == NULL) {
        return;
    }
    FeSubmenu* sub = &m->submenus[idx];
    sub->item_count = 0;
    sub->base_x = sub->base_y = 0;
    sub->title_anchor_x = sub->title_anchor_y = 0;
    sub->auto_center = 0;
    sub->display_title = 0;
    sub->can_scroll = 0;
    sub->play_random = 0;
    sub->scroll = sub->scroll_extent = 0;
    sub->full_name[0] = sub->title[0] = '\0';
    sub->image[0] = sub->image_dark[0] = '\0';
    sub->image_x = sub->image_y = 0.0f;
    sub->image_has_origin = sub->image_keyed = sub->image_dark_keyed = 0;
    sub->lofont[0] = sub->hifont[0] = '\0';
    memset(sub->items, 0, sizeof(sub->items));
    memset(sub->overlays, 0, sizeof(sub->overlays));
    sub->overlay_count = 0;
    char pre[FE_PATH_MAX * 2];
    snprintf(pre, sizeof(pre), "%s::Menu::%s::%s", util_root_name(fe->cfg),
             name, fe_menu_name_idx(idx));
    char buf[FE_PATH_MAX * 2];

    snprintf(buf, sizeof(buf), "%s::Title", pre);
    if (block_get_strptr(fe->cfg, buf) != NULL) {
        snprintf(sub->title, FE_NAME_MAX, "%s", block_get_strptr(fe->cfg, buf));
    }
    snprintf(buf, sizeof(buf), "%s::FullName", pre);
    const char* v = block_get_strptr(fe->cfg, buf);
    if (v != NULL) {
        snprintf(sub->full_name, sizeof(sub->full_name), "%s", v);
        char* underscore = sub->full_name;
        while ((underscore = crt_strchr(underscore, '_')) != NULL) {
            *underscore++ = ' ';
        }
    }

    /* Position x:y */
    snprintf(buf, sizeof(buf), "%s::Position", pre);
    v = block_get_strptr(fe->cfg, buf);
    if (v != NULL) {
        const char* c = strchr(v, ':');
        if (c != NULL) {
            sub->base_x = atoi(v);
            sub->base_y = atoi(c + 1);
        }
    }

    snprintf(buf, sizeof(buf), "%s::AutoCenter", pre);
    sub->auto_center = block_get_bool_path(fe->cfg, buf) == 1;
    snprintf(buf, sizeof(buf), "%s::DisplayTitle", pre);
    sub->display_title = block_get_bool_path(fe->cfg, buf) != 0;
    /* Anchored is stored as x:y; title placement currently uses its y value. */
    snprintf(buf, sizeof(buf), "%s::Anchored", pre);
    v = block_get_strptr(fe->cfg, buf);
    if (v != NULL) {
        const char* c = strchr(v, ':');
        if (c != NULL) {
            char xbuf[16];
            size_t xlen = (size_t)(c - v);
            if (xlen >= sizeof(xbuf)) xlen = sizeof(xbuf) - 1;
            memcpy(xbuf, v, xlen);
            xbuf[xlen] = '\0';
            sub->title_anchor_x = atoi(xbuf);
            sub->title_anchor_y = atoi(c + 1);
        }
    }
    snprintf(buf, sizeof(buf), "%s::CanScroll", pre);
    sub->can_scroll = block_get_bool_path(fe->cfg, buf) == 1;
    snprintf(buf, sizeof(buf), "%s::MenuImage", pre);
    v = block_get_strptr(fe->cfg, buf);
    fe_parse_menu_image(sub, v, 0);
    snprintf(buf, sizeof(buf), "%s::MenuImageDark", pre);
    v = block_get_strptr(fe->cfg, buf);
    fe_parse_menu_image(sub, v, 1);

    /* Bitmap fonts: LoFont = unselected, HiFont = selected (MenuFont is
     * the measurement face — fall back to it for both when the pair is
     * absent). */
    snprintf(buf, sizeof(buf), "%s::LoFont", pre);
    if (block_get_strptr(fe->cfg, buf) != NULL) {
        snprintf(sub->lofont, FE_PATH_MAX, "%s",
                 block_get_strptr(fe->cfg, buf));
    }
    snprintf(buf, sizeof(buf), "%s::HiFont", pre);
    if (block_get_strptr(fe->cfg, buf) != NULL) {
        snprintf(sub->hifont, FE_PATH_MAX, "%s",
                 block_get_strptr(fe->cfg, buf));
    }
    if (sub->lofont[0] == '\0' || sub->hifont[0] == '\0') {
        snprintf(buf, sizeof(buf), "%s::MenuFont", pre);
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

    snprintf(buf, sizeof(buf), "%s::ItemCount", pre);
    const char* item_count_value = block_get_strptr(fe->cfg, buf);
    int ic = atoi(item_count_value != NULL ? item_count_value : "");
    if (ic > FE_ITEMS_MAX) {
        ic = FE_ITEMS_MAX;
    }
    sub->item_count = ic;
    for (int item_index = 0; item_index < ic; item_index++) {
        snprintf(buf, sizeof(buf), "%s::Item%d", pre, item_index + 1);
        const char* item = block_get_strptr(fe->cfg, buf);
        if (item == NULL) {
            continue;
        }
        fe_parse_item(item, &sub->items[item_index]);
    }

    /* FLIC overlays (`!Overlay%i <avi\X.flh>:<SFX>:x:y`) + PlayRandom —
     * finding 23's decoration lane; the `.flh` players open lazily at
     * display time (the paths may be loose Data/AVI copies). */
    snprintf(buf, sizeof(buf), "%s::PlayRandom", pre);
    sub->play_random = block_get_bool_path(fe->cfg, buf) == 1;
    sub->overlay_count = 0;
    for (int overlay_index =  1;
         overlay_index <= FE_OVERLAYS_MAX &&
             sub->overlay_count < FE_OVERLAYS_MAX;
         ++overlay_index) {
        snprintf(buf, sizeof(buf), "%s::!Overlay%d", pre, overlay_index);
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
                o->x = atoi(c2 + 1);
                const char* c3 = strchr(c2 + 1, ':');
                if (c3 != NULL && c3[1] != '\0') {
                    o->y = atoi(c3 + 1);
                }
            }
        }
        ++sub->overlay_count;
    }
}

/* VC5 rand.obj owns the shared target stream in the classic x86 build. Other
 * builds keep the frontend's isolated LCG model. */
static uint32_t fe_overlay_random(FrontEnd* fe)
{
#if defined(_MSC_VER) && (_MSC_VER < 1200) && defined(_M_IX86)
    (void)fe;
    return (uint32_t)rand();
#else
    fe->overlay_rng = fe->overlay_rng * 0x343fdu + 0x269ec3u;
    return (fe->overlay_rng >> 16) & 0x7fffu;
#endif
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
    snprintf(key, sizeof(key), "%s::Samples::!%s", util_root_name(fe->cfg),
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

/* BlockTextTokenSplit-backed config records used by SaveMenuLayoutLoad
 * separate fields with the vertical bar DAT string. Preserve empty fields:
 * the original integer parser turns an empty numeric token into zero. */
static int fe_parse_int_tuple_bar(const char* text, int* out, int count)
{
    const char* p;
    int i;
    if (text == NULL || out == NULL || count <= 0) {
        return 0;
    }
    p = text;
    for (i = 0; i < count; ++i) {
        const char* separator = strchr(p, '|');
        if (i + 1 < count && separator == NULL) {
            return 0;
        }
        if (i + 1 == count && separator != NULL) {
            return 0;
        }
        out[i] = atoi(p);
        if (separator != NULL) {
            p = separator + 1;
        }
    }
    return 1;
}

static void fe_sav_menu_list_walk(FeSavMenuListBuild* build,
                                  const SavTreeNode* node)
{
    if (build == NULL || node == NULL || build->list == NULL
        || build->registry == NULL || node->window >= (uint32_t)build->registry->count
        || build->registry->entries == NULL
        || build->list->row_count >= FE_LEVEL_MENU_ROWS) {
        return;
    }
    for (int i = 0; i < build->seen_count; ++i) {
        if (build->seen[i] == node) {
            return;
        }
    }
    if (build->seen_count >= FE_LEVEL_MENU_ROWS * 4) {
        return;
    }
    build->seen[build->seen_count++] = node;

    const char* name = (const char*)build->registry->entries[node->window];
    if (name == NULL || name[0] == '\0') {
        return;
    }
    const char* group = fe_level_group_name(name);
    FeLevelMenuRow* row = &build->list->rows[build->list->row_count++];
    memset(row, 0, sizeof(*row));
    snprintf(row->level, sizeof(row->level), "%s", group);
    char label[FE_NAME_MAX];
    if (fe_cfg_str(build->fe->cfg, group, "FullName", label,
                   sizeof(label)) != NULL) {
        snprintf(row->full_name, sizeof(row->full_name), "%s",
                 ui_text_underscores_to_spaces(label));
    } else {
        snprintf(row->full_name, sizeof(row->full_name), "%s", group);
    }
    row->x = fe_cfg_int(build->fe->cfg, group, "FrontEndX", 0);
    row->y = fe_cfg_int(build->fe->cfg, group, "FrontEndY", 0);
    (void)fe_cfg_str(build->fe->cfg, group, "MenuBMP", row->menu_bmp,
                     sizeof(row->menu_bmp));
    char path[FE_PATH_MAX * 2];
    snprintf(path, sizeof(path), "%s::Levels::%s::FrontEndOpen",
             util_root_name(build->fe->cfg), group);
    int open = block_get_bool_path(build->fe->cfg, path);
    if (open == 2) {
        snprintf(path, sizeof(path), "%s::%s::FrontEndOpen",
                 util_root_name(build->fe->cfg), group);
        open = block_get_bool_path(build->fe->cfg, path);
    }
    row->open = open == 1;

    const SavTreeNode* const* children =
        (const SavTreeNode* const*)node->children;
    for (uint32_t child_index = 0;
         children != NULL && child_index < node->count; ++child_index) {
        fe_sav_menu_list_walk(build, children[child_index]);
    }
}

/* Prepare the portable modal state after MenuShow has reset run flags. */
static int frontend_open_prepare(FrontEnd* fe, int menu, int submenu_index)
{
    fe_overlay_close_all(fe);
    fe->menu = menu;
    fe->save_action = 0;
    fe->sub = submenu_index;
    fe->sel = 0;
    fe->open = 1;
    fe->quit = 0;
    fe->pending_level[0] = '\0';
    /* MenuBuild's Slider records retain eight loaded surface handles. Open
     * these with the menu and let InterfaceResourceHandlesRelease balance
     * them when the modal menu exits. */
    for (int si = 0; si < fe->menus[menu].submenu_count; ++si) {
        FeSubmenu* sub = &fe->menus[menu].submenus[si];
        for (int ii = 0; ii < sub->item_count; ++ii) {
            FeItem* item = &sub->items[ii];
            if (item->kind != FE_ITEM_SLIDER || item->slider_config == NULL)
                continue;
            const char* p = item->slider_config;
            int field = 0;
            while (field < 8 && *p != '\0') {
                const char* colon = strchr(p, ':');
                if (colon == NULL) { p += strlen(p); break; }
                p = colon + 1;
                ++field;
            }
            for (int ri = 0; ri < 8 && *p != '\0'; ++ri) {
                const char* end = strchr(p, ':');
                size_t len = end != NULL ? (size_t)(end - p) : strlen(p);
                char path[FE_PATH_MAX];
                if (len >= sizeof(path)) len = sizeof(path) - 1;
                memcpy(path, p, len);
                path[len] = '\0';
                if (item->slider_images[ri] == NULL && path[0] != '\0')
                    item->slider_images[ri] = texture_get_or_load(NULL, path);
                if (end == NULL) break;
                p = end + 1;
            }
        }
    }
    /* Issue #85: re-entry shows the effective option values, never the
     * build-time defaults — sync every widget from the store. */
    frontend_options_sync(fe);
    /* corpus DAT_005574e0 reset @ 0x413d29: the plane returns to the top
     * whenever the menu (re-)enters. MenuShow does this before entering the
     * selected submenu's run loop. */
    for (int i = 0; i < fe->menus[menu].submenu_count; i++) {
        fe->menus[menu].submenus[i].scroll = 0;
    }
    return 1;
}

/* ScrollingTextDisplay (FUN_0040a0c0) enters the credits display. The
 * portable frontend is frame-driven, so its body raises the event consumed by
 * frontend_tick; the frame loop then loads the configured text and backdrop,
 * advances the 33 ms scroll clock, draws the centered rows, pumps audio/video,
 * and exits on keyboard input or mouse release. Those operations are split
 * across frontend_credits_lines_load/render_credits and frameloop_credits
 * because this module does not own the platform window or render surface. */
#if !(defined(_MSC_VER) && _MSC_VER < 1200)
static void scrolling_text_display(void* text, void* font, void* background)
{
    /* The retail arguments are renderer handles. In the portable boot path,
     * the configured CreditsTextFile, RSFont, and CreditsBackAVI are resolved
     * by the frame loop that consumes this request. Keep the ABI parameters
     * explicit even though that owner supplies their portable equivalents. */
    (void)text;
    (void)font;
    (void)background;
    s_main_menu_text_item_pending = 1;
}
#endif

static void fe_menu_record_put_word(void* record, int field, uint32_t value)
{
    __builtin_memcpy((char*)record + (size_t)field * sizeof(void*), &value,
                     sizeof(value));
}

static uint32_t fe_legacy_dword(const void* record, size_t field)
{
    uint32_t value = 0;
    if (record != NULL) {
        __builtin_memcpy(&value,
            (const char*)record + field * sizeof(void*), sizeof(value));
    }
    return value;
}

static void* fe_legacy_pointer(const void* record, size_t field)
{
    void* value = NULL;
    if (record != NULL) {
        __builtin_memcpy(&value,
            (const char*)record + field * sizeof(void*), sizeof(value));
    }
    return value;
}

static int fe_legacy_hit(uint32_t mouse_x, uint32_t mouse_y,
                         int32_t x, int32_t y, int32_t width, int32_t height)
{
    return menu_pointer_rectangle_hit_test((int32_t)mouse_x,
        (int32_t)mouse_y, x, y, width, height);
}

/* SavWriteSlot(slot, 0) creates an empty file sized for the current campaign
 * tree. This path does not change the process-global active slot or its
 * GameContext record allocation. */
static int save_slot_write_fresh_file(GameContext* ctx, int slot,
                                      uint32_t record_count)
{
    char path[280];
    uint8_t header[SAVE_HEADER_SIZE];
    uint8_t* records = NULL;
    size_t wrote;
    GameOptions defaults;
    int ok = 0;
    if (ctx == NULL || slot < 0 || slot >= SAVE_SLOTS ||
        record_count > 0x1000u) return 0;
    save_directory_ensure_exists(ctx->saves_dir[0] != '\0'
                                     ? ctx->saves_dir : "Saves");
    save_slot_path(ctx, path, sizeof(path), slot, "sav");
    memset(header, 0, sizeof(header));
    options_defaults(&defaults);
    /* SavWriteSlot's fresh-context defaults, distinct from menu fallback. */
    defaults.sfx_volume = OPT_RETAIL_FRESH_SFX;
    defaults.brightness = OPT_RETAIL_FRESH_BRIGHTNESS;
    __builtin_memcpy(header + 0x04, &defaults.music_volume, 4);
    __builtin_memcpy(header + 0x08, &defaults.sfx_volume, 4);
    __builtin_memcpy(header + 0x0c, &defaults.game_speed, 4);
    __builtin_memcpy(header + 0x10, &defaults.brightness, 4);
    __builtin_memcpy(header + 0x18, &record_count, 4);
    if (record_count != 0u) {
        if ((size_t)record_count > ((size_t)-1) / SAVE_RECORD_SIZE)
            return 0;
        records = (uint8_t*)calloc((size_t)record_count, SAVE_RECORD_SIZE);
        if (records == NULL) return 0;
    }
    FILE* fp = fopen(path, MODE_WB);
    if (fp == NULL) {
        free(records);
        return 0;
    }
    wrote = fwrite(header, 1, sizeof(header), fp);
    ok = wrote == sizeof(header);
    if (ok && record_count != 0u) {
        wrote = fwrite(records, 1,
                       (size_t)record_count * SAVE_RECORD_SIZE, fp);
        ok = wrote == (size_t)record_count * SAVE_RECORD_SIZE;
    }
    fclose(fp);
    free(records);
    return ok;
}

static void* sav_current_slot_record_window(void)
{
    int slot;
    GameContext* ctx;
    void* installed = sav_slot_stride_window_peek();
    if (installed != NULL) return installed;
    slot = save_slot_get_target();
    if (slot < 0 || slot >= SAVE_SLOTS) return NULL;
    if (s_sav_slot_records[slot] != NULL)
        return s_sav_slot_records[slot];
    ctx = save_current_context_get();
    if (ctx != NULL && ctx->active_slot == slot && ctx->records != NULL)
        return ctx->records;
    return NULL;
}

static void record_window_put(void* rec, int field, const void* src)
{
    if (rec == NULL || src == NULL) {
        return;
    }
    __builtin_memcpy((char*)rec + (size_t)field * sizeof(void*), src,
                     sizeof(uint32_t));
}

static void record_window_put_ptr(void* rec, int field, const void* value)
{
    if (rec != NULL) {
        __builtin_memcpy((char*)rec + (size_t)field * sizeof(void*), &value,
                         sizeof(value));
    }
}

static void record_window_put_u32(void* rec, int field, uint32_t value)
{
    record_window_put(rec, field, &value);
}

static void menu_texture_surface_release(TextureSurface* surface)
{
    texture_release(surface);
}

static void menu_resource_entry_release(MenuResourceEntry* entry)
{
    if ((entry->flags & 1u) != 0) {
        engine_heap_free(entry->owned_string);
    } else if ((entry->flags & 2u) != 0) {
        for (int i = 0; i < 3; ++i) {
            menu_texture_surface_release(entry->textures[i]);
        }
    }
}

/* --- FrontEnd.c entries in the original ABI, for Reward.c (GAME-037,
 * finding 560) ---------------------------------------------------------
 * Thin unannotated adapters (ADR 0005 step 6). savegame.cpp reaches
 * list_find_name_target and the campaign snapshot through its frozen
 * private header, so their linkage stays; the snapshot also needs the
 * active GameContext, which the original ABI does not pass. Port globals
 * stand in for FrontEnd.c state the port does not model; nothing sets
 * them yet. */
#include "frontend_api.h"
#include "video_graph.h"

char* g_front_end_avi1;
char* g_front_end_avi2;
void* g_front_main_menu_set;
void* g_front_save_tree;
uint8_t g_front_save_slot_buffer[0xec];
int g_front_save_menu_result;
int g_front_save_menu_open;

/* Front_LevelSet_IndexOf (00416e20) has C linkage under the name
 * front_level_set_index_of; savegame.cpp reaches it through its frozen
 * private header under the port's name, which this forwarder keeps. */
// PORT-ONLY: adapter or forwarder for the port contract
int list_find_name_target(const SavListWindow* win, const char* name)
{
    return front_level_set_index_of(win, name);
}

// PORT-ONLY: adapter or forwarder for the port contract
int front_campaign_snapshot_create(void* snapshot)
{
    return unported_ledger_stubs::ledger_stub_00418040_CampaignChainSnapshotCreate(
        save_current_context_get(), snapshot, 0xb8u);
}

// PORT-ONLY: adapter or forwarder for the port contract
void movie_play_wait(struct DirectShowGraphState* movie, int skippable)
{
    (void)movie;
    (void)skippable;
}

/* Original-ABI adapter (ADR 0005 step 6) for GAME-021's lego_unit.c: the
 * original FUN_00414f90 takes no argument; the port's entry above takes the
 * game context. */
// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (options_scroll_index_store)
extern "C" void options_scroll_index_store_current(float speed)
{
    (void)speed;
    options_scroll_index_store(entity_pool_callback_context_get());
}

/* Original-ABI adapter (ADR 0005 step 6) for GAME-021's lego_unit.c: the
 * original FUN_00415990 plays the level's Video for a level name; the
 * port's movie_level_video_path above only resolves the path and its
 * callers play it. No port equivalent of the playback: nothing. */
// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (no port equivalent of 0x00415990)
extern "C" void movie_play_level_video_adapter(const char* levelName,
                                               int flag)
{
    (void)levelName;
    (void)flag;
}

/* Original-ABI adapter (ADR 0005 step 6) for GAME-021's lego_unit.c: Front_IsFrontEndEnabled (00416d00) takes nothing; the port's entry reads the config. */
extern "C" struct BlockRec* legoGlobs_config;
// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (frontend_enabled)
extern "C" int frontend_enabled_original(void)
{
    return frontend_enabled(legoGlobs_config);
}

/* Original-ABI adapter (ADR 0005 step 6) for GAME-021's lego_unit.c: Front_IsIntrosEnabled (00414ce0) takes nothing; the port's entry reads the config. */
// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (movie_avis_enabled)
extern "C" int movie_avis_enabled_original(void)
{
    return movie_avis_enabled(legoGlobs_config);
}
