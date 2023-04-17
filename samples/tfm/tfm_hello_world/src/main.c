/*
 * Copyright (c) 2020 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <string.h>
#include <stdio.h>
#include <tfm_ns_interface.h>
#include "psa/crypto.h"
#ifdef CONFIG_TFM_PARTITION_PLATFORM
#include <tfm_ioctl_api.h>
#endif
#include <pm_config.h>
#include <hal/nrf_gpio.h>

#define HELLO_PATTERN "Hello World! %s"

#define PIN_XL1 0
#define PIN_XL2 1

#if defined(CONFIG_TFM_PARTITION_PLATFORM)
/* Check if MCU selection is required */
#if NRF_GPIO_HAS_SEL
static void gpio_pin_control_select(uint32_t pin_number, nrf_gpio_pin_sel_t mcu)
{
	uint32_t err;
	enum tfm_platform_err_t plt_err;

	plt_err = tfm_platform_gpio_pin_mcu_select(pin_number, mcu, &err);
	if (plt_err != TFM_PLATFORM_ERR_SUCCESS || err != 0) {
		printk("tfm_..._gpio_pin_mcu_select failed: plt_err: 0x%x, err: 0x%x\n",
			plt_err, err);
	}
}
#endif /* NRF_GPIO_HAS_SEL */

static uint32_t secure_read_word(intptr_t ptr)
{
	uint32_t err = 0;
	uint32_t val;
	enum tfm_platform_err_t plt_err;

	plt_err = tfm_platform_mem_read(&val, ptr, 4, &err);
	if (plt_err != TFM_PLATFORM_ERR_SUCCESS || err != 0) {
		printk("tfm_..._mem_read failed: plt_err: 0x%x, err: 0x%x\n",
			plt_err, err);
		return -1;
	}

	return val;
}
#endif /* defined(CONFIG_TFM_PARTITION_PLATFORM) */

static void print_hex_number(uint8_t *num, size_t len)
{
	printk("0x");
	for (int i = 0; i < len; i++) {
		printk("%02x", num[i]);
	}
	printk("\n");
}

int main(void)
{
	char hello_string[sizeof(HELLO_PATTERN) + sizeof(CONFIG_BOARD)];
	size_t len;

	len = snprintf(hello_string, sizeof(hello_string),
		HELLO_PATTERN, CONFIG_BOARD);

	printk("%s\n", hello_string);

#if defined(CONFIG_TFM_PARTITION_PLATFORM)
	uint32_t info_ram;

	printk("Reading some secure memory that NS is allowed to read\n");

	NRF_TIMER1_NS->TASKS_START = 1;
	info_ram = secure_read_word((intptr_t)&NRF_FICR_S->INFO.RAM);
	NRF_TIMER1_NS->TASKS_CAPTURE[0] = 1;
	printk("Approximate IPC overhead us: %d\n", NRF_TIMER1_NS->CC[0]);

	printk("FICR->INFO.RAM: 0x%08x\n", info_ram);
	printk("FICR->INFO.FLASH: 0x%08x\n",
		secure_read_word((intptr_t)&NRF_FICR_S->INFO.FLASH));
#endif /* defined(CONFIG_TFM_PARTITION_PLATFORM) */

	if (IS_ENABLED(CONFIG_TFM_CRYPTO_RNG_MODULE_ENABLED)) {
		psa_status_t status;
		uint8_t random_bytes[32];

		printk("Generating random number\n");
		status = psa_generate_random(random_bytes, sizeof(random_bytes));
		if (status != PSA_SUCCESS) {
			printk("psa_generate_random failed with status %d\n", status);
		} else {
			print_hex_number(random_bytes, sizeof(random_bytes));
		}
	}

	if (IS_ENABLED(CONFIG_TFM_CRYPTO_HASH_MODULE_ENABLED)) {
		psa_status_t status;
		char hello_digest[32];

		printk("Hashing '%s'\n", hello_string);
		status = psa_crypto_init();
		if (status != PSA_SUCCESS) {
			printk("psa_crypto_init failed with status %d\n", status);
		}
		status = psa_hash_compute(PSA_ALG_SHA_256, hello_string,
						strlen(hello_string), hello_digest,
						sizeof(hello_digest), &len);

		if (status != PSA_SUCCESS) {
			printk("psa_hash_compute failed with status %d\n", status);
		} else {
			printk("SHA256 digest:\n");
			print_hex_number(hello_digest, 32);
		}
	}

#if defined(CONFIG_TFM_PARTITION_PLATFORM)
#if NRF_GPIO_HAS_SEL
	/* Configure properly the XL1 and XL2 pins so that the low-frequency crystal
	 * oscillator (LFXO) can be used.
	 * This configuration has already been done by TF-M so this is redundant.
	 */
	gpio_pin_control_select(PIN_XL1, NRF_GPIO_PIN_SEL_PERIPHERAL);
	gpio_pin_control_select(PIN_XL2, NRF_GPIO_PIN_SEL_PERIPHERAL);
	printk("MCU selection configured\n");
#else
	printk("MCU selection skipped\n");
#endif /* defined(GPIO_PIN_CNF_MCUSEL_Msk) */

#ifdef PM_S1_ADDRESS
	bool s0_active = false;
	int ret;

	ret = tfm_platform_s0_active(PM_S0_ADDRESS, PM_S1_ADDRESS, &s0_active);
	if (ret != 0) {
		printk("Unexpected failure from spm_s0_active: %d\n", ret);
	}

	printk("S0 active? %s\n", s0_active ? "True" : "False");
#endif /*  PM_S1_ADDRESS */
#endif /* defined(CONFIG_TFM_PARTITION_PLATFORM) */

	printk("Finished\n");

	return 0;
}
