/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef CI_DOXYGEN_REVIEWER_TEST_FUNCTION_OUTSIDE_GROUP_H_
#define CI_DOXYGEN_REVIEWER_TEST_FUNCTION_OUTSIDE_GROUP_H_

#include <stddef.h>

/**
 * @file function_outside_group.h
 *
 * @defgroup ci_doxy_test_function_outside_group Reviewer test grouped symbols
 * @{
 * @brief Documentation group with correct opening and closing markers.
 */

/** @brief Counter stored inside the grouped fixture. */
struct ci_doxy_reviewer_grouped_counter {
	/** Current counter value. */
	size_t value;
};

/** @} */

/**
 * @brief Publish buffered measurement data.
 *
 * Copies the supplied payload length into the transport buffer configured for the fixture.
 *
 * @param[in] payload Pointer to the measurement bytes to publish.
 * @param[in] payload_len Number of bytes available at @p payload.
 *
 * @return 0 on success, otherwise a negative errno.
 */
int ci_doxy_reviewer_outside_group_publish(const void *payload, size_t payload_len);

#endif /* CI_DOXYGEN_REVIEWER_TEST_FUNCTION_OUTSIDE_GROUP_H_ */
