// SPDX-License-Identifier: LGPL-3.0-or-later
/* Authored LWSC1 producer. See lwsc_scene.hpp for backend and lifetime seams.
 * Reference: 0x4869f0, 0x487ba0/cb0/fc0, 0x488170/2e0/7c0/900/9a0.
 * Host records deliberately do not impersonate their fixed x86 layouts. */
#include "lwsc_scene.hpp"
#include "resource.hpp"
#include "string.hpp"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <float.h>
#include <errno.h>

struct LwscSceneTemplate {
    unsigned int references;
    unsigned int first_frame, last_frame;
    float fps;
    char path[LWSC_PATH_CAPACITY];
    char directory[LWSC_PATH_CAPACITY];
    LwscNode* nodes;
    unsigned int count, capacity, first_root;
    unsigned int* order;           /* depth-first source indices */
    unsigned int total_keys;
};

typedef struct LwscNameEntry {
    char* key;
    char* name;
} LwscNameEntry;

struct LwscAssemblyTemplate {
    unsigned int references;
    EntityAssembly* metadata;
    LwscSceneTemplate** scenes;
    LwscNameEntry* names;
    unsigned int name_count;
};

typedef struct LwscActivityInstance {
    LwscFrameRef root;
    LwscFrameRef* frames;          /* source order, each owns a real reference */
    LwscMeshRef* meshes;
    float time;
} LwscActivityInstance;

struct LwscAssemblyInstance {
    LwscAssemblyTemplate* assembly;
    LwscBackendOps ops;
    LwscFrameRef root;
    LwscActivityInstance* activities;
    unsigned int current;
    int rendering;
    int valid;
};

typedef struct LwscReader {
    const char* cursor;
    const char* end;
    unsigned int line;
    LwscError* error;
} LwscReader;

static int lwsc_error(LwscError* error, unsigned int line, const char* text)
{
    if (error != NULL) {
        error->line = line;
        strncpy(error->message, text, sizeof(error->message) - 1u);
        error->message[sizeof(error->message) - 1u] = '\0';
    }
    return 0;
}

static void lwsc_error_clear(LwscError* error)
{
    if (error != NULL) memset(error, 0, sizeof(*error));
}

static int lwsc_finite(float value)
{
    return value == value && value <= FLT_MAX && value >= -FLT_MAX;
}

static char* lwsc_copy(const char* text)
{
    size_t size = strlen(text) + 1u;
    char* result = (char*)malloc(size);
    if (result != NULL) memcpy(result, text, size);
    return result;
}

static int lwsc_space(char c)
{
    return c == ' ' || c == '\t' || c == '\r';
}

/* 1 = nonempty line, 0 = EOF, -1 = bad/overlong line. No strtok state. */
static int lwsc_line(LwscReader* reader, char line[1024])
{
    while (reader->cursor < reader->end) {
        const char* first = reader->cursor;
        const char* last;
        size_t length;
        while (reader->cursor < reader->end && *reader->cursor != '\n') {
            if (*reader->cursor == '\0') {
                lwsc_error(reader->error, reader->line + 1u, "embedded NUL");
                return -1;
            }
            ++reader->cursor;
        }
        last = reader->cursor;
        if (reader->cursor < reader->end) ++reader->cursor;
        ++reader->line;
        if ((size_t)(last - first) >= 1024u) {
            lwsc_error(reader->error, reader->line, "line exceeds 1023 bytes");
            return -1;
        }
        while (first < last && lwsc_space(*first)) ++first;
        while (last > first && lwsc_space(last[-1])) --last;
        length = (size_t)(last - first);
        if (length >= 1024u) {
            lwsc_error(reader->error, reader->line, "line exceeds 1023 bytes");
            return -1;
        }
        if (length == 0u) continue;
        memcpy(line, first, length);
        line[length] = '\0';
        return 1;
    }
    return 0;
}

static int lwsc_required_line(LwscReader* reader, char line[1024])
{
    int result = lwsc_line(reader, line);
    if (result == 1) return 1;
    if (result == 0) lwsc_error(reader->error, reader->line, "unexpected EOF");
    return 0;
}

static char* lwsc_argument(char* line)
{
    while (*line != '\0' && !lwsc_space(*line)) ++line;
    if (*line != '\0') *line++ = '\0';
    while (lwsc_space(*line)) ++line;
    return line;
}

static int lwsc_unsigned(const char* text, unsigned int limit,
                          unsigned int* value)
{
    char* end;
    unsigned long result;
    if (*text < '0' || *text > '9') return 0;
    errno = 0;
    result = strtoul(text, &end, 10);
    if (errno == ERANGE || *end != '\0' || result > limit) return 0;
    *value = (unsigned int)result;
    return 1;
}

static int lwsc_numbers(const char* text, float* values, unsigned int count)
{
    unsigned int i;
    for (i = 0u; i < count; ++i) {
        char* end;
        double value;
        while (lwsc_space(*text)) ++text;
        if (*text == '\0') return 0;
        errno = 0;
        value = strtod(text, &end);
        if (text == end || errno == ERANGE || value != value ||
            value > FLT_MAX || value < -FLT_MAX) return 0;
        values[i] = (float)value;
        text = end;
        if (*text != '\0' && !lwsc_space(*text)) return 0;
    }
    while (lwsc_space(*text)) ++text;
    return *text == '\0';
}

static int lwsc_key_metadata(const char* text, uint16_t* frame, int* linear,
    float* tension, float* continuity, float* bias)
{
    float values[5];
    if (!lwsc_numbers(text, values, 5u) || values[0] < 0.0f ||
        values[0] > 65535.0f || values[0] != (float)(unsigned int)values[0] ||
        (values[1] != 0.0f && values[1] != 1.0f)) return 0;
    *frame = (uint16_t)values[0];
    *linear = (int)values[1];
    *tension = values[2]; *continuity = values[3]; *bias = values[4];
    return 1;
}

static int lwsc_native_name(char* output, size_t capacity, const char* base,
                             unsigned int duplicate, int sanitize)
{
    size_t length = strlen(base);
    size_t i;
    char digits[16];
    /* Bounded before sprintf; at most four digits under LWSC_MAX_NODES. */
    if (length + 16u >= capacity || duplicate >= LWSC_MAX_NODES) return 0;
    sprintf(digits, "_%0.2u", duplicate);
    memcpy(output, base, length);
    strcpy(output + length, digits);
    /* Retail _isctype mask 0x157 is printable, not merely alnum/_ .
     * ASCII is deterministic; reject locale-dependent bytes at parse time. */
    for (i = 0u; output[i] != '\0'; ++i) {
        unsigned char c = (unsigned char)output[i];
        if (c > 127u) return 0;
        if (sanitize && (c < 32u || c == 127u)) output[i] = '_';
    }
    return 1;
}

