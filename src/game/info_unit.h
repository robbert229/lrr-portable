// SPDX-License-Identifier: LGPL-3.0-or-later
/* Info.c (original translation unit GAME-019, finding 505): the in-game
 * information messages. Forty message types ("Info_CrystalFound" ...
 * "Info_GenericMonster") each have a text, an icon image, a sound, a
 * cool-down timer and flags, configured from Lego.cfg. Info_Send queues a
 * message instance (its object or block, and its own text) under its type;
 * the queued types stack as icons down the left of the screen, and the
 * first one's text shows in the information panel. Clicking an icon brings
 * its type to the front.
 *
 * C-compatible header (ADR 0005). info_unit.c is compiled as C in the
 * matching build and as C++ in the native port. Shared C++ headers do not
 * include this file: every include and declaration a header adds shifts
 * MSVC's symbol numbering (finding 506). src/game/test_c_boundaries.cpp
 * checks it against the C++ declarations of the shared functions.
 *
 * The offsets are the original's Win32 layout.
 *
 * The port's message queue (message.hpp's MessageConfig and queue records,
 * the hooks and the sidecar text table) is a different contract. It keeps
 * the port's functions, unannotated, in the port tail (info_port.cpp), and
 * the native callers use those (finding 567). The functions marked
 * "shared" below keep a port name: their callers already use the
 * original's contract. */
#ifndef LEGORRR_GAME_INFO_UNIT_H
#define LEGORRR_GAME_INFO_UNIT_H

#include <stdint.h>

#include "gods98/images.h"

