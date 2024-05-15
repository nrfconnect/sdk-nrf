/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "cracen_psa.h"
#include "cracen_psa_primitives.h"
#include "platform_keys.h"
#include <stdint.h>
#include <psa/crypto.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/util.h>

/**
 * PSA Key ID scheme
 *
 * N0  Generation
 * N1  Reserved
 * N2  Usage p0
 * N3  Usage p1
 * N4  Domain p0
 * N5  Domain p1
 * N6  Access rights
 * N7  0x4
 */

#define PLATFORM_KEY_GET_GENERATION(x) ((x)&0xf)
#define PLATFORM_KEY_GET_USAGE(x)      (((x) >> 8) & 0xff)
#define PLATFORM_KEY_GET_DOMAIN(x)     (((x) >> 16) & 0xff)

#define USAGE_FWENC	0x20
#define USAGE_PUBKEY	0x21
#define USAGE_AUTHDEBUG 0x23
#define USAGE_STMTRACE	0x25
#define USAGE_COREDUMP	0x25

#define DOMAIN_NONE	   0x00
#define DOMAIN_SECURE	   0x01
#define DOMAIN_APPLICATION 0x02
#define DOMAIN_RADIO	   0x03
#define DOMAIN_CELL	   0x04
#define DOMAIN_ISIM	   0x05
#define DOMAIN_WIFI	   0x06
#define DOMAIN_SYSCTRL	   0x08

typedef struct sicr_key {
	uint32_t nonce[3];
	uint8_t *nonce_addr;
	psa_key_type_t type;
	psa_key_bits_t bits;
	uint8_t *attr_addr;
	uint8_t *key_buffer;
	size_t key_buffer_max_length;
	uint8_t *mac;
	size_t mac_size;
} sicr_key;

static sicr_key find_key(uint32_t id)
{
	struct sicr_key key = {};

	uint32_t usage = PLATFORM_KEY_GET_USAGE(id);
	uint32_t domain = PLATFORM_KEY_GET_DOMAIN(id);
	uint32_t generation = PLATFORM_KEY_GET_GENERATION(id);

#define FILL_KEY(x, gen)                                                                           \
	{                                                                                          \
		if ((gen) >= ARRAY_SIZE(x)) {                                                      \
			return key;                                                                \
		}                                                                                  \
		key.nonce[0] = (x)[gen].NONCE;                                                     \
		key.nonce_addr = (uint8_t *)&(x)[gen].NONCE;                                       \
		key.type = (x)[gen].ATTR & 0xffff;                                                 \
		key.bits = (x)[gen].ATTR >> 16;                                                    \
		key.attr_addr = (uint8_t *)&(x)[gen].ATTR;                                         \
		key.key_buffer = (uint8_t *)(x)[gen].PUBKEY;                                       \
		key.key_buffer_max_length = sizeof((x)[gen].PUBKEY);                               \
		key.mac = (uint8_t *)(x)[gen].MAC;                                                 \
		key.mac_size = sizeof((x)[gen].MAC);                                               \
	}                                                                                          \
	break;

#define DOMAIN_USAGE(domain, usage) ((domain) << 8 | (usage))

	switch (DOMAIN_USAGE(domain, usage)) {
	case DOMAIN_USAGE(DOMAIN_APPLICATION, USAGE_PUBKEY):
		FILL_KEY(NRF_SECURE_SICR_S->AROT.APPLICATION.SUITPUBKEY, generation);
	case DOMAIN_USAGE(DOMAIN_WIFI, USAGE_PUBKEY):
		FILL_KEY(NRF_SECURE_SICR_S->AROT.WIFICORE.SUITPUBKEY, generation);
	case DOMAIN_USAGE(DOMAIN_CELL, USAGE_PUBKEY):
		FILL_KEY(NRF_SECURE_SICR_S->AROT.CELLULARCORE.SUITPUBKEY, generation);
	case DOMAIN_USAGE(DOMAIN_RADIO, USAGE_PUBKEY):
		FILL_KEY(NRF_SECURE_SICR_S->AROT.RADIO.SUITPUBKEY, generation);
	}

	return key;
}

