/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * Generated using zcbor version 0.7.0
 * https://github.com/NordicSemiconductor/zcbor
 * Generated with a --default-max-qty of 10
 */

#ifndef PGPS_ENCODE_TYPES_H__
#define PGPS_ENCODE_TYPES_H__

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

struct pgps_req {
	uint32_t _pgps_req_predictionCount;
	uint32_t _pgps_req_predictionIntervalMinutes;
	uint32_t _pgps_req_startGPSDay;
	uint32_t _pgps_req_startGPSTimeOfDaySeconds;
};

#ifdef __cplusplus
}
#endif

#endif /* PGPS_ENCODE_TYPES_H__ */
