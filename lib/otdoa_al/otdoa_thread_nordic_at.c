/*
 * Copyright (c) 2025 PHY Wireless, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-PHYW
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <stdlib.h>
#include <stdio.h>

#include <zephyr/kernel.h>
#include <zephyr/posix/unistd.h>
#include <zephyr/net/socket.h>
#include <modem/at_monitor.h>

/*
 * buffer slab for PRS SAMPLES
 * NB: slab must include 1920 complex samples plus SFN, slot and NumSamples.
 * Each buffer is
 * LTE_ACTIVE_SYMBOLS_PER_SUBFRAME_1PBCH*LTE_NUM_CARRIERS_PER_RB*8*sizeof(tRSTD_COMPLEX);
 */
#define PRS_SAMPLES_BUFFER_SIZE (8 * 12 * 8 * sizeof(uint16_t) * 2)

K_MEM_SLAB_DEFINE(nordic_prs_samples_slab, PRS_SAMPLES_BUFFER_SIZE,
		  CONFIG_OTDOA_PRS_SAMPLES_BUFFER_COUNT, 4);

/*
 * Alloc / Free of PRS sample buffers
 */
void *otdoa_alloc_samples(size_t uLen)
{
	if (uLen > PRS_SAMPLES_BUFFER_SIZE) {
		return NULL;
	}
	void *pBuffer = NULL;

	int iRet = k_mem_slab_alloc(&nordic_prs_samples_slab, &pBuffer, K_NO_WAIT);

	if (iRet != 0) {
		return NULL;
	}
	return pBuffer;
}

int otdoa_free_samples(void *pBuffer)
{
	/* check if the buffer to be freed is in the slab */
	if (((char *)pBuffer < _k_mem_slab_buf_nordic_prs_samples_slab) ||
		((char *)pBuffer >= (_k_mem_slab_buf_nordic_prs_samples_slab +
				 sizeof(_k_mem_slab_buf_nordic_prs_samples_slab)))) {
		printk("Attempting to free sample buffer %p that is not in the slab\n", pBuffer);
		return -1;
	}
	if (pBuffer) {
		k_mem_slab_free(&nordic_prs_samples_slab, pBuffer);
	}
	return 0;
}

void otdoa_sleep_msec(int msec)
{
	k_sleep(K_MSEC(msec));
}
