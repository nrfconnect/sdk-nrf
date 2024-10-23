/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/sys/reboot.h>
#include <psa/crypto.h>
#include <sdfw/mram.h>

#include <suit_sdfw_sink.h>
#include <sdfw/sdfw_update.h>
#include <suit_plat_mem_util.h>

#include <suit_digest_sink.h>

#define SUIT_MAX_SDFW_COMPONENTS 1

#define SDFW_SINK_ERR_AGAIN 1 /* Reboot is needed before proceeding. Call the API again. */

LOG_MODULE_REGISTER(suit_sdfw_sink, CONFIG_SUIT_LOG_LEVEL);

typedef int sdf_sink_err_t;

struct sdfw_sink_context {
	bool in_use;
	bool write_called;
};

static struct sdfw_sink_context sdfw_contexts[SUIT_MAX_SDFW_COMPONENTS];

static struct sdfw_sink_context *get_new_context(void)
{
	for (size_t i = 0; i < SUIT_MAX_SDFW_COMPONENTS; ++i) {
		if (!sdfw_contexts[i].in_use) {
			return &sdfw_contexts[i];
		}
	}

	return NULL;
}

static suit_plat_err_t clear_urot_update_status(void)
{
	mram_erase((uintptr_t)&NRF_SICR->UROT.UPDATE,
		   sizeof(NRF_SICR->UROT.UPDATE) / CONFIG_SDFW_MRAM_WORD_SIZE);

	/* Clearing the registers is crucial for correct handling by SecROM. */
	/* Incorrect mram_erase behavior was observed on FPGA. */
	/* Since mram_erase returns void, there is a need for extra check and returning error code
	 * to handle such case.
	 */
	if (NRF_SICR->UROT.UPDATE.STATUS == SICR_UROT_UPDATE_STATUS_CODE_None &&
	    NRF_SICR->UROT.UPDATE.OPERATION == SICR_UROT_UPDATE_OPERATION_OPCODE_Nop) {
		return SUIT_PLAT_SUCCESS;
	} else {
		return SUIT_PLAT_ERR_IO;
	}
}

static suit_plat_err_t schedule_sdfw_update(const uint8_t *buf, size_t size)
{
	int err = 0;

	const struct sdfw_update_blob update_blob = {
		.manifest_addr = (uintptr_t)(buf + CONFIG_SUIT_SDFW_UPDATE_SIGNED_MANIFEST_OFFSET),
		.pubkey_addr = (uintptr_t)(buf + CONFIG_SUIT_SDFW_UPDATE_PUBLIC_KEY_OFFSET),
		.signature_addr = (uintptr_t)(buf + CONFIG_SUIT_SDFW_UPDATE_SIGNATURE_OFFSET),
		.firmware_addr = (uintptr_t)(buf + CONFIG_SUIT_SDFW_UPDATE_FIRMWARE_OFFSET),
		.max_size = CONFIG_SUIT_SDFW_UPDATE_MAX_SIZE,
	};

	LOG_DBG("update_candidate: 0x%08lX", (uintptr_t)buf);
	LOG_DBG("signed_manifest: 0x%08lX", update_blob.manifest_addr);
	LOG_DBG("public_key: 0x%08lX", update_blob.pubkey_addr);
	LOG_DBG("signature: 0x%08lX", update_blob.signature_addr);
	LOG_DBG("firmware: 0x%08lX", update_blob.firmware_addr);
	LOG_DBG("max update size: 0x%08X", update_blob.max_size);
	LOG_DBG("firmware size: 0x%08X (%d)", size, size);

	/* NOTE: SecROM does not use the actual size of new SDFW during update process.
	 *       However, if SDFW is flashed for the first time, the value is used to limit the
	 * future maximum SDFW size. Hence, use the maximum allowed size for safety.
	 */
	err = sdfw_update(&update_blob);

	if (err) {
		LOG_ERR("Failed to schedule SDFW update: %d", err);
		return SUIT_PLAT_ERR_CRASH;
	}

	LOG_INF("SDFW update scheduled");

	return SUIT_PLAT_SUCCESS;
}

static sdf_sink_err_t check_update_candidate(const uint8_t *buf, size_t size)
{
	uint8_t *candidate_digest_in_manifest =
		(uint8_t *)(buf + CONFIG_SUIT_SDFW_UPDATE_DIGEST_OFFSET);
	uint8_t *current_sdfw_digest = (uint8_t *)(NRF_SICR->UROT.SM.TBS.FW.DIGEST);

	sdf_sink_err_t err = SUIT_PLAT_SUCCESS;

	bool digests_match =
		memcmp(candidate_digest_in_manifest, current_sdfw_digest,
		       PSA_HASH_LENGTH(PSA_ALG_SHA_512))
		       == 0;

	if (digests_match) {
		LOG_INF("Same candidate - skip update");
		return SUIT_PLAT_SUCCESS;
	} else {
		LOG_INF("Different candidate");
		err = schedule_sdfw_update(buf, size);
		if (err == SUIT_PLAT_SUCCESS) {
			LOG_DBG("Update scheduled");
			err = SDFW_SINK_ERR_AGAIN;
		}
	}

	return err;
}

static void reboot_to_continue(void)
{
	if (IS_ENABLED(CONFIG_SUIT_UPDATE_REBOOT_ENABLED)) {
		LOG_INF("Reboot the system to continue SDFW update");

		LOG_PANIC();

		sys_reboot(SYS_REBOOT_COLD);
	} else {
		LOG_DBG("Reboot disabled - perform manually");
	}
}

