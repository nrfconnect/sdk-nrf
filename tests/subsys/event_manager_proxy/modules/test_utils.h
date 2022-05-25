/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _TEST_UTILS_H_
#define _TEST_UTILS_H_

#include "test_events.h"

/**
 * @brief Mark the time when the test was started
 *
 * @sa test_time_spent_us
 */
void test_time_start(void);

/**
 * @brief Get number of us in test
 *
 * @return Number of us since @ref test_time_start was called.
 */
uint32_t test_time_spent_us(void);

/**
 * @brief Get current test
 *
 * @return Test id.
 */
enum test_id test_current(void);

/**
 * @brief Send test start event
 *
 * @param test_id The test id.
 */
void test_start(enum test_id test_id);

/**
 * @brief Wait for test start acknowledgment from remote
 */
void test_start_ack_wait(void);

/**
 * @brief Send test end event and wait
 *
 * Function sends test end event and waits for it to be processed.
 * @param test_id The test id.
 */
void test_end(enum test_id test_id);

/**
 * @brief Wait for the test to end
 *
 * @param test_id The test id.
 */
void test_end_wait(enum test_id test_id);

/**
 * @brief Get the result returned by the remote
 *
 * This function returns the result reported by the remote
 * when it sends test end remote event.
 *
 * @return The result from the remote.
 */
int test_remote_result(void);

#endif /* _TEST_UTILS_H_ */
