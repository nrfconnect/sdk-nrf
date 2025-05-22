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
#include <modem/modem_attest_token.h>

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
	NRF_PROVISIONING_EVENT_START = 0x1,
	/** Provisioning process stopped. All provisioning commands (if any) executed. */
	NRF_PROVISIONING_EVENT_STOP,
	/** Provisioning complete. "Finished" command received from the provisioning service. */
	NRF_PROVISIONING_EVENT_DONE,
	/** Provisioning process failed, try again. */
	NRF_PROVISIONING_EVENT_FAILED,
	/** Provisioning process failed, device not claimed.
	 *  Try again after the device is claimed using the attestation token.
	 *
	 *  The event carries a pointer to the modem's attestation token in the 'token' field and is
	 *  only provided if CONFIG_NRF_PROVISIONING_PROVIDE_ATTESTATION_TOKEN is enabled.
	 */
	NRF_PROVISIONING_EVENT_FAILED_DEVICE_NOT_CLAIMED,
	/** Provisioning process failed, wrong CA certificate. */
	NRF_PROVISIONING_EVENT_FAILED_WRONG_ROOT_CA,
	/** Provisioning process failed, no valid datetime reference. */
	NRF_PROVISIONING_EVENT_FAILED_NO_VALID_DATETIME,
	/** Handling credentials internally, need the device to go offline. */
	NRF_PROVISIONING_EVENT_NEED_LTE_DEACTIVATED,
	/** Handling credentials internally, need the device to go online. */
	NRF_PROVISIONING_EVENT_NEED_LTE_ACTIVATED,
	/** Provisioning attempt scheduled
	 *
	 *  The event carries the time until the next provisioning attempt in seconds in the
	 *  'next_attempt_time_seconds' field.
	 *
	 *  This event is only provided if CONFIG_NRF_PROVISIONING_SCHEDULED is enabled.
	 */
	NRF_PROVISIONING_EVENT_SCHEDULED_PROVISIONING,
	/** Error occurred during provisioning. */
	NRF_PROVISIONING_EVENT_FATAL_ERROR,
};

/**
 * @struct nrf_provisioning_callback_data
 * @brief Holds the callback to be called once provisioning state changes together with data
 * set by the callback provider.
 *
 * @param cb        The callback function.
 * @param user_data Application-specific data to be fed to the callback once it is called.
 */
struct nrf_provisioning_callback_data {
	enum nrf_provisioning_event type;
	union {
		/** Modem attestation token for device claiming. */
		struct nrf_attestation_token *token;
		/** Time until next provisioning attempt time in seconds. */
		int64_t next_attempt_time_seconds;
	};
};

/**
 * @typedef nrf_provisioning_event_cb_t
 * @brief Called when provisioning state changes.
 *
 * @param event nrf_provisioning event structure.
 */
typedef void (*nrf_provisioning_event_cb_t)(const struct nrf_provisioning_callback_data *event);

/**
 * @brief Initializes the provisioning library and registers a callback handler.
 *
 * Consequent calls will only change callback functions used.
 * Feeding a null as a callback address means that the corresponding default callback function is
 * taken into use.
 *
 * @param callback_handler Pointer to a library callback handler to receieve notifications.
 * @return < 0 on error, 0 on success.
 */
int nrf_provisioning_init(nrf_provisioning_event_cb_t callback_handler);

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
