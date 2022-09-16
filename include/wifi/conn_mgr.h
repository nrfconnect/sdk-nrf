/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#ifndef INCLUDE_WIFI_CONN_MGR_H_
#define INCLUDE_WIFI_CONN_MGR_H_

#include <zephyr/zephyr.h>
#include <zephyr/net/wifi_mgmt.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file conn_mgr.h
 * 
 * @defgroup Wi-Fi Connection Manager
 * 
 * @{
 * 
 * @brief Public APIs for the Wi-Fi Connection Manager
 * 
 */

/**
 * @brief Function for adding a credential to the list of networks
 *      that the device will try to connect to. The maximum number of
 *      credentials that can be programmed is configured by setting the
 *      value of CONFIG_WIFI_CONN_MGR_SUPPORTED_CREDS in configuration.
 * 
 * @param new_param Credentials for a new network
 * 
 * @retval 0 if successful
 * @retval -ENOMEM if all credentials have been configured
 */
int wifi_mgr_add_credential(struct wifi_connect_req_params *new_param);

/**
 * @brief Function to block a thread until the device has connected to
 *      a Wi-Fi network. 
 * 
 *      NOTE: Currently only one thread can be blocked at a time with this function.
 * 
 * @param wait Amount of time to block.
 * 
 * @retval 0 if the device connected prior to timeout expiration
 * @retval -EIO if time expired
 * @retval -EAGAIN if another thread is already waiting
 */
int wifi_mgr_wait_connection(k_timeout_t wait);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_WIFI_CONN_MGR_H_ */