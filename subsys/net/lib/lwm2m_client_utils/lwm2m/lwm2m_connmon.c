/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <stdlib.h>
#include <zephyr/net/lwm2m.h>
#include <net/lwm2m_client_utils.h>
#include <net/lwm2m_client_utils_location.h>

#include <modem/lte_lc.h>
#include <modem/modem_info.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(lwm2m_connmon, CONFIG_LWM2M_CLIENT_UTILS_LOG_LEVEL);

static struct modem_param_info modem_param;
static struct k_work modem_data_work;
static struct k_work modem_signal_work;
static int32_t modem_rsrp;

/* LTE-FDD bearer & NB-IoT bearer */
#define LTE_FDD_BEARER 6U
#define NB_IOT_BEARER 7U

static uint8_t bearers[2] = { LTE_FDD_BEARER, NB_IOT_BEARER };

static void modem_data_update(struct k_work *work)
{
	int ret;
	enum lte_lc_lte_mode mode;
	LOG_INF("Updating modem data on connmon");

	ret = modem_info_params_get(&modem_param);
	if (ret < 0) {
		LOG_ERR("Unable to obtain modem parameters: %d", ret);
		return;
	}

	ret = lte_lc_lte_mode_get(&mode);
	if (ret < 0) {
		LOG_ERR("Unable to obtain current LTE mode: %d", ret);
		return;
	}

	switch (mode) {
	case LTE_LC_LTE_MODE_LTEM:
		lwm2m_engine_set_u8("4/0/0", LTE_FDD_BEARER);
		break;

	case LTE_LC_LTE_MODE_NBIOT:
		lwm2m_engine_set_u8("4/0/0", NB_IOT_BEARER);
		break;

	case LTE_LC_LTE_MODE_NONE:
	default:
		LOG_DBG("No LTE mode information available");
		break;
	}

	lwm2m_engine_create_res_inst("4/0/1/0");
	lwm2m_engine_set_res_buf("4/0/1/0", &bearers[0], sizeof(bearers[0]), sizeof(bearers[0]),
				  LWM2M_RES_DATA_FLAG_RO);

	lwm2m_engine_create_res_inst("4/0/1/1");
	lwm2m_engine_set_res_buf("4/0/1/1", &bearers[1], sizeof(bearers[1]), sizeof(bearers[1]),
				  LWM2M_RES_DATA_FLAG_RO);
	/* interface IP address */
	lwm2m_engine_create_res_inst("4/0/4/0");
	lwm2m_engine_set_res_buf("4/0/4/0",
		modem_param.network.ip_address.value_string,
		sizeof(modem_param.network.ip_address.value_string),
		sizeof(modem_param.network.ip_address.value_string),
		LWM2M_RES_DATA_FLAG_RO);
	/* APN */
	lwm2m_engine_create_res_inst("4/0/7/0");
	lwm2m_engine_set_res_buf("4/0/7/0",
		modem_param.network.apn.value_string,
		strlen(modem_param.network.apn.value_string),
		strlen(modem_param.network.apn.value_string),
		LWM2M_RES_DATA_FLAG_RO);

	lwm2m_engine_set_u32("4/0/8", (uint32_t)modem_param.network.cellid_dec);
	lwm2m_engine_set_u16("4/0/9", modem_param.network.mnc.value);
	lwm2m_engine_set_u16("4/0/10", modem_param.network.mcc.value);
#if defined(CONFIG_LWM2M_CONNMON_OBJECT_VERSION_1_2)
	lwm2m_engine_set_u16("4/0/12", modem_param.network.area_code.value);
#endif

	/* Set "Firmware Version" as modem firmware version in device object.
	 * Do it here not to repeat the process elsewhere - we read the FW
	 * version from the `modem_param_info` structure.
	 */
	lwm2m_engine_set_res_buf("3/0/3",
		modem_param.device.modem_fw.value_string,
		strlen(modem_param.device.modem_fw.value_string),
		strlen(modem_param.device.modem_fw.value_string),
		LWM2M_RES_DATA_FLAG_RO);
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
	     CONFIG_APP_HOLD_TIME_RSRP * MSEC_PER_SEC)) {
		return;
	}

	lwm2m_engine_set_s8("4/0/2", modem_rsrp);
	timestamp_prev = k_uptime_get_32();
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

	modem_info_params_init(&modem_param);
	return 0;
}

int lwm2m_update_connmon(void)
{
	k_work_submit(&modem_data_work);
	return modem_info_rsrp_register(modem_signal_handler);
}
