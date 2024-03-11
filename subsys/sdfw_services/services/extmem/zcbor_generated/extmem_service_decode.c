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
#include "extmem_service_decode.h"
#include "zcbor_print.h"

#if DEFAULT_MAX_QTY != 3
#error "The type file was generated with a different default_max_qty than this file"
#endif

static bool decode_extmem_read_done_req(zcbor_state_t *state, struct extmem_read_done_req *result);
static bool decode_extmem_write_setup_req(zcbor_state_t *state,
					  struct extmem_write_setup_req *result);
static bool decode_extmem_write_done_req(zcbor_state_t *state,
					 struct extmem_write_done_req *result);
static bool decode_extmem_erase_done_req(zcbor_state_t *state,
					 struct extmem_erase_done_req *result);
static bool decode_extmem_get_capabilities_req(zcbor_state_t *state,
					       struct extmem_get_capabilities_req *result);
static bool decode_extmem_read_pending_notify(zcbor_state_t *state,
					      struct extmem_read_pending_notify *result);
static bool decode_extmem_write_pending_notify(zcbor_state_t *state,
					       struct extmem_write_pending_notify *result);
static bool decode_extmem_erase_pending_notify(zcbor_state_t *state,
					       struct extmem_erase_pending_notify *result);
static bool decode_extmem_get_capabilities_notify_pending(
	zcbor_state_t *state, struct extmem_get_capabilities_notify_pending *result);
static bool decode_extmem_nfy(zcbor_state_t *state, struct extmem_nfy *result);
static bool decode_extmem_rsp(zcbor_state_t *state, struct extmem_rsp *result);
static bool decode_extmem_req(zcbor_state_t *state, struct extmem_req *result);

