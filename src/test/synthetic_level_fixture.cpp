// SPDX-License-Identifier: LGPL-3.0-or-later
#include "test/synthetic_level_fixture.h"
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

#include <SDL3/SDL.h>

#include "gods98/container_link_names.h"
#include "gods98/config.h"
#include "gods98/container_unit.h"
#include "gods98/directdraw.h"
#include "gods98/files.h"
#include "gods98/main_globs.h"
#include "gods98/viewports_link_names.h"
#include "gods98/viewports.h"
#include "game/camera_control.h"
#include "game/lego_level_record.h"
#include "game/legoobject_record.h"
#include "game/legoobject_unit.h"
#include "game/terrain_map.h"
#include "game/lego_unit.h"
#include "platform/native/test_fixture.h"
#include "test/lrr_test.h"
/* Last: its macros name fields the other headers declare. */
#include "game/lego_globs.h"

extern "C" int main_init_app(HINSTANCE instance);
/* Lego.c's own setup steps, under the port's names (lego_unit_imports.h). */
extern "C" void* camera_create(void* parent, int type);
extern "C" int lego_scene_lights_build_from_config(void);
extern "C" int lego_render_quality_config_apply(void);
struct TextureSurface;
/* Lego_Initialise's table set-up, in its order (lego_unit.c). */
extern "C" void lego_game_speed_set(float speed);
extern "C" void keyboard_key_names_init(void);
extern "C" void legoobject_initialise(void);
extern "C" void renderer_record_table_initialize(void);
extern "C" void screen_coord_normalize_init(void);
extern "C" void ui_panel_name_tables_init(void);
extern "C" void aitask_cmd_registry_init_target(void);
extern "C" void message_string_table_initialize(void);
extern "C" void scene_effect_registry_reset(void);
extern "C" void ui_pointer_cursor_name_registry_init(void);
extern "C" void bubble_action_strings_initialize(void);
extern "C" void sound_alias_registry_init(void);
extern "C" int mesh_initialise(const char* sharedTextureDir);
struct BlockRec;
extern "C" void lego_panels_config_entry_apply(BlockRec* config, unsigned int width, unsigned int height);
extern "C" int stats_initialise(BlockRec* config, const char* gameName);
extern "C" void advisor_scene_controller_initialize(const char* gameName, void* camera, void* viewMain);
extern "C" void advisor_scene_visual_state_initialize(float distance);
extern "C" void* select_place_create_adapter(void* root, float size);
extern "C" void surface_visibility_set(const void* record, uint32_t flag);
extern "C" void* font_atlas_record_create(const char* name);
extern "C" void* text_buffer_create(uintptr_t font, const uint32_t* area, long size);
extern "C" void ui_message_strings_initialize(void* textOnlyWindow, void* textImageWindow, unsigned int imageX,
                                              unsigned int imageY, float pauseTime);
struct Area2F { float x, y, width, height; };
extern "C" void text_message_panel_initialise_adapter(const char* imageFile, unsigned int x, unsigned int y,
                                                      unsigned int openY, Area2F* messageWindow,
                                                      Area2F* imageWindow, float timer);
extern "C" void loader_initialise(const char* loadScreenName, const char* shutdownScreenName,
                                  TextureSurface* font, const char* profileName, int progressDirection,
                                  const char* progressBarName, Area2F* progressWindow,
                                  const char* loadingText);

