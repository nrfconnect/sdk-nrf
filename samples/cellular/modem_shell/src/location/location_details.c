/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
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
#include "location_details.h"

static int json_add_num_cs(cJSON *parent, const char *str, double item)
{
	if (!parent || !str) {
		return -EINVAL;
	}

	return cJSON_AddNumberToObjectCS(parent, str, item) ? 0 : -ENOMEM;
}

static const char *location_details_event_str(enum location_event_id event_id)
{
	const char *event_str = NULL;

	switch (event_id) {
	case LOCATION_EVT_LOCATION:
		event_str = "success";
		break;
	case LOCATION_EVT_TIMEOUT:
		event_str = "timeout";
		break;
	case LOCATION_EVT_ERROR:
		event_str = "error";
		break;
	case LOCATION_EVT_RESULT_UNKNOWN:
		event_str = "unknown";
		break;
	case LOCATION_EVT_FALLBACK:
		event_str = "fallback";
		break;
	default:
		event_str = "invalid";
		break;
	}

	return event_str;
}

static int location_details_position_encode(const struct location_data *location, cJSON *root_obj)
{
	cJSON *position_data_obj = NULL;

	position_data_obj = cJSON_CreateObject();
	if (position_data_obj == NULL) {
		goto cleanup;
	}

	if (json_add_num_cs(position_data_obj, "lng", location->longitude) ||
	    json_add_num_cs(position_data_obj, "lat", location->latitude) ||
	    json_add_num_cs(position_data_obj, "acc", location->accuracy)) {
		goto cleanup;
	}

	if (!cJSON_AddItemToObject(root_obj, "position", position_data_obj)) {
		goto cleanup;
	}

	return 0;
cleanup:
	/* No need to delete others as they were added to here */
	cJSON_Delete(position_data_obj);

	return -ENOMEM;
}

#if defined(CONFIG_LOCATION_METHOD_GNSS)

static int location_details_pvt_data_encode(
	const struct nrf_modem_gnss_pvt_data_frame *mdm_pvt,
	cJSON *root_obj)
{
	cJSON *pvt_data_obj = NULL;

	pvt_data_obj = cJSON_CreateObject();
	if (pvt_data_obj == NULL) {
		goto cleanup;
	}

	/* Flags */
	if (json_add_num_cs(pvt_data_obj, "flags", mdm_pvt->flags) ||
	    json_add_num_cs(pvt_data_obj, "execution_time",
			    (double)mdm_pvt->execution_time / 1000)) {
		goto cleanup;
	}

	/* GNSS fix data */
	if (mdm_pvt->flags & NRF_MODEM_GNSS_PVT_FLAG_FIX_VALID) {
		if (json_add_num_cs(pvt_data_obj, "lng", mdm_pvt->longitude) ||
		    json_add_num_cs(pvt_data_obj, "lat", mdm_pvt->latitude) ||
		    json_add_num_cs(pvt_data_obj, "acc",
				    round(mdm_pvt->accuracy * 1000) / 1000) ||
		    json_add_num_cs(pvt_data_obj, "alt",
				    round(mdm_pvt->altitude * 1000) / 1000) ||
		    json_add_num_cs(pvt_data_obj, "altAcc",
				    round(mdm_pvt->altitude_accuracy * 1000) / 1000) ||
		    json_add_num_cs(pvt_data_obj, "spd",
				    round(mdm_pvt->speed * 1000) / 1000) ||
		    json_add_num_cs(pvt_data_obj, "spdAcc",
				    round(mdm_pvt->speed_accuracy * 1000) / 1000) ||
		    json_add_num_cs(pvt_data_obj, "verSpd",
				    round(mdm_pvt->vertical_speed * 1000) / 1000) ||
		    json_add_num_cs(pvt_data_obj, "verSpdAcc",
				    round(mdm_pvt->vertical_speed_accuracy * 1000) / 1000) ||
		    json_add_num_cs(pvt_data_obj, "hdg",
				    round(mdm_pvt->heading * 1000) / 1000) ||
		    json_add_num_cs(pvt_data_obj, "hdgAcc",
				    round(mdm_pvt->heading_accuracy * 1000) / 1000) ||
		    json_add_num_cs(pvt_data_obj, "pdop",
				    round(mdm_pvt->pdop * 1000) / 1000) ||
		    json_add_num_cs(pvt_data_obj, "hdop",
				    round(mdm_pvt->hdop * 1000) / 1000) ||
		    json_add_num_cs(pvt_data_obj, "vdop",
				    round(mdm_pvt->vdop * 1000) / 1000) ||
		    json_add_num_cs(pvt_data_obj, "tdop",
				    round(mdm_pvt->tdop * 1000) / 1000)) {
			goto cleanup;
		}
	}

	cJSON *sat_info_array = NULL;
	cJSON *sat_info_obj = NULL;

	/* SV data */
	sat_info_array = cJSON_AddArrayToObjectCS(pvt_data_obj, "sv_info");
	if (!sat_info_array) {
		goto cleanup;
	}
	for (uint32_t i = 0; i < NRF_MODEM_GNSS_MAX_SATELLITES; i++) {
		if (mdm_pvt->sv[i].sv == 0) {
			break;
		}
		sat_info_obj = cJSON_CreateObject();
		if (sat_info_obj == NULL) {
			goto cleanup;
		}

		if (!cJSON_AddItemToArray(sat_info_array, sat_info_obj)) {
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
			goto cleanup;
		}
	}

	if (!cJSON_AddItemToObject(root_obj, "pvt_data", pvt_data_obj)) {
		goto cleanup;
	}

	return 0;
cleanup:
	/* No need to delete others as they were added to here */
	cJSON_Delete(pvt_data_obj);

	return -ENOMEM;
}
#endif

