/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>

#include <suit_digest_sink.h>

LOG_MODULE_REGISTER(suit_digest_sink, CONFIG_SUIT_LOG_LEVEL);

struct digest_sink_context {
	bool in_use;
	psa_hash_operation_t operation;
	const uint8_t *expected_digest;
	size_t expected_digest_length;
};

static struct digest_sink_context digest_contexts[CONFIG_SUIT_STREAM_SINK_DIGEST_CONTEXT_COUNT];

static struct digest_sink_context *get_new_context(void)
{
	for (size_t i = 0; i < CONFIG_SUIT_STREAM_SINK_DIGEST_CONTEXT_COUNT; ++i) {
		if (!digest_contexts[i].in_use) {
			return &digest_contexts[i];
		}
	}

	return NULL;
}

static suit_plat_err_t write(void *ctx, const uint8_t *buf, size_t size)
{
	if ((ctx == NULL) || (buf == NULL) || (size == 0)) {
		LOG_ERR("Invalid arguments");
		return SUIT_PLAT_ERR_INVAL;
	}

	LOG_DBG("buf: %p", (void *)buf);
	LOG_DBG("size: %d", size);

	struct digest_sink_context *digest_ctx = (struct digest_sink_context *)ctx;

	if (!digest_ctx->in_use) {
		LOG_ERR("Writing to uninitialized sink");
		return SUIT_PLAT_ERR_INCORRECT_STATE;
	}

	psa_status_t status = psa_hash_update(&digest_ctx->operation, buf, size);

	if (status != PSA_SUCCESS) {
		LOG_ERR("Failed to update digest: %d", status);
		return SUIT_PLAT_ERR_CRASH;
	}

	return SUIT_PLAT_SUCCESS;
}

static suit_plat_err_t release(void *ctx)
{
	if (ctx == NULL) {
		LOG_ERR("Invalid argument");
		return SUIT_PLAT_ERR_INVAL;
	}

	struct digest_sink_context *digest_ctx = (struct digest_sink_context *)ctx;

	memset(digest_ctx, 0, sizeof(struct digest_sink_context));

	return SUIT_PLAT_SUCCESS;
}

suit_plat_err_t suit_digest_sink_get(struct stream_sink *sink, psa_algorithm_t algorithm,
				     const uint8_t *expected_digest)
{
	if (sink == NULL || expected_digest == NULL) {
		LOG_ERR("Invalid argument");
		return SUIT_PLAT_ERR_INVAL;
	}

	struct digest_sink_context *digest_ctx = get_new_context();

	if (digest_ctx == NULL) {
		LOG_ERR("Failed to get a new context");
		return SUIT_PLAT_ERR_NO_RESOURCES;
	}

	memset((void *)digest_ctx, 0, sizeof(struct digest_sink_context));

	psa_status_t status = psa_crypto_init();

	if (status != PSA_SUCCESS) {
		LOG_ERR("Failed to init psa crypto: %d", status);
		return SUIT_PLAT_ERR_CRASH;
	}

	status = psa_hash_setup(&digest_ctx->operation, algorithm);
	if (status != PSA_SUCCESS) {
		LOG_ERR("Failed to setup hash algorithm: %d", status);
		return SUIT_PLAT_ERR_CRASH;
	}

	digest_ctx->in_use = true;
	digest_ctx->expected_digest = expected_digest;
	digest_ctx->expected_digest_length = PSA_HASH_LENGTH(algorithm);

	sink->erase = NULL;
	sink->write = write;
	sink->seek = NULL;
	sink->flush = NULL;
	sink->used_storage = NULL;
	sink->release = release;
	sink->ctx = digest_ctx;

	return SUIT_PLAT_SUCCESS;
}

digest_sink_err_t suit_digest_sink_digest_match(void *ctx)
{
	if (ctx == NULL) {
		LOG_ERR("Invalid argument");
		return SUIT_PLAT_ERR_INVAL;
	}

	struct digest_sink_context *digest_ctx = (struct digest_sink_context *)ctx;

	if (!digest_ctx->in_use) {
		LOG_ERR("Sink not initialized");
		return SUIT_PLAT_ERR_INCORRECT_STATE;
	}

	psa_status_t status = psa_hash_verify(&digest_ctx->operation, digest_ctx->expected_digest,
					      digest_ctx->expected_digest_length);
	digest_sink_err_t err = SUIT_PLAT_SUCCESS;

	if (status == PSA_SUCCESS) {
		/* Digest calculation successful; expected digest matches calculated one */
		err = SUIT_PLAT_SUCCESS;
	} else {
		if (status == PSA_ERROR_INVALID_SIGNATURE) {
			/* Digest calculation successful but expected digest does not match
			 * calculated one
			 */
			err = DIGEST_SINK_ERR_DIGEST_MISMATCH;
		} else {
			LOG_ERR("psa_hash_verify error: %d", status);
			err = SUIT_PLAT_ERR_CRASH;
		}
		/* In both cases psa_hash_verify enters error state and must be aborted */
		status = psa_hash_abort(&digest_ctx->operation);
		if (status != PSA_SUCCESS) {
			LOG_ERR("psa_hash_abort error %d", status);
			err = SUIT_PLAT_ERR_CRASH;
		}
	}

	return err;
}
