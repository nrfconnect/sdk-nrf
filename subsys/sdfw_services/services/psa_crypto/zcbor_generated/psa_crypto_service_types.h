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

#ifndef PSA_CRYPTO_SERVICE_TYPES_H__
#define PSA_CRYPTO_SERVICE_TYPES_H__

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

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

struct psa_crypto_rsp {
	uint32_t psa_crypto_rsp_id;
	int32_t psa_crypto_rsp_status;
};

struct psa_get_key_attributes_req {
	uint32_t psa_get_key_attributes_req_key;
	uint32_t psa_get_key_attributes_req_p_attributes;
};

struct psa_reset_key_attributes_req {
	uint32_t psa_reset_key_attributes_req_p_attributes;
};

struct psa_purge_key_req {
	uint32_t psa_purge_key_req_key;
};

struct psa_copy_key_req {
	uint32_t psa_copy_key_req_source_key;
	uint32_t psa_copy_key_req_p_attributes;
	uint32_t psa_copy_key_req_p_target_key;
};

struct psa_destroy_key_req {
	uint32_t psa_destroy_key_req_key;
};

struct psa_import_key_req {
	uint32_t psa_import_key_req_p_attributes;
	uint32_t psa_import_key_req_p_data;
	uint32_t psa_import_key_req_data_length;
	uint32_t psa_import_key_req_p_key;
};

struct psa_export_key_req {
	uint32_t psa_export_key_req_key;
	uint32_t psa_export_key_req_p_data;
	uint32_t psa_export_key_req_data_size;
	uint32_t psa_export_key_req_p_data_length;
};

struct psa_export_public_key_req {
	uint32_t psa_export_public_key_req_key;
	uint32_t psa_export_public_key_req_p_data;
	uint32_t psa_export_public_key_req_data_size;
	uint32_t psa_export_public_key_req_p_data_length;
};

struct psa_hash_compute_req {
	uint32_t psa_hash_compute_req_alg;
	uint32_t psa_hash_compute_req_p_input;
	uint32_t psa_hash_compute_req_input_length;
	uint32_t psa_hash_compute_req_p_hash;
	uint32_t psa_hash_compute_req_hash_size;
	uint32_t psa_hash_compute_req_p_hash_length;
};

struct psa_hash_compare_req {
	uint32_t psa_hash_compare_req_alg;
	uint32_t psa_hash_compare_req_p_input;
	uint32_t psa_hash_compare_req_input_length;
	uint32_t psa_hash_compare_req_p_hash;
	uint32_t psa_hash_compare_req_hash_length;
};

struct psa_hash_setup_req {
	uint32_t psa_hash_setup_req_p_handle;
	uint32_t psa_hash_setup_req_alg;
};

struct psa_hash_update_req {
	uint32_t psa_hash_update_req_p_handle;
	uint32_t psa_hash_update_req_p_input;
	uint32_t psa_hash_update_req_input_length;
};

struct psa_hash_finish_req {
	uint32_t psa_hash_finish_req_p_handle;
	uint32_t psa_hash_finish_req_p_hash;
	uint32_t psa_hash_finish_req_hash_size;
	uint32_t psa_hash_finish_req_p_hash_length;
};

struct psa_hash_verify_req {
	uint32_t psa_hash_verify_req_p_handle;
	uint32_t psa_hash_verify_req_p_hash;
	uint32_t psa_hash_verify_req_hash_length;
};

struct psa_hash_abort_req {
	uint32_t psa_hash_abort_req_p_handle;
};

struct psa_hash_clone_req {
	uint32_t psa_hash_clone_req_handle;
	uint32_t psa_hash_clone_req_p_handle;
};

struct psa_mac_compute_req {
	uint32_t psa_mac_compute_req_key;
	uint32_t psa_mac_compute_req_alg;
	uint32_t psa_mac_compute_req_p_input;
	uint32_t psa_mac_compute_req_input_length;
	uint32_t psa_mac_compute_req_p_mac;
	uint32_t psa_mac_compute_req_mac_size;
	uint32_t psa_mac_compute_req_p_mac_length;
};

struct psa_mac_verify_req {
	uint32_t psa_mac_verify_req_key;
	uint32_t psa_mac_verify_req_alg;
	uint32_t psa_mac_verify_req_p_input;
	uint32_t psa_mac_verify_req_input_length;
	uint32_t psa_mac_verify_req_p_mac;
	uint32_t psa_mac_verify_req_mac_length;
};

struct psa_mac_sign_setup_req {
	uint32_t psa_mac_sign_setup_req_p_handle;
	uint32_t psa_mac_sign_setup_req_key;
	uint32_t psa_mac_sign_setup_req_alg;
};

