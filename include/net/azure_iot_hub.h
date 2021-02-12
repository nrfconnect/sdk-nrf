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
#include <net/mqtt.h>

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
	/** Connecting to Azure IoT Hub. */
	AZURE_IOT_HUB_EVT_CONNECTING = 0x1,
	/** Connected to Azure IoT Hub. */
	AZURE_IOT_HUB_EVT_CONNECTED,
	/** Connection attempt failed. The reported error code from the
	 *  IoT Hub is available in the data.err member in the event structure.
	 *  The error codes correspond to return codes in MQTT CONNACK
	 *  messages.
	 */
	AZURE_IOT_HUB_EVT_CONNECTION_FAILED,
	/** Azure IoT Hub connection established and ready. */
	AZURE_IOT_HUB_EVT_READY,
	/** Disconnected from Azure IoT Hub. */
	AZURE_IOT_HUB_EVT_DISCONNECTED,
	/** Device-bound data received from Azure IoT Hub. */
	AZURE_IOT_HUB_EVT_DATA_RECEIVED,
	/** Acknowledgment for data sent to Azure IoT Hub. */
	AZURE_IOT_HUB_EVT_PUBACK,
	/** Device twin has been received. */
	AZURE_IOT_HUB_EVT_TWIN_RECEIVED,
	/** Device twin has received a desired property update. */
	AZURE_IOT_HUB_EVT_TWIN_DESIRED_RECEIVED,
	/** Device twin update successful. The request ID and status are
	 *  contained in the data.result member of the event structure.
	 */
	AZURE_IOT_HUB_EVT_TWIN_RESULT_SUCCESS,
	/** Device twin update failed. The request ID and status are contained
	 *  in the data.result member of the event structure.
	 */
	AZURE_IOT_HUB_EVT_TWIN_RESULT_FAIL,
	/** Connecting to Device Provisioning Service. */
	AZURE_IOT_HUB_EVT_DPS_CONNECTING,
	/** Device Provisioning Service registration has started. */
	AZURE_IOT_HUB_EVT_DPS_REGISTERING,
	/** Device Provisioning Service is done, and Azure IoT Hub information
	 *  obtained.
	 */
	AZURE_IOT_HUB_EVT_DPS_DONE,
	/** Device Provisioning Service failed to retrieve information to
	 *  connect to Azure IoT Hub.
	 */
	AZURE_IOT_HUB_EVT_DPS_FAILED,
	/** Direct method invoked from the cloud side.
	 *  @note After a direct method has been executed,
	 *  @a azure_iot_hub_method_respond must be called to report back
	 *  the result of the method invocation.
	 */
	AZURE_IOT_HUB_EVT_DIRECT_METHOD,
	/** FOTA download starting. */
	AZURE_IOT_HUB_EVT_FOTA_START,
	/** FOTA update done, reboot required to apply update. */
	AZURE_IOT_HUB_EVT_FOTA_DONE,
	/** FOTA erase pending. On nRF9160-based devices this is typically
	 *  caused by an active LTE connection preventing erase operation.
	 */
	AZURE_IOT_HUB_EVT_FOTA_ERASE_PENDING,
	/** FOTA erase done. */
	AZURE_IOT_HUB_EVT_FOTA_ERASE_DONE,
	/** FOTA failed */
	AZURE_IOT_HUB_EVT_FOTA_ERROR,
	/** Internal library error */
	AZURE_IOT_HUB_EVT_ERROR
};

/** @brief Azure IoT Hub topic type, used to route messages to the correct
 *	   destination.
 */
enum azure_iot_hub_topic_type {
	/** Data received on the devicebound topic. */
	AZURE_IOT_HUB_TOPIC_DEVICEBOUND,
	/** Event topic used to send event data to Azure IoT Hub. */
	AZURE_IOT_HUB_TOPIC_EVENT,
	/** Received "desired" properties from the device twin. */
	AZURE_IOT_HUB_TOPIC_TWIN_DESIRED,
	/** Send updates to the "reported" properties in the device twin. */
	AZURE_IOT_HUB_TOPIC_TWIN_REPORTED,
	/** Topic to request to receive the device twin. */
	AZURE_IOT_HUB_TOPIC_TWIN_REQUEST,
};

/** @brief Property bag structure for key/value string pairs. Per Azure
 *	   IoT Hub documentation, the key must be defined, while the
 *	   value can be a string, an empty string or NULL.
 *
 *  @note If value is provided as a string, it's the equivalent to "key=value".
 *	  If the value is an empty string (only null-terminator), it's the
 *	  equivalent of "key=". If value is NULL, it's the equivalent of
 *	  "key".
 */
struct azure_iot_hub_prop_bag {
	/** Null-terminated key string. */
	char *key;
	/** Null-terminated value string. */
	char *value;
};

/** @brief Azure IoT Hub topic data. */
struct azure_iot_hub_topic_data {
	/** Topic type. */
	enum azure_iot_hub_topic_type type;
	/** Pointer to topic name. */
	char *str;
	/** Length of topic name. */
	size_t len;
	/* Array of property bags. */
	struct azure_iot_hub_prop_bag *prop_bag;
	/* Number of property bag elements in the prop_bag array. */
	size_t prop_bag_count;
};

