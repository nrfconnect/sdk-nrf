/*
 * Copyright (c) 2025 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/sys/printk.h>
#include <ironside/se/uicr.h>

int main(void)
{
	uint32_t first_uicr = IRONSIDE_SE_UICR->CUSTOMER[0];
	printk("First UICR value is: %0x\n",first_uicr);
}
