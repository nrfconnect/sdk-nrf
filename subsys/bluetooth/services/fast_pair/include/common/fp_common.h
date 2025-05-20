/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _FP_COMMON_H_
#define _FP_COMMON_H_

/**
 * @defgroup fp_common Fast Pair common data
 * @brief Internal API for Fast Pair common data
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** Length of Account Key (128 bits = 16 bytes). */
#define FP_ACCOUNT_KEY_LEN	16U

/** @brief Fast Pair Account Key. */
struct fp_account_key {
	/** Account Key. */
	uint8_t key[FP_ACCOUNT_KEY_LEN];
};


#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _FP_COMMON_H_ */
