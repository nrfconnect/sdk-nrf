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
	int status;
	struct sxchannel channel;

	/* This is a special case where "cfg" is used to transmit the random
	 * value instead of a command word.
	 */
	sx_cmdma_newcmd(&channel.dma, channel.descs, csprng_value,
			DMATAG_BA411 | DMATAG_CONFIG(0x68));

	ADD_OUTDESCA(channel.dma, NULL, 0, 0xf);

	sx_cmdma_start(&channel.dma, sizeof(channel.descs), channel.descs);

	status = SX_ERR_HW_PROCESSING;
	while (status == SX_ERR_HW_PROCESSING) {
		status = sx_cmdma_check();
	}

	safe_memzero(&channel, sizeof(channel));
	return status;
}
