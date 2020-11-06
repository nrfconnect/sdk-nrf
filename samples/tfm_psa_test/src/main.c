/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <tfm_ns_interface.h>

extern void psa_api_test(void *arg);

void main(void)
{
	enum tfm_status_e tfm_status;
	tfm_status = tfm_ns_interface_init();
	if (tfm_status != TFM_SUCCESS) {
		printk("tfm_ns_interface_init failed with status %d\n", tfm_status);
	}
	psa_api_test(NULL);
}
