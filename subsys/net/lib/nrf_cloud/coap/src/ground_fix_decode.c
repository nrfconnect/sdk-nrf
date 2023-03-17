/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * Generated using zcbor version 0.7.0
 * https://github.com/NordicSemiconductor/zcbor
 * Generated with a --default-max-qty of 10
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "zcbor_decode.h"
#include "ground_fix_decode.h"

#if DEFAULT_MAX_QTY != 10
#error "The type file was generated with a different default_max_qty than this file"
#endif

static bool decode_ground_fix_resp(zcbor_state_t *state, struct ground_fix_resp *result);

static bool decode_ground_fix_resp(zcbor_state_t *state, struct ground_fix_resp *result)
{
	zcbor_print("%s\r\n", __func__);
	bool int_res;

	bool tmp_result = ((
		(zcbor_map_start_decode(state) &&
		 (((((zcbor_uint32_expect(state, (1)))) &&
		    (zcbor_float64_decode(state, (&(*result)._ground_fix_resp_lat)))) &&
		   (((zcbor_uint32_expect(state, (2)))) &&
		    (zcbor_float64_decode(state, (&(*result)._ground_fix_resp_lon)))) &&
		   (((zcbor_uint32_expect(state, (3)))) &&
		    (zcbor_union_start_code(state) &&
		     (int_res =
			      ((((zcbor_int32_decode(
					state, (&(*result)._ground_fix_resp_uncertainty_int)))) &&
				(((*result)._ground_fix_resp_uncertainty_choice =
					  _ground_fix_resp_uncertainty_int),
				 true)) ||
			       (((zcbor_float_decode(
					state, (&(*result)._ground_fix_resp_uncertainty_float)))) &&
				(((*result)._ground_fix_resp_uncertainty_choice =
					  _ground_fix_resp_uncertainty_float),
				 true))),
		      zcbor_union_end_code(state), int_res))) &&
		   (((zcbor_uint32_expect(state, (4)))) &&
		    (zcbor_tstr_decode(state, (&(*result)._ground_fix_resp_fulfilledWith))))) ||
		  (zcbor_list_map_end_force_decode(state), false)) &&
		 zcbor_map_end_decode(state))));

	if (!tmp_result) {
		zcbor_trace();
	}

	return tmp_result;
}

int cbor_decode_ground_fix_resp(const uint8_t *payload, size_t payload_len,
				struct ground_fix_resp *result, size_t *payload_len_out)
{
	zcbor_state_t states[4];

	zcbor_new_state(states, sizeof(states) / sizeof(zcbor_state_t), payload, payload_len, 1);

	bool ret = decode_ground_fix_resp(states, result);

	if (ret && (payload_len_out != NULL)) {
		*payload_len_out = MIN(payload_len, (size_t)states[0].payload - (size_t)payload);
	}

	if (!ret) {
		int err = zcbor_pop_error(states);

		zcbor_print("Return error: %d\r\n", err);
		return (err == ZCBOR_SUCCESS) ? ZCBOR_ERR_UNKNOWN : err;
	}
	return ZCBOR_SUCCESS;
}
