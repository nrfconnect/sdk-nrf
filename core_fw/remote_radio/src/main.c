/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>
#include <zephyr/logging/log.h>

#include "remote_bt.h"

LOG_MODULE_REGISTER(remote_radio, CONFIG_REMOTE_RADIO_LOG_LEVEL);

int main(void)
{
	int err;

	err = remote_bt_init();
	if ((err) && (err != -ENOSYS)) {
		LOG_ERR("Error initializing remote radio %d", err);
		return err;
	}

	for (;;) {
		err = remote_bt_process();

		if (err == -ENOSYS) {
			/* Particular implementation does not need the process function */
			return 0;
		} else if (err) {
			LOG_ERR("Error processing remote radio %d", err);
			return err;
		}

	}
}
