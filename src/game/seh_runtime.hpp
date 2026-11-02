// SPDX-License-Identifier: LGPL-3.0-or-later
#ifndef LEGORRR_GAME_SEH_RUNTIME_HPP
#define LEGORRR_GAME_SEH_RUNTIME_HPP

#include <stdint.h>

namespace unported_ledger_stubs {

typedef struct SehExceptionRecord {
    uint32_t code;
    uint32_t flags;
} SehExceptionRecord;

typedef struct SehCrtExceptionFrame SehCrtExceptionFrame;

typedef struct SehRuntimeHooks {
    void* user;
    void** registration_head_slot;
    void (*rtl_unwind)(void* target_frame, void* target_ip,
                       SehExceptionRecord* exception_record,
                       void* return_value, void* user);
    uint32_t (*exception_dispatch)(uintptr_t argument_0,
        uintptr_t argument_1, uintptr_t argument_2, uintptr_t argument_3,
        uintptr_t argument_4, uintptr_t argument_5, uintptr_t argument_6,
        uintptr_t argument_7, void* user);
    uint32_t (*call_setting_frame)(void* callback, uintptr_t argument_a,
        uintptr_t argument_b, void* frame, void* user);
    uint32_t (*continuation_invoke)(uintptr_t continuation, void* user);
    void (*exception_install_dispatch)(void* type_info,
        SehCrtExceptionFrame* frame, uintptr_t argument_2,
        uintptr_t argument_3, uintptr_t argument_4, uintptr_t argument_5,
        uintptr_t argument_6, uintptr_t argument_7, void* user);
} SehRuntimeHooks;

struct SehCrtExceptionFrame {
    void* previous_frame;
    uintptr_t argument_8;
    uintptr_t argument_c;
    uintptr_t argument_10;
    uintptr_t argument_14;
    uintptr_t continuation;
    uint32_t unwind_in_progress;
};

void seh_runtime_hooks_set(const SehRuntimeHooks* hooks);
void seh_frame_unwind(void* target_frame,
                      SehExceptionRecord* exception_record); /* FUN_0048ec00 */
uint32_t call_setting_frame_12(uintptr_t argument_1, uintptr_t argument_2,
    uintptr_t callback, int frame_index, uintptr_t argument_5);
void crt_exception_frame_filter_invoke(uintptr_t exception,
    SehCrtExceptionFrame* frame, uintptr_t argument_3); /* FUN_0048ed00 */
uint32_t crt_exception_scope_filter_and_unwind(
    SehExceptionRecord* exception, SehCrtExceptionFrame* frame,
    uintptr_t argument_3); /* FUN_0048ee00 */
uint32_t crt_exception_frame_install_and_dispatch(
    const void* type_info, uintptr_t argument_2, uintptr_t argument_3,
    uintptr_t argument_4, uintptr_t argument_5, uintptr_t argument_6,
    uintptr_t argument_7); /* FUN_0048ed30 */
void* seh_scope_table_lookup(const void* registration_frame,
                             int nesting_depth, uint32_t control_pc,
                             uint32_t* scope_index_out,
                             uint32_t* parent_index_out); /* FUN_0048ee90 */

} // namespace unported_ledger_stubs

#endif
