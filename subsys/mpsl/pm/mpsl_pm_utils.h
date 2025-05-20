/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MPSL_PM_UTILS_H__
#define MPSL_PM_UTILS_H__

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Initialize MPSL Power Management
 *
 * This routine initializes MPSL PM (via `mpsl_pm_init`).
 *
 * @retval 0 MPSL PM was initialized successfully.
 * @retval -NRF_EPERM MPSL PM is already initialized.
 */
int32_t mpsl_pm_utils_init(void);

/** @brief Unitialize MPSL Power Management
 *
 * This routine uninitializes MPSL PM (via `mpsl_pm_uninit`).
 *
 * @pre The function requires the multithreading lock (@see MULTITHREADING_LOCK_ACQUIRE())
 *      to be acquired before.
 *
 * @retval 0 MPSL PM was uninitialized successfully.
 * @retval -NRF_EPERM MPSL was not initialized before the call.
 * @retval -NRF_ETIMEDOUT MPSL PM uninitialization timed out while waiting for completion.
 * @retval -NRF_EFAULT MPSL PM uninitialization failed due to unknown reason.
 */
int32_t mpsl_pm_utils_uninit(void);

/** @brief Handles MPSL Power Management work
 *
 * This calls Zephyr Power Management policy functions according
 * to MPSL PM requirements.
 *
 * @pre mpsl_pm_utils_init() needs to have been called before.
 */
void mpsl_pm_utils_work_handler(void);

#ifdef __cplusplus
}
#endif

#endif /* MPSL_PM_UTILS_H__ */
