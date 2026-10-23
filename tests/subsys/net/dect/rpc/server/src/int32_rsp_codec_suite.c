/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * Roundtrip tests for dect_rpc_int32_rsp_encode/decode (CONNECT/DISCONNECT/IF_SEND rsp).
 */

#include "dect_rpc_common.h"

#include <mock_dect_rpc_transport.h>
#include <test_rpc_env.h>
#include <nrf_rpc.h>
#include <nrf_rpc/nrf_rpc_serialize.h>
#include <nrf_rpc_cbor.h>
#include <zcbor_encode.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

static void nrf_rpc_err_handler(const struct nrf_rpc_err_report *report)
{
	zassert_ok(report->code);
}

static void *suite_setup(void)
{
	mock_dect_rpc_tr_expect_add(RPC_INIT_REQ, RPC_INIT_RSP);
	zassert_ok(nrf_rpc_init(nrf_rpc_err_handler));
	mock_dect_rpc_tr_expect_reset();
	return NULL;
}

static size_t int32_cbor_encoded_len(struct nrf_rpc_cbor_ctx *ctx)
{
	return (size_t)(ctx->zs[0].payload_mut - ctx->out_packet);
}

static void int32_cbor_decode_ctx_init(struct nrf_rpc_cbor_ctx *ctx, const uint8_t *data,
				       size_t len)
{
	zcbor_new_decode_state(ctx->zs, ARRAY_SIZE(ctx->zs), data, len, 1, NULL, 0);
	ctx->zs->constant_state->stop_on_error = true;
}

static bool int32_roundtrip(int32_t in, int32_t *out)
{
	struct nrf_rpc_cbor_ctx enc;
	struct nrf_rpc_cbor_ctx dec;
	size_t len;
	bool ok;

	NRF_RPC_CBOR_ALLOC(&dect_rpc_group, enc, 8);
	dect_rpc_int32_rsp_encode(&enc, in);
	zassert_true(zcbor_nil_put(enc.zs, NULL), "int32 encode nil");
	len = int32_cbor_encoded_len(&enc);

	int32_cbor_decode_ctx_init(&dec, enc.out_packet, len);
	ok = dect_rpc_int32_rsp_decode(&dec, out);
	if (ok) {
		ok = nrf_rpc_decoding_done_and_check(&dect_rpc_group, &dec);
	}

	NRF_RPC_CBOR_DISCARD(&dect_rpc_group, enc);
	return ok;
}

ZTEST(dect_rpc_server_int32_rsp_codec, test_roundtrip_values)
{
	static const int32_t values[] = { 0, 42, -19, -ENODEV, 1500, -ENOMEM };
	int32_t out;

	for (size_t i = 0; i < ARRAY_SIZE(values); i++) {
		zassert_true(int32_roundtrip(values[i], &out));
		zassert_equal(out, values[i]);
	}
}

ZTEST_SUITE(dect_rpc_server_int32_rsp_codec, NULL, suite_setup, NULL, NULL, NULL);
