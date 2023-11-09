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

void set_antenna_configuration(const char *config_value)
{
	int err;

	if (strlen(config_value) > 0) {
		LOG_DBG("Setting configuration: %s", config_value);
		err = nrf_modem_at_printf("%s", config_value);
		if (err) {
			LOG_ERR("Failed to set configuration (err: %d)", err);
		}
	}
}

static void on_modem_lib_init(int ret, void *ctx)
{
	if (ret != 0) {
		return;
	}

	set_antenna_configuration(CONFIG_MODEM_ANTENNA_AT_MAGPIO);
	set_antenna_configuration(CONFIG_MODEM_ANTENNA_AT_COEX0);
	set_antenna_configuration(CONFIG_MODEM_ANTENNA_AT_MIPIRFFEDEV);
	set_antenna_configuration(CONFIG_MODEM_ANTENNA_AT_MIPIRFFECTRL_INIT);
	set_antenna_configuration(CONFIG_MODEM_ANTENNA_AT_MIPIRFFECTRL_ON);
	set_antenna_configuration(CONFIG_MODEM_ANTENNA_AT_MIPIRFFECTRL_OFF);
	set_antenna_configuration(CONFIG_MODEM_ANTENNA_AT_MIPIRFFECTRL_PWROFF);
}
