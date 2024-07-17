/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
#include <suit_decrypt_filter.h>
#include <suit_types.h>
#include <suit_plat_decode_util.h>
#include <psa/crypto.h>
#include "suit_aes_key_unwrap_manual.h"

/**
 * @brief Chunk size for a single decryption operation.
 */
#define SINGLE_CHUNK_SIZE 128

LOG_MODULE_REGISTER(suit_decrypt_filter, CONFIG_SUIT_LOG_LEVEL);

struct decrypt_ctx {
	psa_key_id_t cek_key_id;
	psa_aead_operation_t operation;
	struct stream_sink enc_sink;
	size_t tag_size;
	size_t stored_tag_bytes;
	uint8_t tag[PSA_AEAD_TAG_MAX_SIZE];
	bool in_use;
};

/**
 * Only a single decryption filter can be used at a time. This is a single PSA decryption stream
 * can be used at a time.
 */
struct decrypt_ctx ctx = {0};

static suit_plat_err_t erase(void *ctx)
{
	suit_plat_err_t res = SUIT_PLAT_SUCCESS;

	if (ctx != NULL) {
		struct decrypt_ctx *decrypt_ctx = (struct decrypt_ctx *)ctx;

		decrypt_ctx->stored_tag_bytes = 0;
		memset(decrypt_ctx->tag, 0, sizeof(decrypt_ctx->tag));

		if (decrypt_ctx->enc_sink.erase != NULL) {
			res = decrypt_ctx->enc_sink.erase(decrypt_ctx->enc_sink.ctx);
		}
	} else {
		res = SUIT_PLAT_ERR_INVAL;
	}

	return res;
}

static void store_tag(struct decrypt_ctx *ctx, const uint8_t **buf, size_t *size)
{
	size_t bytes_to_store = MIN(*size, ctx->tag_size - ctx->stored_tag_bytes);

	memcpy(ctx->tag + ctx->stored_tag_bytes, *buf, bytes_to_store);
	ctx->stored_tag_bytes += bytes_to_store;

	*size -= bytes_to_store;
	*buf += bytes_to_store;
}

static suit_plat_err_t write(void *ctx, const uint8_t *buf, size_t size)
{
	psa_status_t status = PSA_SUCCESS;
	suit_plat_err_t err = SUIT_PLAT_SUCCESS;
	uint8_t decrypted_buf[PSA_AEAD_ENCRYPT_OUTPUT_MAX_SIZE(SINGLE_CHUNK_SIZE)] = {0};
	size_t chunk_size = 0;
	size_t decrypted_len = 0;

	if ((ctx != NULL) && (buf != NULL) && (size > 0)) {

		struct decrypt_ctx *decrypt_ctx = (struct decrypt_ctx *)ctx;

		if (!decrypt_ctx->in_use) {
			LOG_ERR("Decrypt filter not initialized.");
			return SUIT_PLAT_ERR_INVAL;
		}

		/* The decrypt_ctx->tag_size starting bytes of the encrypted payload is the tag.
		 * If the whole tag has not yet been stored, store the incoming bytes into the tag memory.
		 */
		if (decrypt_ctx->stored_tag_bytes < decrypt_ctx->tag_size)
		{
			store_tag(decrypt_ctx, &buf, &size);
		}

		while (size > 0) {
			chunk_size = MIN(size, SINGLE_CHUNK_SIZE);

			status = psa_aead_update(&decrypt_ctx->operation, buf, chunk_size, decrypted_buf,
						 sizeof(decrypted_buf), &decrypted_len);

			if (status != PSA_SUCCESS) {
				LOG_ERR("Failed to decrypt data.");
				return SUIT_PLAT_ERR_CRASH;
			}

			err = decrypt_ctx->enc_sink.write(decrypt_ctx->enc_sink.ctx, decrypted_buf, decrypted_len);

			if (err != SUIT_PLAT_SUCCESS) {
				LOG_ERR("Failed to write decrypted data.");
				return err;
			}

			size -= chunk_size;
			buf += chunk_size;
		}

		/* Clear the RAM buffer so that no decrypted data is stored in unwanted places */
		memset(decrypted_buf, 0, sizeof(decrypted_buf));

		return SUIT_PLAT_SUCCESS;
	}

	LOG_ERR("Invalid arguments.");
	return SUIT_PLAT_ERR_INVAL;
}

