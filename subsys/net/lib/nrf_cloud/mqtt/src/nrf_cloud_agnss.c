/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include "nrf_cloud_agnss_internal.h"
#include "nrf_cloud_codec_internal.h"
#include <net/nrf_cloud_codec.h>
#include <net/nrf_cloud_defs.h>
#include <net/nrf_cloud_agnss.h>
#include "nrf_cloud_fsm.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(nrf_cloud_agnss_mqtt, CONFIG_NRF_CLOUD_GPS_LOG_LEVEL);

static bool qzss_assistance_is_supported(void)
{
	/* Assume that all cellular products other than the nRF9160 support this. */
	return (IS_ENABLED(CONFIG_NRF_MODEM_LIB) && !IS_ENABLED(CONFIG_SOC_NRF9160));
}

int nrf_cloud_agnss_request_all(void)
{
	const uint32_t mask = IS_ENABLED(CONFIG_NRF_CLOUD_PGPS) ? 0u : 0xFFFFFFFFu;
	struct nrf_modem_gnss_agnss_data_frame request = {
		.data_flags = NRF_MODEM_GNSS_AGNSS_GPS_UTC_REQUEST |
			      NRF_MODEM_GNSS_AGNSS_KLOBUCHAR_REQUEST |
			      NRF_MODEM_GNSS_AGNSS_NEQUICK_REQUEST |
			      NRF_MODEM_GNSS_AGNSS_GPS_SYS_TIME_AND_SV_TOW_REQUEST |
			      NRF_MODEM_GNSS_AGNSS_POSITION_REQUEST |
			      NRF_MODEM_GNSS_AGNSS_INTEGRITY_REQUEST,
		.system_count = 1,
		.system[0].system_id = NRF_MODEM_GNSS_SYSTEM_GPS,
		.system[0].sv_mask_ephe = mask,
		.system[0].sv_mask_alm = mask};

	if (qzss_assistance_is_supported()) {
		request.system_count = 2;
		request.system[1].system_id = NRF_MODEM_GNSS_SYSTEM_QZSS;
		request.system[1].sv_mask_ephe = 0x3ff;
		request.system[1].sv_mask_alm = 0x3ff;
	}

	return nrf_cloud_agnss_request(&request);
}

int nrf_cloud_agnss_request(const struct nrf_modem_gnss_agnss_data_frame *request)
{
	/* GPS data need is always expected to be present and first in list. */
	__ASSERT(request->system_count > 0, "GNSS system data need not found");
	__ASSERT(request->system[0].system_id == NRF_MODEM_GNSS_SYSTEM_GPS,
		 "GPS data need not found");

	if (nfsm_get_current_state() != STATE_DC_CONNECTED) {
		return -EACCES;
	}
	if (!request) {
		return -EINVAL;
	}

	int err;
	cJSON *agnss_req_obj;
	/* Copy the request so that the ephemeris mask can be modified if necessary */
	struct nrf_modem_gnss_agnss_data_frame req = *request;

	nrf_cloud_agnss_set_request_in_progress(0);

#if defined(CONFIG_NRF_CLOUD_AGNSS_FILTERED)
	/**
	 * Determine if we processed ephemerides assistance less than 2 hours ago; if so,
	 * we can skip this.
	 */
	if (req.system[0].sv_mask_ephe && (last_request_timestamp != 0) &&
	    ((k_uptime_get() - last_request_timestamp) < AGNSS_UPDATE_PERIOD)) {
		LOG_WRN("A-GNSS request was sent less than 2 hours ago");
		req.system[0].sv_mask_ephe = 0;
	}
#endif

	agnss_req_obj = cJSON_CreateObject();
	err = nrf_cloud_agnss_req_json_encode(&req, agnss_req_obj);
	if (!err) {
		err = json_send_to_cloud(agnss_req_obj);
		if (!err) {
			nrf_cloud_agnss_set_request_in_progress(1);
		}
	} else {
		err = -ENOMEM;
	}

	cJSON_Delete(agnss_req_obj);
	return err;
}
