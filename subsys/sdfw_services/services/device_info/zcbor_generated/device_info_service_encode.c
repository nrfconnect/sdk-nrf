/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 * Generated using zcbor version 0.9.0
 * https://github.com/NordicSemiconductor/zcbor
 * Generated with a --default-max-qty of 3
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "zcbor_encode.h"
#include "device_info_service_encode.h"
#include "zcbor_print.h"

#if DEFAULT_MAX_QTY != 3
#error "The type file was generated with a different default_max_qty than this file"
#endif

#define log_result(state, result, func) do { \
	if (!result) { \
		zcbor_trace_file(state); \
		zcbor_log("%s error: %s\r\n", func, zcbor_error_str(zcbor_peek_error(state))); \
	} else { \
		zcbor_log("%s success\r\n", func); \
	} \
} while(0)

static bool encode_operation(zcbor_state_t *state, const struct operation_r *input);
static bool encode_stat(zcbor_state_t *state, const struct stat_r *input);
static bool encode_device_info(zcbor_state_t *state, const struct device_info *input);
static bool encode_read_req(zcbor_state_t *state, const struct read_req *input);
static bool encode_read_resp(zcbor_state_t *state, const struct read_resp *input);
static bool encode_device_info_req(zcbor_state_t *state, const struct device_info_req *input);
static bool encode_device_info_resp(zcbor_state_t *state, const struct device_info_resp *input);

static bool encode_operation(
		zcbor_state_t *state, const struct operation_r *input)
{
	zcbor_log("%s\r\n", __func__);

	bool res = (((((*input).operation_choice == operation_READ_DEVICE_INFO_c) ? ((zcbor_uint32_put(state, (0))))
	: (((*input).operation_choice == operation_UNSUPPORTED_c) ? ((zcbor_uint32_put(state, (1))))
	: false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_stat(
		zcbor_state_t *state, const struct stat_r *input)
{
	zcbor_log("%s\r\n", __func__);

	bool res = (((((*input).stat_choice == stat_SUCCESS_c) ? ((zcbor_uint32_put(state, (0))))
	: (((*input).stat_choice == stat_INTERNAL_ERROR_c) ? ((zcbor_uint32_put(state, (16781313))))
	: (((*input).stat_choice == stat_UNPROGRAMMED_c) ? ((zcbor_uint32_put(state, (16781314))))
	: false)))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_device_info(
		zcbor_state_t *state, const struct device_info *input)
{
	zcbor_log("%s\r\n", __func__);

	bool res = (((zcbor_list_start_encode(state, 6) && (((((((*input).device_info_uuid.len == 16)) || (zcbor_error(state, ZCBOR_ERR_WRONG_RANGE), false))
	&& (zcbor_bstr_encode(state, (&(*input).device_info_uuid))))
	&& ((zcbor_uint32_encode(state, (&(*input).device_info_type))))
	&& (((((*input).device_info_testimprint.len == 32)) || (zcbor_error(state, ZCBOR_ERR_WRONG_RANGE), false))
	&& (zcbor_bstr_encode(state, (&(*input).device_info_testimprint))))
	&& ((zcbor_uint32_encode(state, (&(*input).device_info_partno))))
	&& ((zcbor_uint32_encode(state, (&(*input).device_info_hwrevision))))
	&& ((zcbor_uint32_encode(state, (&(*input).device_info_productionrevision))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_list_end_encode(state, 6))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_read_req(
		zcbor_state_t *state, const struct read_req *input)
{
	zcbor_log("%s\r\n", __func__);

	bool res = (((((zcbor_uint32_put(state, (1))))
	&& ((encode_operation(state, (&(*input).read_req_action)))))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_read_resp(
		zcbor_state_t *state, const struct read_resp *input)
{
	zcbor_log("%s\r\n", __func__);

	bool res = (((((zcbor_uint32_put(state, (1))))
	&& ((encode_operation(state, (&(*input).read_resp_action))))
	&& ((encode_stat(state, (&(*input).read_resp_status))))
	&& ((encode_device_info(state, (&(*input).read_resp_data)))))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_device_info_req(
		zcbor_state_t *state, const struct device_info_req *input)
{
	zcbor_log("%s\r\n", __func__);

	bool res = (((zcbor_list_start_encode(state, 3) && ((((zcbor_uint32_put(state, (2))))
	&& ((encode_read_req(state, (&(*input).device_info_req_msg))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_list_end_encode(state, 3))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_device_info_resp(
		zcbor_state_t *state, const struct device_info_resp *input)
{
	zcbor_log("%s\r\n", __func__);

	bool res = (((zcbor_list_start_encode(state, 5) && ((((zcbor_uint32_put(state, (2))))
	&& ((encode_read_resp(state, (&(*input).device_info_resp_msg))))) || (zcbor_list_map_end_force_encode(state), false)) && zcbor_list_end_encode(state, 5))));

	log_result(state, res, __func__);
	return res;
}



int cbor_encode_device_info_req(
		uint8_t *payload, size_t payload_len,
		const struct device_info_req *input,
		size_t *payload_len_out)
{
	zcbor_state_t states[4];

	return zcbor_entry_function(payload, payload_len, (void *)input, payload_len_out, states,
		(zcbor_decoder_t *)encode_device_info_req, sizeof(states) / sizeof(zcbor_state_t), 1);
}



int cbor_encode_device_info_resp(
		uint8_t *payload, size_t payload_len,
		const struct device_info_resp *input,
		size_t *payload_len_out)
{
	zcbor_state_t states[4];

	return zcbor_entry_function(payload, payload_len, (void *)input, payload_len_out, states,
		(zcbor_decoder_t *)encode_device_info_resp, sizeof(states) / sizeof(zcbor_state_t), 1);
}
