/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef CI_DOXYGEN_REVIEWER_TEST_CALLBACK_MISSING_RETURN_H_
#define CI_DOXYGEN_REVIEWER_TEST_CALLBACK_MISSING_RETURN_H_

#include <stdint.h>

/**
 * @file callback_missing_return.h
 * @ingroup ci_doxygen_reviewer_test
 */

/**
 * @brief Filter callback selecting active records.
 *
 * Determines whether the record identified by @p id should remain active.
 *
 * @param[in] ctx Filter context provided at registration time.
 * @param[in] id Identifier of the candidate record.
 */
typedef int (*ci_doxy_reviewer_cb_missing_return_t)(void *ctx, uint32_t id);

#endif /* CI_DOXYGEN_REVIEWER_TEST_CALLBACK_MISSING_RETURN_H_ */
