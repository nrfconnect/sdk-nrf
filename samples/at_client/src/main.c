/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <stdio.h>
#include <uart.h>
#include <string.h>
#include <at_host.h>

void main(void)
{
	int err;
    at_host_config_t l_config;

    memset(&l_config, 0, sizeof(at_host_config_t));
    l_config.uart = CONFIG_AT_HOST_UART;
    l_config.mode = CONFIG_AT_HOST_TERMINATION;

	printf("The AT host sample started\n");

	err = at_host_init(&l_config);

	if (err != 0) {
		printf("ERROR: AT Host not initialized \r\n");
		return;
	}

	while (1) {
		at_host_process();
	}
}
