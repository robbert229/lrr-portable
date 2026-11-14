// SPDX-License-Identifier: LGPL-3.0-or-later
/* ToolTip.c, original translation unit GAME-046 (finding 505), rebuilt in
 * original shape (ADR 0005; finding 597): text 0046b1d0-0046c030, .rdata
 * 0049e7c0-0049e7d8, .data 004aa64c, literals 004aa650-004aa9c8, and the
 * state in the common region from 0054bf20.
 *
 * The tool tips: one record per type, each a text or an image plus rows of
 * icons. A caller sets a tip's content and activates it every frame the
 * mouse is over its owner; once it has been active for the hover time,
 * ToolTip_Update draws it beside the mouse and plays its sound once.
 *
 * The five functions that followed in the unit map (0046c030-0046c130) are
 * Upgrade.c's part functions (GAME-047, upgrade.c; findings 597 and 617).
 *
 * The port's implementation stays in the port tail tooltip_port.cpp,
 * unannotated. In the matching build tooltip_link_names.h gives this unit's
 * functions the port's names, so every caller reaches them; natively the
 * port tail serves those names. */
#include "tooltip_link_names.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "crt/vc5names.h"
#include "tooltip.h"
#include "tooltip_imports.h"

/* The unit's functions have C linkage in the native C++ compile too. */
#ifdef __cplusplus
extern "C" {
#endif

#define TOOLTIP_TYPE_COUNT 39
#define TOOLTIP_MAXICONS 20

#define TOOLTIP_FLAG_ACTIVE 0x1
#define TOOLTIP_FLAG_ACTIVE2 0x2
#define TOOLTIP_FLAG_UPDATE 0x4
#define TOOLTIP_FLAG_IMAGE 0x8
#define TOOLTIP_FLAG_FLAG10 0x10

/* legoGlobs.flags2's tool tip sound mute (DAT_00556ed0). */
#define TOOLTIP_LEGO_FLAG_NOTOOLTIPSFX 0x100000

/* One tool tip (0x27c bytes). */
typedef struct ToolTip {
    float timer;                     /* +0x000 */
    char textBuffer[512];            /* +0x004 */
    unsigned int textWidth;          /* +0x204 */
    unsigned int textLineCount;      /* +0x208 */
    Image* iconList[TOOLTIP_MAXICONS]; /* +0x20c */
    unsigned int iconCount;          /* +0x25c */
    int iconRowHeight;               /* +0x260 */
    int iconsHeight;                 /* +0x264 */
    int iconRowWidth;                /* +0x268 */
    int iconsWidth;                  /* +0x26c */
    Image* image;                    /* +0x270 */
    int sfxType;                     /* +0x274 */
    unsigned int flags;              /* +0x278 */
} ToolTip;

/* The unit's state (0x61c4 bytes). Each colour channel holds its normal,
 * highlight and shadow value: the box's three colours are read across the
 * channels. */
typedef struct ToolTip_Globs {
    struct TextureSurface* font;     /* 0054bf20 */
    unsigned int fontHeight;         /* 0054bf24 */
    unsigned int borderSize;         /* 0054bf28 */
    unsigned int offset;             /* 0054bf2c */
    unsigned int appWidth;           /* 0054bf30 */
    unsigned int appHeight;          /* 0054bf34 */
    unsigned int offsetY;            /* 0054bf38 */
    float hoverTime;                 /* 0054bf3c */
    float red[3];                    /* 0054bf40 */
    float green[3];                  /* 0054bf4c */
    float blue[3];                   /* 0054bf58 */
    const char* toolTipName[TOOLTIP_TYPE_COUNT]; /* 0054bf64 */
    ToolTip toolTip[TOOLTIP_TYPE_COUNT]; /* 0054c000 */
} ToolTip_Globs;

// GLOBAL: LEGORR 0x004aa64c
static int toolTip_sfxReady = 1;

// GLOBAL: LEGORR 0x0054bf20
ToolTip_Globs toolTipGlobs;

// FUNCTION: LEGORR 0x0046b1d0
void tooltip_initialise(struct TextureSurface* font, unsigned int borderSize,
                        unsigned int offset, float hoverTime,
                        unsigned int appWidth, unsigned int appHeight,
                        unsigned int offsetY, float red, float green,
                        float blue)
{
    unsigned int i;

    // STRING: LEGORR 0x004aa9c8
    toolTipGlobs.toolTipName[1] = "ToolTip_RadarBlock";
    // STRING: LEGORR 0x004aa9b4
    toolTipGlobs.toolTipName[2] = "ToolTip_UnitSelect";
    // STRING: LEGORR 0x004aa99c
    toolTipGlobs.toolTipName[3] = "ToolTip_Construction";
    // STRING: LEGORR 0x004aa984
    toolTipGlobs.toolTipName[4] = "ToolTip_InterfaceMenu";
    // STRING: LEGORR 0x004aa964
    toolTipGlobs.toolTipName[5] = "ToolTip_InterfaceMenuBackButton";
    // STRING: LEGORR 0x004aa950
    toolTipGlobs.toolTipName[6] = "ToolTip_MapBlock";
    // STRING: LEGORR 0x004aa93c
    toolTipGlobs.toolTipName[7] = "ToolTip_Priority";
    // STRING: LEGORR 0x004aa920
    toolTipGlobs.toolTipName[8] = "ToolTip_InfoMenuContinue";
    // STRING: LEGORR 0x004aa900
    toolTipGlobs.toolTipName[9] = "ToolTip_InfoMenuDisableFuture";
    // STRING: LEGORR 0x004aa8ec
    toolTipGlobs.toolTipName[10] = "ToolTip_RadarToggle";
    // STRING: LEGORR 0x004aa8d4
    toolTipGlobs.toolTipName[11] = "ToolTip_RadarObjectView";
    // STRING: LEGORR 0x004aa8c0
    toolTipGlobs.toolTipName[12] = "ToolTip_RadarZoomIn";
    // STRING: LEGORR 0x004aa8a8
    toolTipGlobs.toolTipName[13] = "ToolTip_RadarZoomOut";
    // STRING: LEGORR 0x004aa890
    toolTipGlobs.toolTipName[14] = "ToolTip_RadarMapView";
    // STRING: LEGORR 0x004aa878
    toolTipGlobs.toolTipName[15] = "ToolTip_InfoDockGoto";
    // STRING: LEGORR 0x004aa860
    toolTipGlobs.toolTipName[16] = "ToolTip_InfoDockClose";
    // STRING: LEGORR 0x004aa844
    toolTipGlobs.toolTipName[17] = "ToolTip_TopPanelCallToArms";
    // STRING: LEGORR 0x004aa82c
    toolTipGlobs.toolTipName[18] = "ToolTip_TopPanelInfo";
    // STRING: LEGORR 0x004aa814
    toolTipGlobs.toolTipName[19] = "ToolTip_TopPanelOptions";
    // STRING: LEGORR 0x004aa7f8
    toolTipGlobs.toolTipName[20] = "ToolTip_TopPanelPriorities";
    // STRING: LEGORR 0x004aa7e0
    toolTipGlobs.toolTipName[21] = "ToolTip_PriorityDisable";
    // STRING: LEGORR 0x004aa7c8
    toolTipGlobs.toolTipName[22] = "ToolTip_PriorityUpOne";
    // STRING: LEGORR 0x004aa7b0
    toolTipGlobs.toolTipName[23] = "ToolTip_PriorityReset";
    // STRING: LEGORR 0x004aa794
    toolTipGlobs.toolTipName[24] = "ToolTip_CamControlZoomIn";
    // STRING: LEGORR 0x004aa778
    toolTipGlobs.toolTipName[25] = "ToolTip_CamControlZoomOut";
    // STRING: LEGORR 0x004aa760
    toolTipGlobs.toolTipName[26] = "ToolTip_CamControlCycle";
    // STRING: LEGORR 0x004aa744
    toolTipGlobs.toolTipName[27] = "ToolTip_CamControlRotate";
    // STRING: LEGORR 0x004aa730
    toolTipGlobs.toolTipName[28] = "ToolTip_SideBar_Ore";
    // STRING: LEGORR 0x004aa714
    toolTipGlobs.toolTipName[29] = "ToolTip_SideBar_Crystals";
    // STRING: LEGORR 0x004aa704
    toolTipGlobs.toolTipName[30] = "ToolTip_Close";
    // STRING: LEGORR 0x004aa6ec
    toolTipGlobs.toolTipName[31] = "ToolTip_PreviousMessage";
    // STRING: LEGORR 0x004aa6d8
    toolTipGlobs.toolTipName[32] = "ToolTip_NextMessage";
    // STRING: LEGORR 0x004aa6c8
    toolTipGlobs.toolTipName[33] = "ToolTip_More";
    // STRING: LEGORR 0x004aa6b8
    toolTipGlobs.toolTipName[34] = "ToolTip_Back";
    // STRING: LEGORR 0x004aa694
    toolTipGlobs.toolTipName[35] = "ToolTip_CamControlCycleMinifigures";
    // STRING: LEGORR 0x004aa680
    toolTipGlobs.toolTipName[36] = "ToolTip_Reward_Save";
    // STRING: LEGORR 0x004aa668
    toolTipGlobs.toolTipName[37] = "ToolTip_Reward_Advance";
    // STRING: LEGORR 0x004aa650
    toolTipGlobs.toolTipName[38] = "ToolTip_FrontEnd_Back";

    toolTipGlobs.font = font;
    toolTipGlobs.fontHeight = object_field_be4_dword_get(font);
    toolTipGlobs.borderSize = borderSize;
    toolTipGlobs.offset = offset;
    toolTipGlobs.appWidth = appWidth;
    toolTipGlobs.appHeight = appHeight;
    toolTipGlobs.offsetY = offsetY;
    toolTipGlobs.hoverTime = hoverTime * 25.0f;

    toolTipGlobs.red[0] = red;
    toolTipGlobs.green[0] = green;
    toolTipGlobs.blue[0] = blue;
    toolTipGlobs.red[1] = red + (red * 0.4f);
    toolTipGlobs.green[1] = green + (green * 0.4f);
    toolTipGlobs.blue[1] = blue + (blue * 0.4f);
    toolTipGlobs.red[2] = red - (red * 0.4f);
    toolTipGlobs.green[2] = green - (green * 0.4f);
    toolTipGlobs.blue[2] = blue - (blue * 0.4f);

    for (i = 0; i < 3; i++) {
        if (toolTipGlobs.red[i] > 1.0f) toolTipGlobs.red[i] = 1.0f;
        if (toolTipGlobs.green[i] > 1.0f) toolTipGlobs.green[i] = 1.0f;
        if (toolTipGlobs.blue[i] > 1.0f) toolTipGlobs.blue[i] = 1.0f;
        if (toolTipGlobs.red[i] < 0.0f) toolTipGlobs.red[i] = 0.0f;
        if (toolTipGlobs.green[i] < 0.0f) toolTipGlobs.green[i] = 0.0f;
        if (toolTipGlobs.blue[i] < 0.0f) toolTipGlobs.blue[i] = 0.0f;
    }

    memset(toolTipGlobs.toolTip, 0, sizeof(toolTipGlobs.toolTip));
}

// FUNCTION: LEGORR 0x0046b4d0
int tooltip_get_type(const char* toolTipName, int* toolTipType)
{
    unsigned int i;

    for (i = 1; i < TOOLTIP_TYPE_COUNT; i++) {
        if (stricmp(toolTipName, toolTipGlobs.toolTipName[i]) == 0) {
            *toolTipType = i;
            return 1;
        }
    }
    return 0;
}

// FUNCTION: LEGORR 0x0046b520
void tooltip_set_content(int toolTipType, const char* msg, ...)
{
    ToolTip* toolTip = &toolTipGlobs.toolTip[toolTipType];
    char imageName[260];
    char buff[512];
    char* s;
    char* t;
    int i;
    va_list args;

    va_start(args, msg);
    vsprintf(buff, msg, args);
    va_end(args);

    if (buff[0] == '@') {
        for (i = 1; buff[i] != '\0'; i++) imageName[i - 1] = buff[i];
        imageName[i - 1] = '\0';
        toolTip->image = image_load_bmp_scaled(imageName, 0, 0);
        toolTip->flags |= TOOLTIP_FLAG_IMAGE;
    }
    else {
        for (t = toolTip->textBuffer, s = buff; *s != '\0'; t++, s++) {
            if (*s == '\\' && s[1] == 'n') {
                *t = '\n';
                s++;
            }
            else if (*s == '_') *t = ' ';
            else *t = *s;
        }
        *t = '\0';
        formatted_text_printf(toolTipGlobs.font, &toolTip->textWidth,
                              &toolTip->textLineCount, toolTip->textBuffer);
        toolTip->flags &= ~TOOLTIP_FLAG_IMAGE;
    }

    toolTip->iconCount = 0;
    toolTip->iconsHeight = 0;
    toolTip->iconRowHeight = 0;
    toolTip->iconsWidth = 0;
    toolTip->iconRowWidth = 0;
    toolTip->flags |= TOOLTIP_FLAG_ACTIVE | TOOLTIP_FLAG_ACTIVE2;
}

// FUNCTION: LEGORR 0x0046b660
void tooltip_add_icon(int toolTipType, Image* image)
{
    ToolTip* toolTip = &toolTipGlobs.toolTip[toolTipType];

    if (image != NULL) {
        if ((int)image->height > toolTip->iconRowHeight) {
            toolTip->iconsHeight -= toolTip->iconRowHeight;
            toolTip->iconRowHeight = image->height;
            toolTip->iconsHeight += toolTip->iconRowHeight;
        }
        toolTip->iconRowWidth += image->width;
        if (toolTip->iconRowWidth > toolTip->iconsWidth)
            toolTip->iconsWidth = toolTip->iconRowWidth;
    }
    else {
        toolTip->iconRowHeight = 0;
        toolTip->iconRowWidth = 0;
    }
    toolTip->iconList[toolTip->iconCount++] = image;
}

// FUNCTION: LEGORR 0x0046b700
void tooltip_set_sfx(int toolTipType, int sfxType)
{
    ToolTip* toolTip = &toolTipGlobs.toolTip[toolTipType];

    toolTip->sfxType = sfxType;
    toolTip->flags |= TOOLTIP_FLAG_ACTIVE | TOOLTIP_FLAG_ACTIVE2;
}

// FUNCTION: LEGORR 0x0046b730
void tooltip_set_flag10(int toolTipType, int on)
{
    ToolTip* toolTip = &toolTipGlobs.toolTip[toolTipType];

    if (on) toolTip->flags |= TOOLTIP_FLAG_FLAG10;
    else toolTip->flags &= ~TOOLTIP_FLAG_FLAG10;
}

// FUNCTION: LEGORR 0x0046b770
void tooltip_activate(int toolTipType)
{
    ToolTip* toolTip = &toolTipGlobs.toolTip[toolTipType];

    toolTip->flags |= TOOLTIP_FLAG_UPDATE;
}

// FUNCTION: LEGORR 0x0046b7a0
void tooltip_reset_timer(int toolTipType)
{
    ToolTip* toolTip = &toolTipGlobs.toolTip[toolTipType];

    toolTip->timer = toolTipGlobs.hoverTime;
}

/* legoGlobs' fields (lego_globs.h, finding 653), included before their
 * first use so that the functions above keep their symbol counter
 * (finding 538). */
#include "lego_globs.h"

// FUNCTION: LEGORR 0x0046b7c0
void tooltip_update(unsigned int mouseX, unsigned int mouseY,
                    float elapsedAbs)
{
    int i;
    int drawn = 0;
    int updated = 0;

    for (i = 1; i < TOOLTIP_TYPE_COUNT; i++) {
        if (toolTipGlobs.toolTip[i].flags & (TOOLTIP_FLAG_ACTIVE | TOOLTIP_FLAG_ACTIVE2)) {
            if ((toolTipGlobs.toolTip[i].flags & TOOLTIP_FLAG_UPDATE) && !updated) {
                ToolTip* toolTip = &toolTipGlobs.toolTip[i];
                if (toolTip->timer < toolTipGlobs.hoverTime || drawn) {
                    toolTip->timer += elapsedAbs;
                    toolTip_sfxReady = 1;
                    updated = 1;
                }
                else {
                    if (toolTip->sfxType && toolTip_sfxReady &&
                        !(s_panel_highlight_flags & TOOLTIP_LEGO_FLAG_NOTOOLTIPSFX)) {
                        toolTip_sfxReady = !sound_submit_with_global_cooldown(toolTip->sfxType, NULL);
                    }
                    if (toolTip->textBuffer) tooltip_draw(toolTip, mouseX, mouseY);
                    toolTip->timer += elapsedAbs;
                    drawn = 1;
                    updated = 1;
                }
            }
            else toolTipGlobs.toolTip[i].timer = 0.0f;

            toolTipGlobs.toolTip[i].flags &= ~TOOLTIP_FLAG_UPDATE;
        }
    }
}

// FUNCTION: LEGORR 0x0046b8b0
void tooltip_draw_box(Area2F box, ColourRGBF rgbNormal,
                      ColourRGBF rgbHighlight, ColourRGBF rgbShadow,
                      int dark)
{
    Point2F fromHi[10];
    Point2F toHi[10];
    Point2F fromLo[10];
    Point2F toLo[10];
    unsigned int lineCount = 0;
    unsigned int i;

    for (i = 0; i < toolTipGlobs.borderSize; i++) {
        fromHi[lineCount].x = box.x + i;
        fromHi[lineCount].y = box.y + i;
        toHi[lineCount].x = fromHi[lineCount].x;
        toHi[lineCount].y = (box.y + box.height) - 2.0f - i;
        fromLo[lineCount].x = fromHi[lineCount].x;
        fromLo[lineCount].y = (box.y + box.height) - 1.0f - i;
        toLo[lineCount].x = (box.x + box.width) - 2.0f - i;
        toLo[lineCount].y = fromLo[lineCount].y;
        lineCount++;
        fromHi[lineCount].x = (box.x + box.width) - 1.0f - i;
        fromHi[lineCount].y = fromHi[lineCount - 1].y;
        toHi[lineCount].x = fromHi[lineCount - 1].x - -1.0f;
        toHi[lineCount].y = fromHi[lineCount].y;
        fromLo[lineCount].x = fromHi[lineCount].x;
        fromLo[lineCount].y = fromLo[lineCount - 1].y;
        toLo[lineCount].x = fromLo[lineCount].x;
        toLo[lineCount].y = fromHi[lineCount].y - -1.0f;
        lineCount++;
    }

    if (dark) {
        surface_fill_pixel_spans(&box, 1, rgbNormal.red, rgbNormal.green, rgbNormal.blue, DRAW_EFFECT_HALF_TRANS);
        surface_draw_polyline(fromHi, toHi, lineCount, rgbHighlight.red, rgbHighlight.green, rgbHighlight.blue, DRAW_EFFECT_NONE);
        surface_draw_polyline(fromLo, toLo, lineCount, rgbShadow.red, rgbShadow.green, rgbShadow.blue, DRAW_EFFECT_NONE);
    }
    else {
        surface_fill_pixel_spans(&box, 1, rgbNormal.red, rgbNormal.green, rgbNormal.blue, DRAW_EFFECT_NONE);
        surface_draw_polyline(fromHi, toHi, lineCount, rgbHighlight.red, rgbHighlight.green, rgbHighlight.blue, DRAW_EFFECT_NONE);
        surface_draw_polyline(fromLo, toLo, lineCount, rgbShadow.red, rgbShadow.green, rgbShadow.blue, DRAW_EFFECT_NONE);
    }
}

// FUNCTION: LEGORR 0x0046bc30
void tooltip_draw(ToolTip* toolTip, unsigned int mouseX, unsigned int mouseY)
{
    Area2F box;
    ColourRGBF rgbNormal;
    ColourRGBF rgbHighlight;
    ColourRGBF rgbShadow;
    Point2F pos;
    unsigned int width;
    unsigned int height;
    unsigned int x;
    unsigned int rowHeight;
    unsigned int i;

    rgbNormal.red = toolTipGlobs.red[0];
    rgbNormal.green = toolTipGlobs.green[0];
    rgbNormal.blue = toolTipGlobs.blue[0];
    rgbHighlight.red = toolTipGlobs.red[1];
    rgbHighlight.green = toolTipGlobs.green[1];
    rgbHighlight.blue = toolTipGlobs.blue[1];
    rgbShadow.red = toolTipGlobs.red[2];
    rgbShadow.green = toolTipGlobs.green[2];
    rgbShadow.blue = toolTipGlobs.blue[2];

    height = width = (toolTipGlobs.borderSize + toolTipGlobs.offset) * 2;
    if (toolTip->flags & TOOLTIP_FLAG_IMAGE) {
        width += toolTip->image->width;
        height += toolTip->image->height;
    }
    else {
        width += (toolTip->textWidth > (unsigned int)toolTip->iconsWidth) ? toolTip->textWidth : toolTip->iconsWidth;
        height += (toolTip->textLineCount * toolTipGlobs.fontHeight) + toolTip->iconsHeight;
    }

    if (toolTip->flags & TOOLTIP_FLAG_FLAG10) {
        if ((float)mouseX - -32.0f > (float)toolTipGlobs.appWidth) box.x = (float)(toolTipGlobs.appWidth - width);
        else box.x = ((float)mouseX - -32.0f) - (float)width;
    }
    else {
        if (mouseX + width > toolTipGlobs.appWidth) box.x = (float)(toolTipGlobs.appWidth - width);
        else box.x = (float)mouseX;
    }
    box.y = (float)(toolTipGlobs.offsetY + mouseY);
    box.height = (float)height;
    if (box.y + box.height > (float)toolTipGlobs.appHeight) box.y = (float)(mouseY - height) - 1.0f;
    box.width = (float)width;

    tooltip_draw_box(box, rgbNormal, rgbHighlight, rgbShadow, 0);

    if (toolTip->flags & TOOLTIP_FLAG_IMAGE) {
        pos.x = box.x + toolTipGlobs.borderSize + toolTipGlobs.offset;
        pos.y = box.y + toolTipGlobs.borderSize + toolTipGlobs.offset;
        image_display_scaled(toolTip->image, NULL, &pos, NULL);
    }
    else {
        mouseX = toolTip->textLineCount * toolTipGlobs.fontHeight;
        x = 0;
        rowHeight = 0;
        font_print_f(toolTipGlobs.font,
                     (int)(box.x + toolTipGlobs.borderSize + toolTipGlobs.offset),
                     (int)(box.y + toolTipGlobs.borderSize + toolTipGlobs.offset),
                     "%s", toolTip->textBuffer);
        for (i = 0; i < toolTip->iconCount; i++) {
            if (toolTip->iconList[i]) {
                pos.x = x + (box.x + toolTipGlobs.borderSize + toolTipGlobs.offset);
                pos.y = mouseX + (box.y + toolTipGlobs.borderSize + toolTipGlobs.offset);
                image_display_scaled(toolTip->iconList[i], NULL, &pos, NULL);
                if ((int)toolTip->iconList[i]->height > (int)rowHeight) rowHeight = toolTip->iconList[i]->height;
                x += toolTip->iconList[i]->width;
            }
            else {
                mouseX += rowHeight;
                rowHeight = 0;
                x = 0;
            }
        }
    }
}

#ifdef __cplusplus
}
#endif
