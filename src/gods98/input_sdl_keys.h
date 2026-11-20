// SPDX-License-Identifier: LGPL-3.0-or-later
/* The keyboard substitution's scan-code table (ADR 0007,
 * `// [library:input]`): Input.c fills its DirectInput key map, indexed by
 * DIK_* code, from SDL's keyboard state, indexed by SDL scancode. SDL's
 * scancodes are USB HID usages; the DIK_* codes are PC scan-code set 1,
 * with 0x80 added for the E0-prefixed keys. A key DirectInput has no code
 * for maps to 0 and is never reported. */
#ifndef LEGORRR_GODS98_INPUT_SDL_KEYS_H
#define LEGORRR_GODS98_INPUT_SDL_KEYS_H

#include <SDL3/SDL_scancode.h>

static inline unsigned char input_sdl_scancode_dik(SDL_Scancode scancode)
{
    if (scancode >= SDL_SCANCODE_A && scancode <= SDL_SCANCODE_Z) {
        /* DIK_A..DIK_Z follow the keyboard's rows, not the alphabet. */
        static const unsigned char letters[26] = {
            0x1e, 0x30, 0x2e, 0x20, 0x12, 0x21, 0x22, 0x23, 0x17, 0x24, 0x25, 0x26, 0x32,
            0x31, 0x18, 0x19, 0x10, 0x13, 0x1f, 0x14, 0x16, 0x2f, 0x11, 0x2d, 0x15, 0x2c,
        };
        return letters[scancode - SDL_SCANCODE_A];
    }
    if (scancode >= SDL_SCANCODE_1 && scancode <= SDL_SCANCODE_0) {
        /* DIK_1..DIK_9, DIK_0. */
        return (unsigned char)(0x02 + (scancode - SDL_SCANCODE_1));
    }
    if (scancode >= SDL_SCANCODE_F1 && scancode <= SDL_SCANCODE_F10) {
        return (unsigned char)(0x3b + (scancode - SDL_SCANCODE_F1));
    }

    switch (scancode) {
    case SDL_SCANCODE_RETURN: return 0x1c;       /* DIK_RETURN */
    case SDL_SCANCODE_ESCAPE: return 0x01;       /* DIK_ESCAPE */
    case SDL_SCANCODE_BACKSPACE: return 0x0e;    /* DIK_BACK */
    case SDL_SCANCODE_TAB: return 0x0f;          /* DIK_TAB */
    case SDL_SCANCODE_SPACE: return 0x39;        /* DIK_SPACE */
    case SDL_SCANCODE_MINUS: return 0x0c;        /* DIK_MINUS */
    case SDL_SCANCODE_EQUALS: return 0x0d;       /* DIK_EQUALS */
    case SDL_SCANCODE_LEFTBRACKET: return 0x1a;  /* DIK_LBRACKET */
    case SDL_SCANCODE_RIGHTBRACKET: return 0x1b; /* DIK_RBRACKET */
    case SDL_SCANCODE_BACKSLASH: return 0x2b;    /* DIK_BACKSLASH */
    case SDL_SCANCODE_NONUSHASH: return 0x2b;    /* DIK_BACKSLASH */
    case SDL_SCANCODE_SEMICOLON: return 0x27;    /* DIK_SEMICOLON */
    case SDL_SCANCODE_APOSTROPHE: return 0x28;   /* DIK_APOSTROPHE */
    case SDL_SCANCODE_GRAVE: return 0x29;        /* DIK_GRAVE */
    case SDL_SCANCODE_COMMA: return 0x33;        /* DIK_COMMA */
    case SDL_SCANCODE_PERIOD: return 0x34;       /* DIK_PERIOD */
    case SDL_SCANCODE_SLASH: return 0x35;        /* DIK_SLASH */
    case SDL_SCANCODE_CAPSLOCK: return 0x3a;     /* DIK_CAPITAL */
    case SDL_SCANCODE_F11: return 0x57;          /* DIK_F11 */
    case SDL_SCANCODE_F12: return 0x58;          /* DIK_F12 */
    case SDL_SCANCODE_F13: return 0x64;          /* DIK_F13 */
    case SDL_SCANCODE_F14: return 0x65;          /* DIK_F14 */
    case SDL_SCANCODE_F15: return 0x66;          /* DIK_F15 */
    case SDL_SCANCODE_PRINTSCREEN: return 0xb7;  /* DIK_SYSRQ */
    case SDL_SCANCODE_SCROLLLOCK: return 0x46;   /* DIK_SCROLL */
    case SDL_SCANCODE_PAUSE: return 0xc5;        /* DIK_PAUSE */
    case SDL_SCANCODE_INSERT: return 0xd2;       /* DIK_INSERT */
    case SDL_SCANCODE_HOME: return 0xc7;         /* DIK_HOME */
    case SDL_SCANCODE_PAGEUP: return 0xc9;       /* DIK_PRIOR */
    case SDL_SCANCODE_DELETE: return 0xd3;       /* DIK_DELETE */
    case SDL_SCANCODE_END: return 0xcf;          /* DIK_END */
    case SDL_SCANCODE_PAGEDOWN: return 0xd1;     /* DIK_NEXT */
    case SDL_SCANCODE_RIGHT: return 0xcd;        /* DIK_RIGHT */
    case SDL_SCANCODE_LEFT: return 0xcb;         /* DIK_LEFT */
    case SDL_SCANCODE_DOWN: return 0xd0;         /* DIK_DOWN */
    case SDL_SCANCODE_UP: return 0xc8;           /* DIK_UP */
    case SDL_SCANCODE_NUMLOCKCLEAR: return 0x45; /* DIK_NUMLOCK */
    case SDL_SCANCODE_KP_DIVIDE: return 0xb5;    /* DIK_DIVIDE */
    case SDL_SCANCODE_KP_MULTIPLY: return 0x37;  /* DIK_MULTIPLY */
    case SDL_SCANCODE_KP_MINUS: return 0x4a;     /* DIK_SUBTRACT */
    case SDL_SCANCODE_KP_PLUS: return 0x4e;      /* DIK_ADD */
    case SDL_SCANCODE_KP_ENTER: return 0x9c;     /* DIK_NUMPADENTER */
    case SDL_SCANCODE_KP_1: return 0x4f;         /* DIK_NUMPAD1 */
    case SDL_SCANCODE_KP_2: return 0x50;         /* DIK_NUMPAD2 */
    case SDL_SCANCODE_KP_3: return 0x51;         /* DIK_NUMPAD3 */
    case SDL_SCANCODE_KP_4: return 0x4b;         /* DIK_NUMPAD4 */
    case SDL_SCANCODE_KP_5: return 0x4c;         /* DIK_NUMPAD5 */
    case SDL_SCANCODE_KP_6: return 0x4d;         /* DIK_NUMPAD6 */
    case SDL_SCANCODE_KP_7: return 0x47;         /* DIK_NUMPAD7 */
    case SDL_SCANCODE_KP_8: return 0x48;         /* DIK_NUMPAD8 */
    case SDL_SCANCODE_KP_9: return 0x49;         /* DIK_NUMPAD9 */
    case SDL_SCANCODE_KP_0: return 0x52;         /* DIK_NUMPAD0 */
    case SDL_SCANCODE_KP_PERIOD: return 0x53;    /* DIK_DECIMAL */
    case SDL_SCANCODE_KP_EQUALS: return 0x8d;    /* DIK_NUMPADEQUALS */
    case SDL_SCANCODE_NONUSBACKSLASH: return 0x56; /* DIK_OEM_102 */
    case SDL_SCANCODE_APPLICATION: return 0xdd;  /* DIK_APPS */
    case SDL_SCANCODE_LCTRL: return 0x1d;        /* DIK_LCONTROL */
    case SDL_SCANCODE_LSHIFT: return 0x2a;       /* DIK_LSHIFT */
    case SDL_SCANCODE_LALT: return 0x38;         /* DIK_LMENU */
    case SDL_SCANCODE_LGUI: return 0xdb;         /* DIK_LWIN */
    case SDL_SCANCODE_RCTRL: return 0x9d;        /* DIK_RCONTROL */
    case SDL_SCANCODE_RSHIFT: return 0x36;       /* DIK_RSHIFT */
    case SDL_SCANCODE_RALT: return 0xb8;         /* DIK_RMENU */
    case SDL_SCANCODE_RGUI: return 0xdc;         /* DIK_RWIN */
    default: return 0;
    }
}

#endif /* LEGORRR_GODS98_INPUT_SDL_KEYS_H */