struct psa_mac_verify_setup_req {
	uint32_t psa_mac_verify_setup_req_p_handle;
	uint32_t psa_mac_verify_setup_req_key;
	uint32_t psa_mac_verify_setup_req_alg;
};

struct psa_mac_update_req {
	uint32_t psa_mac_update_req_p_handle;
	uint32_t psa_mac_update_req_p_input;
	uint32_t psa_mac_update_req_input_length;
};

struct psa_mac_sign_finish_req {
	uint32_t psa_mac_sign_finish_req_p_handle;
	uint32_t psa_mac_sign_finish_req_p_mac;
	uint32_t psa_mac_sign_finish_req_mac_size;
	uint32_t psa_mac_sign_finish_req_p_mac_length;
};

struct psa_mac_verify_finish_req {
	uint32_t psa_mac_verify_finish_req_p_handle;
	uint32_t psa_mac_verify_finish_req_p_mac;
	uint32_t psa_mac_verify_finish_req_mac_length;
};

struct psa_mac_abort_req {
	uint32_t psa_mac_abort_req_p_handle;
};

struct psa_cipher_encrypt_req {
	uint32_t psa_cipher_encrypt_req_key;
	uint32_t psa_cipher_encrypt_req_alg;
	uint32_t psa_cipher_encrypt_req_p_input;
	uint32_t psa_cipher_encrypt_req_input_length;
	uint32_t psa_cipher_encrypt_req_p_output;
	uint32_t psa_cipher_encrypt_req_output_size;
	uint32_t psa_cipher_encrypt_req_p_output_length;
};

struct psa_cipher_decrypt_req {
	uint32_t psa_cipher_decrypt_req_key;
	uint32_t psa_cipher_decrypt_req_alg;
	uint32_t psa_cipher_decrypt_req_p_input;
	uint32_t psa_cipher_decrypt_req_input_length;
	uint32_t psa_cipher_decrypt_req_p_output;
	uint32_t psa_cipher_decrypt_req_output_size;
	uint32_t psa_cipher_decrypt_req_p_output_length;
};

struct psa_cipher_encrypt_setup_req {
	uint32_t psa_cipher_encrypt_setup_req_p_handle;
	uint32_t psa_cipher_encrypt_setup_req_key;
	uint32_t psa_cipher_encrypt_setup_req_alg;
};

struct psa_cipher_decrypt_setup_req {
	uint32_t psa_cipher_decrypt_setup_req_p_handle;
	uint32_t psa_cipher_decrypt_setup_req_key;
	uint32_t psa_cipher_decrypt_setup_req_alg;
};

struct psa_cipher_generate_iv_req {
	uint32_t psa_cipher_generate_iv_req_p_handle;
	uint32_t psa_cipher_generate_iv_req_p_iv;
	uint32_t psa_cipher_generate_iv_req_iv_size;
	uint32_t psa_cipher_generate_iv_req_p_iv_length;
};

struct psa_cipher_set_iv_req {
	uint32_t psa_cipher_set_iv_req_p_handle;
	uint32_t psa_cipher_set_iv_req_p_iv;
	uint32_t psa_cipher_set_iv_req_iv_length;
};

struct psa_cipher_update_req {
	uint32_t psa_cipher_update_req_p_handle;
	uint32_t psa_cipher_update_req_p_input;
	uint32_t psa_cipher_update_req_input_length;
	uint32_t psa_cipher_update_req_p_output;
	uint32_t psa_cipher_update_req_output_size;
	uint32_t psa_cipher_update_req_p_output_length;
};

struct psa_cipher_finish_req {
	uint32_t psa_cipher_finish_req_p_handle;
	uint32_t psa_cipher_finish_req_p_output;
	uint32_t psa_cipher_finish_req_output_size;
	uint32_t psa_cipher_finish_req_p_output_length;
};

struct psa_cipher_abort_req {
	uint32_t psa_cipher_abort_req_p_handle;
};

struct psa_aead_encrypt_req {
	uint32_t psa_aead_encrypt_req_key;
	uint32_t psa_aead_encrypt_req_alg;
	uint32_t psa_aead_encrypt_req_p_nonce;
	uint32_t psa_aead_encrypt_req_nonce_length;
	uint32_t psa_aead_encrypt_req_p_additional_data;
	uint32_t psa_aead_encrypt_req_additional_data_length;
	uint32_t psa_aead_encrypt_req_p_plaintext;
	uint32_t psa_aead_encrypt_req_plaintext_length;
	uint32_t psa_aead_encrypt_req_p_ciphertext;
	uint32_t psa_aead_encrypt_req_ciphertext_size;
	uint32_t psa_aead_encrypt_req_p_ciphertext_length;
};

