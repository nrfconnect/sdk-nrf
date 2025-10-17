/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "cracen_psa.h"
#include "cracen_psa_primitives.h"
#include "../common.h"
#include "platform_keys.h"
#include <psa/nrf_platform_key_ids.h>
#include <sdfw/lcs.h>
#include <sdfw/lcs_helpers.h>
#include <stdint.h>
#include <psa/crypto.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/util.h>

#include <nrfx.h>
#include <hal/nrf_mramc.h>

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
#define PLATFORM_KEY_GET_ACCESS(x)     (((x) >> 24) & 0xf)

#define PLATFORM_KEY_REVOKED_FLAG (0xFA50)

#define MAX_KEY_SIZE 32

static struct {
	uint8_t id;
	const uint8_t *label;
} owner_to_label[] = {{DOMAIN_SECURE, "SECURE-"}, {DOMAIN_SYSCTRL, "SYSCTRL-"},
		      {DOMAIN_CELL, "CELL-"}, {DOMAIN_WIFI, "WIFI-"},
		      {DOMAIN_RADIO, "RADIOCORE-"},	  {DOMAIN_APPLICATION, "APPLICATION-"}};

static struct {
	uint8_t id;
	const uint8_t *label;
} key_type_to_label[] = {
	{USAGE_FWENC, "FWENC-"}, {USAGE_STMTRACE, "STMTRACE-"}, {USAGE_COREDUMP, "COREDUMP-"}};

#define DERIVED_KEY_MAX_LABEL_SIZE 32

