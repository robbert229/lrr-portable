// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-009, original translation unit (finding 505), rebuilt in original
 * shape (ADR 0005; finding 609): text 0040a0c0-0040ab10, .rdata
 * 0049e0a8-0049e0c4, literals 004a1d64-004a1d88 and the .bss block
 * 004b8a58-004b8bc4.
 *
 * Two parts, as OpenLRR names them:
 * - the credits loop (Credits_Play, 0040a0c0): it scrolls the lines of the
 *   credits text, centred, over the looping AVI backdrop until a key is
 *   pressed or the left mouse button is released;
 * - the damage font (DamageFont.c, 0040a3d0-0040aae0): a billboard of up to
 *   four digit quads, with a minus sign, that rises over a damaged object
 *   and fades out over fifteen time units.
 *
 * 0040ab10 and 0040ab30 are Dependencies.c's (GAME-010) and stay out of
 * this unit (finding 609).
 *
 * The port's implementation stays in the port tail
 * screen_effect_quads_port.cpp, unannotated. In the matching build
 * screen_effect_quads_link_names.h gives this unit's functions the port's
 * names, so every caller reaches them; natively the port tail serves those
 * names and nothing calls these. */
#include "screen_effect_quads_link_names.h"

#include <stdio.h>
#include <stdlib.h>

#include "crt/vc5names.h"
#include "gods98/winmm.h"
#include "screen_effect_quads.h"
#include "screen_effect_quads_imports.h"

/* The unit's functions have C linkage in the native C++ compile too. */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct DamageFont_Vector3F {
    float x, y, z;
} DamageFont_Vector3F;

#include "screen_effect_quads_maths.h"

/* One rising number (0x20 bytes). */
typedef struct DamageFont_Data {
    void* cont;                    /* +0x00 the container */
    void* mesh;                    /* +0x04 the digit quads */
    unsigned int digitCount;       /* +0x08 the digits and the minus sign */
    DamageFont_Vector3F position;  /* +0x0c relative to the container */
    float timer;                   /* +0x18 */
    unsigned int flags;            /* +0x1c bit 0: in use */
} DamageFont_Data;

#define DAMAGEFONT_MAXSHOWS 10
#define DAMAGEFONT_MAXTIME 15.0f

typedef struct DamageFont_Globs {
    void* fontTextDigits[10];                       /* +0x000 004b8a58 */
    void* fontTextMinus;                            /* +0x028 004b8a80 */
    DamageFont_Data instanceTable[DAMAGEFONT_MAXSHOWS]; /* +0x02c 004b8a84 */
} DamageFont_Globs;                                 /* 0x16c bytes */

/* The minimal view of a LegoObject the unit reads: its type. */
typedef struct DamageFont_LiveObject {
    int type;
} DamageFont_LiveObject;

// GLOBAL: LEGORR 0x004b8a58
static DamageFont_Globs damageFontGlobs;

static void damage_font_mesh_render_callback(void* mesh, DamageFont_Data* data,
                                             void* view);

// FUNCTION: LEGORR 0x0040a0c0
void credits_play(const char* textFile, struct TextureSurface* font,
                  const char* aviFile)
{
    RECT destRect;
    int centre;
    char* buffer;
    int quit;
    unsigned int lastTime;
    int line;
    char** lines;
    unsigned int fontHeight;
    int currLine;
    unsigned int scroll;
    AviMovie* avi;
    int lineCount;
    int loop;
    int linesOnScreen;
    char cconvName[1024];
    int lastMslb;
    int fileSize;

    lineCount = 0;
    scroll = 0;
    quit = FALSE;
    lastTime = timeGetTime();
    destRect.left = 0;
    destRect.top = 0;
    destRect.right = s_display_w;
    destRect.bottom = s_display_h;
    lastMslb = 0;
    fontHeight = object_field_be4_dword_get(font);
    linesOnScreen = (unsigned int)s_display_h / fontHeight + 1;
    centre = s_display_w / 2;

    // STRING: LEGORR 0x004a1d68
    sprintf(cconvName, "%s.cconv", textFile);
    if ((buffer = (char*)file_load_binary(cconvName, (unsigned int*)&fileSize)) ||
        (buffer = (char*)file_load_binary(textFile, (unsigned int*)&fileSize))) {
        buffer = (char*)realloc(buffer, fileSize + 4);
        buffer[fileSize] = '\0';
        avi = avi_load((char*)aviFile);

        for (loop = 1; loop < fileSize; loop++) {
            if (buffer[loop - 1] == '\r' && buffer[loop] == '\n') lineCount++;
        }
        lineCount++;
        lines = (char**)malloc(lineCount * sizeof(char*));
        // STRING: LEGORR 0x004a1d64
        sound_variant_split(buffer, lines, "\r\n");

        currLine = -linesOnScreen;
        do {
            if (input_keyboard_down_key_count() ||
                (!g_renderer_word_c && lastMslb)) quit = TRUE;
            lastMslb = g_renderer_word_c;

            avi_update(avi);
            if (avi_get_time(avi) >= avi_get_length(avi)) avi_set_time(avi, 0);
            avi_blit_to_back_buffer(avi, (struct tagRECT*)&destRect);

            if (timeGetTime() - lastTime > 33) {
                if (++scroll == fontHeight) {
                    scroll = 0;
                    if (currLine++ == lineCount) currLine = -linesOnScreen;
                }
                lastTime += 33;
                if (lastTime < timeGetTime()) lastTime = timeGetTime();
            }

            for (line = currLine; line < lineCount; line++) {
                if (line - currLine > linesOnScreen) break;
                if (line >= 0) {
                    int width = formatted_text_printf_width(font, "%s", lines[line]);
                    font_print_f(font, centre - width / 2,
                                 line * fontHeight - currLine * fontHeight - scroll,
                                 "%s", lines[line]);
                }
            }

            sound_subsystem_frame_tick(1.0f);
            if (avi_is_ok(avi)) {
                main_loop_frame_pump(FALSE);
            } else {
                main_loop_frame_pump(TRUE);
                Sleep(10);
            }
        } while (!quit);

        if (avi) avi_remove(avi);
        free(lines);
        free(buffer);
    }
}

