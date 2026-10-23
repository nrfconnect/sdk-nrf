/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * DECT NR+ RPC unit-test transport: extends the stock tests/mocks/nrf_rpc mock with a
 * larger expect queue, wildcard slots, cmd_ctx id tolerance on byte 0, and sent_count().
 */

#include "zephyr/ztest_assert.h"
#include <mock_dect_rpc_transport.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#include <string.h>

#define MAX_NUM_EXPECTED_PKTS CONFIG_MOCK_DECT_RPC_TR_MAX_EXPECTED_PKTS

#define MOCK_DECT_RPC_PKT_TYPE_CMD	      0x80
#define MOCK_DECT_RPC_CMD_CTX_ID_MASK	      0x07
#define MOCK_DECT_RPC_CMD_HDR_WITHOUT_CTX_ID 0xF8

BUILD_ASSERT(CONFIG_MOCK_NRF_RPC_TR_CB_THREAD_PRIO > CONFIG_NRF_RPC_THREAD_PRIORITY,
	     "The callback thread must have a lower priority than the thread pool");

typedef struct mock_dect_rpc_tr_ctx {
	const struct nrf_rpc_tr *transport;
	nrf_rpc_tr_receive_handler_t receive_cb;
	void *receive_ctx;

	mock_dect_rpc_pkt_t expected[MAX_NUM_EXPECTED_PKTS];
	mock_dect_rpc_pkt_t response[MAX_NUM_EXPECTED_PKTS];
	size_t num_expected;
	size_t cur_expected;

	struct k_msgq cb_msgq;
	char cb_msgq_buffer[MAX_NUM_EXPECTED_PKTS * sizeof(mock_dect_rpc_pkt_t)];

	struct k_sem cb_sem;
	struct k_thread cb_thread;

	K_KERNEL_STACK_MEMBER(cb_thread_stack, CONFIG_MOCK_NRF_RPC_TR_CB_THREAD_STACK_SIZE);
} mock_dect_rpc_tr_ctx_t;

static void log_payload(const char *caption, const uint8_t *payload, size_t length)
{
	char payload_str[128];

	if (!bin2hex(payload, length, payload_str, sizeof(payload_str))) {
		size_t printed_length = (sizeof(payload_str) - sizeof("...")) / 2;

		bin2hex(payload, printed_length, payload_str, sizeof(payload_str));
		strcat(payload_str, "...");
	}

	printk("%s: %s of length %zu\n", caption, payload_str, length);
}

static void cb_thread(void *tr_ctx, void *p2, void *p3)
{
	mock_dect_rpc_pkt_t packet;
	mock_dect_rpc_tr_ctx_t *ctx = tr_ctx;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	for (;;) {
		k_msgq_get(&ctx->cb_msgq, &packet, K_FOREVER);
		ctx->receive_cb(ctx->transport, packet.data, packet.len, ctx->receive_ctx);
		if (ctx->num_expected == ctx->cur_expected) {
			k_sem_give(&ctx->cb_sem);
		}
	}
}

static int init(const struct nrf_rpc_tr *transport, nrf_rpc_tr_receive_handler_t receive_cb,
		void *context)
{
	mock_dect_rpc_tr_ctx_t *ctx = transport->ctx;

	ctx->transport = transport;
	ctx->receive_cb = receive_cb;
	ctx->receive_ctx = context;

	k_sem_init(&ctx->cb_sem, 0, 1);

	k_msgq_init(&ctx->cb_msgq, ctx->cb_msgq_buffer, sizeof(mock_dect_rpc_pkt_t),
		    MAX_NUM_EXPECTED_PKTS);

	(void)k_thread_create(&ctx->cb_thread, ctx->cb_thread_stack,
			      K_KERNEL_STACK_SIZEOF(ctx->cb_thread_stack), cb_thread, ctx, NULL,
			      NULL, CONFIG_MOCK_NRF_RPC_TR_CB_THREAD_PRIO, 0, K_NO_WAIT);

	return 0;
}

static bool expected_cmd_with_ctx_id_zero(const mock_dect_rpc_pkt_t *expected, size_t length)
{
	if (length <= 1 || expected->data == NULL) {
		return false;
	}

	if ((expected->data[0] & MOCK_DECT_RPC_CMD_CTX_ID_MASK) != 0) {
		return false;
	}

	return (expected->data[0] & MOCK_DECT_RPC_PKT_TYPE_CMD) != 0;
}

