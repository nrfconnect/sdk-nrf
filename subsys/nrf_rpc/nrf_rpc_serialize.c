/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>
#include <nrf_rpc/nrf_rpc_cbkproxy.h>
#include <nrf_rpc/nrf_rpc_serialize.h>

static inline bool is_decoder_invalid(const struct nrf_rpc_cbor_ctx *ctx)
{
	/* The logic is reversed */
	return !zcbor_check_error(ctx->zs);
}

void nrf_rpc_decoder_invalid(struct nrf_rpc_cbor_ctx *ctx, int err)
{
	zcbor_error(ctx->zs, err);
}

static inline bool is_encoder_invalid(const struct nrf_rpc_cbor_ctx *ctx)
{
	return !zcbor_check_error(ctx->zs);
}

static void set_encoder_invalid(struct nrf_rpc_cbor_ctx *ctx, int err)
{
	zcbor_error(ctx->zs, err);
}

void nrf_rpc_encoder_invalid(struct nrf_rpc_cbor_ctx *ctx)
{
	zcbor_error(ctx->zs, ZCBOR_ERR_UNKNOWN);
}

bool nrf_rpc_decode_valid(const struct nrf_rpc_cbor_ctx *ctx)
{
	return !is_decoder_invalid(ctx);
}

static void check_final_decode_valid(const struct nrf_rpc_group *group,
				     const struct nrf_rpc_cbor_ctx *ctx)
{
	if (is_decoder_invalid(ctx)) {
		nrf_rpc_err(-EBADMSG, NRF_RPC_ERR_SRC_RECV, group, NRF_RPC_ID_UNKNOWN,
			    NRF_RPC_PACKET_TYPE_RSP);
	}
}

void nrf_rpc_encode_null(struct nrf_rpc_cbor_ctx *ctx)
{
	zcbor_nil_put(ctx->zs, NULL);
}

void nrf_rpc_encode_undefined(struct nrf_rpc_cbor_ctx *ctx)
{
	zcbor_undefined_put(ctx->zs, NULL);
}

void nrf_rpc_encode_bool(struct nrf_rpc_cbor_ctx *ctx, bool value)
{
	zcbor_bool_put(ctx->zs, value);
}

void nrf_rpc_encode_uint(struct nrf_rpc_cbor_ctx *ctx, uint32_t value)
{
	zcbor_uint32_put(ctx->zs, value);
}

void nrf_rpc_encode_int(struct nrf_rpc_cbor_ctx *ctx, int32_t value)
{
	zcbor_int32_put(ctx->zs, value);
}

void nrf_rpc_encode_uint64(struct nrf_rpc_cbor_ctx *ctx, uint64_t value)
{
	zcbor_uint64_put(ctx->zs, value);
}

void nrf_rpc_encode_int64(struct nrf_rpc_cbor_ctx *ctx, int64_t value)
{
	zcbor_int64_put(ctx->zs, value);
}

void nrf_rpc_encode_str(struct nrf_rpc_cbor_ctx *ctx, const char *value, int len)
{
	if (!value) {
		zcbor_nil_put(ctx->zs, NULL);
	} else {
		if (len < 0) {
			len = strlen(value);
		}
		zcbor_tstr_encode_ptr(ctx->zs, value, len);
	}
}

void nrf_rpc_encode_buffer(struct nrf_rpc_cbor_ctx *ctx, const void *data, size_t size)
{
	if (!data) {
		zcbor_nil_put(ctx->zs, NULL);
	} else {
		zcbor_bstr_encode_ptr(ctx->zs, data, size);
	}
}

void nrf_rpc_encode_callback(struct nrf_rpc_cbor_ctx *ctx, void *callback)
{
	int slot;

	if (is_encoder_invalid(ctx)) {
		return;
	}

	if (!callback) {
		zcbor_nil_put(ctx->zs, NULL);
	} else {
		slot = nrf_rpc_cbkproxy_in_set(callback);
		if (slot < 0) {
			set_encoder_invalid(ctx, ZCBOR_ERR_UNKNOWN);
		} else {
			zcbor_int32_put(ctx->zs, slot);
		}
	}
}

void nrf_rpc_decode_skip(struct nrf_rpc_cbor_ctx *ctx)
{
	zcbor_any_skip(ctx->zs, NULL);
}

bool nrf_rpc_decode_is_null(struct nrf_rpc_cbor_ctx *ctx)
{
	if (is_encoder_invalid(ctx)) {
		return false;
	}

	if (zcbor_nil_expect(ctx->zs, NULL)) {
		return true;
	}

	switch (ctx->zs->constant_state->error) {
	/* The next data item is not a simple value (Major 7) */
	case ZCBOR_ERR_WRONG_TYPE:
	/* The next data item is a simple value other than null (for example, a bool) */
	case ZCBOR_ERR_WRONG_VALUE:
		zcbor_pop_error(ctx->zs);
		break;
	default:
		break;
	}

	return false;
}

