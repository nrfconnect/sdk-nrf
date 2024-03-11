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
#include "reset_evt_service_decode.h"
#include "zcbor_print.h"

#if DEFAULT_MAX_QTY != 3
#error "The type file was generated with a different default_max_qty than this file"
#endif

static bool decode_reset_evt_notif(zcbor_state_t *state, struct reset_evt_notif *result);
static bool decode_reset_evt_sub_rsp(zcbor_state_t *state, int32_t *result);
static bool decode_reset_evt_sub_req(zcbor_state_t *state, bool *result);

static bool decode_reset_evt_notif(zcbor_state_t *state, struct reset_evt_notif *result)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result =
		(((zcbor_list_start_decode(state) &&
		   ((((zcbor_uint32_decode(state, (&(*result).reset_evt_notif_domains)))) &&
		     ((zcbor_uint32_decode(state, (&(*result).reset_evt_notif_delay_ms))))) ||
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

static bool decode_reset_evt_sub_rsp(zcbor_state_t *state, int32_t *result)
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

static bool decode_reset_evt_sub_req(zcbor_state_t *state, bool *result)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result = (((zcbor_list_start_decode(state) &&
			     ((((zcbor_bool_decode(state, (&(*result)))))) ||
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

int cbor_decode_reset_evt_sub_req(const uint8_t *payload, size_t payload_len, bool *result,
				  size_t *payload_len_out)
{
	zcbor_state_t states[3];

	return zcbor_entry_function(payload, payload_len, (void *)result, payload_len_out, states,
				    (zcbor_decoder_t *)decode_reset_evt_sub_req,
				    sizeof(states) / sizeof(zcbor_state_t), 1);
}

int cbor_decode_reset_evt_sub_rsp(const uint8_t *payload, size_t payload_len, int32_t *result,
				  size_t *payload_len_out)
{
	zcbor_state_t states[3];

	return zcbor_entry_function(payload, payload_len, (void *)result, payload_len_out, states,
				    (zcbor_decoder_t *)decode_reset_evt_sub_rsp,
				    sizeof(states) / sizeof(zcbor_state_t), 1);
}

int cbor_decode_reset_evt_notif(const uint8_t *payload, size_t payload_len,
				struct reset_evt_notif *result, size_t *payload_len_out)
{
	zcbor_state_t states[3];

	return zcbor_entry_function(payload, payload_len, (void *)result, payload_len_out, states,
				    (zcbor_decoder_t *)decode_reset_evt_notif,
				    sizeof(states) / sizeof(zcbor_state_t), 1);
}
