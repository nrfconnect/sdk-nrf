/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <sys/printk.h>
#include <power/reboot.h>
#include <secure_services.h>
#include <kernel.h>
#include <pm_config.h>
#include <fw_info.h>

void print_hex_number(u8_t *num, size_t len)
{
	printk("0x");
	for (int i = 0; i < len; i++) {
		printk("%02x", num[i]);
	}
	printk("\n");
}

void print_random_number(u8_t *num, size_t len)
{
	printk("Random number len %d: ", len);
	print_hex_number(num, len);
}

void main(void)
{
	struct fw_info info_app;
	const int sleep_time_s = 5;
	const int random_number_count = 16;
	const int random_number_len = 144;
	int ret;

	printk("Secure Services example.\n");
	printk("Generate %d strings of %d random bytes, read MCUboot header "
		"or validate S0 if present, sleep, then reboot.\n\n",
		random_number_count, random_number_len);

	for (int i = 0; i < random_number_count; i++) {
		u8_t random_number[random_number_len];
		size_t olen = random_number_len;

		ret = spm_request_random_number(random_number,
						random_number_len, &olen);
		if (ret != 0) {
			printk("Could not get random number (err: %d)\n", ret);
			continue;
		}
		print_random_number(random_number, olen);
	}

	ret = spm_firmware_info(PM_APP_ADDRESS, &info_app);
	if (ret != 0) {
		printk("Could find firmware info (err: %d)\n", ret);
	}

	printk("App FW version: %d\n", info_app.version);

#ifdef CONFIG_BOOTLOADER_MCUBOOT
	const int num_bytes_to_read = PM_MCUBOOT_PAD_SIZE;
	const int read_address = PM_MCUBOOT_PAD_ADDRESS;
	u8_t buf[num_bytes_to_read];

	printk("\nRead %d bytes from address 0x%x (MCUboot header for current "
		"image):\n", num_bytes_to_read, read_address);
	ret = spm_request_read(buf, read_address, num_bytes_to_read);
	if (ret != 0) {
		printk("Could not read data (err: %d)\n", ret);
	}

	print_hex_number(buf, num_bytes_to_read);
#endif

#if defined(PM_S0_ADDRESS)
	int valid = spm_prevalidate_b1_upgrade(PM_S0_ADDRESS, PM_S0_ADDRESS);

	if (valid < 0 && valid != -ENOTSUP) {
		printk("Unexpected error from spm_prevalidate_b1_upgrade: %d\n",
			valid);
	} else {
		printk("\nS0 valid? %s\n",
			valid == 1 ? "True" : valid == 0 ? "False" : "Unknown");
	}
#endif

	u32_t info_part = 0;
	u32_t ficr_addr = (u32_t)&NRF_FICR_S->INFO.PART;

	printk("\nRead FICR, offset 0x20C (address 0x%08x):\n", ficr_addr);
	ret = spm_request_read(&info_part, ficr_addr, sizeof(u32_t));
	if (ret != 0) {
		printk("Could not read FICR (err: %d)\n", ret);
	} else {
		printk("FICR.INFO.PART (+0x20C) = 0x%08X\n", info_part);
	}

	printk("\nReboot in %d seconds.\n", sleep_time_s);
	k_sleep(K_SECONDS(5));

	sys_reboot(0); /* Argument is ignored. */
}
