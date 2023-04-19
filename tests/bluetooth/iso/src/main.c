/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/bluetooth/bluetooth.h>

#include "iso_broadcast_src.h"
#include "iso_broadcast_sink.h"
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main, 3);

int main(void)
{
	int err;

	LOG_INF("Starting ISO tester");

	err = bt_enable(NULL);
	if (err) {
		LOG_ERR("Bluetooth init failed (err %d)", err);
	}

	err = iso_broadcast_src_init();
	if (err) {
		LOG_ERR("iso_broadcast_src_init failed (err %d)", err);
	}

	err = iso_broadcast_sink_init();
	if (err) {
		LOG_ERR("iso_broadcaster_sink_init failed (err %d)", err);
	}

	return 0;
}
