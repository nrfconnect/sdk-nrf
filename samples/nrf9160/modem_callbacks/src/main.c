/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <stdint.h>
#include <zephyr.h>
#include <modem/nrf_modem_lib.h>

NRF_MODEM_LIB_ON_INIT(init_hook, on_modem_init, NULL);
NRF_MODEM_LIB_ON_SHUTDOWN(shutdown_hook, on_modem_shutdown, NULL);

static void on_modem_init(int ret, void *ctx)
{
	printk("> Initialized with value %d\n", ret);
}

static void on_modem_shutdown(void *ctx)
{
	printk("> Shutting down\n");
}

void main(void)
{
	int err;

	printk("Modem callbacks sample started\n");

	printk("Initializing modem library\n");
	err = nrf_modem_lib_init(NORMAL_MODE);
	if (err) {
		printk("Modem initialization failed, err %d\n", err);
		return;
	}

	printk("Shutting down modem library\n");
	err = nrf_modem_lib_shutdown();
	if (err) {
		printk("Modem initialization failed, err %d\n", err);
		return;
	}

	printk("Bye\n");
}
