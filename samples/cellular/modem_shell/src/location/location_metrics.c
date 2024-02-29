/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <zephyr/shell/shell.h>
#include <date_time.h>
#include <cJSON.h>
#include <math.h>
#include <getopt.h>
#include <modem/lte_lc.h>
#include <net/nrf_cloud.h>

#include <modem/location.h>

#include "mosh_print.h"
#include "link_api.h"
#include "link_shell_print.h"
#include "location_cmd_utils.h"
#include "location_metrics.h"

/******************************************************************************/

static int json_add_num_cs(cJSON *parent, const char *str, double item)
{
	if (!parent || !str) {
		return -EINVAL;
	}

	return cJSON_AddNumberToObjectCS(parent, str, item) ? 0 : -ENOMEM;
}

/******************************************************************************/

#define GNSS_PVT_KEY_LAT "lat"
#define GNSS_PVT_KEY_LON "lng"
#define GNSS_PVT_KEY_ACC "acc"

#define GNSS_PVT_KEY_ALTITUDE "alt"
#define GNSS_PVT_KEY_ALTITUDE_ACC "altAcc"
#define GNSS_PVT_KEY_SPEED "spd"
#define GNSS_PVT_KEY_SPEED_ACC "spdAcc"
#define GNSS_PVT_KEY_VER_SPEED "verSpd"
#define GNSS_PVT_KEY_VER_SPEED_ACC "verSpdAcc"
#define GNSS_PVT_KEY_HEADING "hdg"
#define GNSS_PVT_KEY_HEADING_ACC "hdgAcc"

#define GNSS_PVT_KEY_PDOP "pdop"
#define GNSS_PVT_KEY_HDOP "hdop"
#define GNSS_PVT_KEY_VDOP "vdop"
#define GNSS_PVT_KEY_TDOP "tdop"
#define GNSS_PVT_KEY_FLAGS "flags"

static int location_metrics_simple_pvt_encode(
	const struct location_data *location, cJSON * const root_obj)
{
	cJSON *pvt_data_obj = NULL;

	if (!location || !root_obj) {
		return -EINVAL;
	}

	pvt_data_obj = cJSON_CreateObject();
	if (pvt_data_obj == NULL) {
		mosh_error("No memory create json obj for simple pvt_data_obj");
		goto cleanup;
	}

	if (!cJSON_AddItemToObject(root_obj, "pvt_data_simple", pvt_data_obj)) {
		mosh_error("No memory to add json obj pvt_data_simple");
		goto cleanup;
	}

	if (json_add_num_cs(pvt_data_obj, GNSS_PVT_KEY_LON, location->longitude) ||
	    json_add_num_cs(pvt_data_obj, GNSS_PVT_KEY_LAT, location->latitude) ||
	    json_add_num_cs(pvt_data_obj, GNSS_PVT_KEY_ACC, location->accuracy)) {
		mosh_error("Failed to encode PVT data");
		goto cleanup;
	}

	return 0;
cleanup:
	/* No need to delete others as they were added to here */
	cJSON_Delete(pvt_data_obj);

	return -ENOMEM;
}

