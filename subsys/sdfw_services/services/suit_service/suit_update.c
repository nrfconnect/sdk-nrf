/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zcbor_common.h>
#include <sdfw/sdfw_services/suit_service.h>

#include <sdfw/sdfw_services/ssf_client.h>
#include <sdfw/sdfw_services/ssf_client_notif.h>
#include "suit_service_decode.h"
#include "suit_service_encode.h"
#include "suit_service_types.h"
#include "suit_service_utils.h"

#include <errno.h>
#include <zephyr/sys/util.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(suit_srvc_update, CONFIG_SSF_SUIT_SERVICE_LOG_LEVEL);

extern const struct ssf_client_srvc suit_srvc;

suit_ssf_err_t suit_trigger_update(suit_plat_mreg_t *regions, size_t len)
{
	int ret;
	struct suit_req req;
	struct suit_rsp rsp;
	struct suit_trigger_update_req *req_data;

	if (regions == NULL) {
		return SUIT_PLAT_ERR_INVAL;
	}

	if (len == 0) {
		return SUIT_PLAT_ERR_INVAL;
	}

	memset(&req, 0, sizeof(struct suit_req));
	req.suit_req_msg_choice = SSF_SUIT_REQ_CHOICE(trigger_update);
	req_data = &req.SSF_SUIT_REQ(trigger_update);
	const size_t cache_count = len - 1;

	/* The update candidate consists of two things:
	 *  - The SUIT envelope, describing the update and boot process.
	 *  - A list of payloads, that were fetched in addition to the envelope,
	 *    as a result of payload-fetch SUIT manifest sequence execution.
	 *    The payloads are passed as an array of SUIT cache structures.
	 */
	if (cache_count > ARRAY_SIZE(req_data->SSF_SUIT_REQ_ARG(trigger_update,
								caches_suit_cache_info_entry_m))) {
		return SUIT_PLAT_ERR_NOMEM;
	}

	req_data->SSF_SUIT_REQ_ARG(trigger_update, addr) = (uintptr_t)regions[0].mem;
	req_data->SSF_SUIT_REQ_ARG(trigger_update, size) = regions[0].size;
	req_data->SSF_SUIT_REQ_ARG(trigger_update, caches_suit_cache_info_entry_m_count) =
		cache_count;
	struct suit_cache_info_entry *cache_entries =
		(struct suit_cache_info_entry *)&req_data->SSF_SUIT_REQ_ARG(
			trigger_update, caches_suit_cache_info_entry_m);

	for (size_t i = 0; i < cache_count; i++) {
		/* Skip regions[0] as it contains the SUIT envelope.
		 * SUIT caches starts from index 1.
		 */
		cache_entries[i].suit_cache_info_entry_addr = (uintptr_t)regions[i + 1].mem;
		cache_entries[i].suit_cache_info_entry_size = regions[i + 1].size;
	}

	ret = ssf_client_send_request(&suit_srvc, &req, &rsp, NULL);
	if (ret != 0) {
		return SUIT_PLAT_ERR_IPC;
	}

	return rsp.SSF_SUIT_RSP(trigger_update).SSF_SUIT_RSP_ARG(trigger_update, ret);
}

suit_ssf_err_t suit_check_installed_component_digest(suit_plat_mreg_t *component_id, int alg_id,
						     suit_plat_mreg_t *digest)
{
	int ret;
	struct suit_req req;
	struct suit_rsp rsp;
	struct suit_check_installed_component_digest_req *req_data;

	if (component_id == NULL || digest == NULL) {
		return SUIT_PLAT_ERR_INVAL;
	}

	memset(&req, 0, sizeof(struct suit_req));
	req.suit_req_msg_choice = SSF_SUIT_REQ_CHOICE(check_installed_component_digest);
	req_data = &req.SSF_SUIT_REQ(check_installed_component_digest);
	req_data->SSF_SUIT_REQ_ARG(check_installed_component_digest, component_id).value =
		component_id->mem;
	req_data->SSF_SUIT_REQ_ARG(check_installed_component_digest, component_id).len =
		component_id->size;
	req_data->SSF_SUIT_REQ_ARG(check_installed_component_digest, alg_id) = alg_id;
	req_data->SSF_SUIT_REQ_ARG(check_installed_component_digest, digest).value = digest->mem;
	req_data->SSF_SUIT_REQ_ARG(check_installed_component_digest, digest).len = digest->size;

	ret = ssf_client_send_request(&suit_srvc, &req, &rsp, NULL);
	if (ret != 0) {
		return SUIT_PLAT_ERR_IPC;
	}

	return rsp.SSF_SUIT_RSP(check_installed_component_digest)
		.SSF_SUIT_RSP_ARG(check_installed_component_digest, ret);
}

