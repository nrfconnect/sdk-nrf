/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <cracen/statuscodes.h>
#include "../include/sxsymcrypt/internal.h"
#include "cracen/interrupts.h"
#include "crypmasterregs.h"
#include "hw.h"
#include "cmdma.h"
#include "security/cracen.h"

void sx_cmdma_newcmd(struct sx_dmactl *dma, struct sxdesc *d, uint32_t cmd, uint32_t tag)
{
	dma->d = d;
	dma->dmamem.cfg = cmd;

	dma->mapped = (char *)&dma->dmamem;
	ADD_INDESC_PRIV(*dma, offsetof(struct sx_dmaslot, cfg), sizeof(dma->dmamem.cfg), tag);
}

void sx_cmdma_start(struct sx_dmactl *dma, size_t privsz, struct sxdesc *indescs)
{
	struct sxdesc *m;

#ifdef CONFIG_DCACHE
	m = (struct sxdesc *)(dma->mapped + sizeof(struct sx_dmaslot));
	for (; m != DMA_LAST_DESCRIPTOR; m = m->next) {
		sys_cache_data_flush_and_invd_range(m->addr, m->sz & DMA_SZ_MASK);
	}
	m = (struct sxdesc *)(dma->mapped + offsetof(struct sx_dmaslot, outdescs));
	for (; m != DMA_LAST_DESCRIPTOR; m = m->next) {
		sys_cache_data_flush_and_invd_range(m->addr, m->sz & DMA_SZ_MASK);
	}

	sys_cache_data_flush_range((void *)&dma->dmamem, sizeof(dma->dmamem) + privsz);
#endif

	m = (struct sxdesc *)(dma->mapped + sizeof(struct sx_dmaslot));
	sx_wrreg_addr(REG_FETCH_ADDR, m);
	m = (struct sxdesc *)(dma->mapped + offsetof(struct sx_dmaslot, outdescs));
	sx_wrreg_addr(REG_PUSH_ADDR, m);
	sx_wrreg(REG_CONFIG, REG_CONFIG_SG);
	sx_wrreg(REG_START, REG_START_ALL);
}

void sx_cmdma_finalize_descs(struct sxdesc *start, struct sxdesc *end)
{
	struct sxdesc *d;

	for (d = start; d < end; d++) {
		d->next = d + 1;
	}
	end->next = DMA_LAST_DESCRIPTOR;
	end->dmatag |= DMATAG_LAST;
	end->sz |= DMA_REALIGN;
}

int sx_cmdma_check(void)
{
	uint32_t r = 0xFF;
	uint32_t busy;

	r = cracen_wait_for_cm_interrupt();
	if (!r) {
		r = sx_rdreg(REG_INT_STATRAW);
	}

	busy = sx_rdreg(REG_STATUS) & REG_STATUS_BUSY_MASK;

	if (r & (DMA_BUS_FETCHER_ERROR_MASK | DMA_BUS_PUSHER_ERROR_MASK)) {
		sx_cmdma_reset();
		return SX_ERR_DMA_FAILED;
	}
	if (busy) {
		return SX_ERR_HW_PROCESSING;
	}

	sx_wrreg(REG_INT_STATCLR, ~0);

	return SX_OK;
}

void sx_cmdma_reset(void)
{
	uint32_t intrmask = sx_rdreg(REG_INT_EN);

	sx_wrreg(REG_CONFIG, REG_SOFT_RESET_ENABLE);
	sx_wrreg(REG_CONFIG, 0); /* clear SW reset */

	/* Wait for soft-reset to end */
	while (sx_rdreg(REG_STATUS) & REG_SOFT_RESET_BUSY) {
		;
	}

	if (intrmask) {
		sx_wrreg(REG_INT_EN, intrmask);
	}
}