struct psa_aead_decrypt_req {
	uint32_t psa_aead_decrypt_req_key;
	uint32_t psa_aead_decrypt_req_alg;
	uint32_t psa_aead_decrypt_req_p_nonce;
	uint32_t psa_aead_decrypt_req_nonce_length;
	uint32_t psa_aead_decrypt_req_p_additional_data;
	uint32_t psa_aead_decrypt_req_additional_data_length;
	uint32_t psa_aead_decrypt_req_p_ciphertext;
	uint32_t psa_aead_decrypt_req_ciphertext_length;
	uint32_t psa_aead_decrypt_req_p_plaintext;
	uint32_t psa_aead_decrypt_req_plaintext_size;
	uint32_t psa_aead_decrypt_req_p_plaintext_length;
};

struct psa_aead_encrypt_setup_req {
	uint32_t psa_aead_encrypt_setup_req_p_handle;
	uint32_t psa_aead_encrypt_setup_req_key;
	uint32_t psa_aead_encrypt_setup_req_alg;
};

struct psa_aead_decrypt_setup_req {
	uint32_t psa_aead_decrypt_setup_req_p_handle;
	uint32_t psa_aead_decrypt_setup_req_key;
	uint32_t psa_aead_decrypt_setup_req_alg;
};

struct psa_aead_generate_nonce_req {
	uint32_t psa_aead_generate_nonce_req_p_handle;
	uint32_t psa_aead_generate_nonce_req_p_nonce;
	uint32_t psa_aead_generate_nonce_req_nonce_size;
	uint32_t psa_aead_generate_nonce_req_p_nonce_length;
};

struct psa_aead_set_nonce_req {
	uint32_t psa_aead_set_nonce_req_p_handle;
	uint32_t psa_aead_set_nonce_req_p_nonce;
	uint32_t psa_aead_set_nonce_req_nonce_length;
};

struct psa_aead_set_lengths_req {
	uint32_t psa_aead_set_lengths_req_p_handle;
	uint32_t psa_aead_set_lengths_req_ad_length;
	uint32_t psa_aead_set_lengths_req_plaintext_length;
};

struct psa_aead_update_ad_req {
	uint32_t psa_aead_update_ad_req_p_handle;
	uint32_t psa_aead_update_ad_req_p_input;
	uint32_t psa_aead_update_ad_req_input_length;
};

struct psa_aead_update_req {
	uint32_t psa_aead_update_req_p_handle;
	uint32_t psa_aead_update_req_p_input;
	uint32_t psa_aead_update_req_input_length;
	uint32_t psa_aead_update_req_p_output;
	uint32_t psa_aead_update_req_output_size;
	uint32_t psa_aead_update_req_p_output_length;
};

struct psa_aead_finish_req {
	uint32_t psa_aead_finish_req_p_handle;
	uint32_t psa_aead_finish_req_p_ciphertext;
	uint32_t psa_aead_finish_req_ciphertext_size;
	uint32_t psa_aead_finish_req_p_ciphertext_length;
	uint32_t psa_aead_finish_req_p_tag;
	uint32_t psa_aead_finish_req_tag_size;
	uint32_t psa_aead_finish_req_p_tag_length;
};

struct psa_aead_verify_req {
	uint32_t psa_aead_verify_req_p_handle;
	uint32_t psa_aead_verify_req_p_plaintext;
	uint32_t psa_aead_verify_req_plaintext_size;
	uint32_t psa_aead_verify_req_p_plaintext_length;
	uint32_t psa_aead_verify_req_p_tag;
	uint32_t psa_aead_verify_req_tag_length;
};

struct psa_aead_abort_req {
	uint32_t psa_aead_abort_req_p_handle;
};

struct psa_sign_message_req {
	uint32_t psa_sign_message_req_key;
	uint32_t psa_sign_message_req_alg;
	uint32_t psa_sign_message_req_p_input;
	uint32_t psa_sign_message_req_input_length;
	uint32_t psa_sign_message_req_p_signature;
	uint32_t psa_sign_message_req_signature_size;
	uint32_t psa_sign_message_req_p_signature_length;
};

struct psa_verify_message_req {
	uint32_t psa_verify_message_req_key;
	uint32_t psa_verify_message_req_alg;
	uint32_t psa_verify_message_req_p_input;
	uint32_t psa_verify_message_req_input_length;
	uint32_t psa_verify_message_req_p_signature;
	uint32_t psa_verify_message_req_signature_length;
};

