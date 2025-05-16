/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file nrf_provisioning.h
 *
 * @brief nRF Provisioning API.
 */
#ifndef NRF_PROVISIONING_H__
#define NRF_PROVISIONING_H__

#include <stddef.h>

#include <modem/lte_lc.h>

#ifdef __cplusplus
extern "C" {
#endif


/** @defgroup nrf_provisioning nRF Device Provisioning API
 *  @{
 */

/**
 * @brief nrf_provisioning callback events
 *
 * nrf_provisioning events are passed back to the nrf_provisioning_event_cb_t callback function.
 */
enum nrf_provisioning_event {
	/** Provisioning process started. Client will connect to the provisioning service. */
	NRF_PROVISIONING_EVENT_START,
	/** Provisioning process stopped. All provisioning commands (if any) executed. */
	NRF_PROVISIONING_EVENT_STOP,
	/** Provisioning complete. "Finished" command received from the provisioning service. */
	NRF_PROVISIONING_EVENT_DONE,
	/** Provisioning process failed, try again. */
	NRF_PROVISIONING_EVENT_FAILED,
	/** Provisioning process failed, device not claimed.
	 *  Try again after the device is claimed using the attestation token.
	 */
	NRF_PROVISIONING_EVENT_FAILED_NOT_CLAIMED,
	/** Provisioning process failed, wrong CA certificate. */
	NRF_PROVISIONING_EVENT_FAILED_WRONG_CA,
	/** Handling credentials internally, need the device to go offline. */
	NRF_PROVISIONING_EVENT_NEED_OFFLINE,
	/** Handling credentials internally, need the device to go online. */
	NRF_PROVISIONING_EVENT_NEED_ONLINE,
	/** Error occurred during provisioning. */
	NRF_PROVISIONING_EVENT_ERROR
};

/**
 * @typedef nrf_provisioning_event_cb_t
 * @brief Called when provisioning state changes.
 *
 * @param event nrf_provisioning event code.
 * @param user_data Application-specific data.
 */
typedef void (*nrf_provisioning_event_cb_t)(enum nrf_provisioning_event event, void *user_data);

/**
 * @struct nrf_provisioning_callback_data
 * @brief Holds the callback to be called once provisioning state changes together with data
 * set by the callback provider.
 *
 * @param cb        The callback function.
 * @param user_data Application-specific data to be fed to the callback once it is called.
 */
struct nrf_provisioning_callback_data {
	nrf_provisioning_event_cb_t cb;
	void *user_data;
};

/**
 * @brief Initializes the provisioning library and registers callback handlers.
 *
 * Consequent calls will only change callback functions used.
 * Feeding a null as a callback address means that the corresponding default callback function is
 * taken into use.
 *
 * @param callback_data Library callback, called when provisioning state changes.
 * @return < 0 on error, 0 on success.
 */
int nrf_provisioning_init(struct nrf_provisioning_callback_data *callback_data);

/**
 * @brief Starts provisioning immediately.
 *
 * @retval -EFAULT if the library is uninitialized.
 * @retval 0 on success.
 */
int nrf_provisioning_trigger_manually(void);

/**
 * @brief Set provisioning interval.
 *
 * @param interval Provisioning interval in seconds.
 */
void nrf_provisioning_set_interval(int interval);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* NRF_PROVISIONING_H__ */