bool nrf_rpc_decode_is_undefined(struct nrf_rpc_cbor_ctx *ctx)
{
	if (is_encoder_invalid(ctx)) {
		return false;
	}

	if (zcbor_undefined_expect(ctx->zs, NULL)) {
		return true;
	}

	if (ctx->zs->constant_state->error == ZCBOR_ERR_WRONG_TYPE) {
		zcbor_pop_error(ctx->zs);
	}

	return false;
}

bool nrf_rpc_decode_bool(struct nrf_rpc_cbor_ctx *ctx)
{
	bool result;

	if (zcbor_bool_decode(ctx->zs, &result)) {
		return result;
	}

	return false;
}

uint32_t nrf_rpc_decode_uint(struct nrf_rpc_cbor_ctx *ctx)
{
	uint32_t result;

	if (zcbor_uint32_decode(ctx->zs, &result)) {
		return result;
	}

	return 0;
}

int32_t nrf_rpc_decode_int(struct nrf_rpc_cbor_ctx *ctx)
{
	int32_t result;

	if (zcbor_int32_decode(ctx->zs, &result)) {
		return result;
	}

	return 0;
}

uint64_t nrf_rpc_decode_uint64(struct nrf_rpc_cbor_ctx *ctx)
{
	uint64_t result;

	if (zcbor_uint64_decode(ctx->zs, &result)) {
		return result;
	}

	return 0;
}

int64_t nrf_rpc_decode_int64(struct nrf_rpc_cbor_ctx *ctx)
{
	int64_t result;

	if (zcbor_int64_decode(ctx->zs, &result)) {
		return result;
	}

	return 0;
}

