/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <modem/at_custom_cmd.h>
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

LOG_MODULE_REGISTER(at_custom_cmd, CONFIG_AT_CUSTOM_CMD_LOG_LEVEL);

static int at_custom_cmd_sys_init(void)
{
	int err;
	extern struct nrf_modem_at_cmd_filter _nrf_modem_at_cmd_filter_list_start[];
	int num_items = 0;

	STRUCT_SECTION_FOREACH(nrf_modem_at_cmd_filter, f) {
		LOG_DBG("AT filter: %s (%s)", f->cmd, f->paused ? "paused" : "enabled");
		num_items++;
	}

	err = nrf_modem_at_cmd_filter_set(_nrf_modem_at_cmd_filter_list_start, num_items);
	LOG_INF("AT filter enabled with %d entries.", num_items);

	return err;
}

/* Fill response buffer without overflowing the buffer. */
int at_custom_cmd_respond(char *buf, size_t buf_size,
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

void at_custom_cmd_pause(struct nrf_modem_at_cmd_filter *entry)
{
	entry->paused = true;
	LOG_DBG("%s(%s)", __func__, entry->cmd);
}

void at_custom_cmd_resume(struct nrf_modem_at_cmd_filter *entry)
{
	entry->paused = false;
	LOG_DBG("%s(%s)", __func__, entry->cmd);
}

SYS_INIT(at_custom_cmd_sys_init, APPLICATION, 0);