/** @brief Azure IoT Hub transmission data. */
struct azure_iot_hub_data {
	/** Topic data is sent/received on. */
	struct azure_iot_hub_topic_data topic;
	/** Pointer to data sent/received from Azure IoT Hub. */
	char *ptr;
	/** Length of data. */
	size_t len;
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
	/** Method name, null-terminated string. */
	const char *name;
	/** Method request ID, null-terminated string. */
	char *rid;
	/** Method payload. */
	const char *payload;
	/** Method payload length. */
	size_t payload_len;
};

/** @brief Azure IoT Hub result structure.
 *
 *  @details Used to signal result of direct method execution from device to
 *	     cloud, and to receive result of device twin updates (twin updates
 *	     sent from the device will receive a result message back from the
 *	     cloud with success or failure).
 */
struct azure_iot_hub_result {
	/** Request ID to which the result belongs, null-terminated string. */
	char *rid;
	/** Status code. */
	uint32_t status;
	/** Result payload. */
	const char *payload;
	/** Size of payload. */
	size_t payload_len;
};

/** @brief Struct with data received from Azure IoT Hub. */
struct azure_iot_hub_evt {
	/** Type of event. */
	enum azure_iot_hub_evt_type type;
	union {
		struct azure_iot_hub_data msg;
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
	/** Socket for the Azure IoT Hub connection. Polling on the socket
	 *  happens internally in the library.
	 */
	int socket;
	/** Device id for the Azure IoT Hub connection.
	 *  If NULL, the device ID provided by Kconfig is used.
	 */
	char *device_id;
	/** Length of device string. */
	size_t device_id_len;
};

/** @brief Initialize the module.
 *
 *  @warning This API must be called exactly once, and it must return
 *           successfully for subsequent calls to this library to succeed.
 *
 *  @param[in] config Pointer to struct containing connection parameters. Can be
 *		      NULL if @option{CONFIG_AZURE_IOT_HUB_DEVICE_ID} is set.
 *  @param[in] event_handler Pointer to event handler function.
 *
 *  @retval 0 If successful. Otherwise, a (negative) error code is returned.
 */
int azure_iot_hub_init(const struct azure_iot_hub_config *config,
		       azure_iot_hub_evt_handler_t event_handler);

/** @brief Establish connection to Azure IoT Hub. The function blocks
 *	   until a connection to the hub is established on the transport level.
 *	   Subsequent calls to other library function should await
 *	   AZURE_IOT_HUB_EVT_CONNECTED and AZURE_IOT_HUB_EVT_READY events.
 *
 *  @retval 0 If successful.
 *            Otherwise, a (negative) error code is returned.
 */
int azure_iot_hub_connect(void);

/** @brief Disconnect from Azure IoT Hub. Calling this function initiates the
 *	   disconnection procedure, and the event AZURE_IOT_HUB_EVT_DISCONNECTED
 *	   is received when it is completed.
 *
 *  @retval 0 If successful.
 *            Otherwise, a (negative) error code is returned.
 */
int azure_iot_hub_disconnect(void);

/** @brief Send data to Azure IoT Hub.
 *
 *  @param[in] tx_data Pointer to struct containing data to be transmitted to
 *                     Azure IoT Hub.
 *
 *  @retval 0 If successful.
 *            Otherwise, a (negative) error code is returned.
 */
int azure_iot_hub_send(const struct azure_iot_hub_data *const tx_data);

/** @brief Receive incoming data from Azure IoT Hub.
 *
 *  @details Implicitly this function calls a non-blocking recv() on the
 *	     socket with IoT hub connection. It can be polled periodically,
 *	     but by default this functionality is handled internally in the
 *	     library.
 *
 *  @retval 0 If successful.
 *            Otherwise, a (negative) error code is returned.
 */
int azure_iot_hub_input(void);

/** @brief Ping Azure IoT Hub. Must be called periodically to keep connection
 *	   alive. By default ping is handled internally in the library.
 *
 *  @retval 0 If successful.
 *            Otherwise, a (negative) error code is returned.
 */
int azure_iot_hub_ping(void);

/** @brief Send response to a direct method invoked from the cloud.
 *
 *  @param[in] result Structure containing result data from the direct method
 *		      execution.
 *
 *  @retval 0 If successful.
 *          Otherwise, a (negative) error code is returned.
 */
int azure_iot_hub_method_respond(struct azure_iot_hub_result *result);

/** @brief Helper function to determine when next ping should be sent in order
 *	   to keep the connection alive. By default, ping is sent in time
 *	   automatically by the library.
 *
 *  @return Time in milliseconds until next keepalive ping will be sent.
 *  @return -1 if keepalive messages are not enabled.
 */
int azure_iot_hub_keepalive_time_left(void);

#ifdef __cplusplus
}
#endif

/**
 *@}
 */

#endif /* AZURE_IOT_HUB__ */
