/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 * Generated using zcbor version 0.9.0
 * https://github.com/NordicSemiconductor/zcbor
 * Generated with a --default-max-qty of 3
 */

#include "psa_crypto_service_encode.h"
#include "zcbor_encode.h"
#include "zcbor_print.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#if DEFAULT_MAX_QTY != 3
#error                                                                         \
    "The type file was generated with a different default_max_qty than this file"
#endif

#define log_result(state, result, func)                                        \
  do {                                                                         \
    if (!result) {                                                             \
      zcbor_trace_file(state);                                                 \
      zcbor_log("%s error: %s\r\n", func,                                      \
                zcbor_error_str(zcbor_peek_error(state)));                     \
    } else {                                                                   \
      zcbor_log("%s success\r\n", func);                                       \
    }                                                                          \
  } while (0)

static bool encode_ptr_attr(zcbor_state_t *state, const uint32_t *input);
static bool encode_psa_get_key_attributes_req(
    zcbor_state_t *state, const struct psa_get_key_attributes_req *input);
static bool encode_psa_reset_key_attributes_req(
    zcbor_state_t *state, const struct psa_reset_key_attributes_req *input);
static bool encode_psa_purge_key_req(zcbor_state_t *state,
                                     const struct psa_purge_key_req *input);
static bool encode_ptr_key(zcbor_state_t *state, const uint32_t *input);
static bool encode_psa_copy_key_req(zcbor_state_t *state,
                                    const struct psa_copy_key_req *input);
static bool encode_psa_destroy_key_req(zcbor_state_t *state,
                                       const struct psa_destroy_key_req *input);
static bool encode_ptr_buf(zcbor_state_t *state, const uint32_t *input);
static bool encode_buf_len(zcbor_state_t *state, const uint32_t *input);
static bool encode_psa_import_key_req(zcbor_state_t *state,
                                      const struct psa_import_key_req *input);
static bool encode_ptr_uint(zcbor_state_t *state, const uint32_t *input);
static bool encode_psa_export_key_req(zcbor_state_t *state,
                                      const struct psa_export_key_req *input);
static bool
encode_psa_export_public_key_req(zcbor_state_t *state,
                                 const struct psa_export_public_key_req *input);
static bool
encode_psa_hash_compute_req(zcbor_state_t *state,
                            const struct psa_hash_compute_req *input);
static bool
encode_psa_hash_compare_req(zcbor_state_t *state,
                            const struct psa_hash_compare_req *input);
static bool encode_psa_hash_setup_req(zcbor_state_t *state,
                                      const struct psa_hash_setup_req *input);
static bool encode_psa_hash_update_req(zcbor_state_t *state,
                                       const struct psa_hash_update_req *input);
static bool encode_psa_hash_finish_req(zcbor_state_t *state,
                                       const struct psa_hash_finish_req *input);
static bool encode_psa_hash_verify_req(zcbor_state_t *state,
                                       const struct psa_hash_verify_req *input);
static bool encode_psa_hash_abort_req(zcbor_state_t *state,
                                      const struct psa_hash_abort_req *input);
static bool encode_psa_hash_clone_req(zcbor_state_t *state,
                                      const struct psa_hash_clone_req *input);
static bool encode_psa_mac_compute_req(zcbor_state_t *state,
                                       const struct psa_mac_compute_req *input);
static bool encode_psa_mac_verify_req(zcbor_state_t *state,
                                      const struct psa_mac_verify_req *input);
static bool
encode_psa_mac_sign_setup_req(zcbor_state_t *state,
                              const struct psa_mac_sign_setup_req *input);
static bool
encode_psa_mac_verify_setup_req(zcbor_state_t *state,
                                const struct psa_mac_verify_setup_req *input);
static bool encode_psa_mac_update_req(zcbor_state_t *state,
                                      const struct psa_mac_update_req *input);
static bool
encode_psa_mac_sign_finish_req(zcbor_state_t *state,
                               const struct psa_mac_sign_finish_req *input);
static bool
encode_psa_mac_verify_finish_req(zcbor_state_t *state,
                                 const struct psa_mac_verify_finish_req *input);
static bool encode_psa_mac_abort_req(zcbor_state_t *state,
                                     const struct psa_mac_abort_req *input);
static bool
encode_psa_cipher_encrypt_req(zcbor_state_t *state,
                              const struct psa_cipher_encrypt_req *input);
static bool
encode_psa_cipher_decrypt_req(zcbor_state_t *state,
                              const struct psa_cipher_decrypt_req *input);
static bool encode_psa_cipher_encrypt_setup_req(
    zcbor_state_t *state, const struct psa_cipher_encrypt_setup_req *input);
static bool encode_psa_cipher_decrypt_setup_req(
    zcbor_state_t *state, const struct psa_cipher_decrypt_setup_req *input);
static bool encode_psa_cipher_generate_iv_req(
    zcbor_state_t *state, const struct psa_cipher_generate_iv_req *input);
static bool
encode_psa_cipher_set_iv_req(zcbor_state_t *state,
                             const struct psa_cipher_set_iv_req *input);
static bool
encode_psa_cipher_update_req(zcbor_state_t *state,
                             const struct psa_cipher_update_req *input);
static bool
encode_psa_cipher_finish_req(zcbor_state_t *state,
                             const struct psa_cipher_finish_req *input);
static bool
encode_psa_cipher_abort_req(zcbor_state_t *state,
                            const struct psa_cipher_abort_req *input);
static bool
encode_psa_aead_encrypt_req(zcbor_state_t *state,
                            const struct psa_aead_encrypt_req *input);
static bool
encode_psa_aead_decrypt_req(zcbor_state_t *state,
                            const struct psa_aead_decrypt_req *input);
static bool encode_psa_aead_encrypt_setup_req(
    zcbor_state_t *state, const struct psa_aead_encrypt_setup_req *input);
static bool encode_psa_aead_decrypt_setup_req(
    zcbor_state_t *state, const struct psa_aead_decrypt_setup_req *input);
static bool encode_psa_aead_generate_nonce_req(
    zcbor_state_t *state, const struct psa_aead_generate_nonce_req *input);
static bool
encode_psa_aead_set_nonce_req(zcbor_state_t *state,
                              const struct psa_aead_set_nonce_req *input);
static bool
encode_psa_aead_set_lengths_req(zcbor_state_t *state,
                                const struct psa_aead_set_lengths_req *input);
static bool
encode_psa_aead_update_ad_req(zcbor_state_t *state,
                              const struct psa_aead_update_ad_req *input);
static bool encode_psa_aead_update_req(zcbor_state_t *state,
                                       const struct psa_aead_update_req *input);
static bool encode_psa_aead_finish_req(zcbor_state_t *state,
                                       const struct psa_aead_finish_req *input);
static bool encode_psa_aead_verify_req(zcbor_state_t *state,
                                       const struct psa_aead_verify_req *input);
static bool encode_psa_aead_abort_req(zcbor_state_t *state,
                                      const struct psa_aead_abort_req *input);
static bool
encode_psa_sign_message_req(zcbor_state_t *state,
                            const struct psa_sign_message_req *input);
static bool
encode_psa_verify_message_req(zcbor_state_t *state,
                              const struct psa_verify_message_req *input);
static bool encode_psa_sign_hash_req(zcbor_state_t *state,
                                     const struct psa_sign_hash_req *input);
static bool encode_psa_verify_hash_req(zcbor_state_t *state,
                                       const struct psa_verify_hash_req *input);
static bool encode_psa_asymmetric_encrypt_req(
    zcbor_state_t *state, const struct psa_asymmetric_encrypt_req *input);
static bool encode_psa_asymmetric_decrypt_req(
    zcbor_state_t *state, const struct psa_asymmetric_decrypt_req *input);
static bool encode_psa_key_derivation_setup_req(
    zcbor_state_t *state, const struct psa_key_derivation_setup_req *input);
static bool encode_psa_key_derivation_get_capacity_req(
    zcbor_state_t *state,
    const struct psa_key_derivation_get_capacity_req *input);
static bool encode_psa_key_derivation_set_capacity_req(
    zcbor_state_t *state,
    const struct psa_key_derivation_set_capacity_req *input);
static bool encode_psa_key_derivation_input_bytes_req(
    zcbor_state_t *state,
    const struct psa_key_derivation_input_bytes_req *input);
static bool encode_psa_key_derivation_input_integer_req(
    zcbor_state_t *state,
    const struct psa_key_derivation_input_integer_req *input);
static bool encode_psa_key_derivation_input_key_req(
    zcbor_state_t *state, const struct psa_key_derivation_input_key_req *input);
static bool encode_psa_key_derivation_key_agreement_req(
    zcbor_state_t *state,
    const struct psa_key_derivation_key_agreement_req *input);
static bool encode_psa_key_derivation_output_bytes_req(
    zcbor_state_t *state,
    const struct psa_key_derivation_output_bytes_req *input);