static int location_details_location_req_info_encode(
	const char *mosh_cmd,
	cJSON *root_obj)
{
	cJSON *location_req_info_obj = NULL;

	location_req_info_obj = cJSON_CreateObject();
	if (location_req_info_obj == NULL) {
		goto cleanup;
	}

	if (!cJSON_AddStringToObjectCS(location_req_info_obj, "mosh_cmd", mosh_cmd)) {
		goto cleanup;
	}

	if (!cJSON_AddItemToObject(root_obj, "location_req_info", location_req_info_obj)) {
		goto cleanup;
	}

	return 0;
cleanup:
	/* No need to delete others as they were added to here */
	cJSON_Delete(location_req_info_obj);
	return -ENOMEM;
}

static int location_details_modem_details_encode(cJSON *root_obj)
{
	cJSON *modem_details_obj = NULL;
	struct lte_xmonitor_resp_t xmonitor_resp;
	enum lte_lc_system_mode sysmode;
	int err;
	char tmp_str[OP_FULL_NAME_STR_MAX_LEN + 1];
	int len;

	err = link_api_xmonitor_read(&xmonitor_resp);
	if (err) {
		mosh_error("link_api_xmonitor_read failed, err %d", err);
		return err;
	}

	modem_details_obj = cJSON_CreateObject();
	if (modem_details_obj == NULL) {
		goto cleanup;
	}

	memset(tmp_str, 0, sizeof(tmp_str));

	/* Strip out the quotes from operator name */
	len = strlen(xmonitor_resp.full_name_str);
	if (len > 2 && len <= OP_FULL_NAME_STR_MAX_LEN) {
		strncpy(tmp_str, xmonitor_resp.full_name_str + 1, len - 2);
	}
	if (!cJSON_AddStringToObjectCS(modem_details_obj, "operator_full_name", tmp_str)) {
		goto cleanup;
	}

	/* Strip out the quotes from plmn str */
	memset(tmp_str, 0, sizeof(tmp_str));
	len = strlen(xmonitor_resp.plmn_str);
	if (len > 2 && len <= OP_PLMN_STR_MAX_LEN) {
		strncpy(tmp_str, xmonitor_resp.plmn_str + 1, len - 2);
	}
	if (!cJSON_AddStringToObjectCS(modem_details_obj, "plmn", tmp_str)) {
		goto cleanup;
	}

	if (json_add_num_cs(modem_details_obj, "cell_id", xmonitor_resp.cell_id) ||
	    json_add_num_cs(modem_details_obj, "pci", xmonitor_resp.pci) ||
	    json_add_num_cs(modem_details_obj, "band", xmonitor_resp.band) ||
	    json_add_num_cs(modem_details_obj, "tac", xmonitor_resp.tac) ||
	    json_add_num_cs(modem_details_obj, "rsrp_dbm", RSRP_IDX_TO_DBM(xmonitor_resp.rsrp)) ||
	    json_add_num_cs(modem_details_obj, "snr_db",
			    xmonitor_resp.snr - LINK_SNR_OFFSET_VALUE)) {
		goto cleanup;
	}

	err = lte_lc_system_mode_get(&sysmode, NULL);
	if (err) {
		mosh_warn("lte_lc_system_mode_get failed with err %d", err);
	} else {
		if (!cJSON_AddStringToObjectCS(modem_details_obj, "sysmode",
					       link_shell_sysmode_to_string(sysmode, tmp_str))) {
			goto cleanup;
		}
	}

	if (!cJSON_AddItemToObject(root_obj, "modem_details", modem_details_obj)) {
		goto cleanup;
	}

	return 0;

cleanup:
	/* No need to delete others as they were added to here */
	cJSON_Delete(modem_details_obj);

	return -ENOMEM;
}

