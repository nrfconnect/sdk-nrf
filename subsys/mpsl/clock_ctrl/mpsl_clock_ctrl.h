/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MPSL_CLOCK_CTRL_H__
#define MPSL_CLOCK_CTRL_H__

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Initialize MPSL integration with NRF clock control
 *
 * @retval 0 MPSL clock integration initialized successfully.
 * @retval -NRF_EPERM Clock control is already initialized.
 * @retval -NRF_EINVAL Invalid parameters supplied.
 */
int32_t mpsl_clock_ctrl_init(void);

/** @brief Uninitialize MPSL integration with NRF clock control
 *
 * @retval 0 MPSL clock was uninitialized successfully.
 * @retval -NRF_EPERM MPSL was not initialized before the call.
 */
int32_t mpsl_clock_ctrl_uninit(void);

#ifdef __cplusplus
}
#endif

#endif /* MPSL_CLOCK_CTRL_H__ */
