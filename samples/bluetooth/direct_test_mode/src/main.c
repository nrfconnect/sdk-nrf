/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/device.h>
#include <zephyr/sys/printk.h>
#include <zephyr/pm/device_runtime.h>

#include "transport/dtm_transport.h"

int main(void)
{
	int err;
	union dtm_tr_packet cmd;

	printk("Starting Direct Test Mode sample\n");

#if defined(CONFIG_SOC_SERIES_NRF54H)
	const struct device *dtm_uart = DEVICE_DT_GET_OR_NULL(DT_CHOSEN(ncs_dtm_uart));

	if (dtm_uart != NULL) {
		int ret = pm_device_runtime_get(dtm_uart);

		if (ret < 0) {
			printk("Failed to get DTM UART runtime PM: %d\n", ret);
		}
	}
#endif /* defined(CONFIG_SOC_SERIES_NRF54H) */

	err = dtm_tr_init();
	if (err) {
		printk("Error initializing DTM transport: %d\n", err);
		return err;
	}

	for (;;) {
		cmd = dtm_tr_get();
		err = dtm_tr_process(cmd);
		if (err) {
			printk("Error processing command: %d\n", err);
			return err;
		}
	}
}
