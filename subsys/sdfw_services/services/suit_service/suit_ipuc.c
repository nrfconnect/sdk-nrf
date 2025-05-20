/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <suit_plat_err.h>
#include <suit_plat_decode_util.h>
#include <suit_ipuc.h>
#include <stdint.h>
#include <sdfw/sdfw_services/suit_service.h>
#include <sdfw/sdfw_services/ssf_client.h>
#include "suit_service_decode.h"
#include "suit_service_encode.h"
#include "suit_service_types.h"
#include "suit_service_utils.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(suit_ipuc, CONFIG_SSF_SUIT_SERVICE_LOG_LEVEL);

extern const struct ssf_client_srvc suit_srvc;

suit_plat_err_t suit_ipuc_get_count(size_t *count)
{
	int err;
	struct suit_req req = {0};
	struct suit_rsp rsp = {0};
	struct suit_get_ipuc_count_rsp *rsp_data = NULL;
	const uint8_t *rsp_pkt = NULL;

	if (count == NULL) {
		return SUIT_PLAT_ERR_INVAL;
	}

	req.suit_req_msg_choice = SSF_SUIT_REQ_CHOICE(get_ipuc_count);
	err = ssf_client_send_request(&suit_srvc, &req, &rsp, &rsp_pkt);
	if (err != 0) {
		LOG_ERR("ssf_client_send_request failed");
		return SUIT_PLAT_ERR_IPC;
	}

	rsp_data = &rsp.SSF_SUIT_RSP(get_ipuc_count);
	err = rsp_data->SSF_SUIT_RSP_ARG(get_ipuc_count, ret);
	if (err != SUIT_PLAT_SUCCESS) {
		ssf_client_decode_done(rsp_pkt);
		LOG_ERR("ssf_client response return code: %d", err);
		return err;
	}

	*count = rsp_data->SSF_SUIT_RSP_ARG(get_ipuc_count, count);

	ssf_client_decode_done(rsp_pkt);
	return err;
}

suit_plat_err_t suit_ipuc_get_info(size_t idx, struct zcbor_string *component_id,
				   suit_manifest_role_t *role)
{
	int err;
	struct suit_req req = {0};
	struct suit_rsp rsp = {0};
	struct suit_get_ipuc_info_req *req_data = NULL;
	struct suit_get_ipuc_info_rsp *rsp_data = NULL;
	const uint8_t *rsp_pkt = NULL;

	if (component_id == NULL || role == NULL) {
		return SUIT_PLAT_ERR_INVAL;
	}

	if (component_id->value == NULL) {
		return SUIT_PLAT_ERR_INVAL;
	}

	req.suit_req_msg_choice = SSF_SUIT_REQ_CHOICE(get_ipuc_info);
	req_data = &req.SSF_SUIT_REQ(get_ipuc_info);
	req_data->SSF_SUIT_REQ_ARG(get_ipuc_info, idx) = idx;

	err = ssf_client_send_request(&suit_srvc, &req, &rsp, &rsp_pkt);
	if (err != 0) {
		LOG_ERR("ssf_client_send_request failed");
		return SUIT_PLAT_ERR_IPC;
	}
	rsp_data = &rsp.SSF_SUIT_RSP(get_ipuc_info);
	err = rsp_data->SSF_SUIT_RSP_ARG(get_ipuc_info, ret);
	if (err != SUIT_PLAT_SUCCESS) {
		ssf_client_decode_done(rsp_pkt);
		LOG_ERR("ssf_client response return code: %d", err);
		return err;
	}

	struct zcbor_string *src_component_id =
		&rsp_data->SSF_SUIT_RSP_ARG(get_ipuc_info, component_id);

	if (component_id->len < src_component_id->len) {

		ssf_client_decode_done(rsp_pkt);
		return SUIT_PLAT_ERR_SIZE;
	}

	memcpy((void *)component_id->value, src_component_id->value, src_component_id->len);
	component_id->len = src_component_id->len;
	*role = rsp_data->SSF_SUIT_RSP_ARG(get_ipuc_info, role);
	ssf_client_decode_done(rsp_pkt);

#if defined(CONFIG_SSF_SUIT_SERVICE_LOG_LEVEL_INF) || defined(CONFIG_SSF_SUIT_SERVICE_LOG_LEVEL_DBG)
	suit_component_type_t component_type = SUIT_COMPONENT_TYPE_UNSUPPORTED;

	suit_plat_decode_component_type(component_id, &component_type);

	if (component_type == SUIT_COMPONENT_TYPE_MEM) {
		uint8_t core_id = 0;
		intptr_t run_address = 0;
		size_t slot_size = 0;

		suit_plat_decode_component_id(component_id, &core_id, &run_address, &slot_size);

		LOG_INF("MEM, %08X, %d bytes, core: %d, mfst role: 0x%02X",
			(unsigned int)run_address, slot_size, core_id, *role);

	} else {
		LOG_INF("component_type: %d, mfst role: 0x%02X", component_type, *role);
	}
#endif

	return err;
}

