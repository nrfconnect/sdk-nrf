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

#define HELLO_PATTERN "Hello World! %s"

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

void main(void)
{
	char hello_string[sizeof(HELLO_PATTERN) + sizeof(CONFIG_BOARD)];
	char hello_digest[32];
	size_t len;
	psa_status_t status;

	len = snprintf(hello_string, sizeof(hello_string),
		HELLO_PATTERN, CONFIG_BOARD);

	printk("%s\n", hello_string);

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
		for (int i = 0; i < len; i++) {
			printk("%02x%s", hello_digest[i],
					i % 16 == 15 ? "\n" : "");
		}
	}
}
