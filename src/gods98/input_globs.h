// SPDX-License-Identifier: LGPL-3.0-or-later
/* Input.c's state in the matching build: three commons, as in the
 * original. The previous frame's key map (0076aa00), the device and mouse
 * state (inputGlobs, 0076ab00, 0x80 bytes) and the key map (0076ab80).
 * The key maps are objects of their own: the original keeps the keyboard
 * device in a register across their inline copies (0047eef0), which a
 * struct member's write would not allow. Nothing in the image reads the
 * words marked "never read", and LINK's /OPT:REF drops a common nothing
 * references, so separate commons cannot keep the fields' offsets. LINK
 * lays out an object's commons in reverse symbol-table order, which MSVC
 * 5.0 sorts by a hash of the name (finding 690): these names give the
 * original's order. The macros keep the converted units' names;
 * input_key_names.h renames the key maps. */
#ifndef LEGORRR_GODS98_INPUT_GLOBS_H
#define LEGORRR_GODS98_INPUT_GLOBS_H

struct Input_Globs {
    struct IDirectInputA* lpdi;                  /* +0x00 */
    struct IDirectInputDeviceA* lpdiKeyboard;    /* +0x04 */
    unsigned int unknown_08;                     /* +0x08 never read */
    int keyboardAcquired;                        /* +0x0c */
    unsigned int unknown_10[5];                  /* +0x10 never read */
    unsigned int msx;                            /* +0x24 */
    unsigned int msy;                            /* +0x28 */
    unsigned int unknown_2c[2];                  /* +0x2c never read */
    int diffX;                                   /* +0x34 */
    int diffY;                                   /* +0x38 */
    unsigned int mslb;                           /* +0x3c */
    unsigned int msrb;                           /* +0x40 */
    unsigned int unknown_44;                     /* +0x44 never read */
    unsigned int lClicked;                       /* +0x48 */
    unsigned int word_4c;                        /* +0x4c */
    unsigned int word_50;                        /* +0x50 */
    unsigned int word_54;                        /* +0x54 */
    unsigned int word_58;                        /* +0x58 */
    unsigned int unknown_5c[9];                  /* +0x5c never read */
};

extern struct Input_Globs inputGlobs;

#define s_input_lpdi (inputGlobs.lpdi)
#define s_input_lpdi_keyboard (inputGlobs.lpdiKeyboard)
#define s_input_keyboard_acquired (inputGlobs.keyboardAcquired)
#define s_renderer_word_a (inputGlobs.msx)
#define s_renderer_word_b (inputGlobs.msy)
#define s_cursor_delta_x (inputGlobs.diffX)
#define s_cursor_delta_y (inputGlobs.diffY)
#define g_renderer_word_c (inputGlobs.mslb)
#define s_window_mouse_word_40 (inputGlobs.msrb)
#define s_window_mouse_word_48 (inputGlobs.lClicked)
#define s_window_mouse_word_4c (inputGlobs.word_4c)
#define s_window_mouse_word_50 (inputGlobs.word_50)
#define s_window_mouse_word_54 (inputGlobs.word_54)
#define s_window_mouse_word_58 (inputGlobs.word_58)

#endif /* LEGORRR_GODS98_INPUT_GLOBS_H */
