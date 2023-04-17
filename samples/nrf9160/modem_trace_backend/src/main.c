/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <modem/lte_lc.h>
#include <modem/nrf_modem_lib.h>
#include <nrf_modem_at.h>

/* define callback */
LTE_LC_ON_CFUN(cfun_hook, on_cfun, NULL);

/* callback implementation */
static void on_cfun(enum lte_lc_func_mode mode, void *context)
{
	printk("LTE mode changed to %d\n", mode);
}

int main(void)
{
	int err;

	printk("Modem trace backend sample started\n");

	printk("Initializing modem library\n");
	err = nrf_modem_lib_init();
	if (err) {
		printk("Modem library initialization failed, error: %d\n", err);
		return 0;
	}

	printk("Connecting to network\n");
	err = lte_lc_func_mode_set(LTE_LC_FUNC_MODE_NORMAL);
	if (err) {
		printk("Failed to change LTE mode, err %d\n", err);
		return 0;
	}

	k_sleep(K_SECONDS(5));

	err = lte_lc_func_mode_set(LTE_LC_FUNC_MODE_POWER_OFF);
	if (err) {
		printk("Failed to change LTE mode, err %d\n", err);
		return 0;
	}

	nrf_modem_lib_shutdown();

	printk("Bye\n");

	return 0;
}
