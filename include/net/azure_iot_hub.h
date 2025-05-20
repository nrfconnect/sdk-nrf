/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**@file
 * @brief Azure IoT Hub library.
 */

#ifndef AZURE_IOT_HUB__
#define AZURE_IOT_HUB__

#include <stdio.h>
#include <zephyr/net/mqtt.h>

/**
 * @defgroup azure_iot_hub Azure IoT Hub library
 * @{
 * @brief Library to connect a device to Azure IoT Hub.
 */

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Azure IoT Hub notification events used to notify the user. */
enum azure_iot_hub_evt_type {
	/** Connecting to Azure IoT Hub.
	 *  The event has no associated data.
	 */
	AZURE_IOT_HUB_EVT_CONNECTING = 0x1,

	/** Connected to Azure IoT Hub.
	 *  The event has no associated data.
	 */
	AZURE_IOT_HUB_EVT_CONNECTED,

	/** Connection attempt failed. The reported error code from the
	 *  IoT Hub is available in the ``data.err`` member in the event structure.
	 *  The error codes correspond to return codes in MQTT CONNACK messages.
	 */
	AZURE_IOT_HUB_EVT_CONNECTION_FAILED,

	/** Azure IoT Hub connection established and ready to receive data.
	 *  The event has no associated data.
	 */
	AZURE_IOT_HUB_EVT_READY,

	/** Disconnected from Azure IoT Hub.
	 *  The event has no associated data.
	 */
	AZURE_IOT_HUB_EVT_DISCONNECTED,

	/** Device-bound data received from Azure IoT Hub.
	 *  The event contains the received data in the ``data.msg`` member, and the topic it was
	 *  received on in the ``topic`` member, including message properties in
	 *  ``topic.properties``.
	 */
	AZURE_IOT_HUB_EVT_DATA_RECEIVED,

	/** Acknowledgment for data sent to Azure IoT Hub.
	 *  The acknowledged message ID is in ``data.message_id``.
	 */
	AZURE_IOT_HUB_EVT_PUBACK,

	/** Acknowledgment for ping request message sent to Azure IoT Hub.
	 *  The event has no associated data.
	 */
	AZURE_IOT_HUB_EVT_PINGRESP,

	/** Device twin has been received.
	 *  The event contains the received data in the ``data.msg`` member, and the topic it was
	 *  received on in the ``topic`` member, including message properties in
	 *  ``topic.properties``.
	 */
	AZURE_IOT_HUB_EVT_TWIN_RECEIVED,

	/** Device twin has received a desired property update.
	 *  The event contains the received data in the ``data.msg`` member, and the topic it was
	 *  received on in the ``topic`` member, including message properties in
	 *  ``topic.properties``.
	 */
	AZURE_IOT_HUB_EVT_TWIN_DESIRED_RECEIVED,

	/** Device twin update successful. The request ID and status are
	 *  contained in the data.result member of the event structure.
	 *  The received payload is in the ``data.msg`` member, and the topic it was
	 *  received on is in the ``topic`` member.
	 */
	AZURE_IOT_HUB_EVT_TWIN_RESULT_SUCCESS,

	/** Device twin update failed. The request ID and status are contained
	 *  in the ``data.result`` member of the event structure.
	 */
	AZURE_IOT_HUB_EVT_TWIN_RESULT_FAIL,

	/** Direct method invoked from the cloud side.

	 *  The event contains the method data in the ``data.method`` member, and the topic it was
	 *  received on in the ``topic`` member.
	 *
	 *  @note After a direct method has been executed, @a azure_iot_hub_method_respond must be
	 *	  called to report back the result of the method invocation.
	 */
	AZURE_IOT_HUB_EVT_DIRECT_METHOD,

	/** FOTA download starting.
	 *  The event has no associated data.
	 */
	AZURE_IOT_HUB_EVT_FOTA_START,

	/** FOTA update done, reboot required to apply update.
	 *  The event has no associated data.
	 */
	AZURE_IOT_HUB_EVT_FOTA_DONE,

	/** FOTA erase pending. On nRF9160-based devices this is typically
	 *  caused by an active LTE connection preventing erase operation.
	 *  The event has no associated data.
	 */
	AZURE_IOT_HUB_EVT_FOTA_ERASE_PENDING,

	/** FOTA erase done.
	 *  The event has no associated data.
	 */
	AZURE_IOT_HUB_EVT_FOTA_ERASE_DONE,

	/** FOTA failed.
	 *  The event has no associated data.
	 */
	AZURE_IOT_HUB_EVT_FOTA_ERROR,

	/** A received message is too large for the payload buffer and cannot be processed.
	 *  The event has no associated data.
	 */
	AZURE_IOT_HUB_EVT_ERROR_MSG_SIZE,

	/** Internal library error.
	 *  The event has no associated data.
	 */
	AZURE_IOT_HUB_EVT_ERROR
};

