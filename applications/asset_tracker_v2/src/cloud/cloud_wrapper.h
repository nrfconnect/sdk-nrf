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

#include <zephyr/kernel.h>
#include <stdbool.h>

#if defined(CONFIG_LWM2M)
#include <zephyr/net/lwm2m.h>
#else
#include "cloud/cloud_codec/lwm2m/lwm2m_dummy.h"
#endif
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
	/** Event received when data has been acknowledged by cloud. */
	CLOUD_WRAP_EVT_DATA_ACK,
	/** Event received when a ping response has been received. */
	CLOUD_WRAP_EVT_PING_ACK,
	/** A-GNSS data received from the cloud integration layer.
	 *  Payload is of type @ref cloud_wrap_event_data.
	 */
	CLOUD_WRAP_EVT_AGNSS_DATA_RECEIVED,
	/** P-GPS data received from the cloud integration layer.
	 *  Payload is of type @ref cloud_wrap_event_data.
	 */
	CLOUD_WRAP_EVT_PGPS_DATA_RECEIVED,
	/** Data received from cloud integration layer.
	 *  Payload is of type @ref cloud_wrap_event_data.
	 */
	CLOUD_WRAP_EVT_CLOUD_LOCATION_RESULT_RECEIVED,
	/** Reboot request received from cloud. */
	CLOUD_WRAP_EVT_REBOOT_REQUEST,
	/** Request to connect to LTE. */
	CLOUD_WRAP_EVT_LTE_CONNECT_REQUEST,
	/** Request to disconnect from LTE. */
	CLOUD_WRAP_EVT_LTE_DISCONNECT_REQUEST,
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

/** @brief Structure used to reference application data that is sent and received
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
		uint16_t message_id;
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
 * @param[in] ack Flag signifying if the message should be acknowledged or not.
 * @param[in] id Message ID.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int cloud_wrap_state_get(bool ack, uint32_t id);

/**
 * @brief Send device state data to cloud.
 *
 * @param[in] buf Pointer to buffer containing data to be sent.
 * @param[in] len Length of buffer.
 * @param[in] ack Flag signifying if the message should be acknowledged or not.
 * @param[in] id Message ID.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int cloud_wrap_state_send(char *buf, size_t len, bool ack, uint32_t id);

/**
 * @brief Send data to cloud.
 *
 * @param[in] buf Pointer to buffer containing data to be sent.
 * @param[in] len Length of buffer.
 * @param[in] ack Flag signifying if the message should be acknowledged or not.
 * @param[in] id Message ID.
 * @param[in] path_list List of LwM2M objects to be sent.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int cloud_wrap_data_send(char *buf, size_t len, bool ack, uint32_t id,
			 const struct lwm2m_obj_path path_list[]);

/**
 * @brief Send batched data to cloud.
 *
 * @param[in] buf Pointer to buffer containing data to be sent.
 * @param[in] len Length of buffer.
 * @param[in] ack Flag signifying if the message should be acknowledged or not.
 * @param[in] id Message ID.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int cloud_wrap_batch_send(char *buf, size_t len, bool ack, uint32_t id);

/**
 * @brief Send UI data to cloud.
 *
 * @param[in] buf Pointer to buffer containing data to be sent.
 * @param[in] len Length of buffer.
 * @param[in] ack Flag signifying if the message should be acknowledged or not.
 * @param[in] id Message ID.
 * @param[in] path_list Pointer to list of LwM2M objects to be sent.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int cloud_wrap_ui_send(char *buf, size_t len, bool ack, uint32_t id,
		       const struct lwm2m_obj_path path_list[]);

/**
 * @brief Send cloud location data to cloud.
 *
 * @note LwM2M builds: This function does not require passing in a list of objects, unlike other
 *		       functions in this API. The underlying LwM2M API called when calling this
 *		       function, keeps its own reference of the objects that needs to be updated.
 *
 * @param[in] buf Pointer to buffer containing data to be sent.
 * @param[in] len Length of buffer.
 * @param[in] ack Flag signifying if the message should be acknowledged or not.
 * @param[in] id Message ID.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int cloud_wrap_cloud_location_send(char *buf, size_t len, bool ack, uint32_t id);

/**
 * @brief Indicates whether cloud implementation can and is configured to return the resolved
 *        location back to the device.
 *
 * @return Indicates whether resolved location is sent back to the device.
 */
bool cloud_wrap_cloud_location_response_wait(void);

/**
 * @brief Send A-GNSS request to cloud.
 *
 * @note LwM2M builds: This function does not require passing in a list of objects, unlike other
 *		       functions in this API. The underlying LwM2M API called when calling this
 *		       function, keeps its own reference of the objects that needs to be updated.
 *
 * @param[in] buf Pointer to buffer containing data to be sent.
 * @param[in] len Length of buffer.
 * @param[in] ack Flag signifying if the message should be acknowledged or not.
 * @param[in] id Message ID.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int cloud_wrap_agnss_request_send(char *buf, size_t len, bool ack, uint32_t id);

/**
 * @brief Send P-GPS request to cloud.
 *
 * @note LwM2M builds: This function does not require passing in a list of objects, unlike other
 *		       functions in this API. The underlying LwM2M API called when calling this
 *		       function, keeps its own reference of the objects that needs to be updated.
 *
 * @param[in] buf Pointer to buffer containing data to be sent.
 * @param[in] len Length of buffer.
 * @param[in] ack Flag signifying if the message should be acknowledged or not.
 * @param[in] id Message ID.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int cloud_wrap_pgps_request_send(char *buf, size_t len, bool ack, uint32_t id);

/**
 * @brief Send Memfault data to cloud.
 *
 * @param[in] buf Pointer to buffer containing data to be sent.
 * @param[in] len Length of buffer.
 * @param[in] ack Flag signifying if the message should be acknowledged or not.
 * @param[in] id Message ID.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int cloud_wrap_memfault_data_send(char *buf, size_t len, bool ack, uint32_t id);

#ifdef __cplusplus
}
#endif

/**
 *@}
 */

#endif /* CLOUD_WRAPPER_H__ */