static bool encode_psa_key_derivation_output_key_req(
    zcbor_state_t *state,
    const struct psa_key_derivation_output_key_req *input);
static bool encode_psa_key_derivation_abort_req(
    zcbor_state_t *state, const struct psa_key_derivation_abort_req *input);
static bool
encode_psa_raw_key_agreement_req(zcbor_state_t *state,
                                 const struct psa_raw_key_agreement_req *input);
static bool
encode_psa_generate_random_req(zcbor_state_t *state,
                               const struct psa_generate_random_req *input);
static bool
encode_psa_generate_key_req(zcbor_state_t *state,
                            const struct psa_generate_key_req *input);
static bool encode_ptr_cipher(zcbor_state_t *state, const uint32_t *input);
static bool encode_psa_pake_setup_req(zcbor_state_t *state,
                                      const struct psa_pake_setup_req *input);
static bool
encode_psa_pake_set_role_req(zcbor_state_t *state,
                             const struct psa_pake_set_role_req *input);
static bool
encode_psa_pake_set_user_req(zcbor_state_t *state,
                             const struct psa_pake_set_user_req *input);
static bool
encode_psa_pake_set_peer_req(zcbor_state_t *state,
                             const struct psa_pake_set_peer_req *input);
static bool
encode_psa_pake_set_context_req(zcbor_state_t *state,
                                const struct psa_pake_set_context_req *input);
static bool encode_psa_pake_output_req(zcbor_state_t *state,
                                       const struct psa_pake_output_req *input);
static bool encode_psa_pake_input_req(zcbor_state_t *state,
                                      const struct psa_pake_input_req *input);
static bool encode_psa_pake_get_shared_key_req(
    zcbor_state_t *state, const struct psa_pake_get_shared_key_req *input);
static bool encode_psa_pake_abort_req(zcbor_state_t *state,
                                      const struct psa_pake_abort_req *input);
static bool encode_psa_crypto_rsp(zcbor_state_t *state,
                                  const struct psa_crypto_rsp *input);
static bool encode_psa_crypto_req(zcbor_state_t *state,
                                  const struct psa_crypto_req *input);

static bool encode_ptr_attr(zcbor_state_t *state, const uint32_t *input) {
  zcbor_log("%s\r\n", __func__);

  bool res = ((zcbor_tag_put(state, 32772) &&
               (zcbor_uint32_encode(state, (&(*input))))));

  log_result(state, res, __func__);
  return res;
}

static bool encode_psa_get_key_attributes_req(
    zcbor_state_t *state, const struct psa_get_key_attributes_req *input) {
  zcbor_log("%s\r\n", __func__);

  bool res =
      (((((zcbor_uint32_put(state, (11)))) &&
         ((zcbor_uint32_encode(state,
                               (&(*input).psa_get_key_attributes_req_key)))) &&
         ((encode_ptr_attr(
             state, (&(*input).psa_get_key_attributes_req_p_attributes)))))));

  log_result(state, res, __func__);
  return res;
}

static bool encode_psa_reset_key_attributes_req(
    zcbor_state_t *state, const struct psa_reset_key_attributes_req *input) {
  zcbor_log("%s\r\n", __func__);

  bool res =
      (((((zcbor_uint32_put(state, (12)))) &&
         ((encode_ptr_attr(
             state, (&(*input).psa_reset_key_attributes_req_p_attributes)))))));

  log_result(state, res, __func__);
  return res;
}

static bool encode_psa_purge_key_req(zcbor_state_t *state,
                                     const struct psa_purge_key_req *input) {
  zcbor_log("%s\r\n", __func__);

  bool res =
      (((((zcbor_uint32_put(state, (13)))) &&
         ((zcbor_uint32_encode(state, (&(*input).psa_purge_key_req_key)))))));

  log_result(state, res, __func__);
  return res;
}

static bool encode_ptr_key(zcbor_state_t *state, const uint32_t *input) {
  zcbor_log("%s\r\n", __func__);

  bool res = ((zcbor_tag_put(state, 32773) &&
               (zcbor_uint32_encode(state, (&(*input))))));

  log_result(state, res, __func__);
  return res;
}

static bool encode_psa_copy_key_req(zcbor_state_t *state,
                                    const struct psa_copy_key_req *input) {
  zcbor_log("%s\r\n", __func__);

  bool res = (((
      ((zcbor_uint32_put(state, (14)))) &&
      ((zcbor_uint32_encode(state, (&(*input).psa_copy_key_req_source_key)))) &&
      ((encode_ptr_attr(state, (&(*input).psa_copy_key_req_p_attributes)))) &&
      ((encode_ptr_key(state, (&(*input).psa_copy_key_req_p_target_key)))))));

  log_result(state, res, __func__);
  return res;
}

static bool
encode_psa_destroy_key_req(zcbor_state_t *state,
                           const struct psa_destroy_key_req *input) {
  zcbor_log("%s\r\n", __func__);

  bool res =
      (((((zcbor_uint32_put(state, (15)))) &&
         ((zcbor_uint32_encode(state, (&(*input).psa_destroy_key_req_key)))))));

  log_result(state, res, __func__);
  return res;
}

static bool encode_ptr_buf(zcbor_state_t *state, const uint32_t *input) {
  zcbor_log("%s\r\n", __func__);

  bool res = ((zcbor_tag_put(state, 32770) &&
               (zcbor_uint32_encode(state, (&(*input))))));

  log_result(state, res, __func__);
  return res;
}

static bool encode_buf_len(zcbor_state_t *state, const uint32_t *input) {
  zcbor_log("%s\r\n", __func__);

  bool res = ((zcbor_tag_put(state, 32771) &&
               (zcbor_uint32_encode(state, (&(*input))))));

  log_result(state, res, __func__);
  return res;
}

static bool encode_psa_import_key_req(zcbor_state_t *state,
                                      const struct psa_import_key_req *input) {
  zcbor_log("%s\r\n", __func__);

  bool res = (((
      ((zcbor_uint32_put(state, (16)))) &&
      ((encode_ptr_attr(state, (&(*input).psa_import_key_req_p_attributes)))) &&
      ((encode_ptr_buf(state, (&(*input).psa_import_key_req_p_data)))) &&
      ((encode_buf_len(state, (&(*input).psa_import_key_req_data_length)))) &&
      ((encode_ptr_key(state, (&(*input).psa_import_key_req_p_key)))))));

  log_result(state, res, __func__);
  return res;
}

static bool encode_ptr_uint(zcbor_state_t *state, const uint32_t *input) {
  zcbor_log("%s\r\n", __func__);

  bool res = ((zcbor_tag_put(state, 32774) &&
               (zcbor_uint32_encode(state, (&(*input))))));

  log_result(state, res, __func__);
  return res;
}

static bool encode_psa_export_key_req(zcbor_state_t *state,
                                      const struct psa_export_key_req *input) {
  zcbor_log("%s\r\n", __func__);

  bool res =
      (((((zcbor_uint32_put(state, (17)))) &&
         ((zcbor_uint32_encode(state, (&(*input).psa_export_key_req_key)))) &&
         ((encode_ptr_buf(state, (&(*input).psa_export_key_req_p_data)))) &&
         ((encode_buf_len(state, (&(*input).psa_export_key_req_data_size)))) &&
         ((encode_ptr_uint(state,
                           (&(*input).psa_export_key_req_p_data_length)))))));

  log_result(state, res, __func__);
  return res;
}

static bool encode_psa_export_public_key_req(
    zcbor_state_t *state, const struct psa_export_public_key_req *input) {
  zcbor_log("%s\r\n", __func__);

  bool res = (((
      ((zcbor_uint32_put(state, (18)))) &&
      ((zcbor_uint32_encode(state,
                            (&(*input).psa_export_public_key_req_key)))) &&
      ((encode_ptr_buf(state, (&(*input).psa_export_public_key_req_p_data)))) &&
      ((encode_buf_len(state,
                       (&(*input).psa_export_public_key_req_data_size)))) &&
      ((encode_ptr_uint(
          state, (&(*input).psa_export_public_key_req_p_data_length)))))));

  log_result(state, res, __func__);
  return res;
}

static bool
encode_psa_hash_compute_req(zcbor_state_t *state,
                            const struct psa_hash_compute_req *input) {
  zcbor_log("%s\r\n", __func__);

  bool res = ((
      (((zcbor_uint32_put(state, (19)))) &&
       ((zcbor_uint32_encode(state, (&(*input).psa_hash_compute_req_alg)))) &&
       ((encode_ptr_buf(state, (&(*input).psa_hash_compute_req_p_input)))) &&
       ((encode_buf_len(state,
                        (&(*input).psa_hash_compute_req_input_length)))) &&
       ((encode_ptr_buf(state, (&(*input).psa_hash_compute_req_p_hash)))) &&
       ((encode_buf_len(state, (&(*input).psa_hash_compute_req_hash_size)))) &&
       ((encode_ptr_uint(state,
                         (&(*input).psa_hash_compute_req_p_hash_length)))))));

  log_result(state, res, __func__);
  return res;
}

