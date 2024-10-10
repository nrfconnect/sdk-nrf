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

#include "psa_crypto_service_decode.h"
#include "zcbor_decode.h"
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

static bool decode_ptr_attr(zcbor_state_t *state, uint32_t *result);
static bool
decode_psa_get_key_attributes_req(zcbor_state_t *state,
                                  struct psa_get_key_attributes_req *result);
static bool decode_psa_reset_key_attributes_req(
    zcbor_state_t *state, struct psa_reset_key_attributes_req *result);
static bool decode_psa_purge_key_req(zcbor_state_t *state,
                                     struct psa_purge_key_req *result);
static bool decode_ptr_key(zcbor_state_t *state, uint32_t *result);
static bool decode_psa_copy_key_req(zcbor_state_t *state,
                                    struct psa_copy_key_req *result);
static bool decode_psa_destroy_key_req(zcbor_state_t *state,
                                       struct psa_destroy_key_req *result);
static bool decode_ptr_buf(zcbor_state_t *state, uint32_t *result);
static bool decode_buf_len(zcbor_state_t *state, uint32_t *result);
static bool decode_psa_import_key_req(zcbor_state_t *state,
                                      struct psa_import_key_req *result);
static bool decode_ptr_uint(zcbor_state_t *state, uint32_t *result);
static bool decode_psa_export_key_req(zcbor_state_t *state,
                                      struct psa_export_key_req *result);
static bool
decode_psa_export_public_key_req(zcbor_state_t *state,
                                 struct psa_export_public_key_req *result);
static bool decode_psa_hash_compute_req(zcbor_state_t *state,
                                        struct psa_hash_compute_req *result);
static bool decode_psa_hash_compare_req(zcbor_state_t *state,
                                        struct psa_hash_compare_req *result);
static bool decode_psa_hash_setup_req(zcbor_state_t *state,
                                      struct psa_hash_setup_req *result);
static bool decode_psa_hash_update_req(zcbor_state_t *state,
                                       struct psa_hash_update_req *result);
static bool decode_psa_hash_finish_req(zcbor_state_t *state,
                                       struct psa_hash_finish_req *result);
static bool decode_psa_hash_verify_req(zcbor_state_t *state,
                                       struct psa_hash_verify_req *result);
static bool decode_psa_hash_abort_req(zcbor_state_t *state,
                                      struct psa_hash_abort_req *result);
static bool decode_psa_hash_clone_req(zcbor_state_t *state,
                                      struct psa_hash_clone_req *result);
static bool decode_psa_mac_compute_req(zcbor_state_t *state,
                                       struct psa_mac_compute_req *result);
static bool decode_psa_mac_verify_req(zcbor_state_t *state,
                                      struct psa_mac_verify_req *result);
static bool
decode_psa_mac_sign_setup_req(zcbor_state_t *state,
                              struct psa_mac_sign_setup_req *result);
static bool
decode_psa_mac_verify_setup_req(zcbor_state_t *state,
                                struct psa_mac_verify_setup_req *result);
static bool decode_psa_mac_update_req(zcbor_state_t *state,
                                      struct psa_mac_update_req *result);
static bool
decode_psa_mac_sign_finish_req(zcbor_state_t *state,
                               struct psa_mac_sign_finish_req *result);
static bool
decode_psa_mac_verify_finish_req(zcbor_state_t *state,
                                 struct psa_mac_verify_finish_req *result);
static bool decode_psa_mac_abort_req(zcbor_state_t *state,
                                     struct psa_mac_abort_req *result);
static bool
decode_psa_cipher_encrypt_req(zcbor_state_t *state,
                              struct psa_cipher_encrypt_req *result);
static bool
decode_psa_cipher_decrypt_req(zcbor_state_t *state,
                              struct psa_cipher_decrypt_req *result);
static bool decode_psa_cipher_encrypt_setup_req(
    zcbor_state_t *state, struct psa_cipher_encrypt_setup_req *result);
static bool decode_psa_cipher_decrypt_setup_req(
    zcbor_state_t *state, struct psa_cipher_decrypt_setup_req *result);
static bool
decode_psa_cipher_generate_iv_req(zcbor_state_t *state,
                                  struct psa_cipher_generate_iv_req *result);
static bool decode_psa_cipher_set_iv_req(zcbor_state_t *state,
                                         struct psa_cipher_set_iv_req *result);
static bool decode_psa_cipher_update_req(zcbor_state_t *state,
                                         struct psa_cipher_update_req *result);
static bool decode_psa_cipher_finish_req(zcbor_state_t *state,
                                         struct psa_cipher_finish_req *result);
static bool decode_psa_cipher_abort_req(zcbor_state_t *state,
                                        struct psa_cipher_abort_req *result);
static bool decode_psa_aead_encrypt_req(zcbor_state_t *state,
                                        struct psa_aead_encrypt_req *result);
static bool decode_psa_aead_decrypt_req(zcbor_state_t *state,
                                        struct psa_aead_decrypt_req *result);
static bool
decode_psa_aead_encrypt_setup_req(zcbor_state_t *state,
                                  struct psa_aead_encrypt_setup_req *result);
static bool
decode_psa_aead_decrypt_setup_req(zcbor_state_t *state,
                                  struct psa_aead_decrypt_setup_req *result);
static bool
decode_psa_aead_generate_nonce_req(zcbor_state_t *state,
                                   struct psa_aead_generate_nonce_req *result);
static bool
decode_psa_aead_set_nonce_req(zcbor_state_t *state,
                              struct psa_aead_set_nonce_req *result);
static bool
decode_psa_aead_set_lengths_req(zcbor_state_t *state,
                                struct psa_aead_set_lengths_req *result);
static bool
decode_psa_aead_update_ad_req(zcbor_state_t *state,
                              struct psa_aead_update_ad_req *result);
static bool decode_psa_aead_update_req(zcbor_state_t *state,
                                       struct psa_aead_update_req *result);
static bool decode_psa_aead_finish_req(zcbor_state_t *state,
                                       struct psa_aead_finish_req *result);
static bool decode_psa_aead_verify_req(zcbor_state_t *state,
                                       struct psa_aead_verify_req *result);
static bool decode_psa_aead_abort_req(zcbor_state_t *state,
                                      struct psa_aead_abort_req *result);
static bool decode_psa_sign_message_req(zcbor_state_t *state,
                                        struct psa_sign_message_req *result);
static bool
decode_psa_verify_message_req(zcbor_state_t *state,
                              struct psa_verify_message_req *result);
static bool decode_psa_sign_hash_req(zcbor_state_t *state,
                                     struct psa_sign_hash_req *result);
static bool decode_psa_verify_hash_req(zcbor_state_t *state,
                                       struct psa_verify_hash_req *result);
static bool
decode_psa_asymmetric_encrypt_req(zcbor_state_t *state,
                                  struct psa_asymmetric_encrypt_req *result);
static bool
decode_psa_asymmetric_decrypt_req(zcbor_state_t *state,
                                  struct psa_asymmetric_decrypt_req *result);
static bool decode_psa_key_derivation_setup_req(
    zcbor_state_t *state, struct psa_key_derivation_setup_req *result);
static bool decode_psa_key_derivation_get_capacity_req(
    zcbor_state_t *state, struct psa_key_derivation_get_capacity_req *result);
static bool decode_psa_key_derivation_set_capacity_req(
    zcbor_state_t *state, struct psa_key_derivation_set_capacity_req *result);
static bool decode_psa_key_derivation_input_bytes_req(
    zcbor_state_t *state, struct psa_key_derivation_input_bytes_req *result);
static bool decode_psa_key_derivation_input_integer_req(
    zcbor_state_t *state, struct psa_key_derivation_input_integer_req *result);
static bool decode_psa_key_derivation_input_key_req(
    zcbor_state_t *state, struct psa_key_derivation_input_key_req *result);
static bool decode_psa_key_derivation_key_agreement_req(
    zcbor_state_t *state, struct psa_key_derivation_key_agreement_req *result);
static bool decode_psa_key_derivation_output_bytes_req(
    zcbor_state_t *state, struct psa_key_derivation_output_bytes_req *result);
static bool decode_psa_key_derivation_output_key_req(
    zcbor_state_t *state, struct psa_key_derivation_output_key_req *result);
static bool decode_psa_key_derivation_abort_req(
    zcbor_state_t *state, struct psa_key_derivation_abort_req *result);
static bool
decode_psa_raw_key_agreement_req(zcbor_state_t *state,
                                 struct psa_raw_key_agreement_req *result);
static bool
decode_psa_generate_random_req(zcbor_state_t *state,
                               struct psa_generate_random_req *result);
static bool decode_psa_generate_key_req(zcbor_state_t *state,
                                        struct psa_generate_key_req *result);
static bool decode_ptr_cipher(zcbor_state_t *state, uint32_t *result);
static bool decode_psa_pake_setup_req(zcbor_state_t *state,
                                      struct psa_pake_setup_req *result);
static bool decode_psa_pake_set_role_req(zcbor_state_t *state,
                                         struct psa_pake_set_role_req *result);
static bool decode_psa_pake_set_user_req(zcbor_state_t *state,
                                         struct psa_pake_set_user_req *result);
static bool decode_psa_pake_set_peer_req(zcbor_state_t *state,
                                         struct psa_pake_set_peer_req *result);
static bool
decode_psa_pake_set_context_req(zcbor_state_t *state,
                                struct psa_pake_set_context_req *result);
static bool decode_psa_pake_output_req(zcbor_state_t *state,
                                       struct psa_pake_output_req *result);
static bool decode_psa_pake_input_req(zcbor_state_t *state,
                                      struct psa_pake_input_req *result);
static bool
decode_psa_pake_get_shared_key_req(zcbor_state_t *state,
                                   struct psa_pake_get_shared_key_req *result);
static bool decode_psa_pake_abort_req(zcbor_state_t *state,
                                      struct psa_pake_abort_req *result);
static bool decode_psa_crypto_rsp(zcbor_state_t *state,
                                  struct psa_crypto_rsp *result);
static bool decode_psa_crypto_req(zcbor_state_t *state,
                                  struct psa_crypto_req *result);

static bool decode_ptr_attr(zcbor_state_t *state, uint32_t *result) {
  zcbor_log("%s\r\n", __func__);

  bool res = ((zcbor_tag_expect(state, 32772) &&
               (zcbor_uint32_decode(state, (&(*result))))));

  log_result(state, res, __func__);
  return res;
}

static bool
decode_psa_get_key_attributes_req(zcbor_state_t *state,
                                  struct psa_get_key_attributes_req *result) {
  zcbor_log("%s\r\n", __func__);

  bool res =
      (((((zcbor_uint32_expect(state, (11)))) &&
         ((zcbor_uint32_decode(state,
                               (&(*result).psa_get_key_attributes_req_key)))) &&
         ((decode_ptr_attr(
             state, (&(*result).psa_get_key_attributes_req_p_attributes)))))));

  log_result(state, res, __func__);
  return res;
}

static bool decode_psa_reset_key_attributes_req(
    zcbor_state_t *state, struct psa_reset_key_attributes_req *result) {
  zcbor_log("%s\r\n", __func__);

  bool res = ((
      (((zcbor_uint32_expect(state, (12)))) &&
       ((decode_ptr_attr(
           state, (&(*result).psa_reset_key_attributes_req_p_attributes)))))));

  log_result(state, res, __func__);
  return res;
}

static bool decode_psa_purge_key_req(zcbor_state_t *state,
                                     struct psa_purge_key_req *result) {
  zcbor_log("%s\r\n", __func__);

  bool res =
      (((((zcbor_uint32_expect(state, (13)))) &&
         ((zcbor_uint32_decode(state, (&(*result).psa_purge_key_req_key)))))));

  log_result(state, res, __func__);
  return res;
}

