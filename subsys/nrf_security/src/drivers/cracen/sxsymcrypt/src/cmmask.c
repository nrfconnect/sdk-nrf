/*
 * @copyright Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "../include/sxsymcrypt/cmmask.h"
#include <cracen/statuscodes.h>
#include "crypmasterregs.h"
#include "hw.h"
#include "cmdma.h"
#include <cracen/mem_helpers.h>

int sx_cm_load_mask(uint32_t csprng_value)
{
	int r;
	struct sxdesc *out;
	struct sxcmmask cmmask;

	/* This is a special case where "cfg" is used to transmit the random
	 * value instead of a command word.
	 */
	sx_cmdma_newcmd(&cmmask.dma, cmmask.allindescs, csprng_value,
			DMATAG_BA411 | DMATAG_CONFIG(0x68));
	sx_cmdma_finalize_descs(cmmask.allindescs, cmmask.dma.d - 1);

	out = cmmask.dma.dmamem.outdescs;
	ADD_OUTDESC(out, NULL, 0);
	sx_cmdma_finalize_descs(cmmask.dma.dmamem.outdescs, out - 1);

	sx_cmdma_start(&cmmask.dma, sizeof(cmmask.allindescs), cmmask.allindescs);

	r = SX_ERR_HW_PROCESSING;
	while (r == SX_ERR_HW_PROCESSING) {
		r = sx_cmdma_check();
	}

	safe_memzero(&cmmask, sizeof(cmmask));
	return r;
}
