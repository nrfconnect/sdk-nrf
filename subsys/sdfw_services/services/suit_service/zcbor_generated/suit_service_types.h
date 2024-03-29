/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 * Generated using zcbor version 0.8.1
 * https://github.com/NordicSemiconductor/zcbor
 * Generated with a --default-max-qty of 3
 */

#ifndef SUIT_SERVICE_TYPES_H__
#define SUIT_SERVICE_TYPES_H__

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <zcbor_common.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Which value for --default-max-qty this file was created with.
 *
 *  The define is used in the other generated file to do a build-time
 *  compatibility check.
 *
 *  See `zcbor --help` for more information about --default-max-qty
 */
#define DEFAULT_MAX_QTY 3

struct suit_missing_image_evt_nfy {
	struct zcbor_string suit_missing_image_evt_nfy_resource_id;
	uint32_t suit_missing_image_evt_nfy_stream_session_id;
};

struct suit_chunk_status_evt_nfy {
	uint32_t suit_chunk_status_evt_nfy_stream_session_id;
};

struct suit_nfy {
	union {
		struct suit_missing_image_evt_nfy suit_nfy_msg_suit_missing_image_evt_nfy_m;
		struct suit_chunk_status_evt_nfy suit_nfy_msg_suit_chunk_status_evt_nfy_m;
	};
	enum {
		suit_nfy_msg_suit_missing_image_evt_nfy_m_c,
		suit_nfy_msg_suit_chunk_status_evt_nfy_m_c,
	} suit_nfy_msg_choice;
};

struct suit_cache_info_entry {
	uint32_t suit_cache_info_entry_addr;
	uint32_t suit_cache_info_entry_size;
};

struct suit_trigger_update_req {
	uint32_t suit_trigger_update_req_addr;
	uint32_t suit_trigger_update_req_size;
	struct suit_cache_info_entry suit_trigger_update_req_caches_suit_cache_info_entry_m[6];
	size_t suit_trigger_update_req_caches_suit_cache_info_entry_m_count;
};

struct suit_check_installed_component_digest_req {
	struct zcbor_string suit_check_installed_component_digest_req_component_id;
	int32_t suit_check_installed_component_digest_req_alg_id;
	struct zcbor_string suit_check_installed_component_digest_req_digest;
};

struct suit_get_installed_manifest_info_req {
	struct zcbor_string suit_get_installed_manifest_info_req_manifest_class_id;
};

struct suit_authenticate_manifest_req {
	struct zcbor_string suit_authenticate_manifest_req_manifest_component_id;
	uint32_t suit_authenticate_manifest_req_alg_id;
	struct zcbor_string suit_authenticate_manifest_req_key_id;
	struct zcbor_string suit_authenticate_manifest_req_signature;
	uint32_t suit_authenticate_manifest_req_data_addr;
	uint32_t suit_authenticate_manifest_req_data_size;
};

struct suit_authorize_unsigned_manifest_req {
	struct zcbor_string suit_authorize_unsigned_manifest_req_manifest_component_id;
};

struct suit_authorize_seq_num_req {
	struct zcbor_string suit_authorize_seq_num_req_manifest_component_id;
	uint32_t suit_authorize_seq_num_req_command_seq;
	uint32_t suit_authorize_seq_num_req_seq_num;
};

struct suit_check_component_compatibility_req {
	struct zcbor_string suit_check_component_compatibility_req_manifest_class_id;
	struct zcbor_string suit_check_component_compatibility_req_component_id;
};

struct suit_get_supported_manifest_info_req {
	int32_t suit_get_supported_manifest_info_req_role;
};

struct suit_authorize_process_dependency_req {
	struct zcbor_string suit_authorize_process_dependency_req_dependee_class_id;
	struct zcbor_string suit_authorize_process_dependency_req_dependent_class_id;
	int32_t suit_authorize_process_dependency_req_seq_id;
};

struct suit_evt_sub_req {
	bool suit_evt_sub_req_subscribe;
};

struct suit_chunk_enqueue_req {
	uint32_t suit_chunk_enqueue_req_stream_session_id;
	uint32_t suit_chunk_enqueue_req_chunk_id;
	uint32_t suit_chunk_enqueue_req_offset;
	bool suit_chunk_enqueue_req_last_chunk;
	uint32_t suit_chunk_enqueue_req_addr;
	uint32_t suit_chunk_enqueue_req_size;
};

struct suit_chunk_status_req {
	uint32_t suit_chunk_status_req_stream_session_id;
};

