/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef CI_DOXYGEN_REVIEWER_TEST_CONTROL_IMPERATIVE_FUNCTION_H_
#define CI_DOXYGEN_REVIEWER_TEST_CONTROL_IMPERATIVE_FUNCTION_H_

/**
 * @file control_imperative_function.h
 * @ingroup ci_doxygen_reviewer_test
 */

/**
 * @brief Enable the reviewer test fixture.
 *
 * Activates the fixture so subsequent control cases can execute deterministically.
 *
 * @return 0 on success, otherwise a negative errno.
 */
int ci_doxy_reviewer_control_imperative_enable(void);

#endif /* CI_DOXYGEN_REVIEWER_TEST_CONTROL_IMPERATIVE_FUNCTION_H_ */