static bool decode_extmem_read_done_req(zcbor_state_t *state, struct extmem_read_done_req *result)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result =
		(((((zcbor_uint32_expect(state, (1)))) &&
		   ((zcbor_uint32_decode(state, (&(*result).extmem_read_done_req_request_id)))) &&
		   ((zcbor_uint32_decode(state, (&(*result).extmem_read_done_req_error)))) &&
		   ((zcbor_uint32_decode(state, (&(*result).extmem_read_done_req_addr)))))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool decode_extmem_write_setup_req(zcbor_state_t *state,
					  struct extmem_write_setup_req *result)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result =
		(((((zcbor_uint32_expect(state, (3)))) &&
		   ((zcbor_uint32_decode(state, (&(*result).extmem_write_setup_req_request_id)))) &&
		   ((zcbor_uint32_decode(state, (&(*result).extmem_write_setup_req_error)))) &&
		   ((zcbor_uint32_decode(state, (&(*result).extmem_write_setup_req_addr)))))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool decode_extmem_write_done_req(zcbor_state_t *state, struct extmem_write_done_req *result)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result =
		(((((zcbor_uint32_expect(state, (5)))) &&
		   ((zcbor_uint32_decode(state, (&(*result).extmem_write_done_req_request_id)))) &&
		   ((zcbor_uint32_decode(state, (&(*result).extmem_write_done_req_error)))))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool decode_extmem_erase_done_req(zcbor_state_t *state, struct extmem_erase_done_req *result)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result =
		(((((zcbor_uint32_expect(state, (7)))) &&
		   ((zcbor_uint32_decode(state, (&(*result).extmem_erase_done_req_request_id)))) &&
		   ((zcbor_uint32_decode(state, (&(*result).extmem_erase_done_req_error)))))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool decode_extmem_get_capabilities_req(zcbor_state_t *state,
					       struct extmem_get_capabilities_req *result)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result = (((
		((zcbor_uint32_expect(state, (9)))) &&
		((zcbor_uint32_decode(state,
				      (&(*result).extmem_get_capabilities_req_request_id)))) &&
		((zcbor_uint32_decode(state, (&(*result).extmem_get_capabilities_req_error)))) &&
		((zcbor_uint32_decode(state,
				      (&(*result).extmem_get_capabilities_req_base_addr)))) &&
		((zcbor_uint32_decode(state, (&(*result).extmem_get_capabilities_req_capacity)))) &&
		((zcbor_uint32_decode(state,
				      (&(*result).extmem_get_capabilities_req_erase_size)))) &&
		((zcbor_uint32_decode(state,
				      (&(*result).extmem_get_capabilities_req_write_size)))) &&
		((zcbor_uint32_decode(state,
				      (&(*result).extmem_get_capabilities_req_chunk_size)))) &&
		((zcbor_bool_decode(state,
				    (&(*result).extmem_get_capabilities_req_memory_mapped)))))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool decode_extmem_read_pending_notify(zcbor_state_t *state,
					      struct extmem_read_pending_notify *result)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result =
		(((((zcbor_uint32_expect(state, (2)))) &&
		   ((zcbor_uint32_decode(state,
					 (&(*result).extmem_read_pending_notify_request_id)))) &&
		   ((zcbor_uint32_decode(state, (&(*result).extmem_read_pending_notify_offset)))) &&
		   ((zcbor_uint32_decode(state, (&(*result).extmem_read_pending_notify_size)))))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool decode_extmem_write_pending_notify(zcbor_state_t *state,
					       struct extmem_write_pending_notify *result)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result = ((
		(((zcbor_uint32_expect(state, (6)))) &&
		 ((zcbor_uint32_decode(state,
				       (&(*result).extmem_write_pending_notify_request_id)))) &&
		 ((zcbor_uint32_decode(state, (&(*result).extmem_write_pending_notify_offset)))) &&
		 ((zcbor_uint32_decode(state, (&(*result).extmem_write_pending_notify_size)))))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool decode_extmem_erase_pending_notify(zcbor_state_t *state,
					       struct extmem_erase_pending_notify *result)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result = ((
		(((zcbor_uint32_expect(state, (8)))) &&
		 ((zcbor_uint32_decode(state,
				       (&(*result).extmem_erase_pending_notify_request_id)))) &&
		 ((zcbor_uint32_decode(state, (&(*result).extmem_erase_pending_notify_offset)))) &&
		 ((zcbor_uint32_decode(state, (&(*result).extmem_erase_pending_notify_size)))))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool
decode_extmem_get_capabilities_notify_pending(zcbor_state_t *state,
					      struct extmem_get_capabilities_notify_pending *result)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result = (((
		((zcbor_uint32_expect(state, (10)))) &&
		((zcbor_uint32_decode(
			state, (&(*result).extmem_get_capabilities_notify_pending_request_id)))))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool decode_extmem_nfy(zcbor_state_t *state, struct extmem_nfy *result)
{
	zcbor_log("%s\r\n", __func__);
	bool int_res;

	bool tmp_result = (((zcbor_list_start_decode(state) && ((((zcbor_union_start_code(state) &&
	(int_res = ((((decode_extmem_read_pending_notify(
		state, (&(*result).extmem_nfy_msg_extmem_read_pending_notify_m)))) &&
		(((*result).extmem_nfy_msg_choice =
			extmem_nfy_msg_extmem_read_pending_notify_m_c), true)) ||
		(zcbor_union_elem_code(state) &&
		(((decode_extmem_write_pending_notify(
		state, (&(*result).extmem_nfy_msg_extmem_write_pending_notify_m)))) &&
		(((*result).extmem_nfy_msg_choice =
			extmem_nfy_msg_extmem_write_pending_notify_m_c), true))) ||
		(zcbor_union_elem_code(state) &&
		(((decode_extmem_erase_pending_notify(
			state, (&(*result).extmem_nfy_msg_extmem_erase_pending_notify_m)))) &&
		(((*result).extmem_nfy_msg_choice =
			extmem_nfy_msg_extmem_erase_pending_notify_m_c), true))) ||
		(zcbor_union_elem_code(state) &&
		(((decode_extmem_get_capabilities_notify_pending(
			state,
			(&(*result).extmem_nfy_msg_extmem_get_capabilities_notify_pending_m)))) &&
		(((*result).extmem_nfy_msg_choice =
			extmem_nfy_msg_extmem_get_capabilities_notify_pending_m_c), true)))),
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

static bool decode_extmem_rsp(zcbor_state_t *state, struct extmem_rsp *result)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result = (((zcbor_list_start_decode(state) &&
			     ((((((zcbor_uint_decode(state, &(*result).extmem_rsp_msg_choice,
						     sizeof((*result).extmem_rsp_msg_choice)))) &&
				 ((((((*result).extmem_rsp_msg_choice ==
				      extmem_rsp_msg_extmem_read_done_rsp_m_c) &&
				     ((1))) ||
				    (((*result).extmem_rsp_msg_choice ==
				      extmem_rsp_msg_extmem_write_setup_rsp_m_c) &&
				     ((1))) ||
				    (((*result).extmem_rsp_msg_choice ==
				      extmem_rsp_msg_extmem_write_done_rsp_m_c) &&
				     ((1))) ||
				    (((*result).extmem_rsp_msg_choice ==
				      extmem_rsp_msg_extmem_erase_done_rsp_m_c) &&
				     ((1))) ||
				    (((*result).extmem_rsp_msg_choice ==
				      extmem_rsp_msg_extmem_get_capabilities_rsp_m_c) &&
				     ((1)))) ||
				   (zcbor_error(state, ZCBOR_ERR_WRONG_VALUE), false)))))) ||
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

static bool decode_extmem_req(zcbor_state_t *state, struct extmem_req *result)
{
	zcbor_log("%s\r\n", __func__);
	bool int_res;

	bool tmp_result = (((
		zcbor_list_start_decode(state) &&
		((((zcbor_union_start_code(state) &&
		    (int_res =
			     ((((decode_extmem_read_done_req(
				       state,
				       (&(*result).extmem_req_msg_extmem_read_done_req_m)))) &&
			       (((*result).extmem_req_msg_choice =
					 extmem_req_msg_extmem_read_done_req_m_c),
				true)) ||
			      (zcbor_union_elem_code(state) &&
			       (((decode_extmem_write_setup_req(
					state,
					(&(*result).extmem_req_msg_extmem_write_setup_req_m)))) &&
				(((*result).extmem_req_msg_choice =
					  extmem_req_msg_extmem_write_setup_req_m_c),
				 true))) ||
			      (zcbor_union_elem_code(state) &&
			       (((decode_extmem_write_done_req(
					state,
					(&(*result).extmem_req_msg_extmem_write_done_req_m)))) &&
				(((*result).extmem_req_msg_choice =
					  extmem_req_msg_extmem_write_done_req_m_c),
				 true))) ||
			      (zcbor_union_elem_code(state) &&
			       (((decode_extmem_erase_done_req(
					state,
					(&(*result).extmem_req_msg_extmem_erase_done_req_m)))) &&
				(((*result).extmem_req_msg_choice =
					  extmem_req_msg_extmem_erase_done_req_m_c),
				 true))) ||
			      (zcbor_union_elem_code(state) &&
			       (((decode_extmem_get_capabilities_req(
					state,
					(&(*result)
						.extmem_req_msg_extmem_get_capabilities_req_m)))
					) &&
				(((*result).extmem_req_msg_choice =
					  extmem_req_msg_extmem_get_capabilities_req_m_c),
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

int cbor_decode_extmem_req(const uint8_t *payload, size_t payload_len, struct extmem_req *result,
			   size_t *payload_len_out)
{
	zcbor_state_t states[4];

	return zcbor_entry_function(payload, payload_len, (void *)result, payload_len_out, states,
				    (zcbor_decoder_t *)decode_extmem_req,
				    sizeof(states) / sizeof(zcbor_state_t), 1);
}

int cbor_decode_extmem_rsp(const uint8_t *payload, size_t payload_len, struct extmem_rsp *result,
			   size_t *payload_len_out)
{
	zcbor_state_t states[4];

	return zcbor_entry_function(payload, payload_len, (void *)result, payload_len_out, states,
				    (zcbor_decoder_t *)decode_extmem_rsp,
				    sizeof(states) / sizeof(zcbor_state_t), 1);
}

int cbor_decode_extmem_nfy(const uint8_t *payload, size_t payload_len, struct extmem_nfy *result,
			   size_t *payload_len_out)
{
	zcbor_state_t states[4];

	return zcbor_entry_function(payload, payload_len, (void *)result, payload_len_out, states,
				    (zcbor_decoder_t *)decode_extmem_nfy,
				    sizeof(states) / sizeof(zcbor_state_t), 1);
}
