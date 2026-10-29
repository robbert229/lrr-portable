// SPDX-License-Identifier: LGPL-3.0-only
#include <math.h>
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#if defined(LEGORRR_BACKEND_WINAPI)
#include <windows.h>
#elif !defined(_MSC_VER)
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <strings.h>
extern char** environ;
#endif

#include "crt.hpp"
#include "heap.hpp"
#include "string.hpp"

#if defined(_MSC_VER)
extern "C" void __cdecl _amsg_exit(int);
#endif

#if defined(_MSC_VER) && (_MSC_VER < 1300)
#define CRT_U64(value) value##ui64
#else
#define CRT_U64(value) value##ULL
#endif

/* CRT inverse-trigonometric entries. The corpus's 0x0048e0ad entry is an
 * x87-internal helper: it receives its value on ST(0), forms (1+x)(1-x),
 * then uses FSQRT/FPATAN before CRT boundary/error handling. This C double
 * interface has a different callee signature and retains the portable
 * ordinary-finite-input semantics; matching the corpus entry is therefore
 * a signature/runtime-path barrier, not an expression-order respelling. */
// FUNCTION: LEGORR 0x0048e090
double crt_asin(double value)
{
    return asin(value);
}

// FUNCTION: LEGORR 0x0048e0ad
double crt_acos_core(double value)
{
    double root = sqrt((1.0 - value) * (1.0 + value));
#if defined(_MSC_VER) && defined(_M_IX86)
    double result;
    __asm {
        fld qword ptr [root]
        fld qword ptr [value]
        fpatan
        fstp qword ptr [result]
    }
    return result;
#else
    return atan2(root, value);
#endif
}

/* The 96-bit extended-integer helpers operate on three little-endian DWORDs.
 * Keeping the carries explicit also works with the VC5 compiler used by the
 * original target. */
// FUNCTION: LEGORR 0x0049a050
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) void extended_float96_bit_shift_left_one(uint32_t* value)
{
    __asm {
        mov eax, dword ptr [esp + 4]
        push esi
        push edi
        mov esi, dword ptr [eax]
        mov edi, dword ptr [eax + 4]
        mov ecx, esi
        add esi, esi
        mov dword ptr [eax], esi
        lea esi, [edi + edi]
        shr ecx, 31
        or esi, ecx
        mov ecx, dword ptr [eax + 8]
        mov edx, edi
        mov dword ptr [eax + 4], esi
        shr edx, 31
        shl ecx, 1
        or ecx, edx
        mov dword ptr [eax + 8], ecx
        pop edi
        pop esi
        ret
    }
}
#else
void extended_float96_bit_shift_left_one(uint32_t* value)
{
    uint32_t low;
    uint32_t middle;
    low = value[0];
    middle = value[1];
    value[0] = low << 1;
    value[1] = (middle << 1) | (low >> 31);
    value[2] = (value[2] << 1) | (middle >> 31);
}
#endif

// FUNCTION: LEGORR 0x0049a080
#if defined(_MSC_VER) && defined(_M_IX86)
/* Corpus 0x49a080..0x49a0ac shifts the three dwords high-to-low. */
__declspec(naked) void extended_float96_bit_shift_right_one(uint32_t* value)
{
    __asm {
        mov eax, dword ptr [esp + 4]
        push esi
        push edi
        mov edx, dword ptr [eax + 8]
        mov ecx, dword ptr [eax + 4]
        mov esi, edx
        mov edi, ecx
        shl esi, 1fh
        shr ecx, 1
        or ecx, esi
        mov dword ptr [eax + 4], ecx
        mov ecx, dword ptr [eax]
        shl edi, 1fh
        shr ecx, 1
        shr edx, 1
        or ecx, edi
        pop edi
        mov dword ptr [eax + 8], edx
        mov dword ptr [eax], ecx
        pop esi
        ret
    }
}
#else
void extended_float96_bit_shift_right_one(uint32_t* value)
{
    uint32_t middle;
    middle = value[1];
    value[1] = (middle >> 1) | (value[2] << 31);
    value[2] >>= 1;
    value[0] = (value[0] >> 1) | (middle << 31);
}
#endif

/* FUN_00498000: the corpus's memmove with word loops and byte tails —
 * byte-equivalent to the host memmove (findings 260/394). */
// LIBRARY: LEGORR 0x00498000 SYMBOL
// _memmove
void* crt_memory_move(void* dest, const void* src, size_t count)
{
    return memmove(dest, src, count);
}

/* FUN_00417430: app helper that forwards to the CRT __ftol routine under
 * the truncate-toward-zero control word (findings 381/393). Overflow
 * values are outside the corpus's caller set (deviation in crt.hpp). */
// FUNCTION: LEGORR 0x00417430
int float_to_long(float value)
{
    return (int)value;
}

/* FUN_0041f8e0: empty body; the corpus's callable no-op (finding 381). */
// FUNCTION: LEGORR 0x0041f8e0
void noop_callback(void)
{
}

/* The executable owns these two runtime-error controls. The callback and
 * mode word are zero-filled image globals; the separate mode selector has
 * the image's initialized value 2. */
// GLOBAL: LEGORR 0x00549124
static int s_runtime_error_mode;
// GLOBAL: LEGORR 0x004afd64
static int s_runtime_error_console_mode = 2;
// GLOBAL: LEGORR 0x005493d8
static void (*s_runtime_error_callback)(void);
// GLOBAL: LEGORR 0x005491a0
static void (*s_terminate_handler_callback)(void);

// FUNCTION: LEGORR 0x00495ae0
void crt_terminate_handler_invoke_and_abort(void)
{
    if (s_terminate_handler_callback != NULL) {
        (*s_terminate_handler_callback)();
    }
    abort();
}

typedef struct CrtRuntimeErrorText {
    int code;
    const char* text;
} CrtRuntimeErrorText;

static const CrtRuntimeErrorText s_runtime_error_messages[] = {
    { 0x02, "R6002\r\n- floating point not loaded\r\n" },
    { 0x08, "R6008\r\n- not enough space for arguments\r\n" },
    { 0x09, "R6009\r\n- not enough space for environment\r\n" },
    { 0x0a, "abnormal program termination\r\n" },
    { 0x10, "R6016\r\n- not enough space for thread data\r\n" },
    { 0x11, "R6017\r\n- unexpected multithread lock error\r\n" },
    { 0x12, "R6018\r\n- unexpected heap error\r\n" },
    { 0x13, "R6019\r\n- unable to open console device\r\n" },
    { 0x18, "R6024\r\n- not enough space for _onexit/atexit table\r\n" },
    { 0x19, "R6025\r\n- pure virtual function call\r\n" },
    { 0x1a, "R6026\r\n- not enough space for stdio initialization\r\n" },
    { 0x1b, "R6027\r\n- not enough space for lowio initialization\r\n" },
    { 0x1c, "R6028\r\n- unable to initialize heap\r\n" },
    { 0x78, "DOMAIN error\r\n" },
    { 0x79, "SING error\r\n" },
    { 0x7a, "TLOSS error\r\n" },
    { 0xfc, "\r\n" },
    { 0xff, "runtime error " }
};

static const char* crt_runtime_error_text_get(int code)
{
    unsigned int i;
    for (i = 0u; i < sizeof(s_runtime_error_messages) /
                           sizeof(s_runtime_error_messages[0]); ++i) {
        if (s_runtime_error_messages[i].code == code) {
            return s_runtime_error_messages[i].text;
        }
    }
    return NULL;
}

static void crt_runtime_error_buffer_append(char* buffer, unsigned int capacity,
                                            const char* text)
{
    unsigned int used = 0u;
    if (buffer == NULL || text == NULL || capacity == 0u) {
        return;
    }
    while (used + 1u < capacity && buffer[used] != '\0') {
        ++used;
    }
    while (used + 1u < capacity && *text != '\0') {
        buffer[used++] = *text++;
    }
    buffer[used] = '\0';
}

static int crt_runtime_error_reporting_enabled(void)
{
    return s_runtime_error_mode == 1 ||
        (s_runtime_error_mode == 0 && s_runtime_error_console_mode == 1);
}

// FUNCTION: LEGORR 0x004968b0
void crt_runtime_error_message_enter_exit(void)
{
    /* Corpus 0x4968b0 tests both mode globals here rather than calling the
     * shared reporting predicate. */
    if (s_runtime_error_mode == 1 ||
        (s_runtime_error_mode == 0 && s_runtime_error_console_mode == 1)) {
        crt_runtime_error_message(0xfc);
        if (s_runtime_error_callback != NULL) {
            (*s_runtime_error_callback)();
        }
        crt_runtime_error_message(0xff);
    }
}

// FUNCTION: LEGORR 0x004968f0
void crt_runtime_error_message(int error_code)
{
    const char* message = crt_runtime_error_text_get(error_code);
    if (message == NULL) {
        return;
    }
    if (crt_runtime_error_reporting_enabled()) {
        int handle = crt_file_descriptor_handle_get(2u);
        if (handle == -1) {
#if defined(LEGORRR_BACKEND_WINAPI)
            HANDLE standard_error = GetStdHandle((DWORD)-12);
            DWORD written;
            WriteFile(standard_error, message, (DWORD)strlen(message),
                      &written, NULL);
#elif !defined(_MSC_VER)
            (void)write(2, message, strlen(message));
#else
            (void)fputs(message, stderr);
#endif
        }
        else {
#if defined(LEGORRR_BACKEND_WINAPI)
            DWORD written;
            WriteFile((HANDLE)handle, message, (DWORD)strlen(message),
                      &written, NULL);
#elif !defined(_MSC_VER)
            (void)write(handle, message, strlen(message));
#else
            (void)fputs(message, stderr);
#endif
        }
        return;
    }
    if (error_code != 0xfc) {
        char module_name[260];
        char dialog[512];
        module_name[0] = '\0';
#if defined(LEGORRR_BACKEND_WINAPI)
        if (GetModuleFileNameA(NULL, module_name, sizeof(module_name)) == 0u) {
            strcpy(module_name, "<program name unknown>");
        }
#elif !defined(_MSC_VER)
        {
            ssize_t count = readlink("/proc/self/exe", module_name,
                                     sizeof(module_name) - 1u);
            if (count < 0) {
                strcpy(module_name, "<program name unknown>");
            }
            else {
                module_name[count] = '\0';
            }
        }
#else
        strcpy(module_name, "<program name unknown>");
#endif
        if (strlen(module_name) > 60u) {
            memmove(module_name + 3, module_name + strlen(module_name) - 57u,
                    58u);
            module_name[0] = '.';
            module_name[1] = '.';
            module_name[2] = '.';
        }
        dialog[0] = '\0';
        crt_runtime_error_buffer_append(dialog, sizeof(dialog),
                                        "Runtime Error!\n\nProgram: ");
        crt_runtime_error_buffer_append(dialog, sizeof(dialog), module_name);
        crt_runtime_error_buffer_append(dialog, sizeof(dialog), "\n\n");
        crt_runtime_error_buffer_append(dialog, sizeof(dialog), message);
        (void)crt_runtime_error_message_box_display(
            dialog, "Microsoft Visual C++ Runtime Library", 0u);
    }
}

// GLOBAL: LEGORR 0x00549428
static void* s_runtime_message_box_proc;
// GLOBAL: LEGORR 0x0054942c
static void* s_runtime_active_window_proc;
// GLOBAL: LEGORR 0x00549430
static void* s_runtime_last_popup_proc;

// FUNCTION: LEGORR 0x00499730
int crt_runtime_error_message_box_display(const char* message,
                                          const char* title,
                                          unsigned int flags)
{
#if defined(LEGORRR_BACKEND_WINAPI)
    typedef int (WINAPI *CrtMessageBoxAProc)(HWND, LPCSTR, LPCSTR, UINT);
    typedef HWND (WINAPI *CrtGetActiveWindowProc)(void);
    typedef HWND (WINAPI *CrtGetLastActivePopupProc)(HWND);
    HMODULE module;
    HWND owner = NULL;
    if (s_runtime_message_box_proc == NULL) {
        module = LoadLibraryA("user32.dll");
        if (module == NULL) {
            return 0;
        }
        s_runtime_message_box_proc = (void*)GetProcAddress(module, "MessageBoxA");
        if (s_runtime_message_box_proc == NULL) {
            return 0;
        }
        s_runtime_active_window_proc =
            (void*)GetProcAddress(module, "GetActiveWindow");
        s_runtime_last_popup_proc =
            (void*)GetProcAddress(module, "GetLastActivePopup");
    }
    if (s_runtime_active_window_proc != NULL) {
        owner = ((CrtGetActiveWindowProc)s_runtime_active_window_proc)();
    }
    if (owner != NULL && s_runtime_last_popup_proc != NULL) {
        owner = ((CrtGetLastActivePopupProc)s_runtime_last_popup_proc)(owner);
    }
    return ((CrtMessageBoxAProc)s_runtime_message_box_proc)(
        owner, message, title, (UINT)flags);
#else
    (void)message;
    (void)title;
    (void)flags;
    return 0;
#endif
}

/* FUN_0047d3f0: popcount via the clear-lowest-set-bit loop. */
// FUNCTION: LEGORR 0x0047d3f0
int bit_count32(uint32_t value)
{
    int n = 0;
    while (value != 0u) {
        value &= value - 1u;
        ++n;
    }
    return n;
}

/* FUN_0047d700: popcount via the per-position test loop. */
// FUNCTION: LEGORR 0x0047d700
int pop_count32(uint32_t value)
{
    int n = 0;
    for (unsigned i = 0; i < 32u; ++i) {
        if ((value & (1u << (i & 31u))) != 0u) {
            ++n;
        }
    }
    return n;
}

/* FUN_0047d720: index of the lowest set bit, 0xffffffff for zero. The
 * corpus's 0x47d72e `shl esi,cl` repeatedly shifts its live mask; without
 * resetting that mask, it answers 2 for 0x8 rather than the tested 3.
 * Keep the one-hot-per-position semantics; the i&31 is redundant under
 * the i < 32 bound. */
// FUNCTION: LEGORR 0x0047d720
uint32_t find_first_set_bit32(uint32_t value)
{
    for (uint32_t i = 0; i < 32u; ++i) {
        if ((value & (1u << i)) != 0u) {
            return i;
        }
    }
    return 0xffffffffu;
}

/* FUN_0047e7e0: the corpus's second identical position-loop body. */
// FUNCTION: LEGORR 0x0047e7e0
int bit_count32_by_position(uint32_t value)
{
    return pop_count32(value);
}

/* FUN_00497ab0: zero over exactly 3 dwords. */
// FUNCTION: LEGORR 0x00497ab0
int uint96_is_zero(const uint32_t limbs[3])
{
    int i = 0;
    do {
        if (limbs[i] != 0u) {
            return 0;
        }
        ++i;
    } while (i < 3);
    return 1;
}

/* FUN_0048e870: the double's exponent field not all-ones. */
// FUNCTION: LEGORR 0x0048e870
int double_is_finite(double value)
{
    uint64_t bits;
    __builtin_memcpy(&bits, &value, sizeof bits);
    return ((bits >> 52u) & 0x7ffu) != 0x7ffu;
}

/* FUN_0045b900: the corpus casts the int pair and the divisor to float
 * before both divides (the results land in a caller float pair). */
// FUNCTION: LEGORR 0x0045b900
void integer_pair_divide_to_float_pair(int x, int y, int divisor,
                                       float* out_x, float* out_y)
{
    *out_x = (float)x / (float)divisor;
    *out_y = (float)y / (float)divisor;
}

/* FUN_0045b930: the compiler's two-shot __ftol — each float truncates
 * toward zero into its int output (same deviation as float_to_long). */
// FUNCTION: LEGORR 0x0045b930
void float_pair_to_integer_pair(float x, float y, int* out_x, int* out_y)
{
    *out_x = (int)x;
    *out_y = (int)y;
}

/* FUN_00499fb0: the corpus's carry test — the sum below either operand
 * means the addition carried out of the 32-bit result. */
// FUNCTION: LEGORR 0x00499fb0
int crt_unsigned_add_with_carry(uint32_t a, uint32_t b, uint32_t* sum)
{
    uint32_t s;
    int carry = 0;
    s = b + a;
    if (s < a || s < b) {
        carry = 1;
    }
    *sum = s;
    return carry;
}

/* FUN_00493a80: the exponent/quiet-bit pattern classifier over the two
 * bit words, corpus masks verbatim (finding 399). The corpus loads both
 * parameters up front and tests the infinities first, then reads the
 * exponent+quiet field as a raw dword at esp+0xa — six bytes past the
 * low parameter's address, the high half of the hi word plus two bytes
 * past the last parameter, which the 0x7ff8 mask discards — and compares
 * it as a WORD. */
// FUNCTION: LEGORR 0x00493a80
int double_classify(uint32_t lo, uint32_t hi)
{
    if (hi == 0x7ff00000u && lo == 0u) {
        return 1;                                   /* +infinity */
    }
    if (hi == 0xfff00000u && lo == 0u) {
        return 2;                                   /* -infinity */
    }
    /* The corpus reads the exponent+quiet field as a raw dword six bytes
     * past &lo — esp+0xa, the high half of the hi word plus two bytes
     * past the parameter, which the 0x7ff8 mask discards — and compares
     * it as a WORD; mirror the read shape through the parameter bytes. */
    const uint16_t exp_quiet = (uint16_t)(
        *(const uint32_t*)((const char*)&lo + 6) & 0x7ff8u);
    if (exp_quiet == 0x7ff8u) {
        return 3;                                   /* quiet-NaN-shaped */
    }
    if (exp_quiet == 0x7ff0u && ((hi & 0x7ffffu) != 0u || lo != 0u)) {
        return 4;                                   /* signaling-NaN-shaped */
    }
    return 0;                                       /* finite */
}

/* FUN_00499710: the 16-bit strlen — code units counted up to the NUL. */
// FUNCTION: LEGORR 0x00499710
int wide_string_length(const uint16_t* text)
{
    const uint16_t* p = text;
    uint16_t ch = *p;
    for (;;) {
        ++p;
        if (ch == 0) {
            break;
        }
        ch = *p;
    }
    return (int)(p - text) - 1;
}

/* FUN_0049add0: bounded scan — the terminator index plus one when found
 * within `bound` (count including the NUL), the bound otherwise; a zero
 * bound returns the bound without reading. */
// FUNCTION: LEGORR 0x0049add0
int bounded_wide_string_length_including_null(const uint16_t* text,
                                              int bound)
{
    if (bound != 0) {
        const uint16_t* p = text;
        int left = bound;
        while (*p != 0) {
            ++p;
            --left;
            if (left == 0) {
                break;
            }
        }
        if (left != 0 && *p == 0) {
            return (int)(p - text) + 1;
        }
    }
    return bound;
}

/* DoubleExponentBitsGetOrSpecialValue (FUN_00492068, findings 386/399):
 * exponent field for ordinary values; the untouched high word when the
 * exponent is all ones (Inf/NaN) so the caller sees the full pattern.
 * The corpus callee takes a cdecl double: the low word at [esp+4] is
 * unused, the high word at [esp+8] is live — two uint32_t params so
 * MSVC5 reads the high word at [esp+8]. */
// FUNCTION: LEGORR 0x00492068
#if defined(_MSC_VER) && defined(_M_IX86)
/* Corpus 0x492068..0x49207d tests the exponent bits in EAX, returning the
 * masked word unless it is all ones, then reloads the original high word. */
__declspec(naked) uint32_t double_exponent_bits_or_special(uint32_t low_word,
                                                            uint32_t high_word)
{
    __asm {
        mov eax, dword ptr [esp + 8]
        and eax, 07ff00000h
        cmp eax, 07ff00000h
        je special_value
        ret
    special_value:
        mov eax, dword ptr [esp + 8]
        ret
    }
}
#else
uint32_t double_exponent_bits_or_special(uint32_t low_word, uint32_t high_word)
{
    (void)low_word; /* the corpus reads only [esp+8] */
    if ((high_word & 0x7ff00000u) != 0x7ff00000u) {
        return high_word & 0x7ff00000u;
    }
    return high_word;
}
#endif

/* FormatArgumentReadDword (FUN_00491430, finding 395): read the dword
 * at the cursor, then advance it by four. */
// FUNCTION: LEGORR 0x00491430
uint32_t format_argument_read_dword(uint8_t** cursor)
{
    uint32_t value;
    memcpy(&value, *cursor, sizeof(value));
    *cursor = *cursor + 4;
    return value;
}

/* FormatArgumentReadWord (FUN_00491470, finding 395): read the word, advance
 * the cursor by four bytes, and return the word in the low half with the
 * advanced cursor's high 16 bits in the high half, as in CONCAT22. */
// FUNCTION: LEGORR 0x00491470
uint32_t format_argument_read_word(uint8_t** cursor)
{
    uint16_t value;
    memcpy(&value, *cursor, sizeof(value));
    *cursor = *cursor + 4;
    return value;
}

/* FormatArgumentReadDouble (FUN_00491450, finding 395): read eight
 * bytes, advance by eight. */
// FUNCTION: LEGORR 0x00491450
double format_argument_read_double(uint8_t** cursor)
{
    double value;
    memcpy(&value, *cursor, sizeof(value));
    *cursor = *cursor + 8;
    return value;
}

/* X87RoundToIntegral (FUN_00493150, finding 379): FRNDINT under the
 * default control word — nearest-even integral, still floating.
 * rint() matches it exactly (and keeps -0, per the x87 answer). */
// FUNCTION: LEGORR 0x00493150
double x87_round_to_integral(double value)
{
#if defined(_MSC_VER) && defined(_M_IX86)
    /* MSVC 5.0 has no rint() declaration. Keep this leaf on the same x87
     * instruction as the original CRT implementation. */
    double result;
    __asm {
        fld qword ptr [value]
        frndint
        fstp qword ptr [result]
    }
    return result;
#else
    return rint(value);
#endif
}

/* CrtRoundToIntegralWithMathErrorHandling reads the active x87 control word
 * before deciding whether FRNDINT raised an unmasked precision exception. */
static uint32_t crt_fpu_control_word_get(void)
{
    unsigned short control = 0x037fu;
#if defined(_MSC_VER) && defined(_M_IX86)
    __asm { fnstcw control }
#elif defined(__i386__) || defined(__x86_64__)
    __asm__ __volatile__("fnstcw %0" : "=m"(control));
#endif
    return (uint32_t)control;
}

static int crt_double_classify_words(uint32_t low, uint32_t high)
{
    uint32_t exponent_and_quiet = (high >> 16) & 0x7ff8u;
    if (high == 0x7ff00000u && low == 0u) {
        return 1;
    }
    if (high == 0xfff00000u && low == 0u) {
        return 2;
    }
    if (exponent_and_quiet == 0x7ff8u) {
        return 3;
    }
    if (exponent_and_quiet == 0x7ff0u &&
        ((high & 0x000fffffu) != 0u || low != 0u)) {
        return 4;
    }
    return 0;
}

/* CrtMathDispatchTableInitialize (FUN_0048d940): the six consecutive
 * callback slots contain the CRT fatal handler until startup replaces them
 * with the formatting and decimal-point routines below. Keep the image
 * addresses as DWORDs; function-valued slots use their declared symbols,
 * but are not called through host pointers here. */
// GLOBAL: LEGORR 0x004aff80
static volatile uint32_t s_math_dispatch_table[6] = {
    0x00498340u, 0x00498340u, 0x00498340u,
    0x00498340u, 0x00498340u, 0x00498340u
};

// FUNCTION: LEGORR 0x0048d940
void crt_math_dispatch_table_initialize(void)
{
    s_math_dispatch_table[1] = 0x00490310u;
    s_math_dispatch_table[0] = 0x00490750u;
    s_math_dispatch_table[2] = 0x004903a0u;
    s_math_dispatch_table[3] =
        (uint32_t)(uintptr_t)&crt_normalize_decimal_point;
    s_math_dispatch_table[4] = 0x00490380u;
    s_math_dispatch_table[5] = 0x00490750u;
}

/* CrtMathIntrinsicDispatch (FUN_0048e1aa) calls __cintrindisp2. That
 * compiler-runtime dispatcher consumes an x87 result plus EDX/EBP state
 * owned by its original call frame; a normal C ABI entry has no faithful
 * way to recover that state. Preserve the one explicit global side effect
 * identified in __cintrindisp2 while leaving the context-sensitive work
 * as a seam until its caller/runtime frame is ported. */
// GLOBAL: LEGORR 0x00549168
static volatile unsigned char s_math_intrinsic_dispatch_seen;
// GLOBAL: LEGORR 0x005490c8
static uint32_t s_math_exception_dispatch_mode;
// GLOBAL: LEGORR 0x00549160
static double s_math_exception_dispatch_result;

// FUNCTION: LEGORR 0x0048e1aa
void crt_math_intrinsic_dispatch(void)
{
    s_math_intrinsic_dispatch_seen = 1;
}

/* The two result dispatch entries are called with the caller's EBP frame and
 * x87 ST(0) still live. On the reference x86 backend, the naked adapters keep
 * that calling convention and pass the frame/status to this C implementation.
 * Other backends have no equivalent implicit x87 context. */
static void crt_floating_point_exception_dispatch_core(uint8_t* frame,
                                                        uint32_t fpu_status,
                                                        int clear_dispatch_flag)
{
    int8_t exception_kind;
    uint16_t control_word;
    uint8_t* exception_context;
    uint32_t status;
    int8_t operation;
    union {
        double value;
        uint64_t bits;
    } result;

    if (clear_dispatch_flag != 0) {
        s_math_intrinsic_dispatch_seen = 0u;
    }
    if (s_math_exception_dispatch_mode != 0u) {
        return;
    }

    exception_context = *(uint8_t**)(frame - 0x94);
    exception_kind = *(int8_t*)(frame - 0x90);
    control_word = *(uint16_t*)(frame - 0xa4);
    result.value = s_math_exception_dispatch_result;
    if (exception_kind == 0) {
        if ((control_word & 0x20u) != 0u || (fpu_status & 0x20u) == 0u) {
            return;
        }
        status = 8u;
    }
    else if (exception_kind == -1 || exception_kind == -2) {
        const uint32_t high = (uint32_t)(result.bits >> 32);
        if ((high & 0x7ff00000u) == 0u) {
            status = 4u;
        }
        else if ((*(uint16_t*)((uint8_t*)&s_math_exception_dispatch_result + 6)
                  & 0x7ff0u) == 0x7ff0u) {
            status = 3u;
        }
        else {
            if ((control_word & 0x20u) != 0u || (fpu_status & 0x20u) == 0u) {
                return;
            }
            status = 8u;
        }
    }
    else {
        status = (uint32_t)(int32_t)exception_kind;
    }

    if (s_math_intrinsic_dispatch_seen == 0u) {
        memcpy(frame - 0x86, frame + 8, 8u);
        if (exception_context[0x0d] != 1u) {
            memcpy(frame - 0x7e, frame + 0x10, 8u);
        }
    }
    operation = (int8_t)exception_context[0x0e];
    memcpy(frame - 0x76, &result.value, sizeof(result.value));
    result.value = crt_math_exception_handle_result_with_status(
        status, (uint32_t)(int32_t)operation,
        *(uint32_t*)(frame - 0x86), *(uint32_t*)(frame - 0x82),
        *(uint32_t*)(frame - 0x76), *(uint32_t*)(frame - 0x72),
        control_word);
    s_math_exception_dispatch_result = result.value;
}

// FUNCTION: LEGORR 0x0049221e
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) void crt_floating_point_exception_result_dispatch(void)
{
    __asm {
        fst qword ptr [s_math_exception_dispatch_result]
        fnstsw ax
        movzx eax, ax
        push eax
        mov eax, ebp
        push eax
        push 1
        call crt_floating_point_exception_dispatch_core
        add esp, 0ch
        fld qword ptr [s_math_exception_dispatch_result]
        fldcw word ptr [ebp-0a4h]
        fwait
        ret
    }
}

// FUNCTION: LEGORR 0x00492225
__declspec(naked) void crt_floating_point_exception_result_dispatch_variant(void)
{
    __asm {
        fst qword ptr [s_math_exception_dispatch_result]
        fnstsw ax
        movzx eax, ax
        push eax
        mov eax, ebp
        push eax
        push 0
        call crt_floating_point_exception_dispatch_core
        add esp, 0ch
        fld qword ptr [s_math_exception_dispatch_result]
        fldcw word ptr [ebp-0a4h]
        fwait
        ret
    }
}
#else
void crt_floating_point_exception_result_dispatch(void)
{
    s_math_intrinsic_dispatch_seen = 0u;
}

void crt_floating_point_exception_result_dispatch_variant(void)
{
}
#endif

/* CrtDoubleRemainderReduction / CrtFmodExtendedPrecision (FUN_00492bc6 /
 * FUN_00492dcc): the original entries exchange extended x87 values through
 * ST(0)/ST(1) and use a private stack record. The C interface accepts the same
 * operands explicitly; the CRT fmod implementation supplies truncating
 * remainder semantics and handles NaN, infinity, and zero-divisor cases. */
// FUNCTION: LEGORR 0x00492bc6
double crt_double_remainder_reduction(double dividend, double divisor)
{
    return fmod(dividend, divisor);
}

// FUNCTION: LEGORR 0x00492dcc
double crt_fmod_extended_precision(double dividend, double divisor)
{
    return crt_double_remainder_reduction(dividend, divisor);
}

/* CrtRoundToIntegralWithMathErrorHandling (FUN_0048e280): for finite
 * values the runtime uses FRNDINT, returning that x87 result. The full CRT
 * routes an unmasked precision exception and special values through
 * 0x493170/0x4931c0; those handlers depend on the not-yet-ported matherr
 * and FPU-control-word paths. FRNDINT itself preserves the target rounding
 * mode and raises the hardware precision flag. */
// FUNCTION: LEGORR 0x0048e280
double crt_round_to_integral_with_math_error_handling(double value)
{
    union {
        double value;
        uint64_t bits;
    } input, rounded;
    uint32_t control = crt_fpu_control_word_get();
    uint32_t words[2];
    int classification;

    input.value = value;
    words[0] = (uint32_t)input.bits;
    words[1] = (uint32_t)(input.bits >> 32);
    if (((words[1] >> 20) & 0x7ffu) != 0x7ffu) {
        rounded.value = x87_round_to_integral(value);
        if (rounded.bits != input.bits && (control & 0x20u) == 0u) {
            return crt_math_exception_handle_result_with_status(
                0x10u, 0x0bu, words[0], words[1],
                (uint32_t)rounded.bits, (uint32_t)(rounded.bits >> 32),
                control);
        }
        return rounded.value;
    }

    classification = crt_double_classify_words(words[0], words[1]);
    if (classification == 1 || classification == 2) {
        return value;
    }
    if (classification == 3) {
        return crt_math_exception_handle_single_result(
            0x0bu, words[0], words[1], control);
    }
    rounded.value = value + 1.0;
    return crt_math_exception_handle_result_with_status(
        8u, 0x0bu, words[0], words[1],
        (uint32_t)rounded.bits, (uint32_t)(rounded.bits >> 32), control);
}

/* CrtRoundToIntegralWithMathErrorHandlingVariant (FUN_0048e8e0) has the
 * same FRNDINT finite path as FUN_0048e280. Its distinct math exception
 * operation code is 0x0c (the first routine uses 0x0b); dispatching that
 * error path requires the unported 0x493170/0x4931c0 handler chain. */
// FUNCTION: LEGORR 0x0048e8e0
double crt_round_to_integral_with_math_error_handling_variant(double value)
{
    union {
        double value;
        uint64_t bits;
    } input, rounded;
    uint32_t control = crt_fpu_control_word_get();
    uint32_t words[2];
    int classification;

    input.value = value;
    words[0] = (uint32_t)input.bits;
    words[1] = (uint32_t)(input.bits >> 32);
    if (((words[1] >> 20) & 0x7ffu) != 0x7ffu) {
        rounded.value = x87_round_to_integral(value);
        if (rounded.bits != input.bits && (control & 0x20u) == 0u) {
            return crt_math_exception_handle_result_with_status(
                0x10u, 0x0cu, words[0], words[1],
                (uint32_t)rounded.bits, (uint32_t)(rounded.bits >> 32),
                control);
        }
        return rounded.value;
    }

    classification = crt_double_classify_words(words[0], words[1]);
    if (classification == 1 || classification == 2) {
        return value;
    }
    if (classification == 3) {
        return crt_math_exception_handle_single_result(
            0x0cu, words[0], words[1], control);
    }
    rounded.value = value + 1.0;
    return crt_math_exception_handle_result_with_status(
        8u, 0x0cu, words[0], words[1],
        (uint32_t)rounded.bits, (uint32_t)(rounded.bits >> 32), control);
}

/* CrtDoubleAbsoluteValue (FUN_0048e680): the finite lane uses ABS, while
 * infinities are classified explicitly. Quiet/signaling NaNs still use the
 * sign-clearing result because their CRT math-error dispatch is unported. */
// FUNCTION: LEGORR 0x0048e680
double crt_double_absolute_value(double value)
{
    uint32_t words[2];
    uint32_t high_word;
    int classification;

    memcpy(words, &value, sizeof(words));
    high_word = words[1];
    if (((high_word >> 16) & 0x7ff0u) != 0x7ff0u) {
        return fabs(value);
    }
    classification = crt_double_classify_words(words[0], words[1]);
    if (classification == 1) {
        return value;
    }
    if (classification == 2) {
        return -value;
    }

    if (classification == 3) {
        return crt_math_exception_handle_single_result(
            0x15u, words[0], words[1], crt_fpu_control_word_get());
    }
    {
        union {
            double value;
            uint64_t bits;
        } result;
        result.value = value + 1.0;
        return crt_math_exception_handle_result_with_status(
            8u, 0x15u, words[0], words[1],
            (uint32_t)result.bits, (uint32_t)(result.bits >> 32),
            crt_fpu_control_word_get());
    }
}

/* CrtRunCallbackArray (FUN_0048e5a0): invoke each nonnull callback in the
 * corpus's forward, half-open pointer range. */
// FUNCTION: LEGORR 0x0048e5a0
void crt_run_callback_array(CrtCallback* start, CrtCallback* end)
{
    while (start < end) {
        if (*start != NULL) {
            (*start)();
        }
        ++start;
    }
}

/* DoubleBuildWithExponent (FUN_00493a40, findings 383/399): the corpus
 * builds the floating value from two significand words and the
 * exponent-bias input; Ghidra renders the recipe as CONCAT chains,
 * which decompose exactly to: top word = ((high >> 16) & 0x800f) |
 * ((exponent + 0x3fe) << 4), and the 16-bit halves of `high` plus `low`
 * under it. The sign bit and the fraction nibble of `high` survive the
 * mask, the 16-bit ops truncate like the corpus's, and the assembled
 * bits are bitcast to the port's double (x87 deviation, as documented
 * across this band). What the built value MEANS stays with the caller
 * (the frexp band); this leaf is the faithful bit construction. */
// FUNCTION: LEGORR 0x00493a40
double double_build_with_exponent(uint32_t significand_low,
                                  uint32_t significand_high,
                                  int16_t exponent)
{
    union {
        double value;
        struct {
            uint32_t low;
            uint32_t high;
        } words;
    } result;
    const uint32_t exponent_bits =
        (uint32_t)((int32_t)exponent + 0x3fe) << 4;
    const uint32_t high_word =
        ((significand_high >> 16) & 0x800fu) | exponent_bits;
    result.words.low = significand_low;
    result.words.high = (significand_high & 0xffffu) | (high_word << 16);
    return result.value;
}


/* ------------------------------------------------------------------ *
 *  Constant and DAT-global band (see crt.hpp). */

/* The corpus's .rdata +0.0f (file bytes verified 00 00 00 00): the
 * leaf flds a DWORD data operand (the s_global_float_00556f00 fld
 * width), so the zero rides a file-scope float carrier — the
 * float-to-double widening is free on the x87 return stack. */
// GLOBAL: LEGORR 0x0049e084
static const float  s_zero_float     = 0.0f; /* DAT_0049e084 */

// FUNCTION: LEGORR 0x00408930
double constant_zero_float_get(void)
{
    return (double)s_zero_float; /* corpus: fld DWORD ds:0049e084 / ret */
}

// FUNCTION: LEGORR 0x0047c160
int32_t constant_negative_10000_get(void)
{
    return -10000;
}

// FUNCTION: LEGORR 0x00490230
uint32_t processor_feature_fallback_false(void)
{
    return 0u;
}

/* The original resolves this optional Kernel32 export at runtime because
 * older Windows releases may not provide it. The native miniwin backend
 * has no equivalent processor-feature query, so it follows the original
 * missing-export fallback. */
// FUNCTION: LEGORR 0x00490280
uint32_t crt_floating_point_feature_probe(void)
{
#if defined(LEGORRR_BACKEND_WINAPI)
    typedef BOOL (WINAPI *ProcessorFeatureProbe)(DWORD);
    HMODULE module = GetModuleHandleA("KERNEL32");
    FARPROC address;
    ProcessorFeatureProbe probe;
    if (module == NULL) {
        return processor_feature_fallback_false();
    }
    address = GetProcAddress(module, "IsProcessorFeaturePresent");
    if (address == NULL) {
        return processor_feature_fallback_false();
    }
    probe = (ProcessorFeatureProbe)address;
    return probe(0) ? 1u : 0u;
#else
    return processor_feature_fallback_false();
#endif
}

// FUNCTION: LEGORR 0x00456210
uint32_t flh_magic_uu_noop(void)
{
    return 0u;
}

/* The two DAT globals, static-backed until the corpus's writers (all
 * still unported) land — message.cpp's s_system_state precedent. The
 * float width is the corpus's own: 0x4230b0 flds a DWORD. */
// GLOBAL: LEGORR 0x00556f00
static float    s_global_float_00556f00;
// GLOBAL: LEGORR 0x004ff9bc
static uint32_t s_global_dword_004ff9bc;

// FUNCTION: LEGORR 0x004230b0
float global_float_field_00556f00_get(void)
{
    return s_global_float_00556f00;
}

// FUNCTION: LEGORR 0x004567c0
/* Corpus 0x4567c4..0x4567ce stores one load and reloads the argument for
 * return. Keep the port's NULL behavior because test_static_globals pins it. */
uint32_t global_value_004ff9bc_store_and_return(const void* value)
{
    if (value == NULL) {
        return 0u; /* test-pinned deviation: corpus dereferences blindly */
    }
    s_global_dword_004ff9bc = *(const uint32_t*)value;
    return *(const uint32_t*)value;
}

/* --- unowned DAT-global counter/accumulator band (see crt.hpp) -----------
 * None of these globals has a ported owner or feeder, so each lands as a
 * zero-initialized module static (the s_system_state precedent). */
