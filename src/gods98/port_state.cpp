// SPDX-License-Identifier: LGPL-3.0-only
/* Port tail: the port's state words and accessors that crt/crt.cpp used to
 * hold next to its runtime stand-ins (issue #447, finding 643). None of it is
 * C runtime code. The matching build takes the whole C runtime from LIBC.LIB
 * and no longer links src/crt/, so the accessors that units, legacy files and
 * the port tail call live here, in every build. The declarations stay in
 * crt/crt.hpp and its private headers.
 *
 * The GLOBAL markers below name game data (legoGlobs, mainGlobs and the
 * renderer words); they moved here unchanged with their definitions. */
#include <stdlib.h>
#include <string.h>
#include "gods98/texture.hpp"
/* C linkage for a legoGlobs field that GAME-021's lego_unit.c reads (issue
 * #544). The frozen header below redeclares it and inherits it. */
extern "C" float s_global_float_00556f00;
#include "crt/crt_for_lego.hpp"
#include "crt/crt.hpp"
#include "gods98/heap.hpp"
#include "gods98/string.hpp"
#include "crt/crt_shared.hpp"
#include "crt/crt_for_reward.hpp"
#include "crt/crt_for_panel_unit.hpp"
#include "crt/crt_from_panel_unit.hpp"
#include "crt/crt_for_frontend_unit.hpp"
#include "crt/crt_from_frontend_unit.hpp"
#include "crt/crt_for_container_unit.hpp"
#include "crt/crt_from_mesh_unit.hpp"

/* Portable typed adapter for ordinary C++ callers. */
int float_to_long(float value)
{
    return (int)value;
}

void float_pair_to_integer_pair(float x, float y, int* out_x, int* out_y)
{
    *out_x = (int)x;
    *out_y = (int)y;
}

/* The two DAT globals, static-backed until the corpus's writers (all
 * still unported) land — message.cpp's s_system_state precedent. The
 * float width is the corpus's own: 0x4230b0 flds a DWORD. */
/* legoGlobs +0xe40 (00556f00): in the matching build the field of
 * legoGlobs (lego_unit.c, lego_globs.h; finding 653). */
#if !(defined(_MSC_VER) && _MSC_VER < 1200)
float    s_global_float_00556f00;
#endif

/* Keep the portable API's null behavior pinned by test_static_globals. */
uint32_t global_value_004ff9bc_store_and_return(const void* value)
{
    return value != NULL
        ? global_value_004ff9bc_store_and_return_target(value) : 0u;
}

float    s_float_00554adc   = 0.0f;
float    s_float_00554efc   = 0.0f;

uint32_t s_counter_00555324 = 0u;
uint32_t s_counter_00555328 = 0u;
uint32_t s_counter_0055532c = 0u;
float    s_float_00555330   = 0.0f;
float    s_float_00555120   = 0.0f;

uint32_t crt_counter_004ff990_peek(void) { return s_counter_004ff990; }
float    crt_float_00554adc_peek(void)   { return s_float_00554adc; }
float    crt_float_00554efc_peek(void)   { return s_float_00554efc; }
uint32_t crt_counter_00555324_peek(void) { return s_counter_00555324; }
uint32_t crt_counter_00555328_peek(void) { return s_counter_00555328; }
uint32_t crt_counter_0055532c_peek(void) { return s_counter_0055532c; }
float    crt_float_00555330_peek(void)   { return s_float_00555330; }
float    crt_float_00555120_peek(void)   { return s_float_00555120; }

/* The display-mode dialog (0049d5e0-0049d9d0), its state (the filter
 * tuples at 00568060/005754e0) and the port's mode and resolution lists
 * moved out: the dialog is gods98/init.c (LATE-002, finding 613), and the
 * filter-tuple helpers that the port still calls are in
 * gods98/init_port.cpp. */

/* The port's copy of Bubbles.c's alwaysVisible (0x00557860, bubbles.c). */
uint32_t s_task_refresh_flag = 0u;  /* DAT_00557860 */

