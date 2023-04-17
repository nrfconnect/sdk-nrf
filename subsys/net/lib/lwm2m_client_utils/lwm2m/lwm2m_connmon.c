/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <stdlib.h>
#include <zephyr/net/lwm2m.h>
#include <net/lwm2m_client_utils.h>

#include <modem/lte_lc.h>
#include <modem/modem_info.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(lwm2m_connmon, CONFIG_LWM2M_CLIENT_UTILS_LOG_LEVEL);
#define IP_ADDR_LENGTH 46
#define APN_LENGTH 64
#define FW_VERSION_LENGTH 50

#define CONNMON_NETWORK_BEARER_ID		0
#define CONNMON_AVAIL_NETWORK_BEARER_ID		1
#define CONNMON_RADIO_SIGNAL_STRENGTH		2
#define CONNMON_LINK_QUALITY			3
#define CONNMON_IP_ADDRESSES			4
#define CONNMON_ROUTER_IP_ADDRESSES		5
#define CONNMON_LINK_UTILIZATION		6
#define CONNMON_APN				7
#define CONNMON_CELLID				8
#define CONNMON_SMNC				9
#define CONNMON_SMCC				10
#if defined(CONFIG_LWM2M_CONNMON_OBJECT_VERSION_1_2)
#define CONNMON_SIGNAL_SNR			11
#define CONNMON_LAC				12
#endif

#define DEVICE_FIRMWARE_VERSION_ID		3

static struct modem_param_info modem_param;
static struct k_work modem_data_work;
static struct k_work modem_signal_work;
static int32_t modem_rsrp;

static char *ip_addr[IP_ADDR_LENGTH];
static char *apn[APN_LENGTH];
static char *fw_version[FW_VERSION_LENGTH];

/* LTE-FDD bearer & NB-IoT bearer */
#define LTE_FDD_BEARER 6U
#define NB_IOT_BEARER 7U

static uint8_t bearers[2] = { LTE_FDD_BEARER, NB_IOT_BEARER };

static void connmon_data_init(void)
{
	lwm2m_create_res_inst(&LWM2M_OBJ(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0,
			      CONNMON_AVAIL_NETWORK_BEARER_ID, 0));
	lwm2m_set_res_buf(&LWM2M_OBJ(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0,
			  CONNMON_AVAIL_NETWORK_BEARER_ID, 0), &bearers[0],
			  sizeof(bearers[0]), sizeof(bearers[0]), LWM2M_RES_DATA_FLAG_RO);

	lwm2m_create_res_inst(&LWM2M_OBJ(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0,
			      CONNMON_AVAIL_NETWORK_BEARER_ID, 1));
	lwm2m_set_res_buf(&LWM2M_OBJ(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0,
			  CONNMON_AVAIL_NETWORK_BEARER_ID, 1), &bearers[1],
			  sizeof(bearers[1]), sizeof(bearers[1]), LWM2M_RES_DATA_FLAG_RO);
	/* interface IP address */
	lwm2m_create_res_inst(&LWM2M_OBJ(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0,
			      CONNMON_IP_ADDRESSES, 0));
	lwm2m_set_res_buf(&LWM2M_OBJ(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0,
			  CONNMON_IP_ADDRESSES, 0), ip_addr, sizeof(ip_addr), 0, 0);
	/* APN */
	lwm2m_create_res_inst(&LWM2M_OBJ(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0,
			      CONNMON_APN, 0));
	lwm2m_set_res_buf(&LWM2M_OBJ(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0,
			  CONNMON_APN, 0), apn, sizeof(apn), 0, 0);
	/* Set "Firmware Version" as modem firmware version in device object.
	 * Do it here not to repeat the process elsewhere - we read the FW
	 * version from the `modem_param_info` structure.
	 */
	lwm2m_set_res_buf(&LWM2M_OBJ(LWM2M_OBJECT_DEVICE_ID, 0, DEVICE_FIRMWARE_VERSION_ID),
			  fw_version, sizeof(fw_version), 0, 0);
}

