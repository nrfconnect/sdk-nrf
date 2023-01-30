/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/net/lwm2m.h>
#include <zephyr/net/lwm2m_path.h>
#include <ncs_version.h>

#include "pm_config.h"
#include "lwm2m_app_utils.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(app_lwm2m, CONFIG_APP_LOG_LEVEL);

#define CLIENT_MODEL_NUMBER CONFIG_BOARD
#define CLIENT_HW_VER CONFIG_SOC
#define CLIENT_FLASH_SIZE PM_MCUBOOT_SECONDARY_SIZE

#define UTC_OFFSET_STR_LEN 7 /* '+00:00' + '\0' = 7 */
#define TIMEZONE_STR_LEN 33 /* Longest: 'America/Argentina/ComodRivadavia' + '\0' = 33 */

static uint8_t bat_idx = LWM2M_DEVICE_PWR_SRC_TYPE_BAT_INT;
static int bat_mv;
static int bat_ma;
static uint8_t usb_idx = LWM2M_DEVICE_PWR_SRC_TYPE_USB;
static int usb_mv;
static int usb_ma;
static uint8_t bat_status = LWM2M_DEVICE_BATTERY_STATUS_CHARGING;
static int mem_total = (CLIENT_FLASH_SIZE / 1024);
static char utc_offset[UTC_OFFSET_STR_LEN] = "";
static char timezone[TIMEZONE_STR_LEN] = "";
static uint8_t bat_level;

static int device_factory_default_cb(uint16_t obj_inst_id, uint8_t *args, uint16_t args_len)
{
	ARG_UNUSED(args);
	ARG_UNUSED(args_len);

	LOG_INF("DEVICE: FACTORY DEFAULT (TODO)");

	return 0;
}

