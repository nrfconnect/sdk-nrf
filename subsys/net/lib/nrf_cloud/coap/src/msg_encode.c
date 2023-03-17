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
#include "msg_encode.h"

#if DEFAULT_MAX_QTY != 10
#error "The type file was generated with a different default_max_qty than this file"
#endif

static bool encode_repeated_pvt_spd(zcbor_state_t *state, const struct pvt_spd *input);
static bool encode_repeated_pvt_hdg(zcbor_state_t *state, const struct pvt_hdg *input);
static bool encode_repeated_pvt_alt(zcbor_state_t *state, const struct pvt_alt *input);
static bool encode_pvt(zcbor_state_t *state, const struct pvt *input);
static bool encode_repeated_message_out_ts(zcbor_state_t *state,
					   const struct message_out_ts *input);
static bool encode_message_out(zcbor_state_t *state, const struct message_out *input);

static bool encode_repeated_pvt_spd(zcbor_state_t *state, const struct pvt_spd *input)
{
	zcbor_print("%s\r\n", __func__);

	bool tmp_result = ((((zcbor_uint32_put(state, (7)))) &&
			    (zcbor_float64_encode(state, (&(*input)._pvt_spd)))));

	if (!tmp_result) {
		zcbor_trace();
	}

	return tmp_result;
}

static bool encode_repeated_pvt_hdg(zcbor_state_t *state, const struct pvt_hdg *input)
{
	zcbor_print("%s\r\n", __func__);

	bool tmp_result = ((((zcbor_uint32_put(state, (8)))) &&
			    (zcbor_float64_encode(state, (&(*input)._pvt_hdg)))));

	if (!tmp_result) {
		zcbor_trace();
	}

	return tmp_result;
}

static bool encode_repeated_pvt_alt(zcbor_state_t *state, const struct pvt_alt *input)
{
	zcbor_print("%s\r\n", __func__);

	bool tmp_result = ((((zcbor_uint32_put(state, (9)))) &&
			    (zcbor_float64_encode(state, (&(*input)._pvt_alt)))));

	if (!tmp_result) {
		zcbor_trace();
	}

	return tmp_result;
}

static bool encode_pvt(zcbor_state_t *state, const struct pvt *input)
{
	zcbor_print("%s\r\n", __func__);

	bool tmp_result = (((zcbor_map_start_encode(state, 6) &&
			     (((((zcbor_uint32_put(state, (4)))) &&
				(zcbor_float64_encode(state, (&(*input)._pvt_lat)))) &&
			       (((zcbor_uint32_put(state, (5)))) &&
				(zcbor_float64_encode(state, (&(*input)._pvt_lng)))) &&
			       (((zcbor_uint32_put(state, (6)))) &&
				(zcbor_float64_encode(state, (&(*input)._pvt_acc)))) &&
			       zcbor_present_encode(&((*input)._pvt_spd_present),
						    (zcbor_encoder_t *)encode_repeated_pvt_spd,
						    state, (&(*input)._pvt_spd)) &&
			       zcbor_present_encode(&((*input)._pvt_hdg_present),
						    (zcbor_encoder_t *)encode_repeated_pvt_hdg,
						    state, (&(*input)._pvt_hdg)) &&
			       zcbor_present_encode(&((*input)._pvt_alt_present),
						    (zcbor_encoder_t *)encode_repeated_pvt_alt,
						    state, (&(*input)._pvt_alt))) ||
			      (zcbor_list_map_end_force_encode(state), false)) &&
			     zcbor_map_end_encode(state, 6))));

	if (!tmp_result) {
		zcbor_trace();
	}

	return tmp_result;
}

static bool encode_repeated_message_out_ts(zcbor_state_t *state, const struct message_out_ts *input)
{
	zcbor_print("%s\r\n", __func__);

	bool tmp_result = ((((zcbor_uint32_put(state, (3)))) &&
			    ((((*input)._message_out_ts <= UINT64_MAX)) ||
			     (zcbor_error(state, ZCBOR_ERR_WRONG_RANGE), false)) &&
			    (zcbor_uint64_encode(state, (&(*input)._message_out_ts)))));

	if (!tmp_result) {
		zcbor_trace();
	}

	return tmp_result;
}

static bool encode_message_out(zcbor_state_t *state, const struct message_out *input)
{
	zcbor_print("%s\r\n", __func__);

	bool tmp_result = (((
		zcbor_map_start_encode(state, 3) &&
		(((((zcbor_uint32_put(state, (1)))) &&
		   (zcbor_tstr_encode(state, (&(*input)._message_out_appId)))) &&
		  (((zcbor_uint32_put(state, (2)))) &&
		   (((*input)._message_out_data_choice == _message_out_data_tstr)
			    ? ((zcbor_tstr_encode(state, (&(*input)._message_out_data_tstr))))
			    : (((*input)._message_out_data_choice == _message_out_data_float)
				       ? ((zcbor_float64_encode(
						 state, (&(*input)._message_out_data_float))))
				       : (((*input)._message_out_data_choice ==
					   _message_out_data_int)
						  ? ((zcbor_int32_encode(
							    state,
							    (&(*input)._message_out_data_int))))
						  : (((*input)._message_out_data_choice ==
						      _message_out_data__pvt)
							     ? ((encode_pvt(
								       state,
							       (&(*input)._message_out_data__pvt))))
							     : false))))) &&
		  zcbor_present_encode(&((*input)._message_out_ts_present),
				       (zcbor_encoder_t *)encode_repeated_message_out_ts, state,
				       (&(*input)._message_out_ts))) ||
		 (zcbor_list_map_end_force_encode(state), false)) &&
		zcbor_map_end_encode(state, 3))));

	if (!tmp_result) {
		zcbor_trace();
	}

	return tmp_result;
}

int cbor_encode_message_out(uint8_t *payload, size_t payload_len, const struct message_out *input,
			    size_t *payload_len_out)
{
	zcbor_state_t states[5];

	zcbor_new_state(states, sizeof(states) / sizeof(zcbor_state_t), payload, payload_len, 1);

	bool ret = encode_message_out(states, input);

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
