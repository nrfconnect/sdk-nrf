/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 * Generated using zcbor version 0.5.1
 * https://github.com/NordicSemiconductor/zcbor
 * Generated with a --default-max-qty of 128
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "zcbor_decode.h"
#include "modem_update_decode.h"

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
	zcbor_print("%s\r\n", __func__);

	bool tmp_result = (((((zcbor_uint32_decode(state, (&(*result)._Segment_target_addr)))) &&
			     ((zcbor_uint32_decode(state, (&(*result)._Segment_len)))))));

	if (!tmp_result)
		zcbor_trace();

	return tmp_result;
}

static bool decode_header_map(zcbor_state_t *state, void *result)
{
	zcbor_print("%s\r\n", __func__);

	bool tmp_result =
		(((zcbor_map_start_decode(state) &&
		   (((((zcbor_uint32_expect(state, (1)))) && (zcbor_int32_expect(state, (-37))))) ||
		    (zcbor_list_map_end_force_decode(state), false)) &&
		   zcbor_map_end_decode(state))));

	if (!tmp_result)
		zcbor_trace();

	return tmp_result;
}

static bool decode_Manifest(zcbor_state_t *state, struct Manifest *result)
{
	zcbor_print("%s\r\n", __func__);

	bool tmp_result = (((zcbor_list_start_decode(state) &&
			     ((((zcbor_uint32_decode(state, (&(*result)._Manifest_version)))) &&
			       ((zcbor_uint32_decode(state, (&(*result)._Manifest_compat)))) &&
			       ((zcbor_bstr_decode(state, (&(*result)._Manifest_blob_hash))) &&
				((((*result)._Manifest_blob_hash.len >= 32) &&
				  ((*result)._Manifest_blob_hash.len <= 32)) ||
				 (zcbor_error(state, ZCBOR_ERR_WRONG_RANGE), false))) &&
			       ((zcbor_bstr_decode(state, (&(*result)._Manifest_segments))))) ||
			      (zcbor_list_map_end_force_decode(state), false)) &&
			     zcbor_list_end_decode(state))));

	if (!tmp_result)
		zcbor_trace();

	return tmp_result;
}

static bool decode_COSE_Sign1_Manifest(zcbor_state_t *state, struct COSE_Sign1_Manifest *result)
{
	zcbor_print("%s\r\n", __func__);
	bool int_res;

	bool tmp_result =
		(((zcbor_list_start_decode(state) &&
		   ((((zcbor_bstr_start_decode(state, NULL) &&
		       (int_res = (((decode_header_map(state, NULL)))),
			zcbor_bstr_end_decode(state), int_res))) &&
		     ((zcbor_map_start_decode(state) && zcbor_map_end_decode(state))) &&
		     ((zcbor_bstr_start_decode(state, &(*result)._COSE_Sign1_Manifest_payload) &&
		       (int_res = (((decode_Manifest(
				state, (&(*result)._COSE_Sign1_Manifest_payload_cbor))))),
			zcbor_bstr_end_decode(state), int_res))) &&
		     ((zcbor_bstr_decode(state, (&(*result)._COSE_Sign1_Manifest_signature))) &&
		      ((((*result)._COSE_Sign1_Manifest_signature.len >= 256) &&
			((*result)._COSE_Sign1_Manifest_signature.len <= 256)) ||
		       (zcbor_error(state, ZCBOR_ERR_WRONG_RANGE), false)))) ||
		    (zcbor_list_map_end_force_decode(state), false)) &&
		   zcbor_list_end_decode(state))));

	if (!tmp_result)
		zcbor_trace();

	return tmp_result;
}

static bool decode_Segments(zcbor_state_t *state, struct Segments *result)
{
	zcbor_print("%s\r\n", __func__);

	bool tmp_result =
		(((zcbor_list_start_decode(state) &&
		   ((zcbor_multi_decode(1, 128, &(*result)._Segments__Segment_count,
					(zcbor_decoder_t *)decode_Segment, state,
					(&(*result)._Segments__Segment), sizeof(struct Segment))) ||
		    (zcbor_list_map_end_force_decode(state), false)) &&
		   zcbor_list_end_decode(state))));

	if (!tmp_result)
		zcbor_trace();

	return tmp_result;
}

static bool decode_Sig_structure1(zcbor_state_t *state, struct Sig_structure1 *result)
{
	zcbor_print("%s\r\n", __func__);
	struct zcbor_string tmp_str;
	bool int_res;

	bool tmp_result =
		(((zcbor_list_start_decode(state) &&
		   ((((zcbor_tstr_expect(state,
					 ((tmp_str.value = (uint8_t *)"Signature1",
					   tmp_str.len = sizeof("Signature1") - 1, &tmp_str))))) &&
		     ((zcbor_bstr_start_decode(state, NULL) &&
		       (int_res = (((decode_header_map(state, NULL)))),
			zcbor_bstr_end_decode(state), int_res))) &&
		     ((zcbor_bstr_expect(state, ((tmp_str.value = (uint8_t *)"",
						  tmp_str.len = sizeof("") - 1, &tmp_str))))) &&
		     ((zcbor_bstr_decode(state, (&(*result)._Sig_structure1_payload))))) ||
		    (zcbor_list_map_end_force_decode(state), false)) &&
		   zcbor_list_end_decode(state))));

	if (!tmp_result)
		zcbor_trace();

	return tmp_result;
}

static bool decode_Wrapper(zcbor_state_t *state, struct COSE_Sign1_Manifest *result)
{
	zcbor_print("%s\r\n", __func__);

	bool tmp_result = ((zcbor_tag_expect(state, 18) &&
			    (decode_COSE_Sign1_Manifest(state, (&(*result))))));

	if (!tmp_result)
		zcbor_trace();

	return tmp_result;
}

int cbor_decode_Wrapper(const uint8_t *payload, size_t payload_len,
			struct COSE_Sign1_Manifest *result, size_t *payload_len_out)
{
	zcbor_state_t states[5];

	zcbor_new_state(states, sizeof(states) / sizeof(zcbor_state_t), payload, payload_len, 1);

	bool ret = decode_Wrapper(states, result);

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

int cbor_decode_Sig_structure1(const uint8_t *payload, size_t payload_len,
			       struct Sig_structure1 *result, size_t *payload_len_out)
{
	zcbor_state_t states[5];

	zcbor_new_state(states, sizeof(states) / sizeof(zcbor_state_t), payload, payload_len, 1);

	bool ret = decode_Sig_structure1(states, result);

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

int cbor_decode_Segments(const uint8_t *payload, size_t payload_len, struct Segments *result,
			 size_t *payload_len_out)
{
	zcbor_state_t states[3];

	zcbor_new_state(states, sizeof(states) / sizeof(zcbor_state_t), payload, payload_len, 1);

	bool ret = decode_Segments(states, result);

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
