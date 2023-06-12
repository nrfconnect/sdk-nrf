/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**@file
 *@brief AWS IoT library header.
 */

#ifndef AWS_IOT_H__
#define AWS_IOT_H__

#include <stdio.h>
#include <zephyr/net/mqtt.h>
#include <dfu/dfu_target.h>

/**
 * @defgroup aws_iot AWS IoT library
 * @{
 * @brief Library to connect the device to the AWS IoT message broker.
 */

#ifdef __cplusplus
extern "C" {
#endif

/** @brief AWS IoT shadow topics, used in messages to specify which shadow
 *         topic that will be published to.
 */
enum aws_iot_shadow_topic_type {
	/** Unused default value. */
	AWS_IOT_SHADOW_TOPIC_NONE,
	/** This topic type corresponds to
	 *  $aws/things/<thing-name>/shadow/get, publishing an empty message
	 *  to this topic requests the device shadow document.
	 */
	AWS_IOT_SHADOW_TOPIC_GET,
	/** This topic type corresponds to
	 *  $aws/things/<thing-name>/shadow/update, publishing data to this
	 *  topic updates the device shadow document.
	 */
	AWS_IOT_SHADOW_TOPIC_UPDATE,
	/** This topic type corresponds to
	 *  $aws/things/<thing-name>/shadow/delete, publishing an empty message
	 *  to this topic deletes the device Shadow document.
	 */
	AWS_IOT_SHADOW_TOPIC_DELETE
};

/**@ AWS broker disconnect results. */
enum aws_disconnect_result {
	AWS_IOT_DISCONNECT_USER_REQUEST,
	AWS_IOT_DISCONNECT_CLOSED_BY_REMOTE,
	AWS_IOT_DISCONNECT_INVALID_REQUEST,
	AWS_IOT_DISCONNECT_MISC,
	AWS_IOT_DISCONNECT_COUNT
};

/**@brief AWS broker connect results. */
enum aws_connect_result {
	AWS_IOT_CONNECT_RES_SUCCESS = 0,
	AWS_IOT_CONNECT_RES_ERR_NOT_INITD = -1,
	AWS_IOT_CONNECT_RES_ERR_INVALID_PARAM = -2,
	AWS_IOT_CONNECT_RES_ERR_NETWORK = -3,
	AWS_IOT_CONNECT_RES_ERR_BACKEND = -4,
	AWS_IOT_CONNECT_RES_ERR_MISC = -5,
	AWS_IOT_CONNECT_RES_ERR_NO_MEM = -6,
	/* Invalid private key */
	AWS_IOT_CONNECT_RES_ERR_PRV_KEY = -7,
	/* Invalid CA or client cert */
	AWS_IOT_CONNECT_RES_ERR_CERT = -8,
	/* Other cert issue */
	AWS_IOT_CONNECT_RES_ERR_CERT_MISC = -9,
	/* Timeout, SIM card may be out of data */
	AWS_IOT_CONNECT_RES_ERR_TIMEOUT_NO_DATA = -10,
	AWS_IOT_CONNECT_RES_ERR_ALREADY_CONNECTED = -11,
};

/** @brief AWS IoT notification event types, used to signal the application. */
enum aws_iot_evt_type {
	/** Connecting to AWS IoT broker. */
	AWS_IOT_EVT_CONNECTING = 0x1,
	/** Connected to AWS IoT broker. */
	AWS_IOT_EVT_CONNECTED,
	/** AWS IoT library has subscribed to all configured topics. */
	AWS_IOT_EVT_READY,
	/** Disconnected to AWS IoT broker. */
	AWS_IOT_EVT_DISCONNECTED,
	/** Data received from AWS message broker. */
	AWS_IOT_EVT_DATA_RECEIVED,
	/** Acknowledgment for data sent to AWS IoT. */
	AWS_IOT_EVT_PUBACK,
	/** Acknowledgment for pings sent to AWS IoT. */
	AWS_IOT_EVT_PINGRESP,
	/** FOTA update start. */
	AWS_IOT_EVT_FOTA_START,
	/** FOTA done. Payload of type @ref dfu_target_image_type (image).
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
	/** FOTA progress notification. */
	AWS_IOT_EVT_FOTA_DL_PROGRESS,
	/** FOTA error. Used to propagate FOTA-related errors to the
	 *  application. This is to distinguish between AWS_IOT irrecoverable
	 *  errors and FOTA errors, so they can be handled differently.
	 */
	AWS_IOT_EVT_FOTA_ERROR,
	/** AWS IoT library irrecoverable error. */
	AWS_IOT_EVT_ERROR
};

/** @brief AWS IoT topic data. */
struct aws_iot_topic_data {
	/** Optional: type of shadow topic that will be published to.
	 *  When publishing to a shadow topic this can be set instead of the
	 *  application specific topic below.
	 */
	enum aws_iot_shadow_topic_type type;
	/** Pointer to string of application specific topic. */
	const char *str;
	/** Length of application specific topic. */
	size_t len;
};

/** @brief Structure used to declare a list of application specific topics
 *         passed in by the application.
 */
struct aws_iot_app_topic_data {
	/** List of application specific topics. */
	struct mqtt_topic list[CONFIG_AWS_IOT_APP_SUBSCRIPTION_LIST_COUNT];
	/** Number of entries in topic list. */
	size_t list_count;
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
		struct aws_iot_data msg;
		int err;
		/** FOTA progress in percentage. */
		int fota_progress;
		bool persistent_session;
		uint16_t message_id;
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
	/** Socket for AWS IoT broker connection */
	int socket;
	/** Client id for AWS IoT broker connection, used when
	 *  @kconfig{CONFIG_AWS_IOT_CLIENT_ID_APP} is set. If not set an internal
	 *  configurable static client id is used.
	 */
	char *client_id;
	/** Length of client_id string. */
	size_t client_id_len;
	/** AWS IoT endpoint host name for broker connection, used when
	 *  @kconfig{CONFIG_AWS_IOT_BROKER_HOST_NAME_APP} is set. If not the
	 *  static @kconfig{AWS_IOT_BROKER_HOST_NAME} is used.
	 */
	char *host_name;
	/** Length of host_name string. */
	size_t host_name_len;
};

/** @brief Initialize the module.
 *
 *  @note This API must be called exactly once, and it must return
 *        successfully.
 *
 *  @param[in] config Pointer to struct containing connection parameters.
 *  @param[in] event_handler Pointer to event handler to receive AWS IoT module
 *                           events.
 *
 *  @return 0 If successful.
 *            Otherwise, a (negative) error code is returned.
 */
int aws_iot_init(const struct aws_iot_config *const config,
		 aws_iot_evt_handler_t event_handler);

/** @brief Connect to the AWS IoT broker.
 *
 *  @details This function exposes the MQTT socket to main so that it can be
 *           polled on.
 *
 *  @param[out] config Pointer to struct containing connection parameters,
 *                     the MQTT connection socket number will be copied to the
 *                     socket entry of the struct.
 *
 *  @return 0 If successful.
 *            Otherwise, a (negative) error code is returned.
 */
int aws_iot_connect(struct aws_iot_config *const config);

/** @brief Disconnect from the AWS IoT broker.
 *
 *  @return 0 If successful.
 *            Otherwise, a (negative) error code is returned.
 */
int aws_iot_disconnect(void);

/** @brief Send data to AWS IoT broker.
 *
 *  @param[in] tx_data Pointer to struct containing data to be transmitted to
 *                     the AWS IoT broker.
 *
 *  @return 0 If successful.
 *            Otherwise, a (negative) error code is returned.
 */
int aws_iot_send(const struct aws_iot_data *const tx_data);

/** @brief Get data from AWS IoT broker
 *
 *  @return 0 If successful.
 *            Otherwise, a (negative) error code is returned.
 */
int aws_iot_input(void);

/** @brief Ping AWS IoT broker. Must be called periodically
 *         to keep connection to broker alive.
 *
 *  @return 0 If successful.
 *            Otherwise, a (negative) error code is returned.
 */
int aws_iot_ping(void);

/** @brief Add a list of application specific topics that will be subscribed to
 *         upon connection to AWS IoT broker.
 *
 *  @param[in] topic_list Pointer to list of topics.
 *  @param[in] list_count Number of entries in the list.
 *
 *  @return 0 If successful.
 *            Otherwise, a (negative) error code is returned.
 */
int aws_iot_subscription_topics_add(
			const struct aws_iot_topic_data *const topic_list,
			size_t list_count);

#ifdef __cplusplus
}
#endif

/**
 *@}
 */

#endif /* AWS_IOT_H__ */
