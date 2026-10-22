/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef CI_DOXYGEN_REVIEWER_TEST_CONTROL_LINE_100_CHARS_H_
#define CI_DOXYGEN_REVIEWER_TEST_CONTROL_LINE_100_CHARS_H_

/**
 * @file control_line_100_chars.h
 * @ingroup ci_doxygen_reviewer_test
 */

/**
 * @brief Prepare the one-hundred-character control comment case.
 *
 * The quick brown fox jumps over the lazy dog while padding comment length.xxxxxxxxxxxxxxxxxxxxxxx.
 *
 * @return 0 on success, otherwise a negative errno.
 */
int ci_doxy_reviewer_control_line_100_chars_prepare(void);

#endif /* CI_DOXYGEN_REVIEWER_TEST_CONTROL_LINE_100_CHARS_H_ */
