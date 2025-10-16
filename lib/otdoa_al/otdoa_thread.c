/*
 * Copyright (c) 2025 PHY Wireless, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-PHYW
 */

#include <otdoa_al/otdoa_al2otdoa_api.h>
#include <otdoa_al/otdoa_otdoa2al_api.h>
#include <stdlib.h>
#include <zephyr/kernel.h>
#include <zephyr/posix/unistd.h>

#include "otdoa_al_log.h"
#include "otdoa_http.h"

LOG_MODULE_REGISTER(otdoa_al, LOG_LEVEL_INF);

struct http_work {
	struct k_work work;
	tOTDOA_HTTP_MESSAGE msg;
};

#define SLAB_COUNT (10)

static struct k_work_q http_workq;
static struct k_fifo rs_fifo;
static struct k_thread rs_thread_data;
static void *message_slab_buffer;
struct k_mem_slab message_slab;

static struct {
	atomic_t terminate;
	atomic_t ready;
} gOTDOA;

static struct http_stop_work_item {
	struct k_work work;
	int fail_or_cancel;
} http_stop_work;

K_THREAD_STACK_DEFINE(http_workq_stack, CONFIG_OTDOA_HTTP_QUEUE_STACK_SIZE);
K_THREAD_STACK_DEFINE(rs_thread_stack, CONFIG_OTDOA_RS_THREAD_STACK_SIZE);

void rs_entry_point(void *p1, void *p2, void *p3);

int otdoa_start(void)
{
	/* init the message slab */
	const unsigned int MAX_MSG_SIZE = MAX(OTDOA_MAX_MESSAGE_SIZE, sizeof(struct http_work));

	message_slab_buffer = calloc(SLAB_COUNT, MAX_MSG_SIZE);
	k_mem_slab_init(&message_slab, message_slab_buffer, MAX_MSG_SIZE, SLAB_COUNT);

	/* init the HTTP workqueue */
	struct k_work_queue_config http_cfg = {.name = "http_workq"};

	k_work_queue_init(&http_workq);
	k_work_queue_start(&http_workq, http_workq_stack, K_THREAD_STACK_SIZEOF(http_workq_stack),
			   CONFIG_OTDOA_HTTP_QUEUE_PRIORITY, &http_cfg);

	/* init the RS fifo thread */
	k_fifo_init(&rs_fifo);
	k_thread_create(&rs_thread_data, rs_thread_stack, K_THREAD_STACK_SIZEOF(rs_thread_stack),
			rs_entry_point, NULL, NULL, NULL, CONFIG_OTDOA_RS_THREAD_PRIORITY,
			K_FP_REGS, K_NO_WAIT);
	k_thread_name_set(&rs_thread_data, "rs_thread");

	return 0;
}

int otdoa_stop(void)
{
	atomic_set(&gOTDOA.terminate, 1);
	int rc = k_thread_join(&rs_thread_data, K_MSEC(10000));

	if (rc) {
		LOG_ERR("Timed out waiting for RS Thread to terminate: %d", rc);
	}

	free(message_slab_buffer);
	return rc;
}

void *otdoa_message_alloc(size_t length)
{
	if (length > message_slab.info.block_size) {
		LOG_ERR("message too large for allocation (%u bytes)", length);
		return NULL;
	}

	void *alloc;
	int rc = k_mem_slab_alloc(&message_slab, &alloc, K_NO_WAIT);

	if (rc) {
		LOG_ERR("Memory allocation failure: %d", rc);
		return NULL;
	}

	return alloc;
}

int otdoa_message_free(void *msg)
{
	k_mem_slab_free(&message_slab, msg);
	return 0;
}

void rs_entry_point(void *p1, void *p2, void *p3)
{
	LOG_INF("RS Thread Started");

	while (!atomic_get(&gOTDOA.terminate)) {
		void *msg = k_fifo_get(&rs_fifo, K_FOREVER);

		if (!msg) {
			LOG_INF("RS Thread timed out with no message");
			continue;
		}

		otdoa_ctrl_handle_message(msg);
		otdoa_message_free(msg);
	}

	LOG_INF("RS Thread Exit");
	atomic_clear(&gOTDOA.terminate);
}

void otdoa_queue_handle_http(struct k_work *work)
{
	if (!work) {
		return;
	}
	struct http_work *parent = CONTAINER_OF(work, struct http_work, work);

	if (parent) {
		if ((void *)parent == (void *)&http_stop_work) {
			/* Tell the RS to stop now that the pending HTTP operation has been stopped
			 */
			const struct http_stop_work_item *stop_req =
				CONTAINER_OF(work, struct http_stop_work_item, work);
			otdoa_rs_send_stop_req(stop_req->fail_or_cancel);
			return;
		}
		otdoa_http_handle_message(&parent->msg);
		otdoa_message_free(parent);
	}
}

int otdoa_queue_http_message(const void *msg, const size_t length)
{
	if (!msg) {
		LOG_ERR("no message");
		return -1;
	}

	struct http_work *work;

	work = otdoa_message_alloc(sizeof(struct k_work)+length);
	if (!work) {
		LOG_ERR("failed to alloate http message");
		return -1;
	}

	k_work_init(&work->work, otdoa_queue_handle_http);
	memcpy(&work->msg, msg, length);

	int rc = k_work_submit_to_queue(&http_workq, &work->work);

	if (rc != 1) {
		/* 0 and 2 are success values, but imply the work item has be
		 * double-allocated
		 */
		LOG_ERR("failed to queue http message: %d", rc);
		otdoa_message_free(work);
		return -1;
	}

	return 0;
}

int otdoa_queue_rs_message(const void *pv_msg, const size_t length)
{
	if (!pv_msg) {
		LOG_ERR("no message");
		return -1;
	}

	void *msg = otdoa_message_alloc(length);

	if (!msg) {
		LOG_ERR("failed to allocate rs message");
		return -1;
	}

	memcpy(msg, pv_msg, length);
	k_fifo_put(&rs_fifo, msg);

	return 0;
}

int otdoa_message_check_pending_stop(void)
{
	const bool pending = k_work_is_pending(&http_stop_work.work);

	if (pending) {
		k_work_cancel(&http_stop_work.work);
	}
	return pending;
}

int32_t otdoa_queue_stop_request(int fail_or_cancel)
{
	http_stop_work.fail_or_cancel = fail_or_cancel;
	k_work_init(&http_stop_work.work, otdoa_queue_handle_http);

	int rc = k_work_submit_to_queue(&http_workq, &http_stop_work.work);

	if (rc != 1) {
		/* 0 and 2 are success values, but imply the work item has be
		 * double-allocated
		 */
		LOG_ERR("failed to queue http stop message: %d", rc);
		return -1;
	}

	return 0;
}
