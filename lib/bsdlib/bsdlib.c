/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <init.h>
#include <device.h>
#include <zephyr.h>

#include <bsd.h>
#include <bsd_platform.h>

#ifndef CONFIG_TRUSTED_EXECUTION_NONSECURE
#error  bsdlib must be run as non-secure firmware.\
	Are you building for the correct board ?
#endif

extern void ipc_proxy_irq_handler(void);

static int _bsdlib_init(struct device *unused)
{
	/* Setup the network IRQ used by the BSD library.
	 * Note: No call to irq_enable here, that is done through bsd_init.
	 */
	IRQ_DIRECT_CONNECT(BSD_NETWORK_IRQ, BSD_NETWORK_IRQ_PRIORITY,
			   ipc_proxy_irq_handler, 0);
	bsd_init();

	return 0;
}

int bsdlib_init(void)
{
	return _bsdlib_init(NULL);
}

int bsdlib_shutdown(void)
{
	bsd_shutdown();

	return 0;
}

#if defined(CONFIG_BSD_LIBRARY_SYS_INIT)
/* Initialize during SYS_INIT
 * The option is given since initialization is synchronous and can
 * take a long time when the modem firmware is being updated.
 */
SYS_INIT(_bsdlib_init, POST_KERNEL, 0);
#endif
