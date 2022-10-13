/*
 * Copyright (c) 2018, Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_native_posix_uart

#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pty.h>
#include <fcntl.h>
#include <sys/select.h>
#include <unistd.h>
#include <poll.h>

#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>

#include "cmdline.h" /* native_posix command line options header */
#include "soc.h"

/*
 * UART driver for POSIX ARCH based boards.
 * It can support up to two UARTs.
 *
 * For the first UART:
 *
 * It can either be connected to the process STDIN+STDOUT
 * OR
 * to a dedicated pseudo terminal
 *
 * The 2nd option is the recommended one for interactive use, as the pseudo
 * terminal driver will be configured in "raw" mode, and will therefore behave
 * more like a real UART.
 *
 * When connected to its own pseudo terminal, it may also auto attach a terminal
 * emulator to it, if set so from command line.
 */

static int np_uart_stdin_poll_in(const struct device *dev,
				 unsigned char *p_char);
static int np_uart_tty_poll_in(const struct device *dev,
			       unsigned char *p_char);
static void np_uart_poll_out(const struct device *dev,
				      unsigned char out_char);

static bool auto_attach;
static bool wait_pts;
static const char default_cmd[] = CONFIG_NATIVE_UART_AUTOATTACH_DEFAULT_CMD;
static char *auto_attach_cmd;

struct native_uart_status {
	int out_fd; /* File descriptor used for output */
	int in_fd; /* File descriptor used for input */
};

static struct native_uart_status native_uart_status_0;

static struct uart_driver_api np_uart_driver_api_0 = {
	.poll_out = np_uart_poll_out,
	.poll_in = np_uart_tty_poll_in,
};

#if defined(CONFIG_UART_NATIVE_POSIX_PORT_1_ENABLE)
static struct native_uart_status native_uart_status_1;

static struct uart_driver_api np_uart_driver_api_1 = {
	.poll_out = np_uart_poll_out,
	.poll_in = np_uart_tty_poll_in,
};
#endif /* CONFIG_UART_NATIVE_POSIX_PORT_1_ENABLE */

#define ERROR posix_print_error_and_exit
#define WARN posix_print_warning

/**
 * Attempt to connect a terminal emulator to the slave side of the pty
 * If -attach_uart_cmd=<cmd> is provided as a command line option, <cmd> will be
 * used. Otherwise, the default command,
 * CONFIG_NATIVE_UART_AUTOATTACH_DEFAULT_CMD, will be used instead
 */
static void attach_to_tty(const char *slave_tty)
{
	if (auto_attach_cmd == NULL) {
		auto_attach_cmd = (char *)default_cmd;
	}
	char command[strlen(auto_attach_cmd) + strlen(slave_tty) + 1];

	sprintf(command, auto_attach_cmd, slave_tty);

	int ret = system(command);

	if (ret != 0) {
		WARN("Could not attach to the UART with \"%s\"\n", command);
		WARN("The command returned %i\n", WEXITSTATUS(ret));
	}
}

/**
 * Attempt to allocate and open a new pseudoterminal
 *
 * Returns the file descriptor of the master side
 * If auto_attach was set, it will also attempt to connect a new terminal
 * emulator to its slave side.
 */
