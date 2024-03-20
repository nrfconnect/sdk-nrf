/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @brief
 */

#ifndef WIFI_PROVISION_H__
#define WIFI_PROVISION_H__

/**
 * @defgroup
 * @{
 * @brief
 */

#ifdef __cplusplus
extern "C" {
#endif

enum wifi_provision_evt_type {
	/** The provisioning process has started. */
	WIFI_PROVISION_EVT_STARTED,
	/** A client has connected to the provisioning network. */
	WIFI_PROVISION_EVT_CLIENT_CONNECTED,
	/** A client has disconnected from the provisioning network. */
	WIFI_PROVISION_EVT_CLIENT_DISCONNECTED,
	/** Wi-Fi credentials received. */
	WIFI_PROVISION_EVT_CREDENTIALS_RECEIVED,
	/** The provisioning process has completed. */
	WIFI_PROVISION_EVT_COMPLETED,
	/** Wi-Fi credentials deleted, request reboot. */
	WIFI_PROVISION_EVT_REBOOT,
	/** The provisioning process has failed, irrecoverable error. */
	WIFI_PROVISION_EVT_FATAL_ERROR,
};

/** @brief Struct with data received from the library. */
struct wifi_provision_evt {
	/** Type of event. */
	enum wifi_provision_evt_type type;
};

/** @brief AWS IoT library asynchronous event handler.
 *
 *  @param[in] evt The event and any associated parameters.
 */
typedef void (*wifi_provision_evt_handler_t)(const struct wifi_provision_evt *evt);

/** @brief Initialize and start the Wi-Fi provisioning library.
 *
 *  @param[in] handler Event handler to be called for asynchronous events.
 *
 *  @retval 0 If the operation was successful.
 *	    Otherwise, a (negative) error code is returned.
 */
int wifi_provision_start(const wifi_provision_evt_handler_t handler);

/** @brief Reset the library and restart the provisioning process.
 *
 *  @retval 0 If the operation was successful.
 *	    Otherwise, a (negative) error code is returned.
 */
int wifi_provision_reset(void);

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* WIFI_PROVISION_H__ */
