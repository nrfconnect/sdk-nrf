/*
 * Copyright (c) 2025 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/sys/printk.h>
#include <ironside/se/uicr.h>

#define CUSTOMER_LEN (sizeof(IRONSIDE_SE_UICR->CUSTOMER) / \
		sizeof(IRONSIDE_SE_UICR->CUSTOMER[0]))

int main(void)
{
	printk("Printing all of CUSTOMER->UICR:\n");
	for(int i = 0; i < CUSTOMER_LEN; i++) {
		if(i%4 == 0) {
			printk("\n");
		}
		printk("%08x ",IRONSIDE_SE_UICR->CUSTOMER[i]);
	}

 }