static int open_tty(struct native_uart_status *driver_data,
		    const char *uart_name,
		    bool do_auto_attach)
{
	int master_pty;
	char *slave_pty_name;
	struct termios ter;
	struct winsize win;
	int err_nbr;
	int ret;
	int flags;

	win.ws_col = 80;
	win.ws_row = 24;

	master_pty = posix_openpt(O_RDWR | O_NOCTTY);
	if (master_pty == -1) {
		ERROR("Could not open a new TTY for the UART\n");
	}
	ret = grantpt(master_pty);
	if (ret == -1) {
		err_nbr = errno;
		close(master_pty);
		ERROR("Could not grant access to the slave PTY side (%i)\n",
			err_nbr);
	}
	ret = unlockpt(master_pty);
	if (ret == -1) {
		err_nbr = errno;
		close(master_pty);
		ERROR("Could not unlock the slave PTY side (%i)\n", err_nbr);
	}
	slave_pty_name = ptsname(master_pty);
	if (slave_pty_name == NULL) {
		err_nbr = errno;
		close(master_pty);
		ERROR("Error getting slave PTY device name (%i)\n", err_nbr);
	}
	/* Set the master PTY as non blocking */
	flags = fcntl(master_pty, F_GETFL);
	if (flags == -1) {
		err_nbr = errno;
		close(master_pty);
		ERROR("Could not read the master PTY file status flags (%i)\n",
			err_nbr);
	}

	ret = fcntl(master_pty, F_SETFL, flags | O_NONBLOCK);
	if (ret == -1) {
		err_nbr = errno;
		close(master_pty);
		ERROR("Could not set the master PTY as non-blocking (%i)\n",
			err_nbr);
	}

	(void) err_nbr;

	/*
	 * Set terminal in "raw" mode:
	 *  Not canonical (no line input)
	 *  No signal generation from Ctr+{C|Z..}
	 *  No echoing, no input or output processing
	 *  No replacing of NL or CR
	 *  No flow control
	 */
	ret = tcgetattr(master_pty, &ter);
	if (ret == -1) {
		ERROR("Could not read terminal driver settings\n");
	}
	ter.c_cc[VMIN] = 0;
	ter.c_cc[VTIME] = 0;
	ter.c_lflag &= ~(ICANON | ISIG | IEXTEN | ECHO);
	ter.c_iflag &= ~(BRKINT | ICRNL | IGNBRK | IGNCR | INLCR | INPCK
			 | ISTRIP | IXON | PARMRK);
	ter.c_oflag &= ~OPOST;
	ret = tcsetattr(master_pty, TCSANOW, &ter);
	if (ret == -1) {
		ERROR("Could not change terminal driver settings\n");
	}

	posix_print_trace("%s connected to pseudotty: %s\n",
			  uart_name, slave_pty_name);

	if (wait_pts) {
		/*
		 * This trick sets the HUP flag on the tty master, making it
		 * possible to detect a client connection using poll.
		 * The connection of the client would cause the HUP flag to be
		 * cleared, and in turn set again at disconnect.
		 */
		close(open(slave_pty_name, O_RDWR | O_NOCTTY));
	}
	if (do_auto_attach) {
		attach_to_tty(slave_pty_name);
	}

	return master_pty;
}

/**
 * @brief Initialize the first native_posix serial port
 *
 * @param dev UART_0 device struct
 *
 * @return 0 (if it fails catastrophically, the execution is terminated)
 */
static int np_uart_0_init(const struct device *dev)
{
	struct native_uart_status *d;

	d = (struct native_uart_status *)dev->data;

	if (IS_ENABLED(CONFIG_NATIVE_UART_0_ON_OWN_PTY)) {
		int tty_fn = open_tty(d, dev->name, auto_attach);

		d->in_fd = tty_fn;
		d->out_fd = tty_fn;
		np_uart_driver_api_0.poll_in = np_uart_tty_poll_in;
	} else { /* NATIVE_UART_0_ON_STDINOUT */
		d->in_fd  = STDIN_FILENO;
		d->out_fd = STDOUT_FILENO;
		np_uart_driver_api_0.poll_in = np_uart_stdin_poll_in;

		if (isatty(STDIN_FILENO)) {
			WARN("The UART driver has been configured to map to the"
			     " process stdin&out (NATIVE_UART_0_ON_STDINOUT), "
			     "but stdin seems to be left attached to the shell."
			     " This will most likely NOT behave as you want it "
			     "to. This option is NOT meant for interactive use "
			     "but for piping/feeding from/to files to the UART"
			     );
		}
	}

	return 0;
}

#if defined(CONFIG_UART_NATIVE_POSIX_PORT_1_ENABLE)
/*
 * Initialize the 2nd UART port.
 * This port will be always attached to its own new pseudoterminal.
 */
static int np_uart_1_init(const struct device *dev)
{
	struct native_uart_status *d;
	int tty_fn;

	d = (struct native_uart_status *)dev->data;

	tty_fn = open_tty(d, dev->name, false);

	d->in_fd = tty_fn;
	d->out_fd = tty_fn;

	return 0;
}
#endif

/*
 * @brief Output a character towards the serial port
 *
 * @param dev UART device struct
 * @param out_char Character to send.
 */
static void np_uart_poll_out(const struct device *dev,
				      unsigned char out_char)
{
	int ret;
	struct native_uart_status *d = (struct native_uart_status *)dev->data;

	if (wait_pts) {
		struct pollfd pfd = { .fd = d->out_fd, .events = POLLHUP };

		while (1) {
			poll(&pfd, 1, 0);
			if (!(pfd.revents & POLLHUP)) {
				/* There is now a reader on the slave side */
				break;
			}
			k_sleep(K_MSEC(100));
		}
	}

	/* The return value of write() cannot be ignored (there is a warning)
	 * but we do not need the return value for anything.
	 */
	ret = write(d->out_fd, &out_char, 1);
	(void) ret;
}