struct psa_sign_hash_req {
	uint32_t psa_sign_hash_req_key;
	uint32_t psa_sign_hash_req_alg;
	uint32_t psa_sign_hash_req_p_hash;
	uint32_t psa_sign_hash_req_hash_length;
	uint32_t psa_sign_hash_req_p_signature;
	uint32_t psa_sign_hash_req_signature_size;
	uint32_t psa_sign_hash_req_p_signature_length;
};

struct psa_verify_hash_req {
	uint32_t psa_verify_hash_req_key;
	uint32_t psa_verify_hash_req_alg;
	uint32_t psa_verify_hash_req_p_hash;
	uint32_t psa_verify_hash_req_hash_length;
	uint32_t psa_verify_hash_req_p_signature;
	uint32_t psa_verify_hash_req_signature_length;
};

struct psa_asymmetric_encrypt_req {
	uint32_t psa_asymmetric_encrypt_req_key;
	uint32_t psa_asymmetric_encrypt_req_alg;
	uint32_t psa_asymmetric_encrypt_req_p_input;
	uint32_t psa_asymmetric_encrypt_req_input_length;
	uint32_t psa_asymmetric_encrypt_req_p_salt;
	uint32_t psa_asymmetric_encrypt_req_salt_length;
	uint32_t psa_asymmetric_encrypt_req_p_output;
	uint32_t psa_asymmetric_encrypt_req_output_size;
	uint32_t psa_asymmetric_encrypt_req_p_output_length;
};

struct psa_asymmetric_decrypt_req {
	uint32_t psa_asymmetric_decrypt_req_key;
	uint32_t psa_asymmetric_decrypt_req_alg;
	uint32_t psa_asymmetric_decrypt_req_p_input;
	uint32_t psa_asymmetric_decrypt_req_input_length;
	uint32_t psa_asymmetric_decrypt_req_p_salt;
	uint32_t psa_asymmetric_decrypt_req_salt_length;
	uint32_t psa_asymmetric_decrypt_req_p_output;
	uint32_t psa_asymmetric_decrypt_req_output_size;
	uint32_t psa_asymmetric_decrypt_req_p_output_length;
};

struct psa_key_derivation_setup_req {
	uint32_t psa_key_derivation_setup_req_p_handle;
	uint32_t psa_key_derivation_setup_req_alg;
};

struct psa_key_derivation_get_capacity_req {
	uint32_t psa_key_derivation_get_capacity_req_handle;
	uint32_t psa_key_derivation_get_capacity_req_p_capacity;
};

struct psa_key_derivation_set_capacity_req {
	uint32_t psa_key_derivation_set_capacity_req_p_handle;
	uint32_t psa_key_derivation_set_capacity_req_capacity;
};

struct psa_key_derivation_input_bytes_req {
	uint32_t psa_key_derivation_input_bytes_req_p_handle;
	uint32_t psa_key_derivation_input_bytes_req_step;
	uint32_t psa_key_derivation_input_bytes_req_p_data;
	uint32_t psa_key_derivation_input_bytes_req_data_length;
};

struct psa_key_derivation_input_integer_req {
	uint32_t psa_key_derivation_input_integer_req_p_handle;
	uint32_t psa_key_derivation_input_integer_req_step;
	uint32_t psa_key_derivation_input_integer_req_value;
};

struct psa_key_derivation_input_key_req {
	uint32_t psa_key_derivation_input_key_req_p_handle;
	uint32_t psa_key_derivation_input_key_req_step;
	uint32_t psa_key_derivation_input_key_req_key;
};

struct psa_key_derivation_key_agreement_req {
	uint32_t psa_key_derivation_key_agreement_req_p_handle;
	uint32_t psa_key_derivation_key_agreement_req_step;
	uint32_t psa_key_derivation_key_agreement_req_private_key;
	uint32_t psa_key_derivation_key_agreement_req_p_peer_key;
	uint32_t psa_key_derivation_key_agreement_req_peer_key_length;
};

struct psa_key_derivation_output_bytes_req {
	uint32_t psa_key_derivation_output_bytes_req_p_handle;
	uint32_t psa_key_derivation_output_bytes_req_p_output;
	uint32_t psa_key_derivation_output_bytes_req_output_length;
};

struct psa_key_derivation_output_key_req {
	uint32_t psa_key_derivation_output_key_req_p_attributes;
	uint32_t psa_key_derivation_output_key_req_p_handle;
	uint32_t psa_key_derivation_output_key_req_p_key;
};

struct psa_key_derivation_abort_req {
	uint32_t psa_key_derivation_abort_req_p_handle;
};

