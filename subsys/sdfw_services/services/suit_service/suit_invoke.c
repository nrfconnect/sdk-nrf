/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <sdfw/sdfw_services/ssf_client.h>
#include "suit_service_types.h"
#include <sdfw/sdfw_services/suit_service.h>
#include "suit_service_utils.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(suit_srvc_invoke, CONFIG_SSF_SUIT_SERVICE_LOG_LEVEL);

extern const struct ssf_client_srvc suit_srvc;

suit_ssf_err_t suit_boot_mode_read(suit_boot_mode_t *mode)
{
	int ret;
	struct suit_req req;
	struct suit_rsp rsp;
	struct suit_boot_mode_read_rsp *rsp_data;
	const uint8_t *rsp_pkt;

	if (mode == NULL) {
		return SUIT_PLAT_ERR_INVAL;
	}

	memset(&req, 0, sizeof(struct suit_req));
	req.suit_req_msg_choice = SSF_SUIT_REQ_CHOICE(boot_mode_read);

	ret = ssf_client_send_request(&suit_srvc, &req, &rsp, &rsp_pkt);
	if (ret != 0) {
		ssf_client_decode_done(rsp_pkt);
		return SUIT_PLAT_ERR_IPC;
	}

	rsp_data = &rsp.SSF_SUIT_RSP(boot_mode_read);
	ret = rsp_data->SSF_SUIT_RSP_ARG(boot_mode_read, ret);
	if (ret != SUIT_PLAT_SUCCESS) {
		ssf_client_decode_done(rsp_pkt);
		return ret;
	}

	*mode = (suit_boot_mode_t)rsp_data->SSF_SUIT_RSP_ARG(boot_mode_read, boot_mode);

	ssf_client_decode_done(rsp_pkt);

	return ret;
}

suit_ssf_err_t suit_invoke_confirm(int ret)
{
	int ssf_ret;
	struct suit_req req;
	struct suit_rsp rsp;
	struct suit_invoke_confirm_req *req_data;

	memset(&req, 0, sizeof(struct suit_req));
	req.suit_req_msg_choice = SSF_SUIT_REQ_CHOICE(invoke_confirm);
	req_data = &req.SSF_SUIT_REQ(invoke_confirm);
	req_data->SSF_SUIT_REQ_ARG(invoke_confirm, ret) = ret;

	ssf_ret = ssf_client_send_request(&suit_srvc, &req, &rsp, NULL);
	if (ssf_ret != 0) {
		return SUIT_PLAT_ERR_IPC;
	}

	return rsp.SSF_SUIT_RSP(invoke_confirm).SSF_SUIT_RSP_ARG(invoke_confirm, ret);
}

suit_ssf_err_t suit_boot_flags_reset(void)
{
	int ret;
	struct suit_req req;
	struct suit_rsp rsp;

	memset(&req, 0, sizeof(struct suit_req));
	req.suit_req_msg_choice = SSF_SUIT_REQ_CHOICE(boot_flags_reset);

	ret = ssf_client_send_request(&suit_srvc, &req, &rsp, NULL);
	if (ret != 0) {
		return SUIT_PLAT_ERR_IPC;
	}

	return rsp.SSF_SUIT_RSP(boot_flags_reset).SSF_SUIT_RSP_ARG(boot_flags_reset, ret);
}

suit_ssf_err_t suit_foreground_dfu_required(void)
{
	int ret;
	struct suit_req req;
	struct suit_rsp rsp;

	memset(&req, 0, sizeof(struct suit_req));
	req.suit_req_msg_choice = SSF_SUIT_REQ_CHOICE(foreground_dfu_required);

	ret = ssf_client_send_request(&suit_srvc, &req, &rsp, NULL);
	if (ret != 0) {
		return SUIT_PLAT_ERR_IPC;
	}

	return rsp.SSF_SUIT_RSP(foreground_dfu_required)
		.SSF_SUIT_RSP_ARG(foreground_dfu_required, ret);
}