struct suit_req {
	union {
		struct suit_trigger_update_req suit_req_msg_suit_trigger_update_req_m;
		struct suit_check_installed_component_digest_req
			suit_req_msg_suit_check_installed_component_digest_req_m;
		struct suit_get_installed_manifest_info_req
			suit_req_msg_suit_get_installed_manifest_info_req_m;
		struct suit_authenticate_manifest_req suit_req_msg_suit_authenticate_manifest_req_m;
		struct suit_authorize_unsigned_manifest_req
			suit_req_msg_suit_authorize_unsigned_manifest_req_m;
		struct suit_authorize_seq_num_req suit_req_msg_suit_authorize_seq_num_req_m;
		struct suit_check_component_compatibility_req
			suit_req_msg_suit_check_component_compatibility_req_m;
		struct suit_get_supported_manifest_info_req
			suit_req_msg_suit_get_supported_manifest_info_req_m;
		struct suit_authorize_process_dependency_req
			suit_req_msg_suit_authorize_process_dependency_req_m;
		struct suit_evt_sub_req suit_req_msg_suit_evt_sub_req_m;
		struct suit_chunk_enqueue_req suit_req_msg_suit_chunk_enqueue_req_m;
		struct suit_chunk_status_req suit_req_msg_suit_chunk_status_req_m;
	};
	enum {
		suit_req_msg_suit_trigger_update_req_m_c,
		suit_req_msg_suit_check_installed_component_digest_req_m_c,
		suit_req_msg_suit_get_installed_manifest_info_req_m_c,
		suit_req_msg_suit_get_install_candidate_info_req_m_c,
		suit_req_msg_suit_authenticate_manifest_req_m_c,
		suit_req_msg_suit_authorize_unsigned_manifest_req_m_c,
		suit_req_msg_suit_authorize_seq_num_req_m_c,
		suit_req_msg_suit_check_component_compatibility_req_m_c,
		suit_req_msg_suit_get_supported_manifest_roles_req_m_c,
		suit_req_msg_suit_get_supported_manifest_info_req_m_c,
		suit_req_msg_suit_authorize_process_dependency_req_m_c,
		suit_req_msg_suit_evt_sub_req_m_c,
		suit_req_msg_suit_chunk_enqueue_req_m_c,
		suit_req_msg_suit_chunk_status_req_m_c,
	} suit_req_msg_choice;
};

struct suit_trigger_update_rsp {
	int32_t suit_trigger_update_rsp_ret;
};

struct suit_check_installed_component_digest_rsp {
	int32_t suit_check_installed_component_digest_rsp_ret;
};

struct suit_get_installed_manifest_info_rsp {
	int32_t suit_get_installed_manifest_info_rsp_ret;
	uint32_t suit_get_installed_manifest_info_rsp_seq_num;
	int32_t suit_get_installed_manifest_info_rsp_semver_int[5];
	size_t suit_get_installed_manifest_info_rsp_semver_int_count;
	int32_t suit_get_installed_manifest_info_rsp_digest_status;
	int32_t suit_get_installed_manifest_info_rsp_alg_id;
	struct zcbor_string suit_get_installed_manifest_info_rsp_digest;
};

struct suit_get_install_candidate_info_rsp {
	int32_t suit_get_install_candidate_info_rsp_ret;
	struct zcbor_string suit_get_install_candidate_info_rsp_manifest_class_id;
	uint32_t suit_get_install_candidate_info_rsp_seq_num;
	int32_t suit_get_install_candidate_info_rsp_semver_int[5];
	size_t suit_get_install_candidate_info_rsp_semver_int_count;
	int32_t suit_get_install_candidate_info_rsp_alg_id;
	struct zcbor_string suit_get_install_candidate_info_rsp_digest;
};

struct suit_authenticate_manifest_rsp {
	int32_t suit_authenticate_manifest_rsp_ret;
};

struct suit_authorize_unsigned_manifest_rsp {
	int32_t suit_authorize_unsigned_manifest_rsp_ret;
};

struct suit_authorize_seq_num_rsp {
	int32_t suit_authorize_seq_num_rsp_ret;
};

struct suit_check_component_compatibility_rsp {
	int32_t suit_check_component_compatibility_rsp_ret;
};

struct suit_get_supported_manifest_roles_rsp {
	int32_t suit_get_supported_manifest_roles_rsp_ret;
	int32_t suit_get_supported_manifest_roles_rsp_roles_int[20];
	size_t suit_get_supported_manifest_roles_rsp_roles_int_count;
};

