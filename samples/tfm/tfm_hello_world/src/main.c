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

/*
 * The samples require a timer mapped as non-secure:
 * For nRF91 and nRF53 TIMER1 can be mapped as non-secure
 * For nRF54L TIMER1 doesn't exist so we use TIMER00 instead
 */
#if defined(NRF_TIMER1_NS)
#define SAMPLE_NS_TIMER NRF_TIMER1_NS
#elif defined(NRF_TIMER00_NS)
#define SAMPLE_NS_TIMER NRF_TIMER00_NS
#else
#endif

#if defined(CONFIG_TFM_PARTITION_PLATFORM)

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

#if defined(NRF_FICR_S)
void demonstrate_secure_memory_access(void)
{
	uint32_t info_ram;

	SAMPLE_NS_TIMER->TASKS_START = 1;
	info_ram = secure_read_word((intptr_t)&NRF_FICR_S->INFO.RAM);
	SAMPLE_NS_TIMER->TASKS_CAPTURE[0] = 1;
	printk("Approximate IPC overhead us: %d\n", SAMPLE_NS_TIMER->CC[0]);

	printk("FICR->INFO.FLASH: 0x%08x\n",
		secure_read_word((intptr_t)&NRF_FICR_S->INFO.FLASH));
}
#elif defined(NRF_APPLICATION_CPUC_S)
void demonstrate_secure_memory_access(void)
{
	uint32_t cpu_id;

	SAMPLE_NS_TIMER->TASKS_START = 1;
	cpu_id = secure_read_word((intptr_t)&NRF_APPLICATION_CPUC_S->CPUID);
	SAMPLE_NS_TIMER->TASKS_CAPTURE[0] = 1;
	printk("Approximate IPC overhead us: %d\n", SAMPLE_NS_TIMER->CC[0]);
	printk("NRF_APPLICATION_CPUC_S->CPUID: 0x%08x\n", cpu_id);
}
#else
#error "The samples requires either FICR or the CPUC peripheral mapped as \
	secure to demonstrate secure memory access"
#endif

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
	char hello_string[sizeof(HELLO_PATTERN) + sizeof(CONFIG_BOARD_TARGET)];
	size_t len;

	len = snprintf(hello_string, sizeof(hello_string),
		HELLO_PATTERN, CONFIG_BOARD_TARGET);

	printk("%s\n", hello_string);

#if defined(CONFIG_TFM_PARTITION_PLATFORM)
	printk("Reading some secure memory that NS is allowed to read\n");
	demonstrate_secure_memory_access();

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

	printk("Example finished successfully!\n");

	return 0;
}
