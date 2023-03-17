/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef COAP_CODEC_H_
#define COAP_CODEC_H_

#include <net/nrf_cloud.h>
#include <net/nrf_cloud_pgps.h>
#include <net/nrf_cloud_rest.h>
#include <net/nrf_cloud_location.h>
#include <modem/lte_lc.h>

#define RC_FMT "%u.%02u"
#define RC_PARAM(rc) (rc) / 32u, (rc) & 0x1f

#define LOG_CB_DBG(result_code, offset, len, last_block) { \
		LOG_DBG("result_code:" RC_FMT ", offset:0x%X, len:0x%X, last_block:%d", \
			RC_PARAM(result_code), (offset), (len), (last_block)); \
	}
#define LOG_RESULT_CODE_ERR(msg, result_code) { \
		LOG_ERR("%s " RC_FMT, (msg), RC_PARAM(result_code)); \
}
#define LOG_RESULT_CODE_INF(msg, result_code) { \
		LOG_INF("%s " RC_FMT, (msg), RC_PARAM(result_code)); \
}

int coap_codec_message_encode(const char *app_id,
			      const char *str_val, double float_val, int int_val,
			      int64_t ts, uint8_t *buf, size_t *len, enum coap_content_format fmt);

int coap_codec_sensor_encode(const char *app_id, double float_val,
			     int64_t ts, uint8_t *buf, size_t *len,
			     enum coap_content_format fmt);

int coap_codec_pvt_encode(const char *app_id, const struct nrf_cloud_gnss_pvt *pvt,
			  int64_t ts, uint8_t *buf, size_t *len, enum coap_content_format fmt);

int coap_codec_ground_fix_req_encode(struct lte_lc_cells_info const *const cell_info,
				     struct wifi_scan_info const *const wifi_info,
				     uint8_t *buf, size_t *len, enum coap_content_format fmt);

int coap_codec_ground_fix_resp_decode(struct nrf_cloud_location_result *result,
				      const uint8_t *buf, size_t len, enum coap_content_format fmt);

int coap_codec_agps_encode(struct nrf_cloud_rest_agps_request const *const request,
			   uint8_t *buf, size_t *len,
			   enum coap_content_format fm);

int coap_codec_agps_resp_decode(struct nrf_cloud_rest_agps_result *result,
				const uint8_t *buf, size_t len, enum coap_content_format fmt);

int coap_codec_pgps_encode(struct nrf_cloud_rest_pgps_request const *const request,
			   uint8_t *buf, size_t *len,
			   enum coap_content_format fmt);

int coap_codec_pgps_resp_decode(struct nrf_cloud_pgps_result *result,
				const uint8_t *buf, size_t len, enum coap_content_format fmt);

int coap_codec_fota_resp_decode(struct nrf_cloud_fota_job_info *job,
				const uint8_t *buf, size_t len, enum coap_content_format fmt);

#endif /* COAP_CODEC_H_ */
