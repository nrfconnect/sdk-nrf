/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */


#ifndef __LIB_WIFI_READY_H__
#define __LIB_WIFI_READY_H__

#include <zephyr/net/net_if.h>

/**
 * @defgroup wifi_ready Wi-Fi ready library
 * @brief Library for handling Wi-Fi ready events.
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Structure for storing a callback function to be called when the Wi-Fi is ready.
 */
typedef struct {
	/** Callback function to be called when the Wi-Fi is ready. */
	void (*wifi_ready_cb)(bool wifi_ready);
	/** Interface to which the callback is registered. */
	struct net_if *iface;
	/** Only for internal use. */
	struct k_work item;
} wifi_ready_callback_t;

/**
 * @brief Register a callback to be called when the Wi-Fi is ready.
 *
 * @param cb Callback function to be called when the Wi-Fi is ready.
 *           The callback is called from NET_MGMT thread, so, care should be taken
 *           to avoid blocking the thread and also stack size should be considered.
 * @param iface (optional) Interface to which the callback is registered.
 *
 * @return 0 if the callback was successfully registered, or a negative error code
 *         if the callback could not be registered.
 *
 * @retval -EINVAL if the callback is NULL.
 * @retval -ENOMEM if the callback array is full.
 * @retval -EALREADY if the callback is already registered.
 */

int register_wifi_ready_callback(wifi_ready_callback_t cb, struct net_if *iface);
/**
 * @brief Unregister a callback that was registered to be called when the Wi-Fi is ready.
 *
 * @param cb Callback function to be unregistered.
 * @param iface (optional) Interface to which the callback is registered.
 *
 * @return 0 if the callback was successfully unregistered, or a negative error code
 *         if the callback could not be unregistered.
 *
 * @retval -EINVAL if the callback is NULL.
 * @retval -ENOENT if the callback is not registered.
 */
int unregister_wifi_ready_callback(wifi_ready_callback_t cb, struct net_if *iface);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* __LIB_WIFI_READY_H__*/
