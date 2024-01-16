/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 * Generated using zcbor version 0.8.1
 * https://github.com/NordicSemiconductor/zcbor
 * Generated with a --default-max-qty of 128
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "zcbor_decode.h"
#include "modem_update_decode.h"
#include "zcbor_print.h"

#if DEFAULT_MAX_QTY != 128
#error "The type file was generated with a different default_max_qty than this file"
#endif

static bool decode_Segment(zcbor_state_t *state, struct Segment *result);
static bool decode_header_map(zcbor_state_t *state, void *result);
static bool decode_Manifest(zcbor_state_t *state, struct Manifest *result);
static bool decode_COSE_Sign1_Manifest(zcbor_state_t *state, struct COSE_Sign1_Manifest *result);
static bool decode_Segments(zcbor_state_t *state, struct Segments *result);
static bool decode_Sig_structure1(zcbor_state_t *state, struct Sig_structure1 *result);
static bool decode_Wrapper(zcbor_state_t *state, struct COSE_Sign1_Manifest *result);

static bool decode_Segment(zcbor_state_t *state, struct Segment *result)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result = (((((zcbor_uint32_decode(state, (&(*result).Segment_target_addr)))) &&
			     ((zcbor_uint32_decode(state, (&(*result).Segment_len)))))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool decode_header_map(zcbor_state_t *state, void *result)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result =
		(((zcbor_map_start_decode(state) &&
		   (((((zcbor_uint32_expect(state, (1)))) && (zcbor_int32_expect(state, (-37))))) ||
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

static bool decode_Manifest(zcbor_state_t *state, struct Manifest *result)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result = (((zcbor_list_start_decode(state) &&
			     ((((zcbor_uint32_decode(state, (&(*result).Manifest_version)))) &&
			       ((zcbor_uint32_decode(state, (&(*result).Manifest_compat)))) &&
			       ((zcbor_bstr_decode(state, (&(*result).Manifest_blob_hash))) &&
				((((*result).Manifest_blob_hash.len >= 32) &&
				  ((*result).Manifest_blob_hash.len <= 32)) ||
				 (zcbor_error(state, ZCBOR_ERR_WRONG_RANGE), false))) &&
			       ((zcbor_bstr_decode(state, (&(*result).Manifest_segments))))) ||
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

static bool decode_COSE_Sign1_Manifest(zcbor_state_t *state, struct COSE_Sign1_Manifest *result)
{
	zcbor_log("%s\r\n", __func__);
	bool int_res;

	bool tmp_result =
		(((zcbor_list_start_decode(state) &&
		   ((((zcbor_bstr_start_decode(state, &(*result).COSE_Sign1_Manifest_protected) &&
		       (int_res = (((decode_header_map(state, NULL)))),
			zcbor_bstr_end_decode(state), int_res))) &&
		     ((zcbor_map_start_decode(state) && zcbor_map_end_decode(state))) &&
		     ((zcbor_bstr_start_decode(state, &(*result).COSE_Sign1_Manifest_payload) &&
		       (int_res = (((decode_Manifest(
				state, (&(*result).COSE_Sign1_Manifest_payload_cbor))))),
			zcbor_bstr_end_decode(state), int_res))) &&
		     ((zcbor_bstr_decode(state, (&(*result).COSE_Sign1_Manifest_signature))) &&
		      ((((*result).COSE_Sign1_Manifest_signature.len >= 256) &&
			((*result).COSE_Sign1_Manifest_signature.len <= 256)) ||
		       (zcbor_error(state, ZCBOR_ERR_WRONG_RANGE), false)))) ||
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

static bool decode_Segments(zcbor_state_t *state, struct Segments *result)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result =
		(((zcbor_list_start_decode(state) &&
		   ((zcbor_multi_decode(1, 128, &(*result).Segments_Segment_m_count,
					(zcbor_decoder_t *)decode_Segment, state,
					(&(*result).Segments_Segment_m), sizeof(struct Segment))) ||
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

static bool decode_Sig_structure1(zcbor_state_t *state, struct Sig_structure1 *result)
{
	zcbor_log("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool tmp_result =
		(((zcbor_list_start_decode(state) &&
		   ((((zcbor_tstr_expect(state,
					 ((tmp_str.value = (uint8_t *)"Signature1",
					   tmp_str.len = sizeof("Signature1") - 1, &tmp_str))))) &&
		     ((zcbor_bstr_start_decode(state, &(*result).Sig_structure1_body_protected) &&
		       (int_res = (((decode_header_map(state, NULL)))),
			zcbor_bstr_end_decode(state), int_res))) &&
		     ((zcbor_bstr_expect(state, ((tmp_str.value = (uint8_t *)"",
						  tmp_str.len = sizeof("") - 1, &tmp_str))))) &&
		     ((zcbor_bstr_decode(state, (&(*result).Sig_structure1_payload))))) ||
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

static bool decode_Wrapper(zcbor_state_t *state, struct COSE_Sign1_Manifest *result)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result = ((zcbor_tag_expect(state, 18) &&
			    (decode_COSE_Sign1_Manifest(state, (&(*result))))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

int cbor_decode_Wrapper(const uint8_t *payload, size_t payload_len,
			struct COSE_Sign1_Manifest *result, size_t *payload_len_out)
{
	zcbor_state_t states[5];

	return zcbor_entry_function(payload, payload_len, (void *)result, payload_len_out, states,
				    (zcbor_decoder_t *)decode_Wrapper,
				    sizeof(states) / sizeof(zcbor_state_t), 1);
}

int cbor_decode_Sig_structure1(const uint8_t *payload, size_t payload_len,
			       struct Sig_structure1 *result, size_t *payload_len_out)
{
	zcbor_state_t states[5];

	return zcbor_entry_function(payload, payload_len, (void *)result, payload_len_out, states,
				    (zcbor_decoder_t *)decode_Sig_structure1,
				    sizeof(states) / sizeof(zcbor_state_t), 1);
}

int cbor_decode_Segments(const uint8_t *payload, size_t payload_len, struct Segments *result,
			 size_t *payload_len_out)
{
	zcbor_state_t states[3];

	return zcbor_entry_function(payload, payload_len, (void *)result, payload_len_out, states,
				    (zcbor_decoder_t *)decode_Segments,
				    sizeof(states) / sizeof(zcbor_state_t), 1);
}
