/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <nrf_rpc_cbor.h>

#include <zephyr.h>

#include "bluetooth/crypto.h"

#include "bt_rpc_common.h"
#include "serialize.h"

static void report_decoding_error(uint8_t cmd_evt_id, void *data)
{
	nrf_rpc_err(-EBADMSG, NRF_RPC_ERR_SRC_RECV, &bt_rpc_grp, cmd_evt_id,
		    NRF_RPC_PACKET_TYPE_CMD);
}

static void bt_encrypt_le_rpc_handler(CborValue *value, void *handler_data)
{

	const uint8_t *key;
	const uint8_t *plaintext;
	uint8_t *enc_data;
	int result;
	struct ser_scratchpad scratchpad;

	SER_SCRATCHPAD_DECLARE(&scratchpad, value);

	key = ser_decode_buffer_into_scratchpad(&scratchpad);
	plaintext = ser_decode_buffer_into_scratchpad(&scratchpad);
	enc_data = ser_decode_buffer_into_scratchpad(&scratchpad);

	if (!ser_decoding_done_and_check(value)) {
		goto decoding_error;
	}

	result = bt_encrypt_le(key, plaintext, enc_data);

	ser_rsp_send_int(result);

	return;
decoding_error:
	report_decoding_error(BT_ENCRYPT_LE_RPC_CMD, handler_data);

}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_encrypt_le, BT_ENCRYPT_LE_RPC_CMD,
	bt_encrypt_le_rpc_handler, NULL);

static void bt_encrypt_be_rpc_handler(CborValue *value, void *handler_data)
{

	const uint8_t *key;
	const uint8_t *plaintext;
	uint8_t *enc_data;
	int result;
	struct ser_scratchpad scratchpad;

	SER_SCRATCHPAD_DECLARE(&scratchpad, value);

	key = ser_decode_buffer_into_scratchpad(&scratchpad);
	plaintext = ser_decode_buffer_into_scratchpad(&scratchpad);
	enc_data = ser_decode_buffer_into_scratchpad(&scratchpad);

	if (!ser_decoding_done_and_check(value)) {
		goto decoding_error;
	}

	result = bt_encrypt_be(key, plaintext, enc_data);

	ser_rsp_send_int(result);

	return;
decoding_error:
	report_decoding_error(BT_ENCRYPT_BE_RPC_CMD, handler_data);

}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_encrypt_be, BT_ENCRYPT_BE_RPC_CMD,
	bt_encrypt_be_rpc_handler, NULL);

static void bt_rand_rpc_handler(CborValue *value, void *handler_data)
{

	struct nrf_rpc_cbor_ctx ctx;
	int result;
	size_t len;
	uint8_t *buf;
	size_t buffer_size_max = 10;
	struct ser_scratchpad scratchpad;

	SER_SCRATCHPAD_DECLARE(&scratchpad, value);

	len = ser_decode_uint(value);
	buf = ser_scratchpad_add(&scratchpad, sizeof(uint8_t) * len);

	if (!ser_decoding_done_and_check(value)) {
		goto decoding_error;
	}

	result = bt_rand(buf, len);

	buffer_size_max += sizeof(uint8_t) * len;

	{
		NRF_RPC_CBOR_ALLOC(ctx, buffer_size_max);

		ser_encode_int(&ctx.encoder, result);
		ser_encode_buffer(&ctx.encoder, buf, sizeof(uint8_t) * len);

		nrf_rpc_cbor_rsp_no_err(&ctx);
	}

	return;
decoding_error:
	report_decoding_error(BT_RAND_RPC_CMD, handler_data);

}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_rand, BT_RAND_RPC_CMD,
	bt_rand_rpc_handler, NULL);

