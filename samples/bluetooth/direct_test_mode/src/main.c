/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/device.h>
#include <zephyr/sys/printk.h>

#include "transport/dtm_uart_twowire.h"

int main(void)
{
	int err;
	uint16_t cmd;

	printk("Starting Direct Test Mode example\n");

	err = dtm_uart_twowire_init();
	if (err) {
		printk("Error initializing DTM Two Wire Uart transport: %d\n", err);
		return err;
	}

	for (;;) {
		cmd = dtm_uart_twowire_get();
		err = dtm_uart_twowire_process(cmd);
		if (err) {
			printk("Error processing command(%x): %d\n", cmd, err);
			return err;
		}
	}
}
