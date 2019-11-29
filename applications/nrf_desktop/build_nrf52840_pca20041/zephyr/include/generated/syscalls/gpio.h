
/* auto-generated by gen_syscalls.py, don't edit */
#ifndef Z_INCLUDE_SYSCALLS_GPIO_H
#define Z_INCLUDE_SYSCALLS_GPIO_H


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

extern int z_impl_gpio_config(struct device * port, int access_op, u32_t pin, int flags);
static inline int gpio_config(struct device * port, int access_op, u32_t pin, int flags)
{
#ifdef CONFIG_USERSPACE
	if (z_syscall_trap()) {
		return (int) arch_syscall_invoke4(*(uintptr_t *)&port, *(uintptr_t *)&access_op, *(uintptr_t *)&pin, *(uintptr_t *)&flags, K_SYSCALL_GPIO_CONFIG);
	}
#endif
	compiler_barrier();
	return z_impl_gpio_config(port, access_op, pin, flags);
}


extern int z_impl_gpio_write(struct device * port, int access_op, u32_t pin, u32_t value);
static inline int gpio_write(struct device * port, int access_op, u32_t pin, u32_t value)
{
#ifdef CONFIG_USERSPACE
	if (z_syscall_trap()) {
		return (int) arch_syscall_invoke4(*(uintptr_t *)&port, *(uintptr_t *)&access_op, *(uintptr_t *)&pin, *(uintptr_t *)&value, K_SYSCALL_GPIO_WRITE);
	}
#endif
	compiler_barrier();
	return z_impl_gpio_write(port, access_op, pin, value);
}


extern int z_impl_gpio_read(struct device * port, int access_op, u32_t pin, u32_t * value);
static inline int gpio_read(struct device * port, int access_op, u32_t pin, u32_t * value)
{
#ifdef CONFIG_USERSPACE
	if (z_syscall_trap()) {
		return (int) arch_syscall_invoke4(*(uintptr_t *)&port, *(uintptr_t *)&access_op, *(uintptr_t *)&pin, *(uintptr_t *)&value, K_SYSCALL_GPIO_READ);
	}
#endif
	compiler_barrier();
	return z_impl_gpio_read(port, access_op, pin, value);
}


extern int z_impl_gpio_enable_callback(struct device * port, int access_op, u32_t pin);
static inline int gpio_enable_callback(struct device * port, int access_op, u32_t pin)
{
#ifdef CONFIG_USERSPACE
	if (z_syscall_trap()) {
		return (int) arch_syscall_invoke3(*(uintptr_t *)&port, *(uintptr_t *)&access_op, *(uintptr_t *)&pin, K_SYSCALL_GPIO_ENABLE_CALLBACK);
	}
#endif
	compiler_barrier();
	return z_impl_gpio_enable_callback(port, access_op, pin);
}


extern int z_impl_gpio_disable_callback(struct device * port, int access_op, u32_t pin);
static inline int gpio_disable_callback(struct device * port, int access_op, u32_t pin)
{
#ifdef CONFIG_USERSPACE
	if (z_syscall_trap()) {
		return (int) arch_syscall_invoke3(*(uintptr_t *)&port, *(uintptr_t *)&access_op, *(uintptr_t *)&pin, K_SYSCALL_GPIO_DISABLE_CALLBACK);
	}
#endif
	compiler_barrier();
	return z_impl_gpio_disable_callback(port, access_op, pin);
}


extern int z_impl_gpio_get_pending_int(struct device * dev);
static inline int gpio_get_pending_int(struct device * dev)
{
#ifdef CONFIG_USERSPACE
	if (z_syscall_trap()) {
		return (int) arch_syscall_invoke1(*(uintptr_t *)&dev, K_SYSCALL_GPIO_GET_PENDING_INT);
	}
#endif
	compiler_barrier();
	return z_impl_gpio_get_pending_int(dev);
}


#ifdef __cplusplus
}
#endif

#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic pop
#endif

#endif
#endif /* include guard */
