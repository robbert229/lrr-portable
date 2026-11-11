// SPDX-License-Identifier: LGPL-3.0-or-later
/* Keys.c — original translation unit G98-018 (text 00485a20-00485e80,
 * literals 004ab180-004ab5f7; finding 513).
 *
 * The key-name table: DirectInput scan code to its "KEY_*"/"KEYPAD_*" name,
 * and the case-insensitive reverse lookup the interface config loaders use
 * for shortcut keys. Rebuilt in the original's shape (issue #416, ADR 0005):
 * one file for the unit, its functions in original order and the
 * C-compatible subset, so the matching build compiles it as C. The native
 * port compiles it as C++.
 *
 * The unit has no .bss of its own. Its table is a common symbol. */
#include <string.h>

#include "keys.h"
#include "crt/vc5names.h"

/* Key names by scan code, a common symbol. Unnamed codes stay NULL.
 *
 * Deviation (scoring only): the original table has 256 slots, and its
 * walk ends at 005769e0, where Files.c's directory-token table begins.
 * Reccmp names an immediate only when it falls inside an annotated
 * variable, sizing the original's from this definition, so with 256 slots
 * the end bound compares as two unrelated addresses. The spare slot makes
 * both sides read key_names[256]. Nothing reads or writes it; the clear
 * and the walk cover KEY_NAMES_COUNT slots, as in the original. */
// GLOBAL: LEGORR 0x005765e0
const char* key_names[KEY_NAMES_COUNT + 1];

/* Keys_Initialise: clear the table, then name the 101 known scan codes. The
 * literals are pooled, so the link places them in reverse order of first
 * use: KEY_ESCAPE last, at 004ab5ec. */
