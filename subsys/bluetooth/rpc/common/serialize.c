

#include "nrf_rpc_cbor.h"
#include "cbkproxy.h"

#include "serialize.h"


#define ENCODER_FLAGS_INVALID 0x7FFFFFFF


static inline bool is_decoder_invalid(CborValue *value)
{
	return !value->parser;
}

static void set_decoder_invalid(CborValue *value, CborError err)
{
	if (is_decoder_invalid(value))
		return;
	
	value->parser = NULL;
	value->offset = err;
}


static inline bool is_encoder_invalid(CborEncoder *encoder)
{
	return encoder->flags == ENCODER_FLAGS_INVALID;
}

static void set_encoder_invalid(CborEncoder *encoder, CborError err)
{
	if (is_encoder_invalid(encoder))
		return;

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


void ser_encode_uint(CborEncoder *encoder, uint32_t value)
{
	CborError err;

	if (is_encoder_invalid(encoder))
		return;
	
	err = cbor_encode_uint(encoder, value);

	if (err != CborNoError) {
		set_encoder_invalid(encoder, err);
	}
}


void ser_encode_int(CborEncoder* encoder, int32_t value)
{
	CborError err;

	if (is_encoder_invalid(encoder))
		return;
	
	err = cbor_encode_int(encoder, value);

	if (err != CborNoError) {
		set_encoder_invalid(encoder, err);
	}
}


void ser_encode_callback(CborEncoder* encoder, void* callback)
{
	CborError err;
	int slot;

	if (is_encoder_invalid(encoder))
		return;

	if (callback == NULL) {
		err = cbor_encode_null(encoder);
	} else {
		slot = cbkproxy_in_set(callback);

		if (slot < 0) {
			err = CborUnknownError;
		} else {
			err = cbor_encode_int(encoder, slot);
		}
	}

	if (err != CborNoError)
		set_encoder_invalid(encoder, err);
}


uint32_t ser_decode_uint(CborValue *value)
{
	CborError err = CborErrorIllegalType;
	uint64_t result;

	if (is_decoder_invalid(value))
		return 0;

	if (cbor_value_is_integer(value)) {
		err = cbor_value_get_uint64(value, &result);
		if (result > (uint64_t)0xFFFFFFFF)
			err = CborErrorDataTooLarge;
		if (err != CborNoError)
			goto error_exit;
		err = cbor_value_advance_fixed(value);
		if (err != CborNoError)
			goto error_exit;
	} else {
		goto error_exit;
	}

	return result;

error_exit:
	set_decoder_invalid(value, err);
	return 0;
}

int32_t ser_decode_int(CborValue *value)
{
	CborError err = CborErrorIllegalType;
	int32_t result;

	if (is_decoder_invalid(value))
		return 0;

	if (cbor_value_is_integer(value)) {
		err = cbor_value_get_int_checked(value, &result);
		if (err != CborNoError)
			goto error_exit;
		err = cbor_value_advance_fixed(value);
		if (err != CborNoError)
			goto error_exit;
	} else {
		goto error_exit;
	}

	return result;

error_exit:
	set_decoder_invalid(value, err);
	return 0;
}


void* ser_decode_callback_slot(CborValue *value)
{
	int slot = ser_decode_uint(value);
	void* result = cbkproxy_in_get(slot);

	if (result == NULL)
		set_decoder_invalid(value, CborErrorIO);

	return result;
}


void* ser_decode_callback(CborValue *value, void* handler)
{
	CborError err = CborErrorIllegalType;
	int slot;
	void* result;

	if (is_decoder_invalid(value)) {
		return NULL;
	} else if (cbor_value_is_null(value)) {
		err = cbor_value_advance_fixed(value);
		if (err != CborNoError)
			goto error_exit;
		return NULL;
	} else if (cbor_value_is_integer(value)) {
		err = cbor_value_get_int(value, &slot);
		if (err != CborNoError)
			goto error_exit;
		err = cbor_value_advance_fixed(value);
		if (err != CborNoError)
			goto error_exit;
	} else {
		goto error_exit;
	}

	result = cbkproxy_out_get(slot, handler);

	if (result == NULL)
		goto error_exit;

	return result;

error_exit:
	set_decoder_invalid(value, err);
	return NULL;
}


bool ser_decoding_done_and_check(CborValue *value)
{
	nrf_rpc_cbor_decoding_done(value);
	return !is_decoder_invalid(value);
}


void ser_rsp_simple_i32(CborValue *value, void *handler_data)
{
	*(uint32_t *)handler_data = ser_decode_int(value);
	check_final_decode_valid(value);
}


void ser_rsp_send_i32(int32_t response)
{
	struct nrf_rpc_cbor_ctx ctx;

	NRF_RPC_CBOR_ALLOC(ctx, 1 + sizeof(int32_t));
	ser_encode_int(&ctx.encoder, response);

	nrf_rpc_cbor_rsp_no_err(&ctx);
}