static int lwsc_node_add(LwscSceneTemplate* scene, const char* reference,
                          int is_null, LwscReader* reader)
{
    LwscNode* node;
    const char* base = reference;
    const char* p;
    size_t length;
    unsigned int i;
    if (*reference == '\0' || strlen(reference) >= 1024u ||
        scene->count >= LWSC_MAX_NODES)
        return lwsc_error(reader->error, reader->line, "invalid object name/count");
    if (scene->count == scene->capacity) {
        unsigned int capacity = scene->capacity + 32u;
        LwscNode* nodes = (LwscNode*)realloc(scene->nodes,
                                             capacity * sizeof(*nodes));
        if (nodes == NULL)
            return lwsc_error(reader->error, reader->line, "object allocation failed");
        memset(nodes + scene->capacity, 0,
               (capacity - scene->capacity) * sizeof(*nodes));
        scene->nodes = nodes;
        scene->capacity = capacity;
    }
    node = &scene->nodes[scene->count++];
    node->is_null = is_null != 0;
    node->flags = is_null ? 1u : 0u;
    node->authored_reference = lwsc_copy(reference);
    if (!is_null) {
        for (p = reference; *p != '\0'; ++p)
            if (*p == '\\' || *p == '/') base = p + 1;
    }
    length = strlen(base);
    if (!is_null) {
        if (length <= 4u || strcasecmp_lite(base + length - 4u, ".lwo") != 0)
            return lwsc_error(reader->error, reader->line, "LoadObject requires .lwo");
        length -= 4u;
    }
    node->base_name = (char*)malloc(length + 1u);
    if (node->base_name == NULL || node->authored_reference == NULL)
        return lwsc_error(reader->error, reader->line, "name allocation failed");
    memcpy(node->base_name, base, length);
    node->base_name[length] = '\0';
    /* Search backwards exactly as 0x4869f0; case differences are distinct. */
    for (i = scene->count - 1u; i > 0u; --i) {
        const LwscNode* previous = &scene->nodes[i - 1u];
        if (previous->is_null == node->is_null &&
            strcmp(previous->base_name, node->base_name) == 0) {
            node->duplicate = previous->duplicate + 1u;
            break;
        }
    }
    if (!lwsc_native_name(node->native_name, sizeof(node->native_name),
                           node->base_name, node->duplicate, 1))
        return lwsc_error(reader->error, reader->line, "unsupported native name");
    return 1;
}

static int lwsc_track(LwscSceneTemplate* scene, LwscNode* node,
                       LwscReader* reader, int dissolve)
{
    char line[1024];
    unsigned int channels, count, i;
    if ((!dissolve && node->keys != NULL) ||
        (dissolve && (node->dissolve_keys != NULL || node->has_constant_dissolve)))
        return lwsc_error(reader->error, reader->line, "duplicate track");
    if (!lwsc_required_line(reader, line) ||
        !lwsc_unsigned(line, 9u, &channels) || channels != (dissolve ? 1u : 9u) ||
        !lwsc_required_line(reader, line) ||
        !lwsc_unsigned(line, 65534u, &count) || count == 0u ||
        scene->total_keys + count + 1u > LWSC_MAX_KEYS)
        return lwsc_error(reader->error, reader->line, "invalid track channels/count");
    scene->total_keys += count + 1u;
    if (dissolve) {
        node->dissolve_keys = (LwscDissolveKey*)calloc(count + 1u,
                                                     sizeof(LwscDissolveKey));
        node->authored_dissolve_count = node->dissolve_count = count;
        if (node->dissolve_keys == NULL)
            return lwsc_error(reader->error, reader->line, "track allocation failed");
    } else {
        node->keys = (LwscKey*)calloc(count + 1u, sizeof(LwscKey));
        node->authored_key_count = node->key_count = count;
        if (node->keys == NULL)
            return lwsc_error(reader->error, reader->line, "track allocation failed");
    }
    for (i = 0u; i < count; ++i) {
        uint16_t frame;
        int linear;
        float tension, continuity, bias;
        float values[9];
        unsigned int axis;
        if (!lwsc_required_line(reader, line) ||
            !lwsc_numbers(line, values, channels) ||
            !lwsc_required_line(reader, line) ||
            !lwsc_key_metadata(line, &frame, &linear, &tension, &continuity, &bias))
            return lwsc_error(reader->error, reader->line, "invalid key values/time");
        if (i > 0u && frame <= (dissolve ? node->dissolve_keys[i - 1u].frame
                                        : node->keys[i - 1u].frame))
            return lwsc_error(reader->error, reader->line, "nonincreasing key times");
        if (dissolve) {
            LwscDissolveKey* key = &node->dissolve_keys[i];
            key->dissolve = values[0]; key->frame = frame; key->linear = linear;
            key->tension = tension; key->continuity = continuity; key->bias = bias;
        } else {
            LwscKey* key = &node->keys[i];
            memcpy(key->channels, values, sizeof(values));
            key->frame = frame; key->linear = linear;
            key->tension = tension; key->continuity = continuity; key->bias = bias;
            for (axis = 0u; axis < 3u; ++axis) {
                key->radians[axis] = (float)((double)values[3u + axis] *
                    (double)0.0055555557f * (double)3.1415927f);
                if (!lwsc_finite(key->radians[axis]))
                    return lwsc_error(reader->error, reader->line, "rotation overflow");
            }
        }
    }
    if (!lwsc_required_line(reader, line)) return 0;
    {
        char* argument = lwsc_argument(line);
        unsigned int behavior;
        if (strcmp(line, "EndBehavior") != 0 ||
            !lwsc_unsigned(argument, 5u, &behavior))
            return lwsc_error(reader->error, reader->line, "missing/invalid EndBehavior");
        if (dissolve) node->dissolve_end_behavior = behavior;
        else node->motion_end_behavior = behavior;
    }
    return 1;
}

static int lwsc_hierarchy_finish(LwscSceneTemplate* scene, LwscError* error)
{
    unsigned int i, current;
    scene->order = (unsigned int*)calloc(scene->count, sizeof(unsigned int));
    if (scene->order == NULL) return lwsc_error(error, 0u, "order allocation failed");
    for (i = 0u; i < scene->count; ++i) {
        LwscNode* node = &scene->nodes[i];
        unsigned int steps = 0u;
        if (node->keys == NULL)
            return lwsc_error(error, 0u, "object has no ObjectMotion");
        current = i + 1u;
        while (current != 0u) {
            if (current > scene->count || ++steps > scene->count)
                return lwsc_error(error, 0u, "invalid parent or hierarchy cycle");
            current = scene->nodes[current - 1u].parent_object;
        }
        /* 0x4869f0 adds a last-frame hold only for multi-key vector tracks. */
        if (node->key_count > 1u &&
            node->keys[node->key_count - 1u].frame < scene->last_frame) {
            node->keys[node->key_count] = node->keys[node->key_count - 1u];
            node->keys[node->key_count++].frame = (uint16_t)scene->last_frame;
        }
        if (node->dissolve_count != 0u &&
            node->dissolve_keys[node->dissolve_count - 1u].frame < scene->last_frame) {
            node->dissolve_keys[node->dissolve_count] =
                node->dissolve_keys[node->dissolve_count - 1u];
            node->dissolve_keys[node->dissolve_count++].frame =
                (uint16_t)scene->last_frame;
        }
    }
    /* Include meshes AND nulls in one-based ParentObject numbering. Reverse
     * insertion preserves source-order roots, children and siblings. */
    for (i = scene->count; i > 0u; --i) {
        LwscNode* node = &scene->nodes[i - 1u];
        unsigned int* head = node->parent_object != 0u
            ? &scene->nodes[node->parent_object - 1u].first_child : &scene->first_root;
        node->next_sibling = *head;
        *head = i;
    }
    current = scene->first_root;
    for (i = 0u; i < scene->count; ++i) {
        LwscNode* node;
        if (current == 0u) return lwsc_error(error, 0u, "incomplete hierarchy");
        node = &scene->nodes[current - 1u];
        node->interface_index = i;
        scene->order[i] = current - 1u;
        if (node->first_child != 0u) current = node->first_child;
        else {
            while (current != 0u && scene->nodes[current - 1u].next_sibling == 0u)
                current = scene->nodes[current - 1u].parent_object;
            if (current != 0u) current = scene->nodes[current - 1u].next_sibling;
        }
    }
    return current == 0u ? 1 : lwsc_error(error, 0u, "extra hierarchy entry");
}

