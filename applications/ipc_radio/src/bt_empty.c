/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>
#include <zephyr/logging/log.h>

#include "ipc_bt.h"

LOG_MODULE_DECLARE(ipc_radio, CONFIG_IPC_RADIO_LOG_LEVEL);

int ipc_bt_init(void)
{
	LOG_DBG("Empty ipc_bt_init called.");
	return -ENOSYS;
}

int ipc_bt_process(void)
{
	LOG_DBG("Empty ipc_bt_process called.");
	return -ENOSYS;
}
