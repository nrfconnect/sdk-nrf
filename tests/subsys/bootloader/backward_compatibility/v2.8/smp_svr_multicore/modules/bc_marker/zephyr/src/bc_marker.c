/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/init.h>
#include <zephyr/sys/printk.h>

#if defined(CONFIG_SOC_NRF5340_CPUNET)
#define BC_MARKER_CORE "net"
#else
#define BC_MARKER_CORE "app"
#endif

static int bc_marker(void)
{
	printk("BC_MARKER core=%s id=%d\n", BC_MARKER_CORE, CONFIG_BC_MARKER_ID);

	return 0;
}

SYS_INIT(bc_marker, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
