// SPDX-License-Identifier: LGPL-3.0-or-later
/* NERPs.c, original translation unit GAME-028 (finding 505), rebuilt in
 * original shape (ADR 0005; finding 586): text 00452df0-004574e0, .data
 * 004a5948-004a6768, literals 004a6764-004a7f7c, .bss 004ff958-004ff9c0 and
 * the common nerpsGlobs (00555d40-00555df4).
 *
 * The unit runs the tutorial's NERP script: it loads the compiled script and
 * the message file (lines, inline images and sounds), interprets the script
 * each frame through the NERP function table, and keeps the tutorial's
 * camera tracking, zoom and rotation, message history and timers, the
 * tutorial icon and the per-level block pointer lists.
 *
 * 132 functions are rebuilt here: the interpreter and its support, 102 NERP
 * functions of the table, and the icon and block-pointer helpers they call
 * (finding 640: the menu icons' click, flash and selection rows reach
 * Interface.c as the original does). The other 184 functions of the table
 * (tutorial_script_port_functions.h) are not yet rebuilt: the table names
 * port-tail adapters for them (tutorial_script_port.cpp), which run the
 * port's NERP implementation for the same row.
 *
 * The port's tutorial script stays in the port tail tutorial_script_port.cpp,
 * unannotated: it runs through hook tables and its own state, which native
 * tests pin (the port starts with no "next" button, the original with one).
 * Native callers keep calling it under the port's names. The matching build's
 * C callers call this unit through the aliases in aitask_port_aliases.h,
 * weapons_port_aliases.h and tutorial_script.h. One entry keeps the port's
 * name and serves native callers too: flh_magic_uu_noop (00456210), which
 * returns zero and has no state. The records below are the original's Win32
 * layout and are read only in the matching build. */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "crt/vc5names.h"
#include "tutorial_script_imports.h"

/* The unit's functions have C linkage in the native C++ compile too: the
 * port tail's adapters and the shared entry expect it. */
