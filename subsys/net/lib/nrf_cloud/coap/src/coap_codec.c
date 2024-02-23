/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <string.h>
#include <math.h>

#include <zephyr/kernel.h>
#include <zephyr/net/coap.h>
#include <modem/lte_lc.h>
#include <net/wifi_location_common.h>
#include <net/nrf_cloud_codec.h>
#include <cJSON.h>
#include "nrf_cloud_codec_internal.h"
#include "ground_fix_encode_types.h"
#include "ground_fix_encode.h"
#include "ground_fix_decode_types.h"
#include "ground_fix_decode.h"
#include "agnss_encode_types.h"
#include "agnss_encode.h"
#include "pgps_encode_types.h"
#include "pgps_encode.h"
#include "pgps_decode_types.h"
#include "pgps_decode.h"
#include "msg_encode_types.h"
#include "msg_encode.h"
#include "coap_codec.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(coap_codec, CONFIG_NRF_CLOUD_COAP_LOG_LEVEL);

/* Default elevation angle used by the modem to chose whether to include a given satellite
 * in calculating a fix. Satellites whose elevation is below this angle are not included.
 */
#define DEFAULT_MASK_ANGLE 5

static int encode_message(struct nrf_cloud_obj_coap_cbor *msg, uint8_t *buf, size_t *len,
			  enum coap_content_format fmt)
{
	int err;

	if (fmt == COAP_CONTENT_FORMAT_APP_CBOR) {
		struct message_out input;
		size_t out_len;

		memset(&input, 0, sizeof(struct message_out));
		input.message_out_appId.value = msg->app_id;
		input.message_out_appId.len = strlen(msg->app_id);

		switch (msg->type) {
		case NRF_CLOUD_DATA_TYPE_NONE:
			LOG_ERR("Cannot encode unknown type.");
			return -EINVAL;
		case NRF_CLOUD_DATA_TYPE_STR:
			input.message_out_data_choice = message_out_data_tstr_c;
			input.message_out_data_tstr.value = msg->str_val;
			input.message_out_data_tstr.len = strlen(msg->str_val);
			break;
		case NRF_CLOUD_DATA_TYPE_PVT:
			input.message_out_data_choice = message_out_data_pvt_m_c;
			input.message_out_data_pvt_m.pvt_lat = msg->pvt->lat;
			input.message_out_data_pvt_m.pvt_lng = msg->pvt->lon;
			input.message_out_data_pvt_m.pvt_acc = msg->pvt->accuracy;
			if (msg->pvt->has_speed) {
				input.message_out_data_pvt_m.pvt_spd.pvt_spd = msg->pvt->speed;
				input.message_out_data_pvt_m.pvt_spd_present = true;
			}
			if (msg->pvt->has_heading) {
				input.message_out_data_pvt_m.pvt_hdg.pvt_hdg = msg->pvt->heading;
				input.message_out_data_pvt_m.pvt_hdg_present = true;
			}
			if (msg->pvt->has_alt) {
				input.message_out_data_pvt_m.pvt_alt.pvt_alt = msg->pvt->alt;
				input.message_out_data_pvt_m.pvt_alt_present = true;
			}
			break;
		case NRF_CLOUD_DATA_TYPE_INT:
			input.message_out_data_choice = message_out_data_int_c;
			input.message_out_data_int = msg->int_val;
			break;
		case NRF_CLOUD_DATA_TYPE_DOUBLE:
			input.message_out_data_choice = message_out_data_float_c;
			input.message_out_data_float = msg->double_val;
			break;
		}
		input.message_out_ts.message_out_ts = msg->ts;
		input.message_out_ts_present = true;
		err = cbor_encode_message_out(buf, *len, &input, &out_len);
		if (err) {
			LOG_ERR("Error %d encoding message", err);
			err = -EINVAL;
			*len = 0;
		} else {
			*len = out_len;
		}
	} else if (fmt == COAP_CONTENT_FORMAT_APP_JSON) {
		struct nrf_cloud_data out;

		err = nrf_cloud_encode_message(msg->app_id, msg->double_val, msg->str_val, NULL,
					       msg->ts, &out);
		if (err) {
			*len = 0;
		} else if (*len <= out.len) {
			*len = 0;
			cJSON_free((void *)out.ptr);
			err = -E2BIG;
		} else {
			*len = out.len;
			memcpy(buf, out.ptr, *len);
			buf[*len - 1] = '\0';
			cJSON_free((void *)out.ptr);
		}
	} else {
		err = -EINVAL;
	}
	return err;
}

