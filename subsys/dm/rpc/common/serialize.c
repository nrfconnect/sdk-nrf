/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>
#include "serialize.h"

static inline bool is_decoder_invalid(const struct nrf_rpc_cbor_ctx *ctx)
{
	return !zcbor_check_error(ctx->zs);
}

static inline bool is_encoder_invalid(const struct nrf_rpc_cbor_ctx *ctx)
{
	return !zcbor_check_error(ctx->zs);
}

static void check_final_decode_valid(const struct nrf_rpc_group *group,
				     const struct nrf_rpc_cbor_ctx *ctx)
{
	if (is_decoder_invalid(ctx)) {
		nrf_rpc_err(-EBADMSG, NRF_RPC_ERR_SRC_RECV, group,
			    NRF_RPC_ID_UNKNOWN, NRF_RPC_PACKET_TYPE_RSP);
	}
}

void ser_encode_uint(struct nrf_rpc_cbor_ctx *ctx, uint32_t value)
{
	zcbor_uint32_put(ctx->zs, value);
}

void ser_encode_int(struct nrf_rpc_cbor_ctx *ctx, int32_t value)
{
	zcbor_int32_put(ctx->zs, value);
}

void ser_encode_buffer(struct nrf_rpc_cbor_ctx *ctx, const void *data, size_t size)
{
	if (!data) {
		zcbor_nil_put(ctx->zs, NULL);
	} else {
		zcbor_bstr_encode_ptr(ctx->zs, data, size);
	}
}

uint32_t ser_decode_uint(struct nrf_rpc_cbor_ctx *ctx)
{
	uint32_t result;

	if (zcbor_uint32_decode(ctx->zs, &result)) {
		return result;
	}

	return 0;
}

int32_t ser_decode_int(struct nrf_rpc_cbor_ctx *ctx)
{
	int32_t result;

	if (zcbor_int32_decode(ctx->zs, &result)) {
		return result;
	}

	return 0;
}

void *ser_decode_buffer(struct nrf_rpc_cbor_ctx *ctx, void *buffer, size_t buffer_size)
{
	struct zcbor_string zst = { 0 };

	if (is_decoder_invalid(ctx)) {
		return NULL;
	}

	if (zcbor_nil_expect(ctx->zs, NULL)) {
		return NULL;
	}

	if (ctx->zs->constant_state->error != ZCBOR_ERR_WRONG_TYPE) {
		return NULL;
	}

	zcbor_pop_error(ctx->zs);

	if (!zcbor_bstr_decode(ctx->zs, &zst)) {
		return NULL;
	}

	if (zst.len > buffer_size) {
		zcbor_error(ctx->zs, ZCBOR_ERR_UNKNOWN);
		return NULL;
	}

	memcpy(buffer, zst.value, zst.len);
	return buffer;
}

bool ser_decoding_done_and_check(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx)
{
	nrf_rpc_cbor_decoding_done(group, ctx);
	return !is_decoder_invalid(ctx);
}

void ser_rsp_decode_i32(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
			 void *handler_data)
{
	*(int32_t *)handler_data = ser_decode_int(ctx);
	check_final_decode_valid(group, ctx);
}

void ser_rsp_send_int(const struct nrf_rpc_group *group, int32_t response)
{
	struct nrf_rpc_cbor_ctx ctx;

	NRF_RPC_CBOR_ALLOC(group, ctx, 1 + sizeof(int32_t));
	ser_encode_int(&ctx, response);

	nrf_rpc_cbor_rsp_no_err(group, &ctx);
}