// GLOBAL: LEGORR 0x004ff990
static uint32_t s_counter_004ff990 = 0u;
static float    s_float_00554adc   = 0.0f;
static float    s_float_00554efc   = 0.0f;
// GLOBAL: LEGORR 0x00555324
static uint32_t s_counter_00555324 = 0u;
// GLOBAL: LEGORR 0x00555328
static uint32_t s_counter_00555328 = 0u;
// GLOBAL: LEGORR 0x0055532c
static uint32_t s_counter_0055532c = 0u;
// GLOBAL: LEGORR 0x00555330
static float    s_float_00555330   = 0.0f;
// GLOBAL: LEGORR 0x00555120
static float    s_float_00555120   = 0.0f;

// FUNCTION: LEGORR 0x00454880
void global_counter_004ff990_increment(void)
{
    s_counter_004ff990 = s_counter_004ff990 + 1u;
}

// FUNCTION: LEGORR 0x00454890
void global_counter_004ff990_reset(void)
{
    s_counter_004ff990 = 0u;
}

// FUNCTION: LEGORR 0x00462830
void global_float_at_00554adc_add_one(void)
{
    s_float_00554adc = s_float_00554adc + 1.0f; /* corpus: x - -1.0 */
}

// FUNCTION: LEGORR 0x00462850
void global_float_at_00554efc_add_one(void)
{
    s_float_00554efc = s_float_00554efc + 1.0f;
}

// FUNCTION: LEGORR 0x00462870
void global_counter_at_00555324_increment(void)
{
    s_counter_00555324 = s_counter_00555324 + 1u;
}

// FUNCTION: LEGORR 0x00462880
void global_counter_at_00555328_increment(void)
{
    s_counter_00555328 = s_counter_00555328 + 1u;
}

// FUNCTION: LEGORR 0x00462890
void global_counter_at_0055532c_increment(void)
{
    s_counter_0055532c = s_counter_0055532c + 1u;
}

// FUNCTION: LEGORR 0x004628a0
void global_float_accumulator_00555330_add(float value)
{
    s_float_00555330 = s_float_00555330 + value;
}

// FUNCTION: LEGORR 0x004628c0
void global_float_accumulate_00555120(float value)
{
    s_float_00555120 = s_float_00555120 + value;
}

uint32_t crt_counter_004ff990_peek(void) { return s_counter_004ff990; }
float    crt_float_00554adc_peek(void)   { return s_float_00554adc; }
float    crt_float_00554efc_peek(void)   { return s_float_00554efc; }
uint32_t crt_counter_00555324_peek(void) { return s_counter_00555324; }
uint32_t crt_counter_00555328_peek(void) { return s_counter_00555328; }
uint32_t crt_counter_0055532c_peek(void) { return s_counter_0055532c; }
float    crt_float_00555330_peek(void)   { return s_float_00555330; }
float    crt_float_00555120_peek(void)   { return s_float_00555120; }

/* --- DirectDraw display-mode records + filter tuples -------------------- */
typedef struct FilterTuple {
    int32_t w, h, bpp;
    uint8_t padding[0x104]; /* the corpus advances by 0x110 bytes per row */
} FilterTuple;

#define FILTER_TUPLE_ENTRIES 16
#define DISPLAY_MODE_ENTRIES CRT_DISPLAY_MODE_ENTRIES
#define DISPLAY_MODE_KINDS 2
static CrtDisplayModeEntry
    s_display_modes[DISPLAY_MODE_KINDS][CRT_DISPLAY_MODE_ENTRIES];
static int s_display_mode_counts[DISPLAY_MODE_KINDS];
static int s_active_display_mode;
static CrtDisplayResolutionEntry
    s_display_resolutions[CRT_DISPLAY_RESOLUTION_ENTRIES];
static int s_display_resolution_count;
static int s_display_resolution_enumeration_succeeded;
static FilterTuple s_filter_tuples[FILTER_TUPLE_ENTRIES];
static int         s_filter_tuple_count;

/* Caller-side DirectDraw enumeration replacement. This record occupies
 * 0x110 bytes, with `name` at the same +0x0c offset as the native list. */
int display_mode_entry_append(int mode, int width, int height, int bpp,
                              const char* name)
{
    const int kind = mode != 0 ? 1 : 0;
    int* count = &s_display_mode_counts[kind];
    if (name == NULL || *count >= DISPLAY_MODE_ENTRIES) {
        return 0;
    }
    CrtDisplayModeEntry* entry = &s_display_modes[kind][*count];
    entry->width = width;
    entry->height = height;
    entry->bpp = bpp;
    size_t length = strlen(name);
    if (length >= sizeof(entry->name)) {
        length = sizeof(entry->name) - 1;
    }
    memcpy(entry->name, name, length);
    entry->name[length] = '\0';
    ++*count;
    return 1;
}

int display_mode_entry_count(void)
{
    return s_display_mode_counts[s_active_display_mode];
}

int display_mode_entry_get(int index, CrtDisplayModeEntry* out)
{
    if (out == NULL || index < 0 ||
        index >= s_display_mode_counts[s_active_display_mode]) {
        return 0;
    }
    *out = s_display_modes[s_active_display_mode][index];
    return 1;
}

/* DisplayModeNameIndexFind (FUN_0049d950) compares mode-name bytes exactly
 * and returns the first matching row's zero-based index. */
// FUNCTION: LEGORR 0x0049D950
int display_mode_name_index_find(const char* name, uint32_t* out_index)
{
    const uint32_t count = (uint32_t)s_display_mode_counts[s_active_display_mode];
    const CrtDisplayModeEntry* row = s_display_modes[s_active_display_mode];
    for (uint32_t index = 0; index < count; ++index, ++row) {
        if (strcmp(name, row->name) == 0) {
            *out_index = index;
            return 1;
        }
    }
    return 0;
}

/* DisplayModeListPopulate (FUN_0049d630): form the exact supported-mode
 * index list for a dialog. The native combo box is represented by an
 * ordered array; the first match is selected, and an empty list disables
 * the dialog's default action. */
// FUNCTION: LEGORR 0x0049D630
void display_mode_list_populate(int mode, int desktop_bpp,
                                CrtDisplayModeList* out)
{
    if (out == NULL) return;
    s_active_display_mode = mode != 0 ? 1 : 0;
    memset(out, 0, sizeof(*out));
    out->desktop_bpp = desktop_bpp;
    out->selected_mode_index = 0xffffffffu;
    out->empty_message_kind = mode == 0 ? 1 : 2;
    const int mode_count = s_display_mode_counts[s_active_display_mode];
    for (int index = 0; index < mode_count; ++index) {
        if (!display_mode_matches_filter_tuple(index)) continue;
        if (out->count >= CRT_DISPLAY_MODE_ENTRIES) break;
        out->mode_indices[out->count++] = (uint32_t)index;
    }
    out->default_action_enabled = out->count != 0;
    out->selected_index = 0;
    if (out->count != 0) {
        out->selected_mode_index = out->mode_indices[0];
        out->empty_message_kind = 0;
    }
}

/* DisplayModeDialogSetModeAndRefreshChoices (FUN_0049d5e0): store the
 * windowed/fullscreen selector, repopulate its list, and select the
 * matching localized label resource. */
// FUNCTION: LEGORR 0x0049D5E0
void display_mode_dialog_set_mode_and_refresh_choices(
    CrtDisplayModeDialog* dialog, int mode, int desktop_bpp)
{
    if (dialog == NULL) return;
    dialog->mode = mode;
    dialog->mode_label_resource = mode != 0 ? 0x004b2d44u : 0x004b2d38u;
    display_mode_list_populate(mode, desktop_bpp, &dialog->modes);
}

/* DisplayRefreshRateControlsSync (FUN_0049d9d0): adapter flag 0x20 controls
 * whether the refresh-rate selector is available. When unavailable, force
 * the dialog to its non-refresh mode and remember the previous mode for
 * restoration if a later adapter supports refresh selection. */
// FUNCTION: LEGORR 0x0049D9D0
void display_refresh_rate_controls_sync(CrtDisplayModeDialog* dialog,
                                        uint32_t adapter_flags,
                                        int desktop_bpp)
{
    if (dialog == NULL) return;
    if ((adapter_flags & 0x20u) == 0u) {
        dialog->previous_mode = dialog->mode;
        display_mode_dialog_set_mode_and_refresh_choices(dialog, 1,
                                                         desktop_bpp);
        dialog->refresh_rate_control_enabled = 0;
        dialog->checkbox_3ed_checked = 0;
        dialog->checkbox_3ec_checked = 1;
        return;
    }

    dialog->refresh_rate_control_enabled = 1;
    if (dialog->previous_mode == 0) {
        display_mode_dialog_set_mode_and_refresh_choices(dialog, 0,
                                                         desktop_bpp);
        dialog->checkbox_3ec_checked = 0;
        dialog->checkbox_3ed_checked = 1;
    }
}

/* Platform enumeration handoff for DisplayResolutionComboPopulate. Reset
 * starts a new DirectDraw enumeration result; records retain the names and
 * 0x2000 preferred-mode bit that the native combo builder consumes. */
void display_resolution_entries_reset(int enumeration_succeeded)
{
    s_display_resolution_count = 0;
    s_display_resolution_enumeration_succeeded =
        enumeration_succeeded != 0;
}

int display_resolution_entry_append(const char* name, uint32_t flags)
{
    if (!s_display_resolution_enumeration_succeeded || name == NULL ||
        s_display_resolution_count >= CRT_DISPLAY_RESOLUTION_ENTRIES) {
        return 0;
    }
    CrtDisplayResolutionEntry* entry =
        &s_display_resolutions[s_display_resolution_count];
    size_t length = strlen(name);
    if (length >= sizeof(entry->name)) {
        length = sizeof(entry->name) - 1;
    }
    memcpy(entry->name, name, length);
    entry->name[length] = '\0';
    entry->flags = flags;
    ++s_display_resolution_count;
    return 1;
}

int display_resolution_entry_count(void)
{
    return s_display_resolution_count;
}

int display_resolution_entry_get(int index,
                                 CrtDisplayResolutionEntry* out)
{
    if (out == NULL || index < 0 || index >= s_display_resolution_count) {
        return 0;
    }
    *out = s_display_resolutions[index];
    return 1;
}

/* DisplayResolutionComboPopulate (FUN_0049d780): include every enumerated
 * display resolution, selecting the last row marked with 0x2000. Failure
 * contributes the DirectX error resource as one placeholder row. */
// FUNCTION: LEGORR 0x0049D780
void display_resolution_combo_populate(CrtDisplayResolutionList* out)
{
    if (out == NULL) return;
    memset(out, 0, sizeof(*out));
    if (!s_display_resolution_enumeration_succeeded) {
        out->count = 1;
        out->error_resource = 0x004b2d90u;
        return;
    }
    out->count = (uint32_t)s_display_resolution_count;
    for (int index = 0; index < s_display_resolution_count; ++index) {
        if ((s_display_resolutions[index].flags & 0x2000u) != 0u) {
            out->selected_index = (uint32_t)index;
        }
    }
}

// FUNCTION: LEGORR 0x0049d870
void display_resolution_filter_tuple_append(int width, int height, int bpp)
{
    if (s_filter_tuple_count < FILTER_TUPLE_ENTRIES) {
        s_filter_tuples[s_filter_tuple_count].w = width;
        s_filter_tuples[s_filter_tuple_count].h = height;
        s_filter_tuples[s_filter_tuple_count].bpp = bpp;
    }
    s_filter_tuple_count = s_filter_tuple_count + 1;
}

int display_resolution_filter_tuple_count(void)
{
    return s_filter_tuple_count;
}

void display_resolution_filter_tuple_get(int index, int* w, int* h, int* bpp)
{
    const int n = (index >= 0 && index < s_filter_tuple_count
                   && index < FILTER_TUPLE_ENTRIES)
        ? index : -1;
    if (w != NULL) {
        *w = (n >= 0) ? s_filter_tuples[n].w : 0;
    }
    if (h != NULL) {
        *h = (n >= 0) ? s_filter_tuples[n].h : 0;
    }
    if (bpp != NULL) {
        *bpp = (n >= 0) ? s_filter_tuples[n].bpp : 0;
    }
}

/* DisplayModeMatchesFilterTuple (FUN_0049d8d0): any registered tuple may
 * match; zero tuple fields are wildcards, as in the DirectDraw path. */
// FUNCTION: LEGORR 0x0049D8D0
int display_mode_matches_filter_tuple(int mode_index)
{
    if (s_filter_tuple_count <= 0) {
        return 1;
    }
    if (mode_index < 0 ||
        mode_index >= s_display_mode_counts[s_active_display_mode]) {
        return 0;
    }
    const CrtDisplayModeEntry* mode =
        &s_display_modes[s_active_display_mode][mode_index];
    int count = s_filter_tuple_count;
    if (count > FILTER_TUPLE_ENTRIES) {
        count = FILTER_TUPLE_ENTRIES;
    }
    for (int i = 0; i < count; ++i) {
        const FilterTuple* tuple = &s_filter_tuples[i];
        if ((tuple->w == 0 || tuple->w == mode->width) &&
            (tuple->h == 0 || tuple->h == mode->height) &&
            (tuple->bpp == 0 || tuple->bpp == mode->bpp)) {
            return 1;
        }
    }
    return 0;
}

/* --- the remaining unowned DAT one-liners (see crt.hpp) ---------------------
 * Zero-initialized module statics per the s_system_state precedent — the
 * globals' meanings are unresolved (finding 383) and none has a ported
 * feeder. */
// GLOBAL: LEGORR 0x004ff978
static uint32_t s_flag_004ff978 = 0u;
// GLOBAL: LEGORR 0x004ffe48
static uint32_t s_value_004ffe48 = 0u;

// FUNCTION: LEGORR 0x00456640
uint32_t global_flag_004ff978_get(void)
{
    return s_flag_004ff978;
}

// FUNCTION: LEGORR 0x00458660
void global_value_004ffe48_set(uint32_t value)
{
    s_value_004ffe48 = value;
}

/* --- the unowned DAT word-store band (see crt.hpp) -------------------------- */
// GLOBAL: LEGORR 0x004b31bc
static uint32_t s_advisor_state_word = 0u; /* DAT_004b31bc */
// GLOBAL: LEGORR 0x00557860
static uint32_t s_task_refresh_flag = 0u;  /* DAT_00557860 */
// GLOBAL: LEGORR 0x004b8a54
static uint32_t s_level_stored_word = 0u;  /* DAT_004b8a54 */
// GLOBAL: LEGORR 0x004c7178
static uint32_t s_mode_zero_flag = 0u;     /* DAT_004c7178 */
// GLOBAL: LEGORR 0x004c79a4
static uint32_t s_rockfall_base = 0u;      /* DAT_004c79a4 */
// GLOBAL: LEGORR 0x004c7ea8
static uint32_t s_ui_flag_word = 0u;       /* DAT_004c7ea8 */
static uint32_t s_config_word = 0u;        /* DAT_004a1ee4 */

// FUNCTION: LEGORR 0x00401c20
uint32_t global_flag_bit0_is_set(void)
{
    return s_advisor_state_word & 1u;
}

// FUNCTION: LEGORR 0x00407340
void state_flag_toggle_when_zero(void)
{
    s_task_refresh_flag = (s_task_refresh_flag == 0u) ? 1u : 0u;
}

// FUNCTION: LEGORR 0x00407360
uint32_t global_task_refresh_flag_get(void)
{
    return s_task_refresh_flag;
}

// FUNCTION: LEGORR 0x00409d40
void level_state_stored_value_set(uint32_t v)
{
    s_level_stored_word = v;
}

// FUNCTION: LEGORR 0x0040ab10
void global_mode_zero_input_set(int value)
{
    s_mode_zero_flag = (value == 0) ? 1u : 0u;
}

uint32_t global_mode_zero_input_peek(void)
{
    return s_mode_zero_flag;
}

// FUNCTION: LEGORR 0x0040bfc0
void rockfall_style_table_base_set(uint32_t v)
{
    s_rockfall_base = v;
}

// FUNCTION: LEGORR 0x0040e7e0
void global_flag_bit0_clear(void)
{
    /* corpus: mov eax,ds:004c7ea8 / and al,0FEh / mov ds:004c7ea8,eax.
     * The plain load-op-store spelling (not the &= compound) keeps the
     * /Od register round-trip instead of folding to and mem,imm; the
     * corpus's byte-sized and is MSVC's own narrowing of the mask. */
    s_ui_flag_word = s_ui_flag_word & ~1u;
}

// FUNCTION: LEGORR 0x0040f5e0
void global_config_value_set(uint32_t v)
{
    s_config_word = v;
}

uint32_t advisor_state_word_peek(void)  { return s_advisor_state_word; }
void     advisor_state_word_set(uint32_t v) { s_advisor_state_word = v; }
uint32_t level_stored_word_peek(void)   { return s_level_stored_word; }
uint32_t mode_zero_input_flag_peek(void){ return s_mode_zero_flag; }
uint32_t rockfall_table_base_peek(void) { return s_rockfall_base; }
uint32_t ui_flag_word_peek(void)        { return s_ui_flag_word; }
uint32_t global_config_word_peek(void)  { return s_config_word; }

/* --- display dimensions + request-record predicates (see crt.hpp) ---------- */
// GLOBAL: LEGORR 0x00505924
static int s_display_w;   /* DAT_00505924 */
// GLOBAL: LEGORR 0x00505928
static int s_display_h;   /* DAT_00505928 */

// FUNCTION: LEGORR 0x00401c30
int display_width_get(void)  { return s_display_w; }
// FUNCTION: LEGORR 0x00401c40
int display_height_get(void) { return s_display_h; }

void display_dimensions_set(int w, int h)
{
    s_display_w = w;
    s_display_h = h;
}

/* DAT_00500110 record block: 0x30-byte panel-category records. The corpus
 * 0x45a6e0 pair setter addresses +0x18/+0x1c at that stride; 0x45ab00 reads
 * the flag dword at +0x2c. The image bytes are zero-filled and populated at
 * runtime, so model only the fields used by this port. */
#define REQUEST_RECORD_COUNT 18

typedef struct RequestRecord {
    uint8_t  pad[0x18];
    uint32_t pair_x;  /* +0x18: DAT_00500128, 0x30-byte category stride */
    uint32_t pair_y;  /* +0x1c: DAT_0050012c, 0x30-byte category stride */
    uint8_t  reserved[0x0c];
    uint32_t flags;   /* +0x2c */
} RequestRecord;

// GLOBAL: LEGORR 0x00500110
static RequestRecord s_request_records[REQUEST_RECORD_COUNT];

// FUNCTION: LEGORR 0x0045ab30
int request_record_ready_flag4(int index)
{
    if ((unsigned)index >= REQUEST_RECORD_COUNT) {
        return 0; /* deviation: the corpus strides blindly */
    }
    RequestRecord* record = s_request_records + index;
    const uint32_t f = record->flags;
    return (f & 4u) != 0u && (f & 8u) == 0u ? 1 : 0;
}

/* The corpus's 0x45ab6a/6f sequence adds the unchecked 0x30-byte record
 * address to DAT_00500110, then loads +0x2c; the 0x45ab72/78 tests keep bit
 * 2 then bit 8 in this short-circuit order. Keep the pointer step explicit
 * rather than folding the field offset into the global's displacement. */
// FUNCTION: LEGORR 0x0045ab60
int request_record_ready_flag2(int index)
{
    RequestRecord* record = s_request_records + index;
    const uint32_t f = record->flags;
    if ((f & 2u) != 0u && (f & 8u) == 0u) {
        return 1;
    }
    return 0;
}

// FUNCTION: LEGORR 0x0045ab90
int request_record_flag8_is_set(int index)
{
    if ((unsigned)index >= REQUEST_RECORD_COUNT) {
        return 0; /* deviation: the corpus strides blindly */
    }
    return (int)(s_request_records[index].flags & 8u);
}

void request_record_flags_set(int index, uint32_t flags)
{
    if ((unsigned)index < REQUEST_RECORD_COUNT) {
        s_request_records[index].flags = flags;
    }
}

uint32_t request_record_flags_peek(int index)
{
    if ((unsigned)index >= REQUEST_RECORD_COUNT) {
        return 0u;
    }
    return s_request_records[index].flags;
}

/* --- the remaining DAT one-liner stores/reads (see crt.hpp) ----------------- */
// GLOBAL: LEGORR 0x0050080c
static uint32_t s_value_0050080c = 0u;
static uint32_t s_font_words[16];   /* DAT_00500a9c window */
static uint32_t s_cursor_slot = 0u; /* DAT_00500a98 */

// FUNCTION: LEGORR 0x0045c4a0
void global_value_0050080c_set(uint32_t value)
{
    s_value_0050080c = value;
}

uint32_t global_value_0050080c_peek(void)
{
    return s_value_0050080c;
}

// FUNCTION: LEGORR 0x0045cc10
uint32_t indexed_global_value_at_get(int index)
{
    if (index < 0 || index >= 16) {
        return 0u; /* deviation: the corpus strides blindly */
    }
    return s_font_words[index];
}

// FUNCTION: LEGORR 0x0045cc60
uint32_t global_cursor_word_get(void)
{
    return s_cursor_slot;
}

void font_cursor_word_set(int index, uint32_t v)
{
    if (index == -1) {
        s_cursor_slot = v; /* the DAT_00500a98 word itself */
        return;
    }
    if (index >= 0 && index < 16) {
        s_font_words[index] = v;
    }
}

/* --- pure-return stubs + advisor word clear (see crt.hpp) -------------------- */
// FUNCTION: LEGORR 0x00491ff5
void crt_no_op_stub(void)
{
    /* FUN_00491ff5: pure return — faithful. */
}

/* CrtFpuPrecisionModeCodeGet (FUN_0049200c): this helper consumes the
 * caller's EAX value directly and returns 7 when bit 0x80000 is set, else
 * 1. Its default arm also adds the corpus's 1.0 operand to ST(0). Preserve
 * both register contracts on the original MSVC5 x86 target. */
#if defined(_MSC_VER) && defined(_M_IX86)
#define CRT_FPU_PRECISION_NAKED __declspec(naked)
/* Corpus 0x492019 adds its 1.0 operand (PE bytes: 00 00 00 00 00 00 f0 3f). */
static const double s_fpu_precision_mode_one = 1.0;
#else
#define CRT_FPU_PRECISION_NAKED
#endif
// FUNCTION: LEGORR 0x0049200c
CRT_FPU_PRECISION_NAKED uint32_t crt_fpu_precision_mode_code_get(void)
{
#if defined(_MSC_VER) && defined(_M_IX86)
    __asm {
        test eax, 80000h
        jz precision_default
        mov eax, 7
        ret
    precision_default:
        fadd qword ptr [s_fpu_precision_mode_one]
        mov eax, 1
        ret
    }
#else
    /* Portable callers do not have the original implicit EAX input. */
    return 1u;
#endif
}
#undef CRT_FPU_PRECISION_NAKED

// FUNCTION: LEGORR 0x00497870
void crt_no_op_stub_variant(void)
{
    /* FUN_00497870: pure return — faithful. */
}

// FUNCTION: LEGORR 0x00401b20
void advisor_clear_active_state(void)
{
    s_advisor_state_word &= ~2u;
}

// FUNCTION: LEGORR 0x0048e030
void byte_range_swap(void* a, void* b, size_t n)
{
    if (a == NULL || b == NULL) {
        return; /* deviation: the corpus reads blindly */
    }
    if (a == b) {
        return; /* the corpus's same-range shortcut */
    }
    unsigned char* pa = (unsigned char*)a;
    unsigned char* pb = (unsigned char*)b;
    for (size_t i = 0; i < n; ++i) {
        const unsigned char t = pa[i];
        pa[i] = pb[i];
        pb[i] = t;
    }
}

/* --- the DAT window band (see crt.hpp) ---------------------------------------
 * Auxiliary tables are zero-initialized statics. The panel position pair
 * lives in the corresponding 0x30-byte RequestRecord, preserving the
 * corpus's indexed record stride; public port hooks remain guarded. */
#define DAT_WINDOW_COUNT 18

static float    s_fpair_a[DAT_WINDOW_COUNT]; /* DAT_00552144 entries */
static float    s_fpair_b[DAT_WINDOW_COUNT]; /* DAT_00552148 entries */
static void*    s_metric_records[DAT_WINDOW_COUNT]; /* DAT_00552154 */
static uint32_t s_region_xy[2];  /* DAT_005013a8/3ac */
static float    s_region_wh[2];  /* DAT_005013b0/b4 */
static void*    s_panel_records[DAT_WINDOW_COUNT]; /* DAT_00500134 */


// FUNCTION: LEGORR 0x0045a6e0
void indexed_pair_set(int index, uint32_t x, uint32_t y)
{
    if (index < 0 || index >= DAT_WINDOW_COUNT) {
        return; /* deviation: the corpus strides blindly */
    }
    s_request_records[index].pair_x = x;
    s_request_records[index].pair_y = y;
}

// FUNCTION: LEGORR 0x0045a700
void indexed_pair_get(int index, uint32_t* out_x, uint32_t* out_y)
{
    if (index < 0 || index >= DAT_WINDOW_COUNT
        || out_x == NULL || out_y == NULL) {
        return; /* deviation: the corpus dereferences blindly */
    }
    *out_x = s_request_records[index].pair_x;
    *out_y = s_request_records[index].pair_y;
}

// FUNCTION: LEGORR 0x00463730
void indexed_float_pair_set(int index, uint32_t x, uint32_t y)
{
    if (index < 0 || index >= DAT_WINDOW_COUNT) {
        return; /* deviation: the corpus strides blindly */
    }
    s_fpair_a[index] = (float)x; /* the corpus's uint->float cast */
    s_fpair_b[index] = (float)y;
}

void indexed_float_pair_peek(int index, float* out_x, float* out_y)
{
    if (index < 0 || index >= DAT_WINDOW_COUNT || out_x == NULL
        || out_y == NULL) {
        return;
    }
    *out_x = s_fpair_a[index];
    *out_y = s_fpair_b[index];
}

// FUNCTION: LEGORR 0x00463ed0
void indexed_text_metrics_pair_set(int index, uint32_t x, uint32_t y)
{
    if (index < 0 || index >= DAT_WINDOW_COUNT) {
        return;
    }
    char* rec = (char*)s_metric_records[index];
    if (rec == NULL) {
        return; /* deviation: the corpus dereferences its slot blindly */
    }
    __builtin_memcpy(rec + 0x1c, &x, sizeof(x));
    __builtin_memcpy(rec + 0x20, &y, sizeof(y));
    const uint32_t one = 1u; /* the corpus's dirty flag */
    __builtin_memcpy(rec + 0x24, &one, sizeof(one));
}

// FUNCTION: LEGORR 0x00463ea0
void indexed_text_metrics_pair_get(int index, uint32_t* out_x,
                                   uint32_t* out_y)
{
    if (index < 0 || index >= DAT_WINDOW_COUNT
        || out_x == NULL || out_y == NULL) {
        return;
    }
    const char* rec = (const char*)s_metric_records[index];
    if (rec == NULL) {
        return;
    }
    __builtin_memcpy(out_x, rec + 0x1c, sizeof(*out_x));
    __builtin_memcpy(out_y, rec + 0x20, sizeof(*out_y));
}

void indexed_record_pointer_set(int index, void* record)
{
    if (index >= 0 && index < DAT_WINDOW_COUNT) {
        s_metric_records[index] = record;
    }
}

void* indexed_record_pointer_peek(int index)
{
    if (index < 0 || index >= DAT_WINDOW_COUNT) {
        return NULL;
    }
    return s_metric_records[index];
}

// FUNCTION: LEGORR 0x0045d370
void region_bounds_set(const uint32_t* xy_pair, uint32_t w, uint32_t h)
{
    if (xy_pair == NULL) {
        return; /* deviation: the corpus dereferences blindly */
    }
    s_region_xy[0] = xy_pair[0];
    s_region_xy[1] = xy_pair[1];
    s_region_wh[0] = (float)w; /* the corpus's uint->float casts */
    s_region_wh[1] = (float)h;
}

void region_bounds_peek(uint32_t* out_xy, float* out_wh)
{
    if (out_xy != NULL) {
        out_xy[0] = s_region_xy[0];
        out_xy[1] = s_region_xy[1];
    }
    if (out_wh != NULL) {
        out_wh[0] = s_region_wh[0];
        out_wh[1] = s_region_wh[1];
    }
}

void panel_draw_records_set(int index, void* records)
{
    if (index >= 0 && index < DAT_WINDOW_COUNT) {
        s_panel_records[index] = records;
    }
}

void* panel_draw_records_peek(int index)
{
    if (index < 0 || index >= DAT_WINDOW_COUNT) {
        return NULL;
    }
    return s_panel_records[index];
}

static uint32_t* panel_item_word(int panel, int item)
{
    if (panel < 0 || panel >= DAT_WINDOW_COUNT) {
        return NULL;
    }
    char* block = (char*)s_panel_records[panel];
    if (block == NULL || item < 0) {
        return NULL; /* deviation: the corpus strides blindly */
    }
    return (uint32_t*)(block + 0x28 + (size_t)item * 0x2c);
}

// FUNCTION: LEGORR 0x0045aac0
void panel_item_flag16_set(int panel, int item, int value)
{
    uint32_t* w = panel_item_word(panel, item);
    if (w == NULL) {
        return;
    }
    *w = value != 0 ? *w | 0x10u : *w & ~0x10u;
}

// FUNCTION: LEGORR 0x0045abb0
void panel_item_flag32_set(int panel, int item, int value)
{
    uint32_t* w = panel_item_word(panel, item);
    if (w == NULL) {
        return;
    }
    *w = value != 0 ? *w | 0x20u : *w & ~0x20u;
}

// FUNCTION: LEGORR 0x0045ab00
void panel_state_flags_toggle(int index)
{
    if ((unsigned)index >= DAT_WINDOW_COUNT) {
        return;
    }
    const uint32_t f = s_request_records[index].flags;
    if ((f & 2u) != 0u) {
        s_request_records[index].flags = f & ~2u | 0xcu;
    } else {
        s_request_records[index].flags = f & ~4u | 0xau;
    }
}

static float    s_reward_score;  /* DAT_00555738 */
static float    s_reward_count;  /* DAT_0055573c */

// FUNCTION: LEGORR 0x00462800
void reward_score_add(float value)
{
    s_reward_count = s_reward_count - -1.0f; /* corpus shape */
    s_reward_score = s_reward_score + value;
}

float reward_score_total_peek(void) { return s_reward_score; }
uint32_t reward_score_count_peek(void)
{
    return (uint32_t)s_reward_count;
}

/* --- the panel/view blit band (see crt.hpp) --------------------------------- */
static uint32_t s_view_handle[DAT_WINDOW_COUNT]; /* DAT_00552140 */
static void*    s_view_rect_a[DAT_WINDOW_COUNT]; /* DAT_0055214c */
static void*    s_view_rect_b[DAT_WINDOW_COUNT]; /* DAT_00552150 */
static int      s_view_region[DAT_WINDOW_COUNT]; /* DAT_00552158 */
static int      s_panel_count[DAT_WINDOW_COUNT]; /* DAT_00500138 */

#define CRT_SURFACE_CALLS 256
static uint32_t s_call_handle[CRT_SURFACE_CALLS];
static float    s_call_x[CRT_SURFACE_CALLS];
static float    s_call_y[CRT_SURFACE_CALLS];
static int      s_call_n;
/* Optional rect witnesses: rect1 is the corpus's source rect (4 floats)
 * and rect3 the size pair (2 floats); the plain record leaves both
 * absent. */
static float    s_call_rect1[CRT_SURFACE_CALLS][4];
static float    s_call_rect3[CRT_SURFACE_CALLS][2];
static uint8_t  s_call_rect1_set[CRT_SURFACE_CALLS];
static uint8_t  s_call_rect3_set[CRT_SURFACE_CALLS];

static void surface_call_store(uint32_t handle, float x, float y,
                               const float* rect1, const float* rect3)
{
    if (s_call_n >= CRT_SURFACE_CALLS) {
        return;
    }
    s_call_handle[s_call_n] = handle;
    s_call_x[s_call_n] = x;
    s_call_y[s_call_n] = y;
    s_call_rect1_set[s_call_n] = 0u;
    s_call_rect3_set[s_call_n] = 0u;
    if (rect1 != NULL) {
        __builtin_memcpy(s_call_rect1[s_call_n], rect1,
                         sizeof s_call_rect1[s_call_n]);
        s_call_rect1_set[s_call_n] = 1u;
    }
    if (rect3 != NULL) {
        __builtin_memcpy(s_call_rect3[s_call_n], rect3,
                         sizeof s_call_rect3[s_call_n]);
        s_call_rect3_set[s_call_n] = 1u;
    }
    ++s_call_n;
}

uint32_t crt_surface_handle_next(void)
{
    static uint32_t s_handle_counter = 0u;
    ++s_handle_counter;
    return s_handle_counter;
}

void crt_surface_blit_record(uint32_t handle, float x, float y)
{
    surface_call_store(handle, x, y, NULL, NULL);
}

void crt_surface_blit_record_rect(uint32_t handle, float x, float y,
                                  const float* rect1, const float* rect3)
{
    surface_call_store(handle, x, y, rect1, rect3);
}

int crt_surface_calls_count(void) { return s_call_n; }

int crt_surface_call_get(int index, uint32_t* handle, float* x, float* y)
{
    if (index < 0 || index >= s_call_n) {
        return 0;
    }
    if (handle != NULL) {
        *handle = s_call_handle[index];
    }
    if (x != NULL) {
        *x = s_call_x[index];
    }
    if (y != NULL) {
        *y = s_call_y[index];
    }
    return 1;
}

int crt_surface_call_get_rect(int index, float* rect1, int* rect1_set,
                              float* rect3, int* rect3_set)
{
    if (index < 0 || index >= s_call_n) {
        return 0;
    }
    if (rect1_set != NULL) {
        *rect1_set = s_call_rect1_set[index] != 0u;
    }
    if (rect3_set != NULL) {
        *rect3_set = s_call_rect3_set[index] != 0u;
    }
    if (s_call_rect1_set[index] != 0u && rect1 != NULL) {
        __builtin_memcpy(rect1, s_call_rect1[index],
                         4 * sizeof(float));
    }
    if (s_call_rect3_set[index] != 0u && rect3 != NULL) {
        __builtin_memcpy(rect3, s_call_rect3[index], 2 * sizeof(float));
    }
    return 1;
}

void crt_surface_calls_clear(void) { s_call_n = 0; }

float crt_float_bits(uint32_t bits)
{
    float f = 0.0f;
    __builtin_memcpy(&f, &bits, sizeof(f));
    return f;
}

static uint32_t float_bits(float f)
{
    uint32_t b = 0u;
    __builtin_memcpy(&b, &f, sizeof(b));
    return b;
}

void view_blit_window_set(int view, uint32_t handle, void* rect_a,
                          void* rect_b, int region_index)
{
    if (view < 0 || view >= DAT_WINDOW_COUNT) {
        return;
    }
    s_view_handle[view] = handle;
    s_view_rect_a[view] = rect_a;
    s_view_rect_b[view] = rect_b;
    s_view_region[view] = region_index;
}

void view_blit_window_peek(int view, uint32_t* handle, void** rect_a,
                           void** rect_b, int* region_index)
{
    if (view < 0 || view >= DAT_WINDOW_COUNT) {
        return;
    }
    if (handle != NULL) {
        *handle = s_view_handle[view];
    }
    if (rect_a != NULL) {
        *rect_a = s_view_rect_a[view];
    }
    if (rect_b != NULL) {
        *rect_b = s_view_rect_b[view];
    }
    if (region_index != NULL) {
        *region_index = s_view_region[view];
    }
}

void panel_draw_count_set(int panel, int count)
{
    if (panel >= 0 && panel < DAT_WINDOW_COUNT) {
        s_panel_count[panel] = count;
    }
}

int panel_draw_count_peek(int panel)
{
    if (panel < 0 || panel >= DAT_WINDOW_COUNT) {
        return 0;
    }
    return s_panel_count[panel];
}

/* PanelItemFlag8SetIfEnabled (FUN_0045abf0) — see crt.hpp. Bit 8 of the
 * stride-0x2c item's word at +0x28 only follows the flag while bit 32
 * (the enable gate) is set; otherwise the write is skipped. */
// FUNCTION: LEGORR 0x0045abf0
void panel_item_flag8_set_if_enabled(int panel, int item, int value)
{
    uint32_t* w = panel_item_word(panel, item);
    if (w == NULL) {
        return;
    }
    if ((*w & 0x20u) == 0u) {
        return;
    }
    *w = value != 0 ? *w | 8u : *w & ~8u;
}

/* ViewBlitRectPositionsApply (FUN_00463d20) — see crt.hpp. Rect records:
 * +0x00/+0x04 position, +0x10/+0x14 the size pair (bit-for-bit blit
 * handles), +0x18 the flag word. The corpus strides its own block and
 * dereferences every window blindly — the guards here are the documented
 * deviation. */
// FUNCTION: LEGORR 0x00463d20
void view_blit_rect_positions_apply(int view)
{
    if (view < 0 || view >= DAT_WINDOW_COUNT) {
        return;
    }
    const int k = s_view_region[view];
    if (k < 0 || k >= DAT_WINDOW_COUNT) {
        return; /* deviation: the corpus strides its own block blindly */
    }
    const float ox = s_fpair_a[view] +
                     (float)(int)s_request_records[k].pair_x;
    const float oy = s_fpair_b[view] +
                     (float)(int)s_request_records[k].pair_y;
    crt_surface_blit_record(s_view_handle[view], ox, oy);

    for (int which = 0; which <= 1; ++which) {
        const float* pf = (const float*)(which == 0 ? s_view_rect_a[view]
                                                   : s_view_rect_b[view]);
        if (pf == NULL) {
            continue; /* deviation: the corpus reads blindly */
        }
        uint32_t flags = 0u;
        __builtin_memcpy(&flags, pf + 6, sizeof(flags));
        uint32_t handle = 0u;
        if (((flags & 2u) == 0u) || (pf[4] == 0.0f)) {
            if ((flags & 4u) != 0u && pf[5] != 0.0f) {
                handle = float_bits(pf[5]);
            } else {
                continue;
            }
        } else {
            handle = float_bits(pf[4]);
        }
        crt_surface_blit_record(handle, pf[0] + ox, pf[1] + oy);
    }

    const float* pf = (const float*)s_metric_records[view];
    if (pf == NULL) {
        return; /* deviation: the corpus dereferences its slot blindly */
    }
    crt_surface_blit_record(float_bits(pf[6]),
                            pf[0] + pf[4] + ox, pf[1] + pf[5] + oy);
}

/* PanelTextureSlotsAllocate (FUN_0045ac30) — see crt.hpp. Draw-record
 * fields: +0x00/+0x04 position, +0x08/+0x0c the size sums, +0x10/+0x14/
 * +0x18 the texture-slot handles, +0x20 the id word, +0x28 the flag word.
 * The corpus stores the new block over the panel's previous one without
 * freeing it (faithful); a null allocator answer skips the fill loop
 * (deviation: the corpus strides on through the failure). */
