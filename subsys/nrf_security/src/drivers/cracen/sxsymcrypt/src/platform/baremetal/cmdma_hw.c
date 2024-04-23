/*
 * Copyright (c) 2019-2020 Silex Insight
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "../../hw.h"
#include <stddef.h>
#include <stdint.h>
#include "../../crypmasterregs.h"
#include "../../cmdma.h"
#include <security/cracen.h>
#include <cracen/statuscodes.h>
#include <zephyr/kernel.h>

/* Enable interrupts showing that an operation finished or aborted.
 * For that, we're interested in :
 *     - Fetcher DMA error (bit: 2)
 *     - Pusher DMA error (bit: 5)
 *     - Pusher DMA stop (bit: 4)
 *
 */
#define CMDMA_INTMASK_EN ((1 << 2) | (1 << 5) | (1 << 4))

K_MUTEX_DEFINE(cracen_mutex_symmetric);

void sx_hw_reserve(struct sx_dmactl *dma)
{
	cracen_acquire();
	k_mutex_lock(&cracen_mutex_symmetric, K_FOREVER);
	dma->hw_acquired = true;

	/* Enable CryptoMaster interrupts. */
	sx_wrreg(REG_INT_EN, 0);
	sx_wrreg(REG_INT_STATCLR, ~0);
	sx_wrreg(REG_INT_EN, CMDMA_INTMASK_EN);

	/* Enable interrupts in the wrapper. */
	nrf_cracen_int_enable(NRF_CRACEN, CRACEN_ENABLE_CRYPTOMASTER_Msk);
}

void sx_cmdma_release_hw(struct sx_dmactl *dma)
{
	if (dma->hw_acquired) {
		cracen_release();
		k_mutex_unlock(&cracen_mutex_symmetric);
		dma->hw_acquired = false;
	}
}
