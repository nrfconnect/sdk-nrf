/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <version.h>
#include <logging/log_ctrl.h>
#include <sys/reboot.h>
#include <net/lwm2m.h>
#include "pm_config.h"
#include "lwm2m_client_app.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(app_lwm2m_device, CONFIG_APP_LOG_LEVEL);

#define CLIENT_MODEL_NUMBER CONFIG_BOARD
#define CLIENT_HW_VER CONFIG_SOC
#define CLIENT_FLASH_SIZE PM_MCUBOOT_SECONDARY_SIZE

static uint8_t bat_idx = LWM2M_DEVICE_PWR_SRC_TYPE_BAT_INT;
static int bat_mv = 3800;
static int bat_ma = 125;
static uint8_t usb_idx = LWM2M_DEVICE_PWR_SRC_TYPE_USB;
static int usb_mv = 5000;
static int usb_ma = 900;
static uint8_t bat_status = LWM2M_DEVICE_BATTERY_STATUS_CHARGING;
static int mem_total = (CLIENT_FLASH_SIZE / 1024);

static int device_factory_default_cb(uint16_t obj_inst_id, uint8_t *args,
				     uint16_t args_len)
{
	ARG_UNUSED(args);
	ARG_UNUSED(args_len);

	LOG_INF("DEVICE: FACTORY DEFAULT (TODO)");

	return 0;
}

int lwm2m_app_init_device(char *serial_num)
{
	lwm2m_engine_set_res_data("3/0/0", CONFIG_APP_MANUFACTURER,
				  sizeof(CONFIG_APP_MANUFACTURER),
				  LWM2M_RES_DATA_FLAG_RO);
	lwm2m_engine_set_res_data("3/0/1", CLIENT_MODEL_NUMBER,
				  sizeof(CLIENT_MODEL_NUMBER),
				  LWM2M_RES_DATA_FLAG_RO);
	lwm2m_engine_set_res_data("3/0/2", serial_num, strlen(serial_num),
				  LWM2M_RES_DATA_FLAG_RO);
	lwm2m_engine_register_exec_callback("3/0/5", device_factory_default_cb);
	lwm2m_engine_set_res_data("3/0/17", CONFIG_APP_DEVICE_TYPE,
				  sizeof(CONFIG_APP_DEVICE_TYPE),
				  LWM2M_RES_DATA_FLAG_RO);
	lwm2m_engine_set_res_data("3/0/18", CLIENT_HW_VER,
				  sizeof(CLIENT_HW_VER),
				  LWM2M_RES_DATA_FLAG_RO);
	lwm2m_engine_set_res_data("3/0/20", &bat_status, sizeof(bat_status), 0);
	lwm2m_engine_set_res_data("3/0/21", &mem_total, sizeof(mem_total), 0);

	/* add power source resource instances */
	lwm2m_engine_create_res_inst("3/0/6/0");
	lwm2m_engine_set_res_data("3/0/6/0", &bat_idx, sizeof(bat_idx), 0);
	lwm2m_engine_create_res_inst("3/0/7/0");
	lwm2m_engine_set_res_data("3/0/7/0", &bat_mv, sizeof(bat_mv), 0);
	lwm2m_engine_create_res_inst("3/0/8/0");
	lwm2m_engine_set_res_data("3/0/8/0", &bat_ma, sizeof(bat_ma), 0);
	lwm2m_engine_create_res_inst("3/0/6/1");
	lwm2m_engine_set_res_data("3/0/6/1", &usb_idx, sizeof(usb_idx), 0);
	lwm2m_engine_create_res_inst("3/0/7/1");
	lwm2m_engine_set_res_data("3/0/7/1", &usb_mv, sizeof(usb_mv), 0);
	lwm2m_engine_create_res_inst("3/0/8/1");
	lwm2m_engine_set_res_data("3/0/8/1", &usb_ma, sizeof(usb_ma), 0);

	return 0;
}
