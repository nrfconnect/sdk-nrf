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
#include "zcbor_encode.h"
#include "agnss_encode.h"

#if DEFAULT_MAX_QTY != 10
#error "The type file was generated with a different default_max_qty than this file"
#endif

static bool encode_repeated_agnss_req_types(zcbor_state_t *state,
					    const struct agnss_req_types_ *input);
static bool encode_repeated_agnss_req_filtered(zcbor_state_t *state,
					       const struct agnss_req_filtered *input);
static bool encode_repeated_agnss_req_mask(zcbor_state_t *state,
					   const struct agnss_req_mask *input);
static bool encode_repeated_agnss_req_rsrp(zcbor_state_t *state,
					   const struct agnss_req_rsrp *input);
static bool encode_agnss_req(zcbor_state_t *state, const struct agnss_req *input);

static bool encode_repeated_agnss_req_types(zcbor_state_t *state,
					    const struct agnss_req_types_ *input)
{
	zcbor_print("%s\r\n", __func__);

	bool tmp_result = ((
		((zcbor_uint32_put(state, (1)))) &&
		(zcbor_list_start_encode(state, 13) &&
		 ((zcbor_multi_encode_minmax(1, 13, &(*input)._agnss_req_types_int_count,
					     (zcbor_encoder_t *)zcbor_int32_encode, state,
					     (&(*input)._agnss_req_types_int), sizeof(int32_t))) ||
		  (zcbor_list_map_end_force_encode(state), false)) &&
		 zcbor_list_end_encode(state, 13))));

	if (!tmp_result) {
		zcbor_trace();
	}

	return tmp_result;
}

static bool encode_repeated_agnss_req_filtered(zcbor_state_t *state,
					       const struct agnss_req_filtered *input)
{
	zcbor_print("%s\r\n", __func__);

	bool tmp_result = ((((zcbor_uint32_put(state, (3)))) &&
			    (zcbor_bool_encode(state, (&(*input)._agnss_req_filtered)))));

	if (!tmp_result) {
		zcbor_trace();
	}

	return tmp_result;
}

static bool encode_repeated_agnss_req_mask(zcbor_state_t *state, const struct agnss_req_mask *input)
{
	zcbor_print("%s\r\n", __func__);

	bool tmp_result = ((((zcbor_uint32_put(state, (4)))) &&
			    (zcbor_uint32_encode(state, (&(*input)._agnss_req_mask)))));

	if (!tmp_result) {
		zcbor_trace();
	}

	return tmp_result;
}

static bool encode_repeated_agnss_req_rsrp(zcbor_state_t *state, const struct agnss_req_rsrp *input)
{
	zcbor_print("%s\r\n", __func__);

	bool tmp_result = ((((zcbor_uint32_put(state, (7)))) &&
			    (zcbor_int32_encode(state, (&(*input)._agnss_req_rsrp)))));

	if (!tmp_result) {
		zcbor_trace();
	}

	return tmp_result;
}

static bool encode_agnss_req(zcbor_state_t *state, const struct agnss_req *input)
{
	zcbor_print("%s\r\n", __func__);

	bool tmp_result =
		(((zcbor_map_start_encode(state, 8) &&
		   ((zcbor_present_encode(&((*input)._agnss_req_types_present),
					  (zcbor_encoder_t *)encode_repeated_agnss_req_types, state,
					  (&(*input)._agnss_req_types)) &&
		     (((zcbor_uint32_put(state, (2)))) &&
		      (zcbor_uint32_encode(state, (&(*input)._agnss_req_eci)))) &&
		     zcbor_present_encode(&((*input)._agnss_req_filtered_present),
					  (zcbor_encoder_t *)encode_repeated_agnss_req_filtered,
					  state, (&(*input)._agnss_req_filtered)) &&
		     zcbor_present_encode(&((*input)._agnss_req_mask_present),
					  (zcbor_encoder_t *)encode_repeated_agnss_req_mask, state,
					  (&(*input)._agnss_req_mask)) &&
		     (((zcbor_uint32_put(state, (5)))) &&
		      (zcbor_uint32_encode(state, (&(*input)._agnss_req_mcc)))) &&
		     (((zcbor_uint32_put(state, (6)))) &&
		      (zcbor_uint32_encode(state, (&(*input)._agnss_req_mnc)))) &&
		     zcbor_present_encode(&((*input)._agnss_req_rsrp_present),
					  (zcbor_encoder_t *)encode_repeated_agnss_req_rsrp, state,
					  (&(*input)._agnss_req_rsrp)) &&
		     (((zcbor_uint32_put(state, (8)))) &&
		      (zcbor_uint32_encode(state, (&(*input)._agnss_req_tac))))) ||
		    (zcbor_list_map_end_force_encode(state), false)) &&
		   zcbor_map_end_encode(state, 8))));

	if (!tmp_result) {
		zcbor_trace();
	}

	return tmp_result;
}

int cbor_encode_agnss_req(uint8_t *payload, size_t payload_len, const struct agnss_req *input,
			  size_t *payload_len_out)
{
	zcbor_state_t states[4];

	zcbor_new_state(states, sizeof(states) / sizeof(zcbor_state_t), payload, payload_len, 1);

	bool ret = encode_agnss_req(states, input);

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
