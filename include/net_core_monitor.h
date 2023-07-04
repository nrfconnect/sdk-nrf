/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NET_CORE_MONITOR_H_
#define NET_CORE_MONITOR_H_

#include <kernel.h>

/**
 * @file
 * @defgroup net_core_monitor Network Core Monitor API
 * @{
 * @brief API for the Network Core Monitor.
 */

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Checking net_core status.
 *
 *  Checking the status of the network core.
 *  The function should be called less frequently than CONFIG_NCM_APP_FEEDING_INTERVAL_MSEC
 *
 *  @param reset_on_failure If true network core is reset on failure.
 *
 *  @retval 0 if it works properly.
 *          -EBUSY network core failure occurred
 *          -EFAULT network core restart occurred
 */
int ncm_net_status_check(bool reset_on_failure);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* NET_CORE_MONITOR_H_ */