uint32_t global_mode_zero_input_peek(void)
{
    return s_mode_zero_flag;
}

uint32_t level_stored_word_peek(void)   { return s_level_stored_word; }
uint32_t mode_zero_input_flag_peek(void){ return s_mode_zero_flag; }
uint32_t rockfall_table_base_peek(void) { return s_rockfall_base; }
uint32_t ui_flag_word_peek(void)        { return s_ui_flag_word; }
uint32_t global_config_word_peek(void)  { return s_config_word; }

/* --- display dimensions + request-record predicates (see crt.hpp) ---------- */
// GLOBAL: LEGORR 0x00505924
int s_display_w;           /* DAT_00505924 */
// GLOBAL: LEGORR 0x00505928
int s_display_h;           /* DAT_00505928 */

void display_dimensions_set(int w, int h)
{
    s_display_w = w;
    s_display_h = h;
}

void request_record_panel_bounds_peek(int panel, float* out_x1,
                                      float* out_y1, float* out_x2,
                                      float* out_y2)
{
    PanelData* record = &panelGlobs.panelTable[panel];
    if (out_x1 != NULL) *out_x1 = record->xOut;
    if (out_y1 != NULL) *out_y1 = record->yOut;
    if (out_x2 != NULL) *out_x2 = record->xIn;
    if (out_y2 != NULL) *out_y2 = record->yIn;
}

int request_record_ready_flag4(int index)
{
    if ((unsigned)index >= PANEL_TYPE_COUNT) return 0;
    return request_record_ready_flag4_target(index);
}

int request_record_ready_flag2(int index)
{
    return request_record_ready_flag2_target(index);
}

int request_record_flag8_is_set(int index)
{
    if ((unsigned)index >= PANEL_TYPE_COUNT) {
        return 0; /* deviation: the corpus strides blindly */
    }
    return request_record_flag8_is_set_target(index);
}

void request_record_flags_set(int index, uint32_t flags)
{
    if ((unsigned)index < PANEL_TYPE_COUNT) {
        panelGlobs.panelTable[index].flags = flags;
    }
}

uint32_t request_record_flags_peek(int index)
{
    if ((unsigned)index >= PANEL_TYPE_COUNT) {
        return 0u;
    }
    return panelGlobs.panelTable[index].flags;
}

uint32_t global_value_0050080c_peek(void)
{
    return s_value_0050080c;
}

void font_cursor_word_set(int index, uint32_t v)
{
    if (index == -1) {
        s_cursor_slot = v; /* the DAT_00500a98 word itself */
        return;
    }
    if (index >= 0 && index < 16) {
        s_font_words[index] = v;
    }
}

float    s_fpair_a[DAT_WINDOW_COUNT]; /* DAT_00552144 entries */
float    s_fpair_b[DAT_WINDOW_COUNT]; /* DAT_00552148 entries */
void*    s_metric_records[DAT_WINDOW_COUNT]; /* DAT_00552154 */
void*    s_panel_records[DAT_WINDOW_COUNT]; /* DAT_00500134 */


void indexed_float_pair_peek(int index, float* out_x, float* out_y)
{
    if (index < 0 || index >= DAT_WINDOW_COUNT || out_x == NULL
        || out_y == NULL) {
        return;
    }
    *out_x = s_fpair_a[index];
    *out_y = s_fpair_b[index];
}

void indexed_record_pointer_set(int index, void* record)
{
    if (index >= 0 && index < DAT_WINDOW_COUNT) {
        s_metric_records[index] = record;
    }
}

void* indexed_record_pointer_peek(int index)
{
    if (index < 0 || index >= DAT_WINDOW_COUNT) {
        return NULL;
    }
    return s_metric_records[index];
}



/* Typed ownership for the portable named-panel loader. The record stays in
 * the original 32-bit pointer-word format; this sidecar carries host pointers. */
typedef struct PanelNamedResources {
    void* block;
    int count;
    TextureSurface** slots;
} PanelNamedResources;
static PanelNamedResources s_panel_named[DAT_WINDOW_COUNT];

