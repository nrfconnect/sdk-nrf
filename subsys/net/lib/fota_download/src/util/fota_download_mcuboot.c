/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <dfu/dfu_target.h>
#include <dfu/dfu_target_stream.h>
#include <dfu/dfu_target_mcuboot.h>

static uint8_t mcuboot_buf[CONFIG_FOTA_DOWNLOAD_MCUBOOT_FLASH_BUF_SZ] __aligned(4);

int fota_download_mcuboot_target_init(void)
{
	return dfu_target_mcuboot_set_buf(mcuboot_buf, sizeof(mcuboot_buf));
}
