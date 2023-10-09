/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>
#include <zephyr/logging/log.h>

#include "remote_bt.h"

LOG_MODULE_DECLARE(remote_radio, CONFIG_REMOTE_RADIO_LOG_LEVEL);

int remote_bt_init(void)
{
	LOG_DBG("Empty remote_bt_init called.");
	return -ENOSYS;
}

int remote_bt_process(void)
{
	LOG_DBG("Empty remote_bt_process called.");
	return -ENOSYS;
}