LwscSceneTemplate* lwsc_scene_parse(const char* text, size_t length,
                                   const char* path, LwscError* error)
{
    LwscSceneTemplate* scene;
    LwscReader reader;
    char line[1024];
    int status, in_object = 0;
    int have_first = 0, have_last = 0, have_fps = 0;
    size_t i, directory_length = 0u;
    lwsc_error_clear(error);
    if (text == NULL || length == 0u || length > LWSC_MAX_TEXT ||
        path == NULL || *path == '\0' || strlen(path) >= LWSC_PATH_CAPACITY) {
        lwsc_error(error, 0u, "invalid scene text/path"); return NULL;
    }
    scene = (LwscSceneTemplate*)calloc(1, sizeof(*scene));
    if (scene == NULL) { lwsc_error(error, 0u, "scene allocation failed"); return NULL; }
    scene->references = 1u;
    strcpy(scene->path, path);
    for (i = 0u; path[i] != '\0'; ++i)
        if (path[i] == '\\' || path[i] == '/') directory_length = i + 1u;
    memcpy(scene->directory, path, directory_length);
    reader.cursor = text; reader.end = text + length; reader.line = 0u; reader.error = error;
    if (!lwsc_required_line(&reader, line) || strcmp(line, "LWSC") != 0 ||
        !lwsc_required_line(&reader, line) || strcmp(line, "1") != 0) {
        lwsc_error(error, reader.line, "expected LWSC version 1"); goto failed;
    }
    while ((status = lwsc_line(&reader, line)) == 1) {
        char* argument = lwsc_argument(line);
        LwscNode* node = in_object ? &scene->nodes[scene->count - 1u] : NULL;
        if (strcmp(line, "LoadObject") == 0 || strcmp(line, "AddNullObject") == 0) {
            if (in_object) { lwsc_error(error, reader.line, "missing ShadowOptions"); goto failed; }
            if (!lwsc_node_add(scene, argument, strcmp(line, "AddNullObject") == 0,
                                &reader)) goto failed;
            in_object = 1;
        } else if (strcmp(line, "FirstFrame") == 0 || strcmp(line, "LastFrame") == 0) {
            unsigned int value;
            if (scene->count != 0u || !lwsc_unsigned(argument, 65535u, &value)) {
                lwsc_error(error, reader.line, "invalid scene frame bounds"); goto failed;
            }
            if (strcmp(line, "FirstFrame") == 0) { scene->first_frame = value; have_first = 1; }
            else { scene->last_frame = value == 0u ? 1u : value; have_last = 1; }
        } else if (strcmp(line, "FramesPerSecond") == 0) {
            if (scene->count != 0u || !lwsc_numbers(argument, &scene->fps, 1u) ||
                scene->fps <= 0.0f) { lwsc_error(error, reader.line, "invalid FPS"); goto failed; }
            have_fps = 1;
        } else if (node != NULL && strcmp(line, "ObjectMotion") == 0) {
            if (!lwsc_track(scene, node, &reader, 0)) goto failed;
        } else if (node != NULL && strcmp(line, "ParentObject") == 0) {
            if (!lwsc_unsigned(argument, LWSC_MAX_NODES, &node->parent_object)) {
                lwsc_error(error, reader.line, "invalid ParentObject"); goto failed;
            }
        } else if (node != NULL && strcmp(line, "PivotPoint") == 0) {
            unsigned int axis;
            if (!lwsc_numbers(argument, node->pivot, 3u)) {
                lwsc_error(error, reader.line, "invalid PivotPoint"); goto failed;
            }
            for (axis = 0u; axis < 3u; ++axis) node->pivot[axis] = -node->pivot[axis];
        } else if (node != NULL && strcmp(line, "UnseenByCamera") == 0) {
            unsigned int unseen;
            if (!lwsc_unsigned(argument, 1u, &unseen)) {
                lwsc_error(error, reader.line, "invalid UnseenByCamera"); goto failed;
            }
            if (unseen) node->flags |= 4u;
        } else if (node != NULL && strcmp(line, "ObjDissolve") == 0) {
            if (strcmp(argument, "(envelope)") == 0) {
                if (!lwsc_track(scene, node, &reader, 1)) goto failed;
            } else {
                if (node->has_constant_dissolve || node->dissolve_keys != NULL ||
                    !lwsc_numbers(argument, &node->constant_dissolve, 1u)) {
                    lwsc_error(error, reader.line, "invalid constant dissolve"); goto failed;
                }
                node->has_constant_dissolve = 1;
            }
        } else if (node != NULL && strcmp(line, "ShadowOptions") == 0) {
            unsigned int shadows;
            if (!lwsc_unsigned(argument, 0xffffu, &shadows)) {
                lwsc_error(error, reader.line, "invalid ShadowOptions"); goto failed;
            }
            in_object = 0;
        }
        /* The reference skips camera/light/render directives. ObjectMotion
         * and dissolve bodies above are consumed whole, never mistaken for
         * object delimiters. No filtering by a target node name. */
    }
    if (status < 0) goto failed;
    if (in_object || !have_first || !have_last || !have_fps || scene->count == 0u ||
        scene->first_frame > scene->last_frame) {
        lwsc_error(error, reader.line, "incomplete scene/header/object"); goto failed;
    }
    if (!lwsc_hierarchy_finish(scene, error)) goto failed;
    return scene;
failed:
    lwsc_scene_release(scene);
    return NULL;
}

static int lwsc_join(char output[LWSC_PATH_CAPACITY], const char* prefix,
                      const char* base, const char* suffix)
{
    size_t p = strlen(prefix), b = strlen(base), s = strlen(suffix);
    if (p + b + s >= LWSC_PATH_CAPACITY) return 0;
    memcpy(output, prefix, p); memcpy(output + p, base, b);
    memcpy(output + p + b, suffix, s + 1u);
    return 1;
}

