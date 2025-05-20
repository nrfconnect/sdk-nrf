/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * Generated using zcbor version 0.8.1
 * https://github.com/NordicSemiconductor/zcbor
 * Generated with a --default-max-qty of 1234567890
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "zcbor_encode.h"
#include "nrf_provisioning_cbor_encode.h"
#include "zcbor_print.h"

#if DEFAULT_MAX_QTY != CONFIG_NRF_PROVISIONING_CBOR_RECORDS
#error "The type file was generated with a different default_max_qty than this file"
#endif

static bool encode_error_response(zcbor_state_t *state, const struct error_response *input);
static bool encode_at_response(zcbor_state_t *state, const struct at_response *input);
static bool encode_response(zcbor_state_t *state, const struct response *input);
static bool encode_responses(zcbor_state_t *state, const struct responses *input);

static bool encode_error_response(zcbor_state_t *state, const struct error_response *input)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result = (((((zcbor_uint32_put(state, (99)))) &&
			     ((zcbor_uint32_encode(state, (&(*input).error_response_cme_error)))) &&
			     ((zcbor_tstr_encode(state, (&(*input).error_response_message)))))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool encode_at_response(zcbor_state_t *state, const struct at_response *input)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result = (((((zcbor_uint32_put(state, (100)))) &&
			     ((zcbor_tstr_encode(state, (&(*input).at_response_message)))))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool encode_response(zcbor_state_t *state, const struct response *input)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result = ((
		(zcbor_list_start_encode(state, 4) &&
		 ((((zcbor_tstr_encode(state, (&(*input).response_correlation_m)))) &&
		   ((((*input).response_union_choice == response_union_error_response_m_c)
			     ? ((encode_error_response(
				       state, (&(*input).response_union_error_response_m))))
			     : (((*input).response_union_choice == response_union_at_response_m_c)
					? ((encode_at_response(
						  state, (&(*input).response_union_at_response_m))))
					: (((*input).response_union_choice ==
					    response_union_config_ack_m_c)
						   ? ((zcbor_uint32_put(state, (101))))
						   : (((*input).response_union_choice ==
						       response_union_finished_ack_m_c)
							      ? ((zcbor_uint32_put(state, (102))))
							      : false)))))) ||
		  (zcbor_list_map_end_force_encode(state), false)) &&
		 zcbor_list_end_encode(state, 4))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool encode_responses(zcbor_state_t *state, const struct responses *input)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result = (((zcbor_list_start_encode(state, DEFAULT_MAX_QTY) &&
			     ((zcbor_multi_encode_minmax(
				      1, DEFAULT_MAX_QTY, &(*input).responses_response_m_count,
				      (zcbor_encoder_t *)encode_response, state,
				      (&(*input).responses_response_m), sizeof(struct response))) ||
			      (zcbor_list_map_end_force_encode(state), false)) &&
			     zcbor_list_end_encode(state, DEFAULT_MAX_QTY))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

int cbor_encode_responses(uint8_t *payload, size_t payload_len, const struct responses *input,
			  size_t *payload_len_out)
{
	zcbor_state_t states[5];

	return zcbor_entry_function(payload, payload_len, (void *)input, payload_len_out, states,
				    (zcbor_decoder_t *)encode_responses,
				    sizeof(states) / sizeof(zcbor_state_t), 1);
}
