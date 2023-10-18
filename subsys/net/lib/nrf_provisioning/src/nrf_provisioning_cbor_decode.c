/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
/*
 * Generated using zcbor version 0.7.0
 * https://github.com/NordicSemiconductor/zcbor
 * Generated with a --default-max-qty of CONFIG_NRF_PROVISIONING_CBOR_RECORDS
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "zcbor_decode.h"
#include "nrf_provisioning_cbor_decode.h"

#if DEFAULT_MAX_QTY != CONFIG_NRF_PROVISIONING_CBOR_RECORDS
#error "The type file was generated with a different default_max_qty than this file"
#endif

static bool decode_at_command(zcbor_state_t *state, struct at_command *result);
static bool decode_repeated_properties_tstrunion(zcbor_state_t *state,
						 struct properties_tstrunion_ *result);
static bool decode_config(zcbor_state_t *state, struct config *result);
static bool decode_command(zcbor_state_t *state, struct command *result);
static bool decode_commands(zcbor_state_t *state, struct commands *result);

static bool decode_at_command(zcbor_state_t *state, struct at_command *result)
{
	zcbor_print("%s\r\n", __func__);

	bool tmp_result =
		(((((zcbor_uint32_expect(state, (0)))) &&
		   ((zcbor_tstr_decode(state, (&(*result)._at_command_set_command)))) &&
		   ((zcbor_tstr_decode(state, (&(*result)._at_command_parameters)))) &&
		   ((zcbor_list_start_decode(state) &&
		     ((zcbor_multi_decode(0, 6, &(*result)._at_command_ignore_cme_errors_uint_count,
					  (zcbor_decoder_t *)zcbor_uint32_decode, state,
					  (&(*result)._at_command_ignore_cme_errors_uint),
					  sizeof(uint32_t))) ||
		      (zcbor_list_map_end_force_decode(state), false)) &&
		     zcbor_list_end_decode(state))))));

	if (!tmp_result) {
		zcbor_trace();
	}

	return tmp_result;
}

static bool decode_repeated_properties_tstrunion(zcbor_state_t *state,
						 struct properties_tstrunion_ *result)
{
	zcbor_print("%s\r\n", __func__);
	bool int_res;

	bool tmp_result = ((
		((zcbor_tstr_decode(state, (&(*result)._config_properties_tstrunion_key)))) &&
		(zcbor_union_start_code(state) &&
		 (int_res =
			  ((((zcbor_tstr_decode(state, (&(*result)._properties_tstrunion_tstr)))) &&
			    (((*result)._properties_tstrunion_choice = _properties_tstrunion_tstr),
			     true)) ||
			   (((zcbor_bool_decode(state, (&(*result)._properties_tstrunion_bool)))) &&
			    (((*result)._properties_tstrunion_choice = _properties_tstrunion_bool),
			     true)) ||
			   (((zcbor_int32_decode(state, (&(*result)._properties_tstrunion_int)))) &&
			    (((*result)._properties_tstrunion_choice = _properties_tstrunion_int),
			     true)) ||
			   (((zcbor_bstr_decode(state, (&(*result)._properties_tstrunion_bstr)))) &&
			    (((*result)._properties_tstrunion_choice = _properties_tstrunion_bstr),
			     true))),
		  zcbor_union_end_code(state), int_res))));

	if (!tmp_result) {
		zcbor_trace();
	}

	return tmp_result;
}

static bool decode_config(zcbor_state_t *state, struct config *result)
{
	zcbor_print("%s\r\n", __func__);

	bool tmp_result =
		(((((zcbor_uint32_expect(state, (1)))) &&
		   ((zcbor_map_start_decode(state) &&
		     ((zcbor_multi_decode(0, 100, &(*result)._properties_tstrunion_count,
					  (zcbor_decoder_t *)decode_repeated_properties_tstrunion,
					  state, (&(*result)._properties_tstrunion),
					  sizeof(struct properties_tstrunion_))) ||
		      (zcbor_list_map_end_force_decode(state), false)) &&
		     zcbor_map_end_decode(state))))));

	if (!tmp_result) {
		zcbor_trace();
	}

	return tmp_result;
}

static bool decode_command(zcbor_state_t *state, struct command *result)
{
	zcbor_print("%s\r\n", __func__);
	bool int_res;

	bool tmp_result = ((
		(zcbor_list_start_decode(state) &&
		 ((((zcbor_tstr_decode(state, (&(*result)._command__correlation)))) &&
		   ((zcbor_union_start_code(state) &&
		     (int_res = ((((decode_at_command(state,
						      (&(*result)._command_union__at_command)))) &&
				  (((*result)._command_union_choice = _command_union__at_command),
				   true)) ||
				 (zcbor_union_elem_code(state) &&
				  (((decode_config(state, (&(*result)._command_union__config)))) &&
				   (((*result)._command_union_choice = _command_union__config),
				    true))) ||
				 (((zcbor_uint32_expect_union(state, (2)))) &&
				  (((*result)._command_union_choice = _command_union__finished),
				   true))),
		      zcbor_union_end_code(state), int_res)))) ||
		  (zcbor_list_map_end_force_decode(state), false)) &&
		 zcbor_list_end_decode(state))));

	if (!tmp_result) {
		zcbor_trace();
	}

	return tmp_result;
}

static bool decode_commands(zcbor_state_t *state, struct commands *result)
{
	zcbor_print("%s\r\n", __func__);

	bool tmp_result =
		(((zcbor_list_start_decode(state) &&
		   ((zcbor_multi_decode(1, CONFIG_NRF_PROVISIONING_CBOR_RECORDS,
					&(*result)._commands__command_count,
					(zcbor_decoder_t *)decode_command, state,
					(&(*result)._commands__command), sizeof(struct command))) ||
		    (zcbor_list_map_end_force_decode(state), false)) &&
		   zcbor_list_end_decode(state))));

	if (!tmp_result) {
		zcbor_trace();
	}

	return tmp_result;
}

int cbor_decode_commands(const uint8_t *payload, size_t payload_len, struct commands *result,
			 size_t *payload_len_out)
{
	zcbor_state_t states[7];

	zcbor_new_state(states, sizeof(states) / sizeof(zcbor_state_t), payload, payload_len, 1);

	bool ret = decode_commands(states, result);

	if (ret && (payload_len_out != NULL)) {
		*payload_len_out = MIN(payload_len, (size_t)states[0].payload - (size_t)payload);
	}

	if (!ret) {
		int err = zcbor_pop_error(states);

		zcbor_print("Return error: %d\r\n", err);
		return (err == ZCBOR_SUCCESS) ? ZCBOR_ERR_UNKNOWN : err;
	}
	return ZCBOR_SUCCESS;
}