suit_ssf_err_t suit_get_installed_manifest_info(suit_manifest_class_id_t *manifest_class_id,
						unsigned int *seq_num, suit_semver_raw_t *version,
						suit_digest_status_t *status, int *alg_id,
						suit_plat_mreg_t *digest)
{
	int ret;
	struct suit_req req;
	struct suit_rsp rsp;
	struct suit_get_installed_manifest_info_rsp *rsp_data;
	const uint8_t *rsp_pkt;
	struct suit_get_installed_manifest_info_req *req_data;

	if (seq_num == NULL || manifest_class_id == NULL) {
		return SUIT_PLAT_ERR_INVAL;
	}

	if ((digest != NULL) || (alg_id != NULL)) {
		if (alg_id == NULL || digest == NULL || digest->mem == NULL) {
			return SUIT_PLAT_ERR_INVAL;
		}
	}

	memset(&req, 0, sizeof(struct suit_req));
	req.suit_req_msg_choice = SSF_SUIT_REQ_CHOICE(get_installed_manifest_info);
	req_data = &req.SSF_SUIT_REQ(get_installed_manifest_info);
	req_data->SSF_SUIT_REQ_ARG(get_installed_manifest_info, manifest_class_id).value =
		(uint8_t *)manifest_class_id;
	req_data->SSF_SUIT_REQ_ARG(get_installed_manifest_info, manifest_class_id).len =
		sizeof(suit_manifest_class_id_t);

	ret = ssf_client_send_request(&suit_srvc, &req, &rsp, &rsp_pkt);
	if (ret != 0) {
		ssf_client_decode_done(rsp_pkt);
		return SUIT_PLAT_ERR_IPC;
	}

	rsp_data = &rsp.SSF_SUIT_RSP(get_installed_manifest_info);
	ret = rsp_data->SSF_SUIT_RSP_ARG(get_installed_manifest_info, ret);
	if (ret != SUIT_PLAT_SUCCESS) {
		ssf_client_decode_done(rsp_pkt);
		return ret;
	}

	*seq_num = rsp_data->SSF_SUIT_RSP_ARG(get_installed_manifest_info, seq_num);

	if (version != NULL) {
		if (rsp_data->SSF_SUIT_RSP_ARG(get_installed_manifest_info, semver_int_count) >
		    ARRAY_SIZE(version->raw)) {
			ssf_client_decode_done(rsp_pkt);
			return SUIT_PLAT_ERR_SIZE;
		}

		version->len =
			rsp_data->SSF_SUIT_RSP_ARG(get_installed_manifest_info, semver_int_count);
		for (size_t i = 0; i < version->len; i++) {
			version->raw[i] = rsp_data->SSF_SUIT_RSP_ARG(get_installed_manifest_info,
								     semver_int)[i];
		}
	}

	if (status != NULL) {
		*status = (suit_digest_status_t)rsp_data->SSF_SUIT_RSP_ARG(
			get_installed_manifest_info, digest_status);
	}

	if ((alg_id != NULL) && (digest != NULL)) {
		const size_t manifest_digest_len =
			rsp_data->SSF_SUIT_RSP_ARG(get_installed_manifest_info, digest).len;
		if (manifest_digest_len > digest->size) {
			ssf_client_decode_done(rsp_pkt);
			return SUIT_PLAT_ERR_NOMEM;
		}

		*alg_id = rsp_data->SSF_SUIT_RSP_ARG(get_installed_manifest_info, alg_id);
		memcpy((uint8_t *)digest->mem,
		       rsp_data->SSF_SUIT_RSP_ARG(get_installed_manifest_info, digest).value,
		       manifest_digest_len);
		digest->size = manifest_digest_len;
	}

	ssf_client_decode_done(rsp_pkt);
	return ret;
}

