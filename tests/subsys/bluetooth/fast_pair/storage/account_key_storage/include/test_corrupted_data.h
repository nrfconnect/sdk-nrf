/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _TEST_CORRUPTED_DATA_H_
#define _TEST_CORRUPTED_DATA_H_

/**
 * @defgroup fp_storage_test_corrupted_data Fast Pair storage corrupted data unit test
 * @brief Unit test of handling corrupted data in the Fast Pair storage
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** Start test suite of corrupted data handling in the Fast Pair storage.
 *
 * The test suite verifies handling of corrupted settings data in the Fast Pair storage module.
 */
void test_corrupted_data_run(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _TEST_CORRUPTED_DATA_H_ */
