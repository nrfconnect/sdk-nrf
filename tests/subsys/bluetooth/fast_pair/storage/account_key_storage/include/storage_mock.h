/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _STORAGE_MOCK_H_
#define _STORAGE_MOCK_H_

/**
 * @defgroup fp_storage_test_storage_mock Fast Pair storage unit test's mocked storage
 * @brief API of mocked storage used by the Fast Pair storage unit test
 *
 * The mocked storage registers Zephyr's setting backend and provides API to clear stored data.
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** Clear mocked settings storage.
 *
 * The function removes all data stored in the mocked storage.
 */
void storage_mock_clear(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _STORAGE_MOCK_H_ */
