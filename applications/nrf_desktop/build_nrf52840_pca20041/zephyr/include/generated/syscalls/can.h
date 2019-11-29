
/* auto-generated by gen_syscalls.py, don't edit */
#ifndef Z_INCLUDE_SYSCALLS_CAN_H
#define Z_INCLUDE_SYSCALLS_CAN_H


#ifndef _ASMLANGUAGE

#include <syscall_list.h>
#include <syscall_macros.h>

#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic push
#endif

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif

#ifdef __cplusplus
extern "C" {
#endif

extern int z_impl_can_send(struct device * dev, const struct zcan_frame * msg, s32_t timeout, can_tx_callback_t callback_isr, void * callback_arg);
static inline int can_send(struct device * dev, const struct zcan_frame * msg, s32_t timeout, can_tx_callback_t callback_isr, void * callback_arg)
{
#ifdef CONFIG_USERSPACE
	if (z_syscall_trap()) {
		return (int) arch_syscall_invoke5(*(uintptr_t *)&dev, *(uintptr_t *)&msg, *(uintptr_t *)&timeout, *(uintptr_t *)&callback_isr, *(uintptr_t *)&callback_arg, K_SYSCALL_CAN_SEND);
	}
#endif
	compiler_barrier();
	return z_impl_can_send(dev, msg, timeout, callback_isr, callback_arg);
}


extern int z_impl_can_attach_msgq(struct device * dev, struct k_msgq * msg_q, const struct zcan_filter * filter);
static inline int can_attach_msgq(struct device * dev, struct k_msgq * msg_q, const struct zcan_filter * filter)
{
#ifdef CONFIG_USERSPACE
	if (z_syscall_trap()) {
		return (int) arch_syscall_invoke3(*(uintptr_t *)&dev, *(uintptr_t *)&msg_q, *(uintptr_t *)&filter, K_SYSCALL_CAN_ATTACH_MSGQ);
	}
#endif
	compiler_barrier();
	return z_impl_can_attach_msgq(dev, msg_q, filter);
}


extern void z_impl_can_detach(struct device * dev, int filter_id);
static inline void can_detach(struct device * dev, int filter_id)
{
#ifdef CONFIG_USERSPACE
	if (z_syscall_trap()) {
		arch_syscall_invoke2(*(uintptr_t *)&dev, *(uintptr_t *)&filter_id, K_SYSCALL_CAN_DETACH);
		return;
	}
#endif
	compiler_barrier();
	z_impl_can_detach(dev, filter_id);
}


extern int z_impl_can_configure(struct device * dev, enum can_mode mode, u32_t bitrate);
static inline int can_configure(struct device * dev, enum can_mode mode, u32_t bitrate)
{
#ifdef CONFIG_USERSPACE
	if (z_syscall_trap()) {
		return (int) arch_syscall_invoke3(*(uintptr_t *)&dev, *(uintptr_t *)&mode, *(uintptr_t *)&bitrate, K_SYSCALL_CAN_CONFIGURE);
	}
#endif
	compiler_barrier();
	return z_impl_can_configure(dev, mode, bitrate);
}


extern enum can_state z_impl_can_get_state(struct device * dev, struct can_bus_err_cnt * err_cnt);
static inline enum can_state can_get_state(struct device * dev, struct can_bus_err_cnt * err_cnt)
{
#ifdef CONFIG_USERSPACE
	if (z_syscall_trap()) {
		return (enum can_state) arch_syscall_invoke2(*(uintptr_t *)&dev, *(uintptr_t *)&err_cnt, K_SYSCALL_CAN_GET_STATE);
	}
#endif
	compiler_barrier();
	return z_impl_can_get_state(dev, err_cnt);
}


extern int z_impl_can_recover(struct device * dev, s32_t timeout);
static inline int can_recover(struct device * dev, s32_t timeout)
{
#ifdef CONFIG_USERSPACE
	if (z_syscall_trap()) {
		return (int) arch_syscall_invoke2(*(uintptr_t *)&dev, *(uintptr_t *)&timeout, K_SYSCALL_CAN_RECOVER);
	}
#endif
	compiler_barrier();
	return z_impl_can_recover(dev, timeout);
}


#ifdef __cplusplus
}
#endif

#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic pop
#endif

#endif
#endif /* include guard */