#if defined(CONFIG_BT_HOST_CCM)
static void bt_ccm_encrypt_rpc_handler(CborValue *value, void *handler_data)
{

	struct nrf_rpc_cbor_ctx ctx;
	int result;
	const uint8_t *key;
	uint8_t *nonce;
	size_t len;
	const uint8_t *enc_data;
	size_t aad_len;
	const uint8_t *aad;
	uint8_t *plaintext;
	size_t mic_size;
	size_t buffer_size_max = 10;
	struct ser_scratchpad scratchpad;

	SER_SCRATCHPAD_DECLARE(&scratchpad, value);

	key = ser_decode_buffer_into_scratchpad(&scratchpad);
	nonce = ser_decode_buffer_into_scratchpad(&scratchpad);
	len = ser_decode_uint(value);
	enc_data = ser_decode_buffer_into_scratchpad(&scratchpad);
	aad_len = ser_decode_uint(value);
	aad = ser_decode_buffer_into_scratchpad(&scratchpad);
	plaintext = ser_decode_buffer_into_scratchpad(&scratchpad);
	mic_size = ser_decode_uint(value);

	if (!ser_decoding_done_and_check(value)) {
		goto decoding_error;
	}

	result = bt_ccm_encrypt(key, nonce, enc_data, len, aad, aad_len, plaintext, mic_size);

	buffer_size_max += sizeof(uint8_t) * (len + mic_size);

	{
		NRF_RPC_CBOR_ALLOC(ctx, buffer_size_max);

		ser_encode_int(&ctx.encoder, result);
		ser_encode_buffer(&ctx.encoder, plaintext, sizeof(uint8_t) * (len + mic_size);

		nrf_rpc_cbor_rsp_no_err(&ctx);
	}

	return;
decoding_error:
	report_decoding_error(BT_CCM_ENCRYPT_RPC_CMD, handler_data);

}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_ccm_encrypt, BT_CCM_ENCRYPT_RPC_CMD,
	bt_ccm_encrypt_rpc_handler, NULL);

static void bt_ccm_decrypt_rpc_handler(CborValue *value, void *handler_data)
{

	struct nrf_rpc_cbor_ctx ctx;
	int result;
	const uint8_t *key;
	uint8_t *nonce;
	size_t len;
	const uint8_t *enc_data;
	size_t aad_len;
	const uint8_t *aad;
	uint8_t *plaintext;
	size_t mic_size;
	size_t buffer_size_max = 10;
	struct ser_scratchpad scratchpad;

	SER_SCRATCHPAD_DECLARE(&scratchpad, value);

	key = ser_decode_buffer_into_scratchpad(&scratchpad);
	nonce = ser_decode_buffer_into_scratchpad(&scratchpad);
	len = ser_decode_uint(value);
	enc_data = ser_decode_buffer_into_scratchpad(&scratchpad);
	aad_len = ser_decode_uint(value);
	aad = ser_decode_buffer_into_scratchpad(&scratchpad);
	plaintext = ser_decode_buffer_into_scratchpad(&scratchpad);
	mic_size = ser_decode_uint(value);

	if (!ser_decoding_done_and_check(value)) {
		goto decoding_error;
	}

	result = bt_ccm_decrypt(key, nonce, enc_data, len, aad, aad_len, plaintext, mic_size);

	buffer_size_max += sizeof(uint8_t) * len;

	{
		NRF_RPC_CBOR_ALLOC(ctx, buffer_size_max);

		ser_encode_int(&ctx.encoder, result);
		ser_encode_buffer(&ctx.encoder, plaintext, sizeof(uint8_t) * len);

		nrf_rpc_cbor_rsp_no_err(&ctx);
	}

	return;
decoding_error:
	report_decoding_error(BT_CCM_DECRYPT_RPC_CMD, handler_data);
}

NRF_RPC_CBOR_CMD_DECODER(bt_rpc_grp, bt_ccm_decrypt, BT_CCM_DECRYPT_RPC_CMD,
	bt_ccm_decrypt_rpc_handler, NULL);
#endif /* defined(CONFIG_BT_HOST_CCM) */
