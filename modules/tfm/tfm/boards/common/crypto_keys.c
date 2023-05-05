/*
 * Copyright (c) 2021 - 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stddef.h>
#include <string.h>

#include "psa/crypto_types.h"
#include "tfm_plat_crypto_keys.h"
#include "tfm_builtin_key_ids.h"
#include "tfm_builtin_key_loader.h"
#include "psa_manifest/pid.h"
#include "tfm_spm_log.h"

#include <hw_unique_key.h>
#include <nrf_cc3xx_platform_kmu.h>
#include <nrf_cc3xx_platform_identity_key.h>
#include <nrf_cc3xx_platform_defines.h>

#define TFM_NS_PARTITION_ID (-1)

#ifndef MBEDTLS_PSA_CRYPTO_KEY_ID_ENCODES_OWNER
#error "MBEDTLS_PSA_CRYPTO_KEY_ID_ENCODES_OWNER must be selected in Mbed TLS config file"
#endif

#ifdef NRF5340_XXAA_APPLICATION
#define HUK_KEY_LEN_BYTES  32
#elif defined(NRF91_SERIES)
#define HUK_KEY_LEN_BYTES  16
#endif

static enum tfm_plat_err_t tfm_plat_get_huk(uint8_t *buf, size_t buf_len,
					    size_t *key_len,
					    size_t *key_bits,
					    psa_algorithm_t *algorithm,
					    psa_key_type_t *type)
{
	if (buf_len < HUK_KEY_LEN_BYTES) {
		return TFM_PLAT_ERR_SYSTEM_ERR;
	}

	uint8_t label[] = "TFM_HW_UNIQ_KEY";

	int err  = hw_unique_key_derive_key(HUK_KEYSLOT_MEXT, NULL,
			0, label, sizeof(label), buf, buf_len);

	if (err) {
		SPMLOG_DBGMSGVAL("hw_unique_key_derive_key err: ", err);

		return TFM_PLAT_ERR_SYSTEM_ERR;
	}

	*key_len = HUK_KEY_LEN_BYTES;
	*key_bits = HUK_KEY_LEN_BYTES * 8;
	*algorithm = PSA_ALG_HKDF(PSA_ALG_SHA_256);
	*type = PSA_KEY_TYPE_DERIVE;

	return TFM_PLAT_ERR_SUCCESS;
}


#ifdef TFM_PARTITION_INITIAL_ATTESTATION
static enum tfm_plat_err_t tfm_plat_get_iak(uint8_t *buf, size_t buf_len,
					    size_t *key_len,
					    size_t *key_bits,
					    psa_algorithm_t *algorithm,
					    psa_key_type_t *type)
{
	if (buf_len < 32) {
		return TFM_PLAT_ERR_SYSTEM_ERR;
	}

	int err = nrf_cc3xx_platform_identity_key_retrieve(
		NRF_KMU_SLOT_KIDENT,
		buf);

	if (err != NRF_CC3XX_PLATFORM_SUCCESS) {
		SPMLOG_DBGMSGVAL("nrf_cc3xx_platform_identity_key_retrieve err: ", err);

		return TFM_PLAT_ERR_SYSTEM_ERR;
	}

	*key_len = 32;
	*key_bits = *key_len * 8;
	*algorithm = PSA_ALG_ECDSA(PSA_ALG_SHA_256);
	*type = PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1);

	return TFM_PLAT_ERR_SUCCESS;
}
#endif /* TFM_PARTITION_INITIAL_ATTESTATION */

enum tfm_plat_err_t tfm_plat_builtin_key_get_usage(psa_key_id_t key_id,
						   mbedtls_key_owner_id_t user,
						   psa_key_usage_t *usage)
{
	*usage = 0;

	switch (key_id) {

	case TFM_BUILTIN_KEY_ID_HUK:
		/* Allow access to all partitions */
		*usage = PSA_KEY_USAGE_DERIVE;
		break;

	case TFM_BUILTIN_KEY_ID_IAK:
		switch(user) {
#ifdef TFM_PARTITION_INITIAL_ATTESTATION
		case TFM_SP_INITIAL_ATTESTATION:
			*usage = PSA_KEY_USAGE_SIGN_HASH;
#ifdef SYMMETRIC_INITIAL_ATTESTATION
		/* Needed to calculate the instance ID */
			*usage |= PSA_KEY_USAGE_EXPORT;
#endif /* SYMMETRIC_INITIAL_ATTESTATION */
			break;

#if defined(TEST_S_ATTESTATION) || defined(TEST_NS_ATTESTATION)
		/* So that the tests can validate created tokens */
#ifdef TEST_S_ATTESTATION
		case TFM_SP_SECURE_TEST_PARTITION:
#endif /* TEST_S_ATTESTATION */
#ifdef TEST_NS_ATTESTATION
		case TFM_NS_PARTITION_ID:
#endif /* TEST_NS_ATTESTATION */
			*usage = PSA_KEY_USAGE_VERIFY_HASH;
			break;
#endif /* TEST_S_ATTESTATION || TEST_NS_ATTESTATION */
#endif /* TFM_PARTITION_INITIAL_ATTESTATION */

		default:
			return TFM_PLAT_ERR_NOT_PERMITTED;
		}

		break;
	default:
		return TFM_PLAT_ERR_UNSUPPORTED;
	}

	return TFM_PLAT_ERR_SUCCESS;
}