int coap_codec_message_encode(struct nrf_cloud_obj_coap_cbor *msg, uint8_t *buf, size_t *len,
			      enum coap_content_format fmt)
{
	__ASSERT_NO_MSG(msg != NULL);
	__ASSERT_NO_MSG(msg->app_id != NULL);
	__ASSERT_NO_MSG(buf != NULL);
	__ASSERT_NO_MSG(len != NULL);

	return encode_message(msg, buf, len, fmt);
}

int coap_codec_sensor_encode(const char *app_id, double float_val,
			     int64_t ts, uint8_t *buf, size_t *len, enum coap_content_format fmt)
{
	__ASSERT_NO_MSG(app_id != NULL);
	__ASSERT_NO_MSG(buf != NULL);
	__ASSERT_NO_MSG(len != NULL);

	struct nrf_cloud_obj_coap_cbor msg = {
		.app_id		= (char *)app_id,
		.type		= NRF_CLOUD_DATA_TYPE_DOUBLE,
		.double_val	= float_val,
		.ts		= ts
	};

	return encode_message(&msg, buf, len, fmt);
}

int coap_codec_pvt_encode(const char *app_id, const struct nrf_cloud_gnss_pvt *pvt,
			  int64_t ts, uint8_t *buf, size_t *len, enum coap_content_format fmt)
{
	__ASSERT_NO_MSG(app_id != NULL);
	__ASSERT_NO_MSG(pvt != NULL);
	__ASSERT_NO_MSG(buf != NULL);
	__ASSERT_NO_MSG(len != NULL);

	struct nrf_cloud_obj_coap_cbor msg = {
		.app_id		= (char *)app_id,
		.type		= NRF_CLOUD_DATA_TYPE_PVT,
		.pvt		= (struct nrf_cloud_gnss_pvt *)pvt,
		.ts		= ts
	};

	return encode_message(&msg, buf, len, fmt);
}

static void copy_cell(struct cell *dst, struct lte_lc_cell const *const src)
{
	dst->cell_mcc = src->mcc;
	dst->cell_mnc = src->mnc;
	dst->cell_eci = src->id;
	dst->cell_tac = src->tac;

	dst->cell_earfcn.cell_earfcn = src->earfcn;
	dst->cell_earfcn_present = (src->earfcn != NRF_CLOUD_LOCATION_CELL_OMIT_EARFCN);

	dst->cell_adv.cell_adv = MIN(src->timing_advance, NRF_CLOUD_LOCATION_CELL_TIME_ADV_MAX);
	dst->cell_adv_present = (src->timing_advance != NRF_CLOUD_LOCATION_CELL_OMIT_TIME_ADV);

	dst->cell_rsrp.cell_rsrp = RSRP_IDX_TO_DBM(src->rsrp);
	dst->cell_rsrp_present = (src->rsrp != NRF_CLOUD_LOCATION_CELL_OMIT_RSRP);

	dst->cell_rsrq.cell_rsrq_float32 = RSRQ_IDX_TO_DB(src->rsrq);
	dst->cell_rsrq.cell_rsrq_choice = cell_rsrq_float32_c;
	dst->cell_rsrq_present = (src->rsrq != NRF_CLOUD_LOCATION_CELL_OMIT_RSRQ);
}

