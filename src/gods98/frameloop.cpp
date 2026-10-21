// SPDX-License-Identifier: LGPL-3.0-or-later
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "game/game.hpp"
#include "game/ai.hpp"
#include "game/cmd.hpp"
#include "game/entity.hpp"
#include "game/frontend.hpp"
#include "game/level.hpp"
#include "game/mission.hpp"
#include "game/objective.hpp"
#include "game/render.hpp"
#include "game/savegame.hpp"
#include "game/select.hpp"
#include "platform/platform.h"
#include "audio.hpp"
#include "engine.hpp"
#include "frameloop.hpp"
#include "input.hpp"
#include "resource.hpp"
#include "surface.hpp"
#include "texture.hpp"
#include "tickers.hpp"
#include "view.hpp"
#include "camera.hpp"
#include "message.hpp"

#define WINDOW_W 640
#define WINDOW_H 480
#define FRAME_MS 33 /* ~30 fps */

static const char* env_get(const char* name, const char* fallback)
{
    const char* v = getenv(name);
    return (v != NULL && v[0] != '\0') ? v : fallback;
}

/* Held-key probe for the executor's cheat-keybind pre-pass (finding 06):
 * reads the current frame's input tables. */
static const InputState* s_demo_input = NULL;
static int demo_keyheld(int key)
{
    return s_demo_input != NULL ? input_key_down(s_demo_input, key) : 0;
}

/* Message-bus demo hooks (finding 22 §1): incidents dispatch a per-id
 * sound configured from the InfoMessages config section (message.c), else
 * a stand-in wav. */
static AudioPlayer* s_demo_ap = NULL;
static void demo_msg_sound(int id, float vol)
{
    (void)vol;
    if (s_demo_ap == NULL || s_demo_ap->dev == NULL) {
        return;
    }
    const char* snd = message_sound(id);
    if (snd != NULL && snd[0] != '\0') {
        audio_play_named(s_demo_ap, snd);
    } else {
        audio_play_named(s_demo_ap, "Sounds\\Buildings\\elec5.wav");
    }
}
static int demo_msg_cb(int id)
{
    (void)id; /* the bus appends it to the active queue */
    return 0; /* unhandled: dedupe callback does not consume */
}

