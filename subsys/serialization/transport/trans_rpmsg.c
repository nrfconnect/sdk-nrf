/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr.h>
#include <errno.h>

#include <rp_errors.h>

#include "rp_ll.h"
#include "trans_rpmsg.h"

#include <logging/log.h>


LOG_MODULE_REGISTER(trans_rpmsg);


struct fifo_item {
	void *fifo_reserved;
	size_t size;
};


static rp_trans_receive_handler receive_handler;


static int translate_error(int rpmsg_err)
{
	switch (rpmsg_err) {
	case RPMSG_ERR_NO_MEM:
		return RP_ERROR_NO_MEM;
	case RPMSG_ERR_NO_BUFF:
		return RP_ERROR_NO_MEM;
	case RPMSG_ERR_PARAM:
		return RP_ERROR_INVALID_PARAM;
	case RPMSG_ERR_DEV_STATE:
		return RP_ERROR_INVALID_STATE;
	case RPMSG_ERR_BUFF_SIZE:
		return RP_ERROR_NO_MEM;
	case RPMSG_ERR_INIT:
		return RP_ERROR_INTERNAL;
	case RPMSG_ERR_ADDR:
		return RP_ERROR_INTERNAL;
	default:
		if (rpmsg_err < 0) {
			return RP_ERROR_INTERNAL;
		}
		break;
	}
	return RP_SUCCESS;
}

int rp_trans_init(rp_trans_receive_handler callback)
{
	int result;

	__ASSERT(callback, "Callback cannot be NULL");
	receive_handler = callback;
	result = rp_ll_init();
	return translate_error(result);
}

void rp_trans_uninit(void)
{
	rp_ll_uninit();
}

static void endpoint_thread(void *p1, void *p2, void *p3)
{
	struct rp_trans_endpoint *endpoint = (struct rp_trans_endpoint *)p1;
	struct fifo_item *item;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (true) {
		k_sem_take(&endpoint->sem, K_FOREVER);
		if (!endpoint->running) {
			break;
		}
		do {
			item = k_fifo_get(&endpoint->fifo, K_NO_WAIT);
			if (item == NULL) {
				break;
			}
			receive_handler(endpoint, (const u8_t *)(&item[1]),
				item->size);
			k_free(item);
		} while (true);
	}
}

static void event_handler(struct rp_ll_endpoint *ep,
	enum rp_ll_event_type event, const u8_t *buf, size_t length)
{
	struct rp_trans_endpoint *endpoint =
		CONTAINER_OF(ep, struct rp_trans_endpoint, ep);

	if (event == RP_LL_EVENT_DATA) {
		struct fifo_item *item =
			k_malloc(sizeof(struct fifo_item) + length);
		if (!item) {
			LOG_ERR("Out of memory when receiving incoming packet");
			__ASSERT(item, "Out of memory");
		}
		memcpy(&item[1], buf, length);
		item->size = length;
		k_fifo_put(&endpoint->fifo, item);
	}

	k_sem_give(&endpoint->sem);
}

int rp_trans_endpoint_init(struct rp_trans_endpoint *endpoint,
	int endpoint_number)
{
	int result;
	int prio = endpoint->prio;
	size_t stack_size = endpoint->stack_size;
	k_thread_stack_t *stack = endpoint->stack;

	endpoint->running = true;

	k_fifo_init(&endpoint->fifo);

	k_sem_init(&endpoint->sem, 0, 1);

	result = rp_ll_endpoint_init(&endpoint->ep, endpoint_number,
		event_handler, NULL);

	if (result < 0) {
		return translate_error(result);
	}

	k_sem_take(&endpoint->sem, K_FOREVER);

	k_thread_create(&endpoint->thread, stack, stack_size, endpoint_thread,
		endpoint, NULL, NULL, prio, 0, K_NO_WAIT);

	return RP_SUCCESS;
}

void rp_trans_endpoint_uninit(struct rp_trans_endpoint *endpoint)
{
	struct fifo_item *item;

	do {
		endpoint->running = false;
		k_sem_give(&endpoint->sem);
		if (!strcmp(k_thread_state_str(&endpoint->thread), "dead")) {
			break;
		}
		k_sleep(10);
	} while (true);

	k_thread_abort(&endpoint->thread);
	rp_ll_endpoint_uninit(&endpoint->ep);

	do {
		item = k_fifo_get(&endpoint->fifo, K_NO_WAIT);
		if (item == NULL) {
			break;
		}
		k_free(item);
	} while (true);
}

int rp_trans_send(struct rp_trans_endpoint *endpoint, const u8_t *buf,
	size_t buf_len)
{
	int result = rp_ll_send(&endpoint->ep, buf, buf_len);

	return translate_error(result);
}
