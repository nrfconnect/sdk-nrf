/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stddef.h>
#include <stdint.h>

#include <sdfw/sdfw_services/sdfw_update_service.h>

#include "sdfw_update_service_decode.h"
#include "sdfw_update_service_encode.h"
#include "sdfw_update_service_types.h"
#include <sdfw/sdfw_services/ssf_client.h>
#include "ssf_client_os.h"

SSF_CLIENT_LOG_REGISTER(sdfw_update_service, CONFIG_SSF_SDFW_UPDATE_SERVICE_LOG_LEVEL);

SSF_CLIENT_SERVICE_DEFINE(sdfw_update_srvc, SDFW_UPDATE, cbor_encode_sdfw_update_req,
			  cbor_decode_sdfw_update_rsp);

int ssf_sdfw_update(uintptr_t blob_addr)
{
	int err;
	struct sdfw_update_req req_data;
	int32_t rsp_status;

	req_data.sdfw_update_req_tbs_addr = blob_addr + CONFIG_SSF_SDFW_UPDATE_TBS_OFFSET;
	req_data.sdfw_update_req_dl_max = CONFIG_SSF_SDFW_UPDATE_DOWNLOAD_MAX;
	req_data.sdfw_update_req_dl_addr_fw = blob_addr + CONFIG_SSF_SDFW_UPDATE_FIRMWARE_OFFSET;
	req_data.sdfw_update_req_dl_addr_pk = blob_addr + CONFIG_SSF_SDFW_UPDATE_PUBLIC_KEY_OFFSET;
	req_data.sdfw_update_req_dl_addr_signature =
		blob_addr + CONFIG_SSF_SDFW_UPDATE_SIGNATURE_OFFSET;

	err = ssf_client_send_request(&sdfw_update_srvc, &req_data, &rsp_status, NULL);
	if (err != 0) {
		SSF_CLIENT_LOG_ERR("Error while sending request: %d", err);
		return err;
	}

	if (rsp_status != 0) {
		SSF_CLIENT_LOG_ERR("Error in response: %d", rsp_status);
	}

	return err;
}
