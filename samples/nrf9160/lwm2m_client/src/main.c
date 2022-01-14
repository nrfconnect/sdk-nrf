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
#include <event_manager.h>
#include <date_time.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(app_lwm2m_client, CONFIG_APP_LOG_LEVEL);

#include <modem/lte_lc.h>
#include <modem/modem_info.h>
#include <nrf_modem_at.h>

#if defined(CONFIG_LWM2M_DTLS_SUPPORT)
#if defined(CONFIG_MODEM_KEY_MGMT)
#include <modem/modem_key_mgmt.h>
#endif
#endif

#include "lwm2m_client_app.h"
#include "lwm2m_app_utils.h"
#include "sensor_module.h"
#include "gps_module.h"

#if !defined(CONFIG_LTE_LINK_CONTROL)
#error "Missing CONFIG_LTE_LINK_CONTROL"
#endif

BUILD_ASSERT(sizeof(CONFIG_APP_LWM2M_SERVER) > 1,
	     "CONFIG_APP_LWM2M_SERVER must be set in prj.conf");

#define APP_BANNER "Run LWM2M client"

#define IMEI_LEN 15
#define ENDPOINT_NAME_LEN (IMEI_LEN + sizeof(CONFIG_APP_ENDPOINT_PREFIX) + 1)

#define LWM2M_SECURITY_PRE_SHARED_KEY 0
#define LWM2M_SECURITY_RAW_PUBLIC_KEY 1
#define LWM2M_SECURITY_CERTIFICATE 2
#define LWM2M_SECURITY_NO_SEC 3

static uint8_t endpoint_name[ENDPOINT_NAME_LEN + 1];
static uint8_t imei_buf[IMEI_LEN + sizeof("\r\nOK\r\n")];
static struct lwm2m_ctx client;

#if defined(CONFIG_LWM2M_DTLS_SUPPORT)
#include "config.h"
#endif /* CONFIG_LWM2M_DTLS_SUPPORT */

static struct k_sem lwm2m_restart;

static void rd_client_event(struct lwm2m_ctx *client, enum lwm2m_rd_client_event client_event);

void client_acknowledge(void)
{
	lwm2m_acknowledge(&client);
}

static int remove_whitespace(char *buf)
{
	size_t i, j = 0, len;

	len = strlen(buf);
	for (i = 0; i < len; i++) {
		if (buf[i] >= 32 && buf[i] <= 126) {
			if (j != i) {
				buf[j] = buf[i];
			}

			j++;
		}
	}

	if (j < len) {
		buf[j] = '\0';
	}

	return 0;
}

static int query_modem(const char *cmd, char *buf, size_t buf_len)
{
	int ret;

	/* Using format string in case the command contains characters
	 * that need to be escaped.
	 */
	ret = nrf_modem_at_cmd(buf, buf_len, "%s", cmd);
	if (ret) {
		if (ret > 0) {
			LOG_ERR("nrf_modem_at_cmd[%s] error_type: %d, error_value: %d",
				log_strdup(cmd),
				nrf_modem_at_err_type(ret),
				nrf_modem_at_err(ret));
		} else {
			LOG_ERR("nrf_modem_at_cmd[%s] error: %d", log_strdup(cmd), ret);
		}

		strncpy(buf, "error", buf_len);
		return ret;
	}

	remove_whitespace(buf);
	return 0;
}

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
#if defined(CONFIG_LWM2M_LOCATION_OBJ_SUPPORT)
	initialise_gps();
#endif
	return 0;
}

int lwm2m_security_index_to_inst_id(int index);

static int find_server_security_instance(void)
{
	char pathstr[10];
	bool bootstrap;
	int i;
	int instance;
	int ret;

	for (i = 0; i < CONFIG_LWM2M_SECURITY_INSTANCE_COUNT; i++) {
		instance = lwm2m_security_index_to_inst_id(i);
		if (instance < 0) {
			LOG_DBG("Empty instance at index %d", i);
			continue;
		}

		snprintk(pathstr, sizeof(pathstr), "0/%d/1", instance);

		ret = lwm2m_engine_get_bool(pathstr, &bootstrap);
		if (ret < 0) {
			LOG_ERR("Failed to check bootstrap (%d)", ret);
			continue;
		}

		if (!bootstrap) {
			LOG_DBG("Security instance found, %d", instance);
			return instance;
		}
	}

	return -1;
}

