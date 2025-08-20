/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <sxsymcrypt/blkcipher.h>
#include <sxsymcrypt/keyref.h>
#include <sxsymcrypt/aes.h>
#include <cracen/statuscodes.h>
#include <zephyr/logging/log.h>
#include "../../../cracenpsa/src/common.h"
#include "cracen_sw_common.h"

LOG_MODULE_DECLARE(cracen, CONFIG_CRACEN_LOG_LEVEL);

psa_status_t cracen_aes_ecb_encrypt(const struct sxkeyref *key, const uint8_t *input,
				    uint8_t *output)
{
	struct sxblkcipher blkciph;
	int sx_status;

	sx_status = sx_blkcipher_create_aesecb_enc(&blkciph, key);
	if (sx_status != SX_OK) {
		return silex_statuscodes_to_psa(sx_status);
	}

	sx_status = sx_blkcipher_crypt(&blkciph, input, SX_BLKCIPHER_AES_BLK_SZ, output);
	if (sx_status != SX_OK) {
		return silex_statuscodes_to_psa(sx_status);
	}

	sx_status = sx_blkcipher_run(&blkciph);
	if (sx_status != SX_OK) {
		return silex_statuscodes_to_psa(sx_status);
	}

	sx_status = sx_blkcipher_wait(&blkciph);
	if (sx_status != SX_OK) {
		return silex_statuscodes_to_psa(sx_status);
	}

	return PSA_SUCCESS;
}
