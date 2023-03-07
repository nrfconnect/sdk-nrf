/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef BT_THROUGHPUT_TEST_H_
#define BT_THROUGHPUT_TEST_H_

/**
 * Initialize BLE throughput test
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int bt_throughput_test_init(void);

/**
 * @brief Run BLE throughput test
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int bt_throughput_test_run(void);

/**
 * @brief Exit BLE throughput test
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int bt_throughput_test_exit(void);

#endif /* THROUGHPUT_MAIN_H_ */