static int location_metrics_detailed_pvt_encode(
	const struct nrf_modem_gnss_pvt_data_frame *const mdm_pvt, cJSON *const root_obj)
{
	cJSON *pvt_data_obj = NULL;

	if (!mdm_pvt || !root_obj) {
		return -EINVAL;
	}

	pvt_data_obj = cJSON_CreateObject();
	if (pvt_data_obj == NULL) {
		mosh_error("No memory create json obj for metrics pvt_data_obj");
		goto cleanup;
	}

	if (!cJSON_AddItemToObject(root_obj, "pvt_data_detailed", pvt_data_obj)) {
		mosh_error("No memory to add json obj pvt_data_detailed");
		goto cleanup;
	}

	/* Flags */
	if (json_add_num_cs(pvt_data_obj, GNSS_PVT_KEY_FLAGS, mdm_pvt->flags)) {
		goto cleanup;
	}

	/* GNSS fix data */
	if (mdm_pvt->flags & NRF_MODEM_GNSS_PVT_FLAG_FIX_VALID) {
		if (json_add_num_cs(pvt_data_obj, GNSS_PVT_KEY_LON, mdm_pvt->longitude) ||
		    json_add_num_cs(pvt_data_obj, GNSS_PVT_KEY_LAT, mdm_pvt->latitude) ||
		    json_add_num_cs(pvt_data_obj, GNSS_PVT_KEY_ACC,
				    round(mdm_pvt->accuracy * 1000) / 1000) ||
		    json_add_num_cs(pvt_data_obj, GNSS_PVT_KEY_ALTITUDE,
				    round(mdm_pvt->altitude * 1000) / 1000) ||
		    json_add_num_cs(pvt_data_obj, GNSS_PVT_KEY_ALTITUDE_ACC,
				    round(mdm_pvt->altitude_accuracy * 1000) / 1000) ||
		    json_add_num_cs(pvt_data_obj, GNSS_PVT_KEY_SPEED,
				    round(mdm_pvt->speed * 1000) / 1000) ||
		    json_add_num_cs(pvt_data_obj, GNSS_PVT_KEY_SPEED_ACC,
				    round(mdm_pvt->speed_accuracy * 1000) / 1000) ||
		    json_add_num_cs(pvt_data_obj, GNSS_PVT_KEY_HEADING,
				    round(mdm_pvt->heading * 1000) / 1000) ||
		    json_add_num_cs(pvt_data_obj, GNSS_PVT_KEY_HEADING_ACC,
				    round(mdm_pvt->heading_accuracy * 1000) / 1000) ||
		    json_add_num_cs(pvt_data_obj, GNSS_PVT_KEY_VER_SPEED,
				    round(mdm_pvt->vertical_speed * 1000) / 1000) ||
		    json_add_num_cs(pvt_data_obj, GNSS_PVT_KEY_VER_SPEED_ACC,
				    round(mdm_pvt->vertical_speed_accuracy * 1000) / 1000) ||
		    json_add_num_cs(pvt_data_obj, GNSS_PVT_KEY_PDOP,
				    round(mdm_pvt->pdop * 1000) / 1000) ||
		    json_add_num_cs(pvt_data_obj, GNSS_PVT_KEY_HDOP,
				    round(mdm_pvt->hdop * 1000) / 1000) ||
		    json_add_num_cs(pvt_data_obj, GNSS_PVT_KEY_VDOP,
				    round(mdm_pvt->vdop * 1000) / 1000) ||
		    json_add_num_cs(pvt_data_obj, GNSS_PVT_KEY_TDOP,
				    round(mdm_pvt->tdop * 1000) / 1000)) {
			mosh_error("Failed to encode GNSS fix data");
			goto cleanup;
		}
	}

	cJSON *sat_info_array = NULL;
	cJSON *sat_info_obj = NULL;

	/* SV data */
	sat_info_array = cJSON_AddArrayToObjectCS(pvt_data_obj, "sv_info");
	if (!sat_info_array) {
		mosh_error("Cannot create json array for satellites");
		goto cleanup;
	}
	for (uint32_t i = 0; i < NRF_MODEM_GNSS_MAX_SATELLITES; i++) {
		if (mdm_pvt->sv[i].sv == 0) {
			break;
		}
		sat_info_obj = cJSON_CreateObject();
		if (sat_info_obj == NULL) {
			mosh_error("No memory create json obj for satellite data");
			goto cleanup;
		}

		if (!cJSON_AddItemToArray(sat_info_array, sat_info_obj)) {
			mosh_error("Cannot add sat info json object to sat array");
			cJSON_Delete(sat_info_obj);
			goto cleanup;
		}
		if (json_add_num_cs(sat_info_obj, "sv", mdm_pvt->sv[i].sv) ||
		    json_add_num_cs(sat_info_obj, "c_n0", (mdm_pvt->sv[i].cn0 * 0.1)) ||
		    json_add_num_cs(sat_info_obj, "sig", mdm_pvt->sv[i].signal) ||
		    json_add_num_cs(sat_info_obj, "elev", mdm_pvt->sv[i].elevation) ||
		    json_add_num_cs(sat_info_obj, "az", mdm_pvt->sv[i].azimuth) ||
		    json_add_num_cs(
			    sat_info_obj, "in_fix",
			    (mdm_pvt->sv[i].flags & NRF_MODEM_GNSS_SV_FLAG_USED_IN_FIX ? 1 : 0)) ||
		    json_add_num_cs(
			    sat_info_obj, "unhealthy",
			    (mdm_pvt->sv[i].flags & NRF_MODEM_GNSS_SV_FLAG_UNHEALTHY ? 1 : 0))) {
			mosh_error("Failed to encode detailed PVT data for sv array[%d]", i);
			goto cleanup;
		}
	}

	return 0;
cleanup:
	/* No need to delete others as they were added to here */
	cJSON_Delete(pvt_data_obj);

	return -ENOMEM;
}

