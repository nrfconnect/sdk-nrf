/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <stdint.h>
#include <zephyr/kernel.h>
#include <modem/nrf_modem_lib.h>
#include <modem/lte_lc.h>

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

LTE_LC_ON_CFUN(cfun_monitor, on_cfun, NULL);

static void on_cfun(enum lte_lc_func_mode mode, void *ctx)
{
	printk("> Functional mode has changed to %d\n", mode);
}

int main(void)
{
	int err;

	printk("Modem callbacks sample started\n");

	printk("Initializing modem library\n");
	err = nrf_modem_lib_init();
	if (err) {
		printk("Modem initialization failed, err %d\n", err);
		return 0;
	}

	printk("Changing functional mode\n");
	err = lte_lc_init_and_connect();
	if (err) {
		printk("lte_lc_init_and_connect() failed, err %d\n", err);
	}

	printk("Shutting down modem library\n");
	err = nrf_modem_lib_shutdown();
	if (err) {
		printk("Shutting down modem failed, err %d\n", err);
		return 0;
	}

	printk("Bye\n");

	return 0;
}
