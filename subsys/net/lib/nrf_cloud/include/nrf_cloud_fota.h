/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NRF_CLOUD_FOTA_H__
#define NRF_CLOUD_FOTA_H__

#include <net/mqtt.h>
#include <bluetooth/bluetooth.h>

#ifdef __cplusplus
extern "C" {
#endif

/**@brief FOTA update type. */
enum nrf_cloud_fota_type {
	NRF_CLOUD_FOTA_TYPE__FIRST = 0,

	/** Application update. */
	NRF_CLOUD_FOTA_APPLICATION = NRF_CLOUD_FOTA_TYPE__FIRST,
	/** Modem update. */
	NRF_CLOUD_FOTA_MODEM = 1,
	/** Bootloader update. */
	NRF_CLOUD_FOTA_BOOTLOADER = 2,

	NRF_CLOUD_FOTA_TYPE__INVALID
};

/**@brief FOTA status reported to nRF Cloud. */
enum nrf_cloud_fota_status {
	NRF_CLOUD_FOTA_QUEUED = 0,
	NRF_CLOUD_FOTA_IN_PROGRESS = 1,
	NRF_CLOUD_FOTA_FAILED = 2,
	NRF_CLOUD_FOTA_SUCCEEDED = 3,
	NRF_CLOUD_FOTA_TIMED_OUT = 4,
	NRF_CLOUD_FOTA_CANCELED = 5,
	NRF_CLOUD_FOTA_REJECTED = 6,
	NRF_CLOUD_FOTA_DOWNLOADING = 7,
};

/**@brief FOTA event type provided to callback. */
enum nrf_cloud_fota_evt_id {
	NRF_CLOUD_FOTA_EVT_START,
	NRF_CLOUD_FOTA_EVT_DONE,
	NRF_CLOUD_FOTA_EVT_ERROR,
	NRF_CLOUD_FOTA_EVT_ERASE_PENDING,
	NRF_CLOUD_FOTA_EVT_ERASE_DONE,
	NRF_CLOUD_FOTA_EVT_DL_PROGRESS,
	NRF_CLOUD_FOTA_EVT_BLE_JOB_RCVD,
};

/**@brief FOTA error detail. */
enum nrf_cloud_fota_error {
	/** No error. */
	NRF_CLOUD_FOTA_ERROR_NONE,
	/** Unable to connect to file server/start the download. */
	NRF_CLOUD_FOTA_ERROR_DOWNLOAD_START,
	/** Error during file download. */
	NRF_CLOUD_FOTA_ERROR_DOWNLOAD,
	/** Unable to validate that the firmware was properly installed. */
	NRF_CLOUD_FOTA_ERROR_UNABLE_TO_VALIDATE,
	/** Error applying the update. */
	NRF_CLOUD_FOTA_ERROR_APPLY_FAIL,
};

/**@brief Common FOTA job info */
struct nrf_cloud_fota_job_info {
	enum nrf_cloud_fota_type type;
	char *id;
	char *host;
	char *path;
	int file_size;
};

/**@brief FOTA info for a BLE job */
struct nrf_cloud_fota_ble_job {
	bt_addr_t ble_id;
	struct nrf_cloud_fota_job_info info;
	enum nrf_cloud_fota_error error;
	const int dl_progress; /* Download progress percent, 0-100. */
};

/**@brief FOTA event data provided to @ref nrf_cloud_fota_callback_t */
struct nrf_cloud_fota_evt {
	enum nrf_cloud_fota_evt_id id;

	enum nrf_cloud_fota_status status;
	enum nrf_cloud_fota_type type;
	union {
		enum nrf_cloud_fota_error error;
		/** Download progress percent, 0-100. */
		int dl_progress;
	} evt_data;
};

/**
 * @brief  Event handler registered with the nRF Cloud FOTA module to handle
 *  asynchronous events.
 *
 * @param[in]  evt The FOTA event and any associated parameters.
 */
typedef void (*nrf_cloud_fota_callback_t)
	(const struct nrf_cloud_fota_evt * const evt);

/**
 * @brief  BLE FOTA handler registered with the nRF Cloud FOTA module to handle
 *  asynchronous events.
 *
 * @warning The memory pointed to by ble_job will be freed after the handler
 * has returned.
 *
 * @param[in]  ble_job The BLE FOTA job.
 */
typedef void (*nrf_cloud_fota_ble_callback_t)
	(const struct nrf_cloud_fota_ble_job * const ble_job);

/**
 * @brief Initialize the nRF Cloud FOTA module.
 *
 * @warning This API must be called prior to using nRF Cloud FOTA and it must
 * return successfully.
 *
 * @param[in] cb FOTA event handler.
 *
 * @retval 0 If successful.
 *         1 If successful and a prior FOTA event has been completed.
 *           Otherwise, a (negative) error code is returned.
 */
int nrf_cloud_fota_init(nrf_cloud_fota_callback_t cb);

/**@brief Handler for nRF Cloud FOTA MQTT events.
 *
 * @param _mqtt_evt Pointer to the recived mqtt_evt.
 *
 * @retval 0 FOTA event, the application should not process this event.
 * @retval 1 Not a FOTA event, application should process this event.
 * @return   A negative value on error.
 */
int nrf_cloud_fota_mqtt_evt_handler(const struct mqtt_evt *_mqtt_evt);

/**@brief Set the information required for MQTT transactions and
 *        report status of saved job (if present) to nRF Cloud.
 *        This should be used during initialization.
 * @param client      Pointer to the MQTT client instance.
 * @param client_id   Null terminated client id of this device.
 * @param endpoint    User and device specific MQTT endpoint.
 *
 * @retval 0 If successful.
 *           Otherwise, a (negative) error code is returned.
 */
int nrf_cloud_fota_endpoint_set_and_report(struct mqtt_client *const client,
	const char * const client_id, const struct mqtt_utf8 * const endpoint);

/**@brief Set the information required for MQTT transactions.
 *
 * @param client      Pointer to the MQTT client instance.
 * @param client_id   Null terminated client id of this device.
 * @param endpoint    User and device specific MQTT endpoint.
 *
 * @retval 0 If successful.
 *           Otherwise, a (negative) error code is returned.
 */
int nrf_cloud_fota_endpoint_set(struct mqtt_client *const client,
				const char * const client_id,
				const struct mqtt_utf8 * const endpoint);

/**@brief Clear the information required for MQTT transactions. */
void nrf_cloud_fota_endpoint_clear(void);

/**@brief Subscribe to FOTA related MQTT topics. */
int nrf_cloud_fota_subscribe(void);

/**@brief Unsubscribe from FOTA related MQTT topics. */
int nrf_cloud_fota_unsubscribe(void);

/**@brief Check for pending FOTA updates. */
int nrf_cloud_fota_update_check(void);

/**
 * @brief Set callback for BLE FOTA.
 *
 * @param[in] cb BLE FOTA event handler.
 *
 * @retval 0 If successful.
 *           Otherwise, a (negative) error code is returned.
 */
int nrf_cloud_fota_ble_set_handler(nrf_cloud_fota_ble_callback_t cb);

/**
 * @brief Check for a pending FOTA update for the specified BLE device.
 *
 * @param[in] ble_id BLE device ID.
 *
 * @retval 0 If successful.
 *           Otherwise, a (negative) error code is returned.
 */
int nrf_cloud_fota_ble_update_check(const bt_addr_t * const ble_id);

/**
 * @brief Update the status of a BLE FOTA job.
 *
 * @param[in] ble_job BLE job info.
 * @param[in] status  BLE job.
 *
 * @retval 0 If successful.
 *           Otherwise, a (negative) error code is returned.
 */
int nrf_cloud_fota_ble_job_update(
	const struct nrf_cloud_fota_ble_job * const ble_job,
	const enum nrf_cloud_fota_status status);

#ifdef __cplusplus
}
#endif

#endif /* NRF_CLOUD_FOTA_H__ */