// FUNCTION: LEGORR 0x0045ac30
void panel_texture_slots_allocate(int panel, int count, const int* slot_index,
                                  const float* stream, const int* tex1,
                                  const int* tex2, const int* tex3,
                                  const int* ids)
{
    if (panel < 0 || panel >= DAT_WINDOW_COUNT || count <= 0) {
        return;
    }
    float* block = (float*)engine_heap_alloc((long)count * 0x2c);
    panel_draw_records_set(panel, block);
    panel_draw_count_set(panel, count);
    if (block == NULL) {
        return;
    }
    for (int i = 0; i < count; ++i) {
        const int slot = (slot_index != NULL) ? slot_index[i] : i;
        if (slot < 0 || slot >= count) {
            continue; /* deviation: the corpus strides blindly */
        }
        float* rec = block + (size_t)slot * 11;
        const float* s = stream + (size_t)i * 4;
        rec[0] = s[0];
        rec[1] = s[1];
        rec[2] = s[2] + s[0];
        rec[3] = s[3] + s[1];
        rec[4] = 0.0f;
        rec[5] = 0.0f;
        rec[6] = 0.0f;
        rec[10] = 0.0f;
        if (ids != NULL) {
            rec[8] = crt_float_bits((uint32_t)ids[i]);
        }
        const int* tex[3] = { tex1, tex2, tex3 };
        const int field[3] = { 6, 4, 5 }; /* corpus slot order: +0x18,
                                            +0x10, +0x14 */
        for (int t = 0; t < 3; ++t) {
            if (tex[t] != NULL && tex[t][i] != 0) {
                const uint32_t h = crt_surface_handle_next();
                rec[field[t]] = crt_float_bits(h); /* load seam: a handle */
                crt_surface_blit_record(h, 0.0f, 0.0f); /* transform call */
            }
        }
    }
}

/* --- the small global primitives band (see crt.hpp) -------------------------- */

// FUNCTION: LEGORR 0x00497aa0
void uint96_zero(uint32_t limbs[3])
{
    limbs[0] = 0u;
    limbs[1] = 0u;
    limbs[2] = 0u;
}

/* GlobalValueClearIfEqual (FUN_0040e910) — the screen-effect handle word
 * (the corpus's DAT_004c7ea4, unowned) clears only on a matching release. */
// GLOBAL: LEGORR 0x004c7ea4
static int s_global_value_word;

// FUNCTION: LEGORR 0x0040e910
void global_value_clear_if_equal(int value)
{
    if (s_global_value_word == value) {
        s_global_value_word = 0;
    }
}

void global_value_word_set(int value) { s_global_value_word = value; }
int  global_value_word_peek(void) { return s_global_value_word; }

/* The two global-list pushes (FUN_0046a470/FUN_004792c0) — the corpus's
 * head words have no ported owner, so the heads land here; the link field
 * at record +0x28 is a port-side pointer-sized slot (the corpus's 32-bit
 * dword, widened with the host — deviation documented). */
/* Datacmp note: the corpus words at 0x4aa450/0x4aa458 are file-backed
 * zeros while the port's zero-init statics land in the exe's zero-fill
 * tail; reccmp-datacmp reads only file bytes per side and flags the pair
 * (uninitialized) — both sides are genuinely zero (image-verified). */
// GLOBAL: LEGORR 0x004aa458
static void* s_linked_list_head = NULL;  /* DAT_004aa458 role */
static void* s_record_list_head;  /* DAT_00506518 role */

static void record_list_push(void** head, void* record)
{
    if (record == NULL) {
        return; /* deviation: the corpus strides the record blindly */
    }
    *(void**)((char*)record + 0x1c) = *head;
    *head = record;
}

// FUNCTION: LEGORR 0x0046a470
void linked_list_push_at_global_head(void* record)
{
    record_list_push(&s_linked_list_head, record);
}

void* linked_list_global_head_peek(void) { return s_linked_list_head; }

void linked_list_global_head_set(void* record)
{
    s_linked_list_head = record;
}

// FUNCTION: LEGORR 0x004792c0
void global_record_list_push_head(void* record)
{
    record_list_push(&s_record_list_head, record);
}

void* global_record_list_head_peek(void) { return s_record_list_head; }

/* SceneEffectRegistryReset (FUN_00465380) — the transient-effect registry
 * words (head/tail/shared-texture, unowned) clear and the reset answers 1
 * exactly as the corpus does. The corpus addresses the trio individually at
 * 0x00552128/0x0055212c/0x00552130, so each word is its own annotated
 * static (the s_indexed_window precedent); all three are zero-filled
 * virtual tail, matching the zero-initialized statics here. */
// GLOBAL: LEGORR 0x00552128
static uint32_t s_scene_effect_head;     /* DAT_00552128 role: transient-effect head */
// GLOBAL: LEGORR 0x0055212c
static uint32_t s_scene_effect_tail;     /* DAT_0055212c role: transient-effect tail */
// GLOBAL: LEGORR 0x00552130
static uint32_t s_scene_effect_shared;   /* DAT_00552130 role: shared texture */

// FUNCTION: LEGORR 0x00465380
int scene_effect_registry_reset(void)
{
    s_scene_effect_head = 0u;    /* corpus clears in address order */
    s_scene_effect_tail = 0u;
    s_scene_effect_shared = 0u;
    return 1;
}

void scene_effect_registry_words_peek(uint32_t out_words[3])
{
    if (out_words == NULL) {
        return;
    }
    out_words[0] = s_scene_effect_head;
    out_words[1] = s_scene_effect_tail;
    out_words[2] = s_scene_effect_shared;
}

/* CrtFatalMessageExit (FUN_00498340) — the corpus's exhausted-memory
 * abort. The port aborts with the host runtime, or routes through the
 * observer hook when installed so the path is testable (deviation). */
static void (*s_fatal_exit_observer)(void);

// FUNCTION: LEGORR 0x00498340
void crt_fatal_message_exit(void)
{
    if (s_fatal_exit_observer != NULL) {
        s_fatal_exit_observer();
        return;
    }
    abort();
}

void crt_fatal_exit_observer_set(void (*observer)(void))
{
    s_fatal_exit_observer = observer;
}

/* CrtNoOpStubVariant2 (FUN_00493c40) — see crt.hpp. */
// FUNCTION: LEGORR 0x00493c40
void crt_no_op_stub_variant2(void)
{
    return;
}

/* LinkedRecordChainFree / GlobalLinkedRecordChainClear — see crt.hpp. */
// GLOBAL: LEGORR 0x004aa450
static void* s_linked_list_aux = NULL; /* DAT_004aa450 role: unowned, zero-init */

// FUNCTION: LEGORR 0x0046a370
void linked_record_chain_free(void* record)
{
    if (record == NULL) {
        return; /* the corpus's gate */
    }
    void* next = NULL;
    __builtin_memcpy(&next, (char*)record + 0x1c, sizeof(next));
    linked_record_chain_free(next);
    engine_heap_free(record);
}

// FUNCTION: LEGORR 0x0046a390
void global_linked_record_chain_clear(void)
{
    void* head = s_linked_list_head; /* the corpus reads the head first */
    s_linked_list_aux = NULL;
    linked_record_chain_free(head);
    s_linked_list_head = NULL;
}

void* linked_list_global_aux_peek(void) { return s_linked_list_aux; }
void  linked_list_global_aux_set(void* word) { s_linked_list_aux = word; }

/* ReleaseLinkedObjectList (FUN_00479240) — see crt.hpp. */
// FUNCTION: LEGORR 0x00479240
void linked_object_list_release(void* list)
{
    if (list == NULL) {
        return; /* deviation: the corpus strides the head blindly */
    }
    void* owner = *(void* const*)list;
    engine_heap_free(owner);
    for (void* node = list; node != NULL;) {
        void* next = *(void* const*)((char*)node + 0x14);
        global_record_list_push_head(node);
        node = next;
    }
}

/* ModelRenderListClear (FUN_00482d30) — see crt.hpp. */
// GLOBAL: LEGORR 0x005343c4
static void* s_model_render_list_head; /* DAT_005343c4 role: unowned */

// FUNCTION: LEGORR 0x00482d30
void model_render_list_clear(void)
{
    void* node = s_model_render_list_head;
    while (node != NULL) {
        void* next = NULL;
        __builtin_memcpy(&next, (char*)node + 0x44, sizeof next);
        engine_heap_free(node);
        node = next;
    }
    s_model_render_list_head = NULL;
}

void model_render_list_head_set(void* node) { s_model_render_list_head = node; }
void* model_render_list_head_peek(void) { return s_model_render_list_head; }

/* RendererCommandNodePrepend (FUN_00482ce0) — see crt.hpp. */
// FUNCTION: LEGORR 0x00482ce0
void renderer_command_node_prepend(uint32_t kind, const uint32_t* payload)
{
    /* Deviation: the port lays the corpus's dword node window out with
     * the widened host-word window (the CrtOwnedStreamBufferRelease
     * convention) while the next link rides as a widened void* at the
     * corpus's byte slot +0x44 (the ModelRenderListClear precedent). */
    char* node = (char*)engine_heap_alloc(18 * (long)sizeof(void*));
    if (node == NULL) {
        return; /* deviation: the corpus strides its alloc blindly */
    }
    memset(node, 0, 18 * (size_t)sizeof(void*));
    const uint32_t zero = 0u;
    __builtin_memcpy(node + 0x00, &kind, sizeof kind);
    for (int i = 0; i < 16; ++i) {
        const uint32_t v = (payload != NULL) ? payload[i] : zero;
        __builtin_memcpy(node + 0x04 + (size_t)i * sizeof v, &v, sizeof v);
    }
    void* head = model_render_list_head_peek();
    __builtin_memcpy(node + 0x44, &head, sizeof head);
    model_render_list_head_set(node);
}

/* CrtOwnedStreamBufferRelease (FUN_00494df0) — see crt.hpp. */
// FUNCTION: LEGORR 0x00494df0
void crt_owned_stream_buffer_release(char* record)
{
    /* Fields 0-2 are host pointers; field 3 is the following flags dword. */
    void** fields = (void**)record;
    uint32_t* flags = (uint32_t*)(fields + 3);
    if ((*flags & 0x83u) != 0u && (*flags & 0x8u) != 0u) {
        engine_heap_free(fields[2]);
        *flags &= 0xfffffbf7u; /* corpus mask: bits 3 and 11 */
        fields[0] = NULL;
        fields[2] = NULL;
        fields[1] = NULL;
    }
}

/* --- the shared texture directory word (see crt.hpp) ------------------------ */
// GLOBAL: LEGORR 0x0076cd88
static char* s_shared_texture_dir; /* DAT_0076cd88 role: unowned */

// FUNCTION: LEGORR 0x004728c0
void shared_texture_directory_set(const char* path)
{
    s_shared_texture_dir = string_duplicate(path);
}

char* shared_texture_directory_peek(void) { return s_shared_texture_dir; }
void shared_texture_directory_clear(void)
{
    engine_heap_free(s_shared_texture_dir);
    s_shared_texture_dir = NULL;
}

/* --- the renderer flag word + scene hook pair band (see crt.hpp) ----------- */
// GLOBAL: LEGORR 0x0076cd94
static uint32_t s_renderer_flag_word; /* DAT_0076cd94 role: unowned */
// GLOBAL: LEGORR 0x0076cd80
static void*    s_scene_end_hook;     /* DAT_0076cd80 role: unowned */
// GLOBAL: LEGORR 0x0076cd84
static void*    s_scene_end_value;    /* DAT_0076cd84 role: unowned */
// GLOBAL: LEGORR 0x0076cd78
static void*    s_scene_sample_hook;  /* DAT_0076cd78 role: unowned */
// GLOBAL: LEGORR 0x0076cd7c
static void*    s_scene_sample_value; /* DAT_0076cd7c role: unowned */

// FUNCTION: LEGORR 0x004728e0
void renderer_flag_bit6_set(int value)
{
    s_renderer_flag_word = (value != 0) ? (s_renderer_flag_word | 0x40u)
                                        : (s_renderer_flag_word & ~0x40u);
}

uint32_t renderer_flag_word_peek(void) { return s_renderer_flag_word; }

// FUNCTION: LEGORR 0x00472900
void scene_end_hook_register(void* hook, void* value)
{
    s_scene_end_hook = hook;
    s_scene_end_value = value;
}

void* scene_end_hook_peek(void) { return s_scene_end_hook; }
void* scene_end_hook_value_peek(void) { return s_scene_end_value; }

// FUNCTION: LEGORR 0x00472920
void scene_sample_hook_register(void* hook, void* value)
{
    s_scene_sample_hook = hook;
    s_scene_sample_value = value;
    s_renderer_flag_word |= 0x40u;
}

void* scene_sample_hook_peek(void) { return s_scene_sample_hook; }
void* scene_sample_hook_value_peek(void) { return s_scene_sample_value; }

/* --- the string registry (see crt.hpp) ------------------------------------- */
// GLOBAL: LEGORR 0x004db8cc
static void* s_string_registry_head; /* DAT_004db8cc role: unowned */

// FUNCTION: LEGORR 0x00410450
#if defined(_MSC_VER) && defined(_M_IX86)
/* Corpus 0x410450..0x410485 keeps the empty-head exit and list walk in this
 * order, including the annotated strcasecmp_lite call. */
__declspec(naked) void* string_keyed_cache_entry_find(const char* key)
{
    __asm {
        push esi
        mov esi, dword ptr [s_string_registry_head]
        test esi, esi
        push edi
        je string_cache_empty
        mov edi, dword ptr [esp + 0ch]
    string_cache_next:
        mov eax, dword ptr [esi]
        push eax
        push edi
        call strcasecmp_lite
        add esp, 8
        test eax, eax
        je string_cache_match
        mov esi, dword ptr [esi + 0ch]
        test esi, esi
        jne string_cache_next
        xor eax, eax
        pop edi
        pop esi
        ret
    string_cache_match:
        mov eax, esi
        pop edi
        pop esi
        ret
    string_cache_empty:
        pop edi
        xor eax, eax
        pop esi
        ret
    }
}
#else
void* string_keyed_cache_entry_find(const char* key)
{
    /* Corpus walk: the head gates once, the name rides strcasecmp_lite
     * untested (intern only ever links an entry whose duplicated name
     * landed), and the +0x0c link advances through a bottom-tested
     * test/jne. */
    void* entry = s_string_registry_head;
    if (entry == NULL) {
        return NULL;
    }
    for (;;) {
        if (strcasecmp_lite(*(char**)entry, key) == 0) {
            return entry;
        }
        entry = *(void**)((char*)entry + 0xc); /* corpus mov esi,[esi+0xc] */
        if (entry == NULL) {
            return NULL;
        }
    }
}
#endif

// FUNCTION: LEGORR 0x00410490
void* string_registry_intern(const char* key)
{
    if (key == NULL || *key == '\0') {
        return NULL; /* corpus's null-pointer and empty-string exits */
    }
    void* entry = string_keyed_cache_entry_find(key);
    if (entry != NULL) {
        return entry;
    }
    /* Corpus strdup idiom: both allocations are unchecked (the corpus
     * stores blind — a NULL allocator answer writes through it), the
     * length rides the inlined strlen intrinsic into the alloc push, and
     * the copy pointer lands in field 0 before the inlined strcpy runs. */
    void* record = engine_heap_alloc(0x10);
    char** slots = (char**)record;
    char* copy = (char*)engine_heap_alloc((long)strlen(key) + 1);
    slots[0] = copy; /* the corpus stores the copy pointer before the copy */
    strcpy(copy, key);
    void* head = s_string_registry_head; /* the head is loaded once, here */
    slots[1] = NULL;
    slots[3] = (char*)head;
    slots[2] = NULL;
    s_string_registry_head = record;
    return record;
}

void string_registry_reset(void)
{
    const long field = (long)sizeof(void*);
    void* entry = s_string_registry_head;
    while (entry != NULL) {
        void* next = NULL;
        __builtin_memcpy(&next, (char*)entry + 3 * field, sizeof next);
        char* key = NULL;
        __builtin_memcpy(&key, (char*)entry + 0 * field, sizeof key);
        engine_heap_free(key);
        engine_heap_free(entry);
        entry = next;
    }
    s_string_registry_head = NULL;
}

static void* (*s_bitmap_surface_loader)(const char*);
static void* (*s_font_resource_loader)(const char*);

void string_cache_bitmap_loader_set(void* (*hook)(const char*))
{
    s_bitmap_surface_loader = hook;
}

void string_cache_font_loader_set(void* (*hook)(const char*))
{
    s_font_resource_loader = hook;
}

/* Shared cache-slot fold: load once on a zero slot, return the word. */
static void* s_entry_cache_slot_get(const char* key, long slot,
                                    void* (*loader)(const char*))
{
    if (key == NULL || *key == '\0') {
        return NULL;
    }
    void* entry = string_registry_intern(key);
    if (entry == NULL) {
        return NULL; /* deviation: the corpus strides the intern failure */
    }
    void* value = NULL;
    __builtin_memcpy(&value, (char*)entry + slot * (long)sizeof(void*),
                     sizeof value);
    if (value == NULL) {
        value = (loader != NULL) ? loader(key)
                                 : NULL; /* seam: answers NULL uninstalled */
        if (value != NULL) {
            __builtin_memcpy((char*)entry + slot * (long)sizeof(void*),
                             &value, sizeof value);
        }
    }
    return value;
}

// FUNCTION: LEGORR 0x00410520
void* string_cache_lookup_or_load(const char* key)
{
    return s_entry_cache_slot_get(key, 1, s_bitmap_surface_loader);
}

// FUNCTION: LEGORR 0x00410560
void* resource_name_cache_get_or_load(const char* key)
{
    return s_entry_cache_slot_get(key, 2, s_font_resource_loader);
}

/* --- the selection marker table (see crt.hpp) ------------------------------
 * DAT_004c7e00..e7c is 32 pointer slots plus the count word at e80; the
 * descriptor sits at DAT_004c7df8. All unowned: zero-init statics here. */
// GLOBAL: LEGORR 0x004c7e00
static void*    s_marker_table[32];
// GLOBAL: LEGORR 0x004c7e80
static uint32_t s_marker_count;
// GLOBAL: LEGORR 0x004c7df8
static void*    s_marker_descriptor;

// FUNCTION: LEGORR 0x0040ce30
void selection_marker_resources_release(void)
{
    /* Corpus walk: an unsigned add-4 pointer stride bounded by the table
     * end, entered unconditionally and bottom-tested (body, add, cmp, jb —
     * no pre-loop check). Deviation: each freed slot is cleared in place
     * (the corpus dangles, which a second release could double-free), so
     * the window stays idempotent; pinned by test_crt.cpp's "the freed
     * slots clear out" / double-release checks. */
    void** p;
    void*  desc;

    p = s_marker_table;
    do {
        void* slot = *p;
        if (slot != NULL) {
            engine_heap_free(slot);
            *p = NULL;
        }
        ++p;
    } while (p < s_marker_table + 32);

    desc = s_marker_descriptor; /* the corpus loads it once... */
    s_marker_count = 0u;        /* ...before the count resets */
    if (desc != NULL) {
        engine_heap_free(desc); /* the loaded word rides the push */
        s_marker_descriptor = NULL;
    }
}

void selection_marker_slot_set(int index, void* ptr)
{
    if (index >= 0 && index < 32) {
        s_marker_table[index] = ptr;
    }
}
void* selection_marker_slot_peek(int index)
{
    return (index >= 0 && index < 32) ? s_marker_table[index] : NULL;
}
void  selection_marker_descriptor_set(void* desc) { s_marker_descriptor = desc; }
void* selection_marker_descriptor_peek(void) { return s_marker_descriptor; }
void     selection_marker_count_set(uint32_t count) { s_marker_count = count; }
uint32_t selection_marker_count_peek(void) { return s_marker_count; }

/* OwnedResourceTreeRelease (FUN_0048c180) — see crt.hpp. */
// FUNCTION: LEGORR 0x0048c180
void owned_resource_tree_release(void** node, int own_node)
{
    if (node == NULL) {
        return; /* deviation: the corpus dereferences blindly */
    }
    if (node[0] != NULL) {
        engine_heap_free(node[0]);
    }
    if (node[2] != NULL) {
        engine_heap_free(node[2]);
    }
    if (node[1] != NULL) {
        owned_resource_tree_release((void**)node[1], 1);
    }
    if (own_node != 0) {
        engine_heap_free((void*)node);
    }
}

/* --- the string + stream allocation leaves (see crt.hpp) ------------------- */

/* StringPrefixReserve (FUN_004907c0): the scan is a strlen; the shift is an
 * overlapping move of the whole string (NUL included) `count` bytes right. */
// FUNCTION: LEGORR 0x004907c0
void string_prefix_reserve(char* s, int count)
{
    if (count != 0) {
        const size_t bytes = strlen(s) + 1u;
        crt_memory_move(s + count, s, bytes);
    }
}

/* CrtStreamBufferAllocate (FUN_00498ae0) — see crt.hpp. DAT_005493e4 is
 * unowned: zero-init hooked static. */
// GLOBAL: LEGORR 0x005493e4
static long s_stream_alloc_count;
// GLOBAL: LEGORR 0x005493e8
static void* s_standard_stream_buffers[2];

long stream_allocation_count_peek(void) { return s_stream_alloc_count; }
void stream_allocation_count_reset(void) { s_stream_alloc_count = 0; }

// FUNCTION: LEGORR 0x00498ae0
void crt_stream_buffer_allocate(char* record)
{
#if defined(_WIN64) || defined(__x86_64__) || defined(__aarch64__)
    const long field = (long)sizeof(void*);
    void* buffer = engine_heap_alloc(0x1000);
    void* zero = NULL;
    uint32_t flags;
    ++s_stream_alloc_count;
    __builtin_memcpy(record + 2 * field, &buffer, sizeof buffer);
    __builtin_memcpy(&flags, record + 3 * field, sizeof flags);
    if (buffer != NULL) {
        flags |= 8u;
        {
            const uint32_t cap = 0x1000u;
            __builtin_memcpy(record + 6 * field, &cap, sizeof cap);
        }
    } else {
        flags |= 4u;
        {
            const uint32_t cap = 2u;
            __builtin_memcpy(record + 6 * field, &cap, sizeof cap);
        }
        buffer = record + 5 * field;
        __builtin_memcpy(record + 2 * field, &buffer, sizeof buffer);
    }
    __builtin_memcpy(record + 3 * field, &flags, sizeof flags);
    __builtin_memcpy(record + 1 * field, &zero, sizeof zero);
    __builtin_memcpy(record + 0 * field, &buffer, sizeof buffer);
#else
    void* buffer;
    uint32_t* fields = (uint32_t*)record;
    ++s_stream_alloc_count;
    buffer = engine_heap_alloc(0x1000);
    fields[2] = (uint32_t)buffer;
    if (buffer != NULL) {
        fields[3] |= 8u;
        fields[6] = 0x1000;
        fields[0] = fields[2];
        fields[1] = 0;
    } else {
        fields[6] = 2;
        fields[3] |= 4u;
        fields[2] = (uint32_t)(record + 20);
        fields[0] = fields[2];
        fields[1] = 0;
    }
#endif
}

// FUNCTION: LEGORR 0x00498f50
uint32_t crt_stream_unget_byte(uint32_t value, char* record)
{
    int* fields = (int*)record;
    uint32_t flags;
    if (value != 0xffffffffu &&
        (((uint32_t)fields[3] & 1u) != 0u ||
         (((uint32_t)fields[3] & 0x80u) != 0u &&
          ((uint32_t)fields[3] & 2u) == 0u))) {
        if (fields[2] == 0) {
            crt_stream_buffer_allocate(record);
        }
        if (fields[0] == fields[2]) {
            if (fields[1] != 0) {
                return 0xffffffffu;
            }
            fields[0] = fields[0] + 1;
        }
        if (((uint32_t)fields[3] & 0x40u) != 0u) {
            fields[0]--;
            if (*(char*)fields[0] != (char)value) {
                fields[0]++;
                return 0xffffffffu;
            }
        } else {
            fields[0]--;
            *(char*)fields[0] = (char)value;
        }
        fields[1] = fields[1] + 1;
        fields[3] = (fields[3] & 0xffffffefu) | 1;
        return value & 0xffu;
    }
    return 0xffffffffu;
}

/* UInt96Add (FUN_00499fe0) — see crt.hpp. Every carry-add writes its sum
 * whether or not it carries; only the bump is gated. */
// FUNCTION: LEGORR 0x00499fe0
void crt_uint96_add(uint32_t* dst, const uint32_t* src)
{
    int carry = crt_unsigned_add_with_carry(dst[0], src[0], &dst[0]);
    if (carry != 0) {
        carry = crt_unsigned_add_with_carry(dst[1], 1u, &dst[1]);
        if (carry != 0) {
            dst[2] = dst[2] + 1u;
        }
    }
    carry = crt_unsigned_add_with_carry(dst[1], src[1], &dst[1]);
    if (carry != 0) {
        dst[2] = dst[2] + 1u;
    }
    crt_unsigned_add_with_carry(dst[2], src[2], &dst[2]);
}

// FUNCTION: LEGORR 0x0049a0b0
void crt_decimal_digit_sequence_to_extended96(const char* digits, int count,
                                              void* out_extended96)
{
    uint32_t* limbs = (uint32_t*)out_extended96;
    int16_t exponent = 0x404e;
    int i;

    limbs[0] = 0;
    limbs[1] = 0;
    limbs[2] = 0;
    for (i = 0; i < count; ++i) {
        uint32_t original[3];
        uint32_t digit[3];

        original[0] = limbs[0];
        original[1] = limbs[1];
        original[2] = limbs[2];
        extended_float96_bit_shift_left_one(limbs);
        extended_float96_bit_shift_left_one(limbs);
        crt_uint96_add(limbs, original);
        extended_float96_bit_shift_left_one(limbs);

        /* Corpus 0x49a116 sign-extends the current digit byte with MOVSX. */
        digit[0] = (uint32_t)(int32_t)digits[i];
        digit[1] = 0;
        digit[2] = 0;
        crt_uint96_add(limbs, digit);
    }

    while (limbs[2] == 0) {
        exponent = (int16_t)(exponent - 16);
        limbs[2] = limbs[1] >> 16;
        limbs[1] = (limbs[0] >> 16) | (limbs[1] << 16);
        limbs[0] <<= 16;
    }
    while ((limbs[2] & 0x8000u) == 0) {
        extended_float96_bit_shift_left_one(limbs);
        exponent = (int16_t)(exponent - 1);
    }
    *(int16_t*)((uint8_t*)out_extended96 + 10) = exponent;
}

// FUNCTION: LEGORR 0x00497f40
void crt_double_to_extended80(void* out_extended80, const void* source_double)
{
    const uint32_t* source = (const uint32_t*)source_double;
    uint32_t* destination = (uint32_t*)out_extended80;
    uint16_t source_high = *(const uint16_t*)((const uint8_t*)source_double + 6);
    uint32_t exponent = (source_high & 0x7ff0u) >> 4;
    uint32_t sign_bit = 0x80000000u;
    uint32_t significand_high;
    uint32_t significand_low;
    uint32_t extended_exponent;

    significand_low = source[0];
    if (exponent == 0u) {
        sign_bit = 0u;
        if (((source[1] & 0x000fffffu) == 0u) && (significand_low == 0u)) {
            destination[0] = 0u;
            destination[1] = 0u;
            *(uint16_t*)(destination + 2) = 0u;
            return;
        }
        extended_exponent = 0x3c01u;
    }
    else if (exponent == 0x7ffu) {
        extended_exponent = 0x7fffu;
    }
    else {
        extended_exponent = exponent + 0x3c00u;
    }

    significand_high = (significand_low >> 21) |
                       ((source[1] & 0x000fffffu) << 11) | sign_bit;
    destination[1] = significand_high;
    destination[0] = significand_low << 11;
    while ((significand_high & 0x80000000u) == 0u) {
        significand_high = (destination[0] >> 31) |
                           (significand_high << 1);
        extended_exponent = (extended_exponent - 1u) & 0xffffu;
        destination[1] = significand_high;
        destination[0] <<= 1;
    }
    *(uint16_t*)(destination + 2) =
        (uint16_t)(extended_exponent | (source_high & 0x8000u));
}

// FUNCTION: LEGORR 0x0049a540
void crt_extended80_multiply(uint16_t* destination, const uint16_t* source)
{
    uint16_t left[5];
    uint16_t right[5];
    uint16_t accumulator[6];
    uint16_t left_exponent;
    uint16_t right_exponent;
    uint16_t exponent;
    uint16_t sign;
    uint32_t sticky;
    uint32_t i;
    uint32_t j;

    for (i = 0; i < 5u; ++i) {
        left[i] = destination[i];
        right[i] = source[i];
    }
    for (i = 0; i < 6u; ++i) {
        accumulator[i] = 0u;
    }
    sticky = 0u;
    left_exponent = destination[5] & 0x7fffu;
    right_exponent = source[5] & 0x7fffu;
    sign = (uint16_t)((destination[5] ^ source[5]) & 0x8000u);
    exponent = (uint16_t)(left_exponent + right_exponent);

    if (left_exponent > 0x7ffeu || right_exponent > 0x7ffeu ||
        exponent > 0xbffdu) {
        destination[0] = 0u;
        destination[1] = 0u;
        destination[2] = 0u;
        destination[3] = 0u;
        destination[4] = 0x8000u;
        destination[5] = (uint16_t)(0x7fffu | sign);
        return;
    }
    if (exponent < 0x3fc0u) {
        for (i = 0; i < 6u; ++i) {
            destination[i] = 0u;
        }
        return;
    }
    if (left_exponent == 0u && left[0] == 0u && left[1] == 0u &&
        left[2] == 0u && left[3] == 0u && left[4] == 0u) {
        destination[5] = 0u;
        return;
    }
    if (right_exponent == 0u && right[0] == 0u && right[1] == 0u &&
        right[2] == 0u && right[3] == 0u && right[4] == 0u) {
        for (i = 0; i < 6u; ++i) {
            destination[i] = 0u;
        }
        return;
    }

    for (i = 0; i < 5u; ++i) {
        for (j = 0; j < 5u - i; ++j) {
            uint32_t pair;
            uint32_t product;
            uint32_t sum;
            uint32_t carry;

            product = (uint32_t)left[i + j] * (uint32_t)right[4u - j];
            pair = (uint32_t)accumulator[i] |
                   ((uint32_t)accumulator[i + 1u] << 16);
            carry = (uint32_t)crt_unsigned_add_with_carry(pair, product, &sum);
            accumulator[i] = (uint16_t)sum;
            accumulator[i + 1u] = (uint16_t)(sum >> 16);
            if (carry != 0u) {
                if (i == 4u) {
                    ++sticky;
                }
                else {
                    accumulator[i + 2u] =
                        (uint16_t)(accumulator[i + 2u] + 1u);
                }
            }
        }
    }

    exponent = (uint16_t)(exponent + 0xc002u);
    while ((int16_t)exponent > 0 && (accumulator[5] & 0x8000u) == 0u) {
        uint32_t limbs[3];
        limbs[0] = (uint32_t)accumulator[0] |
                   ((uint32_t)accumulator[1] << 16);
        limbs[1] = (uint32_t)accumulator[2] |
                   ((uint32_t)accumulator[3] << 16);
        limbs[2] = (uint32_t)accumulator[4] |
                   ((uint32_t)accumulator[5] << 16);
        extended_float96_bit_shift_left_one(limbs);
        accumulator[0] = (uint16_t)limbs[0];
        accumulator[1] = (uint16_t)(limbs[0] >> 16);
        accumulator[2] = (uint16_t)limbs[1];
        accumulator[3] = (uint16_t)(limbs[1] >> 16);
        accumulator[4] = (uint16_t)limbs[2];
        accumulator[5] = (uint16_t)(limbs[2] >> 16);
        exponent = (uint16_t)(exponent - 1u);
    }
    if ((int16_t)exponent < 1) {
        int shifts;
        exponent = (uint16_t)(exponent - 1u);
        if ((int16_t)exponent < 0) {
            shifts = -(int16_t)exponent;
            exponent = (uint16_t)(exponent + (int16_t)shifts);
            while (shifts != 0) {
                uint32_t limbs[3];
                if ((accumulator[0] & 1u) != 0u) {
                    ++sticky;
                }
                limbs[0] = (uint32_t)accumulator[0] |
                           ((uint32_t)accumulator[1] << 16);
                limbs[1] = (uint32_t)accumulator[2] |
                           ((uint32_t)accumulator[3] << 16);
                limbs[2] = (uint32_t)accumulator[4] |
                           ((uint32_t)accumulator[5] << 16);
                extended_float96_bit_shift_right_one(limbs);
                accumulator[0] = (uint16_t)limbs[0];
                accumulator[1] = (uint16_t)(limbs[0] >> 16);
                accumulator[2] = (uint16_t)limbs[1];
                accumulator[3] = (uint16_t)(limbs[1] >> 16);
                accumulator[4] = (uint16_t)limbs[2];
                accumulator[5] = (uint16_t)(limbs[2] >> 16);
                --shifts;
            }
        }
        if (sticky != 0u) {
            accumulator[0] = (uint16_t)(accumulator[0] | 1u);
        }
    }

    if (accumulator[0] > 0x8000u ||
        (((uint32_t)accumulator[0] |
          ((uint32_t)accumulator[1] << 16)) & 0x1ffffu) == 0x18000u) {
        i = 1u;
        while (i < 6u) {
            accumulator[i] = (uint16_t)(accumulator[i] + 1u);
            if (accumulator[i] != 0u) {
                break;
            }
            ++i;
        }
        if (i == 6u) {
            accumulator[5] = 0x8000u;
            exponent = (uint16_t)(exponent + 1u);
        }
    }
    if (exponent > 0x7ffeu) {
        destination[0] = 0u;
        destination[1] = 0u;
        destination[2] = 0u;
        destination[3] = 0u;
        destination[4] = 0x8000u;
        destination[5] = (uint16_t)(0x7fffu | sign);
        return;
    }
    for (i = 0; i < 5u; ++i) {
        destination[i] = accumulator[i + 1u];
    }
    destination[5] = (uint16_t)(exponent | sign);
}

/* UInt96IncrementAtBit (FUN_00497970) — see crt.hpp. The carry walks DOWN
 * the limb indices (toward limb 0), the opposite direction from the
 * uint96-add chain; the corpus's bit-mask reduction rides the byte-trick
 * expression, which simplifies to bit % 32 for the non-negative inputs. */
static uint32_t uint96_increment_at_bit_with_carry(uint32_t limbs[3], int bit)
{
    if (limbs == NULL) {
        return 0u; /* deviation: the corpus strides the limbs blindly */
    }
    const int limb = (bit + (int)((bit >> 31) & 0x1fu)) >> 5;
    if (limb < 0 || limb > 2) {
        return 0u; /* deviation: the corpus strides the limbs blindly */
    }
    const unsigned char b = (unsigned char)(bit >> 31);
    int idx = ((((unsigned char)bit ^ b) - b) & 0x1f) ^ b;
    idx = (idx - b) & 0x1f;
    const uint32_t mask = 1u << (31u - (uint32_t)idx);
    uint32_t sum = 0;
    int carry = crt_unsigned_add_with_carry(limbs[limb], mask, &sum);
    limbs[limb] = sum;
    for (int i = limb - 1; i >= 0; --i) {
        if (carry == 0) {
            return 0u;
        }
        carry = crt_unsigned_add_with_carry(limbs[i], 1u, &sum);
        limbs[i] = sum;
    }
    return (uint32_t)carry;
}

// FUNCTION: LEGORR 0x00497970
void crt_uint96_increment_at_bit(uint32_t limbs[3], int bit)
{
    if (limbs == NULL) {
        return; /* deviation: the corpus strides the limbs blindly */
    }
    const int limb = (bit + (int)((bit >> 31) & 0x1fu)) >> 5;
    if (limb < 0 || limb > 2) {
        return; /* deviation: the corpus strides the limbs blindly */
    }
    const unsigned char b = (unsigned char)(bit >> 31);
    int idx = ((((unsigned char)bit ^ b) - b) & 0x1f) ^ b;
    idx = (idx - b) & 0x1f;
    const uint32_t mask = 1u << (31u - (uint32_t)idx);
    uint32_t sum = 0;
    int carry = crt_unsigned_add_with_carry(limbs[limb], mask, &sum);
    limbs[limb] = sum;
    for (int i = limb - 1; i >= 0; --i) {
        if (carry == 0) {
            return;
        }
        carry = crt_unsigned_add_with_carry(limbs[i], 1u, &sum);
        limbs[i] = sum;
    }
}

/* UInt96HasNoBitsAtOrAbove (FUN_00497900) — see crt.hpp. The bit-index
 * numbering is the corpus's MSB-first one (bit n lives in limb n / 32 at
 * word position 31 - n % 32), and the in-limb mask
 * ~(-1 << (31 - bit % 32)) covers the positions STRICTLY below the named
 * bit, i.e. the bits above it; equal-index bits don't count. */
// FUNCTION: LEGORR 0x00497900
uint32_t crt_uint96_has_no_bits_at_or_above(const uint32_t limbs[3], int bit)
{
    if (limbs == NULL) {
        return 1u; /* deviation: the corpus strides the limbs blindly */
    }
    const int limb = (bit + (int)((bit >> 31) & 0x1fu)) >> 5;
    if (limb < 0 || limb > 2) {
        return 1u; /* deviation: the corpus strides the limbs blindly */
    }
    const unsigned char b = (unsigned char)(bit >> 31);
    int idx = ((((unsigned char)bit ^ b) - b) & 0x1f) ^ b;
    idx = (idx - b) & 0x1f;
    const uint32_t mask = ~((uint32_t)0xffffffffu << (31u - (uint32_t)idx));
    if ((limbs[limb] & mask) != 0u) {
        return 0u;
    }
    for (int i = limb + 1; i < 3; ++i) {
        if (limbs[i] != 0u) {
            return 0u;
        }
    }
    return 1u;
}

/* UInt96ClearFromBit (FUN_004979e0) — see crt.hpp. The increment branch
 * runs only while "bit is set AND something lives at or above bit + 1";
 * its final carry is returned because the extended-float converter uses it
 * to adjust the exponent after rounding. */
// FUNCTION: LEGORR 0x004979e0
uint32_t crt_uint96_clear_from_bit(uint32_t limbs[3], int bit)
{
    const int limb = (bit + (int)((bit >> 31) & 0x1fu)) >> 5;
    const unsigned char b = (unsigned char)(bit >> 31);
    int idx = ((((unsigned char)bit ^ b) - b) & 0x1f) ^ b;
    idx = (idx - b) & 0x1f;
    const uint32_t shift = 31u - (uint32_t)idx;
    uint32_t result = 0u;
    if ((limbs[limb] & (1u << shift)) != 0u &&
        crt_uint96_has_no_bits_at_or_above(limbs, bit + 1) == 0u) {
        result = uint96_increment_at_bit_with_carry(limbs, bit - 1);
    }
    limbs[limb] &= ((uint32_t)0xffffffffu) << shift;
    for (int i = limb + 1; i < 3; ++i) {
        limbs[i] = 0u;
    }
    return result;
}