static bool decode_ptr_key(zcbor_state_t *state, uint32_t *result) {
  zcbor_log("%s\r\n", __func__);

  bool res = ((zcbor_tag_expect(state, 32773) &&
               (zcbor_uint32_decode(state, (&(*result))))));

  log_result(state, res, __func__);
  return res;
}

static bool decode_psa_copy_key_req(zcbor_state_t *state,
                                    struct psa_copy_key_req *result) {
  zcbor_log("%s\r\n", __func__);

  bool res = ((
      (((zcbor_uint32_expect(state, (14)))) &&
       ((zcbor_uint32_decode(state,
                             (&(*result).psa_copy_key_req_source_key)))) &&
       ((decode_ptr_attr(state, (&(*result).psa_copy_key_req_p_attributes)))) &&
       ((decode_ptr_key(state, (&(*result).psa_copy_key_req_p_target_key)))))));

  log_result(state, res, __func__);
  return res;
}

static bool decode_psa_destroy_key_req(zcbor_state_t *state,
                                       struct psa_destroy_key_req *result) {
  zcbor_log("%s\r\n", __func__);

  bool res = ((
      (((zcbor_uint32_expect(state, (15)))) &&
       ((zcbor_uint32_decode(state, (&(*result).psa_destroy_key_req_key)))))));

  log_result(state, res, __func__);
  return res;
}

static bool decode_ptr_buf(zcbor_state_t *state, uint32_t *result) {
  zcbor_log("%s\r\n", __func__);

  bool res = ((zcbor_tag_expect(state, 32770) &&
               (zcbor_uint32_decode(state, (&(*result))))));

  log_result(state, res, __func__);
  return res;
}

static bool decode_buf_len(zcbor_state_t *state, uint32_t *result) {
  zcbor_log("%s\r\n", __func__);

  bool res = ((zcbor_tag_expect(state, 32771) &&
               (zcbor_uint32_decode(state, (&(*result))))));

  log_result(state, res, __func__);
  return res;
}

static bool decode_psa_import_key_req(zcbor_state_t *state,
                                      struct psa_import_key_req *result) {
  zcbor_log("%s\r\n", __func__);

  bool res = ((
      (((zcbor_uint32_expect(state, (16)))) &&
       ((decode_ptr_attr(state,
                         (&(*result).psa_import_key_req_p_attributes)))) &&
       ((decode_ptr_buf(state, (&(*result).psa_import_key_req_p_data)))) &&
       ((decode_buf_len(state, (&(*result).psa_import_key_req_data_length)))) &&
       ((decode_ptr_key(state, (&(*result).psa_import_key_req_p_key)))))));

  log_result(state, res, __func__);
  return res;
}

static bool decode_ptr_uint(zcbor_state_t *state, uint32_t *result) {
  zcbor_log("%s\r\n", __func__);

  bool res = ((zcbor_tag_expect(state, 32774) &&
               (zcbor_uint32_decode(state, (&(*result))))));

  log_result(state, res, __func__);
  return res;
}

static bool decode_psa_export_key_req(zcbor_state_t *state,
                                      struct psa_export_key_req *result) {
  zcbor_log("%s\r\n", __func__);

  bool res =
      (((((zcbor_uint32_expect(state, (17)))) &&
         ((zcbor_uint32_decode(state, (&(*result).psa_export_key_req_key)))) &&
         ((decode_ptr_buf(state, (&(*result).psa_export_key_req_p_data)))) &&
         ((decode_buf_len(state, (&(*result).psa_export_key_req_data_size)))) &&
         ((decode_ptr_uint(state,
                           (&(*result).psa_export_key_req_p_data_length)))))));

  log_result(state, res, __func__);
  return res;
}

static bool
decode_psa_export_public_key_req(zcbor_state_t *state,
                                 struct psa_export_public_key_req *result) {
  zcbor_log("%s\r\n", __func__);

  bool res =
      (((((zcbor_uint32_expect(state, (18)))) &&
         ((zcbor_uint32_decode(state,
                               (&(*result).psa_export_public_key_req_key)))) &&
         ((decode_ptr_buf(state,
                          (&(*result).psa_export_public_key_req_p_data)))) &&
         ((decode_buf_len(state,
                          (&(*result).psa_export_public_key_req_data_size)))) &&
         ((decode_ptr_uint(
             state, (&(*result).psa_export_public_key_req_p_data_length)))))));

  log_result(state, res, __func__);
  return res;
}

static bool decode_psa_hash_compute_req(zcbor_state_t *state,
                                        struct psa_hash_compute_req *result) {
  zcbor_log("%s\r\n", __func__);

  bool res = ((
      (((zcbor_uint32_expect(state, (19)))) &&
       ((zcbor_uint32_decode(state, (&(*result).psa_hash_compute_req_alg)))) &&
       ((decode_ptr_buf(state, (&(*result).psa_hash_compute_req_p_input)))) &&
       ((decode_buf_len(state,
                        (&(*result).psa_hash_compute_req_input_length)))) &&
       ((decode_ptr_buf(state, (&(*result).psa_hash_compute_req_p_hash)))) &&
       ((decode_buf_len(state, (&(*result).psa_hash_compute_req_hash_size)))) &&
       ((decode_ptr_uint(state,
                         (&(*result).psa_hash_compute_req_p_hash_length)))))));

  log_result(state, res, __func__);
  return res;
}

static bool decode_psa_hash_compare_req(zcbor_state_t *state,
                                        struct psa_hash_compare_req *result) {
  zcbor_log("%s\r\n", __func__);

  bool res = ((
      (((zcbor_uint32_expect(state, (20)))) &&
       ((zcbor_uint32_decode(state, (&(*result).psa_hash_compare_req_alg)))) &&
       ((decode_ptr_buf(state, (&(*result).psa_hash_compare_req_p_input)))) &&
       ((decode_buf_len(state,
                        (&(*result).psa_hash_compare_req_input_length)))) &&
       ((decode_ptr_buf(state, (&(*result).psa_hash_compare_req_p_hash)))) &&
       ((decode_buf_len(state,
                        (&(*result).psa_hash_compare_req_hash_length)))))));

  log_result(state, res, __func__);
  return res;
}

static bool decode_psa_hash_setup_req(zcbor_state_t *state,
                                      struct psa_hash_setup_req *result) {
  zcbor_log("%s\r\n", __func__);

  bool res =
      (((((zcbor_uint32_expect(state, (21)))) &&
         ((decode_ptr_uint(state, (&(*result).psa_hash_setup_req_p_handle)))) &&
         ((zcbor_uint32_decode(state, (&(*result).psa_hash_setup_req_alg)))))));

  log_result(state, res, __func__);
  return res;
}

static bool decode_psa_hash_update_req(zcbor_state_t *state,
                                       struct psa_hash_update_req *result) {
  zcbor_log("%s\r\n", __func__);

  bool res = ((
      (((zcbor_uint32_expect(state, (22)))) &&
       ((decode_ptr_uint(state, (&(*result).psa_hash_update_req_p_handle)))) &&
       ((decode_ptr_buf(state, (&(*result).psa_hash_update_req_p_input)))) &&
       ((decode_buf_len(state,
                        (&(*result).psa_hash_update_req_input_length)))))));

  log_result(state, res, __func__);
  return res;
}

static bool decode_psa_hash_finish_req(zcbor_state_t *state,
                                       struct psa_hash_finish_req *result) {
  zcbor_log("%s\r\n", __func__);

  bool res = ((
      (((zcbor_uint32_expect(state, (23)))) &&
       ((decode_ptr_uint(state, (&(*result).psa_hash_finish_req_p_handle)))) &&
       ((decode_ptr_buf(state, (&(*result).psa_hash_finish_req_p_hash)))) &&
       ((decode_buf_len(state, (&(*result).psa_hash_finish_req_hash_size)))) &&
       ((decode_ptr_uint(state,
                         (&(*result).psa_hash_finish_req_p_hash_length)))))));

  log_result(state, res, __func__);
  return res;
}

static bool decode_psa_hash_verify_req(zcbor_state_t *state,
                                       struct psa_hash_verify_req *result) {
  zcbor_log("%s\r\n", __func__);

  bool res = ((
      (((zcbor_uint32_expect(state, (24)))) &&
       ((decode_ptr_uint(state, (&(*result).psa_hash_verify_req_p_handle)))) &&
       ((decode_ptr_buf(state, (&(*result).psa_hash_verify_req_p_hash)))) &&
       ((decode_buf_len(state,
                        (&(*result).psa_hash_verify_req_hash_length)))))));

  log_result(state, res, __func__);
  return res;
}

static bool decode_psa_hash_abort_req(zcbor_state_t *state,
                                      struct psa_hash_abort_req *result) {
  zcbor_log("%s\r\n", __func__);

  bool res = ((
      (((zcbor_uint32_expect(state, (25)))) &&
       ((decode_ptr_uint(state, (&(*result).psa_hash_abort_req_p_handle)))))));

  log_result(state, res, __func__);
  return res;
}

static bool decode_psa_hash_clone_req(zcbor_state_t *state,
                                      struct psa_hash_clone_req *result) {
  zcbor_log("%s\r\n", __func__);

  bool res = ((
      (((zcbor_uint32_expect(state, (26)))) &&
       ((zcbor_uint32_decode(state, (&(*result).psa_hash_clone_req_handle)))) &&
       ((decode_ptr_uint(state, (&(*result).psa_hash_clone_req_p_handle)))))));

  log_result(state, res, __func__);
  return res;
}

static bool decode_psa_mac_compute_req(zcbor_state_t *state,
                                       struct psa_mac_compute_req *result) {
  zcbor_log("%s\r\n", __func__);

  bool res =
      (((((zcbor_uint32_expect(state, (27)))) &&
         ((zcbor_uint32_decode(state, (&(*result).psa_mac_compute_req_key)))) &&
         ((zcbor_uint32_decode(state, (&(*result).psa_mac_compute_req_alg)))) &&
         ((decode_ptr_buf(state, (&(*result).psa_mac_compute_req_p_input)))) &&
         ((decode_buf_len(state,
                          (&(*result).psa_mac_compute_req_input_length)))) &&
         ((decode_ptr_buf(state, (&(*result).psa_mac_compute_req_p_mac)))) &&
         ((decode_buf_len(state, (&(*result).psa_mac_compute_req_mac_size)))) &&
         ((decode_ptr_uint(state,
                           (&(*result).psa_mac_compute_req_p_mac_length)))))));

  log_result(state, res, __func__);
  return res;
}

static bool decode_psa_mac_verify_req(zcbor_state_t *state,
                                      struct psa_mac_verify_req *result) {
  zcbor_log("%s\r\n", __func__);

  bool res = (((
      ((zcbor_uint32_expect(state, (28)))) &&
      ((zcbor_uint32_decode(state, (&(*result).psa_mac_verify_req_key)))) &&
      ((zcbor_uint32_decode(state, (&(*result).psa_mac_verify_req_alg)))) &&
      ((decode_ptr_buf(state, (&(*result).psa_mac_verify_req_p_input)))) &&
      ((decode_buf_len(state, (&(*result).psa_mac_verify_req_input_length)))) &&
      ((decode_ptr_buf(state, (&(*result).psa_mac_verify_req_p_mac)))) &&
      ((decode_buf_len(state, (&(*result).psa_mac_verify_req_mac_length)))))));

  log_result(state, res, __func__);
  return res;
}

static bool
decode_psa_mac_sign_setup_req(zcbor_state_t *state,
                              struct psa_mac_sign_setup_req *result) {
  zcbor_log("%s\r\n", __func__);

  bool res = (((
      ((zcbor_uint32_expect(state, (29)))) &&
      ((decode_ptr_uint(state,
                        (&(*result).psa_mac_sign_setup_req_p_handle)))) &&
      ((zcbor_uint32_decode(state, (&(*result).psa_mac_sign_setup_req_key)))) &&
      ((zcbor_uint32_decode(state,
                            (&(*result).psa_mac_sign_setup_req_alg)))))));

  log_result(state, res, __func__);
  return res;
}

