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
#include "sdfw_update_service_decode.h"
#include "zcbor_print.h"

#if DEFAULT_MAX_QTY != 3
#error "The type file was generated with a different default_max_qty than this file"
#endif

static bool decode_sdfw_update_rsp(zcbor_state_t *state, int32_t *result);
static bool decode_sdfw_update_req(zcbor_state_t *state, struct sdfw_update_req *result);

static bool decode_sdfw_update_rsp(zcbor_state_t *state, int32_t *result)
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

static bool decode_sdfw_update_req(zcbor_state_t *state, struct sdfw_update_req *result)
{
	zcbor_log("%s\r\n", __func__);

	bool tmp_result = (((
		zcbor_list_start_decode(state) &&
		((((zcbor_uint32_decode(state, (&(*result).sdfw_update_req_tbs_addr)))) &&
		  ((zcbor_uint32_decode(state, (&(*result).sdfw_update_req_dl_max)))) &&
		  ((zcbor_uint32_decode(state, (&(*result).sdfw_update_req_dl_addr_fw)))) &&
		  ((zcbor_uint32_decode(state, (&(*result).sdfw_update_req_dl_addr_pk)))) &&
		  ((zcbor_uint32_decode(state, (&(*result).sdfw_update_req_dl_addr_signature))))) ||
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

int cbor_decode_sdfw_update_req(const uint8_t *payload, size_t payload_len,
				struct sdfw_update_req *result, size_t *payload_len_out)
{
	zcbor_state_t states[3];

	return zcbor_entry_function(payload, payload_len, (void *)result, payload_len_out, states,
				    (zcbor_decoder_t *)decode_sdfw_update_req,
				    sizeof(states) / sizeof(zcbor_state_t), 1);
}

int cbor_decode_sdfw_update_rsp(const uint8_t *payload, size_t payload_len, int32_t *result,
				size_t *payload_len_out)
{
	zcbor_state_t states[3];

	return zcbor_entry_function(payload, payload_len, (void *)result, payload_len_out, states,
				    (zcbor_decoder_t *)decode_sdfw_update_rsp,
				    sizeof(states) / sizeof(zcbor_state_t), 1);
}