static bool
encode_psa_hash_compare_req(zcbor_state_t *state,
                            const struct psa_hash_compare_req *input) {
  zcbor_log("%s\r\n", __func__);

  bool res =
      (((((zcbor_uint32_put(state, (20)))) &&
         ((zcbor_uint32_encode(state, (&(*input).psa_hash_compare_req_alg)))) &&
         ((encode_ptr_buf(state, (&(*input).psa_hash_compare_req_p_input)))) &&
         ((encode_buf_len(state,
                          (&(*input).psa_hash_compare_req_input_length)))) &&
         ((encode_ptr_buf(state, (&(*input).psa_hash_compare_req_p_hash)))) &&
         ((encode_buf_len(state,
                          (&(*input).psa_hash_compare_req_hash_length)))))));

  log_result(state, res, __func__);
  return res;
}

static bool encode_psa_hash_setup_req(zcbor_state_t *state,
                                      const struct psa_hash_setup_req *input) {
  zcbor_log("%s\r\n", __func__);

  bool res =
      (((((zcbor_uint32_put(state, (21)))) &&
         ((encode_ptr_uint(state, (&(*input).psa_hash_setup_req_p_handle)))) &&
         ((zcbor_uint32_encode(state, (&(*input).psa_hash_setup_req_alg)))))));

  log_result(state, res, __func__);
  return res;
}

static bool
encode_psa_hash_update_req(zcbor_state_t *state,
                           const struct psa_hash_update_req *input) {
  zcbor_log("%s\r\n", __func__);

  bool res =
      (((((zcbor_uint32_put(state, (22)))) &&
         ((encode_ptr_uint(state, (&(*input).psa_hash_update_req_p_handle)))) &&
         ((encode_ptr_buf(state, (&(*input).psa_hash_update_req_p_input)))) &&
         ((encode_buf_len(state,
                          (&(*input).psa_hash_update_req_input_length)))))));

  log_result(state, res, __func__);
  return res;
}

static bool
encode_psa_hash_finish_req(zcbor_state_t *state,
                           const struct psa_hash_finish_req *input) {
  zcbor_log("%s\r\n", __func__);

  bool res =
      (((((zcbor_uint32_put(state, (23)))) &&
         ((encode_ptr_uint(state, (&(*input).psa_hash_finish_req_p_handle)))) &&
         ((encode_ptr_buf(state, (&(*input).psa_hash_finish_req_p_hash)))) &&
         ((encode_buf_len(state, (&(*input).psa_hash_finish_req_hash_size)))) &&
         ((encode_ptr_uint(state,
                           (&(*input).psa_hash_finish_req_p_hash_length)))))));

  log_result(state, res, __func__);
  return res;
}

static bool
encode_psa_hash_verify_req(zcbor_state_t *state,
                           const struct psa_hash_verify_req *input) {
  zcbor_log("%s\r\n", __func__);

  bool res = (((
      ((zcbor_uint32_put(state, (24)))) &&
      ((encode_ptr_uint(state, (&(*input).psa_hash_verify_req_p_handle)))) &&
      ((encode_ptr_buf(state, (&(*input).psa_hash_verify_req_p_hash)))) &&
      ((encode_buf_len(state, (&(*input).psa_hash_verify_req_hash_length)))))));

  log_result(state, res, __func__);
  return res;
}

static bool encode_psa_hash_abort_req(zcbor_state_t *state,
                                      const struct psa_hash_abort_req *input) {
  zcbor_log("%s\r\n", __func__);

  bool res =
      (((((zcbor_uint32_put(state, (25)))) &&
         ((encode_ptr_uint(state, (&(*input).psa_hash_abort_req_p_handle)))))));

  log_result(state, res, __func__);
  return res;
}

static bool encode_psa_hash_clone_req(zcbor_state_t *state,
                                      const struct psa_hash_clone_req *input) {
  zcbor_log("%s\r\n", __func__);

  bool res = ((
      (((zcbor_uint32_put(state, (26)))) &&
       ((zcbor_uint32_encode(state, (&(*input).psa_hash_clone_req_handle)))) &&
       ((encode_ptr_uint(state, (&(*input).psa_hash_clone_req_p_handle)))))));

  log_result(state, res, __func__);
  return res;
}

static bool
encode_psa_mac_compute_req(zcbor_state_t *state,
                           const struct psa_mac_compute_req *input) {
  zcbor_log("%s\r\n", __func__);

  bool res = (((
      ((zcbor_uint32_put(state, (27)))) &&
      ((zcbor_uint32_encode(state, (&(*input).psa_mac_compute_req_key)))) &&
      ((zcbor_uint32_encode(state, (&(*input).psa_mac_compute_req_alg)))) &&
      ((encode_ptr_buf(state, (&(*input).psa_mac_compute_req_p_input)))) &&
      ((encode_buf_len(state, (&(*input).psa_mac_compute_req_input_length)))) &&
      ((encode_ptr_buf(state, (&(*input).psa_mac_compute_req_p_mac)))) &&
      ((encode_buf_len(state, (&(*input).psa_mac_compute_req_mac_size)))) &&
      ((encode_ptr_uint(state,
                        (&(*input).psa_mac_compute_req_p_mac_length)))))));

  log_result(state, res, __func__);
  return res;
}

static bool encode_psa_mac_verify_req(zcbor_state_t *state,
                                      const struct psa_mac_verify_req *input) {
  zcbor_log("%s\r\n", __func__);

  bool res = ((
      (((zcbor_uint32_put(state, (28)))) &&
       ((zcbor_uint32_encode(state, (&(*input).psa_mac_verify_req_key)))) &&
       ((zcbor_uint32_encode(state, (&(*input).psa_mac_verify_req_alg)))) &&
       ((encode_ptr_buf(state, (&(*input).psa_mac_verify_req_p_input)))) &&
       ((encode_buf_len(state, (&(*input).psa_mac_verify_req_input_length)))) &&
       ((encode_ptr_buf(state, (&(*input).psa_mac_verify_req_p_mac)))) &&
       ((encode_buf_len(state, (&(*input).psa_mac_verify_req_mac_length)))))));

  log_result(state, res, __func__);
  return res;
}

static bool
encode_psa_mac_sign_setup_req(zcbor_state_t *state,
                              const struct psa_mac_sign_setup_req *input) {
  zcbor_log("%s\r\n", __func__);

  bool res = (((
      ((zcbor_uint32_put(state, (29)))) &&
      ((encode_ptr_uint(state, (&(*input).psa_mac_sign_setup_req_p_handle)))) &&
      ((zcbor_uint32_encode(state, (&(*input).psa_mac_sign_setup_req_key)))) &&
      ((zcbor_uint32_encode(state, (&(*input).psa_mac_sign_setup_req_alg)))))));

  log_result(state, res, __func__);
  return res;
}

static bool
encode_psa_mac_verify_setup_req(zcbor_state_t *state,
                                const struct psa_mac_verify_setup_req *input) {
  zcbor_log("%s\r\n", __func__);

  bool res =
      (((((zcbor_uint32_put(state, (30)))) &&
         ((encode_ptr_uint(state,
                           (&(*input).psa_mac_verify_setup_req_p_handle)))) &&
         ((zcbor_uint32_encode(state,
                               (&(*input).psa_mac_verify_setup_req_key)))) &&
         ((zcbor_uint32_encode(state,
                               (&(*input).psa_mac_verify_setup_req_alg)))))));

  log_result(state, res, __func__);
  return res;
}

static bool encode_psa_mac_update_req(zcbor_state_t *state,
                                      const struct psa_mac_update_req *input) {
  zcbor_log("%s\r\n", __func__);

  bool res = (((
      ((zcbor_uint32_put(state, (31)))) &&
      ((encode_ptr_uint(state, (&(*input).psa_mac_update_req_p_handle)))) &&
      ((encode_ptr_buf(state, (&(*input).psa_mac_update_req_p_input)))) &&
      ((encode_buf_len(state, (&(*input).psa_mac_update_req_input_length)))))));

  log_result(state, res, __func__);
  return res;
}

static bool
encode_psa_mac_sign_finish_req(zcbor_state_t *state,
                               const struct psa_mac_sign_finish_req *input) {
  zcbor_log("%s\r\n", __func__);

  bool res = (((
      ((zcbor_uint32_put(state, (32)))) &&
      ((encode_ptr_uint(state,
                        (&(*input).psa_mac_sign_finish_req_p_handle)))) &&
      ((encode_ptr_buf(state, (&(*input).psa_mac_sign_finish_req_p_mac)))) &&
      ((encode_buf_len(state, (&(*input).psa_mac_sign_finish_req_mac_size)))) &&
      ((encode_ptr_uint(state,
                        (&(*input).psa_mac_sign_finish_req_p_mac_length)))))));

  log_result(state, res, __func__);
  return res;
}

