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

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "zcbor_encode.h"
#include "psa_crypto_service_encode.h"
#include "zcbor_print.h"

#if DEFAULT_MAX_QTY != 3
#error "The type file was generated with a different default_max_qty than this file"
#endif

static bool encode_psa_get_key_attributes_req(zcbor_state_t *state,
					      const struct psa_get_key_attributes_req *input);
static bool encode_psa_reset_key_attributes_req(zcbor_state_t *state,
						const struct psa_reset_key_attributes_req *input);
static bool encode_psa_purge_key_req(zcbor_state_t *state, const struct psa_purge_key_req *input);
static bool encode_psa_copy_key_req(zcbor_state_t *state, const struct psa_copy_key_req *input);
static bool encode_psa_destroy_key_req(zcbor_state_t *state,
				       const struct psa_destroy_key_req *input);
static bool encode_psa_import_key_req(zcbor_state_t *state, const struct psa_import_key_req *input);
static bool encode_psa_export_key_req(zcbor_state_t *state, const struct psa_export_key_req *input);
static bool encode_psa_export_public_key_req(zcbor_state_t *state,
					     const struct psa_export_public_key_req *input);
static bool encode_psa_hash_compute_req(zcbor_state_t *state,
					const struct psa_hash_compute_req *input);
static bool encode_psa_hash_compare_req(zcbor_state_t *state,
					const struct psa_hash_compare_req *input);
static bool encode_psa_hash_setup_req(zcbor_state_t *state, const struct psa_hash_setup_req *input);
static bool encode_psa_hash_update_req(zcbor_state_t *state,
				       const struct psa_hash_update_req *input);
static bool encode_psa_hash_finish_req(zcbor_state_t *state,
				       const struct psa_hash_finish_req *input);
static bool encode_psa_hash_verify_req(zcbor_state_t *state,
				       const struct psa_hash_verify_req *input);
static bool encode_psa_hash_abort_req(zcbor_state_t *state, const struct psa_hash_abort_req *input);
static bool encode_psa_hash_clone_req(zcbor_state_t *state, const struct psa_hash_clone_req *input);
static bool encode_psa_mac_compute_req(zcbor_state_t *state,
				       const struct psa_mac_compute_req *input);
static bool encode_psa_mac_verify_req(zcbor_state_t *state, const struct psa_mac_verify_req *input);
static bool encode_psa_mac_sign_setup_req(zcbor_state_t *state,
					  const struct psa_mac_sign_setup_req *input);
static bool encode_psa_mac_verify_setup_req(zcbor_state_t *state,
					    const struct psa_mac_verify_setup_req *input);
static bool encode_psa_mac_update_req(zcbor_state_t *state, const struct psa_mac_update_req *input);
static bool encode_psa_mac_sign_finish_req(zcbor_state_t *state,
					   const struct psa_mac_sign_finish_req *input);
static bool encode_psa_mac_verify_finish_req(zcbor_state_t *state,
					     const struct psa_mac_verify_finish_req *input);
static bool encode_psa_mac_abort_req(zcbor_state_t *state, const struct psa_mac_abort_req *input);
static bool encode_psa_cipher_encrypt_req(zcbor_state_t *state,
					  const struct psa_cipher_encrypt_req *input);
static bool encode_psa_cipher_decrypt_req(zcbor_state_t *state,
					  const struct psa_cipher_decrypt_req *input);
static bool encode_psa_cipher_encrypt_setup_req(zcbor_state_t *state,
						const struct psa_cipher_encrypt_setup_req *input);
static bool encode_psa_cipher_decrypt_setup_req(zcbor_state_t *state,
						const struct psa_cipher_decrypt_setup_req *input);
static bool encode_psa_cipher_generate_iv_req(zcbor_state_t *state,
					      const struct psa_cipher_generate_iv_req *input);
static bool encode_psa_cipher_set_iv_req(zcbor_state_t *state,
					 const struct psa_cipher_set_iv_req *input);
static bool encode_psa_cipher_update_req(zcbor_state_t *state,
					 const struct psa_cipher_update_req *input);
static bool encode_psa_cipher_finish_req(zcbor_state_t *state,
					 const struct psa_cipher_finish_req *input);
static bool encode_psa_cipher_abort_req(zcbor_state_t *state,
					const struct psa_cipher_abort_req *input);
static bool encode_psa_aead_encrypt_req(zcbor_state_t *state,
					const struct psa_aead_encrypt_req *input);
static bool encode_psa_aead_decrypt_req(zcbor_state_t *state,
					const struct psa_aead_decrypt_req *input);
static bool encode_psa_aead_encrypt_setup_req(zcbor_state_t *state,
					      const struct psa_aead_encrypt_setup_req *input);
static bool encode_psa_aead_decrypt_setup_req(zcbor_state_t *state,
					      const struct psa_aead_decrypt_setup_req *input);
static bool encode_psa_aead_generate_nonce_req(zcbor_state_t *state,
					       const struct psa_aead_generate_nonce_req *input);
static bool encode_psa_aead_set_nonce_req(zcbor_state_t *state,
					  const struct psa_aead_set_nonce_req *input);
static bool encode_psa_aead_set_lengths_req(zcbor_state_t *state,
					    const struct psa_aead_set_lengths_req *input);
static bool encode_psa_aead_update_ad_req(zcbor_state_t *state,
					  const struct psa_aead_update_ad_req *input);
static bool encode_psa_aead_update_req(zcbor_state_t *state,
				       const struct psa_aead_update_req *input);
static bool encode_psa_aead_finish_req(zcbor_state_t *state,
				       const struct psa_aead_finish_req *input);
static bool encode_psa_aead_verify_req(zcbor_state_t *state,
				       const struct psa_aead_verify_req *input);
static bool encode_psa_aead_abort_req(zcbor_state_t *state, const struct psa_aead_abort_req *input);
static bool encode_psa_sign_message_req(zcbor_state_t *state,
					const struct psa_sign_message_req *input);
static bool encode_psa_verify_message_req(zcbor_state_t *state,
					  const struct psa_verify_message_req *input);
static bool encode_psa_sign_hash_req(zcbor_state_t *state, const struct psa_sign_hash_req *input);
static bool encode_psa_verify_hash_req(zcbor_state_t *state,
				       const struct psa_verify_hash_req *input);
static bool encode_psa_asymmetric_encrypt_req(zcbor_state_t *state,
					      const struct psa_asymmetric_encrypt_req *input);
static bool encode_psa_asymmetric_decrypt_req(zcbor_state_t *state,
					      const struct psa_asymmetric_decrypt_req *input);
static bool encode_psa_key_derivation_setup_req(zcbor_state_t *state,
						const struct psa_key_derivation_setup_req *input);
static bool
encode_psa_key_derivation_get_capacity_req(zcbor_state_t *state,
					   const struct psa_key_derivation_get_capacity_req *input);
static bool
encode_psa_key_derivation_set_capacity_req(zcbor_state_t *state,
					   const struct psa_key_derivation_set_capacity_req *input);
static bool
encode_psa_key_derivation_input_bytes_req(zcbor_state_t *state,
					  const struct psa_key_derivation_input_bytes_req *input);
static bool encode_psa_key_derivation_input_integer_req(
	zcbor_state_t *state, const struct psa_key_derivation_input_integer_req *input);
static bool
encode_psa_key_derivation_input_key_req(zcbor_state_t *state,
					const struct psa_key_derivation_input_key_req *input);
static bool encode_psa_key_derivation_key_agreement_req(
	zcbor_state_t *state, const struct psa_key_derivation_key_agreement_req *input);
static bool
encode_psa_key_derivation_output_bytes_req(zcbor_state_t *state,
					   const struct psa_key_derivation_output_bytes_req *input);
static bool
encode_psa_key_derivation_output_key_req(zcbor_state_t *state,
					 const struct psa_key_derivation_output_key_req *input);
static bool encode_psa_key_derivation_abort_req(zcbor_state_t *state,
						const struct psa_key_derivation_abort_req *input);
static bool encode_psa_raw_key_agreement_req(zcbor_state_t *state,
					     const struct psa_raw_key_agreement_req *input);
static bool encode_psa_generate_random_req(zcbor_state_t *state,
					   const struct psa_generate_random_req *input);
static bool encode_psa_generate_key_req(zcbor_state_t *state,
					const struct psa_generate_key_req *input);
static bool encode_psa_sign_hash_start_req(zcbor_state_t *state,
					   const struct psa_sign_hash_start_req *input);
static bool encode_psa_sign_hash_abort_req(zcbor_state_t *state,
					   const struct psa_sign_hash_abort_req *input);
static bool encode_psa_verify_hash_start_req(zcbor_state_t *state,
					     const struct psa_verify_hash_start_req *input);
static bool encode_psa_verify_hash_abort_req(zcbor_state_t *state,
					     const struct psa_verify_hash_abort_req *input);
static bool encode_psa_pake_setup_req(zcbor_state_t *state, const struct psa_pake_setup_req *input);
static bool encode_psa_pake_set_role_req(zcbor_state_t *state,
					 const struct psa_pake_set_role_req *input);
static bool encode_psa_pake_set_user_req(zcbor_state_t *state,
					 const struct psa_pake_set_user_req *input);
static bool encode_psa_pake_set_peer_req(zcbor_state_t *state,
					 const struct psa_pake_set_peer_req *input);
