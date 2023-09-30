/* Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <net/nrf_cloud.h>
#if defined(CONFIG_NRF_CLOUD_COAP)
#include <net/nrf_cloud_coap.h>
#endif
#include <date_time.h>
#include <zephyr/logging/log.h>
#include <net/nrf_cloud_codec.h>

#include "message_queue.h"
#include "cloud_connection.h"

#include "led_control.h"

LOG_MODULE_REGISTER(message_queue, CONFIG_MULTI_SERVICE_LOG_LEVEL);

/* Message Queue for enqueing outgoing messages during offline periods. */
K_MSGQ_DEFINE(device_message_queue,
	      sizeof(struct nrf_cloud_obj *),
	      CONFIG_MAX_OUTGOING_MESSAGES,
	      sizeof(struct nrf_cloud_obj *));

/* Tracks the number of consecutive message-send failures. A total count greater than
 * CONFIG_MAX_CONSECUTIVE_SEND_FAILURES will trigger a connection reset and cooldown.
 * Resets on every successful device message send.
 */
static int send_failure_count;

static struct nrf_cloud_obj *allocate_dev_msg_for_queue(struct nrf_cloud_obj *msg_to_copy)
{
	struct nrf_cloud_obj *new_msg = k_malloc(sizeof(struct nrf_cloud_obj));

	if (new_msg && msg_to_copy) {
		*new_msg = *msg_to_copy;
	}

	return new_msg;
}

static int enqueue_device_message(struct nrf_cloud_obj *const msg_obj, const bool create_copy)
{
	if (!msg_obj) {
		return -EINVAL;
	}

	struct nrf_cloud_obj *q_msg = msg_obj;

	if (create_copy) {
		/* Allocate a new nrf_cloud_obj structure for the message queue.
		 * Copy the contents of msg_obj, which contains a pointer to the
		 * original message data, into the new structure.
		 */
		q_msg = allocate_dev_msg_for_queue(msg_obj);
		if (!q_msg) {
			return -ENOMEM;
		}
	}

	/* Attempt to append data onto message queue. */
	LOG_DBG("Adding device message to queue");
	if (k_msgq_put(&device_message_queue, &q_msg, K_NO_WAIT)) {
		LOG_ERR("Device message rejected, outgoing message queue is full");
		if (create_copy) {
			k_free(q_msg);
		}
		return -ENOMEM;
	}

	return 0;
}

static void free_queued_dev_msg_message(struct nrf_cloud_obj *msg_obj)
{
	/* Free the memory pointed to by the msg_obj struct */
	nrf_cloud_obj_free(msg_obj);
	/* Free the msg_obj struct itself */
	k_free(msg_obj);
}

/**
 * @brief Consume (attempt to send) a single device message from the device message queue.
 *	  Waits for nRF Cloud readiness before sending each message.
 *	  If the message fails to send, it will be reenqueued.
 *
 * @return int - 0 on success, otherwise negative error code.
 */
static int consume_device_message(void)
{
	struct nrf_cloud_obj *queued_msg;
	int ret;

	LOG_DBG("Consuming an enqueued device message");

	/* Wait until a message is available to send. */
	ret = k_msgq_get(&device_message_queue, &queued_msg, K_FOREVER);
	if (ret) {
		LOG_ERR("Failed to retrieve item from outgoing message queue, error: %d", ret);
		return -ret;
	}

	/* Wait until we are able to send it. */
	LOG_DBG("Waiting for valid connection before transmitting device message");
	(void)await_cloud_ready(K_FOREVER);

	/* Attempt to send it.
	 *
	 * Note, it is possible (and better) to batch-send device messages when more than one is
	 * queued up. We limit this sample to sending individual messages mainly to keep the sample
	 * simple and accessible. See the Asset Tracker V2 application for an example of batch
	 * message sending.
	 */
	LOG_DBG("Attempting to transmit enqueued device message");

#if defined(CONFIG_NRF_CLOUD_MQTT)
	struct nrf_cloud_tx_data mqtt_msg = {
		.qos = MQTT_QOS_1_AT_LEAST_ONCE,
		.topic_type = NRF_CLOUD_TOPIC_MESSAGE,
		.obj = queued_msg
	};

	/* Send message */
	ret = nrf_cloud_send(&mqtt_msg);

#elif defined(CONFIG_NRF_CLOUD_COAP)
	ret = nrf_cloud_coap_obj_send(queued_msg);

#endif /* CONFIG_NRF_CLOUD_COAP */

	if (ret < 0) {
		LOG_ERR("Transmission of enqueued device message failed, nrf_cloud_send "
			"gave error: %d. The message will be re-enqueued and tried again "
			"later.", ret);

		/* Re-enqueue the message for later retry.
		 * No need to create a copy since we already copied the
		 * message object struct when it was first enqueued.
		 */
		ret = enqueue_device_message(queued_msg, false);
		if (ret) {
			LOG_ERR("Could not re-enqueue message, discarding.");
			free_queued_dev_msg_message(queued_msg);
		}

		/* Increment the failure counter. */
		send_failure_count += 1;

		/* If we have failed too many times in a row, there is likely a bigger problem,
		 * and we should reset our connection to nRF Cloud, and wait for a few seconds.
		 */
		if (send_failure_count > CONFIG_MAX_CONSECUTIVE_SEND_FAILURES) {
			/* Disconnect. */
			disconnect_cloud();

			/* Wait for a few seconds before trying again. */
			k_sleep(K_SECONDS(CONFIG_CONSECUTIVE_SEND_FAILURE_COOLDOWN_SECONDS));
		}
	} else {
		/* Clean up the message receive from the queue */
		free_queued_dev_msg_message(queued_msg);

		LOG_DBG("Enqueued device message consumed successfully");

		/* Either overwrite the existing pattern with a short success pattern, or just
		 * disable the previously requested pattern, depending on if we are in verbose mode.
		 */
		if (IS_ENABLED(CONFIG_LED_VERBOSE_INDICATION)) {
			short_led_pattern(LED_SUCCESS);
		} else {
			stop_led_pattern();
		}

		/* Reset the failure counter, since we succeeded. */
		send_failure_count = 0;
	}

	return ret;
}

int send_device_message(struct nrf_cloud_obj *const msg_obj)
{
	/* Enqueue the message, creating a copy to be managed by the queue. */
	int ret = enqueue_device_message(msg_obj, true);

	if (ret) {
		LOG_ERR("Cannot add message to queue");
		nrf_cloud_obj_free(msg_obj);
	} else {
		/* The message data now belongs to the queue.
		 * Reset the provided object so it cannot be modified.
		 */
		nrf_cloud_obj_reset(msg_obj);
	}

	return ret;
}

void message_queue_thread_fn(void)
{
	/* Continually attempt to consume device messages */
	while (true) {
		(void) consume_device_message();
	}
}
