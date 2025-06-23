/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file nrf_provisioning.h
 *
 * @brief nRF Provisioning API.
 *
 * This library provides functionality for device provisioning over cellular networks.
 * It handles communication with provisioning services to configure device credentials,
 * certificates, and other configuration data securely.
 *
 * The library supports both HTTP and CoAP transport protocols for communication with
 * the provisioning service. It provides automatic certificate provisioning, scheduled
 * provisioning with configurable intervals, and error handling with
 * retry mechanisms.
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
 *
 * @brief API for device provisioning over cellular networks.
 *
 * This API provides functionality for secure device provisioning using either HTTP or CoAP
 * transport protocols. The library handles:
 *
 * - Device identity verification using modem attestation tokens
 * - Secure communication with provisioning services
 * - Automatic certificate and credential management
 * - Scheduled provisioning with configurable intervals
 * - Comprehensive error handling and retry mechanisms
 */

/**
 * @brief nrf_provisioning callback events
 *
 * These events are passed to the registered callback function to inform the application
 * about the current state of the provisioning process. Events are generated asynchronously
 * as the provisioning state machine progresses through different phases.
 *
 * @note The callback function should not perform blocking operations as it may affect
 *       provisioning performance and other system operations.
 */
enum nrf_provisioning_event {
	/** Provisioning process started. Client will connect to the provisioning service. */
	NRF_PROVISIONING_EVENT_START = 0x1,

	/** Provisioning process stopped. All provisioning commands (if any)
	 *  executed successfully.
	 */
	NRF_PROVISIONING_EVENT_STOP,

	/**
	 * Provisioning complete. "Finished" command received from the provisioning service.
	 * Device is now fully provisioned.
	 */
	NRF_PROVISIONING_EVENT_DONE,

	/**
	 * Provisioning process failed, retry recommended. If CONFIG_NRF_PROVISIONING_SCHEDULED
	 * is enabled, the provisioning will be automatically rescheduled with exponential
	 * backoff up to a maximum interval.
	 */
	NRF_PROVISIONING_EVENT_FAILED,

	/**
	 * Provisioning process completed, but no commands were received from the server.
	 * This may indicate the device is already fully provisioned.
	 */
	NRF_PROVISIONING_EVENT_NO_COMMANDS,

	/**
	 * Provisioning process failed due to too many commands for the device to handle.
	 * Increase CONFIG_NRF_PROVISIONING_CBOR_RECORDS to allow more commands.
	 * It also might be needed to increase heap size to allocate the larger number of
	 * commands.
	 */
	NRF_PROVISIONING_EVENT_FAILED_TOO_MANY_COMMANDS,

	/**
	 * Provisioning process failed because the device is not claimed on the
	 * provisioning service. The device must be claimed using the attestation token
	 * before provisioning can succeed.
	 *
	 * The event carries a pointer to the modem's attestation token in the 'token'
	 * field and is only provided if CONFIG_NRF_PROVISIONING_PROVIDE_ATTESTATION_TOKEN
	 * is enabled. The token pointer is valid only during the callback execution and
	 * should not be stored.
	 */
	NRF_PROVISIONING_EVENT_FAILED_DEVICE_NOT_CLAIMED,

	/**
	 * Provisioning process failed due to incorrect CA certificate configuration.
	 * Verify the root CA certificate matches the provisioning service.
	 */
	NRF_PROVISIONING_EVENT_FAILED_WRONG_ROOT_CA,

	/**
	 * Provisioning process failed because no valid datetime reference is available.
	 * Ensure the device has access to network time or SNTP.
	 */
	NRF_PROVISIONING_EVENT_FAILED_NO_VALID_DATETIME,

	/**
	 * Credential handling requires LTE to be deactivated.
	 * Application should call lte_lc_offline(), conn_mgr_all_if_disconnect(),
	 * or similar to deactivate LTE connectivity. This event is generated when
	 * the library needs to write certificates or credentials to the modem,
	 * which requires the LTE connection to be offline.
	 *
	 * Note that conn_mgr_all_if_disconnect() disconnects
	 * only LTE connectivity, meaning that if enabled, GNSS remains active.
	 * If the modem's functional mode is LTE_LC_FUNC_MODE_ACTIVATE_GNSS, GNSS
	 * must also be deactivated by setting the functional mode to
	 * LTE_LC_FUNC_MODE_DEACTIVATE_GNSS. The application should check the
	 * current functional mode and ensure both LTE and GNSS are disabled for
	 * successful credential operations.
	 */
	NRF_PROVISIONING_EVENT_NEED_LTE_DEACTIVATED,

	/**
	 * Credential handling complete, LTE can be reactivated.
	 * Application should call lte_lc_connect() or similar to restore LTE
	 * connectivity. This event is generated after certificate/credential operations
	 * are completed.
	 */
	NRF_PROVISIONING_EVENT_NEED_LTE_ACTIVATED,

	/**
	 * Next provisioning attempt has been scheduled.
	 *
	 * The event carries the time until the next provisioning attempt in seconds in
	 * the 'next_attempt_time_seconds' field.
	 *
	 * This event is only provided if CONFIG_NRF_PROVISIONING_SCHEDULED is enabled.
	 */
	NRF_PROVISIONING_EVENT_SCHEDULED_PROVISIONING,

