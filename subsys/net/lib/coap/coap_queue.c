/*
 * Copyright (c) 2014 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <logging/log.h>
#define LOG_LEVEL CONFIG_NRF_COAP_LOG_LEVEL
LOG_MODULE_REGISTER(coap_queue);

#include <string.h>
#include <errno.h>

#include "coap.h"
#include "coap_queue.h"

static coap_queue_item_t queue[COAP_MESSAGE_QUEUE_SIZE];
static u8_t message_queue_count;

u32_t coap_queue_init(void)
{
	for (u8_t i = 0; i < COAP_MESSAGE_QUEUE_SIZE; i++) {
		memset(&queue[i], 0, sizeof(coap_queue_item_t));
		queue[i].handle = i;
	}
	message_queue_count = 0;

	return 0;
}

u32_t coap_queue_add(coap_queue_item_t *item)
{
	NULL_PARAM_CHECK(item);

	if (message_queue_count >= COAP_MESSAGE_QUEUE_SIZE) {
		return ENOMEM;
	}

	for (u8_t i = 0; i < COAP_MESSAGE_QUEUE_SIZE; i++) {
		if (queue[i].buffer == NULL) {
			/* Free spot in message queue. Add message here... */
			memcpy(&queue[i], item, sizeof(coap_queue_item_t));
			message_queue_count++;

			return 0;
		}
	}

	return EACCES;
}

u32_t coap_queue_remove(coap_queue_item_t *item)
{
	for (u8_t i = 0; i < COAP_MESSAGE_QUEUE_SIZE; i++) {
		if (item == (coap_queue_item_t *)&queue[i]) {
			memset(&queue[i], 0, sizeof(coap_queue_item_t));
			message_queue_count--;
			return 0;
		}
	}

	return ENOENT;
}

u32_t coap_queue_item_by_token_get(coap_queue_item_t **item, u8_t *token,
				   u8_t token_len)
{
	for (u8_t i = 0; i < COAP_MESSAGE_QUEUE_SIZE; i++) {
		if (queue[i].token_len == token_len) {
			if ((queue[i].token_len != 0) &&
			    (memcmp(queue[i].token, token,
				    queue[i].token_len) == 0)) {
				*item = &queue[i];
				return 0;
			}
		}
	}

	return ENOENT;
}

u32_t coap_queue_item_by_mid_get(coap_queue_item_t **item, u16_t message_id)
{


	for (u8_t i = 0; i < COAP_MESSAGE_QUEUE_SIZE; i++) {
		if (queue[i].mid == message_id) {
			*item = &queue[i];
			return 0;
		}
	}

	return ENOENT;
}

u32_t coap_queue_item_next_get(coap_queue_item_t **item,
			       coap_queue_item_t *start)
{
	if (start == NULL) {
		for (u8_t i = 0; i < COAP_MESSAGE_QUEUE_SIZE; i++) {
			if (queue[i].buffer != NULL) {
				(*item) = &queue[i];
				return 0;
			}
		}
	} else {
		u8_t index_to_previous = (u8_t)(((u32_t)start - (u32_t)queue) /
					 (u32_t)sizeof(coap_queue_item_t));

		for (u8_t i = index_to_previous + 1;
		     i < COAP_MESSAGE_QUEUE_SIZE; i++) {
			if (queue[i].buffer != NULL) {
				(*item) = &queue[i];
				return 0;
			}
		}
	}
	(*item) = NULL;

	return ENOENT;
}
