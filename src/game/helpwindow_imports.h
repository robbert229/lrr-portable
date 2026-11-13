// SPDX-License-Identifier: LGPL-3.0-or-later
/* HelpWindow.c (GAME-018) imports: C declarations of the callees and globals
 * the unit uses from other modules (ADR 0005 step 6; finding 595). Included
 * only by helpwindow.c and test_c_boundaries.cpp, so the other C units'
 * symbol numbering does not change (finding 568, "Unit-private imports
 * header"). Each declaration names a definition with C linkage, with the
 * port's parameter types; helpwindow.c casts its original-layout records
 * to them. */
#ifndef LEGORRR_GAME_HELPWINDOW_IMPORTS_H
#define LEGORRR_GAME_HELPWINDOW_IMPORTS_H

#include <stdint.h>

#include "gods98/config.h"
#include "gods98/font_api.h"
#include "gods98/images.h"
#include "gods98/textwindow.h"
#include "gods98/utils.h"

#ifdef __cplusplus
extern "C" {
#endif

struct TextureSurface;

#if defined(_MSC_VER) && _MSC_VER < 1200 && !defined(__cplusplus)
/* Images.c is images_unit.c in the matching build (finding 632): the
 * C callers' calls go to the unit (images_link_names.h). */
#define image_surface_blit_transformed Image_SetupTrans /* 0047dbf0 */
#endif
/* G98-011 Images.c (images_unit.cpp): Image_SetupTrans. */
void image_surface_blit_transformed(struct TextureSurface* surface,
                                    float low_red, float low_green,
                                    float low_blue, float high_red,
                                    float high_green, float high_blue); /* 0047dbf0 */
/* G98-017 TextWindow.c (text_window.cpp): TextWindow_PrintF. */
void text_buffer_append_format(void* record, const char* format, ...); /* 00485230 */

/* GAME-010 Dependencies.c (dependencies.cpp). The port's entries have C++
 * linkage: the matches collector and the occupancy pass are in namespace
 * unported_ledger_stubs, and the three flag accessors are declared in the
 * frozen level_for_*.hpp headers. PORT-ONLY adapters with the original
 * signatures are at the end of helpwindow_port.cpp. The matches record is
 * the original's: four arrays of 32 dwords and the count at +0x200. */
void help_window_dependencies_matches_adapter(int type, int index,
                                              int level,
                                              void* matches); /* 0040b000 */
void help_window_dependencies_pass_adapter(void);              /* 0040b1b0 */
unsigned int help_window_dependencies_flag4_adapter(int type, int index,
                                                    int level); /* 0040b250 */
void help_window_dependencies_flag100_set_adapter(int type, int index,
                                                  int level);   /* 0040b2a0 */
unsigned int help_window_dependencies_flag100_adapter(int type, int index,
                                                      int level); /* 0040b2e0 */

/* GAME-020 Interface.c (interface_unit.c): an object type's dependency
 * icon. */
void* input_binding_default_value_get(int objType, int objIndex,
                                      int available);         /* 0041cb90 */

/* GAME-021 Lego.c (lego_unit.c). */
void lego_transition_control(int on, unsigned int flags);     /* 00425c30 */
void objective_score_text_origin_set(const float* pair);      /* 004262e0 */
void media_action_feedback_sound_dispatch(int event);         /* 00428800 */
extern struct BlockRec* legoGlobs_config;                     /* 005560c0 */
extern char** legoGlobs_vehicleName;                          /* 005561e8 */
extern char** legoGlobs_miniFigureName;                       /* 005561ec */
extern char** legoGlobs_buildingName;                         /* 005561f4 */
extern struct Image* legoGlobs_NextButtonImage;               /* 00556eec */
/* legoGlobs' type counts (0055627c-0055628c). The port's copies are
 * entity.cpp's C++ s_catalog_counts, so the unit reads them at their
 * offset in legoGlobs, as lego_unit.c does (lego_unit_imports.h). */
#define helpWindow_typeCounts ((unsigned int*)((char*)&legoGlobs_config + 0x1bc))

/* GAME-023 LegoObject.c (legoobject_unit.c). */
const char* legoobject_the_name_get(unsigned int kind, int index); /* 00439790 */

/* GAME-028 NERPs (tutorial_script.c). */
int nerpfunc_get_tutorial_flags(unsigned int* stack);          /* 00456240 */

/* GAME-046 ToolTip.c (frontend.h): ToolTip_SetContent is a port adapter. */
void tooltip_set_content(int index, const char* format, ...); /* 0046b520 */
void timed_effect_update_request_set_corpus(int index);      /* 0046b770 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_HELPWINDOW_IMPORTS_H */
