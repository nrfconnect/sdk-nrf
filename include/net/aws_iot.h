/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/**@file
 *@brief AWS IoT library header.
 */

#ifndef AWS_IOT_H__
#define AWS_IOT_H__

#include <stdio.h>
#include <net/mqtt.h>

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
enum aws_iot_topic_type {
	AWS_IOT_SHADOW_TOPIC_GET = 0x1,
	AWS_IOT_SHADOW_TOPIC_UPDATE,
	AWS_IOT_SHADOW_TOPIC_DELETE
};

/** @brief AWS IoT notification event types, used to signal the application. */
enum aws_iot_evt_type {
	/** Connected to AWS IoT broker. */
	AWS_IOT_EVT_CONNECTED = 0x1,
	/** AWS IoT broker ready. */
	AWS_IOT_EVT_READY,
	/** Disconnected to AWS IoT broker. */
	AWS_IOT_EVT_DISCONNECTED,
	/** Data received from AWS message broker. */
	AWS_IOT_EVT_DATA_RECEIVED,
	/** FOTA update start. */
	AWS_IOT_EVT_FOTA_START,
	/** FOTA update done, request to reboot. */
	AWS_IOT_EVT_FOTA_DONE,
	/** FOTA erase pending. */
	AWS_IOT_EVT_FOTA_ERASE_PENDING,
	/** FOTA erase done. */
	AWS_IOT_EVT_FOTA_ERASE_DONE,
};

/** @brief Struct with data received from AWS IoT broker. */
struct aws_iot_evt {
	/** Type of event. */
	enum aws_iot_evt_type type;
	/** Pointer to data received from the AWS IoT broker. */
	char *ptr;
	/** Length of data. */
	size_t len;
};

/** @brief AWS IoT topic data. */
struct aws_iot_topic_data {
	/** Type of shadow topic that will be published to. */
	enum aws_iot_topic_type type;
	/** Pointer to string of application specific topic. */
	char *str;
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
struct aws_iot_tx_data {
	/** Topic that the message will be sent to. */
	struct aws_iot_topic_data topic;
	/** Pointer to message to be sent to AWS IoT broker. */
	char *str;
	/** Length of message. */
	size_t len;
	/** Quality of Service of the message. */
	enum mqtt_qos qos;
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
	 *  CONFIG_AWS_IOT_CLIENT_ID_APP is set. If not set an internal
	 *  configurable static client id is used.
	 */
	char *client_id;
	/** Length of client_id string. */
	size_t client_id_len;
};

/** @brief Initialize the module.
 *
 *  @warning This API must be called exactly once, and it must return
 *           successfully.
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
int aws_iot_send(const struct aws_iot_tx_data *const tx_data);

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
