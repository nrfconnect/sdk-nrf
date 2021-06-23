/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <nrf_rpc_cbor.h>

#include "cbkproxy.h"
#include "serialize.h"

#define ENCODER_FLAGS_INVALID 0x7FFFFFFF

static inline bool is_decoder_invalid(CborValue *value)
{
	return !value->parser;
}

void ser_decoder_invalid(CborValue *value, CborError err)
{
	if (is_decoder_invalid(value)) {
		return;
	}

	value->parser = NULL;
	value->offset = err;
}

static inline bool is_encoder_invalid(CborEncoder *encoder)
{
	return encoder->flags == ENCODER_FLAGS_INVALID;
}

static void set_encoder_invalid(CborEncoder *encoder, CborError err)
{
	if (is_encoder_invalid(encoder)) {
		return;
	}

	encoder->flags = ENCODER_FLAGS_INVALID;
	encoder->added = err;
}

bool ser_decode_valid(CborValue *value)
{
	return !is_decoder_invalid(value);
}

static void check_final_decode_valid(CborValue *value)
{
	if (is_decoder_invalid(value)) {
		nrf_rpc_err(-EBADMSG, NRF_RPC_ERR_SRC_RECV, NULL,
			    NRF_RPC_ID_UNKNOWN, NRF_RPC_PACKET_TYPE_RSP);
	}
}

void ser_encode_null(CborEncoder *encoder)
{
	CborError err;

	if (is_encoder_invalid(encoder)) {
		return;
	}

	err = cbor_encode_null(encoder);

	if (err != CborNoError) {
		set_encoder_invalid(encoder, err);
	}
}

void ser_encode_undefined(CborEncoder *encoder)
{
	CborError err;

	if (is_encoder_invalid(encoder)) {
		return;
	}

	err = cbor_encode_undefined(encoder);
	if (err != CborNoError) {
		set_encoder_invalid(encoder, err);
	}
}

void ser_encode_bool(CborEncoder *encoder, bool value)
{
	CborError err;

	if (is_encoder_invalid(encoder)) {
		return;
	}

	err = cbor_encode_boolean(encoder, value);
	if (err != CborNoError) {
		set_encoder_invalid(encoder, err);
	}
}

void ser_encode_uint(CborEncoder *encoder, uint32_t value)
{
	CborError err;

	if (is_encoder_invalid(encoder)) {
		return;
	}

	err = cbor_encode_uint(encoder, value);
	if (err != CborNoError) {
		set_encoder_invalid(encoder, err);
	}
}


void ser_encode_int(CborEncoder *encoder, int32_t value)
{
	CborError err;

	if (is_encoder_invalid(encoder)) {
		return;
	}

	err = cbor_encode_int(encoder, value);
	if (err != CborNoError) {
		set_encoder_invalid(encoder, err);
	}
}

void ser_encode_str(CborEncoder *encoder, const char *value, int len)
{
	CborError err;

	if (is_encoder_invalid(encoder)) {
		return;
	}

	if (!value) {
		err = cbor_encode_null(encoder);
	} else if (len < 0) {
		err = cbor_encode_text_stringz(encoder, value);
	} else {
		err = cbor_encode_text_string(encoder, value, len);
	}

	if (err != CborNoError) {
		set_encoder_invalid(encoder, err);
	}

}

void ser_encode_buffer(CborEncoder *encoder, const void *data, size_t size)
{
	CborError err;

	if (is_encoder_invalid(encoder)) {
		return;
	}

	if (!data) {
		err = cbor_encode_null(encoder);
	} else {
		err = cbor_encode_byte_string(encoder, (const uint8_t *)data, size);
	}

	if (err != CborNoError) {
		set_encoder_invalid(encoder, err);
	}
}

void ser_encode_callback(CborEncoder *encoder, void *callback)
{
	CborError err;
	int slot;

	if (is_encoder_invalid(encoder)) {
		return;
	}

	if (!callback) {
		err = cbor_encode_null(encoder);
	} else {
		slot = cbkproxy_in_set(callback);
		if (slot < 0) {
			err = CborUnknownError;
		} else {
			err = cbor_encode_int(encoder, slot);
		}
	}

	if (err != CborNoError) {
		set_encoder_invalid(encoder, err);
	}
}

void ser_encoder_invalid(CborEncoder *encoder)
{
	set_encoder_invalid(encoder, CborErrorIO);
}


void ser_decode_skip(CborValue *value)
{
	CborError err;

	if (is_decoder_invalid(value)) {
		return;
	}

	err = cbor_value_advance(value);
	if (err != CborNoError) {
		ser_decoder_invalid(value, err);
	}
}

bool ser_decode_is_null(CborValue *value)
{
	if (is_decoder_invalid(value)) {
		return true;
	}

	return cbor_value_is_null(value);
}

bool ser_decode_is_undefined(CborValue *value)
{
	if (is_decoder_invalid(value)) {
		return true;
	}

	return cbor_value_is_undefined(value);
}

bool ser_decode_bool(CborValue *value)
{
	CborError err = CborErrorIllegalType;
	bool result;

	if (is_decoder_invalid(value)) {
		return 0;
	}

	if (cbor_value_is_boolean(value)) {
		err = cbor_value_get_boolean(value, &result);
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

	return result;

error_exit:
	ser_decoder_invalid(value, err);
	return 0;
}

uint32_t ser_decode_uint(CborValue *value)
{
	CborError err = CborErrorIllegalType;
	uint64_t result;

	if (is_decoder_invalid(value)) {
		return 0;
	}

	if (cbor_value_is_integer(value)) {
		err = cbor_value_get_uint64(value, &result);
		if (result > (uint64_t)0xFFFFFFFF) {
			err = CborErrorDataTooLarge;
		}

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

	return result;

error_exit:
	ser_decoder_invalid(value, err);
	return 0;
}

int32_t ser_decode_int(CborValue *value)
{
	CborError err = CborErrorIllegalType;
	int32_t result;

	if (is_decoder_invalid(value)) {
		return 0;
	}

	if (cbor_value_is_integer(value)) {
		err = cbor_value_get_int_checked(value, &result);
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

	return result;

error_exit:
	ser_decoder_invalid(value, err);
	return 0;
}

void *ser_decode_buffer(CborValue *value, void *buffer, size_t buffer_size)
{
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