// FUNCTION: LEGORR 0x0040a3d0
void damage_font_cleanup(void)
{
    unsigned int i;
    for (i = 0; i < DAMAGEFONT_MAXSHOWS; i++) {
        DamageFont_Data* data = &damageFontGlobs.instanceTable[i];
        if (data->flags & 1) {
            data->flags &= ~1;
            scene_render_lock_toggle(data->cont, TRUE);
        }
    }
}

// FUNCTION: LEGORR 0x0040a400
void damage_font_load_frames(const char* dirName, const char* fileName)
{
    char baseName[260];
    char name[260];
    unsigned int i;

    for (i = 0; i < 11; i++) {
        /* "%s\\" (004a1d88) and "%s%i.bmp" (004a1d7c) are marked in sfx.c. */
        sprintf(baseName, "%s\\", dirName);
        if (fileName) {
            sprintf(name, "%s%i.bmp", fileName, i);
        } else {
            // STRING: LEGORR 0x004a1d74
            sprintf(name, "%i.bmp", i);
        }
        if (i == 10) {
            damageFontGlobs.fontTextMinus =
                sfx_mesh_load_texture_adapter(baseName, name, NULL, NULL);
        } else {
            damageFontGlobs.fontTextDigits[i] =
                sfx_mesh_load_texture_adapter(baseName, name, NULL, NULL);
        }
    }
}

// FUNCTION: LEGORR 0x0040a4b0
void damage_font_display_damage_over_live_object(struct EntitySlot* liveObj,
                                                 unsigned int damage)
{
    unsigned int indices[6] = { 0, 2, 1, 0, 3, 2 };
    DamageFont_Data* data;
    unsigned int i;

    if (damage_font_live_object_check_can_show_damage(liveObj) &&
        damage != 0 && damage < 999 &&
        (data = damage_font_get_next_free()) != NULL) {
        data->timer = 0.0f;
        data->position.y = 0.0f;
        data->position.x = 0.0f;
        data->position.z = -entity_stats_float_at74(liveObj);
        if (data->cont == NULL) {
            data->cont = ddiscene_object_allocate(
                legoobject_type_component_get((struct Lego_Object*)liveObj));
            data->mesh = engine_task_callback_record_allocate_and_initialize(
                *(void**)((char*)data->cont + 4),
                (void*)damage_font_mesh_render_callback, 0x3000, data, 0);
            for (i = 0; i < 4; i++) {
                mesh_record_allocate_and_fill((struct MeshRecordPool*)data->mesh,
                                              4, 2, 3, (const uint16_t*)indices);
            }
        } else {
            ddiscene_scene_object_attach_or_unlink(
                data->cont, legoobject_type_component_get((struct Lego_Object*)liveObj));
            scene_render_lock_toggle(data->cont, FALSE);
        }
        damage_font_set_damage_number(data, damage);
    }
}

// FUNCTION: LEGORR 0x0040a5c0
DamageFont_Data* damage_font_get_next_free(void)
{
    DamageFont_Data* data;
    for (data = damageFontGlobs.instanceTable;
         data < &damageFontGlobs.instanceTable[DAMAGEFONT_MAXSHOWS]; data++) {
        if (!(data->flags & 1)) {
            data->flags |= 1;
            return data;
        }
    }
    return NULL;
}