/* Port ownership: free the button list (and its images) that
 * panel_texture_slots_allocate_named had Panel.c's loader build; the
 * original never frees one. Lists installed some other way are not ours. */
static void panel_named_release(int panel)
{
    PanelNamedResources* owner = &s_panel_named[panel];
    PanelButton* list = (PanelButton*)owner->block;
    int i;
    for (i = 0; i < owner->count && list != NULL; ++i) {
        Image* images[3];
        int k;
        images[0] = list[i].normalImage;
        images[1] = list[i].highlightImage;
        images[2] = list[i].pressedImage;
        for (k = 0; k < 3; ++k) {
            if (images[k] == NULL) continue;
            texture_free((TextureSurface*)images[k]->surface);
            free(images[k]);
        }
    }
    engine_heap_free(owner->block);
    owner->slots = NULL;
    owner->count = 0;
    owner->block = NULL;
}

TextureSurface* panel_named_resource_get(int panel, const void* block,
                                         int item, int field, uint32_t word)
{
    PanelNamedResources* owner;
    TextureSurface* surface;
    if (panel < 0 || panel >= DAT_WINDOW_COUNT || field < 4 || field > 6)
        return NULL;
    owner = &s_panel_named[panel];
    if (owner->block != block || item < 0 || item >= owner->count)
        return NULL;
    surface = owner->slots[item * 3 + field - 4];
    if (surface == NULL || (uint32_t)(uintptr_t)surface != word) return NULL;
    return surface;
}

void panel_draw_records_set(int index, void* records)
{
    if (index >= 0 && index < PANEL_TYPE_COUNT) {
        if (panelGlobs.panelTable[index].buttonList != records &&
            panelGlobs.panelTable[index].buttonList == s_panel_named[index].block)
            panel_named_release(index);
        panelGlobs.panelTable[index].buttonList = (PanelButton*)records;
    }
}

void* panel_draw_records_peek(int index)
{
    if (index < 0 || index >= PANEL_TYPE_COUNT) {
        return NULL;
    }
    return panelGlobs.panelTable[index].buttonList;
}

float    s_reward_score;  /* DAT_00555738 */
float    s_reward_count;  /* DAT_0055573c */

float reward_score_total_peek(void) { return s_reward_score; }
uint32_t reward_score_count_peek(void)
{
    return (uint32_t)s_reward_count;
}

/* --- the panel/view blit band (see crt.hpp) --------------------------------- */
uint32_t s_view_handle[DAT_WINDOW_COUNT]; /* DAT_00552140 */
void*    s_view_rect_a[DAT_WINDOW_COUNT]; /* DAT_0055214c */
void*    s_view_rect_b[DAT_WINDOW_COUNT]; /* DAT_00552150 */
int      s_view_region[DAT_WINDOW_COUNT]; /* DAT_00552158 */
static int      s_panel_count[DAT_WINDOW_COUNT]; /* DAT_00500138 */

#define CRT_SURFACE_CALLS 256
static uint32_t s_call_handle[CRT_SURFACE_CALLS];
static float    s_call_x[CRT_SURFACE_CALLS];
static float    s_call_y[CRT_SURFACE_CALLS];
static int      s_call_n;
/* Optional rect witnesses: rect1 is the corpus's source rect (4 floats)
 * and rect3 the size pair (2 floats); the plain record leaves both
 * absent. */
/* The rect witnesses are kept natively only (#700, finding 627): in the
 * matching build nothing reachable records a call, and there no witness
 * is ever set. */
#if !(defined(_MSC_VER) && _MSC_VER < 1200)
static float    s_call_rect1[CRT_SURFACE_CALLS][4];
static float    s_call_rect3[CRT_SURFACE_CALLS][2];
#endif
static uint8_t  s_call_rect1_set[CRT_SURFACE_CALLS];
static uint8_t  s_call_rect3_set[CRT_SURFACE_CALLS];

