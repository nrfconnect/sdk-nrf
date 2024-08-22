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

#ifndef EXTMEM_SERVICE_DECODE_H__
#define EXTMEM_SERVICE_DECODE_H__

#include "extmem_service_types.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#if DEFAULT_MAX_QTY != 3
#error                                                                         \
    "The type file was generated with a different default_max_qty than this file"
#endif

int cbor_decode_extmem_req(const uint8_t *payload, size_t payload_len,
                           struct extmem_req *result, size_t *payload_len_out);

int cbor_decode_extmem_rsp(const uint8_t *payload, size_t payload_len,
                           struct extmem_rsp *result, size_t *payload_len_out);

int cbor_decode_extmem_nfy(const uint8_t *payload, size_t payload_len,
                           struct extmem_nfy *result, size_t *payload_len_out);

#ifdef __cplusplus
}
#endif

#endif /* EXTMEM_SERVICE_DECODE_H__ */
