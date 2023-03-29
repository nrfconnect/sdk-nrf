/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdbool.h>
#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <dfu/dfu_target_smp.h>

int fota_download_smp_update_apply(void)
{
	return dfu_target_smp_reboot();
}
