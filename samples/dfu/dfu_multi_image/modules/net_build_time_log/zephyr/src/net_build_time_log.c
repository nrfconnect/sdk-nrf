/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <zephyr/init.h>

static int net_build_time_log(void)
{
	printf("Network core build time: %s %s\n", __DATE__, __TIME__);

	return 0;
}

SYS_INIT(net_build_time_log, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
