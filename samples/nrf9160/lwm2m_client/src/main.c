/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <ctype.h>
#include <drivers/gpio.h>
#include <stdio.h>
#include <net/lwm2m.h>
#include <modem/nrf_modem_lib.h>
#include <settings/settings.h>

#include <net/lwm2m_client_utils.h>
#include <net/lwm2m_client_utils_fota.h>
#include <app_event_manager.h>
#include <net/lwm2m_client_utils_location.h>
#include <date_time.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(app_lwm2m_client, CONFIG_APP_LOG_LEVEL);

#include <modem/lte_lc.h>
#include <modem/modem_info.h>
#include <nrf_modem_at.h>
#include <modem/pdn.h>

#if defined(CONFIG_LWM2M_DTLS_SUPPORT)
#if defined(CONFIG_MODEM_KEY_MGMT)
#include <modem/modem_key_mgmt.h>
#endif
#endif

#include "lwm2m_client_app.h"
#include "lwm2m_app_utils.h"
#include "sensor_module.h"
#include "gnss_module.h"
#include "lwm2m_engine.h"

#if !defined(CONFIG_LTE_LINK_CONTROL)
#error "Missing CONFIG_LTE_LINK_CONTROL"
#endif

#define APP_BANNER "Run LWM2M client"

#define IMEI_LEN 15
#define ENDPOINT_NAME_LEN (IMEI_LEN + sizeof(CONFIG_APP_ENDPOINT_PREFIX) + 1)

#define LWM2M_SECURITY_PRE_SHARED_KEY 0
#define LWM2M_SECURITY_RAW_PUBLIC_KEY 1
#define LWM2M_SECURITY_CERTIFICATE 2
#define LWM2M_SECURITY_NO_SEC 3

static uint8_t endpoint_name[ENDPOINT_NAME_LEN + 1];
static uint8_t imei_buf[IMEI_LEN + sizeof("\r\nOK\r\n")];
static struct lwm2m_ctx client = {0};
static bool reconnect;
static int reconnection_counter;
static struct k_sem lwm2m_restart;

static void rd_client_event(struct lwm2m_ctx *client, enum lwm2m_rd_client_event client_event);

void client_acknowledge(void)
{
	lwm2m_acknowledge(&client);
}

#if defined(CONFIG_LWM2M_CLIENT_UTILS_SIGNAL_MEAS_INFO_OBJ_SUPPORT)
static struct k_work_delayable ncell_meas_work;
void ncell_meas_work_handler(struct k_work *work)
{
	lte_lc_neighbor_cell_measurement(LTE_LC_NEIGHBOR_SEARCH_TYPE_DEFAULT);
	k_work_schedule(&ncell_meas_work, K_SECONDS(CONFIG_APP_NEIGHBOUR_CELL_SCAN_INTERVAL));
}
#endif

static int lwm2m_setup(void)
{
#if defined(CONFIG_LWM2M_CLIENT_UTILS_DEVICE_OBJ_SUPPORT)
	/* Manufacturer independent */
	lwm2m_init_device();
#endif

	/* Manufacturer dependent */
	/* use IMEI as serial number */
	lwm2m_app_init_device(imei_buf);
	lwm2m_init_security(&client, endpoint_name);

	if (IS_ENABLED(CONFIG_LWM2M_DTLS_SUPPORT) && sizeof(CONFIG_APP_LWM2M_PSK) > 1) {
		/* Write hard-coded PSK key to engine */
		char buf[1 + sizeof(CONFIG_APP_LWM2M_PSK) / 2];
		size_t len = hex2bin(CONFIG_APP_LWM2M_PSK, sizeof(CONFIG_APP_LWM2M_PSK) - 1, buf,
				     sizeof(buf));

		/* First security instance is the right one, because in bootstrap mode, */
		/* it is the bootstrap PSK. In normal mode, it is the server key */
		lwm2m_engine_set_opaque("0/0/5", buf, len);
	}

#if defined(CONFIG_LWM2M_CLIENT_UTILS_FIRMWARE_UPDATE_OBJ_SUPPORT)
	lwm2m_init_firmware();
#endif
#if defined(CONFIG_LWM2M_CLIENT_UTILS_CONN_MON_OBJ_SUPPORT)
	lwm2m_init_connmon();
#endif
#if defined(CONFIG_LWM2M_APP_LIGHT_CONTROL)
	lwm2m_init_light_control();
#endif
#if defined(CONFIG_LWM2M_APP_TEMP_SENSOR)
	lwm2m_init_temp_sensor();
#endif
#if defined(CONFIG_LWM2M_APP_PRESS_SENSOR)
	lwm2m_init_press_sensor();
#endif
#if defined(CONFIG_LWM2M_APP_HUMID_SENSOR)
	lwm2m_init_humid_sensor();
#endif
#if defined(CONFIG_LWM2M_APP_GAS_RES_SENSOR)
	lwm2m_init_gas_res_sensor();
#endif
#if defined(CONFIG_LWM2M_APP_BUZZER)
	lwm2m_init_buzzer();
#endif
#if defined(CONFIG_LWM2M_APP_PUSH_BUTTON)
	lwm2m_init_push_button();
#endif
#if defined(CONFIG_LWM2M_APP_ONOFF_SWITCH)
	lwm2m_init_onoff_switch();
#endif
#if defined(CONFIG_LWM2M_APP_ACCELEROMETER)
	lwm2m_init_accel();
#endif
#if defined(CONFIG_LWM2M_APP_LIGHT_SENSOR)
	lwm2m_init_light_sensor();
#endif
#if defined(CONFIG_LWM2M_CLIENT_UTILS_SIGNAL_MEAS_INFO_OBJ_SUPPORT) && \
	defined(CONFIG_LWM2M_CLIENT_UTILS_NEIGHBOUR_CELL_LISTENER)
	init_neighbour_cell_info();
#endif
#if defined(CONFIG_LWM2M_PORTFOLIO_OBJ_SUPPORT)
	lwm2m_init_portfolio_object();
#endif
#if defined(CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_OBJ_SUPPORT) && \
	defined(CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_EVENTS)
	location_event_handler_init(&client);
#endif
	return 0;
}

