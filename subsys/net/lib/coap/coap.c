/*
 * Copyright (c) 2014 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <logging/log.h>
#define LOG_LEVEL CONFIG_NRF_COAP_LOG_LEVEL
LOG_MODULE_REGISTER(coap);

#include <stdbool.h>
#include <string.h>
#include <errno.h>

#include <net/coap_api.h>
#include <net/coap_transport.h>
#include <net/coap_observe_api.h>

#include "coap.h"
#include "coap_queue.h"
#include "coap_resource.h"
#include "coap_observe.h"

#define COAP_MESSAGE_RST_SET(REMOTE, T_HANDLE, MID) { \
		coap_empty_message.remote = (REMOTE); \
		coap_empty_message.transport = (T_HANDLE); \
		coap_empty_message.header.id = (MID); \
		coap_empty_message.header.type = COAP_TYPE_RST; \
}

/** Token seed provided by application to be used for generating token numbers.
 */
static u32_t token_seed;
/** Message ID counter, used to generate unique message IDs. */
static u32_t message_id_counter;
/** Function pointer to an application CoAP error handler. */
static coap_error_callback_t error_callback;

/** Request handler where to forward all incoming requests. */
static coap_request_handler_t request_handler;
/** Memory allocator function, populated on @coap_init. */
static coap_alloc_t coap_alloc_fn;
/** Memory free function, populated on @coap_init. */
static coap_free_t coap_free_fn;

static coap_message_t coap_empty_message = {
	.header = {
		.version = 1,
		.type = COAP_TYPE_ACK,
		.token_len = 0,
		.code = COAP_CODE_EMPTY_MESSAGE,
		.id = 0,
	},
	.payload = NULL,
	.payload_len = 0,
	.options_count = 0,
	.arg = NULL,
	.response_callback = NULL,
	.local = NULL,
	.transport = -1,
	.options_len = 0,
	.options_offset = 0,
	.data = NULL,
	.data_len = 0
};

static inline bool is_ping(coap_message_t *message)
{
	return (message->header.code == COAP_CODE_EMPTY_MESSAGE) &&
	       (message->header.type == COAP_TYPE_CON);
}

static inline bool is_ack(coap_message_t *message)
{
	return (message->header.code == COAP_CODE_EMPTY_MESSAGE) &&
	       (message->header.type == COAP_TYPE_ACK);
}

static inline bool is_reset(coap_message_t *message)
{
	return (message->header.type == COAP_TYPE_RST);
}

static inline bool is_con(coap_message_t *message)
{
	return (message->header.type == COAP_TYPE_CON);
}

static inline bool is_non(coap_message_t *message)
{
	return (message->header.type == COAP_TYPE_NON);
}

static inline bool is_request(u8_t message_code)
{
	return (message_code >= 1) && (message_code < 32);
}

static inline bool is_response(u8_t message_code)
{
	return (message_code >= 64) && (message_code < 192);
}

static inline void app_error_notify(u32_t err_code, coap_message_t *message)
{
	if (error_callback != NULL) {
		COAP_MUTEX_UNLOCK();

		error_callback(err_code, message);

		COAP_MUTEX_LOCK();
	}
}

u32_t coap_init(u32_t token_rand_seed,
		coap_transport_init_t *transport_param,
		coap_alloc_t alloc_fn,
		coap_free_t free_fn)
{
	if ((alloc_fn == NULL) || (free_fn == NULL)) {
		return EINVAL;
	}

	COAP_ENTRY();

	u32_t err_code;

	COAP_MUTEX_LOCK();

	error_callback = NULL;
	coap_alloc_fn = alloc_fn;
	coap_free_fn = free_fn;
	token_seed = token_rand_seed;
	(void)token_seed;

	internal_coap_observe_init();
	message_id_counter = 1;

	err_code = coap_transport_init(transport_param);
	if (err_code != 0) {
		COAP_MUTEX_UNLOCK();
		COAP_EXIT();
		return err_code;
	}

	err_code = coap_queue_init();
	if (err_code != 0) {
		COAP_MUTEX_UNLOCK();
		COAP_EXIT();
		return err_code;
	}

	err_code = coap_resource_init();

	COAP_MUTEX_UNLOCK();
	COAP_EXIT();
	return err_code;

}

