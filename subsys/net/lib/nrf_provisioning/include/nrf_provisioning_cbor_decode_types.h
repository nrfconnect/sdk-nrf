/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * Generated using zcbor version 0.8.1
 * https://github.com/NordicSemiconductor/zcbor
 * Generated with a --default-max-qty of CONFIG_NRF_PROVISIONING_CBOR_RECORDS
 */

#ifndef NRF_PROVISIONING_CBOR_DECODE_TYPES_H__
#define NRF_PROVISIONING_CBOR_DECODE_TYPES_H__

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <zcbor_common.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Which value for --default-max-qty this file was created with.
 *
 *  The define is used in the other generated file to do a build-time
 *  compatibility check.
 *
 *  See `zcbor --help` for more information about --default-max-qty
 */
#define DEFAULT_MAX_QTY CONFIG_NRF_PROVISIONING_CBOR_RECORDS

struct at_command {
	struct zcbor_string at_command_set_command;
	struct zcbor_string at_command_parameters;
	uint32_t at_command_ignore_cme_errors_uint[6];
	size_t at_command_ignore_cme_errors_uint_count;
};

struct properties_tstrtstr {
	struct zcbor_string config_properties_tstrtstr_key;
	struct zcbor_string properties_tstrtstr;
};

struct config {
	struct properties_tstrtstr properties_tstrtstr[100];
	size_t properties_tstrtstr_count;
};

struct command {
	struct zcbor_string command_correlation_m;
	union {
		struct at_command command_union_at_command_m;
		struct config command_union_config_m;
	};
	enum {
		command_union_at_command_m_c,
		command_union_config_m_c,
		command_union_finished_m_c,
	} command_union_choice;
};

struct commands {
	struct command commands_command_m[CONFIG_NRF_PROVISIONING_CBOR_RECORDS];
	size_t commands_command_m_count;
};

#ifdef __cplusplus
}
#endif

#endif /* NRF_PROVISIONING_CBOR_DECODE_TYPES_H__ */
