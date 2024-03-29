/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <suit_platform.h>
#include <suit_plat_component_compatibility.h>
#include <suit_plat_decode_util.h>

#include <sdfw/sdfw_services/ssf_client.h>
#include "suit_service_types.h"
#include "suit_service_utils.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(suit_srvc_auth, CONFIG_SSF_SUIT_SERVICE_LOG_LEVEL);

extern const struct ssf_client_srvc suit_srvc;

int suit_plat_authenticate_manifest(struct zcbor_string *manifest_component_id,
				    enum suit_cose_alg alg_id, struct zcbor_string *key_id,
				    struct zcbor_string *signature, struct zcbor_string *data)
{
	int ret;
	struct suit_req req;
	struct suit_rsp rsp;
	struct suit_authenticate_manifest_req *req_data;

	if (manifest_component_id == NULL || key_id == NULL || signature == NULL || data == NULL) {
		return SUIT_ERR_DECODING;
	}

	memset(&req, 0, sizeof(struct suit_req));
	req.suit_req_msg_choice = SSF_SUIT_REQ_CHOICE(authenticate_manifest);

	req_data = &req.SSF_SUIT_REQ(authenticate_manifest);
	req_data->SSF_SUIT_REQ_ARG(authenticate_manifest, manifest_component_id) =
		*manifest_component_id;
	req_data->SSF_SUIT_REQ_ARG(authenticate_manifest, alg_id) = alg_id;
	req_data->SSF_SUIT_REQ_ARG(authenticate_manifest, key_id) = *key_id;
	req_data->SSF_SUIT_REQ_ARG(authenticate_manifest, signature) = *signature;
	req_data->SSF_SUIT_REQ_ARG(authenticate_manifest, data_addr) = (uintptr_t)data->value;
	req_data->SSF_SUIT_REQ_ARG(authenticate_manifest, data_size) = data->len;

	ret = ssf_client_send_request(&suit_srvc, &req, &rsp, NULL);
	if (ret != 0) {
		return SUIT_ERR_CRASH;
	}

	struct suit_authenticate_manifest_rsp *rsp_data = &rsp.SSF_SUIT_RSP(authenticate_manifest);

	return rsp_data->SSF_SUIT_RSP_ARG(authenticate_manifest, ret);
}

int suit_plat_authorize_unsigned_manifest(struct zcbor_string *manifest_component_id)
{
	int ret;
	struct suit_req req;
	struct suit_rsp rsp;
	struct suit_authorize_unsigned_manifest_req *req_data;

	if (manifest_component_id == NULL) {
		return SUIT_ERR_DECODING;
	}

	memset(&req, 0, sizeof(struct suit_req));
	req.suit_req_msg_choice = SSF_SUIT_REQ_CHOICE(authorize_unsigned_manifest);

	req_data = &req.SSF_SUIT_REQ(authorize_unsigned_manifest);
	req_data->SSF_SUIT_REQ_ARG(authorize_unsigned_manifest, manifest_component_id) =
		*manifest_component_id;

	ret = ssf_client_send_request(&suit_srvc, &req, &rsp, NULL);
	if (ret != 0) {
		return SUIT_ERR_CRASH;
	}

	return rsp.SSF_SUIT_RSP(authorize_unsigned_manifest)
		.SSF_SUIT_RSP_ARG(authorize_unsigned_manifest, ret);
}

int suit_plat_component_compatibility_check(const suit_manifest_class_id_t *class_id,
					    struct zcbor_string *component_id)
{
	int ret;
	struct suit_req req;
	struct suit_rsp rsp;
	struct suit_check_component_compatibility_req *req_data;

	if (class_id == NULL || component_id == NULL) {
		return SUIT_ERR_DECODING;
	}

	memset(&req, 0, sizeof(struct suit_req));
	req.suit_req_msg_choice = SSF_SUIT_REQ_CHOICE(check_component_compatibility);

	req_data = &req.SSF_SUIT_REQ(check_component_compatibility);
	req_data->SSF_SUIT_REQ_ARG(check_component_compatibility, manifest_class_id).value =
		(const uint8_t *)class_id;
	req_data->SSF_SUIT_REQ_ARG(check_component_compatibility, manifest_class_id).len =
		sizeof(*class_id);
	req_data->SSF_SUIT_REQ_ARG(check_component_compatibility, component_id) = *component_id;

	ret = ssf_client_send_request(&suit_srvc, &req, &rsp, NULL);
	if (ret != 0) {
		return SUIT_ERR_CRASH;
	}

	return rsp.SSF_SUIT_RSP(check_component_compatibility)
		.SSF_SUIT_RSP_ARG(check_component_compatibility, ret);
}

int suit_plat_authorize_sequence_num(enum suit_command_sequence seq_name,
				     struct zcbor_string *manifest_component_id,
				     unsigned int seq_num)
{
	int ret;
	struct suit_req req;
	struct suit_rsp rsp;
	struct suit_authorize_seq_num_req *req_data;

	if (manifest_component_id == NULL) {
		return SUIT_ERR_DECODING;
	}

	memset(&req, 0, sizeof(struct suit_req));
	req.suit_req_msg_choice = SSF_SUIT_REQ_CHOICE(authorize_seq_num);

	req_data = &req.SSF_SUIT_REQ(authorize_seq_num);
	req_data->SSF_SUIT_REQ_ARG(authorize_seq_num, manifest_component_id) =
		*manifest_component_id;
	req_data->SSF_SUIT_REQ_ARG(authorize_seq_num, seq_num) = seq_num;
	req_data->SSF_SUIT_REQ_ARG(authorize_seq_num, command_seq) = (uint32_t)seq_name;

	ret = ssf_client_send_request(&suit_srvc, &req, &rsp, NULL);
	if (ret != 0) {
		return SUIT_ERR_CRASH;
	}

	return rsp.SSF_SUIT_RSP(authorize_seq_num).SSF_SUIT_RSP_ARG(authorize_seq_num, ret);
}