static suit_plat_err_t check_urot_none(const uint8_t *buf, size_t size)
{
	/* Detect update candidate. */
	/* It is enough to check Public Key Size field which occupies first 4B of Signed Manifest.
	 */
	if (*((uint32_t *)buf) == EMPTY_STORAGE_VALUE) {
		LOG_INF("Update candidate not found");
		return SUIT_PLAT_ERR_NOT_FOUND;
	}

	LOG_INF("Update candidate found");

	suit_plat_err_t err = check_update_candidate(buf, size);

	if (err == SDFW_SINK_ERR_AGAIN) {
		/* Update scheduled, continue after reboot */
		reboot_to_continue();
		if (IS_ENABLED(CONFIG_SUIT_UPDATE_REBOOT_ENABLED)) {
			/* If this code is reached, it means that reboot did not work. */
			/* In such case report an error and convert the error code. */
			LOG_ERR("Expected reboot did not happen");
			err = SUIT_PLAT_ERR_UNREACHABLE_PATH;
		}
	}

	return err;
}

static suit_plat_err_t check_urot_activated(const uint8_t *buf, size_t size)
{
	uint8_t *current_sdfw_digest = (uint8_t *)(NRF_SICR->UROT.SM.TBS.FW.DIGEST);
	uint8_t *candidate_digest_in_manifest =
		(uint8_t *)(buf + CONFIG_SUIT_SDFW_UPDATE_DIGEST_OFFSET);

	bool digests_match =
		memcmp(candidate_digest_in_manifest, current_sdfw_digest,
		       PSA_HASH_LENGTH(PSA_ALG_SHA_512))
		       == 0;

	if (digests_match) {
		LOG_DBG("Digest match - update success");
		return SUIT_PLAT_SUCCESS;
	}

	LOG_ERR("Digest mismatch - update failure");
	return SUIT_PLAT_ERR_AUTHENTICATION;

}

/* NOTE: Size means size of the SDFW binary to be updated,
 * excluding Signed Manifest preceding it within update candidate
 */
static suit_plat_err_t write(void *ctx, const uint8_t *buf, size_t size)
{
	if (NULL == ctx || NULL == buf || 0 == size) {
		LOG_ERR("NULL argument");
		return SUIT_PLAT_ERR_INVAL;
	}

	LOG_DBG("buf: %p", (void *)buf);
	LOG_DBG("size: %d", size);

	struct sdfw_sink_context *context = (struct sdfw_sink_context *)ctx;

	if (context->write_called) {
		LOG_ERR("Multiple write calls not allowed");
		return SUIT_PLAT_ERR_INCORRECT_STATE;
	}

	context->write_called = true;

	suit_plat_err_t err = SUIT_PLAT_SUCCESS;
	bool clear_registers = true;

	switch (NRF_SICR->UROT.UPDATE.STATUS) {
	case SICR_UROT_UPDATE_STATUS_CODE_None: {
		err = check_urot_none(buf, size);
		/* Potential start of update process - SecROM needs the registers to be set */
		clear_registers = false;
		break;
	}

	case SICR_UROT_UPDATE_STATUS_CODE_UROTActivated: {
		err = check_urot_activated(buf, size);
		clear_registers = true;
		break;
	}

	case SICR_UROT_UPDATE_STATUS_CODE_VerifyOK:
	case SICR_UROT_UPDATE_STATUS_CODE_RecoveryActivated:
	case SICR_UROT_UPDATE_STATUS_CODE_AROTRecovery: {
		LOG_ERR("Unsupported UROT update status: 0x%08X", NRF_SICR->UROT.UPDATE.STATUS);
		err = SUIT_PLAT_ERR_INCORRECT_STATE;
		clear_registers = true;
		break;
	}

	default: {
		LOG_ERR("SDFW update failure: 0x%08X", NRF_SICR->UROT.UPDATE.STATUS);
		err = NRF_SICR->UROT.UPDATE.STATUS;
		clear_registers = true;
		break;
	}
	}

	if (clear_registers) {
		suit_plat_err_t clear_err = clear_urot_update_status();

		if (clear_err) {
			LOG_ERR("Failed to clear UROT update status");
			/* If the only error was during register clearing - report it. */
			/* Otherwise report the original cause of failure. */
			if (err == SUIT_PLAT_SUCCESS) {
				err = clear_err;
			}
		} else {
			LOG_DBG("UROT update status cleared");
		}
	}

	return err;
}

static suit_plat_err_t release(void *ctx)
{
	if (ctx == NULL) {
		LOG_ERR("Invalid argument");
		return SUIT_PLAT_ERR_INVAL;
	}

	struct sdfw_sink_context *context = (struct sdfw_sink_context *)ctx;

	memset(context, 0, sizeof(struct sdfw_sink_context));

	return SUIT_PLAT_SUCCESS;
}

suit_plat_err_t suit_sdfw_sink_get(struct stream_sink *sink)
{
	if (sink == NULL) {
		LOG_ERR("Invalid arguments");
		return SUIT_PLAT_ERR_INVAL;
	}

	struct sdfw_sink_context *ctx = get_new_context();

	if (ctx == NULL) {
		LOG_ERR("Failed to get a new context");
		return SUIT_PLAT_ERR_NO_RESOURCES;
	}

	memset((void *)ctx, 0, sizeof(struct sdfw_sink_context));
	ctx->in_use = true;
	ctx->write_called = false;

	sink->erase = NULL;
	sink->write = write;
	sink->seek = NULL;
	sink->flush = NULL;
	sink->used_storage = NULL;
	sink->release = release;
	sink->ctx = ctx;

	return SUIT_PLAT_SUCCESS;
}
