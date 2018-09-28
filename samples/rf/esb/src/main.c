/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <misc/printk.h>
#include <esb.h>

void main(void)
{

	esb_init();
	printk("ESB Initialized\n");

}

