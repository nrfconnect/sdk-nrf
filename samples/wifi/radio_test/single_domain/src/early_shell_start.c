/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/init.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_uart.h>

/*
 * CONFIG_SHELL_AUTOSTART is disabled for this sample (see prj.conf): the
 * shell thread only calls shell_start() itself when autostart is enabled,
 * and that call happens whenever the thread first gets scheduled - not
 * guaranteed to be before the SYS_INIT(APPLICATION, 0) boot banner on
 * this sample's thin boot. shell_start() is what enables the shell's log
 * backend (SHELL_LOG_BACKEND_DISABLED -> ENABLED), and printk()/LOG_*
 * messages emitted while it is still disabled are silently dropped.
 *
 * Call shell_start() ourselves instead, from a SYS_INIT placed right
 * after the shell UART backend's own init. Both run within POST_KERNEL,
 * which always completes before APPLICATION level starts, so this
 * enables the log backend deterministically before the boot banner runs
 * - no dependency on scheduling order.
 *
 * The priority below must stay a plain literal greater than
 * CONFIG_SHELL_BACKEND_SERIAL_INIT_PRIORITY (currently 90): SYS_INIT's
 * priority argument is token-pasted into a linker section name, so an
 * arithmetic expression there breaks the link with "Undefined
 * initialization levels used".
 */
static int early_shell_start(void)
{
	return shell_start(shell_backend_uart_get_ptr());
}

BUILD_ASSERT(91 > CONFIG_SHELL_BACKEND_SERIAL_INIT_PRIORITY,
	     "early_shell_start must run after the shell UART backend's own init");
SYS_INIT(early_shell_start, POST_KERNEL, 91);
