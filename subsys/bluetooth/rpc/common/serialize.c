/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <nrf_rpc_cbor.h>

#include "cbkproxy.h"
#include "serialize.h"

static inline bool is_decoder_invalid(const struct nrf_rpc_cbor_ctx *ctx)
{
	/* The logic is reversed */
	return !zcbor_check_error(ctx->zs);
}

void ser_decoder_invalid(struct nrf_rpc_cbor_ctx *ctx, int err)
{
	zcbor_error(ctx->zs, err);
}

static inline bool is_encoder_invalid(const struct nrf_rpc_cbor_ctx *ctx)
{
	return !zcbor_check_error(ctx->zs);
}

static inline bool is_decoder_invalid(const struct nrf_rpc_cbor_ctx *ctx)
{
	return !zcbor_check_error(ctx->zs);
}

static void set_encoder_invalid(struct nrf_rpc_cbor_ctx *ctx, int err)
{
	zcbor_error(ctx->zs, ZCBOR_ERR_UNKNOWN);
}

bool ser_decode_valid(CborValue *value)
{
	return !is_decoder_invalid(value);
}

bool ser_decode_valid(struct nrf_rpc_cbor_ctx *ctx)
{
	return !is_decoder_invalid(ctx);
}

static void check_final_decode_valid(const struct nrf_rpc_cbor_ctx *ctx)
{
	if (is_decoder_invalid(ctx)) {
		nrf_rpc_err(-EBADMSG, NRF_RPC_ERR_SRC_RECV, NULL,
			    NRF_RPC_ID_UNKNOWN, NRF_RPC_PACKET_TYPE_RSP);
	}
}

void ser_encode_null(struct nrf_rpc_cbor_ctx *ctx)
{
	zcbor_nll_put(ctx->zs, NULL);
}

void ser_encode_undefined(struct nrf_rpc_cbor_ctx *ctx)
{
	zcbor_undefined_put(ctx->zs, NULL);
}

void ser_encode_bool(struct nrf_rpc_cbor_ctx *ctx, bool value)
{
	zcbor_bool_put(ctx->zs, value);
}

void ser_encode_uint(struct nrf_rpc_cbor_ctx *ctx, uint32_t value)
{
	zcbor_uint32_put(ctx->zs, value);
}

void ser_encode_int(struct nrf_rpc_cbor_ctx *ctx, int32_t value)
{
	zcbor_int32_put(ctx->zs, value);
}

void ser_encode_str(struct nrf_rpc_cbor_ctx *ctx, const char *value, int len)
{
	zcbor_tstr_encode(ctx->zs, value, len);
}

void ser_encode_buffer(struct nrf_rpc_cbor_ctx *ctx, const void *data, size_t size)
{
	zcbor_bstr_encode(ctx->zs, data, size);
}

void ser_encode_callback(struct nrf_rpc_cbor_ctx *ctx, void *callback)
{
	int slot;

	if (is_encoder_invalid(ctx)) {
		return;
	}

	if (!callback) {
		zcbor_nil_put(ctx->zs);
	} else {
		slot = cbkproxy_in_set(callback);
		if (slot < 0) {
			set_encoder_invalid(ctx, ZCBOR_ERR_UNKNOWN);
		} else {
			zcbor_int32_encode(ctx->zs, slot);
		}
	}
}

void ser_decode_skip(struct nrf_rpc_cbor_ctx *ctx)
{
	zcbor_any_skip(ctx->zs);
}

bool ser_decode_is_null(struct nrf_rpc_cbor_ctx *ctx)
{
	return zcbor_nil_expect(ctx->zs, NULL);
}

bool ser_decode_is_undefined(struct nrf_rpc_cbor_ctx *ctx)
{
	return zcbor_undefined_expect(ctx->zs, NULL);
}

bool ser_decode_bool(struct nrf_rpc_cbor_ctx *ctx)
{
	bool result;

	if (zcbor_bool_decode(ctx->zs, &result)) {
		return result;
	}

	return false;
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
	CborError err = CborErrorIllegalType;
	void *result;
	size_t len;
	struct zcbor_string zst;

	if (is_decoder_invalid(value)) {
		return NULL;
	}

	if (cbor_value_is_byte_string(value)) {
		err = cbor_value_get_string_length(value, &len);
		if (err == CborErrorUnknownLength) {
			err = cbor_value_calculate_string_length(value, &len);
		}

		if (err != CborNoError) {
			goto error_exit;
		}

		if (len > buffer_size) {
			err = CborErrorIO;
			goto error_exit;
		}

		result = buffer;

		err = cbor_value_copy_byte_string(value, result, &len, value);
		if (err != CborNoError) {
			goto error_exit;
		}

	} else if (cbor_value_is_null(value)) {

		err = cbor_value_advance_fixed(value);
		if (err != CborNoError) {
			goto error_exit;
		}

		result = NULL;

	} else {
		goto error_exit;
	}

	return result;

error_exit:
	ser_decoder_invalid(value, err);
	return NULL;
}

size_t ser_decode_buffer_size(CborValue *value)
{
	CborError err = CborErrorIllegalType;
	size_t result;

	if (is_decoder_invalid(value)) {
		goto error_exit;
	}

	if (cbor_value_is_byte_string(value)) {
		err = cbor_value_get_string_length(value, &result);
		if (err == CborErrorUnknownLength) {
			err = cbor_value_calculate_string_length(value, &result);
		}

		if (err != CborNoError) {
			goto error_exit;
		}
	} else if (cbor_value_is_null(value)) {
		result = 0;
	} else {
		goto error_exit;
	}

	return result;

error_exit:
	ser_decoder_invalid(value, err);
	return 0;
}

