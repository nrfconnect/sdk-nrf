/*
 * Copyright (c) 2026 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/init.h>
#include <zephyr/sys/printk.h>

static int code_extender_init(void)
{
	printk("App extended size: 0x%x\n", CONFIG_CODE_EXTENDER_SIZE);

	return 0;
}

SYS_INIT(code_extender_init, APPLICATION, 0);
