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
#include "zcbor_encode.h"
#include "msg_encode.h"
#include "zcbor_print.h"

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
	zcbor_log("%s\r\n", __func__);

	bool tmp_result = ((((zcbor_uint32_put(state, (7)))) &&
			    (zcbor_float64_encode(state, (&(*input).pvt_spd)))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool encode_repeated_pvt_hdg(zcbor_state_t *state, const struct pvt_hdg *input)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result = ((((zcbor_uint32_put(state, (8)))) &&
			    (zcbor_float64_encode(state, (&(*input).pvt_hdg)))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool encode_repeated_pvt_alt(zcbor_state_t *state, const struct pvt_alt *input)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result = ((((zcbor_uint32_put(state, (9)))) &&
			    (zcbor_float64_encode(state, (&(*input).pvt_alt)))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool encode_pvt(zcbor_state_t *state, const struct pvt *input)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result = (((zcbor_map_start_encode(state, 6) &&
			     (((((zcbor_uint32_put(state, (4)))) &&
				(zcbor_float64_encode(state, (&(*input).pvt_lat)))) &&
			       (((zcbor_uint32_put(state, (5)))) &&
				(zcbor_float64_encode(state, (&(*input).pvt_lng)))) &&
			       (((zcbor_uint32_put(state, (6)))) &&
				(zcbor_float64_encode(state, (&(*input).pvt_acc)))) &&
			       (!(*input).pvt_spd_present ||
				encode_repeated_pvt_spd(state, (&(*input).pvt_spd))) &&
			       (!(*input).pvt_hdg_present ||
				encode_repeated_pvt_hdg(state, (&(*input).pvt_hdg))) &&
			       (!(*input).pvt_alt_present ||
				encode_repeated_pvt_alt(state, (&(*input).pvt_alt)))) ||
			      (zcbor_list_map_end_force_encode(state), false)) &&
			     zcbor_map_end_encode(state, 6))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool encode_repeated_message_out_ts(zcbor_state_t *state, const struct message_out_ts *input)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result = ((((zcbor_uint32_put(state, (3)))) &&
			    ((((*input).message_out_ts <= UINT64_MAX)) ||
			     (zcbor_error(state, ZCBOR_ERR_WRONG_RANGE), false)) &&
			    (zcbor_uint64_encode(state, (&(*input).message_out_ts)))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool encode_message_out(zcbor_state_t *state, const struct message_out *input)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result = (((
		zcbor_map_start_encode(state, 3) &&
		(((((zcbor_uint32_put(state, (1)))) &&
		   (zcbor_tstr_encode(state, (&(*input).message_out_appId)))) &&
		  (((zcbor_uint32_put(state, (2)))) &&
		   (((*input).message_out_data_choice == message_out_data_tstr_c)
		    ? ((zcbor_tstr_encode(state, (&(*input).message_out_data_tstr))))
		    : (((*input).message_out_data_choice == message_out_data_float_c)
		       ? ((zcbor_float64_encode(
				 state, (&(*input).message_out_data_float))))
		       : (((*input).message_out_data_choice ==
			   message_out_data_int_c)
			  ? ((zcbor_int32_encode(
				    state,
				    (&(*input).message_out_data_int))))
			  : (((*input).message_out_data_choice ==
			      message_out_data_pvt_m_c)
				     ? ((encode_pvt(
					       state,
					       (&(*input).message_out_data_pvt_m))))
				     : false))))) &&
		  (!(*input).message_out_ts_present ||
		   encode_repeated_message_out_ts(state, (&(*input).message_out_ts)))) ||
		 (zcbor_list_map_end_force_encode(state), false)) &&
		zcbor_map_end_encode(state, 3))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

int cbor_encode_message_out(uint8_t *payload, size_t payload_len, const struct message_out *input,
			    size_t *payload_len_out)
{
	zcbor_state_t states[5];

	return zcbor_entry_function(payload, payload_len, (void *)input, payload_len_out, states,
				    (zcbor_decoder_t *)encode_message_out,
				    sizeof(states) / sizeof(zcbor_state_t), 1);
}
