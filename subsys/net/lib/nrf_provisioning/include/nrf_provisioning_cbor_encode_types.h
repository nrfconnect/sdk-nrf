/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * Generated using zcbor version 0.8.1
 * https://github.com/NordicSemiconductor/zcbor
 * Generated with a --default-max-qty of 1234567890
 */

#ifndef NRF_PROVISIONING_CBOR_ENCODE_TYPES_H__
#define NRF_PROVISIONING_CBOR_ENCODE_TYPES_H__

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

struct error_response {
	uint32_t error_response_cme_error;
	struct zcbor_string error_response_message;
};

struct at_response {
	struct zcbor_string at_response_message;
};

struct response {
	struct zcbor_string response_correlation_m;
	union {
		struct error_response response_union_error_response_m;
		struct at_response response_union_at_response_m;
	};
	enum {
		response_union_error_response_m_c,
		response_union_at_response_m_c,
		response_union_config_ack_m_c,
		response_union_finished_ack_m_c,
	} response_union_choice;
};

struct responses {
	struct response responses_response_m[CONFIG_NRF_PROVISIONING_CBOR_RECORDS];
	size_t responses_response_m_count;
};

#ifdef __cplusplus
}
#endif

#endif /* NRF_PROVISIONING_CBOR_ENCODE_TYPES_H__ */