void *nrf_rpc_decode_buffer(struct nrf_rpc_cbor_ctx *ctx, void *buffer, size_t buffer_size)
{
	struct zcbor_string zst = {0};

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

const void *nrf_rpc_decode_buffer_ptr_and_size(struct nrf_rpc_cbor_ctx *ctx, size_t *size)
{
	struct zcbor_string zst = {0};

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

	*size = zst.len;
	return zst.value;
}

char *nrf_rpc_decode_str(struct nrf_rpc_cbor_ctx *ctx, char *buffer, size_t buffer_size)
{
	struct zcbor_string zst;

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

	if (!zcbor_tstr_decode(ctx->zs, &zst)) {
		return NULL;
	}

	/* We need to have place for null terminator also. */
	if ((zst.len + 1) > buffer_size) {
		zcbor_error(ctx->zs, ZCBOR_ERR_UNKNOWN);
		return NULL;
	}

	memcpy(buffer, zst.value, zst.len);

	/* Add NULL terminator */
	buffer[zst.len] = '\0';

	return buffer;
}

char *nrf_rpc_decode_str_into_scratchpad(struct nrf_rpc_scratchpad *scratchpad, size_t *len)
{
	struct nrf_rpc_cbor_ctx *ctx = scratchpad->ctx;
	struct zcbor_string zst;
	char *result;
	int err = ZCBOR_ERR_WRONG_TYPE;

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

	if (!zcbor_tstr_decode(ctx->zs, &zst)) {
		return NULL;
	}

	/* Reserve place for string and a string NULL terminator. */
	result = (char *)nrf_rpc_scratchpad_add(scratchpad, (zst.len + 1));
	if (!result) {
		err = ZCBOR_ERR_UNKNOWN;
		goto error_exit;
	}

	memcpy(result, zst.value, zst.len);

	/* Add NULL terminator */
	result[zst.len] = '\0';

	if (len != NULL) {
		*len = zst.len;
	}
	return result;

error_exit:
	nrf_rpc_decoder_invalid(ctx, err);
	return NULL;
}

void *nrf_rpc_decode_buffer_into_scratchpad(struct nrf_rpc_scratchpad *scratchpad, size_t *len)
{
	struct nrf_rpc_cbor_ctx *ctx = scratchpad->ctx;
	struct zcbor_string zst;
	char *result;
	int err = ZCBOR_ERR_WRONG_TYPE;

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

	result = (char *)nrf_rpc_scratchpad_add(scratchpad, zst.len);
	if (!result) {
		err = ZCBOR_ERR_UNKNOWN;
		goto error_exit;
	}

	memcpy(result, zst.value, zst.len);

	if (len != NULL) {
		*len = zst.len;
	}

	return result;

error_exit:
	nrf_rpc_decoder_invalid(ctx, err);
	return NULL;
}

void *nrf_rpc_decode_callback_call(struct nrf_rpc_cbor_ctx *ctx)
{
	int slot = nrf_rpc_decode_uint(ctx);
	void *result = nrf_rpc_cbkproxy_in_get(slot);

	if (!result) {
		nrf_rpc_decoder_invalid(ctx, ZCBOR_ERR_WRONG_TYPE);
	}

	return result;
}

void *nrf_rpc_decode_callbackd(struct nrf_rpc_cbor_ctx *ctx, void *handler)
{
	int err = ZCBOR_ERR_WRONG_TYPE;
	int slot;
	void *result;

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

	if (!zcbor_int32_decode(ctx->zs, &slot)) {
		return NULL;
	}

	result = nrf_rpc_cbkproxy_out_get(slot, handler);
	if (!result) {
		err = ZCBOR_ERR_UNKNOWN;
		goto error_exit;
	}

	return result;

error_exit:
	nrf_rpc_decoder_invalid(ctx, err);
	return NULL;
}

bool nrf_rpc_decoding_done_and_check(const struct nrf_rpc_group *group,
				     struct nrf_rpc_cbor_ctx *ctx)
{
	nrf_rpc_cbor_decoding_done(group, ctx);
	return !is_decoder_invalid(ctx);
}

void nrf_rpc_rsp_decode_bool(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
			     void *handler_data)
{
	*(bool *)handler_data = nrf_rpc_decode_bool(ctx);
	check_final_decode_valid(group, ctx);
}

void nrf_rpc_rsp_decode_uint(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
			     void *result, size_t result_size)
{
	if (!zcbor_uint_decode(ctx->zs, result, result_size)) {
		memset(result, 0, result_size);
	}

	check_final_decode_valid(group, ctx);
}

void nrf_rpc_rsp_decode_u8(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
			   void *handler_data)
{
	nrf_rpc_rsp_decode_uint(group, ctx, handler_data, sizeof(uint8_t));
}

void nrf_rpc_rsp_decode_u16(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
			    void *handler_data)
{
	nrf_rpc_rsp_decode_uint(group, ctx, handler_data, sizeof(uint16_t));
}

void nrf_rpc_rsp_decode_u32(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
			    void *handler_data)
{
	nrf_rpc_rsp_decode_uint(group, ctx, handler_data, sizeof(uint32_t));
}

void nrf_rpc_rsp_decode_int(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
			    void *result, size_t result_size)
{
	if (!zcbor_int_decode(ctx->zs, result, result_size)) {
		memset(result, 0, result_size);
	}

	check_final_decode_valid(group, ctx);
}

void nrf_rpc_rsp_decode_i8(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
			   void *handler_data)
{
	nrf_rpc_rsp_decode_int(group, ctx, handler_data, sizeof(int8_t));
}

void nrf_rpc_rsp_decode_i16(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
			    void *handler_data)
{
	nrf_rpc_rsp_decode_int(group, ctx, handler_data, sizeof(int16_t));
}

void nrf_rpc_rsp_decode_i32(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
			    void *handler_data)
{
	nrf_rpc_rsp_decode_int(group, ctx, handler_data, sizeof(int32_t));
}

void nrf_rpc_rsp_decode_void(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
			     void *handler_data)
{
	ARG_UNUSED(ctx);
	ARG_UNUSED(handler_data);
}

void nrf_rpc_rsp_send_int(const struct nrf_rpc_group *group, int32_t response)
{
	struct nrf_rpc_cbor_ctx ctx;

	NRF_RPC_CBOR_ALLOC(group, ctx, 1 + sizeof(int32_t));
	nrf_rpc_encode_int(&ctx, response);

	nrf_rpc_cbor_rsp_no_err(group, &ctx);
}

void nrf_rpc_rsp_send_uint(const struct nrf_rpc_group *group, uint32_t response)
{
	struct nrf_rpc_cbor_ctx ctx;

	NRF_RPC_CBOR_ALLOC(group, ctx, 1 + sizeof(uint32_t));
	nrf_rpc_encode_uint(&ctx, response);

	nrf_rpc_cbor_rsp_no_err(group, &ctx);
}

void nrf_rpc_rsp_send_bool(const struct nrf_rpc_group *group, bool response)
{
	struct nrf_rpc_cbor_ctx ctx;

	NRF_RPC_CBOR_ALLOC(group, ctx, 1);
	nrf_rpc_encode_bool(&ctx, response);

	nrf_rpc_cbor_rsp_no_err(group, &ctx);
}

void nrf_rpc_rsp_send_void(const struct nrf_rpc_group *group)
{
	struct nrf_rpc_cbor_ctx ctx;

	NRF_RPC_CBOR_ALLOC(group, ctx, 0);

	nrf_rpc_cbor_rsp_no_err(group, &ctx);
}