	/**
	 * A fatal unrecoverable error occurred during provisioning.
	 * Manual intervention or device reset may be required.
	 */
	NRF_PROVISIONING_EVENT_FATAL_ERROR,
};

/**
 * @struct nrf_provisioning_callback_data
 * @brief Event data structure passed to the provisioning callback function.
 *
 * Contains the event type and associated data based on the specific event.
 * The union fields are valid only for specific event types as documented
 * for each event.
 *
 * @warning Pointers in the union (such as token) are only valid during
 *          the callback execution and must not be stored or accessed after
 *          the callback returns.
 */
struct nrf_provisioning_callback_data {
	/** The type of provisioning event that occurred. */
	enum nrf_provisioning_event type;
	union {
		/**
		 * Modem attestation token for device claiming.
		 * Valid only for NRF_PROVISIONING_EVENT_FAILED_DEVICE_NOT_CLAIMED events.
		 */
		struct nrf_attestation_token *token;
		/**
		 * Time until next provisioning attempt in seconds.
		 * Valid only for NRF_PROVISIONING_EVENT_SCHEDULED_PROVISIONING events.
		 */
		int64_t next_attempt_time_seconds;
	};
};

/**
 * @typedef nrf_provisioning_event_cb_t
 * @brief Callback function type for provisioning state changes.
 *
 * This callback is called whenever the provisioning state changes or events occur.
 * The callback should not block for extended periods as it may affect provisioning
 * performance and other system operations.
 *
 * The callback is executed from a work queue context, so it can safely call most
 * Zephyr APIs. However, blocking operations should be avoided or performed in a
 * separate thread/workqueue.
 *
 * @param event Pointer to event data structure containing event type and associated data.
 *              The pointer and its contents are only valid during the callback execution.
 *
 * @note For NRF_PROVISIONING_EVENT_NEED_LTE_DEACTIVATED and
 *       NRF_PROVISIONING_EVENT_NEED_LTE_ACTIVATED events, the callback should
 *       handle the LTE state changes appropriately to allow the provisioning
 *       process to continue.
 */
typedef void (*nrf_provisioning_event_cb_t)(const struct nrf_provisioning_callback_data *event);

/**
 * @brief Initialize the provisioning library and registers a callback handler.
 *
 * This function must be called before any other provisioning API functions.
 * It sets up the necessary resources and registers the callback for receiving
 * provisioning events. The initialization process includes:
 * - Setting up work queues for asynchronous operations
 * - Initializing transport layer (HTTP or CoAP based on configuration)
 * - Registering Zephyr NET MGMT handlers
 * - Starting provisioning if CONFIG_NRF_PROVISIONING_AUTO_START_ON_INIT is enabled
 *
 * @note Initialization work is performed asynchronously. The function returns immediately
 *       after basic setup, while settings initialization continue in the background.
 *	 Monitor callback events for initialization progress.
 *
 * @param callback_handler Pointer to a callback function to receive provisioning notifications.
 *			   The callback will be called from a work queue context.
 *			   Must not be NULL.
 *
 * @retval 0 on success.
 * @retval -EINVAL if callback_handler is NULL.
 * @retval -EALREADY if the library is already initialized.
 */
int nrf_provisioning_init(nrf_provisioning_event_cb_t callback_handler);

/**
 * @brief Start provisioning immediately.
 *
 * Triggers an immediate provisioning attempt, bypassing any scheduled intervals.
 * This function returns immediately; the actual provisioning process is asynchronous
 * and status is reported through the registered callback function.
 *
 * The provisioning process includes:
 * - Waiting for valid date/time (required for JWT generation)
 * - Connecting to the provisioning service
 * - Exchanging provisioning commands
 * - Processing any received credentials or configuration
 *
 * @note If CONFIG_NRF_PROVISIONING_SCHEDULED is enabled, this will reschedule
 *       the next automatic provisioning attempt after completion.
 *
 * @retval 0 on success (provisioning started).
 * @retval -EFAULT if the library is not initialized.
 */
int nrf_provisioning_trigger_manually(void);

/**
 * @brief Set provisioning interval for scheduled provisioning.
 *
 * Configures the interval between automatic provisioning attempts when
 * CONFIG_NRF_PROVISIONING_SCHEDULED is enabled. If scheduled provisioning
 * is active, this will reschedule the next attempt based on the new interval.
 *
 * The interval is persistent and stored in settings. It will be restored
 * across device reboots. The actual provisioning time may include a random
 * spread to distribute server load (controlled by CONFIG_NRF_PROVISIONING_SPREAD_S).
 *
 * @note This function requires CONFIG_NRF_PROVISIONING_SCHEDULED to be enabled.
 *       The interval is capped at CONFIG_NRF_PROVISIONING_INTERVAL_S.
 *
 * @param interval Provisioning interval in seconds. Must be greater than or equal to 0.
 *		   Values less than the minimum interval may be adjusted internally.
 *		   A value of 0 disables scheduled provisioning.
 *
 * @retval 0 on success.
 * @retval -EFAULT if the library is not initialized or unable to convert interval to string format.
 * @retval -EINVAL if interval is negative.
 * @retval -ENOTSUP if CONFIG_NRF_PROVISIONING_SCHEDULED is not enabled.
 * @retval -ENOSTR if unable to store interval in settings.
 */
int nrf_provisioning_set_interval(int interval);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* NRF_PROVISIONING_H__ */