static void copy_ncells(struct ncell *dst, int num, struct lte_lc_ncell *src)
{
	for (int i = 0; i < num; i++) {
		dst->ncell_earfcn = src->earfcn;
		dst->ncell_pci = src->phys_cell_id;
		if (src->rsrp != NRF_CLOUD_LOCATION_CELL_OMIT_RSRP) {
			dst->ncell_rsrp.ncell_rsrp = RSRP_IDX_TO_DBM(src->rsrp);
			dst->ncell_rsrp_present = true;
		} else {
			dst->ncell_rsrp_present = false;
		}
		if (src->rsrq != NRF_CLOUD_LOCATION_CELL_OMIT_RSRQ) {
			dst->ncell_rsrq.ncell_rsrq_float32 = RSRQ_IDX_TO_DB(src->rsrq);
			dst->ncell_rsrq.ncell_rsrq_choice = ncell_rsrq_float32_c;
			dst->ncell_rsrq_present = true;
		} else {
			dst->ncell_rsrq_present = false;
		}
		if (src->time_diff != LTE_LC_CELL_TIME_DIFF_INVALID) {
			dst->ncell_timeDiff.ncell_timeDiff = src->time_diff;
			dst->ncell_timeDiff_present = true;
		} else {
			dst->ncell_timeDiff_present = false;
		}
		src++;
		dst++;
	}
}

static void copy_cell_info(struct lte_ar *lte_encode,
			   struct lte_lc_cells_info const *const cell_info)
{
	if (cell_info == NULL) {
		return;
	}

	const size_t max_cells = ARRAY_SIZE(lte_encode->lte_ar_cell_m);
	size_t cnt = 0;
	size_t nmrs;
	struct cell *enc_cell = lte_encode->lte_ar_cell_m;

	if (cell_info->current_cell.id != LTE_LC_CELL_EUTRAN_ID_INVALID) {

		/* Copy serving cell */
		copy_cell(enc_cell, &cell_info->current_cell);

		/* Copy neighbor cell(s) */
		nmrs = MIN(ARRAY_SIZE(enc_cell->cell_nmr.cell_nmr_ncells),
			   cell_info->ncells_count);
		if (nmrs) {
			copy_ncells(enc_cell->cell_nmr.cell_nmr_ncells,
				    nmrs,
				    cell_info->neighbor_cells);
		}

		enc_cell->cell_nmr.cell_nmr_ncells_count = nmrs;
		enc_cell->cell_nmr_present = (nmrs > 0);

		LOG_DBG("Copied serving cell and %zd neighbor cells", nmrs);

		/* Advance to next cell */
		cnt++;
		enc_cell++;
	}

	if ((cell_info->gci_cells != NULL) && (cell_info->gci_cells_count)) {

		for (int i = 0; (i < cell_info->gci_cells_count) && (cnt < max_cells); i++) {
			copy_cell(enc_cell++, &cell_info->gci_cells[i]);
			cnt++;
		}

		LOG_DBG("Copied %u GCI cells", cell_info->gci_cells_count);
	}

	lte_encode->lte_ar_cell_m_count = cnt;
}

static void copy_wifi_info(struct wifi_ob *wifi_encode,
			   struct wifi_scan_info const *const wifi_info)
{
	struct ap *dst = wifi_encode->wifi_ob_accessPoints_ap_m;
	struct wifi_scan_result *src = wifi_info->ap_info;
	size_t num_aps = MIN(ARRAY_SIZE(wifi_encode->wifi_ob_accessPoints_ap_m), wifi_info->cnt);

	wifi_encode->wifi_ob_accessPoints_ap_m_count = num_aps;

	for (int i = 0; i < num_aps; i++, src++, dst++) {
		dst->ap_macAddress.value = src->mac;
		dst->ap_macAddress.len = src->mac_length;
		dst->ap_age_present = false;

		dst->ap_signalStrength.ap_signalStrength = src->rssi;
		dst->ap_signalStrength_present = (src->rssi != NRF_CLOUD_LOCATION_WIFI_OMIT_RSSI);

		dst->ap_channel.ap_channel = src->channel;
		dst->ap_channel_present = (src->channel != NRF_CLOUD_LOCATION_WIFI_OMIT_CHAN);

		dst->ap_frequency_present = false;
		dst->ap_ssid_present = (IS_ENABLED(CONFIG_NRF_CLOUD_COAP_SEND_SSIDS) &&
					 (src->ssid_length && src->ssid[0]));
		dst->ap_ssid.ap_ssid.value = src->ssid;
		dst->ap_ssid.ap_ssid.len = src->ssid_length;
	}
}

