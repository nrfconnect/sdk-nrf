/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "esb_queues.h"


void esb_fifo_clear(struct esb_fifo_list *queue)
{
	queue->last = NULL;
}

void esb_fifo_push(struct esb_fifo_list *queue, struct esb_list_item *item)
{
	struct esb_list_item *begin;

	if (queue->last != NULL) {
		begin = queue->last->next;
		queue->last->next = item;
	} else {
		begin = item;
	}
	item->next = begin;
	queue->last = item;
}

void *esb_fifo_pop(struct esb_fifo_list *queue)
{
	struct esb_list_item *result;

	if (queue->last == NULL) {
		result = NULL;
	} else {
		result = queue->last->next;
		if (result == queue->last) {
			queue->last = NULL;
		} else {
			queue->last->next = result->next;
		}
	}

	return result;
}
