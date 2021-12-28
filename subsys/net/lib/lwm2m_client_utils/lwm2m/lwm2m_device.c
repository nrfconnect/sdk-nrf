/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <version.h>
#include <logging/log_ctrl.h>
#include <sys/reboot.h>
#include <net/lwm2m.h>
#include "pm_config.h"
#include <net/lwm2m_client_utils.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(lwm2m_device, CONFIG_LWM2M_CLIENT_UTILS_LOG_LEVEL);

#define REBOOT_DELAY K_SECONDS(1)

static struct k_work_delayable reboot_work;

static void reboot_work_handler(struct k_work *work)
{
	LOG_PANIC();
	sys_reboot(0);
}

static int device_reboot_cb(uint16_t obj_inst_id, uint8_t *args,
			    uint16_t args_len)
{
	ARG_UNUSED(args);
	ARG_UNUSED(args_len);

	LOG_INF("DEVICE: Reboot in progress");

	k_work_schedule(&reboot_work, REBOOT_DELAY);

	return 0;
}

int lwm2m_init_device(void)
{
	k_work_init_delayable(&reboot_work, reboot_work_handler);

	lwm2m_engine_register_exec_callback("3/0/4", device_reboot_cb);

	return 0;
}
