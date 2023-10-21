/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 *  @brief Memfault NCS integration
 */

#ifndef _MEMFAULT_NCS_H_
#define _MEMFAULT_NCS_H_

/**
 * @defgroup memfault_ncs Memfault integration in NCS
 * @brief Convenience functions for using Memfault firmware SDK with nRF Connect SDK.
 *
 * @{
 */

#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Set the Memfault device ID.
 *
 *  @note In order to use this API successfully, CONFIG_MEMFAULT_NCS_DEVICE_ID_RUNTIME
 *	  must be enabled.
 *
 *  @param device_id Pointer to device ID buffer
 *  @param len Length of device ID. Cannot exceed CONFIG_MEMFAULT_NCS_DEVICE_ID_MAX_LEN.
 *
 *  @return 0 on success, otherwise a negative error code
 */
int memfault_ncs_device_id_set(const char *device_id, size_t len);

/** @brief Trigger NCS metrics collection. Called by default from NCS
 *	   memfault_metrics_heartbeat_collect_data(), but can instead be called
 *	   from user's implementation of
 *	   memfault_metrics_heartbeat_collect_data(), see
 *	   CONFIG_MEMFAULT_NCS_IMPLEMENT_METRICS_COLLECTION Kconfig option.
 */
void memfault_ncs_metrics_collect_data(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _MEMFAULT_NCS_H_ */
