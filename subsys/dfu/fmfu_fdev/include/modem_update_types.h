/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 * Generated with cddl_gen.py (https://github.com/oyvindronningstad/cddl_gen)
 * Generated with a default_maxq of 128
 */

#ifndef MODEM_UPDATE_TYPES_H__
#define MODEM_UPDATE_TYPES_H__

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "cbor_decode.h"

#define DEFAULT_MAXQ 128

struct Segment {
	uint32_t _Segment_target_addr;
	uint32_t _Segment_len;
};

struct Segments {
	struct Segment _Segments__Segment[128];
	size_t _Segments__Segment_count;
};

struct header_map {
};

struct Sig_structure1 {
	cbor_string_type_t _Sig_structure1_body_protected;
	struct header_map _Sig_structure1_body_protected_cbor;
	cbor_string_type_t _Sig_structure1_payload;
};

struct Manifest {
	uint32_t _Manifest_version;
	uint32_t _Manifest_compat;
	cbor_string_type_t _Manifest_blob_hash;
	cbor_string_type_t _Manifest_segments;
};

struct COSE_Sign1_Manifest {
	cbor_string_type_t _COSE_Sign1_Manifest_protected;
	struct header_map _COSE_Sign1_Manifest_protected_cbor;
	cbor_string_type_t _COSE_Sign1_Manifest_payload;
	struct Manifest _COSE_Sign1_Manifest_payload_cbor;
	cbor_string_type_t _COSE_Sign1_Manifest_signature;
};

#endif /* MODEM_UPDATE_TYPES_H__ */
