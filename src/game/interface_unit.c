// SPDX-License-Identifier: LGPL-3.0-or-later
/* Interface.c — original translation unit GAME-020 (text 0041a300-0041f870,
 * .data 004a2b58-004a2b5c, literals 004a2b60-004a354c, .bss
 * 004dcd50-004de204; finding 505).
 *
 * Rebuilt in the original's shape (ADR 0005): one file for the unit, its
 * functions in original order, its state in one typed globals struct laid
 * out as the original .bss, and the C-compatible subset so the matching
 * build compiles it as C. The native port compiles it as C++.
 *
 * The port's interface is a different model (hooks, its own state words and
 * the native HUD), so its implementations stay, unannotated, in the port
 * tail game/interface_port.cpp; native callers keep calling them (finding
 * 560). Callee contracts that this reconstruction has not converted yet are
 * used as they are, or through the thin adapters that game/interface_callees.h
 * lists. */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "interface_unit.h"
#include "interface_callees.h"
#include "advisor.h"
#include "frontend.h"
#include "lego.h"
#include "message.h"
#include "panel_unit.h"
#include "terrain_map.h"
#include "tutorial_script.h"
#include "crt/crt.h"
#include "crt/vc5names.h"
#include "gods98/config.h"
#include "gods98/draw.h"
#include "gods98/font_api.h"
#include "gods98/engine.h"
#include "gods98/image_adapters.h"
#include "gods98/images.h"
#include "gods98/keys.h"
#include "gods98/sound.h"
#include "gods98/utils.h"

/* Port-only code: the entries that keep the port's names delegate to the
 * port's model in the native build (finding 564). The matching build
 * compiles the original bodies only (ADR 0006). */
#if !(defined(_MSC_VER) && _MSC_VER == 1100)
#define INTERFACE_PORT 1
#endif

/* Lego.c and Info.c read the menu state and the font directly, so the struct
 * is external. The zero initialiser keeps it in this object's .bss rather
 * than the common region (finding 554). */
// GLOBAL: LEGORR 0x004dcd58
InterfaceGlobs interfaceGlobs = { 0 };

// FUNCTION: LEGORR 0x0041a300
void interface_initialise(unsigned int x, unsigned int y,
                          struct TextureSurface* font)
{
    tooltip_set_persistent(4, 1);

    interfaceGlobs.currMenuPosition.x = (float)x;
    interfaceGlobs.currMenuPosition.y = (float)y;
    interfaceGlobs.font = font;

    // STRING: LEGORR 0x004a34b4
    interfaceGlobs.menuItemName[0] = "Interface_MenuItem_BackToDefault";
    // STRING: LEGORR 0x004a3494
    interfaceGlobs.menuItemName[1] = "Interface_MenuItem_TeleportMan";
    // STRING: LEGORR 0x004a3470
    interfaceGlobs.menuItemName[2] = "Interface_MenuItem_BuildBuilding";
    // STRING: LEGORR 0x004a3448
    interfaceGlobs.menuItemName[3] = "Interface_MenuItem_BuildSmallVehicle";
    // STRING: LEGORR 0x004a3420
    interfaceGlobs.menuItemName[4] = "Interface_MenuItem_BuildLargeVehicle";
    // STRING: LEGORR 0x004a3404
    interfaceGlobs.menuItemName[5] = "Interface_MenuItem_LayPath";
    // STRING: LEGORR 0x004a33e4
    interfaceGlobs.menuItemName[7] = "Interface_MenuItem_RepairLava";
    // STRING: LEGORR 0x004a33c0
    interfaceGlobs.menuItemName[8] = "Interface_MenuItem_GeologistTest";
    // STRING: LEGORR 0x004a33a0
    interfaceGlobs.menuItemName[9] = "Interface_MenuItem_ClearRubble";
    // STRING: LEGORR 0x004a3388
    interfaceGlobs.menuItemName[10] = "Interface_MenuItem_Dam";
    // STRING: LEGORR 0x004a3368
    interfaceGlobs.menuItemName[17] = "Interface_MenuItem_SelectMan";
    // STRING: LEGORR 0x004a3348
    interfaceGlobs.menuItemName[32] = "Interface_MenuItem_DeleteMan";
    // STRING: LEGORR 0x004a3324
    interfaceGlobs.menuItemName[41] = "Interface_MenuItem_DeleteVehicle";
    // STRING: LEGORR 0x004a3300
    interfaceGlobs.menuItemName[40] = "Interface_MenuItem_DeleteBuilding";
    // STRING: LEGORR 0x004a32e4
    interfaceGlobs.menuItemName[43] = "Interface_MenuItem_Attack";
    // STRING: LEGORR 0x004a32c0
    interfaceGlobs.menuItemName[18] = "Interface_MenuItem_SelectVehicle";
    // STRING: LEGORR 0x004a329c
    interfaceGlobs.menuItemName[22] = "Interface_MenuItem_UnLoadVehicle";
    // STRING: LEGORR 0x004a3278
    interfaceGlobs.menuItemName[23] = "Interface_MenuItem_UnLoadMinifigure";
    // STRING: LEGORR 0x004a3254
    interfaceGlobs.menuItemName[20] = "Interface_MenuItem_VehiclePickUp";
    // STRING: LEGORR 0x004a3230
    interfaceGlobs.menuItemName[21] = "Interface_MenuItem_MinifigurePickUp";
    // STRING: LEGORR 0x004a3214
    interfaceGlobs.menuItemName[25] = "Interface_MenuItem_GetOut";
    // STRING: LEGORR 0x004a31f8
    interfaceGlobs.menuItemName[24] = "Interface_MenuItem_GetIn";
    // STRING: LEGORR 0x004a31dc
    interfaceGlobs.menuItemName[26] = "Interface_MenuItem_GotoDock";
    // STRING: LEGORR 0x004a31c4
    interfaceGlobs.menuItemName[11] = "Interface_MenuItem_Dig";
    // STRING: LEGORR 0x004a31a4
    interfaceGlobs.menuItemName[12] = "Interface_MenuItem_Reinforce";
    // STRING: LEGORR 0x004a3188
    interfaceGlobs.menuItemName[13] = "Interface_MenuItem_Dynamite";
    // STRING: LEGORR 0x004a3168
    interfaceGlobs.menuItemName[64] = "Interface_MenuItem_UpgradeMan";
    // STRING: LEGORR 0x004a314c
    interfaceGlobs.menuItemName[31] = "Interface_MenuItem_GoFeed";
    // STRING: LEGORR 0x004a3128
    interfaceGlobs.menuItemName[45] = "Interface_MenuItem_GotoFirstPerson";
    // STRING: LEGORR 0x004a3104
    interfaceGlobs.menuItemName[46] = "Interface_MenuItem_GotoSecondPerson";
    // STRING: LEGORR 0x004a30e4
    interfaceGlobs.menuItemName[57] = "Interface_MenuItem_TrainSkill";
    // STRING: LEGORR 0x004a30c8
    interfaceGlobs.menuItemName[34] = "Interface_MenuItem_PowerOn";
    // STRING: LEGORR 0x004a30ac
    interfaceGlobs.menuItemName[35] = "Interface_MenuItem_PowerOff";
    // STRING: LEGORR 0x004a3088
    interfaceGlobs.menuItemName[65] = "Interface_MenuItem_UpgradeBuilding";
    // STRING: LEGORR 0x004a306c
    interfaceGlobs.menuItemName[36] = "Interface_MenuItem_Repair";
    // STRING: LEGORR 0x004a3048
    interfaceGlobs.menuItemName[19] = "Interface_MenuItem_SelectBuilding";
    // STRING: LEGORR 0x004a3028
    interfaceGlobs.menuItemName[44] = "Interface_MenuItem_GotoTopView";
    // STRING: LEGORR 0x004a300c
    interfaceGlobs.menuItemName[48] = "Interface_MenuItem_GetTool";
    // STRING: LEGORR 0x004a2ff0
    interfaceGlobs.menuItemName[49] = "Interface_MenuItem_GetDrill";
    // STRING: LEGORR 0x004a2fd4
    interfaceGlobs.menuItemName[50] = "Interface_MenuItem_GetSpade";
    // STRING: LEGORR 0x004a2fb4
    interfaceGlobs.menuItemName[51] = "Interface_MenuItem_GetHammer";
    // STRING: LEGORR 0x004a2f94
    interfaceGlobs.menuItemName[52] = "Interface_MenuItem_GetSpanner";
    // STRING: LEGORR 0x004a2f74
    interfaceGlobs.menuItemName[58] = "Interface_MenuItem_TrainDriver";
    // STRING: LEGORR 0x004a2f50
    interfaceGlobs.menuItemName[59] = "Interface_MenuItem_TrainEngineer";
    // STRING: LEGORR 0x004a2f2c
    interfaceGlobs.menuItemName[60] = "Interface_MenuItem_TrainGeologist";
    // STRING: LEGORR 0x004a2f0c
    interfaceGlobs.menuItemName[61] = "Interface_MenuItem_TrainPilot";
    // STRING: LEGORR 0x004a2eec
    interfaceGlobs.menuItemName[62] = "Interface_MenuItem_TrainSailor";
    // STRING: LEGORR 0x004a2ec8
    interfaceGlobs.menuItemName[63] = "Interface_MenuItem_TrainDynamite";
    // STRING: LEGORR 0x004a2eac
    interfaceGlobs.menuItemName[73] = "Interface_MenuItem_Build";
    // STRING: LEGORR 0x004a2e8c
    interfaceGlobs.menuItemName[14] = "Interface_MenuItem_PlaceFence";
    // STRING: LEGORR 0x004a2e60
    interfaceGlobs.menuItemName[37] = "Interface_MenuItem_MakeTeleporterPrimary";
    // STRING: LEGORR 0x004a2e40
    interfaceGlobs.menuItemName[15] = "Interface_MenuItem_DeselectDig";
    // STRING: LEGORR 0x004a2e1c
    interfaceGlobs.menuItemName[72] = "Interface_MenuItem_ClearSelection";
    // STRING: LEGORR 0x004a2dfc
    interfaceGlobs.menuItemName[71] = "Interface_MenuItem_Encyclopedia";
    // STRING: LEGORR 0x004a2ddc
    interfaceGlobs.menuItemName[38] = "Interface_MenuItem_EjectCrystal";
    // STRING: LEGORR 0x004a2dc0
    interfaceGlobs.menuItemName[39] = "Interface_MenuItem_EjectOre";
    // STRING: LEGORR 0x004a2da0
    interfaceGlobs.menuItemName[47] = "Interface_MenuItem_TrackObject";
    // STRING: LEGORR 0x004a2d80
    interfaceGlobs.menuItemName[27] = "Interface_MenuItem_LegoManGoto";
    // STRING: LEGORR 0x004a2d60
    interfaceGlobs.menuItemName[30] = "Interface_MenuItem_LegoManDig";
    // STRING: LEGORR 0x004a2d40
    interfaceGlobs.menuItemName[28] = "Interface_MenuItem_VehicleGoto";
    // STRING: LEGORR 0x004a2d20
    interfaceGlobs.menuItemName[29] = "Interface_MenuItem_VehicleDig";
    // STRING: LEGORR 0x004a2d00
    interfaceGlobs.menuItemName[6] = "Interface_MenuItem_RemovePath";
    // STRING: LEGORR 0x004a2cdc
    interfaceGlobs.menuItemName[66] = "Interface_MenuItem_UpgradeVehicle";
    // STRING: LEGORR 0x004a2cb8
    interfaceGlobs.menuItemName[67] = "Interface_MenuItem_UpgradeEngine";
    // STRING: LEGORR 0x004a2c98
    interfaceGlobs.menuItemName[68] = "Interface_MenuItem_UpgardeDrill";
    // STRING: LEGORR 0x004a2c78
    interfaceGlobs.menuItemName[69] = "Interface_MenuItem_UpgardeScan";
    // STRING: LEGORR 0x004a2c58
    interfaceGlobs.menuItemName[70] = "Interface_MenuItem_UpgardeCarry";
    // STRING: LEGORR 0x004a2c30
    interfaceGlobs.menuItemName[42] = "Interface_MenuItem_DeleteElectricFence";
    // STRING: LEGORR 0x004a2c14
    interfaceGlobs.menuItemName[53] = "Interface_MenuItem_GetLaser";
    // STRING: LEGORR 0x004a2bf4
    interfaceGlobs.menuItemName[54] = "Interface_MenuItem_GetPusherGun";
    // STRING: LEGORR 0x004a2bd0
    interfaceGlobs.menuItemName[55] = "Interface_MenuItem_GetFreezerGun";
    // STRING: LEGORR 0x004a2bac
    interfaceGlobs.menuItemName[56] = "Interface_MenuItem_GetBirdScarer";
    // STRING: LEGORR 0x004a2b88
    interfaceGlobs.menuItemName[33] = "Interface_MenuItem_DropBirdScarer";
    // STRING: LEGORR 0x004a2b60
    interfaceGlobs.menuItemName[16] = "Interface_MenuItem_CancelConstruction";

    interface_init_menus();
    interface_set_slide_positions(x, y, s_display_w + 10, y, 750.0f);
    interface_reset_menu();
}

// FUNCTION: LEGORR 0x0041a660
void interface_init_menus(void)
{
    interface_set_menu_items(0, 4, 1, 2, 3, 4);
    interface_set_menu_items(1, 3, 5, 6, 14);
    interface_set_menu_items(2, 1, 7);
    interface_set_menu_items(3, 1, 14);
    interface_set_menu_items(4, 1, 16);
    interface_set_menu_items(5, 2, 9, 14);
    interface_set_menu_items(7, 4, 11, 12, 13, 15);
    interface_set_menu_items(8, 2, 43, 47);
    interface_set_menu_items(10, 7, 22, 20, 66, 25, 45, 46, 41);
    interface_set_menu_items(11, 6, 22, 20, 25, 45, 46, 26);
    interface_set_menu_items(12, 2, 24, 41);
    interface_set_menu_items(9, 10, 31, 23, 21, 48, 33, 64, 57, 45, 46, 32);
    interface_set_menu_items(13, 4, 36, 34, 65, 40);
    interface_set_menu_items(14, 1, 42);
    interface_set_menu_items(15, 3, 44, 46, 45);
    interface_set_menu_items(16, 8, 49, 50, 51, 52, 53, 54, 55, 56);
    interface_set_menu_items(17, 6, 58, 59, 60, 61, 62, 63);
    interface_set_menu_items(18, 4, 67, 68, 69, 70);
}

// FUNCTION: LEGORR 0x0041a7d0
void interface_set_menu_items(int menu, unsigned int count, ...)
{
    int items[30];
    int* item;
    va_list args;

    interfaceGlobs.menus[menu].count = count;
    interfaceGlobs.menus[menu].items = (int*)malloc(sizeof(int) * count);

    va_start(args, count);
    item = items;
    for (; count != 0; count--) {
        *item++ = va_arg(args, int);
    }
    va_end(args);

    memcpy(interfaceGlobs.menus[menu].items, items,
           sizeof(int) * interfaceGlobs.menus[menu].count);
}

