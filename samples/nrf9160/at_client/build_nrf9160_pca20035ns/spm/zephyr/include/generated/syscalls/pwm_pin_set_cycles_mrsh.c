/* auto-generated by gen_syscalls.py, don't edit */
#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic push
#endif
#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif
#include <syscalls/pwm.h>

extern int z_vrfy_pwm_pin_set_cycles(struct device * dev, u32_t pwm, u32_t period, u32_t pulse);
u32_t z_mrsh_pwm_pin_set_cycles(u32_t arg0, u32_t arg1, u32_t arg2,
		u32_t arg3, u32_t arg4, u32_t arg5, void *ssf)
{
	_current_cpu->syscall_frame = ssf;
	(void) arg4;	/* unused */
	(void) arg5;	/* unused */
	int ret = z_vrfy_pwm_pin_set_cycles(*(struct device **)&arg0, *(u32_t*)&arg1, *(u32_t*)&arg2, *(u32_t*)&arg3)
;
	return (u32_t) ret;
}

#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic pop
#endif
