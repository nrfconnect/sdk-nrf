/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef CI_DOXYGEN_REVIEWER_TEST_CALLBACK_MISSING_PARAM_H_
#define CI_DOXYGEN_REVIEWER_TEST_CALLBACK_MISSING_PARAM_H_

#include <stddef.h>
#include <stdint.h>

/**
 * @file callback_missing_param.h
 * @ingroup ci_doxygen_reviewer_test
 */

/**
 * @brief Progress callback for chunked transfer completion.
 *
 * Invoked after each chunk completes verification.
 *
 * @param[in] ctx User context supplied at registration time.
 *
 * @return 0 to continue processing, otherwise a negative errno to abort.
 */
typedef int (*ci_doxy_reviewer_cb_missing_param_t)(void *ctx, size_t chunk_index);

#endif /* CI_DOXYGEN_REVIEWER_TEST_CALLBACK_MISSING_PARAM_H_ */