int coap_codec_ground_fix_req_encode(struct lte_lc_cells_info const *const cell_info,
				     struct wifi_scan_info const *const wifi_info,
				     uint8_t *buf, size_t *len, enum coap_content_format fmt)
{
	__ASSERT_NO_MSG((cell_info != NULL) || (wifi_info != NULL));
	__ASSERT_NO_MSG(buf != NULL);
	__ASSERT_NO_MSG(len != NULL);

	if (fmt != COAP_CONTENT_FORMAT_APP_CBOR) {
		LOG_ERR("Invalid format for ground fix: %d", fmt);
		return -ENOTSUP;
	}

	int err = 0;
	struct ground_fix_req input;
	size_t out_len;

	memset(&input, 0, sizeof(struct ground_fix_req));
	input.ground_fix_req_lte_present = (cell_info != NULL);
	if (cell_info) {
		copy_cell_info(&input.ground_fix_req_lte.ground_fix_req_lte, cell_info);
	}
	input.ground_fix_req_wifi_present = (wifi_info != NULL);
	if (wifi_info) {
		copy_wifi_info(&input.ground_fix_req_wifi.ground_fix_req_wifi, wifi_info);
	}
	err = cbor_encode_ground_fix_req(buf, *len, &input, &out_len);
	if (err) {
		LOG_ERR("Error %d encoding ground fix", err);
		err = -EINVAL;
		*len = 0;
	} else {
		*len = out_len;
	}
	return err;
}

int coap_codec_ground_fix_resp_decode(struct nrf_cloud_location_result *result,
				      const uint8_t *buf, size_t len, enum coap_content_format fmt)
{
	__ASSERT_NO_MSG(result != NULL);
	__ASSERT_NO_MSG(buf != NULL);
	int err;

	if (fmt == COAP_CONTENT_FORMAT_APP_JSON) {
		enum nrf_cloud_error nrf_err;

		/* Check for a potential ground fix JSON error message from nRF Cloud */
		err = nrf_cloud_error_msg_decode(buf, NRF_CLOUD_JSON_APPID_VAL_LOCATION,
						 NRF_CLOUD_JSON_MSG_TYPE_VAL_DATA, &nrf_err);
		if (!err) {
			LOG_ERR("nRF Cloud returned ground fix error: %d", nrf_err);
			return -EFAULT;
		}
		LOG_ERR("Invalid ground fix response format: %d", err);
		return -EPROTO;
	}
	if (fmt != COAP_CONTENT_FORMAT_APP_CBOR) {
		LOG_ERR("Invalid format for ground fix: %d", fmt);
		return -ENOTSUP;
	}

	struct ground_fix_resp res;
	size_t out_len;

	err = cbor_decode_ground_fix_resp(buf, len, &res, &out_len);
	if (!err && (out_len != len)) {
		LOG_WRN("Different response length: expected:%zd, decoded:%zd",
			len, out_len);
	}
	if (err) {
		return err;
	}

	const char *with = res.ground_fix_resp_fulfilledWith.value;
	int rlen = res.ground_fix_resp_fulfilledWith.len;

	if (strncmp(with, NRF_CLOUD_LOCATION_TYPE_VAL_MCELL, rlen) == 0) {
		result->type = LOCATION_TYPE_MULTI_CELL;
	} else if (strncmp(with, NRF_CLOUD_LOCATION_TYPE_VAL_SCELL, rlen) == 0) {
		result->type = LOCATION_TYPE_SINGLE_CELL;
	} else if (strncmp(with, NRF_CLOUD_LOCATION_TYPE_VAL_WIFI, rlen) == 0) {
		result->type = LOCATION_TYPE_WIFI;
	} else {
		result->type = LOCATION_TYPE__INVALID;
		LOG_WRN("Unhandled location type: %*s", rlen, with);
	}

	result->lat = res.ground_fix_resp_lat;
	result->lon = res.ground_fix_resp_lon;

	if (res.ground_fix_resp_uncertainty_choice == ground_fix_resp_uncertainty_int_c) {
		result->unc = res.ground_fix_resp_uncertainty_int;
	} else {
		result->unc = (uint32_t)lround(res.ground_fix_resp_uncertainty_float);
	}
	return err;
}