LwscSceneTemplate* lwsc_scene_open(const char* path, LwscError* error)
{
    ResourceStream* stream;
    LwscSceneTemplate* scene;
    uint32_t size;
    char* text;
    unsigned int i;
    lwsc_error_clear(error);
    if (path == NULL || strlen(path) >= LWSC_PATH_CAPACITY) {
        lwsc_error(error, 0u, "invalid scene path"); return NULL;
    }
    /* Resource::size is -1 for a not-yet-read WAD member. The existing
     * seekable stream exposes the resolved logical size before this buffer
     * allocation and keeps its own cursor/ownership on both WAD/STD paths. */
    stream = resource_stream_open(path, RESOURCE_AUTO, &size);
    if (stream == NULL) { lwsc_error(error, 0u, "scene resource open failed"); return NULL; }
    if (size == 0u || size > LWSC_MAX_TEXT) {
        resource_stream_close(stream); lwsc_error(error, 0u, "scene resource size exceeds limit"); return NULL;
    }
    text = (char*)malloc((size_t)size);
    if (text == NULL || !resource_stream_read_at(stream, 0u, text, size)) {
        free(text); resource_stream_close(stream);
        lwsc_error(error, 0u, "scene resource read/allocation failed"); return NULL;
    }
    scene = lwsc_scene_parse(text, (size_t)size, path, error);
    free(text); resource_stream_close(stream);
    if (scene == NULL) return NULL;
    for (i = 0u; i < scene->count; ++i) {
        LwscNode* node = &scene->nodes[i];
        const char* fallback;
        if (node->is_null) continue;
        if (!lwsc_join(node->model_path, scene->directory, node->base_name, ".lwo")) {
            lwsc_error(error, 0u, "model path too long"); goto failed;
        }
        if (strlen(node->model_path) >= 260u) {
            lwsc_error(error, 0u, "model owner requires a path below 260 bytes"); goto failed;
        }
        /* Dedicated Model owners avoid borrowing the global unretained
         * model cache across cache_clear/reload. Resolution order is the
         * same scene-directory then configured shared-object prefix. */
        node->model = lwo_model_entry_load(node->model_path, NULL, NULL);
        fallback = model_resource_fallback_prefix_get();
        if (node->model == NULL && fallback != NULL && *fallback != '\0') {
            if (!lwsc_join(node->model_path, fallback, node->base_name, ".lwo")) {
                lwsc_error(error, 0u, "shared model path too long"); goto failed;
            }
            if (strlen(node->model_path) >= 260u) {
                lwsc_error(error, 0u, "shared model owner path exceeds 259 bytes"); goto failed;
            }
            node->model = lwo_model_entry_load(node->model_path, NULL, NULL);
        }
        if (node->model == NULL) {
            lwsc_error(error, 0u, "authored mesh resource missing; no null-only fallback"); goto failed;
        }
        if (node->model->render_source == NULL ||
            node->model->material_binding_count != node->model->n_surfaces ||
            (node->model->n_surfaces != 0u && node->model->material_bindings == NULL)) {
            lwsc_error(error, 0u, "incomplete LWO render/material owner"); goto failed;
        }
    }
    return scene;
failed:
    lwsc_scene_release(scene);
    return NULL;
}

void lwsc_scene_retain(LwscSceneTemplate* scene)
{
    if (scene != NULL) ++scene->references;
}

void lwsc_scene_release(LwscSceneTemplate* scene)
{
    unsigned int i;
    if (scene == NULL || --scene->references != 0u) return;
    for (i = 0u; i < scene->count; ++i) {
        LwscNode* node = &scene->nodes[i];
        free(node->authored_reference); free(node->base_name);
        free(node->keys); free(node->dissolve_keys); model_free(node->model);
    }
    free(scene->order); free(scene->nodes); free(scene);
}

unsigned int lwsc_scene_node_count(const LwscSceneTemplate* scene)
{ return scene != NULL ? scene->count : 0u; }
const LwscNode* lwsc_scene_node(const LwscSceneTemplate* scene, unsigned int index)
{ return scene != NULL && index < scene->count ? &scene->nodes[index] : NULL; }
unsigned int lwsc_scene_first_frame(const LwscSceneTemplate* scene)
{ return scene != NULL ? scene->first_frame : 0u; }
unsigned int lwsc_scene_last_frame(const LwscSceneTemplate* scene)
{ return scene != NULL ? scene->last_frame : 0u; }
float lwsc_scene_fps(const LwscSceneTemplate* scene)
{ return scene != NULL ? scene->fps : 0.0f; }
const char* lwsc_scene_path(const LwscSceneTemplate* scene)
{ return scene != NULL ? scene->path : NULL; }

/* Validate the existing .ae parser's bounded metadata against the still-live
 * BlockFile. It may otherwise return partial rows on allocation failure or
 * truncate fixed-width fields. This producer must fail rather than silently
 * drop an authored activity. No change to the shared assembly API is needed. */
static int lwsc_assembly_complete(BlockFile* block, const char* root,
    uint32_t feature, const EntityAssembly* metadata)
{
    char query[1024];
    BlockRec* group;
    BlockRec* record;
    unsigned int index = 0u;
    size_t root_length = strlen(root);
    if (root_length > 250u || metadata == NULL || metadata->count == 0u ||
        metadata->count > LWSC_MAX_ACTIVITIES || !lwsc_finite(metadata->scale)) return 0;
    sprintf(query, "%s::Activities", root);
    group = block_subgroup(block, query);
    if (group == NULL) return 0;
    for (record = group; record != NULL;
         record = block_next_sibling(record)) {
        const char* key = block_name(record);
        const char* name = block_value(record);
        const char* file;
        const char* sample;
        const EntityActivity* activity;
        if (key == NULL || name == NULL) continue;
        if (*key == '!') {
            if ((feature & ENTITY_FEATURE_REDUCE_ANIM) != 0u) continue;
            ++key;
        }
        if (strlen(key) >= 64u || strlen(name) >= 64u) return 0;
        sprintf(query, "%s::%s::FILE", root, name);
        file = block_get_strptr(block, query);
        if (file == NULL) continue;
        if (*file == '\0' || strlen(file) >= 260u || index >= metadata->count) return 0;
        activity = &metadata->acts[index++];
        if (strcmp(key, activity->state) != 0 || strcmp(name, activity->name) != 0 ||
            strcmp(file, activity->file) != 0 || !lwsc_finite(activity->transcoef)) return 0;
        sprintf(query, "%s::%s::SAMPLE", root, name);
        sample = block_get_strptr(block, query);
        if (sample != NULL && (strlen(sample) >= 260u ||
                              strcmp(sample, activity->sample) != 0)) return 0;
    }
    return index == metadata->count;
}

