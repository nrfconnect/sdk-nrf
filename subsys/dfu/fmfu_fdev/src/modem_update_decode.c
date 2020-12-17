/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 * Generated with cddl_gen.py (https://github.com/oyvindronningstad/cddl_gen)
 * Generated with a default_maxq of 128
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "cbor_decode.h"
#include "modem_update_decode.h"

#if DEFAULT_MAXQ != 128
#error "The type file was generated with a different default_maxq than this file"
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
			 (((tstrx_expect(state,
					 ((tmp_str.value = "Signature1",
					   tmp_str.len = 10, &tmp_str))))) &&
			  (((int_res =
				     (bstrx_cbor_start_decode(
					      state,
					      &(*result)._Sig_structure1_body_protected) &&
				      ((decode_header_map(
					      state,
					      (&(*result)
							._Sig_structure1_body_protected_cbor)))))),
			    bstrx_cbor_end_decode(state), int_res)) &&
			  ((bstrx_expect(state,
					 ((tmp_str.value = "", tmp_str.len = 0,
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

__attribute__((unused)) static bool
type_test_decode_Wrapper(struct COSE_Sign1_Manifest *result)
{
	/* This function should not be called, it is present only to test that
	 * the types of the function and struct match, since this information
	 * is lost with the casts in the entry funciton.
	 */
	return decode_Wrapper(NULL, result);
}

bool cbor_decode_Wrapper(const uint8_t *payload, size_t payload_len,
			 struct COSE_Sign1_Manifest *result,
			 size_t *payload_len_out)
{
	return entry_function(payload, payload_len, (const void *)result,
			      payload_len_out, (void *)decode_Wrapper, 1, 3);
}

__attribute__((unused)) static bool
type_test_decode_Sig_structure1(struct Sig_structure1 *result)
{
	/* This function should not be called, it is present only to test that
	 * the types of the function and struct match, since this information
	 * is lost with the casts in the entry funciton.
	 */
	return decode_Sig_structure1(NULL, result);
}

bool cbor_decode_Sig_structure1(const uint8_t *payload, size_t payload_len,
				struct Sig_structure1 *result,
				size_t *payload_len_out)
{
	return entry_function(payload, payload_len, (const void *)result,
			      payload_len_out, (void *)decode_Sig_structure1, 1,
			      3);
}

__attribute__((unused)) static bool
type_test_decode_Segments(struct Segments *result)
{
	/* This function should not be called, it is present only to test that
	 * the types of the function and struct match, since this information
	 * is lost with the casts in the entry funciton.
	 */
	return decode_Segments(NULL, result);
}

bool cbor_decode_Segments(const uint8_t *payload, size_t payload_len,
			  struct Segments *result, size_t *payload_len_out)
{
	return entry_function(payload, payload_len, (const void *)result,
			      payload_len_out, (void *)decode_Segments, 1, 1);
}