/** @brief Azure IoT Hub topic type, used to route messages to the correct
 *	   destination.
 */
enum azure_iot_hub_topic_type {
	/** Data received on the devicebound topic. */
	AZURE_IOT_HUB_TOPIC_DEVICEBOUND,

	/** Data received on the direct method topic. */
	AZURE_IOT_HUB_TOPIC_DIRECT_METHOD,

	/** Received "desired" properties from the device twin. */
	AZURE_IOT_HUB_TOPIC_TWIN_DESIRED,

	/** Topic to receive twin request result messages.
	 *  Both success and error messages are received on the same topic.
	 */
	AZURE_IOT_HUB_TOPIC_TWIN_REQUEST_RESULT,

	/** Topic to receive Device Provisioning Service (DPS) messages. */
	AZURE_IOT_HUB_TOPIC_DPS,

	/** Event topic used to send event data to Azure IoT Hub. */
	AZURE_IOT_HUB_TOPIC_EVENT,

	/** Send updates to the "reported" properties in the device twin. */
	AZURE_IOT_HUB_TOPIC_TWIN_REPORTED,

	/** Topic to request to receive the device twin. */
	AZURE_IOT_HUB_TOPIC_TWIN_REQUEST,

	/** Indicates that the message was received on an unknown topic. */
	AZURE_IOT_HUB_TOPIC_UNKNOWN,
};

/** Buffer to store data together with buffer size. */
struct azure_iot_hub_buf {
	/* Pointer to buffer. */
	char *ptr;

	/* Size of buffer. */
	size_t size;
};

/** @brief Property bag structure for key/value string pairs. Per Azure
 *	   IoT Hub documentation, the key must be defined, while the
 *	   value can be a string or empty.
 *
 *  @note If the value is provided as a string, it is the equivalent to "key=value".
 *	  If the value is empty, it is the equivalent of "key=" or "key".
 */
struct azure_iot_hub_property {
	/** Property key. */
	struct azure_iot_hub_buf key;

	/** Property value. */
	struct azure_iot_hub_buf value;
};

/** @brief Azure IoT Hub topic data. */
struct azure_iot_hub_topic_data {
	/** Topic type. */
	enum azure_iot_hub_topic_type type;

	/** Topic name ptr and size. */
	struct azure_iot_hub_buf name;

	/* Array of property bags. */
	struct azure_iot_hub_property *properties;

	/* Number of property bag elements in the properties array. */
	size_t property_count;
};

/** @brief Azure IoT Hub transmission data. */
struct azure_iot_hub_msg {
	/** Topic data is sent/received on. */
	struct azure_iot_hub_topic_data topic;

	/** Pointer to the payload sent/received from Azure IoT Hub. */
	struct azure_iot_hub_buf payload;

	/** Request ID that can be populated if it is relevant for the message type. */
	struct azure_iot_hub_buf request_id;

	/** Quality of Service for the message. */
	enum mqtt_qos qos;

	/** Message id used for the message, used to match acknowledgments. */
	uint16_t message_id;

	/** Duplicate flag. 1 indicates the message is a retransmission,
	 *  Usually triggered by missing publication acknowledgment.
	 */
	uint8_t dup_flag;

	/** Retain flag. 1 indicates to the IoT hub that the message should be
	 *  stored persistently.
	 */
	uint8_t retain_flag;
};

/** @brief Azure IoT Hub direct method data. */
struct azure_iot_hub_method {
	/** Method name. */
	struct azure_iot_hub_buf name;

	/** Method request ID. */
	struct azure_iot_hub_buf request_id;

	/** Method payload. */
	struct azure_iot_hub_buf payload;
};

/** @brief Azure IoT Hub result structure.
 *
 *  @details Used to signal result of direct method execution from device to
 *	     cloud, and to receive result of device twin updates (twin updates
 *	     sent from the device will receive a result message back from the
 *	     cloud with success or failure).
 */
struct azure_iot_hub_result {
	/** Request ID to which the result belongs. */
	struct azure_iot_hub_buf request_id;

	/** Status code. */
	uint32_t status;

	/** Result payload. */
	struct azure_iot_hub_buf payload;
};

/** @brief Struct with data received from Azure IoT Hub. */
struct azure_iot_hub_evt {
	/** Type of event. */
	enum azure_iot_hub_evt_type type;
	union {
		struct azure_iot_hub_msg msg;
		struct azure_iot_hub_method method;
		struct azure_iot_hub_result result;
		int err;
		bool persistent_session;
		uint16_t message_id;
	} data;
	/* Associated topic, not relevant for all events. */
	struct azure_iot_hub_topic_data topic;
};

/** @brief Azure IoT Hub library event handler.
 *
 *  @param evt Pointer to event structure.
 */
