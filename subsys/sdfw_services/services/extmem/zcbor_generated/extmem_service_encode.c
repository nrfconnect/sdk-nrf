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
#include "extmem_service_encode.h"
#include "zcbor_print.h"

#if DEFAULT_MAX_QTY != 3
#error "The type file was generated with a different default_max_qty than this file"
#endif

static bool encode_extmem_read_done_req(zcbor_state_t *state,
					const struct extmem_read_done_req *input);
static bool encode_extmem_write_setup_req(zcbor_state_t *state,
					  const struct extmem_write_setup_req *input);
static bool encode_extmem_write_done_req(zcbor_state_t *state,
					 const struct extmem_write_done_req *input);
static bool encode_extmem_erase_done_req(zcbor_state_t *state,
					 const struct extmem_erase_done_req *input);
static bool encode_extmem_get_capabilities_req(zcbor_state_t *state,
					       const struct extmem_get_capabilities_req *input);
static bool encode_extmem_read_pending_notify(zcbor_state_t *state,
					      const struct extmem_read_pending_notify *input);
static bool encode_extmem_write_pending_notify(zcbor_state_t *state,
					       const struct extmem_write_pending_notify *input);
static bool encode_extmem_erase_pending_notify(zcbor_state_t *state,
					       const struct extmem_erase_pending_notify *input);
static bool encode_extmem_get_capabilities_notify_pending(
	zcbor_state_t *state, const struct extmem_get_capabilities_notify_pending *input);
static bool encode_extmem_nfy(zcbor_state_t *state, const struct extmem_nfy *input);
static bool encode_extmem_rsp(zcbor_state_t *state, const struct extmem_rsp *input);
static bool encode_extmem_req(zcbor_state_t *state, const struct extmem_req *input);

