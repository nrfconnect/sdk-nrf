/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/init.h>
#include <zephyr/logging/log.h>
#include <psa/crypto.h>
#include <psa/nrf_platform_key_ids.h>

LOG_MODULE_REGISTER(pcd, CONFIG_KEY_LOADER_LOG_LEVEL);

static const uint8_t KEY_MANIFEST_PUBKEY_APPLICATION_GEN1[] = {
#include "MANIFEST_PUBKEY_APPLICATION_GEN1"
};
static const uint8_t KEY_MANIFEST_PUBKEY_APPLICATION_GEN2[] = {
#include "MANIFEST_PUBKEY_APPLICATION_GEN2"
};
static const uint8_t KEY_MANIFEST_PUBKEY_APPLICATION_GEN3[] = {
#include "MANIFEST_PUBKEY_APPLICATION_GEN3"
};
static const uint8_t KEY_FWENC_APPLICATION_GEN1[] = {
#include "FWENC_APPLICATION_GEN1"
};
static const uint8_t KEY_FWENC_APPLICATION_GEN2[] = {
#include "FWENC_APPLICATION_GEN2"
};
static const uint8_t KEY_MANIFEST_PUBKEY_RADIO_GEN1[] = {
#include "MANIFEST_PUBKEY_RADIO_GEN1"
};
static const uint8_t KEY_MANIFEST_PUBKEY_RADIO_GEN2[] = {
#include "MANIFEST_PUBKEY_RADIO_GEN2"
};
static const uint8_t KEY_MANIFEST_PUBKEY_RADIO_GEN3[] = {
#include "MANIFEST_PUBKEY_RADIO_GEN3"
};
static const uint8_t KEY_FWENC_RADIO_GEN1[] = {
#include "FWENC_RADIO_GEN1"
};
static const uint8_t KEY_FWENC_RADIO_GEN2[] = {
#include "FWENC_RADIO_GEN2"
};
static const uint8_t KEY_MANIFEST_PUBKEY_OEM_ROOT_GEN1[] = {
#include "MANIFEST_PUBKEY_OEM_ROOT_GEN1"
};
static const uint8_t KEY_MANIFEST_PUBKEY_OEM_ROOT_GEN2[] = {
#include "MANIFEST_PUBKEY_OEM_ROOT_GEN2"
};
static const uint8_t KEY_MANIFEST_PUBKEY_OEM_ROOT_GEN3[] = {
#include "MANIFEST_PUBKEY_OEM_ROOT_GEN3"
};

#define PSA_KEY_LOCATION_CRACEN ((psa_key_location_t)(0x800000 | ('N' << 8)))

/* Map usage, domain, generation to the definition of the key identifier */
#define KEY_ID(usage, domain, gen) UTIL_EVAL(usage##_##domain##_GEN##gen)
#define KEY_VAR(usage, domain, gen) KEY_##usage##_##domain##_GEN##gen

#define LOAD_KEY(fn, usage, domain, gen)                                                            \
	do {                                                                                       \
		fn(UTIL_EVAL(KEY_ID(usage, domain, gen)),                             \
				UTIL_EVAL(KEY_VAR(usage, domain, gen)),                            \
				sizeof(UTIL_EVAL(KEY_VAR(usage, domain, gen))));                   \
	} while (0)

#define LOAD_PUBKEY(usage, domain, gen) LOAD_KEY(load_verify_key, usage, domain, gen)
#define LOAD_ENCKEY(usage, domain, gen) LOAD_KEY(load_enc_key, usage, domain, gen)

static void load_key(mbedtls_svc_key_id_t key_id,
		     const uint8_t *key, size_t key_len, psa_key_attributes_t *key_attributes)
{
	mbedtls_svc_key_id_t volatile_key_id = 0;
	psa_set_key_id(key_attributes, key_id);
	psa_set_key_lifetime(key_attributes, PSA_KEY_LIFETIME_FROM_PERSISTENCE_AND_LOCATION(
				     PSA_KEY_LIFETIME_PERSISTENT, PSA_KEY_LOCATION_CRACEN));

	psa_status_t status = psa_import_key(key_attributes, key, key_len, &volatile_key_id);

	if (status == PSA_SUCCESS) {
		LOG_INF("Key loaded 0x%x", volatile_key_id);
	} else {
		LOG_ERR("SDFW builtin keys - psa_import_key failed, status %d", status);
	}
}

static void load_verify_key(mbedtls_svc_key_id_t key_id, const uint8_t *key, size_t key_len)
{
	psa_key_attributes_t key_attributes = PSA_KEY_ATTRIBUTES_INIT;
	psa_set_key_type(&key_attributes, PSA_KEY_TYPE_ECC_PUBLIC_KEY(PSA_ECC_FAMILY_TWISTED_EDWARDS));
	psa_set_key_algorithm(&key_attributes, PSA_ALG_ECDSA(PSA_ALG_SHA_512));
	psa_set_key_bits(&key_attributes, 255);
	psa_set_key_usage_flags(&key_attributes, PSA_KEY_USAGE_VERIFY_MESSAGE | PSA_KEY_USAGE_VERIFY_HASH);

	load_key(key_id, key, key_len, &key_attributes);
}

static void load_enc_key(mbedtls_svc_key_id_t key_id, const uint8_t *key, size_t key_len)
{
	psa_key_attributes_t key_attributes = PSA_KEY_ATTRIBUTES_INIT;
	psa_set_key_type(&key_attributes, PSA_KEY_TYPE_AES);
	psa_set_key_bits(&key_attributes, 256);
	psa_set_key_usage_flags(&key_attributes, PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT);

	load_key(key_id, key, key_len, &key_attributes);
}

static int load_verify_keys(void)
{

	psa_status_t status = PSA_ERROR_GENERIC_ERROR;

	/* Initialize PSA Crypto */
	status = psa_crypto_init();
	if (status != PSA_SUCCESS) {
		return -1;
	}

	LOAD_PUBKEY(MANIFEST_PUBKEY, APPLICATION, 1);
	LOAD_PUBKEY(MANIFEST_PUBKEY, APPLICATION, 2);
	LOAD_PUBKEY(MANIFEST_PUBKEY, APPLICATION, 3);
	LOAD_ENCKEY(FWENC, APPLICATION, 1);
	LOAD_ENCKEY(FWENC, APPLICATION, 2);

	LOAD_PUBKEY(MANIFEST_PUBKEY, RADIO, 1);
	LOAD_PUBKEY(MANIFEST_PUBKEY, RADIO, 2);
	LOAD_PUBKEY(MANIFEST_PUBKEY, RADIO, 3);
	LOAD_ENCKEY(FWENC, RADIO, 1);
	LOAD_ENCKEY(FWENC, RADIO, 2);

	LOAD_PUBKEY(MANIFEST_PUBKEY, OEM_ROOT, 1);
	LOAD_PUBKEY(MANIFEST_PUBKEY, OEM_ROOT, 2);
	LOAD_PUBKEY(MANIFEST_PUBKEY, OEM_ROOT, 3);

	return 0;

}

static int load_keys(void)
{
	return load_verify_keys();
}

SYS_INIT(load_keys, APPLICATION, 99);