static suit_plat_err_t release(void *ctx)
{
	suit_plat_err_t res = SUIT_PLAT_SUCCESS;
	psa_status_t status = PSA_SUCCESS;

	uint8_t decrypted_buf[PSA_AEAD_VERIFY_OUTPUT_MAX_SIZE] = {0};
	size_t decrypted_len = 0;

	if (ctx != NULL) {
		struct decrypt_ctx *decrypt_ctx = (struct decrypt_ctx *)ctx;

		if (decrypt_ctx->stored_tag_bytes < decrypt_ctx->tag_size) {
			LOG_ERR("Tag not fully stored.");

			psa_aead_abort(&decrypt_ctx->operation);
			res = SUIT_PLAT_ERR_INCORRECT_STATE;
		}

		if (res == SUIT_PLAT_SUCCESS) {
			status = psa_aead_verify(&decrypt_ctx->operation, decrypted_buf,
						 sizeof(decrypted_buf), &decrypted_len,
						 decrypt_ctx->tag, decrypt_ctx->tag_size);
			if (status != PSA_SUCCESS) {
				LOG_ERR("Failed to verify tag/finish decryption - psa error %d.",
					status);
				/* Revert all the changes so that no decrypted data remains */
				erase(decrypt_ctx);
				psa_aead_abort(&decrypt_ctx->operation);
				res = SUIT_PLAT_ERR_AUTHENTICATION;
			}
			else
			{
				LOG_INF("Firmware decryption successful");

				res = decrypt_ctx->enc_sink.write(decrypt_ctx->enc_sink.ctx, decrypted_buf, decrypted_len);
				if (res != SUIT_PLAT_SUCCESS) {
					LOG_ERR("Failed to write decrypted data.");
				}
			}
		}

		decrypt_ctx->enc_sink.release(decrypt_ctx->enc_sink.ctx);
		psa_destroy_key(decrypt_ctx->cek_key_id);

		memset(decrypted_buf, 0, sizeof(decrypted_buf));

		decrypt_ctx->cek_key_id = 0;
		memset(&decrypt_ctx->operation, 0, sizeof(decrypt_ctx->operation));
		memset(&decrypt_ctx->enc_sink, 0, sizeof(struct stream_sink));
		decrypt_ctx->tag_size = 0;
		decrypt_ctx->stored_tag_bytes = 0;
		memset(decrypt_ctx->tag, 0, sizeof(decrypt_ctx->tag));

		decrypt_ctx->in_use = false;

		return res;
	}

	LOG_ERR("Invalid arguments - decrypt ctx is NULL");
	return SUIT_PLAT_ERR_INVAL;
}

static suit_plat_err_t used_storage(void *ctx, size_t *size)
{
	if ((ctx != NULL) && (size != NULL)) {
		struct decrypt_ctx *decrypt_ctx = (struct decrypt_ctx *)ctx;

		if (decrypt_ctx->enc_sink.used_storage != NULL) {
			return decrypt_ctx->enc_sink.used_storage(decrypt_ctx->enc_sink.ctx, size);
		}

	}

	LOG_ERR("Invalid arguments.");
	return SUIT_PLAT_ERR_INVAL;
}

