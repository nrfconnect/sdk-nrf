/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>

#include "../../hw.h"
#include <stddef.h>
#include <stdint.h>
#include "../../crypmasterregs.h"
#include "../../cmdma.h"
#include <cracen/hardware.h>
#include <cracen/statuscodes.h>
#include <cracen/prng_pool.h>
#include <sxsymcrypt/cmmask.h>

#include <nrf_security_mutexes.h>

/* Enable interrupts showing that an operation finished or aborted.
 * For that, we're interested in :
 *     - Fetcher DMA error (bit: 2)
 *     - Pusher DMA error (bit: 5)
 *     - Pusher DMA stop (bit: 4)
 *
 */
#define CMDMA_INTMASK_EN ((1 << 2) | (1 << 5) | (1 << 4))

NRF_SECURITY_MUTEX_DEFINE(cracen_mutex_symmetric);

static void sx_hw_enable_interrupts(void)
{
	/* Enable CryptoMaster interrupts. */
	sx_wrreg(REG_INT_EN, 0);
	sx_wrreg(REG_INT_STATCLR, ~0);
	sx_wrreg(REG_INT_EN, CMDMA_INTMASK_EN);
	/* Enable interrupts in the wrapper. */
	nrf_cracen_int_enable(NRF_CRACEN, CRACEN_ENABLE_CRYPTOMASTER_Msk);
}

int sx_hw_reserve(struct sx_dmactl *dma, sx_hw_reserve_flags_t flags)
{
	int err;

	cracen_acquire();
	nrf_security_mutex_lock(cracen_mutex_symmetric);

	if (dma) {
		dma->hw_acquired = true;
	}
	if (IS_ENABLED(CONFIG_CRACEN_USE_INTERRUPTS)) {
		sx_hw_enable_interrupts();
	}

	if (flags & SX_HW_RESERVE_CM_ENABLED) {
		uint32_t prng_value;

		err = cracen_prng_value_from_pool(&prng_value);
		if (err == SX_OK) {
			err = sx_cm_load_mask(prng_value);
		}
		if (err != SX_OK) {
			sx_hw_release(dma);
			return err;
		}
	}

	return SX_OK;
}

void sx_hw_release(struct sx_dmactl *dma)
{
	if (dma == NULL || dma->hw_acquired) {
		cracen_release();
		nrf_security_mutex_unlock(cracen_mutex_symmetric);
		if (dma) {
			dma->hw_acquired = false;
		}
	}
}
