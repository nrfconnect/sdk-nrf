/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * Generated using zcbor version 0.8.1
 * https://github.com/NordicSemiconductor/zcbor
 * Generated with a --default-max-qty of 10
 */

#ifndef AGNSS_ENCODE_H__
#define AGNSS_ENCODE_H__

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "agnss_encode_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#if DEFAULT_MAX_QTY != 10
#error "The type file was generated with a different default_max_qty than this file"
#endif

int cbor_encode_agnss_req(uint8_t *payload, size_t payload_len, const struct agnss_req *input,
			  size_t *payload_len_out);

#ifdef __cplusplus
}
#endif

#endif /* AGNSS_ENCODE_H__ */
