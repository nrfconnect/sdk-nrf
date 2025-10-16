/*
 * Copyright (c) 2025 PHY Wireless, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-PHYW
 */
#include <string.h>
#include <stdlib.h>

#include <otdoa_al/phywi_otdoa_api.h>
#include <otdoa_al/otdoa_nordic_at_h1.h>
#include "otdoa_http.h"
#include "otdoa_al_log.h"

static otdoa_api_callback_t al_event_callback;
/**
 * @brief Initialize the OTDOA AL library
 * @param[in] event_callback Callback function used by the library
 *                     to return results and status to the client
 * @return 0 on success
 */
int32_t otdoa_al_init(otdoa_api_callback_t event_callback)
{
	al_event_callback = event_callback;
	otdoa_http_init();
	otdoa_log_init();

#if CONFIG_OTDOA_API_TLS_CERT_INSTALL
	bool exists;
	int rc = modem_key_mgmt_exists(CONFIG_OTDOA_TLS_SEC_TAG, OTDOA_TLS_CERT_TYPE, &exists);

	if (rc) {
		OTDOA_LOG_ERR("otdoa_al_init: failed to check for TLS certificate in tag %d: %d",
			      CONFIG_OTDOA_TLS_SEC_TAG, rc);
		return OTDOA_API_INTERNAL_ERROR;
	}

	if (!exists) {
		OTDOA_LOG_ERR("otdoa_al_init: TLS certificate not found in tag %d",
			      CONFIG_OTDOA_TLS_SEC_TAG);
		return OTDOA_API_INTERNAL_ERROR;
	}
#endif

	return OTDOA_API_SUCCESS;
}

void otdoa_http_invoke_callback_dl_compl(int status)
{
	if (!al_event_callback) {
		otdoa_log_err("No registered callback");
		return;
	}

	otdoa_api_event_data_t event_data = {0};

	event_data.event = OTDOA_EVENT_UBSA_DL_COMPL;
	event_data.dl_compl.status = status;
	al_event_callback(&event_data);
}

void otdoa_http_invoke_callback_ul_compl(int status)
{
	if (!al_event_callback) {
		otdoa_log_err("No registered callback");
		return;
	}

	otdoa_api_event_data_t event_data = {0};

	event_data.event = OTDOA_EVENT_RESULTS_UL_COMPL;
	event_data.ul_compl.status = status;
	al_event_callback(&event_data);
}

int32_t otdoa_api_ubsa_download(const otdoa_api_ubsa_dl_req_t *p_dl_request,
				const char *const ubsa_file_path, const bool reset_blacklist)
{
	int32_t rc = 0;

	/* if input ECGI is 0, get current serving cell ECGI & DLEARFCN */
	uint32_t ecgi = p_dl_request->ecgi;
	uint32_t dlearfcn = p_dl_request->dlearfcn;
	uint16_t mcc = p_dl_request->mcc;
	uint16_t mnc = p_dl_request->mnc;

	if (ecgi == 0) {
		rc = otdoa_nordic_at_get_ecgi_and_dlearfcn(&ecgi, &dlearfcn, &mcc, &mnc);
		OTDOA_LOG_INF("otdoa_nordic_at_get_ecgi_and_dlearfcn() returned %d.  ECGI: %u", rc,
			      ecgi);
		if (rc == OTDOA_EVENT_FAIL_NO_DLEARFCN && ecgi != 0) {
			/* got the ECGI OK but we miss the DLEARFCN. So default to 5230 */
			dlearfcn = DEFAULT_UBSA_DLEARFCN;
			rc = 0;
		} else if (rc != 0) {
			/* other failures */
			return rc;
		}
	}

	rc = otdoa_http_send_ubsa_req(BSA_DL_SERVER_URL, ecgi, dlearfcn,
				      p_dl_request->ubsa_radius_meters, p_dl_request->max_cells,
				      mcc, mnc, reset_blacklist);
	return rc;
}

int otdoa_api_cfg_download(void)
{
	tOTDOA_MSG_HTTP_GET_CFG msg;

	msg.u32MsgId = OTDOA_HTTP_MSG_GET_H1_CONFIG_FILE;
	msg.u32MsgLen = sizeof(msg);

	return otdoa_http_send_message((tOTDOA_HTTP_MESSAGE *)&msg);
}

#ifdef CONFIG_OTDOA_ENABLE_RESULTS_UPLOAD
int32_t otdoa_api_upload_results(const otdoa_api_results_t *p_results, const char *true_lat,
				 const char *true_lon, const char *notes)
{
	if (!p_results) {
		return OTDOA_API_ERROR_PARAM;
	}
	otdoa_api_results_t *p_http_results = calloc(1, sizeof(otdoa_api_results_t));

	if (!p_http_results) {
		return OTDOA_API_INTERNAL_ERROR;
	}
	memcpy(p_http_results, p_results, sizeof(otdoa_api_results_t));
	int rv = otdoa_http_send_results_upload(UPLOAD_SERVER_URL, p_http_results, notes, true_lat,
						true_lon);
	return (rv == 0 ? OTDOA_API_SUCCESS : OTDOA_API_INTERNAL_ERROR);
}
#endif
