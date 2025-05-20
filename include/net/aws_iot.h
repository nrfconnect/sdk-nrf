/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @brief AWS IoT library header.
 */

#ifndef AWS_IOT_H__
#define AWS_IOT_H__

#include <stdio.h>
#include <zephyr/net/mqtt.h>
#include <dfu/dfu_target.h>

/**
 * @defgroup aws_iot AWS IoT library
 * @{
 * @brief Library used to connect an IP enabled device to the AWS IoT Core MQTT broker.
 */

#ifdef __cplusplus
extern "C" {
#endif

/** @brief AWS IoT shadow received topic types.
 *	   If a message is received on a subscribed shadow topic, the type will be set to the
 *	   corresponding topic.
 */
enum aws_iot_shadow_topic_received_type {
	/** This type is used if the message is published to an application specific topic. */
	AWS_IOT_SHADOW_TOPIC_APPLICATION_SPECIFIC,

	/** This topic type corresponds to
	 *  $aws/things/<thing-name>/shadow/get/accepted.
	 *  When requesting the shadow document and the request is accepted,
	 *  the response is published to this topic.
	 */
	AWS_IOT_SHADOW_TOPIC_GET_ACCEPTED,

	/** This topic type corresponds to
	 *  $aws/things/<thing-name>/shadow/get/rejected.
	 *  When requesting the shadow document and the request is rejected,
	 *  the response is published to this topic.
	 *  The response includes an error code that indicates the reason for rejection.
	 *  The error code can be looked up in the AWS documentation under
	 *  "Device Shadow error messages".
	 */
	AWS_IOT_SHADOW_TOPIC_GET_REJECTED,

	/** This topic type corresponds to
	 *  $aws/things/<thing-name>/shadow/update/delta.
	 *  When there is a mismatch between the state.reported and state.desired shadow properties
	 *  The desired state is published to this topic. These desired properties should be updated
	 *  on the device, and the device should report back its updated state to clear the delta.
	 */
	AWS_IOT_SHADOW_TOPIC_UPDATE_DELTA,

	/** This topic type corresponds to
	 *  $aws/things/<thing-name>/shadow/update/accepted.
	 *  When an update of the device shadow is accepted the device shadow document is
	 *  published to this topic.
	 */
	AWS_IOT_SHADOW_TOPIC_UPDATE_ACCEPTED,

	/** This topic type corresponds to
	 *  $aws/things/<thing-name>/shadow/update/rejected.
	 *  When an update of the device shadow is rejected a response is received on this topic
	 *  with an error code stating the reason of rejection.
	 */
	AWS_IOT_SHADOW_TOPIC_UPDATE_REJECTED,

	/** This topic type corresponds to
	 *  $aws/things/<thing-name>/shadow/delete/accepted.
	 *  When the device shadow is deleted a message is sent to this topic.
	 */
	AWS_IOT_SHADOW_TOPIC_DELETE_ACCEPTED,

	/** This topic type corresponds to
	 *  $aws/things/<thing-name>/shadow/delete/rejected.
	 *  When a deletion of the device shadow is rejected a response is published to this topic
	 *  with an error code stating the reason for rejection.
	 */
	AWS_IOT_SHADOW_TOPIC_DELETE_REJECTED
};

/** @brief AWS IoT shadow topic types.
 *	   Used to address messages to the AWS IoT shadow service.
 */
enum aws_iot_shadow_topic_type {
	/** Unused default value. */
	AWS_IOT_SHADOW_TOPIC_NONE,

	/** This topic type corresponds to
	 *  $aws/things/<thing-name>/shadow/get, publishing an empty string
	 *  to this topic requests the device shadow document.
	 */
	AWS_IOT_SHADOW_TOPIC_GET,

	/** This topic type corresponds to
	 *  $aws/things/<thing-name>/shadow/update, publishing data to this
	 *  topic updates the device shadow document.
	 */
	AWS_IOT_SHADOW_TOPIC_UPDATE,

	/** This topic type corresponds to
	 *  $aws/things/<thing-name>/shadow/delete, publishing an empty string
	 *  to this topic deletes the device shadow document.
	 */
	AWS_IOT_SHADOW_TOPIC_DELETE
};

/** @brief AWS IoT notification event types, used to signal the application. */
enum aws_iot_evt_type {
	/** Connecting to the broker. */
	AWS_IOT_EVT_CONNECTING = 0x1,

	/** Connected to the broker.
	 *  Payload is of type bool (.data.persistent_session)
	 */
	AWS_IOT_EVT_CONNECTED,

	/** Disconnected from the broker. */
	AWS_IOT_EVT_DISCONNECTED,

	/** Data received from the broker.
	 *  Payload is of type @ref aws_iot_data (.data.msg)
	 */
	AWS_IOT_EVT_DATA_RECEIVED,

	/** Acknowledgment for data sent to the broker.
	 *  Payload is of type uint16_t (.data.message_id)
	 */
	AWS_IOT_EVT_PUBACK,

	/** Acknowledgment for pings sent to the broker. */
	AWS_IOT_EVT_PINGRESP,

	/** FOTA update start. */
	AWS_IOT_EVT_FOTA_START,

	/** FOTA done. Payload of type @ref dfu_target_image_type (.data.image).
	 *
	 *  If the image parameter type is of type DFU_TARGET_IMAGE_TYPE_MCUBOOT the device needs to
	 *  reboot to apply the new application image.
	 *
	 *  If the image parameter type is of type DFU_TARGET_IMAGE_TYPE_MODEM_DELTA the modem
	 *  needs to be reinitialized to apply the new modem image.
	 */
	AWS_IOT_EVT_FOTA_DONE,

	/** FOTA erase pending. */
	AWS_IOT_EVT_FOTA_ERASE_PENDING,

	/** FOTA erase done. */
	AWS_IOT_EVT_FOTA_ERASE_DONE,

	/** FOTA progress notification.
	 *  Payload is of type int (.data.fota_progress)
	 */
	AWS_IOT_EVT_FOTA_DL_PROGRESS,

	/** FOTA error. Something went wrong during the FOTA process, try again. */
	AWS_IOT_EVT_FOTA_ERROR,

	/** Irrecoverable error, if this event is received the device should perform a reboot.
	 *  Payload is of type int (.data.err)
	 */
	AWS_IOT_EVT_ERROR
};

/** @brief AWS IoT topic data. */
struct aws_iot_topic_data {

	union {
		/** Optional: type of shadow topic that will be published to.
		 *  When publishing to a shadow topic this can be set instead of the
		 *  application specific topic below.
		 */
		enum aws_iot_shadow_topic_type type;

		/** Type of shadow topic that the incoming message is received on.
		 *  If a message is received on an application specific topic the type will be set
		 *  to AWS_IOT_SHADOW_TOPIC_NONE.
		 */
		enum aws_iot_shadow_topic_received_type type_received;
	};

	/** Pointer to string of application specific topic. */
	const char *str;

	/** Length of application specific topic. */
	size_t len;
};

/** @brief AWS IoT transmission data. */
struct aws_iot_data {
	/** Topic data is sent/received on. */
	struct aws_iot_topic_data topic;

	/** Pointer to data sent/received from the AWS IoT broker. */
	char *ptr;

	/** Length of data. */
	size_t len;

	/** Quality of Service of the message. */
	enum mqtt_qos qos;

	/** Message id, used to match acknowledgments. */
	uint16_t message_id;

	/** Duplicate flag. 1 indicates the message is a retransmission,
	 *  Usually triggered by missing publication acknowledgment.
	 */
	uint8_t dup_flag;

	/** Retain flag. 1 indicates to AWS IoT that the message should be
	 *  stored persistently.
	 */
	uint8_t retain_flag;
};

/** @brief Struct with data received from AWS IoT broker. */
struct aws_iot_evt {
	/** Type of event. */
	enum aws_iot_evt_type type;
	union {
		/** Structure that contains data that was received from the broker. */
		struct aws_iot_data msg;

		/** Error code that indicates reason of failure when receiving
		 *  the AWS_IOT_EVT_ERROR event.
		 */
		int err;

		/** FOTA progress in percentage. */
		int fota_progress;

		/** Boolean that indicates if a previous MQTT session is being used. */
		bool persistent_session;

		/** Message ID of the message that was acknowledged. */
		uint16_t message_id;

		/** DFU target type, used to determine if a reboot is required after FOTA based on
		 *  the image that was updated.
		 *
		 *  See the documentation for the event AWS_IOT_EVT_FOTA_DONE for more information.
		 */
		enum dfu_target_image_type image;
	} data;
};

/** @brief AWS IoT library asynchronous event handler.
 *
 *  @param[in] evt The event and any associated parameters.
 */
typedef void (*aws_iot_evt_handler_t)(const struct aws_iot_evt *evt);

/** @brief Structure for AWS IoT broker connection parameters. */
struct aws_iot_config {
	/** AWS IoT client ID. Must be NULL terminated.
	 *  If not set (NULL), CONFIG_AWS_IOT_CLIENT_ID_STATIC will be used.
	 */
	char *client_id;

	/** AWS IoT broker hostname. Must be NULL terminated.
	 *  If not set (NULL), CONFIG_AWS_IOT_BROKER_HOST_NAME will be used.
	 */
	char *host_name;
};

/** @brief Initialize the library.
 *
 *  @note This API must be called exactly once, and it must return
 *	  successfully before any other library APIs can be called.
 *
 *  @param[in] event_handler Pointer to an event handler to receive library events.
 *
 *  @retval 0 If successful.
 *  @retval -EINVAL if an invalid parameter is passed in.
 */
int aws_iot_init(aws_iot_evt_handler_t event_handler);

/** @brief Connect to the AWS IoT broker.
 *
 *  @details This function is synchronous and will only return success when the client has connected
 *	     to the broker and all subscriptions (if any) have been acknowledged.
 *	     If these conditions have not completed within CONFIG_AWS_IOT_CONNECT_TIMEOUT_SECONDS,
 *	     the function times out and the application will need to call this function again.
 *
 *  @param[out] config Pointer to a structure that contains connection parameters.
 *		       If this structure is passed in as NULL, static configurations set via
 *		       Kconfig options are used.
 *		       See CONFIG_AWS_IOT_BROKER_HOST_NAME and CONFIG_AWS_IOT_CLIENT_ID_STATIC.
 *
 *  @retval 0 If successful.
 *  @retval -EAGAIN If the client failed to connect within CONFIG_AWS_IOT_CONNECT_TIMEOUT_SECONDS.
 */
int aws_iot_connect(const struct aws_iot_config *const config);

/** @brief Disconnect from the AWS IoT broker.
 *
 *  @retval 0 If successful.
 */
int aws_iot_disconnect(void);

/** @brief Send data to AWS IoT broker.
 *
 *  @param[in] tx_data Pointer to struct containing data to be transmitted to
 *                     the AWS IoT broker.
 *
 *  @retval 0 If successful.
 *  @retval -EINVAL if an invalid parameter is passed in.
 *  @retval -ENODATA if no valid topic nor shadow topic type is passed in.
 */
int aws_iot_send(const struct aws_iot_data *const tx_data);

/** @brief Pass in a list of application specific topics that will be subscribed to
 *	   upon connection to the AWS IoT broker.
 *	   The number of application specific topics that the library can subscribe
 *	   to is limited to 8.
 *
 *  @note This function passes a reference to topics that are defined in the application.
 *	  Therefore it is important that the memory of the passed in topic list is valid
 *	  when aws_iot_connect() is called.
 *
 *  @param[in] list Pointer to list a of topics.
 *  @param[in] count Number of entries in the list.
 *
 *  @retval 0 If successful.
 *  @retval -EINVAL if an invalid parameter is passed in.
 */
int aws_iot_application_topics_set(const struct mqtt_topic *const list, size_t count);

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* AWS_IOT_H__ */
