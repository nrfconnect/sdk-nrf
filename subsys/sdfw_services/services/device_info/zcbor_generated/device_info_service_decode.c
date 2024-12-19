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
#include "zcbor_decode.h"
#include "device_info_service_decode.h"
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

static bool decode_operation(zcbor_state_t *state, struct operation_r *result);
static bool decode_stat(zcbor_state_t *state, struct stat_r *result);
static bool decode_device_info(zcbor_state_t *state, struct device_info *result);
static bool decode_read_req(zcbor_state_t *state, struct read_req *result);
static bool decode_read_resp(zcbor_state_t *state, struct read_resp *result);
static bool decode_device_info_req(zcbor_state_t *state, struct device_info_req *result);
static bool decode_device_info_resp(zcbor_state_t *state, struct device_info_resp *result);


static bool decode_operation(
		zcbor_state_t *state, struct operation_r *result)
{
	zcbor_log("%s\r\n", __func__);

	bool res = (((((zcbor_uint_decode(state, &(*result).operation_choice, sizeof((*result).operation_choice)))) && ((((((*result).operation_choice == operation_READ_DEVICE_INFO_c) && ((1)))
	|| (((*result).operation_choice == operation_UNSUPPORTED_c) && ((1)))) || (zcbor_error(state, ZCBOR_ERR_WRONG_VALUE), false))))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_stat(
		zcbor_state_t *state, struct stat_r *result)
{
	zcbor_log("%s\r\n", __func__);

	bool res = (((((zcbor_uint_decode(state, &(*result).stat_choice, sizeof((*result).stat_choice)))) && ((((((*result).stat_choice == stat_SUCCESS_c) && ((1)))
	|| (((*result).stat_choice == stat_INTERNAL_ERROR_c) && ((1)))
	|| (((*result).stat_choice == stat_UNPROGRAMMED_c) && ((1)))) || (zcbor_error(state, ZCBOR_ERR_WRONG_VALUE), false))))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_device_info(
		zcbor_state_t *state, struct device_info *result)
{
	zcbor_log("%s\r\n", __func__);

	bool res = (((zcbor_list_start_decode(state) && ((((zcbor_bstr_decode(state, (&(*result).device_info_uuid)))
	&& ((((*result).device_info_uuid.len == 16)) || (zcbor_error(state, ZCBOR_ERR_WRONG_RANGE), false)))
	&& ((zcbor_uint32_decode(state, (&(*result).device_info_type))))
	&& ((zcbor_bstr_decode(state, (&(*result).device_info_testimprint)))
	&& ((((*result).device_info_testimprint.len == 32)) || (zcbor_error(state, ZCBOR_ERR_WRONG_RANGE), false)))
	&& ((zcbor_uint32_decode(state, (&(*result).device_info_partno))))
	&& ((zcbor_uint32_decode(state, (&(*result).device_info_hwrevision))))
	&& ((zcbor_uint32_decode(state, (&(*result).device_info_productionrevision))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_list_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_read_req(
		zcbor_state_t *state, struct read_req *result)
{
	zcbor_log("%s\r\n", __func__);

	bool res = (((((zcbor_uint32_expect(state, (1))))
	&& ((decode_operation(state, (&(*result).read_req_action)))))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_read_resp(
		zcbor_state_t *state, struct read_resp *result)
{
	zcbor_log("%s\r\n", __func__);

	bool res = (((((zcbor_uint32_expect(state, (1))))
	&& ((decode_operation(state, (&(*result).read_resp_action))))
	&& ((decode_stat(state, (&(*result).read_resp_status))))
	&& ((decode_device_info(state, (&(*result).read_resp_data)))))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_device_info_req(
		zcbor_state_t *state, struct device_info_req *result)
{
	zcbor_log("%s\r\n", __func__);

	bool res = (((zcbor_list_start_decode(state) && ((((zcbor_uint32_expect(state, (2))))
	&& ((decode_read_req(state, (&(*result).device_info_req_msg))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_list_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_device_info_resp(
		zcbor_state_t *state, struct device_info_resp *result)
{
	zcbor_log("%s\r\n", __func__);

	bool res = (((zcbor_list_start_decode(state) && ((((zcbor_uint32_expect(state, (2))))
	&& ((decode_read_resp(state, (&(*result).device_info_resp_msg))))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_list_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}



int cbor_decode_device_info_req(
		const uint8_t *payload, size_t payload_len,
		struct device_info_req *result,
		size_t *payload_len_out)
{
	zcbor_state_t states[4];

	return zcbor_entry_function(payload, payload_len, (void *)result, payload_len_out, states,
		(zcbor_decoder_t *)decode_device_info_req, sizeof(states) / sizeof(zcbor_state_t), 1);
}



int cbor_decode_device_info_resp(
		const uint8_t *payload, size_t payload_len,
		struct device_info_resp *result,
		size_t *payload_len_out)
{
	zcbor_state_t states[4];

	return zcbor_entry_function(payload, payload_len, (void *)result, payload_len_out, states,
		(zcbor_decoder_t *)decode_device_info_resp, sizeof(states) / sizeof(zcbor_state_t), 1);
}
