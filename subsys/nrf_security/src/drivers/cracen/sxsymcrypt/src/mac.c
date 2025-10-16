/*
 *  Copyright (c) 2023 Nordic Semiconductor ASA
 *
 *  SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "../include/sxsymcrypt/mac.h"
#include "../include/sxsymcrypt/keyref.h"
#include "../include/sxsymcrypt/cmmask.h"
#include <cracen/statuscodes.h>
#include "keyrefdefs.h"
#include "macdefs.h"
#include "crypmasterregs.h"
#include "hw.h"
#include "cmdma.h"
#include "cmaes.h"
#include <cracen/prng_pool.h>

int sx_mac_free(struct sxmac *mac_ctx)
{
	int sx_err = SX_OK;
	if (mac_ctx->key->clean_key) {
		sx_err = mac_ctx->key->clean_key(mac_ctx->key->user_data);
	}
	sx_cmdma_release_hw(&mac_ctx->dma);
	return sx_err;
}

int sx_mac_hw_reserve(struct sxmac *mac_ctx)
{
	int err = SX_OK;

	uint32_t prng_value;

	err = cracen_prng_value_from_pool(&prng_value);
	if (err != SX_OK) {
		return err;
	}

	sx_hw_reserve(&mac_ctx->dma);

	err = sx_cm_load_mask(prng_value);
	if (err != SX_OK) {
		goto exit;
	}

	if (mac_ctx->key->prepare_key) {
		err = mac_ctx->key->prepare_key(mac_ctx->key->user_data);
	}

exit:
	if (err != SX_OK) {
		return sx_handle_nested_error(sx_mac_free(mac_ctx), err);
	}

	return SX_OK;
}

int sx_mac_feed(struct sxmac *mac_ctx, const uint8_t *datain, size_t sz)
{
	if (!mac_ctx->dma.hw_acquired) {
		return SX_ERR_UNINITIALIZED_OBJ;
	}
	if (sz >= DMA_MAX_SZ) {
		return sx_handle_nested_error(sx_mac_free(mac_ctx), SX_ERR_TOO_BIG);
	}
	if (mac_ctx->cntindescs >= (ARRAY_SIZE(mac_ctx->descs))) {
		return sx_handle_nested_error(sx_mac_free(mac_ctx), SX_ERR_FEED_COUNT_EXCEEDED);
	}

	if (sz != 0) {
		ADD_RAW_INDESC(mac_ctx->dma, datain, sz, mac_ctx->cfg->dmatags->data);
		mac_ctx->cntindescs++;
		mac_ctx->feedsz += sz;
	}

	return SX_OK;
}

static int sx_mac_run(struct sxmac *mac_ctx)
{
	if ((mac_ctx->feedsz == 0) && (mac_ctx->dma.dmamem.cfg & mac_ctx->cfg->loadstate)) {
		return sx_handle_nested_error(sx_mac_free(mac_ctx), SX_ERR_INPUT_BUFFER_TOO_SMALL);
	}
	sx_cmdma_start(&mac_ctx->dma, sizeof(mac_ctx->descs), mac_ctx->descs);

	return SX_OK;
}

int sx_mac_generate(struct sxmac *mac_ctx, uint8_t *mac)
{
	if (!mac_ctx->dma.hw_acquired) {
		return SX_ERR_UNINITIALIZED_OBJ;
	}

	if (mac_ctx->feedsz == 0) {
		ADD_EMPTY_INDESC(mac_ctx->dma, (mac_ctx->cfg->cmdma_mask + 1),
				 mac_ctx->cfg->dmatags->data);
	}
	SET_LAST_DESC_IGN(mac_ctx->dma, mac_ctx->feedsz, mac_ctx->cfg->cmdma_mask);

	ADD_OUTDESCA(mac_ctx->dma, mac, mac_ctx->macsz, mac_ctx->cfg->cmdma_mask);

	mac_ctx->dma.dmamem.cfg &= ~mac_ctx->cfg->savestate;

	return sx_mac_run(mac_ctx);
}

int sx_mac_resume_state(struct sxmac *mac_ctx)
{
	int err;

	if (mac_ctx->dma.hw_acquired) {
		return SX_ERR_UNINITIALIZED_OBJ;
	}

	if (!mac_ctx->cfg->statesz) {
		return SX_ERR_CONTEXT_SAVING_NOT_SUPPORTED;
	}

	/* Note that the sx_mac APIs are used only with CMAC at the moment so we always need to
	 * enable the AES countermeasures.
	 */
	err = sx_mac_hw_reserve(mac_ctx);
	if (err != SX_OK) {
		return err;
	}

	sx_cmdma_newcmd(&mac_ctx->dma, mac_ctx->descs, mac_ctx->dma.dmamem.cfg,
			mac_ctx->cfg->dmatags->cfg);
	mac_ctx->cntindescs = 1;
	if (KEYREF_IS_USR(mac_ctx->key)) {
		ADD_CFGDESC(mac_ctx->dma, mac_ctx->key->key, mac_ctx->key->sz,
			    mac_ctx->cfg->dmatags->key);
		mac_ctx->cntindescs++;
	}
	ADD_INDESC_PRIV(mac_ctx->dma, OFFSET_EXTRAMEM(mac_ctx), mac_ctx->cfg->statesz,
			mac_ctx->cfg->dmatags->state);
	mac_ctx->cntindescs++;
	mac_ctx->dma.dmamem.cfg |= mac_ctx->cfg->loadstate;
	mac_ctx->feedsz = 0;

	return SX_OK;
}

int sx_mac_save_state(struct sxmac *mac_ctx)
{
	uint32_t sz;

	if (!mac_ctx->dma.hw_acquired) {
		return SX_ERR_UNINITIALIZED_OBJ;
	}
	sz = mac_ctx->feedsz;

	if (sz < mac_ctx->cfg->blocksz) {
		return sx_handle_nested_error(sx_mac_free(mac_ctx), SX_ERR_INPUT_BUFFER_TOO_SMALL);
	}
	if (sz % mac_ctx->cfg->granularity) {
		return sx_handle_nested_error(sx_mac_free(mac_ctx), SX_ERR_WRONG_SIZE_GRANULARITY);
	}

	mac_ctx->dma.dmamem.cfg |= mac_ctx->cfg->savestate;

	ADD_OUTDESC_PRIV(mac_ctx->dma, OFFSET_EXTRAMEM(mac_ctx), mac_ctx->cfg->statesz, 0x0F);

	return sx_mac_run(mac_ctx);
}

int sx_mac_status(struct sxmac *mac_ctx)
{
	int status;

	if (!mac_ctx->dma.hw_acquired) {
		return SX_ERR_UNINITIALIZED_OBJ;
	}
	status = sx_cmdma_check();
	if (status == SX_ERR_HW_PROCESSING) {
		return status;
	}

#if CONFIG_DCACHE
	sys_cache_data_invd_range((void *)&mac_ctx->extramem, sizeof(mac_ctx->extramem));
#endif

	return sx_handle_nested_error(sx_mac_free(mac_ctx), status);
}

int sx_mac_wait(struct sxmac *mac_ctx)
{
	int status = SX_ERR_HW_PROCESSING;

	if (!mac_ctx->dma.hw_acquired) {
		return SX_ERR_UNINITIALIZED_OBJ;
	}

	while (status == SX_ERR_HW_PROCESSING) {
		status = sx_mac_status(mac_ctx);
	}

	return status;
}
