/* auto-generated by gen_syscalls.py, don't edit */
#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic push
#endif
#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif
#include <syscalls/socket.h>

extern int z_vrfy_zsock_connect(int sock, const struct sockaddr * addr, socklen_t addrlen);
u32_t z_mrsh_zsock_connect(u32_t arg0, u32_t arg1, u32_t arg2,
		u32_t arg3, u32_t arg4, u32_t arg5, void *ssf)
{
	_current_cpu->syscall_frame = ssf;
	(void) arg3;	/* unused */
	(void) arg4;	/* unused */
	(void) arg5;	/* unused */
	int ret = z_vrfy_zsock_connect(*(int*)&arg0, *(const struct sockaddr **)&arg1, *(socklen_t*)&arg2)
;
	return (u32_t) ret;
}

#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic pop
#endif