static void assert_sent_matches_expected(const mock_dect_rpc_pkt_t *expected,
					 const uint8_t *data, size_t length)
{
	if (expected->len != length) {
		zexpect_true(false, "Unexpected nRF RPC packet sent");
		return;
	}

	if (expected_cmd_with_ctx_id_zero(expected, length) &&
	    (data[0] & MOCK_DECT_RPC_PKT_TYPE_CMD) != 0) {
		zexpect_equal(expected->data[0] & MOCK_DECT_RPC_CMD_HDR_WITHOUT_CTX_ID,
			      data[0] & MOCK_DECT_RPC_CMD_HDR_WITHOUT_CTX_ID,
			      "Unexpected nRF RPC command header (byte 0)");
		zexpect_mem_equal(expected->data + 1, data + 1, length - 1,
				  "Unexpected nRF RPC packet sent");
		return;
	}

	zexpect_mem_equal(expected->data, data, length, "Unexpected nRF RPC packet sent");
}

static int send(const struct nrf_rpc_tr *transport, const uint8_t *data, size_t length)
{
	mock_dect_rpc_tr_ctx_t *ctx = transport->ctx;
	mock_dect_rpc_pkt_t *expected, *response;

	log_payload("Sending nRF RPC packet", data, length);

	zassert_not_equal(ctx->cur_expected, ctx->num_expected, "Unexpected nRF RPC packet sent");

	expected = &ctx->expected[ctx->cur_expected];
	response = &ctx->response[ctx->cur_expected];
	++ctx->cur_expected;

	if (expected->data == NULL && expected->len == 0) {
		printk("Expected nRF RPC packet: (any)\n");
	} else {
		log_payload("Expected nRF RPC packet", expected->data, expected->len);
	}

	if (expected->data == NULL && expected->len == 0) {
		/* Wildcard slot: accept any single outgoing packet. */
	} else {
		assert_sent_matches_expected(expected, data, length);
	}
	k_free((void *)data);

	if (response->len > 0) {
		zassert_true(k_msgq_put(&ctx->cb_msgq, response, K_NO_WAIT) == 0);
	}

	return 0;
}

static void *tx_buf_alloc(const struct nrf_rpc_tr *transport, size_t *size)
{
	void *data = k_malloc(*size);

	zassert_not_null(data);

	return data;
}

static void tx_buf_free(const struct nrf_rpc_tr *transport, void *buf)
{
	k_free(buf);
}

static mock_dect_rpc_tr_ctx_t mock_dect_rpc_tr_ctx;

static const struct nrf_rpc_tr_api mock_dect_rpc_tr_api = {
	.init = init,
	.send = send,
	.tx_buf_alloc = tx_buf_alloc,
	.tx_buf_free = tx_buf_free,
};

const struct nrf_rpc_tr mock_dect_rpc_tr = {
	.api = &mock_dect_rpc_tr_api,
	.ctx = &mock_dect_rpc_tr_ctx,
};

void mock_dect_rpc_tr_expect_add(mock_dect_rpc_pkt_t expect, mock_dect_rpc_pkt_t response)
{
	mock_dect_rpc_tr_ctx_t *ctx = mock_dect_rpc_tr.ctx;

	zassert_true(ctx->num_expected < MAX_NUM_EXPECTED_PKTS,
		     "No more nRF RPC expected packets can be defined (increase "
		     "CONFIG_MOCK_DECT_RPC_TR_MAX_EXPECTED_PKTS)");

	ctx->expected[ctx->num_expected] = expect;
	ctx->response[ctx->num_expected] = response;
	++ctx->num_expected;
}

void mock_dect_rpc_tr_expect_done(void)
{
	mock_dect_rpc_tr_ctx_t *ctx = mock_dect_rpc_tr.ctx;

	(void)k_sem_take(&ctx->cb_sem, K_FOREVER);

	zexpect_equal(ctx->cur_expected, ctx->num_expected,
		      "%zu nRF RPC packets expected but not sent",
		      ctx->num_expected - ctx->cur_expected);

	ctx->num_expected = 0;
	ctx->cur_expected = 0;
}

void mock_dect_rpc_tr_expect_reset(void)
{
	mock_dect_rpc_tr_ctx_t *ctx = mock_dect_rpc_tr.ctx;

	ctx->num_expected = 0;
	ctx->cur_expected = 0;
}

size_t mock_dect_rpc_tr_sent_count(void)
{
	mock_dect_rpc_tr_ctx_t *ctx = mock_dect_rpc_tr.ctx;

	return ctx->cur_expected;
}

void mock_dect_rpc_tr_receive(mock_dect_rpc_pkt_t packet)
{
	mock_dect_rpc_tr_ctx_t *ctx = mock_dect_rpc_tr.ctx;

	zassert_not_null(ctx->receive_cb);

	log_payload("Received nRF RPC packet", packet.data, packet.len);

	zassert_true(k_msgq_put(&ctx->cb_msgq, &packet, K_NO_WAIT) == 0);
}
