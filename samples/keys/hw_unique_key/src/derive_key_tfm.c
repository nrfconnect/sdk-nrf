/*
 * Copyright (c) 2021 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * Based on "Protected Storage service key management" documentation in "Key derivation" chapter:
 * docs/technical_references/design_docs/ps_key_management.rst
 * in Trusted Firmware-M project.
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <psa/crypto.h>
#include <tfm_crypto_defs.h>

#include "derive_key.h"

LOG_MODULE_DECLARE(app, LOG_LEVEL_DBG);

psa_status_t derive_key(psa_key_attributes_t *attributes, uint8_t *key_label,
			uint32_t key_label_len, psa_key_id_t *key_id_out)
{
	psa_status_t status;
	psa_key_derivation_operation_t op = PSA_KEY_DERIVATION_OPERATION_INIT;
	psa_key_id_t key_id;

	*key_id_out = PSA_KEY_ID_NULL;

	status = psa_key_derivation_setup(&op, PSA_ALG_HKDF(PSA_ALG_SHA_256));
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_key_derivation_setup returned error: %d", status);
		return status;
	}

	/* Set up a key derivation operation with HUK  */
	status = psa_key_derivation_input_key(&op, PSA_KEY_DERIVATION_INPUT_SECRET,
					      TFM_BUILTIN_KEY_ID_HUK);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_key_derivation_input_key returned error: %d", status);
		return status;
	}

	/* Supply the PS key label as an input to the key derivation */
	status = psa_key_derivation_input_bytes(&op, PSA_KEY_DERIVATION_INPUT_INFO,
						key_label,
						key_label_len);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_key_derivation_input_bytes returned error: %d", status);
		return status;
	}

	/* Create the storage key from the key derivation operation */
	status = psa_key_derivation_output_key(attributes, &op, &key_id);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_key_derivation_output_key returned error: %d", status);
		return status;
	}

	LOG_INF("(Key resides internally in TF-M)");

	/* Finish key derivation operation and free associated resources */
	status = psa_key_derivation_abort(&op);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_key_derivation_abort returned error: %d", status);
	}

	*key_id_out = key_id;
	return PSA_SUCCESS;
}
