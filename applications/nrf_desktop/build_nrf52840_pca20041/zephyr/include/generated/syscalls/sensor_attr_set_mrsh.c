/* auto-generated by gen_syscalls.py, don't edit */
#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic push
#endif
#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif
#include <syscalls/sensor.h>

extern int z_vrfy_sensor_attr_set(struct device * dev, enum sensor_channel chan, enum sensor_attribute attr, const struct sensor_value * val);
uintptr_t z_mrsh_sensor_attr_set(uintptr_t arg0, uintptr_t arg1, uintptr_t arg2,
		uintptr_t arg3, uintptr_t arg4, uintptr_t arg5, void *ssf)
{
	_current_cpu->syscall_frame = ssf;
	(void) arg4;	/* unused */
	(void) arg5;	/* unused */
	int ret = z_vrfy_sensor_attr_set(*(struct device **)&arg0, *(enum sensor_channel*)&arg1, *(enum sensor_attribute*)&arg2, *(const struct sensor_value **)&arg3)
;
	return (uintptr_t) ret;
}

#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic pop
#endif
