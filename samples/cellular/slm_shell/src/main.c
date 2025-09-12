/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdlib.h>
#include <modem/modem_slm.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(app, CONFIG_LOG_DEFAULT_LEVEL);

SLM_MONITOR(network, "\r\n+CEREG:", cereg_mon);

static void cereg_mon(const char *notif)
{
	int status = atoi(notif + strlen("\r\n+CEREG: "));

	if (status == 1 || status == 5) {
		LOG_INF("LTE connected");
	}
}

void slm_shell_data_indication(const uint8_t *data, size_t datalen)
{
	LOG_INF("Data received (len=%d): %.*s", datalen, datalen, (const char *)data);
}

void slm_shell_ri_handler(void)
{
	LOG_INF("Ring Indicate (RI) triggered");
}

int main(void)
{
	int err;

	LOG_INF("SLM Shell starts on %s", CONFIG_BOARD);

	err = modem_slm_init(slm_shell_data_indication, true, K_MSEC(100));
	if (err) {
		LOG_ERR("Failed to initialize SLM: %d", err);
	}

	err = modem_slm_register_ri_handler(slm_shell_ri_handler);
	if (err) {
		LOG_ERR("Failed to register RI handler: %d", err);
	}

	return 0;
}
