/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _FP_ADV_MANAGER_USE_CASE_H_
#define _FP_ADV_MANAGER_USE_CASE_H_

#include <stdint.h>

#include <bluetooth/services/fast_pair/fast_pair.h>

/**
 * @defgroup fp_adv_manager_use_case Use case API for the Fast Pair Advertising Manager
 * @brief Internal API for the use case specific parts of the Fast Pair Advertising Manager
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** Use case callback structure for the Fast Pair Advertising Manager. */
struct fp_adv_manager_use_case_cb {
	/** Enable the use case specific part of the Fast Pair Advertising Manager. */
	int (*enable)(void);

	/** Disable the use case specific part of the Fast Pair Advertising Manager. */
	int (*disable)(void);
};

/** @brief Register use case callbacks for the Fast Pair Advertising Manager.
 *
 *  This function registers an instance of use case callbacks. The registered instance needs
 *  to persist in the memory after this function exits, as it is used directly without the copy
 *  operation. It is possible to register only one instance of callbacks with this API.
 *
 *  This function can only be called before enabling Fast Pair Advertising Manager with the
 *  @ref bt_fast_pair_adv_manager_enable API.
 *
 *  @param cb Callback struct.
 *
 *  @return Zero on success or negative error code otherwise
 */
void fp_adv_manager_use_case_cb_register(const struct fp_adv_manager_use_case_cb *cb);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _FP_ADV_MANAGER_USE_CASE_H_ */
