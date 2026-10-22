/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef CI_DOXYGEN_REVIEWER_TEST_STRUCT_MISSING_FIELD_H_
#define CI_DOXYGEN_REVIEWER_TEST_STRUCT_MISSING_FIELD_H_

#include <stdint.h>

/**
 * @file struct_missing_field.h
 * @ingroup ci_doxygen_reviewer_test
 */

/** @brief Reviewer test measurement sample. */
struct ci_doxy_reviewer_measurement {
	/** Sample timestamp in milliseconds. */
	uint64_t timestamp_ms;
	/** Signed amplitude reading. */
	int32_t amplitude;
	uint16_t channel_id;
};

#endif /* CI_DOXYGEN_REVIEWER_TEST_STRUCT_MISSING_FIELD_H_ */