char *ser_decode_str_into_scratchpad(struct ser_scratchpad *scratchpad)
{
	CborValue *value = scratchpad->value;
	CborError err = CborErrorIllegalType;
	char *result;
	size_t len;

	if (is_decoder_invalid(value)) {
		return NULL;
	}

	if (cbor_value_is_text_string(value)) {
		err = cbor_value_get_string_length(value, &len);
		if (err == CborErrorUnknownLength) {
			err = cbor_value_calculate_string_length(value, &len);
		}

		if (err != CborNoError) {
			goto error_exit;
		}

		result = (char *)ser_scratchpad_add(scratchpad, len);
		if (!result) {
			err = CborErrorIO;
			goto error_exit;
		}

		err = cbor_value_copy_text_string(value, result, &len, value);
		if (err != CborNoError) {
			goto error_exit;
		}

	} else if (cbor_value_is_null(value)) {
		err = cbor_value_advance_fixed(value);
		if (err != CborNoError) {
			goto error_exit;
		}

		result = NULL;
	} else {
		goto error_exit;
	}

	return result;

error_exit:
	ser_decoder_invalid(value, err);
	return NULL;
}

void *ser_decode_buffer_into_scratchpad(struct ser_scratchpad *scratchpad)
{
	CborValue *value = scratchpad->value;
	CborError err = CborErrorIllegalType;
	void *result;
	size_t len;

	if (is_decoder_invalid(value)) {
		return NULL;
	}

	if (cbor_value_is_byte_string(value)) {
		err = cbor_value_get_string_length(value, &len);
		if (err == CborErrorUnknownLength) {
			err = cbor_value_calculate_string_length(value, &len);
		}

		if (err != CborNoError) {
			goto error_exit;
		}

		result = ser_scratchpad_add(scratchpad, len);
		if (!result) {
			err = CborErrorIO;
			goto error_exit;
		}

		err = cbor_value_copy_byte_string(value, result, &len, value);
		if (err != CborNoError) {
			goto error_exit;
		}

	} else if (cbor_value_is_null(value)) {
		err = cbor_value_advance_fixed(value);
		if (err != CborNoError) {
			goto error_exit;
		}

		result = NULL;
	} else {
		goto error_exit;
	}

	return result;

error_exit:
	ser_decoder_invalid(value, err);
	return NULL;
}

void *ser_decode_callback_call(CborValue *value)
{
	int slot = ser_decode_uint(value);
	void *result = cbkproxy_in_get(slot);

	if (!result) {
		ser_decoder_invalid(value, CborErrorIO);
	}

	return result;
}

void *ser_decode_callback(CborValue *value, void *handler)
{
	CborError err = CborErrorIllegalType;
	int slot;
	void *result;

	if (is_decoder_invalid(value)) {
		return NULL;
	} else if (cbor_value_is_null(value)) {
		err = cbor_value_advance_fixed(value);
		if (err != CborNoError) {
			goto error_exit;
		}

		return NULL;

	} else if (cbor_value_is_integer(value)) {
		err = cbor_value_get_int(value, &slot);
		if (err != CborNoError) {
			goto error_exit;
		}

		err = cbor_value_advance_fixed(value);
		if (err != CborNoError) {
			goto error_exit;
		}
	} else {
		goto error_exit;
	}

	result = cbkproxy_out_get(slot, handler);
	if (!result) {
		goto error_exit;
	}

	return result;

error_exit:
	ser_decoder_invalid(value, err);
	return NULL;
}

bool ser_decoding_done_and_check(CborValue *value)
{
	nrf_rpc_cbor_decoding_done(value);
	return !is_decoder_invalid(value);
}

void ser_rsp_decode_i32(CborValue *value, void *handler_data)
{
	*(int32_t *)handler_data = ser_decode_int(value);
	check_final_decode_valid(value);
}

void ser_rsp_decode_bool(CborValue *value, void *handler_data)
{
	*(bool *)handler_data = ser_decode_bool(value);
	check_final_decode_valid(value);
}

void ser_rsp_decode_u8(CborValue *value, void *handler_data)
{
	*(uint8_t *)handler_data = ser_decode_int(value);
	check_final_decode_valid(value);
}

void ser_rsp_decode_u16(CborValue *value, void *handler_data)
{
	*(uint16_t *)handler_data = ser_decode_int(value);
	check_final_decode_valid(value);
}

void ser_rsp_decode_void(CborValue *value, void *handler_data)
{
	ARG_UNUSED(value);
	ARG_UNUSED(handler_data);
}

void ser_rsp_send_int(int32_t response)
{
	struct nrf_rpc_cbor_ctx ctx;

	NRF_RPC_CBOR_ALLOC(ctx, 1 + sizeof(int32_t));
	ser_encode_int(&ctx.encoder, response);

	nrf_rpc_cbor_rsp_no_err(&ctx);
}

void ser_rsp_send_uint(uint32_t response)
{
	struct nrf_rpc_cbor_ctx ctx;

	NRF_RPC_CBOR_ALLOC(ctx, 1 + sizeof(uint32_t));
	ser_encode_uint(&ctx.encoder, response);

	nrf_rpc_cbor_rsp_no_err(&ctx);
}

void ser_rsp_send_bool(bool response)
{
	struct nrf_rpc_cbor_ctx ctx;

	NRF_RPC_CBOR_ALLOC(ctx, 1);
	ser_encode_bool(&ctx.encoder, response);

	nrf_rpc_cbor_rsp_no_err(&ctx);
}

void ser_rsp_send_void(void)
{
	struct nrf_rpc_cbor_ctx ctx;

	NRF_RPC_CBOR_ALLOC(ctx, 0);

	nrf_rpc_cbor_rsp_no_err(&ctx);
}
