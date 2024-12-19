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

#include "echo_service_encode.h"
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

static bool encode_echo_service_rsp(zcbor_state_t *state,
                                    const struct echo_service_rsp *input);
static bool encode_echo_service_req(zcbor_state_t *state,
                                    const struct zcbor_string *input);

static bool encode_echo_service_rsp(zcbor_state_t *state,
                                    const struct echo_service_rsp *input) {
  zcbor_log("%s\r\n", __func__);

  bool res = ((
      (zcbor_list_start_encode(state, 2) &&
       ((((zcbor_int32_encode(state, (&(*input).echo_service_rsp_ret)))) &&
         ((zcbor_tstr_encode(state, (&(*input).echo_service_rsp_str_out))))) ||
        (zcbor_list_map_end_force_encode(state), false)) &&
       zcbor_list_end_encode(state, 2))));

  log_result(state, res, __func__);
  return res;
}

static bool encode_echo_service_req(zcbor_state_t *state,
                                    const struct zcbor_string *input) {
  zcbor_log("%s\r\n", __func__);

  bool res = (((zcbor_list_start_encode(state, 1) &&
                ((((zcbor_tstr_encode(state, (&(*input)))))) ||
                 (zcbor_list_map_end_force_encode(state), false)) &&
                zcbor_list_end_encode(state, 1))));

  log_result(state, res, __func__);
  return res;
}

int cbor_encode_echo_service_req(uint8_t *payload, size_t payload_len,
                                 const struct zcbor_string *input,
                                 size_t *payload_len_out) {
  zcbor_state_t states[3];

  return zcbor_entry_function(payload, payload_len, (void *)input,
                              payload_len_out, states,
                              (zcbor_decoder_t *)encode_echo_service_req,
                              sizeof(states) / sizeof(zcbor_state_t), 1);
}

int cbor_encode_echo_service_rsp(uint8_t *payload, size_t payload_len,
                                 const struct echo_service_rsp *input,
                                 size_t *payload_len_out) {
  zcbor_state_t states[3];

  return zcbor_entry_function(payload, payload_len, (void *)input,
                              payload_len_out, states,
                              (zcbor_decoder_t *)encode_echo_service_rsp,
                              sizeof(states) / sizeof(zcbor_state_t), 1);
}