// FUNCTION: LEGORR 0x0040a5e0
void damage_font_set_damage_number(DamageFont_Data* data, unsigned int damage)
{
    unsigned int i;
    unsigned int digitCount = 0;
    unsigned int n;

    for (i = 0; i < 4; i++) {
        if (damage < 5) {
            render_object_color_vector_set(data->mesh, i, 0.0f, 1.0f, 0.0f, 0);
            render_object_color_vector_set(data->mesh, i, 0.0f, 1.0f, 0.0f, 3);
        } else if (damage < 10) {
            render_object_color_vector_set(data->mesh, i, 1.0f, 1.0f, 0.0f, 0);
            render_object_color_vector_set(data->mesh, i, 1.0f, 1.0f, 0.0f, 3);
        } else {
            render_object_color_vector_set(data->mesh, i, 1.0f, 0.25f, 0.0f, 0);
            render_object_color_vector_set(data->mesh, i, 1.0f, 0.25f, 0.0f, 3);
        }
    }

    for (n = 1; damage >= n; n *= 10) digitCount++;

    for (i = 0; i < digitCount; i++) {
        animated_texture_frame_surface_set(
            data->mesh, i, damageFontGlobs.fontTextDigits[damage % 10]);
        indexed_record_flag6c_bit0_set(data->mesh, i, FALSE);
        damage = (damage - damage % 10) / 10;
    }
    animated_texture_frame_surface_set(data->mesh, i,
                                       damageFontGlobs.fontTextMinus);
    digitCount++;
    for (i = digitCount; i < 4; i++) {
        indexed_record_flag6c_bit0_set(data->mesh, i, TRUE);
    }
    data->digitCount = digitCount;
}

// FUNCTION: LEGORR 0x0040a740
static void damage_font_mesh_render_callback(void* mesh, DamageFont_Data* data,
                                             void* view)
{
    DamageFont_Vector3F verts[4];
    void* camera;
    void* cam;
    DamageFont_Vector3F side;
    float offset;
    unsigned int i;
    float scale;
    DamageFont_Vector3F dir;
    DamageFont_Vector3F camPos;
    float uvs[8] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f };
    DamageFont_Vector3F up;
    float size;

    camera = (void*)(uintptr_t)lego_context_selected_global_pointer_get();
    scale = (data->timer * (1.0f / DAMAGEFONT_MAXTIME)) * 2.0f - -1.0f;
    ddiscene_object_pose_method_dispatch(data->cont, NULL, 0.0f, 0.0f, 1.0f,
                                         0.0f, 1.0f, 0.0f);
    cam = (void*)(uintptr_t)scene_object_query_value_if_ready(view);
    ddiscene_object_facing_pair_get(cam, NULL, (float*)&dir, (float*)&up);
    ddiscene_object_position_get(camera, NULL, (float*)&camPos);

    damage_font_vector3d_cross_product(&side, &up, &dir);
    size = scale * 3.0f;
    damage_font_vector3d_scale(&side, &side, size);
    damage_font_vector3d_scale(&up, &up, size);
    offset = 1.0f - (float)data->digitCount * 0.5f;
    for (i = 0; i < data->digitCount; i++) {
        verts[0].x = data->position.x - side.x * offset;
        verts[0].y = data->position.y - side.y * offset;
        verts[0].z = data->position.z - side.z * offset;
        damage_font_vector3d_add(&verts[1], &verts[0], &side);
        damage_font_vector3d_add(&verts[2], &verts[1], &up);
        damage_font_vector3d_add(&verts[3], &verts[0], &up);
        instance_quad_vertex_data_fill(data->mesh, i, 0, 4, (float*)verts,
                                       (float*)&camPos, (const uint32_t*)uvs);
        offset += 1.0f;
    }
}

// FUNCTION: LEGORR 0x0040aa10
void damage_font_update_all(float elapsed)
{
    unsigned int i;
    for (i = 0; i < DAMAGEFONT_MAXSHOWS; i++) {
        if (damageFontGlobs.instanceTable[i].flags & 1) {
            damage_font_update(&damageFontGlobs.instanceTable[i], elapsed);
        }
    }
}

// FUNCTION: LEGORR 0x0040aa40
void damage_font_update(DamageFont_Data* data, float elapsed)
{
    DamageFont_Vector3F dir;
    float alpha;
    unsigned int i;

    if (data->timer >= DAMAGEFONT_MAXTIME) {
        data->flags &= ~1;
        scene_render_lock_toggle(data->cont, TRUE);
        return;
    }
    dir.x = 0.0f;
    dir.y = 0.0f;
    dir.z = -1.0f;
    damage_font_vector3d_scale(&dir, &dir, elapsed);
    damage_font_vector3d_add(&data->position, &data->position, &dir);
    alpha = 1.0f - data->timer * (1.0f / DAMAGEFONT_MAXTIME);
    for (i = 0; i < data->digitCount; i++) {
        mesh_group_material_values_set(data->mesh, i, alpha, 4);
    }
    data->timer += elapsed;
}

// FUNCTION: LEGORR 0x0040aae0
int damage_font_live_object_check_can_show_damage(struct EntitySlot* liveObj)
{
    if (!(entity_state_flags_get(liveObj) & 0x8000000) &&
        ((DamageFont_LiveObject*)liveObj)->type != 8 &&
        ((DamageFont_LiveObject*)liveObj)->type != 13) {
        return TRUE;
    }
    return FALSE;
}

#ifdef __cplusplus
}
#endif
