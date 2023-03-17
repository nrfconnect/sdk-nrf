/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * Generated using zcbor version 0.7.0
 * https://github.com/NordicSemiconductor/zcbor
 * Generated with a --default-max-qty of 10
 */

#ifndef AGPS_ENCODE_TYPES_H__
#define AGPS_ENCODE_TYPES_H__

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

struct agps_req_types_ {
	int32_t _agps_req_types_int[10];
	size_t _agps_req_types_int_count;
};

struct agps_req_filtered {
	bool _agps_req_filtered;
};

struct agps_req_mask {
	uint32_t _agps_req_mask;
};

struct type_ {
	enum {
		_type__rtAssistance = 10,
		_type__custom = 11,
	} _type_choice;
};

struct agps_req_requestType {
	struct type_ _agps_req_requestType;
};

struct agps_req_rsrp {
	int32_t _agps_req_rsrp;
};

struct agps_req {
	struct agps_req_types_ _agps_req_types;
	bool _agps_req_types_present;
	uint32_t _agps_req_eci;
	struct agps_req_filtered _agps_req_filtered;
	bool _agps_req_filtered_present;
	struct agps_req_mask _agps_req_mask;
	bool _agps_req_mask_present;
	uint32_t _agps_req_mcc;
	uint32_t _agps_req_mnc;
	struct agps_req_requestType _agps_req_requestType;
	bool _agps_req_requestType_present;
	struct agps_req_rsrp _agps_req_rsrp;
	bool _agps_req_rsrp_present;
	uint32_t _agps_req_tac;
};

#ifdef __cplusplus
}
#endif

#endif /* AGPS_ENCODE_TYPES_H__ */
