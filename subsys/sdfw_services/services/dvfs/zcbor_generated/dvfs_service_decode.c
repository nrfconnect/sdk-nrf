/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 * Generated using zcbor version 0.9.1
 * https://github.com/NordicSemiconductor/zcbor
 * Generated with a --default-max-qty of 3
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "zcbor_decode.h"
#include "dvfs_service_decode.h"
#include "zcbor_print.h"

#if DEFAULT_MAX_QTY != 3
#error "The type file was generated with a different default_max_qty than this file"
#endif

#define log_result(state, result, func)                                                            \
	do {                                                                                       \
		if (!result) {                                                                     \
			zcbor_trace_file(state);                                                   \
			zcbor_log("%s error: %s\r\n", func,                                        \
				  zcbor_error_str(zcbor_peek_error(state)));                       \
		} else {                                                                           \
			zcbor_log("%s success\r\n", func);                                         \
		}                                                                                  \
	} while (0)

static bool decode_dvfs_oppoint(zcbor_state_t *state, struct dvfs_oppoint_r *result);
static bool decode_stat(zcbor_state_t *state, struct stat_r *result);
static bool decode_dvfs_oppoint_rsp(zcbor_state_t *state, struct dvfs_oppoint_rsp *result);
static bool decode_dvfs_oppoint_req(zcbor_state_t *state, struct dvfs_oppoint_req *result);

static bool decode_dvfs_oppoint(zcbor_state_t *state, struct dvfs_oppoint_r *result)
{
	zcbor_log("%s\r\n", __func__);

	bool res = ((
		(((zcbor_uint_decode(state, &(*result).dvfs_oppoint_choice,
				     sizeof((*result).dvfs_oppoint_choice)))) &&
		 ((((((*result).dvfs_oppoint_choice == dvfs_oppoint_DVFS_FREQ_HIGH_c) && ((1))) ||
		    (((*result).dvfs_oppoint_choice == dvfs_oppoint_DVFS_FREQ_MEDLOW_c) && ((1))) ||
		    (((*result).dvfs_oppoint_choice == dvfs_oppoint_DVFS_FREQ_LOW_c) && ((1)))) ||
		   (zcbor_error(state, ZCBOR_ERR_WRONG_VALUE), false))))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_stat(zcbor_state_t *state, struct stat_r *result)
{
	zcbor_log("%s\r\n", __func__);

	bool res = (((((zcbor_uint_decode(state, &(*result).stat_choice,
					  sizeof((*result).stat_choice)))) &&
		      ((((((*result).stat_choice == stat_SUCCESS_c) && ((1))) ||
			 (((*result).stat_choice == stat_INTERNAL_ERROR_c) && ((1)))) ||
			(zcbor_error(state, ZCBOR_ERR_WRONG_VALUE), false))))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_dvfs_oppoint_rsp(zcbor_state_t *state, struct dvfs_oppoint_rsp *result)
{
	zcbor_log("%s\r\n", __func__);

	bool res = (((((zcbor_uint32_expect(state, (1)))) &&
		      ((decode_dvfs_oppoint(state, (&(*result).dvfs_oppoint_rsp_data)))) &&
		      ((decode_stat(state, (&(*result).dvfs_oppoint_rsp_status)))))));

	log_result(state, res, __func__);
	return res;
}

static bool decode_dvfs_oppoint_req(zcbor_state_t *state, struct dvfs_oppoint_req *result)
{
	zcbor_log("%s\r\n", __func__);

	bool res = (((((zcbor_uint32_expect(state, (1)))) &&
		      ((decode_dvfs_oppoint(state, (&(*result).dvfs_oppoint_req_data)))))));

	log_result(state, res, __func__);
	return res;
}

int cbor_decode_dvfs_oppoint_req(const uint8_t *payload, size_t payload_len,
				 struct dvfs_oppoint_req *result, size_t *payload_len_out)
{
	zcbor_state_t states[3];

	return zcbor_entry_function(payload, payload_len, (void *)result, payload_len_out, states,
				    (zcbor_decoder_t *)decode_dvfs_oppoint_req,
				    sizeof(states) / sizeof(zcbor_state_t), 2);
}

int cbor_decode_dvfs_oppoint_rsp(const uint8_t *payload, size_t payload_len,
				 struct dvfs_oppoint_rsp *result, size_t *payload_len_out)
{
	zcbor_state_t states[3];

	return zcbor_entry_function(payload, payload_len, (void *)result, payload_len_out, states,
				    (zcbor_decoder_t *)decode_dvfs_oppoint_rsp,
				    sizeof(states) / sizeof(zcbor_state_t), 3);
}