suit_plat_err_t suit_ipuc_write_setup(struct zcbor_string *component_id,
				      struct zcbor_string *encryption_info,
				      struct zcbor_string *compression_info)
{
	int err;
	struct suit_req req = {0};
	struct suit_rsp rsp = {0};
	struct suit_setup_write_ipuc_req *req_data = {0};

	if (component_id == NULL) {
		return SUIT_PLAT_ERR_INVAL;
	}

	if (encryption_info != NULL || compression_info != NULL) {
		/* Encryption and/or Compression are not supported yet
		 */
		return SUIT_PLAT_ERR_UNSUPPORTED;
	}

	req.suit_req_msg_choice = SSF_SUIT_REQ_CHOICE(setup_write_ipuc);
	req_data = &req.SSF_SUIT_REQ(setup_write_ipuc);

	req_data->SSF_SUIT_REQ_ARG(setup_write_ipuc, component_id).value = component_id->value;
	req_data->SSF_SUIT_REQ_ARG(setup_write_ipuc, component_id).len = component_id->len;
	req_data->SSF_SUIT_REQ_ARG(setup_write_ipuc, encryption_info).value = NULL;
	req_data->SSF_SUIT_REQ_ARG(setup_write_ipuc, encryption_info).len = 0;
	req_data->SSF_SUIT_REQ_ARG(setup_write_ipuc, compression_info).value = NULL;
	req_data->SSF_SUIT_REQ_ARG(setup_write_ipuc, compression_info).len = 0;

	err = ssf_client_send_request(&suit_srvc, &req, &rsp, NULL);
	if (err != 0) {
		LOG_ERR("ssf_client_send_request failed");
		return SUIT_PLAT_ERR_IPC;
	}

	return rsp.SSF_SUIT_RSP(setup_write_ipuc).SSF_SUIT_RSP_ARG(setup_write_ipuc, ret);
}

suit_plat_err_t suit_ipuc_write(struct zcbor_string *component_id, size_t offset, uintptr_t buffer,
				size_t chunk_size, bool last_chunk)
{
	int err;
	struct suit_req req = {0};
	struct suit_rsp rsp = {0};
	struct suit_write_ipuc_req *req_data = {0};

	if (component_id == NULL) {
		return SUIT_PLAT_ERR_INVAL;
	}

	req.suit_req_msg_choice = SSF_SUIT_REQ_CHOICE(write_ipuc);
	req_data = &req.SSF_SUIT_REQ(write_ipuc);

	req_data->SSF_SUIT_REQ_ARG(write_ipuc, component_id).value = component_id->value;
	req_data->SSF_SUIT_REQ_ARG(write_ipuc, component_id).len = component_id->len;
	req_data->SSF_SUIT_REQ_ARG(write_ipuc, offset) = offset;
	req_data->SSF_SUIT_REQ_ARG(write_ipuc, addr) = buffer;
	req_data->SSF_SUIT_REQ_ARG(write_ipuc, size) = chunk_size;
	req_data->SSF_SUIT_REQ_ARG(write_ipuc, last_chunk) = last_chunk;

	err = ssf_client_send_request(&suit_srvc, &req, &rsp, NULL);
	if (err != 0) {
		LOG_ERR("ssf_client_send_request failed");
		return SUIT_PLAT_ERR_IPC;
	}

	return rsp.SSF_SUIT_RSP(write_ipuc).SSF_SUIT_RSP_ARG(write_ipuc, ret);
}
