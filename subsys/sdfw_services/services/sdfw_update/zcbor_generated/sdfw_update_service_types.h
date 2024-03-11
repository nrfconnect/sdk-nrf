/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 * Generated using zcbor version 0.8.1
 * https://github.com/NordicSemiconductor/zcbor
 * Generated with a --default-max-qty of 3
 */

#ifndef SDFW_UPDATE_SERVICE_TYPES_H__
#define SDFW_UPDATE_SERVICE_TYPES_H__

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

struct sdfw_update_req {
	uint32_t sdfw_update_req_tbs_addr;
	uint32_t sdfw_update_req_dl_max;
	uint32_t sdfw_update_req_dl_addr_fw;
	uint32_t sdfw_update_req_dl_addr_pk;
	uint32_t sdfw_update_req_dl_addr_signature;
};

#ifdef __cplusplus
}
#endif

#endif /* SDFW_UPDATE_SERVICE_TYPES_H__ */
