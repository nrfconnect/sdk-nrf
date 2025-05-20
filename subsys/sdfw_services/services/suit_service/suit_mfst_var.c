/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <suit_manifest_variables.h>
#include <sdfw/sdfw_services/ssf_client.h>
#include "suit_service_decode.h"
#include "suit_service_encode.h"
#include "suit_service_types.h"
#include "suit_service_utils.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(suit_mfst_var, CONFIG_SSF_SUIT_SERVICE_LOG_LEVEL);

extern const struct ssf_client_srvc suit_srvc;

suit_plat_err_t suit_mfst_var_get(uint32_t id, uint32_t *val)
{
	int err;
	struct suit_req req = {0};
	struct suit_rsp rsp = {0};
	struct suit_get_manifest_var_req *req_data = NULL;
	struct suit_get_manifest_var_rsp *rsp_data = NULL;
	const uint8_t *rsp_pkt = NULL;

	if (val == NULL) {
		return SUIT_PLAT_ERR_INVAL;
	}

	req.suit_req_msg_choice = SSF_SUIT_REQ_CHOICE(get_manifest_var);
	req_data = &req.SSF_SUIT_REQ(get_manifest_var);

	req_data->SSF_SUIT_REQ_ARG(get_manifest_var, id) = id;

	err = ssf_client_send_request(&suit_srvc, &req, &rsp, &rsp_pkt);
	if (err != 0) {
		LOG_ERR("ssf_client_send_request failed");
		return SUIT_PLAT_ERR_IPC;
	}

	rsp_data = &rsp.SSF_SUIT_RSP(get_manifest_var);
	err = rsp_data->SSF_SUIT_RSP_ARG(get_manifest_var, ret);
	if (err != SUIT_PLAT_SUCCESS) {
		ssf_client_decode_done(rsp_pkt);
		LOG_ERR("ssf_client response return code: %d", err);
		return err;
	}

	*val = rsp_data->SSF_SUIT_RSP_ARG(get_manifest_var, value);
	ssf_client_decode_done(rsp_pkt);

	LOG_INF("suit_mfst_var_get, id: %d, value: %d", id, *val);

	return SUIT_PLAT_SUCCESS;
}

suit_plat_err_t suit_mfst_var_set(uint32_t id, uint32_t val)
{
	int err;
	struct suit_req req = {0};
	struct suit_rsp rsp = {0};
	struct suit_set_manifest_var_req *req_data = NULL;

	LOG_INF("suit_mfst_var_set, id: %d, value: %d", id, val);

	req.suit_req_msg_choice = SSF_SUIT_REQ_CHOICE(set_manifest_var);
	req_data = &req.SSF_SUIT_REQ(set_manifest_var);

	req_data->SSF_SUIT_REQ_ARG(set_manifest_var, id) = id;
	req_data->SSF_SUIT_REQ_ARG(set_manifest_var, value) = val;

	err = ssf_client_send_request(&suit_srvc, &req, &rsp, NULL);
	if (err != 0) {
		LOG_ERR("ssf_client_send_request failed");
		return SUIT_PLAT_ERR_IPC;
	}

	return rsp.SSF_SUIT_RSP(set_manifest_var).SSF_SUIT_RSP_ARG(set_manifest_var, ret);
}
