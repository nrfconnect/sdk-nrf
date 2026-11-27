/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef CI_DOXYGEN_REVIEWER_TEST_GROUP_WRONG_ORDER_H_
#define CI_DOXYGEN_REVIEWER_TEST_GROUP_WRONG_ORDER_H_

#include <stdint.h>

/**
 * @file group_wrong_order.h
 *
 * @defgroup ci_doxy_test_group_wrong_order Reviewer test group marker order
 * @brief Documentation group for validating Doxygen group marker ordering.
 * @}
 * @{
 */

/** @brief Sample value published by the wrong-order group fixture. */
struct ci_doxy_reviewer_wrong_order_value {
	/** Published numeric payload. */
	uint32_t value;
};

/** @} */

#endif /* CI_DOXYGEN_REVIEWER_TEST_GROUP_WRONG_ORDER_H_ */
