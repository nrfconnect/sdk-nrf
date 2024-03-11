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

#ifndef EXTMEM_SERVICE_ENCODE_H__
#define EXTMEM_SERVICE_ENCODE_H__

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "extmem_service_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#if DEFAULT_MAX_QTY != 3
#error "The type file was generated with a different default_max_qty than this file"
#endif

int cbor_encode_extmem_req(uint8_t *payload, size_t payload_len, const struct extmem_req *input,
			   size_t *payload_len_out);

int cbor_encode_extmem_rsp(uint8_t *payload, size_t payload_len, const struct extmem_rsp *input,
			   size_t *payload_len_out);

int cbor_encode_extmem_nfy(uint8_t *payload, size_t payload_len, const struct extmem_nfy *input,
			   size_t *payload_len_out);

#ifdef __cplusplus
}
#endif

#endif /* EXTMEM_SERVICE_ENCODE_H__ */