typedef void (*azure_iot_hub_evt_handler_t)(struct azure_iot_hub_evt *evt);

/** @brief Structure for Azure IoT Hub connection parameters. */
struct azure_iot_hub_config {
	/** Hostname to IoT Hub to connect to.
	 *  If the buffer size is zero, the device ID provided by
	 *  @kconfig{CONFIG_AZURE_IOT_HUB_HOSTNAME} is used. If DPS is enabled and `use_dps` is
	 *  set to true, the provided hostname is ignored.
	 */
	struct azure_iot_hub_buf hostname;
	/** Device id for the Azure IoT Hub connection.
	 *  If the buffer size is zero, the device ID provided by Kconfig is used.
	 */
	struct azure_iot_hub_buf device_id;

	/** Use DPS to obtain hostname and device ID if true.
	 *  Using DPS requires that @kconfig{CONFIG_AZURE_IOT_HUB_DPS} is enabled and DPS
	 *  configured accordingly.
	 *  If a hostname and device ID have already been obtained previously, the stored values
	 *  will be used. To re-run DPS, the DPS information must be reset first.
	 *  Note that using this option will use the device ID as DPS registration ID and the
	 *  ID cope from @kconfig{CONFIG_AZURE_IOT_HUB_DPS_ID_SCOPE}.
	 *  For more fine-grained control over DPS, use the azure_iot_hub_dps APIs directly instead.
	 */
	bool use_dps;
};

/** @brief Initialize the module.
 *
 *  @note This API must be called exactly once, and it must return
 *	  successfully for subsequent calls to this library to succeed.
 *
 *  @param[in] event_handler Pointer to event handler function.
 *
 *  @retval 0 If successful.
 *  @retval -EALREADY if the library has already been initialized and is in a state where it can
 *		      not be re-initialized.
 *  @retval -EINVAL in invalid argument was provided.
 */
int azure_iot_hub_init(azure_iot_hub_evt_handler_t event_handler);

/** @brief Establish connection to Azure IoT Hub. The function blocks
 *	   until a connection to the hub is established on the transport level.
 *	   Subsequent calls to other library function should await
 *	   AZURE_IOT_HUB_EVT_CONNECTED and AZURE_IOT_HUB_EVT_READY events.
 *
 *  @param[in] config Pointer to struct containing connection parameters. If NULL, values from
 *		      Kconfig will be used instead.
 *
 *  @retval 0 If successful.
 *  @retval -EALREADY if the device is already connected to an IoT Hub.
 *  @retval -EINPROGRESS if a connection attempt is already in progress.
 *  @retval -ENOENT if the library is not in disconnected state.
 *  @retval -EINVAL if the provided configuration is invalid.
 *  @retval -EMSGSIZE if the provided device ID is larger than the internal buffer size.
 *  @retval -EFAULT if there was an internal error in the library.
 */
int azure_iot_hub_connect(const struct azure_iot_hub_config *config);

/** @brief Disconnect from Azure IoT Hub. Calling this function initiates the
 *	   disconnection procedure, and the event AZURE_IOT_HUB_EVT_DISCONNECTED
 *	   is received when it is completed.
 *
 *  @retval 0 If successful.
 *  @retval -ENOTCONN if the device is not connected to an IoT Hub.
 *  @retval -ENXIO if the MQTT library reported an error.
 */
int azure_iot_hub_disconnect(void);

/** @brief Send data to Azure IoT Hub.
 *
 *  @param[in] tx_data Pointer to struct containing data to be transmitted to
 *                     Azure IoT Hub.
 *
 *  @retval 0 If successful.
 *  @retval -EINVAL if a NULL pointer was provided.
 *  @retval -ENOMSG if the provided message topic was invalid.
 *  @retval -EMSGSIZE an internal buffer is too small to hold the topic data. This can for instance
 *		      happen if message properties are in use, as they are appended to the topic.
 *  @retval -ENOTCONN if the device is not connected to an IoT Hub.
 *  @retval -ENOMEM if the request ID buffer was insufficient to create the ID.
 *  @retval -EFAULT if there was an internal error in the library.
 */
int azure_iot_hub_send(const struct azure_iot_hub_msg *const tx_data);

/** @brief Send response to a direct method invoked from the cloud.
 *
 *  @param[in] result Structure containing result data from the direct method
 *		      execution.
 *
 *  @retval 0 If successful.
 *  @retval -EINVAL if a NULL pointer was provided.
 *  @retval -ENOTCONN if there was no IoT hub connection.
 *  @retval -EFAULT if there was an error when creating the response message.
 *  @retval -ENXIO if the MQTT library reported an error when publishing the response.
 */
int azure_iot_hub_method_respond(struct azure_iot_hub_result *result);

#ifdef __cplusplus
}
#endif

/**
 *@}
 */

#endif /* AZURE_IOT_HUB__ */