static bool encode_psa_pake_set_context_req(zcbor_state_t *state,
					    const struct psa_pake_set_context_req *input);
static bool encode_psa_pake_output_req(zcbor_state_t *state,
				       const struct psa_pake_output_req *input);
static bool encode_psa_pake_input_req(zcbor_state_t *state, const struct psa_pake_input_req *input);
static bool encode_psa_pake_get_shared_key_req(zcbor_state_t *state,
					       const struct psa_pake_get_shared_key_req *input);
static bool encode_psa_pake_abort_req(zcbor_state_t *state, const struct psa_pake_abort_req *input);
static bool encode_psa_crypto_rsp(zcbor_state_t *state, const struct psa_crypto_rsp *input);
static bool encode_psa_crypto_req(zcbor_state_t *state, const struct psa_crypto_req *input);

static bool encode_psa_get_key_attributes_req(zcbor_state_t *state,
					      const struct psa_get_key_attributes_req *input)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result =
		(((((zcbor_uint32_put(state, (11)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).psa_get_key_attributes_req_key)))) &&
		   ((zcbor_uint32_encode(state,
					 (&(*input).psa_get_key_attributes_req_p_attributes)))))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool encode_psa_reset_key_attributes_req(zcbor_state_t *state,
						const struct psa_reset_key_attributes_req *input)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result =
		(((((zcbor_uint32_put(state, (12)))) &&
		   ((zcbor_uint32_encode(
			   state, (&(*input).psa_reset_key_attributes_req_p_attributes)))))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool encode_psa_purge_key_req(zcbor_state_t *state, const struct psa_purge_key_req *input)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result = (((((zcbor_uint32_put(state, (13)))) &&
			     ((zcbor_uint32_encode(state, (&(*input).psa_purge_key_req_key)))))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool encode_psa_copy_key_req(zcbor_state_t *state, const struct psa_copy_key_req *input)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result =
		(((((zcbor_uint32_put(state, (14)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).psa_copy_key_req_source_key)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).psa_copy_key_req_p_attributes)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).psa_copy_key_req_p_target_key)))))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool encode_psa_destroy_key_req(zcbor_state_t *state,
				       const struct psa_destroy_key_req *input)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result = (((((zcbor_uint32_put(state, (15)))) &&
			     ((zcbor_uint32_encode(state, (&(*input).psa_destroy_key_req_key)))))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool encode_psa_import_key_req(zcbor_state_t *state, const struct psa_import_key_req *input)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result =
		(((((zcbor_uint32_put(state, (16)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).psa_import_key_req_p_attributes)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).psa_import_key_req_p_data)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).psa_import_key_req_data_length)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).psa_import_key_req_p_key)))))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool encode_psa_export_key_req(zcbor_state_t *state, const struct psa_export_key_req *input)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result =
		(((((zcbor_uint32_put(state, (17)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).psa_export_key_req_key)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).psa_export_key_req_p_data)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).psa_export_key_req_data_size)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).psa_export_key_req_p_data_length)))))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool encode_psa_export_public_key_req(zcbor_state_t *state,
					     const struct psa_export_public_key_req *input)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result = ((
		(((zcbor_uint32_put(state, (18)))) &&
		 ((zcbor_uint32_encode(state, (&(*input).psa_export_public_key_req_key)))) &&
		 ((zcbor_uint32_encode(state, (&(*input).psa_export_public_key_req_p_data)))) &&
		 ((zcbor_uint32_encode(state, (&(*input).psa_export_public_key_req_data_size)))) &&
		 ((zcbor_uint32_encode(state,
				       (&(*input).psa_export_public_key_req_p_data_length)))))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool encode_psa_hash_compute_req(zcbor_state_t *state,
					const struct psa_hash_compute_req *input)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result = ((
		(((zcbor_uint32_put(state, (19)))) &&
		 ((zcbor_uint32_encode(state, (&(*input).psa_hash_compute_req_alg)))) &&
		 ((zcbor_uint32_encode(state, (&(*input).psa_hash_compute_req_p_input)))) &&
		 ((zcbor_uint32_encode(state, (&(*input).psa_hash_compute_req_input_length)))) &&
		 ((zcbor_uint32_encode(state, (&(*input).psa_hash_compute_req_p_hash)))) &&
		 ((zcbor_uint32_encode(state, (&(*input).psa_hash_compute_req_hash_size)))) &&
		 ((zcbor_uint32_encode(state, (&(*input).psa_hash_compute_req_p_hash_length)))))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool encode_psa_hash_compare_req(zcbor_state_t *state,
					const struct psa_hash_compare_req *input)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result =
		(((((zcbor_uint32_put(state, (20)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).psa_hash_compare_req_alg)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).psa_hash_compare_req_p_input)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).psa_hash_compare_req_input_length)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).psa_hash_compare_req_p_hash)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).psa_hash_compare_req_hash_length)))))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool encode_psa_hash_setup_req(zcbor_state_t *state, const struct psa_hash_setup_req *input)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result =
		(((((zcbor_uint32_put(state, (21)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).psa_hash_setup_req_p_handle)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).psa_hash_setup_req_alg)))))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool encode_psa_hash_update_req(zcbor_state_t *state,
				       const struct psa_hash_update_req *input)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result =
		(((((zcbor_uint32_put(state, (22)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).psa_hash_update_req_p_handle)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).psa_hash_update_req_p_input)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).psa_hash_update_req_input_length)))))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool encode_psa_hash_finish_req(zcbor_state_t *state,
				       const struct psa_hash_finish_req *input)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result =
		(((((zcbor_uint32_put(state, (23)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).psa_hash_finish_req_p_handle)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).psa_hash_finish_req_p_hash)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).psa_hash_finish_req_hash_size)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).psa_hash_finish_req_p_hash_length)))))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool encode_psa_hash_verify_req(zcbor_state_t *state,
				       const struct psa_hash_verify_req *input)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result =
		(((((zcbor_uint32_put(state, (24)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).psa_hash_verify_req_p_handle)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).psa_hash_verify_req_p_hash)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).psa_hash_verify_req_hash_length)))))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool encode_psa_hash_abort_req(zcbor_state_t *state, const struct psa_hash_abort_req *input)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result =
		(((((zcbor_uint32_put(state, (25)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).psa_hash_abort_req_p_handle)))))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool encode_psa_hash_clone_req(zcbor_state_t *state, const struct psa_hash_clone_req *input)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result =
		(((((zcbor_uint32_put(state, (26)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).psa_hash_clone_req_handle)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).psa_hash_clone_req_p_handle)))))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool encode_psa_mac_compute_req(zcbor_state_t *state,
				       const struct psa_mac_compute_req *input)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result =
		(((((zcbor_uint32_put(state, (27)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).psa_mac_compute_req_key)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).psa_mac_compute_req_alg)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).psa_mac_compute_req_p_input)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).psa_mac_compute_req_input_length)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).psa_mac_compute_req_p_mac)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).psa_mac_compute_req_mac_size)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).psa_mac_compute_req_p_mac_length)))))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool encode_psa_mac_verify_req(zcbor_state_t *state, const struct psa_mac_verify_req *input)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result =
		(((((zcbor_uint32_put(state, (28)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).psa_mac_verify_req_key)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).psa_mac_verify_req_alg)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).psa_mac_verify_req_p_input)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).psa_mac_verify_req_input_length)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).psa_mac_verify_req_p_mac)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).psa_mac_verify_req_mac_length)))))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool encode_psa_mac_sign_setup_req(zcbor_state_t *state,
					  const struct psa_mac_sign_setup_req *input)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result =
		(((((zcbor_uint32_put(state, (29)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).psa_mac_sign_setup_req_p_handle)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).psa_mac_sign_setup_req_key)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).psa_mac_sign_setup_req_alg)))))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool encode_psa_mac_verify_setup_req(zcbor_state_t *state,
					    const struct psa_mac_verify_setup_req *input)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result =
		(((((zcbor_uint32_put(state, (30)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).psa_mac_verify_setup_req_p_handle)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).psa_mac_verify_setup_req_key)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).psa_mac_verify_setup_req_alg)))))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool encode_psa_mac_update_req(zcbor_state_t *state, const struct psa_mac_update_req *input)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result =
		(((((zcbor_uint32_put(state, (31)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).psa_mac_update_req_p_handle)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).psa_mac_update_req_p_input)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).psa_mac_update_req_input_length)))))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool encode_psa_mac_sign_finish_req(zcbor_state_t *state,
					   const struct psa_mac_sign_finish_req *input)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result = (((
		((zcbor_uint32_put(state, (32)))) &&
		((zcbor_uint32_encode(state, (&(*input).psa_mac_sign_finish_req_p_handle)))) &&
		((zcbor_uint32_encode(state, (&(*input).psa_mac_sign_finish_req_p_mac)))) &&
		((zcbor_uint32_encode(state, (&(*input).psa_mac_sign_finish_req_mac_size)))) &&
		((zcbor_uint32_encode(state, (&(*input).psa_mac_sign_finish_req_p_mac_length)))))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool encode_psa_mac_verify_finish_req(zcbor_state_t *state,
					     const struct psa_mac_verify_finish_req *input)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result = (((
		((zcbor_uint32_put(state, (33)))) &&
		((zcbor_uint32_encode(state, (&(*input).psa_mac_verify_finish_req_p_handle)))) &&
		((zcbor_uint32_encode(state, (&(*input).psa_mac_verify_finish_req_p_mac)))) &&
		((zcbor_uint32_encode(state, (&(*input).psa_mac_verify_finish_req_mac_length)))))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool encode_psa_mac_abort_req(zcbor_state_t *state, const struct psa_mac_abort_req *input)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result =
		(((((zcbor_uint32_put(state, (34)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).psa_mac_abort_req_p_handle)))))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool encode_psa_cipher_encrypt_req(zcbor_state_t *state,
					  const struct psa_cipher_encrypt_req *input)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result = ((
		(((zcbor_uint32_put(state, (35)))) &&
		 ((zcbor_uint32_encode(state, (&(*input).psa_cipher_encrypt_req_key)))) &&
		 ((zcbor_uint32_encode(state, (&(*input).psa_cipher_encrypt_req_alg)))) &&
		 ((zcbor_uint32_encode(state, (&(*input).psa_cipher_encrypt_req_p_input)))) &&
		 ((zcbor_uint32_encode(state, (&(*input).psa_cipher_encrypt_req_input_length)))) &&
		 ((zcbor_uint32_encode(state, (&(*input).psa_cipher_encrypt_req_p_output)))) &&
		 ((zcbor_uint32_encode(state, (&(*input).psa_cipher_encrypt_req_output_size)))) &&
		 ((zcbor_uint32_encode(state,
				       (&(*input).psa_cipher_encrypt_req_p_output_length)))))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool encode_psa_cipher_decrypt_req(zcbor_state_t *state,
					  const struct psa_cipher_decrypt_req *input)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result = ((
		(((zcbor_uint32_put(state, (36)))) &&
		 ((zcbor_uint32_encode(state, (&(*input).psa_cipher_decrypt_req_key)))) &&
		 ((zcbor_uint32_encode(state, (&(*input).psa_cipher_decrypt_req_alg)))) &&
		 ((zcbor_uint32_encode(state, (&(*input).psa_cipher_decrypt_req_p_input)))) &&
		 ((zcbor_uint32_encode(state, (&(*input).psa_cipher_decrypt_req_input_length)))) &&
		 ((zcbor_uint32_encode(state, (&(*input).psa_cipher_decrypt_req_p_output)))) &&
		 ((zcbor_uint32_encode(state, (&(*input).psa_cipher_decrypt_req_output_size)))) &&
		 ((zcbor_uint32_encode(state,
				       (&(*input).psa_cipher_decrypt_req_p_output_length)))))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool encode_psa_cipher_encrypt_setup_req(zcbor_state_t *state,
						const struct psa_cipher_encrypt_setup_req *input)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result = (((
		((zcbor_uint32_put(state, (37)))) &&
		((zcbor_uint32_encode(state, (&(*input).psa_cipher_encrypt_setup_req_p_handle)))) &&
		((zcbor_uint32_encode(state, (&(*input).psa_cipher_encrypt_setup_req_key)))) &&
		((zcbor_uint32_encode(state, (&(*input).psa_cipher_encrypt_setup_req_alg)))))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool encode_psa_cipher_decrypt_setup_req(zcbor_state_t *state,
						const struct psa_cipher_decrypt_setup_req *input)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result = (((
		((zcbor_uint32_put(state, (38)))) &&
		((zcbor_uint32_encode(state, (&(*input).psa_cipher_decrypt_setup_req_p_handle)))) &&
		((zcbor_uint32_encode(state, (&(*input).psa_cipher_decrypt_setup_req_key)))) &&
		((zcbor_uint32_encode(state, (&(*input).psa_cipher_decrypt_setup_req_alg)))))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool encode_psa_cipher_generate_iv_req(zcbor_state_t *state,
					      const struct psa_cipher_generate_iv_req *input)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result = ((
		(((zcbor_uint32_put(state, (39)))) &&
		 ((zcbor_uint32_encode(state, (&(*input).psa_cipher_generate_iv_req_p_handle)))) &&
		 ((zcbor_uint32_encode(state, (&(*input).psa_cipher_generate_iv_req_p_iv)))) &&
		 ((zcbor_uint32_encode(state, (&(*input).psa_cipher_generate_iv_req_iv_size)))) &&
		 ((zcbor_uint32_encode(state,
				       (&(*input).psa_cipher_generate_iv_req_p_iv_length)))))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool encode_psa_cipher_set_iv_req(zcbor_state_t *state,
					 const struct psa_cipher_set_iv_req *input)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result =
		(((((zcbor_uint32_put(state, (40)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).psa_cipher_set_iv_req_p_handle)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).psa_cipher_set_iv_req_p_iv)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).psa_cipher_set_iv_req_iv_length)))))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool encode_psa_cipher_update_req(zcbor_state_t *state,
					 const struct psa_cipher_update_req *input)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result =
		(((((zcbor_uint32_put(state, (41)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).psa_cipher_update_req_p_handle)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).psa_cipher_update_req_p_input)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).psa_cipher_update_req_input_length)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).psa_cipher_update_req_p_output)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).psa_cipher_update_req_output_size)))) &&
		   ((zcbor_uint32_encode(state,
					 (&(*input).psa_cipher_update_req_p_output_length)))))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool encode_psa_cipher_finish_req(zcbor_state_t *state,
					 const struct psa_cipher_finish_req *input)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result =
		(((((zcbor_uint32_put(state, (42)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).psa_cipher_finish_req_p_handle)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).psa_cipher_finish_req_p_output)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).psa_cipher_finish_req_output_size)))) &&
		   ((zcbor_uint32_encode(state,
					 (&(*input).psa_cipher_finish_req_p_output_length)))))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool encode_psa_cipher_abort_req(zcbor_state_t *state,
					const struct psa_cipher_abort_req *input)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result =
		(((((zcbor_uint32_put(state, (43)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).psa_cipher_abort_req_p_handle)))))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool encode_psa_aead_encrypt_req(zcbor_state_t *state,
					const struct psa_aead_encrypt_req *input)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result = (((
		((zcbor_uint32_put(state, (44)))) &&
		((zcbor_uint32_encode(state, (&(*input).psa_aead_encrypt_req_key)))) &&
		((zcbor_uint32_encode(state, (&(*input).psa_aead_encrypt_req_alg)))) &&
		((zcbor_uint32_encode(state, (&(*input).psa_aead_encrypt_req_p_nonce)))) &&
		((zcbor_uint32_encode(state, (&(*input).psa_aead_encrypt_req_nonce_length)))) &&
		((zcbor_uint32_encode(state,
				      (&(*input).psa_aead_encrypt_req_p_additional_data)))) &&
		((zcbor_uint32_encode(state,
				      (&(*input).psa_aead_encrypt_req_additional_data_length)))) &&
		((zcbor_uint32_encode(state, (&(*input).psa_aead_encrypt_req_p_plaintext)))) &&
		((zcbor_uint32_encode(state, (&(*input).psa_aead_encrypt_req_plaintext_length)))) &&
		((zcbor_uint32_encode(state, (&(*input).psa_aead_encrypt_req_p_ciphertext)))) &&
		((zcbor_uint32_encode(state, (&(*input).psa_aead_encrypt_req_ciphertext_size)))) &&
		((zcbor_uint32_encode(state,
				      (&(*input).psa_aead_encrypt_req_p_ciphertext_length)))))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool encode_psa_aead_decrypt_req(zcbor_state_t *state,
					const struct psa_aead_decrypt_req *input)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result = ((
		(((zcbor_uint32_put(state, (45)))) &&
		 ((zcbor_uint32_encode(state, (&(*input).psa_aead_decrypt_req_key)))) &&
		 ((zcbor_uint32_encode(state, (&(*input).psa_aead_decrypt_req_alg)))) &&
		 ((zcbor_uint32_encode(state, (&(*input).psa_aead_decrypt_req_p_nonce)))) &&
		 ((zcbor_uint32_encode(state, (&(*input).psa_aead_decrypt_req_nonce_length)))) &&
		 ((zcbor_uint32_encode(state,
				       (&(*input).psa_aead_decrypt_req_p_additional_data)))) &&
		 ((zcbor_uint32_encode(state,
				       (&(*input).psa_aead_decrypt_req_additional_data_length)))) &&
		 ((zcbor_uint32_encode(state, (&(*input).psa_aead_decrypt_req_p_ciphertext)))) &&
		 ((zcbor_uint32_encode(state,
				       (&(*input).psa_aead_decrypt_req_ciphertext_length)))) &&
		 ((zcbor_uint32_encode(state, (&(*input).psa_aead_decrypt_req_p_plaintext)))) &&
		 ((zcbor_uint32_encode(state, (&(*input).psa_aead_decrypt_req_plaintext_size)))) &&
		 ((zcbor_uint32_encode(state,
				       (&(*input).psa_aead_decrypt_req_p_plaintext_length)))))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool encode_psa_aead_encrypt_setup_req(zcbor_state_t *state,
					      const struct psa_aead_encrypt_setup_req *input)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result = ((
		(((zcbor_uint32_put(state, (46)))) &&
		 ((zcbor_uint32_encode(state, (&(*input).psa_aead_encrypt_setup_req_p_handle)))) &&
		 ((zcbor_uint32_encode(state, (&(*input).psa_aead_encrypt_setup_req_key)))) &&
		 ((zcbor_uint32_encode(state, (&(*input).psa_aead_encrypt_setup_req_alg)))))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool encode_psa_aead_decrypt_setup_req(zcbor_state_t *state,
					      const struct psa_aead_decrypt_setup_req *input)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result = ((
		(((zcbor_uint32_put(state, (47)))) &&
		 ((zcbor_uint32_encode(state, (&(*input).psa_aead_decrypt_setup_req_p_handle)))) &&
		 ((zcbor_uint32_encode(state, (&(*input).psa_aead_decrypt_setup_req_key)))) &&
		 ((zcbor_uint32_encode(state, (&(*input).psa_aead_decrypt_setup_req_alg)))))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool encode_psa_aead_generate_nonce_req(zcbor_state_t *state,
					       const struct psa_aead_generate_nonce_req *input)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result = ((
		(((zcbor_uint32_put(state, (48)))) &&
		 ((zcbor_uint32_encode(state, (&(*input).psa_aead_generate_nonce_req_p_handle)))) &&
		 ((zcbor_uint32_encode(state, (&(*input).psa_aead_generate_nonce_req_p_nonce)))) &&
		 ((zcbor_uint32_encode(state,
				       (&(*input).psa_aead_generate_nonce_req_nonce_size)))) &&
		 ((zcbor_uint32_encode(state,
				       (&(*input).psa_aead_generate_nonce_req_p_nonce_length)))))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool encode_psa_aead_set_nonce_req(zcbor_state_t *state,
					  const struct psa_aead_set_nonce_req *input)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result = ((
		(((zcbor_uint32_put(state, (49)))) &&
		 ((zcbor_uint32_encode(state, (&(*input).psa_aead_set_nonce_req_p_handle)))) &&
		 ((zcbor_uint32_encode(state, (&(*input).psa_aead_set_nonce_req_p_nonce)))) &&
		 ((zcbor_uint32_encode(state, (&(*input).psa_aead_set_nonce_req_nonce_length)))))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool encode_psa_aead_set_lengths_req(zcbor_state_t *state,
					    const struct psa_aead_set_lengths_req *input)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result =
		(((((zcbor_uint32_put(state, (50)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).psa_aead_set_lengths_req_p_handle)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).psa_aead_set_lengths_req_ad_length)))) &&
		   ((zcbor_uint32_encode(
			   state, (&(*input).psa_aead_set_lengths_req_plaintext_length)))))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool encode_psa_aead_update_ad_req(zcbor_state_t *state,
					  const struct psa_aead_update_ad_req *input)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result = ((
		(((zcbor_uint32_put(state, (51)))) &&
		 ((zcbor_uint32_encode(state, (&(*input).psa_aead_update_ad_req_p_handle)))) &&
		 ((zcbor_uint32_encode(state, (&(*input).psa_aead_update_ad_req_p_input)))) &&
		 ((zcbor_uint32_encode(state, (&(*input).psa_aead_update_ad_req_input_length)))))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool encode_psa_aead_update_req(zcbor_state_t *state,
				       const struct psa_aead_update_req *input)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result = ((
		(((zcbor_uint32_put(state, (52)))) &&
		 ((zcbor_uint32_encode(state, (&(*input).psa_aead_update_req_p_handle)))) &&
		 ((zcbor_uint32_encode(state, (&(*input).psa_aead_update_req_p_input)))) &&
		 ((zcbor_uint32_encode(state, (&(*input).psa_aead_update_req_input_length)))) &&
		 ((zcbor_uint32_encode(state, (&(*input).psa_aead_update_req_p_output)))) &&
		 ((zcbor_uint32_encode(state, (&(*input).psa_aead_update_req_output_size)))) &&
		 ((zcbor_uint32_encode(state, (&(*input).psa_aead_update_req_p_output_length)))))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool encode_psa_aead_finish_req(zcbor_state_t *state,
				       const struct psa_aead_finish_req *input)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result = ((
		(((zcbor_uint32_put(state, (53)))) &&
		 ((zcbor_uint32_encode(state, (&(*input).psa_aead_finish_req_p_handle)))) &&
		 ((zcbor_uint32_encode(state, (&(*input).psa_aead_finish_req_p_ciphertext)))) &&
		 ((zcbor_uint32_encode(state, (&(*input).psa_aead_finish_req_ciphertext_size)))) &&
		 ((zcbor_uint32_encode(state,
				       (&(*input).psa_aead_finish_req_p_ciphertext_length)))) &&
		 ((zcbor_uint32_encode(state, (&(*input).psa_aead_finish_req_p_tag)))) &&
		 ((zcbor_uint32_encode(state, (&(*input).psa_aead_finish_req_tag_size)))) &&
		 ((zcbor_uint32_encode(state, (&(*input).psa_aead_finish_req_p_tag_length)))))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool encode_psa_aead_verify_req(zcbor_state_t *state,
				       const struct psa_aead_verify_req *input)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result =
		(((((zcbor_uint32_put(state, (54)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).psa_aead_verify_req_p_handle)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).psa_aead_verify_req_p_plaintext)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).psa_aead_verify_req_plaintext_size)))) &&
		   ((zcbor_uint32_encode(state,
					 (&(*input).psa_aead_verify_req_p_plaintext_length)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).psa_aead_verify_req_p_tag)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).psa_aead_verify_req_tag_length)))))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool encode_psa_aead_abort_req(zcbor_state_t *state, const struct psa_aead_abort_req *input)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result =
		(((((zcbor_uint32_put(state, (55)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).psa_aead_abort_req_p_handle)))))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool encode_psa_sign_message_req(zcbor_state_t *state,
					const struct psa_sign_message_req *input)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result = ((
		(((zcbor_uint32_put(state, (56)))) &&
		 ((zcbor_uint32_encode(state, (&(*input).psa_sign_message_req_key)))) &&
		 ((zcbor_uint32_encode(state, (&(*input).psa_sign_message_req_alg)))) &&
		 ((zcbor_uint32_encode(state, (&(*input).psa_sign_message_req_p_input)))) &&
		 ((zcbor_uint32_encode(state, (&(*input).psa_sign_message_req_input_length)))) &&
		 ((zcbor_uint32_encode(state, (&(*input).psa_sign_message_req_p_signature)))) &&
		 ((zcbor_uint32_encode(state, (&(*input).psa_sign_message_req_signature_size)))) &&
		 ((zcbor_uint32_encode(state,
				       (&(*input).psa_sign_message_req_p_signature_length)))))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool encode_psa_verify_message_req(zcbor_state_t *state,
					  const struct psa_verify_message_req *input)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result = ((
		(((zcbor_uint32_put(state, (57)))) &&
		 ((zcbor_uint32_encode(state, (&(*input).psa_verify_message_req_key)))) &&
		 ((zcbor_uint32_encode(state, (&(*input).psa_verify_message_req_alg)))) &&
		 ((zcbor_uint32_encode(state, (&(*input).psa_verify_message_req_p_input)))) &&
		 ((zcbor_uint32_encode(state, (&(*input).psa_verify_message_req_input_length)))) &&
		 ((zcbor_uint32_encode(state, (&(*input).psa_verify_message_req_p_signature)))) &&
		 ((zcbor_uint32_encode(state,
				       (&(*input).psa_verify_message_req_signature_length)))))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool encode_psa_sign_hash_req(zcbor_state_t *state, const struct psa_sign_hash_req *input)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result = (((
		((zcbor_uint32_put(state, (58)))) &&
		((zcbor_uint32_encode(state, (&(*input).psa_sign_hash_req_key)))) &&
		((zcbor_uint32_encode(state, (&(*input).psa_sign_hash_req_alg)))) &&
		((zcbor_uint32_encode(state, (&(*input).psa_sign_hash_req_p_hash)))) &&
		((zcbor_uint32_encode(state, (&(*input).psa_sign_hash_req_hash_length)))) &&
		((zcbor_uint32_encode(state, (&(*input).psa_sign_hash_req_p_signature)))) &&
		((zcbor_uint32_encode(state, (&(*input).psa_sign_hash_req_signature_size)))) &&
		((zcbor_uint32_encode(state, (&(*input).psa_sign_hash_req_p_signature_length)))))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool encode_psa_verify_hash_req(zcbor_state_t *state,
				       const struct psa_verify_hash_req *input)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result = (((
		((zcbor_uint32_put(state, (59)))) &&
		((zcbor_uint32_encode(state, (&(*input).psa_verify_hash_req_key)))) &&
		((zcbor_uint32_encode(state, (&(*input).psa_verify_hash_req_alg)))) &&
		((zcbor_uint32_encode(state, (&(*input).psa_verify_hash_req_p_hash)))) &&
		((zcbor_uint32_encode(state, (&(*input).psa_verify_hash_req_hash_length)))) &&
		((zcbor_uint32_encode(state, (&(*input).psa_verify_hash_req_p_signature)))) &&
		((zcbor_uint32_encode(state, (&(*input).psa_verify_hash_req_signature_length)))))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool encode_psa_asymmetric_encrypt_req(zcbor_state_t *state,
					      const struct psa_asymmetric_encrypt_req *input)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result = ((
		(((zcbor_uint32_put(state, (60)))) &&
		 ((zcbor_uint32_encode(state, (&(*input).psa_asymmetric_encrypt_req_key)))) &&
		 ((zcbor_uint32_encode(state, (&(*input).psa_asymmetric_encrypt_req_alg)))) &&
		 ((zcbor_uint32_encode(state, (&(*input).psa_asymmetric_encrypt_req_p_input)))) &&
		 ((zcbor_uint32_encode(state,
				       (&(*input).psa_asymmetric_encrypt_req_input_length)))) &&
		 ((zcbor_uint32_encode(state, (&(*input).psa_asymmetric_encrypt_req_p_salt)))) &&
		 ((zcbor_uint32_encode(state,
				       (&(*input).psa_asymmetric_encrypt_req_salt_length)))) &&
		 ((zcbor_uint32_encode(state, (&(*input).psa_asymmetric_encrypt_req_p_output)))) &&
		 ((zcbor_uint32_encode(state,
				       (&(*input).psa_asymmetric_encrypt_req_output_size)))) &&
		 ((zcbor_uint32_encode(state,
				       (&(*input).psa_asymmetric_encrypt_req_p_output_length)))))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool encode_psa_asymmetric_decrypt_req(zcbor_state_t *state,
					      const struct psa_asymmetric_decrypt_req *input)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result = ((
		(((zcbor_uint32_put(state, (61)))) &&
		 ((zcbor_uint32_encode(state, (&(*input).psa_asymmetric_decrypt_req_key)))) &&
		 ((zcbor_uint32_encode(state, (&(*input).psa_asymmetric_decrypt_req_alg)))) &&
		 ((zcbor_uint32_encode(state, (&(*input).psa_asymmetric_decrypt_req_p_input)))) &&
		 ((zcbor_uint32_encode(state,
				       (&(*input).psa_asymmetric_decrypt_req_input_length)))) &&
		 ((zcbor_uint32_encode(state, (&(*input).psa_asymmetric_decrypt_req_p_salt)))) &&
		 ((zcbor_uint32_encode(state,
				       (&(*input).psa_asymmetric_decrypt_req_salt_length)))) &&
		 ((zcbor_uint32_encode(state, (&(*input).psa_asymmetric_decrypt_req_p_output)))) &&
		 ((zcbor_uint32_encode(state,
				       (&(*input).psa_asymmetric_decrypt_req_output_size)))) &&
		 ((zcbor_uint32_encode(state,
				       (&(*input).psa_asymmetric_decrypt_req_p_output_length)))))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool encode_psa_key_derivation_setup_req(zcbor_state_t *state,
						const struct psa_key_derivation_setup_req *input)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result = (((
		((zcbor_uint32_put(state, (62)))) &&
		((zcbor_uint32_encode(state, (&(*input).psa_key_derivation_setup_req_p_handle)))) &&
		((zcbor_uint32_encode(state, (&(*input).psa_key_derivation_setup_req_alg)))))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool
encode_psa_key_derivation_get_capacity_req(zcbor_state_t *state,
					   const struct psa_key_derivation_get_capacity_req *input)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result =
		(((((zcbor_uint32_put(state, (63)))) &&
		   ((zcbor_uint32_encode(
			   state, (&(*input).psa_key_derivation_get_capacity_req_handle)))) &&
		   ((zcbor_uint32_encode(
			   state, (&(*input).psa_key_derivation_get_capacity_req_p_capacity)))))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool
encode_psa_key_derivation_set_capacity_req(zcbor_state_t *state,
					   const struct psa_key_derivation_set_capacity_req *input)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result =
		(((((zcbor_uint32_put(state, (64)))) &&
		   ((zcbor_uint32_encode(
			   state, (&(*input).psa_key_derivation_set_capacity_req_p_handle)))) &&
		   ((zcbor_uint32_encode(
			   state, (&(*input).psa_key_derivation_set_capacity_req_capacity)))))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool
encode_psa_key_derivation_input_bytes_req(zcbor_state_t *state,
					  const struct psa_key_derivation_input_bytes_req *input)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result =
		(((((zcbor_uint32_put(state, (65)))) &&
		   ((zcbor_uint32_encode(
			   state, (&(*input).psa_key_derivation_input_bytes_req_p_handle)))) &&
		   ((zcbor_uint32_encode(state,
					 (&(*input).psa_key_derivation_input_bytes_req_step)))) &&
		   ((zcbor_uint32_encode(state,
					 (&(*input).psa_key_derivation_input_bytes_req_p_data)))) &&
		   ((zcbor_uint32_encode(
			   state, (&(*input).psa_key_derivation_input_bytes_req_data_length)))))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool encode_psa_key_derivation_input_integer_req(
	zcbor_state_t *state, const struct psa_key_derivation_input_integer_req *input)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result =
		(((((zcbor_uint32_put(state, (66)))) &&
		   ((zcbor_uint32_encode(
			   state, (&(*input).psa_key_derivation_input_integer_req_p_handle)))) &&
		   ((zcbor_uint32_encode(state,
					 (&(*input).psa_key_derivation_input_integer_req_step)))) &&
		   ((zcbor_uint32_encode(
			   state, (&(*input).psa_key_derivation_input_integer_req_value)))))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool
encode_psa_key_derivation_input_key_req(zcbor_state_t *state,
					const struct psa_key_derivation_input_key_req *input)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result = (((
		((zcbor_uint32_put(state, (67)))) &&
		((zcbor_uint32_encode(state,
				      (&(*input).psa_key_derivation_input_key_req_p_handle)))) &&
		((zcbor_uint32_encode(state, (&(*input).psa_key_derivation_input_key_req_step)))) &&
		((zcbor_uint32_encode(state, (&(*input).psa_key_derivation_input_key_req_key)))))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool encode_psa_key_derivation_key_agreement_req(
	zcbor_state_t *state, const struct psa_key_derivation_key_agreement_req *input)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result =
		(((((zcbor_uint32_put(state, (68)))) &&
		   ((zcbor_uint32_encode(
			   state, (&(*input).psa_key_derivation_key_agreement_req_p_handle)))) &&
		   ((zcbor_uint32_encode(state,
					 (&(*input).psa_key_derivation_key_agreement_req_step)))) &&
		   ((zcbor_uint32_encode(
			   state, (&(*input).psa_key_derivation_key_agreement_req_private_key)))) &&
		   ((zcbor_uint32_encode(
			   state, (&(*input).psa_key_derivation_key_agreement_req_p_peer_key)))) &&
		   ((zcbor_uint32_encode(
			   state,
			   (&(*input).psa_key_derivation_key_agreement_req_peer_key_length)))))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool
encode_psa_key_derivation_output_bytes_req(zcbor_state_t *state,
					   const struct psa_key_derivation_output_bytes_req *input)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result = (((
		((zcbor_uint32_put(state, (69)))) &&
		((zcbor_uint32_encode(state,
				      (&(*input).psa_key_derivation_output_bytes_req_p_handle)))) &&
		((zcbor_uint32_encode(state,
				      (&(*input).psa_key_derivation_output_bytes_req_p_output)))) &&
		((zcbor_uint32_encode(
			state, (&(*input).psa_key_derivation_output_bytes_req_output_length)))))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool
encode_psa_key_derivation_output_key_req(zcbor_state_t *state,
					 const struct psa_key_derivation_output_key_req *input)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result =
		(((((zcbor_uint32_put(state, (70)))) &&
		   ((zcbor_uint32_encode(
			   state, (&(*input).psa_key_derivation_output_key_req_p_attributes)))) &&
		   ((zcbor_uint32_encode(
			   state, (&(*input).psa_key_derivation_output_key_req_p_handle)))) &&
		   ((zcbor_uint32_encode(state,
					 (&(*input).psa_key_derivation_output_key_req_p_key)))))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool encode_psa_key_derivation_abort_req(zcbor_state_t *state,
						const struct psa_key_derivation_abort_req *input)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result = (((((zcbor_uint32_put(state, (71)))) &&
			     ((zcbor_uint32_encode(
				     state, (&(*input).psa_key_derivation_abort_req_p_handle)))))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool encode_psa_raw_key_agreement_req(zcbor_state_t *state,
					     const struct psa_raw_key_agreement_req *input)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result = (((
		((zcbor_uint32_put(state, (72)))) &&
		((zcbor_uint32_encode(state, (&(*input).psa_raw_key_agreement_req_alg)))) &&
		((zcbor_uint32_encode(state, (&(*input).psa_raw_key_agreement_req_private_key)))) &&
		((zcbor_uint32_encode(state, (&(*input).psa_raw_key_agreement_req_p_peer_key)))) &&
		((zcbor_uint32_encode(state,
				      (&(*input).psa_raw_key_agreement_req_peer_key_length)))) &&
		((zcbor_uint32_encode(state, (&(*input).psa_raw_key_agreement_req_p_output)))) &&
		((zcbor_uint32_encode(state, (&(*input).psa_raw_key_agreement_req_output_size)))) &&
		((zcbor_uint32_encode(state,
				      (&(*input).psa_raw_key_agreement_req_p_output_length)))))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool encode_psa_generate_random_req(zcbor_state_t *state,
					   const struct psa_generate_random_req *input)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result = ((
		(((zcbor_uint32_put(state, (73)))) &&
		 ((zcbor_uint32_encode(state, (&(*input).psa_generate_random_req_p_output)))) &&
		 ((zcbor_uint32_encode(state, (&(*input).psa_generate_random_req_output_size)))))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool encode_psa_generate_key_req(zcbor_state_t *state,
					const struct psa_generate_key_req *input)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result =
		(((((zcbor_uint32_put(state, (74)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).psa_generate_key_req_p_attributes)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).psa_generate_key_req_p_key)))))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool encode_psa_sign_hash_start_req(zcbor_state_t *state,
					   const struct psa_sign_hash_start_req *input)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result = ((
		(((zcbor_uint32_put(state, (75)))) &&
		 ((zcbor_uint32_encode(state, (&(*input).psa_sign_hash_start_req_p_operation)))) &&
		 ((zcbor_uint32_encode(state, (&(*input).psa_sign_hash_start_req_key)))) &&
		 ((zcbor_uint32_encode(state, (&(*input).psa_sign_hash_start_req_alg)))) &&
		 ((zcbor_uint32_encode(state, (&(*input).psa_sign_hash_start_req_p_hash)))) &&
		 ((zcbor_uint32_encode(state, (&(*input).psa_sign_hash_start_req_hash_length)))))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool encode_psa_sign_hash_abort_req(zcbor_state_t *state,
					   const struct psa_sign_hash_abort_req *input)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result = ((
		(((zcbor_uint32_put(state, (76)))) &&
		 ((zcbor_uint32_encode(state, (&(*input).psa_sign_hash_abort_req_p_operation)))))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool encode_psa_verify_hash_start_req(zcbor_state_t *state,
					     const struct psa_verify_hash_start_req *input)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result = (((
		((zcbor_uint32_put(state, (77)))) &&
		((zcbor_uint32_encode(state, (&(*input).psa_verify_hash_start_req_p_operation)))) &&
		((zcbor_uint32_encode(state, (&(*input).psa_verify_hash_start_req_key)))) &&
		((zcbor_uint32_encode(state, (&(*input).psa_verify_hash_start_req_alg)))) &&
		((zcbor_uint32_encode(state, (&(*input).psa_verify_hash_start_req_p_hash)))) &&
		((zcbor_uint32_encode(state, (&(*input).psa_verify_hash_start_req_hash_length)))) &&
		((zcbor_uint32_encode(state, (&(*input).psa_verify_hash_start_req_p_signature)))) &&
		((zcbor_uint32_encode(state,
				      (&(*input).psa_verify_hash_start_req_signature_length)))))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool encode_psa_verify_hash_abort_req(zcbor_state_t *state,
					     const struct psa_verify_hash_abort_req *input)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result = (((((zcbor_uint32_put(state, (78)))) &&
			     ((zcbor_uint32_encode(
				     state, (&(*input).psa_verify_hash_abort_req_p_operation)))))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool encode_psa_pake_setup_req(zcbor_state_t *state, const struct psa_pake_setup_req *input)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result =
		(((((zcbor_uint32_put(state, (79)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).psa_pake_setup_req_p_handle)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).psa_pake_setup_req_password_key)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).psa_pake_setup_req_p_cipher_suite)))))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool encode_psa_pake_set_role_req(zcbor_state_t *state,
					 const struct psa_pake_set_role_req *input)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result =
		(((((zcbor_uint32_put(state, (80)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).psa_pake_set_role_req_p_handle)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).psa_pake_set_role_req_role)))))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool encode_psa_pake_set_user_req(zcbor_state_t *state,
					 const struct psa_pake_set_user_req *input)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result =
		(((((zcbor_uint32_put(state, (81)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).psa_pake_set_user_req_p_handle)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).psa_pake_set_user_req_p_user_id)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).psa_pake_set_user_req_user_id_len)))))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool encode_psa_pake_set_peer_req(zcbor_state_t *state,
					 const struct psa_pake_set_peer_req *input)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result =
		(((((zcbor_uint32_put(state, (82)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).psa_pake_set_peer_req_p_handle)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).psa_pake_set_peer_req_p_peer_id)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).psa_pake_set_peer_req_peer_id_len)))))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool encode_psa_pake_set_context_req(zcbor_state_t *state,
					    const struct psa_pake_set_context_req *input)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result = (((
		((zcbor_uint32_put(state, (83)))) &&
		((zcbor_uint32_encode(state, (&(*input).psa_pake_set_context_req_p_handle)))) &&
		((zcbor_uint32_encode(state, (&(*input).psa_pake_set_context_req_p_context)))) &&
		((zcbor_uint32_encode(state, (&(*input).psa_pake_set_context_req_context_len)))))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool encode_psa_pake_output_req(zcbor_state_t *state,
				       const struct psa_pake_output_req *input)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result = ((
		(((zcbor_uint32_put(state, (84)))) &&
		 ((zcbor_uint32_encode(state, (&(*input).psa_pake_output_req_p_handle)))) &&
		 ((zcbor_uint32_encode(state, (&(*input).psa_pake_output_req_step)))) &&
		 ((zcbor_uint32_encode(state, (&(*input).psa_pake_output_req_p_output)))) &&
		 ((zcbor_uint32_encode(state, (&(*input).psa_pake_output_req_output_size)))) &&
		 ((zcbor_uint32_encode(state, (&(*input).psa_pake_output_req_p_output_length)))))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool encode_psa_pake_input_req(zcbor_state_t *state, const struct psa_pake_input_req *input)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result =
		(((((zcbor_uint32_put(state, (85)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).psa_pake_input_req_p_handle)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).psa_pake_input_req_step)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).psa_pake_input_req_p_input)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).psa_pake_input_req_input_length)))))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool encode_psa_pake_get_shared_key_req(zcbor_state_t *state,
					       const struct psa_pake_get_shared_key_req *input)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result = ((
		(((zcbor_uint32_put(state, (86)))) &&
		 ((zcbor_uint32_encode(state, (&(*input).psa_pake_get_shared_key_req_p_handle)))) &&
		 ((zcbor_uint32_encode(state,
				       (&(*input).psa_pake_get_shared_key_req_p_attributes)))) &&
		 ((zcbor_uint32_encode(state, (&(*input).psa_pake_get_shared_key_req_p_key)))))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool encode_psa_pake_abort_req(zcbor_state_t *state, const struct psa_pake_abort_req *input)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result =
		(((((zcbor_uint32_put(state, (87)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).psa_pake_abort_req_p_handle)))))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool encode_psa_crypto_rsp(zcbor_state_t *state, const struct psa_crypto_rsp *input)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result = (((zcbor_list_start_encode(state, 2) &&
			     ((((zcbor_uint32_encode(state, (&(*input).psa_crypto_rsp_id)))) &&
			       ((zcbor_int32_encode(state, (&(*input).psa_crypto_rsp_status))))) ||
			      (zcbor_list_map_end_force_encode(state), false)) &&
			     zcbor_list_end_encode(state, 2))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool encode_psa_crypto_req(zcbor_state_t *state, const struct psa_crypto_req *input)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result = (((zcbor_list_start_encode(state, 12) && ((((((*input).psa_crypto_req_msg_choice == psa_crypto_req_msg_psa_crypto_init_req_m_c) ? ((zcbor_uint32_put(state, (10))))
	: (((*input).psa_crypto_req_msg_choice == psa_crypto_req_msg_psa_get_key_attributes_req_m_c) ? ((encode_psa_get_key_attributes_req(state, (&(*input).psa_crypto_req_msg_psa_get_key_attributes_req_m))))
	: (((*input).psa_crypto_req_msg_choice == psa_crypto_req_msg_psa_reset_key_attributes_req_m_c) ? ((encode_psa_reset_key_attributes_req(state, (&(*input).psa_crypto_req_msg_psa_reset_key_attributes_req_m))))
	: (((*input).psa_crypto_req_msg_choice == psa_crypto_req_msg_psa_purge_key_req_m_c) ? ((encode_psa_purge_key_req(state, (&(*input).psa_crypto_req_msg_psa_purge_key_req_m))))
	: (((*input).psa_crypto_req_msg_choice == psa_crypto_req_msg_psa_copy_key_req_m_c) ? ((encode_psa_copy_key_req(state, (&(*input).psa_crypto_req_msg_psa_copy_key_req_m))))
	: (((*input).psa_crypto_req_msg_choice == psa_crypto_req_msg_psa_destroy_key_req_m_c) ? ((encode_psa_destroy_key_req(state, (&(*input).psa_crypto_req_msg_psa_destroy_key_req_m))))
	: (((*input).psa_crypto_req_msg_choice == psa_crypto_req_msg_psa_import_key_req_m_c) ? ((encode_psa_import_key_req(state, (&(*input).psa_crypto_req_msg_psa_import_key_req_m))))
	: (((*input).psa_crypto_req_msg_choice == psa_crypto_req_msg_psa_export_key_req_m_c) ? ((encode_psa_export_key_req(state, (&(*input).psa_crypto_req_msg_psa_export_key_req_m))))
	: (((*input).psa_crypto_req_msg_choice == psa_crypto_req_msg_psa_export_public_key_req_m_c) ? ((encode_psa_export_public_key_req(state, (&(*input).psa_crypto_req_msg_psa_export_public_key_req_m))))
	: (((*input).psa_crypto_req_msg_choice == psa_crypto_req_msg_psa_hash_compute_req_m_c) ? ((encode_psa_hash_compute_req(state, (&(*input).psa_crypto_req_msg_psa_hash_compute_req_m))))
	: (((*input).psa_crypto_req_msg_choice == psa_crypto_req_msg_psa_hash_compare_req_m_c) ? ((encode_psa_hash_compare_req(state, (&(*input).psa_crypto_req_msg_psa_hash_compare_req_m))))
	: (((*input).psa_crypto_req_msg_choice == psa_crypto_req_msg_psa_hash_setup_req_m_c) ? ((encode_psa_hash_setup_req(state, (&(*input).psa_crypto_req_msg_psa_hash_setup_req_m))))
	: (((*input).psa_crypto_req_msg_choice == psa_crypto_req_msg_psa_hash_update_req_m_c) ? ((encode_psa_hash_update_req(state, (&(*input).psa_crypto_req_msg_psa_hash_update_req_m))))
	: (((*input).psa_crypto_req_msg_choice == psa_crypto_req_msg_psa_hash_finish_req_m_c) ? ((encode_psa_hash_finish_req(state, (&(*input).psa_crypto_req_msg_psa_hash_finish_req_m))))
	: (((*input).psa_crypto_req_msg_choice == psa_crypto_req_msg_psa_hash_verify_req_m_c) ? ((encode_psa_hash_verify_req(state, (&(*input).psa_crypto_req_msg_psa_hash_verify_req_m))))
	: (((*input).psa_crypto_req_msg_choice == psa_crypto_req_msg_psa_hash_abort_req_m_c) ? ((encode_psa_hash_abort_req(state, (&(*input).psa_crypto_req_msg_psa_hash_abort_req_m))))
	: (((*input).psa_crypto_req_msg_choice == psa_crypto_req_msg_psa_hash_clone_req_m_c) ? ((encode_psa_hash_clone_req(state, (&(*input).psa_crypto_req_msg_psa_hash_clone_req_m))))
	: (((*input).psa_crypto_req_msg_choice == psa_crypto_req_msg_psa_mac_compute_req_m_c) ? ((encode_psa_mac_compute_req(state, (&(*input).psa_crypto_req_msg_psa_mac_compute_req_m))))
	: (((*input).psa_crypto_req_msg_choice == psa_crypto_req_msg_psa_mac_verify_req_m_c) ? ((encode_psa_mac_verify_req(state, (&(*input).psa_crypto_req_msg_psa_mac_verify_req_m))))
	: (((*input).psa_crypto_req_msg_choice == psa_crypto_req_msg_psa_mac_sign_setup_req_m_c) ? ((encode_psa_mac_sign_setup_req(state, (&(*input).psa_crypto_req_msg_psa_mac_sign_setup_req_m))))
	: (((*input).psa_crypto_req_msg_choice == psa_crypto_req_msg_psa_mac_verify_setup_req_m_c) ? ((encode_psa_mac_verify_setup_req(state, (&(*input).psa_crypto_req_msg_psa_mac_verify_setup_req_m))))
	: (((*input).psa_crypto_req_msg_choice == psa_crypto_req_msg_psa_mac_update_req_m_c) ? ((encode_psa_mac_update_req(state, (&(*input).psa_crypto_req_msg_psa_mac_update_req_m))))
	: (((*input).psa_crypto_req_msg_choice == psa_crypto_req_msg_psa_mac_sign_finish_req_m_c) ? ((encode_psa_mac_sign_finish_req(state, (&(*input).psa_crypto_req_msg_psa_mac_sign_finish_req_m))))
	: (((*input).psa_crypto_req_msg_choice == psa_crypto_req_msg_psa_mac_verify_finish_req_m_c) ? ((encode_psa_mac_verify_finish_req(state, (&(*input).psa_crypto_req_msg_psa_mac_verify_finish_req_m))))
	: (((*input).psa_crypto_req_msg_choice == psa_crypto_req_msg_psa_mac_abort_req_m_c) ? ((encode_psa_mac_abort_req(state, (&(*input).psa_crypto_req_msg_psa_mac_abort_req_m))))
	: (((*input).psa_crypto_req_msg_choice == psa_crypto_req_msg_psa_cipher_encrypt_req_m_c) ? ((encode_psa_cipher_encrypt_req(state, (&(*input).psa_crypto_req_msg_psa_cipher_encrypt_req_m))))
	: (((*input).psa_crypto_req_msg_choice == psa_crypto_req_msg_psa_cipher_decrypt_req_m_c) ? ((encode_psa_cipher_decrypt_req(state, (&(*input).psa_crypto_req_msg_psa_cipher_decrypt_req_m))))
	: (((*input).psa_crypto_req_msg_choice == psa_crypto_req_msg_psa_cipher_encrypt_setup_req_m_c) ? ((encode_psa_cipher_encrypt_setup_req(state, (&(*input).psa_crypto_req_msg_psa_cipher_encrypt_setup_req_m))))
	: (((*input).psa_crypto_req_msg_choice == psa_crypto_req_msg_psa_cipher_decrypt_setup_req_m_c) ? ((encode_psa_cipher_decrypt_setup_req(state, (&(*input).psa_crypto_req_msg_psa_cipher_decrypt_setup_req_m))))
	: (((*input).psa_crypto_req_msg_choice == psa_crypto_req_msg_psa_cipher_generate_iv_req_m_c) ? ((encode_psa_cipher_generate_iv_req(state, (&(*input).psa_crypto_req_msg_psa_cipher_generate_iv_req_m))))
	: (((*input).psa_crypto_req_msg_choice == psa_crypto_req_msg_psa_cipher_set_iv_req_m_c) ? ((encode_psa_cipher_set_iv_req(state, (&(*input).psa_crypto_req_msg_psa_cipher_set_iv_req_m))))
	: (((*input).psa_crypto_req_msg_choice == psa_crypto_req_msg_psa_cipher_update_req_m_c) ? ((encode_psa_cipher_update_req(state, (&(*input).psa_crypto_req_msg_psa_cipher_update_req_m))))
	: (((*input).psa_crypto_req_msg_choice == psa_crypto_req_msg_psa_cipher_finish_req_m_c) ? ((encode_psa_cipher_finish_req(state, (&(*input).psa_crypto_req_msg_psa_cipher_finish_req_m))))
	: (((*input).psa_crypto_req_msg_choice == psa_crypto_req_msg_psa_cipher_abort_req_m_c) ? ((encode_psa_cipher_abort_req(state, (&(*input).psa_crypto_req_msg_psa_cipher_abort_req_m))))
	: (((*input).psa_crypto_req_msg_choice == psa_crypto_req_msg_psa_aead_encrypt_req_m_c) ? ((encode_psa_aead_encrypt_req(state, (&(*input).psa_crypto_req_msg_psa_aead_encrypt_req_m))))
	: (((*input).psa_crypto_req_msg_choice == psa_crypto_req_msg_psa_aead_decrypt_req_m_c) ? ((encode_psa_aead_decrypt_req(state, (&(*input).psa_crypto_req_msg_psa_aead_decrypt_req_m))))
	: (((*input).psa_crypto_req_msg_choice == psa_crypto_req_msg_psa_aead_encrypt_setup_req_m_c) ? ((encode_psa_aead_encrypt_setup_req(state, (&(*input).psa_crypto_req_msg_psa_aead_encrypt_setup_req_m))))
	: (((*input).psa_crypto_req_msg_choice == psa_crypto_req_msg_psa_aead_decrypt_setup_req_m_c) ? ((encode_psa_aead_decrypt_setup_req(state, (&(*input).psa_crypto_req_msg_psa_aead_decrypt_setup_req_m))))
	: (((*input).psa_crypto_req_msg_choice == psa_crypto_req_msg_psa_aead_generate_nonce_req_m_c) ? ((encode_psa_aead_generate_nonce_req(state, (&(*input).psa_crypto_req_msg_psa_aead_generate_nonce_req_m))))
	: (((*input).psa_crypto_req_msg_choice == psa_crypto_req_msg_psa_aead_set_nonce_req_m_c) ? ((encode_psa_aead_set_nonce_req(state, (&(*input).psa_crypto_req_msg_psa_aead_set_nonce_req_m))))
	: (((*input).psa_crypto_req_msg_choice == psa_crypto_req_msg_psa_aead_set_lengths_req_m_c) ? ((encode_psa_aead_set_lengths_req(state, (&(*input).psa_crypto_req_msg_psa_aead_set_lengths_req_m))))
	: (((*input).psa_crypto_req_msg_choice == psa_crypto_req_msg_psa_aead_update_ad_req_m_c) ? ((encode_psa_aead_update_ad_req(state, (&(*input).psa_crypto_req_msg_psa_aead_update_ad_req_m))))
	: (((*input).psa_crypto_req_msg_choice == psa_crypto_req_msg_psa_aead_update_req_m_c) ? ((encode_psa_aead_update_req(state, (&(*input).psa_crypto_req_msg_psa_aead_update_req_m))))
	: (((*input).psa_crypto_req_msg_choice == psa_crypto_req_msg_psa_aead_finish_req_m_c) ? ((encode_psa_aead_finish_req(state, (&(*input).psa_crypto_req_msg_psa_aead_finish_req_m))))
	: (((*input).psa_crypto_req_msg_choice == psa_crypto_req_msg_psa_aead_verify_req_m_c) ? ((encode_psa_aead_verify_req(state, (&(*input).psa_crypto_req_msg_psa_aead_verify_req_m))))
	: (((*input).psa_crypto_req_msg_choice == psa_crypto_req_msg_psa_aead_abort_req_m_c) ? ((encode_psa_aead_abort_req(state, (&(*input).psa_crypto_req_msg_psa_aead_abort_req_m))))
	: (((*input).psa_crypto_req_msg_choice == psa_crypto_req_msg_psa_sign_message_req_m_c) ? ((encode_psa_sign_message_req(state, (&(*input).psa_crypto_req_msg_psa_sign_message_req_m))))
	: (((*input).psa_crypto_req_msg_choice == psa_crypto_req_msg_psa_verify_message_req_m_c) ? ((encode_psa_verify_message_req(state, (&(*input).psa_crypto_req_msg_psa_verify_message_req_m))))
	: (((*input).psa_crypto_req_msg_choice == psa_crypto_req_msg_psa_sign_hash_req_m_c) ? ((encode_psa_sign_hash_req(state, (&(*input).psa_crypto_req_msg_psa_sign_hash_req_m))))
	: (((*input).psa_crypto_req_msg_choice == psa_crypto_req_msg_psa_verify_hash_req_m_c) ? ((encode_psa_verify_hash_req(state, (&(*input).psa_crypto_req_msg_psa_verify_hash_req_m))))
	: (((*input).psa_crypto_req_msg_choice == psa_crypto_req_msg_psa_asymmetric_encrypt_req_m_c) ? ((encode_psa_asymmetric_encrypt_req(state, (&(*input).psa_crypto_req_msg_psa_asymmetric_encrypt_req_m))))
	: (((*input).psa_crypto_req_msg_choice == psa_crypto_req_msg_psa_asymmetric_decrypt_req_m_c) ? ((encode_psa_asymmetric_decrypt_req(state, (&(*input).psa_crypto_req_msg_psa_asymmetric_decrypt_req_m))))
	: (((*input).psa_crypto_req_msg_choice == psa_crypto_req_msg_psa_key_derivation_setup_req_m_c) ? ((encode_psa_key_derivation_setup_req(state, (&(*input).psa_crypto_req_msg_psa_key_derivation_setup_req_m))))
	: (((*input).psa_crypto_req_msg_choice == psa_crypto_req_msg_psa_key_derivation_get_capacity_req_m_c) ? ((encode_psa_key_derivation_get_capacity_req(state, (&(*input).psa_crypto_req_msg_psa_key_derivation_get_capacity_req_m))))
	: (((*input).psa_crypto_req_msg_choice == psa_crypto_req_msg_psa_key_derivation_set_capacity_req_m_c) ? ((encode_psa_key_derivation_set_capacity_req(state, (&(*input).psa_crypto_req_msg_psa_key_derivation_set_capacity_req_m))))
	: (((*input).psa_crypto_req_msg_choice == psa_crypto_req_msg_psa_key_derivation_input_bytes_req_m_c) ? ((encode_psa_key_derivation_input_bytes_req(state, (&(*input).psa_crypto_req_msg_psa_key_derivation_input_bytes_req_m))))
	: (((*input).psa_crypto_req_msg_choice == psa_crypto_req_msg_psa_key_derivation_input_integer_req_m_c) ? ((encode_psa_key_derivation_input_integer_req(state, (&(*input).psa_crypto_req_msg_psa_key_derivation_input_integer_req_m))))
	: (((*input).psa_crypto_req_msg_choice == psa_crypto_req_msg_psa_key_derivation_input_key_req_m_c) ? ((encode_psa_key_derivation_input_key_req(state, (&(*input).psa_crypto_req_msg_psa_key_derivation_input_key_req_m))))
	: (((*input).psa_crypto_req_msg_choice == psa_crypto_req_msg_psa_key_derivation_key_agreement_req_m_c) ? ((encode_psa_key_derivation_key_agreement_req(state, (&(*input).psa_crypto_req_msg_psa_key_derivation_key_agreement_req_m))))
	: (((*input).psa_crypto_req_msg_choice == psa_crypto_req_msg_psa_key_derivation_output_bytes_req_m_c) ? ((encode_psa_key_derivation_output_bytes_req(state, (&(*input).psa_crypto_req_msg_psa_key_derivation_output_bytes_req_m))))
	: (((*input).psa_crypto_req_msg_choice == psa_crypto_req_msg_psa_key_derivation_output_key_req_m_c) ? ((encode_psa_key_derivation_output_key_req(state, (&(*input).psa_crypto_req_msg_psa_key_derivation_output_key_req_m))))
	: (((*input).psa_crypto_req_msg_choice == psa_crypto_req_msg_psa_key_derivation_abort_req_m_c) ? ((encode_psa_key_derivation_abort_req(state, (&(*input).psa_crypto_req_msg_psa_key_derivation_abort_req_m))))
	: (((*input).psa_crypto_req_msg_choice == psa_crypto_req_msg_psa_raw_key_agreement_req_m_c) ? ((encode_psa_raw_key_agreement_req(state, (&(*input).psa_crypto_req_msg_psa_raw_key_agreement_req_m))))
	: (((*input).psa_crypto_req_msg_choice == psa_crypto_req_msg_psa_generate_random_req_m_c) ? ((encode_psa_generate_random_req(state, (&(*input).psa_crypto_req_msg_psa_generate_random_req_m))))
	: (((*input).psa_crypto_req_msg_choice == psa_crypto_req_msg_psa_generate_key_req_m_c) ? ((encode_psa_generate_key_req(state, (&(*input).psa_crypto_req_msg_psa_generate_key_req_m))))
	: (((*input).psa_crypto_req_msg_choice == psa_crypto_req_msg_psa_sign_hash_start_req_m_c) ? ((encode_psa_sign_hash_start_req(state, (&(*input).psa_crypto_req_msg_psa_sign_hash_start_req_m))))
	: (((*input).psa_crypto_req_msg_choice == psa_crypto_req_msg_psa_sign_hash_abort_req_m_c) ? ((encode_psa_sign_hash_abort_req(state, (&(*input).psa_crypto_req_msg_psa_sign_hash_abort_req_m))))
	: (((*input).psa_crypto_req_msg_choice == psa_crypto_req_msg_psa_verify_hash_start_req_m_c) ? ((encode_psa_verify_hash_start_req(state, (&(*input).psa_crypto_req_msg_psa_verify_hash_start_req_m))))
	: (((*input).psa_crypto_req_msg_choice == psa_crypto_req_msg_psa_verify_hash_abort_req_m_c) ? ((encode_psa_verify_hash_abort_req(state, (&(*input).psa_crypto_req_msg_psa_verify_hash_abort_req_m))))
	: (((*input).psa_crypto_req_msg_choice == psa_crypto_req_msg_psa_pake_setup_req_m_c) ? ((encode_psa_pake_setup_req(state, (&(*input).psa_crypto_req_msg_psa_pake_setup_req_m))))
	: (((*input).psa_crypto_req_msg_choice == psa_crypto_req_msg_psa_pake_set_role_req_m_c) ? ((encode_psa_pake_set_role_req(state, (&(*input).psa_crypto_req_msg_psa_pake_set_role_req_m))))
	: (((*input).psa_crypto_req_msg_choice == psa_crypto_req_msg_psa_pake_set_user_req_m_c) ? ((encode_psa_pake_set_user_req(state, (&(*input).psa_crypto_req_msg_psa_pake_set_user_req_m))))
	: (((*input).psa_crypto_req_msg_choice == psa_crypto_req_msg_psa_pake_set_peer_req_m_c) ? ((encode_psa_pake_set_peer_req(state, (&(*input).psa_crypto_req_msg_psa_pake_set_peer_req_m))))
	: (((*input).psa_crypto_req_msg_choice == psa_crypto_req_msg_psa_pake_set_context_req_m_c) ? ((encode_psa_pake_set_context_req(state, (&(*input).psa_crypto_req_msg_psa_pake_set_context_req_m))))
	: (((*input).psa_crypto_req_msg_choice == psa_crypto_req_msg_psa_pake_output_req_m_c) ? ((encode_psa_pake_output_req(state, (&(*input).psa_crypto_req_msg_psa_pake_output_req_m))))
	: (((*input).psa_crypto_req_msg_choice == psa_crypto_req_msg_psa_pake_input_req_m_c) ? ((encode_psa_pake_input_req(state, (&(*input).psa_crypto_req_msg_psa_pake_input_req_m))))
	: (((*input).psa_crypto_req_msg_choice == psa_crypto_req_msg_psa_pake_get_shared_key_req_m_c) ? ((encode_psa_pake_get_shared_key_req(state, (&(*input).psa_crypto_req_msg_psa_pake_get_shared_key_req_m))))
	: (((*input).psa_crypto_req_msg_choice == psa_crypto_req_msg_psa_pake_abort_req_m_c) ? ((encode_psa_pake_abort_req(state, (&(*input).psa_crypto_req_msg_psa_pake_abort_req_m))))
	: false)))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_list_end_encode(state, 12))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

int cbor_encode_psa_crypto_req(uint8_t *payload, size_t payload_len,
			       const struct psa_crypto_req *input, size_t *payload_len_out)
{
	zcbor_state_t states[4];

	return zcbor_entry_function(payload, payload_len, (void *)input, payload_len_out, states,
				    (zcbor_decoder_t *)encode_psa_crypto_req,
				    sizeof(states) / sizeof(zcbor_state_t), 1);
}

int cbor_encode_psa_crypto_rsp(uint8_t *payload, size_t payload_len,
			       const struct psa_crypto_rsp *input, size_t *payload_len_out)
{
	zcbor_state_t states[3];

	return zcbor_entry_function(payload, payload_len, (void *)input, payload_len_out, states,
				    (zcbor_decoder_t *)encode_psa_crypto_rsp,
				    sizeof(states) / sizeof(zcbor_state_t), 1);
}
