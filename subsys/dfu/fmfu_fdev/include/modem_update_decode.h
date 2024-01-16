/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 * Generated using zcbor version 0.8.1
 * https://github.com/NordicSemiconductor/zcbor
 * Generated with a --default-max-qty of 128
 */

#ifndef MODEM_UPDATE_DECODE_H__
#define MODEM_UPDATE_DECODE_H__

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "modem_update_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#if DEFAULT_MAX_QTY != 128
#error "The type file was generated with a different default_max_qty than this file"
#endif

int cbor_decode_Wrapper(const uint8_t *payload, size_t payload_len,
			struct COSE_Sign1_Manifest *result, size_t *payload_len_out);

int cbor_decode_Sig_structure1(const uint8_t *payload, size_t payload_len,
			       struct Sig_structure1 *result, size_t *payload_len_out);

int cbor_decode_Segments(const uint8_t *payload, size_t payload_len, struct Segments *result,
			 size_t *payload_len_out);

#ifdef __cplusplus
}
#endif

#endif /* MODEM_UPDATE_DECODE_H__ */
