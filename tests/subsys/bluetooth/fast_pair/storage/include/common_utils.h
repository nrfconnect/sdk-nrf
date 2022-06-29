/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _COMMON_UTILS_H_
#define _COMMON_UTILS_H_

/**
 * @defgroup fp_storage_test_common_utils Fast Pair storage unit test common utilities
 * @brief Common utilities of the Fast Pair storage unit test
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** Generate mocked Account Key data.
 *
 * @param[in]  seed		Seed used to generate the Account Key.
 * @param[out] account_key 	Buffer used to store generated Account Key.
 */
void cu_generate_account_key(uint8_t seed, struct fp_account_key *account_key);

/** Check if Account Key data was generated from provided seed.
 *
 * @param[in] seed		Seed to be verified.
 * @param[in] account_key	Account Key data to be verified.
 *
 * @return true If the Account Key data was generated from provided seed.
 *              Otherwise, false is returned.
 */
bool cu_check_account_key_seed(uint8_t seed, const struct fp_account_key *account_key);

/** Validate that Account Keys are not loaded.
 *
 * The function uses Fast Pair storage API to validate that the data is not loaded from settings.
 * Ztest asserts are used to signalize error.
 */
void cu_account_keys_validate_unloaded(void);

/** Validate the loaded Account Keys.
 *
 * The function uses Fast Pair storage API to validate that the data is valid.
 * The function assumes that @ref cu_generate_account_key was used to generate subsequent Account
 * Keys and seed value was incremented by one for each subsequent Account Key.
 * Ztest asserts are used to signalize error.
 *
 * @param[in] seed_first	Seed assigned to the first generated Account Key.
 * @param[in] stored_cnt	Number of stored keys.
 */
void cu_account_keys_validate_loaded(uint8_t seed_first, uint8_t stored_cnt);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _COMMON_UTILS_H_ */
