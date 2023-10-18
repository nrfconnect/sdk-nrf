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
	struct zcbor_string _at_command_set_command;
	struct zcbor_string _at_command_parameters;
	uint32_t _at_command_ignore_cme_errors_uint[6];
	size_t _at_command_ignore_cme_errors_uint_count;
};

struct properties_tstrunion_ {
	struct zcbor_string _config_properties_tstrunion_key;
	union {
		struct zcbor_string _properties_tstrunion_tstr;
		bool _properties_tstrunion_bool;
		int32_t _properties_tstrunion_int;
		struct zcbor_string _properties_tstrunion_bstr;
	};
	enum {
		_properties_tstrunion_tstr,
		_properties_tstrunion_bool,
		_properties_tstrunion_int,
		_properties_tstrunion_bstr,
	} _properties_tstrunion_choice;
};

struct config {
	struct properties_tstrunion_ _properties_tstrunion[100];
	size_t _properties_tstrunion_count;
};

struct command {
	struct zcbor_string _command__correlation;
	union {
		struct at_command _command_union__at_command;
		struct config _command_union__config;
	};
	enum {
		_command_union__at_command,
		_command_union__config,
		_command_union__finished,
	} _command_union_choice;
};

struct commands {
	struct command _commands__command[CONFIG_NRF_PROVISIONING_CBOR_RECORDS];
	size_t _commands__command_count;
};

#ifdef __cplusplus
}
#endif

#endif /* NRF_PROVISIONING_CBOR_DECODE_TYPES_H__ */
