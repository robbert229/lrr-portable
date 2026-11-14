// SPDX-License-Identifier: LGPL-3.0-or-later
/* String and token helpers — original translation unit G98-004 (text
 * 00477440-004777a0, literals 004aabd8-004aabef; finding 505). The unit has
 * no state of its own.
 *
 * Rebuilt in the original's shape (issue #396, ADR 0005): one file for the
 * unit, its functions in original order and the C-compatible subset, so the
 * matching build compiles it as C. The native port compiles it as C++. */
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"
#include "crt/vc5names.h"

// FUNCTION: LEGORR 0x00477440
int sound_variant_split(char* text, char** out, const char* separators)
{
    char* s = text;
    int count = 0;
    unsigned int length = strlen(separators);

    if (*s == '\0') return 0;

    out[count++] = s;
    while (*s != '\0') {
        if (strncmp(separators, s, length) == 0) {
            *s = '\0';
            out[count++] = s + length;
        }
        s++;
    }

    return count;
}

// FUNCTION: LEGORR 0x004774b0
int whitespace_tokenize(char* text, char** fields)
{
    char* s = text;
    int count = 0;

    if (*s == '\0') return 0;

    fields[count++] = s;
    while (*s != '\0') {
        if (isspace((unsigned char)*s)) {
            *s = '\0';
            s++;
            while (isspace((unsigned char)*s)) s++;
            fields[count++] = s;
        }
        else s++;
    }

    return count;
}

// FUNCTION: LEGORR 0x00477550
char* string_duplicate(const char* text)
{
    char* copy = (char*)malloc(strlen(text) + 1);
    strcpy(copy, text);
    return copy;
}

// FUNCTION: LEGORR 0x00477590
char* formatted_text_escape_decode_and_emit(const char* format, ...)
{
    char line[256];
    char decoded[256];
    char* s;
    char* t;
    va_list args;

    /* The original's _vsprintf at 0048e1e0 is annotated as the port's
     * formatted_text_buffer_build (finding 505), so this call's target name
     * differs in the comparison. */
    va_start(args, format);
    vsprintf(line, format, args);
    va_end(args);

    for (t = decoded, s = line; *s != '\0'; t++, s++) {
        if (*s == '\\' && s[1] == 'n') {
            *t = '\n';
            s++;
        }
        else if (*s == '_') *t = ' ';
        else *t = *s;
    }
    *t = '\0';

    return string_duplicate(decoded);
}

// FUNCTION: LEGORR 0x00477610
const char* case_insensitive_substring_find(const char* haystack,
                                            const char* needle)
{
    const char* s = haystack;
    unsigned int haystack_length = strlen(s);
    unsigned int needle_length = strlen(needle);
    unsigned int loop;

    if (haystack_length && needle_length) {
        for (loop = 0; loop < haystack_length; loop++, s++) {
            if (strnicmp(s, needle, needle_length) == 0) return s;
        }
    }

    return NULL;
}

// FUNCTION: LEGORR 0x00477670
int game_string_hash_compute(const unsigned char* text,
                             int skip_whitespace, int uppercase)
{
    unsigned int sum = 0;
    unsigned int multiplier = 0x00600937;
    unsigned int step = 1;
    unsigned char c;

    while (*text != '\0') {
        if (skip_whitespace) {
            while (isspace(*text)) text++;
        }
        c = *text++;
        if (uppercase) c = (unsigned char)toupper(c);
        sum += c * step * multiplier;
        multiplier *= 0x00600937;
        step++;
    }

    return (int)sum;
}

// FUNCTION: LEGORR 0x00477710
int block_get_bool(const char* value)
{
    // STRING: LEGORR 0x004aabec
    if (stricmp(value, "YES") == 0) return 1;
    // STRING: LEGORR 0x004aabe4
    if (stricmp(value, "TRUE") == 0) return 1;
    // STRING: LEGORR 0x004a4124
    if (stricmp(value, "ON") == 0) return 1;
    // STRING: LEGORR 0x004aabe0
    if (stricmp(value, "NO") == 0) return 0;
    // STRING: LEGORR 0x004aabd8
    if (stricmp(value, "FALSE") == 0) return 0;
    // STRING: LEGORR 0x004a4120
    if (stricmp(value, "OFF") == 0) return 0;
    return 2;
}
