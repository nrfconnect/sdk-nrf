/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 * Generated using zcbor version 0.9.1
 * https://github.com/NordicSemiconductor/zcbor
 * Generated with a --default-max-qty of 3
 */

#ifndef DVFS_SERVICE_TYPES_H__
#define DVFS_SERVICE_TYPES_H__

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
#define DEFAULT_MAX_QTY 3

struct dvfs_oppoint_r {
	enum {
		dvfs_oppoint_DVFS_FREQ_HIGH_c = 0,
		dvfs_oppoint_DVFS_FREQ_MEDLOW_c = 1,
		dvfs_oppoint_DVFS_FREQ_LOW_c = 2,
	} dvfs_oppoint_choice;
};

struct dvfs_oppoint_req {
	struct dvfs_oppoint_r dvfs_oppoint_req_data;
};

struct stat_r {
	enum {
		stat_SUCCESS_c = 0,
		stat_INTERNAL_ERROR_c = 16781313,
	} stat_choice;
};

struct dvfs_oppoint_rsp {
	struct dvfs_oppoint_r dvfs_oppoint_rsp_data;
	struct stat_r dvfs_oppoint_rsp_status;
};

#ifdef __cplusplus
}
#endif

#endif /* DVFS_SERVICE_TYPES_H__ */