static void surface_call_store(uint32_t handle, float x, float y,
                               const float* rect1, const float* rect3)
{
    if (s_call_n >= CRT_SURFACE_CALLS) {
        return;
    }
    s_call_handle[s_call_n] = handle;
    s_call_x[s_call_n] = x;
    s_call_y[s_call_n] = y;
    s_call_rect1_set[s_call_n] = 0u;
    s_call_rect3_set[s_call_n] = 0u;
#if !(defined(_MSC_VER) && _MSC_VER < 1200)
    if (rect1 != NULL) {
        __builtin_memcpy(s_call_rect1[s_call_n], rect1,
                         sizeof s_call_rect1[s_call_n]);
        s_call_rect1_set[s_call_n] = 1u;
    }
    if (rect3 != NULL) {
        __builtin_memcpy(s_call_rect3[s_call_n], rect3,
                         sizeof s_call_rect3[s_call_n]);
        s_call_rect3_set[s_call_n] = 1u;
    }
#else
    (void)rect1; (void)rect3;
#endif
    ++s_call_n;
}

uint32_t crt_surface_handle_next(void)
{
    static uint32_t s_handle_counter = 0u;
    ++s_handle_counter;
    return s_handle_counter;
}

void crt_surface_blit_record(uint32_t handle, float x, float y)
{
    surface_call_store(handle, x, y, NULL, NULL);
}

void crt_surface_blit_record_rect(uint32_t handle, float x, float y,
                                  const float* rect1, const float* rect3)
{
    surface_call_store(handle, x, y, rect1, rect3);
}

int crt_surface_calls_count(void) { return s_call_n; }

int crt_surface_call_get(int index, uint32_t* handle, float* x, float* y)
{
    if (index < 0 || index >= s_call_n) {
        return 0;
    }
    if (handle != NULL) {
        *handle = s_call_handle[index];
    }
    if (x != NULL) {
        *x = s_call_x[index];
    }
    if (y != NULL) {
        *y = s_call_y[index];
    }
    return 1;
}

int crt_surface_call_get_rect(int index, float* rect1, int* rect1_set,
                              float* rect3, int* rect3_set)
{
    if (index < 0 || index >= s_call_n) {
        return 0;
    }
    if (rect1_set != NULL) {
        *rect1_set = s_call_rect1_set[index] != 0u;
    }
    if (rect3_set != NULL) {
        *rect3_set = s_call_rect3_set[index] != 0u;
    }
#if !(defined(_MSC_VER) && _MSC_VER < 1200)
    if (s_call_rect1_set[index] != 0u && rect1 != NULL) {
        __builtin_memcpy(rect1, s_call_rect1[index],
                         4 * sizeof(float));
    }
    if (s_call_rect3_set[index] != 0u && rect3 != NULL) {
        __builtin_memcpy(rect3, s_call_rect3[index], 2 * sizeof(float));
    }
#endif
    return 1;
}

void crt_surface_calls_clear(void) { s_call_n = 0; }

float crt_float_bits(uint32_t bits)
{
    float f = 0.0f;
    __builtin_memcpy(&f, &bits, sizeof(f));
    return f;
}

void view_blit_window_set(int view, uint32_t handle, void* rect_a,
                          void* rect_b, int region_index)
{
    if (view < 0 || view >= DAT_WINDOW_COUNT) {
        return;
    }
    s_view_handle[view] = handle;
    s_view_rect_a[view] = rect_a;
    s_view_rect_b[view] = rect_b;
    s_view_region[view] = region_index;
}

void view_blit_window_peek(int view, uint32_t* handle, void** rect_a,
                           void** rect_b, int* region_index)
{
    if (view < 0 || view >= DAT_WINDOW_COUNT) {
        return;
    }
    if (handle != NULL) {
        *handle = s_view_handle[view];
    }
    if (rect_a != NULL) {
        *rect_a = s_view_rect_a[view];
    }
    if (rect_b != NULL) {
        *rect_b = s_view_rect_b[view];
    }
    if (region_index != NULL) {
        *region_index = s_view_region[view];
    }
}

