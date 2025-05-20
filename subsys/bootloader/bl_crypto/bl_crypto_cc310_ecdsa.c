/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/types.h>
#include <errno.h>
#include <nrf_cc310_bl_ecdsa_verify_secp256r1.h>
#include <nrf_cc310_bl_hash_sha256.h>
#include <bl_crypto.h>
#include "bl_crypto_internal.h"
#include "bl_crypto_cc310_common.h"


/*! Illegal signature pointer. */
#define CRYS_ECDSA_VERIFY_INVALID_SIGNATURE_IN_PTR_ERROR \
	(CRYS_ECPKI_MODULE_ERROR_BASE + 0x76UL)
/*! Illegal data in pointer. */
#define CRYS_ECDSA_VERIFY_INVALID_MESSAGE_DATA_IN_PTR_ERROR \
	(CRYS_ECPKI_MODULE_ERROR_BASE + 0x80UL)
/*! Illegal data in size. */
#define CRYS_ECDSA_VERIFY_INVALID_MESSAGE_DATA_IN_SIZE_ERROR \
	(CRYS_ECPKI_MODULE_ERROR_BASE + 0x81UL)
/*! Context validation failed. */
#define CRYS_ECDSA_VERIFY_SIGNER_PUBL_KEY_VALIDATION_TAG_ERROR \
	(CRYS_ECPKI_MODULE_ERROR_BASE + 0x83UL)
/*! Verification failed. */
#define CRYS_ECDSA_VERIFY_INCONSISTENT_VERIFY_ERROR \
	(CRYS_ECPKI_MODULE_ERROR_BASE + 0x84UL)

int crypto_init_signing(void)
{
	return cc310_bl_init();
}


int bl_secp256r1_validate(const uint8_t *hash, uint32_t hash_len,
		const uint8_t *public_key, const uint8_t *signature)
{
	CRYSError_t ret;
	nrf_cc310_bl_ecdsa_verify_context_secp256r1_t context;

	cc310_bl_backend_enable();
	ret = nrf_cc310_bl_ecdsa_verify_secp256r1(&context,
			(nrf_cc310_bl_ecc_public_key_secp256r1_t *) public_key,
			(nrf_cc310_bl_ecc_signature_secp256r1_t *)  signature,
			hash,
			hash_len
	);
	cc310_bl_backend_disable();

	switch (ret) {
	case CRYS_ECDSA_VERIFY_SIGNER_PUBL_KEY_VALIDATION_TAG_ERROR:
	case CRYS_ECDSA_VERIFY_INVALID_SIGNATURE_IN_PTR_ERROR:
	case CRYS_ECDSA_VERIFY_INVALID_MESSAGE_DATA_IN_PTR_ERROR:
	case CRYS_ECDSA_VERIFY_INVALID_MESSAGE_DATA_IN_SIZE_ERROR:
		return -EINVAL;
	case CRYS_ECDSA_VERIFY_INCONSISTENT_VERIFY_ERROR:
		return -ESIGINV;
	default:
		return ret;
	}
}