/* --- the pair-table window pair (see crt.hpp) ------------------------------
 * The find scan keeps the corpus's masked walk: the compare index is the
 * low 16 bits of the hop counter and the bound test reads that mask. It also
 * calls the shared strcasecmp_lite helper, as the corpus does.
 * Deviations: the port guards the table and the key; the corpus strides
 * them (and its append blindly stores past the caller's window). */
// FUNCTION: LEGORR 0x00488730
void* pair_table_find(const void* table, uint32_t count, const char* key)
{
    if (table == NULL || count == 0u || key == NULL) {
        return NULL; /* deviation: the corpus strides the table blindly */
    }
    const long field = (long)sizeof(void*);
    uint32_t hops = 0;
    for (;;) {
        const uint32_t idx = hops & 0xffffu; /* pinned: corpus walk mask */
        char* name = NULL;
        __builtin_memcpy(&name, (const char*)table + (long)idx * 2 * field,
                         sizeof name);
        void* value = NULL;
        __builtin_memcpy(&value,
                         (const char*)table + ((long)idx * 2 + 1) * field,
                         sizeof value);
        if (name != NULL && strcasecmp_lite(key, name) == 0) {
            return value;
        }
        ++hops;
        if ((hops & 0xffffu) >= count) {
            return NULL;
        }
    }
}

// FUNCTION: LEGORR 0x00488790
void pair_table_append(void* table, int* count, const char* key,
                       void* value)
{
    if (table == NULL || count == NULL) {
        return; /* deviation: the corpus strides both blindly */
    }
    const long field = (long)sizeof(void*);
    char* copy = string_duplicate(key); /* FUN_00477550 */
    char* entry = (char*)table + (long)*count * 2 * field;
    __builtin_memcpy(entry, &copy, sizeof copy);
    __builtin_memcpy(entry + field, &value, sizeof value);
    *count += 1;
}

/* --- the renderer-word band (2026-10-25) ------------------------------------
 * The corpus's DAT_0076ab24/b28/b3c + DAT_0076ac84/ac8c + DAT_0050608c band
 * has no ported writer; each word lands as a zero-init static and the six
 * getters answer their installed half (the unowned-global deviation, per the
 * DAT-window convention). The install hooks are port-side. */
// GLOBAL: LEGORR 0x0076ab24
static uint32_t s_renderer_word_a;      /* DAT_0076ab24 */
// GLOBAL: LEGORR 0x0076ab28
static uint32_t s_renderer_word_b;      /* DAT_0076ab28 */
// GLOBAL: LEGORR 0x0076ab3c
uint32_t g_renderer_word_c;             /* DAT_0076ab3c */
// GLOBAL: LEGORR 0x0076ac84
static uint32_t s_renderer_global_dword; /* DAT_0076ac84 */
// GLOBAL: LEGORR 0x0076ac8c
static uint32_t s_render_surface_secondary; /* DAT_0076ac8c */
// GLOBAL: LEGORR 0x0050608c
static uint32_t s_render_feature_flags; /* DAT_0050608c */

// FUNCTION: LEGORR 0x00410b30
uint32_t renderer_word_a_get(void) { return s_renderer_word_a; } /* FUN_00410b30 */
// FUNCTION: LEGORR 0x00410b40
uint32_t renderer_word_b_get(void) { return s_renderer_word_b; } /* FUN_00410b40 */
// FUNCTION: LEGORR 0x00410b50
uint32_t renderer_word_c_get(void) { return g_renderer_word_c; } /* FUN_00410b50 */
void renderer_words_set(uint32_t a, uint32_t b, uint32_t c)
{
    s_renderer_word_a = a;
    s_renderer_word_b = b;
    g_renderer_word_c = c;
}

// FUNCTION: LEGORR 0x004065d0
uint32_t renderer_global_dword_get(void) { return s_renderer_global_dword; }
void renderer_global_dword_set(uint32_t value) { s_renderer_global_dword = value; }

// FUNCTION: LEGORR 0x004065e0
uint32_t render_surface_secondary_get(void) { return s_render_surface_secondary; }
void render_surface_secondary_set(uint32_t value) { s_render_surface_secondary = value; }

// FUNCTION: LEGORR 0x00477f70
uint32_t render_feature_flags_get(void) { return s_render_feature_flags; }
void render_feature_flags_set(uint32_t value) { s_render_feature_flags = value; }

/* SetLowTextureFlag (2026-10-25): the corpus's DAT_00505f84 engine word has
 * a second half-model (EngineContext::feature in engine.hpp). This band's
 * store carries no context, so the port models it as a module static with
 * install/peek hooks — the two half-models do not interoperate
 * (unowned-global deviation). */
// GLOBAL: LEGORR 0x00505f84
static uint32_t s_engine_feature_word; /* DAT_00505f84 */

uint32_t engine_feature_word_peek(void)
{
    return s_engine_feature_word;
}

void engine_feature_word_install(uint32_t value) /* port hook */
{
    s_engine_feature_word = value;
}

// FUNCTION: LEGORR 0x00477bd0
void low_texture_flag_set(void) /* FUN_00477bd0 */
{
    if ((s_engine_feature_word & 0x40000u) == 0u) {
        s_engine_feature_word |= 0x20u;
    }
}

/* --- integer/string helper band (2026-10-25) -------------------------------- */

/* UInt96Copy (FUN_00497a80) — see crt.hpp. The corpus computes dst − src as
 * a byte distance and walks three dwords forward through the *source* side,
 * reading each word at the walked pointer and writing it back through the
 * delta, so each source word is read only after earlier writes land; the
 * ordered copy reproduces the overlap. Keeping the post-copy decrement in
 * the loop condition mirrors the corpus's `dec edx; jne` tail. */
// FUNCTION: LEGORR 0x00497a80
void uint96_copy(uint32_t dst[3], const uint32_t src[3])
{
    const uint32_t* p = src;
    const ptrdiff_t delta = (char*)dst - (const char*)src;
    int n = 3;
    do {
        *(uint32_t*)((char*)p + delta) = *p;
        p += 1;
    } while (--n != 0);
}

/* UInt96ShiftRight (FUN_00497ad0): shift the three big-endian DWORD limbs.
 * The conversion callers use nonnegative shifts, including bit and whole-word
 * components; preserve the signed quotient/remainder decomposition of the
 * original for callers outside that range as well. */
// FUNCTION: LEGORR 0x00497ad0
void uint96_shift_right(uint32_t limbs[3], int count)
{
    int word_shift;
    unsigned int bit_shift;
    uint32_t carry = 0u;
    int i;

    if (limbs == NULL) {
        return;
    }
    word_shift = (count + (int)((count >> 31) & 0x1fu)) >> 5;
    bit_shift = (unsigned int)count & 0x1fu;
    for (i = 0; i < 3; ++i) {
        const uint32_t current = limbs[i];
        limbs[i] = (current >> bit_shift) | carry;
        carry = (bit_shift == 0u) ? 0u :
            ((current & ((1u << bit_shift) - 1u)) << (32u - bit_shift));
    }
    for (i = 2; i >= 0; --i) {
        const int source = i - word_shift;
        if (source < 0 || source > 2) {
            limbs[i] = 0u;
        }
        else if (source != i) {
            limbs[i] = limbs[source];
        }
    }
}

// GLOBAL: LEGORR 0x004b28a8
static const int32_t s_extended_float_format_a[6] = {
    1024, -1023, 53, 11, 64, 1023
};
// GLOBAL: LEGORR 0x004b28c0
static const int32_t s_extended_float_format_b[6] = {
    128, -127, 24, 8, 32, 127
};

/* CrtExtended80ToBinaryFloat (FUN_00497b90): convert the CRT's packed
 * 80-bit significand/exponent form into a 32- or 64-bit IEEE layout. */
// FUNCTION: LEGORR 0x00497b90
uint32_t crt_extended80_to_binary_float(const uint16_t* source,
                                        uint32_t* destination,
                                        const int32_t* format)
{
    uint16_t sign_and_exponent = source[5];
    uint32_t limbs[3];
    uint32_t original_limbs[3];
    uint32_t low_word;
    uint32_t high_word;
    uint32_t exponent = sign_and_exponent & 0x7fffu;
    int binary_exponent = (int)exponent - 0x3fff;
    uint32_t status = 0u;

    memcpy(&limbs[1], source + 1, sizeof(uint32_t));
    memcpy(&limbs[0], source + 3, sizeof(uint32_t));
    limbs[2] = (uint32_t)source[0] << 16;

    if (binary_exponent == -0x3fff) {
        binary_exponent = 0;
        if (!uint96_is_zero(limbs)) {
            uint96_zero(limbs);
            status = 2u;
        }
    }
    else {
        uint96_copy(original_limbs, limbs);
        if (crt_uint96_clear_from_bit(limbs, format[2]) != 0u) {
            binary_exponent = (int)exponent - 0x3ffe;
        }
        if (binary_exponent < format[1] - format[2]) {
            uint96_zero(limbs);
            binary_exponent = 0;
            status = 2u;
        }
        else if (binary_exponent <= format[1]) {
            uint96_copy(limbs, original_limbs);
            uint96_shift_right(limbs, format[1] - binary_exponent);
            crt_uint96_clear_from_bit(limbs, format[2]);
            uint96_shift_right(limbs, format[3] + 1);
            binary_exponent = 0;
            status = 2u;
        }
        else if (binary_exponent >= format[0]) {
            uint96_zero(limbs);
            limbs[0] |= 0x80000000u;
            uint96_shift_right(limbs, format[3]);
            binary_exponent = format[5] + format[0];
            status = 1u;
        }
        else {
            binary_exponent += format[5];
            limbs[0] &= 0x7fffffffu;
            uint96_shift_right(limbs, format[3]);
        }
    }

    high_word = ((uint32_t)binary_exponent <<
                (0x1fu - (uint32_t)format[3] & 0x1fu)) |
                ((sign_and_exponent & 0x8000u) ? 0x80000000u : 0u) |
                limbs[0];
    low_word = limbs[1];
    if (format[4] == 0x40) {
        destination[0] = low_word;
        destination[1] = high_word;
    }
    else if (format[4] == 0x20) {
        destination[0] = high_word;
    }
    return status;
}

/* The two format wrappers return the converter's status even though the
 * decompiler's prototype loses the EAX result; callers branch on 1/2. */
// FUNCTION: LEGORR 0x00497d60
uint32_t crt_extended_float_convert_format_a(const uint16_t* source,
                                             uint32_t* destination)
{
    return crt_extended80_to_binary_float(source, destination,
                                          s_extended_float_format_a);
}

// FUNCTION: LEGORR 0x00497d80
uint32_t crt_extended_float_convert_format_b(const uint16_t* source,
                                             uint32_t* destination)
{
    return crt_extended80_to_binary_float(source, destination,
                                          s_extended_float_format_b);
}

/* CrtDoubleToExtendedFloatConvertA/B (FUN_00497da0/00497de0): expand the
 * by-value IEEE double into the CRT's 80-bit temporary, then convert it to
 * the selected binary format. */
// FUNCTION: LEGORR 0x00497da0
uint32_t crt_double_to_extended_float_convert_a(uint32_t* destination,
                                                double value)
{
    uint16_t extended[6];
    crt_double_to_extended80(extended, &value);
    return crt_extended_float_convert_format_a(extended, destination);
}

// FUNCTION: LEGORR 0x00497de0
uint32_t crt_double_to_extended_float_convert_b(uint32_t* destination,
                                                double value)
{
    uint16_t extended[6];
    crt_double_to_extended80(extended, &value);
    return crt_extended_float_convert_format_b(extended, destination);
}

/* CrtStringDuplicate (FUN_0049b660) — see crt.hpp. The corpus's length walk
 * counts bytes through the NUL, so the copy is strlen + 1 bytes. */
// FUNCTION: LEGORR 0x0049b660
char* crt_string_duplicate(const char* text)
{
    if (text == NULL) {
        return NULL; /* deviation: the corpus strides the string blindly */
    }
    const size_t n = strlen(text) + 1u;
    char* copy = (char*)engine_heap_alloc((long)n);
    if (copy != NULL) {
        memcpy(copy, text, n);
    }
    return copy; /* NULL allocation answers NULL, like the corpus */
}

/* CrtUnsignedIntegerFormatRadix (FUN_0049dba0) — see crt.hpp. The corpus's
 * digit fold is `digit < 10 ? digit + '0' : digit + 'W'`, which lands the
 * letters lowercase; the port pins that expression shape. */
// FUNCTION: LEGORR 0x0049dba0
void crt_unsigned_format_radix(uint32_t value, char* out, int radix,
                               int signed_value)
{
    if (out == NULL) {
        return; /* deviation: the corpus strides the buffer blindly */
    }
    if (radix < 2) {
        out[0] = '0';
        out[1] = '\0';
        return; /* deviation: the corpus's divide loop never terminates */
    }
    char* head = out;
    char* end;
    uint32_t v = value;
    if (signed_value != 0) {
        *out = '-';
        ++out;
        v = (uint32_t)0u - v;
        head = out;
    }
    end = out;
    for (;;) {
        const uint32_t digit = v % (uint32_t)radix;
        v /= (uint32_t)radix;
        *end++ = (char)(digit < 10u ? digit + (uint32_t)'0'
                                    : digit + (uint32_t)'W');
        if (v == 0u) {
            break;
        }
    }
    *end = '\0';
    /* Reverse in place: the corpus swaps the last digit with the buffer
     * head, then walks the ends toward each other until they cross. */
    char* tail = end - 1;
    while (head < tail) {
        const char t = *head;
        *head = *tail;
        *tail = t;
        ++head;
        --tail;
    }
}

/* SignedDecimalToStringConvert (FUN_0049db60) — see crt.hpp. The corpus
 * picks the callee's negate flag: 1 only for radix 10 with a negative
 * value (push 1 / push 0 at 0x49db77/0x49db90), the sign half living in
 * the callee at 0x49dbac. */
// FUNCTION: LEGORR 0x0049db60
char* crt_signed_to_string(int value, char* out, int radix)
{
    if (radix == 10 && value < 0) {
        /* The corpus negates with wrapping int arithmetic; INT_MIN rides
         * through as its 2^31 magnitude. */
        crt_unsigned_format_radix((uint32_t)value, out, 10, 1);
    } else {
        crt_unsigned_format_radix((uint32_t)value, out, radix, 0);
    }
    return out;
}

/* StringToUpper / StringToLowercase (FUN_0049dcd0 / FUN_0049dc10) — see
 * crt.hpp. The C-locale fast half folds ASCII letters in place; the
 * DAT_00549180 locale half rides the recorded locale-map chain. */
// GLOBAL: LEGORR 0x00549180
static uint32_t s_crt_locale_handle; /* DAT_00549180: zero-init locale handle */

// FUNCTION: LEGORR 0x0049dcd0
char* crt_string_to_upper(char* text)
{
    if (text == NULL) {
        return NULL;
    }
    char* original = text;
    if (s_crt_locale_handle == 0u) {
        if (*text != '\0') {
            do {
                if (*text >= 'a' && *text <= 'z') {
                    *text = (char)(*text - 0x20);
                }
                ++text;
            } while (*text != '\0');
            return original;
        }
    }
    else {
        int mapped_count = crt_locale_character_map_string(
            s_crt_locale_handle, 0x200u, text, -1, NULL, 0, 0u);
        if (mapped_count != 0) {
            char* mapped = (char*)engine_heap_alloc((long)mapped_count);
            if (mapped != NULL) {
                if (crt_locale_character_map_string(
                        s_crt_locale_handle, 0x200u, text, -1,
                        mapped, mapped_count, 0u) != 0) {
                    size_t copy_count = strlen(mapped) + 1u;
                    memcpy(text, mapped, copy_count);
                }
                engine_heap_free(mapped);
            }
        }
    }
    return original;
}

// FUNCTION: LEGORR 0x0049dc10
char* crt_string_to_lower(char* text)
{
    if (text == NULL) {
        return NULL;
    }
    char* original = text;
    if (s_crt_locale_handle == 0u) {
        if (*text != '\0') {
            do {
                if (*text >= 'A' && *text <= 'Z') {
                    *text = (char)(*text + 0x20);
                }
                ++text;
            } while (*text != '\0');
            return original;
        }
    }
    else {
        int mapped_count = crt_locale_character_map_string(
            s_crt_locale_handle, 0x100u, text, -1, NULL, 0, 0u);
        if (mapped_count != 0) {
            char* mapped = (char*)engine_heap_alloc((long)mapped_count);
            if (mapped != NULL) {
                if (crt_locale_character_map_string(
                        s_crt_locale_handle, 0x100u, text, -1,
                        mapped, mapped_count, 0u) != 0) {
                    size_t copy_count = strlen(mapped) + 1u;
                    memcpy(text, mapped, copy_count);
                }
                engine_heap_free(mapped);
            }
        }
    }
    return original;
}

/* --- math error band (2026-10-25) -------------------------------------------
 * The corpus's _DAT_005490d0 math-error slot lands as guarded own storage
 * (the fd/codec readers ride the unported CRT runtime band — deviation). */
// GLOBAL: LEGORR 0x005490d0
static uint32_t s_math_error_code; /* _DAT_005490d0 role */
// GLOBAL: LEGORR 0x005490d4
static uint32_t s_win32_error_code; /* DAT_005490d4 role */
// GLOBAL: LEGORR 0x004b28e0
static uint32_t s_math_exception_mode[4] = { 0x2694u, 0u, 0u, 0u };

// FUNCTION: LEGORR 0x004939a0
void crt_math_error_code_set(int param)
{
    if (param != 1) {
        if (param <= 1 || param > 3) {
            return;
        }
        s_math_error_code = 0x22u;
        return;
    }
    s_math_error_code = 0x21u;
    /* other parameters: the slot stays (faithful) */
}

uint32_t crt_math_error_code_peek(void) { return s_math_error_code; }

// FUNCTION: LEGORR 0x00493be0
#if defined(_MSC_VER) && defined(_M_IX86)
/* Corpus 0x493be4 waits, then stores the status word to [ebp-2]. */
__declspec(naked) int fpu_status_word_get_variant_a(void)
{
    __asm {
        push ebp
        mov ebp, esp
        push ecx
        fstsw word ptr [ebp - 2]
        movsx eax, word ptr [ebp - 2]
        mov esp, ebp
        pop ebp
        ret
    }
}
#else
int fpu_status_word_get_variant_a(void)
{
    unsigned short status = 0;
#if defined(__i386__) || defined(__x86_64__)
    __asm__ __volatile__("fnstsw %0" : "=a"(status));
#endif
    return (int)(short)status;
}
#endif

// FUNCTION: LEGORR 0x00493bf0
int fpu_status_word_get_variant_b(void)
{
    unsigned short status = 0;
#if defined(_MSC_VER) && defined(_M_IX86)
    __asm {
        fnstsw ax
        mov status, ax
    }
#elif defined(__i386__) || defined(__x86_64__)
    __asm__ __volatile__("fnstsw %0" : "=a"(status));
#endif
    return (int)(short)status;
}

// FUNCTION: LEGORR 0x00493c10
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) int fpu_control_word_update_masked(uint32_t value,
                                                     uint32_t mask)
{
    __asm {
        push ebp
        mov ebp, esp
        push ecx
        fstcw word ptr [ebp - 4]
        mov eax, dword ptr [ebp + 0ch]
        mov ecx, dword ptr [ebp + 8]
        mov edx, eax
        and edx, ecx
        mov ecx, dword ptr [ebp - 4]
        not eax
        and eax, ecx
        or edx, eax
        mov dword ptr [ebp + 0ch], edx
        fldcw word ptr [ebp + 0ch]
        movsx eax, cx
        mov esp, ebp
        pop ebp
        ret
    }
}
#else
int fpu_control_word_update_masked(uint32_t value, uint32_t mask)
{
    unsigned short old_word = 0;
    unsigned short new_word;
#if defined(_MSC_VER) && defined(_M_IX86)
    __asm { fnstcw old_word }
#elif defined(__i386__) || defined(__x86_64__)
    __asm__ __volatile__("fnstcw %0" : "=m"(old_word));
#endif
    new_word = (unsigned short)(((unsigned short)value & (unsigned short)mask) |
                               (old_word & (unsigned short)~mask));
#if defined(_MSC_VER) && defined(_M_IX86)
    __asm { fldcw new_word }
#elif defined(__i386__) || defined(__x86_64__)
    __asm__ __volatile__("fldcw %0" : : "m"(new_word));
#endif
    return (int)(short)old_word;
}
#endif

/* CrtMathExceptionCodeFromStatus (FUN_00493a00) — see crt.hpp. The priority
 * arms follow the decompile; the final raw low-byte return follows the
 * shipped instruction (`and eax,0xff`). */
// FUNCTION: LEGORR 0x00493a00
int crt_math_exception_code_from_status(uint32_t status)
{
    if ((status & 0x20u) != 0u) {
        return 5;
    }
    if ((status & 8u) != 0u) {
        return 1;
    }
    if ((status & 4u) != 0u) {
        return 2;
    }
    if ((status & 1u) != 0u) {
        return 3;
    }
    return (int)(status & 0xffu);
}

void global_flag_004ff978_install(uint32_t value)
{
    s_flag_004ff978 = value;
}

/* --- math handler table + 64-bit multiply thunk (2026-10-25) --------------- */

/* CrtMathExceptionHandlerGet (FUN_004939d0) — see crt.hpp. The corpus scans
 * the DAT_004b2170 table (27 stride-8 records, id dword then handler dword)
 * with an add-8 pointer walk bounded by 0x4b2248 and loads a hit through the
 * scaled index; the table is no longer zeroed own storage, the seed below
 * reproduces its image bytes. The install hook still stands in for the
 * unported CRT startup band (deviation). */
#define CRT_MATH_HANDLER_ENTRIES 27

/* The corpus's stride-8 handler table (id dword, handler dword), image-
 * populated with the CRT startup band's twenty-seven stub handlers
 * (0x49ec70..0x49ed20); seeded so the DAT's initial bytes match. */
struct CrtMathHandlerRecord {
    int      id;
    uint32_t handler;
};

// GLOBAL: LEGORR 0x004b2170
static CrtMathHandlerRecord s_math_handlers[CRT_MATH_HANDLER_ENTRIES] = {
    { 0x14, 0x49ed20u },
    { 0x1d, 0x49ed1cu },
    { 0x1a, 0x49ed18u },
    { 0x1b, 0x49ed10u },
    { 0x1f, 0x49ed08u },
    { 0x13, 0x49ed00u },
    { 0x21, 0x49ecf8u },
    { 0xe,  0x49ecf0u },
    { 0xd,  0x49ece8u },
    { 0xf,  0x49ece0u },
    { 0x10, 0x49ecd8u },
    { 5,    0x49ecd0u },
    { 0x1e, 0x49ecccu },
    { 0x12, 0x49ecc8u },
    { 0x20, 0x49ecc4u },
    { 0xc,  0x49ecbcu },
    { 0xb,  0x49ecb4u },
    { 0x15, 0x49ecacu },
    { 0x1c, 0x49eca4u },
    { 0x19, 0x49ec9cu },
    { 0x11, 0x49ec94u },
    { 0x18, 0x49ec8cu },
    { 0x16, 0x49ec84u },
    { 0x17, 0x49ec7cu },
    { 0x22, 0x49ec78u },
    { 0x23, 0x49ec74u },
    { 0x24, 0x49ec70u }
};

// FUNCTION: LEGORR 0x004939d0
uint32_t crt_math_exception_handler_get(int id)
{
    /* Corpus walk shape: the counter clears before the walk seeds, the
     * add-8 pointer bounds the scan against the table end through a
     * SIGNED (long) compare so MSVC emits jl, and the hit loads back
     * through the hop counter's scaled index. */
    int i = 0;
    const CrtMathHandlerRecord* p = s_math_handlers;
    while ((long)p < (long)(s_math_handlers + CRT_MATH_HANDLER_ENTRIES)) {
        if (p->id == id) {
            return s_math_handlers[i].handler;
        }
        ++p;
        ++i;
    }
    return 0u;
}

void crt_math_exception_handler_install(int index, int id, uint32_t handler)
{
    if (index >= 0 && index < CRT_MATH_HANDLER_ENTRIES) {
        s_math_handlers[index].id = id;
        s_math_handlers[index].handler = handler;
    }
}

/* CrtInteger64Multiply (FUN_0048ea90, __allmul) — see crt.hpp. The two
 * cross-word products contribute only their low 32 bits (the bit pattern
 * is sign-agnostic), so the fold lands as plain wrapping 32-bit adds.
 * The winapi branch is a byte-faithful naked reproduction of the corpus's
 * unsigned mul sequence, including the fast-path skip: when both high
 * words are zero the cross terms are never computed (the `or`/`jne`
 * test at the top); the portable branch keeps the same three-way fold
 * in C. */
// FUNCTION: LEGORR 0x0048ea90
#if defined(LEGORRR_BACKEND_WINAPI)
__declspec(naked) uint64_t crt_integer64_multiply(uint32_t lo_a, int32_t hi_a,
                                                  uint32_t lo_b, int32_t hi_b)
{
    __asm {
        mov eax, [esp + 8]
        mov ecx, [esp + 0x10]
        or ecx, eax
        mov ecx, [esp + 0xc]
        jne cross_terms
        mov eax, [esp + 4]
        mul ecx
        ret 0x10
    cross_terms:
        push ebx
        mul ecx
        mov ebx, eax
        mov eax, [esp + 8]
        mul dword ptr [esp + 0x14]
        add ebx, eax
        mov eax, [esp + 8]
        mul ecx
        add edx, ebx
        pop ebx
        ret 0x10
    }
}
#else
uint64_t crt_integer64_multiply(uint32_t lo_a, int32_t hi_a,
                                uint32_t lo_b, int32_t hi_b)
{
    const uint64_t low64 = (uint64_t)lo_a * (uint64_t)lo_b;
    if (hi_a == 0 && hi_b == 0) {
        return low64;
    }
    const uint32_t high32 = (uint32_t)(low64 >> 32)
                          + (uint32_t)((uint32_t)(uint32_t)hi_a * lo_b)
                          + (uint32_t)(lo_a * (uint32_t)(uint32_t)hi_b);
    return ((uint64_t)high32 << 32) | (uint32_t)low64;
}
#endif

/* CrtInteger64Divide (FUN_0048e9e0, __alldiv) — see crt.hpp. The Win32
 * body follows the corpus's signed word-pair normalization, two-word
 * divisor shift, one-word trial divide, and multiply-back repair. Its one
 * extra zero-divisor branch preserves this port's tested zero result rather
 * than the corpus's divide exception. The portable fallback keeps the same
 * value semantics without relying on x86 inline assembly. */
// FUNCTION: LEGORR 0x0048e9e0
#if defined(LEGORRR_BACKEND_WINAPI)
__declspec(naked) uint64_t crt_integer64_divide(uint32_t lo_a, uint32_t hi_a,
                                                uint32_t lo_b, uint32_t hi_b)
{
    __asm {
        push edi
        push esi
        push ebx
        xor edi, edi
        mov eax, [esp + 0x14]
        or eax, eax
        jge numerator_nonnegative
        inc edi
        mov edx, [esp + 0x10]
        neg eax
        neg edx
        sbb eax, 0
        mov [esp + 0x14], eax
        mov [esp + 0x10], edx
    numerator_nonnegative:
        mov eax, [esp + 0x1c]
        or eax, eax
        jge divisor_nonnegative
        inc edi
        mov edx, [esp + 0x18]
        neg eax
        neg edx
        sbb eax, 0
        mov [esp + 0x1c], eax
        mov [esp + 0x18], edx
    divisor_nonnegative:
        or eax, eax
        jne wide_divisor
        mov ecx, [esp + 0x18]
        mov eax, [esp + 0x14]
        xor edx, edx
        test ecx, ecx
        jz zero_divisor
        div ecx
        mov ebx, eax
        mov eax, [esp + 0x10]
        div ecx
        mov edx, ebx
        jmp quotient_ready
    wide_divisor:
        mov ebx, eax
        mov ecx, [esp + 0x18]
        mov edx, [esp + 0x14]
        mov eax, [esp + 0x10]
    shift_divisor_and_numerator:
        shr ebx, 1
        rcr ecx, 1
        shr edx, 1
        rcr eax, 1
        or ebx, ebx
        jne shift_divisor_and_numerator
        div ecx
        mov esi, eax
        mul dword ptr [esp + 0x1c]
        mov ecx, eax
        mov eax, [esp + 0x18]
        mul esi
        add edx, ecx
        jb quotient_too_large
        cmp edx, [esp + 0x14]
        ja quotient_too_large
        jb quotient_in_range
        cmp eax, [esp + 0x10]
        jbe quotient_in_range
    quotient_too_large:
        dec esi
    quotient_in_range:
        xor edx, edx
        mov eax, esi
    quotient_ready:
        dec edi
        jne quotient_sign_done
        neg edx
        neg eax
        sbb edx, 0
    quotient_sign_done:
        pop ebx
        pop esi
        pop edi
        ret 0x10
    zero_divisor:
        xor eax, eax
        xor edx, edx
        pop ebx
        pop esi
        pop edi
        ret 0x10
    }
}
#else
uint64_t crt_integer64_divide(uint32_t lo_a, uint32_t hi_a,
                              uint32_t lo_b, uint32_t hi_b)
{
    uint32_t negative = 0;
    uint32_t dividend_lo = lo_a;
    uint32_t dividend_hi = hi_a;
    uint32_t divisor_lo = lo_b;
    uint32_t divisor_hi = hi_b;
    uint32_t quotient_lo;
    uint32_t quotient_hi;

    if ((int32_t)dividend_hi < 0) {
        uint32_t borrow = dividend_lo != 0;
        dividend_lo = 0u - dividend_lo;
        dividend_hi = 0u - borrow - dividend_hi;
        negative++;
    }
    if ((int32_t)divisor_hi < 0) {
        uint32_t borrow = divisor_lo != 0;
        divisor_lo = 0u - divisor_lo;
        divisor_hi = 0u - borrow - divisor_hi;
        negative++;
    }

    if (divisor_hi == 0) {
        uint32_t remainder;
        quotient_hi = dividend_hi / divisor_lo;
        remainder = dividend_hi % divisor_lo;
        quotient_lo = (uint32_t)((((uint64_t)remainder << 32) | dividend_lo)
                                 / divisor_lo);
    }
    else {
        uint32_t shifted_dividend_lo = dividend_lo;
        uint32_t shifted_dividend_hi = dividend_hi;
        uint32_t shifted_divisor_lo = divisor_lo;
        uint32_t shifted_divisor_hi = divisor_hi;
        uint64_t trial_product;
        uint32_t trial_quotient;
        uint32_t product_hi;
        uint32_t product_lo;
        uint32_t product_overflow;

        do {
            uint32_t divisor_carry = shifted_divisor_hi & 1;
            uint32_t dividend_carry = shifted_dividend_hi & 1;
            shifted_divisor_hi >>= 1;
            shifted_divisor_lo = (shifted_divisor_lo >> 1)
                               | (divisor_carry << 31);
            shifted_dividend_hi >>= 1;
            shifted_dividend_lo = (shifted_dividend_lo >> 1)
                                | (dividend_carry << 31);
        } while (shifted_divisor_hi != 0);

        trial_quotient = (uint32_t)((((uint64_t)shifted_dividend_hi << 32)
                                    | shifted_dividend_lo)
                                   / shifted_divisor_lo);
        trial_product = (uint64_t)divisor_lo * trial_quotient;
        product_lo = (uint32_t)trial_product;
        product_hi = (uint32_t)(trial_product >> 32);
        product_overflow = product_hi + trial_quotient * divisor_hi;
        if (product_overflow < product_hi || dividend_hi < product_overflow
            || (dividend_hi == product_overflow && dividend_lo < product_lo)) {
            trial_quotient--;
        }
        quotient_lo = trial_quotient;
        quotient_hi = 0;
    }

    if (negative == 1) {
        uint32_t borrow = quotient_lo != 0;
        quotient_lo = 0u - quotient_lo;
        quotient_hi = 0u - borrow - quotient_hi;
    }
    return ((uint64_t)quotient_hi << 32) | quotient_lo;
}
#endif

/* CrtUnsigned64Divide (FUN_00498bf0, __aulldiv) — see crt.hpp. The
 * unsigned sibling: the same shift/trial/repair shape, so the same fold;
 * the high half of the answer is pinned zero only when the repair needs
 * it (the estimate can overflow the 32-bit trial), never by construction. */
// FUNCTION: LEGORR 0x00498bf0
#if defined(_MSC_VER) && defined(_M_IX86)
/* Corpus 0x498c12 reuses EAX's high divisor and loads its low word into EBX. */
__declspec(naked) uint64_t __stdcall crt_unsigned64_divide(
    uint32_t lo_a, uint32_t hi_a, uint32_t lo_b, uint32_t hi_b)
{
    __asm {
        push ebx
        push esi
        mov eax, dword ptr [esp + 18h]
        or eax, eax
        jne divide_wide
        mov ecx, dword ptr [esp + 14h]
        mov eax, dword ptr [esp + 10h]
        xor edx, edx
        div ecx
        mov ebx, eax
        mov eax, dword ptr [esp + 0ch]
        div ecx
        mov edx, ebx
        jmp divide_return

    divide_wide:
        mov ecx, eax
        mov ebx, dword ptr [esp + 14h]
        mov edx, dword ptr [esp + 10h]
        mov eax, dword ptr [esp + 0ch]
    divide_normalize:
        shr ecx, 1
        rcr ebx, 1
        shr edx, 1
        rcr eax, 1
        or ecx, ecx
        jne divide_normalize
        div ebx
        mov esi, eax
        mul dword ptr [esp + 18h]
        mov ecx, eax
        mov eax, dword ptr [esp + 14h]
        mul esi
        add edx, ecx
        jb divide_adjust
        cmp edx, dword ptr [esp + 10h]
        ja divide_adjust
        jb divide_no_adjust
        cmp eax, dword ptr [esp + 0ch]
        jbe divide_no_adjust
    divide_adjust:
        dec esi
    divide_no_adjust:
        xor edx, edx
        mov eax, esi
    divide_return:
        pop esi
        pop ebx
        ret 10h
    }
}
#else
uint64_t crt_unsigned64_divide(uint32_t lo_a, uint32_t hi_a,
                              uint32_t lo_b, uint32_t hi_b)
{
    uint32_t quotient_low;
    uint32_t quotient_high;
    uint32_t remainder;
    uint32_t shifted_dividend_low;
    uint32_t shifted_dividend_high;
    uint32_t shifted_divisor_low;
    uint32_t shifted_divisor_high;
    uint32_t trial_quotient;
    uint32_t product_low;
    uint32_t product_high;
    uint32_t product_overflow;
    uint64_t trial_product;

    if (hi_b == 0u) {
        quotient_high = hi_a / lo_b;
        remainder = hi_a % lo_b;
#if defined(_MSC_VER) && defined(_M_IX86)
        __asm {
            mov eax, lo_a
            mov edx, remainder
            div lo_b
            mov quotient_low, eax
        }
#else
        quotient_low = (uint32_t)((((uint64_t)remainder << 32) | lo_a)
                                  / lo_b);
#endif
    }
    else {
        shifted_dividend_low = lo_a;
        shifted_dividend_high = hi_a;
        shifted_divisor_low = lo_b;
        shifted_divisor_high = hi_b;
        do {
            uint32_t divisor_carry = shifted_divisor_high & 1u;
            uint32_t dividend_carry = shifted_dividend_high & 1u;
            shifted_divisor_high >>= 1;
            shifted_divisor_low = (shifted_divisor_low >> 1) |
                                  (divisor_carry << 31);
            shifted_dividend_high >>= 1;
            shifted_dividend_low = (shifted_dividend_low >> 1) |
                                   (dividend_carry << 31);
        } while (shifted_divisor_high != 0u);
#if defined(_MSC_VER) && defined(_M_IX86)
        __asm {
            mov eax, shifted_dividend_low
            mov edx, shifted_dividend_high
            div shifted_divisor_low
            mov trial_quotient, eax
        }
#else
        trial_quotient = (uint32_t)((((uint64_t)shifted_dividend_high << 32) |
                                     shifted_dividend_low) /
                                    shifted_divisor_low);
#endif
        trial_product = (uint64_t)lo_b * trial_quotient;
        product_low = (uint32_t)trial_product;
        product_high = (uint32_t)(trial_product >> 32);
        product_overflow = product_high + trial_quotient * hi_b;
        if (product_overflow < product_high || hi_a < product_overflow ||
            (hi_a == product_overflow && lo_a < product_low)) {
            --trial_quotient;
        }
        quotient_low = trial_quotient;
        quotient_high = 0u;
    }

    return ((uint64_t)quotient_high << 32) | quotient_low;
}
#endif

// GLOBAL: LEGORR 0x004afd72
static uint16_t s_ctype_character_flags[256] = {
    0x0020u, 0x0020u, 0x0020u, 0x0020u, 0x0020u, 0x0020u, 0x0020u, 0x0020u,
    0x0020u, 0x0028u, 0x0028u, 0x0028u, 0x0028u, 0x0028u, 0x0020u, 0x0020u,
    0x0020u, 0x0020u, 0x0020u, 0x0020u, 0x0020u, 0x0020u, 0x0020u, 0x0020u,
    0x0020u, 0x0020u, 0x0020u, 0x0020u, 0x0020u, 0x0020u, 0x0020u, 0x0020u,
    0x0048u, 0x0010u, 0x0010u, 0x0010u, 0x0010u, 0x0010u, 0x0010u, 0x0010u,
    0x0010u, 0x0010u, 0x0010u, 0x0010u, 0x0010u, 0x0010u, 0x0010u, 0x0010u,
    0x0084u, 0x0084u, 0x0084u, 0x0084u, 0x0084u, 0x0084u, 0x0084u, 0x0084u,
    0x0084u, 0x0084u, 0x0010u, 0x0010u, 0x0010u, 0x0010u, 0x0010u, 0x0010u,
    0x0010u, 0x0081u, 0x0081u, 0x0081u, 0x0081u, 0x0081u, 0x0081u, 0x0001u,
    0x0001u, 0x0001u, 0x0001u, 0x0001u, 0x0001u, 0x0001u, 0x0001u, 0x0001u,
    0x0001u, 0x0001u, 0x0001u, 0x0001u, 0x0001u, 0x0001u, 0x0001u, 0x0001u,
    0x0001u, 0x0001u, 0x0001u, 0x0010u, 0x0010u, 0x0010u, 0x0010u, 0x0010u,
    0x0010u, 0x0082u, 0x0082u, 0x0082u, 0x0082u, 0x0082u, 0x0082u, 0x0002u,
    0x0002u, 0x0002u, 0x0002u, 0x0002u, 0x0002u, 0x0002u, 0x0002u, 0x0002u,
    0x0002u, 0x0002u, 0x0002u, 0x0002u, 0x0002u, 0x0002u, 0x0002u, 0x0002u,
    0x0002u, 0x0002u, 0x0002u, 0x0010u, 0x0010u, 0x0010u, 0x0010u, 0x0020u
};