typedef struct sicr_key {
	/* The nonce stored in SICR is 4 bytes, but we put it into a 12-byte array here, so it can
	 * easily be supplied using the PSA APIs.
	 */
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

typedef struct derived_key {
	uint8_t label[DERIVED_KEY_MAX_LABEL_SIZE];
} derived_key;

typedef struct ikg_key {
	uint32_t id;
	uint32_t domain;
} ikg_key;

typedef union {
	sicr_key sicr;
	derived_key derived;
	ikg_key ikg;
} platform_key;

typedef enum {
	INVALID,
	DERIVED,
	SICR,
	IKG,
	REVOKED
} key_type;

#define APPEND_STR(str, end, part)                                                                 \
	{                                                                                          \
		if ((str) + strlen(part) > (end))                                                  \
			return INVALID;                                                            \
		memcpy((str), (part), strlen(part));                                               \
		(str) += strlen(part);                                                             \
	}

static key_type find_key(uint32_t id, platform_key *key)
{
	safe_memzero(key, sizeof(platform_key));

	uint32_t usage = PLATFORM_KEY_GET_USAGE(id);
	uint32_t domain = PLATFORM_KEY_GET_DOMAIN(id);
	uint32_t generation = PLATFORM_KEY_GET_GENERATION(id);

#define FILL_AESKEY(x, gen)                                                                        \
	{                                                                                          \
		if ((gen) >= ARRAY_SIZE(x)) {                                                      \
			return INVALID;                                                            \
		}                                                                                  \
		key->sicr.nonce[0] = (x)[gen].NONCE;                                               \
		key->sicr.nonce_addr = (uint8_t *)&(x)[gen].NONCE;                                 \
		key->sicr.type = (x)[gen].ATTR & 0xffff;                                           \
		key->sicr.bits = (x)[gen].ATTR >> 16;                                              \
		key->sicr.attr_addr = (uint8_t *)&(x)[gen].ATTR;                                   \
		key->sicr.key_buffer = (uint8_t *)(x)[gen].CIPHERTEXT;                             \
		key->sicr.key_buffer_max_length = sizeof((x)[gen].CIPHERTEXT);                     \
		key->sicr.mac = (uint8_t *)(x)[gen].MAC;                                           \
		key->sicr.mac_size = sizeof((x)[gen].MAC);                                         \
		return (key->sicr.type == PLATFORM_KEY_REVOKED_FLAG) ? REVOKED : SICR;             \
	}                                                                                          \
	break;

#define FILL_PUBKEY(x, gen)                                                                        \
	{                                                                                          \
		if ((gen) >= ARRAY_SIZE(x)) {                                                      \
			return INVALID;                                                            \
		}                                                                                  \
		key->sicr.nonce[0] = (x)[gen].NONCE;                                               \
		key->sicr.nonce_addr = (uint8_t *)&(x)[gen].NONCE;                                 \
		key->sicr.type = (x)[gen].ATTR & 0xffff;                                           \
		key->sicr.bits = (x)[gen].ATTR >> 16;                                              \
		key->sicr.attr_addr = (uint8_t *)&(x)[gen].ATTR;                                   \
		key->sicr.key_buffer = (uint8_t *)(x)[gen].PUBKEY;                                 \
		key->sicr.key_buffer_max_length = sizeof((x)[gen].PUBKEY);                         \
		key->sicr.mac = (uint8_t *)(x)[gen].MAC;                                           \
		key->sicr.mac_size = sizeof((x)[gen].MAC);                                         \
		return (key->sicr.type == PLATFORM_KEY_REVOKED_FLAG) ? REVOKED : SICR;             \
	}                                                                                          \
	break;

#define DOMAIN_USAGE(domain, usage) ((domain) << 8 | (usage))

	switch (DOMAIN_USAGE(domain, usage)) {
	case DOMAIN_USAGE(DOMAIN_APPLICATION, USAGE_PUBKEY):
		FILL_PUBKEY(NRF_SECURE_SICR_S->AROT.APPLICATION.SUITPUBKEY, generation);
	case DOMAIN_USAGE(DOMAIN_WIFI, USAGE_PUBKEY):
		FILL_PUBKEY(NRF_SECURE_SICR_S->AROT.WIFICORE.SUITPUBKEY, generation);
	case DOMAIN_USAGE(DOMAIN_CELL, USAGE_PUBKEY):
		FILL_PUBKEY(NRF_SECURE_SICR_S->AROT.CELLULARCORE.SUITPUBKEY, generation);
	case DOMAIN_USAGE(DOMAIN_RADIO, USAGE_PUBKEY):
		FILL_PUBKEY(NRF_SECURE_SICR_S->AROT.RADIO.SUITPUBKEY, generation);
	case DOMAIN_USAGE(DOMAIN_NONE, USAGE_RMOEM):
		FILL_PUBKEY(NRF_SECURE_SICR_S->AROT.SECURE.OEMPUBKEY, generation);
	case DOMAIN_USAGE(DOMAIN_APPLICATION, USAGE_AUTHDEBUG):
		FILL_PUBKEY(NRF_SECURE_SICR_S->AROT.APPLICATION.AUTHDEBUGKEY, generation);
	case DOMAIN_USAGE(DOMAIN_RADIO, USAGE_AUTHDEBUG):
		FILL_PUBKEY(NRF_SECURE_SICR_S->AROT.RADIO.AUTHDEBUGKEY, generation);
	case DOMAIN_USAGE(DOMAIN_WIFI, USAGE_AUTHDEBUG):
		FILL_PUBKEY(NRF_SECURE_SICR_S->AROT.WIFICORE.AUTHDEBUGKEY, generation);
	case DOMAIN_USAGE(DOMAIN_CELL, USAGE_AUTHDEBUG):
		FILL_PUBKEY(NRF_SECURE_SICR_S->AROT.CELLULARCORE.AUTHDEBUGKEY, generation);
	case DOMAIN_USAGE(DOMAIN_APPLICATION, USAGE_FWENC):
		FILL_AESKEY(NRF_SECURE_SICR_S->AROT.APPLICATION.FWENC, generation);
	case DOMAIN_USAGE(DOMAIN_RADIO, USAGE_FWENC):
		FILL_AESKEY(NRF_SECURE_SICR_S->AROT.RADIO.FWENC, generation);
	}

	if (usage == USAGE_FWENC || usage == USAGE_STMTRACE || usage == USAGE_COREDUMP) {
		uint8_t *labelptr = key->derived.label;
		uint8_t *end = labelptr + sizeof(key->derived.label);

		bool valid_owner = false;

		for (size_t i = 0; i < ARRAY_SIZE(owner_to_label); i++) {
			if (owner_to_label[i].id == domain) {
				APPEND_STR(labelptr, end, owner_to_label[i].label);
				valid_owner = true;
				break;
			}
		}

		bool valid_key_type = false;

		for (size_t i = 0; i < ARRAY_SIZE(key_type_to_label); i++) {
			if (key_type_to_label[i].id == usage) {
				APPEND_STR(labelptr, end, key_type_to_label[i].label);
				valid_key_type = true;
				break;
			}
		}

		static const uint8_t *const genstr[] = {"0", "1", "2", "3"};

		if (generation > ARRAY_SIZE(genstr) || !valid_key_type || !valid_owner) {
			return INVALID;
		}

		APPEND_STR(labelptr, end, genstr[generation]);

		return DERIVED;
	}

	if (usage == USAGE_IAK || usage == USAGE_MKEK || usage == USAGE_MEXT) {
		/* IKG keys are populated in cracen_load_keyref and cracen_get_builtin_key.
		 * Here we only do the matching of platform key id to the internal
		 * Cracen key id that the rest of the driver can understand.
		 */
		switch (usage) {
		case USAGE_IAK:
			key->ikg.id = CRACEN_BUILTIN_IDENTITY_KEY_ID;
			break;
		case USAGE_MKEK:
			key->ikg.id = CRACEN_BUILTIN_MKEK_ID;
			break;
		case USAGE_MEXT:
			key->ikg.id = CRACEN_BUILTIN_MEXT_ID;
			break;
		}

		key->ikg.domain = domain;
		return IKG;
	}

	return INVALID;
}

static void write_sicr_key_to_mram(platform_key *key, uint32_t sicr_attr, const uint8_t *key_buffer,
				   size_t key_buffer_size)
{
	NRF_MRAMC_Type *mramc = (NRF_MRAMC_Type *)DT_REG_ADDR(DT_NODELABEL(mramc));
	nrf_mramc_config_t mramc_config, mramc_config_write_enabled;
	nrf_mramc_readynext_timeout_t readynext_timeout, short_readynext_timeout;

	nrf_mramc_config_get(mramc, &mramc_config);
	mramc_config_write_enabled = mramc_config;

	/* Ensure MRAMC is configured for SICR writing */
	mramc_config_write_enabled.mode_write = NRF_MRAMC_MODE_WRITE_DIRECT;

	nrf_mramc_config_set(mramc, &mramc_config_write_enabled);

	memcpy(key->sicr.attr_addr, &sicr_attr, sizeof(sicr_attr));
	memcpy(key->sicr.key_buffer, key_buffer, key_buffer_size);

	nrf_mramc_readynext_timeout_get(mramc, &readynext_timeout);

	/* Ensure that nonce is committed to MRAM by setting MRAMC READYNEXT timeout to 0 */
	short_readynext_timeout.value = 0;
	short_readynext_timeout.direct_write = true;
	nrf_mramc_readynext_timeout_set(mramc, &short_readynext_timeout);

	/* Only store the 4 first bytes of the nonce, the rest are padded with zeros */
	memcpy(key->sicr.nonce_addr, &key->sicr.nonce, sizeof(key->sicr.nonce[0]));

	/* Restore MRAMC config */
	nrf_mramc_config_set(mramc, &mramc_config);
	nrf_mramc_readynext_timeout_set(mramc, &readynext_timeout);
}

/**
 * @brief Checks whether key usage from a certain domain can access key.
 *
 * @param[in] domain_id     Originator domain of the key access request.
 * @param[in] key_id        Key id with domain in N5-N4.
 *
 * @return psa_status_t
 */
static psa_status_t verify_access(uint32_t domain_id, uint32_t key_id)
{
	switch (PLATFORM_KEY_GET_ACCESS(key_id)) {
	case ACCESS_INTERNAL:
		/* These keys can be accessed by secure domain. */
		if (domain_id == DOMAIN_SECURE) {
			return PSA_SUCCESS;
		}

		/* Check if access to the target domain key is allowed depending on LCS */
		enum lcs_domain_id domain_lcs = 0;
		const int key_domain = PLATFORM_KEY_GET_DOMAIN(key_id);

		if (key_domain == 0) {
			/* OEM keys can be installed by any domain */
			return PSA_SUCCESS;
		}

		int status = nrf_domain_to_lcs_domain(key_domain, &domain_lcs);

		if (status != 0) {
			return PSA_ERROR_DOES_NOT_EXIST;
		}

		switch (lcs_get(domain_lcs)) {
		case LCS_EMPTY:
		case LCS_ROT:
			return PSA_SUCCESS;
		default:
			return PSA_ERROR_NOT_PERMITTED;
		}
	case ACCESS_LOCAL:
		/* Key can be accessed by both secure domain and the domain it belongs to. */
		if (domain_id == DOMAIN_SECURE) {
			return PSA_SUCCESS;
		} else if (domain_id == PLATFORM_KEY_GET_DOMAIN(key_id)) {
			return PSA_SUCCESS;
		}
		return PSA_ERROR_NOT_PERMITTED;
	default:
		return PSA_ERROR_DOES_NOT_EXIST;
	}
}

psa_status_t cracen_platform_get_builtin_key(psa_drv_slot_number_t slot_number,
					     psa_key_attributes_t *attributes, uint8_t *key_buffer,
					     size_t key_buffer_size, size_t *key_buffer_length)
{
	platform_key key;
	key_type type = find_key((uint32_t)slot_number, &key);

	if (type == REVOKED) {
		return PSA_ERROR_NOT_PERMITTED;
	}

	if (type == SICR) {
		uint32_t key_id = (uint32_t)slot_number;
		uint32_t domain = PLATFORM_KEY_GET_DOMAIN(key_id);
		size_t key_bytes = PSA_BITS_TO_BYTES(key.sicr.bits);
		size_t outlen;

		uint8_t decrypted_key[MAX_KEY_SIZE];

		psa_set_key_bits(attributes, key.sicr.bits);
		psa_set_key_type(attributes, key.sicr.type);

		if (key.sicr.type == PSA_KEY_TYPE_ECC_PUBLIC_KEY(PSA_ECC_FAMILY_TWISTED_EDWARDS)) {
			psa_set_key_algorithm(attributes, PSA_ALG_PURE_EDDSA);
			psa_set_key_usage_flags(attributes, PSA_KEY_USAGE_VERIFY_MESSAGE);
		} else if (key.sicr.type == PSA_KEY_TYPE_AES) {
			/*
			 * Currently AES-KW is not supported, so the key stored
			 * encrypted in SICR is always used as the content
			 * encryption key to decrypt the firmware (using
			 * AES-GCM). When AES-KW is supported, this will possibly
			 * need to be extended so that both AES-KW (encrypted key
			 * stored in SICR used as KEK) and direct decryption
			 * (encrypted key stored in SICR used as CEK) can be
			 * supported.
			 */
			psa_set_key_algorithm(attributes, PSA_ALG_GCM);
			psa_set_key_usage_flags(attributes, PSA_KEY_USAGE_DECRYPT);

			if (PSA_BITS_TO_BYTES(key.sicr.bits) > sizeof(decrypted_key)) {
				return PSA_ERROR_CORRUPTION_DETECTED;
			}
		} else {
			return PSA_ERROR_INVALID_HANDLE;
		}

		/* Note: PSA Driver wrapper API require that attributes are filled before returning
		 * error.
		 */
		if (key_bytes > key_buffer_size) {
			return PSA_ERROR_BUFFER_TOO_SMALL;
		} else if (key_buffer == NULL || key_buffer_length == NULL) {
			return PSA_ERROR_INVALID_ARGUMENT;
		}

		uint32_t mkek_plat_key_id =
			HALTIUM_PLATFORM_PSA_KEY_ID(ACCESS_INTERNAL, domain, USAGE_MKEK, 0);
		/* The opaque key is populated by the PSA core when the IKG key is accessed from the
		 * top level PSA APIs, now that we use the PSA driver APIs directly we need to
		 * populate it manually.
		 */
		ikg_opaque_key mkek_ikg_opaque_key = {.slot_number = CRACEN_INTERNAL_HW_KEY1_ID,
						      .owner_id = domain};

		psa_key_attributes_t mkek_attr = PSA_KEY_ATTRIBUTES_INIT;

		psa_set_key_id(&mkek_attr, mbedtls_svc_key_id_make(domain, mkek_plat_key_id));
		psa_set_key_lifetime(&mkek_attr, PSA_KEY_LIFETIME_FROM_PERSISTENCE_AND_LOCATION(
							 PSA_KEY_PERSISTENCE_READ_ONLY,
							 PSA_KEY_LOCATION_CRACEN));
		psa_set_key_type(&mkek_attr, PSA_KEY_TYPE_AES);


		cracen_aead_operation_t op = {};

		psa_status_t status = cracen_aead_decrypt_setup(
			&op, &mkek_attr, (uint8_t *)&mkek_ikg_opaque_key,
			sizeof(mkek_ikg_opaque_key),
			PSA_ALG_AEAD_WITH_SHORTENED_TAG(PSA_ALG_GCM, key.sicr.mac_size));
		if (status != PSA_SUCCESS) {
			return status;
		}

		status = cracen_aead_set_nonce(&op, (uint8_t *)key.sicr.nonce,
					       sizeof(key.sicr.nonce));
		if (status != PSA_SUCCESS) {
			return status;
		}

		status = cracen_aead_update_ad(&op, (uint8_t *)&key.sicr.type,
					       sizeof(key.sicr.type));
		if (status != PSA_SUCCESS) {
			return status;
		}
		status = cracen_aead_update_ad(&op, (uint8_t *)&key.sicr.bits,
					       sizeof(key.sicr.bits));
		if (status != PSA_SUCCESS) {
			return status;
		}

		if (key.sicr.type == PSA_KEY_TYPE_AES) {
			status = cracen_aead_update(&op, key.sicr.key_buffer, key_bytes,
						    decrypted_key, sizeof(decrypted_key), &outlen);
		} else {
			status = cracen_aead_update_ad(&op, key.sicr.key_buffer, key_bytes);
		}

		if (status != PSA_SUCCESS) {
			goto cleanup;
		}

		status = cracen_aead_finish(&op, NULL, 0, &outlen, key.sicr.mac, key.sicr.mac_size,
					    &key.sicr.mac_size);
		if (status != PSA_SUCCESS) {
			goto cleanup;
		}

		if (key.sicr.type == PSA_KEY_TYPE_AES) {
			memcpy(key_buffer, decrypted_key, key_bytes);
		} else {
			memcpy(key_buffer, key.sicr.key_buffer, key_bytes);
		}
		*key_buffer_length = key_bytes;

cleanup:
		safe_memzero(decrypted_key, sizeof(decrypted_key));

		return status;
	}

	if (type == DERIVED) {
		psa_set_key_bits(attributes, 256);
		psa_set_key_type(attributes, PSA_KEY_TYPE_AES);
		psa_set_key_algorithm(attributes, PSA_ALG_GCM);
		psa_set_key_usage_flags(attributes, PSA_KEY_USAGE_DECRYPT);

		/* Note: PSA Driver wrapper API require that attributes are filled before returning
		 * error.
		 */
		if (32 > key_buffer_size) {
			return PSA_ERROR_BUFFER_TOO_SMALL;
		} else if (key_buffer == NULL || key_buffer_length == NULL) {
			return PSA_ERROR_INVALID_ARGUMENT;
		}

		cracen_key_derivation_operation_t op = {};
		psa_status_t status;
		psa_key_attributes_t prot_attr = PSA_KEY_ATTRIBUTES_INIT;

		psa_set_key_id(&prot_attr,
			       mbedtls_svc_key_id_make(0, CRACEN_PROTECTED_RAM_AES_KEY0_ID));
		psa_set_key_type(&prot_attr, PSA_KEY_TYPE_AES);
		psa_set_key_algorithm(&prot_attr, PSA_ALG_SP800_108_COUNTER_CMAC);
		psa_set_key_lifetime(&prot_attr, PSA_KEY_LIFETIME_FROM_PERSISTENCE_AND_LOCATION(
							 PSA_KEY_PERSISTENCE_READ_ONLY,
							 PSA_KEY_LOCATION_CRACEN));

		status = cracen_key_derivation_setup(&op, PSA_ALG_SP800_108_COUNTER_CMAC);
		if (status != PSA_SUCCESS) {
			return status;
		}
		status = cracen_key_derivation_input_key(&op, PSA_KEY_DERIVATION_INPUT_SECRET,
							 &prot_attr, NULL, 0);
		if (status != PSA_SUCCESS) {
			return status;
		}
		status = cracen_key_derivation_input_bytes(&op, PSA_KEY_DERIVATION_INPUT_LABEL,
							   key.derived.label,
							   strlen(key.derived.label));
		if (status != PSA_SUCCESS) {
			return status;
		}
		status = cracen_key_derivation_output_bytes(&op, key_buffer, 32);
		if (status != PSA_SUCCESS) {
			return status;
		}

		*key_buffer_length = 32;
		return status;
	}

	return PSA_ERROR_CORRUPTION_DETECTED;
}

psa_status_t cracen_platform_keys_get_size(psa_key_attributes_t const *attributes, size_t *key_size)
{
	platform_key key;
	key_type type = find_key(MBEDTLS_SVC_KEY_ID_GET_KEY_ID(psa_get_key_id(attributes)), &key);
	psa_key_type_t key_type = psa_get_key_type(attributes);

	if (type == REVOKED) {
		return PSA_ERROR_NOT_PERMITTED;
	} else if (type == IKG) {
		*key_size = sizeof(ikg_opaque_key);
		return PSA_SUCCESS;
	} else if (key_type == PSA_KEY_TYPE_ECC_PUBLIC_KEY(PSA_ECC_FAMILY_TWISTED_EDWARDS) ||
		   key_type == PSA_KEY_TYPE_AES) {
		*key_size = PSA_BITS_TO_BYTES(psa_get_key_bits(attributes));
		return PSA_SUCCESS;
	}

	return PSA_ERROR_INVALID_ARGUMENT;
}

bool cracen_platform_keys_is_ikg_key(psa_key_attributes_t const *attributes)
{
	platform_key key;
	key_type type = find_key(MBEDTLS_SVC_KEY_ID_GET_KEY_ID(psa_get_key_id(attributes)), &key);

	return (type == IKG);
}

uint32_t cracen_platform_keys_get_owner(psa_key_attributes_t const *attributes)
{
	return PLATFORM_KEY_GET_DOMAIN(MBEDTLS_SVC_KEY_ID_GET_KEY_ID(psa_get_key_id(attributes)));
}

psa_status_t cracen_platform_get_key_slot(mbedtls_svc_key_id_t key_id, psa_key_lifetime_t *lifetime,
					  psa_drv_slot_number_t *slot_number)
{
	platform_key key;
	key_type type = find_key(MBEDTLS_SVC_KEY_ID_GET_KEY_ID(key_id), &key);

	if (type == REVOKED) {
		return PSA_ERROR_NOT_PERMITTED;
	}

	psa_status_t status = verify_access(MBEDTLS_SVC_KEY_ID_GET_OWNER_ID(key_id),
					    MBEDTLS_SVC_KEY_ID_GET_KEY_ID(key_id));
	if (status != PSA_SUCCESS) {
		return status;
	}

	if (type == SICR || type == DERIVED) {
		*slot_number = MBEDTLS_SVC_KEY_ID_GET_KEY_ID(key_id);
		*lifetime = PSA_KEY_LIFETIME_FROM_PERSISTENCE_AND_LOCATION(
			PSA_KEY_PERSISTENCE_READ_ONLY, PSA_KEY_LOCATION_CRACEN);

		if (type == SICR && key.sicr.bits == UINT16_MAX) {
			return PSA_ERROR_DOES_NOT_EXIST;
		}
		return PSA_SUCCESS;
	}

	if (type == IKG) {
		*slot_number = key.ikg.id;
		*lifetime = PSA_KEY_LIFETIME_FROM_PERSISTENCE_AND_LOCATION(
			PSA_KEY_PERSISTENCE_READ_ONLY, PSA_KEY_LOCATION_CRACEN);

		return PSA_SUCCESS;
	}

	return PSA_ERROR_DOES_NOT_EXIST;
}

psa_status_t cracen_platform_keys_provision(const psa_key_attributes_t *attributes,
					    const uint8_t *key_buffer, size_t key_buffer_size)
{
	uint32_t key_id = MBEDTLS_SVC_KEY_ID_GET_KEY_ID(psa_get_key_id(attributes));
	platform_key key;
	key_type type = find_key(MBEDTLS_SVC_KEY_ID_GET_KEY_ID(psa_get_key_id(attributes)), &key);
	uint32_t domain = PLATFORM_KEY_GET_DOMAIN(key_id);
	uint8_t encrypted_key[MAX_KEY_SIZE];
	size_t outlen;

	if (type == REVOKED) {
		return PSA_ERROR_NOT_PERMITTED;
	} else if (type != SICR) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	psa_status_t status =
		verify_access(MBEDTLS_SVC_KEY_ID_GET_OWNER_ID(psa_get_key_id(attributes)),
			      MBEDTLS_SVC_KEY_ID_GET_KEY_ID(psa_get_key_id(attributes)));
	if (status != PSA_SUCCESS) {
		return status;
	}

	/* The memory is required to be filled with 0xFF before the keys are provisioned. If another
	 * value is found, we assume the key has already been provisioned.
	 */
	if (key.sicr.bits != UINT16_MAX) {
		return PSA_ERROR_ALREADY_EXISTS;
	}

	if (key_buffer_size > key.sicr.key_buffer_max_length) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	key.sicr.type = psa_get_key_type(attributes);
	key.sicr.bits = psa_get_key_bits(attributes);

	/* Generate the 4 first bytes of the nonce, the rest are padded with zeros */
	status = cracen_get_random(NULL, (uint8_t *)key.sicr.nonce, sizeof(key.sicr.nonce[0]));
	if (status != PSA_SUCCESS) {
		return status;
	}

	psa_key_attributes_t mkek_attr = PSA_KEY_ATTRIBUTES_INIT;

	uint32_t mkek_plat_key_id =
		HALTIUM_PLATFORM_PSA_KEY_ID(ACCESS_INTERNAL, domain, USAGE_MKEK, 0);
	/* The opaque key is populated by the PSA core when the IKG key is accessed from the top
	 * level PSA APIs, now that we use the PSA driver APIs directly we need to populate it
	 * manually.
	 */
	ikg_opaque_key mkek_ikg_opaque_key = {.slot_number = CRACEN_INTERNAL_HW_KEY1_ID,
					      .owner_id = domain};

	psa_set_key_id(&mkek_attr, mbedtls_svc_key_id_make(domain, mkek_plat_key_id));
	psa_set_key_lifetime(&mkek_attr,
			     PSA_KEY_LIFETIME_FROM_PERSISTENCE_AND_LOCATION(
				     PSA_KEY_PERSISTENCE_READ_ONLY, PSA_KEY_LOCATION_CRACEN));
	psa_set_key_type(&mkek_attr, PSA_KEY_TYPE_AES);

	cracen_aead_operation_t op = {};

	status = cracen_aead_encrypt_setup(
		&op, &mkek_attr, (uint8_t *)&mkek_ikg_opaque_key, sizeof(mkek_ikg_opaque_key),
		PSA_ALG_AEAD_WITH_SHORTENED_TAG(PSA_ALG_GCM, key.sicr.mac_size));
	if (status != PSA_SUCCESS) {
		return status;
	}
	status = cracen_aead_set_nonce(&op, (uint8_t *)key.sicr.nonce, sizeof(key.sicr.nonce));
	if (status != PSA_SUCCESS) {
		return status;
	}
	status = cracen_aead_update_ad(&op, (uint8_t *)&key.sicr.type, sizeof(key.sicr.type));
	if (status != PSA_SUCCESS) {
		return status;
	}
	status = cracen_aead_update_ad(&op, (uint8_t *)&key.sicr.bits, sizeof(key.sicr.bits));
	if (status != PSA_SUCCESS) {
		return status;
	}

	if (key.sicr.type == PSA_KEY_TYPE_AES) {
		status = cracen_aead_update(&op, key_buffer, key_buffer_size, encrypted_key,
					    sizeof(encrypted_key), &outlen);
	} else {
		status = cracen_aead_update_ad(&op, key_buffer, key_buffer_size);
	}

	if (status != PSA_SUCCESS) {
		return status;
	}

	status = cracen_aead_finish(&op, NULL, 0, &outlen, key.sicr.mac, key.sicr.mac_size,
				    &key.sicr.mac_size);
	if (status != PSA_SUCCESS) {
		return status;
	}

	uint32_t sicr_attr = (key.sicr.bits << 16) | key.sicr.type;

	if (key.sicr.type == PSA_KEY_TYPE_AES) {
		write_sicr_key_to_mram(&key, sicr_attr, encrypted_key, key_buffer_size);
	} else {
		write_sicr_key_to_mram(&key, sicr_attr, key_buffer, key_buffer_size);
	}

	return status;
}

psa_status_t cracen_platform_destroy_key(const psa_key_attributes_t *attributes)
{
	platform_key key;
	key_type type = find_key(MBEDTLS_SVC_KEY_ID_GET_KEY_ID(psa_get_key_id(attributes)), &key);
	/* The value 0x00 was chosen arbitrarily here, 0xFF was not used to distinguish revoked keys
	 * from keys not yet written.
	 */
	static const uint8_t revoked_key_val[MAX_KEY_SIZE] = {0x0};

	if (type == REVOKED) {
		return PSA_ERROR_NOT_PERMITTED;
	} else if (type != SICR) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	psa_status_t status =
		verify_access(MBEDTLS_SVC_KEY_ID_GET_OWNER_ID(psa_get_key_id(attributes)),
			      MBEDTLS_SVC_KEY_ID_GET_KEY_ID(psa_get_key_id(attributes)));

	if (status != PSA_SUCCESS) {
		return status;
	}

	uint32_t revoked_key_attr = (key.sicr.bits << 16) | PLATFORM_KEY_REVOKED_FLAG;

	/* The nonce will be written to MRAM based on the buffer in the platform_key, so we
	 * set it here before the call to write function.
	 */
	key.sicr.nonce[0] = 0x0;
	write_sicr_key_to_mram(&key, revoked_key_attr, revoked_key_val, sizeof(revoked_key_val));

	return PSA_SUCCESS;
}