namespace synthetic_fixture {
/* Names each step on stderr, unbuffered, so a crash's log shows where. */
static void step(const char* name)
{
    std::fprintf(stderr, "level_boot: %s\n", name);
    std::fflush(stderr);
}

static void put_le(unsigned char* dest, uint32_t value, unsigned bytes)
{
    for (unsigned i = 0; i < bytes; ++i) dest[i] = (unsigned char)(value >> (8 * i));
}

/* A map file: "MAP ", the file size, width, height, then a uint16 per
 * block, row by row. */
bool write_map(const char* path, unsigned width, unsigned height, unsigned (*value)(unsigned x, unsigned y))
{
    FILE* f = std::fopen(path, "wb");
    if (!f) return false;
    unsigned char header[16] = { 'M', 'A', 'P', ' ' };
    put_le(header + 4, 16 + width * height * 2, 4);
    put_le(header + 8, width, 4);
    put_le(header + 12, height, 4);
    bool ok = std::fwrite(header, 1, sizeof(header), f) == sizeof(header);
    for (unsigned y = 0; y < height; ++y) {
        for (unsigned x = 0; x < width; ++x) {
            unsigned char v[2];
            put_le(v, value(x, y), 2);
            ok = std::fwrite(v, 1, sizeof(v), f) == sizeof(v) && ok;
        }
    }
    return std::fclose(f) == 0 && ok;
}

bool write_text(const char* path, const char* text)
{
    FILE* f = std::fopen(path, "wb");
    if (!f) return false;
    bool ok = std::fputs(text, f) >= 0;
    return std::fclose(f) == 0 && ok;
}

/* An 8-bit palettised BMP of one colour, the format the game's images and
 * textures use. */
bool write_bmp(const char* path, unsigned width, unsigned height)
{
    const unsigned stride = (width + 3) & ~3u;
    const uint32_t offset = 14 + 40 + 256 * 4;
    const uint32_t size = offset + stride * height;
    unsigned char header[54] = { 'B', 'M' };
    auto put32 = [&](int at, uint32_t v) { put_le(header + at, v, 4); };
    auto put16 = [&](int at, uint16_t v) { put_le(header + at, v, 2); };
    put32(2, size);
    put32(10, offset);
    put32(14, 40);
    put32(18, width);
    put32(22, height);
    put16(26, 1);
    put16(28, 8);
    put32(34, stride * height);
    put32(46, 256);
    FILE* f = std::fopen(path, "wb");
    if (!f) return false;
    bool ok = std::fwrite(header, sizeof(header), 1, f) == 1;
    for (unsigned i = 0; i < 256; ++i) {
        unsigned char entry[4] = { (unsigned char)i, (unsigned char)i, (unsigned char)i, 0 };
        ok = std::fwrite(entry, sizeof(entry), 1, f) == 1 && ok;
    }
    std::string row(stride, '\x80');
    for (unsigned y = 0; y < height; ++y) ok = std::fwrite(row.data(), 1, row.size(), f) == row.size() && ok;
    return std::fclose(f) == 0 && ok;
}

/* An LWOB triangle (as test_lwo.cpp's), for the ore and crystal models. */
bool write_triangle(const char* path)
{
    static const unsigned char object[] = {
        'F', 'O', 'R', 'M', 0, 0, 0, 134, 'L', 'W', 'O', 'B',
        'P', 'N', 'T', 'S', 0, 0, 0, 36,
        0, 0, 0, 0,        0, 0, 0, 0,        0, 0, 0, 0,
        0x3f, 0x80, 0, 0,  0, 0, 0, 0,        0, 0, 0, 0,
        0, 0, 0, 0,        0xc0, 0, 0, 0,     0x3f, 0, 0, 0,
        'P', 'O', 'L', 'S', 0, 0, 0, 10,
        0, 3, 0, 0, 0, 2, 0, 1, 0, 2,
        'S', 'R', 'F', 'S', 0, 0, 0, 12,
        'S', 't', 'o', 'n', 'e', 0, 'M', 'e', 't', 'a', 'l', 0,
        'S', 'U', 'R', 'F', 0, 0, 0, 16,
        'M', 'e', 't', 'a', 'l', 0, 'C', 'O', 'L', 'R', 0, 4, 10, 20, 30, 0,
        'S', 'U', 'R', 'F', 0, 0, 0, 16,
        'S', 't', 'o', 'n', 'e', 0, 'C', 'O', 'L', 'R', 0, 4, 90, 80, 70, 0,
    };
    FILE* f = std::fopen(path, "wb");
    if (!f) return false;
    bool ok = std::fwrite(object, sizeof(object), 1, f) == 1;
    return std::fclose(f) == 0 && ok;
}

static bool border(unsigned x, unsigned y) { return x == 0 || y == 0 || x == W - 1 || y == H - 1; }
static unsigned height_of(unsigned, unsigned) { return 0; }
/* The walls x = 3 and 4 run the height of the level, splitting it into
 * two caves. The map's last row and column are outside the playable
 * blocks. */
static bool wall(unsigned x) { return x == 3 || x == 4; }
/* Surface types: immovable rock (1) round the edge, loose rock (4) for the
 * wall, and floor (0) in the caves. */
static unsigned terrain_of(unsigned x, unsigned y) { return border(x, y) ? 1 : wall(x) ? 4 : 0; }
/* Pre-dug: the caves are open (1), the rest is wall (0). */
static unsigned predug_of(unsigned x, unsigned y) { return border(x, y) || wall(x) ? 0 : 1; }

/* The least of Lego.cfg that the steps below read. Every panel needs an
 * image; a level needs a PTL file holding a block and an EmergeCreature
 * (Lego_LoadLevel looks the name up without a NULL check). */
static const char CONFIG[] =
    "Lego* {\n"
    "  Main {\n"
    "    fpclipblocks 6 quality Gouraud\n"
    "    topspotrgb 255:255:255 trackspotrgb 255:255:255 ambientrgb 100:100:100 fplightrgb 255:255:255\n"
    "  }\n"
    "  Panels640x480 {\n"
    "    Panel_Radar L\\panel.bmp,0,0,0,0\n"
    "    Panel_RadarFill L\\panel.bmp,0,0,0,0\n"
    "    Panel_RadarOverlay L\\panel.bmp,0,0,0,0\n"
    "    Panel_Messages L\\panel.bmp,0,0,0,0\n"
    "    Panel_MessagesSide L\\panel.bmp,0,0,0,0\n"
    "    Panel_CrystalSideBar L\\panel.bmp,0,0,0,0\n"
    "    Panel_TopPanel L\\panel.bmp,0,0,0,0\n"
    "    Panel_Information L\\panel.bmp,0,0,0,0\n"
    "    Panel_PriorityList L\\panel.bmp,0,0,0,0\n"
    "    Panel_CameraControl L\\panel.bmp,0,0,0,0\n"
    "    Panel_InfoDock L\\panel.bmp,0,0,0,0\n"
    "    Panel_Encyclopedia L\\panel.bmp,0,0,0,0\n"
    "  }\n"
    "  Stats {\n"
    "    Ore { Levels 1 CollRadius 5 PickSphere 10 }\n"
    "    PowerCrystal { Levels 1 CollRadius 5 PickSphere 10 }\n"
    "  }\n"
    "  TS { surftextwidth 8 surftextheight 8 meshbasename L\\m texturebasename L\\t rooftexture L\\roof.bmp }\n"
    "  Level01 {\n"
    "    blocksize 40 roughlevel 0 textureset TS\n"
    "    surfacemap L\\surf.map predugmap L\\dug.map terrainmap L\\terr.map\n"
    "    PTLFile L\\empty.ptl olistfile L\\objects.ol EmergeCreature TVCamera\n"
    "  }\n"
    "}\n";

/* Object list positions count blocks from 1: 3.5 is block 2's centre. */
static const char OBJECTS[] =
    "Lego* {\n"
    "  Object1 { type TVCamera xPos 4.5 yPos 4.5 heading 0 }\n"
    "  Object2 { type Ore xPos 3.5 yPos 3.5 heading 0 }\n"
    "  Object3 { type PowerCrystal xPos 6.5 yPos 5.5 heading 90 }\n"
    "}\n";

bool make_fixture(const char* directory)
{
    return fixture_enter(directory) && fixture_mkdir("Data") && fixture_mkdir("Data/L") &&
           write_map("Data/L/surf.map", W, H, height_of) && write_map("Data/L/dug.map", W, H, predug_of) &&
           write_map("Data/L/terr.map", W, H, terrain_of) && write_text("Data/L/empty.ptl", "Lego* {\n}\n") &&
           write_text("Data/L/objects.ol", OBJECTS) && write_bmp("Data/L/roof.bmp", 8, 8) &&
           write_bmp("Data/L/load.bmp", 32, 32) && write_bmp("Data/L/font.bmp", 80, 190) &&
           write_bmp("Data/L/panel.bmp", 16, 16) && write_triangle("Data/L/ore.lwo");
}

/* Lego_Initialise's steps that the level loader and the main loop rely
 * on, in its order (lego_unit.c, lego_game_config_init). */
bool boot_game()
{
    step("config");
    block_text_token_pool_init();
    legoGlobs.config = block_parse(CONFIG, sizeof(CONFIG) - 1);
    legoGlobs.gameName = "Lego";
    if (!CHECK(legoGlobs.config != nullptr, "parse config")) return false;
    legoGlobs_rootCont = Container_Initialise("Lego");
    lego_game_speed_set(1.0f);
    keyboard_key_names_init();
    viewport_initialise();
    legoobject_initialise();
    renderer_record_table_initialize();
    screen_coord_normalize_init();
    ui_panel_name_tables_init();
    aitask_cmd_registry_init_target();
    message_string_table_initialize();
    scene_effect_registry_reset();
    ui_pointer_cursor_name_registry_init();
    bubble_action_strings_initialize();

    step("message windows");
    /* The message windows, in one font. */
    void* font = font_atlas_record_create("L\\font.bmp");
    if (!CHECK(font != nullptr, "load the font")) return false;
    Area2F textArea = { 55.0f, 435.0f, 325.0f, 42.0f };
    legoGlobs_textOnlyWindow = text_buffer_create((uintptr_t)font, (const uint32_t*)&textArea, 1024);
    legoGlobs_textImageWindow = text_buffer_create((uintptr_t)font, (const uint32_t*)&textArea, 1024);
    ui_message_strings_initialize(legoGlobs_textOnlyWindow, legoGlobs_textImageWindow, 316, 425, 0.0f);
    Area2F panelImageArea = { 55.0f, 363.0f, 325.0f, 140.0f };
    text_message_panel_initialise_adapter("L\\load.bmp", 42, 409, 338, &textArea, &panelImageArea, 20.0f);

    step("cameras, viewports and lights");
    /* The cameras, viewports and lights. */
    s_zoom_step_context = (LegoCamera*)camera_create(legoGlobs_rootCont, 1);
    legoGlobs_cameraFP = (LegoCamera*)camera_create(legoGlobs_rootCont, 3);
    legoGlobs_cameraTrack = (LegoCamera*)camera_create(legoGlobs_rootCont, 2);
    if (!CHECK(s_zoom_step_context && legoGlobs_cameraFP && legoGlobs_cameraTrack, "create the cameras"))
        return false;
    legoGlobs_viewMain = viewport_create(0.0f, 0.0f, 1.0f, 1.0f, (Container*)s_zoom_step_context->contCam);
    legoGlobs_viewTrack = viewport_create_pixel(16, 13, 151, 151, (Container*)legoGlobs_cameraTrack->contCam);
    if (!CHECK(legoGlobs_viewMain && legoGlobs_viewTrack, "create the viewports")) return false;
    if (!CHECK(lego_scene_lights_build_from_config(), "build the lights")) return false;
    if (!CHECK(lego_render_quality_config_apply(), "apply the render quality")) return false;
    step("sounds, meshes and placement grid");
    sound_alias_registry_init();
    mesh_initialise(nullptr);
    /* The building placement grid, hidden, and the top-down view. */
    s_refresh_record = select_place_create_adapter(legoGlobs_rootCont, 5.0f);
    surface_visibility_set(s_refresh_record, 1);
    s_context_mode = 1;
    step("ore and crystal models");
    /* Lego_LoadMiscObjects' ore and crystal models. */
    legoGlobs_contOresTable = Container_Load((Container*)legoGlobs_rootCont, "L\\ore", "LWO", 1);
    legoGlobs_contCrystal = Container_Load((Container*)legoGlobs_rootCont, "L\\ore", "LWO", 1);
    if (!CHECK(legoGlobs_contOresTable && legoGlobs_contCrystal, "load the ore and crystal models")) return false;
    Container_Hide((Container*)legoGlobs_contOresTable, 1);
    Container_Hide((Container*)legoGlobs_contCrystal, 1);
    step("stats, panels, advisor and loader");
    if (!CHECK(stats_initialise((BlockRec*)legoGlobs.config, legoGlobs.gameName), "load the stats")) return false;
    lego_panels_config_entry_apply((BlockRec*)legoGlobs.config, 640, 480);
    advisor_scene_controller_initialize(legoGlobs.gameName, s_zoom_step_context, legoGlobs_viewMain);
    advisor_scene_visual_state_initialize(200.0f);
    Area2F bar = { 0.0f, 0.0f, 16.0f, 4.0f };
    loader_initialise("L\\load.bmp", "L\\load.bmp", nullptr, "L\\profile.txt", 1, "L\\load.bmp", &bar,
                      nullptr);
    return true;
}

bool initialise_headless(const char* programName)
{
    file_initialise(programName, 0, "");
    if (!CHECK(SDL_Init(SDL_INIT_VIDEO), SDL_GetError())) return false;
    std::strcpy(mainGlobs.programName, programName);
    mainGlobs.className = mainGlobs.programName;
    if (CHECK(main_init_app(nullptr), "create dummy-driver window")) {
        directdraw_initialise(mainGlobs.hWnd);
        DirectDraw_Driver drivers[16] = {};
        unsigned int driverCount = 0;
        DirectDraw_Device devices[32] = {};
        unsigned int count = 0;
        const DirectDraw_Device* software = nullptr;
        if (CHECK(directdraw_enum_drivers(drivers, &driverCount) && driverCount > 0, "enumerate drivers") &&
            CHECK(directdraw_enum_devices(&drivers[0], devices, &count), "enumerate devices")) {
            for (unsigned int i = 0; i < count; ++i) {
                if (std::strstr(devices[i].desc, "Miniwin Emulation") != nullptr) software = &devices[i];
            }
        }
        if (CHECK(software != nullptr, "find software device") &&
            CHECK(directdraw_setup(FALSE, nullptr, software, nullptr, 0, 0, 320, 200),
                  "set up software device")) {
            return true;
        }
    }
    return false;
}
}
