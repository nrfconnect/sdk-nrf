/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * Generated using zcbor version 0.8.1
 * https://github.com/NordicSemiconductor/zcbor
 * Generated with a --default-max-qty of 10
 */

#ifndef GROUND_FIX_DECODE_TYPES_H__
#define GROUND_FIX_DECODE_TYPES_H__

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
#define DEFAULT_MAX_QTY 10

struct ground_fix_resp {
	double ground_fix_resp_lat;
	double ground_fix_resp_lon;
	union {
		int32_t ground_fix_resp_uncertainty_int;
		double ground_fix_resp_uncertainty_float;
	};
	enum {
		ground_fix_resp_uncertainty_int_c,
		ground_fix_resp_uncertainty_float_c,
	} ground_fix_resp_uncertainty_choice;
	struct zcbor_string ground_fix_resp_fulfilledWith;
};

#ifdef __cplusplus
}
#endif

#endif /* GROUND_FIX_DECODE_TYPES_H__ */
