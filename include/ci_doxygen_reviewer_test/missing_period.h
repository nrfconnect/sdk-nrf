/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef CI_DOXYGEN_REVIEWER_TEST_MISSING_PERIOD_H_
#define CI_DOXYGEN_REVIEWER_TEST_MISSING_PERIOD_H_

/**
 * @file missing_period.h
 * @ingroup ci_doxygen_reviewer_test
 */

/**
 * @brief Reset the reviewer test module state
 *
 * Clears internal counters used by the reviewer test fixtures.
 *
 * @return 0 on success, otherwise a negative errno.
 */
void ci_doxy_reviewer_missing_period_reset(void);

#endif /* CI_DOXYGEN_REVIEWER_TEST_MISSING_PERIOD_H_ */
