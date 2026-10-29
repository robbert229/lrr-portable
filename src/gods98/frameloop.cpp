// SPDX-License-Identifier: LGPL-3.0-or-later
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "game/game.hpp"
#include "game/ai.hpp"
#include "game/cmd.hpp"
#include "game/entity.hpp"
#include "game/frontend.hpp"
#include "game/level.hpp"
#include "game/mission.hpp"
#include "game/options.hpp"
#include "game/objective.hpp"
#include "game/render.hpp"
#include "game/savegame.hpp"
#include "game/select.hpp"
#include "platform/platform.h"
#include "audio.hpp"
#include "engine.hpp"
#include "frameloop.hpp"
#include "input.hpp"
#include "movie.hpp"
#include "resource.hpp"
#include "surface.hpp"
#include "sound.hpp"
#include "texture.hpp"
#include "tickers.hpp"
#include "view.hpp"
#include "camera.hpp"
#include "message.hpp"
#include "game/util.hpp"

#define WINDOW_W 640
#define WINDOW_H 480
#define FRAME_MS 33 /* ~30 fps */

static const char* env_get(const char* name, const char* fallback)
{
    const char* v = getenv(name);
    return (v != NULL && v[0] != '\0') ? v : fallback;
}

static int s_icontains(const char* haystack, const char* needle)
{
    if (haystack == NULL || needle == NULL) {
        return 0;
    }
    for (size_t i = 0; haystack[i] != '\0'; i++) {
        size_t j = 0;
        while (needle[j] != '\0' && (unsigned char)(haystack[i + j] | 0x20)
               == (unsigned char)(needle[j] | 0x20)) {
            ++j;
        }
        if (needle[j] == '\0') {
            return 1;
        }
    }
    return 0;
}

static void frameloop_game_context_dispose(GameContext* game)
{
    if (game == NULL) {
        return;
    }
    checkpoint_blob_free(game);
    free(game->records);
    level_destroy(game->level);
    block_close(game->cfg);
}

/* Streamed soundtrack aliases live beside the menu SFX aliases in Samples.
 * Their leading '@' marks a streamed WAVE in Lego.cfg, not part of its path. */