/******************************************************************************/

static int location_metrics_request_config_encode(
	const struct location_config *loc_conf, cJSON * const root_obj)
{
	cJSON *loc_conf_obj = NULL;
	char tmp_str[9] = { 0 };

	if (!loc_conf || !root_obj) {
		return -EINVAL;
	}

	loc_conf_obj = cJSON_CreateObject();
	if (loc_conf_obj == NULL) {
		mosh_error("No memory create json obj for metrics loc_conf_obj");
		goto cleanup;
	}

	if (!cJSON_AddItemToObject(root_obj, "location_config", loc_conf_obj)) {
		mosh_error("No memory to create json obj location_config");
		goto cleanup;
	}

	if (loc_conf->mode == LOCATION_REQ_MODE_FALLBACK) {
		strcpy(tmp_str, "fallback");
	} else if (loc_conf->mode == LOCATION_REQ_MODE_ALL) {
		strcpy(tmp_str, "all");
	}
	if (!cJSON_AddStringToObjectCS(loc_conf_obj, "req_mode", tmp_str)) {
		mosh_error("No memory to add req_mode string");
		goto cleanup;
	}

	if (json_add_num_cs(loc_conf_obj, "methods_count", loc_conf->methods_count) ||
	    json_add_num_cs(loc_conf_obj, "interval", loc_conf->interval) ||
	    json_add_num_cs(loc_conf_obj, "timeout", loc_conf->timeout)) {
		mosh_error("Failed to location_config base data");
		goto cleanup;
	}

	cJSON *loc_method_conf_arr = NULL;
	cJSON *loc_method_conf_obj = NULL;

	/* location_method_config data */
	loc_method_conf_arr = cJSON_AddArrayToObjectCS(loc_conf_obj, "methods");
	if (!loc_method_conf_arr) {
		mosh_error("Cannot create json array loc_method_conf_arr");
		goto cleanup;
	}

	for (int i = 0; i < loc_conf->methods_count; i++) {
		loc_method_conf_obj = cJSON_CreateObject();
		if (loc_method_conf_obj == NULL) {
			mosh_error("No memory create json obj loc_method_conf_obj");
			goto cleanup;
		}

		if (!cJSON_AddItemToArray(loc_method_conf_arr, loc_method_conf_obj)) {
			mosh_error("Cannot add item to loc_method_conf_arr");
			cJSON_Delete(loc_method_conf_obj);
			goto cleanup;
		}
		if (loc_conf->methods[i].method == LOCATION_METHOD_GNSS) {
			char tmp_str[9] = { 0 };

			if (json_add_num_cs(loc_method_conf_obj, "gnss_timeout",
					    loc_conf->methods[i].gnss.timeout) ||
			    json_add_num_cs(loc_method_conf_obj, "gnss_num_con_fixes",
					    loc_conf->methods[i].gnss.num_consecutive_fixes) ||
			    json_add_num_cs(loc_method_conf_obj, "gnss_visibility_detect",
					    loc_conf->methods[i].gnss.visibility_detection) ||
			    json_add_num_cs(loc_method_conf_obj, "gnss_prio_mode",
					    loc_conf->methods[i].gnss.priority_mode)) {
				mosh_error("Cannot add gnss conf item to loc_method_conf_obj");
				goto cleanup;
			}

			if (loc_conf->methods[i].gnss.accuracy == LOCATION_ACCURACY_LOW) {
				strcpy(tmp_str, "low");
			} else if (loc_conf->methods[i].gnss.accuracy == LOCATION_ACCURACY_NORMAL) {
				strcpy(tmp_str, "normal");
			} else if (loc_conf->methods[i].gnss.accuracy == LOCATION_ACCURACY_HIGH) {
				strcpy(tmp_str, "high");
			}

			if (!cJSON_AddStringToObjectCS(loc_method_conf_obj, "gnss_accuracy",
						       tmp_str)) {
				mosh_error("No memory to add accuracy string");
				goto cleanup;
			}
		} else if (loc_conf->methods[i].method == LOCATION_METHOD_WIFI) {
			char tmp_str[10] = { 0 };

			if (json_add_num_cs(loc_method_conf_obj, "wifi_timeout",
					    loc_conf->methods[i].wifi.timeout)) {
				mosh_error("Cannot add wifi conf item to loc_method_conf_obj");
				goto cleanup;
			}
			if (loc_conf->methods[i].wifi.service == LOCATION_SERVICE_ANY) {
				strcpy(tmp_str, "any");
			} else if (loc_conf->methods[i].wifi.service ==
				   LOCATION_SERVICE_NRF_CLOUD) {
				strcpy(tmp_str, "nrf cloud");
			} else if (loc_conf->methods[i].wifi.service == LOCATION_SERVICE_HERE) {
				strcpy(tmp_str, "here");
			}

			if (!cJSON_AddStringToObjectCS(loc_method_conf_obj, "wifi_service",
						       tmp_str)) {
				mosh_error("No memory to add wifi service string");
				goto cleanup;
			}
		} else if (loc_conf->methods[i].method == LOCATION_METHOD_CELLULAR) {
			char tmp_str[16] = { 0 };

			if (json_add_num_cs(loc_method_conf_obj, "cell_timeout",
					    loc_conf->methods[i].cellular.timeout) ||
			    json_add_num_cs(
				    loc_method_conf_obj, "cellular_cell_count",
				    loc_conf->methods[i].cellular.cell_count)) {
				mosh_error("Cannot add cellular conf item to loc_method_conf_obj");
				goto cleanup;
			}
			if (loc_conf->methods[i].cellular.service == LOCATION_SERVICE_ANY) {
				strcpy(tmp_str, "any");
			} else if (loc_conf->methods[i].cellular.service ==
				   LOCATION_SERVICE_NRF_CLOUD) {
				strcpy(tmp_str, "nrf cloud");
			} else if (loc_conf->methods[i].cellular.service == LOCATION_SERVICE_HERE) {
				strcpy(tmp_str, "here");
			}
			if (!cJSON_AddStringToObjectCS(loc_method_conf_obj, "cell_service",
						       tmp_str)) {
				mosh_error("No memory to add cellular service string");
				goto cleanup;
			}
		}
	}

	return 0;

cleanup:
	/* No need to delete others as they were added to here */
	cJSON_Delete(loc_conf_obj);
	return -ENOMEM;
}

