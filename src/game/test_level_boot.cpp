// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_level_boot — a game session on a synthetic level: Lego.c's level
 * loader (Lego_LoadLevel, 0x00429890) reads an 8x8 level, and its main loop
 * (Lego_MainLoop, 0x004232e0) runs 250 frames on a headless software
 * device, simulation and render.
 *
 * Lego_Initialise cannot run here: it gives up unless every object type
 * loads a real model. So boot_game() performs the parts of it that the
 * loader and the main loop rely on, in its order, with a config, maps,
 * images and one triangle model that the test writes. The level has two
 * caves split by a two-block dirt wall, an ore and a power crystal.
 *
 * Establishes: the maps load into blocks (rock, floor, wall); the object
 * list creates its objects at their blocks; destroying half the wall
 * collapses the unsupported other half and joins the caves; the main loop
 * runs 250 frames and keeps the level, the objects and the open blocks. */
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

static const unsigned W = 8, H = 8;

/* A map file: "MAP ", the file size, width, height, then a uint16 per
 * block, row by row. */
static bool write_map(const char* path, unsigned (*value)(unsigned x, unsigned y))
{
    FILE* f = std::fopen(path, "wb");
    if (!f) return false;
    uint32_t header[4];
    std::memcpy(&header[0], "MAP ", 4);
    header[1] = (uint32_t)(sizeof(header) + W * H * 2);
    header[2] = W;
    header[3] = H;
    std::fwrite(header, sizeof(header), 1, f);
    for (unsigned y = 0; y < H; ++y) {
        for (unsigned x = 0; x < W; ++x) {
            uint16_t v = (uint16_t)value(x, y);
            std::fwrite(&v, sizeof(v), 1, f);
        }
    }
    return std::fclose(f) == 0;
}

static bool write_text(const char* path, const char* text)
{
    FILE* f = std::fopen(path, "wb");
    if (!f) return false;
    std::fputs(text, f);
    return std::fclose(f) == 0;
}

/* An 8-bit palettised BMP of one colour, the format the game's images and
 * textures use. */
static bool write_bmp(const char* path, unsigned width, unsigned height)
{
    const unsigned stride = (width + 3) & ~3u;
    const uint32_t offset = 14 + 40 + 256 * 4;
    const uint32_t size = offset + stride * height;
    unsigned char header[54] = { 'B', 'M' };
    auto put32 = [&](int at, uint32_t v) { std::memcpy(header + at, &v, 4); };
    auto put16 = [&](int at, uint16_t v) { std::memcpy(header + at, &v, 2); };
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
    std::fwrite(header, sizeof(header), 1, f);
    for (unsigned i = 0; i < 256; ++i) {
        unsigned char entry[4] = { (unsigned char)i, (unsigned char)i, (unsigned char)i, 0 };
        std::fwrite(entry, sizeof(entry), 1, f);
    }
    std::string row(stride, '\x80');
    for (unsigned y = 0; y < height; ++y) std::fwrite(row.data(), 1, row.size(), f);
    return std::fclose(f) == 0;
}

/* An LWOB triangle (as test_lwo.cpp's), for the ore and crystal models. */
static bool write_triangle(const char* path)
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
    std::fwrite(object, sizeof(object), 1, f);
    return std::fclose(f) == 0;
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

static bool make_fixture()
{
    return fixture_enter("level_boot_fixture") && fixture_mkdir("Data") && fixture_mkdir("Data/L") &&
           write_map("Data/L/surf.map", height_of) && write_map("Data/L/dug.map", predug_of) &&
           write_map("Data/L/terr.map", terrain_of) && write_text("Data/L/empty.ptl", "Lego* {\n}\n") &&
           write_text("Data/L/objects.ol", OBJECTS) && write_bmp("Data/L/roof.bmp", 8, 8) &&
           write_bmp("Data/L/load.bmp", 32, 32) && write_bmp("Data/L/font.bmp", 80, 190) &&
           write_bmp("Data/L/panel.bmp", 16, 16) && write_triangle("Data/L/ore.lwo");
}

static Lego_Block* block_at(Lego_Level* level, unsigned x, unsigned y)
{
    return &level->blocks[y * level->width + x];
}

/* The objects in the level, by LegoObject_Type, and where the ore is. */
struct Census {
    int ore = 0, crystals = 0;
    float oreX = 0.0f, oreY = 0.0f;
};

static int count_object(Lego_Object* obj, void* data)
{
    Census* census = static_cast<Census*>(data);
    if (obj->type == 7) {
        ++census->ore;
        legoobject_transform_position_get(obj, &census->oreX, &census->oreY);
    } else if (obj->type == 6) {
        ++census->crystals;
    }
    return 0;
}

static Census census()
{
    Census census;
    legoobject_run_through_lists(count_object, &census, 0);
    return census;
}

/* Lego_Initialise's steps that the level loader and the main loop rely
 * on, in its order (lego_unit.c, lego_game_config_init). */
