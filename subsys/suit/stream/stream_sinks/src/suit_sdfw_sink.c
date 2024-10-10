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

#define SUIT_MAX_SDFW_COMPONENTS 1

LOG_MODULE_REGISTER(suit_sdfw_sink, CONFIG_SUIT_LOG_LEVEL);

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

static bool is_update_needed(const uint8_t *buf, size_t size)
{
	const uint8_t *candidate_digest_in_manifest =
		(uint8_t *)(buf + CONFIG_SUIT_SDFW_UPDATE_DIGEST_OFFSET);
	const uint8_t *current_sdfw_digest = (uint8_t *)(NRF_SICR->UROT.SM.TBS.FW.DIGEST);

	bool digests_match = memcmp(candidate_digest_in_manifest, current_sdfw_digest,
				    PSA_HASH_LENGTH(PSA_ALG_SHA_512)) == 0;

	/* Update is needed when candidate's digest doesn't match current FW digest */
	return !digests_match;
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

static suit_plat_err_t schedule_update(const uint8_t *buf, size_t size)
{

	suit_plat_err_t clear_err = clear_urot_update_status();
	if (clear_err) {
		LOG_ERR("Failed to clear UROT update status");
		/* There is not much we can do about it - just continue */
	} else {
		/* TODO: Change to dbg */
		LOG_INF("UROT update status cleared");
	}

	int err = 0;

	// LOG_WRN("adsz: SIMULATE SDFW UPDATE ERROR!");
	const struct sdfw_update_blob update_blob = {
		.manifest_addr = (uintptr_t)(buf + CONFIG_SUIT_SDFW_UPDATE_SIGNED_MANIFEST_OFFSET),
		.pubkey_addr = (uintptr_t)(buf + CONFIG_SUIT_SDFW_UPDATE_PUBLIC_KEY_OFFSET),
		.signature_addr = (uintptr_t)(buf + CONFIG_SUIT_SDFW_UPDATE_SIGNATURE_OFFSET),
		.firmware_addr = (uintptr_t)(buf + CONFIG_SUIT_SDFW_UPDATE_FIRMWARE_OFFSET),
		// .firmware_addr = (uintptr_t)(buf + CONFIG_SUIT_SDFW_UPDATE_FIRMWARE_OFFSET + 512),
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
		err = SUIT_PLAT_ERR_CRASH;
	} else {
		err = SUIT_PLAT_SUCCESS;
	}

	return err;
}

static void reboot_to_continue(void)
{
	if (IS_ENABLED(CONFIG_SUIT_UPDATE_REBOOT_ENABLED)) {
		LOG_INF("Reboot the system to continue update");

		LOG_PANIC();

		sys_reboot(SYS_REBOOT_COLD);
	} else {
		LOG_DBG("Reboot disabled - perform manually");
	}
}

static suit_plat_err_t schedule_update_and_reboot(const uint8_t *buf, size_t size)
{
	suit_plat_err_t err = schedule_update(buf, size);

	if (err == SUIT_PLAT_SUCCESS) {
		reboot_to_continue();
		if (IS_ENABLED(CONFIG_SUIT_UPDATE_REBOOT_ENABLED)) {
			/* If this code is reached, it means that reboot did not work. */
			/* In such case report an error. */
			LOG_ERR("Expected reboot did not happen");
			err = SUIT_PLAT_ERR_UNREACHABLE_PATH;
		}
	}

	return err;
}

static suit_plat_err_t update_already_ongoing(const uint8_t *buf, size_t size)
{
	suit_plat_err_t err = SUIT_PLAT_SUCCESS;

	enum sdfw_update_status update_status = sdfw_update_initial_status_get();

	/* Candidate is different than current FW but SDFW update is already ongoing. */
	switch (update_status) {
	case SDFW_UPDATE_STATUS_NONE: {
		/* No pending operation even though operation indicates SDFW update.
		 * Yet candidate differs from current FW, so schedule the update.
		 * TODO: Check if this situation is possible or under what circumstances
		 */
		err = schedule_update_and_reboot(buf, size);
		break;
	}
	default: {
		/* SecROM indicates error during update */
		LOG_ERR("Update failure: %08x", update_status);
		err = SUIT_PLAT_ERR_CRASH;
		break;
	}
	}

	return err;
}

static suit_plat_err_t update_needed(const uint8_t *buf, size_t size)
{
	suit_plat_err_t err = SUIT_PLAT_SUCCESS;

	enum sdfw_update_operation initial_operation = sdfw_update_initial_operation_get();

	switch (initial_operation) {
	case SDFW_UPDATE_OPERATION_NOP: {
		/* No update ongoing - start SDFW update */
		err = schedule_update_and_reboot(buf, size);
		break;
	}
	case SDFW_UPDATE_OPERATION_UROT_ACTIVATE: {
		/* SDFW update already ongoing */
		err = update_already_ongoing(buf, size);
		break;
	}
	case SDFW_UPDATE_OPERATION_RECOVERY_ACTIVATE: {
		/* SDFW Recovery update was ongoing */

		LOG_INF("Proceed with SDFW update");
		err = schedule_update_and_reboot(buf, size);
		break;
	}
	default: {
		LOG_ERR("Unhandled operation: %08x", initial_operation);
		err = SUIT_PLAT_ERR_CRASH;
		break;
	}
	}

	return err;
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

	if (is_update_needed(buf, size)) {
		LOG_INF("Update needed");
		err = update_needed(buf, size);
	} else {
		LOG_INF("Update not needed");
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
