/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * Generated using zcbor version 0.9.1
 * https://github.com/NordicSemiconductor/zcbor
 * Generated with a --default-max-qty of 10
 */

#ifndef PGNSS_ENCODE_TYPES_H__
#define PGNSS_ENCODE_TYPES_H__

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

struct pgnss_req {
	uint32_t pgnss_req_predictionCount;
	uint32_t pgnss_req_predictionIntervalMinutes;
	uint32_t pgnss_req_startGPSDay;
	uint32_t pgnss_req_startGPSTimeOfDaySeconds;
};

#ifdef __cplusplus
}
#endif

#endif /* PGNSS_ENCODE_TYPES_H__ */