static int get_security_mode(int instance)
{
	char pathstr[10];
	uint8_t security_mode;
	int ret;

	snprintk(pathstr, sizeof(pathstr), "0/%d/2", instance);

	ret = lwm2m_engine_get_u8(pathstr, &security_mode);
	if (ret < 0) {
		LOG_ERR("Failed to obtain security mode (%d)", ret);
		return -1;
	}

	return security_mode;
}

#if defined(CONFIG_LWM2M_DTLS_SUPPORT)
static void provision_psk(int instance)
{
	char pathstr[10];
	char psk_hex[64];
	int ret;
	uint8_t *identity;
	uint16_t identity_len;
	uint8_t *psk;
	uint16_t psk_len;
	uint8_t flags;

	/* Obtain identity. */
	snprintk(pathstr, sizeof(pathstr), "0/%d/3", instance);

	ret = lwm2m_engine_get_res_data(pathstr, (void **)&identity, &identity_len, &flags);
	if (ret < 0) {
		LOG_ERR("Failed to obtain client identity (%d)", ret);
		return;
	}

	/* Obtain PSK. */
	snprintk(pathstr, sizeof(pathstr), "0/%d/5", instance);

	ret = lwm2m_engine_get_res_data(pathstr, (void **)&psk, &psk_len, &flags);
	if (ret < 0) {
		LOG_ERR("Failed to obtain PSK (%d)", ret);
		return;
	}

	/* Convert PSK to a format accepted by the modem. */
	psk_len = bin2hex(psk, psk_len, psk_hex, sizeof(psk_hex));
	if (psk_len == 0) {
		LOG_ERR("PSK is too large to convert (%d)", -EOVERFLOW);
		return;
	}

	lwm2m_rd_client_stop(&client, rd_client_event, false);
	lte_lc_offline();

	ret = modem_key_mgmt_write(client.tls_tag, MODEM_KEY_MGMT_CRED_TYPE_PSK, psk_hex, psk_len);
	if (ret < 0) {
		LOG_ERR("Setting cred tag %d type %d failed (%d)", client.tls_tag,
			MODEM_KEY_MGMT_CRED_TYPE_PSK, ret);
		goto exit;
	}

	ret = modem_key_mgmt_write(client.tls_tag, MODEM_KEY_MGMT_CRED_TYPE_IDENTITY, identity,
				   identity_len);
	if (ret < 0) {
		LOG_ERR("Setting cred tag %d type %d failed (%d)", client.tls_tag,
			MODEM_KEY_MGMT_CRED_TYPE_IDENTITY, ret);
	}

exit:
	lte_lc_connect();
	lwm2m_rd_client_start(&client, endpoint_name, false, rd_client_event, NULL);
}
#endif

