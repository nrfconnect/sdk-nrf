/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file pcd_common.h
 *
 * @ingroup pcd
 * @{
 * @brief Common definitions for the PCD API.
 *
 * Common definitions are split out from the main PCD API to allow usage
 * from non-Zephyr code.
 */

#ifndef PCD_COMMON_H__
#define PCD_COMMON_H__

#ifndef CONFIG_SOC_SERIES_NRF53
#error "PCD is only supported on nRF53 series"
#endif

#ifdef CONFIG_PCD_CMD_ADDRESS
/* PCD command block location is static. */
#define PCD_CMD_ADDRESS CONFIG_PCD_CMD_ADDRESS

#else
/* PCD command block location is configured with Partition Manager. */
#include <pm_config.h>

#ifdef PM_PCD_SRAM_ADDRESS
/* PCD command block is in this domain, we are compiling for application core. */
#define PCD_CMD_ADDRESS PM_PCD_SRAM_ADDRESS
#else
/* PCD command block is in a different domain, we are compiling for network core.
 * Extra '_' since its in a different domain.
 */
#define PCD_CMD_ADDRESS PM__PCD_SRAM_ADDRESS
#endif /* PM_PCD_SRAM_ADDRESS */

#endif /* CONFIG_PCD_CMD_ADDRESS */

/** Magic value written to indicate that a copy should take place. */
#define PCD_CMD_MAGIC_COPY 0xb5b4b3b6
/** Magic value written to indicate that debug should be locked. */
#define PCD_CMD_MAGIC_LOCK_DEBUG 0xb6f249ec
/** Magic value written to indicate that a something failed. */
#define PCD_CMD_MAGIC_FAIL 0x25bafc15
/** Magic value written to indicate that a copy is done. */
#define PCD_CMD_MAGIC_DONE 0xf103ce5d
/** Magic value written to indicate that a version number read should take place. */
#define PCD_CMD_MAGIC_READ_VERSION 0xdca345ea

struct pcd_cmd {
	uint32_t magic;         /* Magic value to identify this structure in memory */
	const void *data;       /* Data to copy*/
	size_t len;             /* Number of bytes to copy */
	__INTPTR_TYPE__ offset; /* Offset to store the flash image in */
} __aligned(4);

#define PCD_CMD ((volatile struct pcd_cmd * const)(PCD_CMD_ADDRESS))

static inline void pcd_write_cmd_lock_debug(void)
{
	*PCD_CMD = (struct pcd_cmd){
		.magic = PCD_CMD_MAGIC_LOCK_DEBUG,
	};
}

static inline bool pcd_read_cmd_done(void)
{
	return PCD_CMD->magic == PCD_CMD_MAGIC_DONE;
}

static inline bool pcd_read_cmd_lock_debug(void)
{
	return PCD_CMD->magic == PCD_CMD_MAGIC_LOCK_DEBUG;
}

#endif /* PCD_COMMON_H__ */

/**@} */