u32_t coap_error_handler_register(coap_error_callback_t callback)
{
	/* TODO: error handling, null pointer, module initialized etc. */
	COAP_MUTEX_LOCK();

	error_callback = callback;

	COAP_MUTEX_UNLOCK();

	return 0;
}

u32_t internal_coap_message_send(u32_t *handle, coap_message_t *message)
{
	if ((message == NULL) || (message->remote == NULL)) {
		return EINVAL;
	}

	/* Compiled away if COAP_ENABLE_OBSERVE_CLIENT is not set to 1. */
	coap_observe_client_send_handle(message);

	COAP_ENTRY();

	/* Fetch the expected length of the packet serialized by passing length
	 * of 0.
	 */
	u16_t expected_length = 0;
	u32_t err_code = coap_message_encode(message, NULL, &expected_length);

	if (err_code != 0) {
		COAP_EXIT();
		return err_code;
	}

	err_code = ENOMEM;

	/* Allocate a buffer to serialize the message into. */
	u8_t *buffer;
	u32_t request_length = expected_length;

	buffer = coap_alloc_fn(request_length);
	if (buffer == NULL) {
		COAP_TRC("buffer alloc error = 0x%08lX!",
			 (unsigned long)err_code);
		COAP_EXIT();
		return err_code;
	}
	memset(buffer, 0, request_length);
	COAP_TRC("Alloc mem, buffer = %p", (u8_t *)buffer);

	/* Serialize the message. */
	u16_t buffer_length = (u16_t)request_length;

	err_code = coap_message_encode(message, buffer, &buffer_length);
	if (err_code != 0) {
		COAP_TRC("Encode error!");
		COAP_TRC("Free mem, buffer = %p", buffer);
		coap_free_fn(buffer);
		COAP_EXIT();
		return err_code;
	}

	err_code = coap_transport_write(message->transport, message->remote,
					buffer, buffer_length);

	if (err_code == 0) {
		if (is_con(message) || (is_non(message) &&
					is_request(message->header.code) &&
					(message->response_callback != NULL))) {
			coap_queue_item_t item;

			item.arg = message->arg;
			item.mid = message->header.id;
			item.callback = message->response_callback;
			item.buffer = buffer;
			item.buffer_len = buffer_length;
			item.timeout_val = COAP_ACK_TIMEOUT *
					   COAP_ACK_RANDOM_FACTOR;

			if (message->header.type == COAP_TYPE_CON) {
				item.timeout = item.timeout_val;
				item.retrans_count = 0;
			} else {
				item.timeout = COAP_MAX_TRANSMISSION_SPAN;
				item.retrans_count = COAP_MAX_RETRANSMIT_COUNT;
			}

			item.transport = message->transport;
			item.token_len = message->header.token_len;

			if (message->remote->sa_family == AF_INET6) {
				memcpy(&item.remote, message->remote,
				       sizeof(struct sockaddr_in6));
			} else {
				memcpy(&item.remote, message->remote,
				       sizeof(struct sockaddr_in));
			}
			memcpy(item.token, message->token,
			       message->header.token_len);

			err_code = coap_queue_add(&item);
			if (err_code != 0) {
				COAP_TRC("Message queue error = 0x%08lX!",
					 (unsigned long)err_code);

				COAP_TRC("Free mem, buffer = %p", buffer);
				coap_free_fn(buffer);
				COAP_EXIT();
				return err_code;
			}

			*handle = item.handle;
		} else {
			*handle = COAP_MESSAGE_QUEUE_SIZE;

			COAP_TRC("Free mem, buffer = %p", buffer);
			coap_free_fn(buffer);
		}
	} else {
		COAP_TRC("Free mem, buffer = %p", buffer);
		coap_free_fn(buffer);
	}

	COAP_EXIT();
	return err_code;
}


