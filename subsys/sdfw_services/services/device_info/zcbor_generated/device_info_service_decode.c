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

static bool decode_entity(zcbor_state_t *state, struct entity_r *result);
static bool decode_stat(zcbor_state_t *state, struct stat_r *result);
static bool decode_read_req(zcbor_state_t *state, struct read_req *result);
static bool decode_write_req(zcbor_state_t *state, struct write_req *result);
static bool decode_read_resp(zcbor_state_t *state, struct read_resp *result);
static bool decode_write_resp(zcbor_state_t *state, struct write_resp *result);
static bool decode_device_info_req(zcbor_state_t *state, struct device_info_req *result);
static bool decode_device_info_resp(zcbor_state_t *state, struct device_info_resp *result);


static bool decode_entity(
		zcbor_state_t *state, struct entity_r *result)
{
	zcbor_log("%s\r\n", __func__);

	bool res = (((((zcbor_uint_decode(state, &(*result).entity_choice, sizeof((*result).entity_choice)))) && ((((((*result).entity_choice == entity_UUID_c) && ((1)))
	|| (((*result).entity_choice == entity_TYPE_c) && ((1)))
	|| (((*result).entity_choice == entity_TESTIMPRINT_c) && ((1)))
	|| (((*result).entity_choice == entity_PARTNO_c) && ((1)))
	|| (((*result).entity_choice == entity_HWREVISION_c) && ((1)))
	|| (((*result).entity_choice == entity_PRODUCTIONREVISION_c) && ((1)))) || (zcbor_error(state, ZCBOR_ERR_WRONG_VALUE), false))))));

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

static bool decode_read_req(
		zcbor_state_t *state, struct read_req *result)
{
	zcbor_log("%s\r\n", __func__);

	bool res = (((((zcbor_uint32_expect(state, (1))))
	&& ((decode_entity(state, (&(*result).read_req_target)))))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_write_req(
		zcbor_state_t *state, struct write_req *result)
{
	zcbor_log("%s\r\n", __func__);

	bool res = (((((zcbor_uint32_expect(state, (2))))
	&& ((decode_entity(state, (&(*result).write_req_target))))
	&& ((zcbor_list_start_decode(state) && ((zcbor_multi_decode(1, 8, &(*result).write_req_data_uint_count, (zcbor_decoder_t *)zcbor_uint32_decode, state, (*&(*result).write_req_data_uint), sizeof(uint32_t))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_list_end_decode(state))))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		zcbor_uint32_decode(state, (*&(*result).write_req_data_uint));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_read_resp(
		zcbor_state_t *state, struct read_resp *result)
{
	zcbor_log("%s\r\n", __func__);

	bool res = (((((zcbor_uint32_expect(state, (1))))
	&& ((decode_entity(state, (&(*result).read_resp_target))))
	&& ((decode_stat(state, (&(*result).read_resp_status))))
	&& ((zcbor_list_start_decode(state) && ((zcbor_multi_decode(1, 8, &(*result).read_resp_data_uint_count, (zcbor_decoder_t *)zcbor_uint32_decode, state, (*&(*result).read_resp_data_uint), sizeof(uint32_t))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_list_end_decode(state))))));

	if (false) {
		/* For testing that the types of the arguments are correct.
		 * A compiler error here means a bug in zcbor.
		 */
		zcbor_uint32_decode(state, (*&(*result).read_resp_data_uint));
	}

	log_result(state, res, __func__);
	return res;
}

static bool decode_write_resp(
		zcbor_state_t *state, struct write_resp *result)
{
	zcbor_log("%s\r\n", __func__);

	bool res = (((((zcbor_uint32_expect(state, (2))))
	&& ((decode_entity(state, (&(*result).write_resp_target))))
	&& ((decode_stat(state, (&(*result).write_resp_status)))))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_device_info_req(
		zcbor_state_t *state, struct device_info_req *result)
{
	zcbor_log("%s\r\n", __func__);
	bool int_res;

	bool res = (((zcbor_list_start_decode(state) && ((((zcbor_uint32_expect(state, (3))))
	&& ((zcbor_union_start_code(state) && (int_res = ((((decode_read_req(state, (&(*result).req_msg_read_req_m)))) && (((*result).req_msg_choice = req_msg_read_req_m_c), true))
	|| (zcbor_union_elem_code(state) && (((decode_write_req(state, (&(*result).req_msg_write_req_m)))) && (((*result).req_msg_choice = req_msg_write_req_m_c), true)))), zcbor_union_end_code(state), int_res)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_list_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_device_info_resp(
		zcbor_state_t *state, struct device_info_resp *result)
{
	zcbor_log("%s\r\n", __func__);
	bool int_res;

	bool res = (((zcbor_list_start_decode(state) && ((((zcbor_uint32_expect(state, (3))))
	&& ((zcbor_union_start_code(state) && (int_res = ((((decode_read_resp(state, (&(*result).resp_msg_read_resp_m)))) && (((*result).resp_msg_choice = resp_msg_read_resp_m_c), true))
	|| (zcbor_union_elem_code(state) && (((decode_write_resp(state, (&(*result).resp_msg_write_resp_m)))) && (((*result).resp_msg_choice = resp_msg_write_resp_m_c), true)))), zcbor_union_end_code(state), int_res)))) || (zcbor_list_map_end_force_decode(state), false)) && zcbor_list_end_decode(state))));

	log_result(state, res, __func__);
	return res;
}



int cbor_decode_device_info_req(
		const uint8_t *payload, size_t payload_len,
		struct device_info_req *result,
		size_t *payload_len_out)
{
	zcbor_state_t states[5];

	return zcbor_entry_function(payload, payload_len, (void *)result, payload_len_out, states,
		(zcbor_decoder_t *)decode_device_info_req, sizeof(states) / sizeof(zcbor_state_t), 1);
}



int cbor_decode_device_info_resp(
		const uint8_t *payload, size_t payload_len,
		struct device_info_resp *result,
		size_t *payload_len_out)
{
	zcbor_state_t states[5];

	return zcbor_entry_function(payload, payload_len, (void *)result, payload_len_out, states,
		(zcbor_decoder_t *)decode_device_info_resp, sizeof(states) / sizeof(zcbor_state_t), 1);
}
