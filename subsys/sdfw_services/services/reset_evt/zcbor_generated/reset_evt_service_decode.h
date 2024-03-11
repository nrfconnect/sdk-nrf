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

#ifndef RESET_EVT_SERVICE_DECODE_H__
#define RESET_EVT_SERVICE_DECODE_H__

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "reset_evt_service_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#if DEFAULT_MAX_QTY != 3
#error "The type file was generated with a different default_max_qty than this file"
#endif

int cbor_decode_reset_evt_sub_req(const uint8_t *payload, size_t payload_len, bool *result,
				  size_t *payload_len_out);

int cbor_decode_reset_evt_sub_rsp(const uint8_t *payload, size_t payload_len, int32_t *result,
				  size_t *payload_len_out);

int cbor_decode_reset_evt_notif(const uint8_t *payload, size_t payload_len,
				struct reset_evt_notif *result, size_t *payload_len_out);

#ifdef __cplusplus
}
#endif

#endif /* RESET_EVT_SERVICE_DECODE_H__ */
