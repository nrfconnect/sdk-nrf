/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * Generated using zcbor version 0.7.0
 * https://github.com/NordicSemiconductor/zcbor
 * Generated with a --default-max-qty of 10
 */

#ifndef AGNSS_ENCODE_TYPES_H__
#define AGNSS_ENCODE_TYPES_H__

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

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
#define DEFAULT_MAX_QTY 10

struct agnss_req_types_ {
	int32_t _agnss_req_types_int[14];
	size_t _agnss_req_types_int_count;
};

struct agnss_req_filtered {
	bool _agnss_req_filtered;
};

struct agnss_req_mask {
	uint32_t _agnss_req_mask;
};

struct type_ {
	enum {
		_type__rtAssistance = 10,
		_type__custom = 11,
	} _type_choice;
};

struct agnss_req_requestType {
	struct type_ _agnss_req_requestType;
};

struct agnss_req_rsrp {
	int32_t _agnss_req_rsrp;
};

struct agnss_req {
	struct agnss_req_types_ _agnss_req_types;
	bool _agnss_req_types_present;
	uint32_t _agnss_req_eci;
	struct agnss_req_filtered _agnss_req_filtered;
	bool _agnss_req_filtered_present;
	struct agnss_req_mask _agnss_req_mask;
	bool _agnss_req_mask_present;
	uint32_t _agnss_req_mcc;
	uint32_t _agnss_req_mnc;
	struct agnss_req_requestType _agnss_req_requestType;
	bool _agnss_req_requestType_present;
	struct agnss_req_rsrp _agnss_req_rsrp;
	bool _agnss_req_rsrp_present;
	uint32_t _agnss_req_tac;
};

#ifdef __cplusplus
}
#endif

#endif /* AGNSS_ENCODE_TYPES_H__ */