LwscAssemblyTemplate* lwsc_assembly_open(const char* entity_path,
    const char* config_root, uint32_t feature,
    const char* const* name_keys, unsigned int name_key_count, LwscError* error)
{
    LwscAssemblyTemplate* assembly;
    BlockFile* block;
    const char* base;
    const char* p;
    char directory[LWSC_PATH_CAPACITY];
    char path[LWSC_PATH_CAPACITY];
    char query[1024];
    unsigned int i;
    unsigned int total_nodes = 0u, total_keys = 0u;
    size_t length;
    lwsc_error_clear(error);
    if (entity_path == NULL || *entity_path == '\0' || config_root == NULL ||
        *config_root == '\0' || strlen(config_root) > 250u || name_key_count > 32u ||
        (name_key_count != 0u && name_keys == NULL)) {
        lwsc_error(error, 0u, "invalid assembly arguments"); return NULL;
    }
    length = strlen(entity_path);
    if (length + 2u > sizeof(directory)) {
        lwsc_error(error, 0u, "entity path too long"); return NULL;
    }
    base = entity_path;
    for (p = entity_path; *p != '\0'; ++p)
        if (*p == '/' || *p == '\\') base = p + 1;
    if (*base == '\0') { lwsc_error(error, 0u, "empty entity basename"); return NULL; }
    memcpy(directory, entity_path, length);
    directory[length] = '\\'; directory[length + 1u] = '\0';
    if (!lwsc_join(path, directory, base, ".ae")) {
        lwsc_error(error, 0u, "assembly path too long"); return NULL;
    }
    block = block_open(path);
    if (block == NULL) { lwsc_error(error, 0u, "assembly resource open failed"); return NULL; }
    /* Bound the rows before EntityAssembly allocates its activity array. */
    {
        BlockRec* activities;
        BlockRec* row;
        unsigned int rows = 0u;
        sprintf(query, "%s::Activities", config_root);
        activities = block_subgroup(block, query);
        if (activities == NULL || block_record_count(block) > LWSC_MAX_NODES) {
            block_close(block); lwsc_error(error, 0u, "assembly block count/group invalid"); return NULL;
        }
        for (row = activities; row != NULL; row = block_next_sibling(row)) {
            if (++rows > LWSC_MAX_ACTIVITIES) {
                block_close(block); lwsc_error(error, 0u, "assembly activity row limit exceeded"); return NULL;
            }
        }
    }
    assembly = (LwscAssemblyTemplate*)calloc(1, sizeof(*assembly));
    if (assembly == NULL) {
        block_close(block); lwsc_error(error, 0u, "assembly allocation failed"); return NULL;
    }
    assembly->references = 1u;
    assembly->metadata = entity_assembly_parse(block, config_root, feature);
    if (!lwsc_assembly_complete(block, config_root, feature, assembly->metadata)) {
        lwsc_error(error, 0u, "incomplete/truncated assembly metadata"); goto failed;
    }
    if (name_key_count != 0u) {
        assembly->names = (LwscNameEntry*)calloc(name_key_count, sizeof(LwscNameEntry));
        if (assembly->names == NULL) {
            lwsc_error(error, 0u, "configured-name allocation failed"); goto failed;
        }
    }
    assembly->name_count = name_key_count;
    for (i = 0u; i < name_key_count; ++i) {
        const char* value;
        if (name_keys[i] == NULL || *name_keys[i] == '\0' || strlen(name_keys[i]) > 250u) {
            lwsc_error(error, 0u, "invalid configured-name key"); goto failed;
        }
        sprintf(query, "%s::%s", config_root, name_keys[i]);
        value = block_get_strptr(block, query);
        assembly->names[i].key = lwsc_copy(name_keys[i]);
        if (value != NULL) {
            if (strlen(value) + 16u >= LWSC_NAME_CAPACITY) {
                lwsc_error(error, 0u, "configured name too long"); goto failed;
            }
            assembly->names[i].name = lwsc_copy(value);
        }
        if (assembly->names[i].key == NULL ||
            (value != NULL && assembly->names[i].name == NULL)) {
            lwsc_error(error, 0u, "configured-name copy failed"); goto failed;
        }
    }
    block_close(block); block = NULL;
    assembly->scenes = (LwscSceneTemplate**)calloc(assembly->metadata->count,
                                                  sizeof(LwscSceneTemplate*));
    if (assembly->scenes == NULL) {
        lwsc_error(error, 0u, "activity table allocation failed"); goto failed;
    }
    for (i = 0u; i < (unsigned int)assembly->metadata->count; ++i) {
        const EntityActivity* activity = &assembly->metadata->acts[i];
        /* 0x472cd0 passes (LWSFILE==1), (LOOPING!=0) separately. Missing
         * LOOPING is 2 and consequently repeats; do not treat it as false. */
        if (activity->lwsfile != 1) {
            lwsc_error(error, 0u, "activity is not native LWSFILE TRUE"); goto failed;
        }
        /* Retail appends .lws to FILE, which is a base path, not a glob. */
        if (!lwsc_join(path, directory, activity->file, ".lws")) {
            lwsc_error(error, 0u, "activity FILE path too long"); goto failed;
        }
        assembly->scenes[i] = lwsc_scene_open(path, error);
        if (assembly->scenes[i] == NULL) goto failed;
        total_nodes += assembly->scenes[i]->count;
        total_keys += assembly->scenes[i]->total_keys;
        if (total_nodes > LWSC_MAX_NODES || total_keys > LWSC_MAX_KEYS) {
            lwsc_error(error, 0u, "assembly exceeds total node/key budget"); goto failed;
        }
    }
    return assembly;
failed:
    if (block) block_close(block);
    lwsc_assembly_release(assembly);
    return NULL;
}

void lwsc_assembly_retain(LwscAssemblyTemplate* assembly)
{ if (assembly != NULL) ++assembly->references; }

void lwsc_assembly_release(LwscAssemblyTemplate* assembly)
{
    unsigned int i;
    if (assembly == NULL || --assembly->references != 0u) return;
    if (assembly->scenes != NULL) {
        for (i = 0u; i < (unsigned int)assembly->metadata->count; ++i)
            lwsc_scene_release(assembly->scenes[i]);
    }
    for (i = 0u; i < assembly->name_count; ++i) {
        free(assembly->names[i].key); free(assembly->names[i].name);
    }
    free(assembly->names); free(assembly->scenes);
    entity_assembly_free(assembly->metadata); free(assembly);
}

const EntityAssembly* lwsc_assembly_metadata(const LwscAssemblyTemplate* assembly)
{ return assembly != NULL ? assembly->metadata : NULL; }

const char* lwsc_assembly_configured_name(const LwscAssemblyTemplate* assembly,
                                         const char* key)
{
    unsigned int i;
    if (assembly == NULL || key == NULL) return NULL;
    for (i = 0u; i < assembly->name_count; ++i)
        if (strcasecmp_lite(key, assembly->names[i].key) == 0) return assembly->names[i].name;
    return NULL;
}

const LwscSceneTemplate* lwsc_assembly_scene(const LwscAssemblyTemplate* assembly,
                                            unsigned int activity)
{
    return assembly != NULL && activity < assembly->metadata->count
        ? assembly->scenes[activity] : NULL;
}

static void lwsc_identity(float matrix[16])
{
    memset(matrix, 0, 16u * sizeof(float));
    matrix[0] = matrix[5] = matrix[10] = matrix[15] = 1.0f;
}