static bool
decode_psa_mac_verify_setup_req(zcbor_state_t *state,
                                struct psa_mac_verify_setup_req *result) {
  zcbor_log("%s\r\n", __func__);

  bool res =
      (((((zcbor_uint32_expect(state, (30)))) &&
         ((decode_ptr_uint(state,
                           (&(*result).psa_mac_verify_setup_req_p_handle)))) &&
         ((zcbor_uint32_decode(state,
                               (&(*result).psa_mac_verify_setup_req_key)))) &&
         ((zcbor_uint32_decode(state,
                               (&(*result).psa_mac_verify_setup_req_alg)))))));

  log_result(state, res, __func__);
  return res;
}

static bool decode_psa_mac_update_req(zcbor_state_t *state,
                                      struct psa_mac_update_req *result) {
  zcbor_log("%s\r\n", __func__);

  bool res =
      (((((zcbor_uint32_expect(state, (31)))) &&
         ((decode_ptr_uint(state, (&(*result).psa_mac_update_req_p_handle)))) &&
         ((decode_ptr_buf(state, (&(*result).psa_mac_update_req_p_input)))) &&
         ((decode_buf_len(state,
                          (&(*result).psa_mac_update_req_input_length)))))));

  log_result(state, res, __func__);
  return res;
}

static bool
decode_psa_mac_sign_finish_req(zcbor_state_t *state,
                               struct psa_mac_sign_finish_req *result) {
  zcbor_log("%s\r\n", __func__);

  bool res = ((
      (((zcbor_uint32_expect(state, (32)))) &&
       ((decode_ptr_uint(state,
                         (&(*result).psa_mac_sign_finish_req_p_handle)))) &&
       ((decode_ptr_buf(state, (&(*result).psa_mac_sign_finish_req_p_mac)))) &&
       ((decode_buf_len(state,
                        (&(*result).psa_mac_sign_finish_req_mac_size)))) &&
       ((decode_ptr_uint(
           state, (&(*result).psa_mac_sign_finish_req_p_mac_length)))))));

  log_result(state, res, __func__);
  return res;
}

static bool
decode_psa_mac_verify_finish_req(zcbor_state_t *state,
                                 struct psa_mac_verify_finish_req *result) {
  zcbor_log("%s\r\n", __func__);

  bool res = (((
      ((zcbor_uint32_expect(state, (33)))) &&
      ((decode_ptr_uint(state,
                        (&(*result).psa_mac_verify_finish_req_p_handle)))) &&
      ((decode_ptr_buf(state, (&(*result).psa_mac_verify_finish_req_p_mac)))) &&
      ((decode_buf_len(state,
                       (&(*result).psa_mac_verify_finish_req_mac_length)))))));

  log_result(state, res, __func__);
  return res;
}

static bool decode_psa_mac_abort_req(zcbor_state_t *state,
                                     struct psa_mac_abort_req *result) {
  zcbor_log("%s\r\n", __func__);

  bool res =
      (((((zcbor_uint32_expect(state, (34)))) &&
         ((decode_ptr_uint(state, (&(*result).psa_mac_abort_req_p_handle)))))));

  log_result(state, res, __func__);
  return res;
}

static bool
decode_psa_cipher_encrypt_req(zcbor_state_t *state,
                              struct psa_cipher_encrypt_req *result) {
  zcbor_log("%s\r\n", __func__);

  bool res = (((
      ((zcbor_uint32_expect(state, (35)))) &&
      ((zcbor_uint32_decode(state, (&(*result).psa_cipher_encrypt_req_key)))) &&
      ((zcbor_uint32_decode(state, (&(*result).psa_cipher_encrypt_req_alg)))) &&
      ((decode_ptr_buf(state, (&(*result).psa_cipher_encrypt_req_p_input)))) &&
      ((decode_buf_len(state,
                       (&(*result).psa_cipher_encrypt_req_input_length)))) &&
      ((decode_ptr_buf(state, (&(*result).psa_cipher_encrypt_req_p_output)))) &&
      ((decode_buf_len(state,
                       (&(*result).psa_cipher_encrypt_req_output_size)))) &&
      ((decode_ptr_uint(
          state, (&(*result).psa_cipher_encrypt_req_p_output_length)))))));

  log_result(state, res, __func__);
  return res;
}

static bool
decode_psa_cipher_decrypt_req(zcbor_state_t *state,
                              struct psa_cipher_decrypt_req *result) {
  zcbor_log("%s\r\n", __func__);

  bool res = (((
      ((zcbor_uint32_expect(state, (36)))) &&
      ((zcbor_uint32_decode(state, (&(*result).psa_cipher_decrypt_req_key)))) &&
      ((zcbor_uint32_decode(state, (&(*result).psa_cipher_decrypt_req_alg)))) &&
      ((decode_ptr_buf(state, (&(*result).psa_cipher_decrypt_req_p_input)))) &&
      ((decode_buf_len(state,
                       (&(*result).psa_cipher_decrypt_req_input_length)))) &&
      ((decode_ptr_buf(state, (&(*result).psa_cipher_decrypt_req_p_output)))) &&
      ((decode_buf_len(state,
                       (&(*result).psa_cipher_decrypt_req_output_size)))) &&
      ((decode_ptr_uint(
          state, (&(*result).psa_cipher_decrypt_req_p_output_length)))))));

  log_result(state, res, __func__);
  return res;
}

static bool decode_psa_cipher_encrypt_setup_req(
    zcbor_state_t *state, struct psa_cipher_encrypt_setup_req *result) {
  zcbor_log("%s\r\n", __func__);

  bool res =
      (((((zcbor_uint32_expect(state, (37)))) &&
         ((decode_ptr_uint(
             state, (&(*result).psa_cipher_encrypt_setup_req_p_handle)))) &&
         ((zcbor_uint32_decode(
             state, (&(*result).psa_cipher_encrypt_setup_req_key)))) &&
         ((zcbor_uint32_decode(
             state, (&(*result).psa_cipher_encrypt_setup_req_alg)))))));

  log_result(state, res, __func__);
  return res;
}

static bool decode_psa_cipher_decrypt_setup_req(
    zcbor_state_t *state, struct psa_cipher_decrypt_setup_req *result) {
  zcbor_log("%s\r\n", __func__);

  bool res =
      (((((zcbor_uint32_expect(state, (38)))) &&
         ((decode_ptr_uint(
             state, (&(*result).psa_cipher_decrypt_setup_req_p_handle)))) &&
         ((zcbor_uint32_decode(
             state, (&(*result).psa_cipher_decrypt_setup_req_key)))) &&
         ((zcbor_uint32_decode(
             state, (&(*result).psa_cipher_decrypt_setup_req_alg)))))));

  log_result(state, res, __func__);
  return res;
}

static bool
decode_psa_cipher_generate_iv_req(zcbor_state_t *state,
                                  struct psa_cipher_generate_iv_req *result) {
  zcbor_log("%s\r\n", __func__);

  bool res = (((
      ((zcbor_uint32_expect(state, (39)))) &&
      ((decode_ptr_uint(state,
                        (&(*result).psa_cipher_generate_iv_req_p_handle)))) &&
      ((decode_ptr_buf(state, (&(*result).psa_cipher_generate_iv_req_p_iv)))) &&
      ((decode_buf_len(state,
                       (&(*result).psa_cipher_generate_iv_req_iv_size)))) &&
      ((decode_ptr_uint(
          state, (&(*result).psa_cipher_generate_iv_req_p_iv_length)))))));

  log_result(state, res, __func__);
  return res;
}

static bool decode_psa_cipher_set_iv_req(zcbor_state_t *state,
                                         struct psa_cipher_set_iv_req *result) {
  zcbor_log("%s\r\n", __func__);

  bool res = (((
      ((zcbor_uint32_expect(state, (40)))) &&
      ((decode_ptr_uint(state, (&(*result).psa_cipher_set_iv_req_p_handle)))) &&
      ((decode_ptr_buf(state, (&(*result).psa_cipher_set_iv_req_p_iv)))) &&
      ((decode_buf_len(state,
                       (&(*result).psa_cipher_set_iv_req_iv_length)))))));

  log_result(state, res, __func__);
  return res;
}

static bool decode_psa_cipher_update_req(zcbor_state_t *state,
                                         struct psa_cipher_update_req *result) {
  zcbor_log("%s\r\n", __func__);

  bool res = (((
      ((zcbor_uint32_expect(state, (41)))) &&
      ((decode_ptr_uint(state, (&(*result).psa_cipher_update_req_p_handle)))) &&
      ((decode_ptr_buf(state, (&(*result).psa_cipher_update_req_p_input)))) &&
      ((decode_buf_len(state,
                       (&(*result).psa_cipher_update_req_input_length)))) &&
      ((decode_ptr_buf(state, (&(*result).psa_cipher_update_req_p_output)))) &&
      ((decode_buf_len(state,
                       (&(*result).psa_cipher_update_req_output_size)))) &&
      ((decode_ptr_uint(
          state, (&(*result).psa_cipher_update_req_p_output_length)))))));

  log_result(state, res, __func__);
  return res;
}

static bool decode_psa_cipher_finish_req(zcbor_state_t *state,
                                         struct psa_cipher_finish_req *result) {
  zcbor_log("%s\r\n", __func__);

  bool res = (((
      ((zcbor_uint32_expect(state, (42)))) &&
      ((decode_ptr_uint(state, (&(*result).psa_cipher_finish_req_p_handle)))) &&
      ((decode_ptr_buf(state, (&(*result).psa_cipher_finish_req_p_output)))) &&
      ((decode_buf_len(state,
                       (&(*result).psa_cipher_finish_req_output_size)))) &&
      ((decode_ptr_uint(
          state, (&(*result).psa_cipher_finish_req_p_output_length)))))));

  log_result(state, res, __func__);
  return res;
}

static bool decode_psa_cipher_abort_req(zcbor_state_t *state,
                                        struct psa_cipher_abort_req *result) {
  zcbor_log("%s\r\n", __func__);

  bool res = (((
      ((zcbor_uint32_expect(state, (43)))) &&
      ((decode_ptr_uint(state, (&(*result).psa_cipher_abort_req_p_handle)))))));

  log_result(state, res, __func__);
  return res;
}

static bool decode_psa_aead_encrypt_req(zcbor_state_t *state,
                                        struct psa_aead_encrypt_req *result) {
  zcbor_log("%s\r\n", __func__);

  bool res = ((
      (((zcbor_uint32_expect(state, (44)))) &&
       ((zcbor_uint32_decode(state, (&(*result).psa_aead_encrypt_req_key)))) &&
       ((zcbor_uint32_decode(state, (&(*result).psa_aead_encrypt_req_alg)))) &&
       ((decode_ptr_buf(state, (&(*result).psa_aead_encrypt_req_p_nonce)))) &&
       ((decode_buf_len(state,
                        (&(*result).psa_aead_encrypt_req_nonce_length)))) &&
       ((decode_ptr_buf(
           state, (&(*result).psa_aead_encrypt_req_p_additional_data)))) &&
       ((decode_buf_len(
           state, (&(*result).psa_aead_encrypt_req_additional_data_length)))) &&
       ((decode_ptr_buf(state,
                        (&(*result).psa_aead_encrypt_req_p_plaintext)))) &&
       ((decode_buf_len(state,
                        (&(*result).psa_aead_encrypt_req_plaintext_length)))) &&
       ((decode_ptr_buf(state,
                        (&(*result).psa_aead_encrypt_req_p_ciphertext)))) &&
       ((decode_buf_len(state,
                        (&(*result).psa_aead_encrypt_req_ciphertext_size)))) &&
       ((decode_ptr_uint(
           state, (&(*result).psa_aead_encrypt_req_p_ciphertext_length)))))));

  log_result(state, res, __func__);
  return res;
}

