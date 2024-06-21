/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <nrfs_mram.h>
#include <zephyr/logging/log.h>
#include "common.h"

LOG_MODULE_REGISTER(app);

int main(void)
{
	if (IS_ENABLED(CONFIG_MCUMGR_TRANSPORT_BT)) {
		nrfs_err_t err = nrfs_mram_init(NULL);

		if (err != NRFS_SUCCESS) {
			printk("Unable to initialize MRAM latency service: %d\r\n", err);
		} else {
			err = nrfs_mram_set_latency(MRAM_LATENCY_NOT_ALLOWED, NULL);
			if (err != NRFS_SUCCESS) {
				printk("Unable to disable MRAM auto poweroff: %d\r\n", err);
			}
		}

		start_smp_bluetooth_adverts();
	}

	return 0;
}
