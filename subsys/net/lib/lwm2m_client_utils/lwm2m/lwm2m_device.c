/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/net/lwm2m.h>
#include <net/lwm2m_client_utils.h>
#include "lwm2m_engine.h"
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(lwm2m_device, CONFIG_LWM2M_CLIENT_UTILS_LOG_LEVEL);

#define REBOOT_DELAY K_SECONDS(1)

static void reboot_work_handler(struct k_work *work);

/* Delayed work that is used to trigger a reboot. */
static K_WORK_DELAYABLE_DEFINE(reboot_work, reboot_work_handler);

static void reboot_work_handler(struct k_work *work)
{
	int rc;

	rc = lte_lc_offline();
	if (rc) {
		LOG_ERR("Failed to put LTE link in offline state (%d)", rc);
	}

	LOG_PANIC();
	sys_reboot(0);
}

int lwm2m_device_reboot_cb(uint16_t obj_inst_id, uint8_t *args,
			    uint16_t args_len)
{
	ARG_UNUSED(obj_inst_id);
	ARG_UNUSED(args);
	ARG_UNUSED(args_len);

	LOG_INF("DEVICE: Reboot in progress");

	return k_work_schedule(&reboot_work, REBOOT_DELAY);
}

static int lwm2m_init_reboot_cb(void)
{
	return lwm2m_register_exec_callback(&LWM2M_OBJ(3, 0, 4), lwm2m_device_reboot_cb);
}

LWM2M_APP_INIT(lwm2m_init_reboot_cb);
