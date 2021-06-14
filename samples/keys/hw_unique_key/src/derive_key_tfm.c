/*
 * Copyright (c) 2021 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * Adapted from ps_crypto_setkey() in ps_crypto_interface.c from the
 * Trusted Firmware-M project.
 */

#include <zephyr.h>
#include <sys/printk.h>
#include <psa/crypto.h>
#include <tfm_crypto_defs.h>


psa_key_id_t derive_key(psa_key_attributes_t *attributes, uint8_t *key_label,
			uint32_t label_size)
{
	psa_status_t status;
	psa_key_derivation_operation_t op = PSA_KEY_DERIVATION_OPERATION_INIT;
	psa_key_id_t key_id_out = 0;

	/* Set up a key derivation operation with HUK derivation as the alg */
	status = psa_key_derivation_setup(&op, TFM_CRYPTO_ALG_HUK_DERIVATION);
	if (status != PSA_SUCCESS) {
		printk("psa_key_derivation_setup returned error: %d\n", status);
		return 0;
	}

	/* Supply the key label as an input to the key derivation */
	status = psa_key_derivation_input_bytes(&op, PSA_KEY_DERIVATION_INPUT_LABEL,
						key_label, label_size);
	if (status != PSA_SUCCESS) {
		printk("psa_key_derivation_input_bytes returned error: %d\n", status);
		return 0;
	}

	/* Create the encryption key from the key derivation operation */
	status = psa_key_derivation_output_key(attributes, &op, &key_id_out);
	if (status != PSA_SUCCESS) {
		printk("psa_key_derivation_output_key returned error: %d\n", status);
		return 0;
	}
	printk("(Key resides internally in TF-M)\n");

	/* Free resources associated with the key derivation operation */
	status = psa_key_derivation_abort(&op);
	if (status != PSA_SUCCESS) {
		printk("psa_key_derivation_abort returned error: %d\n", status);
		return 0;
	}

	return key_id_out;
}