// FUNCTION: LEGORR 0x0041a850
void interface_shutdown(void)
{
    InterfaceMenu* menu;

    for (menu = interfaceGlobs.menus;
         menu < interfaceGlobs.menus + INTERFACE_MENU_COUNT; menu++) {
        if (menu->items != NULL) {
            free(menu->items);
        }
    }

    free(interfaceGlobs.vehicleItemIcons);
    free(interfaceGlobs.vehicleItemIconsDisabled);
    free(interfaceGlobs.vehicleItemIconsPressed);
    free(interfaceGlobs.vehicleItemClicks);
    free(interfaceGlobs.vehicleItemF2keys);
    free(interfaceGlobs.vehicleItemFlags);
    free(interfaceGlobs.buildingItemIcons);
    free(interfaceGlobs.buildingItemIconsDisabled);
    free(interfaceGlobs.buildingItemIconsPressed);
    free(interfaceGlobs.buildingItemClicks);
    free(interfaceGlobs.buildingItemF2keys);
    free(interfaceGlobs.buildingItemFlags);
}

// FUNCTION: LEGORR 0x0041a920
void interface_reset(void)
{
    interfaceGlobs.flags = 0;
    memset(interfaceGlobs.menuItemClicks, 0,
           sizeof(interfaceGlobs.menuItemClicks));
    memset(interfaceGlobs.menuItemFlags, 0,
           sizeof(interfaceGlobs.menuItemFlags));
    memset(interfaceGlobs.vehicleItemClicks, 0,
           sizeof(unsigned int) * LEGO_VEHICLE_TYPE_COUNT);
    memset(interfaceGlobs.vehicleItemFlags, 0,
           sizeof(unsigned int) * LEGO_VEHICLE_TYPE_COUNT);
    memset(interfaceGlobs.buildingItemClicks, 0,
           sizeof(unsigned int) * LEGO_BUILDING_TYPE_COUNT);
    memset(interfaceGlobs.buildingItemFlags, 0,
           sizeof(unsigned int) * LEGO_BUILDING_TYPE_COUNT);
    memset(interfaceGlobs.blockPermissions, 0,
           sizeof(interfaceGlobs.blockPermissions));
    interface_reset_menu();
}

// FUNCTION: LEGORR 0x0041a990
void interface_reset_menu(void)
{
    interfaceGlobs.currMenuType = 0;
    interfaceGlobs.currMenuPosition.x = interfaceGlobs.slideInPosition.x;
    interfaceGlobs.flags |= INTERFACE_FLAG_OPEN | INTERFACE_FLAG_IN;
}

// FUNCTION: LEGORR 0x0041a9c0
int interface_get_menu_item_type(const char* name, int* type)
{
    int i;

    for (i = 0; i < INTERFACE_MENUITEM_COUNT; i++) {
        if (_strcmpi(interfaceGlobs.menuItemName[i], name) == 0) {
            *type = i;
            return 1;
        }
    }
    return 0;
}

// FUNCTION: LEGORR 0x0041aa00
void interface_init_object_items(void)
{
    interfaceGlobs.vehicleItemClicks = (unsigned int*)malloc(
        sizeof(unsigned int) * LEGO_VEHICLE_TYPE_COUNT);
    memset(interfaceGlobs.vehicleItemClicks, 0,
           sizeof(unsigned int) * LEGO_VEHICLE_TYPE_COUNT);
    interfaceGlobs.vehicleItemF2keys =
        (unsigned char*)malloc(LEGO_VEHICLE_TYPE_COUNT);
    memset(interfaceGlobs.vehicleItemF2keys, 0, LEGO_VEHICLE_TYPE_COUNT);
    interfaceGlobs.vehicleItemFlags = (unsigned int*)malloc(
        sizeof(unsigned int) * LEGO_VEHICLE_TYPE_COUNT);
    memset(interfaceGlobs.vehicleItemFlags, 0,
           sizeof(unsigned int) * LEGO_VEHICLE_TYPE_COUNT);

    interfaceGlobs.buildingItemClicks = (unsigned int*)malloc(
        sizeof(unsigned int) * LEGO_BUILDING_TYPE_COUNT);
    memset(interfaceGlobs.buildingItemClicks, 0,
           sizeof(unsigned int) * LEGO_BUILDING_TYPE_COUNT);
    interfaceGlobs.buildingItemF2keys =
        (unsigned char*)malloc(LEGO_BUILDING_TYPE_COUNT);
    memset(interfaceGlobs.buildingItemF2keys, 0, LEGO_BUILDING_TYPE_COUNT);
    interfaceGlobs.buildingItemFlags = (unsigned int*)malloc(
        sizeof(unsigned int) * LEGO_BUILDING_TYPE_COUNT);
    memset(interfaceGlobs.buildingItemFlags, 0,
           sizeof(unsigned int) * LEGO_BUILDING_TYPE_COUNT);
}

// FUNCTION: LEGORR 0x0041ab00
void interface_load_object_items(BlockFile* config, const char* gameName)
{
    unsigned int i;
    const char* str;
    char* argv[4];
    unsigned char key;

    interfaceGlobs.vehicleItemIcons =
        (Image**)malloc(sizeof(Image*) * LEGO_VEHICLE_TYPE_COUNT);
    interfaceGlobs.vehicleItemIconsDisabled =
        (Image**)malloc(sizeof(Image*) * LEGO_VEHICLE_TYPE_COUNT);
    interfaceGlobs.vehicleItemIconsPressed =
        (Image**)malloc(sizeof(Image*) * LEGO_VEHICLE_TYPE_COUNT);

    for (i = 0; i < LEGO_VEHICLE_TYPE_COUNT; i++) {
        // STRING: LEGORR 0x004a34d8
        str = block_get_strptr(config, string_path_join(gameName, "InterfaceBuildImages", LEGO_VEHICLE_NAME(i), NULL));
        if (str != NULL) {
            sound_variant_split((char*)str, argv, ":");
            interfaceGlobs.vehicleItemIcons[i] =
                image_load_bmp_scaled(argv[0], 0, 0);
            interfaceGlobs.vehicleItemIconsDisabled[i] =
                image_load_bmp_scaled(argv[1], 0, 0);
            interfaceGlobs.vehicleItemIconsPressed[i] =
                image_load_bmp_scaled(argv[2], 0, 0);
            if (named_option_index_find_case_insensitive(argv[3], &key)) {
                interfaceGlobs.vehicleItemF2keys[i] = key;
            }
        } else {
            interfaceGlobs.vehicleItemIcons[i] = NULL;
            interfaceGlobs.vehicleItemIconsDisabled[i] = NULL;
            interfaceGlobs.vehicleItemIconsPressed[i] = NULL;
        }
    }

    interfaceGlobs.buildingItemIcons =
        (Image**)malloc(sizeof(Image*) * LEGO_BUILDING_TYPE_COUNT);
    interfaceGlobs.buildingItemIconsDisabled =
        (Image**)malloc(sizeof(Image*) * LEGO_BUILDING_TYPE_COUNT);
    interfaceGlobs.buildingItemIconsPressed =
        (Image**)malloc(sizeof(Image*) * LEGO_BUILDING_TYPE_COUNT);

    for (i = 0; i < LEGO_BUILDING_TYPE_COUNT; i++) {
        str = block_get_strptr(config, string_path_join(gameName, "InterfaceBuildImages", LEGO_BUILDING_NAME(i), NULL));
        if (str != NULL) {
            sound_variant_split((char*)str, argv, ":");
            interfaceGlobs.buildingItemIcons[i] =
                image_load_bmp_scaled(argv[0], 0, 0);
            interfaceGlobs.buildingItemIconsDisabled[i] =
                image_load_bmp_scaled(argv[1], 0, 0);
            interfaceGlobs.buildingItemIconsPressed[i] =
                image_load_bmp_scaled(argv[2], 0, 0);
            if (named_option_index_find_case_insensitive(argv[3], &key)) {
                interfaceGlobs.buildingItemF2keys[i] = key;
            }
        } else {
            interfaceGlobs.buildingItemIcons[i] = NULL;
            interfaceGlobs.buildingItemIconsDisabled[i] = NULL;
            interfaceGlobs.buildingItemIconsPressed[i] = NULL;
        }
    }
}

// FUNCTION: LEGORR 0x0041ada0
void interface_load_menu_items(BlockFile* config, const char* gameName)
{
    BlockRec* item;
    int type;
    char* argv[10];
    char* parts[2];
    unsigned char key;
    char line[1024];
    int count;
    int useName;

    // STRING: LEGORR 0x004a34f0
    for (item = block_subgroup(config, string_path_join(gameName, "InterfaceImages", NULL));
         item != NULL; item = block_next_equal_key(item)) {
        if (interface_get_menu_item_type(item->key, &type)) {
            useName = 0;
            strcpy(line, item->value);
            if (sound_variant_split(line, argv, ":") == 7 &&
                block_get_bool(argv[6]) != 2) {
                useName = (block_get_bool(argv[6]) == 1);
            }
            interfaceGlobs.menuItemTooltipsUseName[type] = useName;
            interfaceGlobs.menuItemIcons[type] =
                image_load_bmp_scaled(argv[0], 0, 0);
            interfaceGlobs.menuItemIconsDisabled[type] =
                image_load_bmp_scaled(argv[1], 0, 0);
            interfaceGlobs.menuItemIconsPressed[type] =
                image_load_bmp_scaled(argv[2], 0, 0);

            count = sound_variant_split(argv[3], parts, "|");
            interfaceGlobs.menuItemTooltips[type] = string_duplicate(parts[0]);
            if (count == 2) {
                sound_alias_lookup(parts[1], &interfaceGlobs.menuItemSFX[type]);
            }

            count = sound_variant_split(argv[4], parts, "|");
            interfaceGlobs.menuItemTooltipsDisabled[type] =
                string_duplicate(parts[0]);
            if (count == 2) {
                sound_alias_lookup(parts[1],
                                   &interfaceGlobs.menuItemSFXDisabled[type]);
            }

            if (named_option_index_find_case_insensitive(argv[5], &key)) {
                interfaceGlobs.menuItemF2keys[type] = key;
            }
        }
    }

    interface_load_object_items(config, gameName);
    interface_load_back_button(config, gameName);
}