suit_ssf_err_t suit_get_install_candidate_info(suit_manifest_class_id_t *manifest_class_id,
					       unsigned int *seq_num, suit_semver_raw_t *version,
					       int *alg_id, suit_plat_mreg_t *digest)
{
	int ret;
	struct suit_req req;
	struct suit_rsp rsp;
	struct suit_get_install_candidate_info_rsp *rsp_data;
	const uint8_t *rsp_pkt;

	if (seq_num == NULL || manifest_class_id == NULL) {
		return SUIT_PLAT_ERR_INVAL;
	}

	if ((digest != NULL) || (alg_id != NULL)) {
		if (alg_id == NULL || digest == NULL || digest->mem == NULL) {
			return SUIT_PLAT_ERR_INVAL;
		}
	}

	memset(&req, 0, sizeof(struct suit_req));
	req.suit_req_msg_choice = SSF_SUIT_REQ_CHOICE(get_install_candidate_info);

	ret = ssf_client_send_request(&suit_srvc, &req, &rsp, &rsp_pkt);
	if (ret != 0) {
		ssf_client_decode_done(rsp_pkt);
		return SUIT_PLAT_ERR_IPC;
	}

	rsp_data = &rsp.SSF_SUIT_RSP(get_install_candidate_info);
	ret = rsp_data->SSF_SUIT_RSP_ARG(get_install_candidate_info, ret);
	if (ret != SUIT_PLAT_SUCCESS) {
		ssf_client_decode_done(rsp_pkt);
		return ret;
	}

	const size_t manifest_digest_len =
		rsp_data->SSF_SUIT_RSP_ARG(get_install_candidate_info, digest).len;
	*seq_num = rsp_data->SSF_SUIT_RSP_ARG(get_install_candidate_info, seq_num);
	if (manifest_digest_len > digest->size) {
		ssf_client_decode_done(rsp_pkt);
		return SUIT_PLAT_ERR_NOMEM;
	}

	if (version != NULL) {
		if (rsp_data->SSF_SUIT_RSP_ARG(get_install_candidate_info, semver_int_count) >
		    ARRAY_SIZE(version->raw)) {
			ssf_client_decode_done(rsp_pkt);
			return SUIT_PLAT_ERR_SIZE;
		}

		version->len =
			rsp_data->SSF_SUIT_RSP_ARG(get_install_candidate_info, semver_int_count);
		for (size_t i = 0; i < version->len; i++) {
			version->raw[i] = rsp_data->SSF_SUIT_RSP_ARG(get_install_candidate_info,
								     semver_int)[i];
		}
	}

	if ((alg_id != NULL) && (digest != NULL)) {
		*alg_id = rsp_data->SSF_SUIT_RSP_ARG(get_install_candidate_info, alg_id),
		memcpy((uint8_t *)digest->mem,
		       rsp_data->SSF_SUIT_RSP_ARG(get_install_candidate_info, digest).value,
		       manifest_digest_len);
		digest->size = manifest_digest_len;
	}

	const size_t manifest_class_id_len =
		rsp_data->SSF_SUIT_RSP_ARG(get_install_candidate_info, manifest_class_id).len;
	if (manifest_class_id_len != sizeof(suit_manifest_class_id_t)) {
		ssf_client_decode_done(rsp_pkt);
		return SUIT_PLAT_ERR_NOMEM;
	}

	memcpy(manifest_class_id,
	       rsp_data->SSF_SUIT_RSP_ARG(get_install_candidate_info, manifest_class_id).value,
	       manifest_class_id_len);

	ssf_client_decode_done(rsp_pkt);
	return ret;
}
