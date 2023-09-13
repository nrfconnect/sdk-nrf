/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _FP_STORAGE_PN_H_
#define _FP_STORAGE_PN_H_

#include <sys/types.h>

/**
 * @defgroup fp_storage_pn Fast Pair Personalized Name storage
 * @brief Internal API of Fast Pair Personalized Name storage
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** Length of buffer used to store Personalized Name. */
#ifdef CONFIG_BT_FAST_PAIR_STORAGE_PN_LEN_MAX
#define FP_STORAGE_PN_BUF_LEN	(CONFIG_BT_FAST_PAIR_STORAGE_PN_LEN_MAX + 1)
#else
#define FP_STORAGE_PN_BUF_LEN	0
#endif

/** Save Personalized Name.
 *
 * @param[in] pn_to_save Personalized Name to be saved.
 *
 * @retval 0 If the operation was successful.
 * @retval -ENOMEM If length of Personalized Name string is greater than
 *	   @ref CONFIG_BT_FAST_PAIR_STORAGE_PN_LEN_MAX.
 * @return Negative error code otherwise.
 */
int fp_storage_pn_save(const char *pn_to_save);

/** Get stored Personalized Name.
 *
 * @param[out] buf Pointer to buffer used to store Personalized Name string. Buffer size must be
 *		   at least @ref FP_STORAGE_PN_BUF_LEN.
 *
 * @return 0 If the operation was successful. Otherwise, a (negative) error code is returned.
 */
int fp_storage_pn_get(char *buf);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _FP_STORAGE_PN_H_ */
