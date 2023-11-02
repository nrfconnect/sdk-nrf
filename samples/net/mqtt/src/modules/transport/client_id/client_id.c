/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/random/random.h>
#include <hw_id.h>
#include <stdio.h>
#include <string.h>

int client_id_get(char *const buffer, size_t buffer_size)
{
	int ret;

	if (sizeof(CONFIG_MQTT_SAMPLE_TRANSPORT_CLIENT_ID) - 1 > 0) {
		ret = snprintk(buffer, buffer_size, "%s", CONFIG_MQTT_SAMPLE_TRANSPORT_CLIENT_ID);
		if ((ret < 0) || (ret >= buffer_size)) {
			return -EMSGSIZE;
		}
	} else if (IS_ENABLED(CONFIG_BOARD_NATIVE_POSIX)) {
		ret = snprintk(buffer, buffer_size, "%d", sys_rand32_get());
		if ((ret < 0) || (ret >= buffer_size)) {
			return -EMSGSIZE;
		}
	} else {
		ret = hw_id_get(buffer, buffer_size);
		if (ret) {
			return ret;
		}
	}

	return 0;
}
