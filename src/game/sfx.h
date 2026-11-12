// SPDX-License-Identifier: LGPL-3.0-or-later
/* SFX.c (original translation unit GAME-041, finding 505): sound effect
 * ids, sample chains, the global sample, the play queue and the queue
 * mode; and the smoke emitters (finding 585).
 *
 * C-compatible header (ADR 0005). sfx.c is compiled as C in the matching
 * build and as C++ in the native port. Shared C++ headers do not include this
 * file: every include and declaration a header adds shifts MSVC's symbol
 * numbering (finding 506). src/game/test_c_boundaries.cpp includes it.
 *
 * In the matching build sfx_link_names.h, which sfx.c includes first, gives
 * these functions the port's names (sound.hpp, effects.hpp, crt.hpp), so
 * every caller reaches them. Natively they keep these names and nothing
 * calls them: the port tail sfx_port.cpp serves the port's names. */
#ifndef LEGORRR_GAME_SFX_H
#define LEGORRR_GAME_SFX_H

#ifdef __cplusplus
extern "C" {
#endif

struct SFX_Vector3F;
struct Smoke;

void sfx_initialise(void);                                        /* 00464740 */
void sfx_container_sound_trigger_callback(const char* sampleName,
                                         void* cont, void* data); /* 00464c20 */
void sfx_set_sample_populate_mode(int on);                        /* 00464c50 */
int sfx_get_type(const char* sfxName, int* sfxID);                /* 00464c70 */
int sfx_load_sample_property(char* value, int sfxID);             /* 00464d00 */
int sfx_random_get_sound3d_handle(int sfxID);                     /* 00464e20 */
void sfx_stop_global_sample(void);                                /* 00464e80 */
int sfx_set_global_sample_duration_if_le0(float duration);        /* 00464ec0 */
int sfx_random_set_and_play_global_sample(int sfxID, int* handle); /* 00464ef0 */
void sfx_add_to_queue(int sfxID, int mode);                       /* 00464f60 */
int sfx_random_play_sound_normal(int sfxID, int loop);            /* 00464fa0 */
void sfx_random_set_buffer_volume(int sfxID, int volume);         /* 00465010 */
int sfx_random_get_buffer_volume(int sfxID);                      /* 00465030 */
int sfx_random_play_sound3d_on_container(void* cont, int sfxID, int loop,
                                         int onCont, float* wPos); /* 00465050 */
int sfx_random_play_sound3d_on_frame_internal(void* frame, int sfxID,
                                              int loop, int onCont,
                                              float* wPos);       /* 00465090 */
float sfx_random_get_sample_play_time(int sfxID);                 /* 00465160 */
void sfx_sound3d_stop_sound(int handle);                          /* 00465190 */
void sfx_update(float elapsed);                                   /* 004651a0 */
void sfx_sound3d_update(void);                                    /* 00465250 */
void sfx_set_sound_on(int on, int stopAll);                       /* 00465260 */
void sfx_set_sound_on_and_stop_all(int on);                       /* 004652a0 */
int sfx_is_queue_mode(void);                                      /* 004652b0 */
void sfx_set_queue_mode_and_flush(int on);                        /* 004652c0 */
void sfx_set_queue_mode(int on, int flushQueued);                 /* 004652d0 */
int sfx_is_sound_on(void);                                        /* 00465370 */

int smoke_initialise(void);                                       /* 00465380 */
void smoke_load_textures(const char* dirName, const char* baseName,
                         unsigned int textureCount);              /* 004653a0 */
struct Smoke* smoke_create_smoke_area(void* cont, int bx, int by,
                                      struct SFX_Vector3F* dir, float r,
                                      float g, float b, float density,
                                      float speed, int type,
                                      unsigned int seed, int sfxID); /* 00465430 */
void smoke_remove_all(void);                                      /* 00465970 */
void smoke_remove(struct Smoke* smoke, void* cont);               /* 004659b0 */
void smoke_hide_all(int hide);                                    /* 00465a90 */
void smoke_hide(struct Smoke* smoke, int hide);                   /* 00465ac0 */
void smoke_update(float elapsed);                                 /* 00465b00 */
void smoke_group_show(struct Smoke* smoke, unsigned int groupID); /* 00465c50 */
void smoke_group_update(struct Smoke* smoke, unsigned int groupID,
                        float elapsed);                           /* 00465e00 */
void smoke_mesh_render_callback(void* mesh, struct Smoke* smoke,
                                void* viewport);                  /* 00465ee0 */
void smoke_group_set_vertices(struct Smoke* smoke, unsigned int groupID,
                              void* viewport,
                              struct SFX_Vector3F* camPos);       /* 00465f40 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_SFX_H */