// FUNCTION: LEGORR 0x0041afd0
void interface_load_item_panels(BlockFile* config, const char* gameName)
{
    int i;
    const char* str;
    char num[20];
    char* argv[10];

    for (i = 0; i < INTERFACE_SURROUND_COUNT; i++) {
        _itoa(i + 1, num, 10);
        // STRING: LEGORR 0x004a3500
        str = block_get_strptr(config, string_path_join(gameName, "InterfaceSurroundImages", num, NULL));
        if (str != NULL) {
            sound_variant_split((char*)str, argv, ":");
            if (_strcmpi(argv[0], "NULL")) {
                interfaceGlobs.surroundImages[i] =
                    image_load_bmp_scaled(argv[0], 0, 0);
                if (interfaceGlobs.surroundImages[i] != NULL) {
                    image_setup_trans(interfaceGlobs.surroundImages[i],
                                      0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
                }
                interfaceGlobs.surroundOffsets[i].x = (float)atoi(argv[1]);
                interfaceGlobs.surroundOffsets[i].y = (float)atoi(argv[2]);
                interfaceGlobs.backButtonOffsets[i].x = (float)atoi(argv[3]);
                interfaceGlobs.backButtonOffsets[i].y = (float)atoi(argv[4]);
            }
            if (_strcmpi(argv[5], "NULL")) {
                interfaceGlobs.objectSurroundImages[i] =
                    image_load_bmp_scaled(argv[5], 0, 0);
                if (interfaceGlobs.objectSurroundImages[i] != NULL) {
                    image_setup_trans(interfaceGlobs.objectSurroundImages[i],
                                      0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
                }
                interfaceGlobs.objectSurroundOffsets[i].x =
                    (float)atoi(argv[6]);
                interfaceGlobs.objectSurroundOffsets[i].y =
                    (float)atoi(argv[7]);
            }
        } else {
            interfaceGlobs.surroundImages[i] = NULL;
        }
    }
}

// FUNCTION: LEGORR 0x0041b1b0
void interface_load_back_button(BlockFile* config, const char* gameName)
{
    const char* str;
    char* argv[5];

    // STRING: LEGORR 0x004a3518
    str = block_get_strptr(config, string_path_join(gameName, "InterfaceBackButton", NULL));
    if (str != NULL) {
        sound_variant_split((char*)str, argv, ":");
        interfaceGlobs.backButtonWidth = atoi(argv[0]);
        interfaceGlobs.backButtonHeight = atoi(argv[1]);
        interfaceGlobs.backButtonImageHighlight =
            image_load_bmp_scaled(argv[2], 0, 0);
        interfaceGlobs.backButtonImagePressed =
            image_load_bmp_scaled(argv[3], 0, 0);
        interfaceGlobs.backButtonText =
            formatted_text_escape_decode_and_emit(argv[4]);
        tooltip_set_content(5, interfaceGlobs.backButtonText);
    }
}

// FUNCTION: LEGORR 0x0041b270
void interface_load_dependency_images(const char* plusName,
                                      const char* equalsName)
{
    interfaceGlobs.dependencyPlusImage = image_load_bmp_scaled(plusName, 0, 0);
    if (interfaceGlobs.dependencyPlusImage != NULL) {
        image_setup_trans(interfaceGlobs.dependencyPlusImage,
                          0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    }
    interfaceGlobs.dependencyEqualsImage =
        image_load_bmp_scaled(equalsName, 0, 0);
    if (interfaceGlobs.dependencyEqualsImage != NULL) {
        image_setup_trans(interfaceGlobs.dependencyEqualsImage,
                          0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    }
}

// FUNCTION: LEGORR 0x0041b2d0
void command_context_post_to_hud(int menu, const uint32_t blockPos[2])
{
#ifdef INTERFACE_PORT
    command_context_post_to_hud_port(menu, blockPos);
    return;
#endif
    global_state_low_byte_clear_and_refresh();
    interface_set_block_highlight(menu, (const Point2I*)blockPos);
    interface_set_menu(menu, (const Point2I*)blockPos);
}

// FUNCTION: LEGORR 0x0041b300
void interface_set_block_highlight(int menu, const Point2I* blockPos)
{
    int highlight;

    if (interfaceGlobs.nextMenuType == 1 || interfaceGlobs.nextMenuType == 2 ||
        interfaceGlobs.nextMenuType == 7 || interfaceGlobs.nextMenuType == 5 ||
        interfaceGlobs.nextMenuType == 3 || interfaceGlobs.nextMenuType == 4) {
        if (terrain_map_block_highlight_get(lego_get_map(),
                                            interfaceGlobs.nextBlockPos.x,
                                            interfaceGlobs.nextBlockPos.y)) {
            terrain_map_block_highlight_set(lego_get_map(),
                                            interfaceGlobs.nextBlockPos.x,
                                            interfaceGlobs.nextBlockPos.y,
                                            interfaceGlobs.blockHighlight);
        } else {
            interfaceGlobs.blockHighlight = 0;
        }
    }

    if (menu == 1 || menu == 2 || menu == 7 || menu == 5 || menu == 3 ||
        menu == 4) {
        highlight = terrain_map_block_highlight_set(lego_get_map(), blockPos->x,
                                                    blockPos->y, 4);
        if (highlight != 4) {
            interfaceGlobs.blockHighlight = highlight;
        }
    }
}

// FUNCTION: LEGORR 0x0041b3c0
unsigned int interface_get_menu_item_count(int menu)
{
    unsigned int i;
    unsigned int count;

    if (menu == INTERFACE_MENU_BUILDINGS) {
        return LEGO_BUILDING_TYPE_COUNT;
    }
    if (menu == INTERFACE_MENU_SMALL_VEHICLES) {
        count = 0;
        for (i = 0; i < LEGO_VEHICLE_TYPE_COUNT; i++) {
            if ((stats_table_word_at_148(1, i) & 0x1000) ||
                (stats_table_word_at_148(1, i) & 0x2000)) {
                if (!(stats_table_word_at_14c(1, i) & 0x8)) {
                    count++;
                }
            }
        }
        return count;
    }
    if (menu == INTERFACE_MENU_LARGE_VEHICLES) {
        count = 0;
        for (i = 0; i < LEGO_VEHICLE_TYPE_COUNT; i++) {
            if ((stats_table_word_at_148(1, i) & 0x800) ||
                (stats_table_word_at_14c(1, i) & 0x8)) {
                count++;
            }
        }
        return count;
    }
    return interfaceGlobs.menus[menu].count;
}

// FUNCTION: LEGORR 0x0041b470
int interface_menu_has_back_button(int menu)
{
    if (menu != 0 && menu != 15) {
        return 1;
    }
    return 0;
}

// FUNCTION: LEGORR 0x0041b490
void interface_draw(void)
{
    unsigned int count;
    unsigned int flash;
    Image* image;
    Point2F pos;

    if (!(interfaceGlobs.flags & INTERFACE_FLAG_OPEN)) {
        return;
    }

    count = interface_get_menu_item_count(interfaceGlobs.currMenuType);
    if (interface_menu_has_back_button(interfaceGlobs.currMenuType)) {
        if (interfaceGlobs.surroundImages[count - 1] != NULL) {
            pos.x = interfaceGlobs.surroundOffsets[count - 1].x +
                    interfaceGlobs.currMenuPosition.x;
            pos.y = interfaceGlobs.surroundOffsets[count - 1].y +
                    interfaceGlobs.currMenuPosition.y;
            image_display_scaled(interfaceGlobs.surroundImages[count - 1],
                                 NULL, &pos, NULL);
        }

        flash = interfaceGlobs.menuItemFlags[0] & INTERFACE_ITEM_FLASH;
        if (interface_is_item_pressed(0)) {
            if (flash && interfaceGlobs.flashingState) {
                image = interfaceGlobs.backButtonImageHighlight;
            } else {
                image = interfaceGlobs.backButtonImagePressed;
            }
        } else if (interfaceGlobs.flags & INTERFACE_FLAG_HOVER_BACK) {
            if (!flash || interfaceGlobs.flashingState) {
                image = interfaceGlobs.backButtonImageHighlight;
            } else {
                image = NULL;
            }
        } else if (flash && interfaceGlobs.flashingState) {
            image = interfaceGlobs.backButtonImageHighlight;
        } else {
            image = NULL;
        }

        if (image != NULL) {
            pos.x = interfaceGlobs.backButtonOffsets[count - 1].x +
                    interfaceGlobs.currMenuPosition.x;
            pos.y = interfaceGlobs.backButtonOffsets[count - 1].y +
                    interfaceGlobs.currMenuPosition.y;
            image_display_scaled(image, NULL, &pos, NULL);
        }

        interface_update_advisor(0, 0, 0, NULL);
        interface_set_item_pressed(0, 0);
        interface_update_item_enabled(0);
    } else if (interfaceGlobs.objectSurroundImages[count - 1] != NULL) {
        pos.x = interfaceGlobs.objectSurroundOffsets[count - 1].x +
                interfaceGlobs.currMenuPosition.x;
        pos.y = interfaceGlobs.objectSurroundOffsets[count - 1].y +
                interfaceGlobs.currMenuPosition.y;
        image_display_scaled(interfaceGlobs.objectSurroundImages[count - 1],
                             NULL, &pos, NULL);
    }

    pos = interfaceGlobs.currMenuPosition;
    interface_enumerate_menu_items(interfaceGlobs.currMenuType,
                                   interface_callback_draw_item, &pos);

    if (interfaceGlobs.flags &
        (INTERFACE_FLAG_HOVER_ITEM | INTERFACE_FLAG_HOVER_DEPENDENCY)) {
        position_tracker_box_draw(&interfaceGlobs.dependencyArea.x);
    }
    if (interfaceGlobs.flags & (INTERFACE_FLAG_HOVER_DEPENDENCY | 0x2000)) {
        interface_draw_dependencies(interfaceGlobs.dependencyArea.x,
                                    interfaceGlobs.dependencyArea.y);
    } else {
        interface_reset_dependency_timer();
    }
}

// FUNCTION: LEGORR 0x0041b680
int interface_enumerate_menu_items(int menu,
                                   InterfaceMenuItemCallback callback,
                                   void* data)
{
    unsigned int i;

    i = 0;
    if (menu == INTERFACE_MENU_BUILDINGS) {
        for (i = 0; i < LEGO_BUILDING_TYPE_COUNT; i++) {
            if (callback(INTERFACE_MENUITEM_OBJECT, 4, i, data)) {
                return 1;
            }
        }
    } else if (menu == INTERFACE_MENU_SMALL_VEHICLES) {
        for (i = 0; i < LEGO_VEHICLE_TYPE_COUNT; i++) {
            if ((stats_table_word_at_148(1, i) & 0x1000) ||
                (stats_table_word_at_148(1, i) & 0x2000)) {
                if (!(stats_table_word_at_14c(1, i) & 0x8)) {
                    if (callback(INTERFACE_MENUITEM_OBJECT, 1, i, data)) {
                        return 1;
                    }
                }
            }
        }
    } else if (menu == INTERFACE_MENU_LARGE_VEHICLES) {
        for (i = 0; i < LEGO_VEHICLE_TYPE_COUNT; i++) {
            if ((stats_table_word_at_148(1, i) & 0x800) ||
                (stats_table_word_at_14c(1, i) & 0x8)) {
                if (callback(INTERFACE_MENUITEM_OBJECT, 1, i, data)) {
                    return 1;
                }
            }
        }
    } else {
        for (i = 0; i < interfaceGlobs.menus[menu].count; i++) {
            int type = interfaceGlobs.menus[menu].items[i];
            if (type == 34 || type == 35) {
                type = interface_resolve_teleporter_item(
                    &interfaceGlobs.menus[menu].items[i]);
            }
            if (callback(type, 0, 0, data)) {
                return 1;
            }
        }
    }
    return 0;
}

// FUNCTION: LEGORR 0x0041b800
int interface_callback_draw_item(int type, int objType, int objIndex,
                                 void* data)
{
    Point2F* pos = (Point2F*)data;
    Image* image;
    unsigned int flash;
    unsigned int height;

    if (type == INTERFACE_MENUITEM_OBJECT) {
        interface_update_object_item_enabled(objType, objIndex);
        image = interface_get_object_item_image(objType, objIndex);
        if (objType == 1) {
            flash = interfaceGlobs.vehicleItemFlags[objIndex] &
                    INTERFACE_ITEM_FLASH;
        } else if (objType == 4) {
            flash = interfaceGlobs.buildingItemFlags[objIndex] &
                    INTERFACE_ITEM_FLASH;
        } else {
            flash = objType;
        }
        interface_set_object_item_pressed(objType, objIndex, 0);
    } else {
        interface_update_item_enabled(type);
        image = interface_get_item_image(type);
        flash = interfaceGlobs.menuItemFlags[type] & INTERFACE_ITEM_FLASH;
        interface_set_item_pressed(type, 0);
    }

    interface_update_advisor(type, objType, objIndex, pos);

    if (image != NULL) {
        height = image->height;
        if (!flash || interfaceGlobs.flashingState) {
            image_display_scaled(image, NULL, pos, NULL);
        }
    } else {
        height = 40;
    }

    if (type == 1) {
        interface_draw_object_count(2, 0, pos);
    } else if (type == INTERFACE_MENUITEM_OBJECT) {
        interface_draw_object_count(objType, objIndex, pos);
    }

    pos->y += (float)height;
    return 0;
}

// FUNCTION: LEGORR 0x0041b930
void interface_update(float elapsed)
{
    interfaceGlobs.flashingTimer -= elapsed;
    if (interfaceGlobs.flashingTimer < 0.0f) {
        interfaceGlobs.flashingState = !interfaceGlobs.flashingState;
        interfaceGlobs.flashingTimer = 15.0f;
    }

    interface_update_slide(elapsed);

    if (interfaceGlobs.dependencyTimer > 0.0f) {
        interfaceGlobs.dependencyTimer -= elapsed;
    } else {
        interfaceGlobs.flags |= INTERFACE_FLAG_DEPENDENCIES;
    }
}

// FUNCTION: LEGORR 0x0041b9b0
uint32_t sim_status_banner_word_get(void)
{
#ifdef INTERFACE_PORT
    return sim_status_banner_word_get_port();
#endif
    return interfaceGlobs.flashingState;
}

// FUNCTION: LEGORR 0x0041b9c0
int interface_resolve_teleporter_item(int* type)
{
    struct LegoObject* obj = selection_active_entity_get_obj();

    if (!(interfaceGlobs.flags & INTERFACE_FLAG_IN) &&
        (interfaceGlobs.flags & INTERFACE_FLAG_OUT)) {
        return *type;
    }
    if (obj->flags3 & 0x80000000) {
        *type = 34;
        return 34;
    }
    *type = 35;
    return 35;
}

/* The hover sound plays after the cursor has rested on one item for 25
 * frames. */
// GLOBAL: LEGORR 0x004a2b58
static float s_hoverSFXTimer = 25.0f;

// FUNCTION: LEGORR 0x0041ba10
void interface_update_hover_sfx(float elapsed)
{
    if (interfaceGlobs.hoverSFX != interfaceGlobs.lastHoverSFX) {
        interfaceGlobs.lastHoverSFX = interfaceGlobs.hoverSFX;
        s_hoverSFXTimer = 25.0f;
        interfaceGlobs.hoverSFXHandle = 0;
        interfaceGlobs.hoverSFX = 0;
        return;
    }

    s_hoverSFXTimer -= elapsed;
    if (s_hoverSFXTimer <= 0.0f && !interfaceGlobs.hoverSFXHandle) {
        interfaceGlobs.hoverSFXHandle =
            sound_submit_with_global_cooldown(interfaceGlobs.hoverSFX, NULL);
    }
    interfaceGlobs.hoverSFX = 0;
}

// FUNCTION: LEGORR 0x0041ba90
void simulation_global_word_set(uint32_t sfx)
{
#ifdef INTERFACE_PORT
    simulation_global_word_set_port(sfx);
    return;
#endif
    interfaceGlobs.hoverSFX = sfx;
}

// FUNCTION: LEGORR 0x0041baa0
int interface_do_mouse(int x, int y, int leftButton, int leftButtonLast,
                       int leftReleased)
{
    int type;
    int objIndex;
    int result;
    int objType;
    int depIndex;
    int depType;
    unsigned int iconCount;
    int depLevel;
    unsigned int pixel;
    unsigned int argc;
    char* argv[20];
    char* tooltip;
    char* text;
    unsigned int count;
    Image* image;
    struct LegoObject* obj;
    unsigned int last;
    unsigned int i;
    char* p;
    char buff[512];
    char nameSep[512];
    char tipCopy[512];
    char costText[1024];

    tooltip = NULL;
    text = NULL;
    result = 0;
    iconCount = 0;

    interfaceGlobs.flags &= ~0x7600;
    if (!(interfaceGlobs.flags & INTERFACE_FLAG_OPEN) ||
        !(interfaceGlobs.flags & INTERFACE_FLAG_IN)) {
        return 0;
    }

    if (interface_get_dependency_at_point(x, y, interfaceGlobs.dependencyArea.x,
                                          interfaceGlobs.dependencyArea.y,
                                          &depType, &depIndex, &depLevel)) {
        interfaceGlobs.flags |= INTERFACE_FLAG_HOVER_DEPENDENCY;
        if (!leftButton && !leftReleased && depType != 20) {
            if (lego_upgrade_level_names[depLevel] != NULL) {
                // STRING: LEGORR 0x004a3544
                sprintf(nameSep, "%s (%s)", localized_object_name_get(depType, depIndex), lego_upgrade_level_names[depLevel]);
            } else {
                sprintf(nameSep, "%s", localized_object_name_get(depType, depIndex));
            }
            tooltip_set_content(4, nameSep);
            timed_effect_update_request_set_corpus(4);
            tooltip_reset_timer(4);
            if (!(s_panel_highlight_flags & 0x100000)) {
                simulation_global_word_set(
                    object_tooltip_sfx[depType * 15 + depIndex]);
            }
        }
        interface_handle_item_press(0, 0, 0, leftButton, leftButtonLast);
        result = 1;
    } else if (interface_get_item_at_point(x, y, &type, &objType, &objIndex)) {
        if (type == 0) {
            interfaceGlobs.flags |= INTERFACE_FLAG_HOVER_BACK;
            timed_effect_update_request_set_corpus(5);
        } else {
            interfaceGlobs.flags |= INTERFACE_FLAG_HOVER_ITEM;
            if (type == INTERFACE_MENUITEM_OBJECT) {
                interfaceGlobs.flags |= INTERFACE_FLAG_HOVER_OBJECT;
                interfaceGlobs.flags |= 0x2000;
                sprintf(buff, "%s", localized_object_name_get(objType, objIndex));
                if (!(s_panel_highlight_flags & 0x100000)) {
                    simulation_global_word_set(
                        object_tooltip_sfx[objType * 15 + objIndex]);
                }
                text = buff;
            } else {
                if (type == 5 || type == 14) {
                    interfaceGlobs.flags |= 0x2000;
                }

                if (interface_is_item_disabled(type, 0)) {
                    if (interfaceGlobs.menuItemTooltipsDisabled[type] != NULL) {
                        obj = selection_active_entity_get_obj();
                        if (obj != NULL && type == 65 &&
                            obj->objLevel + 1 < building_upgrade_admission_check(obj->type, obj->id) &&
                            lego_get_resource(0) < lego_upgrade_cost_ore &&
                            lego_get_resource(1) < lego_upgrade_cost_studs &&
                            lego_tooltip_ore_icon != NULL) {
                            if (lego_upgrade_cost_text != NULL) {
                                sprintf(costText, "%s", lego_upgrade_cost_text);
                                tooltip = costText;
                            }
                            iconCount = lego_upgrade_cost_ore - lego_get_resource(0);
                        } else {
                            tooltip = interfaceGlobs.menuItemTooltipsDisabled[type];
                        }
                        if (!(s_panel_highlight_flags & 0x100000)) {
                            simulation_global_word_set(
                                interfaceGlobs.menuItemSFXDisabled[type]);
                        }
                    }
                } else if (interfaceGlobs.menuItemTooltips[type] != NULL) {
                    tooltip = interfaceGlobs.menuItemTooltips[type];
                    if (!(s_panel_highlight_flags & 0x100000)) {
                        simulation_global_word_set(interfaceGlobs.menuItemSFX[type]);
                    }
                }

                if (tooltip != NULL) {
                    if (interfaceGlobs.menuItemTooltipsUseName[type] &&
                        selection_active_count_get() == 1 &&
                        selection_active_entity_get() != NULL &&
                        selection_active_entity_get_obj()->customName != NULL &&
                        *selection_active_entity_get_obj()->customName != '\0') {
                        strcpy(nameSep, lego_custom_name_separator);
                        strcpy(tipCopy, tooltip);
                        for (p = nameSep; *p != '\0'; p++) {
                            if (*p == ' ') {
                                *p = '_';
                            }
                        }
                        argc = sound_variant_split(tipCopy, argv, nameSep);
                        buff[0] = '\0';
                        for (i = 0; i < argc; i++) {
                            strcat(buff, argv[i]);
                            if (i < argc - 1) {
                                strcat(buff, selection_active_entity_get_obj()->customName);
                            }
                        }
                        text = buff;
                    } else {
                        text = tooltip;
                    }
                }
            }

            if (!leftButton && !leftReleased) {
                if (text != NULL) {
                    tooltip_set_content(4, text);
                    for (i = iconCount; i != 0; i--) {
                        command_sample_stats_accumulate(4, lego_tooltip_ore_icon);
                    }
                } else {
                    // STRING: LEGORR 0x004a352c
                    sprintf(buff, "Missing config for\n'%s'", interfaceGlobs.menuItemName[type]);
                    tooltip_set_content(4, buff);
                }
                timed_effect_update_request_set_corpus(4);
                tooltip_reset_timer(4);
            } else {
                simulation_global_word_set(0);
            }
        }

        if (interface_handle_item_press(type, objType, objIndex, leftButton,
                                        leftButtonLast) &&
            leftReleased) {
            if (type == INTERFACE_MENUITEM_OBJECT) {
                if (interfaceGlobs.currMenuType == INTERFACE_MENU_SMALL_VEHICLES ||
                    interfaceGlobs.currMenuType == INTERFACE_MENU_LARGE_VEHICLES) {
                    interfaceGlobs.vehicleItemClicks[objIndex]++;
                } else if (interfaceGlobs.currMenuType == INTERFACE_MENU_BUILDINGS) {
                    interfaceGlobs.buildingItemClicks[objIndex]++;
                }
            } else {
                interfaceGlobs.menuItemClicks[type]++;
            }
            media_action_feedback_sound_dispatch(interface_do_action(type) == 0);
        }
        result = 1;
    } else {
        count = interface_get_menu_item_count(interfaceGlobs.currMenuType);
        if (interface_menu_has_back_button(interfaceGlobs.currMenuType)) {
            image = interfaceGlobs.surroundImages[count - 1];
            x -= (int)(interfaceGlobs.surroundOffsets[count - 1].x +
                       interfaceGlobs.currMenuPosition.x);
            y -= (int)(interfaceGlobs.surroundOffsets[count - 1].y +
                       interfaceGlobs.currMenuPosition.y);
        } else {
            image = interfaceGlobs.objectSurroundImages[count - 1];
            x -= (int)(interfaceGlobs.objectSurroundOffsets[count - 1].x +
                       interfaceGlobs.currMenuPosition.x);
            y -= (int)(interfaceGlobs.objectSurroundOffsets[count - 1].y +
                       interfaceGlobs.currMenuPosition.y);
        }
        if (image != NULL && image_get_pixel(image, x, y, &pixel) && pixel != 0) {
            if (leftReleased) {
                media_action_feedback_sound_dispatch(1);
            }
            return 1;
        }
    }
    return result;
}

// FUNCTION: LEGORR 0x0041c1c0
int interface_get_item_at_point(unsigned int x, unsigned int y, int* type,
                                int* objType, int* objIndex)
{
    unsigned int count;
    float left;
    float top;
    InterfaceItemSearch search;

    count = interface_get_menu_item_count(interfaceGlobs.currMenuType);
    if (interface_menu_has_back_button(interfaceGlobs.currMenuType)) {
        left = interfaceGlobs.backButtonOffsets[count - 1].x +
               interfaceGlobs.currMenuPosition.x;
        top = interfaceGlobs.backButtonOffsets[count - 1].y +
              interfaceGlobs.currMenuPosition.y;
        if ((float)x >= left &&
            (float)x <= (float)interfaceGlobs.backButtonWidth + left &&
            (float)y >= top &&
            (float)y <= (float)interfaceGlobs.backButtonHeight + top) {
            *type = 0;
            return 1;
        }
    }

    search.point.x = x;
    search.point.y = y;
    search.pos = interfaceGlobs.currMenuPosition;
    if (interface_enumerate_menu_items(interfaceGlobs.currMenuType,
                                       interface_callback_hit_item, &search)) {
        *type = search.type;
        *objType = search.objType;
        *objIndex = search.objIndex;
        return 1;
    }
    return 0;
}

// FUNCTION: LEGORR 0x0041c310
int interface_callback_hit_item(int type, int objType, int objIndex,
                                void* data)
{
    InterfaceItemSearch* search = (InterfaceItemSearch*)data;
    Image* image;
    unsigned int width;
    unsigned int height;

    if (type == INTERFACE_MENUITEM_OBJECT) {
        image = interface_get_object_item_image(objType, objIndex);
    } else {
        image = interface_get_item_image(type);
    }
    if (image != NULL) {
        width = image->width;
        height = image->height;
    } else {
        height = 40;
        width = height;
    }

    if (search->pos.x <= (float)(unsigned int)search->point.x &&
        (float)(unsigned int)search->point.x <= (float)width + search->pos.x &&
        search->pos.y <= (float)(unsigned int)search->point.y &&
        (float)(unsigned int)search->point.y <= (float)height + search->pos.y) {
        search->type = type;
        interface_set_dependency_item(type, objType, objIndex);
        if (type == INTERFACE_MENUITEM_OBJECT) {
            search->objType = objType;
            search->objIndex = objIndex;
        }
        interface_set_dependency_area(&search->pos, width - 1, height - 1);
        return 1;
    }

    search->pos.y += (float)height;
    return 0;
}

// FUNCTION: LEGORR 0x0041c440
void advisor_callback_schedule_gate(void)
{
#ifdef INTERFACE_PORT
    advisor_callback_schedule_gate_port();
    return;
#endif
    if ((interfaceGlobs.flags & INTERFACE_FLAG_OPEN) &&
        (interfaceGlobs.flags & INTERFACE_FLAG_IN) &&
        input_key_map[0x3c]) {
        interface_enumerate_menu_items(interfaceGlobs.currMenuType,
                                       interface_callback_hotkey, NULL);
    }
}

// FUNCTION: LEGORR 0x0041c470
int interface_callback_hotkey(int type, int objType, int objIndex, void* data)
{
    unsigned char key;

    if (type == INTERFACE_MENUITEM_OBJECT) {
        if (objType == 1) {
            key = interfaceGlobs.vehicleItemF2keys[objIndex];
        } else if (objType == 4) {
            key = interfaceGlobs.buildingItemF2keys[objIndex];
        }
    } else {
        key = interfaceGlobs.menuItemF2keys[type];
    }

    if (input_key_map[key] && (input_key_map[key] != input_prev_key_map[key])) {
        interface_set_dependency_item(type, objType, objIndex);
        interface_do_action(type);
    }
    return 0;
}

// FUNCTION: LEGORR 0x0041c4f0
void interface_update_advisor(int type, int objType, int objIndex,
                              const Point2F* position)
{
    int found;
    int backButton;
    Image* image;
    unsigned int count;
    unsigned int height;
    Point2F pos;
    Point2F advisorPos;

    backButton = 0;
    if ((interfaceGlobs.flags & INTERFACE_FLAG_ADVISOR) &&
        !global_flag_bit0_is_set()) {
        found = 0;
        if (interfaceGlobs.advisorItemType == INTERFACE_MENUITEM_OBJECT) {
            if (interfaceGlobs.advisorObjType == objType &&
                interfaceGlobs.advisorObjIndex == objIndex) {
                image = interface_get_object_item_image(objType, objIndex);
                found = 1;
            }
        } else if (type == interfaceGlobs.advisorItemType) {
            if (type == 0) {
                backButton = 1;
                found = 1;
            } else {
                image = interface_get_item_image(type);
                found = 1;
            }
        }

        if (found) {
            if (backButton) {
                count = interface_get_menu_item_count(interfaceGlobs.currMenuType);
                if (!(interfaceGlobs.flags & INTERFACE_FLAG_IN)) {
                    pos.x = interfaceGlobs.backButtonOffsets[count - 1].x +
                            interfaceGlobs.slideInPosition.x;
                } else {
                    pos.x = interfaceGlobs.backButtonOffsets[count - 1].x +
                            interfaceGlobs.currMenuPosition.x;
                }
                pos.x -= (float)interfaceGlobs.backButtonWidth * -0.5f;
                pos.y = (interfaceGlobs.backButtonOffsets[count - 1].y +
                         interfaceGlobs.currMenuPosition.y) -
                        (float)interfaceGlobs.backButtonHeight * -0.5f;
            } else {
                if (image != NULL) {
                    height = image->height;
                } else {
                    height = 40;
                }
                pos = *position;
                if (!(interfaceGlobs.flags & INTERFACE_FLAG_IN)) {
                    pos.x = interfaceGlobs.slideInPosition.x;
                }
                pos.y = (float)height * 0.5f + pos.y;
            }

            advisor_action_position_get(interfaceGlobs.advisorType,
                                        &advisorPos.x, &advisorPos.y);
            advisor_action_position_and_mode_set(
                interfaceGlobs.advisorType, 12, advisorPos.x + pos.x,
                advisorPos.y + pos.y);
            advisor_request_admit(interfaceGlobs.advisorType,
                                  interfaceGlobs.flags & INTERFACE_FLAG_ADVISOR_LOOP);
            interfaceGlobs.flags &= ~INTERFACE_FLAG_ADVISOR;
        }
    }
}

// FUNCTION: LEGORR 0x0041c6e0
int interface_handle_item_press(int type, int objType, int objIndex,
                                int leftButton, int leftButtonLast)
{
    int pressed = interfaceGlobs.pressedItemType;

    if (leftButton) {
        if (!(interfaceGlobs.flags & INTERFACE_FLAG_HOVER_DEPENDENCY)) {
            if (interfaceGlobs.pressedItemType == INTERFACE_MENUITEM_COUNT) {
                if (!leftButtonLast) {
                    interfaceGlobs.pressedItemType = type;
                    interfaceGlobs.pressedObjType = objType;
                    interfaceGlobs.pressedObjIndex = objIndex;
                    if (type == INTERFACE_MENUITEM_OBJECT) {
                        interface_set_object_item_pressed(objType, objIndex, 1);
                    } else {
                        interface_set_item_pressed(type, 1);
                    }
                }
            } else if (interfaceGlobs.pressedItemType == type) {
                if (type == INTERFACE_MENUITEM_OBJECT) {
                    if (interfaceGlobs.pressedObjType == objType &&
                        interfaceGlobs.pressedObjIndex == objIndex) {
                        interface_set_object_item_pressed(objType, objIndex, 1);
                    }
                } else {
                    interface_set_item_pressed(type, 1);
                }
            }
        }
    } else {
        interfaceGlobs.pressedItemType = INTERFACE_MENUITEM_COUNT;
    }

    if (pressed == type) {
        if (type != INTERFACE_MENUITEM_OBJECT) {
            return 1;
        }
        if (interfaceGlobs.pressedObjType == objType &&
            interfaceGlobs.pressedObjIndex == objIndex) {
            return 1;
        }
    }
    return 0;
}

// FUNCTION: LEGORR 0x0041c7b0
void interface_update_item_enabled(int type)
{
    if (!(interfaceGlobs.menuItemFlags[type] & INTERFACE_ITEM_FORCE_ENABLED) &&
        (interfaceGlobs.flags & INTERFACE_FLAG_IN) &&
        interface_is_item_available(type)) {
        interfaceGlobs.menuItemFlags[type] &= ~INTERFACE_ITEM_DISABLED;
    } else {
        interfaceGlobs.menuItemFlags[type] |= INTERFACE_ITEM_DISABLED;
    }
}

// FUNCTION: LEGORR 0x0041c800
void interface_update_object_item_enabled(int objType, int objIndex)
{
    if (objType == 1) {
        if (!(interfaceGlobs.vehicleItemFlags[objIndex] &
              INTERFACE_ITEM_FORCE_ENABLED) &&
            (interfaceGlobs.flags & INTERFACE_FLAG_IN)) {
            if (interface_object_type_present(1, objIndex) &&
                ensure_cell_entries_loaded(1, objIndex, 0) &&
                (int)entity_indexed_record_word_at_b4_get(1, objIndex, 0) <=
                    lego_get_resource_count(1)) {
                interfaceGlobs.vehicleItemFlags[objIndex] &=
                    ~INTERFACE_ITEM_DISABLED;
                return;
            }
            interfaceGlobs.vehicleItemFlags[objIndex] |= INTERFACE_ITEM_DISABLED;
            return;
        }
        interfaceGlobs.vehicleItemFlags[objIndex] |= INTERFACE_ITEM_DISABLED;
    } else if (objType == 4) {
        if (!(interfaceGlobs.buildingItemFlags[objIndex] &
              INTERFACE_ITEM_FORCE_ENABLED) &&
            (interfaceGlobs.flags & INTERFACE_FLAG_IN)) {
            if (interface_object_type_present(4, objIndex) &&
                ensure_cell_entries_loaded(4, objIndex, 0)) {
                interfaceGlobs.buildingItemFlags[objIndex] &=
                    ~INTERFACE_ITEM_DISABLED;
                return;
            }
            interfaceGlobs.buildingItemFlags[objIndex] |= INTERFACE_ITEM_DISABLED;
            return;
        }
        interfaceGlobs.buildingItemFlags[objIndex] |= INTERFACE_ITEM_DISABLED;
    }
}

// FUNCTION: LEGORR 0x0041c8f0
int interface_is_item_disabled(int type, int checkFlash)
{
    if (type == INTERFACE_MENUITEM_OBJECT) {
        return 0;
    }
    if (lego_flags1 & 0x10000000) {
        return 1;
    }
    if (checkFlash && (system_state_flags_get() & 1)) {
        return !(interfaceGlobs.menuItemFlags[type] & INTERFACE_ITEM_FLASH);
    }
    return interfaceGlobs.menuItemFlags[type] & INTERFACE_ITEM_DISABLED;
}

// FUNCTION: LEGORR 0x0041c950
int interface_is_object_item_disabled(int objType, int objIndex,
                                      int checkFlash)
{
    if (lego_flags1 & 0x10000000) {
        return 1;
    }
    if (checkFlash && (system_state_flags_get() & 1)) {
        if (objType == 1) {
            return !(interfaceGlobs.vehicleItemFlags[objIndex] &
                     INTERFACE_ITEM_FLASH);
        }
        if (objType == 4) {
            return !(interfaceGlobs.buildingItemFlags[objIndex] &
                     INTERFACE_ITEM_FLASH);
        }
    }
    if (objType == 1) {
        return interfaceGlobs.vehicleItemFlags[objIndex] &
               INTERFACE_ITEM_DISABLED;
    }
    if (objType == 4) {
        return interfaceGlobs.buildingItemFlags[objIndex] &
               INTERFACE_ITEM_DISABLED;
    }
    return 0;
}

// FUNCTION: LEGORR 0x0041c9f0
void interface_set_item_pressed(int type, int on)
{
    if (on) {
        interfaceGlobs.menuItemFlags[type] |= INTERFACE_ITEM_PRESSED;
    } else {
        interfaceGlobs.menuItemFlags[type] &= ~INTERFACE_ITEM_PRESSED;
    }
}

// FUNCTION: LEGORR 0x0041ca20
void interface_set_object_item_pressed(int objType, int objIndex, int on)
{
    unsigned int* flags;

    if (objType == 1) {
        flags = interfaceGlobs.vehicleItemFlags;
    } else if (objType == 4) {
        flags = interfaceGlobs.buildingItemFlags;
    } else {
        return;
    }
    if (on) {
        flags[objIndex] |= INTERFACE_ITEM_PRESSED;
    } else {
        flags[objIndex] &= ~INTERFACE_ITEM_PRESSED;
    }
}

// FUNCTION: LEGORR 0x0041ca60
unsigned int interface_is_item_pressed(int type)
{
    return interfaceGlobs.menuItemFlags[type] & INTERFACE_ITEM_PRESSED;
}

// FUNCTION: LEGORR 0x0041ca70
unsigned int interface_is_object_item_pressed(int objType, int objIndex)
{
    if (objType == 1) {
        return interfaceGlobs.vehicleItemFlags[objIndex] & INTERFACE_ITEM_PRESSED;
    }
    if (objType == 4) {
        return interfaceGlobs.buildingItemFlags[objIndex] &
               INTERFACE_ITEM_PRESSED;
    }
    return 0;
}

// FUNCTION: LEGORR 0x0041cab0
Image* interface_get_item_image(int type)
{
    if (interface_is_item_disabled(type, 0)) {
        return interfaceGlobs.menuItemIconsDisabled[type];
    }
    if (interface_is_item_pressed(type)) {
        return interfaceGlobs.menuItemIconsPressed[type];
    }
    return interfaceGlobs.menuItemIcons[type];
}

// FUNCTION: LEGORR 0x0041caf0
Image* interface_get_object_item_image(int objType, int objIndex)
{
    if (interface_is_object_item_disabled(objType, objIndex, 0)) {
        if (objType == 1) {
            return interfaceGlobs.vehicleItemIconsDisabled[objIndex];
        }
        if (objType == 4) {
            return interfaceGlobs.buildingItemIconsDisabled[objIndex];
        }
    } else if (interface_is_object_item_pressed(objType, objIndex)) {
        if (objType == 1) {
            return interfaceGlobs.vehicleItemIconsPressed[objIndex];
        }
        if (objType == 4) {
            return interfaceGlobs.buildingItemIconsPressed[objIndex];
        }
    } else {
        if (objType == 1) {
            return interfaceGlobs.vehicleItemIcons[objIndex];
        }
        if (objType == 4) {
            return interfaceGlobs.buildingItemIcons[objIndex];
        }
    }
    return NULL;
}

// FUNCTION: LEGORR 0x0041cb90
void* input_binding_default_value_get(int objType, int objIndex, int available)
{
#ifdef INTERFACE_PORT
    return input_binding_default_value_get_port(objType, objIndex, available);
#endif
    if (available) {
        switch (objType) {
        case 2:
            return interfaceGlobs.menuItemIcons[1];
        case 11:
            return interfaceGlobs.menuItemIcons[14];
        case 1:
            return interfaceGlobs.vehicleItemIcons[objIndex];
        case 4:
            return interfaceGlobs.buildingItemIcons[objIndex];
        case 15:
            return interfaceGlobs.menuItemIcons[5];
        }
    } else {
        switch (objType) {
        case 2:
            return interfaceGlobs.menuItemIconsDisabled[1];
        case 11:
            return interfaceGlobs.menuItemIconsDisabled[14];
        case 1:
            return interfaceGlobs.vehicleItemIconsDisabled[objIndex];
        case 4:
            return interfaceGlobs.buildingItemIconsDisabled[objIndex];
        case 15:
            return interfaceGlobs.menuItemIconsDisabled[5];
        }
    }
    return NULL;
}

// FUNCTION: LEGORR 0x0041cc80
void interface_set_dependency_item(int type, int objType, int objIndex)
{
    if (type == INTERFACE_MENUITEM_OBJECT) {
        interfaceGlobs.dependencyObjType = objType;
        interfaceGlobs.dependencyObjIndex = objIndex;
    } else if (type == 5) {
        interfaceGlobs.dependencyObjType = 15;
        interfaceGlobs.dependencyObjIndex = 0;
    } else if (type == 14) {
        interfaceGlobs.dependencyObjType = 11;
        interfaceGlobs.dependencyObjIndex = 0;
    }
}

// FUNCTION: LEGORR 0x0041cce0
void interface_set_dependency_area(const Point2F* position,
                                   unsigned int width, unsigned int height)
{
    interfaceGlobs.dependencyArea.x = position->x;
    interfaceGlobs.dependencyArea.width = (float)width;
    interfaceGlobs.dependencyArea.y = position->y;
    interfaceGlobs.dependencyArea.height = (float)height;
}

// FUNCTION: LEGORR 0x0041cd30
void position_tracker_box_draw(const float bounds[4])
{
    const Area2F* area = (const Area2F*)bounds;
    Point2F list[5];

#ifdef INTERFACE_PORT
    position_tracker_box_draw_port(bounds);
    return;
#endif
    if (!(lego_flags1 & 0x100000)) {
        list[0].x = area->x - 1.0f;
        list[0].y = area->y - 1.0f;
        list[2].y = (area->height + area->y) - -1.0f;
        list[1].x = (area->x + area->width) - -1.0f;
        list[1].y = list[0].y;
        list[2].x = list[1].x;
        list[3].x = list[0].x;
        list[3].y = list[2].y;
        list[4].x = list[0].x;
        list[4].y = list[0].y;
        surface_draw_polyline(list, list + 1, 4, 0.0f, 1.0f, 0.0f, 0);

        list[0].x = area->x - 2.0f;
        list[0].y = area->y - 2.0f;
        list[1].x = (area->x + area->width) - -2.0f;
        list[2].y = (area->height + area->y) - -2.0f;
        list[1].y = list[0].y;
        list[2].x = list[1].x;
        list[3].x = list[0].x;
        list[3].y = list[2].y;
        list[4].x = list[0].x;
        list[4].y = list[0].y;
        surface_draw_polyline(list, list + 1, 4, 0.0f, 1.0f, 0.0f, 0);
    }
}

// FUNCTION: LEGORR 0x0041cea0
void interface_object_menu_check(struct LegoObject* obj)
{
    unsigned int count;
    struct LegoObject** list;
    unsigned int i;

    if (interfaceGlobs.currMenuType == 12) {
        count = selection_active_count_get();
        list = selection_active_records_get_obj();
        for (i = count; i != 0; i--) {
            if (obj == *list) {
                if (count == 1) {
                    if (!(entity_stats_word_get(LEGOOBJ(obj)) & 0x2000000) &&
                        !(obj->flags4 & 0x40)) {
                        command_context_post_to_hud(11, NULL);
                    } else {
                        command_context_post_to_hud(10, NULL);
                    }
                } else {
                    selection_clear_request_and_hud_reset();
                }
            }
            list++;
        }
    }
}

// FUNCTION: LEGORR 0x0041cf20
void interface_driven_object_menu_check(struct LegoObject* obj)
{
    unsigned int count;
    struct LegoObject** list;
    unsigned int i;

    if (interfaceGlobs.currMenuType == 10 || interfaceGlobs.currMenuType == 18 ||
        interfaceGlobs.currMenuType == 11) {
        count = selection_active_count_get();
        list = selection_active_records_get_obj();
        for (i = count; i != 0; i--) {
            if (obj->driveObject == *list) {
                if (count == 1) {
                    command_context_post_to_hud(12, NULL);
                } else {
                    selection_clear_request_and_hud_reset();
                }
            }
            list++;
        }
    }
}

// FUNCTION: LEGORR 0x0041cf80
void selection_mode7_cancel_at_point(const uint32_t coordinate[2])
{
    const Point2I* blockPos = (const Point2I*)coordinate;

#ifdef INTERFACE_PORT
    selection_mode7_cancel_at_point_port(coordinate);
    return;
#endif
    if (interfaceGlobs.currMenuType == 7 &&
        interfaceGlobs.selBlockPos.x == blockPos->x &&
        interfaceGlobs.selBlockPos.y == blockPos->y) {
        selection_clear_request_and_hud_reset();
    }
}

// FUNCTION: LEGORR 0x0041cfb0
void cancel_action_for_matching_coordinates(const uint32_t coordinate[2])
{
    const Point2I* blockPos = (const Point2I*)coordinate;

#ifdef INTERFACE_PORT
    cancel_action_for_matching_coordinates_port(coordinate);
    return;
#endif
    if ((interfaceGlobs.currMenuType == 1 || interfaceGlobs.currMenuType == 4) &&
        interfaceGlobs.selBlockPos.x == blockPos->x &&
        interfaceGlobs.selBlockPos.y == blockPos->y) {
        selection_clear_request_and_hud_reset();
    }
}

// FUNCTION: LEGORR 0x0041cfe0
void selection_mode5_cancel_at_point(const uint32_t coordinate[2])
{
    const Point2I* blockPos = (const Point2I*)coordinate;

#ifdef INTERFACE_PORT
    selection_mode5_cancel_at_point_port(coordinate);
    return;
#endif
    if (interfaceGlobs.currMenuType == 5 &&
        interfaceGlobs.selBlockPos.x == blockPos->x &&
        interfaceGlobs.selBlockPos.y == blockPos->y) {
        selection_clear_request_and_hud_reset();
    }
}

// FUNCTION: LEGORR 0x0041d010
void interface_back_to_main_if_selected(struct LegoObject* obj)
{
    if (s_context_mode != 0 &&
        selection_contains_entity_active_target(LEGOOBJ(obj), NULL)) {
        selection_clear_request_and_hud_reset();
    }
}

// FUNCTION: LEGORR 0x0041d040
int interface_is_item_available(int type)
{
    struct LegoObject* obj;
    int objType;
    int mask;
    unsigned int abilities;
    unsigned char terrain;

    obj = selection_active_entity_get_obj();
    switch (type) {
    case 0:
        if (interfaceGlobs.nextMenuType == 15) {
            return 0;
        }
        break;
    case 1: {
        int barracksType;
        int barracksIndex;

        // STRING: LEGORR 0x004a354c
        lego_object_type_for_name("Barracks", &barracksType, &barracksIndex, NULL);
        if (!interface_object_type_present(2, 0) ||
            !ensure_cell_entries_loaded(2, 0, 0) ||
            !lego_object_pair_requirement_met(2, 0, barracksType, barracksIndex)) {
            return 0;
        }
        break;
    }
    case 30:
        if (!aitask_list_contains(0) && !interface_any_object_with_flags2_100000()) {
            return 0;
        }
        break;
    case 29:
        if (!(obj->flags3 & 0x2) || (obj->flags1 & 0x400)) {
            return 0;
        }
        break;
    case 41: {
        Point2I blockPos;

        lego_object_get_block(obj, &blockPos.x, &blockPos.y);
        if (obj->driveObject != NULL &&
            !terrain_cell_route_and_occupancy_check_current(LEGOOBJ(obj->driveObject), blockPos.x, blockPos.y, blockPos.x, blockPos.y, 1)) {
            return 0;
        }
        if ((entity_capability_flags_get(LEGOOBJ(obj)) & 1) &&
            (obj->flags1 & 0x400) && obj->interactObject != NULL &&
            !terrain_cell_route_and_occupancy_check_current(LEGOOBJ(obj->interactObject), blockPos.x, blockPos.y, blockPos.x, blockPos.y, 1)) {
            return 0;
        }
        if (!(entity_stats_word_get(LEGOOBJ(obj)) & 0x2000000) &&
            !(obj->flags4 & 0x40)) {
            return 0;
        }
        break;
    }
    case 11:
        if (!interface_can_lay_path(&interfaceGlobs.selBlockPos)) {
            return 0;
        }
        if (selection_active_has_any_get()) {
            if ((!aitask_list_contains(0) && !interface_any_object_with_flags2_100000()) ||
                !selected_entities_any_predicate_match(interfaceGlobs.selBlockPos.x, interfaceGlobs.selBlockPos.y, 0)) {
                return 0;
            }
        } else if (!interface_any_object_can_reach(&interfaceGlobs.selBlockPos)) {
            return 0;
        }
        break;
    case 9:
        if (selection_active_has_any_get() &&
            !aitask_list_contains(1) && !interface_any_object_with_flags2_100000()) {
            return 0;
        }
        break;
    case 25:
        if ((obj->flags1 & 0x1) ||
            !(entity_stats_word_get(LEGOOBJ(obj)) & 0x20000) ||
            (!(entity_stats_word_get(LEGOOBJ(obj)) & 0x2000000) &&
             !entity_vehicle_unload_cell_find_if_eligible(LEGOOBJ(obj), NULL) &&
             !(obj->flags4 & 0x40))) {
            return 0;
        }
        break;
    case 26:
        if ((obj->flags1 & 0x1) ||
            !(entity_stats_word_get(LEGOOBJ(obj)) & 0x20000)) {
            return 0;
        }
        if (obj->flags4 & 0x40) {
            return 0;
        }
        if (!entity_route_marker_candidate_find(LEGOOBJ(obj), 1)) {
            return 0;
        }
        break;
    case 24:
        objType = 2;
        mask = 8;
        if (entity_capability_flags_get(LEGOOBJ(obj)) & 0x100) {
            objType = 1;
            mask = 4;
        } else if (entity_stats_word_get(LEGOOBJ(obj)) & 0x2000000) {
            if (entity_stats_word_get(LEGOOBJ(obj)) & 0x1000000) {
                objType = 1;
                mask = 4;
            } else {
                objType = 4;
                mask = 16;
            }
        }
        if ((entity_capability_flags_get(LEGOOBJ(obj)) & 0x20) &&
            !(obj->flags4 & 0x40)) {
            if (!entity_record_flag20_query(LEGOOBJ(obj), NULL)) {
                return 0;
            }
        } else if (objType == 2 && !(obj->flags4 & 0x40)) {
            return 0;
        }
        if ((obj->flags2 & 0x10) ||
            (!interface_any_object_with_ability(objType) &&
             (!interface_any_object_of_type(2, 0) ||
              !interface_any_object_with_flags3(mask)))) {
            return 0;
        }
        break;
    case 12:
        if (!interface_can_lay_path(&interfaceGlobs.selBlockPos)) {
            return 0;
        }
        if (terrain_cell_flag_20_bounded_current(interfaceGlobs.selBlockPos.x, interfaceGlobs.selBlockPos.y) ||
            terrain_cell_mask_c0_current(interfaceGlobs.selBlockPos.x, interfaceGlobs.selBlockPos.y) ||
            !lego_object_can_enter_block(NULL, interfaceGlobs.selBlockPos.x, interfaceGlobs.selBlockPos.y)) {
            return 0;
        }
        if (selection_active_has_any_get()) {
            if ((!aitask_list_contains(2) && !interface_any_object_with_flags2_100000()) ||
                !any_object_can_occupy_terrain_cell(interfaceGlobs.selBlockPos.x, interfaceGlobs.selBlockPos.y)) {
                return 0;
            }
        } else if (!interface_any_object_can_reach_alt(&interfaceGlobs.selBlockPos)) {
            return 0;
        }
        break;
    case 13:
        if (!interface_can_lay_path(&interfaceGlobs.selBlockPos)) {
            return 0;
        }
        terrain = LEGO_BLOCK(interfaceGlobs.selBlockPos.x, interfaceGlobs.selBlockPos.y)->terrain;
        if (terrain == 1 || terrain == 11 ||
            !terrain_cell_walkable_flag_current(interfaceGlobs.selBlockPos.x, interfaceGlobs.selBlockPos.y) ||
            terrain_cell_flag_reserved_current(&interfaceGlobs.selBlockPos.x) ||
            !interface_any_object_with_flags2_100000()) {
            return 0;
        }
        if (!interface_any_object_with_ability(8) &&
            (!interface_any_object_of_type(2, 0) ||
             !interface_any_object_with_flags3(32))) {
            return 0;
        }
        break;
    case 45:
        if (obj == lego_object_fp && lego_camera_fp->mode == 0) {
            return 0;
        }
        break;
    case 46:
        if (obj == lego_object_fp && lego_camera_fp->mode == 1) {
            return 0;
        }
        break;
    case 31:
        if (!interface_enumerate_selected(interface_callback_hungry, NULL)) {
            return 0;
        }
        break;
    case 22:
    case 23:
        if ((obj->flags1 & 0x400) && obj->interactObject->type == 9) {
            return 0;
        }
        if ((entity_capability_flags_get(LEGOOBJ(obj)) & 1) &&
            !entity_action_target_cell_is_valid(LEGOOBJ(obj))) {
            return 0;
        }
        if (!interface_enumerate_selected(interface_callback_can_unload, NULL)) {
            return 0;
        }
        break;
    case 36:
        if (!interface_enumerate_selected(interface_callback_damaged, NULL) ||
            (!interface_any_object_with_ability(16) &&
             (!interface_any_object_of_type(2, 0) ||
              !interface_any_object_with_flags3(64)))) {
            return 0;
        }
        break;
    case 5:
        if (terrain_cell_predicate_check_current((const uint32_t*)&interfaceGlobs.selBlockPos) ||
            level_cell_occupied_at_current(&interfaceGlobs.selBlockPos.x) ||
            !ensure_cell_entries_loaded(15, 0, 0)) {
            return 0;
        }
        break;
    case 7:
        if (terrain_cell_predicate_check_current((const uint32_t*)&interfaceGlobs.selBlockPos) ||
            LEGO_BLOCK(interfaceGlobs.selBlockPos.x, interfaceGlobs.selBlockPos.y)->terrain == 6) {
            return 0;
        }
        break;
    case 6:
        if (!level_cell_occupied_at_current(&interfaceGlobs.selBlockPos.x)) {
            return 0;
        }
        break;
    case 16:
        if (!(LEGO_BLOCK(interfaceGlobs.selBlockPos.x, interfaceGlobs.selBlockPos.y)->flags1 & 0x40100000) ||
            terrain_cell_flag_8000_current(&interfaceGlobs.selBlockPos.x) ||
            level_cell_occupied_at_current(&interfaceGlobs.selBlockPos.x) ||
            terrain_cell_occupied_or_flag_current(interfaceGlobs.selBlockPos.x, interfaceGlobs.selBlockPos.y, 1)) {
            return 0;
        }
        break;
    case 64:
        if (obj->objLevel + 1 >= building_upgrade_admission_check(obj->type, obj->id)) {
            return 0;
        }
        if (obj->flags1 & 0x400) {
            return 0;
        }
        if (!interface_any_object_with_flags2_100000()) {
            return 0;
        }
        break;
    case 65:
        if (!building_upgrade_resource_admission_check(LEGOOBJ(obj), NULL)) {
            return 0;
        }
        break;
    case 49:
        if (!interface_enumerate_selected(interface_callback_lacks_tool, (void*)0)) {
            return 0;
        }
        if (!interface_any_object_with_flags2_100000()) {
            return 0;
        }
        break;
    case 50:
        if (!interface_enumerate_selected(interface_callback_lacks_tool, (void*)1)) {
            return 0;
        }
        if (!interface_any_object_with_flags2_100000()) {
            return 0;
        }
        break;
    case 51:
        if (!interface_enumerate_selected(interface_callback_lacks_tool, (void*)2)) {
            return 0;
        }
        if (!interface_any_object_with_flags2_100000()) {
            return 0;
        }
        break;
    case 52:
        if (!interface_enumerate_selected(interface_callback_lacks_tool, (void*)3)) {
            return 0;
        }
        if (!interface_any_object_with_flags2_100000()) {
            return 0;
        }
        break;
    case 53:
        if (!interface_enumerate_selected(interface_callback_lacks_tool, (void*)4)) {
            return 0;
        }
        if (!interface_any_object_with_flags2_100000()) {
            return 0;
        }
        break;
    case 54:
        if (!interface_enumerate_selected(interface_callback_lacks_tool, (void*)5)) {
            return 0;
        }
        if (!interface_any_object_with_flags2_100000()) {
            return 0;
        }
        break;
    case 55:
        if (!interface_enumerate_selected(interface_callback_lacks_tool, (void*)7)) {
            return 0;
        }
        if (!interface_any_object_with_flags2_100000()) {
            return 0;
        }
        break;
    case 56:
        if (!interface_enumerate_selected(interface_callback_lacks_tool, (void*)6)) {
            return 0;
        }
        if (!interface_any_object_with_flags2_100000()) {
            return 0;
        }
        break;
    case 48:
        if (!interface_any_object_with_flags2_100000()) {
            return 0;
        }
        break;
    case 33:
        if (!interface_enumerate_selected(interface_callback_has_tool, (void*)6)) {
            return 0;
        }
        break;
    case 57:
        if (!interface_any_object_with_flags3(0xfc)) {
            return 0;
        }
        abilities = obj->abilityFlags;
        if ((abilities & 0x1) && (abilities & 0x2) && (abilities & 0x4) &&
            (abilities & 0x8) && (abilities & 0x10) && (abilities & 0x20)) {
            return 0;
        }
        break;
    case 58:
        if (interface_primary_has_ability(4) ||
            !interface_any_object_with_flags3(16) ||
            obj->field_3d4 < lego_get_min_health()) {
            return 0;
        }
        break;
    case 59:
        if (interface_primary_has_ability(16) ||
            !interface_any_object_with_flags3(64) ||
            obj->field_3d4 < lego_get_min_health()) {
            return 0;
        }
        break;
    case 60:
        if (interface_primary_has_ability(32) ||
            !interface_any_object_with_flags3(128) ||
            obj->field_3d4 < lego_get_min_health()) {
            return 0;
        }
        break;
    case 61:
        if (interface_primary_has_ability(1) ||
            !interface_any_object_with_flags3(4) ||
            obj->field_3d4 < lego_get_min_health()) {
            return 0;
        }
        break;
    case 62:
        if (interface_primary_has_ability(2) ||
            !interface_any_object_with_flags3(8) ||
            obj->field_3d4 < lego_get_min_health()) {
            return 0;
        }
        break;
    case 63:
        if (interface_primary_has_ability(8) ||
            !interface_any_object_with_flags3(32) ||
            obj->field_3d4 < lego_get_min_health()) {
            return 0;
        }
        break;
    case 14:
        if (terrain_cell_secondary_flag_8_current(interfaceGlobs.selBlockPos.x, interfaceGlobs.selBlockPos.y) ||
            !lego_block_has_lava_neighbour(interfaceGlobs.selBlockPos.x, interfaceGlobs.selBlockPos.y) ||
            !ensure_cell_entries_loaded(11, 0, 0)) {
            return 0;
        }
        break;
    case 37:
        if (!entity_stats_flag10_test(LEGOOBJ(obj)) &&
            !entity_stats_flag20_test(LEGOOBJ(obj)) &&
            !entity_stats_flag40_test(LEGOOBJ(obj))) {
            return 0;
        }
        break;
    case 38:
        if (!lego_object_can_eject(obj, 6, 0, 0)) {
            return 0;
        }
    case 39:
        if (!lego_object_can_eject(obj, 7, 0, 0)) {
            return 0;
        }
        break;
    case 20:
    case 21:
        if (!interface_enumerate_selected(interface_callback_can_pick_up, NULL)) {
            return 0;
        }
        break;
    case 66:
        if (!interface_any_object_of_type2()) {
            return 0;
        }
        if ((!entity_action_kind_prerequisite_check(LEGOOBJ(obj), 1) &&
             !entity_action_kind_prerequisite_check(LEGOOBJ(obj), 0) &&
             !entity_action_kind_prerequisite_check(LEGOOBJ(obj), 2) &&
             !entity_action_kind_prerequisite_check(LEGOOBJ(obj), 3))) {
            return 0;
        }
        break;
    case 67:
        if (!interface_any_object_of_type2() ||
            !entity_action_kind_prerequisite_check(LEGOOBJ(obj), 1)) {
            return 0;
        }
        break;
    case 68:
        if (!interface_any_object_of_type2() ||
            !entity_action_kind_prerequisite_check(LEGOOBJ(obj), 0)) {
            return 0;
        }
        break;
    case 69:
        if (!interface_any_object_of_type2() ||
            !entity_action_kind_prerequisite_check(LEGOOBJ(obj), 2)) {
            return 0;
        }
        break;
    case 70:
        if (!interface_any_object_of_type2() ||
            !entity_action_kind_prerequisite_check(LEGOOBJ(obj), 3)) {
            return 0;
        }
        break;
    case 34:
    case 35:
        if ((entity_state_flags_get(LEGOOBJ(obj)) & 0x400) ||
            !(obj->flags3 & 0x20000000)) {
            return 0;
        }
        break;
    }

    if (!(s_global_low_word_c & 0x6)) {
        if (!(s_global_low_word_c & 0x18)) {
            if ((s_global_low_word_c & 0x60) && type != 0 &&
                !(type > 28 && type <= 30)) {
                return 0;
            }
        } else if (type != 0 && !(type > 19 && type <= 21)) {
            return 0;
        }
    } else if (type != 0 && !(type > 26 && type <= 28)) {
        return 0;
    }
    return 1;
}

// FUNCTION: LEGORR 0x0041dca0
int interface_do_action(int type)
{
    struct LegoObject* obj;
    struct LegoObject* found;
    unsigned int mask;

    obj = selection_active_entity_get_obj();
    if (interface_is_item_disabled(type, 1)) {
        return 0;
    }

    switch (type) {
    case 0:
        switch (interfaceGlobs.currMenuType) {
        case 16:
        case 17:
            command_context_post_to_hud(9, NULL);
            return 1;
        case 18:
            if (!(entity_stats_word_get(LEGOOBJ(obj)) & 0x2000000) &&
                !(obj->flags4 & 0x40)) {
                command_context_post_to_hud(11, NULL);
                return 1;
            }
            command_context_post_to_hud(10, NULL);
            return 1;
        }
        break;
    case 1:
        interface_spawn_request_enqueue(2, 0);
        selection_clear_request_and_hud_reset();
        return 1;
    case 17:
        cmd_produce(3, NULL, 0, NULL);
        cmd_produce(1, obj, 0, NULL);
        command_context_post_to_hud(9, NULL);
        return 1;
    case 18:
        cmd_produce(3, NULL, 0, NULL);
        cmd_produce(1, obj, 0, NULL);
        if (!(entity_stats_word_get(LEGOOBJ(obj)) & 0x2000000) &&
            !(obj->flags4 & 0x40)) {
            command_context_post_to_hud(11, NULL);
            return 1;
        }
        command_context_post_to_hud(10, NULL);
        return 1;
    case 19:
        cmd_produce(3, NULL, 0, NULL);
        cmd_produce(1, obj, 0, NULL);
        command_context_post_to_hud(13, NULL);
        return 1;
    case 72:
        cmd_produce(3, NULL, 0, NULL);
        selection_clear_request_and_hud_reset();
        return 1;
    case 32:
    case 40:
    case 41:
    case 42:
        interface_enumerate_selected(interface_callback_delete, NULL);
        selection_clear_request_and_hud_reset();
        return 1;
    case 11:
        lego_ai_create_dig(&interfaceGlobs.selBlockPos, 0, 1);
        mask = terrain_map_block_highlight_get(lego_get_map(), interfaceGlobs.selBlockPos.x, interfaceGlobs.selBlockPos.y);
        interfaceGlobs.blockHighlight = mask;
        selection_clear_request_and_hud_reset();
        return 1;
    case 9:
        cmd_mark_matching(12, 0, &interfaceGlobs.selBlockPos.x);
        selection_clear_request_and_hud_reset();
        return 1;
    case 15:
        terrain_cell_refresh_dispatch_current(&interfaceGlobs.selBlockPos.x, 0);
        interfaceGlobs.blockHighlight = 0;
        terrain_map_block_highlight_set(lego_get_map(), interfaceGlobs.selBlockPos.x, interfaceGlobs.selBlockPos.y, 4);
        selection_clear_request_and_hud_reset();
        return 1;
    case 25:
        lego_object_go_to_block(obj, 0);
        selection_clear_request_and_hud_reset();
        return 1;
    case 26:
        lego_object_go_to_block(obj, 1);
        selection_clear_request_and_hud_reset();
        return 1;
    case 24:
        selection_clear_active();
        entity_kind_command_issue(LEGOOBJ(obj));
        selection_clear_request_and_hud_reset();
        return 1;
    case 12:
        lego_ai_create_reinforce(&interfaceGlobs.selBlockPos);
        mask = terrain_map_block_highlight_get(lego_get_map(), interfaceGlobs.selBlockPos.x, interfaceGlobs.selBlockPos.y);
        interfaceGlobs.blockHighlight = mask;
        selection_clear_request_and_hud_reset();
        return 1;
    case 13:
        lego_ai_create_dynamite(&interfaceGlobs.selBlockPos, 0);
        mask = terrain_map_block_highlight_get(lego_get_map(), interfaceGlobs.selBlockPos.x, interfaceGlobs.selBlockPos.y);
        interfaceGlobs.blockHighlight = mask;
        selection_clear_request_and_hud_reset();
        return 1;
    case 45:
        cmd_produce(9, NULL, 0, NULL);
        command_context_post_to_hud(15, NULL);
        return 1;
    case 46:
        cmd_produce(9, NULL, 1, NULL);
        command_context_post_to_hud(15, NULL);
        return 1;
    case 44:
        cmd_produce(11, NULL, 0, NULL);
        switch (obj->type) {
        case 1:
            if (!(entity_stats_word_get(LEGOOBJ(obj)) & 0x2000000) &&
                !(obj->flags4 & 0x40)) {
                command_context_post_to_hud(11, NULL);
                return 1;
            }
            command_context_post_to_hud(10, NULL);
            return 1;
        case 2:
            command_context_post_to_hud(9, NULL);
            return 1;
        case 4:
            command_context_post_to_hud(13, NULL);
            return 1;
        }
        break;
    case 47:
        lego_set_tracked_object(obj);
        selection_clear_request_and_hud_reset();
        return 1;
    case 31:
        interface_enumerate_selected(interface_callback_go_feed, NULL);
        selection_clear_request_and_hud_reset();
        return 1;
    case 22:
    case 23:
        interface_enumerate_selected(interface_callback_detach, NULL);
        return 1;
    case 36:
        interface_enumerate_selected(interface_callback_repair, NULL);
        selection_clear_request_and_hud_reset();
        return 1;
    case 5:
    case 7:
        tutorial_path_descriptor_create((const uint32_t*)&interfaceGlobs.selBlockPos);
        selection_clear_request_and_hud_reset();
        return 1;
    case 6:
        terrain_cell_work_flag_clear_current(&interfaceGlobs.selBlockPos.x, 0x3f);
        LEGO_LEVEL_BLOCK(level_current_owner_get(), interfaceGlobs.selBlockPos.x, interfaceGlobs.selBlockPos.y)->flags1 &= ~0x20000000;
        lego_level_block_update(lego_current_level, interfaceGlobs.selBlockPos.x, interfaceGlobs.selBlockPos.y, 0);
        selection_clear_request_and_hud_reset();
        return 1;
    case 16:
        if (!(LEGO_BLOCK(interfaceGlobs.selBlockPos.x, interfaceGlobs.selBlockPos.y)->flags1 & 0x40000000)) {
            lego_path_remove(&interfaceGlobs.selBlockPos);
            selection_clear_request_and_hud_reset();
            return 1;
        }
        simtick_selection_group_clear((const uint32_t*)&interfaceGlobs.selBlockPos);
        selection_clear_request_and_hud_reset();
        return 1;
    case 64:
        object_command_task_create(LEGOOBJ(obj), 0);
        selection_clear_request_and_hud_reset();
        return 1;
    case 65:
        building_upgrade_commit(LEGOOBJ(obj));
        selection_clear_request_and_hud_reset();
        return 1;
    case 48:
        command_context_post_to_hud(16, NULL);
        return 1;
    case 49:
        interface_enumerate_selected(interface_callback_get_tool, (void*)0);
        selection_clear_request_and_hud_reset();
        return 1;
    case 50:
        interface_enumerate_selected(interface_callback_get_tool, (void*)1);
        selection_clear_request_and_hud_reset();
        return 1;
    case 51:
        interface_enumerate_selected(interface_callback_get_tool, (void*)2);
        selection_clear_request_and_hud_reset();
        return 1;
    case 52:
        interface_enumerate_selected(interface_callback_get_tool, (void*)3);
        selection_clear_request_and_hud_reset();
        return 1;
    case 53:
        interface_enumerate_selected(interface_callback_get_tool, (void*)4);
        selection_clear_request_and_hud_reset();
        return 1;
    case 54:
        interface_enumerate_selected(interface_callback_get_tool, (void*)5);
        selection_clear_request_and_hud_reset();
        return 1;
    case 55:
        interface_enumerate_selected(interface_callback_get_tool, (void*)7);
        selection_clear_request_and_hud_reset();
        return 1;
    case 56:
        interface_enumerate_selected(interface_callback_get_tool, (void*)6);
        selection_clear_request_and_hud_reset();
        return 1;
    case 33:
        interface_enumerate_selected(interface_callback_remove_spider, NULL);
        selection_clear_request_and_hud_reset();
        return 1;
    case 57:
        command_context_post_to_hud(17, NULL);
        return 1;
    case 58:
    case 59:
    case 60:
    case 61:
    case 62:
    case 63:
        switch (type) {
        case 58:
            mask = 4;
            break;
        case 59:
            mask = 16;
            break;
        case 60:
            mask = 32;
            break;
        case 61:
            mask = 1;
            break;
        case 62:
            mask = 2;
            break;
        case 63:
            mask = 8;
            break;
        }
        found = lego_object_find_trainer(obj, NULL, mask);
        if (found == NULL) {
            return 0;
        }
        task_type23_create_for_category_mask(LEGOOBJ(found), mask, 1);
        selection_clear_request_and_hud_reset();
        return 1;
    case 2:
        command_context_post_to_hud(INTERFACE_MENU_BUILDINGS, NULL);
        return 1;
    case 3:
        command_context_post_to_hud(INTERFACE_MENU_SMALL_VEHICLES, NULL);
        return 1;
    case 4:
        command_context_post_to_hud(INTERFACE_MENU_LARGE_VEHICLES, NULL);
        return 1;
    case 73:
        if (interface_is_object_item_disabled(interfaceGlobs.dependencyObjType, interfaceGlobs.dependencyObjIndex, 1)) {
            return 0;
        }
        interface_spawn_request_enqueue(interfaceGlobs.dependencyObjType, interfaceGlobs.dependencyObjIndex);
        if (interfaceGlobs.dependencyObjType == 4) {
            return 1;
        }
        break;
    case 14:
        lego_ai_create_dam(&interfaceGlobs.selBlockPos);
        selection_clear_request_and_hud_reset();
        return 1;
    case 37:
        if (entity_stats_flag10_test(LEGOOBJ(obj))) {
            lego_teleport_primary_small = obj;
            selection_clear_request_and_hud_reset();
            return 1;
        }
        if (entity_stats_flag20_test(LEGOOBJ(obj))) {
            lego_teleport_primary_big = obj;
            selection_clear_request_and_hud_reset();
            return 1;
        }
        if (entity_stats_flag40_test(LEGOOBJ(obj))) {
            lego_teleport_primary_laser = obj;
            selection_clear_request_and_hud_reset();
            return 1;
        }
        break;
    case 71:
        if (s_global_low_word_c & 0x1) {
            global_state_low_byte_clear_and_refresh();
            selection_clear_request_and_hud_reset();
            return 1;
        }
        global_state_low_byte_clear_and_refresh();
        s_global_low_word_c |= 0x1;
        global_flag_bit0_clear();
        selection_clear_request_and_hud_reset();
        return 1;
    case 38:
        entity_action_kind_entity_create_and_command_issue(LEGOOBJ(obj), 6, 0, 0);
        selection_clear_request_and_hud_reset();
        return 1;
    case 39:
        entity_action_kind_entity_create_and_command_issue(LEGOOBJ(obj), 7, 0, 0);
        selection_clear_request_and_hud_reset();
        return 1;
    case 20:
        if (entity_capability_flags_get(LEGOOBJ(obj)) & 1) {
            entity_conditional_task_schedule(LEGOOBJ(obj));
            return 1;
        }
        mask = 0x10;
        if ((unsigned char)s_global_low_word_c & (unsigned char)mask) {
            global_state_low_byte_clear_and_refresh();
            return 1;
        }
        global_state_low_byte_clear_and_refresh();
        s_global_low_word_c |= mask;
        return 1;
    case 21:
        mask = 0x8;
        if ((unsigned char)s_global_low_word_c & (unsigned char)mask) {
            global_state_low_byte_clear_and_refresh();
            return 1;
        }
        global_state_low_byte_clear_and_refresh();
        s_global_low_word_c |= mask;
        return 1;
    case 28:
        mask = 0x4;
        if ((unsigned char)s_global_low_word_c & (unsigned char)mask) {
            global_state_low_byte_clear_and_refresh();
            return 1;
        }
        global_state_low_byte_clear_and_refresh();
        s_global_low_word_c |= mask;
        return 1;
    case 27:
        mask = 0x2;
        if ((unsigned char)s_global_low_word_c & (unsigned char)mask) {
            global_state_low_byte_clear_and_refresh();
            return 1;
        }
        global_state_low_byte_clear_and_refresh();
        s_global_low_word_c |= mask;
        return 1;
    case 30:
        mask = 0x20;
        if ((unsigned char)s_global_low_word_c & (unsigned char)mask) {
            global_state_low_byte_clear_and_refresh();
            return 1;
        }
        global_state_low_byte_clear_and_refresh();
        s_global_low_word_c |= mask;
        return 1;
    case 29:
        mask = 0x40;
        if ((unsigned char)s_global_low_word_c & (unsigned char)mask) {
            global_state_low_byte_clear_and_refresh();
            return 1;
        }
        global_state_low_byte_clear_and_refresh();
        s_global_low_word_c |= mask;
        return 1;
    case 66:
        command_context_post_to_hud(18, NULL);
        return 1;
    case 67:
        object_command_task_create(LEGOOBJ(obj), 1);
        obj->field_3d8 = 0;
        selection_clear_request_and_hud_reset();
        return 1;
    case 68:
        object_command_task_create(LEGOOBJ(obj), 0);
        obj->field_3d8 = 0;
        selection_clear_request_and_hud_reset();
        return 1;
    case 69:
        object_command_task_create(LEGOOBJ(obj), 2);
        obj->field_3d8 = 0;
        selection_clear_request_and_hud_reset();
        return 1;
    case 70:
        object_command_task_create(LEGOOBJ(obj), 3);
        obj->field_3d8 = 0;
        selection_clear_request_and_hud_reset();
        return 1;
    case 34:
        lego_object_set_primary_teleporter(obj, 1);
        break;
    case 35:
        lego_object_set_primary_teleporter(obj, 0);
        break;
    }

    selection_clear_request_and_hud_reset();
    return 1;
}

// FUNCTION: LEGORR 0x0041e750
void selection_clear_request_and_hud_reset(void)
{
#ifdef INTERFACE_PORT
    selection_clear_request_and_hud_reset_port();
    return;
#endif
    cmd_produce(3, NULL, 0, NULL);
    command_context_post_to_hud(0, NULL);
}

// FUNCTION: LEGORR 0x0041e770
void interface_set_item_flash(int type, int on)
{
    if (on) {
        interfaceGlobs.menuItemFlags[type] |= INTERFACE_ITEM_FLASH;
    } else {
        interfaceGlobs.menuItemFlags[type] &= ~INTERFACE_ITEM_FLASH;
    }
}

// FUNCTION: LEGORR 0x0041e7a0
void interface_set_object_item_flash(int objType, int objIndex, int on)
{
    unsigned int* flags;

    if (objType == 1) {
        flags = interfaceGlobs.vehicleItemFlags;
    } else if (objType == 4) {
        flags = interfaceGlobs.buildingItemFlags;
    } else {
        return;
    }
    if (on) {
        flags[objIndex] |= INTERFACE_ITEM_FLASH;
    } else {
        flags[objIndex] &= ~INTERFACE_ITEM_FLASH;
    }
}

// FUNCTION: LEGORR 0x0041e7e0
struct LegoObject* interface_get_primary_selected(void)
{
    return selection_active_entity_get_obj();
}

// FUNCTION: LEGORR 0x0041e7f0
unsigned int interface_set_item_clicks(int type, unsigned int clicks)
{
    unsigned int old = interfaceGlobs.menuItemClicks[type];
    interfaceGlobs.menuItemClicks[type] = clicks;
    return old;
}

// FUNCTION: LEGORR 0x0041e810
unsigned int interface_get_item_clicks(int type)
{
    return interfaceGlobs.menuItemClicks[type];
}

// FUNCTION: LEGORR 0x0041e820
unsigned int interface_set_object_item_clicks(int objType, int objIndex,
                                              unsigned int clicks)
{
    unsigned int old = 0;

    if (objType == 1) {
        old = interfaceGlobs.vehicleItemClicks[objIndex];
        interfaceGlobs.vehicleItemClicks[objIndex] = clicks;
    } else if (objType == 4) {
        old = interfaceGlobs.buildingItemClicks[objIndex];
        interfaceGlobs.buildingItemClicks[objIndex] = clicks;
    }
    return old;
}

// FUNCTION: LEGORR 0x0041e860
unsigned int interface_get_object_item_clicks(int objType, int objIndex)
{
    if (objType == 1) {
        return interfaceGlobs.vehicleItemClicks[objIndex];
    }
    if (objType == 4) {
        return interfaceGlobs.buildingItemClicks[objIndex];
    }
    return 0;
}

// FUNCTION: LEGORR 0x0041e890
int interface_set_advisor_item(int type, int loop)
{
    interfaceGlobs.flags |= INTERFACE_FLAG_ADVISOR;
    if (loop) {
        interfaceGlobs.flags |= INTERFACE_FLAG_ADVISOR_LOOP;
    }
    interfaceGlobs.advisorType = interface_get_advisor_type(type);
    interfaceGlobs.advisorItemType = type;
    return 1;
}

// FUNCTION: LEGORR 0x0041e8d0
int interface_get_advisor_type(int type)
{
    switch (type) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
        return 4;
    case 5:
        return 5;
    }
    return 3;
}

// FUNCTION: LEGORR 0x0041e990
int cell_action_permission_get(int x, int y)
{
#ifdef INTERFACE_PORT
    return cell_action_permission_get_port(x, y);
#endif
    if (system_state_flags_get() & 4) {
        return interfaceGlobs.blockPermissions[x][y];
    }
    return 1;
}

// FUNCTION: LEGORR 0x0041e9d0
void interface_set_slide_positions(unsigned int inX, unsigned int inY,
                                   unsigned int outX, unsigned int outY,
                                   float speed)
{
    interfaceGlobs.slideOutPosition.x = (float)outX;
    interfaceGlobs.slideOutPosition.y = (float)outY;
    interfaceGlobs.slideInPosition.x = (float)inX;
    interfaceGlobs.slideInPosition.y = (float)inY;
    interfaceGlobs.slideSpeed = speed * 0.04f;
}

// FUNCTION: LEGORR 0x0041ea50
void interface_set_menu(int menu, const Point2I* blockPos)
{
    if (!(interfaceGlobs.flags & INTERFACE_FLAG_IN) ||
        interfaceGlobs.currMenuType != menu) {
        interfaceGlobs.flags = (interfaceGlobs.flags & ~INTERFACE_FLAG_IN) |
                               INTERFACE_FLAG_OUT;
        sound_alias_play(22, 0);
    } else {
        interfaceGlobs.currMenuType = menu;
        if (blockPos == NULL) {
            interfaceGlobs.nextMenuType = menu;
            return;
        }
        interfaceGlobs.selBlockPos = *blockPos;
    }
    if (blockPos != NULL) {
        interfaceGlobs.nextBlockPos = *blockPos;
    }
    interfaceGlobs.nextMenuType = menu;
}

// FUNCTION: LEGORR 0x0041eac0
void interface_update_slide(float elapsed)
{
    if (!(interfaceGlobs.flags & INTERFACE_FLAG_IN)) {
        if (interfaceGlobs.flags & INTERFACE_FLAG_OUT) {
            interfaceGlobs.currMenuPosition.x += elapsed * interfaceGlobs.slideSpeed;
            if (interfaceGlobs.currMenuType == 22 && (panelGlobs.flags & 2)) {
                panel_state_flags_toggle(8);
                panel_item_flag8_set_if_enabled(6, 1, 0);
                interfaceGlobs.currMenuPosition.x = interfaceGlobs.slideOutPosition.x;
            }
            if (interfaceGlobs.currMenuPosition.x >= interfaceGlobs.slideOutPosition.x) {
                interfaceGlobs.flags &= ~INTERFACE_FLAG_OUT;
                interfaceGlobs.currMenuPosition.x = interfaceGlobs.slideOutPosition.x;
                sound_alias_play(21, 0);
                if (interfaceGlobs.nextMenuType == 22) {
                    interfaceGlobs.flags = (interfaceGlobs.flags & ~INTERFACE_FLAG_OPEN) | INTERFACE_FLAG_IN;
                    panel_state_flags_toggle(8);
                    simtick_global_ui_flags_clear_bits12();
                    panelGlobs.flags |= 2;
                } else if (interfaceGlobs.nextMenuType == 23) {
                    interfaceGlobs.flags = (interfaceGlobs.flags & ~INTERFACE_FLAG_OPEN) | INTERFACE_FLAG_IN;
                    panel_state_flags_toggle(11);
                    s_global_low_word_c |= 1;
                    global_flag_bit0_clear();
                } else {
                    interfaceGlobs.flags |= INTERFACE_FLAG_OPEN;
                }
                interfaceGlobs.currMenuType = interfaceGlobs.nextMenuType;
                interfaceGlobs.selBlockPos = interfaceGlobs.nextBlockPos;
            }
        }
    } else if (!(interfaceGlobs.flags & INTERFACE_FLAG_OUT) &&
               (interfaceGlobs.flags & INTERFACE_FLAG_OPEN)) {
        interfaceGlobs.currMenuPosition.x -= elapsed * interfaceGlobs.slideSpeed;
        if (interfaceGlobs.currMenuPosition.x <= interfaceGlobs.slideInPosition.x) {
            interfaceGlobs.flags |= INTERFACE_FLAG_IN;
            interfaceGlobs.currMenuPosition.x = interfaceGlobs.slideInPosition.x;
        }
    }
}

// FUNCTION: LEGORR 0x0041ec30
void interface_draw_object_count(int objType, int objIndex,
                                 const Point2F* position)
{
    unsigned int i;
    unsigned int count;

    count = 0;
    for (i = 0; i < lego_object_request_count; i++) {
        if (lego_object_requests.type[i] == objType &&
            lego_object_requests.index[i] == objIndex) {
            count++;
        }
    }
    if (count != 0) {
        // STRING: LEGORR 0x004a2b54
        font_print_f(interfaceGlobs.font, (int)position->x + 3, (int)position->y + 3, "%i", count);
    }
}

// GLOBAL: LEGORR 0x004dcd50
static int s_lastDependencyObjIndex;

// FUNCTION: LEGORR 0x0041eca0
void interface_draw_dependencies(float x, float y)
{
    // GLOBAL: LEGORR 0x004a2b5c
    static int lastObjType = 20;
    InterfaceDependency* list;
    unsigned int count;
    unsigned int i;
    Point2F pos;
    Image* image;

    if (lastObjType != interfaceGlobs.dependencyObjType ||
        s_lastDependencyObjIndex != interfaceGlobs.dependencyObjIndex) {
        interface_reset_dependency_timer();
        lastObjType = interfaceGlobs.dependencyObjType;
        s_lastDependencyObjIndex = interfaceGlobs.dependencyObjIndex;
    }

    if (interfaceGlobs.flags & INTERFACE_FLAG_DEPENDENCIES) {
        dependencies_get_requirements(interfaceGlobs.dependencyObjType, interfaceGlobs.dependencyObjIndex, 0, &list, &count);
        if (count != 0) {
            pos.x = x - 15.0f;
            pos.y = y;
            for (i = 0; i < count; i++) {
                if (i == 0) {
                    image = interfaceGlobs.dependencyEqualsImage;
                } else {
                    image = interfaceGlobs.dependencyPlusImage;
                }
                if (image != NULL) {
                    pos.x -= (float)(int)image->width;
                    image_display_scaled(image, NULL, &pos, NULL);
                } else {
                    pos.x -= 20.0f;
                }

                image = (Image*)input_binding_default_value_get(list[i].objType, list[i].objIndex, entity_pool_find_by_callback_filtered_current(INTERFACE_OBJECT_CALLBACK(terrain_cell_resource_entry_matches), &list[i]) != 0);
                if (image != NULL) {
                    pos.x -= (float)(int)image->width;
                    image_display_scaled(image, NULL, &pos, NULL);
                    if (list[i].showLevel && list[i].level) {
                        font_print_f(interfaceGlobs.font, (int)pos.x + 3, (int)pos.y + 3, "%s", lego_upgrade_level_names[list[i].level]);
                    }
                } else {
                    pos.x -= 40.0f;
                }
            }
        }
    }
}

// FUNCTION: LEGORR 0x0041ee60
void interface_reset_dependency_timer(void)
{
    interfaceGlobs.dependencyTimer = 25.0f;
    interfaceGlobs.flags &= ~INTERFACE_FLAG_DEPENDENCIES;
}

// FUNCTION: LEGORR 0x0041ee80
int interface_get_dependency_at_point(int x, int y, float areaX, float areaY,
                                      int* type, int* objType, int* objIndex)
{
    InterfaceDependency* list;
    unsigned int count;
    unsigned int i;
    Image* image;
    float fx;
    float left;
    unsigned int width;
    unsigned int height;

    if ((interfaceGlobs.flags & INTERFACE_FLAG_DEPENDENCIES) &&
        (dependencies_get_requirements(interfaceGlobs.dependencyObjType, interfaceGlobs.dependencyObjIndex, 0, &list, &count), count != 0)) {
        areaX -= 15.0f;
        fx = (float)(unsigned int)x;
        if (areaX <= fx && fx <= areaX - -15.0f &&
            areaY <= (float)(unsigned int)y &&
            (float)(unsigned int)y <= areaY - -40.0f) {
            *type = 20;
            return 1;
        }

        for (i = 0; i < count; i++) {
            if (i == 0) {
                image = interfaceGlobs.dependencyEqualsImage;
            } else {
                image = interfaceGlobs.dependencyPlusImage;
            }
            if (image != NULL) {
                width = image->width;
                height = image->height;
            } else {
                width = 20;
                height = 40;
            }
            left = areaX - (float)width;
            if (left <= fx && fx <= left + (float)width &&
                areaY <= (float)(unsigned int)y &&
                (float)(unsigned int)y <= (float)height + areaY) {
                *type = 20;
                return 1;
            }

            image = (Image*)input_binding_default_value_get(list[i].objType, list[i].objIndex, 0);
            if (image != NULL) {
                width = image->width;
                height = image->height;
            } else {
                width = 40;
                height = 40;
            }
            areaX = left - (float)width;
            if (areaX <= fx && fx <= areaX + (float)width &&
                areaY <= (float)(unsigned int)y &&
                (float)(unsigned int)y <= (float)height + areaY) {
                *type = list[i].objType;
                *objType = list[i].objIndex;
                *objIndex = list[i].level;
                return 1;
            }
        }
    }
    return 0;
}

// FUNCTION: LEGORR 0x0041f100
int interface_object_type_present(int objType, int objIndex)
{
    InterfaceObjectSearch search;
    unsigned int flags;

    search.objIndex = 0;
    search.statsMask = 0;
    search.objType = 0;
    search.stateMask = 0;
    search.abilityMask = 0;
    search.flags = 0;

    flags = stats_table_word_at_148(objType, objIndex);
    if (flags & 0x1000) {
        search.statsMask = 0x10;
    } else if (flags & 0x800) {
        search.statsMask = 0x20;
    } else if (flags & 0x2000) {
        search.statsMask = 0x40;
    } else if (flags & 0x400000) {
        search.statsMask = 0x80000000;
    } else {
        return 1;
    }
    return entity_pool_find_by_callback_filtered_current(INTERFACE_OBJECT_CALLBACK(interface_callback_object_search), &search);
}

// FUNCTION: LEGORR 0x0041f190
int interface_callback_object_search(struct LegoObject* obj, void* data)
{
    InterfaceObjectSearch* search = (InterfaceObjectSearch*)data;

    if (obj->type == 4 && !entity_shared_eligibility_check(LEGOOBJ(obj), 0)) {
        return 0;
    }
    if ((search->flags & 1) &&
        (obj->type != search->objType || obj->id != search->objIndex)) {
        return 0;
    }
    if ((search->flags & 2) && !(obj->abilityFlags & search->abilityMask)) {
        return 0;
    }
    if (search->statsMask &&
        !(search->statsMask & entity_stats_word_get(LEGOOBJ(obj)))) {
        return 0;
    }
    if (search->stateMask &&
        !(search->stateMask & entity_state_flags_get(LEGOOBJ(obj)))) {
        return 0;
    }
    return 1;
}

// FUNCTION: LEGORR 0x0041f230
int interface_any_object_of_type2(void)
{
    InterfaceObjectSearch search;

    search.objIndex = 0;
    search.statsMask = 0;
    search.objType = 0;
    search.abilityMask = 0;
    search.flags = 0;
    search.stateMask = 2;
    return entity_pool_find_by_callback_filtered_current(INTERFACE_OBJECT_CALLBACK(interface_callback_object_search), &search);
}

// FUNCTION: LEGORR 0x0041f270
int interface_any_object_with_flags3(unsigned int mask)
{
    InterfaceObjectSearch search;

    search.objIndex = 0;
    search.statsMask = 0;
    search.abilityMask = 0;
    search.objType = 0;
    search.flags = 0;
    search.stateMask = mask;
    return entity_pool_find_by_callback_filtered_current(INTERFACE_OBJECT_CALLBACK(interface_callback_object_search), &search);
}

// FUNCTION: LEGORR 0x0041f2b0
int interface_any_object_with_flags2_100000(void)
{
    InterfaceObjectSearch search;

    search.objIndex = 0;
    search.stateMask = 0;
    search.objType = 0;
    search.abilityMask = 0;
    search.statsMask = 0x100000;
    search.flags = 0;
    return entity_pool_find_by_callback_filtered_current(INTERFACE_OBJECT_CALLBACK(interface_callback_object_search), &search);
}

// FUNCTION: LEGORR 0x0041f2f0
int interface_any_object_of_type(int objType, int objIndex)
{
    InterfaceObjectSearch search;

    search.statsMask = 0;
    search.stateMask = 0;
    search.abilityMask = 0;
    search.flags = 1;
    search.objType = objType;
    search.objIndex = objIndex;
    return entity_pool_find_by_callback_filtered_current(INTERFACE_OBJECT_CALLBACK(interface_callback_object_search), &search);
}

// FUNCTION: LEGORR 0x0041f340
int interface_any_object_with_ability(unsigned int mask)
{
    InterfaceObjectSearch search;

    search.objIndex = 0;
    search.statsMask = 0;
    search.stateMask = 0;
    search.flags = 2;
    search.objType = 0;
    search.abilityMask = mask;
    return entity_pool_find_by_callback_filtered_current(INTERFACE_OBJECT_CALLBACK(interface_callback_object_search), &search);
}

// FUNCTION: LEGORR 0x0041f390
int interface_callback_can_reach(struct LegoObject* obj, void* data)
{
    Point2I* blockPos = (Point2I*)data;

    return terrain_cell_walkability_query_current(LEGOOBJ(obj), blockPos->x, blockPos->y, 0, 0) != 0;
}

// FUNCTION: LEGORR 0x0041f3c0
int interface_any_object_can_reach(const Point2I* blockPos)
{
    return entity_pool_find_by_callback_filtered_current(INTERFACE_OBJECT_CALLBACK(interface_callback_can_reach), (void*)blockPos);
}

// FUNCTION: LEGORR 0x0041f3e0
int interface_callback_can_enter(struct LegoObject* obj, void* data)
{
    Point2I* blockPos = (Point2I*)data;

    return lego_object_can_enter_block(obj, blockPos->x, blockPos->y) != 0;
}

// FUNCTION: LEGORR 0x0041f400
int interface_any_object_can_reach_alt(const Point2I* blockPos)
{
    return entity_pool_find_by_callback_filtered_current(INTERFACE_OBJECT_CALLBACK(interface_callback_can_enter), (void*)blockPos);
}

// FUNCTION: LEGORR 0x0041f420
int interface_enumerate_selected(InterfaceObjectCallback callback, void* data)
{
    unsigned int count;
    struct LegoObject** list;
    unsigned int i;

    count = selection_active_count_get();
    list = selection_active_records_get_obj();
    for (i = 0; i < count; i++) {
        if (callback(list[i], data)) {
            return 1;
        }
    }
    return 0;
}

// FUNCTION: LEGORR 0x0041f470
int interface_callback_can_unload(struct LegoObject* obj, void* data)
{
    if ((obj->flags1 & 0x400) && !(obj->flags1 & 0x2000)) {
        return 1;
    }
    return 0;
}

// FUNCTION: LEGORR 0x0041f490
int interface_callback_hungry(struct LegoObject* obj, void* data)
{
    if (obj->energy < 100.0f) {
        return 1;
    }
    return 0;
}

// FUNCTION: LEGORR 0x0041f4b0
int interface_callback_damaged(struct LegoObject* obj, void* data)
{
    if (obj->health < 100.0f) {
        return 1;
    }
    return 0;
}

// FUNCTION: LEGORR 0x0041f4d0
int interface_callback_can_pick_up(struct LegoObject* obj, void* data)
{
    void* container;
    Point2F wPos;
    int bx;
    int by;
    unsigned int capacity;

    if (entity_capability_flags_get(LEGOOBJ(obj)) & 1) {
        if (!(obj->flags1 & 0x400) && !(obj->flags4 & 0x10000) &&
            (container = lego_object_get_container(obj)) != NULL) {
            container_get_position(container, NULL, &wPos);
            if (terrain_map_world_to_block(lego_get_map(), wPos.x, wPos.y, &bx, &by) &&
                terrain_cell_route_and_occupancy_check_current(NULL, bx, by, bx, by, 1)) {
                return 1;
            }
        }
    } else if (obj->type == 2) {
        if (!(obj->flags1 & 0x400) && (obj->flags3 & 0x20) &&
            (obj->flags3 & 0x40)) {
            return 1;
        }
    } else if ((obj->flags3 & 0x20) && (obj->flags3 & 0x40)) {
        capacity = (obj->carryCount < stats_instance_word_at_ec(obj)) ? obj->carryCount : stats_instance_word_at_ec(obj);
        if (obj->carriedCount < capacity) {
            return 1;
        }
    }
    return 0;
}

// FUNCTION: LEGORR 0x0041f5f0
int interface_callback_delete(struct LegoObject* obj, void* data)
{
    obj->health = -1.0f;
    obj->flags4 |= 0x8;
    return 0;
}

// FUNCTION: LEGORR 0x0041f610
int interface_callback_go_feed(struct LegoObject* obj, void* data)
{
    if (obj->energy < 100.0f) {
        entity_activity23_state_transition(LEGOOBJ(obj));
    }
    return 0;
}

// FUNCTION: LEGORR 0x0041f640
int interface_callback_repair(struct LegoObject* obj, void* data)
{
    if (obj->health < 100.0f) {
        entity_threshold_progress_event_queue(obj, 0);
    }
    return 0;
}

// FUNCTION: LEGORR 0x0041f670
int interface_callback_lacks_tool(struct LegoObject* obj, void* data)
{
    return !ai_activity_scan(LEGOOBJ(obj), (int)(size_t)data);
}

// FUNCTION: LEGORR 0x0041f690
int interface_callback_get_tool(struct LegoObject* obj, void* data)
{
    if (!ai_activity_scan(LEGOOBJ(obj), (int)(size_t)data)) {
        object_event_type2_queue((void*)data);
    }
    return 0;
}

// FUNCTION: LEGORR 0x0041f6c0
int interface_callback_detach(struct LegoObject* obj, void* data)
{
    if (entity_capability_flags_get(LEGOOBJ(obj)) & 1) {
        if ((obj->flags1 & 0x400) && obj->interactObject != NULL) {
            lego_object_drop_carried(obj->interactObject);
            return 0;
        }
    } else if (!(obj->flags1 & 0x400) || obj->interactObject->type != 9) {
        lego_object_detach_all(obj, 0);
    }
    return 0;
}

// FUNCTION: LEGORR 0x0041f720
int interface_primary_has_ability(unsigned int mask)
{
    return (selection_active_entity_get_obj()->abilityFlags & mask) != 0;
}

// FUNCTION: LEGORR 0x0041f740
int interface_can_lay_path(const Point2I* blockPos)
{
    int dirs[8];
    int* dir;
    unsigned int i;
    unsigned int count;

    dirs[4] = -1;
    dirs[7] = -1;
    dirs[0] = 1;
    dirs[1] = 0;
    dirs[2] = 0;
    dirs[3] = 1;
    dirs[5] = 0;
    dirs[6] = 0;

    if (!terrain_cell_mask_c0_current(blockPos->x, blockPos->y)) {
        dir = dirs;
        for (i = 0; i < 4; i++) {
            if (terrain_cell_occupied_or_flag_current(blockPos->x + dir[0], dir[1] + blockPos->y, 1)) {
                return 0;
            }
            dir += 2;
        }
        return 1;
    }

    count = 0;
    dir = dirs;
    for (i = 0; i < 4; i++) {
        if (terrain_cell_occupied_or_flag_current(blockPos->x + dir[0], dir[1] + blockPos->y, 1)) {
            count++;
        }
        dir += 2;
    }
    if (count > 1) {
        return 0;
    }
    return 1;
}

// FUNCTION: LEGORR 0x0041f820
int interface_callback_has_tool(struct LegoObject* obj, void* data)
{
    return ai_activity_scan(LEGOOBJ(obj), (int)(size_t)data) != 0;
}

// FUNCTION: LEGORR 0x0041f840
int interface_callback_remove_spider(struct LegoObject* obj, void* data)
{
    if (ai_activity_scan(LEGOOBJ(obj), 6)) {
        entity_kind6_tick_down_marker_spawn_obj(obj);
    }
    return 0;
}
