/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <stdlib.h>
#include <net/lwm2m.h>

#include <modem/modem_info.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(app_lwm2m_connmon, CONFIG_APP_LOG_LEVEL);

static struct modem_param_info modem_param;
static struct k_work modem_data_work;
static struct k_work modem_signal_work;
static s32_t modem_rsrp;

/* LTE-FDD bearer & NB-IoT bearer */
static u8_t bearers[2] = { 6U, 7U };

static void modem_data_update(struct k_work *work)
{
	int ret;

	ret = modem_info_params_get(&modem_param);
	if (ret < 0) {
		LOG_ERR("Unable to obtain modem parameters: %d", ret);
		return;
	}

	/* TODO: Make this less hard-coded */
	lwm2m_engine_set_s8("4/0/0", bearers[0]);
	lwm2m_engine_create_res_inst("4/0/1/0");
	lwm2m_engine_set_res_data("4/0/1/0", &bearers[0], sizeof(bearers[0]),
				  LWM2M_RES_DATA_FLAG_RO);
	lwm2m_engine_create_res_inst("4/0/1/1");
	lwm2m_engine_set_res_data("4/0/1/1", &bearers[1], sizeof(bearers[1]),
				  LWM2M_RES_DATA_FLAG_RO);
	/* interface IP address */
	lwm2m_engine_create_res_inst("4/0/4/0");
	lwm2m_engine_set_res_data("4/0/4/0",
		modem_param.network.ip_address.value_string,
		sizeof(modem_param.network.ip_address.value_string),
		LWM2M_RES_DATA_FLAG_RO);
	/* APN */
	lwm2m_engine_create_res_inst("4/0/7/0");
	lwm2m_engine_set_res_data("4/0/7/0",
		modem_param.network.current_operator.value_string,
		sizeof(modem_param.network.current_operator.value_string),
		LWM2M_RES_DATA_FLAG_RO);

	lwm2m_engine_set_u16("4/0/9", modem_param.network.mnc.value);
	lwm2m_engine_set_u16("4/0/10", modem_param.network.mcc.value);

	/* set "Firmware Version" as modem firmware version in device object */
	lwm2m_engine_set_res_data("3/0/3",
		modem_param.device.modem_fw.value_string,
		strlen(modem_param.device.modem_fw.value_string),
		LWM2M_RES_DATA_FLAG_RO);
	/* set "Software Version" as NCS APP_VERSION in device object */
	lwm2m_engine_set_res_data("3/0/19",
		(char *)modem_param.device.app_version,
		strlen(modem_param.device.app_version),
		LWM2M_RES_DATA_FLAG_RO);
}

/**@brief Callback handler for LTE RSRP data. */
static void modem_signal_handler(char rsrp_value)
{
	modem_rsrp = (s8_t)rsrp_value - MODEM_INFO_RSRP_OFFSET_VAL;
	LOG_DBG("rsrp:%d", modem_rsrp);
	k_work_submit(&modem_signal_work);
}

static void modem_signal_update(struct k_work *work)
{
	static u32_t timestamp_prev;

	if (k_uptime_get_32() - timestamp_prev <
	    K_SECONDS(CONFIG_APP_HOLD_TIME_RSRP)) {
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

int lwm2m_start_connmon(void)
{
	k_work_submit(&modem_data_work);
	return modem_info_rsrp_register(modem_signal_handler);
}
