/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef CI_DOXYGEN_REVIEWER_TEST_MISSING_BRIEF_H_
#define CI_DOXYGEN_REVIEWER_TEST_MISSING_BRIEF_H_

#include <stddef.h>

/**
 * @file missing_brief.h
 * @ingroup ci_doxygen_reviewer_test
 */

/**
 * Query the reviewer test module status.
 *
 * Reads the accumulated counter value from the test fixture.
 *
 * @param[out] value Stored counter value on success.
 *
 * @return 0 on success, otherwise a negative errno.
 */
int ci_doxy_reviewer_missing_brief_query(size_t *value);

#endif /* CI_DOXYGEN_REVIEWER_TEST_MISSING_BRIEF_H_ */
