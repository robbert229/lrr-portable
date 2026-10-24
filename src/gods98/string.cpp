// SPDX-License-Identifier: LGPL-3.0-only
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "string.hpp"

/* FUN_00477550: allocate strlen+1 through the heap seam and copy. The
 * corpus allocates via its engine-heap wrapper (TODO(corpus)); the port
 * mallocs (deviation in string.hpp). */
char* string_duplicate(const char* text)
{
    if (text == NULL) {
        return NULL;
    }
    const size_t n = strlen(text) + 1u;
    char* copy = (char*)malloc(n);
    if (copy == NULL) {
        return NULL; /* corpus: the heap seam would have asserted */
    }
    memcpy(copy, text, n);
    return copy;
}

/* FUN_0048da50: substring find. The corpus's aligned-word/SWAR scan is a
 * byte-equivalent strstr, plus the same early-outs (empty needle -> the
 * haystack start). */
const char* string_find_substring(const char* haystack, const char* needle)
{
    if (haystack == NULL || needle == NULL) {
        return NULL;
    }
    if (*needle == '\0') {
        return haystack;
    }
    return strstr(haystack, needle);
}

/* FUN_00493ec0: length bounded by `limit` (the corpus walks byte-wise,
 * decrementing the bound first, and answers `limit` when no NUL turns up
 * inside it — byte-identical to scanning min(strlen, limit)). */
int string_length_bounded(const char* text, int limit)
{
    if (text == NULL || limit <= 0) {
        return 0;
    }
    int n = 0;
    while (n < limit && text[n] != '\0') {
        ++n;
    }
    return n;
}

/* FUN_00478f50: `root::comp1::comp2…` into the static builder. */
const char* string_path_join(const char* root, ...)
{
    static char s_path[STRING_PATH_MAX]; /* the corpus static scratch */
    if (root == NULL) {
        root = "";
    }
    int room = (int)sizeof s_path;
    int used = 0;
    size_t len = strlen(root);
    if (len >= (size_t)room) {
        len = (size_t)room - 1u;
    }
    memcpy(s_path, root, len);
    used = (int)len;
    s_path[used] = '\0';

    va_list ap;
    va_start(ap, root);
    for (const char* part = va_arg(ap, const char*);
         part != NULL; part = va_arg(ap, const char*)) {
        if (room - used < 3) {
            break; /* out of room: stop joining (bounded deviation) */
        }
        s_path[used++] = ':';
        s_path[used++] = ':';
        len = strlen(part);
        if (len > (size_t)(room - used - 1)) {
            len = (size_t)(room - used - 1);
        }
        memcpy(s_path + used, part, len);
        used += (int)len;
        s_path[used] = '\0';
    }
    va_end(ap);
    return s_path;
}
