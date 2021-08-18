/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef TFM_READ_RANGES_H__
#define TFM_READ_RANGES_H__

#include <tfm_ioctl_api.h>
#include <pm_config.h>

#define FICR_BASE               NRF_FICR_S_BASE
#define FICR_PUBLIC_ADDR        (FICR_BASE + 0x204)
#define FICR_PUBLIC_SIZE        0xA1C
#define FICR_RESTRICTED_ADDR    (FICR_BASE + 0x130)
#define FICR_RESTRICTED_SIZE    0x8

static const struct tfm_read_service_range ranges[] = {
#ifdef PM_MCUBOOT_ADDRESS
	/* Allow reads of mcuboot metadata */
	{.start = PM_MCUBOOT_PAD_ADDRESS,
		.size = PM_MCUBOOT_PAD_SIZE},
#ifdef PM_MCUBOOT_PAD_1_ADDRESS
	{.start = PM_MCUBOOT_PAD_1_ADDRESS,
		.size = PM_MCUBOOT_PAD_1_SIZE},
#endif
#endif
	{.start = FICR_PUBLIC_ADDR,
		.size = FICR_PUBLIC_SIZE},
	{.start = FICR_RESTRICTED_ADDR,
		.size = FICR_RESTRICTED_SIZE},
};

#endif /* TFM_READ_RANGES_H__ */