#if defined(CONFIG_NRF_CLOUD_AGNSS)
int coap_codec_agnss_encode(struct nrf_cloud_rest_agnss_request const *const request,
			   uint8_t *buf, size_t *len, enum coap_content_format fmt)
{
	__ASSERT_NO_MSG(request != NULL);
	__ASSERT_NO_MSG(buf != NULL);
	__ASSERT_NO_MSG(len != NULL);

	if (fmt != COAP_CONTENT_FORMAT_APP_CBOR) {
		LOG_ERR("Invalid format for A-GNSS: %d", fmt);
		return -ENOTSUP;
	}

	int ret;
	struct agnss_req input;
	struct agnss_req_types_r *t = &input.agnss_req_types;
	size_t out_len;
	enum nrf_cloud_agnss_type types[ARRAY_SIZE(t->agnss_req_types_int)];
	int cnt;

	memset(&input, 0, sizeof(struct agnss_req));
	input.agnss_req_eci = request->net_info->current_cell.id;
	input.agnss_req_mcc = request->net_info->current_cell.mcc;
	input.agnss_req_mnc = request->net_info->current_cell.mnc;
	input.agnss_req_tac = request->net_info->current_cell.tac;
	if (request->type == NRF_CLOUD_REST_AGNSS_REQ_CUSTOM) {
		cnt = nrf_cloud_agnss_type_array_get(request->agnss_req, types, ARRAY_SIZE(types));
		t->agnss_req_types_int_count = cnt;
		input.agnss_req_types_present = true;
		LOG_DBG("num elements: %d", cnt);
		for (int i = 0; i < cnt; i++) {
			LOG_DBG("  %d: %d", i, types[i]);
			t->agnss_req_types_int[i] = (int)types[i];
		}
	} else {
		LOG_ERR("Invalid request type: %d", request->type);
		return -ENOTSUP;
	}
	if (request->filtered) {
		input.agnss_req_filtered_present = true;
		input.agnss_req_filtered.agnss_req_filtered = true;
		if (request->mask_angle != DEFAULT_MASK_ANGLE) {
			input.agnss_req_mask_present = true;
			input.agnss_req_mask.agnss_req_mask = request->mask_angle;
		}
	}
	if (request->net_info->current_cell.rsrp != NRF_CLOUD_LOCATION_CELL_OMIT_RSRP) {
		input.agnss_req_rsrp.agnss_req_rsrp = request->net_info->current_cell.rsrp;
		input.agnss_req_rsrp_present = true;
	}
	ret = cbor_encode_agnss_req(buf, *len, &input, &out_len);
	if (ret) {
		LOG_ERR("Error %d encoding A-GNSS req", ret);
		ret = -EINVAL;
		*len = 0;
	} else {
		*len = out_len;
	}
	return ret;
}

int coap_codec_agnss_resp_decode(struct nrf_cloud_rest_agnss_result *result,
				const uint8_t *buf, size_t len, enum coap_content_format fmt)
{
	__ASSERT_NO_MSG(result != NULL);
	__ASSERT_NO_MSG(buf != NULL);

	if (fmt == COAP_CONTENT_FORMAT_APP_JSON) {
		enum nrf_cloud_error nrf_err;
		int err;

		/* Check for a potential A-GNSS JSON error message from nRF Cloud */
		err = nrf_cloud_error_msg_decode(buf, NRF_CLOUD_JSON_APPID_VAL_AGNSS,
						 NRF_CLOUD_JSON_MSG_TYPE_VAL_DATA, &nrf_err);
		if (!err) {
			LOG_ERR("nRF Cloud returned A-GNSS error: %d", nrf_err);
			return -EFAULT;
		}
		LOG_ERR("Invalid A-GNSS response format");
		return -EPROTO;
	}
	if (fmt != COAP_CONTENT_FORMAT_APP_OCTET_STREAM) {
		LOG_ERR("Invalid format for A-GNSS data: %d", fmt);
		return -ENOTSUP;
	}
	if (!result) {
		return -EINVAL;
	}

	if (result->buf_sz < len) {
		LOG_WRN("A-GNSS buffer is too small; expected: %zd, got:%zd; truncated",
			len, result->buf_sz);
	}
	result->agnss_sz = MIN(result->buf_sz, len);
	memcpy(result->buf, buf, result->agnss_sz);
	return 0;
}
#endif /* CONFIG_NRF_CLOUD_AGNSS */

