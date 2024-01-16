/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * Generated using zcbor version 0.8.1
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

struct agnss_req_types_r {
	int32_t agnss_req_types_int[13];
	size_t agnss_req_types_int_count;
};

struct agnss_req_filtered {
	bool agnss_req_filtered;
};

struct agnss_req_mask {
	uint32_t agnss_req_mask;
};

struct agnss_req_rsrp {
	int32_t agnss_req_rsrp;
};

struct agnss_req {
	struct agnss_req_types_r agnss_req_types;
	bool agnss_req_types_present;
	uint32_t agnss_req_eci;
	struct agnss_req_filtered agnss_req_filtered;
	bool agnss_req_filtered_present;
	struct agnss_req_mask agnss_req_mask;
	bool agnss_req_mask_present;
	uint32_t agnss_req_mcc;
	uint32_t agnss_req_mnc;
	struct agnss_req_rsrp agnss_req_rsrp;
	bool agnss_req_rsrp_present;
	uint32_t agnss_req_tac;
};

#ifdef __cplusplus
}
#endif

#endif /* AGNSS_ENCODE_TYPES_H__ */
