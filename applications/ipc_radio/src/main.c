/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>
#include <zephyr/logging/log.h>

#include "ipc_bt.h"

LOG_MODULE_REGISTER(ipc_radio, CONFIG_IPC_RADIO_LOG_LEVEL);

#if !(CONFIG_IPC_RADIO_802154 || CONFIG_IPC_RADIO_BT)
#error "No radio serialization selected."
#endif

int main(void)
{
	int err;

	err = ipc_bt_init();
	if ((err) && (err != -ENOSYS)) {
		LOG_ERR("Error initializing ipc radio %d", err);
		return err;
	}

	for (;;) {
		err = ipc_bt_process();

		if (err == -ENOSYS) {
			/* Particular implementation does not need the process function */
			return 0;
		} else if (err) {
			LOG_ERR("Error processing ipc radio %d", err);
			return err;
		}
	}
}
