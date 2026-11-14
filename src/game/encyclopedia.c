// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-014: the encyclopedia (the original's Encyclopedia.c), rebuilt in
 * original shape (ADR 0005, finding 633). Text 0040e490-0040e930, strings
 * 004a1e98-004a1ee1, .bss 004c7e88-004c7eac.
 *
 * Encyclopedia_Initialise opens, for every vehicle, minifigure, rock monster
 * and building type and for the power crystal and ore, the text file that
 * Lego.cfg's Encyclopedia block names. Selecting an object while the
 * encyclopedia panel is in use picks its file; Encyclopedia_Update prints
 * the file into the info dock once, then keeps the dock drawn.
 *
 * The port's models of the same functions stay in encyclopedia_port.cpp,
 * unannotated: natively they are the live ones and nothing calls the unit.
 * In the matching build four functions take the port's names
 * (encyclopedia_link_names.h). */
#include "encyclopedia_link_names.h"

#include <stdlib.h>
#include <string.h>

#include "crt/vc5names.h"
#include "encyclopedia.h"
#include "encyclopedia_imports.h"

/* LegoObject_Type. */
#define LEGOOBJECT_VEHICLE 1
#define LEGOOBJECT_MINIFIGURE 2
#define LEGOOBJECT_ROCKMONSTER 3
#define LEGOOBJECT_BUILDING 4
#define LEGOOBJECT_POWERCRYSTAL 6
#define LEGOOBJECT_ORE 7

/* Encyclopedia_Globs flags. */
#define ENCYCLOPEDIA_GLOB_FLAG_ACTIVE 0x1
#define ENCYCLOPEDIA_GLOB_FLAG_UPDATED 0x2

/* Encyclopedia.c's .bss (004c7e88-004c7eac), 0x24 bytes. */
typedef struct Encyclopedia_Globs {
    File** vehicleFiles;            /* +0x00 */
    File** minifigureFiles;         /* +0x04 */
    File** rockMonsterFiles;        /* +0x08 */
    File** buildingFiles;           /* +0x0c */
    File* powerCrystalFile;         /* +0x10 */
    File* oreFile;                  /* +0x14 */
    File* currFile;                 /* +0x18 */
    struct Lego_Object* currObject; /* +0x1c */
    unsigned int flags;             /* +0x20 */
} Encyclopedia_Globs;

// GLOBAL: LEGORR 0x004c7e88
static Encyclopedia_Globs encyclopediaGlobs;

/* legoGlobs' fields (lego_globs.h, finding 653), included before their
 * first use so that the functions above keep their symbol counter
 * (finding 538). */
#include "lego_globs.h"

// FUNCTION: LEGORR 0x0040e490
void encyclopedia_initialise(struct BlockRec* config, const char* gameName)
{
    unsigned int loop;
    const char* fileName;

    encyclopediaGlobs.vehicleFiles = (File**)malloc(encyclopedia_typeCounts[0] * sizeof(File*));
    memset(encyclopediaGlobs.vehicleFiles, 0, encyclopedia_typeCounts[0] * sizeof(File*));
    for (loop = 0; loop < encyclopedia_typeCounts[0]; loop++) {
        fileName = block_get_strptr(config, string_path_join(gameName,
                                                             // STRING: LEGORR 0x004a1eb0
                                                             "Encyclopedia", legoGlobs_vehicleName[loop], NULL));
        if (fileName) encyclopediaGlobs.vehicleFiles[loop] = file_open(fileName, "r");
    }

    encyclopediaGlobs.minifigureFiles = (File**)malloc(encyclopedia_typeCounts[1] * sizeof(File*));
    memset(encyclopediaGlobs.minifigureFiles, 0, encyclopedia_typeCounts[1] * sizeof(File*));
    for (loop = 0; loop < encyclopedia_typeCounts[1]; loop++) {
        fileName = block_get_strptr(config, string_path_join(gameName, "Encyclopedia", legoGlobs_miniFigureName[loop], NULL));
        if (fileName) encyclopediaGlobs.minifigureFiles[loop] = file_open(fileName, "r");
    }

    encyclopediaGlobs.rockMonsterFiles = (File**)malloc(encyclopedia_typeCounts[2] * sizeof(File*));
    memset(encyclopediaGlobs.rockMonsterFiles, 0, encyclopedia_typeCounts[2] * sizeof(File*));
    for (loop = 0; loop < encyclopedia_typeCounts[2]; loop++) {
        fileName = block_get_strptr(config, string_path_join(gameName, "Encyclopedia", legoGlobs_rockMonsterName[loop], NULL));
        if (fileName) encyclopediaGlobs.rockMonsterFiles[loop] = file_open(fileName, "r");
    }

    encyclopediaGlobs.buildingFiles = (File**)malloc(encyclopedia_typeCounts[3] * sizeof(File*));
    memset(encyclopediaGlobs.buildingFiles, 0, encyclopedia_typeCounts[3] * sizeof(File*));
    for (loop = 0; loop < encyclopedia_typeCounts[3]; loop++) {
        fileName = block_get_strptr(config, string_path_join(gameName, "Encyclopedia", legoGlobs_buildingName[loop], NULL));
        if (fileName) encyclopediaGlobs.buildingFiles[loop] = file_open(fileName, "r");
    }

    fileName = block_get_strptr(config, string_path_join(gameName, "Encyclopedia",
                                                         // STRING: LEGORR 0x004a1e9c
                                                         "PowerCrystal", NULL));
    if (fileName) encyclopediaGlobs.powerCrystalFile = file_open(fileName, "r");

    fileName = block_get_strptr(config, string_path_join(gameName, "Encyclopedia",
                                                         // STRING: LEGORR 0x004a1e98
                                                         "Ore", NULL));
    if (fileName) encyclopediaGlobs.oreFile = file_open(fileName, "r");
}