static const char* menu_music_resource(BlockFile* cfg)
{
    const char* music = block_get_strptr(cfg,
                                         "Lego* Samples SFX_AmbientMusicLoop");
    if (music != NULL && music[0] == '@') {
        ++music;
    }
    return music;
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

typedef struct FrameloopOverlayMenuContext {
    FrontEnd* fe;
    GameContext* game;
    FeOverlayMenuGateState* gate;
    const FeInput* input;
    const TextureSurface* measure;
    FeEvent event;
    int mode;
} FrameloopOverlayMenuContext;

static void frameloop_overlay_menu_scroll_refresh(void* context)
{
    FrameloopOverlayMenuContext* gate_ctx =
        (FrameloopOverlayMenuContext*)context;
    scroll_slider_refresh(gate_ctx != NULL ? gate_ctx->game : NULL);
}

static int frameloop_overlay_menu_tick(void* context, int active_item, int mode)
{
    FrameloopOverlayMenuContext* gate_ctx =
        (FrameloopOverlayMenuContext*)context;
    (void)active_item;
    (void)mode;
    if (gate_ctx == NULL || gate_ctx->fe == NULL || gate_ctx->input == NULL) {
        return 0;
    }
    gate_ctx->event = frontend_tick(gate_ctx->fe, gate_ctx->input,
                                    gate_ctx->measure);
    if (gate_ctx->event != FE_EVT_NONE && gate_ctx->gate != NULL) {
        FeOverlayMenuSet* menu = &gate_ctx->gate->menus[gate_ctx->mode];
        const int slot = gate_ctx->mode == 0 && gate_ctx->fe->sel == 2 ? 2 : 0;
        menu->entries[slot].action_state = 1;
    }
    return gate_ctx->fe->open ? gate_ctx->fe->sel + 1 : 0;
}
static int demo_msg_cb(int id)
{
    (void)id; /* the bus appends it to the active queue */
    return 0; /* unhandled: dedupe callback does not consume */
}

/* Monotonic wall clock (ms) — the shell's frame pacing clock (the corpus
 * uses timeGetTime; the port is libc-portable). */
static int64_t now_ms(void)
{
#if defined(LEGORRR_BACKEND_WINAPI)
    return (int64_t)GetTickCount();
#else
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    return (int64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
#endif
}

/* Play one AVI through the software decoder (src/gods98/movie.*) through
 * the rect-aware pump entry (movie_video_playback_loop, the port
 * replacement for DirectShowVideoPlaybackLoop @ 0x42efc0 — the level-load
 * and objective-tick video paths shape through this same seam): decode ->
 * scale-fill (full window for rect == NULL, into rect otherwise) ->
 * present, feed the movie's audio track to the seam, paced at the movie's
 * fps. Modal like the corpus's MoviePlayLoop @ 0x4157c0 — steps until done
 * or a skip key (ESC/Space/Enter/click), and propagates a window quit.
 * Returns 1 when a movie was played (or skipped after opening), 0 when it
 * could not be opened. */
typedef struct MoviePumpState {
    Movie*             mv;
    MovieInfo          mi;
    const MovieRect*   rect;      /* NULL = full-window branch */
    PlatformWindow*    win;
    DisplaySurface*    surf;
    AudioPlayer*       ap;
    int*               quit;
    int                skipped;
    int64_t            next_t;
    int                frame_ms;
    int                frames;
} MoviePumpState;

static int s_movie_frame_next(MovieOut* out, void* user)
{
    MoviePumpState* st = (MoviePumpState*)user;
    int r = movie_next(st->mv, out);
    return r == 0 ? 0 : 1;   /* EOF or error ends the branch */
}

static int s_movie_frame_present(const MovieOut* out, const MovieInfo* mi,
                                const MovieRect* rect, void* user)
{
    MoviePumpState* st = (MoviePumpState*)user;
    if (st->ap->dev != NULL && out->audio_frames > 0 && out->audio_rate > 0) {
        platform_audio_play_stereo(st->ap->dev, out->audio, out->audio_frames,
                                   out->audio_rate);
    }
    void* pixels = NULL;
    int   pitch = 0;
    if (surface_lock(st->surf, &pixels, &pitch) == 0) {
        if (rect != NULL) {
            movie_scale_rgba_rect(out->rgba, mi->width, mi->height,
                                  out->rgba_pitch, (uint8_t*)pixels,
                                  WINDOW_W, WINDOW_H, pitch, rect);
        } else {
            movie_scale_rgba(out->rgba, mi->width, mi->height, out->rgba_pitch,
                             (uint8_t*)pixels, WINDOW_W, WINDOW_H, pitch);
        }
        platform_screenshot(st->win, pixels, pitch, WINDOW_W, WINDOW_H,
                            (int)st->frames);
        platform_present_pixels(st->win, pixels, pitch, WINDOW_W, WINDOW_H);
        surface_unlock(st->surf, pixels);
    }
    st->next_t += st->frame_ms;
    int64_t now = now_ms();
    if (st->next_t > now) {
        Sleep((DWORD)(st->next_t - now));
    } else if (now - st->next_t > 2 * st->frame_ms) {
        st->next_t = now;    /* fell behind — resync instead of bursting */
    }
    ++st->frames;
    return 0;
}

static int s_movie_frame_quit(void* user)
{
    MoviePumpState* st = (MoviePumpState*)user;
    PlatformEvent evb[16];
    int nev = platform_pump_events(st->win, evb, 16);
    for (int i = 0; i < nev; ++i) {
        if (evb[i].kind == PLATFORM_EVENT_QUIT) {
            *st->quit = 1;
            return 1;
        }
        if (evb[i].kind == PLATFORM_EVENT_KEYDOWN
            && (evb[i].code == PLATFORM_KEY_ESC
                || evb[i].code == ' ' || evb[i].code == PLATFORM_KEY_ENTER)) {
            st->skipped = 1;
            return 1;
        }
        if (evb[i].kind == PLATFORM_EVENT_MOUSE && evb[i].button == 1) {
            st->skipped = 1;
            return 1;
        }
    }
    return 0;
}

static int movie_play_avi_rect(const char* os_path, const char* label,
                               PlatformWindow* win, DisplaySurface* surf,
                               AudioPlayer* ap, int* quit,
                               const MovieRect* rect, FILE* log);

static int movie_play_avi(const char* os_path, const char* label,
                          PlatformWindow* win, DisplaySurface* surf,
                          AudioPlayer* ap, int* quit, FILE* log)
{
    return movie_play_avi_rect(os_path, label, win, surf, ap, quit, NULL, log);
}

static int movie_play_avi_rect(const char* os_path, const char* label,
                               PlatformWindow* win, DisplaySurface* surf,
                               AudioPlayer* ap, int* quit,
                               const MovieRect* rect, FILE* log)
{
    Movie* mv = movie_open_path(os_path);
    if (mv == NULL) {
        fprintf(log, "movie: %s: cannot open/decode %s\n", label, os_path);
        return 0;
    }
    MoviePumpState st;
    memset(&st, 0, sizeof(st));
    st.mv = mv;
    if (movie_info(mv, &st.mi) != 0 || st.mi.width <= 0 || st.mi.height <= 0) {
        fprintf(log, "movie: %s: no video stream in %s\n", label, os_path);
        movie_close(mv);
        return 0;
    }
    fprintf(log, "movie: %s %s — %dx%d @ %d fps, %d Hz audio%s\n", label,
            os_path, st.mi.width, st.mi.height,
            st.mi.fps_den > 0 && st.mi.fps_num > 0 ? st.mi.fps_num / st.mi.fps_den
                                             : st.mi.fps_num,
            st.mi.audio_rate, rect != NULL ? " (rect)" : "");

    /* The movie carries its own soundtrack — silence the demo's ambient
     * loop while it plays (the caller re-seeds it afterwards). */
    audio_stop(ap);

    st.rect = rect;
    st.win = win;
    st.surf = surf;
    st.ap = ap;
    st.quit = quit;
    st.frame_ms = st.mi.fps_den > 0 && st.mi.fps_num > 0
                      ? 1000 * st.mi.fps_den / st.mi.fps_num
                      : 40;
    if (st.frame_ms < 1) {
        st.frame_ms = 1;
    }
    st.next_t = now_ms();

    MoviePump pump;
    memset(&pump, 0, sizeof(pump));
    pump.next = s_movie_frame_next;
    pump.next_user = &st;
    pump.info = &st.mi;
    pump.rect = rect;                 /* NULL = full-window branch */
    pump.present = s_movie_frame_present;
    pump.present_user = &st;
    pump.quit = s_movie_frame_quit;
    pump.quit_user = &st;
    /* No restore hook: the port's lock-present-blit path leaves no lasting
     * surface state, so the corpus's setup-time restore seam is inert
     * here. */
    movie_video_playback_loop(&pump);

    movie_close(mv);
    fprintf(log, "movie: %s done — %d frames%s\n", label, st.frames,
            st.skipped ? " (skipped)" : "");
    return 1;
}

static int credits_load_lines(const char* name, char*** out_lines,
                              int* out_count)
{
    *out_lines = NULL;
    *out_count = 0;
    Resource* r = resource_open(name, RESOURCE_AUTO);
    if (r == NULL) {
        return 0;
    }
    int32_t size = resource_read(r);
    if (size <= 0) {
        resource_free(r);
        return 0;
    }
    char* text = (char*)malloc((size_t)size + 1);
    if (text == NULL) {
        resource_free(r);
        return 0;
    }
    memcpy(text, r->data, (size_t)size);
    text[size] = '\0';
    resource_free(r);

    int count = 1;
    for (int32_t i = 0; i < size; ++i) {
        if (text[i] == '\n') {
            ++count;
        }
    }
    char** lines = (char**)calloc((size_t)count, sizeof(*lines));
    if (lines == NULL) {
        free(text);
        return 0;
    }
    int n = 0;
    char* start = text;
    for (char* p = text;; ++p) {
        if (*p != '\n' && *p != '\0') {
            continue;
        }
        char saved = *p;
        *p = '\0';
        size_t len = strlen(start);
        if (len > 0 && start[len - 1] == '\r') {
            start[--len] = '\0';
        }
        lines[n] = (char*)malloc(len + 1);
        if (lines[n] == NULL) {
            for (int i = 0; i < n; ++i) {
                free(lines[i]);
            }
            free(lines);
            free(text);
            return 0;
        }
        memcpy(lines[n++], start, len + 1);
        if (saved == '\0') {
            break;
        }
        start = p + 1;
    }
    free(text);
    *out_lines = lines;
    *out_count = n;
    return 1;
}

static void credits_free_lines(char** lines, int count)
{
    for (int i = 0; i < count; ++i) {
        free(lines[i]);
    }
    free(lines);
}

/* CreditsTextFile + CreditsBackAVI: scroll centered text over the configured
 * AVI until the player presses a key or clicks. */
static void frameloop_credits(PlatformWindow* win, DisplaySurface* surf,
                              BlockFile* cfg, const char* wad_dir,
                              AudioPlayer* ap, int audio_status, int* quit,
                              FILE* log, long* frame_number)
{
    const char* text_name = block_get_strptr(cfg,
                                             "Lego* Main CreditsTextFile");
    char** lines = NULL;
    int line_count = 0;
    if (text_name == NULL
        || !credits_load_lines(text_name, &lines, &line_count)) {
        fprintf(log, "credits: cannot load %s\n",
                text_name != NULL ? text_name : "Lego* Main CreditsTextFile");
        return;
    }

    TextureSurface* font = texture_get_or_load(
        NULL, "Interface\\Fonts\\RSFont.bmp");
    Movie* movie = NULL;
    MovieInfo mi;
    MovieOut out;
    memset(&mi, 0, sizeof(mi));
    memset(&out, 0, sizeof(out));
    int video_frame_ms = FRAME_MS;
    char movie_rel[256], movie_os[512];
    if (movie_avis_enabled(cfg)
        && movie_key_path(cfg, "CreditsBackAVI", movie_rel,
                          sizeof(movie_rel))
        && movie_path_resolve(wad_dir, movie_rel, movie_os,
                              sizeof(movie_os))) {
        movie = movie_open_path(movie_os);
        if (movie == NULL || movie_info(movie, &mi) != 0
            || mi.width <= 0 || mi.height <= 0) {
            fprintf(log, "credits: cannot open backdrop %s\n", movie_os);
            movie_close(movie);
            movie = NULL;
        } else if (mi.fps_num > 0 && mi.fps_den > 0) {
            video_frame_ms = 1000 * mi.fps_den / mi.fps_num;
            if (video_frame_ms < 1) {
                video_frame_ms = 1;
            }
        }
    }
    InputState input;
    input_reset(&input);
    long scroll_px = 0;
    int64_t next_video_ms = now_ms();
    int have_video_frame = 0;
    int leave = 0;
    while (!*quit && !leave) {
        PlatformEvent evbuf[32];
        int nev = platform_pump_events(win, evbuf, 32);
        input_capture_events(&input, evbuf, nev);
        for (int i = 0; i < nev; ++i) {
            if (evbuf[i].kind == PLATFORM_EVENT_QUIT) {
                *quit = 1;
                break;
            }
            if (evbuf[i].kind == PLATFORM_EVENT_KEYDOWN
                || (evbuf[i].kind == PLATFORM_EVENT_MOUSE
                    && evbuf[i].button == 1)) {
                leave = 1;
            }
        }
        if (*quit || leave) {
            break;
        }

        if (movie != NULL && now_ms() >= next_video_ms) {
            int mr = movie_next(movie, &out);
            if (mr == 1) {
                movie_close(movie);
                movie = movie_open_path(movie_os);
                if (movie != NULL && movie_info(movie, &mi) == 0) {
                    mr = movie_next(movie, &out);
                } else {
                    movie_close(movie);
                    movie = NULL;
                    mr = -1;
                }
            }
            if (mr == 0) {
                have_video_frame = 1;
            } else if (mr < 0) {
                movie_close(movie);
                movie = NULL;
                have_video_frame = 0;
            }
            next_video_ms += video_frame_ms;
            if (next_video_ms < now_ms() - video_frame_ms) {
                next_video_ms = now_ms();
            }
        }

        void* pixels = NULL;
        int pitch = 0;
        if (surface_lock(surf, &pixels, &pitch) == 0) {
            if (have_video_frame) {
                movie_scale_rgba(out.rgba, mi.width, mi.height, out.rgba_pitch,
                                 (uint8_t*)pixels, WINDOW_W, WINDOW_H, pitch);
            } else {
                surface_fill(pixels, pitch, WINDOW_W, WINDOW_H, 0, 0, 0);
            }
            frontend_render_credits(font,
                                    (const char* const*)lines, line_count,
                                    (int)scroll_px, WINDOW_W, WINDOW_H,
                                    pixels, pitch);
            platform_screenshot(win, pixels, pitch, WINDOW_W, WINDOW_H,
                                (int)*frame_number);
            platform_present_pixels(win, pixels, pitch, WINDOW_W, WINDOW_H);
            surface_unlock(surf, pixels);
        }
        if (audio_status == 0) {
            audio_update(ap);
        }
        Sleep(FRAME_MS);
        ++scroll_px;
        ++*frame_number;
        input_begin_frame(&input);
    }

    movie_close(movie);
    texture_release(font);
    credits_free_lines(lines, line_count);
}

/* Level load + mission seeding + level intro movie + engine binding. Both
 * the post-front-end boot path and the pause menu's Restart route here
 * (finding 07/09 + #47's level cinematics). */
static void frameloop_load_level(FILE* log, const char* wad_dir,
                                 int play_movies, PlatformWindow* win,
                                 DisplaySurface* surf, AudioPlayer* ap,
                                 int audio_status, int* quit,
                                 GameContext* game, EngineContext* ctx,
                                 ViewContext* views, const char* level_name)
{
    game->level = level_load(game->cfg, level_name);
    if (game->level != NULL) {
        fprintf(log, "frameloop: level %s loaded: %dx%d grid, voxel stride "
                     "0x48, objective word 0x%08x\n",
                level_name, level_width(game->level), level_height(game->level),
                game->level->objective);
        /* Level intro cinematic — MoviePlayLevelVideo @ 0x415990 reads the
         * level's `Video` config key and plays it after load (the corpus
         * plays it inside LevelLoad @ 0x429890 through
         * DirectShowVideoPlaybackLoop @ 0x42efc0 — whose full-window branch
         * this seam reproduces: the corpus's rect blit never applied its
         * computed rectangle, finding 206), same gates as boot. */
        if (play_movies) {
            char mp[256], os[512];
            if (movie_level_video_path(game->cfg, level_name, mp, sizeof(mp))
                && movie_path_resolve(wad_dir, mp, os, sizeof(os))) {
                movie_play_avi(os, level_name, win, surf, ap, quit, log);
                if (audio_status == 0 && !*quit) {
                    /* the level movie also silenced the ambient */
                    audio_loop_named(ap, "Sounds\\Iambloop.wav", 0.0f);
                }
            }
        }
        /* Mission plumbing (finding 09/10): seed the 10-task table +
         * reward screen from the level's Reward group, then place the
         * `.ol` object list into the entity tables. */
        mission_task_init(game, game->cfg, level_name);
        mission_reward_init(game, game->cfg, level_name);
        int placed = object_list_load(game, game->cfg, level_name);
        fprintf(log, "frameloop: mission tasks seeded, %s, %d object(s) "
                     "placed, crystal quota=%d\n",
                game->cfg != NULL ? "reward config ok" : "no cfg",
                placed, game->crystal_quota);
    } else {
        fprintf(log, "frameloop: level %s load FAILED (maps/config missing)\n",
                level_name);
    }
    /* Engine <-> level binding: the engine sim reads the loaded LevelState
     * (width/height/voxels) so cameras/tickers see the real terrain. The
     * full pointer cannot ride the 32-bit corpus slot on a 64-bit host, so
     * the resolved voxel pointer is bound here via the game layer — after
     * the front end chose this level. */
    ctx->level_block = game->level;
    ctx->level_voxels = game->level != NULL
                            ? (const uint8_t*)level_load_ptr(game->level,
                                                             LEVEL_OFF_VOXELS)
                            : NULL;
    ctx->level_terrain = game->level != NULL
                             ? level_load_ptr(game->level, LEVEL_OFF_TERRAIN)
                             : NULL;
    if (game->level != NULL) {
        fprintf(log, "frameloop: engine bound to level %dx%d, blocksize %.1f, "
                     "voxels@%sresolved\n",
                engine_level_width(ctx), engine_level_height(ctx),
                engine_level_blocksize(ctx),
                ctx->level_voxels != NULL ? "" : "UN");
        /* Let the engine's camera grid sweep walk the real voxel grid. */
        g_camera_grid_enabled = 1;
    }
    /* Seed the follow camera at the grid centre so mode-1 starts over the
     * loaded level instead of the origin. */
    if (game->level != NULL) {
        cam_set_pos(&views[0].cam_follow,
                    (float)engine_level_width(ctx) * 0.5f,
                    (float)engine_level_height(ctx) * 0.5f, 0.0f);
    }
}

static void menu_save_image_config_rect_blit(FrontEnd* fe,
                                             GameContext* game,
                                             int item_index, void* pixels,
                                             int pitch, int width, int height);

static void frameloop_blit_texture_scaled(const TextureSurface* texture,
                                          void* pixels, int pitch,
                                          int width, int height, int x, int y,
                                          int draw_w, int draw_h)
{
    if (texture == NULL || texture->pixels == NULL || pixels == NULL
        || texture->width <= 0 || texture->height <= 0
        || draw_w <= 0 || draw_h <= 0) {
        return;
    }
    for (int dy = 0; dy < draw_h; ++dy) {
        int py = y + dy;
        if (py < 0 || py >= height) {
            continue;
        }
        int sy = (int)((int64_t)dy * texture->height / draw_h);
        uint8_t* dst = (uint8_t*)pixels + (size_t)py * pitch;
        for (int dx = 0; dx < draw_w; ++dx) {
            int px = x + dx;
            if (px < 0 || px >= width) {
                continue;
            }
            int sx = (int)((int64_t)dx * texture->width / draw_w);
            uint32_t color = texture->pixels[(size_t)sy * texture->width + sx];
            dst[(size_t)px * 4 + 0] = (uint8_t)(color >> 16);
            dst[(size_t)px * 4 + 1] = (uint8_t)(color >> 8);
            dst[(size_t)px * 4 + 2] = (uint8_t)color;
            dst[(size_t)px * 4 + 3] = (uint8_t)(color >> 24);
        }
    }
}

/* The first tutorial's Mission Brief is shipped as the Objective entry in
 * ObjectiveText.txt. Keep this read at the resource boundary so the briefing
 * follows the installed language data instead of duplicating its prose. */
static int frameloop_tutorial_brief_load(char* out, size_t out_size)
{
    if (out == NULL || out_size == 0) return 0;
    out[0] = '\0';
    Resource* r = resource_open("Languages\\ObjectiveText.txt", RESOURCE_AUTO);
    if (r == NULL || resource_read(r) < 0 || r->data == NULL) {
        resource_free(r);
        return 0;
    }
    const char* data = (const char*)r->data;
    const size_t size = (size_t)r->size;
    const char* section = NULL;
    const char* end = data + size;
    for (const char* p = data; p < end;) {
        const char* line_end = (const char*)memchr(p, '\n', (size_t)(end - p));
        if (line_end == NULL) line_end = end;
        const char* line = p;
        while (line < line_end && (*line == ' ' || *line == '\t' || *line == '\r')) ++line;
        if (line < line_end && *line == '[') {
            section = ((size_t)(line_end - line) >= 13
                       && memcmp(line, "[MoveTuto_01]", 13) == 0)
                          ? line_end : NULL;
        } else if (section != NULL && line_end - line >= 10
                   && memcmp(line, "Objective:", 10) == 0) {
            line += 10;
            while (line < line_end && (*line == ' ' || *line == '\t')) ++line;
            size_t n = (size_t)(line_end - line);
            while (n > 0 && (line[n - 1] == '\r' || line[n - 1] == ' ')) --n;
            /* The \a marker separates the mission briefing from the later
             * objective update; the opening panel shows the first paragraph. */
            for (size_t i = 0; i + 1 < n; ++i) {
                if (line[i] == '\\' && line[i + 1] == 'a') { n = i; break; }
            }
            if (n >= out_size) n = out_size - 1;
            memcpy(out, line, n);
            out[n] = '\0';
            resource_free(r);
            return n > 0;
        }
        p = line_end < end ? line_end + 1 : end;
    }
    resource_free(r);
    return 0;
}

static void frameloop_wrap_brief(const char* src, char* out, size_t out_size,
                                int max_chars)
{
    size_t at = 0;
    int col = 0;
    for (const char* p = src; p != NULL && *p != '\0' && at + 1 < out_size;) {
        while (*p == ' ' || *p == '\t') ++p;
        if (*p == '\0') break;
        const char* word = p;
        int len = 0;
        while (p[len] != '\0' && p[len] != ' ' && p[len] != '\t') ++len;
        if (col > 0 && col + 1 + len > max_chars) {
            out[at++] = '\n';
            col = 0;
        } else if (col > 0 && at + 1 < out_size) {
            out[at++] = ' ';
            ++col;
        }
        for (int i = 0; i < len && at + 1 < out_size; ++i) {
            out[at++] = word[i];
            ++col;
        }
        p += len;
    }
    out[at] = '\0';
}

static void frameloop_briefing_render(PlatformWindow* win,
                                     DisplaySurface* surf,
                                     const TextureSurface* panel,
                                     const TextureSurface* title_font,
                                     const TextureSurface* body_font,
                                     const char* brief,
                                     LevelState* level, GameContext* game,
                                     int frame)
{
    void* pixels = NULL;
    int pitch = 0;
    if (surface_lock(surf, &pixels, &pitch) != 0) return;
    level_render_scene(level, game, WINDOW_W, WINDOW_H, pixels, pitch);
    if (panel != NULL) {
        frameloop_blit_texture_scaled(panel, pixels, pitch, WINDOW_W, WINDOW_H,
                                      76, 100, 488, 281);
    }
    if (title_font != NULL) {
        const char* title = "Mission Brief";
        int tw = (int)formatted_text_printf_width(title_font, title);
        formatted_text_draw(title_font, pixels, pitch, WINDOW_W, WINDOW_H,
                            (WINDOW_W - tw) / 2, 130, title);
    }
    if (body_font != NULL) {
        char wrapped[1200];
        if (brief != NULL && brief[0] != '\0') {
            frameloop_wrap_brief(brief, wrapped, sizeof(wrapped), 43);
            formatted_text_draw(body_font, pixels, pitch, WINDOW_W, WINDOW_H,
                                119, 160, wrapped);
        }
        uint8_t* frame = (uint8_t*)pixels;
        for (int y = 438; y < WINDOW_H; ++y) {
            uint8_t* row = frame + (size_t)y * pitch;
            for (int x = 0; x < WINDOW_W; ++x) {
                row[(size_t)x * 4 + 0] = 5;
                row[(size_t)x * 4 + 1] = 47;
                row[(size_t)x * 4 + 2] = 9;
                row[(size_t)x * 4 + 3] = 255;
            }
        }
        const TextureSurface* prompt_font = title_font != NULL
            ? title_font : body_font;
        formatted_text_draw(prompt_font, pixels, pitch, WINDOW_W, WINDOW_H,
                            122, 452, "Press SPACE to continue.");
    }
    render_apply_brightness(pixels, pitch, WINDOW_W, WINDOW_H,
                            game->options.brightness);
    platform_screenshot(win, pixels, pitch, WINDOW_W, WINDOW_H, frame);
    platform_present_pixels(win, pixels, pitch, WINDOW_W, WINDOW_H);
    surface_unlock(surf, pixels);
}

/* MenuOverlaySurfaceAndTextSetup (FUN_004129d0): the corpus resets the
 * save/load panel surface and its text resources when entering these menu
 * records. The frame-driven adapter composes the typed panel layout and the
 * current menu text on the software surface. */
// FUNCTION: LEGORR 0x004129d0
static void menu_overlay_surface_and_text_setup(
    FrontEnd* fe, const TextureSurface* hi_font, const TextureSurface* lo_font,
    void* pixels, int pitch, int width, int height)
{
    if (fe == NULL || pixels == NULL) {
        return;
    }
    const char* image = frontend_active_image(fe);
    const FeSubmenu* active = frontend_active_submenu(fe);
    if (active != NULL
        && (fe->menu == FE_MENU_SAVE
            || (fe->menu == FE_MENU_MAIN && image != NULL
                && s_icontains(image, "SaveLoad")))) {
        const FeSaveMenuLayout* layout = fe->save_action
                                             ? &fe->save_text_layout
                                             : &fe->level_text_layout;
        if (layout->has_panel && layout->panel_path[0] != '\0') {
            TextureSurface* panel = texture_get_or_load(NULL,
                                                        layout->panel_path);
            if (panel != NULL) {
                frameloop_blit_texture_scaled(panel, pixels, pitch, width,
                                              height, layout->panel_x,
                                              layout->panel_y, layout->panel_w,
                                              layout->panel_h);
                texture_release(panel);
            }
        }
    }
    frontend_render(fe, hi_font, lo_font, width, height, pixels, pitch);
}

/* Draw one front-end frame: frozen level (in-game pause) or a plain fill
 * (boot menu), then the submenu's MenuImage art + the menu layer. */
static void frameloop_menu_render(PlatformWindow* win, DisplaySurface* surf,
                                  FrontEnd* fe, const TextureSurface* hi_font,
                                  const TextureSurface* lo_font,
                                  GameContext* game, int frame)
{
    void* pixels = NULL;
    int   pitch = 0;
    if (surface_lock(surf, &pixels, &pitch) != 0) {
        return;
    }
    if (game->level != NULL) {
        level_render_scene(game->level, game, WINDOW_W, WINDOW_H,
                           pixels, pitch);
    } else {
        surface_fill(pixels, pitch, WINDOW_W, WINDOW_H, 8, 8, 20);
    }
    const char* img = frontend_active_image(fe);
    if (img != NULL) {
        TextureSurface* bg = texture_get_or_load(NULL, img);
        if (bg != NULL) {
            surface_blit_texture(pixels, pitch, WINDOW_W, WINDOW_H, bg, 0, 0);
        }
        texture_release(bg);
    }
    menu_overlay_surface_and_text_setup(fe, hi_font,
                                        lo_font, pixels, pitch, WINDOW_W,
                                        WINDOW_H);
    const FeSubmenu* active = frontend_active_submenu(fe);
    if (active != NULL) {
        for (int i = 0; i < active->item_count; ++i) {
            menu_save_image_config_rect_blit(fe, game, i, pixels, pitch,
                                             WINDOW_W, WINDOW_H);
        }
    }
    /* Brightness (issue #85) is a post-pass over the finished frame. */
    render_apply_brightness(pixels, pitch, WINDOW_W, WINDOW_H,
                            game->options.brightness);
    platform_screenshot(win, pixels, pitch, WINDOW_W, WINDOW_H, frame);
    platform_present_pixels(win, pixels, pitch, WINDOW_W, WINDOW_H);
    surface_unlock(surf, pixels);
}

/* MenuSaveImageConfigRectBlit (FUN_004124f0): load the selected slot's
 * framebuffer thumbnail and draw it in the configured SaveImage rectangle.
 * The port's TextureSurface replaces the corpus's cached DirectDraw handle. */
// FUNCTION: LEGORR 0x004124f0
static void menu_save_image_config_rect_blit(FrontEnd* fe,
                                             GameContext* game,
                                             int item_index, void* pixels,
                                             int pitch, int width, int height)
{
    if (fe == NULL || game == NULL || pixels == NULL || fe->cfg == NULL
        || item_index < 0) {
        return;
    }
    const FeSubmenu* sub = frontend_active_submenu(fe);
    const char* image = frontend_active_image(fe);
    if (sub == NULL || image == NULL || !s_icontains(image, "SaveLoad")
        || item_index >= sub->item_count) {
        return;
    }
    const FeItem* item = &sub->items[item_index];
    if (item->load_slot <= 0) {
        return;
    }

    char key[160];
    snprintf(key, sizeof(key), "%s Menu::SaveImage pos%d",
             util_root_name(fe->cfg), item->load_slot);
    const char* position = block_get_strptr(fe->cfg, key);
    if (position == NULL) {
        return;
    }
    char* end = NULL;
    long x = strtol(position, &end, 10);
    if (end == position || *end != ':') {
        return;
    }
    char* y_start = end + 1;
    long y = strtol(y_start, &end, 10);
    if (end == y_start) {
        return;
    }
    snprintf(key, sizeof(key), "%s Menu::SaveImage Width",
             util_root_name(fe->cfg));
    int draw_w = block_get_int(fe->cfg, key, 160);
    snprintf(key, sizeof(key), "%s Menu::SaveImage Height",
             util_root_name(fe->cfg));
    int draw_h = block_get_int(fe->cfg, key, 120);
    if (draw_w <= 0 || draw_h <= 0 || draw_w > width * 2
        || draw_h > height * 2) {
        return;
    }

    char thumb_path[320];
    save_slot_path(game, thumb_path, sizeof(thumb_path), item->load_slot,
                   "dat");
    const int cache_index = item->load_slot - 1;
    if (cache_index < 0 || cache_index >= FE_SAVE_THUMBNAIL_COUNT) {
        return;
    }
    if (fe->save_thumbnails[cache_index] == NULL) {
        fe->save_thumbnails[cache_index] =
            texture_get_or_load(NULL, thumb_path);
    }
    TextureSurface* thumb = fe->save_thumbnails[cache_index];
    if (thumb == NULL || thumb->pixels == NULL || thumb->width <= 0
        || thumb->height <= 0) {
        return;
    }
    frameloop_blit_texture_scaled(thumb, pixels, pitch, width, height,
                                  (int)x, (int)y, draw_w, draw_h);
}

int LegorrrRun(void)
{
    const char* wad_dir = env_get("LEGORRR_WAD_DIR", ".");
    const char* autostop = env_get("LEGORRR_AUTOSTOP_FRAMES", "0");
    long frame_cap = strtol(autostop, NULL, 10);
    if (frame_cap < 0) {
        frame_cap = 0;
    }
    /* CI/headless hook: open the pause overlay at this in-game frame (as if
     * Esc was pressed) so the PausedMenu can be screenshotted. */
    long autopause = strtol(env_get("LEGORRR_AUTOPAUSE_FRAMES", "0"), NULL, 10);

    FILE* log = fopen("legoRR-stub.log", "a");
    if (log == NULL) {
        log = stderr;
    }

    int mounted = resource_mount(wad_dir, resource_get_wad_prefix());
    resource_set_cd_root(wad_dir);
    fprintf(log, "frameloop: mounted %d wad(s) from %s\n", mounted, wad_dir);
    if (mounted == 0) {
        if (log != stderr) {
            fclose(log);
        }
        return 0;
    }

    /* These reconstructed contexts contain large fixed-capacity tables. The
     * reference keeps its corresponding state outside LegorrrRun's stack;
     * placing both port structs here creates a >1 MiB frame and overflows
     * the executable's 1 MiB PE stack before startup reaches this function. */
    GameContext* game_storage = (GameContext*)calloc(1, sizeof(GameContext));
    FrontEnd* fe_storage = (FrontEnd*)calloc(1, sizeof(FrontEnd));
    if (game_storage == NULL || fe_storage == NULL) {
        fprintf(log, "frameloop: could not allocate game/frontend contexts\n");
        free(game_storage);
        free(fe_storage);
        if (log != stderr) {
            fclose(log);
        }
        return 1;
    }
    GameContext& game = *game_storage;
    FrontEnd& fe = *fe_storage;

    /* --- Game layer: config DB + level load (findings 07/12/22) ---------- */
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

    EngineState state = { 0 };
    InputState  input = { 0 };
    input_reset(&input);
    /* The three view contexts (DAT_005560e4/e8/ec): free/cruise cameras with
     * a glide step; the demo has no world yet, so they tick quietly. */
    ViewContext views[3] = { 0 };
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
        0.0f, 1.0f, 0,
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
        frameloop_game_context_dispose(&game);
        free(game_storage);
        free(fe_storage);
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
        frameloop_game_context_dispose(&game);
        free(game_storage);
        free(fe_storage);
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
    AudioPlayer ap = { 0 };
    int audio_status = audio_open(&ap);
    int sfx_frames = 0;
    if (audio_status == 0) {
        /* Sound-system init succeeded: raise the bank gate, mirroring the
         * corpus's DAT_00533878 = 1 on the DirectSound-init success path
         * (0x47a766); the static stays 0 when audio is unavailable. */
        sound_bank_sound_on_set(1);
        /* Master gain from the store, not a fixed 0.6: the active save
         * context's restored SFX_Volume (or the default) applies immediately
         * (issue #85) — saved settings take effect without a menu nudge. */
        audio_set_volume(&ap, options_audio_gain(&game.options));
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

    long frames = 0;
    int thumb_slot = -1;   /* pending `.dat` thumbnail dump (see below) */
    int quit = 0;
    const char* stop = "closed";

    /* --- Boot movies (finding 14 §4 + finding 23 callbacks) ----------------
     * The corpus's boot hub (FUN_0041fb50) plays the LMI logo (`LegoAvi`)
     * then the intro (`RRAvi`) before the front-end, each gated by
     * AvisEnabled @ 0x414ce0 (the `DontPlayAvis` kill switch). Decoding is
     * software (src/gods98/movie.*, FFmpeg dlopen'd at runtime), so movies
     * play where the runtime libs exist; headless/CI runs
     * (LEGORRR_AUTOSTOP_FRAMES) and LEGORRR_NO_MOVIES skip them. */
    int avis_enabled = movie_avis_enabled(game.cfg);
    const char* no_movies = env_get("LEGORRR_NO_MOVIES", "");
    int play_movies = frame_cap == 0
                      && avis_enabled
                      && no_movies[0] == '\0'
                      && movie_available();
    if (play_movies) {
        char mp[256], os[512];
        if (movie_key_path(game.cfg, "LegoAvi", mp, sizeof(mp))
            && movie_path_resolve(wad_dir, mp, os, sizeof(os))) {
            movie_play_avi(os, "LegoAvi", win, &surf, &ap, &quit, log);
        } else {
            fprintf(log, "movieloop: no LegoAvi (%s)\n",
                    mp[0] != '\0' ? mp : "(no config key)");
        }
        if (!quit
            && movie_key_path(game.cfg, "RRAvi", mp, sizeof(mp))
            && movie_path_resolve(wad_dir, mp, os, sizeof(os))) {
            movie_play_avi(os, "RRAvi", win, &surf, &ap, &quit, log);
        }
        if (audio_status == 0 && !quit) {
            /* The movies silenced the ambient; bring it back. */
            audio_loop_named(&ap, "Sounds\\Iambloop.wav", 0.0f);
        }
    } else if (frame_cap == 0) {
        const char* reason = !avis_enabled ? "DontPlayAvis"
                              : no_movies[0] != '\0' ? "LEGORRR_NO_MOVIES"
                                                     : "FFmpeg runtime unavailable";
        fprintf(log, "movieloop: boot movies off (%s)\n", reason);
    }

    /* --- Front-end boot (finding 23): main menu before level load -----------
     * Ordinary launches enter the reconstructed front end when enabled.
     * LEGORRR_LEVEL remains a direct-launch override for development. */
    char level_name[FE_LEVEL_NAME_MAX];
    int boot_menu = 0, frontend_built = 0;
    if (game.cfg != NULL) {
        fprintf(log, "frameloop: config Lego.cfg loaded\n");
        int built = frontend_build(&fe, game.cfg);
        /* Functional options (issue #85): attach the live store so the menu
         * widgets show and set the effective values. frontend_build ran
         * with no store (defaults); sync seeds the loaded slot's values. */
        fe.opts = &game.options;
        frontend_options_sync(&fe);
        frontend_built = built;
        boot_menu = frontend_built && frontend_enabled(game.cfg)
                 && env_get("LEGORRR_LEVEL", "")[0] == '\0';
    } else {
        fprintf(log, "frameloop: Lego.cfg not found (wads need mounted game data)\n");
    }
    if (frontend_built && audio_status == 0) {
        frontend_set_audio(&fe, &ap);
    }
    snprintf(level_name, sizeof(level_name), "%s",
             env_get("LEGORRR_LEVEL", "Level01"));

    /* Runtime save/load slots (SaveMenuBuild @ 0x415a10 rows): state from
     * the on-disk slots, grafted onto the SaveLoad screens by the front
     * end. Slot slots run 1..SAVE_SLOTS-1 (5 is the default/autosave). */
    int pending_load_slot = -1, pending_load_record = -1;
    {
        FeSlot slots[FE_SLOT_COUNT];
        int sn = 0;
        for (int s = 1; s < SAVE_SLOTS; s++) {
            int filled = 0, completed = 0, resume = -1;
            save_slot_state(&game, s, &filled, &completed, &resume);
            memset(&slots[sn], 0, sizeof(slots[sn]));
            snprintf(slots[sn].label, FE_NAME_MAX, "Save Game %d", s);
            if (resume >= 0) {
                char level[FE_LEVEL_NAME_MAX];
                if (fe_level_for_record(&fe, resume, level, sizeof(level))) {
                    snprintf(slots[sn].status, FE_NAME_MAX,
                             "%s (%d complete)", level, completed);
                }
            } else if (filled > 0) {
                snprintf(slots[sn].status, FE_NAME_MAX,
                         "Saved game (%d records)", filled);
            }
            slots[sn].id = s;
            slots[sn].present = filled > 0;
            sn++;
        }
        frontend_build_slots(&fe, slots, sn);
    }
    if (boot_menu) {
        if (audio_status == 0) {
            const char* music = menu_music_resource(game.cfg);
            int music_rc = (music != NULL && music[0] != '\0')
                         ? audio_loop_named(&ap, music, 0.0f) : -1;
            if (music_rc == 0) {
                /* Unlike one-shot FLIC sounds, the continuous track only
                 * reaches the platform queue from audio_update(). */
                audio_update(&ap);
            }
            fprintf(log, "frameloop: main-menu music %s -> %s (%d)\n",
                    music != NULL ? music : "(missing SFX_AmbientMusicLoop)",
                    music_rc == 0 ? "queued" : "unavailable", music_rc);
        }
        menu_enter(&fe, 0);
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
            /* Bitmap fonts (finding-23 grid): HiFont = selected items,
             * LoFont = unselected; NULL falls back to the 3x5 face. The
             * Hi font also measures pointer hit-testing. */
            TextureSurface* hi_font = NULL, *lo_font = NULL;
            const FeSubmenu* fs = frontend_active_submenu(&fe);
            if (fs != NULL) {
                if (fs->hifont[0] != '\0') {
                    hi_font = texture_get_or_load(NULL, fs->hifont);
                }
                if (fs->lofont[0] != '\0') {
                    lo_font = texture_get_or_load(NULL, fs->lofont);
                }
            }
            FeInput fi;
            fi.up = input_key_edge(&input, PLATFORM_KEY_UP);
            fi.down = input_key_edge(&input, PLATFORM_KEY_DOWN);
            fi.enter = input_key_edge(&input, ' ')
                       || input_key_edge(&input, PLATFORM_KEY_ENTER);
            fi.esc = input_key_edge(&input, PLATFORM_KEY_ESC);
            fi.mouse_x = input.mouse_x;
            fi.mouse_y = input.mouse_y;
            fi.click = input_mouse_pressed(&input);
            fi.mdown = input.mouse_down;
            FeEvent ev = frontend_tick(&fe, &fi, hi_font);
            if (fe.option_changed) {
                const FeItem* oit = frontend_active_item(&fe);
                if (oit != NULL) {
                    int is_sfx = s_icontains(oit->label, "sfx")
                                 || s_icontains(oit->label, "sound");
                    int is_music = s_icontains(oit->label, "music");
                    int live = oit->kind == FE_ITEM_SLIDER
                               && (is_sfx || is_music) && oit->max > 0;
                    if (live && ap.dev != NULL) {
                        if (is_sfx) {
                            audio_menu_sfx_volume_set(oit->value);
                        } else {
                            audio_menu_volume_set(oit->value);
                        }
                    }
                    fprintf(log, "frameloop: option %s -> %d/%d (%s)\n",
                            oit->label, oit->value, oit->max,
                            live ? "audio" : "store — applied live");
                    fe.option_changed = 0;
                }
            }
            if (ev == FE_EVT_QUIT) {
                quit = 1;
                stop = "menu quit";
                break;
            }
            if (ev == FE_EVT_START_LEVEL) {
                snprintf(level_name, sizeof(level_name), "%s", fe.pending_level);
                /* record<->level mapping (port of CheckpointStep's
                 * list-index assignment): checkpoints during this level
                 * land in the level's own record, so mid-campaign saves
                 * neither collide nor resume short. */
                game.current_record = fe_record_for_level(&fe, level_name);
                if (game.current_record < 0) {
                    game.current_record = 0;
                }
                fprintf(log, "frameloop: front end picked level %s\n", level_name);
                break;
            }
            if (ev == FE_EVT_LOAD_GAME) {
                int filled = 0, completed = 0, resume = -1;
                if (save_slot_state(&game, fe.load_slot, &filled, &completed,
                                    &resume)) {
                    /* record index -> level (the inverse mapping — the
                     * port's level-tree stand-in for SavTreeLoad): resume
                     * the furthest completed record's level, else start
                     * the campaign fresh. */
                    pending_load_slot = fe.load_slot;
                    pending_load_record = resume >= 0 ? resume : 0;
                    if (resume >= 0
                        && fe_level_for_record(&fe, resume, level_name,
                                               sizeof(level_name))) {
                        /* mapped */
                    } else if (fe.campaign_n > 0) {
                        snprintf(level_name, sizeof(level_name), "%s",
                                 fe.campaign[0]);
                    } else {
                        snprintf(level_name, sizeof(level_name), "%s",
                                 "Level01");
                    }
                    game.save_slot = fe.load_slot;
                    game.current_record = pending_load_record;
                    fprintf(log, "frameloop: loading slot %d record %d -> "
                                 "level %s (%d/%d completed)\n",
                            pending_load_slot, pending_load_record,
                            level_name, completed, filled);
                } else {
                    snprintf(level_name, sizeof(level_name), "%s",
                             env_get("LEGORRR_LEVEL", "Level01"));
                    fprintf(log, "frameloop: slot %d absent — starting fresh "
                                 "at %s\n", fe.load_slot, level_name);
                }
                break;
            }
            if (ev == FE_EVT_CREDITS) {
                frameloop_credits(win, &surf, game.cfg, wad_dir, &ap,
                                  audio_status, &quit, log, &frames);
            }

            frameloop_menu_render(win, &surf, &fe, hi_font, lo_font, &game,
                                  (int)frames);
            if (audio_status == 0) {
                audio_update(&ap);
            }
            Sleep((DWORD)FRAME_MS);
            ++frames;
            input_begin_frame(&input);
        }
    }

    /* Restore the game's ambient loop after leaving the main menu. */
    if (boot_menu && audio_status == 0 && !quit) {
        audio_loop_named(&ap, "Sounds\\Iambloop.wav", 0.0f);
    }

    /* --- Deferred level load (the front end chose a level) ------------------ */
    if (!quit && game.cfg != NULL) {
        frameloop_load_level(log, wad_dir, play_movies, win, &surf, &ap,
                             audio_status, &quit, &game, &ctx, views,
                             level_name);
        if (pending_load_slot >= 0 && game.level != NULL) {
            /* Restore the saved checkpoint over the freshly-seeded task
             * table (the corpus's model is a per-level checkpoint, not an
             * entity dump — finding 08). The slot's records are still live
             * in ctx->records from save_slot_state. */
            uint32_t* fl = save_record_flags(&game, pending_load_record);
            if (fl != NULL) {
                checkpoint_blob_apply(&game, (const uint8_t*)(fl + 1));
                fprintf(log, "frameloop: restored checkpoint slot %d record "
                             "%d\n", pending_load_slot, pending_load_record);
            }
            /* Load-side `.osf` sidecar restore (LevelLoad @ 0x429890 →
             * OsfRead @ 0x459470): bring the slot's completed-objective
             * table back the way the shipped game does. The corpus gates
             * the read on a clean in-memory table (OsfDirtyGet()==0) and
             * an active slot (SavGetSlot()!=-1); the port mirrors it here
             * — after the checkpoint apply the slot is active and the
             * table has nothing unsaved, so the sidecar replaces it. */
            if (game.osf_dirty == 0) {
                (void)osf_read_path(&game, pending_load_slot);
                fprintf(log, "frameloop: restored .osf sidecar slot %d "
                             "(%d completed objective(s))\n",
                        pending_load_slot, game.osf_count);
            }
            pending_load_slot = -1;
        }
    }

    char mission_brief[1024] = { 0 };
    int mission_brief_active = game.level != NULL
        && pending_load_slot < 0
        && env_get("LEGORRR_SKIP_BRIEFING", "")[0] == '\0'
        && (s_icontains(level_name, "MoveTuto_01")
            || s_icontains(level_name, "Tutorial01"));
    TextureSurface* briefing_panel = NULL;
    TextureSurface* briefing_title_font = NULL;
    TextureSurface* briefing_body_font = NULL;
    if (mission_brief_active) {
        (void)frameloop_tutorial_brief_load(mission_brief,
                                            sizeof(mission_brief));
        briefing_panel = texture_get_or_load(
            NULL, "Interface\\BriefingPanel\\BriefingPanel.bmp");
        briefing_title_font = texture_get_or_load(
            NULL, "Interface\\Fonts\\MbriefFont2.bmp");
        briefing_body_font = texture_get_or_load(
            NULL, "Interface\\Fonts\\MbriefFont.bmp");
        int voice_frames = audio_status == 0
            ? audio_play_named(&ap,
                "Sounds\\Streamed\\Training\\Moving1\\mov1br.wav")
            : -1;
        fprintf(log, "frameloop: Mission Brief ready (%s, voice %s, %d frames)\n",
                mission_brief[0] != '\0' ? "ObjectiveText loaded"
                                          : "ObjectiveText unavailable",
                voice_frames >= 0 ? "played" : "unavailable", voice_frames);
    }

    const float dt = 1.0f / 30.0f;
    /* Game Speed (issue #85): options_scale_frame stamps the store's factor
     * into ctx.time_scale (EngineTick's subsystem tickers) and returns the
     * scaled dt for the game-layer tickers (ai/objective). */
    /* Interactive proof: the arrow keys pan the blitted texture. */
    int rock_x = 24, rock_y = 24;

    while (!quit) {
        /* 1. Device capture for this frame (the finding-05 "writer" half):
         *    platform events -> canonical codes -> input tables. */
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
        if (quit) {
            break;
        }
        if (frame_cap > 0 && frames >= frame_cap) {
            stop = "autostop";
            break;
        }

        /* The mission introduction holds simulation and accepts the same
         * simple continue input as the reference. On dismissal the next
         * frame enters normal gameplay without consuming that input twice. */
        if (mission_brief_active) {
            const int dismissed = input_key_edge(&input, ' ')
                || input_key_edge(&input, PLATFORM_KEY_ENTER)
                || input_mouse_pressed(&input);
            if (dismissed) {
                mission_brief_active = 0;
                fprintf(log, "frameloop: Mission Brief dismissed; controls live\n");
            } else {
                frameloop_briefing_render(win, &surf, briefing_panel,
                                          briefing_title_font,
                                          briefing_body_font, mission_brief,
                                          game.level, &game, (int)frames);
                if (audio_status == 0) audio_update(&ap);
                Sleep((DWORD)FRAME_MS);
                ++frames;
                input_begin_frame(&input);
                continue;
            }
            Sleep((DWORD)FRAME_MS);
            ++frames;
            input_begin_frame(&input);
            continue;
        }

        /* SaveMenuOpenFromReward is frame-driven in this port: once mode 4
         * opens the slot picker, consume frames here until a slot is saved or
         * the user backs out. The level sim remains paused under the menu. */
        if (fe.open && fe.save_action) {
            const FeSubmenu* fs = frontend_active_submenu(&fe);
            TextureSurface* hi_font = fs != NULL && fs->hifont[0] != '\0'
                                          ? texture_get_or_load(NULL, fs->hifont)
                                          : NULL;
            TextureSurface* lo_font = fs != NULL && fs->lofont[0] != '\0'
                                          ? texture_get_or_load(NULL, fs->lofont)
                                          : NULL;
            FeInput fi;
            fi.up = input_key_edge(&input, PLATFORM_KEY_UP);
            fi.down = input_key_edge(&input, PLATFORM_KEY_DOWN);
            fi.left = fi.right = 0;
            fi.enter = input_key_edge(&input, ' ')
                       || input_key_edge(&input, PLATFORM_KEY_ENTER);
            fi.esc = input_key_edge(&input, PLATFORM_KEY_ESC);
            fi.mouse_x = input.mouse_x;
            fi.mouse_y = input.mouse_y;
            fi.click = input_mouse_pressed(&input);
            fi.mdown = input.mouse_down;
            FeEvent save_ev = frontend_tick(&fe, &fi, hi_font);
            if (save_ev == FE_EVT_SAVE_GAME) {
                save_slot_set(&game, fe.load_slot);
                checkpoint_blob_build(&game);
                save_write_record(&game, game.current_record,
                                  game.checkpoint_blob);
                save_mark_record(&game, game.current_record);
                show_level_overlay_for_current_selection(&game);
                int filled = 0, completed = 0, resume = -1;
                if (save_slot_state(&game, fe.load_slot, &filled, &completed,
                                    &resume)) {
                    FeSlot saved_slot;
                    memset(&saved_slot, 0, sizeof(saved_slot));
                    saved_slot.id = fe.load_slot;
                    saved_slot.present = filled > 0;
                    snprintf(saved_slot.label, sizeof(saved_slot.label),
                             "Save Game %d", fe.load_slot);
                    if (resume >= 0) {
                        char level[FE_LEVEL_NAME_MAX];
                        if (fe_level_for_record(&fe, resume, level,
                                                sizeof(level))) {
                            snprintf(saved_slot.status,
                                     sizeof(saved_slot.status),
                                     "%s (%d complete)", level, completed);
                        }
                    } else if (filled > 0) {
                        snprintf(saved_slot.status,
                                 sizeof(saved_slot.status),
                                 "Saved game (%d records)", filled);
                    }
                    frontend_update_slot(&fe, &saved_slot);
                }
                mission_reward_set_active(&game, 0);
                thumb_slot = fe.load_slot;
                fprintf(log, "frameloop: reward checkpoint saved to slot %d\n",
                        fe.load_slot);
            }
            if (fe.open) {
                frameloop_menu_render(win, &surf, &fe, hi_font, lo_font,
                                      &game, (int)frames);
            }
            if (audio_status == 0) {
                audio_update(&ap);
            }
            Sleep((DWORD)FRAME_MS);
            ++frames;
            input_begin_frame(&input);
            continue;
        }

        /* Esc (or the LEGORRR_AUTOPAUSE_FRAMES CI hook) opens the in-game
         * pause overlay — the shipped PausedMenu — instead of quitting. */
        if (input_key_edge(&input, PLATFORM_KEY_ESC)
            || (autopause > 0 && frames == autopause)) {
            snprintf(fe.resume_level, sizeof(fe.resume_level), "%s",
                     level_name);
            menu_enter(&fe, 2);
            FeOverlayMenuGateState overlay_gate;
            memset(&overlay_gate, 0, sizeof(overlay_gate));
            overlay_gate.cached_menu = -1;
            overlay_gate.menus[0].entries[0].handle = 1;
            overlay_gate.menus[0].entries[2].handle = 1;
            fprintf(log, "frameloop: paused (Continue / Options / Restart / "
                         "Quit)\n");
            int want_reload = 0;
            while (fe.open && !quit) {
                PlatformEvent pev[32];
                int pn = platform_pump_events(win, pev, 32);
                input_capture_events(&input, pev, pn);
                for (int i = 0; i < pn; ++i) {
                    if (pev[i].kind == PLATFORM_EVENT_QUIT) {
                        quit = 1;
                        stop = "closed";
                        break;
                    }
                }
                if (quit) {
                    break;
                }
                if (frame_cap > 0 && frames >= frame_cap) {
                    quit = 1;
                    stop = "autostop";
                    break;
                }
                const FeSubmenu* fs = frontend_active_submenu(&fe);
                TextureSurface* hi_font = NULL, *lo_font = NULL;
                if (fs != NULL) {
                    if (fs->hifont[0] != '\0') {
                        hi_font = texture_get_or_load(NULL, fs->hifont);
                    }
                    if (fs->lofont[0] != '\0') {
                        lo_font = texture_get_or_load(NULL, fs->lofont);
                    }
                }
                FeInput fi;
                fi.up = input_key_edge(&input, PLATFORM_KEY_UP);
                fi.down = input_key_edge(&input, PLATFORM_KEY_DOWN);
                fi.enter = input_key_edge(&input, ' ')
                           || input_key_edge(&input, PLATFORM_KEY_ENTER);
                fi.esc = input_key_edge(&input, PLATFORM_KEY_ESC);
                fi.mouse_x = input.mouse_x;
                fi.mouse_y = input.mouse_y;
                fi.click = input_mouse_pressed(&input);
                fi.mdown = input.mouse_down;
                FrameloopOverlayMenuContext gate_ctx;
                gate_ctx.fe = &fe;
                gate_ctx.game = &game;
                gate_ctx.gate = &overlay_gate;
                gate_ctx.input = &fi;
                gate_ctx.measure = hi_font;
                gate_ctx.event = FE_EVT_NONE;
                gate_ctx.mode = 0;
                overlay_gate.level_start_flag = level_start_flag();
                overlay_gate.options_flag = menu_options_flag();
                overlay_gate.blocked =
                    global_option_low_bits_enabled(game.level);
                (void)overlay_menu_gate(&overlay_gate, 0,
                                        frameloop_overlay_menu_scroll_refresh,
                                        frameloop_overlay_menu_tick, &gate_ctx);
                FeEvent ev = gate_ctx.event;
                if (fe.option_changed) {
                    const FeItem* oit = frontend_active_item(&fe);
                    if (oit != NULL) {
                        int is_sfx = s_icontains(oit->label, "sfx")
                                     || s_icontains(oit->label, "sound");
                        int is_music = s_icontains(oit->label, "music");
                        int live = oit->kind == FE_ITEM_SLIDER
                                   && (is_sfx || is_music) && oit->max > 0;
                        if (live && ap.dev != NULL) {
                            if (is_sfx) {
                                audio_menu_sfx_volume_set(oit->value);
                            } else {
                                audio_menu_volume_set(oit->value);
                            }
                        }
                        fprintf(log, "frameloop: option %s -> %d/%d (%s)\n",
                                oit->label, oit->value, oit->max,
                                live ? "audio" : "store — applied live");
                        fe.option_changed = 0;
                    }
                }
                if (ev == FE_EVT_QUIT) {
                    quit = 1;
                    stop = "menu quit";
                    break;
                }
                if (ev == FE_EVT_RESUME) {
                    break;
                }
                if (ev == FE_EVT_START_LEVEL) {
                    want_reload = 1;
                    break;
                }
                frameloop_menu_render(win, &surf, &fe, hi_font, lo_font,
                                      &game, (int)frames);
                Sleep((DWORD)FRAME_MS);
                ++frames;
                input_begin_frame(&input);
            }
            if (want_reload && !quit) {
                /* Pause -> Restart_Mission: reload the level in place. */
                fprintf(log, "frameloop: restarting level %s\n", level_name);
                level_destroy(game.level);
                game.level = NULL;
                frameloop_load_level(log, wad_dir, play_movies, win, &surf,
                                     &ap, audio_status, &quit, &game, &ctx,
                                     views, level_name);
                /* The corpus re-enters LevelLoad for this reload through
                 * the in-game callback registered by FUN_0041fa20
                 * (FUN_0041fb50 calls FUN_00429890), so the `.osf` sidecar
                 * read runs again under the same gate: clean table +
                 * active slot -> re-read; dirty -> skipped and the
                 * unsaved live table survives untouched. */
                if (game.level != NULL && game.osf_dirty == 0
                    && save_slot_get(&game) >= 0) {
                    (void)osf_read_path(&game, save_slot_get(&game));
                }
            }
            if (!quit && game.level == NULL) {
                /* not restarting; Esc from the pause root resumes */
                fprintf(log, "frameloop: resumed\n");
            }
            /* The pause consumed this whole frame (its own loop rendered).
             * Continue to the next in-game frame without running the game
             * sim or the level renderer over the menu. */
            if (!quit || game.level != NULL) {
                Sleep((DWORD)FRAME_MS);
                input_begin_frame(&input);
                continue;
            }
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
            save_write_record(&game, game.current_record,
                              game.checkpoint_blob);
            save_mark_record(&game, game.current_record);
            save_commit(&game);
            fprintf(log, "frameloop: saved blob to slot %d record %d (0x%x bytes)\n",
                    SAVE_DEFAULT_SLOT, game.current_record, SAVE_BLOB_SIZE);
        }
        if (game.level != NULL && input_key_edge(&input, 'l')) {
            game.save_slot = -1; /* force the bootstrap path */
            checkpoint_step(&game);
            fprintf(log, "frameloop: checkpoint_step committed slot %d record %d\n",
                    SAVE_DEFAULT_SLOT, game.current_record);
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
            float cx = 0.0f, cy = 0.0f;
            int mouse_on_terrain = level_scene_mouse_cell(
                &game, game.level, WINDOW_W, WINDOW_H,
                input.mouse_x, input.mouse_y,
                &cx, &cy);
            if (!mouse_on_terrain) {
                cx = lw > 0 ? (float)input.mouse_x / (float)WINDOW_W * (float)lw : 0.0f;
                cy = lh > 0 ? (float)input.mouse_y / (float)WINDOW_H * (float)lh : 0.0f;
            }

            /* Left-click a Raider to select it; click open ground to send
             * the selected group there. Both operations use the rendered
             * camera projection, so the pointer agrees with what is shown. */
            if (input_mouse_pressed(&input)) {
                EntitySlot* hit = level_scene_pick_entity(
                    &game, game.level, WINDOW_W, WINDOW_H,
                    input.mouse_x, input.mouse_y, 30.0f);
                if (hit != NULL) {
                    selection_clear(&game);
                    selection_add(&game, hit);
                    fprintf(log, "frameloop: selected Raider %s\n", hit->name);
                } else if (mouse_on_terrain && selection_has_any(&game)) {
                    int n = selection_cmd_move(&game, cx, cy);
                    fprintf(log, "frameloop: click move %d selected -> "
                                 "cell (%.1f,%.1f)\n", n, cx, cy);
                }
            }

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
            float sdt = options_scale_frame(&game.options, &ctx, dt);
            int ai_done = ai_tick(&game, sdt);
            if (ai_done != 0) {
                fprintf(log, "frameloop: ai commands completed: %d\n", ai_done);
            }
            int done = mission_update(&game);
            if (done != 0) {
                fprintf(log, "frameloop: tasks completed this frame (mask 0x%x)\n",
                        done);
            }
            int resolved = objective_tick(game.level, sdt);
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
                            if (save_menu_open_from_reward(&fe)) {
                                fprintf(log, "frameloop: reward Save clicked -> "
                                             "save-slot menu opened\n");
                            }
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
                level_render_scene(game.level, &game, WINDOW_W, WINDOW_H,
                                   pixels, pitch);
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
            /* Help Window (issue #85): the option toggles the shipped help
             * panel over the level; the advisor stream text leads when the
             * announcer has one. frontend_help_overlay holds the On/Off
             * gate — this call is unconditional. */
            if (game.level != NULL) {
                frontend_help_overlay(pixels, pitch, WINDOW_W, WINDOW_H,
                                      objective_stream(game.level),
                                      &game.options);
                if ((s_icontains(level_name, "MoveTuto_01")
                     || s_icontains(level_name, "Tutorial01"))
                    && briefing_body_font != NULL) {
                    uint8_t* frame = (uint8_t*)pixels;
                    for (int y = WINDOW_H - 22; y < WINDOW_H - 2; ++y) {
                        uint8_t* row = frame + (size_t)y * pitch;
                        for (int x = 0; x < WINDOW_W; ++x) {
                            row[(size_t)x * 4 + 0] = 8;
                            row[(size_t)x * 4 + 1] = 18;
                            row[(size_t)x * 4 + 2] = 10;
                            row[(size_t)x * 4 + 3] = 255;
                        }
                    }
                    formatted_text_draw(briefing_body_font, pixels, pitch,
                        WINDOW_W, WINDOW_H, 12, WINDOW_H - 19,
                        "Click Raider then cave to move. Keys 1-4 select, M move. Esc pauses.");
                }
            }
            /* Brightness (issue #85): BrightnessSet's scalar on the frame. */
            render_apply_brightness(pixels, pitch, WINDOW_W, WINDOW_H,
                                    game.options.brightness);
            platform_screenshot(win, pixels, pitch, WINDOW_W, WINDOW_H,
                                (int)frames);
            platform_present_pixels(win, pixels, pitch, WINDOW_W, WINDOW_H);
            if (thumb_slot >= 0) {
                /* Save-confirm thumbnail (FUN_0047e440): dump the frame we
                 * just presented to Saves\<slot>.dat as a 24bpp BMP. */
                save_write_thumbnail(&game, thumb_slot, pixels, pitch,
                                     WINDOW_W, WINDOW_H);
                frontend_save_thumbnail_invalidate(&fe, thumb_slot);
                fprintf(log, "frameloop: .dat thumbnail written (slot %d)\n",
                        thumb_slot);
                thumb_slot = -1;
            }
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

    frontend_close(&fe);
    frameloop_game_context_dispose(&game);
    free(fe_storage);
    free(game_storage);

    if (log != stderr) {
        fclose(log);
    }
    return 0;
}