static int location_details_location_data_encode(
	const struct location_event_data *loc_evt_data,
	cJSON *root_obj)
{
	cJSON *location_data_obj = NULL;
	cJSON *method_details_obj = NULL;
	const struct location_data_details *details;
	int err = -ENOMEM;

	details = location_details_get(loc_evt_data);
	if (details == NULL) {
		err = -EFAULT;
		goto cleanup;
	}

	location_data_obj = cJSON_CreateObject();
	if (location_data_obj == NULL) {
		goto cleanup;
	}

	if (!cJSON_AddStringToObjectCS(location_data_obj, "method",
				       location_method_str(loc_evt_data->method))) {
		goto cleanup;
	}

	if (!cJSON_AddStringToObjectCS(location_data_obj, "event",
				       location_details_event_str(loc_evt_data->id))) {
		goto cleanup;
	}

	if (!cJSON_AddNumberToObjectCS(location_data_obj, "elapsed_time_method_sec",
				       (double)details->elapsed_time_method / 1000)) {
		goto cleanup;
	}

	if (loc_evt_data->id == LOCATION_EVT_LOCATION) {
		err = location_details_position_encode(&loc_evt_data->location, location_data_obj);
		if (err) {
			goto cleanup;
		}
	}

	method_details_obj = cJSON_CreateObject();
	if (method_details_obj == NULL) {
		goto cleanup;
	}

	if (!cJSON_AddItemToObject(location_data_obj, "method_details", method_details_obj)) {
		cJSON_Delete(method_details_obj);
		goto cleanup;
	}

#if defined(CONFIG_LOCATION_METHOD_GNSS)
	if (loc_evt_data->method == LOCATION_METHOD_GNSS) {
		if (json_add_num_cs(method_details_obj, "tracked_satellites",
				    details->gnss.satellites_tracked)) {
			goto cleanup;
		}

		if (json_add_num_cs(method_details_obj, "used_satellites",
				    details->gnss.satellites_used)) {
			goto cleanup;
		}

		if (json_add_num_cs(method_details_obj, "elapsed_time_gnss",
				    (double)details->gnss.elapsed_time_gnss / 1000)) {
			goto cleanup;
		}

		err = location_details_pvt_data_encode(
			&details->gnss.pvt_data,
			method_details_obj);
		if (err) {
			goto cleanup;
		}
	}
#endif
#if defined(CONFIG_LOCATION_METHOD_CELLULAR)
	if (loc_evt_data->method == LOCATION_METHOD_CELLULAR ||
	    loc_evt_data->method == LOCATION_METHOD_WIFI_CELLULAR) {
		if (json_add_num_cs(method_details_obj, "cellular_ncells_count",
				    details->cellular.ncells_count)) {
			goto cleanup;
		}
		if (json_add_num_cs(method_details_obj, "cellular_gci_cells_count",
				    details->cellular.gci_cells_count)) {
			goto cleanup;
		}
	}
#endif
#if defined(CONFIG_LOCATION_METHOD_WIFI)
	if (loc_evt_data->method == LOCATION_METHOD_WIFI ||
	    loc_evt_data->method == LOCATION_METHOD_WIFI_CELLULAR) {
		if (json_add_num_cs(method_details_obj, "wifi_ap_cnt",
				    details->wifi.ap_count)) {
			goto cleanup;
		}
	}
#endif
	if (!cJSON_AddItemToObject(root_obj, "location_data", location_data_obj)) {
		goto cleanup;
	}

	return 0;

cleanup:
	/* No need to delete others as they were added to here */
	cJSON_Delete(location_data_obj);

	return err;
}

static int location_details_root_encode(
	const struct location_event_data *loc_evt_data,
	const char *mosh_cmd,
	cJSON *root_obj)
{
	int err = 0;

	/* Fill location_req_info */
	err = location_details_location_req_info_encode(mosh_cmd, root_obj);
	if (err) {
		return err;
	}

	/* Fill actual location_data */
	err = location_details_location_data_encode(loc_evt_data, root_obj);
	if (err) {
		return err;
	}

	/* Fill common modem details */
	err = location_details_modem_details_encode(root_obj);
	if (err) {
		mosh_warn("Failed to encode modem_details data to json, err %d", err);
		/* We didn't get modem_details, but we don't care. Let it be empty. */
	}

	return 0;
}