// FUNCTION: LEGORR 0x0040e700
void encyclopedia_select_object(struct Lego_Object* liveObj)
{
    unsigned int objType;
    unsigned int objIndex;
    File* file;

    legoobject_copy_two_dword_pair(liveObj, &objType, &objIndex);

    switch (objType) {
    case LEGOOBJECT_VEHICLE:
        file = encyclopediaGlobs.vehicleFiles[objIndex];
        break;
    case LEGOOBJECT_MINIFIGURE:
        file = encyclopediaGlobs.minifigureFiles[objIndex];
        break;
    case LEGOOBJECT_ROCKMONSTER:
        file = encyclopediaGlobs.rockMonsterFiles[objIndex];
        break;
    case LEGOOBJECT_BUILDING:
        file = encyclopediaGlobs.buildingFiles[objIndex];
        break;
    case LEGOOBJECT_POWERCRYSTAL:
        file = encyclopediaGlobs.powerCrystalFile;
        break;
    case LEGOOBJECT_ORE:
        file = encyclopediaGlobs.oreFile;
        break;
    default:
        encyclopediaGlobs.currFile = NULL;
        encyclopediaGlobs.flags &= ~ENCYCLOPEDIA_GLOB_FLAG_ACTIVE;
        return;
    }

    encyclopediaGlobs.flags |= ENCYCLOPEDIA_GLOB_FLAG_ACTIVE;
    encyclopediaGlobs.flags |= ENCYCLOPEDIA_GLOB_FLAG_UPDATED;
    encyclopediaGlobs.currObject = liveObj;
    encyclopediaGlobs.currFile = file;
    ui_text_output_object_release_target((void* const*)panelGlobs.infoDockTextWindow);
}

// FUNCTION: LEGORR 0x0040e7e0
void encyclopedia_disable_update(void)
{
    encyclopediaGlobs.flags &= ~ENCYCLOPEDIA_GLOB_FLAG_ACTIVE;
}

// FUNCTION: LEGORR 0x0040e7f0
void encyclopedia_update(float elapsed)
{
    char line[512];

    if ((encyclopediaGlobs.flags & ENCYCLOPEDIA_GLOB_FLAG_ACTIVE) &&
        !request_record_ready_flag4_target(PANEL_ENCYCLOPEDIA)) {
        if (encyclopediaGlobs.flags & ENCYCLOPEDIA_GLOB_FLAG_UPDATED) {
            if (encyclopediaGlobs.currFile) {
                file_seek(encyclopediaGlobs.currFile, 0, SEEK_SET);
                while (file_gets(line, sizeof(line), encyclopediaGlobs.currFile)) {
                    panel_text_window_print_f(panelGlobs.infoDockTextWindow, "%s", line);
                }
            } else {
                // STRING: LEGORR 0x004a1ec0
                panel_text_window_print_f(panelGlobs.infoDockTextWindow, "Object has no encyclopedia file.");
            }
            encyclopediaGlobs.flags &= ~ENCYCLOPEDIA_GLOB_FLAG_UPDATED;
        }
        ui_text_output_position_and_draw(panelGlobs.infoDockTextWindow, 0, elapsed);
    }
}

// FUNCTION: LEGORR 0x0040e8d0
void encyclopedia_draw_select_box(struct ViewProjectionAdapter* view)
{
    if ((encyclopediaGlobs.flags & ENCYCLOPEDIA_GLOB_FLAG_ACTIVE) &&
        (panelGlobs.panelTable[PANEL_ENCYCLOPEDIA].flags & PANEL_FLAG_OUT) &&
        encyclopediaGlobs.currObject) {
        lego_selected_entity_ground_marker_render((struct EntitySlot*)encyclopediaGlobs.currObject,
                                                  view, 0.0f, 1.0f, 0.0f);
    }
}

// FUNCTION: LEGORR 0x0040e910
void encyclopedia_remove_current_reference(struct Lego_Object* dead)
{
    if (encyclopediaGlobs.currObject == dead) encyclopediaGlobs.currObject = NULL;
}