void panel_draw_count_set(int panel, int count)
{
    if (panel >= 0 && panel < PANEL_TYPE_COUNT) {
        panelGlobs.panelTable[panel].buttonCount = (unsigned int)count;
    }
}

int panel_draw_count_peek(int panel)
{
    if (panel < 0 || panel >= PANEL_TYPE_COUNT) {
        return 0;
    }
    return (int)panelGlobs.panelTable[panel].buttonCount;
}

void panel_texture_slots_allocate_named(
    int panel, int count, const int* slot_index, const float* stream,
    const char* const* tex1, const char* const* tex2,
    const char* const* tex3, const int* ids)
{
    int i;
    if (panel < 0 || panel >= PANEL_TYPE_COUNT || count <= 0 ||
        slot_index == NULL || stream == NULL || ids == NULL) {
        return;
    }
    /* Panel.c's loader writes button slot_index[i]; the port refuses a
     * table that would write outside the list or leave a button unset. */
    for (i = 0; i < count; ++i) {
        int j;
        if (slot_index[i] < 0 || slot_index[i] >= count) return;
        for (j = 0; j < i; ++j)
            if (slot_index[j] == slot_index[i]) return;
    }
    if (panelGlobs.panelTable[panel].buttonList == s_panel_named[panel].block)
        panel_named_release(panel);
    panel_texture_slots_allocate(panel, (unsigned int)count,
                                 (const unsigned int*)slot_index,
                                 stream,
                                 (char**)tex1, (char**)tex2, (char**)tex3,
                                 ids);
    s_panel_named[panel].block = panelGlobs.panelTable[panel].buttonList;
    s_panel_named[panel].count = count;
}

/* --- the small global primitives band (see crt.hpp) -------------------------- */

void global_value_word_set(int value) { s_global_value_word = value; }
int  global_value_word_peek(void) { return s_global_value_word; }

void* linked_list_global_head_peek(void) { return s_linked_list_head; }

void linked_list_global_head_set(void* record)
{
    s_linked_list_head = record;
}

/* SceneEffectRegistryReset (FUN_00465380) — the transient-effect registry
 * words (head/tail/shared-texture, unowned) clear and the reset answers 1
 * exactly as the corpus does. The corpus addresses the trio individually at
 * 0x00552128/0x0055212c/0x00552130, so each word is its own annotated
 * static (the s_indexed_window precedent); all three are zero-filled
 * virtual tail, matching the zero-initialized statics here. */
uint32_t s_scene_effect_head;     /* DAT_00552128 role: transient-effect head */
uint32_t s_scene_effect_tail;     /* DAT_0055212c role: transient-effect tail */
uint32_t s_scene_effect_shared;   /* DAT_00552130 role: shared texture */

void scene_effect_registry_words_peek(uint32_t out_words[3])
{
    if (out_words == NULL) {
        return;
    }
    out_words[0] = s_scene_effect_head;
    out_words[1] = s_scene_effect_tail;
    out_words[2] = s_scene_effect_shared;
}

void* linked_list_global_aux_peek(void) { return s_linked_list_aux; }
void  linked_list_global_aux_set(void* word) { s_linked_list_aux = word; }

void model_render_list_head_set(void* node) { s_model_render_list_head = node; }
void* model_render_list_head_peek(void) { return s_model_render_list_head; }

/* --- the shared texture directory word (see crt.hpp) ------------------------ *
 * The port's word for containerGlobs.sharedDir (0076cd88; finding 624). */
char* s_shared_texture_dir; /* DAT_0076cd88 role: unowned */

char* shared_texture_directory_peek(void) { return s_shared_texture_dir; }
void shared_texture_directory_clear(void)
{
    engine_heap_free(s_shared_texture_dir);
    s_shared_texture_dir = NULL;
}

/* --- the renderer flag word + scene hook pair band (see crt.hpp) ----------- *
 * The port's words for containerGlobs' flags and trigger callbacks
 * (0076cd78-0076cd94), which container.c now defines (finding 624). */
