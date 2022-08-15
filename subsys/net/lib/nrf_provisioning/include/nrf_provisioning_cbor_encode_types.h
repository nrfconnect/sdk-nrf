/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
/*
 * Generated using zcbor version 0.6.0
 * https://github.com/NordicSemiconductor/zcbor
 * Generated with a --default-max-qty of 1234567890
 */

#ifndef NRF_PROVISIONING_CBOR_ENCODE_TYPES_H__
#define NRF_PROVISIONING_CBOR_ENCODE_TYPES_H__

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "zcbor_encode.h"

/** Which value for --default-max-qty this file was created with.
 *
 *  The define is used in the other generated file to do a build-time
 *  compatibility check.
 *
 *  See `zcbor --help` for more information about --default-max-qty
 */
#define DEFAULT_MAX_QTY CONFIG_NRF_PROVISIONING_CBOR_RECORDS

struct error_response {
	uint32_t _error_response_cme_error;
	struct zcbor_string _error_response_message;
};

struct at_response {
	struct zcbor_string _at_response_message;
};

struct response {
	struct zcbor_string _response__correlation;
	union {
		struct error_response _response_union__error_response;
		struct at_response _response_union__at_response;
	};
	enum {
		_response_union__error_response,
		_response_union__at_response,
		_response_union__config_ack,
		_response_union__finished_ack,
	} _response_union_choice;
};

struct responses {
	struct response _responses__response[CONFIG_NRF_PROVISIONING_CBOR_RECORDS];
	uint_fast32_t _responses__response_count;
};

#endif /* NRF_PROVISIONING_CBOR_ENCODE_TYPES_H__ */