static void modem_data_update(struct k_work *work)
{
	int ret;

	ret = modem_info_params_get(&modem_param);
	if (ret < 0) {
		LOG_ERR("Unable to obtain modem parameters: %d", ret);
		return;
	}

	lwm2m_set_string(&LWM2M_OBJ(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0,
			 CONNMON_IP_ADDRESSES, 0),
			 modem_param.network.ip_address.value_string);
	lwm2m_set_string(&LWM2M_OBJ(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0,
			 CONNMON_APN, 0), modem_param.network.apn.value_string);
	lwm2m_set_string(&LWM2M_OBJ(LWM2M_OBJECT_DEVICE_ID, 0, DEVICE_FIRMWARE_VERSION_ID),
			 modem_param.device.modem_fw.value_string);
	lwm2m_set_u32(&LWM2M_OBJ(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, CONNMON_CELLID),
		      (uint32_t)modem_param.network.cellid_dec);
	lwm2m_set_u16(&LWM2M_OBJ(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, CONNMON_SMNC),
		      modem_param.network.mnc.value);
	lwm2m_set_u16(&LWM2M_OBJ(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, CONNMON_SMCC),
		      modem_param.network.mcc.value);
#if defined(CONFIG_LWM2M_CONNMON_OBJECT_VERSION_1_2)
	lwm2m_set_u16(&LWM2M_OBJ(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, CONNMON_LAC),
		      modem_param.network.area_code.value);
#endif
}

/**@brief Callback handler for LTE RSRP data. */
static void modem_signal_handler(char rsrp_value)
{
	/* Only send a value from a valid range (0 - 97). */
	if (rsrp_value > 97) {
		return;
	}

	modem_rsrp = (int8_t)RSRP_IDX_TO_DBM(rsrp_value);
	k_work_submit(&modem_signal_work);
}

static void modem_signal_update(struct k_work *work)
{
	static uint32_t timestamp_prev;

	if ((timestamp_prev != 0) &&
	    (k_uptime_get_32() - timestamp_prev <
	     CONFIG_LWM2M_CONN_HOLD_TIME_RSRP * MSEC_PER_SEC)) {
		return;
	}

	lwm2m_set_s8(&LWM2M_OBJ(4, 0, 2), modem_rsrp);
	timestamp_prev = k_uptime_get_32();
}

static void lwm2m_update_connmon_cell(void)
{
	k_work_submit(&modem_data_work);
}

static void lwm2m_update_connmon_mode(const enum lte_lc_lte_mode lte_mode)
{
	switch (lte_mode) {
	case LTE_LC_LTE_MODE_LTEM:
		lwm2m_set_u8(&LWM2M_OBJ(4, 0, 0), LTE_FDD_BEARER);
		break;

	case LTE_LC_LTE_MODE_NBIOT:
		lwm2m_set_u8(&LWM2M_OBJ(4, 0, 0), NB_IOT_BEARER);
		break;

	case LTE_LC_LTE_MODE_NONE:
	default:
		LOG_DBG("No LTE mode information available");
		break;
	}
}

static void connmon_lte_notify_handler(const struct lte_lc_evt *const evt)
{
	int ret;
	static bool connected;

	switch (evt->type) {
	case LTE_LC_EVT_CELL_UPDATE:
		if (connected) {
			lwm2m_update_connmon_cell();
		}
		break;

	case LTE_LC_EVT_LTE_MODE_UPDATE:
		lwm2m_update_connmon_mode(evt->lte_mode);
		break;
	case LTE_LC_EVT_NW_REG_STATUS:
		connected = ((evt->nw_reg_status == LTE_LC_NW_REG_REGISTERED_HOME) ||
			    (evt->nw_reg_status == LTE_LC_NW_REG_REGISTERED_ROAMING));
		if (connected) {
			/* Register and update when connected as the rsrp register and
			 * modem_info_params_get will fail if called in disconnected state.
			 */
			ret = modem_info_rsrp_register(modem_signal_handler);
			if (ret) {
				LOG_ERR("Error registering rsrp handler: %d", ret);
			}
			lwm2m_update_connmon_cell();
		}
	default:
		break;
	}
}

int lwm2m_init_connmon(void)
{
	int ret;

	k_work_init(&modem_data_work, modem_data_update);
	k_work_init(&modem_signal_work, modem_signal_update);

	ret = modem_info_init();
	if (ret) {
		LOG_ERR("Modem info could not be established: %d", ret);
		return ret;
	}

	ret = modem_info_params_init(&modem_param);
	if (ret) {
		LOG_ERR("Modem parameters could not be initialized: %d", ret);
		return ret;
	}
	connmon_data_init();

	lte_lc_register_handler(connmon_lte_notify_handler);
	return 0;
}

SYS_INIT(lwm2m_init_connmon, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
