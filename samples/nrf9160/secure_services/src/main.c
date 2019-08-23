/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <misc/printk.h>
#include <misc/reboot.h>
#include <secure_services.h>
#include <kernel.h>
#include <pm_config.h>

void print_number(u8_t *num, size_t len)
{
	printk("Random number len %d: 0x", len);
	for (int i = 0; i < len; i++) {
		printk("%02x", num[i]);
	}
	printk("\n");
}

void main(void)
{
	const int sleep_time_ms = 5000;
	const int random_number_count = 16;
	const int random_number_len = 144;
	int ret;

	printk("Secure Services example.\n");
	printk("Generate %d strings of %d random bytes, sleep, then reboot.\n\n",
		random_number_count, random_number_len);

	for (int i = 0; i < random_number_count; i++) {
		u8_t random_number[random_number_len];
		size_t olen = random_number_len;

		ret = spm_request_random_number(random_number, random_number_len, &olen);
		if (ret != 0) {
			printk("Could not get random number (err: %d)\n", ret);
			continue;
		}
		print_number(random_number, olen);
	}

#ifdef CONFIG_BOOTLOADER_MCUBOOT
	const int num_bytes_to_read = PM_MCUBOOT_PAD_SIZE;
	const int read_address = PM_MCUBOOT_PAD_ADDRESS;
	u8_t buf[num_bytes_to_read];

	printk("\nRead %d bytes from address %d\n", num_bytes_to_read,
			read_address);
	ret = spm_request_read(buf, read_address, num_bytes_to_read);
	if (ret != 0) {
		printk("Could not read data (err: %d)\n", ret);
	}
	for (int i = 0; i < num_bytes_to_read; i++) {
		printk("%d", buf[i]);
	}
#endif

	printk("\nReboot in %d ms.\n", sleep_time_ms);
	k_sleep(sleep_time_ms);

	sys_reboot(0); /* Argument is ignored. */
}
