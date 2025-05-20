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
LOG_MODULE_REGISTER(suit_srvc_mci, CONFIG_SSF_SUIT_SERVICE_LOG_LEVEL);

extern const struct ssf_client_srvc suit_srvc;

suit_ssf_err_t suit_get_supported_manifest_roles(suit_manifest_role_t *roles, size_t *size)
{
	int ret;
	struct suit_req req;
	struct suit_rsp rsp;
	struct suit_get_supported_manifest_roles_rsp *rsp_data;
	const uint8_t *rsp_pkt;

	if (roles == NULL || size == NULL) {
		return SUIT_PLAT_ERR_INVAL;
	}

	memset(&req, 0, sizeof(struct suit_req));
	req.suit_req_msg_choice = SSF_SUIT_REQ_CHOICE(get_supported_manifest_roles);

	ret = ssf_client_send_request(&suit_srvc, &req, &rsp, &rsp_pkt);
	if (ret != 0) {
		ssf_client_decode_done(rsp_pkt);
		return SUIT_PLAT_ERR_IPC;
	}

	rsp_data = &rsp.SSF_SUIT_RSP(get_supported_manifest_roles);
	ret = rsp_data->SSF_SUIT_RSP_ARG(get_supported_manifest_roles, ret);
	if (ret != SUIT_PLAT_SUCCESS) {
		ssf_client_decode_done(rsp_pkt);
		return ret;
	}

	const size_t cnt =
		rsp_data->SSF_SUIT_RSP_ARG(get_supported_manifest_roles, roles_int_count);
	uint32_t *roles_array = rsp_data->SSF_SUIT_RSP_ARG(get_supported_manifest_roles, roles_int);

	if (cnt > *size) {
		ssf_client_decode_done(rsp_pkt);
		return SUIT_PLAT_ERR_SIZE;
	}

	for (size_t i = 0; i < cnt; i++) {
		/* Manifest role encoded as two nibbles: <domain ID> <index>,
		 * so it cannot exceed 0xFF
		 */
		if (roles_array[i] > 0xFF) {
			ssf_client_decode_done(rsp_pkt);
			return SUIT_PLAT_ERR_CBOR_DECODING;
		}

		roles[i] = (suit_manifest_role_t)roles_array[i];
	}

	*size = cnt;
	ssf_client_decode_done(rsp_pkt);

	return ret;
}

suit_ssf_err_t suit_get_supported_manifest_info(suit_manifest_role_t role,
						suit_ssf_manifest_class_info_t *class_info)
{
	int ret;
	struct suit_req req;
	struct suit_rsp rsp;
	struct suit_get_supported_manifest_info_req *req_data;
	struct suit_get_supported_manifest_info_rsp *rsp_data;
	const uint8_t *rsp_pkt;

	if (class_info == NULL) {
		return SUIT_PLAT_ERR_INVAL;
	}

	memset(&req, 0, sizeof(struct suit_req));
	req.suit_req_msg_choice = SSF_SUIT_REQ_CHOICE(get_supported_manifest_info);

	req_data = &req.SSF_SUIT_REQ(get_supported_manifest_info);
	req_data->SSF_SUIT_REQ_ARG(get_supported_manifest_info, role) = role;

	ret = ssf_client_send_request(&suit_srvc, &req, &rsp, &rsp_pkt);
	if (ret != 0) {
		ssf_client_decode_done(rsp_pkt);
		return SUIT_PLAT_ERR_IPC;
	}

	rsp_data = &rsp.SSF_SUIT_RSP(get_supported_manifest_info);
	ret = rsp_data->SSF_SUIT_RSP_ARG(get_supported_manifest_info, ret);
	if (ret != SUIT_PLAT_SUCCESS) {
		ssf_client_decode_done(rsp_pkt);
		return ret;
	}

	if (rsp_data->SSF_SUIT_RSP_ARG(get_supported_manifest_info, role) != role) {
		ssf_client_decode_done(rsp_pkt);
		return SUIT_PLAT_ERR_CBOR_DECODING;
	}

	struct zcbor_string *class_id_str =
		&rsp_data->SSF_SUIT_RSP_ARG(get_supported_manifest_info, class_id);
	if (class_id_str->len != sizeof(class_info->class_id)) {
		ssf_client_decode_done(rsp_pkt);
		return SUIT_PLAT_ERR_CBOR_DECODING;
	}

	struct zcbor_string *vendor_id_str =
		&rsp_data->SSF_SUIT_RSP_ARG(get_supported_manifest_info, vendor_id);
	if (vendor_id_str->len != sizeof(class_info->vendor_id)) {
		ssf_client_decode_done(rsp_pkt);
		return SUIT_PLAT_ERR_CBOR_DECODING;
	}

	int32_t downgrade_prevention_policy = rsp_data->SSF_SUIT_RSP_ARG(
		get_supported_manifest_info, downgrade_prevention_policy);

	if (downgrade_prevention_policy != SUIT_DOWNGRADE_PREVENTION_DISABLED &&
	    downgrade_prevention_policy != SUIT_DOWNGRADE_PREVENTION_ENABLED) {
		ssf_client_decode_done(rsp_pkt);
		return SUIT_PLAT_ERR_CBOR_DECODING;
	}

	int32_t independent_updateability_policy = rsp_data->SSF_SUIT_RSP_ARG(
		get_supported_manifest_info, independent_updateability_policy);
	if (independent_updateability_policy != SUIT_INDEPENDENT_UPDATE_DENIED &&
	    independent_updateability_policy != SUIT_INDEPENDENT_UPDATE_ALLOWED) {
		ssf_client_decode_done(rsp_pkt);
		return SUIT_PLAT_ERR_CBOR_DECODING;
	}

	int32_t signature_verification_policy = rsp_data->SSF_SUIT_RSP_ARG(
		get_supported_manifest_info, signature_verification_policy);
	if (signature_verification_policy != SUIT_SIGNATURE_CHECK_DISABLED &&
	    signature_verification_policy != SUIT_SIGNATURE_CHECK_ENABLED_ON_UPDATE &&
	    signature_verification_policy != SUIT_SIGNATURE_CHECK_ENABLED_ON_UPDATE_AND_BOOT) {
		ssf_client_decode_done(rsp_pkt);
		return SUIT_PLAT_ERR_CBOR_DECODING;
	}

	memcpy(&class_info->class_id, class_id_str->value, class_id_str->len);
	memcpy(&class_info->vendor_id, vendor_id_str->value, vendor_id_str->len);
	class_info->role =
		(suit_manifest_role_t)rsp_data->SSF_SUIT_RSP_ARG(get_supported_manifest_info, role);
	class_info->downgrade_prevention_policy =
		(suit_downgrade_prevention_policy_t)downgrade_prevention_policy;
	class_info->independent_updateability_policy =
		(suit_independent_updateability_policy_t)independent_updateability_policy;
	class_info->signature_verification_policy =
		(suit_signature_verification_policy_t)signature_verification_policy;

	ssf_client_decode_done(rsp_pkt);

	return ret;
}
