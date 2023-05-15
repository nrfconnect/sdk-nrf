/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <modem/at_cmd_custom.h>
#include <nrf_errno.h>
#include <nrf_modem_at.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <zephyr/toolchain/common.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(at_cmd_custom, CONFIG_AT_CMD_CUSTOM_LOG_LEVEL);

#define CMD_COUNT (_nrf_modem_at_cmd_custom_list_end - _nrf_modem_at_cmd_custom_list_start)

static int at_cmd_custom_sys_init(void)
{
	int err;
	extern struct nrf_modem_at_cmd_custom _nrf_modem_at_cmd_custom_list_start[];
	extern struct nrf_modem_at_cmd_custom _nrf_modem_at_cmd_custom_list_end[];

	err = nrf_modem_at_cmd_custom_set(_nrf_modem_at_cmd_custom_list_start, CMD_COUNT);
	LOG_INF("Custom AT commands enabled with %d entries.", CMD_COUNT);

	return err;
}

/* Fill response buffer without overflowing the buffer. */
int at_cmd_custom_respond(char *buf, size_t buf_size,
			  const char *response, ...)
{
	va_list args;
	size_t buf_size_required;

	if (buf == NULL) {
		LOG_ERR("%s called with NULL buffer", __func__);
		return -NRF_EFAULT;
	}

	va_start(args, response);
	buf_size_required = vsnprintf(buf, buf_size, response, args);
	va_end(args);

	if (buf_size_required > buf_size) {
		LOG_ERR("%s: formatted response exceeds the response buffer (%d > %d)",
		__func__, buf_size_required, buf_size);
		return -NRF_E2BIG;
	}

	return 0;
}

SYS_INIT(at_cmd_custom_sys_init, APPLICATION, 0);
