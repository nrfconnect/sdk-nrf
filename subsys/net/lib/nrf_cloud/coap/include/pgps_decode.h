/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * Generated using zcbor version 0.7.0
 * https://github.com/NordicSemiconductor/zcbor
 * Generated with a --default-max-qty of 10
 */

#ifndef PGPS_DECODE_H__
#define PGPS_DECODE_H__

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "pgps_decode_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#if DEFAULT_MAX_QTY != 10
#error "The type file was generated with a different default_max_qty than this file"
#endif

int cbor_decode_pgps_resp(const uint8_t *payload, size_t payload_len, struct pgps_resp *result,
			  size_t *payload_len_out);

#ifdef __cplusplus
}
#endif

#endif /* PGPS_DECODE_H__ */
