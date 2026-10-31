// SPDX-License-Identifier: LGPL-3.0-or-later
#include "video_graph.hpp"

#include "../gods98/heap.hpp"

#include <string.h>

namespace lego {

namespace {

DirectShowQueryHook s_query_hook;
DirectShowReleaseHook s_release_hook;
DirectShowGraphBuildHook s_build_hook;
DirectShowSampleHook s_sample_hook;
DirectShowDurationHook s_duration_hook;
DirectShowSurfaceHook s_surface_hook;

void directshow_partial_release(DirectShowGraphState* state)
{
    static const unsigned int indices[] = { 2, 3, 6, 4, 5, 0 };
    unsigned int i;
    if (state == 0 || s_release_hook == 0) return;
    for (i = 0; i < sizeof(indices) / sizeof(indices[0]); ++i) {
        unsigned int index = indices[i];
        if (state->interfaces[index] != 0) {
            s_release_hook(state->interfaces[index]);
            state->interfaces[index] = 0;
        }
    }
}

char* copy_string(const char* value)
{
    unsigned int length = 0;
    char* result;
    if (value == 0) return 0;
    while (value[length] != '\0' && length < 32767u) ++length;
    result = (char*)engine_heap_alloc((long)length + 1);
    if (result == 0) return 0;
    memcpy(result, value, length);
    result[length] = '\0';
    return result;
}

} // namespace

void directshow_graph_hooks_set(DirectShowQueryHook query,
                                DirectShowReleaseHook release,
                                DirectShowGraphBuildHook build,
                                DirectShowSampleHook sample,
                                DirectShowDurationHook duration,
                                DirectShowSurfaceHook surface)
{
    s_query_hook = query;
    s_release_hook = release;
    s_build_hook = build;
    s_sample_hook = sample;
    s_duration_hook = duration;
    s_surface_hook = surface;
}

// FUNCTION: LEGORR 0x004721E0
int directshow_sample_setup(DirectShowGraphState* state, void* root)
{
    static const unsigned int targets[] = { 2, 3, 6, 4, 5 };
    void* source;
    unsigned int i;
    if (state == 0 || root == 0 || s_query_hook == 0) return 0;
    source = root;
    for (i = 0; i < sizeof(targets) / sizeof(targets[0]); ++i) {
        unsigned int target = targets[i];
        state->last_status = s_query_hook(source, i, &state->interfaces[target]);
        if (state->last_status < 0 || state->interfaces[target] == 0) {
            directshow_partial_release(state);
            return 0;
        }
        source = state->interfaces[target];
    }
    state->last_status = s_query_hook(state->interfaces[7], 5,
                                      &state->interfaces[8]);
    if (state->last_status < 0 || state->interfaces[0] == 0) {
        directshow_partial_release(state);
        return 0;
    }
    return 1;
}

// FUNCTION: LEGORR 0x004722B0
int configured_video_graph_setup(DirectShowGraphState* state, const char* path,
                                 void* owner, void* context)
{
    if (state == 0 || path == 0 || s_build_hook == 0) return 0;
    state->last_status = s_build_hook(state, path, owner, context);
    return state->last_status >= 0;
}

// FUNCTION: LEGORR 0x00472390
void configured_video_com_route_init(DirectShowGraphState* state,
                                     const char* path, void* owner,
                                     void* context)
{
    if (state == 0) return;
    memset(state, 0, sizeof(*state));
    state->interfaces[7] = owner;
    state->interfaces[13] = context;
    state->path = copy_string(path);
    if (path == 0 || state->path == 0) {
        state->last_status = -1;
        return;
    }
    if (configured_video_graph_setup(state, state->path, owner, context) &&
        state->interfaces[0] != 0) {
        directshow_sample_setup(state, state->interfaces[0]);
    }
}

// FUNCTION: LEGORR 0x00472430
void directshow_interface_release(DirectShowGraphState* state)
{
    static const unsigned int indices[] = { 2, 3, 6, 4, 7, 13, 0 };
    unsigned int i;
    if (state == 0) return;
    if (s_release_hook != 0) {
        for (i = 0; i < sizeof(indices) / sizeof(indices[0]); ++i) {
            unsigned int index = indices[i];
            if (state->interfaces[index] != 0) {
                s_release_hook(state->interfaces[index]);
                state->interfaces[index] = 0;
            }
        }
    }
    if (state->path != 0) {
        engine_heap_free(state->path);
        state->path = 0;
    }
}

// FUNCTION: LEGORR 0x004724A0
int configured_video_sample_update(DirectShowGraphState* state,
                                   void* source, void* destination)
{
    if (state == 0 || state->interfaces[6] == 0 || s_sample_hook == 0)
        return 0;
    state->last_status = s_sample_hook(state, source, destination);
    if (state->last_status == 0) return 1;
    directshow_partial_release(state);
    return 0;
}

// FUNCTION: LEGORR 0x00472530
void video_duration_frame_bound_update(DirectShowGraphState* state)
{
    if (state == 0 || s_duration_hook == 0) return;
    s_duration_hook(state, &state->duration_100ns);
    state->frame_bound = state->duration_100ns / 10000u;
}

// FUNCTION: LEGORR 0x00472560
DirectShowGraphState* directshow_render_surface_create(const char* path)
{
    DirectShowGraphState* state;
    if (path == 0) return 0;
    state = (DirectShowGraphState*)engine_heap_alloc(
        (long)sizeof(DirectShowGraphState));
    if (state == 0) return 0;
    memset(state, 0, sizeof(*state));
    configured_video_com_route_init(state, path, 0, 0);
    if (state->last_status < 0) {
        directshow_wrapper_destroy(state);
        return 0;
    }
    return state;
}

// FUNCTION: LEGORR 0x00472670
void directdraw_surface_description_get(DirectShowGraphState* state,
                                        DirectShowSurfaceDescription* desc)
{
    if (desc == 0) return;
    memset(desc, 0, sizeof(*desc));
    desc->size = 0x6c;
    if (state == 0 || s_surface_hook == 0) return;
    state->last_status = s_surface_hook(state, desc);
    if (state->last_status >= 0) {
        state->surface_width = desc->width;
        state->surface_height = desc->height;
    }
}

// FUNCTION: LEGORR 0x004726C0
void video_frame_bound_refresh(DirectShowGraphState* state)
{
    video_duration_frame_bound_update(state);
}

// FUNCTION: LEGORR 0x004726D0
int directshow_sample_update_and_blit(DirectShowGraphState* state,
                                      void* source, void* destination)
{
    return configured_video_sample_update(state, source, destination);
}

// FUNCTION: LEGORR 0x004726F0
void directshow_wrapper_destroy(DirectShowGraphState* state)
{
    if (state == 0) return;
    directshow_interface_release(state);
    engine_heap_free(state);
}

} // namespace lego