/*
 * Encode location details into JSON format to be sent to nRF Cloud.
 * This is a MOSH specific custom data format.
 *
 * An example output in json_str_out for GNSS is as follows:
 *{
 *	"data": "5.087",
 *	"appId": "LOC_GNSS_TTF",
 *	"messageType": "DATA",
 *	"ts": 1669711208428,
 *	"location_req_info": {
 *		"mosh_cmd": "location get --mode all -m cellular -m wifi -m gnss
 *			     --gnss_cloud_pvt --cloud_details --interval 15",
 *	},
 *	"location_data": {
 *		"method": "GNSS",
 *		"event": "success",
 *		"elapsed_time_method_sec": 5.087,
 *		"position": {
 *			"lng": 23.775882648600284,
 *			"lat": 61.493788965179519,
 *			"acc": 4.484
 *		},
 *		"method_details": {
 *			"tracked_satellites": 10,
 *			"used_satellites": 9,
 *			"pvt_data": {
 *				"flags": 35,
 *				"lng": 23.775882648600284,
 *				"lat": 61.493788965179519,
 *				"acc": 4.484,
 *				"alt": 144.601,
 *				"altAcc": 7.932,
 *				"spd": 0.098,
 *				"spdAcc": 0.345,
 *				"hdg": 0,
 *				"hdgAcc": 180,
 *				"verSpd": 0.026,
 *				"verSpdAcc": 0.655,
 *				"pdop": 2.13,
 *				"hdop": 0.955,
 *				"vdop": 1.904,
 *				"tdop": 1.171,
 *				"sv_info": [
 *					{
 *						"sv": 4,
 *						"c_n0": 47.2,
 *						"sig": 1,
 *						"elev": 52,
 *						"az": 93,
 *						"in_fix": 1,
 *						"unhealthy": 0
 *					},
 *					{
 *						"sv": 6,
 *						"c_n0": 47.1,
 *						"sig": 1,
 *						"elev": 35,
 *						"az": 237,
 *						"in_fix": 1,
 *						"unhealthy": 0
 *					},
 *					{
 *						"sv": 7,
 *						"c_n0": 48.4,
 *						"sig": 1,
 *						"elev": 33,
 *						"az": 191,
 *						"in_fix": 1,
 *						"unhealthy": 0
 *					},
 *					{
 *						"sv": 9,
 *						"c_n0": 48.1,
 *						"sig": 1,
 *						"elev": 80,
 *						"az": 194,
 *						"in_fix": 1,
 *						"unhealthy": 0
 *					},
 *					{
 *						"sv": 11,
 *						"c_n0": 47.5,
 *						"sig": 1,
 *						"elev": 39,
 *						"az": 284,
 *						"in_fix": 1,
 *						"unhealthy": 0
 *					},
 *					{
 *						"sv": 16,
 *						"c_n0": 46.4,
 *						"sig": 1,
 *						"elev": 25,
 *						"az": 84,
 *						"in_fix": 1,
 *						"unhealthy": 0
 *					},
 *					{
 *						"sv": 26,
 *						"c_n0": 42.7,
 *						"sig": 1,
 *						"elev": 25,
 *						"az": 48,
 *						"in_fix": 1,
 *						"unhealthy": 0
 *					},
 *					{
 *						"sv": 29,
 *						"c_n0": 43.5,
 *						"sig": 1,
 *						"elev": 14,
 *						"az": 354,
 *						"in_fix": 1,
 *						"unhealthy": 0
 *					},
 *					{
 *						"sv": 20,
 *						"c_n0": 42.9,
 *						"sig": 1,
 *						"elev": 27,
 *						"az": 302,
 *						"in_fix": 1,
 *						"unhealthy": 0
 *					},
 *					{
 *						"sv": 3,
 *						"c_n0": 42.2,
 *						"sig": 1,
 *						"elev": 9,
 *						"az": 137,
 *						"in_fix": 0,
 *						"unhealthy": 0
 *					}
 *				]
 *			}
 *		}
 *	},
 *	"modem_details": {
 *		"operator_full_name": "ExampleOp",
 *		"plmn": "12398",
 *		"cell_id": 1234567,
 *		"pci": 110,
 *		"band": 20,
 *		"tac": 312,
 *		"rsrp_dbm": -85,
 *		"snr_db": 4,
 *		"sysmode": "LTE-M - GNSS"
 *	}
 *}
 *
 * An example output in json_str_out for Wi-Fi and cellular positioning is as follows:
 *{
 *	"data": "6.804",
 *	"appId": "LOC_WIFI_CELLULAR_TTF",
 *	"messageType": "DATA",
 *	"ts": 1669711208428,
 *	"location_req_info": {
 *		"mosh_cmd": "location get -m gnss -m cellular -m wifi
 *                           --cloud_details --interval 15",
 *	},
 *	"location_data": {
 *		"method": "Wi-Fi + Cellular",
 *		"elapsed_time_method_sec": 6.804,
 *		"position": {
 *			"lng": 23.775882648600284,
 *			"lat": 61.493788965179519,
 *			"acc": 4.484
 *		},
 *		"method_details": {
 *			"cellular_ncells_count":4,
 *			"cellular_gci_cells_count":0,
 *			"wifi_ap_cnt":4
 *		}
 *	},
 *	"modem_details": {
 *		"operator_full_name": "ExampleOp",
 *		"plmn": "12398",
 *		"cell_id": 1234567,
 *		"pci": 110,
 *		"band": 20,
 *		"tac": 312,
 *		"rsrp_dbm": -85,
 *		"snr_db": 4,
 *		"sysmode": "LTE-M - GNSS"
 *	}
 */
