/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * Unit tests for dect_rpc_decode_skip_extra_ipv6_{addrs,prefixes} in common/dect_rpc_common.c.
 */

#include "dect_rpc_common.h"

#include <mock_dect_rpc_transport.h>
#include <test_rpc_env.h>
#include <nrf_rpc.h>
#include <nrf_rpc/nrf_rpc_serialize.h>
#include <nrf_rpc_cbor.h>

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#define SKIP_CBOR_MAX_PARAMS 255

#define IPV6_BSTR_BYTES                                                                            \
	0x20, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, \
		0x0f

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

static void cbor_decode_ctx_init(struct nrf_rpc_cbor_ctx *ctx, const uint8_t *data, size_t len)
{
	zcbor_new_decode_state(ctx->zs, ARRAY_SIZE(ctx->zs), data, len, SKIP_CBOR_MAX_PARAMS, NULL,
			       0);
	ctx->zs->constant_state->stop_on_error = true;
}

static void assert_cbor_fully_consumed(const struct nrf_rpc_cbor_ctx *ctx)
{
	zassert_equal(ctx->zs->payload_mut, ctx->zs->payload_end, "trailing CBOR after skip");
}

static void consume_one_prefix(struct nrf_rpc_cbor_ctx *ctx)
{
	size_t sz;

	zassert_not_null(nrf_rpc_decode_buffer_ptr_and_size(ctx, &sz));
	zassert_equal(sz, 16U);
	(void)nrf_rpc_decode_uint(ctx);
}

ZTEST(dect_rpc_skip_ipv6_codec, test_skip_prefixes_consumes_extra_entry)
{
	/* Two prefix tuples; skip the second after the first was applied. */
	static const uint8_t wire[] = {
		CBOR_BSTR(16, IPV6_BSTR_BYTES),
		CBOR_UINT8(64),
		CBOR_BSTR(16, IPV6_BSTR_BYTES),
		CBOR_UINT8(48),
	};
	struct nrf_rpc_cbor_ctx ctx;

	cbor_decode_ctx_init(&ctx, wire, sizeof(wire));
	consume_one_prefix(&ctx);

	dect_rpc_decode_skip_extra_ipv6_prefixes(&ctx, 2, 1);
	assert_cbor_fully_consumed(&ctx);
}

ZTEST(dect_rpc_skip_ipv6_codec, test_skip_prefixes_stops_on_empty_stream)
{
	static const uint8_t wire[] = { };
	struct nrf_rpc_cbor_ctx ctx;

	cbor_decode_ctx_init(&ctx, wire, sizeof(wire));
	dect_rpc_decode_skip_extra_ipv6_prefixes(&ctx, 1, 0);
	zassert_false(nrf_rpc_decode_valid(&ctx));
}

ZTEST(dect_rpc_skip_ipv6_codec, test_skip_prefixes_stops_on_non_bstr)
{
	static const uint8_t wire[] = { CBOR_UINT_SMALL(7) };
	struct nrf_rpc_cbor_ctx ctx;

	cbor_decode_ctx_init(&ctx, wire, sizeof(wire));
	dect_rpc_decode_skip_extra_ipv6_prefixes(&ctx, 1, 0);
	zassert_false(nrf_rpc_decode_valid(&ctx));
}

ZTEST(dect_rpc_skip_ipv6_codec, test_skip_prefixes_stops_after_bstr_without_len_uint)
{
	/* Prefix skip expects bstr then uint; truncated after bstr. */
	static const uint8_t wire[] = { CBOR_BSTR(16, IPV6_BSTR_BYTES) };
	struct nrf_rpc_cbor_ctx ctx;

	cbor_decode_ctx_init(&ctx, wire, sizeof(wire));
	dect_rpc_decode_skip_extra_ipv6_prefixes(&ctx, 1, 0);
	zassert_false(nrf_rpc_decode_valid(&ctx));
}

ZTEST(dect_rpc_skip_ipv6_codec, test_skip_addrs_consumes_extra_entry)
{
	static const uint8_t wire[] = {
		CBOR_BSTR(16, IPV6_BSTR_BYTES),
		CBOR_BSTR(16, IPV6_BSTR_BYTES),
	};
	struct nrf_rpc_cbor_ctx ctx;
	size_t sz;

	cbor_decode_ctx_init(&ctx, wire, sizeof(wire));
	zassert_not_null(nrf_rpc_decode_buffer_ptr_and_size(&ctx, &sz));
	zassert_equal(sz, 16U);

	dect_rpc_decode_skip_extra_ipv6_addrs(&ctx, 2, 1);
	assert_cbor_fully_consumed(&ctx);
}

ZTEST(dect_rpc_skip_ipv6_codec, test_skip_addrs_stops_on_empty_stream)
{
	static const uint8_t wire[] = { };
	struct nrf_rpc_cbor_ctx ctx;

	cbor_decode_ctx_init(&ctx, wire, sizeof(wire));
	dect_rpc_decode_skip_extra_ipv6_addrs(&ctx, 1, 0);
	zassert_false(nrf_rpc_decode_valid(&ctx));
}

ZTEST(dect_rpc_skip_ipv6_codec, test_skip_addrs_stops_on_non_bstr)
{
	static const uint8_t wire[] = { CBOR_FALSE };
	struct nrf_rpc_cbor_ctx ctx;

	cbor_decode_ctx_init(&ctx, wire, sizeof(wire));
	dect_rpc_decode_skip_extra_ipv6_addrs(&ctx, 1, 0);
	zassert_false(nrf_rpc_decode_valid(&ctx));
}

ZTEST(dect_rpc_skip_ipv6_codec, test_skip_addrs_stops_on_truncated_bstr)
{
	/* Declared 16-byte bstr but payload ends early. */
	static const uint8_t wire[] = { 0x50, 0xaa, 0xbb };
	struct nrf_rpc_cbor_ctx ctx;

	cbor_decode_ctx_init(&ctx, wire, sizeof(wire));
	dect_rpc_decode_skip_extra_ipv6_addrs(&ctx, 1, 0);
	zassert_false(nrf_rpc_decode_valid(&ctx));
}

ZTEST_SUITE(dect_rpc_skip_ipv6_codec, NULL, suite_setup, NULL, NULL, NULL);