static int location_metrics_request_info_encode(
	const struct location_metrics_data *loc_metrics_data, cJSON * const root_obj)
{
	if (!loc_metrics_data || !root_obj) {
		return -EINVAL;
	}

	cJSON *loc_req_info_obj = NULL;
	struct location_event_data *loc_evt_data =
		(struct location_event_data *)&loc_metrics_data->event_data;
	struct location_data_details *details;
	//int err;

	if (loc_evt_data->id == LOCATION_EVT_LOCATION) {
		details = (struct location_data_details *)&loc_evt_data->location.details;
	} else {
		/* error/timeout  */
		details = (struct location_data_details *)&loc_evt_data->error.details;
	}

	loc_req_info_obj = cJSON_CreateObject();
	if (loc_req_info_obj == NULL) {
		mosh_error("No memory create json obj for loc_req_info_obj");
		goto cleanup;
	}

	if (!cJSON_AddItemToObject(root_obj, "location_req_info", loc_req_info_obj)) {
		mosh_error("No memory to add json obj location_req_info");
		goto cleanup;
	}

	if (!cJSON_AddNumberToObjectCS(loc_req_info_obj, "used_time_sec",
				       round(details->elapsed_time_method)) ||
	    !cJSON_AddStringToObjectCS(loc_req_info_obj, "location_cmd_str",
				       loc_metrics_data->loc_cmd_str)) {
		mosh_error("Failed to encode used_time_sec or location_cmd_str");
		goto cleanup;
	}

	/*err = location_metrics_request_config_encode(&details->used_config,
						     loc_req_info_obj);
	if (err) {
		mosh_warn("Failed to encode location_config data to json, err %d", err);
		goto cleanup;
	}*/

	return 0;
cleanup:
	/* No need to delete others as they were added to here */
	cJSON_Delete(loc_req_info_obj);
	return -ENOMEM;
}