static bool decode_psa_aead_decrypt_req(zcbor_state_t *state,
                                        struct psa_aead_decrypt_req *result) {
  zcbor_log("%s\r\n", __func__);

  bool res = (((
      ((zcbor_uint32_expect(state, (45)))) &&
      ((zcbor_uint32_decode(state, (&(*result).psa_aead_decrypt_req_key)))) &&
      ((zcbor_uint32_decode(state, (&(*result).psa_aead_decrypt_req_alg)))) &&
      ((decode_ptr_buf(state, (&(*result).psa_aead_decrypt_req_p_nonce)))) &&
      ((decode_buf_len(state,
                       (&(*result).psa_aead_decrypt_req_nonce_length)))) &&
      ((decode_ptr_buf(state,
                       (&(*result).psa_aead_decrypt_req_p_additional_data)))) &&
      ((decode_buf_len(
          state, (&(*result).psa_aead_decrypt_req_additional_data_length)))) &&
      ((decode_ptr_buf(state,
                       (&(*result).psa_aead_decrypt_req_p_ciphertext)))) &&
      ((decode_buf_len(state,
                       (&(*result).psa_aead_decrypt_req_ciphertext_length)))) &&
      ((decode_ptr_buf(state,
                       (&(*result).psa_aead_decrypt_req_p_plaintext)))) &&
      ((decode_buf_len(state,
                       (&(*result).psa_aead_decrypt_req_plaintext_size)))) &&
      ((decode_ptr_uint(
          state, (&(*result).psa_aead_decrypt_req_p_plaintext_length)))))));

  log_result(state, res, __func__);
  return res;
}

static bool
decode_psa_aead_encrypt_setup_req(zcbor_state_t *state,
                                  struct psa_aead_encrypt_setup_req *result) {
  zcbor_log("%s\r\n", __func__);

  bool res =
      (((((zcbor_uint32_expect(state, (46)))) &&
         ((decode_ptr_uint(
             state, (&(*result).psa_aead_encrypt_setup_req_p_handle)))) &&
         ((zcbor_uint32_decode(state,
                               (&(*result).psa_aead_encrypt_setup_req_key)))) &&
         ((zcbor_uint32_decode(
             state, (&(*result).psa_aead_encrypt_setup_req_alg)))))));

  log_result(state, res, __func__);
  return res;
}

static bool
decode_psa_aead_decrypt_setup_req(zcbor_state_t *state,
                                  struct psa_aead_decrypt_setup_req *result) {
  zcbor_log("%s\r\n", __func__);

  bool res =
      (((((zcbor_uint32_expect(state, (47)))) &&
         ((decode_ptr_uint(
             state, (&(*result).psa_aead_decrypt_setup_req_p_handle)))) &&
         ((zcbor_uint32_decode(state,
                               (&(*result).psa_aead_decrypt_setup_req_key)))) &&
         ((zcbor_uint32_decode(
             state, (&(*result).psa_aead_decrypt_setup_req_alg)))))));

  log_result(state, res, __func__);
  return res;
}

static bool
decode_psa_aead_generate_nonce_req(zcbor_state_t *state,
                                   struct psa_aead_generate_nonce_req *result) {
  zcbor_log("%s\r\n", __func__);

  bool res = ((
      (((zcbor_uint32_expect(state, (48)))) &&
       ((decode_ptr_uint(state,
                         (&(*result).psa_aead_generate_nonce_req_p_handle)))) &&
       ((decode_ptr_buf(state,
                        (&(*result).psa_aead_generate_nonce_req_p_nonce)))) &&
       ((decode_buf_len(
           state, (&(*result).psa_aead_generate_nonce_req_nonce_size)))) &&
       ((decode_ptr_uint(
           state, (&(*result).psa_aead_generate_nonce_req_p_nonce_length)))))));

  log_result(state, res, __func__);
  return res;
}

static bool
decode_psa_aead_set_nonce_req(zcbor_state_t *state,
                              struct psa_aead_set_nonce_req *result) {
  zcbor_log("%s\r\n", __func__);

  bool res = ((
      (((zcbor_uint32_expect(state, (49)))) &&
       ((decode_ptr_uint(state,
                         (&(*result).psa_aead_set_nonce_req_p_handle)))) &&
       ((decode_ptr_buf(state, (&(*result).psa_aead_set_nonce_req_p_nonce)))) &&
       ((decode_buf_len(state,
                        (&(*result).psa_aead_set_nonce_req_nonce_length)))))));

  log_result(state, res, __func__);
  return res;
}

static bool
decode_psa_aead_set_lengths_req(zcbor_state_t *state,
                                struct psa_aead_set_lengths_req *result) {
  zcbor_log("%s\r\n", __func__);

  bool res = ((
      (((zcbor_uint32_expect(state, (50)))) &&
       ((decode_ptr_uint(state,
                         (&(*result).psa_aead_set_lengths_req_p_handle)))) &&
       ((decode_buf_len(state,
                        (&(*result).psa_aead_set_lengths_req_ad_length)))) &&
       ((decode_buf_len(
           state, (&(*result).psa_aead_set_lengths_req_plaintext_length)))))));

  log_result(state, res, __func__);
  return res;
}

static bool
decode_psa_aead_update_ad_req(zcbor_state_t *state,
                              struct psa_aead_update_ad_req *result) {
  zcbor_log("%s\r\n", __func__);

  bool res = ((
      (((zcbor_uint32_expect(state, (51)))) &&
       ((decode_ptr_uint(state,
                         (&(*result).psa_aead_update_ad_req_p_handle)))) &&
       ((decode_ptr_buf(state, (&(*result).psa_aead_update_ad_req_p_input)))) &&
       ((decode_buf_len(state,
                        (&(*result).psa_aead_update_ad_req_input_length)))))));

  log_result(state, res, __func__);
  return res;
}

static bool decode_psa_aead_update_req(zcbor_state_t *state,
                                       struct psa_aead_update_req *result) {
  zcbor_log("%s\r\n", __func__);

  bool res = (((
      ((zcbor_uint32_expect(state, (52)))) &&
      ((decode_ptr_uint(state, (&(*result).psa_aead_update_req_p_handle)))) &&
      ((decode_ptr_buf(state, (&(*result).psa_aead_update_req_p_input)))) &&
      ((decode_buf_len(state,
                       (&(*result).psa_aead_update_req_input_length)))) &&
      ((decode_ptr_buf(state, (&(*result).psa_aead_update_req_p_output)))) &&
      ((decode_buf_len(state, (&(*result).psa_aead_update_req_output_size)))) &&
      ((decode_ptr_uint(state,
                        (&(*result).psa_aead_update_req_p_output_length)))))));

  log_result(state, res, __func__);
  return res;
}

static bool decode_psa_aead_finish_req(zcbor_state_t *state,
                                       struct psa_aead_finish_req *result) {
  zcbor_log("%s\r\n", __func__);

  bool res = ((
      (((zcbor_uint32_expect(state, (53)))) &&
       ((decode_ptr_uint(state, (&(*result).psa_aead_finish_req_p_handle)))) &&
       ((decode_ptr_buf(state,
                        (&(*result).psa_aead_finish_req_p_ciphertext)))) &&
       ((decode_buf_len(state,
                        (&(*result).psa_aead_finish_req_ciphertext_size)))) &&
       ((decode_ptr_uint(
           state, (&(*result).psa_aead_finish_req_p_ciphertext_length)))) &&
       ((decode_ptr_buf(state, (&(*result).psa_aead_finish_req_p_tag)))) &&
       ((decode_buf_len(state, (&(*result).psa_aead_finish_req_tag_size)))) &&
       ((decode_ptr_uint(state,
                         (&(*result).psa_aead_finish_req_p_tag_length)))))));

  log_result(state, res, __func__);
  return res;
}

static bool decode_psa_aead_verify_req(zcbor_state_t *state,
                                       struct psa_aead_verify_req *result) {
  zcbor_log("%s\r\n", __func__);

  bool res = (((
      ((zcbor_uint32_expect(state, (54)))) &&
      ((decode_ptr_uint(state, (&(*result).psa_aead_verify_req_p_handle)))) &&
      ((decode_ptr_buf(state, (&(*result).psa_aead_verify_req_p_plaintext)))) &&
      ((decode_buf_len(state,
                       (&(*result).psa_aead_verify_req_plaintext_size)))) &&
      ((decode_ptr_uint(
          state, (&(*result).psa_aead_verify_req_p_plaintext_length)))) &&
      ((decode_ptr_buf(state, (&(*result).psa_aead_verify_req_p_tag)))) &&
      ((decode_buf_len(state, (&(*result).psa_aead_verify_req_tag_length)))))));

  log_result(state, res, __func__);
  return res;
}

static bool decode_psa_aead_abort_req(zcbor_state_t *state,
                                      struct psa_aead_abort_req *result) {
  zcbor_log("%s\r\n", __func__);

  bool res = ((
      (((zcbor_uint32_expect(state, (55)))) &&
       ((decode_ptr_uint(state, (&(*result).psa_aead_abort_req_p_handle)))))));

  log_result(state, res, __func__);
  return res;
}

static bool decode_psa_sign_message_req(zcbor_state_t *state,
                                        struct psa_sign_message_req *result) {
  zcbor_log("%s\r\n", __func__);

  bool res = ((
      (((zcbor_uint32_expect(state, (56)))) &&
       ((zcbor_uint32_decode(state, (&(*result).psa_sign_message_req_key)))) &&
       ((zcbor_uint32_decode(state, (&(*result).psa_sign_message_req_alg)))) &&
       ((decode_ptr_buf(state, (&(*result).psa_sign_message_req_p_input)))) &&
       ((decode_buf_len(state,
                        (&(*result).psa_sign_message_req_input_length)))) &&
       ((decode_ptr_buf(state,
                        (&(*result).psa_sign_message_req_p_signature)))) &&
       ((decode_buf_len(state,
                        (&(*result).psa_sign_message_req_signature_size)))) &&
       ((decode_ptr_uint(
           state, (&(*result).psa_sign_message_req_p_signature_length)))))));

  log_result(state, res, __func__);
  return res;
}

static bool
decode_psa_verify_message_req(zcbor_state_t *state,
                              struct psa_verify_message_req *result) {
  zcbor_log("%s\r\n", __func__);

  bool res = (((
      ((zcbor_uint32_expect(state, (57)))) &&
      ((zcbor_uint32_decode(state, (&(*result).psa_verify_message_req_key)))) &&
      ((zcbor_uint32_decode(state, (&(*result).psa_verify_message_req_alg)))) &&
      ((decode_ptr_buf(state, (&(*result).psa_verify_message_req_p_input)))) &&
      ((decode_buf_len(state,
                       (&(*result).psa_verify_message_req_input_length)))) &&
      ((decode_ptr_buf(state,
                       (&(*result).psa_verify_message_req_p_signature)))) &&
      ((decode_buf_len(
          state, (&(*result).psa_verify_message_req_signature_length)))))));

  log_result(state, res, __func__);
  return res;
}

static bool decode_psa_sign_hash_req(zcbor_state_t *state,
                                     struct psa_sign_hash_req *result) {
  zcbor_log("%s\r\n", __func__);

  bool res = ((
      (((zcbor_uint32_expect(state, (58)))) &&
       ((zcbor_uint32_decode(state, (&(*result).psa_sign_hash_req_key)))) &&
       ((zcbor_uint32_decode(state, (&(*result).psa_sign_hash_req_alg)))) &&
       ((decode_ptr_buf(state, (&(*result).psa_sign_hash_req_p_hash)))) &&
       ((decode_buf_len(state, (&(*result).psa_sign_hash_req_hash_length)))) &&
       ((decode_ptr_buf(state, (&(*result).psa_sign_hash_req_p_signature)))) &&
       ((decode_buf_len(state,
                        (&(*result).psa_sign_hash_req_signature_size)))) &&
       ((decode_ptr_uint(
           state, (&(*result).psa_sign_hash_req_p_signature_length)))))));

  log_result(state, res, __func__);
  return res;
}