struct suit_get_supported_manifest_info_rsp {
	int32_t suit_get_supported_manifest_info_rsp_ret;
	int32_t suit_get_supported_manifest_info_rsp_role;
	struct zcbor_string suit_get_supported_manifest_info_rsp_vendor_id;
	struct zcbor_string suit_get_supported_manifest_info_rsp_class_id;
	int32_t suit_get_supported_manifest_info_rsp_downgrade_prevention_policy;
	int32_t suit_get_supported_manifest_info_rsp_independent_updateability_policy;
	int32_t suit_get_supported_manifest_info_rsp_signature_verification_policy;
};

struct suit_authorize_process_dependency_rsp {
	int32_t suit_authorize_process_dependency_rsp_ret;
};

struct suit_evt_sub_rsp {
	int32_t suit_evt_sub_rsp_ret;
};

struct suit_chunk_enqueue_rsp {
	int32_t suit_chunk_enqueue_rsp_ret;
};

struct suit_chunk_info_entry {
	uint32_t suit_chunk_info_entry_chunk_id;
	uint32_t suit_chunk_info_entry_status;
};

struct suit_chunk_status_rsp {
	int32_t suit_chunk_status_rsp_ret;
	struct suit_chunk_info_entry suit_chunk_status_rsp_chunk_info_suit_chunk_info_entry_m[3];
	size_t suit_chunk_status_rsp_chunk_info_suit_chunk_info_entry_m_count;
};

struct suit_rsp {
	union {
		struct suit_trigger_update_rsp suit_rsp_msg_suit_trigger_update_rsp_m;
		struct suit_check_installed_component_digest_rsp
			suit_rsp_msg_suit_check_installed_component_digest_rsp_m;
		struct suit_get_installed_manifest_info_rsp
			suit_rsp_msg_suit_get_installed_manifest_info_rsp_m;
		struct suit_get_install_candidate_info_rsp
			suit_rsp_msg_suit_get_install_candidate_info_rsp_m;
		struct suit_authenticate_manifest_rsp suit_rsp_msg_suit_authenticate_manifest_rsp_m;
		struct suit_authorize_unsigned_manifest_rsp
			suit_rsp_msg_suit_authorize_unsigned_manifest_rsp_m;
		struct suit_authorize_seq_num_rsp suit_rsp_msg_suit_authorize_seq_num_rsp_m;
		struct suit_check_component_compatibility_rsp
			suit_rsp_msg_suit_check_component_compatibility_rsp_m;
		struct suit_get_supported_manifest_roles_rsp
			suit_rsp_msg_suit_get_supported_manifest_roles_rsp_m;
		struct suit_get_supported_manifest_info_rsp
			suit_rsp_msg_suit_get_supported_manifest_info_rsp_m;
		struct suit_authorize_process_dependency_rsp
			suit_rsp_msg_suit_authorize_process_dependency_rsp_m;
		struct suit_evt_sub_rsp suit_rsp_msg_suit_evt_sub_rsp_m;
		struct suit_chunk_enqueue_rsp suit_rsp_msg_suit_chunk_enqueue_rsp_m;
		struct suit_chunk_status_rsp suit_rsp_msg_suit_chunk_status_rsp_m;
	};
	enum {
		suit_rsp_msg_suit_trigger_update_rsp_m_c,
		suit_rsp_msg_suit_check_installed_component_digest_rsp_m_c,
		suit_rsp_msg_suit_get_installed_manifest_info_rsp_m_c,
		suit_rsp_msg_suit_get_install_candidate_info_rsp_m_c,
		suit_rsp_msg_suit_authenticate_manifest_rsp_m_c,
		suit_rsp_msg_suit_authorize_unsigned_manifest_rsp_m_c,
		suit_rsp_msg_suit_authorize_seq_num_rsp_m_c,
		suit_rsp_msg_suit_check_component_compatibility_rsp_m_c,
		suit_rsp_msg_suit_get_supported_manifest_roles_rsp_m_c,
		suit_rsp_msg_suit_get_supported_manifest_info_rsp_m_c,
		suit_rsp_msg_suit_authorize_process_dependency_rsp_m_c,
		suit_rsp_msg_suit_evt_sub_rsp_m_c,
		suit_rsp_msg_suit_chunk_enqueue_rsp_m_c,
		suit_rsp_msg_suit_chunk_status_rsp_m_c,
	} suit_rsp_msg_choice;
};

#ifdef __cplusplus
}
#endif

#endif /* SUIT_SERVICE_TYPES_H__ */