/******************************************************************************/

static int location_metrics_modem_json_encode(cJSON *const root_obj)
{
	cJSON *mdm_metrics_data_obj = NULL;
	struct lte_xmonitor_resp_t xmonitor_resp;
	enum lte_lc_system_mode sysmode;
	int err = link_api_xmonitor_read(&xmonitor_resp);
	char tmp_str[OP_FULL_NAME_STR_MAX_LEN + 1];
	int len;

	if (err) {
		mosh_error("link_api_xmonitor_read failed, result: ret %d", err);
		return err;
	}

	mdm_metrics_data_obj = cJSON_CreateObject();
	if (mdm_metrics_data_obj == NULL) {
		mosh_error("No memory create json obj mdm_metrics_data_obj");
		goto cleanup;
	}

	if (!cJSON_AddItemToObject(root_obj, "modem_metrics", mdm_metrics_data_obj)) {
		goto cleanup;
	}

	memset(tmp_str, 0, sizeof(tmp_str));

	/* Strip out the quotes from operator name */
	len = strlen(xmonitor_resp.full_name_str);
	if (len > 2 && len <= OP_FULL_NAME_STR_MAX_LEN) {
		strncpy(tmp_str, xmonitor_resp.full_name_str + 1, len - 2);
	}
	if (!cJSON_AddStringToObjectCS(mdm_metrics_data_obj, "operator_full_name", tmp_str)) {
		mosh_error("No memory create json obj operator_full_name");
		goto cleanup;
	}

	/* Strip out the quotes from plmn str */
	memset(tmp_str, 0, sizeof(tmp_str));
	len = strlen(xmonitor_resp.plmn_str);
	if (len > 2 && len <= OP_PLMN_STR_MAX_LEN) {
		strncpy(tmp_str, xmonitor_resp.plmn_str + 1, len - 2);
	}
	if (!cJSON_AddStringToObjectCS(mdm_metrics_data_obj, "plmn", tmp_str)) {
		mosh_error("No memory create json obj plmn");
		goto cleanup;
	}

	if (json_add_num_cs(mdm_metrics_data_obj, "cell_id", xmonitor_resp.cell_id) ||
	    json_add_num_cs(mdm_metrics_data_obj, "pci", xmonitor_resp.pci) ||
	    json_add_num_cs(mdm_metrics_data_obj, "band", xmonitor_resp.band) ||
	    json_add_num_cs(mdm_metrics_data_obj, "tac", xmonitor_resp.tac) ||
	    json_add_num_cs(mdm_metrics_data_obj, "rsrp_dbm",
			    RSRP_IDX_TO_DBM(xmonitor_resp.rsrp)) ||
	    json_add_num_cs(mdm_metrics_data_obj, "snr_db",
			    xmonitor_resp.snr - LINK_SNR_OFFSET_VALUE)) {
		mosh_error("No memory create modem metrics json obj");
		goto cleanup;
	}

	err = lte_lc_system_mode_get(&sysmode, NULL);
	if (err) {
		mosh_warn("lte_lc_system_mode_get failed with err %d", err);
	} else {
		if (!cJSON_AddStringToObjectCS(mdm_metrics_data_obj, "sysmode",
					       link_shell_sysmode_to_string(sysmode, tmp_str))) {
			mosh_error("No memory create json obj sysmode");
			goto cleanup;
		}
	}

	return 0;

cleanup:
	/* No need to delete others as they were added to here */
	cJSON_Delete(mdm_metrics_data_obj);

	return -ENOMEM;
}

/******************************************************************************/

