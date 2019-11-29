
/* auto-generated by gen_syscalls.py, don't edit */
#ifndef Z_INCLUDE_SYSCALLS_ADC_H
#define Z_INCLUDE_SYSCALLS_ADC_H


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

extern int z_impl_adc_channel_setup(struct device * dev, const struct adc_channel_cfg * channel_cfg);
static inline int adc_channel_setup(struct device * dev, const struct adc_channel_cfg * channel_cfg)
{
#ifdef CONFIG_USERSPACE
	if (z_syscall_trap()) {
		return (int) arch_syscall_invoke2(*(uintptr_t *)&dev, *(uintptr_t *)&channel_cfg, K_SYSCALL_ADC_CHANNEL_SETUP);
	}
#endif
	compiler_barrier();
	return z_impl_adc_channel_setup(dev, channel_cfg);
}


extern int z_impl_adc_read(struct device * dev, const struct adc_sequence * sequence);
static inline int adc_read(struct device * dev, const struct adc_sequence * sequence)
{
#ifdef CONFIG_USERSPACE
	if (z_syscall_trap()) {
		return (int) arch_syscall_invoke2(*(uintptr_t *)&dev, *(uintptr_t *)&sequence, K_SYSCALL_ADC_READ);
	}
#endif
	compiler_barrier();
	return z_impl_adc_read(dev, sequence);
}


extern int z_impl_adc_read_async(struct device * dev, const struct adc_sequence * sequence, struct k_poll_signal * async);
static inline int adc_read_async(struct device * dev, const struct adc_sequence * sequence, struct k_poll_signal * async)
{
#ifdef CONFIG_USERSPACE
	if (z_syscall_trap()) {
		return (int) arch_syscall_invoke3(*(uintptr_t *)&dev, *(uintptr_t *)&sequence, *(uintptr_t *)&async, K_SYSCALL_ADC_READ_ASYNC);
	}
#endif
	compiler_barrier();
	return z_impl_adc_read_async(dev, sequence, async);
}


#ifdef __cplusplus
}
#endif

#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic pop
#endif

#endif
#endif /* include guard */