static u32_t create_response(coap_message_t **response, coap_message_t *request,
			     u16_t data_size)
{
	u32_t err_code = ENOMEM;

	/* Allocate space for a new message. */
	u32_t size = sizeof(coap_message_t);

	(*response) = coap_alloc_fn(size);
	if (*response == NULL) {
		return err_code;
	}


	memset((*response), 0, sizeof(coap_message_t));
	COAP_TRC("Alloc mem, response = %p", (u8_t *)(*response));

	if (data_size > 0) {
		/* Allocate a scratch buffer for payload and options. */
		size = data_size;
		(*response)->data = coap_alloc_fn(size);
		if ((*response)->data == NULL) {
			coap_free_fn(*response);
			return err_code;
		}
		memset((*response)->data, 0, size);
		(*response)->data_len = size;
		COAP_TRC("Alloc mem, response->data = %p", (*response)->data);
	}

	coap_message_conf_t config;

	memset(&config, 0, sizeof(coap_message_conf_t));
	config.token_len = request->header.token_len;
	config.id = request->header.id;
	config.code = COAP_CODE_404_NOT_FOUND;
	config.transport = request->transport;

	memcpy(config.token, request->token, request->header.token_len);

	if ((coap_msg_type_t)request->header.type == COAP_TYPE_CON) {
		config.type = COAP_TYPE_ACK;
	} else {
		config.type = (coap_msg_type_t)request->header.type;
	}

	err_code = coap_message_create(*response, &config);
	if (err_code != 0) {
		return err_code;
	}

	(void)coap_message_remote_addr_set(*response, request->remote);

	return 0;
}


/**@brief Common function for sending response error message
 *
 * @param[in] message Pointer to the original request message.
 * @param[in] code    CoAP message code to send in the response.
 *
 * @retval 0 If the response was sent out successfully.
 */
static u32_t send_error_response(coap_message_t *message, u8_t code)
{
	coap_message_t *error_response = NULL;

	u32_t err_code = create_response(&error_response, message,
					 COAP_MESSAGE_DATA_MAX_SIZE);

	if (err_code != 0) {
		/* If message could not be created, notify the application. */
		app_error_notify(err_code, message);
		return err_code;
	}

	/* Set the response code. */
	error_response->header.code = code;

	if (error_response != NULL) {
		u32_t handle;

		err_code = internal_coap_message_send(&handle, error_response);

		COAP_TRC("Free mem, response->data = %p", error_response->data);
		coap_free_fn(error_response->data);

		COAP_TRC("Free mem, response = %p", (u8_t *)error_response);
		coap_free_fn(error_response);
	}

	return err_code;
}

