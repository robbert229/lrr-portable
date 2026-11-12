// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what interface uses from frontend.cpp.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by both files. */
#ifndef LRR_GAME_FRONTEND_FOR_INTERFACE_HPP
#define LRR_GAME_FRONTEND_FOR_INTERFACE_HPP

#if defined(LEGORRR_BACKEND_WINAPI)
#define MENU_ACTION_TABLE_ALLOC __declspec(allocate(".data$zzzzzz"))
#else
#define MENU_ACTION_TABLE_ALLOC
#endif

/* --- the cell-action permission window (see frontend.hpp) ------------------- */
#define CELL_PERMISSION_ENTRIES 300 /* corpus's 300-dword clear loop */

void frontend_status_flash_tick(float delta);
extern MENU_ACTION_TABLE_ALLOC const char* const s_interface_menu_action_labels[74];
extern char s_back_button_action[FE_NAME_MAX];
extern FeSurroundImage s_interface_surround_images[FE_SURROUND_IMAGES_MAX];
extern FeInterfaceImage s_interface_images[FE_INTERFACE_ACTION_COUNT];
extern FeInterfaceBuildImage* s_interface_build_images[2];
extern uint32_t s_interface_build_image_counts[2];
int s_strcaseeq(const char* a, const char* b);
extern float s_interface_menu_action_xy[2];
extern const void* s_interface_menu_action_context;
extern PositionTrackerBoxDrawOps s_position_tracker_box_draw_ops;
extern AdvisorDialogStackOps s_advisor_dialog_stack_ops;
extern uint32_t s_advisor_dialog_stack_width;
extern uint32_t s_advisor_dialog_stack_height;
extern FeAdvisorCatalogCallback s_advisor_callback_callback;
namespace unported_ledger_stubs {
extern CellGridPointHitTestState* s_cell_grid_point_hit_test_state;
} // namespace unported_ledger_stubs
extern UiActionDispatchState* s_ui_action_dispatch_state;
namespace unported_ledger_stubs {
extern SimulationPointerDispatchOps s_simulation_pointer_dispatch_ops;
} // namespace unported_ledger_stubs

#endif /* LRR_GAME_FRONTEND_FOR_INTERFACE_HPP */