// GLOBAL: LEGORR 0x004afd68
static uint16_t* volatile s_ctype_pointer = s_ctype_character_flags;
// GLOBAL: LEGORR 0x005492b8
static uint8_t s_multibyte_character_flags[257];
// GLOBAL: LEGORR 0x005493bc
static uint32_t s_active_codepage;
// GLOBAL: LEGORR 0x00549190
static uint32_t s_ansi_codepage;
// GLOBAL: LEGORR 0x005493c0
static uint32_t s_active_locale_table;
// GLOBAL: LEGORR 0x005490f8
static char** s_environment;
// GLOBAL: LEGORR 0x005490fc
static char** s_environment_sentinel;
// GLOBAL: LEGORR 0x00549100
static uint16_t** s_wide_environment;
// GLOBAL: LEGORR 0x0054911c
static char* s_environment_block;
// GLOBAL: LEGORR 0x005492b0
static int32_t s_environment_api_mode;
// GLOBAL: LEGORR 0x00549510
static int32_t s_compare_string_api_mode;
// GLOBAL: LEGORR 0x005493c8
static uint32_t s_codepage_metadata_word0;
// GLOBAL: LEGORR 0x005493cc
static uint32_t s_codepage_metadata_word1;
// GLOBAL: LEGORR 0x005493d0
static uint32_t s_codepage_metadata_word2;
// GLOBAL: LEGORR 0x005493d4
static uint32_t s_codepage_fallback_mode;
// GLOBAL: LEGORR 0x005490d8
static uint32_t s_file_open_attribute_policy;
// GLOBAL: LEGORR 0x005494f4
static uint32_t s_file_open_share_state;
// GLOBAL: LEGORR 0x00549170
static int32_t s_locale_map_api_mode;
// GLOBAL: LEGORR 0x005493e0
static int32_t s_string_type_api_mode;
// GLOBAL: LEGORR 0x004b2330
static const uint8_t s_predefined_codepage_masks[4] = { 1u, 2u, 4u, 8u };
// GLOBAL: LEGORR 0x004b2338
static const uint8_t s_predefined_codepage_records[5][48] = {
    { 0xa4u,0x03u,0x00u,0x00u,0x60u,0x82u,0x79u,0x82u,
      0x21u,0x00u,0x00u,0x00u,0x00u,0x00u,0x00u,0x00u,
      0xa6u,0xdfu,0x00u,0x00u,0x00u,0x00u,0x00u,0x00u,
      0xa1u,0xa5u,0x00u,0x00u,0x00u,0x00u,0x00u,0x00u,
      0x81u,0x9fu,0xe0u,0xfcu,0x00u,0x00u,0x00u,0x00u,
      0x40u,0x7eu,0x80u,0xfcu,0x00u,0x00u,0x00u,0x00u },
    { 0xa8u,0x03u,0x00u,0x00u,0xc1u,0xa3u,0xdau,0xa3u,
      0x20u,0x00u,0x00u,0x00u,0x00u,0x00u,0x00u,0x00u,
      0x00u,0x00u,0x00u,0x00u,0x00u,0x00u,0x00u,0x00u,
      0x00u,0x00u,0x00u,0x00u,0x00u,0x00u,0x00u,0x00u,
      0x81u,0xfeu,0x00u,0x00u,0x00u,0x00u,0x00u,0x00u,
      0x40u,0xfeu,0x00u,0x00u,0x00u,0x00u,0x00u,0x00u },
    { 0xb5u,0x03u,0x00u,0x00u,0xc1u,0xa3u,0xdau,0xa3u,
      0x20u,0x00u,0x00u,0x00u,0x00u,0x00u,0x00u,0x00u,
      0x00u,0x00u,0x00u,0x00u,0x00u,0x00u,0x00u,0x00u,
      0x00u,0x00u,0x00u,0x00u,0x00u,0x00u,0x00u,0x00u,
      0x81u,0xfeu,0x00u,0x00u,0x00u,0x00u,0x00u,0x00u,
      0x41u,0xfeu,0x00u,0x00u,0x00u,0x00u,0x00u,0x00u },
    { 0xb6u,0x03u,0x00u,0x00u,0xcfu,0xa2u,0xe4u,0xa2u,
      0x1au,0x00u,0xe5u,0xa2u,0xe8u,0xa2u,0x5bu,0x00u,
      0x00u,0x00u,0x00u,0x00u,0x00u,0x00u,0x00u,0x00u,
      0x00u,0x00u,0x00u,0x00u,0x00u,0x00u,0x00u,0x00u,
      0x81u,0xfeu,0x00u,0x00u,0x00u,0x00u,0x00u,0x00u,
      0x40u,0x7eu,0xa1u,0xfeu,0x00u,0x00u,0x00u,0x00u },
    { 0x51u,0x05u,0x00u,0x00u,0x51u,0xdau,0x5eu,0xdau,
      0x20u,0x00u,0x5fu,0xdau,0x6au,0xdau,0x32u,0x00u,
      0x00u,0x00u,0x00u,0x00u,0x00u,0x00u,0x00u,0x00u,
      0x00u,0x00u,0x00u,0x00u,0x00u,0x00u,0x00u,0x00u,
      0x81u,0xd3u,0xd8u,0xdeu,0xe0u,0xf9u,0x00u,0x00u,
      0x31u,0x7eu,0x81u,0xfeu,0x00u,0x00u,0x00u,0x00u }
};
// GLOBAL: LEGORR 0x004b28f0
static int32_t s_timezone_bias_seconds = 28800;
// GLOBAL: LEGORR 0x004b28f4
static int32_t s_daylight_saving_available = 1;
// GLOBAL: LEGORR 0x004b28f8
static int32_t s_daylight_saving_bias_seconds = -3600;
// GLOBAL: LEGORR 0x004b2900
static char s_standard_timezone_name_buffer[64] = "PST";
// GLOBAL: LEGORR 0x004b2940
static char s_daylight_timezone_name_buffer[64] = "PDT";
// GLOBAL: LEGORR 0x004b2980
static char* s_standard_timezone_name = s_standard_timezone_name_buffer;
// GLOBAL: LEGORR 0x004b2984
static char* s_daylight_timezone_name = s_daylight_timezone_name_buffer;
struct CrtDaylightTransitionCache {
    uint32_t start_year;
    int32_t start_day;
    int32_t start_milliseconds;
    uint32_t reserved;
    uint32_t end_year;
    int32_t end_day;
    int32_t end_milliseconds;
};
// GLOBAL: LEGORR 0x004b2988
static CrtDaylightTransitionCache s_daylight_transition_cache = {
    0xffffffffu, 0, 0, 0, 0xffffffffu, 0, 0
};
#define s_daylight_start_year (s_daylight_transition_cache.start_year)
#define s_daylight_start_day (s_daylight_transition_cache.start_day)
#define s_daylight_start_milliseconds (s_daylight_transition_cache.start_milliseconds)
#define s_daylight_end_year (s_daylight_transition_cache.end_year)
#define s_daylight_end_day (s_daylight_transition_cache.end_day)
#define s_daylight_end_milliseconds (s_daylight_transition_cache.end_milliseconds)
// GLOBAL: LEGORR 0x00549438
static int32_t s_timezone_from_windows;

/* TIME_ZONE_INFORMATION has a fixed 0xac-byte layout. Using 16-bit name
 * elements keeps this layout identical on hosts where wchar_t is 32 bits. */
struct CrtSystemTime {
    uint16_t year;
    uint16_t month;
    uint16_t day_of_week;
    uint16_t day;
    uint16_t hour;
    uint16_t minute;
    uint16_t second;
    uint16_t milliseconds;
};

struct CrtTimeZoneInformation {
    int32_t bias;
    uint16_t standard_name[32];
    CrtSystemTime standard_date;
    int32_t standard_bias;
    uint16_t daylight_name[32];
    CrtSystemTime daylight_date;
    int32_t daylight_bias;
};

// GLOBAL: LEGORR 0x00549440
static CrtTimeZoneInformation s_timezone_information;
// GLOBAL: LEGORR 0x005494ec
static char* s_timezone_environment_copy;
// GLOBAL: LEGORR 0x005494f0
static int32_t s_timezone_initialized;
// GLOBAL: LEGORR 0x004b29a4
static const int32_t s_leap_month_day_offsets[14] = {
    0, -1, 30, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365
};
// GLOBAL: LEGORR 0x004b29dc
static const int32_t s_common_month_day_offsets[14] = {
    0, -1, 30, 58, 89, 119, 150, 180, 211, 242, 272, 303, 333, 364
};
// GLOBAL: LEGORR 0x004b2a18
static const uint16_t s_positive_power_of_ten_table[28][6] = {
    { 0x0000u, 0x0000u, 0x0000u, 0x0000u, 0xa000u, 0x4002u },
    { 0x0000u, 0x0000u, 0x0000u, 0x0000u, 0xc800u, 0x4005u },
    { 0x0000u, 0x0000u, 0x0000u, 0x0000u, 0xfa00u, 0x4008u },
    { 0x0000u, 0x0000u, 0x0000u, 0x0000u, 0x9c40u, 0x400cu },
    { 0x0000u, 0x0000u, 0x0000u, 0x0000u, 0xc350u, 0x400fu },
    { 0x0000u, 0x0000u, 0x0000u, 0x0000u, 0xf424u, 0x4012u },
    { 0x0000u, 0x0000u, 0x0000u, 0x8000u, 0x9896u, 0x4016u },
    { 0x0000u, 0x0000u, 0x0000u, 0x2000u, 0xbebcu, 0x4019u },
    { 0x0000u, 0x0000u, 0x0400u, 0xc9bfu, 0x8e1bu, 0x4034u },
    { 0x0000u, 0xa100u, 0xccedu, 0x1bceu, 0xd3c2u, 0x404eu },
    { 0xf020u, 0xb59eu, 0x2b70u, 0xada8u, 0x9dc5u, 0x4069u },
    { 0x5dd0u, 0x25fdu, 0x1ae5u, 0x4f8eu, 0xeb19u, 0x4083u },
    { 0x9671u, 0x95d7u, 0x0e43u, 0x8d05u, 0xaf29u, 0x409eu },
    { 0xbff9u, 0x44a0u, 0x81edu, 0x8f12u, 0x8281u, 0x40b9u },
    { 0x3cbfu, 0xa6d5u, 0xffcfu, 0x1f49u, 0xc278u, 0x40d3u },
    { 0xc66fu, 0x8ce0u, 0x80e9u, 0x47c9u, 0x93bau, 0x41a8u },
    { 0x85bcu, 0x556bu, 0x3927u, 0xf78du, 0xe070u, 0x427cu },
    { 0xddbcu, 0xde8eu, 0x9df9u, 0xebfbu, 0xaa7eu, 0x4351u },
    { 0xe6a1u, 0xe376u, 0xf2ccu, 0x2f29u, 0x8184u, 0x4426u },
    { 0x1028u, 0xaa17u, 0xaef8u, 0xe310u, 0xc4c5u, 0x44fau },
    { 0xa7ebu, 0xf3d4u, 0xebf7u, 0x4ae1u, 0x957au, 0x45cfu },
    { 0xcc65u, 0x91c7u, 0xa60eu, 0xa0aeu, 0xe319u, 0x46a3u },
    { 0x650du, 0x0c17u, 0x8175u, 0x7586u, 0xc976u, 0x4d48u },
    { 0x4258u, 0xa7e4u, 0x3993u, 0x353bu, 0xb2b8u, 0x53edu },
    { 0xa74du, 0x5de5u, 0xc53du, 0x3b5du, 0x9e8bu, 0x5a92u },
    { 0x5dffu, 0xf0a6u, 0x20a1u, 0x54c0u, 0x8ca5u, 0x6137u },
    { 0xfdd1u, 0x5a8bu, 0xd88bu, 0x5d25u, 0xf989u, 0x67dbu },
    { 0x95aau, 0xf3f8u, 0xbf27u, 0xc8a2u, 0xdd5du, 0x6e80u }
};
// GLOBAL: LEGORR 0x004b2b78
static const uint16_t s_negative_power_of_ten_table[28][6] = {
    { 0xcccdu, 0xcccdu, 0xccccu, 0xccccu, 0xccccu, 0x3ffbu },
    { 0x3d71u, 0xd70au, 0x70a3u, 0x0a3du, 0xa3d7u, 0x3ff8u },
    { 0x645au, 0xdf3bu, 0x8d4fu, 0x6e97u, 0x8312u, 0x3ff5u },
    { 0xd3c3u, 0x652cu, 0xe219u, 0x1758u, 0xd1b7u, 0x3ff1u },
    { 0x0fd0u, 0x8423u, 0x1b47u, 0xac47u, 0xa7c5u, 0x3feeu },
    { 0xa640u, 0x69b6u, 0xaf6cu, 0xbd05u, 0x8637u, 0x3febu },
    { 0x3d33u, 0x42bcu, 0xe57au, 0x94d5u, 0xd6bfu, 0x3fe7u },
    { 0xfdc2u, 0xcefdu, 0x8461u, 0x7711u, 0xabccu, 0x3fe4u },
    { 0x4c2fu, 0xe15bu, 0xc44du, 0x94beu, 0xe695u, 0x3fc9u },
    { 0xc492u, 0x3b53u, 0x4475u, 0x14cdu, 0x9abeu, 0x3fafu },
    { 0x67deu, 0x94bau, 0x4539u, 0x1eadu, 0xcfb1u, 0x3f94u },
    { 0x2324u, 0xe2c6u, 0xbabcu, 0x313bu, 0x8b61u, 0x3f7au },
    { 0x5561u, 0xc159u, 0xb17eu, 0x7c53u, 0xbb12u, 0x3f5fu },
    { 0xeed7u, 0x8d2fu, 0xbe06u, 0x8592u, 0xfb15u, 0x3f44u },
    { 0x3f24u, 0xe9a5u, 0xa539u, 0xea27u, 0xa87fu, 0x3f2au },
    { 0xac7du, 0xe4a1u, 0x64bcu, 0x467cu, 0xddd0u, 0x3e55u },
    { 0x7b63u, 0xcc06u, 0x5423u, 0x8377u, 0x91ffu, 0x3d81u },
    { 0xfa91u, 0x193au, 0x637au, 0x4325u, 0xc031u, 0x3cacu },
    { 0x8921u, 0x38d1u, 0x4782u, 0xb897u, 0xfd00u, 0x3bd7u },
    { 0x88dcu, 0x0858u, 0xb11bu, 0xe3e8u, 0xa686u, 0x3b03u },
    { 0x84c6u, 0x4245u, 0xb607u, 0x7599u, 0xdb37u, 0x3a2eu },
    { 0x7133u, 0xd21cu, 0xdb23u, 0xee32u, 0x9049u, 0x395au },
    { 0x87a6u, 0xc0beu, 0xda57u, 0x82a5u, 0xa2a6u, 0x32b5u },
    { 0x68e2u, 0x11b2u, 0x52a7u, 0x449fu, 0xb759u, 0x2c10u },
    { 0x4925u, 0x2de4u, 0x3436u, 0x534fu, 0xceaeu, 0x256bu },
    { 0x598fu, 0xa404u, 0xdec0u, 0x7dc2u, 0xe8fbu, 0x1ec6u },
    { 0xe79eu, 0x5a88u, 0x9157u, 0xbf3cu, 0x8350u, 0x1822u },
    { 0x4b4eu, 0x6265u, 0x83fdu, 0xaf8fu, 0x9406u, 0x117du }
};
// GLOBAL: LEGORR 0x00549128
static CrtDecimalFormatContext* s_current_decimal_format;
// GLOBAL: LEGORR 0x0054912c
static uint8_t s_decimal_format_prepared;
// GLOBAL: LEGORR 0x00549130
static int32_t s_decimal_format_position;
// GLOBAL: LEGORR 0x00549134
static uint8_t s_decimal_format_truncated;
// GLOBAL: LEGORR 0x005493f0
static uint8_t s_decimal_conversion_buffer[32];
// GLOBAL: LEGORR 0x00549410
static CrtDecimalFormatContext s_decimal_format_context;
// GLOBAL: LEGORR 0x004aff74
static volatile uint32_t s_max_multibyte_width = 1u;
// GLOBAL: LEGORR 0x004aff78
static char s_decimal_separator = '.';

/* CrtCodePageLocaleIdLookup (FUN_00496530): the four DBCS pages with
 * built-in Windows locale mappings; all other pages have no mapped LCID. */
// FUNCTION: LEGORR 0x00496530
uint32_t crt_codepage_locale_id_lookup(uint32_t codepage)
{
    switch (codepage) {
    case 0x3a4u: return 0x411u;
    case 0x3a8u: return 0x804u;
    case 0x3b5u: return 0x412u;
    case 0x3b6u: return 0x404u;
    default: return 0u;
    }
}

/* CrtDaylightTransitionBoundaryCompute (FUN_00499da0): calculate a
 * transition's zero-based day-of-year and millisecond-of-day from either
 * an nth-weekday rule or a fixed day rule, then update its cached year. */
// FUNCTION: LEGORR 0x00499da0
void crt_daylight_transition_boundary_compute(int is_start,
                                              int use_nth_weekday,
                                              uint32_t year,
                                              int month_index,
                                              int week,
                                              int weekday,
                                              int fixed_day,
                                              int hour,
                                              int minute,
                                              int second,
                                              int millisecond)
{
    const int32_t* offsets;
    int32_t day;
    int32_t first_weekday;
    int32_t time_of_day;

    offsets = ((year & 3u) == 0u)
        ? s_leap_month_day_offsets : s_common_month_day_offsets;
    if (use_nth_weekday == 1) {
        day = offsets[month_index];
        first_weekday = (int32_t)(((int32_t)(year - 1u) >> 2) - 0x63db +
                                  year * 0x16du + day + 1) % 7;
        if (first_weekday < weekday) {
            day = day - 6 + (week * 7 - first_weekday) + weekday;
        } else {
            day = day + 1 + (week * 7 - first_weekday) + weekday;
        }
        if (week == 5 && day > offsets[month_index + 1]) {
            day -= 7;
        }
    } else {
        day = offsets[month_index] + fixed_day;
    }

    time_of_day = millisecond +
        (second + (minute + hour * 60) * 60) * 1000;
    if (is_start == 1) {
        s_daylight_start_day = day;
        s_daylight_start_year = year;
        s_daylight_start_milliseconds = time_of_day;
        return;
    }

    s_daylight_end_day = day;
    s_daylight_end_milliseconds =
        time_of_day + s_daylight_saving_bias_seconds * 1000;
    if (s_daylight_end_milliseconds < 0) {
        s_daylight_end_year = year;
        s_daylight_end_milliseconds += 86399999;
        return;
    }
    if (s_daylight_end_milliseconds > 86399999) {
        s_daylight_end_milliseconds -= 86399999;
    }
    s_daylight_end_year = year;
}

/* CrtCalendarFieldsToTimeScalar (FUN_00496f50): build the CRT's 32-bit
 * seconds value from calendar fields and optionally apply its DST bias. */
// FUNCTION: LEGORR 0x00496f50
int32_t crt_calendar_fields_to_time_scalar(int year, int month, int day,
                                            int hour, int minute, int second,
                                            int daylight_mode)
{
    int32_t calendar_fields[9];
    uint32_t year_since_1900;
    int32_t day_of_year;
    int32_t result;

    year_since_1900 = (uint32_t)(year - 1900);
    if ((int32_t)year_since_1900 < 0x46 ||
        (int32_t)year_since_1900 > 0x8a) {
        return -1;
    }

    day_of_year = s_common_month_day_offsets[month] + day;
    if ((year_since_1900 & 3u) == 0u && month > 2) {
        ++day_of_year;
    }

    crt_timezone_initialize_once();
    result = second + (minute + (hour +
        (((year - 1901) >> 2) + (int32_t)year_since_1900 * 365 +
         day_of_year) * 24) * 60) * 60 + 0x7c558180 +
        s_timezone_bias_seconds;

    if (daylight_mode != 1 && daylight_mode != -1) {
        return result;
    }
    if (daylight_mode == -1) {
        if (s_daylight_saving_available == 0) {
            return result;
        }
        memset(calendar_fields, 0, sizeof(calendar_fields));
        calendar_fields[0] = second;
        calendar_fields[1] = minute;
        calendar_fields[2] = hour;
        calendar_fields[4] = month - 1;
        calendar_fields[5] = (int32_t)year_since_1900;
        calendar_fields[7] = day_of_year;
        if (crt_daylight_saving_time_check(calendar_fields) == 0) {
            return result;
        }
    }
    return result + s_daylight_saving_bias_seconds;
}

/* CrtTimeZoneInitializeOnce (FUN_00499860): the CRT's one-shot timezone
 * setup guard. */
// FUNCTION: LEGORR 0x00499860
void crt_timezone_initialize_once(void)
{
    if (s_timezone_initialized == 0) {
        crt_timezone_state_init();
        ++s_timezone_initialized;
    }
}

/* CrtTimeZoneStateInit (FUN_00499880): load Windows timezone settings when
 * TZ is absent, otherwise cache and parse the POSIX TZ prefix used by this
 * runtime. The original parser consumes the offset and three-letter DST
 * abbreviation; it does not consume POSIX transition-rule suffixes. */
// FUNCTION: LEGORR 0x00499880
void crt_timezone_state_init(void)
{
    char* timezone_text;
    int negative_offset;

    s_timezone_from_windows = 0;
    s_daylight_end_year = 0xffffffffu;
    s_daylight_start_year = 0xffffffffu;

    timezone_text = crt_environment_variable_lookup("TZ");
    if (timezone_text == NULL) {
#if defined(LEGORRR_BACKEND_WINAPI)
        DWORD timezone_result;
        timezone_result = GetTimeZoneInformation(
            (LPTIME_ZONE_INFORMATION)&s_timezone_information);
        if (timezone_result != TIME_ZONE_ID_INVALID) {
            s_timezone_from_windows = 1;
            s_timezone_bias_seconds = s_timezone_information.bias * 60;
            if (s_timezone_information.standard_date.month != 0) {
                s_timezone_bias_seconds +=
                    s_timezone_information.standard_bias * 60;
            }
            if (s_timezone_information.daylight_date.month == 0 ||
                s_timezone_information.daylight_bias == 0) {
                s_daylight_saving_available = 0;
                s_daylight_saving_bias_seconds = 0;
            } else {
                s_daylight_saving_available = 1;
                s_daylight_saving_bias_seconds =
                    (s_timezone_information.daylight_bias -
                     s_timezone_information.standard_bias) * 60;
            }
            crt_wide_char_to_multi_byte_compat(
                s_standard_timezone_name,
                s_timezone_information.standard_name, 0x40u);
            crt_wide_char_to_multi_byte_compat(
                s_daylight_timezone_name,
                s_timezone_information.daylight_name, 0x40u);
            s_standard_timezone_name[63] = '\0';
            s_daylight_timezone_name[63] = '\0';
        }
#endif
        return;
    }
    if (*timezone_text == '\0') {
        return;
    }

    if (s_timezone_environment_copy != NULL &&
        strcmp(timezone_text, s_timezone_environment_copy) == 0) {
        return;
    }
    engine_heap_free(s_timezone_environment_copy);
    s_timezone_environment_copy = (char*)engine_heap_alloc(
        (long)strlen(timezone_text) + 1L);
    if (s_timezone_environment_copy == NULL) {
        return;
    }
    strcpy(s_timezone_environment_copy, timezone_text);

    crt_string_copy_n(s_standard_timezone_name, timezone_text, 3u);
    s_standard_timezone_name[3] = '\0';
    timezone_text += 3;
    /* Corpus 0x499a3b and 0x499a70..0x499ae2 use '-' and ASCII 0..9 gates. */
    negative_offset = 0;
    if (*timezone_text == '-') {
        negative_offset = 1;
        ++timezone_text;
    }
    s_timezone_bias_seconds = crt_parse_signed_decimal(
        (const unsigned char*)timezone_text) * 3600;
    while ((*timezone_text == '+' ||
            (*timezone_text >= '0' && *timezone_text <= '9'))) {
        ++timezone_text;
    }
    if (*timezone_text == ':') {
        ++timezone_text;
        s_timezone_bias_seconds += crt_parse_signed_decimal(
            (const unsigned char*)timezone_text) * 60;
        while (*timezone_text >= '0' && *timezone_text <= '9') {
            ++timezone_text;
        }
        if (*timezone_text == ':') {
            ++timezone_text;
            s_timezone_bias_seconds += crt_parse_signed_decimal(
                (const unsigned char*)timezone_text);
            while (*timezone_text >= '0' && *timezone_text <= '9') {
                ++timezone_text;
            }
        }
    }
    if (negative_offset != 0) {
        s_timezone_bias_seconds = -s_timezone_bias_seconds;
    }

    s_daylight_saving_available = (int)(signed char)*timezone_text;
    if (s_daylight_saving_available != 0) {
        crt_string_copy_n(s_daylight_timezone_name, timezone_text, 3u);
        s_daylight_timezone_name[3] = '\0';
    } else {
        s_daylight_timezone_name[0] = '\0';
    }
}

/* CrtDaylightSavingTimeCheck (FUN_00499b30): compare a CRT tm-like field
 * vector against cached start/end transition day and time boundaries. */
// FUNCTION: LEGORR 0x00499b30
int crt_daylight_saving_time_check(const int* calendar_fields)
{
    uint32_t year;
    int32_t day_of_year;
    int32_t time_of_day;

    if (s_daylight_saving_available == 0) {
        return 0;
    }
    year = (uint32_t)calendar_fields[5];
    if (year != s_daylight_start_year || year != s_daylight_end_year) {
        if (s_timezone_from_windows == 0) {
            crt_daylight_transition_boundary_compute(
                1, 1, year, 4, 1, 0, 0, 2, 0, 0, 0);
        } else {
            CrtSystemTime* daylight_date =
                &s_timezone_information.daylight_date;
            int use_nth_weekday;
            int weekday;
            int week;
            int fixed_day;

            use_nth_weekday = daylight_date->year == 0;
            weekday = use_nth_weekday ? daylight_date->day_of_week : 0;
            week = use_nth_weekday ? daylight_date->day : 0;
            fixed_day = use_nth_weekday ? 0 : daylight_date->day;
            crt_daylight_transition_boundary_compute(
                1, use_nth_weekday, year, daylight_date->month,
                week, weekday, fixed_day, daylight_date->hour,
                daylight_date->minute, daylight_date->second,
                daylight_date->milliseconds);
        }

        if (s_timezone_from_windows == 0) {
            crt_daylight_transition_boundary_compute(
                0, 1, year, 10, 5, 0, 0, 2, 0, 0, 0);
        } else {
            CrtSystemTime* standard_date =
                &s_timezone_information.standard_date;
            int use_nth_weekday;
            int weekday;
            int week;
            int fixed_day;

            use_nth_weekday = standard_date->year == 0;
            weekday = use_nth_weekday ? standard_date->day_of_week : 0;
            week = use_nth_weekday ? standard_date->day : 0;
            fixed_day = use_nth_weekday ? 0 : standard_date->day;
            crt_daylight_transition_boundary_compute(
                0, use_nth_weekday, year, standard_date->month,
                week, weekday, fixed_day, standard_date->hour,
                standard_date->minute, standard_date->second,
                standard_date->milliseconds);
        }
    }

    day_of_year = calendar_fields[7];
    if (s_daylight_start_day < s_daylight_end_day) {
        if (day_of_year < s_daylight_start_day ||
            s_daylight_end_day < day_of_year) {
            return 0;
        }
        if (s_daylight_start_day < day_of_year &&
            day_of_year < s_daylight_end_day) {
            return 1;
        }
    } else {
        if (day_of_year < s_daylight_end_day ||
            s_daylight_start_day < day_of_year) {
            return 1;
        }
        if (s_daylight_end_day < day_of_year &&
            day_of_year < s_daylight_start_day) {
            return 0;
        }
    }

    time_of_day = (calendar_fields[0] +
        (calendar_fields[1] + calendar_fields[2] * 60) * 60) * 1000;
    if (day_of_year != s_daylight_start_day) {
        return time_of_day < s_daylight_end_milliseconds;
    }
    return s_daylight_start_milliseconds <= time_of_day;
}

/* ExtendedFloatPowerOfTenScale (FUN_0049a800): consume three exponent bits
 * per table row, multiplying the 80-bit value by that row's power of ten. */
// FUNCTION: LEGORR 0x0049a800
void crt_extended_float_power_of_ten_scale(uint16_t value[6],
                                           uint32_t exponent,
                                           int initialize)
{
    const uint16_t (*table)[6] = s_positive_power_of_ten_table;
    uint32_t group;

    if (exponent == 0u) {
        return;
    }
    if ((int32_t)exponent < 0) {
        exponent = 0u - exponent;
        table = s_negative_power_of_ten_table;
    }
    if (initialize == 0) {
        value[0] = 0u;
    }

    group = 0u;
    while (exponent != 0u) {
        const uint32_t digit = exponent & 7u;
        exponent >>= 3;
        if (digit != 0u && group < 4u) {
            const uint16_t* factor = table[group * 7u + digit - 1u];
            uint16_t adjusted[6];

            if (factor[0] > 0x7fffu) {
                uint32_t middle;
                uint32_t i;
                for (i = 0u; i < 6u; ++i) {
                    adjusted[i] = factor[i];
                }
                middle = (uint32_t)adjusted[1] |
                         ((uint32_t)adjusted[2] << 16);
                --middle;
                adjusted[1] = (uint16_t)middle;
                adjusted[2] = (uint16_t)(middle >> 16);
                factor = adjusted;
            }
            crt_extended80_multiply(value, factor);
        }
        ++group;
    }
}

/* CrtCodePageGet (FUN_004964e0): select the OEM/ANSI code page or update
 * the fallback-mode flag before returning an explicit/current page. */
// FUNCTION: LEGORR 0x004964e0
int crt_codepage_get(int codepage)
{
    /* Corpus 0x4964e4 clears fallback mode before testing the selector. */
    s_codepage_fallback_mode = 0u;
    if (codepage == -2) {
        s_codepage_fallback_mode = 1u;
#if defined(LEGORRR_BACKEND_WINAPI)
        return (int)GetOEMCP();
#else
        return 0;
#endif
    }
    if (codepage == -3) {
        s_codepage_fallback_mode = 1u;
#if defined(LEGORRR_BACKEND_WINAPI)
        return (int)GetACP();
#else
        return 0;
#endif
    }
    if (codepage == -4) {
        /* Corpus 0x49651d loads the page before setting fallback mode. */
        const int ansi_codepage = (int)s_ansi_codepage;
        s_codepage_fallback_mode = 1u;
        return ansi_codepage;
    }
    return codepage;
}

/* CrtCtypeTablesReset (FUN_00496590): reset only the lead-byte flag table
 * and the active code-page conversion metadata. The separate _pctype table
 * and the GetACP/GetOEMCP fallback flag are left as the image leaves them. */
// FUNCTION: LEGORR 0x00496590
void crt_ctype_tables_reset(void)
{
    memset(s_multibyte_character_flags, 0, sizeof s_multibyte_character_flags);
    s_active_codepage = 0u;
    s_active_locale_table = 0u;
    s_codepage_metadata_word0 = 0u;
    s_codepage_metadata_word1 = 0u;
    s_codepage_metadata_word2 = 0u;
}

/* CrtLocaleCodePageTableUpdate (FUN_004962f0): load one of the image's
 * five byte-range tables, or ask Windows for the current page's lead bytes. */
// FUNCTION: LEGORR 0x004962f0
int crt_locale_codepage_table_update(int requested_codepage)
{
    const uint32_t codepage = (uint32_t)crt_codepage_get(requested_codepage);
    unsigned record_index;

    if (codepage == s_active_codepage) {
        return 0;
    }
    if (codepage == 0u) {
        crt_ctype_tables_reset();
        return 0;
    }

    for (record_index = 0u; record_index < 5u; ++record_index) {
        const uint8_t* record = s_predefined_codepage_records[record_index];
        uint32_t record_codepage;
        memcpy(&record_codepage, record, sizeof record_codepage);
        if (record_codepage == codepage) {
            unsigned group;
            memset(s_multibyte_character_flags, 0,
                   sizeof s_multibyte_character_flags);
            for (group = 0u; group < 4u; ++group) {
                const uint8_t* ranges = record + 16u + group * 8u;
                unsigned pair;
                for (pair = 0u; pair < 4u; ++pair) {
                    const uint8_t first = ranges[pair * 2u];
                    const uint8_t last = ranges[pair * 2u + 1u];
                    unsigned value;
                    if (first == 0u || last == 0u) {
                        break;
                    }
                    for (value = first; value <= (unsigned)last; ++value) {
                        s_multibyte_character_flags[value + 1u] |=
                            s_predefined_codepage_masks[group];
                    }
                }
            }
            s_active_codepage = codepage;
            s_active_locale_table = crt_codepage_locale_id_lookup(codepage);
            memcpy(&s_codepage_metadata_word0, record + 4u, 4u);
            memcpy(&s_codepage_metadata_word1, record + 8u, 4u);
            memcpy(&s_codepage_metadata_word2, record + 12u, 4u);
            return 0;
        }
    }

#if defined(LEGORRR_BACKEND_WINAPI)
    {
        CPINFO info;
        if (GetCPInfo((UINT)codepage, &info) == 0) {
            if (s_codepage_fallback_mode == 0u) {
                return -1;
            }
            crt_ctype_tables_reset();
            return 0;
        }
        memset(s_multibyte_character_flags, 0,
               sizeof s_multibyte_character_flags);
        /* Corpus 0x496370 uses the unsigned <= 1 width gate. */
        if (info.MaxCharSize <= 1u) {
            s_active_codepage = 0u;
            s_active_locale_table = 0u;
        }
        else {
            unsigned byte_index;
            unsigned range_index;
            for (range_index = 0u; range_index + 1u < 12u;
                 range_index += 2u) {
                const unsigned first = info.LeadByte[range_index];
                const unsigned last = info.LeadByte[range_index + 1u];
                if (first == 0u || last == 0u) {
                    break;
                }
                for (byte_index = first; byte_index <= last; ++byte_index) {
                    s_multibyte_character_flags[byte_index + 1u] |= 4u;
                }
            }
            for (byte_index = 1u; byte_index < 0xffu; ++byte_index) {
                s_multibyte_character_flags[byte_index + 1u] |= 8u;
            }
            s_active_codepage = codepage;
            s_active_locale_table =
                crt_codepage_locale_id_lookup(codepage);
        }
        s_codepage_metadata_word0 = 0u;
        s_codepage_metadata_word1 = 0u;
        s_codepage_metadata_word2 = 0u;
        return 0;
    }
#else
    if (s_codepage_fallback_mode == 0u) {
        return -1;
    }
    crt_ctype_tables_reset();
    return 0;
#endif
}

/* CrtAnsiCodePageTablesInit (FUN_004965c0): select the system ANSI page. */
// FUNCTION: LEGORR 0x004965c0
void crt_ansi_codepage_tables_init(void)
{
    (void)crt_locale_codepage_table_update(-3);
}

/* CharacterClassifyMask (FUN_0048f1d0): use _pctype for byte/EOF values;
 * wider values are classified by the active Windows locale when available. */
// FUNCTION: LEGORR 0x0048f1d0
uint32_t crt_character_classify_mask(int value, uint32_t mask)
{
    uint32_t flags;
    unsigned char bytes[4];
    unsigned count;
    uint32_t wide_flags;

    /* Corpus 0x48f1d6..0x48f1de uses the inclusive unsigned 0x100 gate. */
    if (((uint32_t)value + 1u) <= 0x100u) {
        if (value == -1) {
            return 0x20u & mask;
        }
        return (uint32_t)(s_ctype_pointer[value] & mask);
    }

    /* Corpus 0x48f1fb..0x48f231 tests the signed high byte and presents a
     * DBCS pair high-byte first to CrtGetStringTypeACompat. */
    if (((s_ctype_pointer[(uint32_t)(value >> 8) & 0xffu] >> 8) & 0x80u)
        != 0u) {
        bytes[0] = (unsigned char)(value >> 8);
        bytes[1] = (unsigned char)value;
        bytes[2] = 0u;
        count = 2u;
    }
    else {
        bytes[0] = (unsigned char)value;
        bytes[1] = 0u;
        count = 1u;
    }
    flags = crt_get_string_type_a_compat(1u, (const char*)bytes,
                                         (int)count, (uint16_t*)&wide_flags,
                                         0u, 0u);
    if (flags == 0u) {
        return 0u;
    }
    return wide_flags & 0xffffu & mask;
}

/* CrtGetStringTypeACompat (FUN_00496ad0): prefer the ANSI API, falling back
 * to its Unicode form after converting through the requested code page. */
// FUNCTION: LEGORR 0x00496ad0
int crt_get_string_type_a_compat(uint32_t info_type, const char* source,
                                 int source_count, uint16_t* result,
                                 uint32_t codepage, uint32_t locale)
{
#if defined(LEGORRR_BACKEND_WINAPI)
    if (s_string_type_api_mode == 0) {
        WORD probe;
        if (GetStringTypeA(0, 1u, "", 1, &probe) != 0) {
            s_string_type_api_mode = 2;
        }
        else if (GetStringTypeW(1u, L"", 1, &probe) != 0) {
            s_string_type_api_mode = 1;
        }
        else {
            return 0;
        }
    }
    if (s_string_type_api_mode == 2) {
        if (locale == 0u) {
            locale = s_crt_locale_handle;
        }
        return GetStringTypeA((LCID)locale, info_type, source,
                              source_count, (LPWORD)result);
    }
    if (s_string_type_api_mode == 1) {
        int wide_count;
        WCHAR* wide_source = NULL;
        BOOL success;
        if (codepage == 0u) {
            codepage = s_ansi_codepage;
        }
        wide_count = MultiByteToWideChar((UINT)codepage, 9u, source,
                                         source_count, NULL, 0);
        if (wide_count != 0) {
            wide_source = (WCHAR*)crt_calloc_aligned(2, wide_count);
            if (wide_source != NULL) {
                wide_count = MultiByteToWideChar((UINT)codepage, 1u, source,
                                                 source_count, wide_source,
                                                 wide_count);
                if (wide_count != 0) {
                    success = GetStringTypeW(info_type, wide_source,
                                             wide_count, (LPWORD)result);
                    engine_heap_free(wide_source);
                    return success;
                }
            }
        }
        engine_heap_free(wide_source);
        return 0;
    }
    return s_string_type_api_mode;
#else
    (void)info_type;
    (void)source;
    (void)source_count;
    (void)result;
    (void)codepage;
    (void)locale;
    return 0;
#endif
}

/* CrtMultibyteToWideChar (FUN_00498e00): convert one CRT character, retaining
 * the original error and incomplete-sequence behavior. */
