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

#ifndef DVFS_SERVICE_DECODE_H__
#define DVFS_SERVICE_DECODE_H__

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "dvfs_service_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#if DEFAULT_MAX_QTY != 3
#error "The type file was generated with a different default_max_qty than this file"
#endif

int cbor_decode_dvfs_oppoint_req(const uint8_t *payload, size_t payload_len,
				 struct dvfs_oppoint_req *result, size_t *payload_len_out);

int cbor_decode_dvfs_oppoint_rsp(const uint8_t *payload, size_t payload_len,
				 struct dvfs_oppoint_rsp *result, size_t *payload_len_out);

#ifdef __cplusplus
}
#endif

#endif /* DVFS_SERVICE_DECODE_H__ */
