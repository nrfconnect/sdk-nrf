/* auto-generated by gen_syscalls.py, don't edit */
#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic push
#endif
#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif
#include <syscalls/kernel.h>

extern s32_t z_vrfy_k_queue_alloc_prepend(struct k_queue * queue, void * data);
u32_t z_mrsh_k_queue_alloc_prepend(u32_t arg0, u32_t arg1, u32_t arg2,
		u32_t arg3, u32_t arg4, u32_t arg5, void *ssf)
{
	_current_cpu->syscall_frame = ssf;
	(void) arg2;	/* unused */
	(void) arg3;	/* unused */
	(void) arg4;	/* unused */
	(void) arg5;	/* unused */
	s32_t ret = z_vrfy_k_queue_alloc_prepend(*(struct k_queue **)&arg0, *(void **)&arg1)
;
	return (u32_t) ret;
}

#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic pop
#endif
