/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
/*
 * Generated using zcbor version 0.6.0
 * https://github.com/NordicSemiconductor/zcbor
 * Generated with a --default-max-qty of 1234567890
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "zcbor_encode.h"
#include "nrf_provisioning_cbor_encode.h"

#if DEFAULT_MAX_QTY != CONFIG_NRF_PROVISIONING_CBOR_RECORDS
#error "The type file was generated with a different default_max_qty than this file"
#endif

static bool encode_error_response(zcbor_state_t *state, const struct error_response *input);
static bool encode_at_response(zcbor_state_t *state, const struct at_response *input);
static bool encode_response(zcbor_state_t *state, const struct response *input);
static bool encode_responses(zcbor_state_t *state, const struct responses *input);

static bool encode_error_response(zcbor_state_t *state, const struct error_response *input)
{
	zcbor_print("%s\r\n", __func__);

	bool tmp_result =
		(((((zcbor_uint32_put(state, (99)))) &&
		   ((zcbor_uint32_encode(state, (&(*input)._error_response_cme_error)))) &&
		   ((zcbor_tstr_encode(state, (&(*input)._error_response_message)))))));

	if (!tmp_result)
		zcbor_trace();

	return tmp_result;
}

static bool encode_at_response(zcbor_state_t *state, const struct at_response *input)
{
	zcbor_print("%s\r\n", __func__);

	bool tmp_result = (((((zcbor_uint32_put(state, (100)))) &&
			     ((zcbor_tstr_encode(state, (&(*input)._at_response_message)))))));

	if (!tmp_result)
		zcbor_trace();

	return tmp_result;
}

static bool encode_response(zcbor_state_t *state, const struct response *input)
{
	zcbor_print("%s\r\n", __func__);

	bool tmp_result =
		(((zcbor_list_start_encode(state, 4) &&
		   ((((zcbor_tstr_encode(state, (&(*input)._response__correlation)))) &&
		     ((((*input)._response_union_choice == _response_union__error_response) ?
			       ((encode_error_response(
				       state, (&(*input)._response_union__error_response)))) :
			       (((*input)._response_union_choice == _response_union__at_response) ?
					((encode_at_response(
						state, (&(*input)._response_union__at_response)))) :
					(((*input)._response_union_choice ==
					  _response_union__config_ack) ?
						 ((zcbor_uint32_put(state, (101)))) :
						 (((*input)._response_union_choice ==
						   _response_union__finished_ack) ?
							  ((zcbor_uint32_put(state, (102)))) :
							  false)))))) ||
		    (zcbor_list_map_end_force_encode(state), false)) &&
		   zcbor_list_end_encode(state, 4))));

	if (!tmp_result)
		zcbor_trace();

	return tmp_result;
}

static bool encode_responses(zcbor_state_t *state, const struct responses *input)
{
	zcbor_print("%s\r\n", __func__);

	bool tmp_result =
		(((zcbor_list_start_encode(state, DEFAULT_MAX_QTY) &&
			     ((zcbor_multi_encode_minmax(
				      1, DEFAULT_MAX_QTY, &(*input)._responses__response_count,
				      (zcbor_encoder_t *)encode_response, state,
				      (&(*input)._responses__response), sizeof(struct response))) ||
			      (zcbor_list_map_end_force_encode(state), false)) &&
			     zcbor_list_end_encode(state, DEFAULT_MAX_QTY))));

	if (!tmp_result)
		zcbor_trace();

	return tmp_result;
}

int cbor_encode_responses(uint8_t *payload, size_t payload_len, const struct responses *input,
			  size_t *payload_len_out)
{
	zcbor_state_t states[5];

	zcbor_new_state(states, sizeof(states) / sizeof(zcbor_state_t), payload, payload_len, 1);

	bool ret = encode_responses(states, input);

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
