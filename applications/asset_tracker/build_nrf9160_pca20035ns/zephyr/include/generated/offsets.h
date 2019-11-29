/* THIS FILE IS AUTO GENERATED.  PLEASE DO NOT EDIT.
 *
 * This header file provides macros for the offsets of various structure
 * members.  These offset macros are primarily intended to be used in
 * assembly code.
 */

#ifndef __GEN_OFFSETS_H__
#define __GEN_OFFSETS_H__

#define ___kernel_t_current_OFFSET 0x8
#define ___kernel_t_nested_OFFSET 0x0
#define ___kernel_t_irq_stack_OFFSET 0x4
#define ___cpu_t_current_OFFSET 0x8
#define ___cpu_t_nested_OFFSET 0x0
#define ___cpu_t_irq_stack_OFFSET 0x4
#define ___kernel_t_idle_OFFSET 0x20
#define ___kernel_t_ready_q_OFFSET 0x24
#define ___ready_q_t_cache_OFFSET 0x0
#define ___kernel_t_current_fp_OFFSET 0x30
#define ___thread_base_t_user_options_OFFSET 0xc
#define ___thread_base_t_thread_state_OFFSET 0xd
#define ___thread_base_t_prio_OFFSET 0xe
#define ___thread_base_t_sched_locked_OFFSET 0xf
#define ___thread_base_t_preempt_OFFSET 0xe
#define ___thread_base_t_swap_data_OFFSET 0x14
#define ___thread_t_base_OFFSET 0x0
#define ___thread_t_callee_saved_OFFSET 0x28
#define ___thread_t_arch_OFFSET 0x64
#define ___thread_stack_info_t_start_OFFSET 0x0
#define ___thread_stack_info_t_size_OFFSET 0x4
#define ___thread_t_stack_info_OFFSET 0x58
#define K_THREAD_SIZEOF 0xb0
#define _DEVICE_STRUCT_SIZEOF 0xc
#define ___thread_arch_t_basepri_OFFSET 0x0
#define ___thread_arch_t_swap_return_value_OFFSET 0x4
#define ___thread_arch_t_mode_OFFSET 0x48
#define ___thread_arch_t_preempt_float_OFFSET 0x8
#define ___basic_sf_t_a1_OFFSET 0x0
#define ___basic_sf_t_a2_OFFSET 0x4
#define ___basic_sf_t_a3_OFFSET 0x8
#define ___basic_sf_t_a4_OFFSET 0xc
#define ___basic_sf_t_ip_OFFSET 0x10
#define ___basic_sf_t_lr_OFFSET 0x14
#define ___basic_sf_t_pc_OFFSET 0x18
#define ___basic_sf_t_xpsr_OFFSET 0x1c
#define ___esf_t_s_OFFSET 0x20
#define ___esf_t_fpscr_OFFSET 0x60
#define ___esf_t_SIZEOF 0x68
#define ___callee_saved_t_v1_OFFSET 0x0
#define ___callee_saved_t_v2_OFFSET 0x4
#define ___callee_saved_t_v3_OFFSET 0x8
#define ___callee_saved_t_v4_OFFSET 0xc
#define ___callee_saved_t_v5_OFFSET 0x10
#define ___callee_saved_t_v6_OFFSET 0x14
#define ___callee_saved_t_v7_OFFSET 0x18
#define ___callee_saved_t_v8_OFFSET 0x1c
#define ___callee_saved_t_psp_OFFSET 0x20
#define ___callee_saved_t_SIZEOF 0x24
#define _K_THREAD_NO_FLOAT_SIZEOF 0x70

#endif /* __GEN_OFFSETS_H__ */
