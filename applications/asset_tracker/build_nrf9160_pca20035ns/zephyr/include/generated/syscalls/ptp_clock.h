
/* auto-generated by gen_syscalls.py, don't edit */
#ifndef Z_INCLUDE_SYSCALLS_PTP_CLOCK_H
#define Z_INCLUDE_SYSCALLS_PTP_CLOCK_H


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

extern int z_impl_ptp_clock_get(struct device * dev, struct net_ptp_time * tm);
static inline int ptp_clock_get(struct device * dev, struct net_ptp_time * tm)
{
#ifdef CONFIG_USERSPACE
	if (z_syscall_trap()) {
		return (int) z_arch_syscall_invoke2(*(u32_t *)&dev, *(u32_t *)&tm, K_SYSCALL_PTP_CLOCK_GET);
	}
#endif
	compiler_barrier();
	return z_impl_ptp_clock_get(dev, tm);
}


#ifdef __cplusplus
}
#endif

#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic pop
#endif

#endif
#endif /* include guard */