static bool decode_psa_verify_hash_req(zcbor_state_t *state,
                                       struct psa_verify_hash_req *result) {
  zcbor_log("%s\r\n", __func__);

  bool res = (((
      ((zcbor_uint32_expect(state, (59)))) &&
      ((zcbor_uint32_decode(state, (&(*result).psa_verify_hash_req_key)))) &&
      ((zcbor_uint32_decode(state, (&(*result).psa_verify_hash_req_alg)))) &&
      ((decode_ptr_buf(state, (&(*result).psa_verify_hash_req_p_hash)))) &&
      ((decode_buf_len(state, (&(*result).psa_verify_hash_req_hash_length)))) &&
      ((decode_ptr_buf(state, (&(*result).psa_verify_hash_req_p_signature)))) &&
      ((decode_buf_len(state,
                       (&(*result).psa_verify_hash_req_signature_length)))))));

  log_result(state, res, __func__);
  return res;
}

static bool
decode_psa_asymmetric_encrypt_req(zcbor_state_t *state,
                                  struct psa_asymmetric_encrypt_req *result) {
  zcbor_log("%s\r\n", __func__);

  bool res = (((
      ((zcbor_uint32_expect(state, (60)))) &&
      ((zcbor_uint32_decode(state,
                            (&(*result).psa_asymmetric_encrypt_req_key)))) &&
      ((zcbor_uint32_decode(state,
                            (&(*result).psa_asymmetric_encrypt_req_alg)))) &&
      ((decode_ptr_buf(state,
                       (&(*result).psa_asymmetric_encrypt_req_p_input)))) &&
      ((decode_buf_len(
          state, (&(*result).psa_asymmetric_encrypt_req_input_length)))) &&
      ((decode_ptr_buf(state,
                       (&(*result).psa_asymmetric_encrypt_req_p_salt)))) &&
      ((decode_buf_len(state,
                       (&(*result).psa_asymmetric_encrypt_req_salt_length)))) &&
      ((decode_ptr_buf(state,
                       (&(*result).psa_asymmetric_encrypt_req_p_output)))) &&
      ((decode_buf_len(state,
                       (&(*result).psa_asymmetric_encrypt_req_output_size)))) &&
      ((decode_ptr_uint(
          state, (&(*result).psa_asymmetric_encrypt_req_p_output_length)))))));

  log_result(state, res, __func__);
  return res;
}

static bool
decode_psa_asymmetric_decrypt_req(zcbor_state_t *state,
                                  struct psa_asymmetric_decrypt_req *result) {
  zcbor_log("%s\r\n", __func__);

  bool res = (((
      ((zcbor_uint32_expect(state, (61)))) &&
      ((zcbor_uint32_decode(state,
                            (&(*result).psa_asymmetric_decrypt_req_key)))) &&
      ((zcbor_uint32_decode(state,
                            (&(*result).psa_asymmetric_decrypt_req_alg)))) &&
      ((decode_ptr_buf(state,
                       (&(*result).psa_asymmetric_decrypt_req_p_input)))) &&
      ((decode_buf_len(
          state, (&(*result).psa_asymmetric_decrypt_req_input_length)))) &&
      ((decode_ptr_buf(state,
                       (&(*result).psa_asymmetric_decrypt_req_p_salt)))) &&
      ((decode_buf_len(state,
                       (&(*result).psa_asymmetric_decrypt_req_salt_length)))) &&
      ((decode_ptr_buf(state,
                       (&(*result).psa_asymmetric_decrypt_req_p_output)))) &&
      ((decode_buf_len(state,
                       (&(*result).psa_asymmetric_decrypt_req_output_size)))) &&
      ((decode_ptr_uint(
          state, (&(*result).psa_asymmetric_decrypt_req_p_output_length)))))));

  log_result(state, res, __func__);
  return res;
}

static bool decode_psa_key_derivation_setup_req(
    zcbor_state_t *state, struct psa_key_derivation_setup_req *result) {
  zcbor_log("%s\r\n", __func__);

  bool res =
      (((((zcbor_uint32_expect(state, (62)))) &&
         ((decode_ptr_uint(
             state, (&(*result).psa_key_derivation_setup_req_p_handle)))) &&
         ((zcbor_uint32_decode(
             state, (&(*result).psa_key_derivation_setup_req_alg)))))));

  log_result(state, res, __func__);
  return res;
}

static bool decode_psa_key_derivation_get_capacity_req(
    zcbor_state_t *state, struct psa_key_derivation_get_capacity_req *result) {
  zcbor_log("%s\r\n", __func__);

  bool res = ((
      (((zcbor_uint32_expect(state, (63)))) &&
       ((zcbor_uint32_decode(
           state, (&(*result).psa_key_derivation_get_capacity_req_handle)))) &&
       ((decode_ptr_uint(
           state,
           (&(*result).psa_key_derivation_get_capacity_req_p_capacity)))))));

  log_result(state, res, __func__);
  return res;
}

static bool decode_psa_key_derivation_set_capacity_req(
    zcbor_state_t *state, struct psa_key_derivation_set_capacity_req *result) {
  zcbor_log("%s\r\n", __func__);

  bool res = (((
      ((zcbor_uint32_expect(state, (64)))) &&
      ((decode_ptr_uint(
          state, (&(*result).psa_key_derivation_set_capacity_req_p_handle)))) &&
      ((zcbor_uint32_decode(
          state,
          (&(*result).psa_key_derivation_set_capacity_req_capacity)))))));

  log_result(state, res, __func__);
  return res;
}

static bool decode_psa_key_derivation_input_bytes_req(
    zcbor_state_t *state, struct psa_key_derivation_input_bytes_req *result) {
  zcbor_log("%s\r\n", __func__);

  bool res = ((
      (((zcbor_uint32_expect(state, (65)))) &&
       ((decode_ptr_uint(
           state, (&(*result).psa_key_derivation_input_bytes_req_p_handle)))) &&
       ((zcbor_uint32_decode(
           state, (&(*result).psa_key_derivation_input_bytes_req_step)))) &&
       ((decode_ptr_buf(
           state, (&(*result).psa_key_derivation_input_bytes_req_p_data)))) &&
       ((decode_buf_len(
           state,
           (&(*result).psa_key_derivation_input_bytes_req_data_length)))))));

  log_result(state, res, __func__);
  return res;
}

static bool decode_psa_key_derivation_input_integer_req(
    zcbor_state_t *state, struct psa_key_derivation_input_integer_req *result) {
  zcbor_log("%s\r\n", __func__);

  bool res = ((
      (((zcbor_uint32_expect(state, (66)))) &&
       ((decode_ptr_uint(
           state,
           (&(*result).psa_key_derivation_input_integer_req_p_handle)))) &&
       ((zcbor_uint32_decode(
           state, (&(*result).psa_key_derivation_input_integer_req_step)))) &&
       ((zcbor_uint32_decode(
           state, (&(*result).psa_key_derivation_input_integer_req_value)))))));

  log_result(state, res, __func__);
  return res;
}

static bool decode_psa_key_derivation_input_key_req(
    zcbor_state_t *state, struct psa_key_derivation_input_key_req *result) {
  zcbor_log("%s\r\n", __func__);

  bool res =
      (((((zcbor_uint32_expect(state, (67)))) &&
         ((decode_ptr_uint(
             state, (&(*result).psa_key_derivation_input_key_req_p_handle)))) &&
         ((zcbor_uint32_decode(
             state, (&(*result).psa_key_derivation_input_key_req_step)))) &&
         ((zcbor_uint32_decode(
             state, (&(*result).psa_key_derivation_input_key_req_key)))))));

  log_result(state, res, __func__);
  return res;
}

static bool decode_psa_key_derivation_key_agreement_req(
    zcbor_state_t *state, struct psa_key_derivation_key_agreement_req *result) {
  zcbor_log("%s\r\n", __func__);

  bool res =
      (((((zcbor_uint32_expect(state, (68)))) &&
         ((decode_ptr_uint(
             state,
             (&(*result).psa_key_derivation_key_agreement_req_p_handle)))) &&
         ((zcbor_uint32_decode(
             state, (&(*result).psa_key_derivation_key_agreement_req_step)))) &&
         ((zcbor_uint32_decode(
             state,
             (&(*result).psa_key_derivation_key_agreement_req_private_key)))) &&
         ((decode_ptr_buf(
             state,
             (&(*result).psa_key_derivation_key_agreement_req_p_peer_key)))) &&
         ((decode_buf_len(
             state,
             (&(*result)
                   .psa_key_derivation_key_agreement_req_peer_key_length)))))));

  log_result(state, res, __func__);
  return res;
}

static bool decode_psa_key_derivation_output_bytes_req(
    zcbor_state_t *state, struct psa_key_derivation_output_bytes_req *result) {
  zcbor_log("%s\r\n", __func__);

  bool res = (((
      ((zcbor_uint32_expect(state, (69)))) &&
      ((decode_ptr_uint(
          state, (&(*result).psa_key_derivation_output_bytes_req_p_handle)))) &&
      ((decode_ptr_buf(
          state, (&(*result).psa_key_derivation_output_bytes_req_p_output)))) &&
      ((decode_buf_len(
          state,
          (&(*result).psa_key_derivation_output_bytes_req_output_length)))))));

  log_result(state, res, __func__);
  return res;
}

static bool decode_psa_key_derivation_output_key_req(
    zcbor_state_t *state, struct psa_key_derivation_output_key_req *result) {
  zcbor_log("%s\r\n", __func__);

  bool res = ((
      (((zcbor_uint32_expect(state, (70)))) &&
       ((decode_ptr_attr(
           state,
           (&(*result).psa_key_derivation_output_key_req_p_attributes)))) &&
       ((decode_ptr_uint(
           state, (&(*result).psa_key_derivation_output_key_req_p_handle)))) &&
       ((decode_ptr_key(
           state, (&(*result).psa_key_derivation_output_key_req_p_key)))))));

  log_result(state, res, __func__);
  return res;
}

static bool decode_psa_key_derivation_abort_req(
    zcbor_state_t *state, struct psa_key_derivation_abort_req *result) {
  zcbor_log("%s\r\n", __func__);

  bool res =
      (((((zcbor_uint32_expect(state, (71)))) &&
         ((decode_ptr_uint(
             state, (&(*result).psa_key_derivation_abort_req_p_handle)))))));

  log_result(state, res, __func__);
  return res;
}

static bool
decode_psa_raw_key_agreement_req(zcbor_state_t *state,
                                 struct psa_raw_key_agreement_req *result) {
  zcbor_log("%s\r\n", __func__);

  bool res = ((
      (((zcbor_uint32_expect(state, (72)))) &&
       ((zcbor_uint32_decode(state,
                             (&(*result).psa_raw_key_agreement_req_alg)))) &&
       ((zcbor_uint32_decode(
           state, (&(*result).psa_raw_key_agreement_req_private_key)))) &&
       ((decode_ptr_buf(state,
                        (&(*result).psa_raw_key_agreement_req_p_peer_key)))) &&
       ((decode_buf_len(
           state, (&(*result).psa_raw_key_agreement_req_peer_key_length)))) &&
       ((decode_ptr_buf(state,
                        (&(*result).psa_raw_key_agreement_req_p_output)))) &&
       ((decode_buf_len(state,
                        (&(*result).psa_raw_key_agreement_req_output_size)))) &&
       ((decode_ptr_uint(
           state, (&(*result).psa_raw_key_agreement_req_p_output_length)))))));

  log_result(state, res, __func__);
  return res;
}

static bool
decode_psa_generate_random_req(zcbor_state_t *state,
                               struct psa_generate_random_req *result) {
  zcbor_log("%s\r\n", __func__);

  bool res =
      (((((zcbor_uint32_expect(state, (73)))) &&
         ((decode_ptr_buf(state,
                          (&(*result).psa_generate_random_req_p_output)))) &&
         ((decode_buf_len(
             state, (&(*result).psa_generate_random_req_output_size)))))));

  log_result(state, res, __func__);
  return res;
}

