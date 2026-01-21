/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/init.h>
#include <zephyr/sys/printk.h>
#include <fw_info.h>
#include <bl_storage.h>

static int invalidate_s0(void)
{
	uint32_t s0_addr = s0_address_read();
	const struct fw_info *s0_info = fw_info_find(s0_addr);

	printk("Invalidating S0 in order to ensure the S1 image is started\n");
	fw_info_invalidate(s0_info);

	return 0;
}

SYS_INIT(invalidate_s0, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
