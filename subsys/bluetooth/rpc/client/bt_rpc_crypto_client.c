/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <sys/types.h>
#include <stddef.h>


#include <zephyr/bluetooth/crypto.h>

#include "bt_rpc_common.h"
#include "serialize.h"
#include "nrf_rpc_cbor.h"

struct bt_rand_rpc_res {
	int result;
	size_t len;
	uint8_t *buf;
};

static void bt_rand_rpc_rsp(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
			    void *handler_data)
{
	struct bt_rand_rpc_res *res =
		(struct bt_rand_rpc_res *)handler_data;

	res->result = ser_decode_int(ctx);
	ser_decode_buffer(ctx, res->buf, sizeof(uint8_t) * res->len);
}

int bt_rand(void *buf, size_t len)
{
	struct nrf_rpc_cbor_ctx ctx;
	struct bt_rand_rpc_res result;
	size_t scratchpad_size = 0;
	size_t buffer_size_max = 10;

	scratchpad_size += SCRATCHPAD_ALIGN(sizeof(uint8_t) * len);

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);
	ser_encode_uint(&ctx, scratchpad_size);
	ser_encode_uint(&ctx, len);

	result.len = len;
	result.buf = buf;

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_RAND_RPC_CMD,
		&ctx, bt_rand_rpc_rsp, &result);

	return result.result;
}

int bt_encrypt_le(const uint8_t key[16], const uint8_t plaintext[16],
		  uint8_t enc_data[16])
{
	struct nrf_rpc_cbor_ctx ctx;
	size_t key_size;
	size_t plaintext_size;
	size_t enc_data_size;
	int result;
	size_t scratchpad_size = 0;
	size_t buffer_size_max = 20;

	key_size = sizeof(uint8_t) * 16;
	buffer_size_max += key_size;
	plaintext_size = sizeof(uint8_t) * 16;
	buffer_size_max += plaintext_size;
	enc_data_size = sizeof(uint8_t) * 16;
	buffer_size_max += enc_data_size;

	scratchpad_size += SCRATCHPAD_ALIGN(key_size);
	scratchpad_size += SCRATCHPAD_ALIGN(plaintext_size);
	scratchpad_size += SCRATCHPAD_ALIGN(enc_data_size);

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	ser_encode_uint(&ctx, scratchpad_size);
	ser_encode_buffer(&ctx, key, key_size);
	ser_encode_buffer(&ctx, plaintext, plaintext_size);
	ser_encode_buffer(&ctx, enc_data, enc_data_size);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_ENCRYPT_LE_RPC_CMD,
		&ctx, ser_rsp_decode_i32, &result);

	return result;
}

int bt_encrypt_be(const uint8_t key[16], const uint8_t plaintext[16],
		  uint8_t enc_data[16])
{
	struct nrf_rpc_cbor_ctx ctx;
	size_t key_size;
	size_t plaintext_size;
	size_t enc_data_size;
	int result;
	size_t scratchpad_size = 0;
	size_t buffer_size_max = 20;

	key_size = sizeof(uint8_t) * 16;
	buffer_size_max += key_size;
	plaintext_size = sizeof(uint8_t) * 16;
	buffer_size_max += plaintext_size;
	enc_data_size = sizeof(uint8_t) * 16;
	buffer_size_max += enc_data_size;

	scratchpad_size += SCRATCHPAD_ALIGN(key_size);
	scratchpad_size += SCRATCHPAD_ALIGN(plaintext_size);
	scratchpad_size += SCRATCHPAD_ALIGN(enc_data_size);

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	ser_encode_uint(&ctx, scratchpad_size);
	ser_encode_buffer(&ctx, key, key_size);
	ser_encode_buffer(&ctx, plaintext, plaintext_size);
	ser_encode_buffer(&ctx, enc_data, enc_data_size);

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_ENCRYPT_BE_RPC_CMD,
		&ctx, ser_rsp_decode_i32, &result);

	return result;
}

#if defined(CONFIG_BT_HOST_CCM)
struct bt_ccm_decrypt_rpc_res {
	int result;
	size_t len;
	uint8_t *plaintext;
};

static void bt_ccm_decrypt_rpc_rsp(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
				   void *handler_data)
{

	struct bt_ccm_decrypt_rpc_res *res =
		(struct bt_ccm_decrypt_rpc_res *)handler_data;

	res->result = ser_decode_int(ctx);
	ser_decode_buffer(ctx, res->plaintext, sizeof(uint8_t) * res->len);
}

