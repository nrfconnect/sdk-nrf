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

extern volatile struct pcd_cmd *pcd_cmd_p;

static inline void pcd_write_cmd_lock_debug(void)
{
	*pcd_cmd_p = (struct pcd_cmd){
		.magic = PCD_CMD_MAGIC_LOCK_DEBUG,
	};
}

static inline bool pcd_read_cmd_done(void)
{
	return pcd_cmd_p->magic == PCD_CMD_MAGIC_DONE;
}

static inline bool pcd_read_cmd_lock_debug(void)
{
	return pcd_cmd_p->magic == PCD_CMD_MAGIC_LOCK_DEBUG;
}

#endif /* PCD_COMMON_H__ */

/**@} */
