/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef TEST_STARTUP_FAILURE_COMMON_H__
#define TEST_STARTUP_FAILURE_COMMON_H__

/**
 * @brief Assert that calling @ref suit_execution_mode_startup_failed in the current state
 *        is correctly handled.
 */
void check_startup_failure(void);

#endif /* TEST_STARTUP_FAILURE_COMMON_H__ */