uint32_t s_renderer_flag_word; /* DAT_0076cd94 role: unowned */
void*    s_scene_end_hook;     /* DAT_0076cd80 role: unowned */
void*    s_scene_end_value;    /* DAT_0076cd84 role: unowned */
void*    s_scene_sample_hook;  /* DAT_0076cd78 role: unowned */
void*    s_scene_sample_value; /* DAT_0076cd7c role: unowned */

uint32_t renderer_flag_word_peek(void) { return s_renderer_flag_word; }

void* scene_end_hook_peek(void) { return s_scene_end_hook; }
void* scene_end_hook_value_peek(void) { return s_scene_end_value; }

void* scene_sample_hook_peek(void) { return s_scene_sample_hook; }
void* scene_sample_hook_value_peek(void) { return s_scene_sample_value; }

void string_registry_reset(void)
{
    const long field = (long)sizeof(void*);
    void* entry = s_string_registry_head;
    while (entry != NULL) {
        void* next = NULL;
        __builtin_memcpy(&next, (char*)entry + 3 * field, sizeof next);
        char* key = NULL;
        __builtin_memcpy(&key, (char*)entry + 0 * field, sizeof key);
        engine_heap_free(key);
        engine_heap_free(entry);
        entry = next;
    }
    s_string_registry_head = NULL;
}

static void* (*s_bitmap_surface_loader)(const char*);
void* (*s_font_resource_loader)(const char*);

void string_cache_bitmap_loader_set(void* (*hook)(const char*))
{
    s_bitmap_surface_loader = hook;
}

void string_cache_font_loader_set(void* (*hook)(const char*))
{
    s_font_resource_loader = hook;
}

/* Shared cache-slot fold: load once on a zero slot, return the word. */
static void* s_entry_cache_slot_get(const char* key, long slot,
                                    void* (*loader)(const char*))
{
    if (key == NULL || *key == '\0') {
        return NULL;
    }
    void* entry = string_registry_intern(key);
    if (entry == NULL) {
        return NULL; /* deviation: the corpus strides the intern failure */
    }
    void* value = NULL;
    __builtin_memcpy(&value, (char*)entry + slot * (long)sizeof(void*),
                     sizeof value);
    if (value == NULL) {
        value = (loader != NULL) ? loader(key)
                                 : NULL; /* seam: answers NULL uninstalled */
        if (value != NULL) {
            __builtin_memcpy((char*)entry + slot * (long)sizeof(void*),
                             &value, sizeof value);
        }
    }
    return value;
}

void* string_cache_lookup_or_load(const char* key)
{
    if (s_bitmap_surface_loader != NULL) {
        return s_entry_cache_slot_get(key, 1, s_bitmap_surface_loader);
    }
    return string_cache_lookup_or_load_corpus(key);
}

void selection_marker_slot_set(int index, void* ptr)
{
    if (index >= 0 && index < 32) {
        s_marker_table[index] = ptr;
    }
}
void* selection_marker_slot_peek(int index)
{
    return (index >= 0 && index < 32) ? s_marker_table[index] : NULL;
}
void  selection_marker_descriptor_set(void* desc) { s_marker_descriptor = desc; }
void* selection_marker_descriptor_peek(void) { return s_marker_descriptor; }
void     selection_marker_count_set(uint32_t count) { s_marker_count = count; }
uint32_t selection_marker_count_peek(void) { return s_marker_count; }

/* --- the renderer-word band (2026-10-25) ------------------------------------
 * The corpus's DAT_0076ab24/b28/b3c + DAT_0076ac84/ac8c + DAT_0050608c band
 * has no ported writer; each word lands as a zero-init static and the six
 * getters answer their installed half (the unowned-global deviation, per the
 * DAT-window convention). The install hooks are port-side. */
