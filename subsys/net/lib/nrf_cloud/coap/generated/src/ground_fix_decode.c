/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * Generated using zcbor version 0.8.1
 * https://github.com/NordicSemiconductor/zcbor
 * Generated with a --default-max-qty of 10
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "zcbor_decode.h"
#include "ground_fix_decode.h"
#include "zcbor_print.h"

#if DEFAULT_MAX_QTY != 10
#error "The type file was generated with a different default_max_qty than this file"
#endif

static bool decode_ground_fix_resp(zcbor_state_t *state, struct ground_fix_resp *result);

static bool decode_ground_fix_resp(zcbor_state_t *state, struct ground_fix_resp *result)
{
	zcbor_log("%s\r\n", __func__);
	bool int_res;

	bool tmp_result = (((
		zcbor_map_start_decode(state) &&
		(((((zcbor_uint32_expect(state, (1)))) &&
		   (zcbor_float64_decode(state, (&(*result).ground_fix_resp_lat)))) &&
		  (((zcbor_uint32_expect(state, (2)))) &&
		   (zcbor_float64_decode(state, (&(*result).ground_fix_resp_lon)))) &&
		  (((zcbor_uint32_expect(state, (3)))) &&
		   (zcbor_union_start_code(state) &&
		    (int_res = ((((zcbor_int32_decode(
					 state, (&(*result).ground_fix_resp_uncertainty_int)))) &&
				 (((*result).ground_fix_resp_uncertainty_choice =
					   ground_fix_resp_uncertainty_int_c),
				  true)) ||
				(((zcbor_float_decode(
					 state, (&(*result).ground_fix_resp_uncertainty_float)))) &&
				 (((*result).ground_fix_resp_uncertainty_choice =
					   ground_fix_resp_uncertainty_float_c),
				  true))),
		     zcbor_union_end_code(state), int_res))) &&
		  (((zcbor_uint32_expect(state, (4)))) &&
		   (zcbor_tstr_decode(state, (&(*result).ground_fix_resp_fulfilledWith))))) ||
		 (zcbor_list_map_end_force_decode(state), false)) &&
		zcbor_map_end_decode(state))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

int cbor_decode_ground_fix_resp(const uint8_t *payload, size_t payload_len,
				struct ground_fix_resp *result, size_t *payload_len_out)
{
	zcbor_state_t states[4];

	return zcbor_entry_function(payload, payload_len, (void *)result, payload_len_out, states,
				    (zcbor_decoder_t *)decode_ground_fix_resp,
				    sizeof(states) / sizeof(zcbor_state_t), 1);
}