static int lwsc_matrix_valid(const float matrix[16])
{
    unsigned int i;
    if (matrix == NULL) return 0;
    for (i = 0u; i < 16u; ++i) if (!lwsc_finite(matrix[i])) return 0;
    return matrix[3] == 0.0f && matrix[7] == 0.0f && matrix[11] == 0.0f &&
           matrix[15] == 1.0f;
}

static float lwsc_lerp(float a, float b, float fraction)
{ return a + (b - a) * fraction; }

/* Host-width form of 0x487cb0 + 0x488170 + 0x4882e0. The exact row-vector
 * basis/pivot formula is the existing view.cpp helper's formula, not a
 * guessed LightWave axis conversion. It cannot call that helper with these
 * records: the helper dereferences retail owner+0x14 / track+0x18 layouts.
 * Terminal endpoints use the last complete key rather than an out-of-bounds
 * read of a nonexistent next key. Negative time retains retail extrapolation. */
static int lwsc_node_matrix(const LwscSceneTemplate* scene, const LwscNode* node,
    const LwscFrameRef* frame, const LwscBackendOps* ops, float time, float matrix[16])
{
    const LwscKey* a;
    const LwscKey* b;
    unsigned int low = 0u, high = node->key_count - 1u, axis;
    float fraction = 0.0f;
    float position[3], rotation[3], scale[3];
    float sx, cx, sy, cy, sz, cz;
    float m3c, m40, m38, m44, m48, m10, m2c, m30, m28, m1c, m20, m18;
    (void)scene;
    if (time >= (float)node->keys[high].frame) low = high;
    else {
        while (high > low + 1u) {
            unsigned int middle = (low + high) / 2u;
            if (time < (float)node->keys[middle].frame) high = middle;
            else low = middle;
        }
    }
    a = &node->keys[low];
    b = low + 1u < node->key_count ? a + 1 : a;
    if (a != b) fraction = (time - (float)a->frame) / (float)(b->frame - a->frame);
    for (axis = 0u; axis < 3u; ++axis) {
        position[axis] = lwsc_lerp(a->channels[axis], b->channels[axis], fraction);
        rotation[axis] = lwsc_lerp(a->radians[axis], b->radians[axis], fraction);
        scale[axis] = lwsc_lerp(a->channels[axis + 6u], b->channels[axis + 6u], fraction);
        if (!lwsc_finite(position[axis]) || !lwsc_finite(rotation[axis]) ||
            !lwsc_finite(scale[axis])) return 0;
    }
    if ((node->flags & 4u) != 0u) {
        /* Preserve the flagged branch exactly: read current local matrix,
         * scale its diagonal and replace translation. Backend releases the
         * temporary parent/query interface used by the real RM GetTransform. */
        if (ops->matrix_get == NULL || !ops->matrix_get(frame, matrix, ops->context)) return 0;
        matrix[0] *= scale[0]; matrix[5] *= scale[1]; matrix[10] *= scale[2];
        matrix[12] = position[0]; matrix[13] = position[1]; matrix[14] = position[2];
        matrix[15] = 1.0f;
        return lwsc_matrix_valid(matrix);
    }
    lwsc_identity(matrix);
    sx = (float)sin((double)rotation[0]); cx = (float)cos((double)rotation[0]);
    sy = (float)sin((double)rotation[1]); cy = (float)cos((double)rotation[1]);
    sz = (float)sin((double)rotation[2]); cz = (float)cos((double)rotation[2]);
    m3c = scale[0] * sz;
    m40 = sx * (sy * m3c) + cx * (scale[0] * cz);
    m38 = cx * (sy * m3c) - sx * (scale[0] * cz);
    m3c = cy * m3c;
    m44 = scale[1] * cz; m48 = sy * m44; m10 = -(scale[1] * sz);
    m2c = cy * m44; m30 = sx * m48 + cx * m10; m28 = -(sx * m10) + cx * m48;
    m1c = -(scale[2] * sy); m20 = sx * (scale[2] * cy); m18 = cx * (scale[2] * cy);
    matrix[0] = m40; matrix[1] = m3c; matrix[2] = m38;
    matrix[4] = m30; matrix[5] = m2c; matrix[6] = m28;
    matrix[8] = m20; matrix[9] = m1c; matrix[10] = m18;
    matrix[12] = node->pivot[0] * m40 + node->pivot[1] * m30 +
                 node->pivot[2] * m20 + position[0];
    matrix[13] = node->pivot[0] * m3c + node->pivot[1] * m2c +
                 node->pivot[2] * m1c + position[1];
    matrix[14] = node->pivot[0] * m38 + node->pivot[1] * m28 +
                 node->pivot[2] * m18 + position[2];
    return lwsc_matrix_valid(matrix);
}

static float lwsc_opacity(float dissolve)
{
    if (dissolve < 0.0f) dissolve = 0.0f;
    if (dissolve > 1.0f) dissolve = 1.0f;
    return 1.0f - dissolve;
}

static float lwsc_dissolve_evaluate(const LwscNode* node, float time)
{
    unsigned int i;
    /* 0x487fc0 uses the first upper bound, unlike the vector binary search. */
    for (i = 1u; i < node->dissolve_count; ++i) {
        const LwscDissolveKey* a = &node->dissolve_keys[i - 1u];
        const LwscDissolveKey* b = a + 1;
        if (time <= (float)b->frame) {
            float fraction = (time - (float)a->frame) / (float)(b->frame - a->frame);
            /* Retail clamps/inverts EACH parsed key before interpolation. */
            return lwsc_lerp(lwsc_opacity(a->dissolve), lwsc_opacity(b->dissolve), fraction);
        }
    }
    return lwsc_opacity(node->dissolve_keys[node->dissolve_count - 1u].dissolve);
}

static int lwsc_activity_time_apply(LwscAssemblyInstance* instance,
    unsigned int activity, float time, int initialize, LwscError* error)
{
    const LwscSceneTemplate* scene = instance->assembly->scenes[activity];
    LwscActivityInstance* target = &instance->activities[activity];
    const EntityActivity* metadata = &instance->assembly->metadata->acts[activity];
    unsigned int i;
    if (!lwsc_finite(time)) return lwsc_error(error, 0u, "invalid frame time");
    if (metadata->looping != 0) time = (float)fmod((double)time, (double)scene->last_frame);
    else if (time > (float)scene->last_frame) time = (float)scene->last_frame;
    for (i = 0u; i < scene->count; ++i) {
        unsigned int index = scene->order[i];
        const LwscNode* node = &scene->nodes[index];
        float matrix[16];
        /* Single-key transforms run once via 0x488900/9a0, not just the
         * animated branch. A later time change must not rescale flag4 keys. */
        if (initialize || node->key_count > 1u) {
            float pose_time = initialize && node->key_count == 1u
                ? (float)node->keys[0].frame : time;
            if (!lwsc_node_matrix(scene, node, &target->frames[index],
                                    &instance->ops, pose_time, matrix) ||
                !instance->ops.matrix_set(&target->frames[index], matrix, instance->ops.context))
                return lwsc_error(error, 0u, "authored node transform failed");
        }
        if (instance->rendering && !node->is_null && node->dissolve_count > 1u) {
            float opacity = lwsc_dissolve_evaluate(node, time);
            if (!lwsc_finite(opacity) || instance->ops.model_opacity_set == NULL ||
                !instance->ops.model_opacity_set(&target->meshes[index], opacity,
                                                  instance->ops.context))
                return lwsc_error(error, 0u, "authored dissolve application failed");
        }
    }
    target->time = time;
    return 1;
}

