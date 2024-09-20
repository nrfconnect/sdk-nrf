/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "zephyr/ztest_assert.h"
#include <mock_nrf_rpc_transport.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#include <string.h>

#define MAX_NUM_EXPECTED_PKTS 5

typedef struct mock_nrf_rpc_tr_ctx {
	const struct nrf_rpc_tr *transport;
	nrf_rpc_tr_receive_handler_t receive_cb;
	void *receive_ctx;

	mock_nrf_rpc_pkt_t expected[MAX_NUM_EXPECTED_PKTS];
	mock_nrf_rpc_pkt_t response[MAX_NUM_EXPECTED_PKTS];
	size_t num_expected;
	size_t cur_expected;

	mock_nrf_rpc_pkt_t *cur_response;
	struct k_work response_work;
} mock_nrf_rpc_tr_ctx_t;

static void log_payload(const char *caption, const uint8_t *payload, size_t length)
{
	char payload_str[32];

	if (!bin2hex(payload, length, payload_str, sizeof(payload_str))) {
		/*
		 * If bin2hex() returns 0, this means that the payload converted to HEX is too
		 * large to fit within 'payload_str'. In such a case, log as long prefix of the
		 * payload as possible, followed by "...". The code below calculates the prefix
		 * length.
		 */
		size_t printed_length = (sizeof(payload_str) - sizeof("...")) / 2;

		bin2hex(payload, printed_length, payload_str, sizeof(payload_str));
		strcat(payload_str, "...");
	}

	printk("%s: %s of length %zu\n", caption, payload_str, length);
}

/* Asynchronous task to simulate reception of a response packet. */
static void response_send(struct k_work *work)
{
	mock_nrf_rpc_tr_ctx_t *ctx = CONTAINER_OF(work, mock_nrf_rpc_tr_ctx_t, response_work);
	mock_nrf_rpc_pkt_t *response = ctx->cur_response;

	log_payload("Responding with nRF RPC packet", response->data, response->len);

	ctx->receive_cb(ctx->transport, response->data, response->len, ctx->receive_ctx);
}

static int init(const struct nrf_rpc_tr *transport, nrf_rpc_tr_receive_handler_t receive_cb,
		void *context)
{
	mock_nrf_rpc_tr_ctx_t *ctx = transport->ctx;

	ctx->transport = transport;
	ctx->receive_cb = receive_cb;
	ctx->receive_ctx = context;

	k_work_init(&ctx->response_work, response_send);

	return 0;
}

static int send(const struct nrf_rpc_tr *transport, const uint8_t *data, size_t length)
{
	mock_nrf_rpc_tr_ctx_t *ctx = transport->ctx;
	mock_nrf_rpc_pkt_t *expected, *response;

	log_payload("Sending nRF RPC packet", data, length);

	zassert_not_equal(ctx->cur_expected, ctx->num_expected, "Unexpected nRF RPC packet sent");

	expected = &ctx->expected[ctx->cur_expected];
	response = &ctx->response[ctx->cur_expected];
	++ctx->cur_expected;
	log_payload("Expected nRF RPC packet", expected->data, expected->len);

	if (expected->len != length) {
		zexpect_true(false, "Unexpected nRF RPC packet sent");
	} else {
		zexpect_mem_equal(expected->data, data, length, "Unexpected nRF RPC packet sent");
	}
	k_free((void *)data);

	if (response->len > 0) {
		/* nRF RPC can't handle a synchronous response, so send it asynchronously. */
		ctx->cur_response = response;
		zassert_true(k_work_submit(&ctx->response_work) >= 0);
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
	return k_free(buf);
}

/* Mock nRF RPC transport context object. */
static mock_nrf_rpc_tr_ctx_t mock_nrf_rpc_tr_ctx;

/* Mock nRF RPC transport API object. */
static const struct nrf_rpc_tr_api mock_nrf_rpc_tr_api = {
	.init = init,
	.send = send,
	.tx_buf_alloc = tx_buf_alloc,
	.tx_buf_free = tx_buf_free,
};

/* Mock nRF RPC transport object. */
const struct nrf_rpc_tr mock_nrf_rpc_tr = {
	.api = &mock_nrf_rpc_tr_api,
	.ctx = &mock_nrf_rpc_tr_ctx,
};

void mock_nrf_rpc_tr_expect_add(mock_nrf_rpc_pkt_t expect, mock_nrf_rpc_pkt_t response)
{
	mock_nrf_rpc_tr_ctx_t *ctx = mock_nrf_rpc_tr.ctx;

	zassert_true(ctx->num_expected < MAX_NUM_EXPECTED_PKTS,
		     "No more nRF RPC expected packets can be defined");

	ctx->expected[ctx->num_expected] = expect;
	ctx->response[ctx->num_expected] = response;
	++ctx->num_expected;
}

void mock_nrf_rpc_tr_expect_done(void)
{
	mock_nrf_rpc_tr_ctx_t *ctx = mock_nrf_rpc_tr.ctx;

	zexpect_equal(ctx->cur_expected, ctx->num_expected,
		      "%zu nRF RPC packets expected but not sent",
		      ctx->num_expected - ctx->cur_expected);

	ctx->num_expected = 0;
	ctx->cur_expected = 0;
}

void mock_nrf_rpc_tr_expect_reset(void)
{
	mock_nrf_rpc_tr_ctx_t *ctx = mock_nrf_rpc_tr.ctx;

	ctx->num_expected = 0;
	ctx->cur_expected = 0;
}

void mock_nrf_rpc_tr_receive(mock_nrf_rpc_pkt_t packet)
{
	mock_nrf_rpc_tr_ctx_t *ctx = mock_nrf_rpc_tr.ctx;

	zassert_not_null(ctx->receive_cb);

	log_payload("Received nRF RPC packet", packet.data, packet.len);

	ctx->receive_cb(ctx->transport, packet.data, packet.len, ctx->receive_ctx);
}