static void date_time_event_handler(const struct date_time_evt *evt)
{
	switch (evt->type) {
	case DATE_TIME_OBTAINED_MODEM: {
		int64_t time = 0;

		LOG_INF("Obtained date-time from modem");
		date_time_now(&time);
		lwm2m_engine_set_s32(LWM2M_PATH(LWM2M_OBJECT_DEVICE_ID, 0, CURRENT_TIME_RID),
				     (int32_t)(time / 1000));
		break;
	}

	case DATE_TIME_OBTAINED_NTP: {
		int64_t time = 0;

		LOG_INF("Obtained date-time from NTP server");
		date_time_now(&time);
		lwm2m_engine_set_s32(LWM2M_PATH(LWM2M_OBJECT_DEVICE_ID, 0, CURRENT_TIME_RID),
				     (int32_t)(time / 1000));
		break;
	}

	case DATE_TIME_NOT_OBTAINED:
		LOG_INF("Could not obtain date-time update");
		break;

	default:
		break;
	}
}

static void rd_client_event(struct lwm2m_ctx *client, enum lwm2m_rd_client_event client_event)
{
	switch (client_event) {
	case LWM2M_RD_CLIENT_EVENT_NONE:
		/* do nothing */
		break;

	case LWM2M_RD_CLIENT_EVENT_BOOTSTRAP_REG_FAILURE:
		LOG_DBG("Bootstrap registration failure!");
		k_sem_give(&lwm2m_restart);
		break;

	case LWM2M_RD_CLIENT_EVENT_BOOTSTRAP_REG_COMPLETE:
		LOG_DBG("Bootstrap registration complete");
		reconnection_counter = 0;
		break;

	case LWM2M_RD_CLIENT_EVENT_BOOTSTRAP_TRANSFER_COMPLETE:
		LOG_DBG("Bootstrap transfer complete");
		break;

	case LWM2M_RD_CLIENT_EVENT_REGISTRATION_FAILURE:
		LOG_WRN("Registration failure!");
		k_sem_give(&lwm2m_restart);
		break;

	case LWM2M_RD_CLIENT_EVENT_REGISTRATION_COMPLETE:
		reconnection_counter = 0;
		LOG_DBG("Registration complete");
		/* Get current time and date */
		date_time_update_async(date_time_event_handler);
#if defined(CONFIG_APP_GNSS)
		start_gnss();
#endif
#if defined(CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_CELL)
		LOG_INF("Send cell location request event");
		struct cell_location_request_event *event = new_cell_location_request_event();

		APP_EVENT_SUBMIT(event);
#endif
		break;

	case LWM2M_RD_CLIENT_EVENT_REG_UPDATE_FAILURE:
		LOG_DBG("Registration update failure!");
		k_sem_give(&lwm2m_restart);
		break;

	case LWM2M_RD_CLIENT_EVENT_REG_UPDATE_COMPLETE:
		LOG_DBG("Registration update complete");
		break;

	case LWM2M_RD_CLIENT_EVENT_DEREGISTER_FAILURE:
		LOG_DBG("Deregister failure!");
		reconnect = true;
		k_sem_give(&lwm2m_restart);
		break;

	case LWM2M_RD_CLIENT_EVENT_DISCONNECT:
		LOG_DBG("Disconnected");
		break;

	case LWM2M_RD_CLIENT_EVENT_QUEUE_MODE_RX_OFF:
		LOG_DBG("Queue mode RX window closed");
		break;

	case LWM2M_RD_CLIENT_EVENT_NETWORK_ERROR:
		LOG_ERR("LwM2M engine reported a network error.");
		reconnect = true;
		k_sem_give(&lwm2m_restart);
		break;
	}
}

