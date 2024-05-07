/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <sdfw/sdfw_services/extmem_remote.h>

int main(void)
{
	extmem_remote_init();
	return 0;
}