static void provision_credentials(void)
{
	int security_instance;
	int security_mode;

	security_instance = find_server_security_instance();
	if (security_instance == -1) {
		LOG_ERR("No security instance found");
		return;
	}

	security_mode = get_security_mode(security_instance);
	if (security_mode == -1) {
		return;
	}

	switch (security_mode) {
#if defined(CONFIG_LWM2M_DTLS_SUPPORT)
	case LWM2M_SECURITY_PRE_SHARED_KEY:
		LOG_DBG("PSK mode, provisioning key and identity.");
		client.tls_tag = SERVER_TLS_TAG;
		provision_psk(security_instance);
		break;
#endif

	case LWM2M_SECURITY_NO_SEC:
		LOG_DBG("NoSec mode, no provisioning needed.");
		break;

	default:
		LOG_ERR("Unsupported security mode (%d)", -ENOTSUP);
		break;
	}
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
		break;

	case LWM2M_RD_CLIENT_EVENT_BOOTSTRAP_REG_COMPLETE:
		LOG_DBG("Bootstrap registration complete");
		break;

	case LWM2M_RD_CLIENT_EVENT_BOOTSTRAP_TRANSFER_COMPLETE:
		LOG_DBG("Bootstrap transfer complete");
		LOG_DBG("Boostrap finished, provisioning credentials.");
		provision_credentials();
		break;

	case LWM2M_RD_CLIENT_EVENT_REGISTRATION_FAILURE:
		LOG_DBG("Registration failure!");
		break;

	case LWM2M_RD_CLIENT_EVENT_REGISTRATION_COMPLETE:
		LOG_DBG("Registration complete");
		/* Get current time and date */
		date_time_update_async(date_time_event_handler);
#if defined(CONFIG_APP_GPS)
		start_gps_search();
#endif
		break;

	case LWM2M_RD_CLIENT_EVENT_REG_UPDATE_FAILURE:
		LOG_DBG("Registration update failure!");
		break;

	case LWM2M_RD_CLIENT_EVENT_REG_UPDATE_COMPLETE:
		LOG_DBG("Registration update complete");
		break;

	case LWM2M_RD_CLIENT_EVENT_DEREGISTER_FAILURE:
		LOG_DBG("Deregister failure!");
		break;

	case LWM2M_RD_CLIENT_EVENT_DISCONNECT:
		LOG_DBG("Disconnected");
		break;

	case LWM2M_RD_CLIENT_EVENT_QUEUE_MODE_RX_OFF:
		LOG_DBG("Queue mode RX window closed");
		break;

	case LWM2M_RD_CLIENT_EVENT_NETWORK_ERROR:
		LOG_ERR("LwM2M engine reported a network error.");
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
	uint32_t flags = IS_ENABLED(CONFIG_LWM2M_RD_CLIENT_SUPPORT_BOOTSTRAP) ?
				       LWM2M_RD_CLIENT_FLAG_BOOTSTRAP :
				       0;
	int ret;

	LOG_INF(APP_BANNER);

	k_sem_init(&lwm2m_restart, 0, 1);

	ret = event_manager_init();
	if (ret) {
		LOG_ERR("Unable to init event manager (%d)", ret);
		return;
	}

#if !defined(CONFIG_NRF_MODEM_LIB_SYS_INIT)
	ret = nrf_modem_lib_init(NORMAL_MODE);
	if (ret != 0 && ret != MODEM_DFU_RESULT_OK) {
		LOG_ERR("Unable to init modem library (%d)", ret);
		return;
	}
#endif
#if defined(CONFIG_LWM2M_CLIENT_UTILS_FIRMWARE_UPDATE_OBJ_SUPPORT)
	ret = fota_settings_init();
	if (ret < 0) {
		LOG_ERR("Unable to init settings (%d)", ret);
		return;
	}
#endif

	/* Load *all* persistent settings */
	settings_load();

#if defined(CONFIG_LWM2M_CLIENT_UTILS_FIRMWARE_UPDATE_OBJ_SUPPORT)
	/* Modem FW update needs to be verified before modem is used. */
	lwm2m_verify_modem_fw_update();
#endif

	LOG_INF("Initializing modem.");
	ret = lte_lc_init();
	if (ret < 0) {
		LOG_ERR("Unable to init modem (%d)", ret);
		return;
	}

	/* query IMEI */
	ret = query_modem("AT+CGSN", imei_buf, sizeof(imei_buf));

	if (ret != 0) {
		LOG_ERR("Unable to get IMEI");
		return;
	}

	/* remove trailing AT "OK" message */
	uint8_t *ok_resp = strstr(imei_buf, "OK");

	uint32_t index = ok_resp - imei_buf;

	imei_buf[index] = '\0';

	/* use IMEI as unique endpoint name */
	snprintk(endpoint_name, sizeof(endpoint_name), "%s%s", CONFIG_APP_ENDPOINT_PREFIX,
		 imei_buf);
	LOG_INF("endpoint: %s", log_strdup(endpoint_name));

	/* Setup LwM2M */
	(void)memset(&client, 0x0, sizeof(client));

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

#if defined(CONFIG_LWM2M_DTLS_SUPPORT)
	ret = modem_key_mgmt_write(client.tls_tag, MODEM_KEY_MGMT_CRED_TYPE_PSK, client_psk,
				   strlen(client_psk));
	if (ret < 0) {
		LOG_ERR("Setting cred tag %d type %d failed (%d)", client.tls_tag,
			MODEM_KEY_MGMT_CRED_TYPE_PSK, ret);
	}

	ret = modem_key_mgmt_write(client.tls_tag, MODEM_KEY_MGMT_CRED_TYPE_IDENTITY, endpoint_name,
				   strlen(endpoint_name));
	if (ret < 0) {
		LOG_ERR("Setting cred tag %d type %d failed (%d)", client.tls_tag,
			MODEM_KEY_MGMT_CRED_TYPE_IDENTITY, ret);
	}
#endif

	modem_connect();

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

	while (true) {
		lwm2m_rd_client_start(&client, endpoint_name, flags, rd_client_event, NULL);

		k_sem_take(&lwm2m_restart, K_FOREVER);

		LOG_INF("LwM2M restart requested. The sample will try to"
			" re-establish network connection.");

		/* Stop the LwM2M engine. */
		lwm2m_rd_client_stop(&client, rd_client_event, false);

		/* Try to reconnect to the network. */
		ret = lte_lc_offline();
		if (ret < 0) {
			LOG_ERR("Failed to put LTE link in offline state (%d)", ret);
		}

		modem_connect();
	}
}
