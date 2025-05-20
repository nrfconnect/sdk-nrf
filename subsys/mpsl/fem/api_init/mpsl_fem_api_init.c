/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <mpsl_fem_init.h>
#include <zephyr/device.h>

static int mpsl_fem_api_init(void)
{

	mpsl_fem_init();

	return 0;
}

SYS_INIT(mpsl_fem_api_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
