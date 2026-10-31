// SPDX-License-Identifier: LGPL-3.0-or-later
#ifndef LEGORRR_GAME_VIDEO_GRAPH_HPP
#define LEGORRR_GAME_VIDEO_GRAPH_HPP

#include <stdint.h>

namespace lego {

struct DirectShowGraphState {
    void* interfaces[14];
    char* path;
    int32_t last_status;
    uint32_t duration_100ns;
    uint32_t frame_bound;
    uint32_t surface_width;
    uint32_t surface_height;
};

struct DirectShowSurfaceDescription {
    uint32_t size;
    uint32_t width;
    uint32_t height;
    uint32_t format;
};

typedef int (*DirectShowQueryHook)(void* source, uint32_t stage,
                                   void** result_out);
typedef void (*DirectShowReleaseHook)(void* interface_pointer);
typedef int (*DirectShowGraphBuildHook)(DirectShowGraphState* state,
                                        const char* path, void* owner,
                                        void* context);
typedef int (*DirectShowSampleHook)(DirectShowGraphState* state,
                                    void* source, void* destination);
typedef void (*DirectShowDurationHook)(DirectShowGraphState* state,
                                       uint32_t* duration_100ns);
typedef int (*DirectShowSurfaceHook)(DirectShowGraphState* state,
                                     DirectShowSurfaceDescription* desc);

void directshow_graph_hooks_set(DirectShowQueryHook query,
                                DirectShowReleaseHook release,
                                DirectShowGraphBuildHook build,
                                DirectShowSampleHook sample,
                                DirectShowDurationHook duration,
                                DirectShowSurfaceHook surface);
int directshow_sample_setup(DirectShowGraphState* state, void* root);
int configured_video_graph_setup(DirectShowGraphState* state, const char* path,
                                void* owner, void* context);
void configured_video_com_route_init(DirectShowGraphState* state,
                                     const char* path, void* owner,
                                     void* context);
void directshow_interface_release(DirectShowGraphState* state);
int configured_video_sample_update(DirectShowGraphState* state,
                                   void* source, void* destination);
void video_duration_frame_bound_update(DirectShowGraphState* state);
DirectShowGraphState* directshow_render_surface_create(const char* path);
void directdraw_surface_description_get(DirectShowGraphState* state,
                                        DirectShowSurfaceDescription* desc);
void video_frame_bound_refresh(DirectShowGraphState* state);
int directshow_sample_update_and_blit(DirectShowGraphState* state,
                                      void* source, void* destination);
void directshow_wrapper_destroy(DirectShowGraphState* state);

} // namespace lego

#endif
