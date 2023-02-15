/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <string.h>
#include <stdlib.h>
#include <modem/modem_info.h>
#include <modem/at_params.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(modem_info_params);

int modem_info_params_init(struct modem_param_info *modem)
{
	if (modem == NULL) {
		return -EINVAL;
	}

	modem->network.current_band.type	= MODEM_INFO_CUR_BAND;
	modem->network.sup_band.type		= MODEM_INFO_SUP_BAND;
	modem->network.area_code.type		= MODEM_INFO_AREA_CODE;
	modem->network.current_operator.type	= MODEM_INFO_OPERATOR;
	modem->network.mcc.type			= MODEM_INFO_MCC;
	modem->network.mnc.type			= MODEM_INFO_MNC;
	modem->network.cellid_hex.type		= MODEM_INFO_CELLID;
	modem->network.ip_address.type		= MODEM_INFO_IP_ADDRESS;
	modem->network.ue_mode.type		= MODEM_INFO_UE_MODE;
	modem->network.lte_mode.type		= MODEM_INFO_LTE_MODE;
	modem->network.nbiot_mode.type		= MODEM_INFO_NBIOT_MODE;
	modem->network.gps_mode.type		= MODEM_INFO_GPS_MODE;
	modem->network.date_time.type		= MODEM_INFO_DATE_TIME;
	modem->network.apn.type			= MODEM_INFO_APN;
	modem->network.rsrp.type		= MODEM_INFO_RSRP;

	modem->sim.uicc.type			= MODEM_INFO_UICC;
	modem->sim.iccid.type			= MODEM_INFO_ICCID;
	modem->sim.imsi.type		        = MODEM_INFO_IMSI;

	modem->device.modem_fw.type		= MODEM_INFO_FW_VERSION;
	modem->device.battery.type		= MODEM_INFO_BATTERY;
	modem->device.imei.type			= MODEM_INFO_IMEI;
	modem->device.board			= CONFIG_BOARD;
	modem->device.app_version		= STRINGIFY(APP_VERSION);

#ifdef PROJECT_NAME
	modem->device.app_name			= STRINGIFY(PROJECT_NAME);
#else
	modem->device.app_name			= "N/A";
#endif

	return 0;
}

static int area_code_parse(struct lte_param *area_code)
{
	if (area_code == NULL) {
		return -EINVAL;
	}

	area_code->value_string[4] = '\0';
	/* Parses the string, interpreting its content as an 	*/
	/* integral number with base 16. (Hexadecimal)		*/
	area_code->value = strtol(area_code->value_string, NULL, 16);

	return 0;
}

static int mcc_mnc_parse(struct lte_param *current_operator,
			 struct lte_param *mcc,
			 struct lte_param *mnc)
{
	if (current_operator == NULL || mcc == NULL || mnc == NULL) {
		return -EINVAL;
	}

	memcpy(mcc->value_string, current_operator->value_string, 3);

	if (sizeof(current_operator->value_string) - 1 < 6) {
		mnc->value_string[0] = '0';
		memcpy(&mnc->value_string[1], &current_operator->value_string[3], 2);
	} else {
		memcpy(&mnc->value_string, &current_operator->value_string[3], 3);
	}

	/* Parses the string, interpreting its content as an	*/
	/* integral number with base 10. (Decimal)		*/

	mcc->value = (double)strtol(mcc->value_string, NULL, 10);
	mnc->value = (double)strtol(mnc->value_string, NULL, 10);

	return 0;
}

static int cellid_to_dec(struct lte_param *cellID, double *cellID_dec)
{
	/* Parses the string, interpreting its content as an	*/
	/* integral number with base 16. (Hexadecimal)		*/

	*cellID_dec = (double)strtol(cellID->value_string, NULL, 16);

	return 0;
}

static int modem_data_get(struct lte_param *param)
{
	enum at_param_type data_type;
	int ret;

	data_type = modem_info_type_get(param->type);

	if (data_type < 0) {
		return -EINVAL;
	}

	if (data_type == AT_PARAM_TYPE_STRING) {
		ret = modem_info_string_get(param->type,
				param->value_string,
				sizeof(param->value_string));
		if (ret < 0) {
			LOG_ERR("Link data not obtained: %d %d", param->type, ret);
			return ret;
		}
	} else if (data_type == AT_PARAM_TYPE_NUM_INT) {
		ret = modem_info_short_get(param->type, &param->value);
		if (ret < 0) {
			LOG_ERR("Link data not obtained: %d", ret);
			return ret;
		}
	}

	return 0;
}

int modem_info_params_get(struct modem_param_info *modem)
{
	int ret;

	if (modem == NULL) {
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_MODEM_INFO_ADD_NETWORK) ||
	    IS_ENABLED(CONFIG_MODEM_INFO_ADD_SIM) ||
	    IS_ENABLED(CONFIG_MODEM_INFO_ADD_DEVICE)) {
		struct lte_param *params[] = {
#if IS_ENABLED(CONFIG_MODEM_INFO_ADD_NETWORK)
			&modem->network.current_band,
			&modem->network.sup_band,
			&modem->network.ip_address,
			&modem->network.ue_mode,
			&modem->network.current_operator,
			&modem->network.cellid_hex,
			&modem->network.area_code,
			&modem->network.lte_mode,
			&modem->network.nbiot_mode,
			&modem->network.gps_mode,
			&modem->network.apn,
			&modem->network.rsrp,
#endif
#if IS_ENABLED(CONFIG_MODEM_INFO_ADD_SIM_ICCID)
			&modem->sim.iccid,
#endif
#if IS_ENABLED(CONFIG_MODEM_INFO_ADD_SIM_IMSI)
			&modem->sim.imsi,
#endif
#if IS_ENABLED(CONFIG_MODEM_INFO_ADD_DEVICE)
			&modem->device.modem_fw,
			&modem->device.battery,
			&modem->device.imei,
#endif
		};

		for (size_t i = 0; i < ARRAY_SIZE(params); ++i) {
			ret = modem_data_get(params[i]);
			if (ret) {
				return ret;
			}
		}
	}

	if (IS_ENABLED(CONFIG_MODEM_INFO_ADD_NETWORK)) {
		if (IS_ENABLED(CONFIG_MODEM_INFO_ADD_DATE_TIME)) {
			ret = modem_data_get(&modem->network.date_time);
			if (ret) {
				LOG_ERR("Could not get time, error: %d", ret);
				/* non-critical error: continue */
			}
		}

		ret = mcc_mnc_parse(&modem->network.current_operator,
				&modem->network.mcc,
				&modem->network.mnc);
		if (ret) {
			return ret;
		}
		ret = cellid_to_dec(&modem->network.cellid_hex,
				&modem->network.cellid_dec);
		if (ret) {
			return ret;
		}
		ret = area_code_parse(&modem->network.area_code);
		if (ret) {
			return ret;
		}
	}
	return 0;
}
