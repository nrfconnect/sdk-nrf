/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * Generated using zcbor version 0.8.1
 * https://github.com/NordicSemiconductor/zcbor
 * Generated with a --default-max-qty of 1234567890
 */

#ifndef NRF_PROVISIONING_CBOR_ENCODE_H__
#define NRF_PROVISIONING_CBOR_ENCODE_H__

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "nrf_provisioning_cbor_encode_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#if DEFAULT_MAX_QTY != CONFIG_NRF_PROVISIONING_CBOR_RECORDS
#error "The type file was generated with a different default_max_qty than this file"
#endif

int cbor_encode_responses(uint8_t *payload, size_t payload_len, const struct responses *input,
			  size_t *payload_len_out);

#ifdef __cplusplus
}
#endif

#endif /* NRF_PROVISIONING_CBOR_ENCODE_H__ */
