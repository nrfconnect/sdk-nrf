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

#ifndef MODEM_UPDATE_TYPES_H__
#define MODEM_UPDATE_TYPES_H__

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
#define DEFAULT_MAX_QTY 128

struct Segment {
	uint32_t Segment_target_addr;
	uint32_t Segment_len;
};

struct Segments {
	struct Segment Segments_Segment_m[128];
	size_t Segments_Segment_m_count;
};

struct Sig_structure1 {
	struct zcbor_string Sig_structure1_body_protected;
	struct zcbor_string Sig_structure1_payload;
};

struct Manifest {
	uint32_t Manifest_version;
	uint32_t Manifest_compat;
	struct zcbor_string Manifest_blob_hash;
	struct zcbor_string Manifest_segments;
};

struct COSE_Sign1_Manifest {
	struct zcbor_string COSE_Sign1_Manifest_protected;
	struct zcbor_string COSE_Sign1_Manifest_payload;
	struct Manifest COSE_Sign1_Manifest_payload_cbor;
	struct zcbor_string COSE_Sign1_Manifest_signature;
};

#ifdef __cplusplus
}
#endif

#endif /* MODEM_UPDATE_TYPES_H__ */