static bool encode_psa_mac_verify_finish_req(
    zcbor_state_t *state, const struct psa_mac_verify_finish_req *input) {
  zcbor_log("%s\r\n", __func__);

  bool res = ((
      (((zcbor_uint32_put(state, (33)))) &&
       ((encode_ptr_uint(state,
                         (&(*input).psa_mac_verify_finish_req_p_handle)))) &&
       ((encode_ptr_buf(state, (&(*input).psa_mac_verify_finish_req_p_mac)))) &&
       ((encode_buf_len(state,
                        (&(*input).psa_mac_verify_finish_req_mac_length)))))));

  log_result(state, res, __func__);
  return res;
}

static bool encode_psa_mac_abort_req(zcbor_state_t *state,
                                     const struct psa_mac_abort_req *input) {
  zcbor_log("%s\r\n", __func__);

  bool res =
      (((((zcbor_uint32_put(state, (34)))) &&
         ((encode_ptr_uint(state, (&(*input).psa_mac_abort_req_p_handle)))))));

  log_result(state, res, __func__);
  return res;
}

static bool
encode_psa_cipher_encrypt_req(zcbor_state_t *state,
                              const struct psa_cipher_encrypt_req *input) {
  zcbor_log("%s\r\n", __func__);

  bool res = ((
      (((zcbor_uint32_put(state, (35)))) &&
       ((zcbor_uint32_encode(state, (&(*input).psa_cipher_encrypt_req_key)))) &&
       ((zcbor_uint32_encode(state, (&(*input).psa_cipher_encrypt_req_alg)))) &&
       ((encode_ptr_buf(state, (&(*input).psa_cipher_encrypt_req_p_input)))) &&
       ((encode_buf_len(state,
                        (&(*input).psa_cipher_encrypt_req_input_length)))) &&
       ((encode_ptr_buf(state, (&(*input).psa_cipher_encrypt_req_p_output)))) &&
       ((encode_buf_len(state,
                        (&(*input).psa_cipher_encrypt_req_output_size)))) &&
       ((encode_ptr_uint(
           state, (&(*input).psa_cipher_encrypt_req_p_output_length)))))));

  log_result(state, res, __func__);
  return res;
}

static bool
encode_psa_cipher_decrypt_req(zcbor_state_t *state,
                              const struct psa_cipher_decrypt_req *input) {
  zcbor_log("%s\r\n", __func__);

  bool res = ((
      (((zcbor_uint32_put(state, (36)))) &&
       ((zcbor_uint32_encode(state, (&(*input).psa_cipher_decrypt_req_key)))) &&
       ((zcbor_uint32_encode(state, (&(*input).psa_cipher_decrypt_req_alg)))) &&
       ((encode_ptr_buf(state, (&(*input).psa_cipher_decrypt_req_p_input)))) &&
       ((encode_buf_len(state,
                        (&(*input).psa_cipher_decrypt_req_input_length)))) &&
       ((encode_ptr_buf(state, (&(*input).psa_cipher_decrypt_req_p_output)))) &&
       ((encode_buf_len(state,
                        (&(*input).psa_cipher_decrypt_req_output_size)))) &&
       ((encode_ptr_uint(
           state, (&(*input).psa_cipher_decrypt_req_p_output_length)))))));

  log_result(state, res, __func__);
  return res;
}

static bool encode_psa_cipher_encrypt_setup_req(
    zcbor_state_t *state, const struct psa_cipher_encrypt_setup_req *input) {
  zcbor_log("%s\r\n", __func__);

  bool res =
      (((((zcbor_uint32_put(state, (37)))) &&
         ((encode_ptr_uint(
             state, (&(*input).psa_cipher_encrypt_setup_req_p_handle)))) &&
         ((zcbor_uint32_encode(
             state, (&(*input).psa_cipher_encrypt_setup_req_key)))) &&
         ((zcbor_uint32_encode(
             state, (&(*input).psa_cipher_encrypt_setup_req_alg)))))));

  log_result(state, res, __func__);
  return res;
}

static bool encode_psa_cipher_decrypt_setup_req(
    zcbor_state_t *state, const struct psa_cipher_decrypt_setup_req *input) {
  zcbor_log("%s\r\n", __func__);

  bool res =
      (((((zcbor_uint32_put(state, (38)))) &&
         ((encode_ptr_uint(
             state, (&(*input).psa_cipher_decrypt_setup_req_p_handle)))) &&
         ((zcbor_uint32_encode(
             state, (&(*input).psa_cipher_decrypt_setup_req_key)))) &&
         ((zcbor_uint32_encode(
             state, (&(*input).psa_cipher_decrypt_setup_req_alg)))))));

  log_result(state, res, __func__);
  return res;
}

static bool encode_psa_cipher_generate_iv_req(
    zcbor_state_t *state, const struct psa_cipher_generate_iv_req *input) {
  zcbor_log("%s\r\n", __func__);

  bool res = ((
      (((zcbor_uint32_put(state, (39)))) &&
       ((encode_ptr_uint(state,
                         (&(*input).psa_cipher_generate_iv_req_p_handle)))) &&
       ((encode_ptr_buf(state, (&(*input).psa_cipher_generate_iv_req_p_iv)))) &&
       ((encode_buf_len(state,
                        (&(*input).psa_cipher_generate_iv_req_iv_size)))) &&
       ((encode_ptr_uint(
           state, (&(*input).psa_cipher_generate_iv_req_p_iv_length)))))));

  log_result(state, res, __func__);
  return res;
}

static bool
encode_psa_cipher_set_iv_req(zcbor_state_t *state,
                             const struct psa_cipher_set_iv_req *input) {
  zcbor_log("%s\r\n", __func__);

  bool res = (((
      ((zcbor_uint32_put(state, (40)))) &&
      ((encode_ptr_uint(state, (&(*input).psa_cipher_set_iv_req_p_handle)))) &&
      ((encode_ptr_buf(state, (&(*input).psa_cipher_set_iv_req_p_iv)))) &&
      ((encode_buf_len(state, (&(*input).psa_cipher_set_iv_req_iv_length)))))));

  log_result(state, res, __func__);
  return res;
}

static bool
encode_psa_cipher_update_req(zcbor_state_t *state,
                             const struct psa_cipher_update_req *input) {
  zcbor_log("%s\r\n", __func__);

  bool res = ((
      (((zcbor_uint32_put(state, (41)))) &&
       ((encode_ptr_uint(state, (&(*input).psa_cipher_update_req_p_handle)))) &&
       ((encode_ptr_buf(state, (&(*input).psa_cipher_update_req_p_input)))) &&
       ((encode_buf_len(state,
                        (&(*input).psa_cipher_update_req_input_length)))) &&
       ((encode_ptr_buf(state, (&(*input).psa_cipher_update_req_p_output)))) &&
       ((encode_buf_len(state,
                        (&(*input).psa_cipher_update_req_output_size)))) &&
       ((encode_ptr_uint(
           state, (&(*input).psa_cipher_update_req_p_output_length)))))));

  log_result(state, res, __func__);
  return res;
}

static bool
encode_psa_cipher_finish_req(zcbor_state_t *state,
                             const struct psa_cipher_finish_req *input) {
  zcbor_log("%s\r\n", __func__);

  bool res = ((
      (((zcbor_uint32_put(state, (42)))) &&
       ((encode_ptr_uint(state, (&(*input).psa_cipher_finish_req_p_handle)))) &&
       ((encode_ptr_buf(state, (&(*input).psa_cipher_finish_req_p_output)))) &&
       ((encode_buf_len(state,
                        (&(*input).psa_cipher_finish_req_output_size)))) &&
       ((encode_ptr_uint(
           state, (&(*input).psa_cipher_finish_req_p_output_length)))))));

  log_result(state, res, __func__);
  return res;
}

static bool
encode_psa_cipher_abort_req(zcbor_state_t *state,
                            const struct psa_cipher_abort_req *input) {
  zcbor_log("%s\r\n", __func__);

  bool res = ((
      (((zcbor_uint32_put(state, (43)))) &&
       ((encode_ptr_uint(state, (&(*input).psa_cipher_abort_req_p_handle)))))));

  log_result(state, res, __func__);
  return res;
}