static bool encode_extmem_read_done_req(zcbor_state_t *state,
					const struct extmem_read_done_req *input)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result =
		(((((zcbor_uint32_put(state, (1)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).extmem_read_done_req_request_id)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).extmem_read_done_req_error)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).extmem_read_done_req_addr)))))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool encode_extmem_write_setup_req(zcbor_state_t *state,
					  const struct extmem_write_setup_req *input)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result =
		(((((zcbor_uint32_put(state, (3)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).extmem_write_setup_req_request_id)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).extmem_write_setup_req_error)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).extmem_write_setup_req_addr)))))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool encode_extmem_write_done_req(zcbor_state_t *state,
					 const struct extmem_write_done_req *input)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result =
		(((((zcbor_uint32_put(state, (5)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).extmem_write_done_req_request_id)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).extmem_write_done_req_error)))))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool encode_extmem_erase_done_req(zcbor_state_t *state,
					 const struct extmem_erase_done_req *input)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result =
		(((((zcbor_uint32_put(state, (7)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).extmem_erase_done_req_request_id)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).extmem_erase_done_req_error)))))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool encode_extmem_get_capabilities_req(zcbor_state_t *state,
					       const struct extmem_get_capabilities_req *input)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result = (((
		((zcbor_uint32_put(state, (9)))) &&
		((zcbor_uint32_encode(state,
				      (&(*input).extmem_get_capabilities_req_request_id)))) &&
		((zcbor_uint32_encode(state, (&(*input).extmem_get_capabilities_req_error)))) &&
		((zcbor_uint32_encode(state, (&(*input).extmem_get_capabilities_req_base_addr)))) &&
		((zcbor_uint32_encode(state, (&(*input).extmem_get_capabilities_req_capacity)))) &&
		((zcbor_uint32_encode(state,
				      (&(*input).extmem_get_capabilities_req_erase_size)))) &&
		((zcbor_uint32_encode(state,
				      (&(*input).extmem_get_capabilities_req_write_size)))) &&
		((zcbor_uint32_encode(state,
				      (&(*input).extmem_get_capabilities_req_chunk_size)))) &&
		((zcbor_bool_encode(state,
				    (&(*input).extmem_get_capabilities_req_memory_mapped)))))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool encode_extmem_read_pending_notify(zcbor_state_t *state,
					      const struct extmem_read_pending_notify *input)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result = (((
		((zcbor_uint32_put(state, (2)))) &&
		((zcbor_uint32_encode(state, (&(*input).extmem_read_pending_notify_request_id)))) &&
		((zcbor_uint32_encode(state, (&(*input).extmem_read_pending_notify_offset)))) &&
		((zcbor_uint32_encode(state, (&(*input).extmem_read_pending_notify_size)))))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool encode_extmem_write_pending_notify(zcbor_state_t *state,
					       const struct extmem_write_pending_notify *input)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result =
		(((((zcbor_uint32_put(state, (6)))) &&
		   ((zcbor_uint32_encode(state,
					 (&(*input).extmem_write_pending_notify_request_id)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).extmem_write_pending_notify_offset)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).extmem_write_pending_notify_size)))))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool encode_extmem_erase_pending_notify(zcbor_state_t *state,
					       const struct extmem_erase_pending_notify *input)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result =
		(((((zcbor_uint32_put(state, (8)))) &&
		   ((zcbor_uint32_encode(state,
					 (&(*input).extmem_erase_pending_notify_request_id)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).extmem_erase_pending_notify_offset)))) &&
		   ((zcbor_uint32_encode(state, (&(*input).extmem_erase_pending_notify_size)))))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool encode_extmem_get_capabilities_notify_pending(
	zcbor_state_t *state, const struct extmem_get_capabilities_notify_pending *input)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result = ((
		(((zcbor_uint32_put(state, (10)))) &&
		 ((zcbor_uint32_encode(
			 state, (&(*input).extmem_get_capabilities_notify_pending_request_id)))))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

static bool encode_extmem_nfy(zcbor_state_t *state, const struct extmem_nfy *input)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result = (((
		zcbor_list_start_encode(state, 4) &&
		((((((*input).extmem_nfy_msg_choice ==
			extmem_nfy_msg_extmem_read_pending_notify_m_c)
			? ((encode_extmem_read_pending_notify(
				state, (&(*input).extmem_nfy_msg_extmem_read_pending_notify_m))))
			: (((*input).extmem_nfy_msg_choice ==
				extmem_nfy_msg_extmem_write_pending_notify_m_c)
			? ((encode_extmem_write_pending_notify(
				state, (&(*input).extmem_nfy_msg_extmem_write_pending_notify_m))))
			: (((*input).extmem_nfy_msg_choice ==
				extmem_nfy_msg_extmem_erase_pending_notify_m_c)
			? ((encode_extmem_erase_pending_notify(
				state, (&(*input).extmem_nfy_msg_extmem_erase_pending_notify_m))))
			: (((*input).extmem_nfy_msg_choice ==
				extmem_nfy_msg_extmem_get_capabilities_notify_pending_m_c)
			? ((encode_extmem_get_capabilities_notify_pending(
				state,
			(&(*input).extmem_nfy_msg_extmem_get_capabilities_notify_pending_m))))
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

static bool encode_extmem_rsp(zcbor_state_t *state, const struct extmem_rsp *input)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result = (((
		zcbor_list_start_encode(state, 1) &&
		((((((*input).extmem_rsp_msg_choice == extmem_rsp_msg_extmem_read_done_rsp_m_c)
			? ((zcbor_uint32_put(state, (1))))
			: (((*input).extmem_rsp_msg_choice ==
				extmem_rsp_msg_extmem_write_setup_rsp_m_c)
			? ((zcbor_uint32_put(state, (3))))
			: (((*input).extmem_rsp_msg_choice ==
				extmem_rsp_msg_extmem_write_done_rsp_m_c)
			? ((zcbor_uint32_put(state, (5))))
			: (((*input).extmem_rsp_msg_choice ==
				extmem_rsp_msg_extmem_erase_done_rsp_m_c)
			? ((zcbor_uint32_put(state, (7))))
			: (((*input).extmem_rsp_msg_choice ==
				extmem_rsp_msg_extmem_get_capabilities_rsp_m_c)
			? ((zcbor_uint32_put(state, (9))))
			: false))))))) ||
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

static bool encode_extmem_req(zcbor_state_t *state, const struct extmem_req *input)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result = (((
		zcbor_list_start_encode(state, 9) &&
		((((((*input).extmem_req_msg_choice == extmem_req_msg_extmem_read_done_req_m_c)
			? ((encode_extmem_read_done_req(
				state, (&(*input).extmem_req_msg_extmem_read_done_req_m))))
			: (((*input).extmem_req_msg_choice ==
				extmem_req_msg_extmem_write_setup_req_m_c)
			? ((encode_extmem_write_setup_req(
				state, (&(*input).extmem_req_msg_extmem_write_setup_req_m))))
			: (((*input).extmem_req_msg_choice ==
				extmem_req_msg_extmem_write_done_req_m_c)
			? ((encode_extmem_write_done_req(
				state, (&(*input).extmem_req_msg_extmem_write_done_req_m))))
			: (((*input).extmem_req_msg_choice ==
				extmem_req_msg_extmem_erase_done_req_m_c)
			? ((encode_extmem_erase_done_req(
				state, (&(*input).extmem_req_msg_extmem_erase_done_req_m))))
			: (((*input).extmem_req_msg_choice ==
				extmem_req_msg_extmem_get_capabilities_req_m_c)
			? ((encode_extmem_get_capabilities_req(
				state, (&(*input).extmem_req_msg_extmem_get_capabilities_req_m))))
			: false))))))) ||
		 (zcbor_list_map_end_force_encode(state), false)) &&
		zcbor_list_end_encode(state, 9))));

	if (!tmp_result) {
		zcbor_trace_file(state);
		zcbor_log("%s error: %s\r\n", __func__, zcbor_error_str(zcbor_peek_error(state)));
	} else {
		zcbor_log("%s success\r\n", __func__);
	}

	return tmp_result;
}

int cbor_encode_extmem_req(uint8_t *payload, size_t payload_len, const struct extmem_req *input,
			   size_t *payload_len_out)
{
	zcbor_state_t states[4];

	return zcbor_entry_function(payload, payload_len, (void *)input, payload_len_out, states,
				    (zcbor_decoder_t *)encode_extmem_req,
				    sizeof(states) / sizeof(zcbor_state_t), 1);
}

int cbor_encode_extmem_rsp(uint8_t *payload, size_t payload_len, const struct extmem_rsp *input,
			   size_t *payload_len_out)
{
	zcbor_state_t states[4];

	return zcbor_entry_function(payload, payload_len, (void *)input, payload_len_out, states,
				    (zcbor_decoder_t *)encode_extmem_rsp,
				    sizeof(states) / sizeof(zcbor_state_t), 1);
}

int cbor_encode_extmem_nfy(uint8_t *payload, size_t payload_len, const struct extmem_nfy *input,
			   size_t *payload_len_out)
{
	zcbor_state_t states[4];

	return zcbor_entry_function(payload, payload_len, (void *)input, payload_len_out, states,
				    (zcbor_decoder_t *)encode_extmem_nfy,
				    sizeof(states) / sizeof(zcbor_state_t), 1);
}
