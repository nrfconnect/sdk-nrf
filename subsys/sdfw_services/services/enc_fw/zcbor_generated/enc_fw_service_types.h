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

#ifndef ENC_FW_SERVICE_TYPES_H__
#define ENC_FW_SERVICE_TYPES_H__

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <zcbor_common.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Which value for --default-max-qty this file was created with.
 *
 *  The define is used in the other generated file to do a build-time
 *  compatibility check.
 *
 *  See `zcbor --help` for more information about --default-max-qty
 */
#define DEFAULT_MAX_QTY 3

struct init {
	struct zcbor_string init_aad;
	struct zcbor_string init_nonce;
	struct zcbor_string init_tag;
	uint32_t init_buffer_addr;
	uint32_t init_buffer_len;
	uint32_t init_image_addr;
	uint32_t init_image_len;
};

struct chunk {
	uint32_t chunk_length;
	bool chunk_last;
};

struct enc_fw_req {
	union {
		struct init enc_fw_req_msg_init_m;
		struct chunk enc_fw_req_msg_chunk_m;
	};
	enum {
		enc_fw_req_msg_init_m_c,
		enc_fw_req_msg_chunk_m_c,
	} enc_fw_req_msg_choice;
};

#ifdef __cplusplus
}
#endif

#endif /* ENC_FW_SERVICE_TYPES_H__ */
