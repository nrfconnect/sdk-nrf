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

static int init_ret;

static int _bsdlib_init(struct device *unused)
{
	/* Setup the network IRQ used by the BSD library.
	 * Note: No call to irq_enable() here, that is done through bsd_init().
	 */
	IRQ_DIRECT_CONNECT(BSD_NETWORK_IRQ, BSD_NETWORK_IRQ_PRIORITY,
			   ipc_proxy_irq_handler, 0);

	const bsd_init_params_t init_params = {
		.trace_on = true,
		.bsd_memory_address = BSD_RESERVED_MEMORY_ADDRESS,
		.bsd_memory_size = BSD_RESERVED_MEMORY_SIZE
	};

	init_ret = bsd_init(&init_params);
	if (IS_ENABLED(CONFIG_BSD_LIBRARY_SYS_INIT)) {
		/* bsd_init() returns values from a different namespace
		 * than Zephyr's. Make sure to return something in Zephyr's
		 * namespace, in this case 0, when called during SYS_INIT.
		 * Non-zero values in SYS_INIT are currently ignored.
		 */
		return 0;
	}

	return init_ret;
}

int bsdlib_init(void)
{
	return _bsdlib_init(NULL);
}

int bsdlib_get_init_ret(void)
{
	return init_ret;
}

int bsdlib_shutdown(void)
{
	bsd_shutdown();

	return 0;
}

#if defined(CONFIG_BSD_LIBRARY_SYS_INIT)
/* Initialize during SYS_INIT */
SYS_INIT(_bsdlib_init, POST_KERNEL, 0);
#endif