#if defined(CONFIG_NRF_CLOUD_PGPS)
int coap_codec_pgps_encode(struct nrf_cloud_rest_pgps_request const *const request,
			   uint8_t *buf, size_t *len,
			   enum coap_content_format fmt)
{
	__ASSERT_NO_MSG(request != NULL);
	__ASSERT_NO_MSG(request->pgps_req != NULL);
	__ASSERT_NO_MSG(buf != NULL);
	__ASSERT_NO_MSG(len != NULL);

	if (fmt != COAP_CONTENT_FORMAT_APP_CBOR) {
		LOG_ERR("Invalid format for P-GPS: %d", fmt);
		return -ENOTSUP;
	}

	int ret;
	struct pgps_req input;
	size_t out_len;

	memset(&input, 0, sizeof(input));

	input.pgps_req_predictionCount = request->pgps_req->prediction_count;
	input.pgps_req_predictionIntervalMinutes = request->pgps_req->prediction_period_min;
	input.pgps_req_startGPSDay = request->pgps_req->gps_day;
	input.pgps_req_startGPSTimeOfDaySeconds = request->pgps_req->gps_time_of_day;

	ret = cbor_encode_pgps_req(buf, *len, &input, &out_len);
	if (ret) {
		LOG_ERR("Error %d encoding P-GPS req", ret);
		ret = -EINVAL;
		*len = 0;
	} else {
		*len = out_len;
	}

	return ret;
}

int coap_codec_pgps_resp_decode(struct nrf_cloud_pgps_result *result,
				const uint8_t *buf, size_t len, enum coap_content_format fmt)
{
	__ASSERT_NO_MSG(result != NULL);
	__ASSERT_NO_MSG(buf != NULL);
	int err;

	if (fmt == COAP_CONTENT_FORMAT_APP_JSON) {
		enum nrf_cloud_error nrf_err;

		/* Check for a potential P-GPS JSON error message from nRF Cloud */
		err = nrf_cloud_error_msg_decode(buf, NRF_CLOUD_JSON_APPID_VAL_PGPS,
						 NRF_CLOUD_JSON_MSG_TYPE_VAL_DATA, &nrf_err);
		if (!err) {
			LOG_ERR("nRF Cloud returned P-GPS error: %d", nrf_err);
			return -EFAULT;
		}
		LOG_ERR("Invalid P-GPS response format");
		return -EPROTO;
	}
	if (fmt != COAP_CONTENT_FORMAT_APP_CBOR) {
		LOG_ERR("Invalid format for P-GPS: %d", fmt);
		return -ENOTSUP;
	}

	struct pgps_resp resp;
	size_t resp_len;

	err = cbor_decode_pgps_resp(buf, len, &resp, &resp_len);
	if (!err) {
		strncpy(result->host, resp.pgps_resp_host.value, result->host_sz);
		if (result->host_sz > resp.pgps_resp_host.len) {
			result->host[resp.pgps_resp_host.len] = '\0';
		}
		result->host_sz = resp.pgps_resp_host.len;
		strncpy(result->path, resp.pgps_resp_path.value, result->path_sz);
		if (result->path_sz > resp.pgps_resp_path.len) {
			result->path[resp.pgps_resp_path.len] = '\0';
		}
		result->path_sz = resp.pgps_resp_path.len;
	}
	return err;
}
#endif /* CONFIG_NRF_CLOUD_PGPS */

int coap_codec_fota_resp_decode(struct nrf_cloud_fota_job_info *job,
				const uint8_t *buf, size_t len, enum coap_content_format fmt)
{
	__ASSERT_NO_MSG(job != NULL);
	__ASSERT_NO_MSG(buf != NULL);

	if (fmt == COAP_CONTENT_FORMAT_APP_CBOR) {
		LOG_ERR("Invalid format for FOTA: %d", fmt);
		return -ENOTSUP;
	} else {
		return nrf_cloud_rest_fota_execution_decode(buf, job);
	}
}