// FUNCTION: LEGORR 0x00498e00
uint32_t crt_multibyte_to_wide_char(uint16_t* output, const uint8_t* source,
                                    uint32_t max_bytes)
{
    uint8_t first;
    int converted;
    if (source == NULL || max_bytes == 0u) {
        return 0u;
    }
    first = *source;
    if (first == 0u) {
        if (output != NULL) {
            *output = 0u;
            return 0u;
        }
        return 0u;
    }
    if (s_crt_locale_handle == 0u) {
        if (output != NULL) {
            *output = first;
        }
        return 1u;
    }
    /* Corpus 0x498e5b..0x498e79 handles lead bytes first and uses signed
     * comparisons for the API width gate. */
    if ((s_ctype_pointer[(unsigned)first] & 0x8000u) != 0u) {
        if ((int)s_max_multibyte_width > 1 &&
            (int)s_max_multibyte_width <= (int)max_bytes) {
#if defined(LEGORRR_BACKEND_WINAPI)
            converted = MultiByteToWideChar(s_ansi_codepage, 9u,
                                            (const char*)source,
                                            (int)s_max_multibyte_width,
                                            (LPWSTR)output,
                                            output != NULL ? 1 : 0);
            if (converted != 0) {
                return s_max_multibyte_width;
            }
#endif
        }
        if (max_bytes < s_max_multibyte_width) {
            s_math_error_code = 0x2au;
            return 0xffffffffu;
        }
        if (source[1] != 0u) {
            return s_max_multibyte_width;
        }
        s_math_error_code = 0x2au;
        return 0xffffffffu;
    }
#if defined(LEGORRR_BACKEND_WINAPI)
    converted = MultiByteToWideChar(s_ansi_codepage, 9u,
                                    (const char*)source, 1,
                                    (LPWSTR)output,
                                    output != NULL ? 1 : 0);
    if (converted != 0) {
        return 1u;
    }
#endif
    s_math_error_code = 0x2au;
    return 0xffffffffu;
}

/* CrtCharacterClassFlag8Check (FUN_00498f00): read the CRT space bit for
 * single-byte locales and delegate full character classification for DBCS. */
// FUNCTION: LEGORR 0x00498f00
uint32_t crt_character_class_flag8_check(int value)
{
    /* Corpus 0x498f04..0x498f29 uses a signed <= 1 gate, then indexes the
     * single-byte table directly with the supplied value. */
    if ((int)s_max_multibyte_width > 1) {
        return crt_character_classify_mask(value, 8u);
    }
    return (uint32_t)(s_ctype_pointer[value] & 8u);
}

/* CrtScanFormatCharNormalize (FUN_00494c30) — see crt.hpp. The image's
 * classification table marks decimal digits with bit 4; non-digits take
 * the original ASCII hex-letter fold. */
// FUNCTION: LEGORR 0x00494c30
uint32_t crt_scan_format_char_normalize(uint32_t value)
{
    uint32_t digit;
    if (s_max_multibyte_width < 2u) {
        digit = (value < 256u) ? (s_ctype_pointer[value] & 4u) : 0u;
    }
    else {
        digit = crt_character_class_flag_test(value, 4u, 0u) ? 4u : 0u;
    }
    if (digit != 0u) {
        return value;
    }
    return (value & 0xffffffdfu) - 7u;
}

/* CrtCharacterClassFlagTest (FUN_00495db0): the first flag source is the
 * current multibyte table; if it does not match, test the ordinary CRT
 * classification word only when a word mask was requested. */
// FUNCTION: LEGORR 0x00495db0
uint32_t crt_character_class_flag_test(uint32_t value, uint32_t flags,
                                      uint8_t byte_mask)
{
    const uint8_t character = (uint8_t)value;
    if ((s_multibyte_character_flags[(unsigned)character + 1u]
         & byte_mask) != 0u) {
        return 1u;
    }
    if (flags != 0u
        && (s_ctype_pointer[character] & (uint16_t)flags) != 0u) {
        return 1u;
    }
    return 0u;
}

/* LocaleCharacterMapString (FUN_00493ca0): use the native ANSI mapper when
 * available, otherwise convert through UTF-16 and convert mapped text back. */
// FUNCTION: LEGORR 0x00493ca0
int crt_locale_character_map_string(uint32_t locale, uint32_t flags,
                                     const char* source, int source_count,
                                     void* destination,
                                     int destination_count,
                                     uint32_t codepage)
{
#if defined(LEGORRR_BACKEND_WINAPI)
    int input_count = source_count;
    if (s_locale_map_api_mode == 0) {
        if (LCMapStringA(0, 0x100u, "", 1, NULL, 0) != 0) {
            s_locale_map_api_mode = 2;
        }
        else if (LCMapStringW(0, 0x100u, L"", 1, NULL, 0) != 0) {
            s_locale_map_api_mode = 1;
        }
        else {
            return 0;
        }
    }
    if (input_count > 0) {
        int length = 0;
        while (length < input_count && source[length] != '\0') {
            ++length;
        }
        input_count = length;
    }
    if (s_locale_map_api_mode == 2) {
        return LCMapStringA((LCID)locale, flags, source, input_count,
                            (LPSTR)destination, destination_count);
    }
    if (s_locale_map_api_mode == 1) {
        WCHAR* wide_source;
        WCHAR* wide_mapped = NULL;
        int wide_count;
        int mapped_count;
        int result;
        if (codepage == 0u) {
            codepage = s_ansi_codepage;
        }
        wide_count = MultiByteToWideChar((UINT)codepage, 9u, source,
                                         input_count, NULL, 0);
        if (wide_count == 0) {
            return 0;
        }
        wide_source = (WCHAR*)engine_heap_alloc((long)wide_count * 2L);
        if (wide_source == NULL) {
            return 0;
        }
        if (MultiByteToWideChar((UINT)codepage, 1u, source, input_count,
                                wide_source, wide_count) == 0) {
            engine_heap_free(wide_source);
            return 0;
        }
        mapped_count = LCMapStringW((LCID)locale, flags, wide_source,
                                    wide_count, NULL, 0);
        if (mapped_count == 0) {
            engine_heap_free(wide_source);
            return 0;
        }
        if ((flags & 0x400u) != 0u) {
            if (destination_count == 0 || destination_count < mapped_count) {
                engine_heap_free(wide_source);
                return 0;
            }
            result = LCMapStringW((LCID)locale, flags, wide_source,
                                  wide_count, (LPWSTR)destination,
                                  destination_count);
            engine_heap_free(wide_source);
            return result;
        }
        wide_mapped = (WCHAR*)engine_heap_alloc((long)mapped_count * 2L);
        if (wide_mapped == NULL) {
            engine_heap_free(wide_source);
            return 0;
        }
        if (LCMapStringW((LCID)locale, flags, wide_source, wide_count,
                         wide_mapped, mapped_count) == 0) {
            engine_heap_free(wide_source);
            engine_heap_free(wide_mapped);
            return 0;
        }
        result = WideCharToMultiByte((UINT)codepage, 0x220u, wide_mapped,
                                     mapped_count, (LPSTR)destination,
                                     destination_count, NULL, NULL);
        engine_heap_free(wide_source);
        engine_heap_free(wide_mapped);
        return result;
    }
    return s_locale_map_api_mode;
#else
    (void)locale;
    (void)flags;
    (void)source;
    (void)source_count;
    (void)destination;
    (void)destination_count;
    (void)codepage;
    return 0;
#endif
}

/* CrtCharacterToLowerLocale (FUN_004975d0): apply the CRT locale's lower
 * mapping while preserving DBCS characters as their high-byte-first value. */
// FUNCTION: LEGORR 0x004975d0
uint32_t crt_character_to_lower_locale(uint32_t value)
{
    uint32_t input_value;
    uint32_t mapped[2];
    unsigned count;
    if (s_crt_locale_handle == 0u) {
        if (value > (uint32_t)'A' && value < (uint32_t)'Z' + 1u) {
            return value + 0x20u;
        }
        return value;
    }
    if (value < 0x100u) {
        if (s_max_multibyte_width < 2u) {
            if ((s_ctype_pointer[value] & 1u) == 0u) {
                return value;
            }
        }
        else if (crt_character_classify_mask((int)value, 1u) == 0u) {
            return value;
        }
    }
    if ((s_ctype_pointer[(value >> 8) & 0xffu] & 0x8000u) != 0u) {
        input_value = ((value & 0xffu) << 8) | ((value >> 8) & 0xffu);
        count = 2u;
    }
    else {
        input_value = value & 0xffu;
        count = 1u;
    }
    if (crt_locale_character_map_string(s_crt_locale_handle, 0x100u,
                                        (const char*)&input_value,
                                        (int)count,
                                        mapped, 3, 0u) == 0) {
        return value;
    }
    if (count == 1u) {
        return mapped[0] & 0xffu;
    }
    return ((mapped[0] >> 8) & 0xffu) << 8 | (mapped[0] & 0xffu);
}

// FUNCTION: LEGORR 0x00495d90
uint32_t crt_is_dbcs_lead_byte(uint32_t value)
{
    return crt_character_class_flag_test(value, 0u, 4u);
}

// FUNCTION: LEGORR 0x0048dad0
int crt_parse_signed_decimal(const unsigned char* text)
{
    uint32_t value;
    uint32_t character;
    uint32_t class_flags;
    int sign_character;

    /* Corpus 0x48dad8/0x48db25 use signed <= 1 gates; 0x48db13 retains
     * the original sign byte so the final '-' test needs no boolean flag. */
    while (1) {
        if ((int)s_max_multibyte_width > 1) {
            class_flags = crt_character_classify_mask((int)*text, 8u);
        }
        else {
            class_flags = s_ctype_pointer[*text] & 8u;
        }
        if (class_flags == 0u) {
            break;
        }
        ++text;
    }
    character = *text++;
    sign_character = (int)character;
    if (character == (uint32_t)'-' || character == (uint32_t)'+') {
        character = *text++;
    }
    value = 0u;
    while (1) {
        if ((int)s_max_multibyte_width > 1) {
            class_flags = crt_character_classify_mask((int)character, 4u);
        }
        else {
            class_flags = s_ctype_pointer[character] & 4u;
        }
        if (class_flags == 0u) {
            break;
        }
        value = (character - (uint32_t)'0') + value * 10u;
        character = *text++;
    }
    if (sign_character == '-') {
        value = 0u - value;
    }
    return (int32_t)value;
}

// FUNCTION: LEGORR 0x004902b0
void crt_normalize_decimal_point(char* text)
{
    uint32_t first;
    int current;
    char displaced;

    first = crt_character_to_lower_locale((int)*text);
    if (first != (uint32_t)'e') {
        if ((int)s_max_multibyte_width > 1) {
            do {
                ++text;
            } while (crt_character_classify_mask((int)*text, 4u) != 0u);
        }
        else {
            do {
                ++text;
            } while ((s_ctype_pointer[(int)(signed char)(*text)] & 4u) != 0u);
        }
    }
    displaced = *text;
    *text = s_decimal_separator;
    do {
        ++text;
        current = (int)*text;
        *text = displaced;
        displaced = (char)current;
    } while (*text != '\0');
}

// FUNCTION: LEGORR 0x004914a0
char* crt_strchr(char* text, int character)
{
    const uint32_t wanted = (uint8_t)character;
    const uint32_t repeated = wanted * 0x01010101u;

    while (((uintptr_t)text & 3u) != 0u) {
        const uint8_t byte = (uint8_t)*text;
        if (byte == (uint8_t)wanted) {
            return text;
        }
        if (byte == 0u) {
            return NULL;
        }
        ++text;
    }

    for (;;) {
        uint32_t word;
        uint32_t candidate;
        uint32_t zero_candidate;
        uint32_t sum;

        memcpy(&word, text, sizeof(word));
        /* Keep the target's 32-bit wraparound probes; byte checks below
         * resolve the candidate lanes in address order. */
        candidate = word ^ repeated;
        sum = candidate + 0x7efefeffu;
        candidate = (candidate ^ 0xffffffffu ^ sum) & 0x81010100u;
        if (candidate == 0u) {
            sum = word + 0x7efefeffu;
            zero_candidate =
                (word ^ 0xffffffffu ^ sum) & 0x81010100u;
            if (zero_candidate != 0u) {
                if ((zero_candidate & 0x01010100u) != 0u ||
                    (sum & 0x80000000u) == 0u) {
                    return NULL;
                }
            }
            text += 4;
            continue;
        }

        if ((uint8_t)word == (uint8_t)wanted) {
            return text;
        }
        if ((uint8_t)word == 0u) {
            return NULL;
        }
        if ((uint8_t)(word >> 8) == (uint8_t)wanted) {
            return text + 1;
        }
        if ((uint8_t)(word >> 8) == 0u) {
            return NULL;
        }
        if ((uint8_t)(word >> 16) == (uint8_t)wanted) {
            return text + 2;
        }
        if ((uint8_t)(word >> 16) == 0u) {
            return NULL;
        }
        if ((uint8_t)(word >> 24) == (uint8_t)wanted) {
            return text + 3;
        }
        if ((uint8_t)(word >> 24) == 0u) {
            return NULL;
        }
        text += 4;
    }
}

// FUNCTION: LEGORR 0x004976d0
char* crt_dbcs_aware_bounded_string_copy(char* dest, const char* src,
                                         uint32_t count)
{
    char* cursor;
    uint32_t remaining;

    if (s_active_codepage == 0u) {
        return strncpy(dest, src, count);
    }
    cursor = dest;
    remaining = count;
    while (remaining != 0u) {
        unsigned char first;
        first = (unsigned char)*src;
        *cursor = (char)first;
        if ((s_multibyte_character_flags[(uint32_t)first + 1u] & 4u) == 0u) {
            ++cursor;
            ++src;
            --remaining;
            if (first == 0u) {
                break;
            }
        }
        else {
            if (remaining == 1u) {
                *cursor = '\0';
                remaining = 0u;
                break;
            }
            cursor[1] = src[1];
            cursor += 2;
            src += 2;
            remaining -= 2u;
            if (cursor[-1] == '\0') {
                cursor[-2] = '\0';
                break;
            }
        }
    }
    while (remaining != 0u) {
        *cursor++ = '\0';
        --remaining;
    }
    return dest;
}

/* CrtMultibyteCharacterSearch (FUN_0049b5b0): when no multibyte code page
 * is active, this is the CRT's byte strchr path. Otherwise step over lead
 * and trail bytes together and match the original high-byte-first pair. */
// FUNCTION: LEGORR 0x0049b5b0
char* crt_multibyte_character_search(char* text, uint32_t value)
{
    uint8_t* cursor = (uint8_t*)text;
    uint32_t character;

    if (s_active_codepage == 0u) {
        return (char*)strchr(text, (int)value);
    }
    character = *cursor;
    if (character == 0u) {
        return (value == 0u) ? (char*)cursor : NULL;
    }
    for (;;) {
        uint8_t* last_byte;
        if ((s_multibyte_character_flags[character + 1u] & 4u) == 0u) {
            last_byte = cursor;
            if (value == character) {
                return (char*)cursor;
            }
        }
        else {
            last_byte = cursor + 1;
            if (cursor[1] == 0u) {
                return NULL;
            }
            if (value == ((character << 8) | cursor[1])) {
                return (char*)cursor;
            }
        }
        character = last_byte[1];
        cursor = last_byte + 1;
        if (character == 0u) {
            return (value == 0u) ? (char*)cursor : NULL;
        }
    }
}

/* CrtEnvironmentArrayClone (FUN_0049b540): allocate one extra pointer for
 * the terminator, then duplicate each string in order. The port routes the
 * corpus's __amsg_exit(9) allocation failure through its fatal-exit seam. */
// FUNCTION: LEGORR 0x0049b540
char** crt_environment_array_clone(char** entries)
{
    int count = 0;
    int i;
    char** copy;

    if (entries == NULL) {
        return NULL;
    }
    while (entries[count] != NULL) {
        ++count;
    }
    copy = (char**)engine_heap_alloc((long)((count + 1) * sizeof(char*)));
    if (copy == NULL) {
        crt_fatal_message_exit();
        return NULL;
    }
    for (i = 0; i < count; ++i) {
        copy[i] = crt_string_duplicate(entries[i]);
    }
    copy[count] = NULL;
    return copy;
}

// FUNCTION: LEGORR 0x00496190
char* crt_environment_block_get(void)
{
#if defined(LEGORRR_BACKEND_WINAPI)
    WCHAR* wide_block = NULL;
    char* narrow_block = NULL;
    if (s_environment_api_mode == 0) {
        wide_block = GetEnvironmentStringsW();
        if (wide_block != NULL) s_environment_api_mode = 1;
        else {
            narrow_block = GetEnvironmentStringsA();
            if (narrow_block == NULL) return NULL;
            s_environment_api_mode = 2;
        }
    }
    if (s_environment_api_mode == 1) {
        int count = 0;
        int converted;
        WCHAR* end;
        if (wide_block == NULL) wide_block = GetEnvironmentStringsW();
        if (wide_block == NULL) return NULL;
        end = wide_block;
        while (*end != 0) {
            while (*end != 0) ++end;
            ++end;
        }
        count = (int)(end - wide_block) + 1;
        converted = WideCharToMultiByte(CP_ACP, 0, wide_block, count,
                                        NULL, 0, NULL, NULL);
        if (converted > 0) {
            narrow_block = (char*)engine_heap_alloc(converted);
            if (narrow_block != NULL &&
                WideCharToMultiByte(CP_ACP, 0, wide_block, count, narrow_block,
                                    converted, NULL, NULL) == 0) {
                engine_heap_free(narrow_block);
                narrow_block = NULL;
            }
        }
        FreeEnvironmentStringsW(wide_block);
        return narrow_block;
    }
    if (s_environment_api_mode == 2) {
        int count = 0;
        int i;
        char* end;
        if (narrow_block == NULL) narrow_block = GetEnvironmentStringsA();
        if (narrow_block == NULL) return NULL;
        end = narrow_block;
        while (*end != '\0') {
            while (*end != '\0') ++end;
            ++end;
        }
        count = (int)(end - narrow_block) + 1;
        {
            char* result = (char*)engine_heap_alloc(count);
            if (result != NULL) {
                for (i = 0; i < count; ++i) result[i] = narrow_block[i];
            }
            FreeEnvironmentStringsA(narrow_block);
            return result;
        }
    }
#else
    char** entry;
    long count = 1;
    char* block;
    char* output;
    if (s_environment_api_mode == 0) s_environment_api_mode = 2;
    if (environ == NULL) return NULL;
    for (entry = environ; *entry != NULL; ++entry) count += (long)strlen(*entry) + 1;
    block = (char*)engine_heap_alloc(count + 1);
    if (block == NULL) return NULL;
    output = block;
    for (entry = environ; *entry != NULL; ++entry) {
        const char* source = *entry;
        while (*source != '\0') *output++ = *source++;
        *output++ = '\0';
    }
    *output = '\0';
    return block;
#endif
    return NULL;
}

// FUNCTION: LEGORR 0x00495df0
void crt_environment_pointer_array_build(void)
{
    char* cursor;
    int count = 0;
    int length;
    char** entries;
    char** entry_out;
    if (s_environment_block == NULL) return;
    cursor = s_environment_block;
    while (*cursor != '\0') {
        if (*cursor != '=') ++count;
        length = (int)strlen(cursor) + 1;
        cursor += length;
    }
    entries = (char**)engine_heap_alloc((long)(count + 1) * (long)sizeof(char*));
    s_environment = entries;
    if (entries == NULL) {
        crt_fatal_message_exit();
        return;
    }
    entry_out = entries;
    cursor = s_environment_block;
    while (*cursor != '\0') {
        length = (int)strlen(cursor) + 1;
        if (*cursor != '=') {
            *entry_out = crt_string_duplicate(cursor);
            if (*entry_out == NULL) {
                crt_fatal_message_exit();
                return;
            }
            ++entry_out;
        }
        cursor += length;
    }
    *entry_out = NULL;
    engine_heap_free(s_environment_block);
    s_environment_block = NULL;
    s_environment = entries;
}

// FUNCTION: LEGORR 0x0049afe0
int crt_compare_string_compat(uint32_t locale, uint32_t flags,
                              const char* left, int left_count,
                              const char* right, int right_count,
                              uint32_t codepage)
{
    int left_length = left_count;
    int right_length = right_count;
    int i;
    if (left_count > 0) {
        for (left_length = 0; left_length < left_count && left[left_length] != '\0'; ++left_length) {}
    }
    if (right_count > 0) {
        for (right_length = 0; right_length < right_count && right[right_length] != '\0'; ++right_length) {}
    }
#if defined(LEGORRR_BACKEND_WINAPI)
    if (s_compare_string_api_mode == 0) {
        if (CompareStringA(0, 0, "", 1, "", 1) != 0) s_compare_string_api_mode = 2;
        else if (CompareStringW(0, 0, L"", 1, L"", 1) != 0) s_compare_string_api_mode = 1;
        else return 0;
    }
    if (s_compare_string_api_mode == 2) {
        return CompareStringA((LCID)locale, (DWORD)flags, left, left_length,
                              right, right_length);
    }
    if (s_compare_string_api_mode == 1) {
        WCHAR* wide_left;
        WCHAR* wide_right;
        int wide_left_count;
        int wide_right_count;
        int result;
        if (codepage == 0) codepage = s_ansi_codepage;
        if (left_length == 0 || right_length == 0) {
            CPINFO info;
            const char* candidate;
            int candidate_length;
            int order;
            if (left_length == right_length) return 2;
            if (right_length > 1) return 1;
            if (left_length > 1) return 3;
            if (!GetCPInfo((UINT)codepage, &info)) return 0;
            candidate = left_length > 0 ? left : right;
            candidate_length = left_length > 0 ? left_length : right_length;
            order = left_length > 0 ? 3 : 1;
            if (info.MaxCharSize < 2 || candidate_length <= 0) return order;
            for (i = 0; i + 1 < (int)sizeof(info.LeadByte); i += 2) {
                if (info.LeadByte[i] == 0 || info.LeadByte[i + 1] == 0) break;
                if (info.LeadByte[i] <= (BYTE)candidate[0] &&
                    (BYTE)candidate[0] <= info.LeadByte[i + 1]) return 2;
            }
            return order;
        }
        wide_left_count = MultiByteToWideChar((UINT)codepage, 9, left, left_length, NULL, 0);
        if (wide_left_count == 0) return 0;
        wide_left = (WCHAR*)engine_heap_alloc((long)wide_left_count * sizeof(WCHAR));
        if (wide_left == NULL) return 0;
        if (MultiByteToWideChar((UINT)codepage, 1, left, left_length,
                                wide_left, wide_left_count) == 0) {
            engine_heap_free(wide_left);
            return 0;
        }
        wide_right_count = MultiByteToWideChar((UINT)codepage, 9, right, right_length, NULL, 0);
        if (wide_right_count == 0) {
            engine_heap_free(wide_left);
            return 0;
        }
        wide_right = (WCHAR*)engine_heap_alloc((long)wide_right_count * sizeof(WCHAR));
        if (wide_right == NULL) {
            engine_heap_free(wide_left);
            return 0;
        }
        if (MultiByteToWideChar((UINT)codepage, 1, right, right_length,
                                wide_right, wide_right_count) == 0) result = 0;
        else result = CompareStringW((LCID)locale, (DWORD)flags, wide_left,
                                     wide_left_count, wide_right, wide_right_count);
        engine_heap_free(wide_left);
        engine_heap_free(wide_right);
        return result;
    }
#else
    (void)locale;
    if (left_length < 0) left_length = (int)strlen(left);
    if (right_length < 0) right_length = (int)strlen(right);
    if ((flags & 1u) != 0u) {
        int limit = left_length < right_length ? left_length : right_length;
        for (i = 0; i < limit; ++i) {
            int a = tolower((unsigned char)left[i]);
            int b = tolower((unsigned char)right[i]);
            if (a < b) return 1;
            if (a > b) return 3;
        }
        if (left_length < right_length) return 1;
        if (left_length > right_length) return 3;
        return 2;
    }
    i = left_length < right_length ? left_length : right_length;
    i = memcmp(left, right, (size_t)i);
    if (i < 0) return 1;
    if (i > 0) return 3;
    if (left_length < right_length) return 1;
    if (left_length > right_length) return 3;
    return 2;
#endif
    return 0;
}

// FUNCTION: LEGORR 0x0049af20
int crt_counted_string_compare(const char* left, const char* right, int count)
{
    int result;
    if (count == 0) return 0;
    result = crt_compare_string_compat(s_active_locale_table, 1u, left, count,
                                       right, count, s_active_codepage);
    if (result == 0) return 0x7fffffff;
    return result - 2;
}

// FUNCTION: LEGORR 0x0049b4c0
int crt_environment_entry_find(const char* name, int name_length)
{
    char** entry = s_environment;
    /* Corpus 0x49b4c9 dereferences the environment root without a NULL
     * check. The in-repo call path reaches this through crt_put_env_update
     * with sync_process == 0, which creates the array before searching; the
     * NULL-root behavior was not test-pinned. */
    if (*entry == NULL) return 0;
    while (*entry != NULL) {
        int comparison = crt_counted_string_compare(name, *entry, name_length);
        if (comparison == 0 && ((*entry)[name_length] == '=' || (*entry)[name_length] == '\0')) {
            return (int)(entry - s_environment);
        }
        ++entry;
    }
    /* Corpus 0x49b4fd computes the negative index from the end pointer. */
    return -(int)(entry - s_environment);
}

static char** crt_environment_array_resize(char** entries, int slot_count)
{
    int old_count = 0;
    int copy_count;
    int i;
    char** resized;
    if (entries != NULL) while (entries[old_count] != NULL) ++old_count;
    resized = (char**)engine_heap_alloc((long)slot_count * (long)sizeof(char*));
    if (resized == NULL) return NULL;
    copy_count = (entries == NULL) ? 0 : old_count + 1;
    if (copy_count > slot_count) copy_count = slot_count;
    for (i = 0; i < copy_count; ++i) resized[i] = entries[i];
    if (copy_count < slot_count) resized[copy_count] = NULL;
    if (entries != NULL) engine_heap_free(entries);
    return resized;
}

// FUNCTION: LEGORR 0x0049b2b0
int crt_put_env_update(char* text, int sync_process)
{
    char* equals;
    int deleting;
    int index;
    int name_length;
    char** entries;
    if (text == NULL) return -1;
    equals = crt_multibyte_character_search(text, '=');
    if (equals == NULL || equals == text) return -1;
    deleting = equals[1] == '\0';
    if (s_environment == s_environment_sentinel) s_environment = crt_environment_array_clone(s_environment);
    if (s_environment == NULL) {
        if (sync_process == 0 || s_wide_environment == NULL) {
            if (deleting) return 0;
            s_environment = (char**)engine_heap_alloc((long)sizeof(char*));
            if (s_environment == NULL) return -1;
            s_environment[0] = NULL;
            if (s_wide_environment == NULL) {
                s_wide_environment = (uint16_t**)engine_heap_alloc((long)sizeof(uint16_t*));
                if (s_wide_environment == NULL) return -1;
                s_wide_environment[0] = NULL;
            }
        }
        else if (crt_wide_environment_string_list_convert() != 0) return -1;
    }
    entries = s_environment;
    name_length = (int)(equals - text);
    index = crt_environment_entry_find(text, name_length);
    if (index < 0 || entries[0] == NULL) {
        if (deleting) return 0;
        if (index < 0) index = -index;
        entries = crt_environment_array_resize(entries, index + 2);
        if (entries == NULL) return -1;
        entries[index] = text;
        entries[index + 1] = NULL;
        s_environment = entries;
    }
    else if (deleting) {
        int cursor = index;
        engine_heap_free(entries[index]);
        do {
            entries[cursor] = entries[cursor + 1];
        } while (entries[cursor++] != NULL);
        {
            char** shrunk = crt_environment_array_resize(entries, cursor);
            if (shrunk != NULL) s_environment = shrunk;
        }
    }
    else entries[index] = text;

    if (sync_process != 0) {
        int length = (int)strlen(text) + 1;
        char* copy = (char*)engine_heap_alloc(length);
        if (copy != NULL) {
            int i;
            for (i = 0; i < length; ++i) copy[i] = text[i];
            copy[name_length] = '\0';
#if defined(LEGORRR_BACKEND_WINAPI)
            SetEnvironmentVariableA(copy, deleting ? NULL : copy + name_length + 1);
#else
            if (deleting) unsetenv(copy);
            else setenv(copy, copy + name_length + 1, 1);
#endif
            engine_heap_free(copy);
        }
    }
    return 0;
}

// FUNCTION: LEGORR 0x0049af60
int crt_wide_environment_string_list_convert(void)
{
#if defined(LEGORRR_BACKEND_WINAPI)
    uint16_t** entry = s_wide_environment;
    if (entry == NULL || *entry == NULL) return 0;
    while (*entry != NULL) {
        int count = WideCharToMultiByte(CP_OEMCP, 0, (LPCWCH)*entry, -1,
                                        NULL, 0, NULL, NULL);
        char* converted;
        if (count == 0) return -1;
        converted = (char*)engine_heap_alloc(count);
        if (converted == NULL) return -1;
        if (WideCharToMultiByte(CP_OEMCP, 0, (LPCWCH)*entry, -1,
                                converted, count, NULL, NULL) == 0) return -1;
        crt_put_env_update(converted, 0);
        ++entry;
    }
    return 0;
#else
    return (s_wide_environment == NULL || *s_wide_environment == NULL) ? 0 : -1;
#endif
}

// FUNCTION: LEGORR 0x0049ae10
char* crt_environment_variable_lookup(const char* name)
{
    int name_length;
    char** entry;
    if (s_environment == NULL && s_wide_environment != NULL &&
        crt_wide_environment_string_list_convert() != 0) return NULL;
    if (name == NULL || s_environment == NULL) return NULL;
    name_length = (int)strlen(name);
    /* Corpus 0x49ae64 uses an unsigned JBE length gate. */
    for (entry = s_environment; *entry != NULL; ++entry) {
        if (strlen(*entry) > name_length && (*entry)[name_length] == '=' &&
            crt_counted_string_compare(*entry, name, name_length) == 0) {
            return *entry + name_length + 1;
        }
    }
    return NULL;
}

/* CrtCommandLineArgumentScan (FUN_00495f80): the CRT's two-pass command-line
 * parser. Counts include the final null pointer and each argument's NUL byte,
 * matching the allocation sizes consumed by CrtCommandLineArgumentsInit. */
// FUNCTION: LEGORR 0x00495f80
void crt_command_line_argument_scan(const unsigned char* command_line,
                                    char** argv_out,
                                    unsigned char* string_out,
                                    int* argc_and_null_count,
                                    int* string_byte_count)
{
    const unsigned char* cursor = command_line;
    unsigned char* output = string_out;
    char** argv = argv_out;
    int argc_count = 1;
    int byte_count = 0;

    if (argv != NULL) {
        *argv++ = (char*)output;
    }

    /* argv[0] follows a separate rule: quotes only have meaning when the
     * first byte is a quote, and otherwise the name ends at whitespace. */
    if (*cursor == '"') {
        ++cursor;
        while (*cursor != '"' && *cursor != 0u) {
            if (crt_character_class_flag_test(*cursor, 0u, 4u) != 0u) {
                if (output != NULL) {
                    *output++ = *cursor;
                }
                ++cursor;
                ++byte_count;
            }
            if (output != NULL) {
                *output++ = *cursor;
            }
            ++cursor;
            ++byte_count;
        }
        if (*cursor == '"') {
            ++cursor;
        }
    }
    else {
        while (*cursor != 0u && *cursor != ' ' && *cursor != '\t') {
            const unsigned char ch = *cursor;
            if (output != NULL) {
                *output++ = ch;
            }
            ++cursor;
            ++byte_count;
            if (crt_character_class_flag_test(ch, 0u, 4u) != 0u
                && *cursor != 0u) {
                if (output != NULL) {
                    *output++ = *cursor;
                }
                ++cursor;
                ++byte_count;
            }
        }
    }
    if (output != NULL) {
        *output++ = 0u;
    }
    ++byte_count;

    while (*cursor != 0u) {
        int in_quotes = 0;
        int quote_state = 0;
        while (*cursor == ' ' || *cursor == '\t') {
            ++cursor;
        }
        if (*cursor == 0u) {
            break;
        }
        if (argv != NULL) {
            *argv++ = (char*)output;
        }
        ++argc_count;

        for (;;) {
            unsigned int slash_count = 0u;
            int copy_character = 1;
            unsigned char ch;

            while (*cursor == '\\') {
                ++slash_count;
                ++cursor;
            }
            if (*cursor == '"') {
                if ((slash_count & 1u) == 0u) {
                    if (in_quotes && cursor[1] == '"') {
                        ++cursor;
                    }
                    else {
                        copy_character = 0;
                    }
                    in_quotes = !quote_state;
                    quote_state = in_quotes;
                }
                slash_count >>= 1;
            }
            while (slash_count != 0u) {
                if (output != NULL) {
                    *output++ = '\\';
                }
                ++byte_count;
                --slash_count;
            }

            ch = *cursor;
            if (ch == 0u || (!in_quotes && (ch == ' ' || ch == '\t'))) {
                break;
            }
            if (copy_character) {
                if (crt_character_class_flag_test(ch, 0u, 4u) != 0u) {
                    if (output != NULL) {
                        *output++ = ch;
                    }
                    ++byte_count;
                    ++cursor;
                    if (output != NULL) {
                        *output++ = *cursor;
                    }
                    ++byte_count;
                    if (*cursor != 0u) {
                        ++cursor;
                    }
                }
                else {
                    if (output != NULL) {
                        *output++ = ch;
                    }
                    ++byte_count;
                    ++cursor;
                }
            }
            else {
                ++cursor;
            }
        }
        if (output != NULL) {
            *output++ = 0u;
        }
        ++byte_count;
    }

    if (argv != NULL) {
        *argv = NULL;
    }
    ++argc_count;
    *argc_and_null_count = argc_count;
    *string_byte_count = byte_count;
}

/* CrtDecimalMantissaRoundAndNormalize (FUN_00497e20). The decimal record is
 * the locale-conversion cache shape: the exponent is its second DWORD, while
 * the final field points at the generated digit string. Native-width pointer
 * fields keep this record usable on the portable 64-bit backend. */
// FUNCTION: LEGORR 0x00497e20
void crt_decimal_mantissa_round_and_normalize(char* output, int precision,
                                               CrtDecimalFormatContext* context)
{
    char* source = context->digits;
    char* destination = output + 1;
    int remaining = precision;
    int count = precision;

    *output = '0';
    while (count > 0) {
        char digit = *source;
        if (digit == '\0') {
            digit = '0';
        }
        else {
            ++source;
        }
        *destination++ = digit;
        --count;
        --remaining;
    }
    *destination = '\0';

    if (remaining >= 0 && *source > '4') {
        char* carry = destination - 1;
        while (*carry == '9') {
            *carry = '0';
            --carry;
        }
        *carry = (char)(*carry + 1);
    }

    if (*output == '1') {
        ++context->exponent;
        return;
    }

    /* Discard the guard digit at output[0] when rounding did not carry into
     * it. The source range includes the terminator. */
    {
        char* src = output + 1;
        char* dst = output;
        do {
            *dst++ = *src;
        } while (*src++ != '\0');
    }
}

// FUNCTION: LEGORR 0x0049a1b0
uint32_t crt_extended_float_to_decimal(uint32_t significand_low,
                                      uint32_t significand_high,
                                      uint16_t sign_exponent,
                                      int precision,
                                      uint8_t flags,
                                      uint8_t* output)
{
    uint16_t exponent_field = sign_exponent & 0x7fffu;
    uint16_t value[6];
    uint32_t limbs[3];
    uint32_t i;
    uint32_t digit_count;
    uint32_t estimate;
    int decimal_exponent;
    int exponent_delta;
    int count;
    int end;
    const uint8_t sign = (sign_exponent & 0x8000u) != 0u ? '-' : ' ';

    output[2] = sign;
    if (exponent_field == 0u && significand_high == 0u &&
        significand_low == 0u) {
        *(uint16_t*)output = 0u;
        output[2] = ' ';
        output[3] = 1u;
        output[4] = '0';
        output[5] = '\0';
        return 1u;
    }

    if (exponent_field == 0x7fffu) {
        *(uint16_t*)output = 1u;
        if ((significand_high != 0x80000000u || significand_low != 0u) &&
            (significand_high & 0x40000000u) == 0u) {
            output[3] = 6u;
            memcpy(output + 4, "1#SNAN", 7u);
            return 0u;
        }
        if ((sign_exponent & 0x8000u) != 0u &&
            significand_high == 0xc0000000u && significand_low == 0u) {
            output[3] = 5u;
            memcpy(output + 4, "1#IND", 6u);
            return 0u;
        }
        if (significand_high == 0x80000000u && significand_low == 0u) {
            output[3] = 5u;
            memcpy(output + 4, "1#INF", 6u);
            return 0u;
        }
        output[3] = 6u;
        memcpy(output + 4, "1#QNAN", 7u);
        return 0u;
    }

    value[0] = 0u;
    value[1] = (uint16_t)significand_low;
    value[2] = (uint16_t)(significand_low >> 16);
    value[3] = (uint16_t)significand_high;
    value[4] = (uint16_t)(significand_high >> 16);
    value[5] = exponent_field;
    estimate = (((uint32_t)(exponent_field >> 8) +
                 (significand_high >> 24) * 2u) * 0x4du) +
               0xecbced0cu + (uint32_t)exponent_field * 0x4d10u;
    decimal_exponent = (int16_t)(estimate >> 16);
    crt_extended_float_power_of_ten_scale(value,
                                          (uint32_t)(-decimal_exponent), 1);

    if (exponent_field > 0x3ffeu) {
        static const uint16_t tenth[6] = {
            0xccccu, 0xccccu, 0xccccu, 0xccccu, 0xccccu, 0x3ffbu
        };
        ++decimal_exponent;
        crt_extended80_multiply(value, tenth);
    }
    *(int16_t*)output = (int16_t)decimal_exponent;

    count = precision;
    if ((flags & 1u) != 0u) {
        count += decimal_exponent;
        if (count < 1) {
            *(int16_t*)output = 0;
            output[2] = ' ';
            output[3] = 1u;
            output[4] = '0';
            output[5] = '\0';
            return 1u;
        }
    }
    if (count > 21) {
        count = 21;
    }

    exponent_delta = (int16_t)value[5] - 0x3ffe;
    limbs[0] = (uint32_t)value[0] | ((uint32_t)value[1] << 16);
    limbs[1] = (uint32_t)value[2] | ((uint32_t)value[3] << 16);
    limbs[2] = (uint32_t)value[4] << 16;
    for (i = 0u; i < 8u; ++i) {
        extended_float96_bit_shift_left_one(limbs);
    }
    if (exponent_delta < 0) {
        int shifts = -exponent_delta;
        while (shifts != 0) {
            extended_float96_bit_shift_right_one(limbs);
            --shifts;
        }
    }

    limbs[2] &= 0x00ffffffu;
    digit_count = (uint32_t)(count + 1);
    for (i = 0u; i < digit_count; ++i) {
        uint32_t original[3];
        uint32_t digit;

        original[0] = limbs[0];
        original[1] = limbs[1];
        original[2] = limbs[2];
        extended_float96_bit_shift_left_one(limbs);
        extended_float96_bit_shift_left_one(limbs);
        crt_uint96_add(limbs, original);
        extended_float96_bit_shift_left_one(limbs);
        digit = (limbs[2] >> 24) & 0xffu;
        output[4u + i] = (uint8_t)(digit + (uint32_t)'0');
        limbs[2] &= 0x00ffffffu;
    }

    end = (int)digit_count - 1;
    if (output[4u + end] < '5') {
        while (end >= 0 && output[4u + end] == '0') {
            --end;
        }
        if (end < 0) {
            *(int16_t*)output = 0;
            output[2] = ' ';
            output[3] = 1u;
            output[4] = '0';
            output[5] = '\0';
            return 1u;
        }
    }
    else {
        while (end >= 0 && output[4u + end] == '9') {
            output[4u + end] = '0';
            --end;
        }
        if (end < 0) {
            ++*(int16_t*)output;
            output[4] = '1';
            end = 0;
        }
        else {
            ++output[4u + end];
        }
    }
    output[3] = (uint8_t)(end + 1);
    output[4u + (uint32_t)end + 1u] = '\0';
    return 1u;
}

