/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <modem/nrf_modem_lib.h>
#include <modem/modem_info.h>
#include <lwm2m_object.h>
#include <net/lwm2m_client_utils.h>
#include <net/lwm2m_client_utils_location.h>

#include "mosh_print.h"
#include "cloud_lwm2m.h"

BUILD_ASSERT(sizeof(CONFIG_MOSH_LWM2M_PSK) > 1, "LwM2M pre-shared key (PSK) must be configured");

/* Device RIDs */
#define MANUFACTURER_RID 0
#define MODEL_NUMBER_RID 1
#define SERIAL_NUMBER_RID 2
#define FACTORY_RESET_RID 5
#define POWER_SOURCE_RID 6
#define POWER_SOURCE_VOLTAGE_RID 7
#define POWER_SOURCE_CURRENT_RID 8
#define CURRENT_TIME_RID 13
#define UTC_OFFSET_RID 14
#define TIMEZONE_RID 15
#define DEVICE_TYPE_RID 17
#define HARDWARE_VERSION_RID 18
#define SOFTWARE_VERSION_RID 19
#define BATTERY_STATUS_RID 20
#define MEMORY_TOTAL_RID 21

#define IMEI_LEN 15
static uint8_t imei_buf[IMEI_LEN + sizeof("\r\nOK\r\n")];
#define ENDPOINT_NAME_LEN (IMEI_LEN + sizeof(CONFIG_MOSH_LWM2M_ENDPOINT_PREFIX) + 1)
static uint8_t endpoint_name[ENDPOINT_NAME_LEN + 1];
static struct lwm2m_ctx client;

#define APP_MANUFACTURER "Nordic Semiconductor ASA"
#define CLIENT_MODEL_NUMBER CONFIG_BOARD
#define CLIENT_HW_VER CONFIG_SOC
#define CLIENT_FLASH_SIZE CONFIG_FLASH_SIZE
#define UTC_OFFSET_STR_LEN 7 /* '+00:00' + '\0' = 7 */
static char utc_offset[UTC_OFFSET_STR_LEN] = "";
#define TIMEZONE_STR_LEN 33 /* Longest: 'America/Argentina/ComodRivadavia' + '\0' = 33 */
static char timezone[TIMEZONE_STR_LEN] = "";
#define APP_DEVICE_TYPE "OMA-LWM2M Client"
#if defined(APP_VERSION)
#define CLIENT_SW_VER STRINGIFY(APP_VERSION)
#else
#define CLIENT_SW_VER "unknown"
#endif
static uint8_t bat_status = LWM2M_DEVICE_BATTERY_STATUS_CHARGING;
static int mem_total = CLIENT_FLASH_SIZE;
static uint8_t bat_idx = LWM2M_DEVICE_PWR_SRC_TYPE_BAT_INT;
static int bat_mv = 3800;
static int bat_ma = 125;
static uint8_t usb_idx = LWM2M_DEVICE_PWR_SRC_TYPE_USB;
static int usb_mv = 5000;
static int usb_ma = 900;

static bool connected;
static bool no_serv_suspended;
static bool update_session_lifetime;

static void cloud_lwm2m_init(void);
static void cloud_lwm2m_rd_client_stop(void);

NRF_MODEM_LIB_ON_INIT(cloud_lwm2m_init_hook, on_modem_lib_init, NULL);

static void on_modem_lib_init(int ret, void *ctx)
{
	ARG_UNUSED(ret);
	ARG_UNUSED(ctx);

	static bool initialized;

	if (!initialized) {
		cloud_lwm2m_init();

		initialized = true;
	}
}