u32_t coap_transport_read(const coap_transport_handle_t transport,
			  const struct sockaddr *remote,
			  const struct sockaddr *local,
			  u32_t result, const u8_t *data, u16_t datalen)
{
	COAP_ENTRY();

	/* Discard all packets if not success or truncated. */
	if (result != 0) {
		COAP_EXIT();
		return 0;
	}

	u32_t err_code = ENOMEM;
	coap_message_t *message;
	/* Allocate space for a new message. */
	u32_t size = sizeof(coap_message_t);

	message = coap_alloc_fn(size);
	if (message == NULL) {
		COAP_EXIT();
		return err_code;
	}

	memset(message, 0, sizeof(coap_message_t));
	COAP_TRC("Alloc mem, message = %p", (u8_t *)message);

	err_code = coap_message_decode(message, data, datalen);
	if (err_code != 0) {
		app_error_notify(err_code, message);

		coap_free_fn(message);
		COAP_EXIT();
		return err_code;
	}

	/* Copy the remote address information. */
	message->remote = (struct sockaddr *)remote;
	message->local = (struct sockaddr *)local;
	message->transport = transport;

	if (is_ping(message)) {
		COAP_MESSAGE_RST_SET(message->remote, message->transport,
				     message->header.id);

		u32_t handle;

		err_code = internal_coap_message_send(&handle,
						      &coap_empty_message);
	} else if (is_ack(message) ||
		   is_reset(message)) {
		/* Populate the token with the one used sending,
		 * before passing it to the application.
		 */
		coap_queue_item_t *item = NULL;

		err_code = coap_queue_item_by_mid_get(&item,
						      message->header.id);

		if (err_code == 0) {
			if (item->callback != NULL) {
				/* As the token is missing from peer, it will be
				 * added before giving it to the application.
				 */
				memcpy(message->token, item->token,
				       item->token_len);
				message->header.token_len = item->token_len;

				/* Compiled away if COAP_ENABLE_OBSERVE_CLIENT
				 * is not set to 1.
				 */
				coap_observe_client_response_handle(message,
								    item);

				COAP_TRC(">> application callback");

				COAP_MUTEX_UNLOCK();

				if (is_ack(message)) {
					item->callback(0, item->arg, message);
				} else {
					item->callback(ECONNRESET, item->arg,
						       message);
				}

				COAP_MUTEX_LOCK();

				COAP_TRC("<< application callback");
			}

			COAP_TRC("Free mem, item->buffer = %p", item->buffer);
			coap_free_fn(item->buffer);

			/* Remove the queue element, as a match occurred. */
			err_code = coap_queue_remove(item);
		}
	} else if (is_response(message->header.code)) {
		COAP_TRC("CoAP message type: RESPONSE");

		coap_queue_item_t *item;

		err_code = coap_queue_item_by_token_get(
						&item, message->token,
						message->header.token_len);
		if (err_code != 0) {
			/* Compiled away if COAP_ENABLE_OBSERVE_CLIENT is not
			 * set to 1.
			 */
			coap_observe_client_response_handle(message, NULL);

			coap_free_fn(message);

			COAP_MUTEX_UNLOCK();
			COAP_EXIT();
			return err_code;
		}

		if (item->callback != NULL) {
			/* Compiled away if COAP_ENABLE_OBSERVE_CLIENT is not
			 * set to 1.
			 */
			coap_observe_client_response_handle(message, item);

			COAP_TRC(">> application callback");

			COAP_MUTEX_UNLOCK();

			item->callback(0, item->arg, message);

			COAP_MUTEX_LOCK();

			COAP_TRC("<< application callback");
		}

		COAP_TRC("Free mem, item->buffer = %p", item->buffer);
		coap_free_fn(item->buffer);

		err_code = coap_queue_remove(item);

	} else if (is_request(message->header.code)) {
		COAP_TRC("CoAP message type: REQUEST");

		if (request_handler != NULL) {
			u32_t return_code = request_handler(message);

			/* If success, then all processing and any responses
			 * has been sent by the application callback.
			 *
			 * If not success, then send an appropriate error
			 * message back to the origin with the return_code
			 * from the callback.
			 */
			if (return_code != 0) {
				if (return_code == ENOENT) {
					/* Send response with provided CoAP
					 * code.
					 */
					(void)send_error_response(
					    message,
					    COAP_CODE_404_NOT_FOUND);
				} else if (return_code == EINVAL) {
					(void)send_error_response(
					    message,
					    COAP_CODE_405_METHOD_NOT_ALLOWED);
				} else {
					(void)send_error_response(
					    message,
					    COAP_CODE_400_BAD_REQUEST);
				}
			}
		} else {
			u8_t *uri_pointers[COAP_RESOURCE_MAX_DEPTH] = { 0, };
			u8_t uri_path_count = 0;
			u16_t index;

			for (index = 0; index < message->options_count;
								index++) {
				if (message->options[index].number ==
							COAP_OPT_URI_PATH) {
					uri_pointers[uri_path_count++] =
						message->options[index].data;
				}
			}

			coap_resource_t *found_resource;

			err_code = coap_resource_get(&found_resource,
						     uri_pointers,
						     uri_path_count);

			if (found_resource == NULL) {
				/* Reply with NOT FOUND. */
				err_code = send_error_response(
					message,
					COAP_CODE_404_NOT_FOUND);
			} else if (found_resource->callback == NULL) {
				/* Reply with Method Not Allowed. */
				err_code = send_error_response(
					message,
					COAP_CODE_405_METHOD_NOT_ALLOWED);
			} else if ((found_resource->permission &
				    (1 << ((message->header.code) - 1))) > 0) {
				/* Has permission for the requested CoAP method.
				 */
				COAP_MUTEX_UNLOCK();

				found_resource->callback(found_resource,
							 message);

				COAP_MUTEX_LOCK();
			} else {
				/* Reply with Method Not Allowed. */
				err_code = send_error_response(
					message,
					COAP_CODE_405_METHOD_NOT_ALLOWED);
			}
		}
	}

	COAP_TRC("Free mem, message = %p", (u8_t *)message);
	coap_free_fn((u8_t *)message);

	COAP_EXIT();
	return err_code;
}

u32_t coap_message_send(u32_t *handle, coap_message_t *message)
{
	COAP_MUTEX_LOCK();

	u32_t err_code = internal_coap_message_send(handle, message);

	COAP_MUTEX_UNLOCK();

	return err_code;
}

u32_t coap_message_abort(u32_t handle)
{

	return ENOTSUP;
}