int bt_ccm_decrypt(const uint8_t key[16], uint8_t nonce[13], const uint8_t *enc_data,
		   size_t len, const uint8_t *aad, size_t aad_len,
		   uint8_t *plaintext, size_t mic_size)
{
	struct nrf_rpc_cbor_ctx ctx;
	size_t key_size;
	size_t nonce_size;
	size_t enc_data_size;
	size_t aad_size;
	size_t plaintext_size;
	struct bt_ccm_decrypt_rpc_res result;
	size_t scratchpad_size = 0;
	size_t buffer_size_max = 45;

	key_size = sizeof(uint8_t) * 16;
	buffer_size_max += key_size;
	nonce_size = sizeof(uint8_t) * 13;
	buffer_size_max += nonce_size;
	enc_data_size = sizeof(uint8_t) * len;
	buffer_size_max += enc_data_size;
	aad_size = sizeof(uint8_t) * aad_len;
	buffer_size_max += aad_size;
	plaintext_size = sizeof(uint8_t) * len;
	buffer_size_max += plaintext_size;

	scratchpad_size += SCRATCHPAD_ALIGN(key_size);
	scratchpad_size += SCRATCHPAD_ALIGN(nonce_size);
	scratchpad_size += SCRATCHPAD_ALIGN(enc_data_size);
	scratchpad_size += SCRATCHPAD_ALIGN(aad_size);
	scratchpad_size += SCRATCHPAD_ALIGN(plaintext_size);

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	ser_encode_uint(&ctx, scratchpad_size);
	ser_encode_buffer(&ctx, key, key_size);
	ser_encode_buffer(&ctx, nonce, nonce_size);
	ser_encode_uint(&ctx, len);
	ser_encode_buffer(&ctx, enc_data, enc_data_size);
	ser_encode_uint(&ctx, aad_len);
	ser_encode_buffer(&ctx, aad, aad_size);
	ser_encode_buffer(&ctx, plaintext, plaintext_size);
	ser_encode_uint(&ctx, mic_size);

	result.len = len;
	result.plaintext = plaintext;

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_CCM_DECRYPT_RPC_CMD,
		&ctx, bt_ccm_decrypt_rpc_rsp, &result);

	return result.result;
}

struct bt_ccm_encrypt_rpc_res {
	int result;
	size_t len;
	uint8_t *plaintext;
};

static void bt_ccm_encrypt_rpc_rsp(const struct nrf_rpc_group *group, struct nrf_rpc_cbor_ctx *ctx,
				   void *handler_data)
{
	struct bt_ccm_encrypt_rpc_res *res =
		(struct bt_ccm_encrypt_rpc_res *)handler_data;

	res->result = ser_decode_int(ctx);
	ser_decode_buffer(ctx, res->plaintext, sizeof(uint8_t) * res->len);
}

int bt_ccm_encrypt(const uint8_t key[16], uint8_t nonce[13], const uint8_t *enc_data,
		   size_t len, const uint8_t *aad, size_t aad_len,
		   uint8_t *plaintext, size_t mic_size)
{
	struct nrf_rpc_cbor_ctx ctx;
	size_t key_size;
	size_t nonce_size;
	size_t enc_data_size;
	size_t aad_size;
	size_t plaintext_size;
	struct bt_ccm_encrypt_rpc_res result;
	size_t scratchpad_size = 0;
	size_t buffer_size_max = 45;

	key_size = sizeof(uint8_t) * 16;
	buffer_size_max += key_size;
	nonce_size = sizeof(uint8_t) * 13;
	buffer_size_max += nonce_size;
	enc_data_size = sizeof(uint8_t) * len;
	buffer_size_max += enc_data_size;
	aad_size = sizeof(uint8_t) * aad_len;
	buffer_size_max += aad_size;
	plaintext_size = sizeof(uint8_t) * len;
	buffer_size_max += plaintext_size;

	scratchpad_size += SCRATCHPAD_ALIGN(key_size);
	scratchpad_size += SCRATCHPAD_ALIGN(nonce_size);
	scratchpad_size += SCRATCHPAD_ALIGN(enc_data_size);
	scratchpad_size += SCRATCHPAD_ALIGN(aad_size);
	scratchpad_size += SCRATCHPAD_ALIGN(plaintext_size + mic_size);

	NRF_RPC_CBOR_ALLOC(&bt_rpc_grp, ctx, buffer_size_max);

	ser_encode_uint(&ctx, scratchpad_size);
	ser_encode_buffer(&ctx, key, key_size);
	ser_encode_buffer(&ctx, nonce, nonce_size);
	ser_encode_uint(&ctx, len);
	ser_encode_buffer(&ctx, enc_data, enc_data_size);
	ser_encode_uint(&ctx, aad_len);
	ser_encode_buffer(&ctx, aad, aad_size);
	ser_encode_buffer(&ctx, plaintext, plaintext_size + mic_size);
	ser_encode_uint(&ctx, mic_size);

	result.len = len;
	result.plaintext = plaintext;

	nrf_rpc_cbor_cmd_no_err(&bt_rpc_grp, BT_CCM_ENCRYPT_RPC_CMD,
		&ctx, bt_ccm_encrypt_rpc_rsp, &result);

	return result.result;
}
#endif /* defined(CONFIG_BT_HOST_CCM) */