static int cloud_lwm2m_init_device(char *serial_num)
{
	lwm2m_set_res_buf(&LWM2M_OBJ(LWM2M_OBJECT_DEVICE_ID, 0, MANUFACTURER_RID),
			  APP_MANUFACTURER, sizeof(APP_MANUFACTURER),
			  sizeof(APP_MANUFACTURER), LWM2M_RES_DATA_FLAG_RO);
	lwm2m_set_res_buf(&LWM2M_OBJ(LWM2M_OBJECT_DEVICE_ID, 0, MODEL_NUMBER_RID),
			  CLIENT_MODEL_NUMBER, sizeof(CLIENT_MODEL_NUMBER),
			  sizeof(CLIENT_MODEL_NUMBER), LWM2M_RES_DATA_FLAG_RO);
	lwm2m_set_res_buf(&LWM2M_OBJ(LWM2M_OBJECT_DEVICE_ID, 0, SERIAL_NUMBER_RID),
			  serial_num, strlen(serial_num), strlen(serial_num),
			  LWM2M_RES_DATA_FLAG_RO);
	lwm2m_set_res_buf(&LWM2M_OBJ(LWM2M_OBJECT_DEVICE_ID, 0, UTC_OFFSET_RID),
			  utc_offset, sizeof(utc_offset), sizeof(utc_offset), 0);
	lwm2m_set_res_buf(&LWM2M_OBJ(LWM2M_OBJECT_DEVICE_ID, 0, TIMEZONE_RID),
			  timezone, sizeof(timezone), sizeof(timezone), 0);
	lwm2m_set_res_buf(&LWM2M_OBJ(LWM2M_OBJECT_DEVICE_ID, 0, DEVICE_TYPE_RID),
			  APP_DEVICE_TYPE, sizeof(APP_DEVICE_TYPE),
			  sizeof(APP_DEVICE_TYPE), LWM2M_RES_DATA_FLAG_RO);
	lwm2m_set_res_buf(&LWM2M_OBJ(LWM2M_OBJECT_DEVICE_ID, 0, HARDWARE_VERSION_RID),
			  CLIENT_HW_VER, sizeof(CLIENT_HW_VER), sizeof(CLIENT_HW_VER),
			  LWM2M_RES_DATA_FLAG_RO);
	lwm2m_set_res_buf(&LWM2M_OBJ(LWM2M_OBJECT_DEVICE_ID, 0, SOFTWARE_VERSION_RID),
			  CLIENT_SW_VER, sizeof(CLIENT_SW_VER), sizeof(CLIENT_SW_VER),
			  LWM2M_RES_DATA_FLAG_RO);
	lwm2m_set_res_buf(&LWM2M_OBJ(LWM2M_OBJECT_DEVICE_ID, 0, BATTERY_STATUS_RID),
			  &bat_status, sizeof(bat_status), sizeof(bat_status), 0);
	lwm2m_set_res_buf(&LWM2M_OBJ(LWM2M_OBJECT_DEVICE_ID, 0, MEMORY_TOTAL_RID),
			  &mem_total, sizeof(mem_total), sizeof(mem_total), 0);

	/* Add power source resource instances. */
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

	return 0;
}

static void cloud_lwm2m_lte_lc_evt_handler(const struct lte_lc_evt *const evt)
{
	if (evt->type != LTE_LC_EVT_NW_REG_STATUS) {
		return;
	}

	switch (evt->nw_reg_status) {
	case LTE_LC_NW_REG_REGISTERED_HOME:
	case LTE_LC_NW_REG_REGISTERED_ROAMING:
		if (connected && no_serv_suspended) {
			mosh_print("LwM2M: LTE connected, resuming LwM2M engine");
			lwm2m_engine_resume();
		}
		no_serv_suspended = false;
		break;

	case LTE_LC_NW_REG_NOT_REGISTERED:
	case LTE_LC_NW_REG_REGISTRATION_DENIED:
	case LTE_LC_NW_REG_UNKNOWN:
	case LTE_LC_NW_REG_UICC_FAIL:
		if (connected) {
			mosh_print("LwM2M: LTE not connected, suspending LwM2M engine");
			lwm2m_engine_pause();
			no_serv_suspended = true;
		}
		break;

	case LTE_LC_NW_REG_SEARCHING:
		/* Transient status, don't trigger suspend yet. */
	default:
		break;
	}
}

static void cloud_lwm2m_init(void)
{
	int ret;

	lte_lc_register_handler(cloud_lwm2m_lte_lc_evt_handler);

	ret = modem_info_init();
	if (ret < 0) {
		printk("LwM2M: Unable to init modem_info (%d)", ret);
		return;
	}

	/* Query IMEI. */
	ret = modem_info_string_get(MODEM_INFO_IMEI, imei_buf, sizeof(imei_buf));
	if (ret < 0) {
		printk("LwM2M: Unable to get IMEI");
		return;
	}

	/* Use IMEI as unique endpoint name. */
	snprintk(endpoint_name, sizeof(endpoint_name), "%s%s", CONFIG_MOSH_LWM2M_ENDPOINT_PREFIX,
		 imei_buf);

	lwm2m_init_device();

	cloud_lwm2m_init_device(imei_buf);
	lwm2m_init_security(&client, endpoint_name, NULL);

	if (sizeof(CONFIG_MOSH_LWM2M_PSK) > 1) {
		/* Write hard-coded PSK key to the engine. First security instance is the right
		 * one, because in bootstrap mode, it is the bootstrap PSK. In normal mode, it is
		 * the server key.
		 */
		lwm2m_security_set_psk(0, CONFIG_MOSH_LWM2M_PSK,
				       sizeof(CONFIG_MOSH_LWM2M_PSK), true,
				       endpoint_name);
	}

	/* Disable unnecessary time updates. */
	lwm2m_update_device_service_period(0);
}

static void cloud_lwm2m_rd_client_update_lifetime(int srv_obj_inst)
{
	lwm2m_set_u32(&LWM2M_OBJ(1, srv_obj_inst, 1), CONFIG_LWM2M_ENGINE_DEFAULT_LIFETIME);
	mosh_print("LwM2M: Set session lifetime to default value %d",
		   CONFIG_LWM2M_ENGINE_DEFAULT_LIFETIME);

	update_session_lifetime = false;
}

