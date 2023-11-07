/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef QOS_H__
#define QOS_H__

/**
 * @defgroup qos Quality of Service library
 * @{
 * @brief QoS library that provides functionality to store and handle acknowledgment of multiple
 *	  in-flight messages.
 */

#include <zephyr/kernel.h>
#include <stdbool.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief ID base used for message IDs retrieved using qos_message_id_get_next() API. */
#define QOS_MESSAGE_ID_BASE 15000

/**
 * @defgroup qos_flag_bitmask QoS library flag bitmask values.
 *
 * @brief Use these bitmask values to enable different QoS message flags.
 *
 * @details The values can be OR'ed together to enable multiple flags at the same time.
 *
 * @{
 */

/** @brief Set this flag to disable acknowledging of a message. */
#define QOS_FLAG_RELIABILITY_ACK_DISABLED 0x01

/**
 * @brief Set this flag to require acknowledging of a message.
 *
 * @details By setting this flag the caller will be notified periodically with the
 *	    QOS_EVT_MESSAGE_TIMER_EXPIRED event until qos_message_remove() has been called with
 *	    the corresponding message.
 */
#define QOS_FLAG_RELIABILITY_ACK_REQUIRED 0x02

/** @} */

/** @brief Events notified by the library's callback handler. */
enum qos_evt_type {

	/** A new message is ready.
	 *  Payload is of type @ref qos_data. (message)
	 */
	QOS_EVT_MESSAGE_NEW,

	/** The internal backoff timer has expired. This callback contains one or more messages that
	 *  should be sent.
	 *  Payload is of type @ref qos_data. (message)
	 *
	 *  Callbacks of this event type are notified in system workqueue context.
	 */
	QOS_EVT_MESSAGE_TIMER_EXPIRED,

	/** Event received when the internal list of pending messages is full or a message has
	 *  been removed from the list using the qos_message_remove() API call.
	 *
	 *  If the heap_allocated flag is set in the @ref qos_data message structure
	 *  the corresponding buffer (message.data.buf) must be freed by the caller.
	 *  Payload is of type @ref qos_data.
	 */
	QOS_EVT_MESSAGE_REMOVED_FROM_LIST,
};

/** @brief Structure used to keep track of unACKed messages. */
struct qos_payload {
	/** Pointer to data. */
	uint8_t *buf;
	/** Length of data. */
	size_t len;
};

/** @brief Structure that contains the message payload and corresponding metadata. */
struct qos_data {
	/** Flags associated with the message.
	 *  See @ref qos_flag_bitmask for documentation on the various flags that can be set.
	 */
	uint32_t flags;

	/** Message ID */
	uint16_t id;

	/** Number of times that the message has been notified by the library. This count is not
	 *  incremented for messages flagged with QOS_FLAG_RELIABILITY_ACK_DISABLED.
	 */
	uint16_t notified_count;

	/** Type of the message.
	 *
	 *  The message type can be used to route data after a new or timed out message is
	 *  received with the QOS_MESSAGE_NEW or QOS_EVT_MESSAGE_TIMER_EXPIRED events.
	 */
	uint8_t type;

	/** Variable to keep track of user-allocated payload. */
	struct qos_payload data;

	/** Flag signifying if the payload is heap allocated. When a message has been removed from
	 *  the library using qos_message_remove() the payload buffer must be freed if this flag
	 *  has been set.
	 */
	bool heap_allocated;
};

/** @brief Library callback event structure. */
struct qos_evt {
	/** Event type notified by the library. */
	enum qos_evt_type type;
	/** Payload and corresponding metadata. */
	struct qos_data message;
};

/** @brief QoS library event handler.
 *
 *  @param[in] evt The event and the associated parameters.
 */
typedef void (*qos_evt_handler_t)(const struct qos_evt *evt);

/** @brief Function that initializes the QoS library.
 *
 *  @param evt_handler Pointer to the event callback handler.
 */
int qos_init(qos_evt_handler_t evt_handler);

/** @brief Add a message to the library. If the message fails to be added to the internal list
 *	   because the list is full, the message will be notified with the
 *	   QOS_EVT_MESSAGE_REMOVED_FROM_LIST event, so that it can be freed if heap allocated.
 *	   When this API is called, the event QOS_EVT_MESSAGE_NEW is always notified with the
 *	   corresponding message.
 *
 *  @param message Pointer to the corresponding message
 *
 *  @retval 0 on success.
 *  @retval -EINVAL If the message pointer is NULL.
 *  @retval -ENOMEM If the internal list is full.
 */
int qos_message_add(struct qos_data *message);

/** @brief Remove message from internal list. An event QOS_EVT_MESSAGE_REMOVED_FROM_LIST will be
 *	   notified in the library callback.
 *
 *  @param id Message ID of message to be removed.
 *
 *  @retval 0 on success.
 *  @retval -ENODATA If the passed in message ID is not found in the internal list.
 */
int qos_message_remove(uint32_t id);

/** @brief Function that checks if a flag is part of the bitmask associated with a message.
 *
 *  @param message Pointer to message.
 *  @param flag Flag to check for.
 *
 *  @retval true if the flag checked for is a part of the message flag bitmask.
 *  @retval false if flag checked for is not a part of the message flag bitmask.
 */
bool qos_message_has_flag(const struct qos_data *message, uint32_t flag);

/** @brief Function that prints the contents of a qos message structure.
 *         This requires that the library log level is set to debug level.
 *	   CONFIG_QOS_LOG_LEVEL_DBG=y
 */
void qos_message_print(const struct qos_data *message);

/** @brief Generate message ID that counts from QOS_MESSAGE_ID_BASE message ID base.
 *	   Count is reset if UINT16_MAX is reached.
 *
 *  @retval Message ID.
 */
uint16_t qos_message_id_get_next(void);

/** @brief Notify all pending messages.
 *         All messages that are currently stored in the internal list will be notified via the
 *	   QOS_EVT_MESSAGE_TIMER_EXPIRED event. This API does not clear the internal pending list.
 */
void qos_message_notify_all(void);

/** @brief Remove all pending messages.
 *         All messages that are currently stored in the internal list will be removed. Each message
 *	   will be notified in the QOS_EVT_MESSAGE_REMOVED_FROM_LIST callback event.
 *	   This API clears the internal pending list.
 */
void qos_message_remove_all(void);

/** @brief Reset and stop an ongoing timer backoff. */
void qos_timer_reset(void);

#ifdef __cplusplus
}
#endif

/**
 *@}
 */

#endif /* QOS_H__ */
