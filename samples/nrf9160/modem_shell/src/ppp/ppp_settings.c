/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <strings.h>

#include <assert.h>

#include <shell/shell.h>
#include <drivers/uart.h>
#include <settings/settings.h>

#include "ppp_ctrl.h"
#include "ppp_settings.h"

extern const struct shell *shell_global;

#define PPP_SETT_KEY "ppp_settings"

#define PPP_SETT_UART_BAUDRATE "uart_baudrate"
#define PPP_SETT_UART_PARITY "uart_parity"
#define PPP_SETT_UART_STOP_BITS "uart_stop_bits"
#define PPP_SETT_UART_DATA_BITS "uart_data_bits"
#define PPP_SETT_UART_FLOW_CTRL "uart_flow_ctrl"

static const struct device *ppp_uart_dev;
static bool ppp_uart_conf_valid;
static struct uart_config ppp_uart_conf;

/******************************************************************************/

static int ppp_settings_uart_dev_conf_get(struct uart_config *uart_conf)
{
	int ret;

	if (ppp_uart_dev) {
		ret = uart_config_get(ppp_uart_dev, uart_conf);
		if (ret != 0) {
			shell_warn(shell_global, "Cannot get PPP uart config, ret %d", ret);
		}
	} else {
		shell_warn(shell_global, "No PPP uart device with name %s",
			   CONFIG_NET_PPP_UART_NAME);
		ret = -1;
	}
	return ret;
}

static int ppp_settings_uart_dev_conf_set(struct uart_config *uart_conf)
{
	int ret;

	if (ppp_uart_dev) {
		ret = uart_configure(ppp_uart_dev, uart_conf);
		if (ret != 0) {
			shell_warn(shell_global, "Cannot get PPP uart config, ret %d", ret);
		}
	} else {
		shell_warn(shell_global, "No PPP uart device with name %s",
			   CONFIG_NET_PPP_UART_NAME);
		ret = -1;
	}
	return ret;
}

/******************************************************************************/

/**@brief Callback when settings_load() is called. */
static int ppp_settings_handler(const char *key, size_t len, settings_read_cb read_cb, void *cb_arg)
{
	int ret = 0;

	if (strcmp(key, PPP_SETT_UART_BAUDRATE) == 0) {
		ret = read_cb(cb_arg, &ppp_uart_conf.baudrate, sizeof(ppp_uart_conf.baudrate));
		if (ret < 0) {
			shell_error(shell_global, "Failed to read uart_baudrate, error: %d", ret);
			return ret;
		}
	} else if (strcmp(key, PPP_SETT_UART_PARITY) == 0) {
		ret = read_cb(cb_arg, &ppp_uart_conf.parity, sizeof(ppp_uart_conf.parity));
		if (ret < 0) {
			shell_error(shell_global, "Failed to read uart_parity, error: %d", ret);
			return ret;
		}
	} else if (strcmp(key, PPP_SETT_UART_STOP_BITS) == 0) {
		ret = read_cb(cb_arg, &ppp_uart_conf.stop_bits, sizeof(ppp_uart_conf.stop_bits));
		if (ret < 0) {
			shell_error(shell_global, "Failed to read uart_stop_bits, error: %d", ret);
			return ret;
		}
	} else if (strcmp(key, PPP_SETT_UART_DATA_BITS) == 0) {
		ret = read_cb(cb_arg, &ppp_uart_conf.data_bits, sizeof(ppp_uart_conf.data_bits));
		if (ret < 0) {
			shell_error(shell_global, "Failed to read uart_data_bits, error: %d", ret);
			return ret;
		}
	} else if (strcmp(key, PPP_SETT_UART_FLOW_CTRL) == 0) {
		ret = read_cb(cb_arg, &ppp_uart_conf.flow_ctrl, sizeof(ppp_uart_conf.flow_ctrl));
		if (ret < 0) {
			shell_error(shell_global, "Failed to read uart_flow_ctrl, error: %d", ret);
			return ret;
		}
	}
	return 0;
}

/******************************************************************************/

int ppp_uart_settings_read(struct uart_config *uart_conf)
{
	if (ppp_uart_conf_valid) {
		memcpy(uart_conf, &ppp_uart_conf, sizeof(struct uart_config));
		return 0;
	} else {
		return -EINVAL;
	}
}

int ppp_uart_settings_write(struct uart_config *uart_conf)
{
	int ret;
	char *key;

	key = PPP_SETT_KEY "/" PPP_SETT_UART_BAUDRATE;
	ret = settings_save_one(key, &(uart_conf->baudrate), sizeof(uart_conf->baudrate));
	if (ret) {
		goto return_err;
	}

	key = PPP_SETT_KEY "/" PPP_SETT_UART_PARITY;
	ret = settings_save_one(key, &(uart_conf->parity), sizeof(uart_conf->parity));
	if (ret) {
		goto return_err;
	}

	key = PPP_SETT_KEY "/" PPP_SETT_UART_STOP_BITS;
	ret = settings_save_one(key, &(uart_conf->stop_bits), sizeof(uart_conf->stop_bits));
	if (ret) {
		goto return_err;
	}

	key = PPP_SETT_KEY "/" PPP_SETT_UART_DATA_BITS;
	ret = settings_save_one(key, &(uart_conf->data_bits), sizeof(uart_conf->data_bits));
	if (ret) {
		goto return_err;
	}

	key = PPP_SETT_KEY "/" PPP_SETT_UART_FLOW_CTRL;
	ret = settings_save_one(key, &(uart_conf->flow_ctrl), sizeof(uart_conf->flow_ctrl));
	if (ret) {
		goto return_err;
	}

	ret = ppp_settings_uart_dev_conf_set(uart_conf);
	if (ret) {
		shell_error(shell_global, "Failed to set setting to PPP uart %s, error: %d", ret);
		return ret;
	}

	memcpy(&ppp_uart_conf, uart_conf, sizeof(ppp_uart_conf));
	return 0;

return_err:
	shell_error(shell_global, "Failed to save key %s, error: %d", key, ret);
	return ret;
}

/******************************************************************************/

static struct settings_handler cfg = { .name = PPP_SETT_KEY, .h_set = ppp_settings_handler };

int ppp_settings_init(void)
{
	int ret = 0;

	ppp_uart_conf_valid = false;

	ppp_uart_dev = device_get_binding(CONFIG_NET_PPP_UART_NAME);
	if (!ppp_uart_dev) {
		shell_warn(shell_global, "Cannot get ppp dev binding");
		ppp_uart_dev = NULL;
	}

	/* Read defaults: */
	ret = ppp_settings_uart_dev_conf_get(&ppp_uart_conf);
	if (ret) {
		shell_error(shell_global, "Failed to get current uart settings, error: %d", ret);
		return ret;
	}

	ret = settings_subsys_init();
	if (ret) {
		shell_error(shell_global, "Failed to initialize settings subsystem, error: %d",
			    ret);
		return ret;
	}

	ret = settings_register(&cfg);
	if (ret) {
		shell_error(shell_global, "Cannot register settings handler %d", ret);
		return ret;
	}

	ret = settings_load();
	if (ret) {
		shell_error(shell_global, "Cannot load settings %d", ret);
		return ret;
	}

	/* Set possible custom uart config: */
	ret = ppp_settings_uart_dev_conf_set(&ppp_uart_conf);
	if (ret) {
		shell_error(shell_global, "Failed to set custom uart settings, error: %d", ret);
		return ret;
	}
	ppp_uart_conf_valid = true;

	return ret;
}
