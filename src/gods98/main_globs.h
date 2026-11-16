// SPDX-License-Identifier: LGPL-3.0-or-later
/* Main.c's state (G98-005), the original's mainGlobs (00505800-00506090,
 * finding 679). main.c defines it with `= { 0 }`, so it stays in the
 * unit's .bss (finding 669).
 *
 * Thirteen of its fields are shared with the port, which defines them
 * under its own names in the port tail (main_imports.h lists them). In the
 * matching build the macros below make each name the mainGlobs field the
 * original reads, so no unit references the tail's definitions (finding
 * 668, follow-up 3). The other builds keep the port's variables: natively
 * main.c compiles to nothing, and in the MinGW reference build the port's
 * Windows runtime still reads its own copies.
 *
 * The symbol counter (finding 538) decides where a unit includes it. The
 * first include costs three symbols (the file, the struct tag and
 * mainGlobs); a later one costs none. A unit either
 *   - removes three of its port declarations in the matching build and
 *     includes this file in their place, at the end of its import header
 *     (lego_unit_imports.h, frontend_unit_imports.h, mesh_imports.h), or
 *   - gets the struct from crt/crt.h, which includes this file in place of
 *     its three declarations with LEGORRR_MAIN_GLOBS_NO_NAMES, and includes
 *     it again for the names after its last declaration of them, or
 *   - includes it before the first function that uses the names, where the
 *     three symbols happen to move no function's score.
 * The port declarations a unit keeps stay, unused, in the matching build:
 * Deviation (scoring only), declaration-only (ADR 0005 entropy rule 4). No
 * shared C++ header includes this file. */
#ifndef LEGORRR_GODS98_MAIN_GLOBS_H
#define LEGORRR_GODS98_MAIN_GLOBS_H

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(LEGORRR_MAIN_GLOBS_OWNER)
/* The readers' view of main.c's Main_Globs: the same layout, with the two
 * fields only Main.c uses as plain storage. main.c declares the original's
 * types (Main_State, Main_StateChangeData, the Main_Globs typedef), whose
 * symbols would move each reader's counter by five more (finding 538):
 * Deviation (scoring only), declaration-only. */
extern struct Main_Globs {
    void* hWnd;                         /* 00505800 */
    void* hInst;                        /* 00505804 */
    unsigned int active;                /* 00505808 */
    unsigned int exit;                  /* 0050580c */
    char* className;                    /* 00505810 */
    char programName[256];              /* 00505814 */
    void* lpD3DRM;                      /* 00505914 IDirect3DRM3 */
    void* device;                       /* 00505918 IDirect3DRMDevice3 */
    void* imDevice;                     /* 0050591c IDirect3DDevice3 */
    unsigned int fogMethod;             /* 00505920 */
    int appWidth;                       /* 00505924 */
    int appHeight;                      /* 00505928 */
    void* currState[3];                 /* 0050592c Main_State */
    int stateSet;                       /* 00505938 */
    float fixedFrameTiming;             /* 0050593c */
    unsigned int renderStates[200][2];  /* 00505940 Main_StateChangeData */
    unsigned int style;                 /* 00505f80 */
    unsigned int flags;                 /* 00505f84 */
    int programmerLevel;                /* 00505f88 */
    char startLevel[256];               /* 00505f8c */
    unsigned int clFlags;               /* 0050608c */
} mainGlobs;
#endif

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GODS98_MAIN_GLOBS_H */

/* The port's names, defined at the first include without
 * LEGORRR_MAIN_GLOBS_NO_NAMES. crt/crt.h includes this file that way in
 * place of its own declarations, so the unit's later include, which costs
 * no symbol, defines the names after the unit's last declaration of them. */
#if !defined(LEGORRR_MAIN_GLOBS_NO_NAMES) && !defined(LEGORRR_MAIN_GLOBS_NAMES)
#define LEGORRR_MAIN_GLOBS_NAMES
#define g_platform_cursor_window_handle (mainGlobs.hWnd)
#define g_main_instance_handle          (mainGlobs.hInst)
#define s_window_application_active     (mainGlobs.active)
#define s_window_close_requested        (mainGlobs.exit)
#define s_main_d3drm                    (mainGlobs.lpD3DRM)
#define s_main_device                   (mainGlobs.device)
#define s_main_im_device                (mainGlobs.imDevice)
#define s_scene_fog_mode                (mainGlobs.fogMethod)
#define s_display_w                     (mainGlobs.appWidth)
#define s_display_h                     (mainGlobs.appHeight)
#define s_engine_feature_word           (mainGlobs.flags)
#define s_application_mode              (mainGlobs.programmerLevel)
#define s_render_feature_flags          (mainGlobs.clFlags)
#endif