static bool decode_psa_generate_key_req(zcbor_state_t *state,
                                        struct psa_generate_key_req *result) {
  zcbor_log("%s\r\n", __func__);

  bool res =
      (((((zcbor_uint32_expect(state, (74)))) &&
         ((decode_ptr_attr(state,
                           (&(*result).psa_generate_key_req_p_attributes)))) &&
         ((decode_ptr_key(state, (&(*result).psa_generate_key_req_p_key)))))));

  log_result(state, res, __func__);
  return res;
}

static bool decode_ptr_cipher(zcbor_state_t *state, uint32_t *result) {
  zcbor_log("%s\r\n", __func__);

  bool res = ((zcbor_tag_expect(state, 32775) &&
               (zcbor_uint32_decode(state, (&(*result))))));

  log_result(state, res, __func__);
  return res;
}

static bool decode_psa_pake_setup_req(zcbor_state_t *state,
                                      struct psa_pake_setup_req *result) {
  zcbor_log("%s\r\n", __func__);

  bool res =
      (((((zcbor_uint32_expect(state, (79)))) &&
         ((decode_ptr_uint(state, (&(*result).psa_pake_setup_req_p_handle)))) &&
         ((zcbor_uint32_decode(
             state, (&(*result).psa_pake_setup_req_password_key)))) &&
         ((decode_ptr_cipher(
             state, (&(*result).psa_pake_setup_req_p_cipher_suite)))))));

  log_result(state, res, __func__);
  return res;
}

static bool decode_psa_pake_set_role_req(zcbor_state_t *state,
                                         struct psa_pake_set_role_req *result) {
  zcbor_log("%s\r\n", __func__);

  bool res = (((
      ((zcbor_uint32_expect(state, (80)))) &&
      ((decode_ptr_uint(state, (&(*result).psa_pake_set_role_req_p_handle)))) &&
      ((zcbor_uint32_decode(state,
                            (&(*result).psa_pake_set_role_req_role)))))));

  log_result(state, res, __func__);
  return res;
}

static bool decode_psa_pake_set_user_req(zcbor_state_t *state,
                                         struct psa_pake_set_user_req *result) {
  zcbor_log("%s\r\n", __func__);

  bool res = (((
      ((zcbor_uint32_expect(state, (81)))) &&
      ((decode_ptr_uint(state, (&(*result).psa_pake_set_user_req_p_handle)))) &&
      ((decode_ptr_buf(state, (&(*result).psa_pake_set_user_req_p_user_id)))) &&
      ((decode_buf_len(state,
                       (&(*result).psa_pake_set_user_req_user_id_len)))))));

  log_result(state, res, __func__);
  return res;
}

static bool decode_psa_pake_set_peer_req(zcbor_state_t *state,
                                         struct psa_pake_set_peer_req *result) {
  zcbor_log("%s\r\n", __func__);

  bool res = (((
      ((zcbor_uint32_expect(state, (82)))) &&
      ((decode_ptr_uint(state, (&(*result).psa_pake_set_peer_req_p_handle)))) &&
      ((decode_ptr_buf(state, (&(*result).psa_pake_set_peer_req_p_peer_id)))) &&
      ((decode_buf_len(state,
                       (&(*result).psa_pake_set_peer_req_peer_id_len)))))));

  log_result(state, res, __func__);
  return res;
}

static bool
decode_psa_pake_set_context_req(zcbor_state_t *state,
                                struct psa_pake_set_context_req *result) {
  zcbor_log("%s\r\n", __func__);

  bool res =
      (((((zcbor_uint32_expect(state, (83)))) &&
         ((decode_ptr_uint(state,
                           (&(*result).psa_pake_set_context_req_p_handle)))) &&
         ((decode_ptr_buf(state,
                          (&(*result).psa_pake_set_context_req_p_context)))) &&
         ((decode_buf_len(
             state, (&(*result).psa_pake_set_context_req_context_len)))))));

  log_result(state, res, __func__);
  return res;
}

static bool decode_psa_pake_output_req(zcbor_state_t *state,
                                       struct psa_pake_output_req *result) {
  zcbor_log("%s\r\n", __func__);

  bool res = (((
      ((zcbor_uint32_expect(state, (84)))) &&
      ((decode_ptr_uint(state, (&(*result).psa_pake_output_req_p_handle)))) &&
      ((zcbor_uint32_decode(state, (&(*result).psa_pake_output_req_step)))) &&
      ((decode_ptr_buf(state, (&(*result).psa_pake_output_req_p_output)))) &&
      ((decode_buf_len(state, (&(*result).psa_pake_output_req_output_size)))) &&
      ((decode_ptr_uint(state,
                        (&(*result).psa_pake_output_req_p_output_length)))))));

  log_result(state, res, __func__);
  return res;
}

static bool decode_psa_pake_input_req(zcbor_state_t *state,
                                      struct psa_pake_input_req *result) {
  zcbor_log("%s\r\n", __func__);

  bool res =
      (((((zcbor_uint32_expect(state, (85)))) &&
         ((decode_ptr_uint(state, (&(*result).psa_pake_input_req_p_handle)))) &&
         ((zcbor_uint32_decode(state, (&(*result).psa_pake_input_req_step)))) &&
         ((decode_ptr_buf(state, (&(*result).psa_pake_input_req_p_input)))) &&
         ((decode_buf_len(state,
                          (&(*result).psa_pake_input_req_input_length)))))));

  log_result(state, res, __func__);
  return res;
}

static bool
decode_psa_pake_get_shared_key_req(zcbor_state_t *state,
                                   struct psa_pake_get_shared_key_req *result) {
  zcbor_log("%s\r\n", __func__);

  bool res =
      (((((zcbor_uint32_expect(state, (86)))) &&
         ((decode_ptr_uint(
             state, (&(*result).psa_pake_get_shared_key_req_p_handle)))) &&
         ((decode_ptr_attr(
             state, (&(*result).psa_pake_get_shared_key_req_p_attributes)))) &&
         ((decode_ptr_key(state,
                          (&(*result).psa_pake_get_shared_key_req_p_key)))))));

  log_result(state, res, __func__);
  return res;
}

static bool decode_psa_pake_abort_req(zcbor_state_t *state,
                                      struct psa_pake_abort_req *result) {
  zcbor_log("%s\r\n", __func__);

  bool res = ((
      (((zcbor_uint32_expect(state, (87)))) &&
       ((decode_ptr_uint(state, (&(*result).psa_pake_abort_req_p_handle)))))));

  log_result(state, res, __func__);
  return res;
}

static bool decode_psa_crypto_rsp(zcbor_state_t *state,
                                  struct psa_crypto_rsp *result) {
  zcbor_log("%s\r\n", __func__);

  bool res =
      (((zcbor_list_start_decode(state) &&
         ((((zcbor_uint32_decode(state, (&(*result).psa_crypto_rsp_id)))) &&
           ((zcbor_int32_decode(state, (&(*result).psa_crypto_rsp_status))))) ||
          (zcbor_list_map_end_force_decode(state), false)) &&
         zcbor_list_end_decode(state))));

  log_result(state, res, __func__);
  return res;
}

