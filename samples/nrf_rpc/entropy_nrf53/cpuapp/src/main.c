/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>

#include <nrf_rpc.h>

#include "entropy_ser.h"

#define BUFFER_LENGTH 10

static uint8_t buffer[BUFFER_LENGTH];

static void result_callback(int result, uint8_t *buffer, size_t length)
{
	size_t i;

	if (result) {
		printk("Entropy remote get failed: %d\n", result);
		return;
	}

	for (i = 0; i < length; i++) {
		printk("  0x%02x", buffer[i]);
	}

	printk("\n");
}

int main(void)
{
	int err;

	printk("Entropy sample started[APP Core].\n");

	err = entropy_remote_init();
	if (err) {
		printk("Remote entropy driver initialization failed\n");
		return 0;
	}

	printk("Remote init send\n");

	while (true) {
		k_sleep(K_MSEC(2000));

		err = entropy_remote_get(buffer, sizeof(buffer));
		if (err) {
			printk("Entropy remote get failed: %d\n", err);
			continue;
		}

		for (int i = 0; i < BUFFER_LENGTH; i++) {
			printk("  0x%02x", buffer[i]);
		}

		printk("\n");

		k_sleep(K_MSEC(2000));

		err = entropy_remote_get_inline(buffer, sizeof(buffer));
		if (err) {
			printk("Entropy remote get failed: %d\n", err);
			continue;
		}

		for (int i = 0; i < BUFFER_LENGTH; i++) {
			printk("  0x%02x", buffer[i]);
		}

		printk("\n");

		k_sleep(K_MSEC(2000));

		err = entropy_remote_get_async(sizeof(buffer), result_callback);
		if (err) {
			printk("Entropy remote get async failed: %d\n", err);
			continue;
		}

		k_sleep(K_MSEC(2000));

		err = entropy_remote_get_cbk(sizeof(buffer), result_callback);
		if (err) {
			printk("Entropy remote get callback failed: %d\n", err);
			continue;
		}
	}
}
