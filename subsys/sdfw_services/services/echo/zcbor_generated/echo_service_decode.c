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
#include "zcbor_decode.h"
#include "echo_service_decode.h"
#include "zcbor_print.h"

#if DEFAULT_MAX_QTY != 3
#error "The type file was generated with a different default_max_qty than this file"
#endif

static bool decode_echo_service_rsp(zcbor_state_t *state, struct echo_service_rsp *result);
static bool decode_echo_service_req(zcbor_state_t *state, struct zcbor_string *result);

static bool decode_echo_service_rsp(zcbor_state_t *state, struct echo_service_rsp *result)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result =
		(((zcbor_list_start_decode(state) &&
		   ((((zcbor_int32_decode(state, (&(*result).echo_service_rsp_ret)))) &&
		     ((zcbor_tstr_decode(state, (&(*result).echo_service_rsp_str_out))))) ||
		    (zcbor_list_map_end_force_decode(state), false)) &&
		   zcbor_list_end_decode(state))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool decode_echo_service_req(zcbor_state_t *state, struct zcbor_string *result)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result = (((zcbor_list_start_decode(state) &&
			     ((((zcbor_tstr_decode(state, (&(*result)))))) ||
			      (zcbor_list_map_end_force_decode(state), false)) &&
			     zcbor_list_end_decode(state))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

int cbor_decode_echo_service_req(const uint8_t *payload, size_t payload_len,
				 struct zcbor_string *result, size_t *payload_len_out)
{
	zcbor_state_t states[3];

	return zcbor_entry_function(payload, payload_len, (void *)result, payload_len_out, states,
				    (zcbor_decoder_t *)decode_echo_service_req,
				    sizeof(states) / sizeof(zcbor_state_t), 1);
}

int cbor_decode_echo_service_rsp(const uint8_t *payload, size_t payload_len,
				 struct echo_service_rsp *result, size_t *payload_len_out)
{
	zcbor_state_t states[3];

	return zcbor_entry_function(payload, payload_len, (void *)result, payload_len_out, states,
				    (zcbor_decoder_t *)decode_echo_service_rsp,
				    sizeof(states) / sizeof(zcbor_state_t), 1);
}