static bool boot_game()
{
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

    /* The message windows, in one font. */
    void* font = font_atlas_record_create("L\\font.bmp");
    if (!CHECK(font != nullptr, "load the font")) return false;
    Area2F textArea = { 55.0f, 435.0f, 325.0f, 42.0f };
    legoGlobs_textOnlyWindow = text_buffer_create((uintptr_t)font, (const uint32_t*)&textArea, 1024);
    legoGlobs_textImageWindow = text_buffer_create((uintptr_t)font, (const uint32_t*)&textArea, 1024);
    ui_message_strings_initialize(legoGlobs_textOnlyWindow, legoGlobs_textImageWindow, 316, 425, 0.0f);
    Area2F panelImageArea = { 55.0f, 363.0f, 325.0f, 140.0f };
    text_message_panel_initialise_adapter("L\\load.bmp", 42, 409, 338, &textArea, &panelImageArea, 20.0f);

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
    sound_alias_registry_init();
    mesh_initialise(nullptr);
    /* The building placement grid, hidden, and the top-down view. */
    s_refresh_record = select_place_create_adapter(legoGlobs_rootCont, 5.0f);
    surface_visibility_set(s_refresh_record, 1);
    s_context_mode = 1;
    /* Lego_LoadMiscObjects' ore and crystal models. */
    legoGlobs_contOresTable = Container_Load((Container*)legoGlobs_rootCont, "L\\ore", "LWO", 1);
    legoGlobs_contCrystal = Container_Load((Container*)legoGlobs_rootCont, "L\\ore", "LWO", 1);
    if (!CHECK(legoGlobs_contOresTable && legoGlobs_contCrystal, "load the ore and crystal models")) return false;
    Container_Hide((Container*)legoGlobs_contOresTable, 1);
    Container_Hide((Container*)legoGlobs_contCrystal, 1);
    if (!CHECK(stats_initialise((BlockRec*)legoGlobs.config, legoGlobs.gameName), "load the stats")) return false;
    lego_panels_config_entry_apply((BlockRec*)legoGlobs.config, 640, 480);
    advisor_scene_controller_initialize(legoGlobs.gameName, s_zoom_step_context, legoGlobs_viewMain);
    advisor_scene_visual_state_initialize(200.0f);
    Area2F bar = { 0.0f, 0.0f, 16.0f, 4.0f };
    loader_initialise("L\\load.bmp", "L\\load.bmp", nullptr, "L\\profile.txt", 1, "L\\load.bmp", &bar,
                      nullptr);
    return true;
}

static void check_level()
{
    if (!boot_game()) return;
    int loaded = lego_level_load("Level01");
    CHECK(loaded, "load the level");
    if (!loaded) return;
    Lego_Level* level = (Lego_Level*)legoGlobs.currLevel;
    if (!CHECK(level != nullptr, "level is current")) return;
    CHECK_EQ(level->width, W);
    CHECK_EQ(level->height, H);

    /* The map as loaded: rock round the edge, two caves either side of a
     * two-block dirt wall at x = 3 and 4. Flag 0x8 marks a floor. */
    CHECK(!(block_at(level, 0, 3)->flags1 & 0x8), "the edge is rock");
    CHECK(block_at(level, 2, 3)->flags1 & 0x8, "the left cave is floor");
    CHECK(block_at(level, 5, 3)->flags1 & 0x8, "the right cave is floor");
    CHECK(!(block_at(level, 3, 3)->flags1 & 0x8), "the wall's left half stands");
    CHECK(!(block_at(level, 4, 3)->flags1 & 0x8), "the wall's right half stands");
    CHECK_EQ(block_at(level, 4, 3)->f_02, 4);

    /* The object list's ore and crystal, at their blocks' centres. */
    Census before = census();
    CHECK_EQ(before.ore, 1);
    CHECK_EQ(before.crystals, 1);
    int bx = -1, by = -1;
    CHECK(terrain_map_world_to_block(level->map, before.oreX, before.oreY, &bx, &by), "the ore is on the map");
    CHECK_EQ(bx, 2);
    CHECK_EQ(by, 2);

    /* Destroying one half of the wall leaves the other half one block
     * thick, which collapses too: the caves join. */
    CHECK_EQ(lego_terrain_cell_mutate(level, 4, 3, 0), 1);
    CHECK(block_at(level, 4, 3)->flags1 & 0x8, "the destroyed block is floor");
    CHECK(block_at(level, 3, 3)->flags1 & 0x8, "the unsupported half collapses");
    CHECK_EQ(lego_terrain_cell_mutate(level, 0, 3, 0), 0);

    /* The main loop: simulation, effects, interface and a software render
     * of each frame. */
    for (int frame = 0; frame < 250; ++frame) lego_engine_tick(1.0f);
    CHECK(legoGlobs.currLevel == level, "the level is still current after 250 frames");
    Census after = census();
    CHECK_EQ(after.ore, 1);
    CHECK_EQ(after.crystals, 1);
    CHECK_NEAR(after.oreX, before.oreX, 0.5f);
    CHECK_NEAR(after.oreY, before.oreY, 0.5f);
    CHECK(block_at(level, 3, 3)->flags1 & 0x8, "the joined caves stay open");
}

int main()
{
    if (!CHECK(make_fixture(), "write the fixture")) return lrr_test_result();
    file_initialise("level_boot_test", 0, "");
    if (!CHECK(SDL_Init(SDL_INIT_VIDEO), SDL_GetError())) return lrr_test_result();
    std::strcpy(mainGlobs.programName, "LevelBootTest");
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
            check_level();
        }
    }
    return lrr_test_result();
}