static bool
encode_psa_aead_encrypt_req(zcbor_state_t *state,
                            const struct psa_aead_encrypt_req *input) {
  zcbor_log("%s\r\n", __func__);

  bool res = (((
      ((zcbor_uint32_put(state, (44)))) &&
      ((zcbor_uint32_encode(state, (&(*input).psa_aead_encrypt_req_key)))) &&
      ((zcbor_uint32_encode(state, (&(*input).psa_aead_encrypt_req_alg)))) &&
      ((encode_ptr_buf(state, (&(*input).psa_aead_encrypt_req_p_nonce)))) &&
      ((encode_buf_len(state,
                       (&(*input).psa_aead_encrypt_req_nonce_length)))) &&
      ((encode_ptr_buf(state,
                       (&(*input).psa_aead_encrypt_req_p_additional_data)))) &&
      ((encode_buf_len(
          state, (&(*input).psa_aead_encrypt_req_additional_data_length)))) &&
      ((encode_ptr_buf(state, (&(*input).psa_aead_encrypt_req_p_plaintext)))) &&
      ((encode_buf_len(state,
                       (&(*input).psa_aead_encrypt_req_plaintext_length)))) &&
      ((encode_ptr_buf(state,
                       (&(*input).psa_aead_encrypt_req_p_ciphertext)))) &&
      ((encode_buf_len(state,
                       (&(*input).psa_aead_encrypt_req_ciphertext_size)))) &&
      ((encode_ptr_uint(
          state, (&(*input).psa_aead_encrypt_req_p_ciphertext_length)))))));

  log_result(state, res, __func__);
  return res;
}

static bool
encode_psa_aead_decrypt_req(zcbor_state_t *state,
                            const struct psa_aead_decrypt_req *input) {
  zcbor_log("%s\r\n", __func__);

  bool res = (((
      ((zcbor_uint32_put(state, (45)))) &&
      ((zcbor_uint32_encode(state, (&(*input).psa_aead_decrypt_req_key)))) &&
      ((zcbor_uint32_encode(state, (&(*input).psa_aead_decrypt_req_alg)))) &&
      ((encode_ptr_buf(state, (&(*input).psa_aead_decrypt_req_p_nonce)))) &&
      ((encode_buf_len(state,
                       (&(*input).psa_aead_decrypt_req_nonce_length)))) &&
      ((encode_ptr_buf(state,
                       (&(*input).psa_aead_decrypt_req_p_additional_data)))) &&
      ((encode_buf_len(
          state, (&(*input).psa_aead_decrypt_req_additional_data_length)))) &&
      ((encode_ptr_buf(state,
                       (&(*input).psa_aead_decrypt_req_p_ciphertext)))) &&
      ((encode_buf_len(state,
                       (&(*input).psa_aead_decrypt_req_ciphertext_length)))) &&
      ((encode_ptr_buf(state, (&(*input).psa_aead_decrypt_req_p_plaintext)))) &&
      ((encode_buf_len(state,
                       (&(*input).psa_aead_decrypt_req_plaintext_size)))) &&
      ((encode_ptr_uint(
          state, (&(*input).psa_aead_decrypt_req_p_plaintext_length)))))));

  log_result(state, res, __func__);
  return res;
}

static bool encode_psa_aead_encrypt_setup_req(
    zcbor_state_t *state, const struct psa_aead_encrypt_setup_req *input) {
  zcbor_log("%s\r\n", __func__);

  bool res =
      (((((zcbor_uint32_put(state, (46)))) &&
         ((encode_ptr_uint(state,
                           (&(*input).psa_aead_encrypt_setup_req_p_handle)))) &&
         ((zcbor_uint32_encode(state,
                               (&(*input).psa_aead_encrypt_setup_req_key)))) &&
         ((zcbor_uint32_encode(state,
                               (&(*input).psa_aead_encrypt_setup_req_alg)))))));

  log_result(state, res, __func__);
  return res;
}

static bool encode_psa_aead_decrypt_setup_req(
    zcbor_state_t *state, const struct psa_aead_decrypt_setup_req *input) {
  zcbor_log("%s\r\n", __func__);

  bool res =
      (((((zcbor_uint32_put(state, (47)))) &&
         ((encode_ptr_uint(state,
                           (&(*input).psa_aead_decrypt_setup_req_p_handle)))) &&
         ((zcbor_uint32_encode(state,
                               (&(*input).psa_aead_decrypt_setup_req_key)))) &&
         ((zcbor_uint32_encode(state,
                               (&(*input).psa_aead_decrypt_setup_req_alg)))))));

  log_result(state, res, __func__);
  return res;
}

static bool encode_psa_aead_generate_nonce_req(
    zcbor_state_t *state, const struct psa_aead_generate_nonce_req *input) {
  zcbor_log("%s\r\n", __func__);

  bool res = ((
      (((zcbor_uint32_put(state, (48)))) &&
       ((encode_ptr_uint(state,
                         (&(*input).psa_aead_generate_nonce_req_p_handle)))) &&
       ((encode_ptr_buf(state,
                        (&(*input).psa_aead_generate_nonce_req_p_nonce)))) &&
       ((encode_buf_len(state,
                        (&(*input).psa_aead_generate_nonce_req_nonce_size)))) &&
       ((encode_ptr_uint(
           state, (&(*input).psa_aead_generate_nonce_req_p_nonce_length)))))));

  log_result(state, res, __func__);
  return res;
}

static bool
encode_psa_aead_set_nonce_req(zcbor_state_t *state,
                              const struct psa_aead_set_nonce_req *input) {
  zcbor_log("%s\r\n", __func__);

  bool res = (((
      ((zcbor_uint32_put(state, (49)))) &&
      ((encode_ptr_uint(state, (&(*input).psa_aead_set_nonce_req_p_handle)))) &&
      ((encode_ptr_buf(state, (&(*input).psa_aead_set_nonce_req_p_nonce)))) &&
      ((encode_buf_len(state,
                       (&(*input).psa_aead_set_nonce_req_nonce_length)))))));

  log_result(state, res, __func__);
  return res;
}

static bool
encode_psa_aead_set_lengths_req(zcbor_state_t *state,
                                const struct psa_aead_set_lengths_req *input) {
  zcbor_log("%s\r\n", __func__);

  bool res =
      (((((zcbor_uint32_put(state, (50)))) &&
         ((encode_ptr_uint(state,
                           (&(*input).psa_aead_set_lengths_req_p_handle)))) &&
         ((encode_buf_len(state,
                          (&(*input).psa_aead_set_lengths_req_ad_length)))) &&
         ((encode_buf_len(
             state, (&(*input).psa_aead_set_lengths_req_plaintext_length)))))));

  log_result(state, res, __func__);
  return res;
}

static bool
encode_psa_aead_update_ad_req(zcbor_state_t *state,
                              const struct psa_aead_update_ad_req *input) {
  zcbor_log("%s\r\n", __func__);

  bool res = (((
      ((zcbor_uint32_put(state, (51)))) &&
      ((encode_ptr_uint(state, (&(*input).psa_aead_update_ad_req_p_handle)))) &&
      ((encode_ptr_buf(state, (&(*input).psa_aead_update_ad_req_p_input)))) &&
      ((encode_buf_len(state,
                       (&(*input).psa_aead_update_ad_req_input_length)))))));

  log_result(state, res, __func__);
  return res;
}

static bool
encode_psa_aead_update_req(zcbor_state_t *state,
                           const struct psa_aead_update_req *input) {
  zcbor_log("%s\r\n", __func__);

  bool res = (((
      ((zcbor_uint32_put(state, (52)))) &&
      ((encode_ptr_uint(state, (&(*input).psa_aead_update_req_p_handle)))) &&
      ((encode_ptr_buf(state, (&(*input).psa_aead_update_req_p_input)))) &&
      ((encode_buf_len(state, (&(*input).psa_aead_update_req_input_length)))) &&
      ((encode_ptr_buf(state, (&(*input).psa_aead_update_req_p_output)))) &&
      ((encode_buf_len(state, (&(*input).psa_aead_update_req_output_size)))) &&
      ((encode_ptr_uint(state,
                        (&(*input).psa_aead_update_req_p_output_length)))))));

  log_result(state, res, __func__);
  return res;
}

static bool
encode_psa_aead_finish_req(zcbor_state_t *state,
                           const struct psa_aead_finish_req *input) {
  zcbor_log("%s\r\n", __func__);

  bool res = (((
      ((zcbor_uint32_put(state, (53)))) &&
      ((encode_ptr_uint(state, (&(*input).psa_aead_finish_req_p_handle)))) &&
      ((encode_ptr_buf(state, (&(*input).psa_aead_finish_req_p_ciphertext)))) &&
      ((encode_buf_len(state,
                       (&(*input).psa_aead_finish_req_ciphertext_size)))) &&
      ((encode_ptr_uint(
          state, (&(*input).psa_aead_finish_req_p_ciphertext_length)))) &&
      ((encode_ptr_buf(state, (&(*input).psa_aead_finish_req_p_tag)))) &&
      ((encode_buf_len(state, (&(*input).psa_aead_finish_req_tag_size)))) &&
      ((encode_ptr_uint(state,
                        (&(*input).psa_aead_finish_req_p_tag_length)))))));

  log_result(state, res, __func__);
  return res;
}