// FUNCTION: LEGORR 0x00497ec0
CrtDecimalFormatContext* crt_locale_data_get_or_initialize(
    uint32_t double_low, uint32_t double_high)
{
#if !defined(_M_IX86) && !defined(__i386__)
    uint32_t source[2];
#endif
    uint32_t extended[3];
    uint32_t converted;

#if defined(_M_IX86) || defined(__i386__)
    /* Corpus 0x497ec3..0x497ecc passes the adjacent cdecl words in place. */
    (void)double_high;
    crt_double_to_extended80(extended, &double_low);
#else
    source[0] = double_low;
    source[1] = double_high;
    crt_double_to_extended80(extended, source);
#endif
    converted = crt_extended_float_to_decimal(
        extended[0], extended[1], (uint16_t)extended[2], 0x11, 0,
        s_decimal_conversion_buffer);

    s_decimal_format_context.first = (int8_t)s_decimal_conversion_buffer[2];
    s_decimal_format_context.exponent =
        *(int16_t*)s_decimal_conversion_buffer;
    s_decimal_format_context.locale_result = (char*)converted;
    s_decimal_format_context.digits =
        (char*)s_decimal_conversion_buffer + 4;
    return &s_decimal_format_context;
}

// FUNCTION: LEGORR 0x00490400
/* Both format appenders load s_current_decimal_format only in the prepared
 * branch, matching the corpus's branch-local context load. */
char* crt_scientific_float_format_append(const uint32_t* value,
                                         char* output,
                                         int precision,
                                         int uppercase)
{
    CrtDecimalFormatContext* context;
    char* cursor;
    char* exponent_text;
    int exponent;
    uint32_t exponent_prefix;
    uint16_t exponent_suffix;

    if (s_decimal_format_prepared == 0u) {
        context = crt_locale_data_get_or_initialize(value[0], value[1]);
        crt_decimal_mantissa_round_and_normalize(
            output + (context->first == '-' ? 1 : 0) +
                (precision > 0 ? 1 : 0),
            precision + 1, context);
    }
    else {
        context = s_current_decimal_format;
        string_prefix_reserve(output + (context->first == '-' ? 1 : 0),
                              precision > 0);
    }

    cursor = output;
    if (context->first == '-') {
        *cursor++ = '-';
    }
    if (precision > 0) {
        cursor[0] = cursor[1];
        ++cursor;
        *cursor = s_decimal_separator;
    }
    exponent_text = cursor + precision +
                    (s_decimal_format_prepared == 0u ? 1 : 0);
    exponent_prefix = 0x30302b65u;
    exponent_suffix = 0x0030u;
    memcpy(exponent_text, &exponent_prefix, sizeof(exponent_prefix));
    memcpy(exponent_text + 4, &exponent_suffix, sizeof(exponent_suffix));
    if (uppercase != 0) {
        exponent_text[0] = 'E';
    }

    if (context->digits[0] != '0') {
        exponent = context->exponent - 1;
        if (exponent < 0) {
            exponent = -exponent;
            exponent_text[1] = '-';
        }
        if (exponent >= 100) {
            exponent_text[2] = (char)('0' + exponent / 100);
            exponent %= 100;
        }
        if (exponent >= 10) {
            exponent_text[3] = (char)('0' + exponent / 10);
            exponent %= 10;
        }
        exponent_text[4] = (char)('0' + exponent);
    }
    return output;
}

// FUNCTION: LEGORR 0x00490540
char* crt_fixed_float_fraction_format(const uint32_t* value,
                                      char* output,
                                      uint32_t precision)
{
    CrtDecimalFormatContext* context = s_current_decimal_format;
    char* cursor;
    int integer_digits;

    if (s_decimal_format_prepared == 0u) {
        context = crt_locale_data_get_or_initialize(value[0], value[1]);
        crt_decimal_mantissa_round_and_normalize(
            output + (context->first == '-' ? 1 : 0),
            context->exponent + (int)precision, context);
    }
    else if (s_decimal_format_position == (int32_t)precision) {
        const uint32_t zero_index =
            precision + (context->first == '-' ? 1u : 0u);
        output[zero_index] = '0';
        output[zero_index + 1u] = '\0';
    }

    cursor = output;
    if (context->first == '-') {
        *cursor++ = '-';
    }
    integer_digits = context->exponent;
    if (integer_digits < 1) {
        string_prefix_reserve(cursor, 1);
        *cursor++ = '0';
    }
    else {
        cursor += integer_digits;
    }

    if (precision != 0u) {
        string_prefix_reserve(cursor, 1);
        *cursor++ = s_decimal_separator;
        if (integer_digits < 0) {
            int zero_count = (int)precision;
            if (s_decimal_format_prepared != 0u ||
                -integer_digits <= (int)precision) {
                zero_count = -integer_digits;
            }
            string_prefix_reserve(cursor, zero_count);
            while (zero_count > 0) {
                *cursor++ = '0';
                --zero_count;
            }
        }
    }
    return output;
}

// FUNCTION: LEGORR 0x00490640
void crt_float_decimal_format_prepare(const uint32_t* value,
                                      char* output,
                                      int precision,
                                      int uppercase)
{
    char* digits;
    char* end;

    s_current_decimal_format =
        crt_locale_data_get_or_initialize(value[0], value[1]);
    s_decimal_format_position = s_current_decimal_format->exponent - 1;
    digits = output + (s_current_decimal_format->first == '-' ? 1 : 0);
    crt_decimal_mantissa_round_and_normalize(digits, precision,
                                              s_current_decimal_format);
    s_decimal_format_truncated =
        (uint8_t)(s_decimal_format_position <
                  s_current_decimal_format->exponent - 1);
    s_decimal_format_position = s_current_decimal_format->exponent - 1;

    if (s_decimal_format_position >= -4 &&
        s_decimal_format_position < precision) {
        if (s_decimal_format_truncated != 0) {
            end = digits;
            while (*end != '\0') {
                ++end;
            }
            end[-1] = '\0';
        }
        crt_fixed_decimal_format_guarded(value, output,
                                          (uint32_t)precision);
        return;
    }
    crt_scientific_float_format_guarded(value, output, precision, uppercase);
}

// FUNCTION: LEGORR 0x004906f0
void crt_scientific_float_format_guarded(const uint32_t* value,
                                         char* output,
                                         int precision,
                                         int uppercase)
{
    s_decimal_format_prepared = 1u;
    crt_scientific_float_format_append(value, output, precision, uppercase);
    s_decimal_format_prepared = 0u;
}

// FUNCTION: LEGORR 0x00490720
void crt_fixed_decimal_format_guarded(const uint32_t* value,
                                      char* output,
                                      uint32_t precision)
{
    s_decimal_format_prepared = 1u;
    crt_fixed_float_fraction_format(value, output, precision);
    s_decimal_format_prepared = 0u;
}

/* --- the CRT file-descriptor flag window (2026-10-25) ---------------------
 * CrtFileDescriptorFlag40Get (FUN_00498b40): the corpus reads bit 0x40 of
 * the byte at block+4 in its file-descriptor table — 32 eight-byte entries
 * per block, blocks indexed by one of the 64 pointers at DAT_0076ddc0. The
 * bound word at DAT_0076dec0 follows that 0x100-byte pointer table. This
 * table is CRT platform state with no ported owner: the pointer blocks and
 * count word land as zero-init own storage behind install/peek hooks (the
 * unowned-global convention).
 * Deviations: the block window and null blocks are guarded (the corpus
 * strides), and a negative descriptor answers 0. */
#define CRT_FD_BLOCKS 64
// GLOBAL: LEGORR 0x0076ddc0
static void* s_fd_flag_blocks[CRT_FD_BLOCKS]; /* DAT_0076ddc0 role */
// GLOBAL: LEGORR 0x0076dec0
static int   s_fd_flag_count;                 /* DAT_0076dec0 role */
// GLOBAL: LEGORR 0x004b2428
static unsigned char s_fd_fallback_record[8] = {
    0xffu, 0xffu, 0xffu, 0xffu, 0u, 10u, 0u, 0u
};
// GLOBAL: LEGORR 0x004b2648
static int s_standard_stream_record_0[8] = { 0, 0, 0, 2, 1, 0, 0, 0 };
// GLOBAL: LEGORR 0x004b2668
static int s_standard_stream_record_1[8] = { 0, 0, 0, 2, 2, 0, 0, 0 };

// FUNCTION: LEGORR 0x00498b40
unsigned crt_file_descriptor_flag40_get(unsigned fd)
{
    const int descriptor = (int)fd;
    if (fd >= (unsigned)s_fd_flag_count) {
        return 0;
    }
    const unsigned char* block =
        (const unsigned char*)s_fd_flag_blocks[descriptor >> 5];
    return (unsigned)(block[(descriptor & 0x1f) * 8 + 4] & 0x40u);
}

// FUNCTION: LEGORR 0x004991f0
int crt_file_descriptor_handle_get(unsigned fd)
{
    const int descriptor = (int)fd;
    if (fd < (unsigned)s_fd_flag_count) {
        const unsigned char* entry =
            (const unsigned char*)s_fd_flag_blocks[descriptor >> 5] +
            (descriptor & 0x1f) * 8;
        if ((entry[4] & 1u) != 0u) {
            return *(const int*)entry;
        }
    }
    s_math_error_code = 9u;
    s_win32_error_code = 0u;
    return -1;
}

/* CrtFileDescriptorAllocate (FUN_00498fe0): scan 32-entry blocks in the
 * 64-slot pointer table. A new block starts with inactive handles, zero
 * flags and the CRT's default mode byte 10. */
// FUNCTION: LEGORR 0x00498fe0
int crt_file_descriptor_allocate(void)
{
    int block_index;
    int block_base = 0;
    for (block_index = 0; block_index < CRT_FD_BLOCKS; ++block_index) {
        unsigned char* block = (unsigned char*)s_fd_flag_blocks[block_index];
        if (block == NULL) {
            unsigned char* entry;
            block = (unsigned char*)engine_heap_alloc(0x100);
            if (block == NULL) {
                return -1;
            }
            s_fd_flag_count += 0x20;
            s_fd_flag_blocks[block_index] = block;
            for (entry = block; entry < block + 0x100; entry += 8) {
                *(int*)entry = -1;
                entry[4] = 0;
                entry[5] = 10;
            }
            return block_base;
        }
        else {
            unsigned char* entry;
            for (entry = block; entry < block + 0x100; entry += 8) {
                if ((entry[4] & 1u) == 0u) {
                    *(int*)entry = -1;
                    return (int)(entry - block) / 8 + block_base;
                }
            }
        }
        block_base += 0x20;
    }
    return -1;
}

/* CrtFileDescriptorSetHandle (FUN_004990a0): install a handle only into an
 * allocated descriptor whose handle sentinel is -1. The image's standard
 * handle synchronization mode is initialized to 2 and has no writer, so
 * its conditional Win32 synchronization path is dormant in this build. */
// FUNCTION: LEGORR 0x004990a0
int crt_file_descriptor_set_handle(unsigned fd, int handle)
{
    if (fd < (unsigned)s_fd_flag_count) {
        unsigned char* entry =
            (unsigned char*)s_fd_flag_blocks[fd >> 5] + (fd & 0x1fu) * 8u;
        if (*(int*)entry == -1) {
#if defined(LEGORRR_BACKEND_WINAPI)
            if (s_runtime_error_console_mode == 1) {
                if (fd == 0u) {
                    SetStdHandle((DWORD)-10, (HANDLE)handle);
                }
                else if (fd == 1u) {
                    SetStdHandle((DWORD)-11, (HANDLE)handle);
                    *(int*)entry = handle;
                    return 0;
                }
                else if (fd == 2u) {
                    SetStdHandle((DWORD)-12, (HANDLE)handle);
                    *(int*)entry = handle;
                    return 0;
                }
            }
#endif
            *(int*)entry = handle;
            return 0;
        }
    }
    s_math_error_code = 9u;
    s_win32_error_code = 0u;
    return -1;
}

/* CrtFileDescriptorCloseSlot (FUN_00499150): invalidate an open slot's
 * handle while retaining its descriptor flags for the next CRT operation. */
// FUNCTION: LEGORR 0x00499150
int crt_file_descriptor_close_slot(unsigned fd)
{
    if (fd < (unsigned)s_fd_flag_count) {
        unsigned char* entry =
            (unsigned char*)s_fd_flag_blocks[fd >> 5] + (fd & 0x1fu) * 8u;
        if ((entry[4] & 1u) != 0u && *(int*)entry != -1) {
#if defined(LEGORRR_BACKEND_WINAPI)
            if (s_runtime_error_console_mode == 1) {
                if (fd == 0u) {
                    SetStdHandle((DWORD)-10, NULL);
                }
                else if (fd == 1u) {
                    SetStdHandle((DWORD)-11, NULL);
                }
                else if (fd == 2u) {
                    SetStdHandle((DWORD)-12, NULL);
                }
            }
#endif
            *(int*)entry = -1;
            return 0;
        }
    }
    s_math_error_code = 9u;
    s_win32_error_code = 0u;
    return -1;
}

/* CrtFindHandleClose (FUN_0048f950): close one search handle and set the
 * CRT invalid-argument error when the platform close operation fails. */
// FUNCTION: LEGORR 0x0048f950
int crt_find_handle_close(int handle)
{
#if defined(LEGORRR_BACKEND_WINAPI)
    if (FindClose((HANDLE)handle) == 0) {
        s_math_error_code = 0x16u;
        return -1;
    }
    return 0;
#elif !defined(_MSC_VER)
    (void)handle;
    s_math_error_code = EINVAL;
    return -1;
#else
    (void)handle;
    s_math_error_code = 0x16u;
    return -1;
#endif
}

/* CrtFileDescriptorClose (FUN_00494d10): close an active descriptor, keeping
 * shared stdout/stderr handles open while releasing the descriptor slot. */
// FUNCTION: LEGORR 0x00494d10
int crt_file_descriptor_close(unsigned fd)
{
    unsigned char* block;
    unsigned char* entry;
    int handle;
    uint32_t os_error;
    if (fd >= (unsigned)s_fd_flag_count || (fd >> 5) >= CRT_FD_BLOCKS) {
        s_math_error_code = 9u;
        s_win32_error_code = 0u;
        return -1;
    }
    block = (unsigned char*)s_fd_flag_blocks[fd >> 5];
    if (block == NULL) {
        s_math_error_code = 9u;
        s_win32_error_code = 0u;
        return -1;
    }
    entry = block + (fd & 0x1fu) * 8u;
    if ((entry[4] & 1u) == 0u) {
        s_math_error_code = 9u;
        s_win32_error_code = 0u;
        return -1;
    }
    os_error = 0u;
    handle = crt_file_descriptor_handle_get(fd);
    if (handle != -1) {
        if (fd == 1u || fd == 2u) {
            if (crt_file_descriptor_handle_get(1u) ==
                crt_file_descriptor_handle_get(2u)) {
                handle = -1;
            }
        }
        if (handle != -1) {
#if defined(LEGORRR_BACKEND_WINAPI)
            if (CloseHandle((HANDLE)handle) == 0) {
                os_error = GetLastError();
            }
#elif !defined(_MSC_VER)
            if (close(handle) != 0) {
                s_math_error_code = (uint32_t)errno;
                s_win32_error_code = 0u;
                os_error = 0xffffffffu;
            }
#else
            s_math_error_code = 9u;
            s_win32_error_code = 0u;
            os_error = 0xffffffffu;
#endif
        }
    }
    (void)crt_file_descriptor_close_slot(fd);
    entry[4] = 0u;
    if (os_error == 0u) {
        return 0;
    }
    if (os_error != 0xffffffffu) {
        crt_win32_error_map_to_errno(os_error);
    }
    return -1;
}

/* Win32FileDescriptorOpen (FUN_00499240): translate CRT open flags to the
 * corresponding Win32 access, sharing, disposition, and handle flags, then
 * register the resulting operating-system handle in the CRT descriptor table. */
// FUNCTION: LEGORR 0x00499240
int crt_win32_file_descriptor_open(const char* path, uint32_t flags,
                                   uint32_t share, uint32_t attributes)
{
    uint32_t access;
    uint32_t disposition;
    uint32_t handle_flags;
    uint32_t share_mode;
    unsigned char descriptor_flags;
    int fd;
    if ((flags & 3u) == 0u) {
        access = 0x80000000u;
    }
    else if ((flags & 3u) == 1u) {
        access = 0x40000000u;
    }
    else if ((flags & 3u) == 2u) {
        access = 0xc0000000u;
    }
    else {
        s_math_error_code = 0x16u;
        s_win32_error_code = 0u;
        return -1;
    }
    switch (share) {
    case 0x10u: share_mode = 0u; break;
    case 0x20u: share_mode = 1u; break;
    case 0x30u: share_mode = 2u; break;
    case 0x40u: share_mode = 3u; break;
    default:
        s_math_error_code = 0x16u;
        s_win32_error_code = 0u;
        return -1;
    }
    switch (flags & 0x700u) {
    case 0x000u: case 0x400u: disposition = 3u; break;
    case 0x100u: disposition = 4u; break;
    case 0x200u: case 0x600u: disposition = 5u; break;
    case 0x300u: disposition = 2u; break;
    case 0x500u: case 0x700u: disposition = 1u; break;
    default:
        s_math_error_code = 0x16u;
        s_win32_error_code = 0u;
        return -1;
    }
    descriptor_flags = (unsigned char)((flags & 0x80u) == 0u ? 0u : 0x10u);
    if ((flags & 0x8000u) == 0u &&
        ((flags & 0x4000u) != 0u || s_file_open_share_state != 0x8000u)) {
        descriptor_flags |= 0x80u;
    }
    handle_flags = 0x80u;
    if ((flags & 0x100u) != 0u &&
        (((unsigned char)attributes & (unsigned char)~s_file_open_attribute_policy & 0x80u) == 0u)) {
        handle_flags = 1u;
    }
    if ((flags & 0x40u) != 0u) {
        handle_flags |= 0x04000000u;
        access |= 0x10000u;
    }
    if ((flags & 0x1000u) != 0u) {
        handle_flags |= 0x100u;
    }
    if ((flags & 0x20u) != 0u) {
        handle_flags |= 0x08000000u;
    }
    else if ((flags & 0x10u) != 0u) {
        handle_flags |= 0x10000000u;
    }
    fd = crt_file_descriptor_allocate();
    if (fd < 0) {
        s_math_error_code = 0x18u;
        s_win32_error_code = 0u;
        return -1;
    }
#if defined(LEGORRR_BACKEND_WINAPI)
    {
        SECURITY_ATTRIBUTES security;
        HANDLE handle;
        DWORD type;
        security.nLength = sizeof security;
        security.lpSecurityDescriptor = NULL;
        security.bInheritHandle = ((flags & 0x80u) == 0u) ? TRUE : FALSE;
        descriptor_flags = (unsigned char)((flags & 0x80u) == 0u ? 0u : 0x10u);
        if ((flags & 0x8000u) == 0u &&
            ((flags & 0x4000u) != 0u || s_file_open_share_state != 0x8000u)) {
            descriptor_flags |= 0x80u;
        }
        handle = CreateFileA(path, access, share_mode, &security, disposition,
                             handle_flags, NULL);
        if (handle == INVALID_HANDLE_VALUE) {
            crt_win32_error_map_to_errno(GetLastError());
            return -1;
        }
        type = GetFileType(handle);
        if (type == 0u) {
            DWORD error = GetLastError();
            (void)CloseHandle(handle);
            crt_win32_error_map_to_errno(error);
            return -1;
        }
        if ((type & 0xffu) == 2u) {
            descriptor_flags |= 0x40u;
        }
        else if ((type & 0xffu) == 3u) {
            descriptor_flags |= 8u;
        }
        (void)crt_file_descriptor_set_handle((unsigned)fd, (int)handle);
        {
            unsigned char* entry = (unsigned char*)s_fd_flag_blocks[fd >> 5] +
                                  (fd & 0x1f) * 8u;
            entry[4] = (unsigned char)(descriptor_flags | 1u);
            if ((descriptor_flags & 0x48u) == 0u &&
                (descriptor_flags & 0x80u) != 0u && (flags & 2u) != 0u) {
                uint32_t last = crt_win32_file_descriptor_seek((uint32_t)fd, -1, 2u);
                if (last == 0xffffffffu) {
                    if (s_math_error_code != 0x83u) {
                        (void)crt_file_descriptor_close((unsigned)fd);
                        return -1;
                    }
                }
                else {
                    unsigned char last_byte = 0u;
#if defined(LEGORRR_BACKEND_WINAPI)
                    DWORD transferred = 0u;
                    int read_count = ReadFile(handle, &last_byte, 1u,
                                              &transferred, NULL) != 0 &&
                                     transferred == 1u ? 1 : 0;
#else
                    int read_count = 0;
#endif
                    if (read_count == 0 && last_byte == 0x1au) {
                        SetLastError(NO_ERROR);
                        if ((uint32_t)SetFilePointer(handle, (LONG)last, NULL, FILE_BEGIN) == 0xffffffffu &&
                            GetLastError() != NO_ERROR) {
                            crt_win32_error_map_to_errno(GetLastError());
                            (void)crt_file_descriptor_close((unsigned)fd);
                            return -1;
                        }
                        if (SetEndOfFile(handle) == 0) {
                            crt_win32_error_map_to_errno(GetLastError());
                            (void)crt_file_descriptor_close((unsigned)fd);
                            return -1;
                        }
                    }
                    if (crt_win32_file_descriptor_seek((uint32_t)fd, 0, 0u) == 0xffffffffu) {
                        (void)crt_file_descriptor_close((unsigned)fd);
                        return -1;
                    }
                }
            }
            if ((descriptor_flags & 0x48u) == 0u && (flags & 8u) != 0u) {
                entry[4] |= 0x20u;
            }
        }
    }
#elif !defined(_MSC_VER)
    {
        int native_flags;
        int native_fd;
        unsigned char* entry;
        native_flags = (flags & 3u) == 0u ? O_RDONLY :
                       (flags & 3u) == 1u ? O_WRONLY : O_RDWR;
        if ((flags & 0x700u) == 0x100u) native_flags |= O_CREAT;
        else if ((flags & 0x700u) == 0x200u || (flags & 0x700u) == 0x600u)
            native_flags |= O_TRUNC;
        else if ((flags & 0x700u) == 0x300u) native_flags |= O_CREAT | O_TRUNC;
        else if ((flags & 0x700u) == 0x500u || (flags & 0x700u) == 0x700u)
            native_flags |= O_CREAT | O_EXCL;
#ifdef O_BINARY
        if ((flags & 0x8000u) != 0u) native_flags |= O_BINARY;
#endif
        native_fd = open(path, native_flags, 0666);
        if (native_fd < 0) {
            s_math_error_code = (uint32_t)errno;
            s_win32_error_code = 0u;
            (void)crt_file_descriptor_close_slot((unsigned)fd);
            return -1;
        }
        entry = (unsigned char*)s_fd_flag_blocks[fd >> 5] + (fd & 0x1f) * 8u;
        *(int*)entry = native_fd;
        entry[4] = (unsigned char)(descriptor_flags | 1u | 0x80u);
        if ((flags & 8u) != 0u) entry[4] |= 0x20u;
    }
#else
    (void)path;
    (void)flags;
    (void)share;
    (void)attributes;
    (void)access;
    (void)disposition;
    (void)handle_flags;
    (void)share_mode;
    (void)descriptor_flags;
    (void)fd;
    s_math_error_code = 9u;
    s_win32_error_code = 0u;
    return -1;
#endif
    return fd;
}

/* CrtStandardStreamBufferInstall (FUN_004974d0): install one of the two
 * cached 4K buffers on its matching standard stream record. Corpus 0x4974d6
 * dereferences the stream before checking its descriptor, with no null guard;
 * preserve that unchecked input behavior (there is no test pin for NULL). */
// FUNCTION: LEGORR 0x004974d0
int crt_standard_stream_buffer_install(int* stream)
{
    int stream_index;
    unsigned fd;
    uint32_t* fields;
    fd = (unsigned)stream[4];
    if (crt_file_descriptor_flag40_get(fd) == 0u) {
        return 0;
    }
    if (stream == s_standard_stream_record_0) {
        stream_index = 0;
    }
    else if (stream == s_standard_stream_record_1) {
        stream_index = 1;
    }
    else {
        return 0;
    }
    ++s_stream_alloc_count;
    fields = (uint32_t*)stream;
    if ((fields[3] & 0x10cu) == 0u) {
        if (s_standard_stream_buffers[stream_index] == NULL) {
            s_standard_stream_buffers[stream_index] =
                engine_heap_alloc(0x1000);
            if (s_standard_stream_buffers[stream_index] == NULL) {
                return 0;
            }
        }
        fields[6] = 0x1000u;
        fields[2] = (uint32_t)s_standard_stream_buffers[stream_index];
        fields[0] = (uint32_t)s_standard_stream_buffers[stream_index];
        fields[1] = 0x1000u;
        fields[3] |= 0x1102u;
        return 1;
    }
    return 0;
}

/* CrtFileDescriptorTableInit (FUN_004965d0): create the first 32 CRT
 * descriptors, import inherited Win32 handles, then seed descriptors 0-2. */
// FUNCTION: LEGORR 0x004965d0
void crt_file_descriptor_table_init(void)
{
    unsigned char* first_block;
    unsigned char* entry;
    unsigned slot;
    first_block = (unsigned char*)engine_heap_alloc(0x100);
    if (first_block == NULL) {
#if defined(_MSC_VER)
        _amsg_exit(0x1b);
#else
        abort();
#endif
    }
    s_fd_flag_count = 0x20;
    s_fd_flag_blocks[0] = first_block;
    for (entry = first_block; entry < first_block + 0x100; entry += 8) {
        *(int*)entry = -1;
        entry[4] = 0u;
        entry[5] = 10u;
    }
#if defined(LEGORRR_BACKEND_WINAPI)
    {
        STARTUPINFOA startup;
        GetStartupInfoA(&startup);
        if (startup.cbReserved2 != 0u && startup.lpReserved2 != NULL) {
            const unsigned char* reserved = startup.lpReserved2;
            uint32_t inherited_count;
            uint32_t import_count;
            const unsigned char* inherited_flags;
            const unsigned char* inherited_handles;
            memcpy(&inherited_count, reserved, sizeof inherited_count);
            inherited_flags = reserved + 4u;
            inherited_handles = inherited_flags + inherited_count;
            import_count = inherited_count;
            if (import_count > 0x800u) {
                import_count = 0x800u;
            }
            while ((uint32_t)s_fd_flag_count < import_count) {
                const unsigned block_index = (unsigned)s_fd_flag_count >> 5;
                unsigned char* block =
                    (unsigned char*)engine_heap_alloc(0x100);
                if (block == NULL) {
                    import_count = (uint32_t)s_fd_flag_count;
                    break;
                }
                s_fd_flag_blocks[block_index] = block;
                s_fd_flag_count += 0x20;
                for (entry = block; entry < block + 0x100; entry += 8) {
                    *(int*)entry = -1;
                    entry[4] = 0u;
                    entry[5] = 10u;
                }
            }
            for (slot = 0u; slot < import_count; ++slot) {
                uint32_t raw_handle;
                const unsigned block_index = slot >> 5;
                const unsigned entry_index = slot & 0x1fu;
                unsigned char* entry;
                memcpy(&raw_handle, inherited_handles + slot * 4u, 4u);
                if (raw_handle == 0xffffffffu ||
                    (inherited_flags[slot] & 1u) == 0u) {
                    continue;
                }
                if ((inherited_flags[slot] & 8u) == 0u &&
                    GetFileType((HANDLE)raw_handle) == 0u) {
                    continue;
                }
                entry = (unsigned char*)s_fd_flag_blocks[block_index] +
                        entry_index * 8u;
                *(uint32_t*)entry = raw_handle;
                entry[4] = inherited_flags[slot];
            }
        }
    }
#elif !defined(_MSC_VER)
    for (slot = 0u; slot < 3u; ++slot) {
        unsigned char* entry = first_block + slot * 8u;
        *(int*)entry = (int)slot;
        entry[4] = (unsigned char)(0x81u | (isatty((int)slot) ? 8u : 0u));
    }
#endif
    for (slot = 0u; slot < 3u; ++slot) {
        unsigned char* entry = first_block + slot * 8u;
        if (*(int*)entry == -1) {
#if defined(LEGORRR_BACKEND_WINAPI)
            DWORD standard_id = slot == 0u ? (DWORD)-10 :
                                slot == 1u ? (DWORD)-11 : (DWORD)-12;
            HANDLE handle = GetStdHandle(standard_id);
            DWORD file_type;
            entry[4] = 0x81u;
            if (handle == INVALID_HANDLE_VALUE ||
                (file_type = GetFileType(handle)) == 0u) {
                entry[4] |= 0x40u;
            }
            else {
                *(HANDLE*)entry = handle;
                if ((file_type & 0xffu) == 2u) {
                    entry[4] |= 0x40u;
                }
                else if ((file_type & 0xffu) == 3u) {
                    entry[4] |= 8u;
                }
            }
#else
            entry[4] = 0x81u;
#endif
        }
        else {
            entry[4] |= 0x80u;
        }
    }
#if defined(LEGORRR_BACKEND_WINAPI)
    (void)SetHandleCount((unsigned)s_fd_flag_count);
#endif
}

/* Win32FileDescriptorRead (FUN_00497130): refill a CRT descriptor buffer,
 * translate text-mode CR/LF and Ctrl-Z, and preserve a terminal lookahead. */
// FUNCTION: LEGORR 0x00497130
int crt_win32_file_descriptor_read(uint32_t fd, char* buffer,
                                   uint32_t count)
{
    unsigned char* block;
    unsigned char* entry;
    unsigned char flags;
    int output_count;
    uint32_t bytes_read;
    uint32_t os_error;
    char* output;
    if (fd >= (uint32_t)s_fd_flag_count || (fd >> 5) >= CRT_FD_BLOCKS ||
        buffer == NULL) {
        s_math_error_code = 9u;
        s_win32_error_code = 0u;
        return -1;
    }
    block = (unsigned char*)s_fd_flag_blocks[fd >> 5];
    if (block == NULL) {
        s_math_error_code = 9u;
        s_win32_error_code = 0u;
        return -1;
    }
    entry = block + (fd & 0x1fu) * 8u;
    flags = entry[4];
    if ((flags & 1u) == 0u) {
        s_math_error_code = 9u;
        s_win32_error_code = 0u;
        return -1;
    }
    if (count == 0u || (flags & 2u) != 0u) {
        return 0;
    }
    output_count = 0;
    output = buffer;
    if ((flags & 0x48u) != 0u && entry[5] != (unsigned char)'\n') {
        *output++ = (char)entry[5];
        output_count = 1;
        entry[5] = (unsigned char)'\n';
        --count;
    }
    bytes_read = 0u;
    os_error = 0u;
    if (count != 0u) {
#if defined(LEGORRR_BACKEND_WINAPI)
    {
        DWORD transferred = 0u;
        if (ReadFile((HANDLE)*(int*)entry, output, count, &transferred,
                     NULL) == 0) {
            os_error = GetLastError();
        }
        else {
            bytes_read = (uint32_t)transferred;
        }
    }
#elif !defined(_MSC_VER)
    {
        ssize_t transferred = read(*(int*)entry, output, count);
        if (transferred < 0) {
            os_error = (uint32_t)errno;
        }
        else {
            bytes_read = (uint32_t)transferred;
        }
    }
#else
    os_error = 9u;
#endif
    }
    if (os_error != 0u) {
#if !defined(LEGORRR_BACKEND_WINAPI) && !defined(_MSC_VER)
        s_math_error_code = os_error;
        s_win32_error_code = 0u;
        return -1;
#else
        if (os_error == 5u) {
            s_win32_error_code = os_error;
            s_math_error_code = 9u;
            return -1;
        }
        if (os_error == 0x6du) {
            return 0;
        }
        crt_win32_error_map_to_errno(os_error);
        return -1;
#endif
    }
    output_count += (int)bytes_read;
    if ((entry[4] & 0x80u) == 0u) {
        return output_count;
    }
    if (bytes_read == 0u || buffer[0] != '\n') {
        entry[4] = (unsigned char)(entry[4] & 0xfbu);
    }
    else {
        entry[4] = (unsigned char)(entry[4] | 4u);
    }
    {
        char* input = buffer;
        char* input_end = buffer + output_count;
        output = buffer;
        while (input < input_end && *input != '\x1a') {
            if (*input != '\r') {
                *output++ = *input++;
                continue;
            }
            if (input + 1 < input_end) {
                if (input[1] == '\n') {
                    *output++ = '\n';
                    input += 2;
                }
                else {
                    *output++ = *input++;
                }
                continue;
            }
            ++input;
            {
                unsigned char lookahead = 0u;
                uint32_t lookahead_read = 0u;
                uint32_t lookahead_error = 0u;
#if defined(LEGORRR_BACKEND_WINAPI)
                DWORD transferred = 0u;
                if (ReadFile((HANDLE)*(int*)entry, &lookahead, 1u,
                             &transferred, NULL) == 0) {
                    lookahead_error = GetLastError();
                }
                else {
                    lookahead_read = (uint32_t)transferred;
                }
#elif !defined(_MSC_VER)
                ssize_t transferred = read(*(int*)entry, &lookahead, 1u);
                if (transferred < 0) {
                    lookahead_error = (uint32_t)errno;
                }
                else {
                    lookahead_read = (uint32_t)transferred;
                }
#endif
                if (lookahead_error == 0u && lookahead_read != 0u) {
                    if ((entry[4] & 0x48u) == 0u) {
                        if (output == buffer && lookahead == '\n') {
                            *output++ = '\n';
                            continue;
                        }
                        (void)crt_win32_file_descriptor_seek(fd, -1, 1u);
                        if (lookahead != '\n') {
                            *output++ = '\r';
                        }
                        continue;
                    }
                    if (lookahead == '\n') {
                        *output++ = '\n';
                        continue;
                    }
                    *output++ = '\r';
                    entry[5] = lookahead;
                    continue;
                }
            }
            *output++ = '\r';
        }
        if (input < input_end && *input == '\x1a' &&
            (entry[4] & 0x40u) == 0u) {
            entry[4] = (unsigned char)(entry[4] | 2u);
        }
        return (int)(output - buffer);
    }
}

/* CrtStreamReadByte (FUN_00497040): return the next byte from a buffered
 * CRT stream, refilling through Win32FileDescriptorRead as needed. The
 * corpus reads the stream's +0x0c flags before any null check; there are no
 * port tests or internal callers that pin a NULL stream here. */
// FUNCTION: LEGORR 0x00497040
uint32_t crt_stream_read_byte(int* stream)
{
    uint32_t flags;
    int read_count;
    unsigned char* descriptor;
    flags = (uint32_t)stream[3];
    if ((flags & 0x83u) == 0u || (flags & 0x40u) != 0u) {
        return 0xffffffffu;
    }
    if ((flags & 2u) != 0u) {
        stream[3] = (int)(flags | 0x20u);
        return 0xffffffffu;
    }
    stream[3] = (int)(flags | 1u);
    if ((flags & 0x10cu) == 0u) {
        crt_stream_buffer_allocate((char*)stream);
    }
    else {
        stream[0] = stream[2];
    }
    read_count = crt_win32_file_descriptor_read((uint32_t)stream[4],
                                                (char*)stream[2],
                                                (uint32_t)stream[6]);
    stream[1] = read_count;
    if (read_count != 0 && read_count != -1) {
        if ((stream[3] & 0x82) == 0) {
            if ((uint32_t)stream[4] == 0xffffffffu) {
                descriptor = s_fd_fallback_record;
            }
            else if ((uint32_t)stream[4] < (uint32_t)s_fd_flag_count &&
                     ((uint32_t)stream[4] >> 5) < CRT_FD_BLOCKS &&
                     s_fd_flag_blocks[(uint32_t)stream[4] >> 5] != NULL) {
                descriptor = (unsigned char*)s_fd_flag_blocks[
                    (uint32_t)stream[4] >> 5] +
                    ((uint32_t)stream[4] & 0x1fu) * 8u;
            }
            else {
                descriptor = s_fd_fallback_record;
            }
            if ((descriptor[4] & 0x82u) == 0x82u) {
                stream[3] |= 0x2000;
            }
        }
        if (stream[6] == 0x200 && (stream[3] & 8) != 0 &&
            (stream[3] & 0x400) == 0) {
            stream[6] = 0x1000;
        }
        stream[1] = read_count - 1;
        {
            uint32_t value = *(unsigned char*)stream[0];
            ++stream[0];
            return value;
        }
    }
    stream[1] = 0;
    stream[3] |= (read_count != 0 ? 0x20 : 0x10);
    return 0xffffffffu;
}

/* CrtStreamBufferFlushAndRelease (FUN_00497570): flush a managed buffer and
 * optionally detach it; cached standard-stream storage remains allocated. */
// FUNCTION: LEGORR 0x00497570
void crt_stream_buffer_flush_and_release(int installed, int* stream)
{
    if (stream == NULL || (stream[3] & 0x1000) == 0) {
        return;
    }
    (void)crt_flush_stream_buffer(stream);
    if (installed != 0) {
        stream[6] = 0;
        stream[3] &= (int)0xffffeeffu;
        stream[0] = 0;
        stream[2] = 0;
    }
}

// FUNCTION: LEGORR 0x0049aea0
int crt_file_descriptor_flag80_set_or_clear(unsigned fd, int mode)
{
    if (fd < (unsigned)s_fd_flag_count && (fd >> 5) < CRT_FD_BLOCKS) {
        unsigned char* block = (unsigned char*)s_fd_flag_blocks[fd >> 5];
        if (block != NULL) {
            unsigned char* flags = block + (fd & 0x1fu) * 8u + 4u;
            const unsigned char old_flags = *flags;
            if ((old_flags & 1u) != 0u) {
                if (mode == 0x8000) {
                    *flags = (unsigned char)(old_flags & 0x7fu);
                } else if (mode == 0x4000) {
                    *flags = (unsigned char)(old_flags | 0x80u);
                } else {
                    s_math_error_code = 0x16u;
                    return -1;
                }
                return (old_flags & 0x80u) != 0u ? 0x4000 : 0x8000;
            }
        }
    }
    s_math_error_code = 9u;
    return -1;
}

