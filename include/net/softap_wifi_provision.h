/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file softap_wifi_provision.h
 * @brief Header file for the SoftAP Wi-Fi Provision Library.
 */

#ifndef SOFTAP_WIFI_PROVISION_H__
#define SOFTAP_WIFI_PROVISION_H__

/**
 * @defgroup softap_wifi_provision_library SoftAP Wi-Fi Provision library
 * @{
 * @brief Library used to provision a Wi-Fi device to a Wi-Fi network using HTTPS in softAP mode.
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Types of events generated during provisioning.
 */
enum softap_wifi_provision_evt_type {
	/** The provisioning process has started. */
	SOFTAP_WIFI_PROVISION_EVT_STARTED,

	/** A client (station) has connected to the softAP Wi-Fi network. */
	SOFTAP_WIFI_PROVISION_EVT_CLIENT_CONNECTED,

	/** A client (station) has disconnected from the softAP Wi-Fi network. */
	SOFTAP_WIFI_PROVISION_EVT_CLIENT_DISCONNECTED,

	/** Wi-Fi credentials received from the client. */
	SOFTAP_WIFI_PROVISION_EVT_CREDENTIALS_RECEIVED,

	/** The provisioning process has completed. */
	SOFTAP_WIFI_PROVISION_EVT_COMPLETED,

	/** Credentials deleted, the library has become unprovisioned.
	 *  A reboot is needed to restart provisioning.
	 *  Ideally we would bring the Wi-Fi network interface down/up but this is currently not
	 *  supported.
	 */
	SOFTAP_WIFI_PROVISION_EVT_UNPROVISIONED_REBOOT_NEEDED,

	/** The provisioning process has failed, this is an unrecoverable error. */
	SOFTAP_WIFI_PROVISION_EVT_FATAL_ERROR,
};

/**
 * @brief Structure containing event data from the provisioning process.
 */
struct softap_wifi_provision_evt {
	/** Type of the event. */
	enum softap_wifi_provision_evt_type type;
};

/**
 * @brief Type for event handler callback function.
 *
 * @param[in] evt Event and associated parameters.
 */
typedef void (*softap_wifi_provision_evt_handler_t)(const struct softap_wifi_provision_evt *evt);

/**
 * @brief Initialize the library.
 *
 * Must be called before any other function in the library.
 *
 * @param[in] handler Event handler for receiving asynchronous notifications.
 *
 * @retval -EINVAL if the handler is NULL.
 */
int softap_wifi_provision_init(const softap_wifi_provision_evt_handler_t handler);

/**
 * @brief Start the provisioning process.
 *
 * Blocks until provisioning is completed.
 *
 * @returns 0 if successful. Otherwise, a negative error code is returned (errno.h).
 * @retval -ENOTSUP if the library is not initialized.
 * @retval -EINPROGRESS if provisioning is already in progress.
 * @retval -EALREADY if the device is already provisioned. Credentials was found in
 *		     Wi-Fi credentials storage.
 */
int softap_wifi_provision_start(void);

/**
 * @brief Reset the library.
 *
 * Deletes any stored Wi-Fi credentials and requests a reboot via the event
 * SOFTAP_WIFI_PROVISION_EVT_UNPROVISIONED_REBOOT_NEEDED.
 *
 * @retval -ENOTSUP If the library is not initialized.
 */
int softap_wifi_provision_reset(void);

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* SOFTAP_WIFI_PROVISION_H__ */