#ifdef __cplusplus
extern "C" {
#endif

#define INFO_TYPE_COUNT 40
#define INFO_MESSAGE_MAX 40

/* The message types the unit addresses by number. */
#define INFO_TYPE_LANDSLIDE 6
#define INFO_TYPE_CAVEIN 7
#define INFO_TYPE_LEGOMANDEATH 10
#define INFO_TYPE_VEHICLEDEATH 11
#define INFO_TYPE_BUILDINGDEATH 12
#define INFO_TYPE_GENERICDEATH 38

/* InfoData flags (+0x10). */
#define INFO_DATA_FLAG_AUTOGAMESPEED 0x10000 /* slow the game to 1x */
#define INFO_DATA_FLAG_PLAYALWAYS 0x20000    /* sound even when queued */

/* InfoGlobs flags (+0x6f0). */
#define INFO_FLAG_DOCKED 0x1        /* the first message is in the panel */
#define INFO_FLAG_TEXTCHANGED 0x2   /* reprint the panel text */
#define INFO_FLAG_ENABLED 0x4
#define INFO_FLAG_SOUNDS 0x8        /* play the types' sounds */
#define INFO_FLAG_AUTOGAMESPEED 0x10

typedef struct InfoPoint2I {
    int x, y;
} InfoPoint2I;

/* One message type's configuration, 0x14 bytes. */
typedef struct InfoData {
    char* text;               /* +0x00: compiled (info_compile_text) */
    Image* image;             /* +0x04 */
    int sfxType;              /* +0x08 */
    float timer;              /* +0x0c: the sound's cool-down */
    unsigned int flags;       /* +0x10 */
} InfoData;

/* One sent message, 0x14 bytes. */
typedef struct InfoMessageInstance {
    InfoPoint2I blockPos;                 /* +0x00: -1, -1 for none */
    void* object;                         /* +0x08: LegoObject* */
    char* text;                           /* +0x0c: compiled, or NULL */
    struct InfoMessageInstance* next;     /* +0x10 */
} InfoMessageInstance;

/* One queued message type, 0x14 bytes. */
typedef struct InfoMessage {
    InfoMessageInstance* instanceList;    /* +0x00 */
    unsigned int instanceCount;           /* +0x04 */
    int type;                             /* +0x08 */
    float x, y;                           /* +0x0c: the icon's position */
} InfoMessage;

/* Info.c's state, 0x6f4 bytes at 004dc658 on Win32. */
typedef struct InfoGlobs {
    const char* infoName[INFO_TYPE_COUNT];              /* 004dc658 */
    InfoData infoDataTable[INFO_TYPE_COUNT];            /* 004dc6f8 */
    InfoMessage infoMessageTable[INFO_MESSAGE_MAX];     /* 004dca18 */
    unsigned int infoMessageCount;                      /* 004dcd38 */
    void* level;                                        /* 004dcd3c */
    Image* overflowImage;                               /* 004dcd40 */
    int scrollOffset;                                   /* 004dcd44 */
    unsigned int flags;                                 /* 004dcd48 */
} InfoGlobs;

typedef int (*InfoEnumCallback)(InfoMessageInstance* instance, void* data);

void info_initialise(void* level);                                 /* 00419230 */
int info_get_type(const char* name, int* type);                    /* 004193e0 */
Image* info_get_image(int type);                                   /* 00419420 */
void info_set_overflow_image_file(const char* filename);           /* 00419430 */
void info_set_auto_game_speed(int on);                             /* 00419450 */
void info_set_type_auto_game_speed(int type, int on);              /* 00419470 */
void info_set_text(int type, char* text);                          /* 004194b0 */
void info_set_type_play_always(int type, int on);                  /* 004194f0 */
void info_compile_text(char* text, char** outText);                /* 00419530 */
void info_set_image_file(int type, const char* filename);          /* 00419650 */
void info_set_sfx(int type, int sfxType);                          /* 00419680 */
int info_message_enumerate(unsigned int index,
                           InfoEnumCallback callback, void* data); /* 004196a0 */
void info_message_add_instance(unsigned int index,
                               InfoMessageInstance* instance);     /* 004196f0 */
InfoMessageInstance* info_message_get_instance(unsigned int index,
                                               int instIndex);     /* 00419780 */
InfoMessageInstance* info_message_remove_instance(unsigned int index,
                                                  int instIndex);  /* 004197b0 */
void info_remove_object_references(void* object);                  /* 00419830 */
void info_remove_block_references(InfoPoint2I* blockPos);          /* 004198f0 */
int info_is_duplicate(int type, void* object, InfoPoint2I* blockPos); /* 004199f0 */
InfoMessageInstance* info_create_instance(InfoPoint2I* blockPos,
                                          void* object, char* text); /* 00419a80 */
void info_enable(int on);                                          /* 00419ae0 */
int info_has_text(int type);                                       /* 00419b00 */
int info_find_message(int type);                                   /* 00419b20 */
float info_get_next_y(void);                                       /* 00419b50 */
void info_send(int type, char* text, void* object,
               InfoPoint2I* blockPos);                             /* 00419b80 */
/* Shared: Panel.c (GAME-032) calls these four (game/info.h). */
void incident_message_panel_refresh_if_active(void);               /* 00419da0 */
void pending_record_list_entry_move_to_front(unsigned int index);  /* 00419de0 */
void pending_record_release_if_present(void);                      /* 00419e60 */
void pending_record_list_entry_remove_and_release(unsigned int index); /* 00419e80 */
void pending_record_release_all(void);                             /* 00419f10 */
void info_draw(float elapsed);                                     /* 00419f30 */
void info_draw_panel(float elapsed);                               /* 0041a080 */
int ui_entry_hit_test_and_dispatch(int mouseX, int mouseY,
                                   int leftButton);                /* 0041a1a0 */
void scroll_offset_from_cursor_update(void);                       /* 0041a250 */
void screen_right_edge_cache_refresh(void);                        /* 0041a290 */
void info_update_timers(float elapsed);                            /* 0041a2c0 */
/* Interface.c's font (interfaceGlobs.font, 004dcd58). */
struct TextureSurface* info_get_font(void);                        /* 0041a2f0 */

/* Port-only accessor (tests and the port tail): the scroll offset. */
int info_scroll_offset_get(void);

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_INFO_UNIT_H */
