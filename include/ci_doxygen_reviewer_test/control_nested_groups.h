/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef CI_DOXYGEN_REVIEWER_TEST_CONTROL_NESTED_GROUPS_H_
#define CI_DOXYGEN_REVIEWER_TEST_CONTROL_NESTED_GROUPS_H_

#include <stdint.h>

/**
 * @file control_nested_groups.h
 *
 * @defgroup ci_doxy_test_control_outer Reviewer nested group outer scope
 * @{
 * @brief Outer documentation group for nested group control case.
 */

/**
 * @defgroup ci_doxy_test_control_inner Reviewer nested group inner scope
 * @{
 * @brief Inner documentation group closed before the outer group.
 */

/** @brief Token exported from the nested group control fixture. */
struct ci_doxy_reviewer_nested_token {
	/** Token identifier. */
	uint32_t id;
};

/** @} */

/** @} */

#endif /* CI_DOXYGEN_REVIEWER_TEST_CONTROL_NESTED_GROUPS_H_ */
