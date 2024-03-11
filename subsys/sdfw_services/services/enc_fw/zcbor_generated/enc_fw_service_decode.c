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
#include "enc_fw_service_decode.h"
#include "zcbor_print.h"

#if DEFAULT_MAX_QTY != 3
#error "The type file was generated with a different default_max_qty than this file"
#endif

static bool decode_init(zcbor_state_t *state, struct init *result);
static bool decode_chunk(zcbor_state_t *state, struct chunk *result);
static bool decode_enc_fw_rsp(zcbor_state_t *state, int32_t *result);
static bool decode_enc_fw_req(zcbor_state_t *state, struct enc_fw_req *result);

static bool decode_init(zcbor_state_t *state, struct init *result)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result =
		(((((zcbor_uint32_expect(state, (0)))) &&
		   ((zcbor_bstr_decode(state, (&(*result).init_aad))) &&
		    ((((*result).init_aad.len >= 8) && ((*result).init_aad.len <= 8)) ||
		     (zcbor_error(state, ZCBOR_ERR_WRONG_RANGE), false))) &&
		   ((zcbor_bstr_decode(state, (&(*result).init_nonce))) &&
		    ((((*result).init_nonce.len >= 12) && ((*result).init_nonce.len <= 12)) ||
		     (zcbor_error(state, ZCBOR_ERR_WRONG_RANGE), false))) &&
		   ((zcbor_bstr_decode(state, (&(*result).init_tag))) &&
		    ((((*result).init_tag.len >= 16) && ((*result).init_tag.len <= 16)) ||
		     (zcbor_error(state, ZCBOR_ERR_WRONG_RANGE), false))) &&
		   ((zcbor_uint32_decode(state, (&(*result).init_buffer_addr)))) &&
		   ((zcbor_uint32_decode(state, (&(*result).init_buffer_len)))) &&
		   ((zcbor_uint32_decode(state, (&(*result).init_image_addr)))) &&
		   ((zcbor_uint32_decode(state, (&(*result).init_image_len)))))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool decode_chunk(zcbor_state_t *state, struct chunk *result)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result = (((((zcbor_uint32_expect(state, (1)))) &&
			     ((zcbor_uint32_decode(state, (&(*result).chunk_length)))) &&
			     ((zcbor_bool_decode(state, (&(*result).chunk_last)))))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool decode_enc_fw_rsp(zcbor_state_t *state, int32_t *result)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result = (((zcbor_list_start_decode(state) &&
			     ((((zcbor_int32_decode(state, (&(*result)))))) ||
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

static bool decode_enc_fw_req(zcbor_state_t *state, struct enc_fw_req *result)
{
	zcbor_log("%s\r\n", __func__);
	bool int_res;

	bool tmp_result =
		(((zcbor_list_start_decode(state) &&
		   ((((zcbor_union_start_code(state) &&
		       (int_res = ((((decode_init(state, (&(*result).enc_fw_req_msg_init_m)))) &&
				    (((*result).enc_fw_req_msg_choice = enc_fw_req_msg_init_m_c),
				     true)) ||
				   (zcbor_union_elem_code(state) &&
				    (((decode_chunk(state, (&(*result).enc_fw_req_msg_chunk_m)))) &&
				     (((*result).enc_fw_req_msg_choice = enc_fw_req_msg_chunk_m_c),
				      true)))),
			zcbor_union_end_code(state), int_res)))) ||
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

int cbor_decode_enc_fw_req(const uint8_t *payload, size_t payload_len, struct enc_fw_req *result,
			   size_t *payload_len_out)
{
	zcbor_state_t states[4];

	return zcbor_entry_function(payload, payload_len, (void *)result, payload_len_out, states,
				    (zcbor_decoder_t *)decode_enc_fw_req,
				    sizeof(states) / sizeof(zcbor_state_t), 1);
}

int cbor_decode_enc_fw_rsp(const uint8_t *payload, size_t payload_len, int32_t *result,
			   size_t *payload_len_out)
{
	zcbor_state_t states[3];

	return zcbor_entry_function(payload, payload_len, (void *)result, payload_len_out, states,
				    (zcbor_decoder_t *)decode_enc_fw_rsp,
				    sizeof(states) / sizeof(zcbor_state_t), 1);
}