static suit_plat_err_t unwrap_cek(enum suit_cose_alg kw_alg_id,
				  union suit_key_encryption_data kw_key,
				  psa_key_id_t *cek_key_id)
{
	psa_key_id_t kek_key_id;

	switch (kw_alg_id) {
		case suit_cose_aes256_kw:
			if (suit_plat_decode_key_id(&kw_key.aes.key_id, &kek_key_id)
					!= SUIT_PLAT_SUCCESS) {
				return SUIT_PLAT_ERR_INVAL;
			}

			/* TODO proper key unwrap algorithm from PSA */
			if (suit_aes_key_unwrap_manual(kek_key_id, kw_key.aes.ciphertext.value,
					  256, PSA_KEY_TYPE_AES, PSA_ALG_GCM, cek_key_id)
					!= PSA_SUCCESS) {
				LOG_ERR("Failed to unwrap the CEK");
				return SUIT_PLAT_ERR_AUTHENTICATION;
			}
			break;
		default:
			LOG_ERR("Unsupported key wrap/key derivation algorithm");
			return SUIT_PLAT_ERR_INVAL;
	}

	return SUIT_PLAT_SUCCESS;
}

static suit_plat_err_t get_psa_alg_info(enum suit_cose_alg cose_alg_id,
					psa_algorithm_t *psa_alg_id,
					size_t *tag_size)
{
	switch (cose_alg_id) {
		case suit_cose_aes256_gcm:
			*psa_alg_id = PSA_ALG_GCM;
			*tag_size = PSA_AEAD_TAG_LENGTH(PSA_KEY_TYPE_AES, 256, PSA_ALG_GCM);
			break;
		default:
			LOG_ERR("Unsupported decryption algorithm");
			return SUIT_PLAT_ERR_INVAL;
	}

	return SUIT_PLAT_SUCCESS;
}

suit_plat_err_t suit_decrypt_filter_get(struct stream_sink *dec_sink,
					struct suit_encryption_info *enc_info,
					struct stream_sink *enc_sink)
{
	suit_plat_err_t ret = SUIT_PLAT_SUCCESS;
	if (ctx.in_use) {
		LOG_ERR("The decryption filter is busy");
		return SUIT_PLAT_ERR_BUSY;
	}

	if ((enc_info == NULL) || (enc_sink == NULL) || (dec_sink == NULL)) {
		return SUIT_PLAT_ERR_INVAL;
	}

	psa_status_t status = PSA_SUCCESS;
	psa_algorithm_t psa_decrypt_alg_id = 0;

	if (get_psa_alg_info(enc_info->enc_alg_id, &psa_decrypt_alg_id, &ctx.tag_size)
		!= SUIT_PLAT_SUCCESS) {
		return SUIT_PLAT_ERR_INVAL;
	}

	ctx.in_use = true;

	ret = unwrap_cek(enc_info->kw_alg_id, enc_info->kw_key, &ctx.cek_key_id);

	if (ret != SUIT_PLAT_SUCCESS) {
		ctx.in_use = false;
		return ret;
	}

	ctx.operation = psa_aead_operation_init();

	status = psa_aead_decrypt_setup(&ctx.operation, ctx.cek_key_id, psa_decrypt_alg_id);

	if (status != PSA_SUCCESS) {
		LOG_ERR("Failed to setup decryption operation");
		psa_aead_abort(&ctx.operation);
		ctx.in_use = false;
		return SUIT_PLAT_ERR_CRASH;
	}

	status = psa_aead_set_nonce(&ctx.operation, enc_info->IV.value, enc_info->IV.len);

	if (status != PSA_SUCCESS) {
		LOG_ERR("Failed to set initial vector for decryption operation");
		psa_aead_abort(&ctx.operation);
		ctx.in_use = false;
		return SUIT_PLAT_ERR_CRASH;
	}

	status = psa_aead_update_ad(&ctx.operation, enc_info->aad.value, enc_info->aad.len);

	ctx.stored_tag_bytes = 0;
	memcpy(&ctx.enc_sink, enc_sink, sizeof(struct stream_sink));

	dec_sink->ctx = &ctx;

	dec_sink->write =  write;
	dec_sink->erase = erase;
	dec_sink->release = release;
	dec_sink->used_storage = used_storage;

	/* Seeking is not possible on encrypted payload. */
	dec_sink->seek = NULL;
	/* FLushing is not possible on encrypted payload. */
	dec_sink->flush = NULL;

	return SUIT_PLAT_SUCCESS;
}