psa_status_t cracen_platform_get_builtin_key(psa_drv_slot_number_t slot_number,
					     psa_key_attributes_t *attributes, uint8_t *key_buffer,
					     size_t key_buffer_size, size_t *key_buffer_length)
{
	sicr_key key = find_key(slot_number);
	uint32_t key_id = MBEDTLS_SVC_KEY_ID_GET_KEY_ID(psa_get_key_id(attributes));
	uint32_t domain = PLATFORM_KEY_GET_DOMAIN(key_id);

	psa_key_attributes_t mkek_attr = PSA_KEY_ATTRIBUTES_INIT;

	psa_set_key_id(&mkek_attr, mbedtls_svc_key_id_make(domain, CRACEN_BUILTIN_MKEK_ID));
	psa_set_key_lifetime(&mkek_attr,
			     PSA_KEY_LIFETIME_FROM_PERSISTENCE_AND_LOCATION(
				     PSA_KEY_PERSISTENCE_READ_ONLY, PSA_KEY_LOCATION_CRACEN));
	psa_set_key_type(&mkek_attr, PSA_KEY_TYPE_AES);

	cracen_aead_operation_t op = {};
	psa_status_t status = cracen_aead_decrypt_setup(
		&op, &mkek_attr, NULL, 0,
		PSA_ALG_AEAD_WITH_SHORTENED_TAG(PSA_ALG_GCM, key.mac_size));
	if (status != PSA_SUCCESS) {
		return status;
	}

	status = cracen_aead_set_nonce(&op, (uint8_t *)key.nonce, sizeof(key.nonce));
	if (status != PSA_SUCCESS) {
		return status;
	}

	status = cracen_aead_update_ad(&op, (uint8_t *)&key.type, sizeof(key.type));
	if (status != PSA_SUCCESS) {
		return status;
	}
	status = cracen_aead_update_ad(&op, (uint8_t *)&key.bits, sizeof(key.bits));
	if (status != PSA_SUCCESS) {
		return status;
	}
	status = cracen_aead_update_ad(&op, key_buffer, key_buffer_size);
	if (status != PSA_SUCCESS) {
		return status;
	}

	size_t outlen;

	status = cracen_aead_finish(&op, NULL, 0, &outlen, key.mac, key.mac_size, &key.mac_size);
	if (status != PSA_SUCCESS) {
		return status;
	}

	psa_set_key_bits(attributes, key.bits);
	psa_set_key_type(attributes, key.type);

	if (key.type == PSA_KEY_TYPE_ECC_PUBLIC_KEY(PSA_ECC_FAMILY_TWISTED_EDWARDS)) {
		psa_set_key_algorithm(attributes, PSA_ALG_PURE_EDDSA);
		psa_set_key_usage_flags(attributes, PSA_KEY_USAGE_VERIFY_MESSAGE);
	} else {
		return PSA_ERROR_INVALID_HANDLE;
	}

	size_t key_bytes = PSA_BITS_TO_BYTES(key.bits);

	if (key_bytes < key_buffer_size) {
		*key_buffer_length = 0;
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	memcpy(key_buffer, key.key_buffer, key_bytes);
	*key_buffer_length = key_bytes;

	return PSA_SUCCESS;
}

size_t cracen_platform_keys_get_size(psa_key_attributes_t const *attributes)
{
	sicr_key key = find_key(MBEDTLS_SVC_KEY_ID_GET_KEY_ID(psa_get_key_id(attributes)));

	return PSA_BITS_TO_BYTES(key.bits);
}

psa_status_t cracen_platform_get_key_slot(mbedtls_svc_key_id_t key_id, psa_key_lifetime_t *lifetime,
					  psa_drv_slot_number_t *slot_number)
{
	sicr_key key = find_key(MBEDTLS_SVC_KEY_ID_GET_KEY_ID(key_id));

	uint32_t domain = PLATFORM_KEY_GET_DOMAIN(MBEDTLS_SVC_KEY_ID_GET_KEY_ID(key_id));

	if (domain != MBEDTLS_SVC_KEY_ID_GET_OWNER_ID(key_id) &&
	    MBEDTLS_SVC_KEY_ID_GET_OWNER_ID(key_id) != 0) {
		return PSA_ERROR_NOT_PERMITTED;
	}

	if (key.bits > 0) {
		*slot_number = MBEDTLS_SVC_KEY_ID_GET_KEY_ID(key_id);
		*lifetime = PSA_KEY_LIFETIME_FROM_PERSISTENCE_AND_LOCATION(
			PSA_KEY_PERSISTENCE_READ_ONLY, PSA_KEY_LOCATION_CRACEN);
		return PSA_SUCCESS;
	}

	return PSA_ERROR_DOES_NOT_EXIST;
}

psa_status_t cracen_platform_keys_provision(psa_key_attributes_t *attributes, uint8_t *key_buffer,
					    size_t key_buffer_size)
{
	uint32_t key_id = MBEDTLS_SVC_KEY_ID_GET_KEY_ID(psa_get_key_id(attributes));
	sicr_key key = find_key(key_id);
	uint32_t domain = PLATFORM_KEY_GET_DOMAIN(key_id);

	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;

	if (key.bits != UINT16_MAX) {
		return PSA_ERROR_ALREADY_EXISTS;
	}

	if (domain != MBEDTLS_SVC_KEY_ID_GET_OWNER_ID(psa_get_key_id(attributes)) && domain != 0) {
		return PSA_ERROR_NOT_PERMITTED;
	}

	if (key_buffer_size > key.key_buffer_max_length) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	key.type = psa_get_key_type(attributes);
	key.bits = psa_get_key_bits(attributes);

	status = psa_generate_random((uint8_t *)key.nonce, sizeof(key.nonce[0]));
	if (status != PSA_SUCCESS) {
		return status;
	}

	psa_key_attributes_t mkek_attr = PSA_KEY_ATTRIBUTES_INIT;

	psa_set_key_id(&mkek_attr, mbedtls_svc_key_id_make(domain, CRACEN_BUILTIN_MKEK_ID));
	psa_set_key_lifetime(&mkek_attr,
			     PSA_KEY_LIFETIME_FROM_PERSISTENCE_AND_LOCATION(
				     PSA_KEY_PERSISTENCE_READ_ONLY, PSA_KEY_LOCATION_CRACEN));
	psa_set_key_type(&mkek_attr, PSA_KEY_TYPE_AES);

	cracen_aead_operation_t op = {};

	status = cracen_aead_encrypt_setup(
		&op, &mkek_attr, NULL, 0,
		PSA_ALG_AEAD_WITH_SHORTENED_TAG(PSA_ALG_GCM, key.mac_size));
	if (status != PSA_SUCCESS) {
		return status;
	}
	status = cracen_aead_set_nonce(&op, (uint8_t *)key.nonce, sizeof(key.nonce));
	if (status != PSA_SUCCESS) {
		return status;
	}
	status = cracen_aead_update_ad(&op, (uint8_t *)&key.type, sizeof(key.type));
	if (status != PSA_SUCCESS) {
		return status;
	}
	status = cracen_aead_update_ad(&op, (uint8_t *)&key.bits, sizeof(key.bits));
	if (status != PSA_SUCCESS) {
		return status;
	}
	status = cracen_aead_update_ad(&op, key_buffer, key_buffer_size);
	if (status != PSA_SUCCESS) {
		return status;
	}

	size_t outlen;

	status = cracen_aead_finish(&op, NULL, 0, &outlen, key.mac, key.mac_size, &key.mac_size);
	if (status != PSA_SUCCESS) {
		return status;
	}

	uint32_t attr = key.bits | (key.type << 16);

	NRF_MRAMC_Type *mramc = (NRF_MRAMC_Type *)DT_REG_ADDR(DT_NODELABEL(mramc));
	nrf_mramc_config_t mramc_config, mramc_config_write_enabled;

	nrf_mramc_config_get(mramc, &mramc_config);
	mramc_config_write_enabled = mramc_config;

	/* Ensure MRAMC is configured for SICR writing */
	mramc_config_write_enabled.mode_write = NRF_MRAMC_MODE_WRITE_DIRECT;

	nrf_mramc_config_set(mramc, &mramc_config_write_enabled);

	memcpy(key.nonce_addr, &key.nonce, sizeof(key.nonce));
	memcpy(key.attr_addr, &attr, sizeof(attr));
	memcpy(key.key_buffer, key_buffer, key_buffer_size);

	/* Restore MRAMC config */
	nrf_mramc_config_set(mramc, &mramc_config);

	return status;
}