static bool decode_psa_crypto_req(zcbor_state_t *state,
                                  struct psa_crypto_req *result) {
  zcbor_log("%s\r\n", __func__);
  bool int_res;

  bool res = (((
      zcbor_list_start_decode(state) &&
      ((((zcbor_union_start_code(state) &&
          (int_res =
               ((((zcbor_uint32_expect_union(state, (10)))) &&
                 (((*result).psa_crypto_req_msg_choice =
                       psa_crypto_req_msg_psa_crypto_init_req_m_c),
                  true)) ||
                (((decode_psa_get_key_attributes_req(
                     state,
                     (&(*result)
                           .psa_crypto_req_msg_psa_get_key_attributes_req_m)))) &&
                 (((*result).psa_crypto_req_msg_choice =
                       psa_crypto_req_msg_psa_get_key_attributes_req_m_c),
                  true)) ||
                (zcbor_union_elem_code(state) &&
                 (((decode_psa_reset_key_attributes_req(
                      state,
                      (&(*result)
                            .psa_crypto_req_msg_psa_reset_key_attributes_req_m)))) &&
                  (((*result).psa_crypto_req_msg_choice =
                        psa_crypto_req_msg_psa_reset_key_attributes_req_m_c),
                   true))) ||
                (zcbor_union_elem_code(state) &&
                 (((decode_psa_purge_key_req(
                      state,
                      (&(*result).psa_crypto_req_msg_psa_purge_key_req_m)))) &&
                  (((*result).psa_crypto_req_msg_choice =
                        psa_crypto_req_msg_psa_purge_key_req_m_c),
                   true))) ||
                (zcbor_union_elem_code(state) &&
                 (((decode_psa_copy_key_req(
                      state,
                      (&(*result).psa_crypto_req_msg_psa_copy_key_req_m)))) &&
                  (((*result).psa_crypto_req_msg_choice =
                        psa_crypto_req_msg_psa_copy_key_req_m_c),
                   true))) ||
                (zcbor_union_elem_code(state) &&
                 (((decode_psa_destroy_key_req(
                      state,
                      (&(*result)
                            .psa_crypto_req_msg_psa_destroy_key_req_m)))) &&
                  (((*result).psa_crypto_req_msg_choice =
                        psa_crypto_req_msg_psa_destroy_key_req_m_c),
                   true))) ||
                (zcbor_union_elem_code(state) &&
                 (((decode_psa_import_key_req(
                      state,
                      (&(*result).psa_crypto_req_msg_psa_import_key_req_m)))) &&
                  (((*result).psa_crypto_req_msg_choice =
                        psa_crypto_req_msg_psa_import_key_req_m_c),
                   true))) ||
                (zcbor_union_elem_code(state) &&
                 (((decode_psa_export_key_req(
                      state,
                      (&(*result).psa_crypto_req_msg_psa_export_key_req_m)))) &&
                  (((*result).psa_crypto_req_msg_choice =
                        psa_crypto_req_msg_psa_export_key_req_m_c),
                   true))) ||
                (zcbor_union_elem_code(state) &&
                 (((decode_psa_export_public_key_req(
                      state,
                      (&(*result)
                            .psa_crypto_req_msg_psa_export_public_key_req_m)))) &&
                  (((*result).psa_crypto_req_msg_choice =
                        psa_crypto_req_msg_psa_export_public_key_req_m_c),
                   true))) ||
                (zcbor_union_elem_code(state) &&
                 (((decode_psa_hash_compute_req(
                      state,
                      (&(*result)
                            .psa_crypto_req_msg_psa_hash_compute_req_m)))) &&
                  (((*result).psa_crypto_req_msg_choice =
                        psa_crypto_req_msg_psa_hash_compute_req_m_c),
                   true))) ||
                (zcbor_union_elem_code(state) &&
                 (((decode_psa_hash_compare_req(
                      state,
                      (&(*result)
                            .psa_crypto_req_msg_psa_hash_compare_req_m)))) &&
                  (((*result).psa_crypto_req_msg_choice =
                        psa_crypto_req_msg_psa_hash_compare_req_m_c),
                   true))) ||
                (zcbor_union_elem_code(state) &&
                 (((decode_psa_hash_setup_req(
                      state,
                      (&(*result).psa_crypto_req_msg_psa_hash_setup_req_m)))) &&
                  (((*result).psa_crypto_req_msg_choice =
                        psa_crypto_req_msg_psa_hash_setup_req_m_c),
                   true))) ||
                (zcbor_union_elem_code(state) &&
                 (((decode_psa_hash_update_req(
                      state,
                      (&(*result)
                            .psa_crypto_req_msg_psa_hash_update_req_m)))) &&
                  (((*result).psa_crypto_req_msg_choice =
                        psa_crypto_req_msg_psa_hash_update_req_m_c),
                   true))) ||
                (zcbor_union_elem_code(state) &&
                 (((decode_psa_hash_finish_req(
                      state,
                      (&(*result)
                            .psa_crypto_req_msg_psa_hash_finish_req_m)))) &&
                  (((*result).psa_crypto_req_msg_choice =
                        psa_crypto_req_msg_psa_hash_finish_req_m_c),
                   true))) ||
                (zcbor_union_elem_code(state) &&
                 (((decode_psa_hash_verify_req(
                      state,
                      (&(*result)
                            .psa_crypto_req_msg_psa_hash_verify_req_m)))) &&
                  (((*result).psa_crypto_req_msg_choice =
                        psa_crypto_req_msg_psa_hash_verify_req_m_c),
                   true))) ||
                (zcbor_union_elem_code(state) &&
                 (((decode_psa_hash_abort_req(
                      state,
                      (&(*result).psa_crypto_req_msg_psa_hash_abort_req_m)))) &&
                  (((*result).psa_crypto_req_msg_choice =
                        psa_crypto_req_msg_psa_hash_abort_req_m_c),
                   true))) ||
                (zcbor_union_elem_code(state) &&
                 (((decode_psa_hash_clone_req(
                      state,
                      (&(*result).psa_crypto_req_msg_psa_hash_clone_req_m)))) &&
                  (((*result).psa_crypto_req_msg_choice =
                        psa_crypto_req_msg_psa_hash_clone_req_m_c),
                   true))) ||
                (zcbor_union_elem_code(state) &&
                 (((decode_psa_mac_compute_req(
                      state,
                      (&(*result)
                            .psa_crypto_req_msg_psa_mac_compute_req_m)))) &&
                  (((*result).psa_crypto_req_msg_choice =
                        psa_crypto_req_msg_psa_mac_compute_req_m_c),
                   true))) ||
                (zcbor_union_elem_code(state) &&
                 (((decode_psa_mac_verify_req(
                      state,
                      (&(*result).psa_crypto_req_msg_psa_mac_verify_req_m)))) &&
                  (((*result).psa_crypto_req_msg_choice =
                        psa_crypto_req_msg_psa_mac_verify_req_m_c),
                   true))) ||
                (zcbor_union_elem_code(state) &&
                 (((decode_psa_mac_sign_setup_req(
                      state,
                      (&(*result)
                            .psa_crypto_req_msg_psa_mac_sign_setup_req_m)))) &&
                  (((*result).psa_crypto_req_msg_choice =
                        psa_crypto_req_msg_psa_mac_sign_setup_req_m_c),
                   true))) ||
                (zcbor_union_elem_code(state) &&
                 (((decode_psa_mac_verify_setup_req(
                      state,
                      (&(*result)
                            .psa_crypto_req_msg_psa_mac_verify_setup_req_m)))) &&
                  (((*result).psa_crypto_req_msg_choice =
                        psa_crypto_req_msg_psa_mac_verify_setup_req_m_c),
                   true))) ||
                (zcbor_union_elem_code(state) &&
                 (((decode_psa_mac_update_req(
                      state,
                      (&(*result).psa_crypto_req_msg_psa_mac_update_req_m)))) &&
                  (((*result).psa_crypto_req_msg_choice =
                        psa_crypto_req_msg_psa_mac_update_req_m_c),
                   true))) ||
                (zcbor_union_elem_code(state) &&
                 (((decode_psa_mac_sign_finish_req(
                      state,
                      (&(*result)
                            .psa_crypto_req_msg_psa_mac_sign_finish_req_m)))) &&
                  (((*result).psa_crypto_req_msg_choice =
                        psa_crypto_req_msg_psa_mac_sign_finish_req_m_c),
                   true))) ||
                (zcbor_union_elem_code(state) &&
                 (((decode_psa_mac_verify_finish_req(
                      state,
                      (&(*result)
                            .psa_crypto_req_msg_psa_mac_verify_finish_req_m)))) &&
                  (((*result).psa_crypto_req_msg_choice =
                        psa_crypto_req_msg_psa_mac_verify_finish_req_m_c),
                   true))) ||
                (zcbor_union_elem_code(state) &&
                 (((decode_psa_mac_abort_req(
                      state,
                      (&(*result).psa_crypto_req_msg_psa_mac_abort_req_m)))) &&
                  (((*result).psa_crypto_req_msg_choice =
                        psa_crypto_req_msg_psa_mac_abort_req_m_c),
                   true))) ||
                (zcbor_union_elem_code(state) &&
                 (((decode_psa_cipher_encrypt_req(
                      state,
                      (&(*result)
                            .psa_crypto_req_msg_psa_cipher_encrypt_req_m)))) &&
                  (((*result).psa_crypto_req_msg_choice =
                        psa_crypto_req_msg_psa_cipher_encrypt_req_m_c),
                   true))) ||
                (zcbor_union_elem_code(state) &&
                 (((decode_psa_cipher_decrypt_req(
                      state,
                      (&(*result)
                            .psa_crypto_req_msg_psa_cipher_decrypt_req_m)))) &&
                  (((*result).psa_crypto_req_msg_choice =
                        psa_crypto_req_msg_psa_cipher_decrypt_req_m_c),
                   true))) ||
                (zcbor_union_elem_code(state) &&
                 (((decode_psa_cipher_encrypt_setup_req(
                      state,
                      (&(*result)
                            .psa_crypto_req_msg_psa_cipher_encrypt_setup_req_m)))) &&
                  (((*result).psa_crypto_req_msg_choice =
                        psa_crypto_req_msg_psa_cipher_encrypt_setup_req_m_c),
                   true))) ||
                (zcbor_union_elem_code(state) &&
                 (((decode_psa_cipher_decrypt_setup_req(
                      state,
                      (&(*result)
                            .psa_crypto_req_msg_psa_cipher_decrypt_setup_req_m)))) &&
                  (((*result).psa_crypto_req_msg_choice =
                        psa_crypto_req_msg_psa_cipher_decrypt_setup_req_m_c),
                   true))) ||
                (zcbor_union_elem_code(state) &&
                 (((decode_psa_cipher_generate_iv_req(
                      state,
                      (&(*result)
                            .psa_crypto_req_msg_psa_cipher_generate_iv_req_m)))) &&
                  (((*result).psa_crypto_req_msg_choice =
                        psa_crypto_req_msg_psa_cipher_generate_iv_req_m_c),
                   true))) ||
                (zcbor_union_elem_code(state) &&
                 (((decode_psa_cipher_set_iv_req(
                      state,
                      (&(*result)
                            .psa_crypto_req_msg_psa_cipher_set_iv_req_m)))) &&
                  (((*result).psa_crypto_req_msg_choice =
                        psa_crypto_req_msg_psa_cipher_set_iv_req_m_c),
                   true))) ||
                (zcbor_union_elem_code(state) &&
                 (((decode_psa_cipher_update_req(
                      state,
                      (&(*result)
                            .psa_crypto_req_msg_psa_cipher_update_req_m)))) &&
                  (((*result).psa_crypto_req_msg_choice =
                        psa_crypto_req_msg_psa_cipher_update_req_m_c),
                   true))) ||
                (zcbor_union_elem_code(state) &&
                 (((decode_psa_cipher_finish_req(
                      state,
                      (&(*result)
                            .psa_crypto_req_msg_psa_cipher_finish_req_m)))) &&
                  (((*result).psa_crypto_req_msg_choice =
                        psa_crypto_req_msg_psa_cipher_finish_req_m_c),
                   true))) ||
                (zcbor_union_elem_code(state) &&
                 (((decode_psa_cipher_abort_req(
                      state,
                      (&(*result)
                            .psa_crypto_req_msg_psa_cipher_abort_req_m)))) &&
                  (((*result).psa_crypto_req_msg_choice =
                        psa_crypto_req_msg_psa_cipher_abort_req_m_c),
                   true))) ||
                (zcbor_union_elem_code(state) &&
                 (((decode_psa_aead_encrypt_req(
                      state,
                      (&(*result)
                            .psa_crypto_req_msg_psa_aead_encrypt_req_m)))) &&
                  (((*result).psa_crypto_req_msg_choice =
                        psa_crypto_req_msg_psa_aead_encrypt_req_m_c),
                   true))) ||
                (zcbor_union_elem_code(state) &&
                 (((decode_psa_aead_decrypt_req(
                      state,
                      (&(*result)
                            .psa_crypto_req_msg_psa_aead_decrypt_req_m)))) &&
                  (((*result).psa_crypto_req_msg_choice =
                        psa_crypto_req_msg_psa_aead_decrypt_req_m_c),
                   true))) ||
                (zcbor_union_elem_code(state) &&
                 (((decode_psa_aead_encrypt_setup_req(
                      state,
                      (&(*result)
                            .psa_crypto_req_msg_psa_aead_encrypt_setup_req_m)))) &&
                  (((*result).psa_crypto_req_msg_choice =
                        psa_crypto_req_msg_psa_aead_encrypt_setup_req_m_c),
                   true))) ||
                (zcbor_union_elem_code(state) &&
                 (((decode_psa_aead_decrypt_setup_req(
                      state,
                      (&(*result)
                            .psa_crypto_req_msg_psa_aead_decrypt_setup_req_m)))) &&
                  (((*result).psa_crypto_req_msg_choice =
                        psa_crypto_req_msg_psa_aead_decrypt_setup_req_m_c),
                   true))) ||
                (zcbor_union_elem_code(state) &&
                 (((decode_psa_aead_generate_nonce_req(
                      state,
                      (&(*result)
                            .psa_crypto_req_msg_psa_aead_generate_nonce_req_m)))) &&
                  (((*result).psa_crypto_req_msg_choice =
                        psa_crypto_req_msg_psa_aead_generate_nonce_req_m_c),
                   true))) ||
                (zcbor_union_elem_code(state) &&
                 (((decode_psa_aead_set_nonce_req(
                      state,
                      (&(*result)
                            .psa_crypto_req_msg_psa_aead_set_nonce_req_m)))) &&
                  (((*result).psa_crypto_req_msg_choice =
                        psa_crypto_req_msg_psa_aead_set_nonce_req_m_c),
                   true))) ||
                (zcbor_union_elem_code(state) &&
                 (((decode_psa_aead_set_lengths_req(
                      state,
                      (&(*result)
                            .psa_crypto_req_msg_psa_aead_set_lengths_req_m)))) &&
                  (((*result).psa_crypto_req_msg_choice =
                        psa_crypto_req_msg_psa_aead_set_lengths_req_m_c),
                   true))) ||
                (zcbor_union_elem_code(state) &&
                 (((decode_psa_aead_update_ad_req(
                      state,
                      (&(*result)
                            .psa_crypto_req_msg_psa_aead_update_ad_req_m)))) &&
                  (((*result).psa_crypto_req_msg_choice =
                        psa_crypto_req_msg_psa_aead_update_ad_req_m_c),
                   true))) ||
                (zcbor_union_elem_code(state) &&
                 (((decode_psa_aead_update_req(
                      state,
                      (&(*result)
                            .psa_crypto_req_msg_psa_aead_update_req_m)))) &&
                  (((*result).psa_crypto_req_msg_choice =
                        psa_crypto_req_msg_psa_aead_update_req_m_c),
                   true))) ||
                (zcbor_union_elem_code(state) &&
                 (((decode_psa_aead_finish_req(
                      state,
                      (&(*result)
                            .psa_crypto_req_msg_psa_aead_finish_req_m)))) &&
                  (((*result).psa_crypto_req_msg_choice =
                        psa_crypto_req_msg_psa_aead_finish_req_m_c),
                   true))) ||
                (zcbor_union_elem_code(state) &&
                 (((decode_psa_aead_verify_req(
                      state,
                      (&(*result)
                            .psa_crypto_req_msg_psa_aead_verify_req_m)))) &&
                  (((*result).psa_crypto_req_msg_choice =
                        psa_crypto_req_msg_psa_aead_verify_req_m_c),
                   true))) ||
                (zcbor_union_elem_code(state) &&
                 (((decode_psa_aead_abort_req(
                      state,
                      (&(*result).psa_crypto_req_msg_psa_aead_abort_req_m)))) &&
                  (((*result).psa_crypto_req_msg_choice =
                        psa_crypto_req_msg_psa_aead_abort_req_m_c),
                   true))) ||
                (zcbor_union_elem_code(state) &&
                 (((decode_psa_sign_message_req(
                      state,
                      (&(*result)
                            .psa_crypto_req_msg_psa_sign_message_req_m)))) &&
                  (((*result).psa_crypto_req_msg_choice =
                        psa_crypto_req_msg_psa_sign_message_req_m_c),
                   true))) ||
                (zcbor_union_elem_code(state) &&
                 (((decode_psa_verify_message_req(
                      state,
                      (&(*result)
                            .psa_crypto_req_msg_psa_verify_message_req_m)))) &&
                  (((*result).psa_crypto_req_msg_choice =
                        psa_crypto_req_msg_psa_verify_message_req_m_c),
                   true))) ||
                (zcbor_union_elem_code(state) &&
                 (((decode_psa_sign_hash_req(
                      state,
                      (&(*result).psa_crypto_req_msg_psa_sign_hash_req_m)))) &&
                  (((*result).psa_crypto_req_msg_choice =
                        psa_crypto_req_msg_psa_sign_hash_req_m_c),
                   true))) ||
                (zcbor_union_elem_code(state) &&
                 (((decode_psa_verify_hash_req(
                      state,
                      (&(*result)
                            .psa_crypto_req_msg_psa_verify_hash_req_m)))) &&
                  (((*result).psa_crypto_req_msg_choice =
                        psa_crypto_req_msg_psa_verify_hash_req_m_c),
                   true))) ||
                (zcbor_union_elem_code(state) &&
                 (((decode_psa_asymmetric_encrypt_req(
                      state,
                      (&(*result)
                            .psa_crypto_req_msg_psa_asymmetric_encrypt_req_m)))) &&
                  (((*result).psa_crypto_req_msg_choice =
                        psa_crypto_req_msg_psa_asymmetric_encrypt_req_m_c),
                   true))) ||
                (zcbor_union_elem_code(state) &&
                 (((decode_psa_asymmetric_decrypt_req(
                      state,
                      (&(*result)
                            .psa_crypto_req_msg_psa_asymmetric_decrypt_req_m)))) &&
                  (((*result).psa_crypto_req_msg_choice =
                        psa_crypto_req_msg_psa_asymmetric_decrypt_req_m_c),
                   true))) ||
                (zcbor_union_elem_code(state) &&
                 (((decode_psa_key_derivation_setup_req(
                      state,
                      (&(*result)
                            .psa_crypto_req_msg_psa_key_derivation_setup_req_m)))) &&
                  (((*result).psa_crypto_req_msg_choice =
                        psa_crypto_req_msg_psa_key_derivation_setup_req_m_c),
                   true))) ||
                (zcbor_union_elem_code(state) &&
                 (((decode_psa_key_derivation_get_capacity_req(
                      state,
                      (&(*result)
                            .psa_crypto_req_msg_psa_key_derivation_get_capacity_req_m)))) &&
                  (((*result).psa_crypto_req_msg_choice =
                        psa_crypto_req_msg_psa_key_derivation_get_capacity_req_m_c),
                   true))) ||
                (zcbor_union_elem_code(state) &&
                 (((decode_psa_key_derivation_set_capacity_req(
                      state,
                      (&(*result)
                            .psa_crypto_req_msg_psa_key_derivation_set_capacity_req_m)))) &&
                  (((*result).psa_crypto_req_msg_choice =
                        psa_crypto_req_msg_psa_key_derivation_set_capacity_req_m_c),
                   true))) ||
                (zcbor_union_elem_code(state) &&
                 (((decode_psa_key_derivation_input_bytes_req(
                      state,
                      (&(*result)
                            .psa_crypto_req_msg_psa_key_derivation_input_bytes_req_m)))) &&
                  (((*result).psa_crypto_req_msg_choice =
                        psa_crypto_req_msg_psa_key_derivation_input_bytes_req_m_c),
                   true))) ||
                (zcbor_union_elem_code(state) &&
                 (((decode_psa_key_derivation_input_integer_req(
                      state,
                      (&(*result)
                            .psa_crypto_req_msg_psa_key_derivation_input_integer_req_m)))) &&
                  (((*result).psa_crypto_req_msg_choice =
                        psa_crypto_req_msg_psa_key_derivation_input_integer_req_m_c),
                   true))) ||
                (zcbor_union_elem_code(state) &&
                 (((decode_psa_key_derivation_input_key_req(
                      state,
                      (&(*result)
                            .psa_crypto_req_msg_psa_key_derivation_input_key_req_m)))) &&
                  (((*result).psa_crypto_req_msg_choice =
                        psa_crypto_req_msg_psa_key_derivation_input_key_req_m_c),
                   true))) ||
                (zcbor_union_elem_code(state) &&
                 (((decode_psa_key_derivation_key_agreement_req(
                      state,
                      (&(*result)
                            .psa_crypto_req_msg_psa_key_derivation_key_agreement_req_m)))) &&
                  (((*result).psa_crypto_req_msg_choice =
                        psa_crypto_req_msg_psa_key_derivation_key_agreement_req_m_c),
                   true))) ||
                (zcbor_union_elem_code(state) &&
                 (((decode_psa_key_derivation_output_bytes_req(
                      state,
                      (&(*result)
                            .psa_crypto_req_msg_psa_key_derivation_output_bytes_req_m)))) &&
                  (((*result).psa_crypto_req_msg_choice =
                        psa_crypto_req_msg_psa_key_derivation_output_bytes_req_m_c),
                   true))) ||
                (zcbor_union_elem_code(state) &&
                 (((decode_psa_key_derivation_output_key_req(
                      state,
                      (&(*result)
                            .psa_crypto_req_msg_psa_key_derivation_output_key_req_m)))) &&
                  (((*result).psa_crypto_req_msg_choice =
                        psa_crypto_req_msg_psa_key_derivation_output_key_req_m_c),
                   true))) ||
                (zcbor_union_elem_code(state) &&
                 (((decode_psa_key_derivation_abort_req(
                      state,
                      (&(*result)
                            .psa_crypto_req_msg_psa_key_derivation_abort_req_m)))) &&
                  (((*result).psa_crypto_req_msg_choice =
                        psa_crypto_req_msg_psa_key_derivation_abort_req_m_c),
                   true))) ||
                (zcbor_union_elem_code(state) &&
                 (((decode_psa_raw_key_agreement_req(
                      state,
                      (&(*result)
                            .psa_crypto_req_msg_psa_raw_key_agreement_req_m)))) &&
                  (((*result).psa_crypto_req_msg_choice =
                        psa_crypto_req_msg_psa_raw_key_agreement_req_m_c),
                   true))) ||
                (zcbor_union_elem_code(state) &&
                 (((decode_psa_generate_random_req(
                      state,
                      (&(*result)
                            .psa_crypto_req_msg_psa_generate_random_req_m)))) &&
                  (((*result).psa_crypto_req_msg_choice =
                        psa_crypto_req_msg_psa_generate_random_req_m_c),
                   true))) ||
                (zcbor_union_elem_code(state) &&
                 (((decode_psa_generate_key_req(
                      state,
                      (&(*result)
                            .psa_crypto_req_msg_psa_generate_key_req_m)))) &&
                  (((*result).psa_crypto_req_msg_choice =
                        psa_crypto_req_msg_psa_generate_key_req_m_c),
                   true))) ||
                (zcbor_union_elem_code(state) &&
                 (((decode_psa_pake_setup_req(
                      state,
                      (&(*result).psa_crypto_req_msg_psa_pake_setup_req_m)))) &&
                  (((*result).psa_crypto_req_msg_choice =
                        psa_crypto_req_msg_psa_pake_setup_req_m_c),
                   true))) ||
                (zcbor_union_elem_code(state) &&
                 (((decode_psa_pake_set_role_req(
                      state,
                      (&(*result)
                            .psa_crypto_req_msg_psa_pake_set_role_req_m)))) &&
                  (((*result).psa_crypto_req_msg_choice =
                        psa_crypto_req_msg_psa_pake_set_role_req_m_c),
                   true))) ||
                (zcbor_union_elem_code(state) &&
                 (((decode_psa_pake_set_user_req(
                      state,
                      (&(*result)
                            .psa_crypto_req_msg_psa_pake_set_user_req_m)))) &&
                  (((*result).psa_crypto_req_msg_choice =
                        psa_crypto_req_msg_psa_pake_set_user_req_m_c),
                   true))) ||
                (zcbor_union_elem_code(state) &&
                 (((decode_psa_pake_set_peer_req(
                      state,
                      (&(*result)
                            .psa_crypto_req_msg_psa_pake_set_peer_req_m)))) &&
                  (((*result).psa_crypto_req_msg_choice =
                        psa_crypto_req_msg_psa_pake_set_peer_req_m_c),
                   true))) ||
                (zcbor_union_elem_code(state) &&
                 (((decode_psa_pake_set_context_req(
                      state,
                      (&(*result)
                            .psa_crypto_req_msg_psa_pake_set_context_req_m)))) &&
                  (((*result).psa_crypto_req_msg_choice =
                        psa_crypto_req_msg_psa_pake_set_context_req_m_c),
                   true))) ||
                (zcbor_union_elem_code(state) &&
                 (((decode_psa_pake_output_req(
                      state,
                      (&(*result)
                            .psa_crypto_req_msg_psa_pake_output_req_m)))) &&
                  (((*result).psa_crypto_req_msg_choice =
                        psa_crypto_req_msg_psa_pake_output_req_m_c),
                   true))) ||
                (zcbor_union_elem_code(state) &&
                 (((decode_psa_pake_input_req(
                      state,
                      (&(*result).psa_crypto_req_msg_psa_pake_input_req_m)))) &&
                  (((*result).psa_crypto_req_msg_choice =
                        psa_crypto_req_msg_psa_pake_input_req_m_c),
                   true))) ||
                (zcbor_union_elem_code(state) &&
                 (((decode_psa_pake_get_shared_key_req(
                      state,
                      (&(*result)
                            .psa_crypto_req_msg_psa_pake_get_shared_key_req_m)))) &&
                  (((*result).psa_crypto_req_msg_choice =
                        psa_crypto_req_msg_psa_pake_get_shared_key_req_m_c),
                   true))) ||
                (zcbor_union_elem_code(state) &&
                 (((decode_psa_pake_abort_req(
                      state,
                      (&(*result).psa_crypto_req_msg_psa_pake_abort_req_m)))) &&
                  (((*result).psa_crypto_req_msg_choice =
                        psa_crypto_req_msg_psa_pake_abort_req_m_c),
                   true)))),
           zcbor_union_end_code(state), int_res)))) ||
       (zcbor_list_map_end_force_decode(state), false)) &&
      zcbor_list_end_decode(state))));

  log_result(state, res, __func__);
  return res;
}

int cbor_decode_psa_crypto_req(const uint8_t *payload, size_t payload_len,
                               struct psa_crypto_req *result,
                               size_t *payload_len_out) {
  zcbor_state_t states[4];

  return zcbor_entry_function(payload, payload_len, (void *)result,
                              payload_len_out, states,
                              (zcbor_decoder_t *)decode_psa_crypto_req,
                              sizeof(states) / sizeof(zcbor_state_t), 1);
}

int cbor_decode_psa_crypto_rsp(const uint8_t *payload, size_t payload_len,
                               struct psa_crypto_rsp *result,
                               size_t *payload_len_out) {
  zcbor_state_t states[3];

  return zcbor_entry_function(payload, payload_len, (void *)result,
                              payload_len_out, states,
                              (zcbor_decoder_t *)decode_psa_crypto_rsp,
                              sizeof(states) / sizeof(zcbor_state_t), 1);
}