static int location_data_encode(const struct location_metrics_data *loc_metrics_data,
				cJSON *const root_obj)
{
	if (!loc_metrics_data || !root_obj) {
		return -EINVAL;
	}

	cJSON *loc_data_obj = NULL;
	struct location_data_details *details;
	struct location_event_data *loc_evt_data =
		(struct location_event_data *)&loc_metrics_data->event_data;
	int err = -ENOMEM;

	loc_data_obj = cJSON_CreateObject();
	if (loc_data_obj == NULL) {
		mosh_error("No memory create json obj for metrics loc_data_obj");
		goto cleanup;
	}
	if (!cJSON_AddItemToObject(root_obj, "location_data", loc_data_obj)) {
		mosh_error("No memory to add json obj location_data");
		goto cleanup;
	}

	if (!cJSON_AddStringToObjectCS(loc_data_obj, "loc_method",
				       location_method_str(loc_evt_data->method))) {
		mosh_error("No memory create json obj loc_method");
		goto cleanup;
	}

	if (loc_evt_data->id == LOCATION_EVT_LOCATION) {
		details = (struct location_data_details *)&loc_evt_data->location.details;
		if (loc_evt_data->method != LOCATION_METHOD_GNSS) {
			err = location_metrics_simple_pvt_encode(&loc_evt_data->location,
								 loc_data_obj);
			if (err) {
				mosh_error("Failed to encode PVT data for nrf cloud to json");
				goto cleanup;
			}
		}

	} else {
		/* error/timeout  */
		details = (struct location_data_details *)&loc_evt_data->error.details;
		/*if (!cJSON_AddStringToObjectCS(loc_data_obj, "failure_cause_str",
					       loc_evt_data->error.failure_cause_str)) {
			mosh_error("No memory create json obj failure_cause_str");
			goto cleanup;
		}*/
	}

	if (loc_evt_data->method == LOCATION_METHOD_GNSS) {
		if (json_add_num_cs(loc_data_obj, "tracked_satellites",
				    details->gnss.satellites_tracked)) {
			mosh_error("No memory create json obj tracked_satellites");
			goto cleanup;
		}

		if (json_add_num_cs(loc_data_obj, "used_satellites",
				    details->gnss.satellites_used)) {
			mosh_error("No memory create json obj used_satellites");
			goto cleanup;
		}
/*
		if (json_add_num_cs(loc_data_obj, "cn0_avg_of_used_satellites",
				    (details->gnss.cn0_avg_satellites_used * 0.1))) {
			mosh_error("No memory create json obj cn0_avg_of_used_satellites");
			goto cleanup;
		}
*/
		err = location_metrics_detailed_pvt_encode(&details->gnss.pvt_data,
							   loc_data_obj);
		if (err) {
			mosh_error("Failed to encode PVT data for nrf cloud to json");
			goto cleanup;
		}
	}
#if defined(CONFIG_LOCATION_METHOD_WIFI)
	else if (loc_evt_data->method == LOCATION_METHOD_WIFI) {
		if (json_add_num_cs(loc_data_obj, "scanned_wifi_ap_cnt",
				    details->wifi.ap_count)) {
			mosh_error("No memory create json obj scanned_ap_count");
			goto cleanup;
		}
	}
#endif
	return 0;

cleanup:
	/* No need to delete others as they were added to here */
	cJSON_Delete(loc_data_obj);

	return err;
}

/******************************************************************************/

static int location_metrics_json_payload_encode(
	const struct location_metrics_data *loc_metrics_data, cJSON *const root_obj)
{
	if (!loc_metrics_data || !root_obj) {
		return -EINVAL;
	}

	int err = 0;

	/* Fill location_req_info */
	err = location_metrics_request_info_encode(loc_metrics_data, root_obj);
	if (err) {
		mosh_warn("Failed to encode location_req_info data to json, err %d", err);
		goto cleanup;
	}

	/* Fill actual location_data */
	err = location_data_encode(loc_metrics_data, root_obj);
	if (err) {
		mosh_warn("Failed to encode location_data to json, err %d", err);
		goto cleanup;
	}

	/* Fill common modem metrics */
	err = location_metrics_modem_json_encode(root_obj);
	if (err) {
		mosh_warn("Failed to encode modem_metrics data to json, err %d", err);
		/* We didn't get modem_metrics, but we don't care. Let it be empty. */
	}

	return 0;

cleanup:

	return err;
}

