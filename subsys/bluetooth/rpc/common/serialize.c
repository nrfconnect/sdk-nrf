

#include "nrf_rpc_cbor.h"

#include "serialize.h"

bool ser_decode_valid(CborValue *value)
{
	return !!value->parser;
}

static void check_final_decode_valid(CborValue *value)
{
	if (!ser_decode_valid(value)) {
		nrf_rpc_err(-EBADMSG, NRF_RPC_ERR_SRC_RECV, NULL,
			    NRF_RPC_ID_UNKNOWN, NRF_RPC_PACKET_TYPE_RSP);
	}
}


void ser_encode_callback(CborEncoder* encoder, void* callback)
{

}


int32_t ser_decode_i32(CborValue *value)
{
	int32_t result;
	CborError err;

	if (!value->parser || !cbor_value_is_integer(value)) {
		value->parser = NULL;
		return 0;
	}

	err = cbor_value_get_int_checked(value, &result);
	if (err != CborNoError) {
		value->parser = NULL;
		return 0;
	}
	return err;
}


bool ser_decoding_done_and_check(CborValue *value)
{
	nrf_rpc_cbor_decoding_done(value);
	return !!value->parser;
}


void ser_rsp_simple_i32(CborValue *value, void *handler_data)
{
	*(uint32_t *)handler_data = ser_decode_i32(value);
	check_final_decode_valid(value);
}


void ser_encode_int32(CborEncoder* encoder, int32_t value)
{
	cbor_encode_int(encoder, value);
}


void ser_rsp_send_i32(int32_t response)
{
	struct nrf_rpc_cbor_ctx ctx;

	NRF_RPC_CBOR_ALLOC(ctx, 1 + sizeof(int32_t));
	ser_encode_int32(&ctx.encoder, response);

	nrf_rpc_cbor_rsp_no_err(&ctx);
}
