/* auto-generated by gen_syscalls.py, don't edit */
#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic push
#endif
#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif
#include <syscalls/kernel.h>

extern u32_t z_vrfy_k_timer_remaining_get(struct k_timer * timer);
u32_t z_mrsh_k_timer_remaining_get(u32_t arg0, u32_t arg1, u32_t arg2,
		u32_t arg3, u32_t arg4, u32_t arg5, void *ssf)
{
	_current_cpu->syscall_frame = ssf;
	(void) arg1;	/* unused */
	(void) arg2;	/* unused */
	(void) arg3;	/* unused */
	(void) arg4;	/* unused */
	(void) arg5;	/* unused */
	u32_t ret = z_vrfy_k_timer_remaining_get(*(struct k_timer **)&arg0)
;
	return (u32_t) ret;
}

#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic pop
#endif
