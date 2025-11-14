/*
 * Copyright (c) 2025 PHY Wireless, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-PHYW
 */
#include <string.h>
#include <stdlib.h>

#include <otdoa_al/otdoa_api.h>
#include <otdoa_al/otdoa_nordic_at.h>
#include "otdoa_http.h"
#include "otdoa_al_log.h"

LOG_MODULE_DECLARE(otdoa_al, LOG_LEVEL_INF);


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
		LOG_ERR("otdoa_al_init: failed to check for TLS certificate in tag %d: %d",
			      CONFIG_OTDOA_TLS_SEC_TAG, rc);
		return OTDOA_API_INTERNAL_ERROR;
	}

	if (!exists) {
		LOG_ERR("otdoa_al_init: TLS certificate not found in tag %d",
			      CONFIG_OTDOA_TLS_SEC_TAG);
		return OTDOA_API_INTERNAL_ERROR;
	}
#endif

	return OTDOA_API_SUCCESS;
}

void otdoa_http_invoke_callback_dl_compl(int status)
{
	if (!al_event_callback) {
		LOG_ERR("No registered callback");
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
		LOG_ERR("No registered callback");
		return;
	}

	otdoa_api_event_data_t event_data = {0};

	event_data.event = OTDOA_EVENT_RESULTS_UL_COMPL;
	event_data.ul_compl.status = status;
	al_event_callback(&event_data);
}

int32_t otdoa_api_ubsa_download(const otdoa_api_ubsa_dl_req_t *p_dl_request,
				const bool reset_blacklist)
{
	int32_t rc = 0;

	uint32_t ecgi = p_dl_request->ecgi;
	uint32_t dlearfcn = p_dl_request->dlearfcn;
	uint16_t mcc = p_dl_request->mcc;
	uint16_t mnc = p_dl_request->mnc;
	uint16_t pci = p_dl_request->pci;

	if (ecgi == 0) {
		/* if input ECGI is 0, get current serving cell parameters */
		otdoa_xmonitor_params_t params;

		rc = otdoa_nordic_at_get_xmonitor(&params);
		LOG_INF("otdoa_nordic_at_get_xmonitor() returned %d.  ECGI: %u", rc,
			      params.ecgi);
		ecgi = params.ecgi;
		dlearfcn = params.dlearfcn;
		mcc = params.mcc;
		mnc = params.mnc;
		pci = params.pci;
		if (rc == OTDOA_EVENT_FAIL_NO_DLEARFCN) {
			/* got the ECGI OK but we miss the DLEARFCN. So default to 5230 */
			dlearfcn = UNKNOWN_UBSA_DLEARFCN;
			rc = OTDOA_API_SUCCESS;
		}
		if (rc == OTDOA_EVENT_FAIL_NO_PCI) {
			pci = UNKNOWN_UBSA_PCI;
			rc = OTDOA_API_SUCCESS;
		}
		if (rc != OTDOA_API_SUCCESS) {
			/* other failures */
			return rc;
		}
	}
	/* Send the request message */
	tOTDOA_MSG_HTTP_GET_UBSA msg = { 0 };

	msg.u32MsgId = OTDOA_HTTP_MSG_GET_H1_UBSA;
	LOG_INF("Sending uBSA req for ECGI %u PCI %u DLEARFCN %u",
		ecgi, pci, dlearfcn);
	msg.u32MsgLen = sizeof(msg);
	msg.uEcgi = ecgi;
	msg.uDlearfcn = dlearfcn;
	msg.uRadius = p_dl_request->ubsa_radius_meters;
	msg.uNumCells = p_dl_request->max_cells;
	msg.u16MCC = mcc;
	msg.u16MNC = mnc;
	msg.u16PCI = pci;
	msg.bResetBlacklist = reset_blacklist;
	return otdoa_http_send_message((tOTDOA_HTTP_MESSAGE *)&msg, msg.u32MsgLen);
}

int otdoa_api_cfg_download(void)
{
	tOTDOA_MSG_HTTP_GET_CFG msg;

	msg.u32MsgId = OTDOA_HTTP_MSG_GET_H1_CONFIG_FILE;
	msg.u32MsgLen = sizeof(msg);

	return otdoa_http_send_message((tOTDOA_HTTP_MESSAGE *)&msg, msg.u32MsgLen);
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
