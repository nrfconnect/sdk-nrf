/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef CI_DOXYGEN_REVIEWER_TEST_CONTROL_WRAPPED_PARAM_H_
#define CI_DOXYGEN_REVIEWER_TEST_CONTROL_WRAPPED_PARAM_H_

#include <stddef.h>
#include <stdint.h>

/**
 * @file control_wrapped_param.h
 * @ingroup ci_doxygen_reviewer_test
 */

/**
 * @brief Copy reviewer control data into a destination buffer.
 *
 * Validates buffer bounds before copying bytes from the source region.
 *
 * @param[out] dst Destination buffer receiving the copied bytes. The buffer must
 * remain valid until the function returns.
 * @param[in] dst_len Capacity of @p dst in bytes.
 * @param[in] src Source region to copy from.
 * @param[in] src_len Number of bytes available at @p src.
 *
 * @return Number of bytes copied on success, otherwise a negative errno.
 */
int ci_doxy_reviewer_control_wrapped_param_copy(void *dst, size_t dst_len,
					      const void *src, size_t src_len);

#endif /* CI_DOXYGEN_REVIEWER_TEST_CONTROL_WRAPPED_PARAM_H_ */