/******************************************************************************/
#define MSG_TYPE "messageType"
#define MSG_APP_ID "appId"
#define MSG_DATA "data"
#define MSG_TIMESTAMP "ts"

/*
 * Following added to given as an output to json_str_out as an example:
 *{
 *	"data": "14.76",
 *	"appId": "LOC_GNSS_TTF",
 *	"messageType": "DATA",
 *	"ts": 1669711208428,
 *	"location_req_info": {
 *		"used_time_sec": 15,
 *		"location_cmd_str": "location get --mode all -m cellular -m wifi -m gnss
 *					--gnss_cloud_pvt --cloud_metrics --interval 15",
 *		"location_config": {
 *			"req_mode": "all",
 *			"methods_count": 3,
 *			"interval": 15,
 *			"timeout": 300000,
 *			"methods": [
 *				{
 *					"cell_timeout": 30000,
 *					"cell_ncellmeas_at_search_type": 0,
 *					"cell_ncellmeas_at_gci_count": 10,
 *					"cell_service": "any"
 *				},
 *				{
 *					"wifi_timeout": 30000,
 *					"wifi_service": "any"
 *				},
 *				{
 *					"gnss_timeout": 120000,
 *					"gnss_num_con_fixes": 3,
 *					"gnss_visibility_detect": 0,
 *					"gnss_prio_mode": 0,
 *					"gnss_accuracy": "normal"
 *				}
 *			]
 *		}
 *	},
 *	"location_data": {
 *		"loc_method": "GNSS",
 *		"tracked_satellites": 10,
 *		"used_satellites": 9,
 *		"cn0_avg_of_used_satellites": 45.9,
 *		"pvt_data_detailed": {
 *			"flags": 35,
 *			"lng": 23.775882648600284,
 *			"lat": 61.493788965179519,
 *			"acc": 4.484,
 *			"alt": 144.601,
 *			"altAcc": 7.932,
 *			"spd": 0.098,
 *			"spdAcc": 0.345,
 *			"hdg": 0,
 *			"hdgAcc": 180,
 *			"verSpd": 0.026,
 *			"verSpdAcc": 0.655,
 *			"pdop": 2.13,
 *			"hdop": 0.955,
 *			"vdop": 1.904,
 *			"tdop": 1.171,
 *			"sv_info": [
 *				{
 *					"sv": 4,
 *					"c_n0": 47.2,
 *					"sig": 1,
 *					"elev": 52,
 *					"az": 93,
 *					"in_fix": 1,
 *					"unhealthy": 0
 *				},
 *				{
 *					"sv": 6,
 *					"c_n0": 47.1,
 *					"sig": 1,
 *					"elev": 35,
 *					"az": 237,
 *					"in_fix": 1,
 *					"unhealthy": 0
 *				},
 *				{
 *					"sv": 7,
 *					"c_n0": 48.4,
 *					"sig": 1,
 *					"elev": 33,
 *					"az": 191,
 *					"in_fix": 1,
 *					"unhealthy": 0
 *				},
 *				{
 *					"sv": 9,
 *					"c_n0": 48.1,
 *					"sig": 1,
 *					"elev": 80,
 *					"az": 194,
 *					"in_fix": 1,
 *					"unhealthy": 0
 *				},
 *				{
 *					"sv": 11,
 *					"c_n0": 47.5,
 *					"sig": 1,
 *					"elev": 39,
 *					"az": 284,
 *					"in_fix": 1,
 *					"unhealthy": 0
 *				},
 *				{
 *					"sv": 16,
 *					"c_n0": 46.4,
 *					"sig": 1,
 *					"elev": 25,
 *					"az": 84,
 *					"in_fix": 1,
 *					"unhealthy": 0
 *				},
 *				{
 *					"sv": 26,
 *					"c_n0": 42.7,
 *					"sig": 1,
 *					"elev": 25,
 *					"az": 48,
 *					"in_fix": 1,
 *					"unhealthy": 0
 *				},
 *				{
 *					"sv": 29,
 *					"c_n0": 43.5,
 *					"sig": 1,
 *					"elev": 14,
 *					"az": 354,
 *					"in_fix": 1,
 *					"unhealthy": 0
 *				},
 *				{
 *					"sv": 20,
 *					"c_n0": 42.9,
 *					"sig": 1,
 *					"elev": 27,
 *					"az": 302,
 *					"in_fix": 1,
 *					"unhealthy": 0
 *				},
 *				{
 *					"sv": 3,
 *					"c_n0": 42.2,
 *					"sig": 1,
 *					"elev": 9,
 *					"az": 137,
 *					"in_fix": 0,
 *					"unhealthy": 0
 *				}
 *			]
 *		}
 *	},
 *	"modem_metrics": {
 *		"operator_full_name": "DNA",
 *		"plmn": "24412",
 *		"cell_id": 1805086,
 *		"pci": 110,
 *		"band": 20,
 *		"tac": 312,
 *		"rsrp_dbm": -85,
 *		"snr_db": 4,
 *		"sysmode": "LTE-M - GNSS"
 *	}
 *}
 */