// GLOBAL: LEGORR 0x0076ab24
uint32_t s_renderer_word_a;              /* DAT_0076ab24 */
// GLOBAL: LEGORR 0x0076ab28
uint32_t s_renderer_word_b;              /* DAT_0076ab28 */
// GLOBAL: LEGORR 0x0076ab3c
uint32_t g_renderer_word_c;             /* DAT_0076ab3c */
// GLOBAL: LEGORR 0x0076ab40
uint32_t s_window_mouse_word_40; /* DAT_0076ab40 */
// GLOBAL: LEGORR 0x0076ab4c
uint32_t s_window_mouse_word_4c; /* DAT_0076ab4c */
// GLOBAL: LEGORR 0x0076ab50
uint32_t s_window_mouse_word_50; /* DAT_0076ab50 */
// GLOBAL: LEGORR 0x0076ab54
uint32_t s_window_mouse_word_54; /* DAT_0076ab54 */
// GLOBAL: LEGORR 0x0076ab58
uint32_t s_window_mouse_word_58; /* DAT_0076ab58 */
uint32_t s_renderer_global_dword; /* DAT_0076ac84 */
void* s_renderer_primary_surface; /* DAT_0076ac88 */
uint32_t s_render_surface_secondary; /* DAT_0076ac8c */
uint32_t s_renderer_window_state_90; /* DAT_0076ac90 */
uint32_t s_renderer_window_state_94; /* DAT_0076ac94 */
uint32_t s_renderer_window_state_9c; /* DAT_0076ac9c */
uint32_t s_renderer_window_state_a4; /* DAT_0076aca4 */
uint32_t s_renderer_window_state_ac; /* DAT_0076acac */
uint32_t s_renderer_window_state_b0; /* DAT_0076acb0 */
uint32_t s_renderer_window_state_b4; /* DAT_0076acb4 */
uint32_t s_renderer_window_state_b8; /* DAT_0076acb8 */
// GLOBAL: LEGORR 0x0050608c
uint32_t s_render_feature_flags; /* DAT_0050608c */

void renderer_word_c_set(uint32_t value) { g_renderer_word_c = value; }
void renderer_words_set(uint32_t a, uint32_t b, uint32_t c)
{
    s_renderer_word_a = a;
    s_renderer_word_b = b;
    g_renderer_word_c = c;
}

void renderer_global_dword_set(uint32_t value) { s_renderer_global_dword = value; }

void render_surface_secondary_set(uint32_t value) { s_render_surface_secondary = value; }

#if defined(LEGORRR_BACKEND_MINIWIN)
// PORT-ONLY: the native body of Main.c's Main_GetCLFlags; gods98/main.c (G98-005) has the original.
uint32_t render_feature_flags_get(void) { return s_render_feature_flags; }
#endif /* LEGORRR_BACKEND_MINIWIN */
void render_feature_flags_set(uint32_t value) { s_render_feature_flags = value; }

/* SetLowTextureFlag (2026-10-25): the corpus's DAT_00505f84 engine word has
 * a second half-model (EngineContext::feature in engine.hpp). This band's
 * store carries no context, so the port models it as a module static with
 * install/peek hooks — the two half-models do not interoperate
 * (unowned-global deviation). */
// GLOBAL: LEGORR 0x00505f84
uint32_t s_engine_feature_word; /* DAT_00505f84 */

uint32_t engine_feature_word_peek(void)
{
    return s_engine_feature_word;
}

void engine_feature_word_install(uint32_t value) /* port hook */
{
    s_engine_feature_word = value;
}

#if defined(LEGORRR_BACKEND_MINIWIN)
// PORT-ONLY: the native body of Main.c's Main_DisableTextureManagement; gods98/main.c (G98-005) has the original.
void low_texture_flag_set(void) /* FUN_00477bd0 */
{
    if ((s_engine_feature_word & 0x40000u) == 0u) {
        s_engine_feature_word |= 0x20u;
    }
}
#endif /* LEGORRR_BACKEND_MINIWIN */

void global_flag_004ff978_install(uint32_t value)
{
    s_flag_004ff978 = value;
}
