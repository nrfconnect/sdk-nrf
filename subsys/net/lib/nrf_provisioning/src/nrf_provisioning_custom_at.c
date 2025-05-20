/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <modem/at_cmd_custom.h>
#include <zephyr/logging/log.h>
#include <zephyr/settings/settings.h>
#include <net/nrf_provisioning.h>

LOG_MODULE_REGISTER(nrf_provisioning_at_cmd, CONFIG_NRF_PROVISIONING_LOG_LEVEL);

AT_CMD_CUSTOM(provstart_filter, "AT#PROVSTART", provstart_callback);
AT_CMD_CUSTOM(provcfg_filter, "AT#PROVCFG", provcfg_callback);

int provstart_callback(char *buf, size_t len, char *at_cmd)
{
	int ret;

	ret = nrf_provisioning_trigger_manually();
	if (ret) {
		LOG_ERR("Failed to start provisioning");
		return at_cmd_custom_respond(buf, len, "ERROR\r\n");
	}

	return at_cmd_custom_respond(buf, len, "OK\r\n");
}

#define PATH                                                                                       \
	(CONFIG_NRF_PROVISIONING_SETTINGS_STORAGE_PATH "/"                                         \
						       "interval-sec")

struct settings_read_callback_params {
	const char *subtree;
};

static int settings_read_callback(const char *key,
				  size_t len,
				  settings_read_cb read_cb,
				  void            *cb_arg,
				  void            *param)
{
	uint8_t buffer[SETTINGS_MAX_VAL_LEN];
	ssize_t num_read_bytes = MIN(len, SETTINGS_MAX_VAL_LEN);

	num_read_bytes = read_cb(cb_arg, buffer, num_read_bytes);

	if (num_read_bytes < 0) {
		LOG_ERR("Failed to read value: %d", (int) num_read_bytes);
		return 0;
	}

	printk("%s %s\r\n", key, buffer);
	LOG_HEXDUMP_DBG(buffer, num_read_bytes, key);

	if (len > SETTINGS_MAX_VAL_LEN) {
		LOG_WRN("(The output has been truncated)");
	}

	return 0;
}

int provcfg_callback(char *buf, size_t len, char *at_cmd)
{
	int ret;

	struct settings_read_callback_params params = {
		.subtree = NULL,
	};

	ret = settings_load_subtree_direct(params.subtree, settings_read_callback, &params);

	if (ret) {
		LOG_ERR("Failed to load settings: %d", ret);
		return at_cmd_custom_respond(buf, len, "ERROR\r\n");
	}

	return at_cmd_custom_respond(buf, len, "OK\r\n");
}
