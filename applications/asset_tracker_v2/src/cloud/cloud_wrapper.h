/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**@file
 *@brief Cloud wrapper library header.
 */

#ifndef CLOUD_WRAPPER_H__
#define CLOUD_WRAPPER_H__

#include <zephyr.h>

/**
 * @defgroup cloud_wrapper Cloud wrapper library
 * @{
 * @brief A Library that exposes generic functionality of cloud integration layers.
 */

#ifdef __cplusplus
extern "C" {
#endif

/** Event types notified by the cloud wrapper API. */
enum cloud_wrap_event_type {
	/** Cloud integration layer is connecting. */
	CLOUD_WRAP_EVT_CONNECTING,
	/** Cloud integration layer is connected. */
	CLOUD_WRAP_EVT_CONNECTED,
	/** Cloud integration layer is disconnected. */
	CLOUD_WRAP_EVT_DISCONNECTED,
	/** Data received from cloud integration layer.
	 *  Payload is of type @ref cloud_wrap_event_data.
	 */
	CLOUD_WRAP_EVT_DATA_RECEIVED,
	/** User association request received from cloud. */
	CLOUD_WRAP_EVT_USER_ASSOCIATION_REQUEST,
	/** User association completed. */
	CLOUD_WRAP_EVT_USER_ASSOCIATED,
	/** A-GPS data received from the cloud integration layer.
	 *  Payload is of type @ref cloud_wrap_event_data.
	 */
	CLOUD_WRAP_EVT_AGPS_DATA_RECEIVED,
	/** P-GPS data received from the cloud integration layer.
	 *  Payload is of type @ref cloud_wrap_event_data.
	 */
	CLOUD_WRAP_EVT_PGPS_DATA_RECEIVED,
	/** Cloud integration layer has successfully performed a FOTA update.
	 *  Device should now be rebooted.
	 */
	CLOUD_WRAP_EVT_FOTA_DONE,
	/** The cloud integration layer has started a FOTA update. */
	CLOUD_WRAP_EVT_FOTA_START,
	/** An image erase is pending. */
	CLOUD_WRAP_EVT_FOTA_ERASE_PENDING,
	/** Image erase done. */
	CLOUD_WRAP_EVT_FOTA_ERASE_DONE,
	/** An error occurred during FOTA. */
	CLOUD_WRAP_EVT_FOTA_ERROR,
	/** An irrecoverable error has occurred in the integration layer. Error details are
	 *  attached in the event structure.
	 */
	CLOUD_WRAP_EVT_ERROR,
};

/** @brief Structure used to referance application data that is sent and received
 *         from the cloud wrapper library.
 */
struct cloud_wrap_event_data {
	/** Pointer to data. */
	char *buf;
	/** Length of data. */
	size_t len;
};

/** @brief Cloud wrapper API event. */
struct cloud_wrap_event {
	enum cloud_wrap_event_type type;

	union {
		/** Structured that contains data received from the cloud integration layer. */
		struct cloud_wrap_event_data data;
		/** Error code signifying the cause of error. */
		int err;
	};
};

/**
 * @brief Cloud wrapper library asynchronous event handler.
 *
 * @param[in] evt Pointer to the event structure.
 */
typedef void (*cloud_wrap_evt_handler_t)(const struct cloud_wrap_event *evt);

/**
 * @brief Setup and initialize the configured cloud integration layer.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int cloud_wrap_init(cloud_wrap_evt_handler_t event_handler);

/**
 * @brief Connect to cloud.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int cloud_wrap_connect(void);

/**
 * @brief Disconnect from cloud
 *
 * @return 0 on success, or a negative error code on failure.
 */
int cloud_wrap_disconnect(void);

/**
 * @brief Request device state from cloud. The device state contains the device configuration.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int cloud_wrap_state_get(void);

/**
 * @brief Send device state data to cloud.
 *
 * @param[in] buf Pointer to buffer containing data to be sent.
 * @param[in] len Length of buffer.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int cloud_wrap_state_send(char *buf, size_t len);

/**
 * @brief Send data to cloud.
 *
 * @param[in] buf Pointer to buffer containing data to be sent.
 * @param[in] len Length of buffer.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int cloud_wrap_data_send(char *buf, size_t len);

/**
 * @brief Send batched data to cloud.
 *
 * @param[in] buf Pointer to buffer containing data to be sent.
 * @param[in] len Length of buffer.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int cloud_wrap_batch_send(char *buf, size_t len);

/**
 * @brief Send UI data to cloud.
 *
 * @param[in] buf Pointer to buffer containing data to be sent.
 * @param[in] len Length of buffer.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int cloud_wrap_ui_send(char *buf, size_t len);

/**
 * @brief Send neighbor cell data to cloud.
 *
 * @param[in] buf Pointer to buffer containing data to be sent.
 * @param[in] len Length of buffer.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int cloud_wrap_neighbor_cells_send(char *buf, size_t len);

/**
 * @brief Send A-GPS request to cloud.
 *
 * @param[in] buf Pointer to buffer containing data to be sent.
 * @param[in] len Length of buffer.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int cloud_wrap_agps_request_send(char *buf, size_t len);

/**
 * @brief Send P-GPS request to cloud.
 *
 * @param[in] buf Pointer to buffer containing data to be sent.
 * @param[in] len Length of buffer.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int cloud_wrap_pgps_request_send(char *buf, size_t len);

/**
 * @brief Send Memfault data to cloud.
 *
 * @param[in] buf Pointer to buffer containing data to be sent.
 * @param[in] len Length of buffer.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int cloud_wrap_memfault_data_send(char *buf, size_t len);

#ifdef __cplusplus
}
#endif

/**
 *@}
 */

#endif /* CLOUD_WRAPPER_H__ */
