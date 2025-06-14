/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file mpsl_clock_ctrl_cb.h
 *
 * @defgroup mpsl_clock_ctrl_cb MPSL Clock Control Notification Callback API
 *
 * @brief Callback API for MPSL clock control event notifications.
 * @{
 */

#ifndef MPSL_CLOCK_CTRL_CB_H__
#define MPSL_CLOCK_CTRL_CB_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 *  @typedef mpsl_clock_ctrl_cb_t
 *
 *  @brief Callback type for clock control event.
 */
typedef void (*mpsl_clock_ctrl_cb_t)(void);

/**
 * @brief Register callback for HFCLK requested event.
 *
 * @param cb Function to call after HFCLK is requested by MPSL. @c NULL to unregister.
 */
void mpsl_clock_ctrl_cb_register_hfclk_requested(mpsl_clock_ctrl_cb_t cb);

/**
 * @brief Register callback for HFCLK released event.
 *
 * @param cb Function to call after HFCLK is released by MPSL. @c NULL to unregister.
 */
void mpsl_clock_ctrl_cb_register_hfclk_released(mpsl_clock_ctrl_cb_t cb);

#ifdef __cplusplus
}
#endif

#endif /* MPSL_CLOCK_CTRL_CB_H__ */

/**@} */
