// SPDX-License-Identifier: LGPL-3.0-or-later
#include "seh_runtime.hpp"

#include <string.h>

namespace unported_ledger_stubs {

static SehRuntimeHooks s_seh_runtime_hooks;

void seh_runtime_hooks_set(const SehRuntimeHooks* hooks)
{
    if (hooks == NULL) memset(&s_seh_runtime_hooks, 0,
                              sizeof(s_seh_runtime_hooks));
    else s_seh_runtime_hooks = *hooks;
}

// FUNCTION: LEGORR 0x0048ec00
void seh_frame_unwind(void* target_frame,
                      SehExceptionRecord* exception_record)
{
    void* current_frame = NULL;
    void** head_slot = s_seh_runtime_hooks.registration_head_slot;
    if (head_slot != NULL) current_frame = *head_slot;
    if (s_seh_runtime_hooks.rtl_unwind != NULL)
        s_seh_runtime_hooks.rtl_unwind(target_frame,
            (void*)(uintptr_t)0x0048ec2cu, exception_record, NULL,
            s_seh_runtime_hooks.user);
    if (exception_record != NULL) exception_record->flags &= ~2u;
    if (head_slot != NULL && current_frame != NULL) {
        *(void**)current_frame = *head_slot;
        *head_slot = current_frame;
    }
}

// FUNCTION: LEGORR 0x0048eca0
uint32_t call_setting_frame_12(uintptr_t argument_1, uintptr_t argument_2,
    uintptr_t callback, int frame_index, uintptr_t argument_5)
{
    SehCrtExceptionFrame frame;
    void** head_slot = s_seh_runtime_hooks.registration_head_slot;
    uint32_t result = 0u;
    frame.previous_frame = head_slot != NULL ? *head_slot : NULL;
    frame.argument_8 = (uintptr_t)(frame_index + 1);
    frame.argument_c = argument_1;
    frame.argument_10 = argument_2;
    frame.argument_14 = argument_5;
    frame.continuation = callback;
    frame.unwind_in_progress = 0u;
    if (head_slot != NULL) *head_slot = &frame;
    if (s_seh_runtime_hooks.call_setting_frame != NULL)
        result = s_seh_runtime_hooks.call_setting_frame(
            (void*)crt_exception_frame_filter_invoke, argument_1,
            argument_5, &frame, s_seh_runtime_hooks.user);
    if (head_slot != NULL) *head_slot = frame.previous_frame;
    return result;
}

// FUNCTION: LEGORR 0x0048ed00
void crt_exception_frame_filter_invoke(uintptr_t exception,
    SehCrtExceptionFrame* frame, uintptr_t argument_3)
{
    if (frame != NULL &&
        s_seh_runtime_hooks.exception_dispatch != NULL) {
        (void)s_seh_runtime_hooks.exception_dispatch(exception,
            frame->argument_c, argument_3, 0u, frame->argument_8,
            frame->argument_10, (uintptr_t)frame, 0u,
            s_seh_runtime_hooks.user);
    }
}

// FUNCTION: LEGORR 0x0048ee00
uint32_t crt_exception_scope_filter_and_unwind(
    SehExceptionRecord* exception, SehCrtExceptionFrame* frame,
    uintptr_t argument_3)
{
    uint32_t result = 0u;
    if (exception == NULL || frame == NULL) return 0u;
    if ((exception->flags & 0x66u) != 0u) {
        frame->unwind_in_progress = 1u;
        return 1u;
    }
    if (s_seh_runtime_hooks.exception_dispatch != NULL) {
        result = s_seh_runtime_hooks.exception_dispatch(
            (uintptr_t)exception, frame->argument_c, argument_3, 0u,
            frame->argument_8, frame->argument_10, frame->argument_14,
            1u, s_seh_runtime_hooks.user);
    }
    if (frame->unwind_in_progress == 0u)
        seh_frame_unwind(frame, exception);
    if (s_seh_runtime_hooks.continuation_invoke != NULL)
        return s_seh_runtime_hooks.continuation_invoke(frame->continuation,
            s_seh_runtime_hooks.user);
    return result;
}

// FUNCTION: LEGORR 0x0048ed30
uint32_t crt_exception_frame_install_and_dispatch(
    const void* type_info, uintptr_t argument_2, uintptr_t argument_3,
    uintptr_t argument_4, uintptr_t argument_5, uintptr_t argument_6,
    uintptr_t argument_7)
{
    SehCrtExceptionFrame frame;
    void** head_slot = s_seh_runtime_hooks.registration_head_slot;
    frame.previous_frame = head_slot != NULL ? *head_slot : NULL;
    frame.argument_8 = argument_2;
    frame.argument_c = (uintptr_t)type_info;
    frame.argument_10 = argument_3;
    frame.argument_14 = argument_4;
    frame.continuation = 0x0048edcau;
    frame.unwind_in_progress = 0u;
    if (head_slot != NULL) *head_slot = &frame;
    if (s_seh_runtime_hooks.exception_install_dispatch != NULL)
        s_seh_runtime_hooks.exception_install_dispatch(
            type_info != NULL ? *(void* const*)type_info : NULL,
            &frame, argument_2, argument_3, argument_4, argument_5,
            argument_6, argument_7, s_seh_runtime_hooks.user);
    if (head_slot != NULL) {
        if (frame.unwind_in_progress == 0u)
            *head_slot = frame.previous_frame;
        else if (frame.previous_frame != NULL) {
            *(void**)frame.previous_frame = *head_slot;
            *head_slot = frame.previous_frame;
        } else *head_slot = NULL;
    }
    return 0u;
}



/* SehScopeTableLookup (FUN_0048ee90): walk the nested active scope ranges
 * from the innermost table entry outwards. Invalid frame/table data returns
 * NULL in this portable model instead of invoking the CRT's fatal helper. */
// FUNCTION: LEGORR 0x0048ee90
void* seh_scope_table_lookup(const void* registration_frame,
                             int nesting_depth, uint32_t control_pc,
                             uint32_t* scope_index_out,
                             uint32_t* parent_index_out)
{
    const uint8_t* frame = (const uint8_t*)registration_frame;
    const uint8_t* entries;
    uint32_t count;
    uint32_t index;
    uint32_t previous;
    uint32_t last;
    if (scope_index_out != NULL) *scope_index_out = 0u;
    if (parent_index_out != NULL) *parent_index_out = 0u;
    if (frame == NULL || scope_index_out == NULL || parent_index_out == NULL)
        return NULL;
    memcpy(&count, frame + 0x0cu, sizeof(count));
    memcpy(&entries, frame + 0x10u, sizeof(entries));
    if (entries == NULL || count > 4096u) return NULL;

    index = count;
    previous = count;
    last = count;
    while (nesting_depth >= 0) {
        uint32_t start_pc;
        uint32_t end_pc;
        int matched;
        if (index == 0u) return NULL;
        --index;
        memcpy(&start_pc, entries + index * 0x14u + 4u,
               sizeof(start_pc));
        memcpy(&end_pc, entries + index * 0x14u + 8u,
               sizeof(end_pc));
        matched = start_pc < control_pc && control_pc <= end_pc;
        if (matched) {
            --nesting_depth;
            last = previous;
            previous = index;
        }
    }
    ++index;
    *scope_index_out = index;
    *parent_index_out = last;
    if (last > count || last < index) return NULL;
    return (void*)(entries + index * 0x14u);
}

} // namespace unported_ledger_stubs