static int lwsc_frame_create(LwscAssemblyInstance* instance,
    const LwscFrameRef* parent, LwscFrameRef* result, const char* name)
{
    float identity[16];
    lwsc_identity(identity);
    if (!instance->ops.frame_create(parent, result, instance->ops.context) ||
        result->interface_pointer == NULL) return 0;
    return (name == NULL || instance->ops.frame_name_set(result, name, instance->ops.context)) &&
           instance->ops.matrix_set(result, identity, instance->ops.context);
}

static LwscAssemblyInstance* lwsc_instance_create_internal(LwscAssemblyTemplate* assembly,
    const LwscBackendOps* ops, const LwscFrameRef* parent,
    const float root_matrix[16], int rendering, LwscError* error)
{
    LwscAssemblyInstance* instance;
    unsigned int activity;
    lwsc_error_clear(error);
    if (assembly == NULL || ops == NULL || parent == NULL ||
        parent->interface_pointer == NULL || !lwsc_matrix_valid(root_matrix) ||
        ops->frame_create == NULL || ops->frame_destroy == NULL ||
        ops->frame_name_set == NULL || ops->matrix_set == NULL ||
        (rendering && ops->activity_enable == NULL) || ops->world_position == NULL) {
        lwsc_error(error, 0u, "missing real frame backend/root pose"); return NULL;
    }
    instance = (LwscAssemblyInstance*)calloc(1, sizeof(*instance));
    if (instance == NULL) { lwsc_error(error, 0u, "instance allocation failed"); return NULL; }
    instance->ops = *ops; instance->assembly = assembly; lwsc_assembly_retain(assembly);
    instance->rendering = rendering;
    instance->activities = (LwscActivityInstance*)calloc(assembly->metadata->count,
                                                        sizeof(LwscActivityInstance));
    if (instance->activities == NULL ||
        !lwsc_frame_create(instance, parent, &instance->root, NULL)) {
        lwsc_error(error, 0u, "instance root allocation failed"); goto failed;
    }
    instance->valid = 1;
    if (!lwsc_instance_root_set(instance, root_matrix, error)) goto failed;
    for (activity = 0u; activity < (unsigned int)assembly->metadata->count; ++activity) {
        const LwscSceneTemplate* scene = assembly->scenes[activity];
        LwscActivityInstance* target = &instance->activities[activity];
        unsigned int i;
        char activity_frame_name[80];
        /* 0x475d10 names the real activity frame ActFrame_<state>. The
         * state is already checked against EntityActivity's 64-byte field. */
        sprintf(activity_frame_name, "ActFrame_%s", assembly->metadata->acts[activity].state);
        target->frames = (LwscFrameRef*)calloc(scene->count, sizeof(LwscFrameRef));
        if (rendering)
            target->meshes = (LwscMeshRef*)calloc(scene->count, sizeof(LwscMeshRef));
        if (target->frames == NULL || (rendering && target->meshes == NULL) ||
            !lwsc_frame_create(instance, &instance->root, &target->root,
                                activity_frame_name) ||
            (rendering && !ops->activity_enable(&target->root, 0, ops->context))) {
            lwsc_error(error, 0u, "activity frame allocation failed"); goto failed;
        }
        for (i = 0u; i < scene->count; ++i) {
            unsigned int index = scene->order[i];
            const LwscNode* node = &scene->nodes[index];
            const LwscFrameRef* node_parent = node->parent_object == 0u
                ? &target->root : &target->frames[node->parent_object - 1u];
            if (!lwsc_frame_create(instance, node_parent, &target->frames[index], node->native_name)) {
                lwsc_error(error, 0u, "authored frame allocation failed"); goto failed;
            }
            if (!node->is_null) {
                /* Query instances retain every real Model through the
                 * assembly and associate it with this exact source/frame
                 * index. They do not claim a renderer attachment. */
                if (node->model == NULL) {
                    lwsc_error(error, 0u, "authored Model owner unavailable"); goto failed;
                }
                if (rendering && (ops->model_attach == NULL || ops->model_detach == NULL ||
                    !ops->model_attach(&target->frames[index], node->model, node->model_path,
                                         node->flags, &target->meshes[index], ops->context) ||
                    target->meshes[index].owner == NULL)) {
                    lwsc_error(error, 0u, "real mesh attachment unavailable"); goto failed;
                }
            }
        }
        if (!lwsc_activity_time_apply(instance, activity, 0.0f, 1, error)) goto failed;
    }
    if (rendering && !ops->activity_enable(&instance->activities[0].root, 1, ops->context)) {
        lwsc_error(error, 0u, "initial activity activation failed"); goto failed;
    }
    return instance;
failed:
    lwsc_instance_destroy(instance);
    return NULL;
}

LwscAssemblyInstance* lwsc_instance_create(LwscAssemblyTemplate* assembly,
    const LwscBackendOps* ops, const LwscFrameRef* parent,
    const float root_matrix[16], LwscError* error)
{
    return lwsc_instance_create_internal(assembly, ops, parent, root_matrix, 1, error);
}

LwscAssemblyInstance* lwsc_instance_create_for_pose(LwscAssemblyTemplate* assembly,
    const LwscBackendOps* ops, const LwscFrameRef* parent,
    const float root_matrix[16], LwscError* error)
{
    return lwsc_instance_create_internal(assembly, ops, parent, root_matrix, 0, error);
}

LwscAssemblyInstance* lwsc_instance_clone(const LwscAssemblyInstance* source,
    const LwscFrameRef* parent, const float root_matrix[16], LwscError* error)
{
    LwscAssemblyInstance* clone;
    if (source == NULL || !source->valid) {
        lwsc_error(error, 0u, "invalid clone source"); return NULL;
    }
    clone = lwsc_instance_create_internal(source->assembly, &source->ops,
        parent, root_matrix, source->rendering, error);
    if (clone == NULL) return NULL;
    if (!lwsc_instance_activity_set(clone,
            source->assembly->metadata->acts[source->current].state, error)) {
        lwsc_instance_destroy(clone); return NULL;
    }
    return clone;
}