static void modem_connect(void)
{
	int ret;

#if defined(CONFIG_LWM2M_QUEUE_MODE_ENABLED)
	ret = lte_lc_psm_req(true);
	if (ret < 0) {
		LOG_ERR("lte_lc_psm_req, error: (%d)", ret);
	} else {
		LOG_INF("PSM mode requested");
	}
#endif

	do {
		LOG_INF("Connecting to LTE network.");
		LOG_INF("This may take several minutes.");

		ret = lte_lc_connect();
		if (ret < 0) {
			LOG_WRN("Failed to establish LTE connection (%d).", ret);
			LOG_WRN("Will retry in a minute.");
			lte_lc_offline();
			k_sleep(K_SECONDS(60));
		} else {
			LOG_INF("Connected to LTE network");
		}
	} while (ret < 0);
}

void main(void)
{
	int ret;
	uint32_t bootstrap_flags = 0;

	LOG_INF(APP_BANNER);

	k_sem_init(&lwm2m_restart, 0, 1);

#if !defined(CONFIG_NRF_MODEM_LIB_SYS_INIT)
	ret = nrf_modem_lib_init(NORMAL_MODE);
	if (ret < 0) {
		LOG_ERR("Unable to init modem library (%d)", ret);
		return;
	}
#endif

#if defined(CONFIG_LWM2M_CLIENT_UTILS_FIRMWARE_UPDATE_OBJ_SUPPORT)
	ret = fota_settings_init();
	if (ret < 0) {
		LOG_WRN("Unable to init settings (%d)", ret);
	}
	/* Modem FW update needs to be verified before modem is used. */
	lwm2m_verify_modem_fw_update();
#endif

#if defined(CONFIG_PDN)
	ret = pdn_init();
	if (ret < 0) {
		LOG_ERR("Unable to init pdn (%d)", ret);
		return;
	}
#endif

	ret = app_event_manager_init();
	if (ret) {
		LOG_ERR("Unable to init Application Event Manager (%d)", ret);
		return;
	}

	LOG_INF("Initializing modem.");
	ret = lte_lc_init();
	if (ret < 0) {
		LOG_ERR("Unable to init modem (%d)", ret);
		return;
	}

	ret = modem_info_init();
	if (ret < 0) {
		LOG_ERR("Unable to init modem_info (%d)", ret);
		return;
	}

	/* query IMEI */
	ret = modem_info_string_get(MODEM_INFO_IMEI, imei_buf, sizeof(imei_buf));
	if (ret < 0) {
		LOG_ERR("Unable to get IMEI");
		return;
	}

	/* use IMEI as unique endpoint name */
	snprintk(endpoint_name, sizeof(endpoint_name), "%s%s", CONFIG_APP_ENDPOINT_PREFIX,
		 imei_buf);
	LOG_INF("endpoint: %s", log_strdup(endpoint_name));

	/* Setup LwM2M */
	ret = lwm2m_setup();
	if (ret < 0) {
		LOG_ERR("Failed to setup LWM2M fields (%d)", ret);
		return;
	}

	ret = lwm2m_init_image();
	if (ret < 0) {
		LOG_ERR("Failed to setup image properties (%d)", ret);
		return;
	}

	reconnection_counter = 0;
	modem_connect();

#if defined(CONFIG_APP_GNSS)
	initialise_gnss();
#endif

#if defined(CONFIG_LWM2M_CLIENT_UTILS_CONN_MON_OBJ_SUPPORT)
	ret = lwm2m_update_connmon();
	if (ret < 0) {
		LOG_ERR("Registering rsrp handler failed (%d)", ret);
	}
#endif

#ifdef CONFIG_SENSOR_MODULE
	ret = sensor_module_init();
	if (ret) {
		LOG_ERR("Could not initialize sensor module (%d)", ret);
	}
#endif

#if defined(CONFIG_LWM2M_CLIENT_UTILS_SIGNAL_MEAS_INFO_OBJ_SUPPORT)
	k_work_init_delayable(&ncell_meas_work, ncell_meas_work_handler);
	k_work_schedule(&ncell_meas_work, K_SECONDS(1));
#endif

	while (true) {
		if (lwm2m_security_needs_bootstrap()) {
			bootstrap_flags = LWM2M_RD_CLIENT_FLAG_BOOTSTRAP;
		} else {
			bootstrap_flags = 0;
		}

		lwm2m_rd_client_start(&client, endpoint_name, bootstrap_flags, rd_client_event,
				      NULL);

		/* Wait for restart event */
		k_sem_take(&lwm2m_restart, K_FOREVER);

		/* Stop the LwM2M engine. */
		lwm2m_rd_client_stop(&client, rd_client_event, false);

		if (reconnect) {
			reconnect = false;
			reconnection_counter++;

			LOG_INF("LwM2M restart requested. The sample will try to"
				" re-establish network connection.");

			/* Try to reconnect to the network. */
			ret = lte_lc_offline();
			if (ret < 0) {
				LOG_ERR("Failed to put LTE link in offline state (%d)", ret);
			}
			modem_connect();
		}
	}
}