int location_metrics_utils_json_payload_encode(const struct location_metrics_data *loc_metrics_data,
					       char **json_str_out)
{
	__ASSERT_NO_MSG(loc_metrics_data != NULL);
	__ASSERT_NO_MSG(json_str_out != NULL);

	int err = 0;
	cJSON *root_obj = NULL;
	char used_time_sec_str[32];
	char app_id_str[32];
	struct location_data_details *details;
	struct location_event_data *loc_evt_data =
		(struct location_event_data *)&loc_metrics_data->event_data;
	double used_time_sec;

	if (loc_evt_data->id == LOCATION_EVT_LOCATION) {
		details = (struct location_data_details *)&loc_evt_data->location.details;
	} else {
		details = (struct location_data_details *)&loc_evt_data->error.details;
	}
	used_time_sec = details->elapsed_time_method;

	/* Used time for a location request as a root "key"
	 * and custom appId based on used method.
	 */
	if (loc_evt_data->method == LOCATION_METHOD_GNSS) {
		strcpy(app_id_str, "LOC_GNSS_TTF");
	} else if (loc_evt_data->method == LOCATION_METHOD_CELLULAR) {
		strcpy(app_id_str, "LOC_CELL_TTF");
	} else {
		__ASSERT_NO_MSG(loc_evt_data->method == LOCATION_METHOD_WIFI);
		strcpy(app_id_str, "LOC_WIFI_TTF");
	}

	if (loc_evt_data->id == LOCATION_EVT_TIMEOUT) {
		/* We alter the used time for timeout to pop out clearly
		 * from other values in a card
		 */
		used_time_sec = -5;
	} else if (loc_evt_data->id == LOCATION_EVT_ERROR) {
		used_time_sec = -1;
	}

	/* This structure corresponds to the General Message Schema described in the
	 * application-protocols repo:
	 * https://github.com/nRFCloud/application-protocols
	 */

	root_obj = cJSON_CreateObject();
	if (root_obj == NULL) {
		mosh_error("Failed to create root json obj");
		err = -ENOMEM;
		goto cleanup;
	}

	sprintf(used_time_sec_str, "%.2f", used_time_sec);

	if (!cJSON_AddStringToObjectCS(root_obj, MSG_DATA, used_time_sec_str) ||
	    !cJSON_AddStringToObjectCS(root_obj, MSG_APP_ID, app_id_str) ||
	    !cJSON_AddStringToObjectCS(root_obj, MSG_TYPE, "DATA") ||
	    !cJSON_AddNumberToObjectCS(root_obj, MSG_TIMESTAMP, loc_metrics_data->timestamp_ms)) {
		mosh_error("Cannot add metadata json objects");
		err = -ENOMEM;
		goto cleanup;
	}

	err = location_metrics_json_payload_encode(loc_metrics_data, root_obj);
	if (err) {
		mosh_error("Failed to encode metrics data to json");
		goto cleanup;
	}
	*json_str_out = cJSON_PrintUnformatted(root_obj);
	if (*json_str_out == NULL) {
		mosh_error("location metrics: failed to print json objects to string");
		err = -ENOMEM;
	}

cleanup:
	if (root_obj) {
		cJSON_Delete(root_obj);
	}
	return err;
}
