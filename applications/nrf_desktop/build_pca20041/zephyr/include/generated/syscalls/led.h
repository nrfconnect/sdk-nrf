
/* auto-generated by gen_syscalls.py, don't edit */
#ifndef Z_INCLUDE_SYSCALLS_LED_H
#define Z_INCLUDE_SYSCALLS_LED_H


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

extern int z_impl_led_blink(struct device * dev, u32_t led, u32_t delay_on, u32_t delay_off);
static inline int led_blink(struct device * dev, u32_t led, u32_t delay_on, u32_t delay_off)
{
#ifdef CONFIG_USERSPACE
	if (z_syscall_trap()) {
		return (int) arch_syscall_invoke4(*(uintptr_t *)&dev, *(uintptr_t *)&led, *(uintptr_t *)&delay_on, *(uintptr_t *)&delay_off, K_SYSCALL_LED_BLINK);
	}
#endif
	compiler_barrier();
	return z_impl_led_blink(dev, led, delay_on, delay_off);
}


extern int z_impl_led_set_brightness(struct device * dev, u32_t led, u8_t value);
static inline int led_set_brightness(struct device * dev, u32_t led, u8_t value)
{
#ifdef CONFIG_USERSPACE
	if (z_syscall_trap()) {
		return (int) arch_syscall_invoke3(*(uintptr_t *)&dev, *(uintptr_t *)&led, *(uintptr_t *)&value, K_SYSCALL_LED_SET_BRIGHTNESS);
	}
#endif
	compiler_barrier();
	return z_impl_led_set_brightness(dev, led, value);
}


extern int z_impl_led_on(struct device * dev, u32_t led);
static inline int led_on(struct device * dev, u32_t led)
{
#ifdef CONFIG_USERSPACE
	if (z_syscall_trap()) {
		return (int) arch_syscall_invoke2(*(uintptr_t *)&dev, *(uintptr_t *)&led, K_SYSCALL_LED_ON);
	}
#endif
	compiler_barrier();
	return z_impl_led_on(dev, led);
}


extern int z_impl_led_off(struct device * dev, u32_t led);
static inline int led_off(struct device * dev, u32_t led)
{
#ifdef CONFIG_USERSPACE
	if (z_syscall_trap()) {
		return (int) arch_syscall_invoke2(*(uintptr_t *)&dev, *(uintptr_t *)&led, K_SYSCALL_LED_OFF);
	}
#endif
	compiler_barrier();
	return z_impl_led_off(dev, led);
}


#ifdef __cplusplus
}
#endif

#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic pop
#endif

#endif
#endif /* include guard */