enum tfm_plat_err_t tfm_plat_builtin_key_get_lifetime_and_slot(mbedtls_svc_key_id_t key_id,
							       psa_key_lifetime_t *lifetime,
							       psa_drv_slot_number_t *slot_number)
{
	switch (MBEDTLS_SVC_KEY_ID_GET_KEY_ID(key_id)) {

	case TFM_BUILTIN_KEY_ID_HUK:
		*slot_number = TFM_BUILTIN_KEY_SLOT_HUK;
		*lifetime = PSA_KEY_LIFETIME_FROM_PERSISTENCE_AND_LOCATION(
			PSA_KEY_LIFETIME_PERSISTENT,
			TFM_BUILTIN_KEY_LOADER_KEY_LOCATION);
		break;

#ifdef TFM_PARTITION_INITIAL_ATTESTATION
	case TFM_BUILTIN_KEY_ID_IAK:
		*slot_number = TFM_BUILTIN_KEY_SLOT_IAK;
		*lifetime = PSA_KEY_LIFETIME_FROM_PERSISTENCE_AND_LOCATION(
			PSA_KEY_LIFETIME_PERSISTENT,
			TFM_BUILTIN_KEY_LOADER_KEY_LOCATION);
#endif /* TFM_PARTITION_INITIAL_ATTESTATION */
		break;

	default:
		return TFM_PLAT_ERR_UNSUPPORTED;
	}

	return TFM_PLAT_ERR_SUCCESS;
}



enum tfm_plat_err_t tfm_plat_load_builtin_keys(void)
{
	psa_status_t err;
	mbedtls_svc_key_id_t key_id = MBEDTLS_SVC_KEY_ID_INIT;
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
	enum tfm_plat_err_t plat_err;
	uint8_t buf[32];
	size_t key_len;
	size_t key_bits;
	psa_algorithm_t algorithm;
	psa_key_type_t type;

	/* HUK */
	plat_err = tfm_plat_get_huk(buf, sizeof(buf), &key_len, &key_bits,
				    &algorithm, &type);
	if (plat_err != TFM_PLAT_ERR_SUCCESS) {
		return plat_err;
	}

	key_id.MBEDTLS_PRIVATE(key_id) = TFM_BUILTIN_KEY_ID_HUK;
	psa_set_key_id(&attr, key_id);
	psa_set_key_bits(&attr, key_bits);
	psa_set_key_algorithm(&attr, algorithm);
	psa_set_key_type(&attr, type);

	err = tfm_builtin_key_loader_load_key(buf, key_len, &attr);
	if (err != PSA_SUCCESS) {
		return TFM_PLAT_ERR_SYSTEM_ERR;
	}

#ifdef TFM_PARTITION_INITIAL_ATTESTATION
	/* IAK */
	plat_err = tfm_plat_get_iak(buf, sizeof(buf), &key_len, &key_bits,
				    &algorithm, &type);
	if (plat_err != TFM_PLAT_ERR_SUCCESS) {
#ifdef NRF_PROVISIONING
		return TFM_PLAT_ERR_SYSTEM_ERR;
#else
		/* NCSDK-19927: Let the system boot without an IAK provisioned */
		SPMLOG_ERRMSG("No initial attestation key (IAK) provisioned\r\n");
		return TFM_PLAT_ERR_SUCCESS;
#endif
	}

	key_id.MBEDTLS_PRIVATE(key_id) = TFM_BUILTIN_KEY_ID_IAK;
	psa_set_key_id(&attr, key_id);
	psa_set_key_bits(&attr, key_bits);
	psa_set_key_algorithm(&attr, algorithm);
	psa_set_key_type(&attr, type);

	err = tfm_builtin_key_loader_load_key(buf, key_len, &attr);
	if (err != PSA_SUCCESS) {
		return TFM_PLAT_ERR_SYSTEM_ERR;
	}
#endif /* TFM_PARTITION_INITIAL_ATTESTATION */

	return TFM_PLAT_ERR_SUCCESS;
}