int lwm2m_app_init_device(char *serial_num)
{
	char *client_sw_ver = (strlen(CONFIG_APP_CUSTOM_VERSION) > 0) ?
			      CONFIG_APP_CUSTOM_VERSION : NCS_VERSION_STRING;

	lwm2m_set_res_buf(&LWM2M_OBJ(LWM2M_OBJECT_DEVICE_ID, 0, MANUFACTURER_RID),
			  CONFIG_APP_MANUFACTURER, sizeof(CONFIG_APP_MANUFACTURER),
			  sizeof(CONFIG_APP_MANUFACTURER), LWM2M_RES_DATA_FLAG_RO);
	lwm2m_set_res_buf(&LWM2M_OBJ(LWM2M_OBJECT_DEVICE_ID, 0, MODEL_NUMBER_RID),
			  CLIENT_MODEL_NUMBER, sizeof(CLIENT_MODEL_NUMBER),
			  sizeof(CLIENT_MODEL_NUMBER), LWM2M_RES_DATA_FLAG_RO);
	lwm2m_set_res_buf(&LWM2M_OBJ(LWM2M_OBJECT_DEVICE_ID, 0, SERIAL_NUMBER_RID),
			  serial_num, strlen(serial_num), strlen(serial_num),
			  LWM2M_RES_DATA_FLAG_RO);
	lwm2m_register_exec_callback(&LWM2M_OBJ(LWM2M_OBJECT_DEVICE_ID, 0,
						FACTORY_RESET_RID),
				     device_factory_default_cb);
	lwm2m_set_res_buf(&LWM2M_OBJ(LWM2M_OBJECT_DEVICE_ID, 0, UTC_OFFSET_RID),
			  utc_offset, sizeof(utc_offset), sizeof(utc_offset), 0);
	lwm2m_set_res_buf(&LWM2M_OBJ(LWM2M_OBJECT_DEVICE_ID, 0, TIMEZONE_RID),
			  timezone, sizeof(timezone), sizeof(timezone), 0);
	lwm2m_set_res_buf(&LWM2M_OBJ(LWM2M_OBJECT_DEVICE_ID, 0, DEVICE_TYPE_RID),
			  CONFIG_APP_DEVICE_TYPE, sizeof(CONFIG_APP_DEVICE_TYPE),
			  sizeof(CONFIG_APP_DEVICE_TYPE), LWM2M_RES_DATA_FLAG_RO);
	lwm2m_set_res_buf(&LWM2M_OBJ(LWM2M_OBJECT_DEVICE_ID, 0, HARDWARE_VERSION_RID),
			  CLIENT_HW_VER, sizeof(CLIENT_HW_VER), sizeof(CLIENT_HW_VER),
			  LWM2M_RES_DATA_FLAG_RO);
	lwm2m_set_res_buf(&LWM2M_OBJ(LWM2M_OBJECT_DEVICE_ID, 0, SOFTWARE_VERSION_RID),
			  client_sw_ver, strlen(client_sw_ver) + 1,
			  strlen(client_sw_ver) + 1, LWM2M_RES_DATA_FLAG_RO);
	lwm2m_set_res_buf(&LWM2M_OBJ(LWM2M_OBJECT_DEVICE_ID, 0, BATTERY_STATUS_RID),
			  &bat_status, sizeof(bat_status), sizeof(bat_status), 0);
	lwm2m_set_res_buf(&LWM2M_OBJ(LWM2M_OBJECT_DEVICE_ID, 0, MEMORY_TOTAL_RID),
			  &mem_total, sizeof(mem_total), sizeof(mem_total), 0);

	/* add power source resource instances */
	lwm2m_create_res_inst(&LWM2M_OBJ(LWM2M_OBJECT_DEVICE_ID, 0, POWER_SOURCE_RID, 0));
	lwm2m_set_res_buf(&LWM2M_OBJ(LWM2M_OBJECT_DEVICE_ID, 0, POWER_SOURCE_RID, 0),
			  &bat_idx, sizeof(bat_idx), sizeof(bat_idx), 0);
	lwm2m_create_res_inst(&LWM2M_OBJ(LWM2M_OBJECT_DEVICE_ID, 0, POWER_SOURCE_VOLTAGE_RID, 0));
	lwm2m_set_res_buf(&LWM2M_OBJ(LWM2M_OBJECT_DEVICE_ID, 0, POWER_SOURCE_VOLTAGE_RID, 0),
			  &bat_mv, sizeof(bat_mv), sizeof(bat_mv), 0);
	lwm2m_create_res_inst(&LWM2M_OBJ(LWM2M_OBJECT_DEVICE_ID, 0, POWER_SOURCE_CURRENT_RID, 0));
	lwm2m_set_res_buf(&LWM2M_OBJ(LWM2M_OBJECT_DEVICE_ID, 0, POWER_SOURCE_CURRENT_RID, 0),
			  &bat_ma, sizeof(bat_ma), sizeof(bat_ma), 0);
	lwm2m_create_res_inst(&LWM2M_OBJ(LWM2M_OBJECT_DEVICE_ID, 0, POWER_SOURCE_RID, 1));
	lwm2m_set_res_buf(&LWM2M_OBJ(LWM2M_OBJECT_DEVICE_ID, 0, POWER_SOURCE_RID, 1),
			  &usb_idx, sizeof(usb_idx), sizeof(usb_idx), 0);
	lwm2m_create_res_inst(&LWM2M_OBJ(LWM2M_OBJECT_DEVICE_ID, 0, POWER_SOURCE_VOLTAGE_RID, 1));
	lwm2m_set_res_buf(&LWM2M_OBJ(LWM2M_OBJECT_DEVICE_ID, 0, POWER_SOURCE_VOLTAGE_RID, 1),
			  &usb_mv, sizeof(usb_mv), sizeof(usb_mv), 0);
	lwm2m_create_res_inst(&LWM2M_OBJ(LWM2M_OBJECT_DEVICE_ID, 0, POWER_SOURCE_CURRENT_RID, 1));
	lwm2m_set_res_buf(&LWM2M_OBJ(LWM2M_OBJECT_DEVICE_ID, 0, POWER_SOURCE_CURRENT_RID, 1),
			  &usb_ma, sizeof(usb_ma), sizeof(usb_ma), 0);

	lwm2m_set_res_buf(&LWM2M_OBJ(LWM2M_OBJECT_DEVICE_ID, 0, BATTERY_LEVEL_RID),
			  &bat_level, sizeof(bat_level), sizeof(bat_level), 0);
	return 0;
}