static bool
encode_psa_aead_verify_req(zcbor_state_t *state,
                           const struct psa_aead_verify_req *input) {
  zcbor_log("%s\r\n", __func__);

  bool res = ((
      (((zcbor_uint32_put(state, (54)))) &&
       ((encode_ptr_uint(state, (&(*input).psa_aead_verify_req_p_handle)))) &&
       ((encode_ptr_buf(state, (&(*input).psa_aead_verify_req_p_plaintext)))) &&
       ((encode_buf_len(state,
                        (&(*input).psa_aead_verify_req_plaintext_size)))) &&
       ((encode_ptr_uint(
           state, (&(*input).psa_aead_verify_req_p_plaintext_length)))) &&
       ((encode_ptr_buf(state, (&(*input).psa_aead_verify_req_p_tag)))) &&
       ((encode_buf_len(state, (&(*input).psa_aead_verify_req_tag_length)))))));

  log_result(state, res, __func__);
  return res;
}

static bool encode_psa_aead_abort_req(zcbor_state_t *state,
                                      const struct psa_aead_abort_req *input) {
  zcbor_log("%s\r\n", __func__);

  bool res =
      (((((zcbor_uint32_put(state, (55)))) &&
         ((encode_ptr_uint(state, (&(*input).psa_aead_abort_req_p_handle)))))));

  log_result(state, res, __func__);
  return res;
}

static bool
encode_psa_sign_message_req(zcbor_state_t *state,
                            const struct psa_sign_message_req *input) {
  zcbor_log("%s\r\n", __func__);

  bool res = (((
      ((zcbor_uint32_put(state, (56)))) &&
      ((zcbor_uint32_encode(state, (&(*input).psa_sign_message_req_key)))) &&
      ((zcbor_uint32_encode(state, (&(*input).psa_sign_message_req_alg)))) &&
      ((encode_ptr_buf(state, (&(*input).psa_sign_message_req_p_input)))) &&
      ((encode_buf_len(state,
                       (&(*input).psa_sign_message_req_input_length)))) &&
      ((encode_ptr_buf(state, (&(*input).psa_sign_message_req_p_signature)))) &&
      ((encode_buf_len(state,
                       (&(*input).psa_sign_message_req_signature_size)))) &&
      ((encode_ptr_uint(
          state, (&(*input).psa_sign_message_req_p_signature_length)))))));

  log_result(state, res, __func__);
  return res;
}

static bool
encode_psa_verify_message_req(zcbor_state_t *state,
                              const struct psa_verify_message_req *input) {
  zcbor_log("%s\r\n", __func__);

  bool res = ((
      (((zcbor_uint32_put(state, (57)))) &&
       ((zcbor_uint32_encode(state, (&(*input).psa_verify_message_req_key)))) &&
       ((zcbor_uint32_encode(state, (&(*input).psa_verify_message_req_alg)))) &&
       ((encode_ptr_buf(state, (&(*input).psa_verify_message_req_p_input)))) &&
       ((encode_buf_len(state,
                        (&(*input).psa_verify_message_req_input_length)))) &&
       ((encode_ptr_buf(state,
                        (&(*input).psa_verify_message_req_p_signature)))) &&
       ((encode_buf_len(
           state, (&(*input).psa_verify_message_req_signature_length)))))));

  log_result(state, res, __func__);
  return res;
}

static bool encode_psa_sign_hash_req(zcbor_state_t *state,
                                     const struct psa_sign_hash_req *input) {
  zcbor_log("%s\r\n", __func__);

  bool res = (((
      ((zcbor_uint32_put(state, (58)))) &&
      ((zcbor_uint32_encode(state, (&(*input).psa_sign_hash_req_key)))) &&
      ((zcbor_uint32_encode(state, (&(*input).psa_sign_hash_req_alg)))) &&
      ((encode_ptr_buf(state, (&(*input).psa_sign_hash_req_p_hash)))) &&
      ((encode_buf_len(state, (&(*input).psa_sign_hash_req_hash_length)))) &&
      ((encode_ptr_buf(state, (&(*input).psa_sign_hash_req_p_signature)))) &&
      ((encode_buf_len(state, (&(*input).psa_sign_hash_req_signature_size)))) &&
      ((encode_ptr_uint(state,
                        (&(*input).psa_sign_hash_req_p_signature_length)))))));

  log_result(state, res, __func__);
  return res;
}

static bool
encode_psa_verify_hash_req(zcbor_state_t *state,
                           const struct psa_verify_hash_req *input) {
  zcbor_log("%s\r\n", __func__);

  bool res = ((
      (((zcbor_uint32_put(state, (59)))) &&
       ((zcbor_uint32_encode(state, (&(*input).psa_verify_hash_req_key)))) &&
       ((zcbor_uint32_encode(state, (&(*input).psa_verify_hash_req_alg)))) &&
       ((encode_ptr_buf(state, (&(*input).psa_verify_hash_req_p_hash)))) &&
       ((encode_buf_len(state, (&(*input).psa_verify_hash_req_hash_length)))) &&
       ((encode_ptr_buf(state, (&(*input).psa_verify_hash_req_p_signature)))) &&
       ((encode_buf_len(state,
                        (&(*input).psa_verify_hash_req_signature_length)))))));

  log_result(state, res, __func__);
  return res;
}

static bool encode_psa_asymmetric_encrypt_req(
    zcbor_state_t *state, const struct psa_asymmetric_encrypt_req *input) {
  zcbor_log("%s\r\n", __func__);

  bool res = ((
      (((zcbor_uint32_put(state, (60)))) &&
       ((zcbor_uint32_encode(state,
                             (&(*input).psa_asymmetric_encrypt_req_key)))) &&
       ((zcbor_uint32_encode(state,
                             (&(*input).psa_asymmetric_encrypt_req_alg)))) &&
       ((encode_ptr_buf(state,
                        (&(*input).psa_asymmetric_encrypt_req_p_input)))) &&
       ((encode_buf_len(
           state, (&(*input).psa_asymmetric_encrypt_req_input_length)))) &&
       ((encode_ptr_buf(state,
                        (&(*input).psa_asymmetric_encrypt_req_p_salt)))) &&
       ((encode_buf_len(state,
                        (&(*input).psa_asymmetric_encrypt_req_salt_length)))) &&
       ((encode_ptr_buf(state,
                        (&(*input).psa_asymmetric_encrypt_req_p_output)))) &&
       ((encode_buf_len(state,
                        (&(*input).psa_asymmetric_encrypt_req_output_size)))) &&
       ((encode_ptr_uint(
           state, (&(*input).psa_asymmetric_encrypt_req_p_output_length)))))));

  log_result(state, res, __func__);
  return res;
}

static bool encode_psa_asymmetric_decrypt_req(
    zcbor_state_t *state, const struct psa_asymmetric_decrypt_req *input) {
  zcbor_log("%s\r\n", __func__);

  bool res = ((
      (((zcbor_uint32_put(state, (61)))) &&
       ((zcbor_uint32_encode(state,
                             (&(*input).psa_asymmetric_decrypt_req_key)))) &&
       ((zcbor_uint32_encode(state,
                             (&(*input).psa_asymmetric_decrypt_req_alg)))) &&
       ((encode_ptr_buf(state,
                        (&(*input).psa_asymmetric_decrypt_req_p_input)))) &&
       ((encode_buf_len(
           state, (&(*input).psa_asymmetric_decrypt_req_input_length)))) &&
       ((encode_ptr_buf(state,
                        (&(*input).psa_asymmetric_decrypt_req_p_salt)))) &&
       ((encode_buf_len(state,
                        (&(*input).psa_asymmetric_decrypt_req_salt_length)))) &&
       ((encode_ptr_buf(state,
                        (&(*input).psa_asymmetric_decrypt_req_p_output)))) &&
       ((encode_buf_len(state,
                        (&(*input).psa_asymmetric_decrypt_req_output_size)))) &&
       ((encode_ptr_uint(
           state, (&(*input).psa_asymmetric_decrypt_req_p_output_length)))))));

  log_result(state, res, __func__);
  return res;
}

static bool encode_psa_key_derivation_setup_req(
    zcbor_state_t *state, const struct psa_key_derivation_setup_req *input) {
  zcbor_log("%s\r\n", __func__);

  bool res =
      (((((zcbor_uint32_put(state, (62)))) &&
         ((encode_ptr_uint(
             state, (&(*input).psa_key_derivation_setup_req_p_handle)))) &&
         ((zcbor_uint32_encode(
             state, (&(*input).psa_key_derivation_setup_req_alg)))))));

  log_result(state, res, __func__);
  return res;
}

static bool encode_psa_key_derivation_get_capacity_req(
    zcbor_state_t *state,
    const struct psa_key_derivation_get_capacity_req *input) {
  zcbor_log("%s\r\n", __func__);

  bool res =
      (((((zcbor_uint32_put(state, (63)))) &&
         ((zcbor_uint32_encode(
             state, (&(*input).psa_key_derivation_get_capacity_req_handle)))) &&
         ((encode_ptr_uint(
             state,
             (&(*input).psa_key_derivation_get_capacity_req_p_capacity)))))));

  log_result(state, res, __func__);
  return res;
}

