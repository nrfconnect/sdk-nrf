/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/net/lwm2m.h>
#include <zephyr/logging/log.h>
#include <lwm2m_engine.h>
#include <net/lwm2m_client_utils.h>
#include <ncs_version.h>

#include "pm_config.h"

#ifdef CONFIG_SOC_SERIES_NRF91X
#include <modem/modem_info.h>
#endif

LOG_MODULE_DECLARE(slm_lwm2m, CONFIG_SLM_LOG_LEVEL);

#define CLIENT_MANUFACTURER "Nordic Semiconductor ASA"
#define CLIENT_MODEL_NUMBER CONFIG_BOARD
#define CLIENT_FLASH_SIZE PM_MCUBOOT_SECONDARY_SIZE
#define CLIENT_DEVICE_TYPE "Serial LTE Modem"

/* Device RIDs */
#define MANUFACTURER_RID 0
#define MODEL_NUMBER_RID 1
#define SERIAL_NUMBER_RID 2
#define FIRMWARE_VERSION_RID 3
#define FACTORY_RESET_RID 5
#define POWER_SOURCE_RID 6
#define POWER_SOURCE_VOLTAGE_RID 7
#define POWER_SOURCE_CURRENT_RID 8
#define BATTERY_LEVEL_RID 9
#define CURRENT_TIME_RID 13
#define UTC_OFFSET_RID 14
#define TIMEZONE_RID 15
#define DEVICE_TYPE_RID 17
#define HARDWARE_VERSION_RID 18
#define SOFTWARE_VERSION_RID 19
#define BATTERY_STATUS_RID 20
#define MEMORY_TOTAL_RID 21

#define UTC_OFFSET_STR_LEN 7
#define TIMEZONE_STR_LEN 33

static uint8_t bat_level;
static char utc_offset[UTC_OFFSET_STR_LEN];
static char timezone[TIMEZONE_STR_LEN];
static uint8_t bat_status = LWM2M_DEVICE_BATTERY_STATUS_UNKNOWN;
static int mem_total = (CLIENT_FLASH_SIZE / 1024);

static int device_factory_default_cb(uint16_t obj_inst_id, uint8_t *args, uint16_t args_len)
{
	struct lwm2m_engine_res *dev_res;

	ARG_UNUSED(args);
	ARG_UNUSED(args_len);

	dev_res = lwm2m_engine_get_res(&LWM2M_OBJ(3, 0, 4));
	if (dev_res && dev_res->execute_cb) {
		dev_res->execute_cb(0, &(uint8_t){REBOOT_SOURCE_DEVICE_OBJ}, 1);
	}

	return 0;
}

int slm_lwm2m_init_device(char *serial_num)
{
	const void *hw_str = CONFIG_SOC;
	uint16_t hw_str_len = sizeof(CONFIG_SOC);
	static char fw_buf[MODEM_INFO_FWVER_SIZE];
	int err;

	if (IS_ENABLED(CONFIG_MODEM_INFO)) {
		err = modem_info_get_fw_version(fw_buf, sizeof(fw_buf));
		if (err != 0) {
			LOG_ERR("modem_info_get_fw_version() failed, err %d", err);
		}
	}

	if (IS_ENABLED(CONFIG_SOC_SERIES_NRF91X)) {
		static char hw_buf[sizeof("nRF91__ ____ ___ ")];

		err = modem_info_get_hw_version(hw_buf, sizeof(hw_buf));
		if (err == 0) {
			hw_str = hw_buf;
			hw_str_len = strlen(hw_buf) + 1;
		} else {
			LOG_ERR("modem_info_get_hw_version() failed, err %d", err);
		}
	}

	lwm2m_set_res_buf(&LWM2M_OBJ(LWM2M_OBJECT_DEVICE_ID, 0, MANUFACTURER_RID),
			  CLIENT_MANUFACTURER, sizeof(CLIENT_MANUFACTURER),
			  sizeof(CLIENT_MANUFACTURER), LWM2M_RES_DATA_FLAG_RO);
	lwm2m_set_res_buf(&LWM2M_OBJ(LWM2M_OBJECT_DEVICE_ID, 0, MODEL_NUMBER_RID),
			  CLIENT_MODEL_NUMBER, sizeof(CLIENT_MODEL_NUMBER),
			  sizeof(CLIENT_MODEL_NUMBER), LWM2M_RES_DATA_FLAG_RO);
	lwm2m_set_res_buf(&LWM2M_OBJ(LWM2M_OBJECT_DEVICE_ID, 0, SERIAL_NUMBER_RID),
			  serial_num, strlen(serial_num) + 1, strlen(serial_num) + 1,
			  LWM2M_RES_DATA_FLAG_RO);
	lwm2m_set_res_buf(&LWM2M_OBJ(LWM2M_OBJECT_DEVICE_ID, 0, FIRMWARE_VERSION_RID),
			  fw_buf, strlen(fw_buf) + 1, strlen(fw_buf) + 1, LWM2M_RES_DATA_FLAG_RO);
	lwm2m_register_exec_callback(&LWM2M_OBJ(LWM2M_OBJECT_DEVICE_ID, 0, FACTORY_RESET_RID),
				     device_factory_default_cb);
	lwm2m_set_res_buf(&LWM2M_OBJ(LWM2M_OBJECT_DEVICE_ID, 0, BATTERY_LEVEL_RID),
			  &bat_level, sizeof(bat_level), sizeof(bat_level), 0);
	lwm2m_set_res_buf(&LWM2M_OBJ(LWM2M_OBJECT_DEVICE_ID, 0, UTC_OFFSET_RID),
			  utc_offset, strlen(utc_offset) + 1, strlen(utc_offset) + 1, 0);
	lwm2m_set_res_buf(&LWM2M_OBJ(LWM2M_OBJECT_DEVICE_ID, 0, TIMEZONE_RID),
			  timezone, strlen(timezone) + 1, strlen(timezone) + 1, 0);
	lwm2m_set_res_buf(&LWM2M_OBJ(LWM2M_OBJECT_DEVICE_ID, 0, DEVICE_TYPE_RID),
			  CLIENT_DEVICE_TYPE, sizeof(CLIENT_DEVICE_TYPE),
			  sizeof(CLIENT_DEVICE_TYPE), LWM2M_RES_DATA_FLAG_RO);
	lwm2m_set_res_buf(&LWM2M_OBJ(LWM2M_OBJECT_DEVICE_ID, 0, HARDWARE_VERSION_RID),
			  (void *)hw_str, hw_str_len, hw_str_len, LWM2M_RES_DATA_FLAG_RO);
	lwm2m_set_res_buf(&LWM2M_OBJ(LWM2M_OBJECT_DEVICE_ID, 0, SOFTWARE_VERSION_RID),
			  (void *)NCS_VERSION_STRING, sizeof(NCS_VERSION_STRING),
			  sizeof(NCS_VERSION_STRING), LWM2M_RES_DATA_FLAG_RO);
	lwm2m_set_res_buf(&LWM2M_OBJ(LWM2M_OBJECT_DEVICE_ID, 0, BATTERY_STATUS_RID), &bat_status,
			  sizeof(bat_status), sizeof(bat_status), 0);
	lwm2m_set_res_buf(&LWM2M_OBJ(LWM2M_OBJECT_DEVICE_ID, 0, MEMORY_TOTAL_RID),
			  &mem_total, sizeof(mem_total), sizeof(mem_total), 0);

	return 0;
}
