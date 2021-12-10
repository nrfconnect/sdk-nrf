/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 * Generated using cddl_gen version 0.3.99
 * https://github.com/NordicSemiconductor/cddl-gen
 * Generated with a default_max_qty of 128
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "cbor_decode.h"
#include "modem_update_decode.h"

#if DEFAULT_MAX_QTY != 128
#error "The type file was generated with a different default_max_qty than this file"
#endif

static bool decode_Segment(cbor_state_t *state, struct Segment *result)
{
	cbor_print("%s\n", __func__);

	bool tmp_result = ((
		(((uintx32_decode(state, (&(*result)._Segment_target_addr)))) &&
		 ((uintx32_decode(state, (&(*result)._Segment_len)))))));

	if (!tmp_result)
		cbor_trace();

	return tmp_result;
}

static bool decode_header_map(cbor_state_t *state, void *result)
{
	cbor_print("%s\n", __func__);
	bool int_res;

	bool tmp_result = (((map_start_decode(state) &&
			     (int_res = ((((uintx32_expect(state, (1)))) &&
					  (intx32_expect(state, (-37))))),
			      ((map_end_decode(state)) && int_res)))));

	if (!tmp_result)
		cbor_trace();

	return tmp_result;
}

static bool decode_Manifest(cbor_state_t *state, struct Manifest *result)
{
	cbor_print("%s\n", __func__);
	bool int_res;

	bool tmp_result = (((
		list_start_decode(state) &&
		(int_res = (((uintx32_decode(
				    state, (&(*result)._Manifest_version)))) &&
			    ((uintx32_decode(state,
					     (&(*result)._Manifest_compat)))) &&
			    ((bstrx_decode(state,
					   (&(*result)._Manifest_blob_hash))) &&
			     ((*result)._Manifest_blob_hash.len >= 32) &&
			     ((*result)._Manifest_blob_hash.len <= 32)) &&
			    ((bstrx_decode(state,
					   (&(*result)._Manifest_segments))))),
		 ((list_end_decode(state)) && int_res)))));

	if (!tmp_result)
		cbor_trace();

	return tmp_result;
}

static bool decode_COSE_Sign1_Manifest(cbor_state_t *state,
				       struct COSE_Sign1_Manifest *result)
{
	cbor_print("%s\n", __func__);
	bool int_res;

	bool tmp_result = (((
		list_start_decode(state) &&
		(int_res =
			 ((((int_res =
				     (bstrx_cbor_start_decode(
					      state,
					      &(*result)._COSE_Sign1_Manifest_protected) &&
				      ((decode_header_map(
					      state,
					      (&(*result)
							._COSE_Sign1_Manifest_protected_cbor)))))),
			    bstrx_cbor_end_decode(state), int_res)) &&
			  ((map_start_decode(state) && map_end_decode(state))) &&
			  (((int_res =
				     (bstrx_cbor_start_decode(
					      state,
					      &(*result)._COSE_Sign1_Manifest_payload) &&
				      ((decode_Manifest(
					      state,
					      (&(*result)
							._COSE_Sign1_Manifest_payload_cbor)))))),
			    bstrx_cbor_end_decode(state), int_res)) &&
			  ((bstrx_decode(
				   state,
				   (&(*result)._COSE_Sign1_Manifest_signature))) &&
			   ((*result)._COSE_Sign1_Manifest_signature.len >=
			    256) &&
			   ((*result)._COSE_Sign1_Manifest_signature.len <=
			    256))),
		 ((list_end_decode(state)) && int_res)))));

	if (!tmp_result)
		cbor_trace();

	return tmp_result;
}

static bool decode_Segments(cbor_state_t *state, struct Segments *result)
{
	cbor_print("%s\n", __func__);
	bool int_res;

	bool tmp_result =
		(((list_start_decode(state) &&
		   (int_res = (multi_decode(1, 128,
					    &(*result)._Segments__Segment_count,
					    (void *)decode_Segment, state,
					    (&(*result)._Segments__Segment),
					    sizeof(struct Segment))),
		    ((list_end_decode(state)) && int_res)))));

	if (!tmp_result)
		cbor_trace();

	return tmp_result;
}

static bool decode_Sig_structure1(cbor_state_t *state,
				  struct Sig_structure1 *result)
{
	cbor_print("%s\n", __func__);
	cbor_string_type_t tmp_str;
	bool int_res;

	bool tmp_result = (((
		list_start_decode(state) &&
		(int_res =
			 (((tstrx_expect(
				  state,
				  ((tmp_str.value = "Signature1",
				    tmp_str.len = sizeof("Signature1") - 1,
				    &tmp_str))))) &&
			  (((int_res =
				     (bstrx_cbor_start_decode(
					      state,
					      &(*result)._Sig_structure1_body_protected) &&
				      ((decode_header_map(
					      state,
					      (&(*result)
							._Sig_structure1_body_protected_cbor)))))),
			    bstrx_cbor_end_decode(state), int_res)) &&
			  ((bstrx_expect(state, ((tmp_str.value = "",
						  tmp_str.len = sizeof("") - 1,
						  &tmp_str))))) &&
			  ((bstrx_decode(
				  state,
				  (&(*result)._Sig_structure1_payload))))),
		 ((list_end_decode(state)) && int_res)))));

	if (!tmp_result)
		cbor_trace();

	return tmp_result;
}

static bool decode_Wrapper(cbor_state_t *state,
			   struct COSE_Sign1_Manifest *result)
{
	cbor_print("%s\n", __func__);

	bool tmp_result = ((tag_expect(state, 18) &&
			    (decode_COSE_Sign1_Manifest(state, (&(*result))))));

	if (!tmp_result)
		cbor_trace();

	return tmp_result;
}

bool cbor_decode_Wrapper(const uint8_t *payload, uint32_t payload_len,
			 struct COSE_Sign1_Manifest *result,
			 uint32_t *payload_len_out)
{
	cbor_state_t states[5];

	new_state(states, sizeof(states) / sizeof(cbor_state_t), payload,
		  payload_len, 1);

	bool ret = decode_Wrapper(states, result);

	if (ret && (payload_len_out != NULL)) {
		*payload_len_out = MIN(payload_len, (size_t)states[0].payload -
							    (size_t)payload);
	}

	return ret;
}

bool cbor_decode_Sig_structure1(const uint8_t *payload, uint32_t payload_len,
				struct Sig_structure1 *result,
				uint32_t *payload_len_out)
{
	cbor_state_t states[5];

	new_state(states, sizeof(states) / sizeof(cbor_state_t), payload,
		  payload_len, 1);

	bool ret = decode_Sig_structure1(states, result);

	if (ret && (payload_len_out != NULL)) {
		*payload_len_out = MIN(payload_len, (size_t)states[0].payload -
							    (size_t)payload);
	}

	return ret;
}

bool cbor_decode_Segments(const uint8_t *payload, uint32_t payload_len,
			  struct Segments *result, uint32_t *payload_len_out)
{
	cbor_state_t states[3];

	new_state(states, sizeof(states) / sizeof(cbor_state_t), payload,
		  payload_len, 1);

	bool ret = decode_Segments(states, result);

	if (ret && (payload_len_out != NULL)) {
		*payload_len_out = MIN(payload_len, (size_t)states[0].payload -
							    (size_t)payload);
	}

	return ret;
}
