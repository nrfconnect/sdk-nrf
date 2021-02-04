/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <sys/printk.h>
#include <pm_config.h>
#include <string.h>
#include <fprotect.h>
#include <init.h>

#include <hal/nrf_acl.h>
#include <hal/nrf_ficr.h>

/** The nrf52840 doesn't have a KMU but the helper functions
 * handling the KDR key are inside the following header
 */
#include <nrf_cc3xx_platform_kmu.h>

/* The magic data identify the flash page which contains the key */
static uint8_t huk_magic[16] = {
	0x56, 0xB5, 0x6D, 0xAD, 0xAB, 0x5B, 0x6A, 0xB5,
	0x55, 0xAB, 0x5A, 0xDA, 0xAB, 0x55, 0xAD, 0x5A
};

static uint32_t huk_addr = PM_HW_UNIQUE_KEY_PARTITION_ADDRESS;

/* Load the Hardware Unique Key stored in flash */
int hw_unique_key_load(const struct device *unused)
{
	ARG_UNUSED(unused);

	size_t err = -1;

	/* Compare the huk_magic data with the content of the page */
	err = memcmp((uint8_t *)huk_addr, huk_magic, sizeof(huk_magic));
	if (err != 0) {
		printk("Could not load the HUK, magic data are not present\n\r");
		NVIC_SystemReset();
	}

	err = nrf_cc3xx_platform_kdr_load_key((uint8_t *)huk_addr +
					      sizeof(huk_magic));
	if (err != 0) {
		printk("The HUK loading failed "
		       "with error code: %d\n\r",
		       err);
		NVIC_SystemReset();
	}

	/* Lock the flash page which holds the key */
	err = fprotect_area_no_access(huk_addr, CONFIG_FPROTECT_BLOCK_SIZE);
	if (err != 0) {
		printk("Fprotect failed with error code: %d\n\r", err);
		NVIC_SystemReset();
	}

	printk("The Hardware Unique Key loaded successfully!\n\r");
	return 0;
}
