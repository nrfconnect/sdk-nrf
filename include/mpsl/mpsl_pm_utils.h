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
 */
void mpsl_pm_utils_init(void);

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
