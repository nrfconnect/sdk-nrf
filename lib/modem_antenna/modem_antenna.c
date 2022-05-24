/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>
#include <nrf_modem_at.h>
#include <modem/nrf_modem_lib.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(modem_antenna, CONFIG_MODEM_ANTENNA_LOG_LEVEL);

NRF_MODEM_LIB_ON_INIT(gnss_cfg_init_hook, on_modem_lib_init, NULL);

static void on_modem_lib_init(int ret, void *ctx)
{
	int err;

	if (ret != 0) {
		return;
	}

	if (strlen(CONFIG_MODEM_ANTENNA_AT_MAGPIO) > 0) {
		LOG_DBG("Setting MAGPIO configuration: %s", CONFIG_MODEM_ANTENNA_AT_MAGPIO);
		err = nrf_modem_at_printf("%s", CONFIG_MODEM_ANTENNA_AT_MAGPIO);
		if (err) {
			LOG_ERR("Failed to set MAGPIO configuration (err: %d)", err);
		}
	}

	if (strlen(CONFIG_MODEM_ANTENNA_AT_COEX0) > 0) {
		LOG_DBG("Setting COEX0 configuration: %s", CONFIG_MODEM_ANTENNA_AT_COEX0);
		err = nrf_modem_at_printf("%s", CONFIG_MODEM_ANTENNA_AT_COEX0);
		if (err) {
			LOG_ERR("Failed to set COEX0 configuration (err: %d)", err);
		}
	}
}
