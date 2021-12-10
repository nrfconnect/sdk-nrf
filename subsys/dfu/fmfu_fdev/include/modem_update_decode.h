/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 * Generated using cddl_gen version 0.3.99
 * https://github.com/NordicSemiconductor/cddl-gen
 * Generated with a default_max_qty of 128
 */

#ifndef MODEM_UPDATE_DECODE_H__
#define MODEM_UPDATE_DECODE_H__

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "cbor_decode.h"
#include "modem_update_types.h"

#if DEFAULT_MAX_QTY != 128
#error "The type file was generated with a different default_max_qty than this file"
#endif

bool cbor_decode_Wrapper(const uint8_t *payload, uint32_t payload_len,
			 struct COSE_Sign1_Manifest *result,
			 uint32_t *payload_len_out);

bool cbor_decode_Sig_structure1(const uint8_t *payload, uint32_t payload_len,
				struct Sig_structure1 *result,
				uint32_t *payload_len_out);

bool cbor_decode_Segments(const uint8_t *payload, uint32_t payload_len,
			  struct Segments *result, uint32_t *payload_len_out);

#endif /* MODEM_UPDATE_DECODE_H__ */