#ifdef __cplusplus
extern "C" {
#endif

/* A NERP function takes the script's argument words. */
typedef int (*NERPsFunction)(unsigned int* stack);

/* One row of the function table (12 bytes). argumentCount is the row's
 * kind: 0-2 return a value and take 0-2 arguments; 3-6 are statements
 * that take 0-3 arguments and run only when the condition holds; 7 ends
 * the table. */
typedef struct NERPsFunctionSignature {
    const char* name;
    NERPsFunction function;
    unsigned int argumentCount;
} NERPsFunctionSignature;

/* A message-file image or sound token ($name or :name). */
typedef struct NERPs_MessageImage {
    char* key;
    struct Image* image;
} NERPs_MessageImage;

typedef struct NERPs_MessageSound {
    char* key;
    int sound;
} NERPs_MessageSound;

/* One entry of a level's block pointer list. */
typedef struct NERPs_BlockPointer {
    unsigned int x;
    unsigned int y;
    struct NERPs_BlockPointer* next;
} NERPs_BlockPointer;

typedef int (*NERPsBlockPointerCallback)(NERPs_BlockPointer* blockPointer,
                                         unsigned int x, unsigned int y,
                                         void* data);

/* Lego_Block (0x48 bytes), the field this unit reads. */
typedef struct NERPs_Block {
    unsigned char pad_00[0x04];
    unsigned char blockPointer;       /* +0x04 */
    unsigned char pad_05[0x08 - 0x05];
    unsigned int flags1;              /* +0x08 */
    unsigned char pad_0c[0x28 - 0x0c];
    int tutorialBlockClicks;          /* +0x28 */
    int tutorialCrystals;             /* +0x2c */
    int tutorialOre;                  /* +0x30 */
    unsigned char pad_34[0x48 - 0x34];
} NERPs_Block;

/* Lego_Level (0x284 bytes), the fields this unit reads. */
typedef struct NERPs_Level {
    unsigned char pad_000[0x004];
    struct TerrainMap* map;           /* +0x004 */
    unsigned char pad_008[0x014 - 0x008];
    unsigned int width;               /* +0x014 */
    unsigned int height;              /* +0x018 */
    unsigned char pad_01c[0x0b8 - 0x01c];
    NERPs_Block* blocks;              /* +0x0b8 */
    unsigned char pad_0bc[0x114 - 0x0bc];
    NERPs_BlockPointer* blockPointers[0x37]; /* +0x114 */
    unsigned char pad_1f0[0x27c - 0x1f0];
    int status;                       /* +0x27c */
} NERPs_Level;

/* LegoObject (0x40c bytes), the fields this unit reads and writes. */
typedef struct NERPs_Object {
    int type;                         /* +0x000 */
    int id;                           /* +0x004 */
    unsigned char pad_008[0x334 - 0x008];
    unsigned char* stats;             /* +0x334 ObjectStats */
    unsigned char pad_338[0x3f8 - 0x338];
    int tutorialIcon;                 /* +0x3f8 */
} NERPs_Object;

typedef struct NERPs_Vector3F {
    float x, y, z;
} NERPs_Vector3F;

typedef struct NERPs_Point2I {
    int x, y;
} NERPs_Point2I;

/* The tutorial-block search (0x2c bytes): the tutorial rows pass one to the
 * action callback 00456d00, which passes another to LegoObject.c's
 * object-list walk for the object searches. */
typedef struct NERPs_TutorialSearch {
    int action;                       /* +0x00: 00456d00's case */
    unsigned int index;               /* +0x04: the block-pointer list */
    int count;                        /* +0x08: the row's result */
    int f_0c;                         /* +0x0c */
    int f_10;                         /* +0x10 */
    int x;                            /* +0x14 */
    int y;                            /* +0x18 */
    int f_1c;                         /* +0x1c */
    int checkType;                    /* +0x20 */
    struct NERPs_Object* found;       /* +0x24 */
    int objType;                      /* +0x28 */
} NERPs_TutorialSearch;

/* The "next" button acknowledgement (004a6740). */
typedef struct NERPs_Next {
    unsigned int pressed;             /* 004a6740 */
    unsigned int f_4;                 /* 004a6744 */
} NERPs_Next;

/* SetMessageTimerValues' times (004a6748). */
typedef struct NERPs_MessageTime {
    float scale;                      /* 004a6748 */
    float add;                        /* 004a674c */
    float defaultTime;                /* 004a6750 */
    float f_c;                        /* 004a6754 */
} NERPs_MessageTime;

/* The script's state (004ff958-004ff984): cleared with each script. */
typedef struct NERPs_State {
    int registers[8];                 /* 004ff958 R0-R7 */
    unsigned int messagePermit;       /* 004ff978 */
    unsigned int trainFlags;          /* 004ff97c */
    unsigned int tutorialFlags;       /* 004ff980 */
} NERPs_State;

/* The unit's .bss (004ff958-004ff9c0): the script state, then the rest. */
typedef struct NERPs_RuntimeGlobs {
    NERPs_State state;                /* 004ff958 cleared with each script */
    unsigned int previousNextPressed; /* 004ff984 */
    unsigned int messageWait;         /* 004ff988 */
    float messageTimer;               /* 004ff98c */
    unsigned int monstersDestroyed;   /* 004ff990 */
    unsigned int f_994;               /* 004ff994 */
    unsigned int debug;               /* 004ff998 */
    unsigned int f_99c;               /* 004ff99c */
    float timers[4];                  /* 004ff9a0 */
    unsigned int supressArrow;        /* 004ff9b0 */
    unsigned int clickOnly;           /* 004ff9b4 */
    float iconBounce;                 /* 004ff9b8 */
    unsigned int objectiveSwitch;     /* 004ff9bc */
} NERPs_RuntimeGlobs;

/* nerpsGlobs (00555d40, 0xb4 bytes, common). */
typedef struct NERPs_Globs {
    int cameraTrack;                  /* +0x00 */
    unsigned int cameraTrackIndex;    /* +0x04 */
    float cameraX;                    /* +0x08 */
    float cameraY;                    /* +0x0c */
    struct EntitySlot* cameraTrackObject; /* +0x10 */
    int cameraZoom;                   /* +0x14 */
    float cameraZoomAmount;           /* +0x18 */
    float cameraZoomCurrent;          /* +0x1c */
    int cameraRotate;                 /* +0x20 */
    float cameraRotateAmount;         /* +0x24 */
    float cameraRotateCurrent;        /* +0x28 */
    unsigned int* scriptBuffer;       /* +0x2c */
    unsigned int scriptSize;          /* +0x30 */
    unsigned int f_34[11];            /* +0x34 */
    unsigned char* messageBuffer;     /* +0x60 */
    unsigned int messageCount;        /* +0x64 */
    char** messageLines;              /* +0x68 */
    unsigned int imageCount;          /* +0x6c */
    NERPs_MessageImage* imageList;    /* +0x70 */
    unsigned int soundCount;          /* +0x74 */
    NERPs_MessageSound* soundList;    /* +0x78 */
    unsigned int currentSound;        /* +0x7c */
    unsigned int messageHistory[8];   /* +0x80 */
    int messageHistoryCount;          /* +0xa0 */
    int messageOffset;                /* +0xa4 */
    unsigned int messageAcknowledged; /* +0xa8 */
    unsigned int tutorialIconObject;  /* +0xac record object index + 1 */
    unsigned int messageSoundPlaying; /* +0xb0 */
} NERPs_Globs;

/* The original-shape functions, in address order. */
int nerps_file_load_script_file(const char* filename);
int nerps_file_load_message_file(const char* filename);
char* nerps_file_get_message_line(unsigned int line);
int nerps_shutdown(void);
void nerps_runtime_resolve_argument(unsigned int* value);
void nerps_runtime_execute(float elapsed);
int nerpfunc_set_game_speed(unsigned int* stack);
void nerps_set_has_next_button(int hasNext);
void nerps_replay_message_sound(void);
void nerps_message_previous(void);
void nerps_runtime_update_timers(float elapsed);
void nerps_runtime_update_camera(float elapsed);
void nerps_draw_tutorial_icon(NERPs_Level* level, void* viewport,
                              int unused, float elapsed);
int nerps_get_icon_clicks(unsigned int* stack);
void nerps_monster_destroyed(void);
void nerps_monsters_destroyed_reset(void);
int nerpfunc_set_upgrade_building_icon_clicked(unsigned int* stack);
int nerpfunc_get_upgrade_building_icon_clicked(unsigned int* stack);
int nerpfunc_flash_upgrade_building_icon(unsigned int* stack);
int nerpfunc_set_go_back_icon_clicked(unsigned int* stack);
int nerpfunc_get_go_back_icon_clicked(unsigned int* stack);
int nerpfunc_flash_go_back_icon(unsigned int* stack);
int nerpfunc_set_level_completed(unsigned int* stack);
int nerpfunc_set_level_fail(unsigned int* stack);
int nerpfunc_set_tutorial_pointer(unsigned int* stack);
unsigned int nerps_set_object_icon_clicks(const char* name,
                                          unsigned int clicks);
int nerps_flash_object_icon(const char* name, int on);
int nerps_set_tutorial_block_clicks_callback(NERPs_BlockPointer* blockPointer,
                                             unsigned int x, unsigned int y,
                                             void* data);
int nerpfunc_set_tutorial_block_clicks(unsigned int* stack);
int nerps_set_tutorial_crystals_callback(NERPs_BlockPointer* blockPointer,
                                         unsigned int x, unsigned int y,
                                         void* data);
int nerpfunc_set_tutorial_crystals(unsigned int* stack);
int nerps_set_ore_at_icon_positions_callback(NERPs_BlockPointer* blockPointer,
                                             unsigned int x, unsigned int y,
                                             void* data);
int nerpfunc_set_ore_at_icon_positions(unsigned int* stack);
int nerpfunc_get_small_truck_selected(unsigned int* stack);
int nerpfunc_get_small_digger_selected(unsigned int* stack);
int nerpfunc_get_rapid_rider_selected(unsigned int* stack);
int nerpfunc_get_small_helicopter_selected(unsigned int* stack);
int nerpfunc_get_granite_grinder_selected(unsigned int* stack);
int nerpfunc_get_chrome_crusher_selected(unsigned int* stack);
int nerps_add_tutorial_crystals_callback(NERPs_BlockPointer* blockPointer,
                                         unsigned int x, unsigned int y,
                                         void* data);
int nerpfunc_get_tutorial_crystals(unsigned int* stack);
int nerps_add_tutorial_block_clicks_callback(NERPs_BlockPointer* blockPointer,
                                             unsigned int x, unsigned int y,
                                             void* data);
int nerpfunc_get_tutorial_block_clicks(unsigned int* stack);
unsigned int nerps_object_count(const char* name, int level);
unsigned int nerps_get_object_icon_clicks(const char* name);
int nerps_flash_icon(int item, int on);
int nerpfunc_get_barracks_icon_clicked(unsigned int* stack);
int nerpfunc_get_geodome_icon_clicked(unsigned int* stack);
int nerpfunc_get_powerstation_icon_clicked(unsigned int* stack);
int nerpfunc_get_tool_store_icon_clicked(unsigned int* stack);
int nerpfunc_get_gunstation_icon_clicked(unsigned int* stack);
int nerpfunc_get_teleport_pad_icon_clicked(unsigned int* stack);
int nerpfunc_get_vehicle_transport_icon_clicked(unsigned int* stack);
int nerpfunc_get_upgrade_station_icon_clicked(unsigned int* stack);
int nerpfunc_set_barracks_icon_clicked(unsigned int* stack);
int nerpfunc_set_geodome_icon_clicked(unsigned int* stack);
int nerpfunc_set_powerstation_icon_clicked(unsigned int* stack);
int nerpfunc_set_tool_store_icon_clicked(unsigned int* stack);
int nerpfunc_set_gunstation_icon_clicked(unsigned int* stack);
int nerpfunc_set_teleport_pad_icon_clicked(unsigned int* stack);
int nerpfunc_set_vehicle_transport_icon_clicked(unsigned int* stack);
int nerpfunc_set_upgrade_station_icon_clicked(unsigned int* stack);
int nerpfunc_flash_barracks_icon(unsigned int* stack);
int nerpfunc_flash_geodome_icon(unsigned int* stack);
int nerpfunc_flash_power_station_icon(unsigned int* stack);
int nerpfunc_flash_tool_store_icon(unsigned int* stack);
int nerpfunc_flash_gunstation_icon(unsigned int* stack);
int nerpfunc_flash_teleport_pad_icon(unsigned int* stack);
int nerpfunc_flash_vehicle_transport_icon(unsigned int* stack);
int nerpfunc_flash_upgrade_station_icon(unsigned int* stack);
int nerpfunc_get_teleport_icon_clicked(unsigned int* stack);
int nerpfunc_get_dynamite_clicked(unsigned int* stack);
int nerpfunc_get_mount_icon_clicked(unsigned int* stack);
int nerpfunc_get_train_icon_clicked(unsigned int* stack);
int nerpfunc_get_drop_sonic_blaster_icon_clicked(unsigned int* stack);
int nerpfunc_get_get_tool_icon_clicked(unsigned int* stack);
int nerpfunc_get_get_pusher_icon_clicked(unsigned int* stack);
int nerpfunc_get_get_sonic_blaster_icon_clicked(unsigned int* stack);
int nerpfunc_get_train_sailor_icon_clicked(unsigned int* stack);
int nerpfunc_get_train_pilot_icon_clicked(unsigned int* stack);
int nerpfunc_get_train_driver_icon_clicked(unsigned int* stack);
int nerpfunc_get_get_laser_icon_clicked(unsigned int* stack);
int nerpfunc_get_dismount_icon_clicked(unsigned int* stack);
int nerpfunc_get_dig_icon_clicked(unsigned int* stack);
int nerpfunc_get_build_icon_clicked(unsigned int* stack);
int nerpfunc_get_lay_path_icon_clicked(unsigned int* stack);
int nerpfunc_get_place_fence_icon_clicked(unsigned int* stack);
int nerpfunc_set_teleport_icon_clicked(unsigned int* stack);
int nerpfunc_set_dynamite_clicked(unsigned int* stack);
int nerpfunc_set_train_icon_clicked(unsigned int* stack);
int nerpfunc_set_train_driver_icon_clicked(unsigned int* stack);
int nerpfunc_set_train_sailor_icon_clicked(unsigned int* stack);
int nerpfunc_set_get_tool_icon_clicked(unsigned int* stack);
int nerpfunc_set_drop_sonic_blaster_icon_clicked(unsigned int* stack);
int nerpfunc_set_get_laser_icon_clicked(unsigned int* stack);
int nerpfunc_set_get_pusher_icon_clicked(unsigned int* stack);
int nerpfunc_set_get_sonic_blaster_icon_clicked(unsigned int* stack);
int nerpfunc_set_dismount_icon_clicked(unsigned int* stack);
int nerpfunc_set_train_pilot_icon_clicked(unsigned int* stack);
int nerpfunc_set_mount_icon_clicked(unsigned int* stack);
int nerpfunc_set_dig_icon_clicked(unsigned int* stack);
int nerpfunc_set_build_icon_clicked(unsigned int* stack);
int nerpfunc_set_lay_path_icon_clicked(unsigned int* stack);
int nerpfunc_set_place_fence_icon_clicked(unsigned int* stack);
int nerpfunc_flash_teleport_icon(unsigned int* stack);
int nerpfunc_flash_dynamite_icon(unsigned int* stack);
int nerpfunc_flash_mount_icon(unsigned int* stack);
int nerpfunc_flash_train_icon(unsigned int* stack);
int nerpfunc_flash_train_driver_icon(unsigned int* stack);
int nerpfunc_flash_train_pilot_icon(unsigned int* stack);
int nerpfunc_flash_train_sailor_icon(unsigned int* stack);
int nerpfunc_flash_dismount_icon(unsigned int* stack);
int nerpfunc_flash_get_tool_icon(unsigned int* stack);
int nerpfunc_flash_drop_sonic_blaster_icon(unsigned int* stack);
int nerpfunc_flash_get_laser_icon(unsigned int* stack);
int nerpfunc_flash_get_pusher_icon(unsigned int* stack);
int nerpfunc_flash_get_sonic_blaster_icon(unsigned int* stack);
int nerpfunc_flash_dig_icon(unsigned int* stack);
int nerpfunc_flash_build_icon(unsigned int* stack);
int nerpfunc_flash_lay_path_icon(unsigned int* stack);
int nerpfunc_flash_place_fence_icon(unsigned int* stack);
uint32_t flh_magic_uu_noop(void);
int nerpfunc_get_tutorial_flags(unsigned int* stack);
int nerpfunc_set_tutorial_flags(unsigned int* stack);
int nerpfunc_set_message_permit(unsigned int* stack);
int nerps_get_message_permit(void);
int nerpfunc_set_objective_switch(unsigned int* stack);
int nerpfunc_get_objective_switch(unsigned int* stack);
void nerps_play_message_sound(void);
void nerps_parse_message(char* text, char* output, int setTimer);
void nerps_init_block_pointers_table(NERPs_Level* level);
NERPs_BlockPointer* nerps_get_block_pointer(NERPs_Level* level, int index);
void nerps_free_block_pointers(void);
void nerps_block_pointer_enumerate(int index,
                                   NERPsBlockPointerCallback callback,
                                   void* data);
int nerps_tutorial_action_callback(NERPs_BlockPointer* blockPointer,
                                   unsigned int x, unsigned int y,
                                   void* data);
int nerps_object_at_tutorial_block_callback(NERPs_Object* obj, void* data);
int nerps_tutorial_icon_object_callback(NERPs_Object* obj, void* data);
int nerpfunc_make_someone_on_this_block_pick_up_something_on_this_block(unsigned int* stack);
void nerps_set_tutorial_pointer(unsigned int index, int on);
int nerpfunc_set_congregation_at_tutorial(unsigned int* stack);
int nerpfunc_set_rock_monster_at_tutorial(unsigned int* stack);
int nerpfunc_set_camera_goto_tutorial(unsigned int* stack);
int nerpfunc_get_camera_at_tutorial(unsigned int* stack);
int nerpfunc_get_tutorial_block_is_ground(unsigned int* stack);
int nerpfunc_get_tutorial_block_is_path(unsigned int* stack);
int nerpfunc_set_tutorial_block_is_ground(unsigned int* stack);
int nerpfunc_set_tutorial_block_is_path(unsigned int* stack);
int nerpfunc_get_unit_at_block(unsigned int* stack);
int nerpfunc_get_monster_at_tutorial(unsigned int* stack);

/* The table's functions that are not rebuilt yet: port-tail adapters. */
#define NERPS_PORT_FUNCTION(name, row) int nerps_port_##name(unsigned int* stack);
#include "tutorial_script_port_functions.h"
#undef NERPS_PORT_FUNCTION

#define NERPS_LEVEL() ((NERPs_Level*)level_current_owner_get())

// GLOBAL: LEGORR 0x004a5948
NERPsFunctionSignature nerpsFunctions[] = {
    // STRING: LEGORR 0x004a7f74
    { "Stop", (NERPsFunction)flh_magic_uu_noop, 3 },
    // STRING: LEGORR 0x004a7f6c
    { "True", (NERPsFunction)flic_opcode25_success_stub, 0 },
    // STRING: LEGORR 0x004a7f64
    { "False", (NERPsFunction)flh_magic_uu_noop, 0 },
    /* "Null" is pooled with frontend_unit.cpp's 004a23f4. */
    { "Null", (NERPsFunction)flh_magic_uu_noop, 0 },
    // STRING: LEGORR 0x004a7f58
    { "GetRandom", nerps_port_GetRandom, 0 },
    // STRING: LEGORR 0x004a7f4c
    { "GetRandom10", nerps_port_GetRandom10, 0 },
    // STRING: LEGORR 0x004a7f3c
    { "GetRandom100", nerps_port_GetRandom100, 0 },
    // STRING: LEGORR 0x004a7f28
    { "GetRandomTrueFalse", nerps_port_GetRandomTrueFalse, 0 },
    // STRING: LEGORR 0x004a7f14
    { "SetLevelCompleted", nerpfunc_set_level_completed, 3 },
    // STRING: LEGORR 0x004a7f04
    { "SetLevelFail", nerpfunc_set_level_fail, 3 },
    // STRING: LEGORR 0x004a7ef0
    { "SetGameCompleted", nerpfunc_set_level_completed, 3 },
    // STRING: LEGORR 0x004a7ee4
    { "SetGameFail", nerps_port_SetGameFail, 3 },
    // STRING: LEGORR 0x004a7ed0
    { "GetCrystalsPickedUp", nerps_port_GetCrystalsPickedUp, 0 },
    // STRING: LEGORR 0x004a7eb4
    { "GetCrystalsCurrentlyStored", nerps_port_GetCrystalsCurrentlyStored, 0 },
    // STRING: LEGORR 0x004a7ea4
    { "GetCrystalsUsed", (NERPsFunction)flh_magic_uu_noop, 0 },
    // STRING: LEGORR 0x004a7e90
    { "GetCrystalsStolen", (NERPsFunction)flh_magic_uu_noop, 0 },
    // STRING: LEGORR 0x004a7e84
    { "SetMessage", nerps_port_SetMessage, 5 },
    // STRING: LEGORR 0x004a7e7c
    { "GetR0", nerps_port_GetR0, 0 },
    // STRING: LEGORR 0x004a7e74
    { "GetR1", nerps_port_GetR1, 0 },
    // STRING: LEGORR 0x004a7e6c
    { "GetR2", nerps_port_GetR2, 0 },
    // STRING: LEGORR 0x004a7e64
    { "GetR3", nerps_port_GetR3, 0 },
    // STRING: LEGORR 0x004a7e5c
    { "GetR4", nerps_port_GetR4, 0 },
    // STRING: LEGORR 0x004a7e54
    { "GetR5", nerps_port_GetR5, 0 },
    // STRING: LEGORR 0x004a7e4c
    { "GetR6", nerps_port_GetR6, 0 },
    // STRING: LEGORR 0x004a7e44
    { "GetR7", nerps_port_GetR7, 0 },
    // STRING: LEGORR 0x004a7e3c
    { "SetR0", nerps_port_SetR0, 4 },
    // STRING: LEGORR 0x004a7e34
    { "SetR1", nerps_port_SetR1, 4 },
    // STRING: LEGORR 0x004a7e2c
    { "SetR2", nerps_port_SetR2, 4 },
    // STRING: LEGORR 0x004a7e24
    { "SetR3", nerps_port_SetR3, 4 },
    // STRING: LEGORR 0x004a7e1c
    { "SetR4", nerps_port_SetR4, 4 },
    // STRING: LEGORR 0x004a7e14
    { "SetR5", nerps_port_SetR5, 4 },
    // STRING: LEGORR 0x004a7e0c
    { "SetR6", nerps_port_SetR6, 4 },
    // STRING: LEGORR 0x004a7e04
    { "SetR7", nerps_port_SetR7, 4 },
    // STRING: LEGORR 0x004a7dfc
    { "AddR0", nerps_port_AddR0, 4 },
    // STRING: LEGORR 0x004a7df4
    { "AddR1", nerps_port_AddR1, 4 },
    // STRING: LEGORR 0x004a7dec
    { "AddR2", nerps_port_AddR2, 4 },
    // STRING: LEGORR 0x004a7de4
    { "AddR3", nerps_port_AddR3, 4 },
    // STRING: LEGORR 0x004a7ddc
    { "AddR4", nerps_port_AddR4, 4 },
    // STRING: LEGORR 0x004a7dd4
    { "AddR5", nerps_port_AddR5, 4 },
    // STRING: LEGORR 0x004a7dcc
    { "AddR6", nerps_port_AddR6, 4 },
    // STRING: LEGORR 0x004a7dc4
    { "AddR7", nerps_port_AddR7, 4 },
    // STRING: LEGORR 0x004a7dbc
    { "SubR0", nerps_port_SubR0, 4 },
    // STRING: LEGORR 0x004a7db4
    { "SubR1", nerps_port_SubR1, 4 },
    // STRING: LEGORR 0x004a7dac
    { "SubR2", nerps_port_SubR2, 4 },
    // STRING: LEGORR 0x004a7da4
    { "SubR3", nerps_port_SubR3, 4 },
    // STRING: LEGORR 0x004a7d9c
    { "SubR4", nerps_port_SubR4, 4 },
    // STRING: LEGORR 0x004a7d94
    { "SubR5", nerps_port_SubR5, 4 },
    // STRING: LEGORR 0x004a7d8c
    { "SubR6", nerps_port_SubR6, 4 },
    // STRING: LEGORR 0x004a7d84
    { "SubR7", nerps_port_SubR7, 4 },
    // STRING: LEGORR 0x004a7d70
    { "SetMessagePermit", nerpfunc_set_message_permit, 4 },
    // STRING: LEGORR 0x004a7d5c
    { "GetObjectiveShowing", nerps_port_GetObjectiveShowing, 0 },
    // STRING: LEGORR 0x004a7d44
    { "GetTeleportIconClicked", nerpfunc_get_teleport_icon_clicked, 0 },
    // STRING: LEGORR 0x004a7d2c
    { "GetMiniFiguresOnLevel", nerps_port_GetMiniFiguresOnLevel, 0 },
    // STRING: LEGORR 0x004a7d18
    { "GetBuildIconClicked", nerpfunc_get_build_icon_clicked, 0 },
    // STRING: LEGORR 0x004a7d04
    { "SetBuildIconClicked", nerpfunc_set_build_icon_clicked, 4 },
    // STRING: LEGORR 0x004a7cec
    { "SetTeleportIconClicked", nerpfunc_set_teleport_icon_clicked, 4 },
    // STRING: LEGORR 0x004a7cd4
    { "SetToolStoreIconClicked", nerpfunc_set_tool_store_icon_clicked, 4 },
    // STRING: LEGORR 0x004a7cbc
    { "GetToolStoreIconClicked", nerpfunc_get_tool_store_icon_clicked, 0 },
    // STRING: LEGORR 0x004a7ca8
    { "FlashToolStoreIcon", nerpfunc_flash_tool_store_icon, 4 },
    // STRING: LEGORR 0x004a7c98
    { "FlashBuildIcon", nerpfunc_flash_build_icon, 4 },
    // STRING: LEGORR 0x004a7c84
    { "FlashTeleportIcon", nerpfunc_flash_teleport_icon, 4 },
    // STRING: LEGORR 0x004a7c6c
    { "SetGoBackIconClicked", nerpfunc_set_go_back_icon_clicked, 4 },
    // STRING: LEGORR 0x004a7c54
    { "GetGoBackIconClicked", nerpfunc_get_go_back_icon_clicked, 0 },
    // STRING: LEGORR 0x004a7c44
    { "FlashGoBackIcon", nerpfunc_flash_go_back_icon, 4 },
    // STRING: LEGORR 0x004a7c30
    { "SetDigIconClicked", nerpfunc_set_dig_icon_clicked, 4 },
    // STRING: LEGORR 0x004a7c1c
    { "GetDigIconClicked", nerpfunc_get_dig_icon_clicked, 0 },
    // STRING: LEGORR 0x004a7c0c
    { "FlashDigIcon", nerpfunc_flash_dig_icon, 4 },
    // STRING: LEGORR 0x004a7c00
    { "GetTimer0", nerps_port_GetTimer0, 0 },
    // STRING: LEGORR 0x004a7bf4
    { "GetTimer1", nerps_port_GetTimer1, 0 },
    // STRING: LEGORR 0x004a7be8
    { "GetTimer2", nerps_port_GetTimer2, 0 },
    // STRING: LEGORR 0x004a7bdc
    { "GetTimer3", nerps_port_GetTimer3, 0 },
    // STRING: LEGORR 0x004a7bd0
    { "SetTimer0", nerps_port_SetTimer0, 4 },
    // STRING: LEGORR 0x004a7bc4
    { "SetTimer1", nerps_port_SetTimer1, 4 },
    // STRING: LEGORR 0x004a7bb8
    { "SetTimer2", nerps_port_SetTimer2, 4 },
    // STRING: LEGORR 0x004a7bac
    { "SetTimer3", nerps_port_SetTimer3, 4 },
    // STRING: LEGORR 0x004a7b98
    { "GetBarracksBuilt", nerps_port_GetBarracksBuilt, 0 },
    // STRING: LEGORR 0x004a7b80
    { "GetOreRefineriesBuilt", nerps_port_GetOreRefineriesBuilt, 0 },
    // STRING: LEGORR 0x004a7b6c
    { "GetToolStoresBuilt", nerps_port_GetToolStoresBuilt, 0 },
    // STRING: LEGORR 0x004a7b5c
    { "GetStudCount", nerps_port_GetStudCount, 0 },
    // STRING: LEGORR 0x004a7b48
    { "GetTeleportsBuilt", nerps_port_GetTeleportsBuilt, 0 },
    // STRING: LEGORR 0x004a7b30
    { "GetMiniFigureSelected", nerps_port_GetMiniFigureSelected, 0 },
    // STRING: LEGORR 0x004a7b18
    { "GetSmallDiggerSelected", nerpfunc_get_small_digger_selected, 0 },
    // STRING: LEGORR 0x004a7afc
    { "GetMiniFigureinSmallDigger", nerps_port_GetMiniFigureinSmallDigger, 0 },
    // STRING: LEGORR 0x004a7aec
    { "GetTrainFlags", nerps_port_GetTrainFlags, 0 },
    // STRING: LEGORR 0x004a7adc
    { "SetTrainFlags", nerps_port_SetTrainFlags, 4 },
    // STRING: LEGORR 0x004a7ad0
    { "SetIconPos", nerps_port_SetIconPos, 5 },
    // STRING: LEGORR 0x004a7ac0
    { "SetIconSpace", nerps_port_SetIconSpace, 4 },
    // STRING: LEGORR 0x004a7ab0
    { "SetIconWidth", nerps_port_SetIconWidth, 4 },
    // STRING: LEGORR 0x004a7aa0
    { "SetMessageWait", nerps_port_SetMessageWait, 4 },
    // STRING: LEGORR 0x004a7a8c
    { "GetMountIconClicked", nerpfunc_get_mount_icon_clicked, 0 },
    // STRING: LEGORR 0x004a7a78
    { "SetMountIconClicked", nerpfunc_set_mount_icon_clicked, 4 },
    // STRING: LEGORR 0x004a7a68
    { "FlashMountIcon", nerpfunc_flash_mount_icon, 4 },
    // STRING: LEGORR 0x004a7a54
    { "SetTutorialPointer", nerpfunc_set_tutorial_pointer, 5 },
    // STRING: LEGORR 0x004a7a40
    { "GetTutorialFlags", nerpfunc_get_tutorial_flags, 0 },
    // STRING: LEGORR 0x004a7a2c
    { "SetTutorialFlags", nerpfunc_set_tutorial_flags, 4 },
    // STRING: LEGORR 0x004a7a1c
    { "SetRockMonster", nerps_port_SetRockMonster, 5 },
    // STRING: LEGORR 0x004a7a0c
    { "GetOrePickedUp", nerps_port_GetOrePickedUp, 0 },
    // STRING: LEGORR 0x004a79f4
    { "GetOreCurrentlyStored", nerps_port_GetOreCurrentlyStored, 0 },
    // STRING: LEGORR 0x004a79e8
    { "GetOreUsed", (NERPsFunction)flh_magic_uu_noop, 0 },
    // STRING: LEGORR 0x004a79d8
    { "GetOreStolen", (NERPsFunction)flh_magic_uu_noop, 0 },
    // STRING: LEGORR 0x004a79bc
    { "GetCrystalRefineriesBuilt", nerps_port_GetCrystalRefineriesBuilt, 0 },
    // STRING: LEGORR 0x004a79a8
    { "FlashLayPathIcon", nerpfunc_flash_lay_path_icon, 4 },
    // STRING: LEGORR 0x004a7990
    { "SetLayPathIconClicked", nerpfunc_set_lay_path_icon_clicked, 4 },
    // STRING: LEGORR 0x004a7978
    { "GetLayPathIconClicked", nerpfunc_get_lay_path_icon_clicked, 0 },
    // STRING: LEGORR 0x004a795c
    { "GetTeleportPadIconClicked", nerpfunc_get_teleport_pad_icon_clicked, 0 },
    // STRING: LEGORR 0x004a7940
    { "SetTeleportPadIconClicked", nerpfunc_set_teleport_pad_icon_clicked, 4 },
    // STRING: LEGORR 0x004a7928
    { "FlashTeleportPadIcon", nerpfunc_flash_teleport_pad_icon, 4 },
    // STRING: LEGORR 0x004a7918
    { "GetMessageTimer", nerps_port_GetMessageTimer, 0 },
    // STRING: LEGORR 0x004a7900
    { "SetMessageTimerValues", nerps_port_SetMessageTimerValues, 6 },
    // STRING: LEGORR 0x004a78e8
    { "GetTutorialBlockClicks", nerpfunc_get_tutorial_block_clicks, 1 },
    // STRING: LEGORR 0x004a78d0
    { "SetTutorialBlockClicks", nerpfunc_set_tutorial_block_clicks, 5 },
    // STRING: LEGORR 0x004a78bc
    { "GetTutorialCrystals", nerpfunc_get_tutorial_crystals, 1 },
    // STRING: LEGORR 0x004a78a8
    { "SetTutorialCrystals", nerpfunc_set_tutorial_crystals, 5 },
    // STRING: LEGORR 0x004a7898
    { "GetPathsBuilt", nerps_port_GetPathsBuilt, 0 },
    // STRING: LEGORR 0x004a787c
    { "GetTutorialBlockIsGround", nerpfunc_get_tutorial_block_is_ground, 1 },
    // STRING: LEGORR 0x004a7864
    { "SetCameraGotoTutorial", nerpfunc_set_camera_goto_tutorial, 4 },
    // STRING: LEGORR 0x004a7850
    { "FlashDynamiteIcon", nerpfunc_flash_dynamite_icon, 4 },
    // STRING: LEGORR 0x004a783c
    { "GetDynamiteClicked", nerpfunc_get_dynamite_clicked, 0 },
    // STRING: LEGORR 0x004a7828
    { "SetDynamiteClicked", nerpfunc_set_dynamite_clicked, 4 },
    // STRING: LEGORR 0x004a7814
    { "AddPoweredCrystals", nerps_port_AddPoweredCrystals, 4 },
    // STRING: LEGORR 0x004a77f8
    { "GetGraniteGrinderSelected", nerpfunc_get_granite_grinder_selected, 0 },
    // STRING: LEGORR 0x004a77dc
    { "GetChromeCrusherSelected", nerpfunc_get_chrome_crusher_selected, 0 },
    // STRING: LEGORR 0x004a77c4
    { "GetTutorialBlockIsPath", nerpfunc_get_tutorial_block_is_path, 1 },
    // STRING: LEGORR 0x004a77a8
    { "GetGunstationIconClicked", nerpfunc_get_gunstation_icon_clicked, 0 },
    // STRING: LEGORR 0x004a778c
    { "SetGunstationIconClicked", nerpfunc_set_gunstation_icon_clicked, 4 },
    // STRING: LEGORR 0x004a7778
    { "FlashGunstationIcon", nerpfunc_flash_gunstation_icon, 4 },
    // STRING: LEGORR 0x004a7764
    { "GetGunstationsBuilt", nerps_port_GetGunstationsBuilt, 0 },
    // STRING: LEGORR 0x004a774c
    { "SetOreAtIconPositions", nerpfunc_set_ore_at_icon_positions, 5 },
    // STRING: LEGORR 0x004a7730
    { "GetVehicleTeleportsBuilt", nerps_port_GetVehicleTeleportsBuilt, 0 },
    // STRING: LEGORR 0x004a7710
    { "GetVehicleTransportIconClicked", nerpfunc_get_vehicle_transport_icon_clicked, 0 },
    // STRING: LEGORR 0x004a76f0
    { "SetVehicleTransportIconClicked", nerpfunc_set_vehicle_transport_icon_clicked, 4 },
    // STRING: LEGORR 0x004a76d4
    { "FlashVehicleTransportIcon", nerpfunc_flash_vehicle_transport_icon, 4 },
    // STRING: LEGORR 0x004a76b4
    { "GetUpgradeStationIconClicked", nerpfunc_get_upgrade_station_icon_clicked, 0 },
    // STRING: LEGORR 0x004a7694
    { "SetUpgradeStationIconClicked", nerpfunc_set_upgrade_station_icon_clicked, 4 },
    // STRING: LEGORR 0x004a767c
    { "FlashUpgradeStationIcon", nerpfunc_flash_upgrade_station_icon, 4 },
    // STRING: LEGORR 0x004a7664
    { "GetUpgradeStationsBuilt", nerps_port_GetUpgradeStationsBuilt, 0 },
    // STRING: LEGORR 0x004a7648
    { "SetTutorialBlockIsGround", nerpfunc_set_tutorial_block_is_ground, 5 },
    // STRING: LEGORR 0x004a7630
    { "SetTutorialBlockIsPath", nerpfunc_set_tutorial_block_is_path, 5 },
    // STRING: LEGORR 0x004a761c
    { "CameraLockOnObject", nerps_port_CameraLockOnObject, 4 },
    // STRING: LEGORR 0x004a760c
    { "CameraUnlock", nerps_port_CameraUnlock, 3 },
    // STRING: LEGORR 0x004a75fc
    { "CameraZoomIn", nerps_port_CameraZoomIn, 4 },
    // STRING: LEGORR 0x004a75ec
    { "CameraZoomOut", nerps_port_CameraZoomOut, 4 },
    // STRING: LEGORR 0x004a75dc
    { "CameraRotate", nerps_port_CameraRotate, 4 },
    // STRING: LEGORR 0x004a75c8
    { "GetCameraAtTutorial", nerpfunc_get_camera_at_tutorial, 1 },
    // STRING: LEGORR 0x004a75b0
    { "GetSelectedRecordObject", nerps_port_GetSelectedRecordObject, 0 },
    // STRING: LEGORR 0x004a7594
    { "GetSmallHelicopterSelected", nerpfunc_get_small_helicopter_selected, 0 },
    // STRING: LEGORR 0x004a757c
    { "GetRapidRiderSelected", nerpfunc_get_rapid_rider_selected, 0 },
    // STRING: LEGORR 0x004a7560
    { "GetMiniFigureinRapidRider", nerps_port_GetMiniFigureinRapidRider, 0 },
    // STRING: LEGORR 0x004a7548
    { "GetDismountIconClicked", nerpfunc_get_dismount_icon_clicked, 0 },
    // STRING: LEGORR 0x004a7530
    { "SetDismountIconClicked", nerpfunc_set_dismount_icon_clicked, 4 },
    // STRING: LEGORR 0x004a751c
    { "FlashDismountIcon", nerpfunc_flash_dismount_icon, 4 },
    // STRING: LEGORR 0x004a7504
    { "GetGetToolIconClicked", nerpfunc_get_get_tool_icon_clicked, 0 },
    // STRING: LEGORR 0x004a74ec
    { "SetGetToolIconClicked", nerpfunc_set_get_tool_icon_clicked, 4 },
    // STRING: LEGORR 0x004a74d8
    { "FlashGetToolIcon", nerpfunc_flash_get_tool_icon, 4 },
    // STRING: LEGORR 0x004a74c4
    { "GetAnyKeyPressed", nerps_port_GetAnyKeyPressed, 0 },
    // STRING: LEGORR 0x004a74b4
    { "SetPauseGame", nerps_port_SetPauseGame, 4 },
    // STRING: LEGORR 0x004a749c
    { "GetGetLaserIconClicked", nerpfunc_get_get_laser_icon_clicked, 0 },
    // STRING: LEGORR 0x004a7484
    { "SetGetLaserIconClicked", nerpfunc_set_get_laser_icon_clicked, 4 },
    // STRING: LEGORR 0x004a7470
    { "FlashGetLaserIcon", nerpfunc_flash_get_laser_icon, 4 },
    // STRING: LEGORR 0x004a7454
    { "SetRockMonsterAtTutorial", nerpfunc_set_rock_monster_at_tutorial, 4 },
    // STRING: LEGORR 0x004a7438
    { "GetCallToArmsButtonClicked", nerps_port_GetCallToArmsButtonClicked, 0 },
    // STRING: LEGORR 0x004a741c
    { "GetRockMonstersDestroyed", nerps_port_GetRockMonstersDestroyed, 0 },
    // STRING: LEGORR 0x004a7404
    { "SetGetPusherIconClicked", nerpfunc_set_get_pusher_icon_clicked, 4 },
    // STRING: LEGORR 0x004a73ec
    { "GetGetPusherIconClicked", nerpfunc_get_get_pusher_icon_clicked, 0 },
    // STRING: LEGORR 0x004a73d8
    { "FlashGetPusherIcon", nerpfunc_flash_get_pusher_icon, 4 },
    // STRING: LEGORR 0x004a73bc
    { "GetRockMonsterRunningAway", nerps_port_GetRockMonsterRunningAway, 0 },
    // STRING: LEGORR 0x004a73ac
    { "SetCallToArms", nerps_port_SetCallToArms, 4 },
    // STRING: LEGORR 0x004a7394
    { "FlashPowerStationIcon", nerpfunc_flash_power_station_icon, 4 },
    // STRING: LEGORR 0x004a7378
    { "SetPowerstationIconClicked", nerpfunc_set_powerstation_icon_clicked, 4 },
    // STRING: LEGORR 0x004a735c
    { "GetPowerstationIconClicked", nerpfunc_get_powerstation_icon_clicked, 0 },
    // STRING: LEGORR 0x004a7344
    { "GetPowerstationsBuilt", nerps_port_GetPowerstationsBuilt, 0 },
    // STRING: LEGORR 0x004a7334
    { "SetAttackDefer", nerps_port_SetAttackDefer, 4 },
    // STRING: LEGORR 0x004a7318
    { "SetRockMonsterPainThreshold", nerps_port_SetRockMonsterPainThreshold, 4 },
    // STRING: LEGORR 0x004a7300
    { "SetRockMonsterHealth", nerps_port_SetRockMonsterHealth, 4 },
    // STRING: LEGORR 0x004a72f0
    { "SetGameSpeed", nerpfunc_set_game_speed, 5 },
    // STRING: LEGORR 0x004a72dc
    { "FlashBarracksIcon", nerpfunc_flash_barracks_icon, 4 },
    // STRING: LEGORR 0x004a72c4
    { "SetBarracksIconClicked", nerpfunc_set_barracks_icon_clicked, 4 },
    // STRING: LEGORR 0x004a72ac
    { "GetBarracksIconClicked", nerpfunc_get_barracks_icon_clicked, 0 },
    // STRING: LEGORR 0x004a7290
    { "GetRecordObjectAtTutorial", nerps_port_GetRecordObjectAtTutorial, 1 },
    // STRING: LEGORR 0x004a7278
    { "GetHiddenObjectsFound", nerps_port_GetHiddenObjectsFound, 0 },
    // STRING: LEGORR 0x004a7260
    { "SetHiddenObjectsFound", nerps_port_SetHiddenObjectsFound, 4 },
    // STRING: LEGORR 0x004a7250
    { "GetOxygenLevel", nerps_port_GetOxygenLevel, 0 },
    // STRING: LEGORR 0x004a723c
    { "FlashGeodomeIcon", nerpfunc_flash_geodome_icon, 4 },
    // STRING: LEGORR 0x004a7224
    { "GetGeodomeIconClicked", nerpfunc_get_geodome_icon_clicked, 0 },
    // STRING: LEGORR 0x004a720c
    { "SetGeodomeIconClicked", nerpfunc_set_geodome_icon_clicked, 4 },
    // STRING: LEGORR 0x004a71fc
    { "GetGeodomeBuilt", nerps_port_GetGeodomeBuilt, 0 },
    // STRING: LEGORR 0x004a71ec
    { "AddStoredOre", nerps_port_AddStoredOre, 4 },
    // STRING: LEGORR 0x004a71dc
    { "GenerateSlug", nerps_port_GenerateSlug, 3 },
    // STRING: LEGORR 0x004a71cc
    { "GetSlugsOnLevel", nerps_port_GetSlugsOnLevel, 0 },
    // STRING: LEGORR 0x004a71b8
    { "GetMonstersOnLevel", nerps_port_GetMonstersOnLevel, 0 },
    // STRING: LEGORR 0x004a719c
    { "SetCongregationAtTutorial", nerpfunc_set_congregation_at_tutorial, 5 },
    // STRING: LEGORR 0x004a7188
    { "SetObjectiveSwitch", nerpfunc_set_objective_switch, 4 },
    // STRING: LEGORR 0x004a7174
    { "GetObjectiveSwitch", nerpfunc_get_objective_switch, 0 },
    // STRING: LEGORR 0x004a7158
    { "GetGraniteGrindersOnLevel", nerps_port_GetGraniteGrindersOnLevel, 0 },
    // STRING: LEGORR 0x004a7140
    { "GetSmallDiggersOnLevel", nerps_port_GetSmallDiggersOnLevel, 0 },
    // STRING: LEGORR 0x004a7130
    { "GetDocksBuilt", nerps_port_GetDocksBuilt, 0 },
    // STRING: LEGORR 0x004a7118
    { "GetRapidRidersOnLevel", nerps_port_GetRapidRidersOnLevel, 0 },
    // STRING: LEGORR 0x004a7108
    { "GetUnitAtBlock", nerpfunc_get_unit_at_block, 1 },
    // STRING: LEGORR 0x004a70ec
    { "GetSmallHelicoptersOnLevel", nerps_port_GetSmallHelicoptersOnLevel, 0 },
    // STRING: LEGORR 0x004a70cc
    { "GetRecordObjectAmountAtTutorial", nerps_port_GetRecordObjectAmountAtTutorial, 1 },
    // STRING: LEGORR 0x004a70bc
    { "AdvanceMessage", nerps_port_AdvanceMessage, 3 },
    // STRING: LEGORR 0x004a70a8
    { "AllowCameraMovement", nerps_port_AllowCameraMovement, 4 },
    // STRING: LEGORR 0x004a7094
    { "ClickOnlyObjects", nerps_port_ClickOnlyObjects, 3 },
    // STRING: LEGORR 0x004a7084
    { "ClickOnlyMap", nerps_port_ClickOnlyMap, 3 },
    // STRING: LEGORR 0x004a7074
    { "ClickOnlyIcon", nerps_port_ClickOnlyIcon, 3 },
    // STRING: LEGORR 0x004a7068
    { "DisallowAll", nerps_port_DisallowAll, 3 },
    // STRING: LEGORR 0x004a7058
    { "SupressArrow", nerps_port_SupressArrow, 4 },
    // STRING: LEGORR 0x004a7038
    { "GetMiniFigureinGraniteGrinder", nerps_port_GetMiniFigureinGraniteGrinder, 0 },
    // STRING: LEGORR 0x004a7018
    { "GetMiniFigureinChromeCrusher", nerps_port_GetMiniFigureinChromeCrusher, 0 },
    // STRING: LEGORR 0x004a7000
    { "GetMessagesAreUpToDate", nerps_port_GetMessagesAreUpToDate, 0 },
    // STRING: LEGORR 0x004a6fec
    { "SetCrystalPriority", nerps_port_SetCrystalPriority, 4 },
    // STRING: LEGORR 0x004a6fb8
    { "MakeSomeoneOnThisBlockPickUpSomethingOnThisBlock", nerpfunc_make_someone_on_this_block_pick_up_something_on_this_block, 4 },
    // STRING: LEGORR 0x004a6fa4
    { "SetTrainIconClicked", nerpfunc_set_train_icon_clicked, 4 },
    // STRING: LEGORR 0x004a6f90
    { "GetTrainIconClicked", nerpfunc_get_train_icon_clicked, 0 },
    // STRING: LEGORR 0x004a6f80
    { "FlashTrainIcon", nerpfunc_flash_train_icon, 4 },
    // STRING: LEGORR 0x004a6f64
    { "SetTrainDriverIconClicked", nerpfunc_set_train_driver_icon_clicked, 4 },
    // STRING: LEGORR 0x004a6f48
    { "GetTrainDriverIconClicked", nerpfunc_get_train_driver_icon_clicked, 0 },
    // STRING: LEGORR 0x004a6f30
    { "FlashTrainDriverIcon", nerpfunc_flash_train_driver_icon, 4 },
    // STRING: LEGORR 0x004a6f14
    { "SetTrainPilotIconClicked", nerpfunc_set_train_pilot_icon_clicked, 4 },
    // STRING: LEGORR 0x004a6ef8
    { "GetTrainPilotIconClicked", nerpfunc_get_train_pilot_icon_clicked, 0 },
    // STRING: LEGORR 0x004a6ee4
    { "FlashTrainPilotIcon", nerpfunc_flash_train_pilot_icon, 4 },
    // STRING: LEGORR 0x004a6ec8
    { "SetTrainSailorIconClicked", nerpfunc_set_train_sailor_icon_clicked, 4 },
    // STRING: LEGORR 0x004a6eac
    { "GetTrainSailorIconClicked", nerpfunc_get_train_sailor_icon_clicked, 0 },
    // STRING: LEGORR 0x004a6e94
    { "FlashTrainSailorIcon", nerpfunc_flash_train_sailor_icon, 4 },
    // STRING: LEGORR 0x004a6e7c
    { "GetSmallTruckSelected", nerpfunc_get_small_truck_selected, 0 },
    // STRING: LEGORR 0x004a6e60
    { "GetMiniFigureinSmallTruck", nerps_port_GetMiniFigureinSmallTruck, 0 },
    // STRING: LEGORR 0x004a6e40
    { "GetMiniFigureinSmallHelicopter", nerps_port_GetMiniFigureinSmallHelicopter, 0 },
    // STRING: LEGORR 0x004a6e2c
    { "SetBarracksLevel", nerps_port_SetBarracksLevel, 4 },
    // STRING: LEGORR 0x004a6e1c
    { "SetDocksLevel", nerps_port_SetDocksLevel, 4 },
    // STRING: LEGORR 0x004a6e0c
    { "SetGeoDomeLevel", nerps_port_SetGeoDomeLevel, 4 },
    // STRING: LEGORR 0x004a6df4
    { "SetPowerstationLevel", nerps_port_SetPowerstationLevel, 4 },
    // STRING: LEGORR 0x004a6de0
    { "SetToolStoreLevel", nerps_port_SetToolStoreLevel, 4 },
    // STRING: LEGORR 0x004a6dcc
    { "SetGunstationLevel", nerps_port_SetGunstationLevel, 4 },
    // STRING: LEGORR 0x004a6db8
    { "SetTeleportPadLevel", nerps_port_SetTeleportPadLevel, 4 },
    // STRING: LEGORR 0x004a6da0
    { "SetSuperTeleportLevel", nerps_port_SetSuperTeleportLevel, 4 },
    // STRING: LEGORR 0x004a6d88
    { "SetUpgradeStationLevel", nerps_port_SetUpgradeStationLevel, 4 },
    // STRING: LEGORR 0x004a6d74
    { "GetBarracksSelected", nerps_port_GetBarracksSelected, 0 },
    // STRING: LEGORR 0x004a6d60
    { "GetDocksSelected", nerps_port_GetDocksSelected, 0 },
    // STRING: LEGORR 0x004a6d4c
    { "GetGeoDomeSelected", nerps_port_GetGeoDomeSelected, 0 },
    // STRING: LEGORR 0x004a6d34
    { "GetPowerstationSelected", nerps_port_GetPowerstationSelected, 0 },
    // STRING: LEGORR 0x004a6d1c
    { "GetToolStoreSelected", nerps_port_GetToolStoreSelected, 0 },
    // STRING: LEGORR 0x004a6d04
    { "GetGunstationSelected", nerps_port_GetGunstationSelected, 0 },
    // STRING: LEGORR 0x004a6cec
    { "GetTeleportPadSelected", nerps_port_GetTeleportPadSelected, 0 },
    // STRING: LEGORR 0x004a6cd0
    { "GetSuperTeleportSelected", nerps_port_GetSuperTeleportSelected, 0 },
    // STRING: LEGORR 0x004a6cb4
    { "GetUpgradeStationSelected", nerps_port_GetUpgradeStationSelected, 0 },
    // STRING: LEGORR 0x004a6c94
    { "SetUpgradeBuildingIconClicked", nerpfunc_set_upgrade_building_icon_clicked, 4 },
    // STRING: LEGORR 0x004a6c74
    { "GetUpgradeBuildingIconClicked", nerpfunc_get_upgrade_building_icon_clicked, 0 },
    // STRING: LEGORR 0x004a6c58
    { "FlashUpgradeBuildingIcon", nerpfunc_flash_upgrade_building_icon, 4 },
    // STRING: LEGORR 0x004a6c40
    { "GetBuildingsTeleported", nerps_port_GetBuildingsTeleported, 0 },
    // STRING: LEGORR 0x004a6c28
    { "SetBuildingsTeleported", nerps_port_SetBuildingsTeleported, 4 },
    // STRING: LEGORR 0x004a6c14
    { "CameraLockOnMonster", nerps_port_CameraLockOnMonster, 4 },
    // STRING: LEGORR 0x004a6bf4
    { "SetMonsterAttackPowerstation", nerps_port_SetMonsterAttackPowerstation, 4 },
    // STRING: LEGORR 0x004a6be0
    { "ClickOnlyCalltoarms", nerps_port_ClickOnlyCalltoarms, 3 },
    // STRING: LEGORR 0x004a6bcc
    { "FlashCallToArmsIcon", nerps_port_FlashCallToArmsIcon, 4 },
    // STRING: LEGORR 0x004a6bb4
    { "SetRecordObjectPointer", nerps_port_SetRecordObjectPointer, 4 },
    // STRING: LEGORR 0x004a6b94
    { "GetGetSonicBlasterIconClicked", nerpfunc_get_get_sonic_blaster_icon_clicked, 0 },
    // STRING: LEGORR 0x004a6b74
    { "SetGetSonicBlasterIconClicked", nerpfunc_set_get_sonic_blaster_icon_clicked, 4 },
    // STRING: LEGORR 0x004a6b58
    { "FlashGetSonicBlasterIcon", nerpfunc_flash_get_sonic_blaster_icon, 4 },
    // STRING: LEGORR 0x004a6b38
    { "GetDropSonicBlasterIconClicked", nerpfunc_get_drop_sonic_blaster_icon_clicked, 0 },
    // STRING: LEGORR 0x004a6b18
    { "SetDropSonicBlasterIconClicked", nerpfunc_set_drop_sonic_blaster_icon_clicked, 4 },
    // STRING: LEGORR 0x004a6afc
    { "FlashDropSonicBlasterIcon", nerpfunc_flash_drop_sonic_blaster_icon, 4 },
    // STRING: LEGORR 0x004a6ae4
    { "GetMonsterAtTutorial", nerpfunc_get_monster_at_tutorial, 1 },
    // STRING: LEGORR 0x004a6acc
    { "SetMonsterAttackNowt", nerps_port_SetMonsterAttackNowt, 4 },
    // STRING: LEGORR 0x004a6ab0
    { "GetPlaceFenceIconClicked", nerpfunc_get_place_fence_icon_clicked, 0 },
    // STRING: LEGORR 0x004a6a94
    { "SetPlaceFenceIconClicked", nerpfunc_set_place_fence_icon_clicked, 4 },
    // STRING: LEGORR 0x004a6a80
    { "FlashPlaceFenceIcon", nerpfunc_flash_place_fence_icon, 4 },
    // STRING: LEGORR 0x004a6a68
    { "GetLevel1BarracksBuilt", nerps_port_GetLevel1BarracksBuilt, 0 },
    // STRING: LEGORR 0x004a6a54
    { "GetLevel1DocksBuilt", nerps_port_GetLevel1DocksBuilt, 0 },
    // STRING: LEGORR 0x004a6a3c
    { "GetLevel1GeodomeBuilt", nerps_port_GetLevel1GeodomeBuilt, 0 },
    // STRING: LEGORR 0x004a6a20
    { "GetLevel1PowerstationsBuilt", nerps_port_GetLevel1PowerstationsBuilt, 0 },
    // STRING: LEGORR 0x004a6a04
    { "GetLevel1ToolStoresBuilt", nerps_port_GetLevel1ToolStoresBuilt, 0 },
    // STRING: LEGORR 0x004a69e8
    { "GetLevel1GunstationsBuilt", nerps_port_GetLevel1GunstationsBuilt, 0 },
    // STRING: LEGORR 0x004a69d0
    { "GetLevel1TeleportsBuilt", nerps_port_GetLevel1TeleportsBuilt, 0 },
    // STRING: LEGORR 0x004a69b0
    { "GetLevel1VehicleTeleportsBuilt", nerps_port_GetLevel1VehicleTeleportsBuilt, 0 },
    // STRING: LEGORR 0x004a6990
    { "GetLevel1UpgradeStationsBuilt", nerps_port_GetLevel1UpgradeStationsBuilt, 0 },
    // STRING: LEGORR 0x004a6978
    { "GetLevel2BarracksBuilt", nerps_port_GetLevel2BarracksBuilt, 0 },
    // STRING: LEGORR 0x004a6964
    { "GetLevel2DocksBuilt", nerps_port_GetLevel2DocksBuilt, 0 },
    // STRING: LEGORR 0x004a694c
    { "GetLevel2GeodomeBuilt", nerps_port_GetLevel2GeodomeBuilt, 0 },
    // STRING: LEGORR 0x004a6930
    { "GetLevel2PowerstationsBuilt", nerps_port_GetLevel2PowerstationsBuilt, 0 },
    // STRING: LEGORR 0x004a6914
    { "GetLevel2ToolStoresBuilt", nerps_port_GetLevel2ToolStoresBuilt, 0 },
    // STRING: LEGORR 0x004a68f8
    { "GetLevel2GunstationsBuilt", nerps_port_GetLevel2GunstationsBuilt, 0 },
    // STRING: LEGORR 0x004a68e0
    { "GetLevel2TeleportsBuilt", nerps_port_GetLevel2TeleportsBuilt, 0 },
    // STRING: LEGORR 0x004a68c0
    { "GetLevel2VehicleTeleportsBuilt", nerps_port_GetLevel2VehicleTeleportsBuilt, 0 },
    // STRING: LEGORR 0x004a68a0
    { "GetLevel2UpgradeStationsBuilt", nerps_port_GetLevel2UpgradeStationsBuilt, 0 },
    // STRING: LEGORR 0x004a6888
    { "GetPoweredBarracksBuilt", nerps_port_GetPoweredBarracksBuilt, 0 },
    // STRING: LEGORR 0x004a6870
    { "GetPoweredDocksBuilt", nerps_port_GetPoweredDocksBuilt, 0 },
    // STRING: LEGORR 0x004a6858
    { "GetPoweredGeodomeBuilt", nerps_port_GetPoweredGeodomeBuilt, 0 },
    // STRING: LEGORR 0x004a6838
    { "GetPoweredPowerstationsBuilt", nerps_port_GetPoweredPowerstationsBuilt, 0 },
    // STRING: LEGORR 0x004a681c
    { "GetPoweredToolStoresBuilt", nerps_port_GetPoweredToolStoresBuilt, 0 },
    // STRING: LEGORR 0x004a6800
    { "GetPoweredGunstationsBuilt", nerps_port_GetPoweredGunstationsBuilt, 0 },
    // STRING: LEGORR 0x004a67e4
    { "GetPoweredTeleportsBuilt", nerps_port_GetPoweredTeleportsBuilt, 0 },
    // STRING: LEGORR 0x004a67c4
    { "GetPoweredVehicleTeleportsBuilt", nerps_port_GetPoweredVehicleTeleportsBuilt, 0 },
    // STRING: LEGORR 0x004a67a4
    { "GetPoweredUpgradeStationsBuilt", nerps_port_GetPoweredUpgradeStationsBuilt, 0 },
    // STRING: LEGORR 0x004a6794
    { "**End Of List**", (NERPsFunction)flh_magic_uu_noop, 7 },
};

/* The script operators' debug names (unused by the rebuilt functions). */
// GLOBAL: LEGORR 0x004a6710
static const char* nerpsOperatorNames[] = {
    // STRING: LEGORR 0x004a6790
    "+",
    // STRING: LEGORR 0x004a678c
    "#",
    // STRING: LEGORR 0x004a6788
    "/",
    "\\",
    // STRING: LEGORR 0x004a6784
    "?",
    // STRING: LEGORR 0x004a6780
    ">",
    // STRING: LEGORR 0x004a677c
    "<",
    "=",
    // STRING: LEGORR 0x004a6778
    ">=",
    // STRING: LEGORR 0x004a6774
    "<=",
    // STRING: LEGORR 0x004a6770
    "!=",
};

// GLOBAL: LEGORR 0x004a673c
static int nerpsHasNextButton = 1;
/* 004a6744 and 004a6754 are never read or written; each is the zero tail of
 * an initialised object (a zero-initialised scalar would fall into .bss). */
// GLOBAL: LEGORR 0x004a6740
static NERPs_Next nerpsNext = { 1, 0 };
// GLOBAL: LEGORR 0x004a6748
static NERPs_MessageTime nerpsMessageTime = { 1000.0f, 500.0f, 4000.0f, 0.0f };
// GLOBAL: LEGORR 0x004a6758
static float nerpsIconX = 260.0f;
// GLOBAL: LEGORR 0x004a675c
static float nerpsIconY = 386.0f;
// GLOBAL: LEGORR 0x004a6760
static unsigned int nerpsIconWidth = 20;
// GLOBAL: LEGORR 0x004a6764
static unsigned int nerpsIconSpace = 40;
// GLOBAL: LEGORR 0x004a6768
static int nerpsCurrentSound = -1;
/* The tutorial rows' block-pointer callback, read through this pointer. */
// GLOBAL: LEGORR 0x004a676c
static NERPsBlockPointerCallback nerpsTutorialActionCallback = nerps_tutorial_action_callback;

// GLOBAL: LEGORR 0x004ff958
static NERPs_RuntimeGlobs nerpsruntimeGlobs;

// GLOBAL: LEGORR 0x00555d40
NERPs_Globs nerpsGlobs;

// FUNCTION: LEGORR 0x00452df0
int nerps_file_load_script_file(const char* filename)
{
    unsigned int permit = 1;

    memset(&nerpsGlobs, 0, sizeof(nerpsGlobs));
    ui_message_overlay_state_initialize();
    nerpsGlobs.scriptSize = 0;
    nerpsGlobs.scriptBuffer = NULL;
    memset(&nerpsruntimeGlobs.state, 0, sizeof(nerpsruntimeGlobs.state));
    nerpsruntimeGlobs.state.messagePermit = 1;
    nerpfunc_set_message_permit(&permit);
    nerpsGlobs.scriptBuffer = (unsigned int*)file_load_binary(filename, &nerpsGlobs.scriptSize);
    return nerpsGlobs.scriptBuffer != NULL;
}

// FUNCTION: LEGORR 0x00452e70
int nerps_file_load_message_file(const char* filename)
{
    unsigned int lineCount;
    unsigned int fileSize;
    char** lines;
    char imageName[128];
    char imageKey[128];
    char soundName[4096];
    char soundKey[4096];
    unsigned char* s;
    int lineStart = 1;

    lineCount = 0;
    nerpsGlobs.messageBuffer = (unsigned char*)file_load_binary(filename, &fileSize);
    if (nerpsGlobs.messageBuffer != NULL) {
        for (s = nerpsGlobs.messageBuffer; (unsigned int)(s - nerpsGlobs.messageBuffer) < fileSize; s++) {
            if (*s == '_') *s = ' ';
            if (*s < ' ') {
                *s = '\0';
                lineStart = 1;
            } else {
                if (lineStart) {
                    if (*s == ':') {
                        char* key = (char*)++s;
                        if (sscanf((char*)s, "%s", imageKey)) {
                            lineStart = 0;
                            s += strlen(imageKey);
                            while (*s < '!' && (unsigned int)(s - nerpsGlobs.messageBuffer) < fileSize) {
                                if (*s == '\r') lineStart = 1;
                                *s++ = '\0';
                            }
                            if (sscanf((char*)s, "%s", imageName) && !lineStart) {
                                struct Image* image;
                                s += strlen(imageName);
                                image = image_load_bmp_scaled(imageName, 0, 0);
                                if (image != NULL) {
                                    NERPs_MessageImage* list = (NERPs_MessageImage*)malloc(
                                        sizeof(NERPs_MessageImage) * (nerpsGlobs.imageCount + 1));
                                    unsigned int i;
                                    for (i = 0; i < nerpsGlobs.imageCount; i++) list[i] = nerpsGlobs.imageList[i];
                                    if (nerpsGlobs.imageCount) free(nerpsGlobs.imageList);
                                    list[nerpsGlobs.imageCount].key = key;
                                    list[nerpsGlobs.imageCount].image = image;
                                    nerpsGlobs.imageList = list;
                                    nerpsGlobs.imageCount++;
                                }
                            }
                        }
                    }
                    if (*s == '$') {
                        char* key = (char*)++s;
                        if (sscanf((char*)s, "%s", soundKey)) {
                            lineStart = 0;
                            s += strlen(soundKey);
                            while (*s < '!' && (unsigned int)(s - nerpsGlobs.messageBuffer) < fileSize) {
                                if (*s == '\r') lineStart = 1;
                                *s++ = '\0';
                            }
                            if (sscanf((char*)s, "%s", soundName) && !lineStart) {
                                int sound;
                                s += strlen(soundName);
                                sound = sound_bank_entry_load(soundName, 1, 0, 0);
                                if (sound) {
                                    NERPs_MessageSound* list = (NERPs_MessageSound*)malloc(
                                        sizeof(NERPs_MessageSound) * (nerpsGlobs.soundCount + 1));
                                    unsigned int i;
                                    for (i = 0; i < nerpsGlobs.soundCount; i++) list[i] = nerpsGlobs.soundList[i];
                                    if (nerpsGlobs.soundCount) free(nerpsGlobs.soundList);
                                    list[nerpsGlobs.soundCount].key = key;
                                    list[nerpsGlobs.soundCount].sound = sound;
                                    nerpsGlobs.soundList = list;
                                    nerpsGlobs.currentSound = ++nerpsGlobs.soundCount;
                                }
                            }
                        }
                    } else {
                        char** list = (char**)malloc(sizeof(char*) * (lineCount + 1));
                        unsigned int i;
                        for (i = 0; i < lineCount; i++) list[i] = lines[i];
                        if (lineCount) free(lines);
                        list[lineCount] = (char*)s;
                        lineCount++;
                        lines = list;
                    }
                }
                lineStart = 0;
            }
        }
        nerpsGlobs.messageCount = lineCount;
        nerpsGlobs.messageLines = lines;
        return 1;
    }
    nerpsGlobs.currentSound = nerpsGlobs.soundCount;
    return 0;
}

// FUNCTION: LEGORR 0x00453200
char* nerps_file_get_message_line(unsigned int line)
{
    if (line < nerpsGlobs.messageCount) return nerpsGlobs.messageLines[line];
    return NULL;
}

// FUNCTION: LEGORR 0x00453220
int nerps_shutdown(void)
{
    unsigned int i;

    if (nerpsGlobs.scriptSize) free(nerpsGlobs.scriptBuffer);
    if (nerpsGlobs.messageBuffer) {
        free(nerpsGlobs.messageBuffer);
        if (nerpsGlobs.messageLines) free(nerpsGlobs.messageLines);
    }
    nerpsGlobs.messageBuffer = NULL;
    nerpsGlobs.messageLines = NULL;
    nerpsGlobs.messageCount = 0;
    nerpsGlobs.scriptSize = 0;
    for (i = 0; i < nerpsGlobs.soundCount; i++) sound_bank_entry_release(nerpsGlobs.soundList[i].sound);
    if (nerpsGlobs.soundCount > 0) {
        if (nerpsGlobs.soundList) {
            free(nerpsGlobs.soundList);
            nerpsGlobs.soundList = NULL;
        }
        nerpsGlobs.soundCount = 0;
    }
    memset(&nerpsGlobs, 0, sizeof(nerpsGlobs));
    ui_message_overlay_state_initialize();
    return 1;
}

// FUNCTION: LEGORR 0x004532e0
void nerps_runtime_resolve_argument(unsigned int* value)
{
    unsigned int v = *value;
    unsigned int function = v & 0xffff;

    if ((v & 0xffff0000) == 0x20000 && nerpsFunctions[function].argumentCount == 0)
        v = nerpsFunctions[function].function(NULL);
    *value = v;
}

// FUNCTION: LEGORR 0x00453320
void nerps_runtime_execute(float elapsed)
{
    unsigned int i;
    unsigned int result;
    unsigned int statements;
    unsigned int* script;
    unsigned int stack[3];
    unsigned int count;
    char buff[512];
    unsigned int condition;
    unsigned int operation;
    int negate;
    unsigned int argCount;
    unsigned int kind;

    script = nerpsGlobs.scriptBuffer;
    negate = 0;
    statements = 0;
    if (script != NULL) {
        count = nerpsGlobs.scriptSize / 4;
        condition = 0;
        operation = 0;
        for (i = 0; i < count; i++) {
            argCount = 0;
            if (script[i] & 0x20000) {
                if ((script[i] & 0xffff) == 0 && (condition || operation == 0)) break;
                kind = nerpsFunctions[script[i] & 0xffff].argumentCount;
                if (kind == 0 || kind == 1 || kind == 2) {
                    if (kind == 0) {
                        result = nerpsFunctions[script[i] & 0xffff].function(NULL);
                    } else if (kind == 1) {
                        stack[0] = script[i + 1];
                        nerps_runtime_resolve_argument(&stack[0]);
                        result = nerpsFunctions[script[i] & 0xffff].function(stack);
                        argCount = 1;
                    } else if (kind == 2) {
                        stack[0] = script[i + 1];
                        stack[1] = script[i + 2];
                        nerps_runtime_resolve_argument(&stack[0]);
                        nerps_runtime_resolve_argument(&stack[1]);
                        result = nerpsFunctions[script[i] & 0xffff].function(stack);
                        argCount = 2;
                    }
                    i += argCount;
                    if (negate) result = !result;
                    if (operation == 0) condition = result;
                    else if (operation == 1) condition = (condition && result);
                    else if (operation == 2) condition = (condition || result);
                    else if (operation == 3) condition = result < condition;
                    else if (operation == 4) condition = condition < result;
                    else if (operation == 5) condition = condition == result;
                    else if (operation == 6) condition = condition >= result;
                    else if (operation == 7) condition = result >= condition;
                    else if (operation == 8) condition = condition != result;
                    negate = 0;
                    operation = 1;
                } else if (kind == 3) {
                    if (condition) {
                        if (nerpsruntimeGlobs.debug)
                            sprintf(buff, "Func Call %s", nerpsFunctions[script[i] & 0xffff].name);
                        nerpsFunctions[script[i] & 0xffff].function(NULL);
                    }
                    statements++;
                    condition = 0;
                    operation = 0;
                    negate = 0;
                } else if (kind == 4) {
                    if (condition) {
                        if (nerpsruntimeGlobs.debug)
                            sprintf(buff, "Func Call %s", nerpsFunctions[script[i] & 0xffff].name);
                        stack[0] = script[i + 1];
                        nerps_runtime_resolve_argument(&stack[0]);
                        nerpsFunctions[script[i] & 0xffff].function(stack);
                    }
                    i += 1;
                    statements++;
                    condition = 0;
                    operation = 0;
                    negate = 0;
                } else if (kind == 5) {
                    if (condition) {
                        stack[0] = script[i + 1];
                        stack[1] = script[i + 2];
                        nerps_runtime_resolve_argument(&stack[0]);
                        nerps_runtime_resolve_argument(&stack[1]);
                        nerpsFunctions[script[i] & 0xffff].function(stack);
                    }
                    i += 2;
                    statements++;
                    condition = 0;
                    operation = 0;
                    negate = 0;
                } else if (kind == 6) {
                    if (condition) {
                        stack[0] = script[i + 1];
                        stack[1] = script[i + 2];
                        stack[2] = script[i + 3];
                        nerps_runtime_resolve_argument(&stack[0]);
                        nerps_runtime_resolve_argument(&stack[1]);
                        nerps_runtime_resolve_argument(&stack[2]);
                        nerpsFunctions[script[i] & 0xffff].function(stack);
                    }
                    i += 3;
                    statements++;
                    condition = 0;
                    operation = 0;
                    negate = 0;
                }
            } else if (script[i] & 0x10000) {
                negate = 0;
                if ((script[i] & 0xffff) == 0) operation = 1;
                else if ((script[i] & 0xffff) == 2) operation = 2;
                else if ((script[i] & 0xffff) == 1) negate = 1;
                else if ((script[i] & 0xffff) == 5) operation = 3;
                else if ((script[i] & 0xffff) == 6) operation = 4;
                else if ((script[i] & 0xffff) == 7) operation = 5;
                else if ((script[i] & 0xffff) == 8) operation = 6;
                else if ((script[i] & 0xffff) == 9) operation = 7;
                else if ((script[i] & 0xffff) == 10) operation = 8;
            } else if (script[i] & 0x40000) {
                statements++;
                condition = 0;
                operation = 0;
                negate = 0;
            } else if (script[i] & 0x80000) {
                if (condition) i = script[i] & 0xffff;
                statements++;
                condition = 0;
                operation = 0;
                negate = 0;
            } else {
                result = (short)script[i];
                if (negate) result = !result;
                if (operation == 0) condition = result;
                else if (operation == 1) condition = (condition && result);
                else if (operation == 2) condition = (condition || result);
                else if (operation == 3) condition = result < condition;
                else if (operation == 4) condition = condition < result;
                else if (operation == 5) condition = condition == result;
                else if (operation == 6) condition = condition >= result;
                else if (operation == 7) condition = result >= condition;
                else if (operation == 8) condition = condition != result;
                negate = 0;
                operation = 1;
            }
        }
        nerps_runtime_update_camera(elapsed);
    }
}

// FUNCTION: LEGORR 0x004538a0
int nerpfunc_set_game_speed(unsigned int* stack)
{
    if (stack[0] == 0 && NERPS_LEVEL()->status) return 0;
    global_scroll_guard_set(stack[1]);
    lego_game_speed_set((float)stack[0] * 0.01f);
    global_scroll_guard_set(stack[1]);
    return 0;
}

// FUNCTION: LEGORR 0x00453900
void nerps_set_has_next_button(int hasNext)
{
    nerpsHasNextButton = hasNext;
}

// FUNCTION: LEGORR 0x00453910
void nerps_replay_message_sound(void)
{
    if (nerpsHasNextButton) nerps_play_message_sound();
}

// FUNCTION: LEGORR 0x00453920
void nerps_message_previous(void)
{
    unsigned int stack[2];
    char* line;

    stack[0] = 100;
    stack[1] = 0;
    media_action_feedback_sound_dispatch(0);
    if (nerpsGlobs.messageOffset) {
        nerpsGlobs.messageOffset--;
        if (nerpsGlobs.messageOffset > 3) nerpsGlobs.messageOffset = 3;
        if (nerpsGlobs.messageOffset > nerpsGlobs.messageHistoryCount)
            nerpsGlobs.messageOffset = nerpsGlobs.messageHistoryCount;
        line = nerps_file_get_message_line(
            nerpsGlobs.messageHistory[nerpsGlobs.messageHistoryCount - nerpsGlobs.messageOffset - 1]);
        advisor_text_request_setup((uint32_t)(uintptr_t)line, 0);
        if (nerpsGlobs.messageOffset == 0) {
            engine_global_state_flags_bits6_and_9_set_target(0, 1);
            if (nerpfunc_get_tutorial_flags(NULL)) nerpfunc_set_game_speed(stack);
        }
        engine_global_state_flags_bit7_set_target(1);
    } else {
        if (nerpsNext.pressed) nerpsGlobs.messageAcknowledged++;
        nerpsNext.pressed = 0;
    }
}

// FUNCTION: LEGORR 0x00453bb0
void nerps_runtime_update_timers(float elapsed)
{
    unsigned int i;

    elapsed = (elapsed * 1000.0f) * 0.04f;
    for (i = 0; i < 4; i++) nerpsruntimeGlobs.timers[i] += elapsed;
    if (nerpsruntimeGlobs.messageTimer == 0.0f) return;
    if (nerpsruntimeGlobs.messageTimer < 0.0f) {
        nerpsruntimeGlobs.messageTimer = 0.0f;
    } else if (!nerpsHasNextButton || nerpsruntimeGlobs.messageWait) {
        nerpsruntimeGlobs.messageTimer -= elapsed;
        if (nerpsruntimeGlobs.messageTimer <= 0.0f) nerpsruntimeGlobs.messageTimer = 0.0f;
    } else {
        if (nerpsruntimeGlobs.previousNextPressed &&
            ((nerpsGlobs.messageOffset > 0 && nerpsruntimeGlobs.supressArrow) || !nerpsruntimeGlobs.supressArrow))
            engine_global_state_flags_bits6_and_9_set_target(nerpsNext.pressed, 1);
        nerpsruntimeGlobs.previousNextPressed = nerpsNext.pressed;
        if (!nerpsNext.pressed) nerpsruntimeGlobs.messageTimer = 0.0f;
    }
    if (nerpsGlobs.messageSoundPlaying && nerpsruntimeGlobs.messageTimer < 1000.0f)
        advisor_clear_active_state();
}

/* legoGlobs' fields (lego_globs.h, finding 653), included before their
 * first use so that the functions above keep their symbol counter
 * (finding 538). */
#include "lego_globs.h"

// FUNCTION: LEGORR 0x00453da0
void nerps_runtime_update_camera(float elapsed)
{
    NERPs_Point2I blockPos;
    float delta;

    if (nerpsGlobs.cameraTrack) {
        if (nerpsGlobs.cameraTrackObject) {
            entity_position_to_terrain_cell_adapter(nerpsGlobs.cameraTrackObject, &blockPos.x, &blockPos.y);
            entity_transform_position_get_outputs(nerpsGlobs.cameraTrackObject, &nerpsGlobs.cameraX, &nerpsGlobs.cameraY);
            lego_viewport_target_position_clamp(nerpsGlobs.cameraTrackObject, NULL, 1);
        } else if (nerpsGlobs.cameraTrackIndex < 10) {
            void* object;
            if (!lego_object_pointer_registry_get_slot(nerpsGlobs.cameraTrackIndex, &object)) {
                nerpsGlobs.cameraTrack = 0;
            } else {
                entity_position_to_terrain_cell_adapter((struct EntitySlot*)object, &blockPos.x, &blockPos.y);
                entity_transform_position_get_outputs((struct EntitySlot*)object, &nerpsGlobs.cameraX, &nerpsGlobs.cameraY);
                lego_viewport_target_position_clamp((struct EntitySlot*)object, NULL, 1);
            }
        }
    }
    if (nerpsGlobs.cameraZoom) {
        elapsed = nerpsGlobs.cameraZoomAmount / (elapsed * 25.0f);
        nerpsGlobs.cameraZoomCurrent += elapsed;
        if (nerpsGlobs.cameraZoomCurrent > nerpsGlobs.cameraZoomAmount - 5.0f &&
            nerpsGlobs.cameraZoomCurrent < nerpsGlobs.cameraZoomAmount - -5.0f) {
            nerpsGlobs.cameraZoom = 0;
            nerpsGlobs.cameraZoomCurrent = 0.0f;
        }
        camera_zoom_update(s_zoom_step_context, elapsed);
    }
    if (nerpsGlobs.cameraRotate) {
        if (nerpsGlobs.cameraRotateAmount > 0.0f)
            elapsed = (nerpsGlobs.cameraRotateAmount - nerpsGlobs.cameraRotateCurrent) * 0.16666667f;
        else
            elapsed = (nerpsGlobs.cameraRotateAmount + nerpsGlobs.cameraRotateCurrent) * 0.16666667f;
        delta = global_float_field_00556f00_get() * elapsed * 0.33333334f;
        nerpsGlobs.cameraRotateCurrent += delta;
        if ((nerpsGlobs.cameraRotateAmount > 0.0f &&
             nerpsGlobs.cameraRotateCurrent > (float)(nerpsGlobs.cameraRotateAmount - 1.0f)) ||
            (nerpsGlobs.cameraRotateAmount < 0.0f &&
             nerpsGlobs.cameraRotateCurrent < (float)(nerpsGlobs.cameraRotateAmount - -1.0f))) {
            nerpsGlobs.cameraRotate = 0;
            nerpsGlobs.cameraRotateCurrent = 0.0f;
        }
        camera_tilt_axis_update(s_zoom_step_context, delta * 0.017453292f);
    }
    if (NERPS_LEVEL()->status) {
        unsigned int stack[2];
        stack[0] = 100;
        stack[1] = 0;
        nerpfunc_set_game_speed(stack);
    }
}

// FUNCTION: LEGORR 0x00454600
void nerps_draw_tutorial_icon(NERPs_Level* level, void* viewport, int unused, float elapsed)
{
    float bounce;
    NERPs_Object* object;
    float screen[2];
    NERPs_Vector3F world;

    bounce = (float)sin(nerpsruntimeGlobs.iconBounce);
    nerpsruntimeGlobs.iconBounce -= (elapsed * 25.0f) * -0.013333334f;
    if (nerpsGlobs.tutorialIconObject) {
        if (lego_object_pointer_registry_get_slot(nerpsGlobs.tutorialIconObject - 1, (void**)&object)) {
            object->tutorialIcon = 1;
            entity_transform_position_get_outputs((struct EntitySlot*)object, &world.x, &world.y);
            world.z = terrain_map_world_z_get(level->map, world.x, world.y);
            camera_project_point_to_screen((struct ViewProjectionAdapter*)viewport, screen, &world.x);
            screen[0] -= (float)((int)legoGlobs_TutorialIcon->width / 2);
            screen[1] -= bounce * 20.0f + (float)((int)legoGlobs_TutorialIcon->height * 2);
            image_display_scaled(legoGlobs_TutorialIcon, NULL, (Point2F*)screen, NULL);
        }
    }
}

// FUNCTION: LEGORR 0x00454710
int nerps_get_icon_clicks(unsigned int* stack)
{
    return interface_get_item_clicks(stack[0]);
}

// FUNCTION: LEGORR 0x00454880
void nerps_monster_destroyed(void)
{
    nerpsruntimeGlobs.monstersDestroyed++;
}

// FUNCTION: LEGORR 0x00454890
void nerps_monsters_destroyed_reset(void)
{
    nerpsruntimeGlobs.monstersDestroyed = 0;
}

// FUNCTION: LEGORR 0x004548c0
int nerpfunc_set_upgrade_building_icon_clicked(unsigned int* stack)
{
    return interface_set_item_clicks(65, stack[0]);
}

// FUNCTION: LEGORR 0x004548e0
int nerpfunc_get_upgrade_building_icon_clicked(unsigned int* stack)
{
    unsigned int item = 65;
    return nerps_get_icon_clicks(&item);
}

// FUNCTION: LEGORR 0x00454900
int nerpfunc_flash_upgrade_building_icon(unsigned int* stack)
{
    return nerps_flash_icon(65, stack[0]);
}

// FUNCTION: LEGORR 0x00454920
int nerpfunc_set_go_back_icon_clicked(unsigned int* stack)
{
    return interface_set_item_clicks(0, stack[0]);
}

// FUNCTION: LEGORR 0x00454940
int nerpfunc_get_go_back_icon_clicked(unsigned int* stack)
{
    unsigned int item = 0;
    return nerps_get_icon_clicks(&item);
}

// FUNCTION: LEGORR 0x00454960
int nerpfunc_flash_go_back_icon(unsigned int* stack)
{
    return nerps_flash_icon(0, stack[0]);
}

// FUNCTION: LEGORR 0x00454b70
int nerpfunc_set_level_completed(unsigned int* stack)
{
    objective_announce_current(1);
    return 1;
}

// FUNCTION: LEGORR 0x00454b80
int nerpfunc_set_level_fail(unsigned int* stack)
{
    if (!(legoGlobs_flags1 & 0x8000000)) objective_announce_current(2);
    return 1;
}

// FUNCTION: LEGORR 0x00454bb0
int nerpfunc_set_tutorial_pointer(unsigned int* stack)
{
    nerps_set_tutorial_pointer(stack[0], stack[1]);
    return 1;
}

// FUNCTION: LEGORR 0x00454bd0
unsigned int nerps_set_object_icon_clicks(const char* name, unsigned int clicks)
{
    int type;
    unsigned int id;

    lego_entity_type_pair_lookup(name, &type, &id, NULL);
    return interface_set_object_item_clicks(type, id, clicks);
}

// FUNCTION: LEGORR 0x00454c10
int nerps_flash_object_icon(const char* name, int on)
{
    int type;
    unsigned int id;

    lego_entity_type_pair_lookup(name, &type, &id, NULL);
    interface_set_object_item_flash(type, id, on);
    return type;
}

// FUNCTION: LEGORR 0x00454c50
int nerps_set_tutorial_block_clicks_callback(NERPs_BlockPointer* blockPointer,
                                             unsigned int x, unsigned int y,
                                             void* data)
{
    NERPs_Level* level = NERPS_LEVEL();
    level->blocks[y * level->width + x].tutorialBlockClicks = *(int*)data;
    return 1;
}

// FUNCTION: LEGORR 0x00454c80
int nerpfunc_set_tutorial_block_clicks(unsigned int* stack)
{
    nerps_block_pointer_enumerate(stack[0], nerps_set_tutorial_block_clicks_callback, &stack[1]);
    return 1;
}

// FUNCTION: LEGORR 0x00454ca0
int nerps_set_tutorial_crystals_callback(NERPs_BlockPointer* blockPointer,
                                         unsigned int x, unsigned int y,
                                         void* data)
{
    NERPs_Level* level = NERPS_LEVEL();
    NERPs_Point2I blockPos;
    int i;

    blockPos.x = x;
    blockPos.y = y;
    level->blocks[y * level->width + x].tutorialCrystals = *(int*)data;
    for (i = 0; i < *(int*)data; i++)
        legoobject_build_placement_command_setup(&blockPos.x, 0, NULL, 1);
    return 1;
}

// FUNCTION: LEGORR 0x00454d10
int nerpfunc_set_tutorial_crystals(unsigned int* stack)
{
    nerps_block_pointer_enumerate(stack[0], nerps_set_tutorial_crystals_callback, &stack[1]);
    return 1;
}

// FUNCTION: LEGORR 0x00454d30
int nerps_set_ore_at_icon_positions_callback(NERPs_BlockPointer* blockPointer,
                                             unsigned int x, unsigned int y,
                                             void* data)
{
    NERPs_Level* level = NERPS_LEVEL();
    NERPs_Point2I blockPos;

    blockPos.x = x;
    blockPos.y = y;
    level->blocks[y * level->width + x].tutorialOre = *(int*)data;
    legoobject_build_placement_alternate_command_setup(&blockPos.x, 0, NULL, 0);
    return 1;
}

// FUNCTION: LEGORR 0x00454d90
int nerpfunc_set_ore_at_icon_positions(unsigned int* stack)
{
    nerps_block_pointer_enumerate(stack[0], nerps_set_ore_at_icon_positions_callback, &stack[1]);
    return 1;
}

// FUNCTION: LEGORR 0x00454e20
int nerpfunc_get_small_truck_selected(unsigned int* stack)
{
    NERPs_Object* object;
    int type;
    unsigned int id;

    object = (NERPs_Object*)interface_get_primary_selected();
    if (object == NULL) return 0;
    // STRING: LEGORR 0x004a7f8c
    lego_entity_type_pair_lookup("SmallTruck", &type, &id, NULL);
    if (object->id != id) return 0;
    return object->type == type;
}

// FUNCTION: LEGORR 0x00454e80
int nerpfunc_get_small_digger_selected(unsigned int* stack)
{
    NERPs_Object* object;
    int type;
    unsigned int id;

    object = (NERPs_Object*)interface_get_primary_selected();
    if (object == NULL) return 0;
    // STRING: LEGORR 0x004a7f98
    lego_entity_type_pair_lookup("SmallDigger", &type, &id, NULL);
    if (object->id != id) return 0;
    return object->type == type;
}

// FUNCTION: LEGORR 0x00454ee0
int nerpfunc_get_rapid_rider_selected(unsigned int* stack)
{
    NERPs_Object* object;
    int type;
    unsigned int id;

    object = (NERPs_Object*)interface_get_primary_selected();
    if (object == NULL) return 0;
    // STRING: LEGORR 0x004a7fa4
    lego_entity_type_pair_lookup("SmallCat", &type, &id, NULL);
    if (object->id != id) return 0;
    return object->type == type;
}

// FUNCTION: LEGORR 0x00454f40
int nerpfunc_get_small_helicopter_selected(unsigned int* stack)
{
    NERPs_Object* object;
    int type;
    unsigned int id;

    object = (NERPs_Object*)interface_get_primary_selected();
    if (object == NULL) return 0;
    // STRING: LEGORR 0x004a7fb0
    lego_entity_type_pair_lookup("SmallHeli", &type, &id, NULL);
    if (object->id != id) return 0;
    return object->type == type;
}

// FUNCTION: LEGORR 0x00454fa0
int nerpfunc_get_granite_grinder_selected(unsigned int* stack)
{
    NERPs_Object* object;
    int type;
    unsigned int id;

    object = (NERPs_Object*)interface_get_primary_selected();
    if (object == NULL) return 0;
    // STRING: LEGORR 0x004a7fbc
    lego_entity_type_pair_lookup("WalkerDigger", &type, &id, NULL);
    if (object->id != id) return 0;
    return object->type == type;
}

// FUNCTION: LEGORR 0x00455000
int nerpfunc_get_chrome_crusher_selected(unsigned int* stack)
{
    NERPs_Object* object;
    int type;
    unsigned int id;

    object = (NERPs_Object*)interface_get_primary_selected();
    if (object == NULL) return 0;
    // STRING: LEGORR 0x004a7fcc
    lego_entity_type_pair_lookup("LargeDigger", &type, &id, NULL);
    if (object->id != id) return 0;
    return object->type == type;
}

// FUNCTION: LEGORR 0x00455130
int nerps_add_tutorial_crystals_callback(NERPs_BlockPointer* blockPointer,
                                         unsigned int x, unsigned int y,
                                         void* data)
{
    NERPs_Level* level = NERPS_LEVEL();
    *(int*)data += level->blocks[y * level->width + x].tutorialCrystals;
    return 1;
}

// FUNCTION: LEGORR 0x00455160
int nerpfunc_get_tutorial_crystals(unsigned int* stack)
{
    int count = 0;
    nerps_block_pointer_enumerate(stack[0], nerps_add_tutorial_crystals_callback, &count);
    return count;
}

// FUNCTION: LEGORR 0x00455190
int nerps_add_tutorial_block_clicks_callback(NERPs_BlockPointer* blockPointer,
                                             unsigned int x, unsigned int y,
                                             void* data)
{
    NERPs_Level* level = NERPS_LEVEL();
    *(int*)data += level->blocks[y * level->width + x].tutorialBlockClicks;
    return 1;
}

// FUNCTION: LEGORR 0x004551c0
int nerpfunc_get_tutorial_block_clicks(unsigned int* stack)
{
    int count = 0;
    nerps_block_pointer_enumerate(stack[0], nerps_add_tutorial_block_clicks_callback, &count);
    return count;
}

// FUNCTION: LEGORR 0x00455300
unsigned int nerps_object_count(const char* name, int level)
{
    int type;
    unsigned int id;

    lego_entity_type_pair_lookup(name, &type, &id, NULL);
    return legoobject_grid_record_count_get(type, id, level);
}

// FUNCTION: LEGORR 0x00455340
unsigned int nerps_get_object_icon_clicks(const char* name)
{
    int type;
    unsigned int id;

    lego_entity_type_pair_lookup(name, &type, &id, NULL);
    return interface_get_object_item_clicks(type, id);
}

// FUNCTION: LEGORR 0x00455370
int nerps_flash_icon(int item, int on)
{
    if (on == 1) {
        if (interface_set_advisor_item(item, 1)) {
            interface_set_item_flash(item, 1);
            return 1;
        }
    } else {
        advisor_clear_active_state();
        interface_set_item_flash(item, 0);
    }
    return 0;
}

// FUNCTION: LEGORR 0x00455810
int nerpfunc_get_barracks_icon_clicked(unsigned int* stack)
{
    return nerps_get_object_icon_clicks("Barracks");
}

// FUNCTION: LEGORR 0x00455820
int nerpfunc_get_geodome_icon_clicked(unsigned int* stack)
{
    // STRING: LEGORR 0x004a7fe8
    return nerps_get_object_icon_clicks("Geo-dome");
}

// FUNCTION: LEGORR 0x00455830
int nerpfunc_get_powerstation_icon_clicked(unsigned int* stack)
{
    // STRING: LEGORR 0x004a7ff4
    return nerps_get_object_icon_clicks("Powerstation");
}

// FUNCTION: LEGORR 0x00455840
int nerpfunc_get_tool_store_icon_clicked(unsigned int* stack)
{
    // STRING: LEGORR 0x004a8004
    return nerps_get_object_icon_clicks("Toolstation");
}

// FUNCTION: LEGORR 0x00455850
int nerpfunc_get_gunstation_icon_clicked(unsigned int* stack)
{
    // STRING: LEGORR 0x004a8010
    return nerps_get_object_icon_clicks("Gunstation");
}

// FUNCTION: LEGORR 0x00455860
int nerpfunc_get_teleport_pad_icon_clicked(unsigned int* stack)
{
    // STRING: LEGORR 0x004a801c
    return nerps_get_object_icon_clicks("TeleportPad");
}

// FUNCTION: LEGORR 0x00455870
int nerpfunc_get_vehicle_transport_icon_clicked(unsigned int* stack)
{
    // STRING: LEGORR 0x004a8028
    return nerps_get_object_icon_clicks("TeleportBIG");
}

// FUNCTION: LEGORR 0x00455880
int nerpfunc_get_upgrade_station_icon_clicked(unsigned int* stack)
{
    // STRING: LEGORR 0x004a8034
    return nerps_get_object_icon_clicks("Upgrade");
}

// FUNCTION: LEGORR 0x00455890
int nerpfunc_set_barracks_icon_clicked(unsigned int* stack)
{
    return nerps_set_object_icon_clicks("Barracks", stack[0]);
}

// FUNCTION: LEGORR 0x004558b0
int nerpfunc_set_geodome_icon_clicked(unsigned int* stack)
{
    return nerps_set_object_icon_clicks("Geo-dome", stack[0]);
}

// FUNCTION: LEGORR 0x004558d0
int nerpfunc_set_powerstation_icon_clicked(unsigned int* stack)
{
    return nerps_set_object_icon_clicks("Powerstation", stack[0]);
}

// FUNCTION: LEGORR 0x004558f0
int nerpfunc_set_tool_store_icon_clicked(unsigned int* stack)
{
    return nerps_set_object_icon_clicks("Toolstation", stack[0]);
}

// FUNCTION: LEGORR 0x00455910
int nerpfunc_set_gunstation_icon_clicked(unsigned int* stack)
{
    return nerps_set_object_icon_clicks("Gunstation", stack[0]);
}

// FUNCTION: LEGORR 0x00455930
int nerpfunc_set_teleport_pad_icon_clicked(unsigned int* stack)
{
    return nerps_set_object_icon_clicks("TeleportPad", stack[0]);
}

// FUNCTION: LEGORR 0x00455950
int nerpfunc_set_vehicle_transport_icon_clicked(unsigned int* stack)
{
    return nerps_set_object_icon_clicks("TeleportBIG", stack[0]);
}

// FUNCTION: LEGORR 0x00455970
int nerpfunc_set_upgrade_station_icon_clicked(unsigned int* stack)
{
    return nerps_set_object_icon_clicks("Upgrade", stack[0]);
}

// FUNCTION: LEGORR 0x00455990
int nerpfunc_flash_barracks_icon(unsigned int* stack)
{
    return nerps_flash_object_icon("Barracks", stack[0]);
}

// FUNCTION: LEGORR 0x004559b0
int nerpfunc_flash_geodome_icon(unsigned int* stack)
{
    return nerps_flash_object_icon("Geo-dome", stack[0]);
}

// FUNCTION: LEGORR 0x004559d0
int nerpfunc_flash_power_station_icon(unsigned int* stack)
{
    return nerps_flash_object_icon("Powerstation", stack[0]);
}

// FUNCTION: LEGORR 0x004559f0
int nerpfunc_flash_tool_store_icon(unsigned int* stack)
{
    return nerps_flash_object_icon("Toolstation", stack[0]);
}

// FUNCTION: LEGORR 0x00455a10
int nerpfunc_flash_gunstation_icon(unsigned int* stack)
{
    return nerps_flash_object_icon("Gunstation", stack[0]);
}

// FUNCTION: LEGORR 0x00455a30
int nerpfunc_flash_teleport_pad_icon(unsigned int* stack)
{
    return nerps_flash_object_icon("TeleportPad", stack[0]);
}

// FUNCTION: LEGORR 0x00455a50
int nerpfunc_flash_vehicle_transport_icon(unsigned int* stack)
{
    return nerps_flash_object_icon("TeleportBIG", stack[0]);
}

// FUNCTION: LEGORR 0x00455a70
int nerpfunc_flash_upgrade_station_icon(unsigned int* stack)
{
    return nerps_flash_object_icon("Upgrade", stack[0]);
}

// FUNCTION: LEGORR 0x00455b30
int nerpfunc_get_teleport_icon_clicked(unsigned int* stack)
{
    unsigned int item = 1;
    return nerps_get_icon_clicks(&item);
}

// FUNCTION: LEGORR 0x00455b50
int nerpfunc_get_dynamite_clicked(unsigned int* stack)
{
    unsigned int item = 13;
    return nerps_get_icon_clicks(&item);
}

// FUNCTION: LEGORR 0x00455b70
int nerpfunc_get_mount_icon_clicked(unsigned int* stack)
{
    unsigned int item = 24;
    return nerps_get_icon_clicks(&item);
}

// FUNCTION: LEGORR 0x00455b90
int nerpfunc_get_train_icon_clicked(unsigned int* stack)
{
    unsigned int item = 57;
    return nerps_get_icon_clicks(&item);
}

// FUNCTION: LEGORR 0x00455bb0
int nerpfunc_get_drop_sonic_blaster_icon_clicked(unsigned int* stack)
{
    unsigned int item = 33;
    return nerps_get_icon_clicks(&item);
}

// FUNCTION: LEGORR 0x00455bd0
int nerpfunc_get_get_tool_icon_clicked(unsigned int* stack)
{
    unsigned int item = 48;
    return nerps_get_icon_clicks(&item);
}

// FUNCTION: LEGORR 0x00455bf0
int nerpfunc_get_get_pusher_icon_clicked(unsigned int* stack)
{
    unsigned int item = 54;
    return nerps_get_icon_clicks(&item);
}

// FUNCTION: LEGORR 0x00455c10
int nerpfunc_get_get_sonic_blaster_icon_clicked(unsigned int* stack)
{
    unsigned int item = 56;
    return nerps_get_icon_clicks(&item);
}

// FUNCTION: LEGORR 0x00455c30
int nerpfunc_get_train_sailor_icon_clicked(unsigned int* stack)
{
    unsigned int item = 62;
    return nerps_get_icon_clicks(&item);
}

// FUNCTION: LEGORR 0x00455c50
int nerpfunc_get_train_pilot_icon_clicked(unsigned int* stack)
{
    unsigned int item = 61;
    return nerps_get_icon_clicks(&item);
}

// FUNCTION: LEGORR 0x00455c70
int nerpfunc_get_train_driver_icon_clicked(unsigned int* stack)
{
    unsigned int item = 58;
    return nerps_get_icon_clicks(&item);
}

// FUNCTION: LEGORR 0x00455c90
int nerpfunc_get_get_laser_icon_clicked(unsigned int* stack)
{
    unsigned int item = 53;
    return nerps_get_icon_clicks(&item);
}

// FUNCTION: LEGORR 0x00455cb0
int nerpfunc_get_dismount_icon_clicked(unsigned int* stack)
{
    unsigned int item = 25;
    return nerps_get_icon_clicks(&item);
}

// FUNCTION: LEGORR 0x00455cd0
int nerpfunc_get_dig_icon_clicked(unsigned int* stack)
{
    unsigned int item = 11;
    return nerps_get_icon_clicks(&item);
}

// FUNCTION: LEGORR 0x00455cf0
int nerpfunc_get_build_icon_clicked(unsigned int* stack)
{
    unsigned int item = 2;
    return nerps_get_icon_clicks(&item);
}

// FUNCTION: LEGORR 0x00455d10
int nerpfunc_get_lay_path_icon_clicked(unsigned int* stack)
{
    unsigned int item = 5;
    return nerps_get_icon_clicks(&item);
}

// FUNCTION: LEGORR 0x00455d30
int nerpfunc_get_place_fence_icon_clicked(unsigned int* stack)
{
    unsigned int item = 14;
    return nerps_get_icon_clicks(&item);
}

// FUNCTION: LEGORR 0x00455d50
int nerpfunc_set_teleport_icon_clicked(unsigned int* stack)
{
    return interface_set_item_clicks(1, stack[0]);
}

// FUNCTION: LEGORR 0x00455d70
int nerpfunc_set_dynamite_clicked(unsigned int* stack)
{
    return interface_set_item_clicks(13, stack[0]);
}

// FUNCTION: LEGORR 0x00455d90
int nerpfunc_set_train_icon_clicked(unsigned int* stack)
{
    return interface_set_item_clicks(57, stack[0]);
}

// FUNCTION: LEGORR 0x00455db0
int nerpfunc_set_train_driver_icon_clicked(unsigned int* stack)
{
    return interface_set_item_clicks(58, stack[0]);
}

// FUNCTION: LEGORR 0x00455dd0
int nerpfunc_set_train_sailor_icon_clicked(unsigned int* stack)
{
    return interface_set_item_clicks(62, stack[0]);
}

// FUNCTION: LEGORR 0x00455df0
int nerpfunc_set_get_tool_icon_clicked(unsigned int* stack)
{
    return interface_set_item_clicks(48, stack[0]);
}

// FUNCTION: LEGORR 0x00455e10
int nerpfunc_set_drop_sonic_blaster_icon_clicked(unsigned int* stack)
{
    return interface_set_item_clicks(33, stack[0]);
}

// FUNCTION: LEGORR 0x00455e30
int nerpfunc_set_get_laser_icon_clicked(unsigned int* stack)
{
    return interface_set_item_clicks(53, stack[0]);
}

// FUNCTION: LEGORR 0x00455e50
int nerpfunc_set_get_pusher_icon_clicked(unsigned int* stack)
{
    return interface_set_item_clicks(54, stack[0]);
}

// FUNCTION: LEGORR 0x00455e70
int nerpfunc_set_get_sonic_blaster_icon_clicked(unsigned int* stack)
{
    return interface_set_item_clicks(56, stack[0]);
}

// FUNCTION: LEGORR 0x00455e90
int nerpfunc_set_dismount_icon_clicked(unsigned int* stack)
{
    return interface_set_item_clicks(25, stack[0]);
}

// FUNCTION: LEGORR 0x00455eb0
int nerpfunc_set_train_pilot_icon_clicked(unsigned int* stack)
{
    return interface_set_item_clicks(61, stack[0]);
}

// FUNCTION: LEGORR 0x00455ed0
int nerpfunc_set_mount_icon_clicked(unsigned int* stack)
{
    return interface_set_item_clicks(24, stack[0]);
}

// FUNCTION: LEGORR 0x00455ef0
int nerpfunc_set_dig_icon_clicked(unsigned int* stack)
{
    return interface_set_item_clicks(11, stack[0]);
}

// FUNCTION: LEGORR 0x00455f10
int nerpfunc_set_build_icon_clicked(unsigned int* stack)
{
    return interface_set_item_clicks(2, stack[0]);
}

// FUNCTION: LEGORR 0x00455f30
int nerpfunc_set_lay_path_icon_clicked(unsigned int* stack)
{
    return interface_set_item_clicks(5, stack[0]);
}

// FUNCTION: LEGORR 0x00455f50
int nerpfunc_set_place_fence_icon_clicked(unsigned int* stack)
{
    return interface_set_item_clicks(14, stack[0]);
}

// FUNCTION: LEGORR 0x00455f70
int nerpfunc_flash_teleport_icon(unsigned int* stack)
{
    return nerps_flash_icon(1, stack[0]);
}

// FUNCTION: LEGORR 0x00455f90
int nerpfunc_flash_dynamite_icon(unsigned int* stack)
{
    return nerps_flash_icon(13, stack[0]);
}

// FUNCTION: LEGORR 0x00455fb0
int nerpfunc_flash_mount_icon(unsigned int* stack)
{
    return nerps_flash_icon(24, stack[0]);
}

// FUNCTION: LEGORR 0x00455fd0
int nerpfunc_flash_train_icon(unsigned int* stack)
{
    return nerps_flash_icon(57, stack[0]);
}

// FUNCTION: LEGORR 0x00455ff0
int nerpfunc_flash_train_driver_icon(unsigned int* stack)
{
    return nerps_flash_icon(58, stack[0]);
}

// FUNCTION: LEGORR 0x00456010
int nerpfunc_flash_train_pilot_icon(unsigned int* stack)
{
    return nerps_flash_icon(61, stack[0]);
}

// FUNCTION: LEGORR 0x00456030
int nerpfunc_flash_train_sailor_icon(unsigned int* stack)
{
    return nerps_flash_icon(62, stack[0]);
}

// FUNCTION: LEGORR 0x00456050
int nerpfunc_flash_dismount_icon(unsigned int* stack)
{
    return nerps_flash_icon(25, stack[0]);
}

// FUNCTION: LEGORR 0x00456070
int nerpfunc_flash_get_tool_icon(unsigned int* stack)
{
    return nerps_flash_icon(48, stack[0]);
}

// FUNCTION: LEGORR 0x00456090
int nerpfunc_flash_drop_sonic_blaster_icon(unsigned int* stack)
{
    return nerps_flash_icon(33, stack[0]);
}

// FUNCTION: LEGORR 0x004560b0
int nerpfunc_flash_get_laser_icon(unsigned int* stack)
{
    return nerps_flash_icon(53, stack[0]);
}

// FUNCTION: LEGORR 0x004560d0
int nerpfunc_flash_get_pusher_icon(unsigned int* stack)
{
    return nerps_flash_icon(54, stack[0]);
}

// FUNCTION: LEGORR 0x004560f0
int nerpfunc_flash_get_sonic_blaster_icon(unsigned int* stack)
{
    return nerps_flash_icon(56, stack[0]);
}

// FUNCTION: LEGORR 0x00456110
int nerpfunc_flash_dig_icon(unsigned int* stack)
{
    return nerps_flash_icon(11, stack[0]);
}

// FUNCTION: LEGORR 0x00456130
int nerpfunc_flash_build_icon(unsigned int* stack)
{
    return nerps_flash_icon(2, stack[0]);
}

// FUNCTION: LEGORR 0x00456150
int nerpfunc_flash_lay_path_icon(unsigned int* stack)
{
    return nerps_flash_icon(5, stack[0]);
}

// FUNCTION: LEGORR 0x00456170
int nerpfunc_flash_place_fence_icon(unsigned int* stack)
{
    return nerps_flash_icon(14, stack[0]);
}

/* NERPFunc__Stop, False, Null, GetCrystalsUsed, GetCrystalsStolen,
 * GetOreUsed and GetOreStolen. The port's name: flic_unit.cpp and crt.cpp
 * call the same zero-returning body (the original's callers there reach
 * 00456210 too). Shared only under MSVC x86: natively the port tail keeps
 * its own definition, so the gods98 tests that call it do not pull this
 * unit (and its gods98 callees) in after the last gods98 pass of the link. */
// FUNCTION: LEGORR 0x00456210
#if defined(_MSC_VER) && defined(_M_IX86)
uint32_t flh_magic_uu_noop(void)
{
    return 0;
}
#endif

// FUNCTION: LEGORR 0x00456240
int nerpfunc_get_tutorial_flags(unsigned int* stack)
{
    return nerpsruntimeGlobs.state.tutorialFlags;
}

// FUNCTION: LEGORR 0x00456550
int nerpfunc_set_tutorial_flags(unsigned int* stack)
{
    nerpsruntimeGlobs.state.tutorialFlags = stack[0];
    return stack[0];
}

// FUNCTION: LEGORR 0x004565f0
int nerpfunc_set_message_permit(unsigned int* stack)
{
    nerpsruntimeGlobs.state.messagePermit = stack[0];
    if (stack[0] == 1) {
        engine_global_state_flags_bits6_and_9_set_target(0, 1);
        engine_global_state_flags_bit7_set_target(0);
    }
    s_overlay_word_03610 = 0;
    s_overlay_word_03614 = 0;
    return stack[0];
}

// FUNCTION: LEGORR 0x00456640
int nerps_get_message_permit(void)
{
    return nerpsruntimeGlobs.state.messagePermit;
}

// FUNCTION: LEGORR 0x004567c0
int nerpfunc_set_objective_switch(unsigned int* stack)
{
    nerpsruntimeGlobs.objectiveSwitch = stack[0];
    return stack[0];
}

// FUNCTION: LEGORR 0x004567d0
int nerpfunc_get_objective_switch(unsigned int* stack)
{
    if (nerpsruntimeGlobs.objectiveSwitch) {
        nerpsruntimeGlobs.objectiveSwitch = 0;
        return 1;
    }
    return 0;
}

// FUNCTION: LEGORR 0x00456810
void nerps_play_message_sound(void)
{
    if (nerpsCurrentSound != -1) s_play_seam(2, 0, nerpsCurrentSound, 0, NULL);
}

// FUNCTION: LEGORR 0x00456830
void nerps_parse_message(char* text, char* output, int setTimer)
{
    unsigned char i;
    char* out;
    unsigned char imageCount;
    float time;
    struct Image* images[20] = { NULL };
    char name[256];
    char* p;
    unsigned char found;

    imageCount = 0;
    time = nerpsMessageTime.defaultTime;
    out = output;
    if (nerpsGlobs.imageCount == 0 && nerpsGlobs.soundCount == 0 && output != NULL) {
        strcpy(output, text);
    } else {
        while (*text != '\0') {
            if (*text == '<') {
                text++;
                found = 0;
                p = name;
                while (*text != '\0' && !found) {
                    if (*text == '>') {
                        *p++ = '\0';
                        text++;
                        for (i = 0; i < nerpsGlobs.imageCount; i++) {
                            if (!_strcmpi(nerpsGlobs.imageList[i].key, name)) {
                                images[imageCount++] = nerpsGlobs.imageList[i].image;
                                found = 1;
                                break;
                            }
                        }
                    } else {
                        *p++ = *text++;
                    }
                }
            } else if (*text == '#') {
                text++;
                found = 0;
                p = name;
                while (*text != '\0' && !found) {
                    if (*text == '#') {
                        *p++ = '\0';
                        text++;
                        for (i = 0; i < nerpsGlobs.soundCount; i++) {
                            if (!_strcmpi(nerpsGlobs.soundList[i].key, name)) {
                                int sound = nerpsGlobs.soundList[i].sound;
                                if (nerpsGlobs.currentSound != i) {
                                    nerpsGlobs.currentSound = i;
                                    time = sound_streamed_wave_buffer_step_rate(sound) * nerpsMessageTime.scale + nerpsMessageTime.add;
                                    if (sound_subsystem_enabled()) {
                                        sound_deferred_play_expire();
                                        sound_deferred_timer_set_if_idle(sound_streamed_wave_buffer_step_rate(sound) * 25.0f);
                                        s_play_seam(2, 0, sound, 0, NULL);
                                        nerpsCurrentSound = sound;
                                        if (!nerpfunc_get_tutorial_flags(NULL)) {
                                            advisor_request_admit(0x14, 1);
                                            nerpsGlobs.messageSoundPlaying = 1;
                                        }
                                    }
                                }
                                break;
                            }
                        }
                        found = 1;
                    } else {
                        *p++ = *text++;
                    }
                }
            } else {
                if (output != NULL) *out++ = *text;
                text++;
            }
        }
        if (output != NULL) *out = '\0';
        if (imageCount) {
            int k;
            for (k = 0; k < imageCount; k++) {
                Point2F pos;
                pos.x = (float)(nerpsIconSpace * k) + (nerpsIconX - (float)(nerpsIconWidth * imageCount));
                pos.y = nerpsIconY;
                image_display_scaled(images[k], NULL, &pos, NULL);
            }
        }
    }
    if (setTimer) nerpsruntimeGlobs.messageTimer = time;
}

// FUNCTION: LEGORR 0x00456b80
void nerps_init_block_pointers_table(NERPs_Level* level)
{
    unsigned int x, y;

    memset(level->blockPointers, 0, sizeof(level->blockPointers));
    for (x = 0; x < level->width; x++) {
        for (y = 0; y < level->height; y++) {
            if (level->blocks[y * level->width + x].blockPointer) {
                NERPs_BlockPointer* blockPointer = (NERPs_BlockPointer*)malloc(sizeof(NERPs_BlockPointer));
                unsigned int index = level->blocks[y * level->width + x].blockPointer;
                blockPointer->x = x;
                blockPointer->y = y;
                blockPointer->next = level->blockPointers[index];
                level->blockPointers[index] = blockPointer;
                level->blocks[y * level->width + x].blockPointer = 0;
            }
        }
    }
}

// FUNCTION: LEGORR 0x00456c30
NERPs_BlockPointer* nerps_get_block_pointer(NERPs_Level* level, int index)
{
    if (index >= 0 && index < 0x37 && level->blockPointers[index] != NULL)
        return level->blockPointers[index];
    return NULL;
}

// FUNCTION: LEGORR 0x00456c60
void nerps_free_block_pointers(void)
{
    NERPs_Level* level = NERPS_LEVEL();
    int i;

    for (i = 0; i < 0x37; i++) {
        NERPs_BlockPointer* blockPointer = nerps_get_block_pointer(level, i);
        while (blockPointer != NULL) {
            NERPs_BlockPointer* next = blockPointer->next;
            free(blockPointer);
            blockPointer = next;
        }
        level->blockPointers[i] = NULL;
    }
}

// FUNCTION: LEGORR 0x00456cb0
void nerps_block_pointer_enumerate(int index, NERPsBlockPointerCallback callback, void* data)
{
    NERPs_Level* level = NERPS_LEVEL();
    NERPs_BlockPointer* blockPointer = nerps_get_block_pointer(level, index);

    while (blockPointer != NULL && callback(blockPointer, blockPointer->x, blockPointer->y, data))
        blockPointer = blockPointer->next;
}

/* NERPs_TutorialActionCallback: the tutorial rows' block-pointer callback
 * (00456d00, through 004a676c). It acts on one tutorial block for the
 * search's action and always continues the walk. */
// FUNCTION: LEGORR 0x00456d00
int nerps_tutorial_action_callback(NERPs_BlockPointer* blockPointer,
                                   unsigned int x, unsigned int y,
                                   void* data)
{
    NERPs_TutorialSearch* search = (NERPs_TutorialSearch*)data;
    NERPs_Level* level = NERPS_LEVEL();
    NERPs_Point2I blockPos;
    NERPs_Point2I cameraBlockPos;
    NERPs_Vector3F cameraPos;
    NERPs_Object* object;

    blockPos.x = x;
    blockPos.y = y;
    switch (search->action) {
    case 1: /* GetTutorialBlockIsGround */
        search->count += level->blocks[y * level->width + x].flags1 & 8;
        return 1;
    case 2: /* SetTutorialBlockIsGround: Level_DestroyWall */
        lego_terrain_cell_mutate((struct Lego_Level*)level_current_owner_get(), x, y, 0);
        return 1;
    case 3: /* GetTutorialBlockIsPath */
        search->count += level_cell_occupied_at_current(&blockPos.x);
        return 1;
    case 4: /* SetTutorialBlockIsPath */
        level->blocks[y * level->width + x].flags1 &= ~3;
        terrain_cell_state_update_current(&blockPos.x);
        terrain_cell_variant_flag_propagate_current(&blockPos.x);
        return 1;
    case 5: /* SetCameraGotoTutorial */
        lego_viewport_target_position_clamp(NULL, &blockPos.x, 1);
        return 1;
    case 6: /* GetCameraAtTutorial */
        ddiscene_object_position_get(*(void**)((char*)s_zoom_step_context + 0x2c),
                                     NULL, &cameraPos.x);
        terrain_map_world_to_block(level->map, cameraPos.x, cameraPos.y,
                                   &cameraBlockPos.x, &cameraBlockPos.y);
        if (blockPos.x == cameraBlockPos.x && blockPos.y == cameraBlockPos.y) {
            search->count = 1;
            return 1;
        }
        break;
    case 7: /* SetRockMonsterAtTutorial */
    {
        unsigned int type = active_engine_field_b0_get_current();
        object = (NERPs_Object*)legoobject_creature_spawn_from_emergence(
            (char*)legoGlobs_rockMonsterData + type * 0x74, 3, type,
            blockPos.x, blockPos.y);
        if (object != NULL) {
            *(float*)(object->stats + 0xe8) = 60.0f;
            return 1;
        }
        break;
    }
    case 8: /* SetCongregationAtTutorial(on) */
        aitask_coordinate_pair_task_record_queue(&blockPos.x);
        return 1;
    case 9: /* SetCongregationAtTutorial(off) */
        aitask_task_registry_remove_type0_entry_by_pair(&blockPos.x);
        return 1;
    case 10: /* GetUnitAtBlock */
    {
        NERPs_TutorialSearch objectSearch;

            memset(&objectSearch, 0, sizeof(objectSearch));
            objectSearch.x = x;
            objectSearch.y = y;
            legoobject_run_through_lists_skip_upgrade_parts(
                (int (*)(struct Lego_Object*, void*))nerps_object_at_tutorial_block_callback,
                &objectSearch);
            search->count += objectSearch.count;
            return 1;
    }
    case 11: /* SetTutorialPointer: the icon object's flag */
    {
        NERPs_TutorialSearch objectSearch;

            memset(&objectSearch, 0, sizeof(objectSearch));
            objectSearch.x = x;
            objectSearch.y = y;
            objectSearch.f_1c = 1;
            legoobject_run_through_lists_skip_upgrade_parts(
                (int (*)(struct Lego_Object*, void*))nerps_tutorial_icon_object_callback,
                &objectSearch);
            return 1;
    }
    case 12: /* MakeSomeoneOnThisBlockPickUpSomethingOnThisBlock */
    {
        NERPs_TutorialSearch objectSearch;

            memset(&objectSearch, 0, sizeof(objectSearch));
            objectSearch.x = x;
            objectSearch.y = y;
            objectSearch.checkType = 1;
            objectSearch.objType = 2;
            legoobject_run_through_lists_skip_upgrade_parts(
                (int (*)(struct Lego_Object*, void*))nerps_object_at_tutorial_block_callback,
                &objectSearch);
            if (objectSearch.count != 0) {
                object = objectSearch.found;
                objectSearch.objType = 6;
                legoobject_run_through_lists_skip_upgrade_parts(
                    (int (*)(struct Lego_Object*, void*))nerps_object_at_tutorial_block_callback,
                    &objectSearch);
                if (objectSearch.count != 0) {
                    legoobject_gated_step_command_issue((struct Lego_Object*)object,
                        (struct Lego_Object*)objectSearch.found);
                    return 1;
                }
            }
            break;
    }
    case 13: /* GetMonsterAtTutorial */
    {
        NERPs_TutorialSearch monsterSearch;

        memset(&monsterSearch, 0, sizeof(monsterSearch));
        monsterSearch.x = x;
        monsterSearch.y = y;
        legoobject_run_through_lists_skip_upgrade_parts(
            (int (*)(struct Lego_Object*, void*))nerps_object_at_tutorial_block_callback,
            &monsterSearch);
        search->count += monsterSearch.count;
        break;
    }
    }
    return 1;
}

/* The object search: count the objects on the search's block, or, when it
 * checks the type, stop at the first of the search's type. */
// FUNCTION: LEGORR 0x00457060
int nerps_object_at_tutorial_block_callback(NERPs_Object* obj, void* data)
{
    NERPs_Point2I blockPos;

    if (obj != NULL) {
        entity_position_to_terrain_cell_adapter((struct EntitySlot*)obj, &blockPos.x, &blockPos.y);
        if (blockPos.x == ((NERPs_TutorialSearch*)data)->x && blockPos.y == ((NERPs_TutorialSearch*)data)->y) {
            if (((NERPs_TutorialSearch*)data)->checkType && obj->type == ((NERPs_TutorialSearch*)data)->objType) {
                ((NERPs_TutorialSearch*)data)->found = obj;
                ((NERPs_TutorialSearch*)data)->count = 1;
                return 1;
            }
            ((NERPs_TutorialSearch*)data)->count++;
        }
    }
    return 0;
}

/* Flag the object on the search's block as the tutorial icon's, clear the
 * flag on the others. */
// FUNCTION: LEGORR 0x004570d0
int nerps_tutorial_icon_object_callback(NERPs_Object* obj, void* data)
{
    NERPs_Point2I blockPos;

    if (obj != NULL) {
        entity_position_to_terrain_cell_adapter((struct EntitySlot*)obj, &blockPos.x, &blockPos.y);
        if (blockPos.x == ((NERPs_TutorialSearch*)data)->x && blockPos.y == ((NERPs_TutorialSearch*)data)->y) {
            obj->tutorialIcon = 1;
            return 0;
        }
        obj->tutorialIcon = 0;
    }
    return 0;
}

// FUNCTION: LEGORR 0x00457130
int nerpfunc_make_someone_on_this_block_pick_up_something_on_this_block(unsigned int* stack)
{
    NERPs_TutorialSearch search;

    search.count = 0;
    search.index = stack[0];
    search.action = 0xc;
    nerps_block_pointer_enumerate(stack[0], nerpsTutorialActionCallback, &search);
    return search.count;
}

/* NERPs_SetTutorialPointer: mark the list's blocks with the pointer (or
 * clear them), run action 11 over them, then flag the tutorial icon's
 * object if it stands on one. */
// FUNCTION: LEGORR 0x00457170
void nerps_set_tutorial_pointer(unsigned int index, int on)
{
    NERPs_Level* level = NERPS_LEVEL();
    NERPs_BlockPointer* blockPointer = nerps_get_block_pointer(level, index);
    NERPs_Point2I blockPos;
    NERPs_TutorialSearch search;

    while (blockPointer != NULL) {
        if (on == 1) {
            level->blocks[level->width * blockPointer->y + blockPointer->x].blockPointer =
                (unsigned char)index;
            search.index = index;
            search.count = 0;
            search.action = 0xb;
            nerps_block_pointer_enumerate(index, nerpsTutorialActionCallback, &search);
        }
        if (on == 0)
            level->blocks[level->width * blockPointer->y + blockPointer->x].blockPointer = 0;
        blockPointer = blockPointer->next;
    }

    if (nerpsGlobs.tutorialIconObject != 0) {
        void* object;

        if (lego_object_pointer_registry_get_slot(nerpsGlobs.tutorialIconObject - 1,
                                                  &object)) {
            entity_position_to_terrain_cell_adapter((struct EntitySlot*)object,
                                                    &blockPos.x, &blockPos.y);
            search.x = blockPos.x;
            search.y = blockPos.y;
            nerps_tutorial_icon_object_callback((NERPs_Object*)object, &search);
        }
    }
}

// FUNCTION: LEGORR 0x00457260
int nerpfunc_set_congregation_at_tutorial(unsigned int* stack)
{
    NERPs_TutorialSearch search;

    search.count = 0;
    search.index = stack[0];
    search.action = stack[1] ? 8 : 9;
    nerps_block_pointer_enumerate(stack[0], nerpsTutorialActionCallback, &search);
    return search.count;
}

// FUNCTION: LEGORR 0x004572a0
int nerpfunc_set_rock_monster_at_tutorial(unsigned int* stack)
{
    NERPs_TutorialSearch search;

    search.action = 7;
    search.count = 0;
    search.index = stack[0];
    nerps_block_pointer_enumerate(stack[0], nerpsTutorialActionCallback, &search);
    return search.count;
}

// FUNCTION: LEGORR 0x004572e0
int nerpfunc_set_camera_goto_tutorial(unsigned int* stack)
{
    NERPs_TutorialSearch search;

    search.action = 5;
    search.count = 0;
    search.index = stack[0];
    nerps_block_pointer_enumerate(stack[0], nerpsTutorialActionCallback, &search);
    return search.count;
}

// FUNCTION: LEGORR 0x00457320
int nerpfunc_get_camera_at_tutorial(unsigned int* stack)
{
    NERPs_TutorialSearch search;

    search.action = 6;
    search.count = 0;
    search.index = stack[0];
    nerps_block_pointer_enumerate(stack[0], nerpsTutorialActionCallback, &search);
    return search.count;
}

// FUNCTION: LEGORR 0x00457360
int nerpfunc_get_tutorial_block_is_ground(unsigned int* stack)
{
    NERPs_TutorialSearch search;

    search.action = 1;
    search.count = 0;
    search.index = stack[0];
    nerps_block_pointer_enumerate(stack[0], nerpsTutorialActionCallback, &search);
    return search.count;
}

// FUNCTION: LEGORR 0x004573a0
int nerpfunc_get_tutorial_block_is_path(unsigned int* stack)
{
    NERPs_TutorialSearch search;

    search.action = 3;
    search.count = 0;
    search.index = stack[0];
    nerps_block_pointer_enumerate(stack[0], nerpsTutorialActionCallback, &search);
    return search.count;
}

// FUNCTION: LEGORR 0x004573e0
int nerpfunc_set_tutorial_block_is_ground(unsigned int* stack)
{
    NERPs_TutorialSearch search;

    search.action = 2;
    search.count = 0;
    search.index = stack[0];
    nerps_block_pointer_enumerate(stack[0], nerpsTutorialActionCallback, &search);
    return search.count;
}

// FUNCTION: LEGORR 0x00457420
int nerpfunc_set_tutorial_block_is_path(unsigned int* stack)
{
    NERPs_TutorialSearch search;

    search.action = 4;
    search.count = 0;
    search.index = stack[0];
    nerps_block_pointer_enumerate(stack[0], nerpsTutorialActionCallback, &search);
    return search.count;
}

// FUNCTION: LEGORR 0x00457460
int nerpfunc_get_unit_at_block(unsigned int* stack)
{
    NERPs_TutorialSearch search;

    search.action = 0xa;
    search.count = 0;
    search.index = stack[0];
    nerps_block_pointer_enumerate(stack[0], nerpsTutorialActionCallback, &search);
    return search.count;
}

// FUNCTION: LEGORR 0x004574a0
int nerpfunc_get_monster_at_tutorial(unsigned int* stack)
{
    NERPs_TutorialSearch search;

    search.action = 0xd;
    search.count = 0;
    search.index = stack[0];
    nerps_block_pointer_enumerate(stack[0], nerpsTutorialActionCallback, &search);
    return search.count;
}

#ifdef __cplusplus
}
#endif
