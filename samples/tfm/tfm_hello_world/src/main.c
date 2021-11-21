/*
 * Copyright (c) 2020 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <sys/printk.h>
#include <string.h>
#include <stdio.h>
#include <tfm_ns_interface.h>
#include "psa/crypto.h"
#include <tfm/tfm_ioctl_api.h>
#include <pm_config.h>
#include <hal/nrf_gpio.h>

#define HELLO_PATTERN "Hello World! %s"

#define PIN_XL1 0
#define PIN_XL2 1

/* Check if MCU selection is required */
#if defined(GPIO_PIN_CNF_MCUSEL_Msk)
static void gpio_pin_mcu_select(uint32_t pin_number, nrf_gpio_pin_mcusel_t mcu)
{
	uint32_t err;
	enum tfm_platform_err_t plt_err;

	plt_err = tfm_platform_gpio_pin_mcu_select(pin_number, mcu, &err);
	if (plt_err != TFM_PLATFORM_ERR_SUCCESS || err != 0) {
		printk("tfm_..._gpio_pin_mcu_select failed: plt_err: 0x%x, err: 0x%x\n",
			plt_err, err);
	}
}
#endif /* defined(GPIO_PIN_CNF_MCUSEL_Msk) */

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

static void print_hex_number(uint8_t *num, size_t len)
{
	printk("0x");
	for (int i = 0; i < len; i++) {
		printk("%02x", num[i]);
	}
	printk("\n");
}

void main(void)
{
	char hello_string[sizeof(HELLO_PATTERN) + sizeof(CONFIG_BOARD)];
	char hello_digest[32];
	uint8_t random_bytes[32];
	size_t len;
	psa_status_t status;

	len = snprintf(hello_string, sizeof(hello_string),
		HELLO_PATTERN, CONFIG_BOARD);

	printk("%s\n", hello_string);

	printk("Generating random number\n");
	status = psa_generate_random(random_bytes, sizeof(random_bytes));
	if (status != PSA_SUCCESS) {
		printk("psa_generate_random failed with status %d\n", status);
	} else {
		print_hex_number(random_bytes, sizeof(random_bytes));
	}

	printk("Reading some secure memory that NS is allowed to read\n");
	printk("FICR->INFO.PART: 0x%08x\n",
		secure_read_word((intptr_t)&NRF_FICR_S->INFO.PART));
	printk("FICR->INFO.VARIANT: 0x%08x\n",
		secure_read_word((intptr_t)&NRF_FICR_S->INFO.VARIANT));

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

#if defined(GPIO_PIN_CNF_MCUSEL_Msk)
	/* Configure properly the XL1 and XL2 pins so that the low-frequency crystal
	 * oscillator (LFXO) can be used.
	 * This configuration has already been done by TF-M so this is redundant.
	 */
	printk("Configuring MCU selection for LFXO\n");
	gpio_pin_mcu_select(PIN_XL1, NRF_GPIO_PIN_MCUSEL_PERIPHERAL);
	gpio_pin_mcu_select(PIN_XL2, NRF_GPIO_PIN_MCUSEL_PERIPHERAL);
#endif /* defined(GPIO_PIN_CNF_MCUSEL_Msk) */
}