u32_t coap_message_new(coap_message_t **request, coap_message_conf_t *config)
{
	COAP_ENTRY();

	u32_t err_code = ENOMEM;
	coap_message_t *allocated;

	/* If port is not configured, return error and skip initialization
	 * of the message.
	 */
	if (config->transport == -1) {
		COAP_EXIT();
		return EINVAL;
	}

	COAP_MUTEX_LOCK();

	/* Allocate space for a new message. */
	u32_t size = sizeof(coap_message_t);

	allocated = coap_alloc_fn(size);
	if (allocated == NULL) {
		COAP_MUTEX_UNLOCK();
		COAP_EXIT_WITH_RESULT(err_code);
		return err_code;
	}

	memset(allocated, 0, sizeof(coap_message_t));
	COAP_TRC("Alloc mem, *request = %p", allocated);

	/* Allocate a scratch buffer for payload and options. */
	size = COAP_MESSAGE_DATA_MAX_SIZE;
	allocated->data = coap_alloc_fn(size);
	if (allocated->data == NULL) {
		COAP_TRC("Allocation of message data buffer failed!");

		COAP_TRC("Free mem, *request = %p", (u8_t *)(*request));
		coap_free_fn(allocated);

		COAP_MUTEX_UNLOCK();
		COAP_EXIT_WITH_RESULT(err_code);
		return err_code;
	}

	memset(allocated->data, 0, size);
	allocated->data_len = size;

	COAP_TRC("Alloc mem, (*request)->data = %p", (allocated->data));

	if (config->id == 0) { /* Message id is not set, generate one. */
		config->id = message_id_counter++;
	}

	err_code = coap_message_create(allocated, config);
	if (err_code == 0) {
		(*request) = allocated;
	} else {
		coap_free_fn(allocated->data);
		coap_free_fn(allocated);
	}

	COAP_MUTEX_UNLOCK();

	COAP_EXIT_WITH_RESULT(err_code);
	return err_code;
}

u32_t coap_message_delete(coap_message_t *message)
{
	COAP_ENTRY();

	COAP_MUTEX_LOCK();

	/* If this is a request free the coap_message_t and the data buffer. */

	COAP_TRC("Free mem, message->data = %p", message->data);
	coap_free_fn(message->data);

	COAP_TRC("Free mem, message = %p", (u8_t *)message);
	coap_free_fn(message);


	COAP_MUTEX_UNLOCK();

	COAP_EXIT();

	return 0;
}


u32_t coap_time_tick(void)
{
	COAP_MUTEX_LOCK();

	coap_transport_process();

	/* Loop through the message queue to see if any packets needs
	 * retransmission, or has timed out.
	 */
	coap_queue_item_t *item = NULL;

	while (coap_queue_item_next_get(&item, item) == 0) {
		if (item->timeout == 0) {
			/* If there is still retransmission attempts left. */
			if (item->retrans_count < COAP_MAX_RETRANSMIT_COUNT) {
				item->timeout = item->timeout_val * 2;
				item->timeout_val = item->timeout;
				item->retrans_count++;

				/* Retransmit the message. */
				u32_t err_code = coap_transport_write(
					item->transport,
					(struct sockaddr *)&item->remote,
					item->buffer,
					item->buffer_len);
				if (err_code != 0) {
					app_error_notify(err_code, NULL);
				}
			}

			/* No more retransmission attempts left, or max transmit
			 * span reached.
			 */
			if ((item->timeout > COAP_MAX_TRANSMISSION_SPAN) ||
			    (item->retrans_count >=
						COAP_MAX_RETRANSMIT_COUNT)) {

				COAP_MUTEX_UNLOCK();

				item->callback(ETIMEDOUT, item->arg, NULL);

				COAP_MUTEX_LOCK();

				COAP_TRC("Free mem, item->buffer = %p",
					 item->buffer);
				coap_free_fn(item->buffer);

				(void)coap_queue_remove(item);
			}
		} else {
			item->timeout--;
		}
	}

	COAP_MUTEX_UNLOCK();

	return 0;
}

u32_t coap_request_handler_register(coap_request_handler_t handler)
{
	COAP_MUTEX_LOCK();

	request_handler = handler;

	COAP_MUTEX_UNLOCK();

	return 0;
}

__weak void coap_transport_input(void)
{
	/* By default not implemented. Transport specific. */
}

void coap_input(void)
{
	COAP_MUTEX_LOCK();

	coap_transport_input();

	COAP_MUTEX_UNLOCK();
}