int LegorrrRun(void)
{
    const char* wad_dir = env_get("LEGORRR_WAD_DIR", ".");
    const char* autostop = env_get("LEGORRR_AUTOSTOP_FRAMES", "0");
    long frame_cap = strtol(autostop, NULL, 10);
    if (frame_cap < 0) {
        frame_cap = 0;
    }

    FILE* log = fopen("legoRR-stub.log", "a");
    if (log == NULL) {
        log = stderr;
    }

    int mounted = resource_mount(wad_dir, "LegoRR");
    fprintf(log, "frameloop: mounted %d wad(s) from %s\n", mounted, wad_dir);
    if (mounted == 0) {
        if (log != stderr) {
            fclose(log);
        }
        return 0;
    }

    /* --- Game layer: config DB + level load (findings 07/12/22) ---------- */
    GameContext game;
    game_context_init(&game);
    cmd_init();
    cmd_context_set(&game);
    game.cfg = block_open("Lego.cfg");
    if (game.cfg != NULL) {
        /* InfoMessages section -> per-id bus sounds (finding 22 §1). */
        message_load_infomessages(game.cfg);
    }
    int save_loaded = save_read_slot(&game, SAVE_DEFAULT_SLOT, 0);
    fprintf(log, "frameloop: save slot %d %s\n", SAVE_DEFAULT_SLOT,
            save_loaded ? "loaded" : "bootstrap");

    EngineState state = {};
    InputState  input = {};
    input_reset(&input);
    /* The three view contexts (DAT_005560e4/e8/ec): free/cruise cameras with
     * a glide step; the demo has no world yet, so they tick quietly. */
    ViewContext views[3] = {};
    for (int i = 0; i < 3; ++i) {
        views[i].mode = 3;
        views[i].step = 8.0f;
    }
    EngineContext ctx = {
        ENGINE_APP_NORMAL, 0, 0, 0, 1,
        &state, &input,
        { &views[0], &views[1], &views[2] },
        NULL, /* level_block — bound to game.level below */
        NULL, /* level_voxels — resolved at bind (64-bit safe) */
        NULL, /* level_terrain */
        0.0f, 0,
    };
    /* Engine <-> level binding happens after the front end below — the level
     * loads only once the player picks one — so ctx.level_block/voxels/
     * terrain are bound there, not here. */
    /* The finding-22 InfoMessages catalog + bus enabled. */
    message_init_defaults();

    /* Selection/command wiring (finding 06): the executor's held-key probe
     * reads this frame's input tables; a keybind demo for a global op. */
    game.input_keyheld = demo_keyheld;
    cmd_bind_key('z', 0x40, 0, 0, 0, 0);

    PlatformWindow* win = platform_create_window("lrr-portable (frame loop)",
                                                 WINDOW_W, WINDOW_H);
    if (win == NULL) {
        fprintf(log, "frameloop: no platform window (headless?)\n");
        if (log != stderr) {
            fclose(log);
        }
        return 1;
    }

    DisplaySurface surf = { 0, 0, 0, 0, 0 };
    int rc = surface_create(&surf, platform_window_hwnd(win),
                            WINDOW_W, WINDOW_H);
    if (rc != 0) {
        fprintf(log, "frameloop: surface_create rc=%d\n", rc);
        platform_destroy_window(win);
        if (log != stderr) {
            fclose(log);
        }
        return 2;
    }

    /* Real corpus textures through the merged texture loader. */
    TextureSurface* rock = texture_get_or_load(NULL,
                                               "World\\WorldTextures\\ROCKROOF.BMP");
    TextureSurface* ui = texture_get_or_load(NULL,
                                             "Interface\\Buttons\\info_path.bmp");
    if (rock == NULL || ui == NULL) {
        fprintf(log, "frameloop: texture load failed (rock=%p ui=%p)\n",
                (void*)rock, (void*)ui);
    }

    /* Audio: open the DirectSound-analog surface + play a real corpus wav. */
    AudioPlayer ap = {};
    int audio_status = audio_open(&ap);
    int sfx_frames = 0;
    if (audio_status == 0) {
        audio_set_volume(&ap, 0.6f);
        int loop_rc = audio_loop_named(&ap, "Sounds\\Iambloop.wav", 0.0f);
        sfx_frames = audio_play_named(&ap, "Sounds\\Buildings\\elec5.wav");
        fprintf(log,
                "frameloop: audio on, ambient loop Iambloop.wav rc=%d, "
                "startup sfx elec5.wav -> %d frames\n",
                loop_rc, sfx_frames);
    } else {
        fprintf(log, "frameloop: audio unavailable (headless?) rc=%d\n",
                audio_status);
    }
    /* Message bus demo hooks (finding 22): incidents dispatched by the game
     * layer (AI deposits, advisor hints) land here. */
    s_demo_ap = &ap;
    message_set_sound_hook(demo_msg_sound);
    message_set_callback(demo_msg_cb);
    /* Air supply drains in the demo; the air-beat loop + notifications
     * (0x11/0x12) kick in when it runs low. One loop track, so the air beat
     * replaces the ambient. */
    state.air_beat = 3;
    airbeat_set_loop(audio_status == 0 ? &ap : NULL,
                     "Sounds\\Vehicles\\Heliloop1.wav");

    /* --- Front-end boot (finding 23): main menu first -----------------------
     * The ship is modal: the front end runs until the player picks a level
     * (FE_EVT_START_LEVEL) or quits (FE_EVT_QUIT). The level itself loads
     * only after the menu closes, and `LEGORRR_LEVEL` (or FrontEnd FALSE in
     * the cfg) skips the menu straight into a level, like the classic demo. */
    char level_name[FE_LEVEL_NAME_MAX];
    FrontEnd fe;
    int boot_menu = 0;
    if (game.cfg != NULL) {
        fprintf(log, "frameloop: config Lego.cfg loaded\n");
        boot_menu = frontend_build(&fe, game.cfg)
                 && frontend_enabled(game.cfg)
                 && env_get("LEGORRR_LEVEL", "")[0] == '\0';
    } else {
        fprintf(log, "frameloop: Lego.cfg not found (wads need mounted game data)\n");
    }
    snprintf(level_name, sizeof(level_name), "%s",
             env_get("LEGORRR_LEVEL", "Level01"));
    long frames = 0;
    int quit = 0;
    const char* stop = "closed";
    if (boot_menu) {
        frontend_open(&fe, FE_MENU_MAIN);
        fprintf(log, "frameloop: front end up — main menu (arrows select, "
                     "Enter/Space act, Esc quits)\n");
        while (!quit) {
            PlatformEvent evbuf[32];
            int nev = platform_pump_events(win, evbuf, 32);
            input_capture_events(&input, evbuf, nev);
            for (int i = 0; i < nev; ++i) {
                if (evbuf[i].kind == PLATFORM_EVENT_QUIT) {
                    quit = 1;
                    stop = "closed";
                    break;
                }
            }
            if (quit || (frame_cap > 0 && frames >= frame_cap)) {
                if (!quit) {
                    stop = "autostop";
                }
                break;
            }
            FeInput fi;
            fi.up = input_key_edge(&input, PLATFORM_KEY_UP);
            fi.down = input_key_edge(&input, PLATFORM_KEY_DOWN);
            fi.enter = input_key_edge(&input, ' ')
                       || input_key_edge(&input, PLATFORM_KEY_ENTER);
            fi.esc = input_key_edge(&input, PLATFORM_KEY_ESC);
            FeEvent ev = frontend_tick(&fe, &fi);
            if (ev == FE_EVT_QUIT) {
                quit = 1;
                stop = "menu quit";
                break;
            }
            if (ev == FE_EVT_START_LEVEL) {
                snprintf(level_name, sizeof(level_name), "%s", fe.pending_level);
                fprintf(log, "frameloop: front end picked level %s\n", level_name);
                break;
            }

            void* pixels = NULL;
            int   pitch = 0;
            if (surface_lock(&surf, &pixels, &pitch) == 0) {
                surface_fill(pixels, pitch, WINDOW_W, WINDOW_H, 8, 8, 20);
                frontend_render(&fe, WINDOW_W, WINDOW_H, pixels, pitch);
                platform_screenshot(win, pixels, pitch, WINDOW_W, WINDOW_H,
                                    (int)frames);
                platform_present_pixels(win, pixels, pitch, WINDOW_W, WINDOW_H);
                surface_unlock(&surf, pixels);
            }
            Sleep((DWORD)FRAME_MS);
            ++frames;
            input_begin_frame(&input);
        }
    }

    /* --- Deferred level load (the front end chose a level) ------------------ */
    if (!quit && game.cfg != NULL) {
        game.level = level_load(game.cfg, level_name);
        if (game.level != NULL) {
            fprintf(log, "frameloop: level %s loaded: %dx%d grid, voxel stride "
                         "0x48, objective word 0x%08x\n",
                    level_name, level_width(game.level), level_height(game.level),
                    game.level->objective);
            /* Mission plumbing (finding 09/10): seed the 10-task table +
             * reward screen from the level's Reward group, then place the
             * `.ol` object list into the entity tables. */
            mission_task_init(&game, game.cfg, level_name);
            mission_reward_init(&game, game.cfg, level_name);
            int placed = object_list_load(&game, game.cfg, level_name);
            fprintf(log, "frameloop: mission tasks seeded, %s, %d object(s) "
                         "placed, crystal quota=%d\n",
                    game.cfg != NULL ? "reward config ok" : "no cfg",
                    placed, game.crystal_quota);
        } else {
            fprintf(log, "frameloop: level %s load FAILED (maps/config missing)\n",
                    level_name);
        }
        /* Engine <-> level binding: the engine sim reads the loaded
         * LevelState (width/height/voxels) so cameras/tickers see the real
         * terrain. The full pointer cannot ride the 32-bit corpus slot on a
         * 64-bit host, so the resolved voxel pointer is bound here via the
         * game layer — after the front end chose this level. */
        ctx.level_block = game.level;
        ctx.level_voxels = game.level != NULL
                               ? (const uint8_t*)level_load_ptr(game.level,
                                                                LEVEL_OFF_VOXELS)
                               : NULL;
        ctx.level_terrain = game.level != NULL
                                ? level_load_ptr(game.level, LEVEL_OFF_TERRAIN)
                                : NULL;
        if (game.level != NULL) {
            fprintf(log, "frameloop: engine bound to level %dx%d, blocksize %.1f, "
                         "voxels@%sresolved\n",
                    engine_level_width(&ctx), engine_level_height(&ctx),
                    engine_level_blocksize(&ctx),
                    ctx.level_voxels != NULL ? "" : "UN");
            /* Let the engine's camera grid sweep walk the real voxel grid. */
            g_camera_grid_enabled = 1;
        }
        /* Seed the follow camera at the grid centre so mode-1 starts over the
         * loaded level instead of the origin. */
        if (game.level != NULL) {
            cam_set_pos(&views[0].cam_follow,
                        (float)engine_level_width(&ctx) * 0.5f,
                        (float)engine_level_height(&ctx) * 0.5f, 0.0f);
        }
    }

    const float dt = 1.0f / 30.0f;
    /* Interactive proof: the arrow keys pan the blitted texture. */
    int rock_x = 24, rock_y = 24;

    while (!quit) {
        /* 1. Device capture for this frame (the finding-05 "writer" half):
         *    platform events -> canonical codes -> input tables. */
        PlatformEvent evbuf[32];
        int nev = platform_pump_events(win, evbuf, 32);
        input_capture_events(&input, evbuf, nev);
        for (int i = 0; i < nev; ++i) {
            if (evbuf[i].kind == PLATFORM_EVENT_QUIT
                || (evbuf[i].kind == PLATFORM_EVENT_KEYDOWN
                    && evbuf[i].code == PLATFORM_KEY_ESC)) {
                quit = 1;
                stop = "quit/ESC";
                break;
            }
        }
        if (quit) {
            break;
        }
        if (frame_cap > 0 && frames >= frame_cap) {
            stop = "autostop";
            break;
        }

        /* 2. Fine-grained pan via key edges (cur vs prev). */
        if (input_key_edge(&input, PLATFORM_KEY_UP))    { rock_y -= 8; }
        if (input_key_edge(&input, PLATFORM_KEY_DOWN))  { rock_y += 8; }
        if (input_key_edge(&input, PLATFORM_KEY_LEFT))  { rock_x -= 8; }
        if (input_key_edge(&input, PLATFORM_KEY_RIGHT)) { rock_x += 8; }
        /* Space re-triggers the SFX (interactive audio proof). */
        if (ap.dev != NULL && input_key_edge(&input, ' ')) {
            audio_play_named(&ap, "Sounds\\Buildings\\elec5.wav");
        }
        /* '[' / ']' adjust the master volume (FUN_00464ef0 analog). */
        if (ap.dev != NULL) {
            if (input_key_edge(&input, '[')) {
                audio_set_volume(&ap, ap.volume - 0.1f);
                fprintf(log, "frameloop: volume %0.1f\n", ap.volume);
            }
            if (input_key_edge(&input, ']')) {
                audio_set_volume(&ap, ap.volume + 0.1f);
                fprintf(log, "frameloop: volume %0.1f\n", ap.volume);
            }
            /* The looping ambient's pan follows the blitted texture — the
             * texture is the sound emitter in this demo. */
            float pan = (2.0f * (float)rock_x / (float)WINDOW_W) - 1.0f;
            audio_set_loop_pan(&ap, pan);
            audio_update(&ap);

            /* demo: air drains every ~2 s so the air-beat loop + the
             * 0x11/0x12 messages fire, and the air loop audibly replaces
             * the ambient. */
            if (frames > 0 && frames % 60 == 0 && state.air_beat > 0) {
                state.air_beat -= 1;
                fprintf(log, "frameloop: air=%u airbeat_loop=%d\n",
                        state.air_beat, airbeat_loop_active());
            }
        }
        /* Game layer proof-of-life:
         *  'S' — build the checkpoint blob + commit it to the active slot
         *  'L' — run CheckpointStep (bootstrap slot 5 + write-through)      */
        if (game.level != NULL && input_key_edge(&input, 's')) {
            checkpoint_blob_build(&game);
            save_set_current_record(&game, 0);
            save_write_record(&game, 0, game.checkpoint_blob);
            save_mark_record(&game, 0);
            save_commit(&game);
            fprintf(log, "frameloop: saved blob to slot %d record 0 (0x%x bytes)\n",
                    SAVE_DEFAULT_SLOT, SAVE_BLOB_SIZE);
        }
        if (game.level != NULL && input_key_edge(&input, 'l')) {
            game.save_slot = -1; /* force the bootstrap path */
            save_set_current_record(&game, 0);
            checkpoint_step(&game);
            fprintf(log, "frameloop: checkpoint_step committed slot %d\n",
                    SAVE_DEFAULT_SLOT);
        }
        /* Mission live-progress keys (finding 09 bridge):
         *  'c' +1 crystal   'o' +1 ore   '+' +10 score
         * These drive the objective checker and task completions in lieu
         * of the (future) entity sim. */
        if (game.level != NULL && input_key_edge(&input, 'c')) {
            mission_set_crystals(&game, game.mission.crystals + 1);
        }
        if (game.level != NULL && input_key_edge(&input, 'o')) {
            mission_set_ore(&game, game.mission.ore + 1);
        }
        if (game.level != NULL && input_key_edge(&input, '+')) {
            mission_add_score(&game, 10);
        }

        /* Selection / pointer input (finding 06): the tracked mouse maps to
         * a grid cell; 1-4 pick the nearest placed entity, 'm' moves the
         * selection there, 'n' clears. Commands are produced into the
         * double-buffered queue and dispatched by cmd_queue_tick below. */
        if (game.level != NULL) {
            s_demo_input = &input;
            int lw = level_width(game.level);
            int lh = level_height(game.level);
            float cx = lw > 0 ? (float)input.mouse_x / (float)WINDOW_W * (float)lw : 0.0f;
            float cy = lh > 0 ? (float)input.mouse_y / (float)WINDOW_H * (float)lh : 0.0f;

            if (input_key_edge(&input, 'n')) {
                selection_clear(&game);
                fprintf(log, "frameloop: selection cleared\n");
            }
            for (int t = 1; t <= 4; ++t) {
                char k = (char)('0' + t);
                if (input_key_edge(&input, k)) {
                    EntitySlot* hit = selection_pick(&game, cx, cy, 3.0f);
                    if (hit != NULL && (hit->type == t || hit->etype == t)) {
                        selection_add(&game, hit);
                        fprintf(log, "frameloop: picked %s (type %d) at cell "
                                     "(%.1f,%.1f)\n", hit->name, hit->type,
                                hit->x, hit->y);
                    } else if (hit != NULL) {
                        selection_clear(&game);
                        selection_add(&game, hit);
                        fprintf(log, "frameloop: picked nearest %s (type %d)\n",
                                hit->name, hit->type);
                    }
                }
            }
            if (input_key_edge(&input, 'm')) {
                int n = selection_cmd_move(&game, cx, cy);
                fprintf(log, "frameloop: move %d selected -> cell (%.1f,%.1f)\n",
                        n, cx, cy);
            }
            /* 'g' — issue a task (opcode 0x17) for every selected entity;
             * the AI receivers run it and deposit into the mission
             * counters (crystal/ore kinds by entity priority). */
            if (input_key_edge(&input, 'g')) {
                int n = 0;
                for (int i = 0; i < game.selection_count; ++i) {
                    cmd_enqueue(0x17, game.selection[i], 0, 0, 0);
                    ++n;
                }
                fprintf(log, "frameloop: gather task enqueued for %d selected\n",
                        n);
            }
            /* 'y' toggles the reward overlay manually (finding 09). */
            if (input_key_edge(&input, 'y')) {
                mission_reward_set_active(&game,
                                          !mission_reward_active(&game));
            }
        }
        if (rock_x < 0) { rock_x = 0; }
        if (rock_y < 0) { rock_y = 0; }
        if (rock_x > WINDOW_W - 64) { rock_x = WINDOW_W - 64; }
        if (rock_y > WINDOW_H - 64) { rock_y = WINDOW_H - 64; }

        /* Mission plumbing: per-frame progress bridge, command-queue
         * executor, AI receivers, and objective state machine
         * (findings 09/10/22). */
        if (game.level != NULL) {
            cmd_queue_tick();
            int ai_done = ai_tick(&game, dt);
            if (ai_done != 0) {
                fprintf(log, "frameloop: ai commands completed: %d\n", ai_done);
            }
            int done = mission_update(&game);
            if (done != 0) {
                fprintf(log, "frameloop: tasks completed this frame (mask 0x%x)\n",
                        done);
            }
            int resolved = objective_tick(game.level, dt);
            if (resolved == 0) {
                fprintf(log, "frameloop: objective resolved (word 0x%08x)\n",
                        game.level->objective);
                /* Advisor hint runtime (finding 22): the stream text the
                 * announcer built; the sound is gated on the outcome's
                 * suppression bits. */
                const char* adv = objective_stream(game.level);
                if (adv[0] != '\0') {
                    fprintf(log, "frameloop: advisor: %s\n", adv);
                }
                if (objective_advisor_shown(game.level) && ap.dev != NULL) {
                    audio_play_named(&ap, "Sounds\\Buildings\\elec5.wav");
                }
                if (!mission_reward_active(&game)) {
                    mission_reward_set_active(&game, 1);
                    fprintf(log, "frameloop: reward screen shown (click its "
                                 "Save/Advance button)\n");
                }
            }
            /* Reward-screen buttons (finding 09): a real mouse press inside
             * the Save/Advance zone commits the checkpoint. 'b' is the
             * keyboard fallback (headless). */
            if (mission_reward_active(&game)) {
                int hit_save = 0, hit_advance = 0;
                int clicked = input_mouse_pressed(&input);
                if (clicked
                    || (input_key_edge(&input, 'b')
                        && mission_reward_zone(&game, input.mouse_x,
                                               input.mouse_y, &hit_save,
                                               &hit_advance))) {
                    if (mission_reward_zone(&game, input.mouse_x,
                                            input.mouse_y, &hit_save,
                                            &hit_advance)) {
                        if (hit_save) {
                            mission_reward_click(&game, 1);
                            fprintf(log, "frameloop: reward Save clicked -> "
                                         "checkpoint committed\n");
                        } else {
                            const char* nm = mission_reward_advance(&game);
                            fprintf(log, "frameloop: reward Advance clicked -> "
                                         "%s\n",
                                    nm != NULL ? nm : "campaign end");
                        }
                    }
                }
            }
        }

        EngineTick(&ctx, dt);

        void* pixels = NULL;
        int   pitch = 0;
        if (surface_lock(&surf, &pixels, &pitch) == 0) {
            if (game.level != NULL) {
                /* Software level render (finding 22 §3): terrain grid from
                 * the voxel array + live entity markers + cursor cell. */
                level_render_map(game.level, &game, input.mouse_x, input.mouse_y,
                                 WINDOW_W, WINDOW_H, pixels, pitch);
            } else {
                surface_fill(pixels, pitch, WINDOW_W, WINDOW_H, 8, 16, 48);
                surface_draw_sweep(pixels, pitch, WINDOW_W, WINDOW_H,
                                   (int)frames);
                if (rock != NULL) {
                    surface_blit_texture(pixels, pitch, WINDOW_W, WINDOW_H,
                                         rock, rock_x, rock_y);
                }
                if (ui != NULL) {
                    surface_blit_texture(pixels, pitch, WINDOW_W, WINDOW_H,
                                         ui, rock_x + 64 + 8, rock_y);
                }
            }
            platform_screenshot(win, pixels, pitch, WINDOW_W, WINDOW_H,
                                (int)frames);
            platform_present_pixels(win, pixels, pitch, WINDOW_W, WINDOW_H);
            surface_unlock(&surf, pixels);
        }
        Sleep((DWORD)FRAME_MS);
        ++frames;

        /* 3. Frame boundary: this frame's keys become prev; cur clears for
         *    the next capture (edge semantics, findings 04/05). */
        input_begin_frame(&input);
    }

    fprintf(log, "frameloop: %ld frames, gauged state=0x%08x, tickers=%d, "
            "exit=%s\n", frames, ctx.state, ctx.tickers_ran, stop);

    EngineShutdown(&ctx);
    audio_close(&ap);
    texture_cache_clear();
    surface_destroy(&surf);
    platform_destroy_window(win);

    /* Game-layer teardown: checkpoint blob + slot records + level + config. */
    checkpoint_blob_free(&game);
    free(game.records);
    level_destroy(game.level);
    block_close(game.cfg);

    if (log != stderr) {
        fclose(log);
    }
    return 0;
}