/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 * Generated using zcbor version 0.9.0
 * https://github.com/NordicSemiconductor/zcbor
 * Generated with a --default-max-qty of 3
 */

#ifndef DEVICE_INFO_SERVICE_TYPES_H__
#define DEVICE_INFO_SERVICE_TYPES_H__

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
#define DEFAULT_MAX_QTY 3

struct operation_r {
	enum {
		operation_READ_DEVICE_INFO_c = 0,
		operation_UNSUPPORTED_c = 1,
	} operation_choice;
};

struct stat_r {
	enum {
		stat_SUCCESS_c = 0,
		stat_INTERNAL_ERROR_c = 16781313,
		stat_UNPROGRAMMED_c = 16781314,
	} stat_choice;
};

struct device_info {
	struct zcbor_string device_info_uuid;
	uint32_t device_info_type;
	struct zcbor_string device_info_testimprint;
	uint32_t device_info_partno;
	uint32_t device_info_hwrevision;
	uint32_t device_info_productionrevision;
};

struct read_req {
	struct operation_r read_req_action;
};

struct read_resp {
	struct operation_r read_resp_action;
	struct stat_r read_resp_status;
	struct device_info read_resp_data;
};

struct device_info_req {
	struct read_req device_info_req_msg;
};

struct device_info_resp {
	struct read_resp device_info_resp_msg;
};

#ifdef __cplusplus
}
#endif

#endif /* DEVICE_INFO_SERVICE_TYPES_H__ */