void crt_file_descriptor_flag_block_set(int index, void* block)
{
    if (index >= 0 && index < CRT_FD_BLOCKS) {
        s_fd_flag_blocks[index] = block;
    }
}

void* crt_file_descriptor_flag_block_peek(int index)
{
    if (index < 0 || index >= CRT_FD_BLOCKS) {
        return NULL;
    }
    return s_fd_flag_blocks[index];
}

void crt_file_descriptor_count_set(int count)
{
    s_fd_flag_count = count;
}

int crt_file_descriptor_count_peek(void)
{
    return s_fd_flag_count;
}

/* Win32ErrorMapToErrno (FUN_00496ee0) scans the image's 45 pairs at
 * 0x4b24c0, then applies the CRT's two range fallbacks. The initial bytes
 * were checked directly against LegoRR.exe. */
// GLOBAL: LEGORR 0x004b24c0
static const uint32_t s_win32_error_errno_pairs[45][2] = {
    { 0x00000001u, 0x00000016u }, { 0x00000002u, 0x00000002u },
    { 0x00000003u, 0x00000002u }, { 0x00000004u, 0x00000018u },
    { 0x00000005u, 0x0000000du }, { 0x00000006u, 0x00000009u },
    { 0x00000007u, 0x0000000cu }, { 0x00000008u, 0x0000000cu },
    { 0x00000009u, 0x0000000cu }, { 0x0000000au, 0x00000007u },
    { 0x0000000bu, 0x00000008u }, { 0x0000000cu, 0x00000016u },
    { 0x0000000du, 0x00000016u }, { 0x0000000fu, 0x00000002u },
    { 0x00000010u, 0x0000000du }, { 0x00000011u, 0x00000012u },
    { 0x00000012u, 0x00000002u }, { 0x00000021u, 0x0000000du },
    { 0x00000035u, 0x00000002u }, { 0x00000041u, 0x0000000du },
    { 0x00000043u, 0x00000002u }, { 0x00000050u, 0x00000011u },
    { 0x00000052u, 0x0000000du }, { 0x00000053u, 0x0000000du },
    { 0x00000057u, 0x00000016u }, { 0x00000059u, 0x0000000bu },
    { 0x0000006cu, 0x0000000du }, { 0x0000006du, 0x00000020u },
    { 0x00000070u, 0x0000001cu }, { 0x00000072u, 0x00000009u },
    { 0x00000006u, 0x00000016u }, { 0x00000080u, 0x0000000au },
    { 0x00000081u, 0x0000000au }, { 0x00000082u, 0x00000009u },
    { 0x00000083u, 0x00000016u }, { 0x00000084u, 0x0000000du },
    { 0x00000091u, 0x00000029u }, { 0x0000009eu, 0x0000000du },
    { 0x000000a1u, 0x00000002u }, { 0x000000a4u, 0x0000000bu },
    { 0x000000a7u, 0x0000000du }, { 0x000000b7u, 0x00000011u },
    { 0x000000ceu, 0x00000002u }, { 0x000000d7u, 0x0000000bu },
    { 0x00000718u, 0x0000000cu }
};

// FUNCTION: LEGORR 0x00496ee0
void crt_win32_error_map_to_errno(uint32_t error)
{
    unsigned i;
    s_win32_error_code = error;
    for (i = 0; i < 45u; ++i) {
        if (error == s_win32_error_errno_pairs[i][0]) {
            s_math_error_code = s_win32_error_errno_pairs[i][1];
            return;
        }
    }
    /* Corpus 0x496f00 uses unsigned inclusive bounds for this fallback. */
    if (error >= 0x13u && error <= 0x24u) {
        s_math_error_code = 0x0du;
    }
    else if (error >= 0xbcu && error <= 0xcau) {
        s_math_error_code = 8u;
    }
    else {
        s_math_error_code = 0x16u;
    }
}

// FUNCTION: LEGORR 0x00496c00
uint32_t crt_win32_file_descriptor_seek(uint32_t fd, int32_t distance,
                                        uint32_t method)
{
    unsigned char* block;
    unsigned char* entry;
    int handle;
    if (fd < (uint32_t)s_fd_flag_count) {
        block = (unsigned char*)s_fd_flag_blocks[fd >> 5];
        entry = block + (fd & 0x1fu) * 8u;
        if ((entry[4] & 1u) != 0u) {
            handle = crt_file_descriptor_handle_get(fd);
            if (handle == -1) {
                s_math_error_code = 9u;
                return 0xffffffffu;
            }
#if defined(LEGORRR_BACKEND_WINAPI)
            {
                DWORD result = SetFilePointer((HANDLE)handle, (LONG)distance,
                                              NULL, (DWORD)method);
                DWORD error = (result == 0xffffffffu) ? GetLastError() : 0u;
                if (error != 0u) {
                    crt_win32_error_map_to_errno(error);
                    return 0xffffffffu;
                }
                entry[4] = (unsigned char)(entry[4] & 0xfdu);
                return (uint32_t)result;
            }
#elif !defined(_MSC_VER)
            {
                int whence = (method == 0u) ? SEEK_SET :
                             (method == 1u) ? SEEK_CUR : SEEK_END;
                off_t result = lseek(handle, (off_t)distance, whence);
                if (result == (off_t)-1) {
                    s_math_error_code = (uint32_t)errno;
                    s_win32_error_code = 0u;
                    return 0xffffffffu;
                }
                entry[4] = (unsigned char)(entry[4] & 0xfdu);
                return (uint32_t)result;
            }
#else
            (void)distance;
            (void)method;
            s_math_error_code = 9u;
            s_win32_error_code = 0u;
            return 0xffffffffu;
#endif
        }
    }
    {
        s_math_error_code = 9u;
        s_win32_error_code = 0u;
        return 0xffffffffu;
    }
}

// FUNCTION: LEGORR 0x00496cc0
int crt_win32_file_descriptor_write(uint32_t fd, char* buffer,
                                    uint32_t count)
{
    unsigned char* block;
    unsigned char* entry;
    unsigned char flags;
    int handle;
    uint32_t written_total;
    uint32_t inserted_crs;
    uint32_t os_error;
    if (fd >= (uint32_t)s_fd_flag_count) {
        s_math_error_code = 9u;
        s_win32_error_code = 0u;
        return -1;
    }
    block = (unsigned char*)s_fd_flag_blocks[fd >> 5];
    entry = block + (fd & 0x1fu) * 8u;
    flags = entry[4];
    if ((flags & 1u) == 0u) {
        s_math_error_code = 9u;
        s_win32_error_code = 0u;
        return -1;
    }
    if (count == 0u) {
        return 0;
    }
    if ((flags & 0x20u) != 0u) {
        crt_win32_file_descriptor_seek(fd, 0, 2u);
    }
    handle = *(int*)entry;
    written_total = 0u;
    inserted_crs = 0u;
    os_error = 0u;
    if ((entry[5] & 0x80u) == 0u) {
#if defined(LEGORRR_BACKEND_WINAPI)
        DWORD written = 0u;
        if (!WriteFile((HANDLE)handle, buffer, count, &written, NULL)) {
            os_error = GetLastError();
        }
        else {
            written_total = (uint32_t)written;
        }
#elif !defined(_MSC_VER)
        ssize_t written = write(handle, buffer, count);
        if (written < 0) {
            s_math_error_code = (uint32_t)errno;
            s_win32_error_code = 0u;
            return -1;
        }
        written_total = (uint32_t)written;
#else
        s_math_error_code = 9u;
        s_win32_error_code = 0u;
        return -1;
#endif
    }
    else {
        char translated[0x404];
        char* source = buffer;
        char* source_end = buffer + count;
        while (source < source_end) {
            char* out = translated;
            uint32_t out_count;
            do {
                char ch;
                if (source_end <= source) {
                    break;
                }
                ch = *source++;
                if (ch == '\n') {
                    *out++ = '\r';
                    ++inserted_crs;
                }
                *out++ = ch;
            } while (out - translated < 0x400);
            out_count = (uint32_t)(out - translated);
#if defined(LEGORRR_BACKEND_WINAPI)
            {
                DWORD written = 0u;
                if (!WriteFile((HANDLE)handle, translated, out_count,
                               &written, NULL)) {
                    os_error = GetLastError();
                    break;
                }
                written_total += (uint32_t)written;
                if (written < out_count || source_end <= source) {
                    break;
                }
            }
#elif !defined(_MSC_VER)
            {
                ssize_t written = write(handle, translated, out_count);
                if (written < 0) {
                    s_math_error_code = (uint32_t)errno;
                    s_win32_error_code = 0u;
                    return -1;
                }
                written_total += (uint32_t)written;
                if ((uint32_t)written < out_count || source_end <= source) {
                    break;
                }
            }
#else
            s_math_error_code = 9u;
            s_win32_error_code = 0u;
            return -1;
#endif
        }
    }
    if (written_total != 0u) {
        return (int)(written_total - inserted_crs);
    }
    if (os_error == 0u) {
        if ((entry[4] & 0x40u) != 0u && buffer[0] == 0x1a) {
            return 0;
        }
        s_math_error_code = 0x1cu;
        s_win32_error_code = 0u;
        return -1;
    }
    if (os_error == 5u) {
        s_win32_error_code = os_error;
        s_math_error_code = 9u;
        return -1;
    }
    crt_win32_error_map_to_errno(os_error);
    return -1;
}

/* CrtFileStreamClose (FUN_004908a0) is the CRT's buffered character
 * writer. The function name is inherited from the image's symbol map. */
// FUNCTION: LEGORR 0x004908a0
uint32_t crt_file_stream_close(uint32_t value, int* stream)
{
    uint32_t flags = (uint32_t)stream[3];
    uint32_t fd = (uint32_t)stream[4];
    int expected;
    int result = 0;
    if ((flags & 0x82u) == 0u || (flags & 0x40u) != 0u) {
        stream[3] = (int)(flags | 0x20u);
        return 0xffffffffu;
    }
    if ((flags & 1u) != 0u) {
        stream[1] = 0;
        if ((flags & 0x10u) == 0u) {
            stream[3] = (int)(flags | 0x20u);
            return 0xffffffffu;
        }
        *stream = stream[2];
        stream[3] = (int)(flags & 0xfffffffeu);
    }
    flags = (uint32_t)stream[3];
    stream[1] = 0;
    stream[3] = (int)((flags & 0xffffffefu) | 2u);
    if ((flags & 0x10cu) == 0u) {
        if (stream == s_standard_stream_record_0 ||
            stream == s_standard_stream_record_1) {
            if (crt_file_descriptor_flag40_get(fd) == 0u) {
                crt_stream_buffer_allocate((char*)stream);
            }
        }
        else {
            crt_stream_buffer_allocate((char*)stream);
        }
    }
    if (((uint32_t)stream[3] & 0x108u) == 0u) {
        expected = 1;
        result = crt_win32_file_descriptor_write(fd, (char*)&value, 1u);
    }
    else {
        int pending = *stream - stream[2];
        *stream = stream[2] + 1;
        stream[1] = stream[6] - 1;
        if (pending <= 0) { /* Corpus 0x49092a compares against zero (signed JLE). */
            unsigned char* entry;
            if (fd == 0xffffffffu) {
                entry = s_fd_fallback_record;
            }
            else if (fd < (uint32_t)s_fd_flag_count
                     && (fd >> 5) < CRT_FD_BLOCKS
                     && s_fd_flag_blocks[fd >> 5] != NULL) {
                entry = (unsigned char*)s_fd_flag_blocks[fd >> 5]
                      + (fd & 0x1fu) * 8u;
            }
            else {
                entry = NULL;
            }
            if (entry != NULL && (entry[4] & 0x20u) != 0u) {
                crt_win32_file_descriptor_seek(fd, 0, 2u);
            }
            *(unsigned char*)stream[2] = (unsigned char)value;
        }
        else {
            result = crt_win32_file_descriptor_write(fd, (char*)stream[2],
                                                      (uint32_t)pending);
            *(unsigned char*)stream[2] = (unsigned char)value;
        }
        expected = pending;
    }
    if (result != expected) {
        stream[3] |= 0x20;
        return 0xffffffffu;
    }
    return value & 0xffu;
}

// FUNCTION: LEGORR 0x0048fd20
uint32_t crt_flush_stream_buffer(int* stream)
{
    uint32_t flags = (uint32_t)stream[3];
    int pending;
    int result = 0;
    if ((flags & 3u) == 2u && (flags & 0x108u) != 0u) {
        pending = *stream - stream[2];
        if (pending > 0) {
            result = crt_win32_file_descriptor_write((uint32_t)stream[4],
                                                      (char*)stream[2],
                                                      (uint32_t)pending);
            flags = (uint32_t)stream[3];
            if (result == pending) {
                if ((flags & 0x80u) != 0u) {
                    stream[1] = 0;
                    stream[3] = (int)(flags & 0xfffffffdu);
                    *stream = stream[2];
                    return 0u;
                }
            }
            else {
                stream[3] = (int)(flags | 0x20u);
                result = -1;
            }
        }
    }
    stream[1] = 0;
    *stream = stream[2];
    return (uint32_t)result;
}

// FUNCTION: LEGORR 0x00491360
void crt_buffered_character_write(uint32_t value, int* stream, int* written)
{
    int available = stream[1] - 1;
    stream[1] = available;
    if (available >= 0) {
        *(unsigned char*)*stream = (unsigned char)value;
        value &= 0xffu;
        *stream += 1;
    }
    else {
        value = crt_file_stream_close(value, stream);
    }
    if (value == 0xffffffffu) {
        *written = -1;
        return;
    }
    *written += 1;
}

// FUNCTION: LEGORR 0x004913b0
void crt_write_repeated_char(uint32_t value, int count, int* stream,
                             int* written)
{
    for (;;) {
        if (count < 1) {
            return;
        }
        count = count - 1;
        crt_buffered_character_write(value, stream, written);
        if (*written == -1) {
            return;
        }
    }
}

// FUNCTION: LEGORR 0x004913f0
void crt_write_string_bounded(const char* text, int count, int* stream,
                              int* written)
{
    for (;;) {
        char ch;
        if (count < 1) {
            return;
        }
        count = count - 1;
        ch = *text;
        ++text;
        crt_buffered_character_write((uint32_t)(int32_t)ch, stream, written);
        if (*written == -1) {
            return;
        }
    }
}

/* --- the CRT control-flags / signal leaves (2026-10-25) --------------------
 * CrtControlFlagsTranslate (FUN_004977d0): the corpus's flag fold — bits
 * 0/2/3/4/5 cross over to 0x10/8/4/2/1 (bit 1 pairs back as 0x80000), the
 * bits 10/11 pair folds to 0x100/0x200/0x300 (zero stays), an unset bits
 * 8+9 pair answers 0x20000 (only the 0x200 pair answers 0x10000), and
 * bit 12 answers 0x40000. Pure bitwork.
 *
 * CrtWideCharToMultibyte (FUN_00498b70): the corpus converts one wide
 * char through the code-page API, its C-locale half (DAT_00549180 == 0,
 * the port's pinned locale per KeyToAscii) writing the low byte for any
 * char below 0x100. Deviations: the pinned-locale half leaves the
 * code-page branch unreachable (the unported locale-map chain), and the
 * failure half stores the corpus's EILSEQ-shaped 0x2a into the modeled
 * math-error slot (_DAT_005490d0) and answers -1. */
// FUNCTION: LEGORR 0x004977d0
uint32_t crt_control_flags_translate(uint32_t flags)
{
    uint32_t out = 0u;
    if ((flags & 0x01u) != 0u) {
        out = 0x10u;
    }
    if ((flags & 0x04u) != 0u) { out |= 0x08u; }
    if ((flags & 0x08u) != 0u) { out |= 0x04u; }
    if ((flags & 0x10u) != 0u) { out |= 0x02u; }
    if ((flags & 0x20u) != 0u) { out |= 0x01u; }
    if ((flags & 0x02u) != 0u) { out |= 0x80000u; }
    /* Corpus: the low word is masked once; the signed 0x400 gate routes
     * the greater pair values before checking 0x800 and then 0xc00. */
    const uint32_t low16 = flags & 0xffffu;
    const int pair2 = (int)(low16 & 0xc00u);
    if (pair2 <= 0x400) {
        if (pair2 == 0x400) {
            out |= 0x100u;
        }
    } else if (pair2 != 0x800) {
        if (pair2 == 0xc00) {
            out |= 0x300u;
        }
    } else {
        out |= 0x200u;
    }
    if ((low16 & 0x300u) != 0u) {
        if ((low16 & 0x300u) == 0x200u) {
            out |= 0x10000u;
        }
    } else {
        out |= 0x20000u;
    }
    if ((flags & 0x1000u) != 0u) {
        out |= 0x40000u;
    }
    return out;
}

// FUNCTION: LEGORR 0x00497770
uint32_t crt_fpu_control_word_update(uint32_t value, uint32_t mask)
{
    const uint32_t translated = crt_control_flags_translate(value);
    const uint32_t result = (mask & value) | (~mask & translated);
    crt_no_op_stub_variant();
    return result;
}

// FUNCTION: LEGORR 0x004977b0
void crt_fpu_control_word_update_mask(uint32_t value, uint32_t mask)
{
    crt_fpu_control_word_update(value, mask & 0xfff7ffffu);
}

// FUNCTION: LEGORR 0x00498b70
int crt_wide_char_to_multibyte(char* dest, uint32_t wide)
{
    uint16_t ch = (uint16_t)wide;
    if (dest == NULL) {
        return 0; /* pinned: the corpus's null check answers 0 */
    }
    if (s_crt_locale_handle == 0u) {
        if (ch <= 0xffu) {
            *dest = (char)ch;
            return 1;
        }
    }
#if defined(LEGORRR_BACKEND_WINAPI)
    else {
        BOOL used_default = FALSE;
        int converted = WideCharToMultiByte((UINT)s_ansi_codepage, 0x220u,
                                            (LPCWSTR)&ch, 1, dest,
                                            (int)s_max_multibyte_width,
                                            NULL, &used_default);
        if (converted != 0 && used_default == FALSE) {
            return converted;
        }
    }
#endif
    s_math_error_code = 0x2au; /* pinned: the corpus's EILSEQ-shaped store */
    return -1;
}

/* CrtStrncpy (library `_strncpy`, FUN_0048e770) — see crt.hpp. The corpus's
 * aligned-dword fast path is an implementation detail: every lane copies
 * source bytes up to the count, copies the NUL when it lands, and pads the
 * rest of the window with zeros. The fold is that observable behavior;
 * source reads stay inside the count. */
// LIBRARY: LEGORR 0x0048e770 SYMBOL
// _strncpy
char* crt_string_copy_n(char* dest, const char* src, uint32_t count)
{
    if (count == 0u) {
        return dest;
    }
    uint32_t i = 0u;
    for (; i < count; ++i) {
        const char c = src[i];
        dest[i] = c;
        if (c == '\0') {
            ++i;
            break;
        }
    }
    for (; i < count; ++i) {
        dest[i] = 0;
    }
    return dest;
}

/* CrtWideCharToMultiByteCompat (FUN_0049abe0) — see crt.hpp. Pinned to the
 * port's C-locale half (the DAT_00549180 locale half rides the recorded
 * locale-map chain, as in the string-case and codepage leaves): the
 * dest-NULL lane answers the wide length, and the store lane copies low
 * bytes until the NUL or the count. The wide-over-0xff exit takes the
 * corpus's EILSEQ-shaped math-error store and answers -1. */
// FUNCTION: LEGORR 0x0049abe0
int crt_wide_char_to_multi_byte_compat(char* dest, const uint16_t* wide,
                                       uint32_t count)
{
    if (dest != NULL && count == 0u) {
        return 0;
    }
    if (dest == NULL) {
        return wide_string_length(wide);
    }
    {
        uint32_t i = 0u;
        for (;;) {
            const uint32_t ch = wide[i];
            if (ch >= 0x100u) {
                s_math_error_code = 0x2au;
                return -1;
            }
            dest[i] = (char)ch;
            if (ch == 0u) {
                return (int)i;
            }
            ++i;
            if (count <= i) {
                return (int)i;
            }
        }
    }
}

/* SignalHandlerRecordFind (FUN_0049ab90): linear search of the corpus's
 * signal-handler records — 12-byte entries (word, id, handler) in the
 * DAT_004b22a8 table, bounded by the DAT_004b2328 count. The corpus loads
 * the first record's id (0x49ab94) BEFORE the count (0x49ab9b) and forms
 * the count end with `lea [esi+esi*2]` / `lea [ecx*4+table]` at both the
 * loop and final checks; the port mirrors the load order and keeps one
 * cached end (LP64-portable). The table's writers sit in the unported CRT
 * startup band: own storage behind the install hooks below (the
 * unowned-global convention). The LP64 host has a 16-byte record stride;
 * x86 uses 12. */
#define CRT_SIGNAL_RECORDS 10
struct crt_signal_record {
    uint32_t  word0;
    int32_t   id;
    uintptr_t handler;
};
// GLOBAL: LEGORR 0x004b22a8
static struct crt_signal_record s_signal_records[CRT_SIGNAL_RECORDS] = {
    { 0xc0000005u, 11, 0u },
    { 0xc000001du, 4, 0u },
    { 0xc0000096u, 4, 0u },
    { 0xc000008du, 8, 0u },
    { 0xc000008eu, 8, 0u },
    { 0xc000008fu, 8, 0u },
    { 0xc0000090u, 8, 0u },
    { 0xc0000091u, 8, 0u },
    { 0xc0000092u, 8, 0u },
    { 0xc0000093u, 8, 0u }
};
// GLOBAL: LEGORR 0x004b2328
static int s_signal_record_count = CRT_SIGNAL_RECORDS;

// FUNCTION: LEGORR 0x0049ab90
void* crt_signal_handler_record_find(int signal)
{
    struct crt_signal_record* found = &s_signal_records[0];
    int first_id = found->id;
    struct crt_signal_record* end = s_signal_records + s_signal_record_count;
    if (first_id != signal) {
        struct crt_signal_record* previous = found;
        do {
            found = previous + 1;
            if (end <= found) {
                break;
            }
            previous = found;
        } while (previous->id != signal);
    }
    if (end <= found || found->id != signal) {
        return NULL;
    }
    return found;
}

void crt_signal_record_install(int index, uint32_t word0, int signal,
                               const void* handler)
{
    if (index < 0 || index >= CRT_SIGNAL_RECORDS) {
        return; /* deviation: the corpus strides the table */
    }
    s_signal_records[index].word0 = word0;
    s_signal_records[index].id = signal;
    s_signal_records[index].handler = (uintptr_t)handler;
}

void crt_signal_record_peek(int index, uint32_t* out_word0, int* out_id,
                            const void** out_handler)
{
    if (index < 0 || index >= CRT_SIGNAL_RECORDS) {
        return; /* deviation: the corpus strides the table */
    }
    if (out_word0 != NULL) {
        *out_word0 = s_signal_records[index].word0;
    }
    if (out_id != NULL) {
        *out_id = s_signal_records[index].id;
    }
    if (out_handler != NULL) {
        *out_handler = (const void*)s_signal_records[index].handler;
    }
}

void crt_signal_record_count_set(int count)
{
    s_signal_record_count = count > CRT_SIGNAL_RECORDS
                                ? CRT_SIGNAL_RECORDS
                                : (count < 0 ? 0 : count);
}

int crt_signal_record_count_peek(void)
{
    return s_signal_record_count;
}

/* DoubleFrexpDecompose (FUN_00493ae0): normalize the stored significand to
 * the corpus's exponent 0x3fe, then report the exponent such that
 * input = result * 2^exponent. Subnormals are shifted until bit 52 is set. */
// FUNCTION: LEGORR 0x00493ae0
double double_frexp_decompose(uint32_t low_word, uint32_t high_word,
                              int* exponent)
{
    union {
        uint64_t bits;
        double value;
    } input, output;
    uint32_t exponent_bits;
    uint64_t fraction;
    int result_exponent;

    input.bits = ((uint64_t)high_word << 32) | low_word;
    exponent_bits = (high_word >> 20) & 0x7ffu;
    fraction = input.bits & CRT_U64(0x000fffffffffffff);
    if (exponent_bits == 0u && fraction == 0u) {
        *exponent = 0;
        return 0.0;
    }
    if (exponent_bits == 0u) {
        result_exponent = -1021;
        while ((fraction & CRT_U64(0x0010000000000000)) == 0u) {
            fraction <<= 1;
            --result_exponent;
        }
        fraction &= CRT_U64(0x000fffffffffffff);
        output.bits = ((uint64_t)(high_word & 0x80000000u) << 32) |
                      ((uint64_t)0x3feu << 52) | fraction;
    }
    else {
        result_exponent = (int)exponent_bits - 0x3fe;
        output.bits = (input.bits & CRT_U64(0x800fffffffffffff)) |
                      ((uint64_t)0x3feu << 52);
    }
    *exponent = result_exponent;
    return output.value;
}

/* CrtMathExceptionDispatch (FUN_004938f0) — see crt.hpp. The registered
 * image handlers currently resolve to the port's FLH no-op, so a found
 * handler follows the corpus's unhandled result/error-code branch. */
// FUNCTION: LEGORR 0x004938f0
double crt_math_exception_dispatch(uint32_t error_code, uint32_t operation,
                                   uint32_t argument1_low,
                                   uint32_t argument1_high,
                                   uint32_t argument2_low,
                                   uint32_t argument2_high,
                                   uint32_t result_low,
                                   uint32_t result_high,
                                   uint32_t fpu_control_word)
{
    union {
        uint64_t bits;
        double value;
    } result;
    uint32_t handler;
    int handled;

    handler = crt_math_exception_handler_get((int)operation);
    if (handler != 0u) {
        handled = (int)flh_magic_uu_noop();
    }
    else {
        handled = 0;
    }
    (void)fpu_control_word_update_masked(fpu_control_word, 0xffffu);
    if (handled == 0) {
        crt_math_error_code_set((int)error_code);
    }
    result.bits = ((uint64_t)result_high << 32) | result_low;
    return result.value;
}

/* CrtFloatingPointExceptionRaise (FUN_00493280): build the CRT's 21-word
 * FPIEEE exception record, raise the corresponding Win32 floating-point
 * exception, then fold any handler changes back into the control word and
 * result. The caller owns and initializes record word zero before entry. */
// FUNCTION: LEGORR 0x00493280
void crt_floating_point_exception_raise(uint32_t* record,
                                        uint32_t* control_word,
                                        uint8_t exception_flags,
                                        uint32_t precision,
                                        const uint32_t* argument,
                                        uint32_t* result)
{
    uint32_t exception_code = (uint32_t)(uintptr_t)record;
    uint32_t status_word;
    uint32_t flags;
    uint32_t rounding;
    uint32_t precision_code;
#if defined(LEGORRR_BACKEND_WINAPI)
    DWORD exception_argument;
#endif

    record[1] = 0u;
    record[2] = 0u;
    record[3] = 0u;
    if ((exception_flags & 0x10u) != 0u) {
        record[1] |= 1u;
        exception_code = 0xc000008fu;
    }
    if ((exception_flags & 0x02u) != 0u) {
        record[1] |= 2u;
        exception_code = 0xc0000093u;
    }
    if ((exception_flags & 0x01u) != 0u) {
        record[1] |= 4u;
        exception_code = 0xc0000091u;
    }
    if ((exception_flags & 0x04u) != 0u) {
        record[1] |= 8u;
        exception_code = 0xc000008eu;
    }
    if ((exception_flags & 0x08u) != 0u) {
        record[1] |= 0x10u;
        exception_code = 0xc0000090u;
    }

    flags = *control_word;
    if ((flags & 1u) == 0u) { record[2] |= 0x10u; }
    if ((flags & 4u) == 0u) { record[2] |= 0x08u; }
    if ((flags & 8u) == 0u) { record[2] |= 0x04u; }
    if ((flags & 0x10u) == 0u) { record[2] |= 0x02u; }
    if ((flags & 0x20u) == 0u) { record[2] |= 0x01u; }

    status_word = (uint32_t)fpu_status_word_get_variant_b();
    if ((status_word & 0x0001u) != 0u) { record[3] |= 0x10u; }
    if ((status_word & 0x0004u) != 0u) { record[3] |= 0x08u; }
    if ((status_word & 0x0008u) != 0u) { record[3] |= 0x04u; }
    if ((status_word & 0x0010u) != 0u) { record[3] |= 0x02u; }
    if ((status_word & 0x0020u) != 0u) { record[3] |= 0x01u; }

    rounding = flags & 0x0c00u;
    if (rounding == 0x0400u) {
        record[0] = (record[0] & ~3u) | 1u;
    } else if (rounding == 0x0800u) {
        record[0] = (record[0] & ~3u) | 2u;
    } else if (rounding == 0x0c00u) {
        record[0] |= 3u;
    } else {
        record[0] &= ~3u;
    }
    rounding = flags & 0x0300u;
    if (rounding == 0u) {
        record[0] = (record[0] & ~0x14u) | 8u;
    } else if (rounding == 0x0200u) {
        record[0] = (record[0] & ~0x18u) | 4u;
    } else if (rounding == 0x0300u) {
        record[0] &= ~0x1cu;
    }
    record[0] = (record[0] & 0xfffe001fu) | ((precision & 0xfffu) << 5);
    record[8] = (record[8] | 1u) & ~0x1cu | 2u;
    record[4] = argument[0];
    record[5] = argument[1];
    record[20] = (record[20] | 1u) & ~0x1cu | 2u;
    record[16] = result[0];
    record[17] = result[1];

#if defined(LEGORRR_BACKEND_WINAPI)
    exception_argument = (DWORD)record;
    RaiseException((DWORD)exception_code, 0u, 1u, &exception_argument);
#else
    (void)exception_code;
#endif

    flags = *control_word;
    if ((record[2] & 0x10u) != 0u) { flags &= ~1u; }
    if ((record[2] & 0x08u) != 0u) { flags &= ~4u; }
    if ((record[2] & 0x04u) != 0u) { flags &= ~8u; }
    if ((record[2] & 0x02u) != 0u) { flags &= ~0x10u; }
    if ((record[2] & 0x01u) != 0u) { flags &= ~0x20u; }

    switch (record[0] & 3u) {
    case 0u: flags &= ~0x0c00u; break;
    case 1u: flags = (flags & ~0x0800u) | 0x0400u; break;
    case 2u: flags = (flags & ~0x0400u) | 0x0800u; break;
    default: flags |= 0x0c00u; break;
    }
    precision_code = (record[0] >> 2) & 7u;
    if (precision_code == 0u) {
        flags = (flags & ~0x0c00u) | 0x0300u;
    } else if (precision_code == 1u) {
        flags = (flags & ~0x0c00u) | 0x0200u;
    } else if (precision_code == 2u) {
        flags &= ~0x0c00u;
    }
    *control_word = flags;
    result[0] = record[16];
    result[1] = record[17];
}

/* CrtMathExceptionHandleSingleResult (FUN_00493170): the x86 corpus carries
 * this path's value on the x87 stack and keeps its argument loads and
 * cleanup in a specific order. Retain the C implementation as the
 * non-MSVC-x86 fallback. */
// FUNCTION: LEGORR 0x00493170
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) double crt_math_exception_handle_single_result(
    uint32_t operation, uint32_t result_low, uint32_t result_high,
    uint32_t fpu_control_word)
{
    __asm {
        mov eax, dword ptr [s_math_exception_mode]
        test eax, eax
        jne math_exception_mode_enabled

        mov eax, dword ptr [esp + 10h]
        mov ecx, dword ptr [esp + 8]
        push eax
        mov eax, dword ptr [esp + 10h]
        push eax
        push ecx
        push 0
        push 0
        push eax
        push ecx
        mov ecx, dword ptr [esp + 20h]
        push ecx
        push 1
        call crt_math_exception_dispatch
        add esp, 24h
        ret

    math_exception_mode_enabled:
        mov edx, dword ptr [esp + 10h]
        push 0ffffh
        push edx
        mov dword ptr [s_math_error_code], 21h
        call fpu_control_word_update_masked
        add esp, 8
        fld qword ptr [esp + 10h]
        add esp, 8
        ret
    }
}
#else
double crt_math_exception_handle_single_result(uint32_t operation,
                                                uint32_t result_low,
                                                uint32_t result_high,
                                                uint32_t fpu_control_word)
{
    union {
        uint64_t bits;
        double value;
    } result;

    if (s_math_exception_mode[0] == 0u) {
        return crt_math_exception_dispatch(1u, operation,
                                           result_low, result_high,
                                           0u, 0u,
                                           result_low, result_high,
                                           fpu_control_word);
    }

    s_math_error_code = 0x21u;
    (void)fpu_control_word_update_masked(fpu_control_word, 0xffffu);
    result.bits = ((uint64_t)result_high << 32) | result_low;
    return result.value;
}
#endif

/* CrtMathExceptionHandleResultWithStatus (FUN_004931c0): first fold
 * hardware exception status into the result, raise an FPIEEE exception for
 * any unhandled flags, then dispatch the CRT math-error callback or return
 * the result with the selected error/control-word state. */
// FUNCTION: LEGORR 0x004931c0
double crt_math_exception_handle_result_with_status(uint32_t status,
                                                     uint32_t operation,
                                                     uint32_t argument_low,
                                                     uint32_t argument_high,
                                                     uint32_t result_low,
                                                     uint32_t result_high,
                                                     uint32_t fpu_control_word)
{
    uint32_t record[21];
    uint32_t argument[2];
    uint32_t result_words[2];
    union {
        uint64_t bits;
        double value;
    } result;
    int error_code;

    result.bits = ((uint64_t)result_high << 32) | result_low;
    if (crt_double_to_integer_round(status, &result.value,
                                    fpu_control_word) == 0) {
        unsigned i;
        for (i = 0u; i < 21u; ++i) {
            record[i] = 0u;
        }
        argument[0] = argument_low;
        argument[1] = argument_high;
        result_words[0] = (uint32_t)result.bits;
        result_words[1] = (uint32_t)(result.bits >> 32);
        crt_floating_point_exception_raise(record, &fpu_control_word,
                                           (uint8_t)status, operation,
                                           argument, result_words);
        result.bits = ((uint64_t)result_words[1] << 32) | result_words[0];
    }

    error_code = crt_math_exception_code_from_status(status);
    if (s_math_exception_mode[0] == 0u && error_code != 0) {
        return crt_math_exception_dispatch((uint32_t)error_code, operation,
                                           argument_low, argument_high,
                                           0u, 0u,
                                           (uint32_t)result.bits,
                                           (uint32_t)(result.bits >> 32),
                                           fpu_control_word);
    }
    crt_math_error_code_set(error_code);
    (void)fpu_control_word_update_masked(fpu_control_word, 0xffffu);
    return result.value;
}

/* CrtDoubleToIntegerRound (FUN_004935c0): masked status bits are cleared;
 * integer conversion truncates toward zero, and overflow saturates according
 * to the saved x87 rounding mode. */
// FUNCTION: LEGORR 0x004935c0
int crt_double_to_integer_round(uint32_t status, double* value,
                                uint32_t fpu_control_word)
{
    union {
        double value;
        uint64_t bits;
    } number;
    uint32_t remaining = status & 0x1fu;
    uint32_t rounding_mode;

    number.value = *value;
    if ((remaining & 8u) != 0u && (fpu_control_word & 1u) != 0u) {
        crt_no_op_stub_variant2();
        remaining &= ~8u;
    }
    if ((remaining & 4u) != 0u && (fpu_control_word & 4u) != 0u) {
        crt_no_op_stub_variant2();
        remaining &= ~4u;
    }
    if ((remaining & 1u) != 0u) {
        if ((fpu_control_word & 8u) != 0u) {
            crt_no_op_stub_variant2();
            remaining &= ~1u;
        }
        else {
            union {
                uint64_t bits;
                double value;
            } saturation;
            int negative = (number.bits >> 63) != 0u;
            rounding_mode = fpu_control_word & 0x0c00u;
            if (rounding_mode == 0x0400u) {
                if (number.value <= 0.0) {
                    saturation.bits = CRT_U64(0xfff0000000000000);
                }
                else {
                    saturation.bits = CRT_U64(0x7fefffffffffffff);
                }
            }
            else if (rounding_mode == 0x0800u) {
                if (number.value > 0.0) {
                    saturation.bits = CRT_U64(0x7ff0000000000000);
                }
                else {
                    saturation.bits = CRT_U64(0xffefffffffffffff);
                }
            }
            else if (rounding_mode == 0x0c00u) {
                saturation.bits = negative ? CRT_U64(0xffefffffffffffff)
                                           : CRT_U64(0x7fefffffffffffff);
            }
            else {
                saturation.bits = negative ? CRT_U64(0xfff0000000000000)
                                           : CRT_U64(0x7ff0000000000000);
            }
            *value = saturation.value;
            remaining &= ~1u;
        }
    }
    if ((remaining & 2u) != 0u && (fpu_control_word & 0x10u) != 0u) {
        int binary_exponent;
        double normalized;
        union {
            double value;
            uint64_t bits;
        } significand;
        int inexact;

        /* Overflow handling above may have replaced the caller's value with
         * a saturated result; the underflow lane operates on that current
         * result, as the CRT does. */
        number.value = *value;
        inexact = (status & 0x10u) != 0u;
        if (number.value == 0.0) {
            inexact = 1;
        }
        else {
            normalized = double_frexp_decompose(
                (uint32_t)number.bits, (uint32_t)(number.bits >> 32),
                &binary_exponent);
            binary_exponent -= 0x600;
            if (binary_exponent < -0x432) {
                inexact = 1;
                number.bits = 0u;
                *value = number.value;
            }
            else {
                union {
                    double value;
                    uint64_t bits;
                } normalized_bits;
                uint64_t mantissa;
                normalized_bits.value = normalized;
                mantissa = (normalized_bits.bits &
                            CRT_U64(0x000fffffffffffff)) |
                           CRT_U64(0x0010000000000000);
                significand.value = (double)(int64_t)mantissa;
                if (binary_exponent < -0x3fd) {
                    int shift_count = -0x3fd - binary_exponent;
                    while (shift_count != 0) {
                        if ((significand.bits & 1u) != 0u && !inexact) {
                            inexact = 1;
                        }
                        significand.bits >>= 1;
                        --shift_count;
                    }
                }
                if (normalized < 0.0) {
                    significand.value = -significand.value;
                }
                *value = significand.value;
            }
        }
        if (inexact) {
            crt_no_op_stub_variant2();
        }
        remaining &= ~2u;
    }
    if ((remaining & 0x10u) != 0u && (fpu_control_word & 0x20u) != 0u) {
        crt_no_op_stub_variant2();
        remaining &= ~0x10u;
    }
    return remaining == 0u;
}
