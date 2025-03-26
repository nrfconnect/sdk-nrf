/*
 * Copyright (c) 2025 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <psa/crypto.h>
#include <psa/crypto_types.h>
#include <cracen_psa_kmu.h>
#include <zephyr/kernel.h>

#define MAKE_PSA_KMU_KEY_ID(id)						\
PSA_KEY_HANDLE_FROM_CRACEN_KMU_SLOT(CRACEN_KMU_KEY_USAGE_SCHEME_RAW, id)

static psa_key_id_t kmu_key_ids[] =  {
	MAKE_PSA_KMU_KEY_ID(226),
	MAKE_PSA_KMU_KEY_ID(228),
	MAKE_PSA_KMU_KEY_ID(230)
};

int main(void)
{
	psa_status_t status = PSA_ERROR_BAD_STATE;

	status = psa_crypto_init();
	if (status == PSA_SUCCESS) {
		for (int i = 0; i < ARRAY_SIZE(kmu_key_ids); i++) {
			status = psa_destroy_key(kmu_key_ids[i]);
			if (status == PSA_SUCCESS) {
				printk("Destroy ok\n");
			} else {
				printk("Destroy failed: %d\n", status);
			}
		}
	} else {
		printk("PSA crypto init failed with error %d\n", status);
	}
	while (1) {
	}
	return 0;
}