int location_details_json_payload_encode(
	const struct location_event_data *loc_evt_data,
	int64_t timestamp_ms,
	const char *mosh_cmd,
	char **json_str_out)
{
	__ASSERT_NO_MSG(loc_evt_data != NULL);
	__ASSERT_NO_MSG(json_str_out != NULL);

	int err = 0;
	cJSON *root_obj = NULL;
	char app_id_str[32];
	const struct location_data_details *details;
	double elapsed_time_method_sec;
	enum location_event_id event_id;

	details = location_details_get(loc_evt_data);
	if (details == NULL) {
		return -EFAULT;
	}
	elapsed_time_method_sec = (double)details->elapsed_time_method / 1000;

	/* Used time for a location request as a root "key"
	 * and custom appId based on used method.
	 */
	switch (loc_evt_data->method) {
	case LOCATION_METHOD_GNSS:
		strcpy(app_id_str, "LOC_GNSS_TTF");
		break;
	case LOCATION_METHOD_CELLULAR:
		strcpy(app_id_str, "LOC_CELL_TTF");
		break;
	case LOCATION_METHOD_WIFI:
		strcpy(app_id_str, "LOC_WIFI_TTF");
		break;
	case LOCATION_METHOD_WIFI_CELLULAR:
		strcpy(app_id_str, "LOC_WIFI_CELLULAR_TTF");
		break;
	default:
		__ASSERT_NO_MSG(false);
		strcpy(app_id_str, "invalid");
		break;
	}

	event_id = loc_evt_data->id;
	if (loc_evt_data->id == LOCATION_EVT_FALLBACK) {
		event_id = loc_evt_data->fallback.cause;
	}

	/* Alter the time for timeout/error to pop out clearly in the card */
	if (event_id == LOCATION_EVT_TIMEOUT) {
		elapsed_time_method_sec = -5;
	} else if (event_id == LOCATION_EVT_ERROR) {
		elapsed_time_method_sec = -1;
	}

	/* This structure corresponds to the General Message Schema described in the
	 * application-protocols repo:
	 * https://github.com/nRFCloud/application-protocols
	 */

	root_obj = cJSON_CreateObject();
	if (root_obj == NULL) {
		err = -ENOMEM;
		goto cleanup;
	}

	if (!cJSON_AddNumberToObjectCS(root_obj, "data", elapsed_time_method_sec) ||
	    !cJSON_AddStringToObjectCS(root_obj, "appId", app_id_str) ||
	    !cJSON_AddStringToObjectCS(root_obj, "messageType", "DATA") ||
	    !cJSON_AddNumberToObjectCS(root_obj, "ts", timestamp_ms)) {
		err = -ENOMEM;
		goto cleanup;
	}

	err = location_details_root_encode(loc_evt_data, mosh_cmd, root_obj);
	if (err) {
		goto cleanup;
	}
	*json_str_out = cJSON_PrintUnformatted(root_obj);
	if (*json_str_out == NULL) {
		err = -ENOMEM;
	}

cleanup:
	if (err != 0) {
		mosh_error("JSON encoding for location details failed, err=%d", err);
	}

	cJSON_Delete(root_obj);

	return err;
}
