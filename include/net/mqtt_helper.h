/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @defgroup mqtt_helper MQTT helper library
 * @{
 * @brief Convenience library that simplifies Zephyr MQTT API and socket handling.
 */

#ifndef MQTT_HELPER__
#define MQTT_HELPER__

#include <stdio.h>
#include <zephyr/net/mqtt.h>

#ifdef __cplusplus
extern "C" {
#endif

enum mqtt_state {
	MQTT_STATE_UNINIT,
	MQTT_STATE_DISCONNECTED,
	MQTT_STATE_TRANSPORT_CONNECTING,
	MQTT_STATE_CONNECTING,
	MQTT_STATE_TRANSPORT_CONNECTED,
	MQTT_STATE_CONNECTED,
	MQTT_STATE_DISCONNECTING,

	MQTT_STATE_COUNT,
};

enum mqtt_helper_error {
	/** The received payload is larger than the payload buffer. */
	MQTT_HELPER_ERROR_MSG_SIZE,
};

struct mqtt_helper_buf {
	/** Pointer to buffer. */
	char *ptr;

	/** Size of buffer. */
	size_t size;
};

/** @brief Handler invoked for events that are received from the MQTT stack.
 *	   This callback handler can be used to filter incoming MQTT events before they are
 *	   processed by the MQTT helper library.
 *
 *  @param client Pointer to the MQTT client instance.
 *  @param event Pointer to the MQTT event.
 *
 *  @retval 0 if the event is handled by the caller. No further processing of the MQTT event will
 *	      be carried out by the MQTT helper library.
 *  @retval 1 if the MQTT helper library should continue to process the event after
 *	      the handler returns.
 */
typedef bool (*mqtt_helper_on_all_events_t)(struct mqtt_client *const client,
					    const struct mqtt_evt *const event);
typedef void (*mqtt_helper_on_connack_t)(enum mqtt_conn_return_code return_code,
					 bool session_present);
typedef void (*mqtt_helper_on_disconnect_t)(int result);
typedef void (*mqtt_helper_on_publish_t)(struct mqtt_helper_buf topic_buf,
					 struct mqtt_helper_buf payload_buf);
typedef void (*mqtt_helper_on_puback_t)(uint16_t message_id, int result);
typedef void (*mqtt_helper_on_suback_t)(uint16_t message_id, int result);
typedef void (*mqtt_helper_on_pingresp_t)(void);
typedef void (*mqtt_helper_on_error_t)(enum mqtt_helper_error error);

struct mqtt_helper_cfg {
	struct {
		mqtt_helper_on_all_events_t on_all_events;
		mqtt_helper_on_connack_t on_connack;
		mqtt_helper_on_disconnect_t on_disconnect;
		mqtt_helper_on_publish_t on_publish;
		mqtt_helper_on_puback_t on_puback;
		mqtt_helper_on_suback_t on_suback;
		mqtt_helper_on_pingresp_t on_pingresp;
		mqtt_helper_on_error_t on_error;
	} cb;
};

struct mqtt_helper_conn_params {
	/* The hostname must be null-terminated. */
	struct mqtt_helper_buf hostname;
	struct mqtt_helper_buf device_id;
	struct mqtt_helper_buf user_name;
};

/** @brief Initialize the MQTT helper.
 *
 *  @retval 0 if successful.
 *  @retval -EOPNOTSUPP if operation is not supported in the current state.
 *  @return Otherwise a negative error code.
 */
int mqtt_helper_init(struct mqtt_helper_cfg *cfg);


/** @brief Connect to an MQTT broker.
 *
 *  @retval 0 if successful.
 *  @retval -EOPNOTSUPP if operation is not supported in the current state.
 *  @return Otherwise a negative error code.
 */
int mqtt_helper_connect(struct mqtt_helper_conn_params *conn_params);

/** @brief Disconnect from the MQTT broker.
 *
 *  @retval 0 if successful.
 *  @retval -EOPNOTSUPP if operation is not supported in the current state.
 *  @return Otherwise a negative error code.
 */
int mqtt_helper_disconnect(void);

/** @brief Subscribe to MQTT topics.
 *
 *  @retval 0 if successful.
 *  @retval -EOPNOTSUPP if operation is not supported in the current state.
 *  @return Otherwise a negative error code.
 */
int mqtt_helper_subscribe(struct mqtt_subscription_list *sub_list);

/** @brief Publish an MQTT message.
 *
 *  @retval 0 if successful.
 *  @retval -EOPNOTSUPP if operation is not supported in the current state.
 *  @return Otherwise a negative error code.
 */
int mqtt_helper_publish(const struct mqtt_publish_param *param);

/** @brief Deinitialize library. Must be called when all MQTT operations are done to
 *	   release resources and allow for a new client. The client must be in a disconnected state.
 *
 *  @retval 0 if successful.
 *  @retval -EOPNOTSUPP if operation is not supported in the current state.
 *  @return Otherwise a negative error code.
 */
int mqtt_helper_deinit(void);


#ifdef __cplusplus
}
#endif

/**
 *@}
 */

#endif /* MQTT_HELPER__ */
