/* auto-generated by gen_syscalls.py, don't edit */
#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic push
#endif
#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif
#include <syscalls/flash.h>

extern int z_vrfy_flash_read(struct device * dev, off_t offset, void * data, size_t len);
uintptr_t z_mrsh_flash_read(uintptr_t arg0, uintptr_t arg1, uintptr_t arg2,
		uintptr_t arg3, uintptr_t arg4, uintptr_t arg5, void *ssf)
{
	_current_cpu->syscall_frame = ssf;
	(void) arg4;	/* unused */
	(void) arg5;	/* unused */
	int ret = z_vrfy_flash_read(*(struct device **)&arg0, *(off_t*)&arg1, *(void **)&arg2, *(size_t*)&arg3)
;
	return (uintptr_t) ret;
}

#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic pop
#endif
