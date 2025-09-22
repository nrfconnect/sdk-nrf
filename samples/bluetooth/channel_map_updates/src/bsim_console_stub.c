#include <zephyr/kernel.h>
#if defined(CONFIG_ARCH_POSIX)
#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <zephyr/console/console.h>

/* Simple non-blocking console stub for BabbleSim/Native POSIX build */
static int set_stdin_nonblock(bool enable)
{
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    if (flags < 0) {
        return -1;
    }
    if (enable) {
        flags |= O_NONBLOCK;
    } else {
        flags &= ~O_NONBLOCK;
    }
    return fcntl(STDIN_FILENO, F_SETFL, flags);
}

int console_init(void)
{
    set_stdin_nonblock(true);
    return 0;
}

int console_getchar(void)
{
    unsigned char ch;
    ssize_t ret = read(STDIN_FILENO, &ch, 1);
    if (ret == 1) {
        return ch;
    }
    /* No character available */
    k_sleep(K_MSEC(1));
    return -1;
}
#endif /* CONFIG_ARCH_POSIX */ 