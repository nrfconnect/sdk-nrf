/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** This file implements controller time management for 54 Series devices
 */

#include <zephyr/kernel.h>
#include <nrfx_grtc.h>
#include "conn_time_sync.h"

static uint8_t grtc_channel;

int controller_time_init(void)
{
	int ret;

	ret = nrfx_grtc_channel_alloc(&grtc_channel);
	if (ret != NRFX_SUCCESS) {
		printk("Failed allocating GRTC channel (ret: %d)\n",
		       ret - NRFX_ERROR_BASE_NUM);
		return -ENODEV;
	}

	nrf_grtc_sys_counter_compare_event_enable(NRF_GRTC, grtc_channel);

	return 0;
}

uint64_t controller_time_us_get(void)
{
	int ret;
	uint64_t current_time_us;

	ret = nrfx_grtc_syscounter_get(&current_time_us);
	if (ret != NRFX_SUCCESS) {
		printk("Failed obtaining system time (ret: %d)\n", ret - NRFX_ERROR_BASE_NUM);
		return 0;
	}

	return current_time_us;
}

void controller_time_trigger_set(uint64_t timestamp_us)
{
	int ret;

	nrfx_grtc_channel_t chan_data = {
		.channel = grtc_channel,
	};

	ret = nrfx_grtc_syscounter_cc_absolute_set(&chan_data, timestamp_us, false);
	if (ret != NRFX_SUCCESS) {
		printk("Failed setting CC (ret: %d)\n", ret - NRFX_ERROR_BASE_NUM);
	}
}

uint32_t controller_time_trigger_event_addr_get(void)
{
	return nrf_grtc_event_address_get(NRF_GRTC,
					  nrf_grtc_sys_counter_compare_event_get(grtc_channel));
}

SYS_INIT(controller_time_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
