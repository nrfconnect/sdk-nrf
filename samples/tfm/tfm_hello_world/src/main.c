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

#define HELLO_PATTERN "Hello World! %s\n"

void main(void)
{
	char hello_string[sizeof(HELLO_PATTERN) + sizeof(CONFIG_BOARD)];
	char hello_digest[32];
	size_t len;
	psa_status_t status;
	enum tfm_status_e tfm_status;

	len = snprintf(hello_string, sizeof(hello_string),
		HELLO_PATTERN, CONFIG_BOARD);

	printk("%s", hello_string);

	tfm_status = tfm_ns_interface_init();
	if (tfm_status != TFM_SUCCESS) {
		printk("tfm_ns_interface_init failed with status %d\n", tfm_status);
	}
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
