/** MAC common defines and structures.
 *
 * @file
 * @copyright Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MACDEFS_HEADER_FILE
#define MACDEFS_HEADER_FILE

#include <stddef.h>

struct sx_mac_cmdma_tags {
	uint32_t cfg;
	uint32_t state;
	uint32_t key;
	uint32_t data;
};

struct sx_mac_cmdma_cfg {
	int cmdma_mask;
	int granularity;
	uint32_t blocksz;
	uint32_t savestate;
	uint32_t loadstate;
	uint32_t statesz;
	const struct sx_mac_cmdma_tags *dmatags;
};

#define OFFSET_EXTRAMEM(c) (sizeof((c)->dma.dmamem) + sizeof((c)->allindescs))

/* Can be used with BA411 or BA419 Config register -> ModeOfOperation [16:8] */
#define CMDMA_CMAC_MODE_SET(modeid) (1 << (8 + (modeid)))

#endif
