/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(temp_monitor);
#include "../common/ipc_comm.h"
#include "../common/temp_monitor.h"

/* Module implements temperature monitor API as a communication with VPR core
 * which executes the operations.
 */
static void ipc_comm_cb(void *user_data, const void *buf, size_t len);
static temp_monitor_event_handler_t event_handler;
static struct ipc_comm_data ipc_helper_data;
static struct ipc_comm ipc_helper;
static struct ipc_comm ipc_helper = IPC_COMM_INIT(TEMP_MONITOR_EP_NAME, ipc_comm_cb, NULL,
						  DEVICE_DT_GET(DT_ALIAS(remote_temp_monitor_ipc)),
						  &ipc_helper_data, &ipc_helper);

/* IPC callback */
static void ipc_comm_cb(void *user_data, const void *buf, size_t len)
{
	const struct temp_monitor_msg *msg = buf;

	if (event_handler) {
		event_handler(msg->type, msg->report.value);
	}
}

static int rem_temp_init(void)
{
	return ipc_comm_init(&ipc_helper);
}

void temp_monitor_set_handler(temp_monitor_event_handler_t handler)
{
	event_handler = handler;
}

int temp_monitor_start(const struct temp_monitor_start *params)
{
	struct temp_monitor_msg msg = {.type = TEMP_MONITOR_MSG_START, .start = *params};

	return ipc_comm_send(&ipc_helper, &msg, sizeof(msg));
}

int temp_monitor_stop(void)
{
	struct temp_monitor_msg msg = {.type = TEMP_MONITOR_MSG_STOP};

	return ipc_comm_send(&ipc_helper, &msg, sizeof(msg));
}

#if CONFIG_NORDIC_VPR_LAUNCHER_INIT_PRIORITY > CONFIG_IPC_SERVICE_REG_BACKEND_PRIORITY
#define BASE_PRIO CONFIG_NORDIC_VPR_LAUNCHER_INIT_PRIORITY
#else
#define BASE_PRIO CONFIG_IPC_SERVICE_REG_BACKEND_PRIORITY
#endif

/* Need to be started after vpr launcher and launcher can be postponed if STM logging is used. */
SYS_INIT(rem_temp_init, POST_KERNEL, UTIL_INC(BASE_PRIO));
