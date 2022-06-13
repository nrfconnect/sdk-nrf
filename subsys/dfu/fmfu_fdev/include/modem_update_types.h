/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 * Generated using zcbor version 0.5.1
 * https://github.com/NordicSemiconductor/zcbor
 * Generated with a --default-max-qty of 128
 */

#ifndef MODEM_UPDATE_TYPES_H__
#define MODEM_UPDATE_TYPES_H__

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "zcbor_decode.h"

/** Which value for --default-max-qty this file was created with.
 *
 *  The define is used in the other generated file to do a build-time
 *  compatibility check.
 *
 *  See `zcbor --help` for more information about --default-max-qty
 */
#define DEFAULT_MAX_QTY 128

struct Segment {
	uint32_t _Segment_target_addr;
	uint32_t _Segment_len;
};

struct Segments {
	struct Segment _Segments__Segment[128];
	uint_fast32_t _Segments__Segment_count;
};

struct Sig_structure1 {
	struct zcbor_string _Sig_structure1_payload;
};

struct Manifest {
	uint32_t _Manifest_version;
	uint32_t _Manifest_compat;
	struct zcbor_string _Manifest_blob_hash;
	struct zcbor_string _Manifest_segments;
};

struct COSE_Sign1_Manifest {
	struct zcbor_string _COSE_Sign1_Manifest_payload;
	struct Manifest _COSE_Sign1_Manifest_payload_cbor;
	struct zcbor_string _COSE_Sign1_Manifest_signature;
};

#endif /* MODEM_UPDATE_TYPES_H__ */
