/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NET_CORE_MONITOR_H_
#define NET_CORE_MONITOR_H_

/**
 * @file
 * @defgroup net_core_monitor Network Core Monitor API
 * @{
 * @brief API for the Network Core Monitor.
 */

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Check the status of the network core.
 *
 *  The function should be called less frequently than
 *  @kconfig{CONFIG_NCM_APP_FEEDING_INTERVAL_MSEC} to correctly detect the network core status.
 *
 *  @param[out]  reset_reas  Reason for network core reset.
 *                           When the -EFAULT code is returned, the variable is set.
 *                           This value is rewritten from the network core's RESET.RESETREAS
 *                           register.
 *
 *  @retval 0       If network core works properly.
 *  @retval -EBUSY  If network core failure occurred.
 *  @retval -EFAULT If network core restart occurred.
 */
int ncm_net_status_check(uint32_t * const reset_reas);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* NET_CORE_MONITOR_H_ */