struct psa_raw_key_agreement_req {
	uint32_t psa_raw_key_agreement_req_alg;
	uint32_t psa_raw_key_agreement_req_private_key;
	uint32_t psa_raw_key_agreement_req_p_peer_key;
	uint32_t psa_raw_key_agreement_req_peer_key_length;
	uint32_t psa_raw_key_agreement_req_p_output;
	uint32_t psa_raw_key_agreement_req_output_size;
	uint32_t psa_raw_key_agreement_req_p_output_length;
};

struct psa_generate_random_req {
	uint32_t psa_generate_random_req_p_output;
	uint32_t psa_generate_random_req_output_size;
};

struct psa_generate_key_req {
	uint32_t psa_generate_key_req_p_attributes;
	uint32_t psa_generate_key_req_p_key;
};

struct psa_sign_hash_start_req {
	uint32_t psa_sign_hash_start_req_p_operation;
	uint32_t psa_sign_hash_start_req_key;
	uint32_t psa_sign_hash_start_req_alg;
	uint32_t psa_sign_hash_start_req_p_hash;
	uint32_t psa_sign_hash_start_req_hash_length;
};

struct psa_sign_hash_abort_req {
	uint32_t psa_sign_hash_abort_req_p_operation;
};

struct psa_verify_hash_start_req {
	uint32_t psa_verify_hash_start_req_p_operation;
	uint32_t psa_verify_hash_start_req_key;
	uint32_t psa_verify_hash_start_req_alg;
	uint32_t psa_verify_hash_start_req_p_hash;
	uint32_t psa_verify_hash_start_req_hash_length;
	uint32_t psa_verify_hash_start_req_p_signature;
	uint32_t psa_verify_hash_start_req_signature_length;
};

struct psa_verify_hash_abort_req {
	uint32_t psa_verify_hash_abort_req_p_operation;
};

struct psa_pake_setup_req {
	uint32_t psa_pake_setup_req_p_handle;
	uint32_t psa_pake_setup_req_password_key;
	uint32_t psa_pake_setup_req_p_cipher_suite;
};

struct psa_pake_set_role_req {
	uint32_t psa_pake_set_role_req_p_handle;
	uint32_t psa_pake_set_role_req_role;
};

struct psa_pake_set_user_req {
	uint32_t psa_pake_set_user_req_p_handle;
	uint32_t psa_pake_set_user_req_p_user_id;
	uint32_t psa_pake_set_user_req_user_id_len;
};

struct psa_pake_set_peer_req {
	uint32_t psa_pake_set_peer_req_p_handle;
	uint32_t psa_pake_set_peer_req_p_peer_id;
	uint32_t psa_pake_set_peer_req_peer_id_len;
};

struct psa_pake_set_context_req {
	uint32_t psa_pake_set_context_req_p_handle;
	uint32_t psa_pake_set_context_req_p_context;
	uint32_t psa_pake_set_context_req_context_len;
};

struct psa_pake_output_req {
	uint32_t psa_pake_output_req_p_handle;
	uint32_t psa_pake_output_req_step;
	uint32_t psa_pake_output_req_p_output;
	uint32_t psa_pake_output_req_output_size;
	uint32_t psa_pake_output_req_p_output_length;
};

struct psa_pake_input_req {
	uint32_t psa_pake_input_req_p_handle;
	uint32_t psa_pake_input_req_step;
	uint32_t psa_pake_input_req_p_input;
	uint32_t psa_pake_input_req_input_length;
};

struct psa_pake_get_shared_key_req {
	uint32_t psa_pake_get_shared_key_req_p_handle;
	uint32_t psa_pake_get_shared_key_req_p_attributes;
	uint32_t psa_pake_get_shared_key_req_p_key;
};

struct psa_pake_abort_req {
	uint32_t psa_pake_abort_req_p_handle;
};

