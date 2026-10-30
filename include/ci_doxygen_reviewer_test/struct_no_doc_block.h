/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef CI_DOXYGEN_REVIEWER_TEST_STRUCT_NO_DOC_BLOCK_H_
#define CI_DOXYGEN_REVIEWER_TEST_STRUCT_NO_DOC_BLOCK_H_

#include <stdint.h>

/**
 * @file struct_no_doc_block.h
 * @ingroup ci_doxygen_reviewer_test
 */

struct ci_doxy_reviewer_undocumented_payload {
	int schema_version;
	uint32_t sequence_number;
	uint16_t channel_mask;
};

#endif /* CI_DOXYGEN_REVIEWER_TEST_STRUCT_NO_DOC_BLOCK_H_ */
