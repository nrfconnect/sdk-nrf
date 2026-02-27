/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef ESB_GLUE_H__
#define ESB_GLUE_H__

/**
 * @file
 * @brief ESB platform glue layer
 *
 * This file defines the platform glue interface for the Enhanced ShockBurst
 * (ESB) subsystem. The glue layer handles platform-specific initialization
 * required by ESB.
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup esb_glue ESB platform glue
 * @{
 */

/**
 * @brief Start the HF clock required by the ESB radio and apply
 *        platform-specific errata workarounds.
 *
 * @retval 0       HF clock started successfully.
 * @retval -ENXIO  Clock manager not available.
 * @retval other   Negative error code on failure.
 */
int esb_clocks_start(void);

/**
 * @brief Stop the HF clock previously started by @ref esb_clocks_start.
 *
 * @retval 0       HF clock released successfully.
 * @retval -ENXIO  Clock manager not available.
 * @retval other   Negative error code on failure.
 */
int esb_clocks_stop(void);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ESB_GLUE_H__ */
