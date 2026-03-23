/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include "../common/ipc_comm.h"
#include "../common/led_control.h"
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(led_ctrl);

/* Module implements LED control API using remote service on the VPR core. */

static struct ipc_comm_data ipc_helper_data;
static struct ipc_comm ipc_helper;
static struct ipc_comm ipc_helper = IPC_COMM_INIT(LED_CONTROL_EP_NAME, NULL, NULL,
						  DEVICE_DT_GET(DT_ALIAS(remote_temp_monitor_ipc)),
						  &ipc_helper_data, &ipc_helper);

static int rem_led_control_init(void)
{
	return ipc_comm_init(&ipc_helper);
}

int led_control_start(uint32_t period_on, uint32_t period_off)
{
	struct led_control_msg msg = {.type = LED_CONTROL_MSG_START,
				      .start = {
					      .period_on = period_on,
					      .period_off = period_off,
				      }};

	return ipc_comm_send(&ipc_helper, &msg, sizeof(msg));
}

int led_control_stop(void)
{
	struct led_control_msg msg = {.type = LED_CONTROL_MSG_STOP};

	return ipc_comm_send(&ipc_helper, &msg, sizeof(msg));
}

#if CONFIG_NORDIC_VPR_LAUNCHER_INIT_PRIORITY > CONFIG_IPC_SERVICE_REG_BACKEND_PRIORITY
#define BASE_PRIO CONFIG_NORDIC_VPR_LAUNCHER_INIT_PRIORITY
#else
#define BASE_PRIO CONFIG_IPC_SERVICE_REG_BACKEND_PRIORITY
#endif

/* Need to be started after vpr launcher and launcher can be postponed if STM logging is used. */
SYS_INIT(rem_led_control_init, POST_KERNEL, UTIL_INC(BASE_PRIO));
