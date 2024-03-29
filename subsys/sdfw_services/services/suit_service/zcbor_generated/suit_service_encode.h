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

#ifndef SUIT_SERVICE_ENCODE_H__
#define SUIT_SERVICE_ENCODE_H__

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "suit_service_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#if DEFAULT_MAX_QTY != 3
#error "The type file was generated with a different default_max_qty than this file"
#endif

int cbor_encode_suit_req(uint8_t *payload, size_t payload_len, const struct suit_req *input,
			 size_t *payload_len_out);

int cbor_encode_suit_rsp(uint8_t *payload, size_t payload_len, const struct suit_rsp *input,
			 size_t *payload_len_out);

int cbor_encode_suit_nfy(uint8_t *payload, size_t payload_len, const struct suit_nfy *input,
			 size_t *payload_len_out);

#ifdef __cplusplus
}
#endif

#endif /* SUIT_SERVICE_ENCODE_H__ */
