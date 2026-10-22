/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef CI_DOXYGEN_REVIEWER_TEST_CONTROL_DUAL_SA_H_
#define CI_DOXYGEN_REVIEWER_TEST_CONTROL_DUAL_SA_H_

/**
 * @file control_dual_sa.h
 * @ingroup ci_doxygen_reviewer_test
 */

/**
 * @brief Open the primary reviewer control channel.
 *
 * @return 0 on success, otherwise a negative errno.
 */
int ci_doxy_reviewer_control_sa_first(void);

/**
 * @brief Open the secondary reviewer control channel.
 *
 * @return 0 on success, otherwise a negative errno.
 */
int ci_doxy_reviewer_control_sa_second(void);

/**
 * @brief Route notifications through the configured control channels.
 *
 * Dispatches pending reviewer events through the registered handlers.
 *
 * @return 0 on success, otherwise a negative errno.
 * @sa ci_doxy_reviewer_control_sa_first
 * @sa ci_doxy_reviewer_control_sa_second
 */
int ci_doxy_reviewer_control_sa_router(void);

#endif /* CI_DOXYGEN_REVIEWER_TEST_CONTROL_DUAL_SA_H_ */