// FUNCTION: LEGORR 0x00485a20
void keyboard_key_names_init(void)
{
    memset(key_names, 0, KEY_NAMES_COUNT * sizeof(key_names[0]));

    // STRING: LEGORR 0x004ab5ec
    key_names[0x01] = "KEY_ESCAPE";
    // STRING: LEGORR 0x004ab5e4
    key_names[0x02] = "KEY_ONE";
    // STRING: LEGORR 0x004ab5dc
    key_names[0x03] = "KEY_TWO";
    // STRING: LEGORR 0x004ab5d0
    key_names[0x04] = "KEY_THREE";
    // STRING: LEGORR 0x004ab5c4
    key_names[0x05] = "KEY_FOUR";
    // STRING: LEGORR 0x004ab5b8
    key_names[0x06] = "KEY_FIVE";
    // STRING: LEGORR 0x004ab5b0
    key_names[0x07] = "KEY_SIX";
    // STRING: LEGORR 0x004ab5a4
    key_names[0x08] = "KEY_SEVEN";
    // STRING: LEGORR 0x004ab598
    key_names[0x09] = "KEY_EIGHT";
    // STRING: LEGORR 0x004ab58c
    key_names[0x0a] = "KEY_NINE";
    // STRING: LEGORR 0x004ab580
    key_names[0x0b] = "KEY_ZERO";
    // STRING: LEGORR 0x004ab574
    key_names[0x0c] = "KEY_MINUS";
    // STRING: LEGORR 0x004ab568
    key_names[0x0d] = "KEY_EQUALS";
    // STRING: LEGORR 0x004ab558
    key_names[0x0e] = "KEY_BACKSPACE";
    // STRING: LEGORR 0x004ab550
    key_names[0x0f] = "KEY_TAB";
    // STRING: LEGORR 0x004ab548
    key_names[0x10] = "KEY_Q";
    // STRING: LEGORR 0x004ab540
    key_names[0x11] = "KEY_W";
    // STRING: LEGORR 0x004ab538
    key_names[0x12] = "KEY_E";
    // STRING: LEGORR 0x004ab530
    key_names[0x13] = "KEY_R";
    // STRING: LEGORR 0x004ab528
    key_names[0x14] = "KEY_T";
    // STRING: LEGORR 0x004ab520
    key_names[0x15] = "KEY_Y";
    // STRING: LEGORR 0x004ab518
    key_names[0x16] = "KEY_U";
    // STRING: LEGORR 0x004ab510
    key_names[0x17] = "KEY_I";
    // STRING: LEGORR 0x004ab508
    key_names[0x18] = "KEY_O";
    // STRING: LEGORR 0x004ab500
    key_names[0x19] = "KEY_P";
    // STRING: LEGORR 0x004ab4f0
    key_names[0x1a] = "KEY_LEFTBRACE";
    // STRING: LEGORR 0x004ab4e0
    key_names[0x1b] = "KEY_RIGHTBRACE";
    // STRING: LEGORR 0x004ab4d4
    key_names[0x1c] = "KEY_RETURN";
    // STRING: LEGORR 0x004ab4c4
    key_names[0x1d] = "KEY_LEFTCTRL";
    // STRING: LEGORR 0x004ab4bc
    key_names[0x1e] = "KEY_A";
    // STRING: LEGORR 0x004ab4b4
    key_names[0x1f] = "KEY_S";
    // STRING: LEGORR 0x004ab4ac
    key_names[0x20] = "KEY_D";
    // STRING: LEGORR 0x004ab4a4
    key_names[0x21] = "KEY_F";
    // STRING: LEGORR 0x004ab49c
    key_names[0x22] = "KEY_G";
    // STRING: LEGORR 0x004ab494
    key_names[0x23] = "KEY_H";
    // STRING: LEGORR 0x004ab48c
    key_names[0x24] = "KEY_J";
    // STRING: LEGORR 0x004ab484
    key_names[0x25] = "KEY_K";
    // STRING: LEGORR 0x004ab47c
    key_names[0x26] = "KEY_L";
    // STRING: LEGORR 0x004ab46c
    key_names[0x27] = "KEY_SEMICOLON";
    // STRING: LEGORR 0x004ab464
    key_names[0x28] = "KEY_AT";
    // STRING: LEGORR 0x004ab450
    key_names[0x29] = "KEY_RSINGLEQUOTE";
    // STRING: LEGORR 0x004ab440
    key_names[0x2a] = "KEY_LEFTSHIFT";
    // STRING: LEGORR 0x004ab434
    key_names[0x2b] = "KEY_HASH";
    // STRING: LEGORR 0x004ab42c
    key_names[0x2c] = "KEY_Z";
    // STRING: LEGORR 0x004ab424
    key_names[0x2d] = "KEY_X";
    // STRING: LEGORR 0x004ab41c
    key_names[0x2e] = "KEY_C";
    // STRING: LEGORR 0x004ab414
    key_names[0x2f] = "KEY_V";
    // STRING: LEGORR 0x004ab40c
    key_names[0x30] = "KEY_B";
    // STRING: LEGORR 0x004ab404
    key_names[0x31] = "KEY_N";
    // STRING: LEGORR 0x004ab3fc
    key_names[0x32] = "KEY_M";
    // STRING: LEGORR 0x004ab3ec
    key_names[0x33] = "KEY_LEFTARROW";
    // STRING: LEGORR 0x004ab3dc
    key_names[0x34] = "KEY_RIGHTARROW";
    // STRING: LEGORR 0x004ab3c8
    key_names[0x35] = "KEY_QUESTIONMARK";
    // STRING: LEGORR 0x004ab3b8
    key_names[0x36] = "KEY_RIGHTSHIFT";
    // STRING: LEGORR 0x004ab3a8
    key_names[0x37] = "KEYPAD_ASTERISK";
    // STRING: LEGORR 0x004ab3a0
    key_names[0x38] = "KEY_ALT";
    // STRING: LEGORR 0x004ab394
    key_names[0x39] = "KEY_SPACE";
    // STRING: LEGORR 0x004ab388
    key_names[0x3a] = "KEY_CAPLOCK";
    // STRING: LEGORR 0x004ab380
    key_names[0x3b] = "KEY_F1";
    // STRING: LEGORR 0x004ab378
    key_names[0x3c] = "KEY_F2";
    // STRING: LEGORR 0x004ab370
    key_names[0x3d] = "KEY_F3";
    // STRING: LEGORR 0x004ab368
    key_names[0x3e] = "KEY_F4";
    // STRING: LEGORR 0x004ab360
    key_names[0x3f] = "KEY_F5";
    // STRING: LEGORR 0x004ab358
    key_names[0x40] = "KEY_F6";
    // STRING: LEGORR 0x004ab350
    key_names[0x41] = "KEY_F7";
    // STRING: LEGORR 0x004ab348
    key_names[0x42] = "KEY_F8";
    // STRING: LEGORR 0x004ab340
    key_names[0x43] = "KEY_F9";
    // STRING: LEGORR 0x004ab338
    key_names[0x44] = "KEY_F10";
    // STRING: LEGORR 0x004ab328
    key_names[0x45] = "KEYPAD_NUMLOCK";
    // STRING: LEGORR 0x004ab318
    key_names[0x46] = "KEY_SCROLLLOCK";
    // STRING: LEGORR 0x004ab30c
    key_names[0x47] = "KEYPAD_7";
    // STRING: LEGORR 0x004ab300
    key_names[0x48] = "KEYPAD_8";
    // STRING: LEGORR 0x004ab2f4
    key_names[0x49] = "KEYPAD_9";
    // STRING: LEGORR 0x004ab2e4
    key_names[0x4a] = "KEYPAD_MINUS";
    // STRING: LEGORR 0x004ab2d8
    key_names[0x4b] = "KEYPAD_4";
    // STRING: LEGORR 0x004ab2cc
    key_names[0x4c] = "KEYPAD_5";
    // STRING: LEGORR 0x004ab2c0
    key_names[0x4d] = "KEYPAD_6";
    // STRING: LEGORR 0x004ab2b4
    key_names[0x4e] = "KEYPAD_PLUS";
    // STRING: LEGORR 0x004ab2a8
    key_names[0x4f] = "KEYPAD_1";
    // STRING: LEGORR 0x004ab29c
    key_names[0x50] = "KEYPAD_2";
    // STRING: LEGORR 0x004ab290
    key_names[0x51] = "KEYPAD_3";
    // STRING: LEGORR 0x004ab284
    key_names[0x52] = "KEYPAD_0";
    // STRING: LEGORR 0x004ab274
    key_names[0x53] = "KEYPAD_DELETE";
    // STRING: LEGORR 0x004ab264
    key_names[0x56] = "KEY_BACKSLASH";
    // STRING: LEGORR 0x004ab25c
    key_names[0x57] = "KEY_F11";
    // STRING: LEGORR 0x004ab254
    key_names[0x58] = "KEY_F12";
    // STRING: LEGORR 0x004ab244
    key_names[0x9c] = "KEYPAD_ENTER";
    // STRING: LEGORR 0x004ab234
    key_names[0x9d] = "KEY_RIGHTCTRL";
    // STRING: LEGORR 0x004ab220
    key_names[0xb5] = "KEYPAD_FORWARDSLASH";
    // STRING: LEGORR 0x004ab210
    key_names[0xb7] = "KEY_PRINTSCREEN";
    // STRING: LEGORR 0x004ab204
    key_names[0xb8] = "KEY_ALTGR";
    // STRING: LEGORR 0x004ab1f8
    key_names[0xc7] = "KEY_HOME";
    // STRING: LEGORR 0x004ab1e8
    key_names[0xc8] = "KEY_CURSORUP";
    // STRING: LEGORR 0x004ab1dc
    key_names[0xc9] = "KEY_PGUP";
    // STRING: LEGORR 0x004ab1cc
    key_names[0xcb] = "KEY_CURSORLEFT";
    // STRING: LEGORR 0x004ab1bc
    key_names[0xcd] = "KEY_CURSORRIGHT";
    // STRING: LEGORR 0x004ab1b4
    key_names[0xcf] = "KEY_END";
    // STRING: LEGORR 0x004ab1a4
    key_names[0xd0] = "KEY_CURSORDOWN";
    // STRING: LEGORR 0x004ab198
    key_names[0xd1] = "KEY_PGDN";
    // STRING: LEGORR 0x004ab18c
    key_names[0xd2] = "KEY_INSERT";
    // STRING: LEGORR 0x004ab180
    key_names[0xd3] = "KEY_DELETE";
}

/* Key_Find: the scan code whose name equals `name`, ignoring case. */
// FUNCTION: LEGORR 0x00485e30
int named_option_index_find_case_insensitive(const char* name,
                                             unsigned char* out_index)
{
    unsigned int loop;

    for (loop = 0; loop < KEY_NAMES_COUNT; loop++) {
        if (key_names[loop] && _strcmpi(key_names[loop], name) == 0) {
            *out_index = (unsigned char)loop;
            return 1;
        }
    }

    return 0;
}