/**
 * @brief Poll the device for input.
 *
 * @param dev UART device structure.
 * @param p_char Pointer to character.
 *
 * @retval 0 If a character arrived and was stored in p_char
 * @retval -1 If no character was available to read
 */
static int np_uart_stdin_poll_in(const struct device *dev,
				 unsigned char *p_char)
{
	static bool disconnected;

	if (disconnected || feof(stdin)) {
		/*
		 * The stdinput is fed from a file which finished or the user
		 * pressed Ctrl+D
		 */
		disconnected = true;
		return -1;
	}

	int n = -1;
	int in_f = ((struct native_uart_status *)dev->data)->in_fd;

	int ready;
	fd_set readfds;
	static struct timeval timeout; /* just zero */

	FD_ZERO(&readfds);
	FD_SET(in_f, &readfds);

	ready = select(in_f+1, &readfds, NULL, NULL, &timeout);

	if (ready == 0) {
		return -1;
	} else if (ready == -1) {
		ERROR("%s: Error on select ()\n", __func__);
	}

	n = read(in_f, p_char, 1);
	if (n == -1) {
		return -1;
	}

	return 0;
}

/**
 * @brief Poll the device for input.
 *
 * @param dev UART device structure.
 * @param p_char Pointer to character.
 *
 * @retval 0 If a character arrived and was stored in p_char
 * @retval -1 If no character was available to read
 */
static int np_uart_tty_poll_in(const struct device *dev,
			       unsigned char *p_char)
{
	int n = -1;
	int in_f = ((struct native_uart_status *)dev->data)->in_fd;

	n = read(in_f, p_char, 1);
	if (n == -1) {
		return -1;
	}
	return 0;
}

DEVICE_DT_INST_DEFINE(0,
	    &np_uart_0_init, NULL,
	    (void *)&native_uart_status_0, NULL,
	    PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY,
	    &np_uart_driver_api_0);

#if defined(CONFIG_UART_NATIVE_POSIX_PORT_1_ENABLE)
DEVICE_DT_INST_DEFINE(1,
	    &np_uart_1_init, NULL,
	    (void *)&native_uart_status_1, NULL,
	    PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY,
	    &np_uart_driver_api_1);
#endif /* CONFIG_UART_NATIVE_POSIX_PORT_1_ENABLE */

static void auto_attach_cmd_cb(char *argv, int offset)
{
	ARG_UNUSED(argv);
	ARG_UNUSED(offset);

	auto_attach = true;
}

static void np_add_uart_options(void)
{
	if (!IS_ENABLED(CONFIG_NATIVE_UART_0_ON_OWN_PTY)) {
		return;
	}

	static struct args_struct_t uart_options[] = {
		/*
		 * Fields:
		 * manual, mandatory, switch,
		 * option_name, var_name ,type,
		 * destination, callback,
		 * description
		 */
		{false, false, true,
		"attach_uart", "", 'b',
		(void *)&auto_attach, NULL,
		"Automatically attach to the UART terminal"},
		{false, false, false,
		"attach_uart_cmd", "\"cmd\"", 's',
		(void *)&auto_attach_cmd, auto_attach_cmd_cb,
		"Command used to automatically attach to the terminal"
		"(implies auto_attach), by "
		"default: '" CONFIG_NATIVE_UART_AUTOATTACH_DEFAULT_CMD "'"},
		IF_ENABLED(CONFIG_UART_NATIVE_WAIT_PTS_READY_ENABLE, (
			{false, false, true,
			"wait_uart", "", 'b',
			(void *)&wait_pts, NULL,
			"Hold writes to the uart/pts until a client is "
			"connected/ready"},)
		)
		ARG_TABLE_ENDMARKER
	};

	native_add_command_line_opts(uart_options);
}

static void np_cleanup_uart(void)
{
	if (IS_ENABLED(CONFIG_NATIVE_UART_0_ON_OWN_PTY)) {
		if (native_uart_status_0.in_fd != 0) {
			close(native_uart_status_0.in_fd);
		}
	}

#if defined(CONFIG_UART_NATIVE_POSIX_PORT_1_ENABLE)
	if (native_uart_status_1.in_fd != 0) {
		close(native_uart_status_1.in_fd);
	}
#endif
}

NATIVE_TASK(np_add_uart_options, PRE_BOOT_1, 11);
NATIVE_TASK(np_cleanup_uart, ON_EXIT, 99);