void lwsc_instance_destroy(LwscAssemblyInstance* instance)
{
    unsigned int activity;
    if (instance == NULL) return;
    instance->valid = 0;
    if (instance->activities != NULL) {
        for (activity = 0u; activity < (unsigned int)instance->assembly->metadata->count; ++activity) {
            const LwscSceneTemplate* scene = instance->assembly->scenes[activity];
            LwscActivityInstance* target = &instance->activities[activity];
            unsigned int i;
            /* Detach mesh owners before frames; child frames before parents.
             * All partially returned backend references are also released. */
            if (target->meshes != NULL) {
                for (i = 0u; i < scene->count; ++i)
                    if (target->meshes[i].owner != NULL)
                        instance->ops.model_detach(&target->meshes[i], instance->ops.context);
            }
            if (target->frames != NULL) {
                for (i = scene->count; i > 0u; --i) {
                    LwscFrameRef* frame = &target->frames[scene->order[i - 1u]];
                    if (frame->interface_pointer != NULL)
                        instance->ops.frame_destroy(frame, instance->ops.context);
                }
            }
            if (target->root.interface_pointer != NULL)
                instance->ops.frame_destroy(&target->root, instance->ops.context);
            free(target->frames); free(target->meshes);
        }
    }
    if (instance->root.interface_pointer != NULL)
        instance->ops.frame_destroy(&instance->root, instance->ops.context);
    free(instance->activities);
    lwsc_assembly_release(instance->assembly); free(instance);
}

int lwsc_instance_activity_set(LwscAssemblyInstance* instance,
                               const char* activity_state, LwscError* error)
{
    unsigned int i;
    lwsc_error_clear(error);
    if (instance == NULL || !instance->valid || activity_state == NULL)
        return lwsc_error(error, 0u, "invalid activity instance/state");
    for (i = 0u; i < (unsigned int)instance->assembly->metadata->count; ++i) {
        if (strcasecmp_lite(instance->assembly->metadata->acts[i].state, activity_state) == 0) {
            if (i == instance->current) return 1;
            if ((instance->rendering &&
                 !instance->ops.activity_enable(&instance->activities[instance->current].root,
                                                  0, instance->ops.context)) ||
                !lwsc_activity_time_apply(instance, i, 0.0f, 0, error) ||
                (instance->rendering &&
                 !instance->ops.activity_enable(&instance->activities[i].root, 1, instance->ops.context))) {
                instance->valid = 0;
                return lwsc_error(error, 0u, "activity mutation failed; recreate instance");
            }
            instance->current = i;
            return 1;
        }
    }
    return lwsc_error(error, 0u, "activity state absent from authored assembly");
}

int lwsc_instance_time_set(LwscAssemblyInstance* instance, float frame_time,
                           LwscError* error)
{
    lwsc_error_clear(error);
    if (instance == NULL || !instance->valid || !lwsc_finite(frame_time))
        return lwsc_error(error, 0u, "invalid instance/frame time");
    if (!lwsc_activity_time_apply(instance, instance->current, frame_time, 0, error)) {
        instance->valid = 0; return 0;
    }
    return 1;
}

int lwsc_instance_root_set(LwscAssemblyInstance* instance,
                           const float root_matrix[16], LwscError* error)
{
    float matrix[16];
    float scale;
    unsigned int row, column;
    lwsc_error_clear(error);
    if (instance == NULL || !instance->valid || !lwsc_matrix_valid(root_matrix))
        return lwsc_error(error, 0u, "invalid root matrix/instance");
    memcpy(matrix, root_matrix, sizeof(matrix));
    scale = instance->assembly->metadata->scale;
    if (scale != 0.0f) {
        for (row = 0u; row < 3u; ++row)
            for (column = 0u; column < 3u; ++column) matrix[row * 4u + column] *= scale;
    }
    if (!lwsc_matrix_valid(matrix))
        return lwsc_error(error, 0u, "scaled root matrix overflow");
    if (!instance->ops.matrix_set(&instance->root, matrix, instance->ops.context)) {
        instance->valid = 0;
        return lwsc_error(error, 0u, "root pose application failed; recreate instance");
    }
    return 1;
}

unsigned int lwsc_instance_activity(const LwscAssemblyInstance* instance)
{ return instance != NULL && instance->valid ? instance->current : (unsigned int)-1; }
unsigned int lwsc_instance_capabilities(const LwscAssemblyInstance* instance)
{
    if (instance == NULL || !instance->valid) return 0u;
    return LWSC_CAPABILITY_POSE_QUERY |
        (instance->rendering ? LWSC_CAPABILITY_RENDERING : 0u);
}
float lwsc_instance_time(const LwscAssemblyInstance* instance)
{ return instance != NULL && instance->valid ? instance->activities[instance->current].time : 0.0f; }
const LwscFrameRef* lwsc_instance_root(const LwscAssemblyInstance* instance)
{ return instance != NULL && instance->valid ? &instance->root : NULL; }

const LwscFrameRef* lwsc_instance_node(const LwscAssemblyInstance* instance,
    unsigned int activity, unsigned int source_index)
{
    if (instance == NULL || !instance->valid || activity >= instance->assembly->metadata->count ||
        source_index >= instance->assembly->scenes[activity]->count) return NULL;
    return &instance->activities[activity].frames[source_index];
}

const Model* lwsc_instance_node_model(const LwscAssemblyInstance* instance,
    unsigned int activity, unsigned int source_index)
{
    if (lwsc_instance_node(instance, activity, source_index) == NULL) return NULL;
    return instance->assembly->scenes[activity]->nodes[source_index].model;
}

const LwscFrameRef* lwsc_instance_named_node(const LwscAssemblyInstance* instance,
    const char* configured_base, unsigned int ordinal)
{
    char name[LWSC_NAME_CAPACITY];
    const LwscSceneTemplate* scene;
    unsigned int i;
    if (instance == NULL || !instance->valid || configured_base == NULL ||
        *configured_base == '\0' ||
        !lwsc_native_name(name, sizeof(name), configured_base, ordinal, 0)) return NULL;
    scene = instance->assembly->scenes[instance->current];
    for (i = 0u; i < scene->count; ++i) {
        unsigned int index = scene->order[i];
        if (strcasecmp_lite(scene->nodes[index].native_name, name) == 0)
            return &instance->activities[instance->current].frames[index];
    }
    return NULL;
}

int lwsc_instance_node_world_position(const LwscAssemblyInstance* instance,
    const LwscFrameRef* frame, float xyz[3])
{
    unsigned int activity, i;
    float position[3];
    if (instance == NULL || !instance->valid || frame == NULL || xyz == NULL) return 0;
    /* Accept only this instance's owned refs; never turn arbitrary handles
     * into frames or silently return the building origin. Old activity refs
     * remain queryable, as required by the external retail ToolNull cache. */
    for (activity = 0u; activity < (unsigned int)instance->assembly->metadata->count; ++activity) {
        const LwscSceneTemplate* scene = instance->assembly->scenes[activity];
        for (i = 0u; i < scene->count; ++i) {
            if (frame == &instance->activities[activity].frames[i]) {
                if (!instance->ops.world_position(frame, position, instance->ops.context) ||
                    !lwsc_finite(position[0]) || !lwsc_finite(position[1]) ||
                    !lwsc_finite(position[2])) return 0;
                memcpy(xyz, position, sizeof(position)); return 1;
            }
        }
    }
    return 0;
}