static void cloud_lwm2m_rd_client_event_cb(struct lwm2m_ctx *client_ctx,
					   enum lwm2m_rd_client_event client_event)
{
	switch (client_event) {
	case LWM2M_RD_CLIENT_EVENT_SERVER_DISABLED:
	case LWM2M_RD_CLIENT_EVENT_DEREGISTER:
	case LWM2M_RD_CLIENT_EVENT_NONE:
		/* Do nothing. */
		break;

	case LWM2M_RD_CLIENT_EVENT_BOOTSTRAP_REG_FAILURE:
		mosh_print("LwM2M: Bootstrap registration failure!");
		cloud_lwm2m_rd_client_stop();
		break;

	case LWM2M_RD_CLIENT_EVENT_BOOTSTRAP_REG_COMPLETE:
		mosh_print("LwM2M: Bootstrap registration complete");
		connected = false;
		/* After bootstrapping the session lifetime needs to be set to the configured
		 * default, otherwise the default value from the server is taken into use.
		 */
		update_session_lifetime = true;
		break;

	case LWM2M_RD_CLIENT_EVENT_BOOTSTRAP_TRANSFER_COMPLETE:
		mosh_print("LwM2M: Bootstrap transfer complete");
		break;

	case LWM2M_RD_CLIENT_EVENT_REGISTRATION_FAILURE:
		mosh_print("LwM2M: Registration failure!");
		connected = false;
		cloud_lwm2m_rd_client_stop();
		break;

	case LWM2M_RD_CLIENT_EVENT_REGISTRATION_COMPLETE:
		mosh_print("LwM2M: Registration complete");
		connected = true;
		/* Check if session lifetime needs to be updated. */
		if (update_session_lifetime) {
			cloud_lwm2m_rd_client_update_lifetime(client.srv_obj_inst);
		}
		break;

	case LWM2M_RD_CLIENT_EVENT_REG_TIMEOUT:
		mosh_print("LwM2M: Registration timed out!");
		connected = false;
		cloud_lwm2m_rd_client_stop();
		break;

	case LWM2M_RD_CLIENT_EVENT_REG_UPDATE:
		mosh_print("LwM2M: Registration update started");
		break;

	case LWM2M_RD_CLIENT_EVENT_REG_UPDATE_COMPLETE:
		mosh_print("LwM2M: Registration update complete");
		connected = true;
		break;

	case LWM2M_RD_CLIENT_EVENT_DEREGISTER_FAILURE:
		mosh_print("LwM2M: Deregister failure!");
		connected = false;
		break;

	case LWM2M_RD_CLIENT_EVENT_DISCONNECT:
		mosh_print("LwM2M: Disconnected");
		connected = false;
		break;

	case LWM2M_RD_CLIENT_EVENT_QUEUE_MODE_RX_OFF:
		mosh_print("LwM2M: Queue mode RX window closed");
		break;

	case LWM2M_RD_CLIENT_EVENT_ENGINE_SUSPENDED:
		mosh_print("LwM2M: Engine suspended");
		break;

	case LWM2M_RD_CLIENT_EVENT_NETWORK_ERROR:
		mosh_error("LwM2M: Engine reported a network error");
		connected = false;
		cloud_lwm2m_rd_client_stop();
		break;
	}
}

static void cloud_lwm2m_rd_client_stop_work_fn(struct k_work *work)
{
	ARG_UNUSED(work);

	mosh_print("LwM2M: Stopping LwM2M client");

	lwm2m_rd_client_stop(&client, cloud_lwm2m_rd_client_event_cb, false);
}

static K_WORK_DEFINE(cloud_lwm2m_rd_client_stop_work, cloud_lwm2m_rd_client_stop_work_fn);

static void cloud_lwm2m_rd_client_stop(void)
{
	k_work_submit(&cloud_lwm2m_rd_client_stop_work);
}

struct lwm2m_ctx *cloud_lwm2m_client_ctx_get(void)
{
	return &client;
}

bool cloud_lwm2m_is_connected(void)
{
	return connected && !no_serv_suspended;
}

int cloud_lwm2m_connect(void)
{
	uint32_t flags = 0;

	if (lwm2m_security_needs_bootstrap()) {
		flags |= LWM2M_RD_CLIENT_FLAG_BOOTSTRAP;
	}

	mosh_print("LwM2M: Starting LwM2M client");

	return lwm2m_rd_client_start(&client, endpoint_name, flags,
				     cloud_lwm2m_rd_client_event_cb, NULL);
}

int cloud_lwm2m_disconnect(void)
{
	mosh_print("LwM2M: Stopping LwM2M client");

	return lwm2m_rd_client_stop(&client, cloud_lwm2m_rd_client_event_cb, true);
}

int cloud_lwm2m_suspend(void)
{
	return lwm2m_engine_pause();
}

int cloud_lwm2m_resume(void)
{
	return lwm2m_engine_resume();
}

void cloud_lwm2m_update(void)
{
	lwm2m_rd_client_update();
}
