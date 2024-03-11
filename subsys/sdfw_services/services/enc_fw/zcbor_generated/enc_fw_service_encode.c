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
#include "zcbor_encode.h"
#include "enc_fw_service_encode.h"
#include "zcbor_print.h"

#if DEFAULT_MAX_QTY != 3
#error "The type file was generated with a different default_max_qty than this file"
#endif

static bool encode_init(zcbor_state_t *state, const struct init *input);
static bool encode_chunk(zcbor_state_t *state, const struct chunk *input);
static bool encode_enc_fw_rsp(zcbor_state_t *state, const int32_t *input);
static bool encode_enc_fw_req(zcbor_state_t *state, const struct enc_fw_req *input);

static bool encode_init(zcbor_state_t *state, const struct init *input)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result =
		(((((zcbor_uint32_put(state, (0)))) &&
		   (((((*input).init_aad.len >= 8) && ((*input).init_aad.len <= 8)) ||
		     (zcbor_error(state, ZCBOR_ERR_WRONG_RANGE), false)) &&
		    (zcbor_bstr_encode(state, (&(*input).init_aad)))) &&
		   (((((*input).init_nonce.len >= 12) && ((*input).init_nonce.len <= 12)) ||
		     (zcbor_error(state, ZCBOR_ERR_WRONG_RANGE), false)) &&
		    (zcbor_bstr_encode(state, (&(*input).init_nonce)))) &&
		   (((((*input).init_tag.len >= 16) && ((*input).init_tag.len <= 16)) ||
		     (zcbor_error(state, ZCBOR_ERR_WRONG_RANGE), false)) &&
		    (zcbor_bstr_encode(state, (&(*input).init_tag)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).init_buffer_addr)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).init_buffer_len)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).init_image_addr)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).init_image_len)))))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool encode_chunk(zcbor_state_t *state, const struct chunk *input)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result = (((((zcbor_uint32_put(state, (1)))) &&
			     ((zcbor_uint32_encode(state, (&(*input).chunk_length)))) &&
			     ((zcbor_bool_encode(state, (&(*input).chunk_last)))))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool encode_enc_fw_rsp(zcbor_state_t *state, const int32_t *input)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result = (((zcbor_list_start_encode(state, 1) &&
			     ((((zcbor_int32_encode(state, (&(*input)))))) ||
			      (zcbor_list_map_end_force_encode(state), false)) &&
			     zcbor_list_end_encode(state, 1))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool encode_enc_fw_req(zcbor_state_t *state, const struct enc_fw_req *input)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result = (((
		zcbor_list_start_encode(state, 8) &&
		((((((*input).enc_fw_req_msg_choice == enc_fw_req_msg_init_m_c)
			    ? ((encode_init(state, (&(*input).enc_fw_req_msg_init_m))))
			    : (((*input).enc_fw_req_msg_choice == enc_fw_req_msg_chunk_m_c)
				       ? ((encode_chunk(state, (&(*input).enc_fw_req_msg_chunk_m))))
				       : false)))) ||
		 (zcbor_list_map_end_force_encode(state), false)) &&
		zcbor_list_end_encode(state, 8))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

int cbor_encode_enc_fw_req(uint8_t *payload, size_t payload_len, const struct enc_fw_req *input,
			   size_t *payload_len_out)
{
	zcbor_state_t states[4];

	return zcbor_entry_function(payload, payload_len, (void *)input, payload_len_out, states,
				    (zcbor_decoder_t *)encode_enc_fw_req,
				    sizeof(states) / sizeof(zcbor_state_t), 1);
}

int cbor_encode_enc_fw_rsp(uint8_t *payload, size_t payload_len, const int32_t *input,
			   size_t *payload_len_out)
{
	zcbor_state_t states[3];

	return zcbor_entry_function(payload, payload_len, (void *)input, payload_len_out, states,
				    (zcbor_decoder_t *)encode_enc_fw_rsp,
				    sizeof(states) / sizeof(zcbor_state_t), 1);
}