struct psa_crypto_req {
	union {
		struct psa_get_key_attributes_req psa_crypto_req_msg_psa_get_key_attributes_req_m;
		struct psa_reset_key_attributes_req
			psa_crypto_req_msg_psa_reset_key_attributes_req_m;
		struct psa_purge_key_req psa_crypto_req_msg_psa_purge_key_req_m;
		struct psa_copy_key_req psa_crypto_req_msg_psa_copy_key_req_m;
		struct psa_destroy_key_req psa_crypto_req_msg_psa_destroy_key_req_m;
		struct psa_import_key_req psa_crypto_req_msg_psa_import_key_req_m;
		struct psa_export_key_req psa_crypto_req_msg_psa_export_key_req_m;
		struct psa_export_public_key_req psa_crypto_req_msg_psa_export_public_key_req_m;
		struct psa_hash_compute_req psa_crypto_req_msg_psa_hash_compute_req_m;
		struct psa_hash_compare_req psa_crypto_req_msg_psa_hash_compare_req_m;
		struct psa_hash_setup_req psa_crypto_req_msg_psa_hash_setup_req_m;
		struct psa_hash_update_req psa_crypto_req_msg_psa_hash_update_req_m;
		struct psa_hash_finish_req psa_crypto_req_msg_psa_hash_finish_req_m;
		struct psa_hash_verify_req psa_crypto_req_msg_psa_hash_verify_req_m;
		struct psa_hash_abort_req psa_crypto_req_msg_psa_hash_abort_req_m;
		struct psa_hash_clone_req psa_crypto_req_msg_psa_hash_clone_req_m;
		struct psa_mac_compute_req psa_crypto_req_msg_psa_mac_compute_req_m;
		struct psa_mac_verify_req psa_crypto_req_msg_psa_mac_verify_req_m;
		struct psa_mac_sign_setup_req psa_crypto_req_msg_psa_mac_sign_setup_req_m;
		struct psa_mac_verify_setup_req psa_crypto_req_msg_psa_mac_verify_setup_req_m;
		struct psa_mac_update_req psa_crypto_req_msg_psa_mac_update_req_m;
		struct psa_mac_sign_finish_req psa_crypto_req_msg_psa_mac_sign_finish_req_m;
		struct psa_mac_verify_finish_req psa_crypto_req_msg_psa_mac_verify_finish_req_m;
		struct psa_mac_abort_req psa_crypto_req_msg_psa_mac_abort_req_m;
		struct psa_cipher_encrypt_req psa_crypto_req_msg_psa_cipher_encrypt_req_m;
		struct psa_cipher_decrypt_req psa_crypto_req_msg_psa_cipher_decrypt_req_m;
		struct psa_cipher_encrypt_setup_req
			psa_crypto_req_msg_psa_cipher_encrypt_setup_req_m;
		struct psa_cipher_decrypt_setup_req
			psa_crypto_req_msg_psa_cipher_decrypt_setup_req_m;
		struct psa_cipher_generate_iv_req psa_crypto_req_msg_psa_cipher_generate_iv_req_m;
		struct psa_cipher_set_iv_req psa_crypto_req_msg_psa_cipher_set_iv_req_m;
		struct psa_cipher_update_req psa_crypto_req_msg_psa_cipher_update_req_m;
		struct psa_cipher_finish_req psa_crypto_req_msg_psa_cipher_finish_req_m;
		struct psa_cipher_abort_req psa_crypto_req_msg_psa_cipher_abort_req_m;
		struct psa_aead_encrypt_req psa_crypto_req_msg_psa_aead_encrypt_req_m;
		struct psa_aead_decrypt_req psa_crypto_req_msg_psa_aead_decrypt_req_m;
		struct psa_aead_encrypt_setup_req psa_crypto_req_msg_psa_aead_encrypt_setup_req_m;
		struct psa_aead_decrypt_setup_req psa_crypto_req_msg_psa_aead_decrypt_setup_req_m;
		struct psa_aead_generate_nonce_req psa_crypto_req_msg_psa_aead_generate_nonce_req_m;
		struct psa_aead_set_nonce_req psa_crypto_req_msg_psa_aead_set_nonce_req_m;
		struct psa_aead_set_lengths_req psa_crypto_req_msg_psa_aead_set_lengths_req_m;
		struct psa_aead_update_ad_req psa_crypto_req_msg_psa_aead_update_ad_req_m;
		struct psa_aead_update_req psa_crypto_req_msg_psa_aead_update_req_m;
		struct psa_aead_finish_req psa_crypto_req_msg_psa_aead_finish_req_m;
		struct psa_aead_verify_req psa_crypto_req_msg_psa_aead_verify_req_m;
		struct psa_aead_abort_req psa_crypto_req_msg_psa_aead_abort_req_m;
		struct psa_sign_message_req psa_crypto_req_msg_psa_sign_message_req_m;
		struct psa_verify_message_req psa_crypto_req_msg_psa_verify_message_req_m;
		struct psa_sign_hash_req psa_crypto_req_msg_psa_sign_hash_req_m;
		struct psa_verify_hash_req psa_crypto_req_msg_psa_verify_hash_req_m;
		struct psa_asymmetric_encrypt_req psa_crypto_req_msg_psa_asymmetric_encrypt_req_m;
		struct psa_asymmetric_decrypt_req psa_crypto_req_msg_psa_asymmetric_decrypt_req_m;
		struct psa_key_derivation_setup_req
			psa_crypto_req_msg_psa_key_derivation_setup_req_m;
		struct psa_key_derivation_get_capacity_req
			psa_crypto_req_msg_psa_key_derivation_get_capacity_req_m;
		struct psa_key_derivation_set_capacity_req
			psa_crypto_req_msg_psa_key_derivation_set_capacity_req_m;
		struct psa_key_derivation_input_bytes_req
			psa_crypto_req_msg_psa_key_derivation_input_bytes_req_m;
		struct psa_key_derivation_input_integer_req
			psa_crypto_req_msg_psa_key_derivation_input_integer_req_m;
		struct psa_key_derivation_input_key_req
			psa_crypto_req_msg_psa_key_derivation_input_key_req_m;
		struct psa_key_derivation_key_agreement_req
			psa_crypto_req_msg_psa_key_derivation_key_agreement_req_m;
		struct psa_key_derivation_output_bytes_req
			psa_crypto_req_msg_psa_key_derivation_output_bytes_req_m;
		struct psa_key_derivation_output_key_req
			psa_crypto_req_msg_psa_key_derivation_output_key_req_m;
		struct psa_key_derivation_abort_req
			psa_crypto_req_msg_psa_key_derivation_abort_req_m;
		struct psa_raw_key_agreement_req psa_crypto_req_msg_psa_raw_key_agreement_req_m;
		struct psa_generate_random_req psa_crypto_req_msg_psa_generate_random_req_m;
		struct psa_generate_key_req psa_crypto_req_msg_psa_generate_key_req_m;
		struct psa_sign_hash_start_req psa_crypto_req_msg_psa_sign_hash_start_req_m;
		struct psa_sign_hash_abort_req psa_crypto_req_msg_psa_sign_hash_abort_req_m;
		struct psa_verify_hash_start_req psa_crypto_req_msg_psa_verify_hash_start_req_m;
		struct psa_verify_hash_abort_req psa_crypto_req_msg_psa_verify_hash_abort_req_m;
		struct psa_pake_setup_req psa_crypto_req_msg_psa_pake_setup_req_m;
		struct psa_pake_set_role_req psa_crypto_req_msg_psa_pake_set_role_req_m;
		struct psa_pake_set_user_req psa_crypto_req_msg_psa_pake_set_user_req_m;
		struct psa_pake_set_peer_req psa_crypto_req_msg_psa_pake_set_peer_req_m;
		struct psa_pake_set_context_req psa_crypto_req_msg_psa_pake_set_context_req_m;
		struct psa_pake_output_req psa_crypto_req_msg_psa_pake_output_req_m;
		struct psa_pake_input_req psa_crypto_req_msg_psa_pake_input_req_m;
		struct psa_pake_get_shared_key_req psa_crypto_req_msg_psa_pake_get_shared_key_req_m;
		struct psa_pake_abort_req psa_crypto_req_msg_psa_pake_abort_req_m;
	};
	enum {
		psa_crypto_req_msg_psa_crypto_init_req_m_c,
		psa_crypto_req_msg_psa_get_key_attributes_req_m_c,
		psa_crypto_req_msg_psa_reset_key_attributes_req_m_c,
		psa_crypto_req_msg_psa_purge_key_req_m_c,
		psa_crypto_req_msg_psa_copy_key_req_m_c,
		psa_crypto_req_msg_psa_destroy_key_req_m_c,
		psa_crypto_req_msg_psa_import_key_req_m_c,
		psa_crypto_req_msg_psa_export_key_req_m_c,
		psa_crypto_req_msg_psa_export_public_key_req_m_c,
		psa_crypto_req_msg_psa_hash_compute_req_m_c,
		psa_crypto_req_msg_psa_hash_compare_req_m_c,
		psa_crypto_req_msg_psa_hash_setup_req_m_c,
		psa_crypto_req_msg_psa_hash_update_req_m_c,
		psa_crypto_req_msg_psa_hash_finish_req_m_c,
		psa_crypto_req_msg_psa_hash_verify_req_m_c,
		psa_crypto_req_msg_psa_hash_abort_req_m_c,
		psa_crypto_req_msg_psa_hash_clone_req_m_c,
		psa_crypto_req_msg_psa_mac_compute_req_m_c,
		psa_crypto_req_msg_psa_mac_verify_req_m_c,
		psa_crypto_req_msg_psa_mac_sign_setup_req_m_c,
		psa_crypto_req_msg_psa_mac_verify_setup_req_m_c,
		psa_crypto_req_msg_psa_mac_update_req_m_c,
		psa_crypto_req_msg_psa_mac_sign_finish_req_m_c,
		psa_crypto_req_msg_psa_mac_verify_finish_req_m_c,
		psa_crypto_req_msg_psa_mac_abort_req_m_c,
		psa_crypto_req_msg_psa_cipher_encrypt_req_m_c,
		psa_crypto_req_msg_psa_cipher_decrypt_req_m_c,
		psa_crypto_req_msg_psa_cipher_encrypt_setup_req_m_c,
		psa_crypto_req_msg_psa_cipher_decrypt_setup_req_m_c,
		psa_crypto_req_msg_psa_cipher_generate_iv_req_m_c,
		psa_crypto_req_msg_psa_cipher_set_iv_req_m_c,
		psa_crypto_req_msg_psa_cipher_update_req_m_c,
		psa_crypto_req_msg_psa_cipher_finish_req_m_c,
		psa_crypto_req_msg_psa_cipher_abort_req_m_c,
		psa_crypto_req_msg_psa_aead_encrypt_req_m_c,
		psa_crypto_req_msg_psa_aead_decrypt_req_m_c,
		psa_crypto_req_msg_psa_aead_encrypt_setup_req_m_c,
		psa_crypto_req_msg_psa_aead_decrypt_setup_req_m_c,
		psa_crypto_req_msg_psa_aead_generate_nonce_req_m_c,
		psa_crypto_req_msg_psa_aead_set_nonce_req_m_c,
		psa_crypto_req_msg_psa_aead_set_lengths_req_m_c,
		psa_crypto_req_msg_psa_aead_update_ad_req_m_c,
		psa_crypto_req_msg_psa_aead_update_req_m_c,
		psa_crypto_req_msg_psa_aead_finish_req_m_c,
		psa_crypto_req_msg_psa_aead_verify_req_m_c,
		psa_crypto_req_msg_psa_aead_abort_req_m_c,
		psa_crypto_req_msg_psa_sign_message_req_m_c,
		psa_crypto_req_msg_psa_verify_message_req_m_c,
		psa_crypto_req_msg_psa_sign_hash_req_m_c,
		psa_crypto_req_msg_psa_verify_hash_req_m_c,
		psa_crypto_req_msg_psa_asymmetric_encrypt_req_m_c,
		psa_crypto_req_msg_psa_asymmetric_decrypt_req_m_c,
		psa_crypto_req_msg_psa_key_derivation_setup_req_m_c,
		psa_crypto_req_msg_psa_key_derivation_get_capacity_req_m_c,
		psa_crypto_req_msg_psa_key_derivation_set_capacity_req_m_c,
		psa_crypto_req_msg_psa_key_derivation_input_bytes_req_m_c,
		psa_crypto_req_msg_psa_key_derivation_input_integer_req_m_c,
		psa_crypto_req_msg_psa_key_derivation_input_key_req_m_c,
		psa_crypto_req_msg_psa_key_derivation_key_agreement_req_m_c,
		psa_crypto_req_msg_psa_key_derivation_output_bytes_req_m_c,
		psa_crypto_req_msg_psa_key_derivation_output_key_req_m_c,
		psa_crypto_req_msg_psa_key_derivation_abort_req_m_c,
		psa_crypto_req_msg_psa_raw_key_agreement_req_m_c,
		psa_crypto_req_msg_psa_generate_random_req_m_c,
		psa_crypto_req_msg_psa_generate_key_req_m_c,
		psa_crypto_req_msg_psa_sign_hash_start_req_m_c,
		psa_crypto_req_msg_psa_sign_hash_abort_req_m_c,
		psa_crypto_req_msg_psa_verify_hash_start_req_m_c,
		psa_crypto_req_msg_psa_verify_hash_abort_req_m_c,
		psa_crypto_req_msg_psa_pake_setup_req_m_c,
		psa_crypto_req_msg_psa_pake_set_role_req_m_c,
		psa_crypto_req_msg_psa_pake_set_user_req_m_c,
		psa_crypto_req_msg_psa_pake_set_peer_req_m_c,
		psa_crypto_req_msg_psa_pake_set_context_req_m_c,
		psa_crypto_req_msg_psa_pake_output_req_m_c,
		psa_crypto_req_msg_psa_pake_input_req_m_c,
		psa_crypto_req_msg_psa_pake_get_shared_key_req_m_c,
		psa_crypto_req_msg_psa_pake_abort_req_m_c,
	} psa_crypto_req_msg_choice;
};

#ifdef __cplusplus
}
#endif

#endif /* PSA_CRYPTO_SERVICE_TYPES_H__ */
