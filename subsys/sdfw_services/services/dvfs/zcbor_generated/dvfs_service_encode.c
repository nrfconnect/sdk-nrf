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
#include "zcbor_encode.h"
#include "dvfs_service_encode.h"
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

static bool encode_dvfs_oppoint(zcbor_state_t *state, const struct dvfs_oppoint_r *input);
static bool encode_stat(zcbor_state_t *state, const struct stat_r *input);
static bool encode_dvfs_oppoint_rsp(zcbor_state_t *state, const struct dvfs_oppoint_rsp *input);
static bool encode_dvfs_oppoint_req(zcbor_state_t *state, const struct dvfs_oppoint_req *input);

static bool encode_dvfs_oppoint(zcbor_state_t *state, const struct dvfs_oppoint_r *input)
{
	zcbor_log("%s\r\n", __func__);

	bool res = ((
		(((*input).dvfs_oppoint_choice == dvfs_oppoint_DVFS_FREQ_HIGH_c) ?
			 ((zcbor_uint32_put(state, (0)))) :
			 (((*input).dvfs_oppoint_choice == dvfs_oppoint_DVFS_FREQ_MEDLOW_c) ?
				  ((zcbor_uint32_put(state, (1)))) :
				  (((*input).dvfs_oppoint_choice == dvfs_oppoint_DVFS_FREQ_LOW_c) ?
					   ((zcbor_uint32_put(state, (2)))) :
					   false)))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_stat(zcbor_state_t *state, const struct stat_r *input)
{
	zcbor_log("%s\r\n", __func__);

	bool res = (((((*input).stat_choice == stat_SUCCESS_c) ?
			      ((zcbor_uint32_put(state, (0)))) :
			      (((*input).stat_choice == stat_INTERNAL_ERROR_c) ?
				       ((zcbor_uint32_put(state, (16781313)))) :
				       false))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_dvfs_oppoint_rsp(zcbor_state_t *state, const struct dvfs_oppoint_rsp *input)
{
	zcbor_log("%s\r\n", __func__);

	bool res = (((((zcbor_uint32_put(state, (1)))) &&
		      ((encode_dvfs_oppoint(state, (&(*input).dvfs_oppoint_rsp_data)))) &&
		      ((encode_stat(state, (&(*input).dvfs_oppoint_rsp_status)))))));

	log_result(state, res, __func__);
	return res;
}

static bool encode_dvfs_oppoint_req(zcbor_state_t *state, const struct dvfs_oppoint_req *input)
{
	zcbor_log("%s\r\n", __func__);

	bool res = (((((zcbor_uint32_put(state, (1)))) &&
		      ((encode_dvfs_oppoint(state, (&(*input).dvfs_oppoint_req_data)))))));

	log_result(state, res, __func__);
	return res;
}

int cbor_encode_dvfs_oppoint_req(uint8_t *payload, size_t payload_len,
				 const struct dvfs_oppoint_req *input, size_t *payload_len_out)
{
	zcbor_state_t states[3];

	return zcbor_entry_function(payload, payload_len, (void *)input, payload_len_out, states,
				    (zcbor_decoder_t *)encode_dvfs_oppoint_req,
				    sizeof(states) / sizeof(zcbor_state_t), 2);
}

int cbor_encode_dvfs_oppoint_rsp(uint8_t *payload, size_t payload_len,
				 const struct dvfs_oppoint_rsp *input, size_t *payload_len_out)
{
	zcbor_state_t states[3];

	return zcbor_entry_function(payload, payload_len, (void *)input, payload_len_out, states,
				    (zcbor_decoder_t *)encode_dvfs_oppoint_rsp,
				    sizeof(states) / sizeof(zcbor_state_t), 3);
}
