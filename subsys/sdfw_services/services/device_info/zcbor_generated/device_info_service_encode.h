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

#ifndef DEVICE_INFO_SERVICE_ENCODE_H__
#define DEVICE_INFO_SERVICE_ENCODE_H__

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "device_info_service_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#if DEFAULT_MAX_QTY != 3
#error "The type file was generated with a different default_max_qty than this file"
#endif


int cbor_encode_device_info_req(
		uint8_t *payload, size_t payload_len,
		const struct device_info_req *input,
		size_t *payload_len_out);


int cbor_encode_device_info_resp(
		uint8_t *payload, size_t payload_len,
		const struct device_info_resp *input,
		size_t *payload_len_out);


#ifdef __cplusplus
}
#endif

#endif /* DEVICE_INFO_SERVICE_ENCODE_H__ */