static bool encode_psa_key_derivation_set_capacity_req(
    zcbor_state_t *state,
    const struct psa_key_derivation_set_capacity_req *input) {
  zcbor_log("%s\r\n", __func__);

  bool res = (((
      ((zcbor_uint32_put(state, (64)))) &&
      ((encode_ptr_uint(
          state, (&(*input).psa_key_derivation_set_capacity_req_p_handle)))) &&
      ((zcbor_uint32_encode(
          state, (&(*input).psa_key_derivation_set_capacity_req_capacity)))))));

  log_result(state, res, __func__);
  return res;
}

static bool encode_psa_key_derivation_input_bytes_req(
    zcbor_state_t *state,
    const struct psa_key_derivation_input_bytes_req *input) {
  zcbor_log("%s\r\n", __func__);

  bool res = ((
      (((zcbor_uint32_put(state, (65)))) &&
       ((encode_ptr_uint(
           state, (&(*input).psa_key_derivation_input_bytes_req_p_handle)))) &&
       ((zcbor_uint32_encode(
           state, (&(*input).psa_key_derivation_input_bytes_req_step)))) &&
       ((encode_ptr_buf(
           state, (&(*input).psa_key_derivation_input_bytes_req_p_data)))) &&
       ((encode_buf_len(
           state,
           (&(*input).psa_key_derivation_input_bytes_req_data_length)))))));

  log_result(state, res, __func__);
  return res;
}

static bool encode_psa_key_derivation_input_integer_req(
    zcbor_state_t *state,
    const struct psa_key_derivation_input_integer_req *input) {
  zcbor_log("%s\r\n", __func__);

  bool res = (((
      ((zcbor_uint32_put(state, (66)))) &&
      ((encode_ptr_uint(
          state, (&(*input).psa_key_derivation_input_integer_req_p_handle)))) &&
      ((zcbor_uint32_encode(
          state, (&(*input).psa_key_derivation_input_integer_req_step)))) &&
      ((zcbor_uint32_encode(
          state, (&(*input).psa_key_derivation_input_integer_req_value)))))));

  log_result(state, res, __func__);
  return res;
}

static bool encode_psa_key_derivation_input_key_req(
    zcbor_state_t *state,
    const struct psa_key_derivation_input_key_req *input) {
  zcbor_log("%s\r\n", __func__);

  bool res =
      (((((zcbor_uint32_put(state, (67)))) &&
         ((encode_ptr_uint(
             state, (&(*input).psa_key_derivation_input_key_req_p_handle)))) &&
         ((zcbor_uint32_encode(
             state, (&(*input).psa_key_derivation_input_key_req_step)))) &&
         ((zcbor_uint32_encode(
             state, (&(*input).psa_key_derivation_input_key_req_key)))))));

  log_result(state, res, __func__);
  return res;
}

static bool encode_psa_key_derivation_key_agreement_req(
    zcbor_state_t *state,
    const struct psa_key_derivation_key_agreement_req *input) {
  zcbor_log("%s\r\n", __func__);

  bool res = (((
      ((zcbor_uint32_put(state, (68)))) &&
      ((encode_ptr_uint(
          state, (&(*input).psa_key_derivation_key_agreement_req_p_handle)))) &&
      ((zcbor_uint32_encode(
          state, (&(*input).psa_key_derivation_key_agreement_req_step)))) &&
      ((zcbor_uint32_encode(
          state,
          (&(*input).psa_key_derivation_key_agreement_req_private_key)))) &&
      ((encode_ptr_buf(
          state,
          (&(*input).psa_key_derivation_key_agreement_req_p_peer_key)))) &&
      ((encode_buf_len(
          state,
          (&(*input)
                .psa_key_derivation_key_agreement_req_peer_key_length)))))));

  log_result(state, res, __func__);
  return res;
}

static bool encode_psa_key_derivation_output_bytes_req(
    zcbor_state_t *state,
    const struct psa_key_derivation_output_bytes_req *input) {
  zcbor_log("%s\r\n", __func__);

  bool res = ((
      (((zcbor_uint32_put(state, (69)))) &&
       ((encode_ptr_uint(
           state, (&(*input).psa_key_derivation_output_bytes_req_p_handle)))) &&
       ((encode_ptr_buf(
           state, (&(*input).psa_key_derivation_output_bytes_req_p_output)))) &&
       ((encode_buf_len(
           state,
           (&(*input).psa_key_derivation_output_bytes_req_output_length)))))));

  log_result(state, res, __func__);
  return res;
}

static bool encode_psa_key_derivation_output_key_req(
    zcbor_state_t *state,
    const struct psa_key_derivation_output_key_req *input) {
  zcbor_log("%s\r\n", __func__);

  bool res =
      (((((zcbor_uint32_put(state, (70)))) &&
         ((encode_ptr_attr(
             state,
             (&(*input).psa_key_derivation_output_key_req_p_attributes)))) &&
         ((encode_ptr_uint(
             state, (&(*input).psa_key_derivation_output_key_req_p_handle)))) &&
         ((encode_ptr_key(
             state, (&(*input).psa_key_derivation_output_key_req_p_key)))))));

  log_result(state, res, __func__);
  return res;
}

static bool encode_psa_key_derivation_abort_req(
    zcbor_state_t *state, const struct psa_key_derivation_abort_req *input) {
  zcbor_log("%s\r\n", __func__);

  bool res =
      (((((zcbor_uint32_put(state, (71)))) &&
         ((encode_ptr_uint(
             state, (&(*input).psa_key_derivation_abort_req_p_handle)))))));

  log_result(state, res, __func__);
  return res;
}

static bool encode_psa_raw_key_agreement_req(
    zcbor_state_t *state, const struct psa_raw_key_agreement_req *input) {
  zcbor_log("%s\r\n", __func__);

  bool res =
      (((((zcbor_uint32_put(state, (72)))) &&
         ((zcbor_uint32_encode(state,
                               (&(*input).psa_raw_key_agreement_req_alg)))) &&
         ((zcbor_uint32_encode(
             state, (&(*input).psa_raw_key_agreement_req_private_key)))) &&
         ((encode_ptr_buf(state,
                          (&(*input).psa_raw_key_agreement_req_p_peer_key)))) &&
         ((encode_buf_len(
             state, (&(*input).psa_raw_key_agreement_req_peer_key_length)))) &&
         ((encode_ptr_buf(state,
                          (&(*input).psa_raw_key_agreement_req_p_output)))) &&
         ((encode_buf_len(
             state, (&(*input).psa_raw_key_agreement_req_output_size)))) &&
         ((encode_ptr_uint(
             state, (&(*input).psa_raw_key_agreement_req_p_output_length)))))));

  log_result(state, res, __func__);
  return res;
}

static bool
encode_psa_generate_random_req(zcbor_state_t *state,
                               const struct psa_generate_random_req *input) {
  zcbor_log("%s\r\n", __func__);

  bool res = (((
      ((zcbor_uint32_put(state, (73)))) &&
      ((encode_ptr_buf(state, (&(*input).psa_generate_random_req_p_output)))) &&
      ((encode_buf_len(state,
                       (&(*input).psa_generate_random_req_output_size)))))));

  log_result(state, res, __func__);
  return res;
}

static bool
encode_psa_generate_key_req(zcbor_state_t *state,
                            const struct psa_generate_key_req *input) {
  zcbor_log("%s\r\n", __func__);

  bool res =
      (((((zcbor_uint32_put(state, (74)))) &&
         ((encode_ptr_attr(state,
                           (&(*input).psa_generate_key_req_p_attributes)))) &&
         ((encode_ptr_key(state, (&(*input).psa_generate_key_req_p_key)))))));

  log_result(state, res, __func__);
  return res;
}

static bool encode_ptr_cipher(zcbor_state_t *state, const uint32_t *input) {
  zcbor_log("%s\r\n", __func__);

  bool res = ((zcbor_tag_put(state, 32775) &&
               (zcbor_uint32_encode(state, (&(*input))))));

  log_result(state, res, __func__);
  return res;
}

static bool encode_psa_pake_setup_req(zcbor_state_t *state,
                                      const struct psa_pake_setup_req *input) {
  zcbor_log("%s\r\n", __func__);

  bool res =
      (((((zcbor_uint32_put(state, (79)))) &&
         ((encode_ptr_uint(state, (&(*input).psa_pake_setup_req_p_handle)))) &&
         ((zcbor_uint32_encode(state,
                               (&(*input).psa_pake_setup_req_password_key)))) &&
         ((encode_ptr_cipher(
             state, (&(*input).psa_pake_setup_req_p_cipher_suite)))))));

  log_result(state, res, __func__);
  return res;
}

static bool
encode_psa_pake_set_role_req(zcbor_state_t *state,
                             const struct psa_pake_set_role_req *input) {
  zcbor_log("%s\r\n", __func__);

  bool res = (((
      ((zcbor_uint32_put(state, (80)))) &&
      ((encode_ptr_uint(state, (&(*input).psa_pake_set_role_req_p_handle)))) &&
      ((zcbor_uint32_encode(state, (&(*input).psa_pake_set_role_req_role)))))));

  log_result(state, res, __func__);
  return res;
}

static bool
encode_psa_pake_set_user_req(zcbor_state_t *state,
                             const struct psa_pake_set_user_req *input) {
  zcbor_log("%s\r\n", __func__);

  bool res = ((
      (((zcbor_uint32_put(state, (81)))) &&
       ((encode_ptr_uint(state, (&(*input).psa_pake_set_user_req_p_handle)))) &&
       ((encode_ptr_buf(state, (&(*input).psa_pake_set_user_req_p_user_id)))) &&
       ((encode_buf_len(state,
                        (&(*input).psa_pake_set_user_req_user_id_len)))))));

  log_result(state, res, __func__);
  return res;
}

static bool
encode_psa_pake_set_peer_req(zcbor_state_t *state,
                             const struct psa_pake_set_peer_req *input) {
  zcbor_log("%s\r\n", __func__);

  bool res = ((
      (((zcbor_uint32_put(state, (82)))) &&
       ((encode_ptr_uint(state, (&(*input).psa_pake_set_peer_req_p_handle)))) &&
       ((encode_ptr_buf(state, (&(*input).psa_pake_set_peer_req_p_peer_id)))) &&
       ((encode_buf_len(state,
                        (&(*input).psa_pake_set_peer_req_peer_id_len)))))));

  log_result(state, res, __func__);
  return res;
}

static bool
encode_psa_pake_set_context_req(zcbor_state_t *state,
                                const struct psa_pake_set_context_req *input) {
  zcbor_log("%s\r\n", __func__);

  bool res =
      (((((zcbor_uint32_put(state, (83)))) &&
         ((encode_ptr_uint(state,
                           (&(*input).psa_pake_set_context_req_p_handle)))) &&
         ((encode_ptr_buf(state,
                          (&(*input).psa_pake_set_context_req_p_context)))) &&
         ((encode_buf_len(
             state, (&(*input).psa_pake_set_context_req_context_len)))))));

  log_result(state, res, __func__);
  return res;
}

static bool
encode_psa_pake_output_req(zcbor_state_t *state,
                           const struct psa_pake_output_req *input) {
  zcbor_log("%s\r\n", __func__);

  bool res = ((
      (((zcbor_uint32_put(state, (84)))) &&
       ((encode_ptr_uint(state, (&(*input).psa_pake_output_req_p_handle)))) &&
       ((zcbor_uint32_encode(state, (&(*input).psa_pake_output_req_step)))) &&
       ((encode_ptr_buf(state, (&(*input).psa_pake_output_req_p_output)))) &&
       ((encode_buf_len(state, (&(*input).psa_pake_output_req_output_size)))) &&
       ((encode_ptr_uint(state,
                         (&(*input).psa_pake_output_req_p_output_length)))))));

  log_result(state, res, __func__);
  return res;
}

static bool encode_psa_pake_input_req(zcbor_state_t *state,
                                      const struct psa_pake_input_req *input) {
  zcbor_log("%s\r\n", __func__);

  bool res = (((
      ((zcbor_uint32_put(state, (85)))) &&
      ((encode_ptr_uint(state, (&(*input).psa_pake_input_req_p_handle)))) &&
      ((zcbor_uint32_encode(state, (&(*input).psa_pake_input_req_step)))) &&
      ((encode_ptr_buf(state, (&(*input).psa_pake_input_req_p_input)))) &&
      ((encode_buf_len(state, (&(*input).psa_pake_input_req_input_length)))))));

  log_result(state, res, __func__);
  return res;
}

static bool encode_psa_pake_get_shared_key_req(
    zcbor_state_t *state, const struct psa_pake_get_shared_key_req *input) {
  zcbor_log("%s\r\n", __func__);

  bool res =
      (((((zcbor_uint32_put(state, (86)))) &&
         ((encode_ptr_uint(
             state, (&(*input).psa_pake_get_shared_key_req_p_handle)))) &&
         ((encode_ptr_attr(
             state, (&(*input).psa_pake_get_shared_key_req_p_attributes)))) &&
         ((encode_ptr_key(state,
                          (&(*input).psa_pake_get_shared_key_req_p_key)))))));

  log_result(state, res, __func__);
  return res;
}

static bool encode_psa_pake_abort_req(zcbor_state_t *state,
                                      const struct psa_pake_abort_req *input) {
  zcbor_log("%s\r\n", __func__);

  bool res =
      (((((zcbor_uint32_put(state, (87)))) &&
         ((encode_ptr_uint(state, (&(*input).psa_pake_abort_req_p_handle)))))));

  log_result(state, res, __func__);
  return res;
}

static bool encode_psa_crypto_rsp(zcbor_state_t *state,
                                  const struct psa_crypto_rsp *input) {
  zcbor_log("%s\r\n", __func__);

  bool res =
      (((zcbor_list_start_encode(state, 2) &&
         ((((zcbor_uint32_encode(state, (&(*input).psa_crypto_rsp_id)))) &&
           ((zcbor_int32_encode(state, (&(*input).psa_crypto_rsp_status))))) ||
          (zcbor_list_map_end_force_encode(state), false)) &&
         zcbor_list_end_encode(state, 2))));

  log_result(state, res, __func__);
  return res;
}

static bool encode_psa_crypto_req(zcbor_state_t *state,
                                  const struct psa_crypto_req *input) {
  zcbor_log("%s\r\n", __func__);

        bool res = (((zcbor_list_start_encode(state, 12) && ((((((*input).psa_crypto_req_msg_choice == psa_crypto_req_msg_psa_crypto_init_req_m_c) ? ((zcbor_uint32_put(state, (10))))
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
	: (((*input).psa_crypto_req_msg_choice == psa_crypto_req_msg_psa_pake_setup_req_m_c) ? ((encode_psa_pake_setup_req(state, (&(*input).psa_crypto_req_msg_psa_pake_setup_req_m))))
	: (((*input).psa_crypto_req_msg_choice == psa_crypto_req_msg_psa_pake_set_role_req_m_c) ? ((encode_psa_pake_set_role_req(state, (&(*input).psa_crypto_req_msg_psa_pake_set_role_req_m))))
	: (((*input).psa_crypto_req_msg_choice == psa_crypto_req_msg_psa_pake_set_user_req_m_c) ? ((encode_psa_pake_set_user_req(state, (&(*input).psa_crypto_req_msg_psa_pake_set_user_req_m))))
	: (((*input).psa_crypto_req_msg_choice == psa_crypto_req_msg_psa_pake_set_peer_req_m_c) ? ((encode_psa_pake_set_peer_req(state, (&(*input).psa_crypto_req_msg_psa_pake_set_peer_req_m))))
	: (((*input).psa_crypto_req_msg_choice == psa_crypto_req_msg_psa_pake_set_context_req_m_c) ? ((encode_psa_pake_set_context_req(state, (&(*input).psa_crypto_req_msg_psa_pake_set_context_req_m))))
	: (((*input).psa_crypto_req_msg_choice == psa_crypto_req_msg_psa_pake_output_req_m_c) ? ((encode_psa_pake_output_req(state, (&(*input).psa_crypto_req_msg_psa_pake_output_req_m))))
	: (((*input).psa_crypto_req_msg_choice == psa_crypto_req_msg_psa_pake_input_req_m_c) ? ((encode_psa_pake_input_req(state, (&(*input).psa_crypto_req_msg_psa_pake_input_req_m))))
	: (((*input).psa_crypto_req_msg_choice == psa_crypto_req_msg_psa_pake_get_shared_key_req_m_c) ? ((encode_psa_pake_get_shared_key_req(state, (&(*input).psa_crypto_req_msg_psa_pake_get_shared_key_req_m))))
	: (((*input).psa_crypto_req_msg_choice == psa_crypto_req_msg_psa_pake_abort_req_m_c) ? ((encode_psa_pake_abort_req(state, (&(*input).psa_crypto_req_msg_psa_pake_abort_req_m))))
	: false)))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_list_end_encode(state, 12))));

        log_result(state, res, __func__);
        return res;
}

int cbor_encode_psa_crypto_req(uint8_t *payload, size_t payload_len,
                               const struct psa_crypto_req *input,
                               size_t *payload_len_out) {
  zcbor_state_t states[4];

  return zcbor_entry_function(payload, payload_len, (void *)input,
                              payload_len_out, states,
                              (zcbor_decoder_t *)encode_psa_crypto_req,
                              sizeof(states) / sizeof(zcbor_state_t), 1);
}

int cbor_encode_psa_crypto_rsp(uint8_t *payload, size_t payload_len,
                               const struct psa_crypto_rsp *input,
                               size_t *payload_len_out) {
  zcbor_state_t states[3];

  return zcbor_entry_function(payload, payload_len, (void *)input,
                              payload_len_out, states,
                              (zcbor_decoder_t *)encode_psa_crypto_rsp,
                              sizeof(states) / sizeof(zcbor_state_t), 1);
}
