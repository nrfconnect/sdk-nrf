/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <cracen/mem_helpers.h>
#include <cracen/statuscodes.h>
#include <cracen/lib_kmu.h>
#include <nrf_security_mutexes.h>
#include <nrfx.h>
#include <psa/crypto.h>
#include <stdint.h>
#include <sxsymcrypt/internal.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/__assert.h>

#include "cracen_psa.h"
#include "common.h"
#include "kmu.h"

/* Reserved slot, used to record whether provisioning is in progress for a set of slots.
 * We only use the metadata field, formatted as follows:
 *      Bits 31-16: Unused
 *      Bits 15-8:  slot-id
 *      Bits 7-0:   number of slots
 */
#define PROVISIONING_SLOT 250

extern nrf_security_mutex_t cracen_mutex_symmetric;

/* The section .nrf_kmu_reserved_push_area is placed at the top RAM address
 * by the linker scripts. We do that for both the secure and non-secure builds.
 * Since this buffer is placed on the top of RAM we don't need to have the alignment
 * attribute anymore.
 */
uint8_t kmu_push_area[64] __attribute__((section(".nrf_kmu_reserved_push_area")));

typedef struct kmu_metadata {
	uint32_t metadata_version: 4;
	uint32_t key_usage_scheme: 2;
	uint32_t reserved: 10;
	uint32_t algorithm: 4;
	uint32_t size: 3;
	uint32_t rpolicy: 2;
	uint32_t usage_flags: 7;
} kmu_metadata;
_Static_assert(sizeof(kmu_metadata) == 4, "KMU Metadata must be 32 bits.");

enum kmu_metadata_algorithm {
	METADATA_ALG_CHACHA20 = 1,
	METADATA_ALG_CHACHA20_POLY1305 = 2,
	METADATA_ALG_AES_GCM = 3,
	METADATA_ALG_AES_CCM = 4,
	METADATA_ALG_AES_ECB = 5,
	METADATA_ALG_AES_CTR = 6,
	METADATA_ALG_AES_CBC = 7,
	METADATA_ALG_SP800_108_COUNTER_CMAC = 8,
	METADATA_ALG_CMAC = 9,
	METADATA_ALG_ED25519 = 10,
	METADATA_ALG_ECDSA = 11,
	METADATA_ALG_RESERVED2 = 12,
	METADATA_ALG_RESERVED3 = 13,
	METADATA_ALG_RESERVED4 = 14,
	METADATA_ALG_RESERVED5 = 15,
};

static const psa_key_usage_t metadata_usage_flags_mapping[] = {
	/* ENCRYPT */ PSA_KEY_USAGE_ENCRYPT,
	/* DECRYPT */ PSA_KEY_USAGE_DECRYPT,
	/* SIGN */ PSA_KEY_USAGE_SIGN_HASH | PSA_KEY_USAGE_SIGN_MESSAGE,
	/* VERIFY */ PSA_KEY_USAGE_VERIFY_HASH | PSA_KEY_USAGE_VERIFY_MESSAGE,
	/* DERIVE */ PSA_KEY_USAGE_DERIVE,
	/* EXPORT */ PSA_KEY_USAGE_EXPORT,
	/* COPY */ PSA_KEY_USAGE_COPY};

/**
 * @brief Retrieve the encryption key used to encrypt the key for a KMU slot.
 *
 * @param[in]   context   16 byte input buffer.
 * @param[out]  key       32 byte output buffer for key material.
 *
 * @return psa_status_t
 */
static psa_status_t get_encryption_key(const uint8_t *context, uint8_t *key)
{
	psa_status_t status;
	psa_key_attributes_t mkek_attr = PSA_KEY_ATTRIBUTES_INIT;

	psa_set_key_type(&mkek_attr, PSA_KEY_TYPE_AES);
	psa_set_key_id(&mkek_attr, mbedtls_svc_key_id_make(0, CRACEN_BUILTIN_MKEK_ID));
	psa_set_key_lifetime(&mkek_attr,
			     PSA_KEY_LIFETIME_FROM_PERSISTENCE_AND_LOCATION(
				     PSA_KEY_PERSISTENCE_READ_ONLY, PSA_KEY_LOCATION_CRACEN));

	cracen_key_derivation_operation_t op = {};

	status = cracen_key_derivation_setup(&op, PSA_ALG_SP800_108_COUNTER_CMAC);
	if (status != PSA_SUCCESS) {
		return status;
	}
	status = cracen_key_derivation_input_key(&op, PSA_KEY_DERIVATION_INPUT_SECRET, &mkek_attr,
						 NULL, 0);
	if (status != PSA_SUCCESS) {
		return status;
	}
	status = cracen_key_derivation_input_bytes(&op, PSA_KEY_DERIVATION_INPUT_LABEL, "KMU", 3);
	if (status != PSA_SUCCESS) {
		return status;
	}
	status = cracen_key_derivation_input_bytes(&op, PSA_KEY_DERIVATION_INPUT_CONTEXT, context,
						   CRACEN_KMU_SLOT_KEY_SIZE);
	if (status != PSA_SUCCESS) {
		return status;
	}
	status = cracen_key_derivation_output_bytes(&op, key, 32);
	return status;
}

static psa_status_t cracen_kmu_encrypt(const uint8_t *key, size_t key_length,
				       kmu_metadata *metadata, uint8_t *encrypted_buffer,
				       size_t encrypted_buffer_size,
				       size_t *encrypted_buffer_length)
{
	uint8_t key_buffer[32] = {};
	psa_status_t status;
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;

	psa_set_key_algorithm(&attr, PSA_ALG_GCM);
	psa_set_key_type(&attr, PSA_KEY_TYPE_AES);
	psa_set_key_bits(&attr, 256);
	psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_ENCRYPT);

	if (encrypted_buffer_size > CRACEN_KMU_SLOT_KEY_SIZE) {
		status = psa_generate_random(encrypted_buffer, CRACEN_KMU_SLOT_KEY_SIZE);
	} else {
		return PSA_ERROR_GENERIC_ERROR;
	}

	uint8_t *nonce = encrypted_buffer;

	encrypted_buffer += CRACEN_KMU_SLOT_KEY_SIZE;
	encrypted_buffer_size -= CRACEN_KMU_SLOT_KEY_SIZE;

	status = get_encryption_key(nonce, key_buffer);
	if (status != PSA_SUCCESS) {
		return status;
	}

	status = cracen_aead_encrypt(&attr, key_buffer, sizeof(key_buffer), PSA_ALG_GCM, nonce, 12,
				     (uint8_t *)metadata, sizeof(*metadata), key, key_length,
				     encrypted_buffer, encrypted_buffer_size,
				     encrypted_buffer_length);

	if (status == PSA_SUCCESS) {
		*encrypted_buffer_length += 16;
	}
	return status;
}

static psa_status_t cracen_kmu_decrypt(kmu_metadata *metadata, size_t number_of_slots)
{
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;

	psa_set_key_algorithm(&attr, PSA_ALG_GCM);
	psa_set_key_type(&attr, PSA_KEY_TYPE_AES);
	psa_set_key_bits(&attr, 256);
	psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_DECRYPT);

	uint8_t key_buffer[32] = {50};
	psa_status_t status = get_encryption_key(kmu_push_area, key_buffer);

	if (status != PSA_SUCCESS) {
		return status;
	}

	size_t outlen = 0;

	return cracen_aead_decrypt(&attr, key_buffer, sizeof(key_buffer), PSA_ALG_GCM,
				   kmu_push_area, 12, (uint8_t *)metadata, sizeof(*metadata),
				   kmu_push_area + CRACEN_KMU_SLOT_KEY_SIZE,
				   (number_of_slots - 1) * CRACEN_KMU_SLOT_KEY_SIZE, kmu_push_area,
				   sizeof(kmu_push_area), &outlen);
}

/* Used internally in sxsymcrypt so we use sx return codes here. */
int cracen_kmu_prepare_key(const uint8_t *user_data)
{
	const kmu_opaque_key_buffer *key = (const kmu_opaque_key_buffer *)user_data;

	switch (key->key_usage_scheme) {
	case KMU_METADATA_SCHEME_RAW:
	case KMU_METADATA_SCHEME_PROTECTED:
		for (size_t i = 0; i < key->number_of_slots; i++) {
			if (lib_kmu_push_slot(key->slot_id + i) != 0) {
				return SX_ERR_UNKNOWN_ERROR;
			}
		}
		return SX_OK;
	case KMU_METADATA_SCHEME_ENCRYPTED: {
		kmu_metadata metadata;

		if (lib_kmu_read_metadata((int)key->slot_id, (uint32_t *)&metadata) != 0) {
			return SX_ERR_INVALID_KEYREF;
		}
		for (size_t i = 0; i < key->number_of_slots; i++) {
			if (lib_kmu_push_slot(key->slot_id + i) != 0) {
				return SX_ERR_UNKNOWN_ERROR;
			}
		}

		psa_status_t status = cracen_kmu_decrypt(&metadata, key->number_of_slots);

		if (status != PSA_SUCCESS) {
			return SX_ERR_PLATFORM_ERROR;
		}

		return SX_OK;
	}
	default:
		return SX_ERR_INVALID_KEYREF;
	}

	return SX_OK;
}

int cracen_kmu_clean_key(const uint8_t *user_data)
{
	(void)user_data;
	safe_memzero(kmu_push_area, sizeof(kmu_push_area));

	return SX_OK;
}

/**
 * @brief Checks whether this is the secondary slot. If true, the metadata resides in
 *        one of the previous slots;
 */
bool is_secondary_slot(kmu_metadata *metadata)
{
	uint32_t value = 0xffffffff;

	return memcmp(&value, metadata, sizeof(value)) == 0;
}

static bool can_sign(const psa_key_attributes_t *key_attr)
{
	return (psa_get_key_usage_flags(key_attr) & PSA_KEY_USAGE_SIGN_MESSAGE) ||
	       (psa_get_key_usage_flags(key_attr) & PSA_KEY_USAGE_SIGN_HASH);
}

/**
 * @brief Check provisioning state, and delete slots that were not completely provisioned.
 *
 * @return psa_status_t
 */
static psa_status_t verify_provisioning_state(void)
{
	uint32_t data;
	int st = lib_kmu_read_metadata(PROVISIONING_SLOT, &data);

	if (st == LIB_KMU_SUCCESS) {
		uint32_t slot_id = data >> 8;
		uint32_t num_slots = data & 0xff;

		for (uint32_t i = 0; i < num_slots; i++) {
			st = lib_kmu_revoke_slot(slot_id + i);
			if (st != LIB_KMU_SUCCESS) {
				if (!lib_kmu_is_slot_empty(slot_id + i)) {
					return PSA_ERROR_HARDWARE_FAILURE;
				}
			}
		}

		st = lib_kmu_revoke_slot(PROVISIONING_SLOT);
		if (st != LIB_KMU_SUCCESS) {
			return PSA_ERROR_HARDWARE_FAILURE;
		}
	}

	/* An error reading metadata means the slot was empty, and there is no ongoing transaction.
	 */
	return PSA_SUCCESS;
}

/**
 * @brief Set the provisioning state to be in progress for a number of slots.
 *
 * @param slot_id
 * @param num_slots
 * @return psa_status_t
 */
psa_status_t set_provisioning_in_progress(uint32_t slot_id, uint32_t num_slots)
{
	struct kmu_src_t kmu_desc = {};

	kmu_desc.metadata = slot_id << 8 | num_slots;
	kmu_desc.rpolicy = LIB_KMU_REV_POLICY_ROTATING;

	int st = lib_kmu_provision_slot(PROVISIONING_SLOT, &kmu_desc);

	if (st != LIB_KMU_SUCCESS) {
		return PSA_ERROR_HARDWARE_FAILURE;
	}
	return PSA_SUCCESS;
}

/**
 * @brief Signals that the provisioning of several key slots are finalized.
 *
 * @param slot_id
 * @param num_slots
 * @return psa_status_t
 */
psa_status_t end_provisioning(uint32_t slot_id, uint32_t num_slots)
{
	if (lib_kmu_revoke_slot(PROVISIONING_SLOT) != LIB_KMU_SUCCESS) {
		return PSA_ERROR_HARDWARE_FAILURE;
	}
	return PSA_SUCCESS;
}

psa_status_t cracen_kmu_destroy_key(const psa_key_attributes_t *attributes)
{
	psa_key_location_t location =
		PSA_KEY_LIFETIME_GET_LOCATION(psa_get_key_lifetime(attributes));

	if (location == PSA_KEY_LOCATION_CRACEN_KMU) {
		uint32_t slot_id = CRACEN_PSA_GET_KMU_SLOT(
			MBEDTLS_SVC_KEY_ID_GET_KEY_ID(psa_get_key_id(attributes)));
		size_t num_slots = DIV_ROUND_UP(PSA_BITS_TO_BYTES(psa_get_key_bits(attributes)),
						CRACEN_KMU_SLOT_KEY_SIZE);
		psa_status_t status;

		if (CRACEN_PSA_GET_KEY_USAGE_SCHEME(MBEDTLS_SVC_KEY_ID_GET_KEY_ID(
			    psa_get_key_id(attributes))) == KMU_METADATA_SCHEME_ENCRYPTED) {
			num_slots += 2;
		}

		status = set_provisioning_in_progress(slot_id, num_slots);
		if (status != PSA_SUCCESS) {
			return status;
		}
		/* Verify will clear related key slots. */
		return verify_provisioning_state();
	}

	return PSA_ERROR_DOES_NOT_EXIST;
}

psa_status_t convert_to_psa_attributes(kmu_metadata *metadata, psa_key_attributes_t *key_attr)
{
	psa_key_persistence_t key_persistence;

	if (metadata->metadata_version != 0) {
		return PSA_ERROR_BAD_STATE;
	}

	switch (metadata->rpolicy) {
	case LIB_KMU_REV_POLICY_ROTATING:
		key_persistence = PSA_KEY_PERSISTENCE_DEFAULT;
		break;
	case LIB_KMU_REV_POLICY_REVOKED:
		key_persistence = CRACEN_KEY_PERSISTENCE_REVOKABLE;
		break;
	case LIB_KMU_REV_POLICY_LOCKED:
		key_persistence = PSA_KEY_PERSISTENCE_READ_ONLY;
		break;
	default:
		return PSA_ERROR_STORAGE_FAILURE;
	}

	psa_set_key_lifetime(key_attr, PSA_KEY_LIFETIME_FROM_PERSISTENCE_AND_LOCATION(
					       key_persistence, PSA_KEY_LOCATION_CRACEN_KMU));

	if (metadata->key_usage_scheme == KMU_METADATA_SCHEME_SEED) {
		psa_set_key_type(key_attr, PSA_KEY_TYPE_RAW_DATA);
		psa_set_key_bits(key_attr, 384);
		return PSA_SUCCESS;
	}

	psa_key_usage_t usage_flags = 0;

	uint32_t metadata_usage_flags = metadata->usage_flags;

	for (size_t i = 0; metadata_usage_flags; i++) {
		if (metadata_usage_flags & 1) {
			if (i >= ARRAY_SIZE(metadata_usage_flags_mapping)) {
				return PSA_ERROR_GENERIC_ERROR;
			}
			usage_flags |= metadata_usage_flags_mapping[i];
		}

		metadata_usage_flags >>= 1;
	}

	psa_set_key_usage_flags(key_attr, usage_flags);

	switch (metadata->algorithm) {
	case METADATA_ALG_CHACHA20:
		psa_set_key_type(key_attr, PSA_KEY_TYPE_CHACHA20);
		psa_set_key_algorithm(key_attr, PSA_ALG_STREAM_CIPHER);
		break;
	case METADATA_ALG_CHACHA20_POLY1305:
		psa_set_key_type(key_attr, PSA_KEY_TYPE_CHACHA20);
		psa_set_key_algorithm(key_attr, PSA_ALG_CHACHA20_POLY1305);
		break;
	case METADATA_ALG_AES_GCM:
		psa_set_key_type(key_attr, PSA_KEY_TYPE_AES);
		psa_set_key_algorithm(key_attr, PSA_ALG_GCM);
		break;
	case METADATA_ALG_AES_CCM:
		psa_set_key_type(key_attr, PSA_KEY_TYPE_AES);
		psa_set_key_algorithm(key_attr, PSA_ALG_CCM);
		break;
	case METADATA_ALG_AES_ECB:
		psa_set_key_type(key_attr, PSA_KEY_TYPE_AES);
		psa_set_key_algorithm(key_attr, PSA_ALG_ECB_NO_PADDING);
		break;
	case METADATA_ALG_AES_CTR:
		psa_set_key_type(key_attr, PSA_KEY_TYPE_AES);
		psa_set_key_algorithm(key_attr, PSA_ALG_CTR);
		break;
	case METADATA_ALG_AES_CBC:
		psa_set_key_type(key_attr, PSA_KEY_TYPE_AES);
		psa_set_key_algorithm(key_attr, PSA_ALG_CBC_NO_PADDING);
		break;
#ifdef PSA_ALG_SP800_108_COUNTER_CMAC
	case METADATA_ALG_SP800_108_COUNTER_CMAC:
		psa_set_key_type(key_attr, PSA_KEY_TYPE_AES);
		psa_set_key_algorithm(key_attr, PSA_ALG_SP800_108_COUNTER_CMAC);
		break;
#endif
	case METADATA_ALG_CMAC:
		psa_set_key_type(key_attr, PSA_KEY_TYPE_AES);
		psa_set_key_algorithm(key_attr, PSA_ALG_CMAC);
		break;
	case METADATA_ALG_ED25519:
		/* If the key can sign it is assumed it is a private key */
		psa_set_key_type(
			key_attr,
			can_sign(key_attr)
				? PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_TWISTED_EDWARDS)
				: PSA_KEY_TYPE_ECC_PUBLIC_KEY(PSA_ECC_FAMILY_TWISTED_EDWARDS));
		psa_set_key_algorithm(key_attr, PSA_ALG_PURE_EDDSA);
		break;
	case METADATA_ALG_ECDSA:
		psa_set_key_type(key_attr,
				 can_sign(key_attr)
					 ? PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1)
					 : PSA_KEY_TYPE_ECC_PUBLIC_KEY(PSA_ECC_FAMILY_SECP_R1));
		psa_set_key_algorithm(key_attr, PSA_ALG_ECDSA(PSA_ALG_ANY_HASH));
		break;
	default:
		return PSA_ERROR_HARDWARE_FAILURE;
	}

	switch (metadata->size) {
	case METADATA_ALG_KEY_BITS_128:
		psa_set_key_bits(key_attr, 128);
		break;
	case METADATA_ALG_KEY_BITS_192:
		psa_set_key_bits(key_attr, 192);
		break;
	case METADATA_ALG_KEY_BITS_255:
		psa_set_key_bits(key_attr, 255);
		break;
	case METADATA_ALG_KEY_BITS_256:
		psa_set_key_bits(key_attr, 256);
		break;
	}

	switch (metadata->key_usage_scheme) {
	case KMU_METADATA_SCHEME_PROTECTED:
		/* Only AES keys are supported. */
		if (psa_get_key_type(key_attr) != PSA_KEY_TYPE_AES) {
			return PSA_ERROR_CORRUPTION_DETECTED;
		}

		/* It is not possible to provision protected keys with export or copy key usage. */
		if (usage_flags & PSA_KEY_USAGE_EXPORT) {
			return PSA_ERROR_CORRUPTION_DETECTED;
		}
		if (usage_flags & PSA_KEY_USAGE_COPY) {
			return PSA_ERROR_CORRUPTION_DETECTED;
		}
		break;
	case KMU_METADATA_SCHEME_SEED:
		return PSA_ERROR_NOT_PERMITTED;
	}

	return PSA_SUCCESS;
}

psa_status_t convert_from_psa_attributes(const psa_key_attributes_t *key_attr,
					 kmu_metadata *metadata)
{
	memset(metadata, 0, sizeof(*metadata));
	metadata->metadata_version = 0;

	metadata->key_usage_scheme = CRACEN_PSA_GET_KEY_USAGE_SCHEME(
		MBEDTLS_SVC_KEY_ID_GET_KEY_ID(psa_get_key_id(key_attr)));

	switch (metadata->key_usage_scheme) {
	case KMU_METADATA_SCHEME_PROTECTED:
	case KMU_METADATA_SCHEME_SEED:
	case KMU_METADATA_SCHEME_ENCRYPTED:
	case KMU_METADATA_SCHEME_RAW:
		break;
	default:
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	if (metadata->key_usage_scheme == KMU_METADATA_SCHEME_PROTECTED) {
		if (psa_get_key_usage_flags(key_attr) & PSA_KEY_USAGE_EXPORT) {
			return PSA_ERROR_INVALID_ARGUMENT;
		}
		if (psa_get_key_usage_flags(key_attr) & PSA_KEY_USAGE_COPY) {
			return PSA_ERROR_INVALID_ARGUMENT;
		}
	}

	if (metadata->key_usage_scheme == KMU_METADATA_SCHEME_SEED) {
		metadata->rpolicy = LIB_KMU_REV_POLICY_LOCKED;
		metadata->size = METADATA_ALG_KEY_BITS_384_SEED;
		return PSA_SUCCESS;
	}

	switch (psa_get_key_algorithm(key_attr)) {
	case PSA_ALG_STREAM_CIPHER:
		metadata->algorithm = METADATA_ALG_CHACHA20;
		if (psa_get_key_type(key_attr) != PSA_KEY_TYPE_CHACHA20) {
			return PSA_ERROR_NOT_SUPPORTED;
		}
		break;
	case PSA_ALG_CHACHA20_POLY1305:
		metadata->algorithm = METADATA_ALG_CHACHA20_POLY1305;
		if (psa_get_key_type(key_attr) != PSA_KEY_TYPE_CHACHA20) {
			return PSA_ERROR_NOT_SUPPORTED;
		}
		break;
	case PSA_ALG_GCM:
		metadata->algorithm = METADATA_ALG_AES_GCM;
		if (psa_get_key_type(key_attr) != PSA_KEY_TYPE_AES) {
			return PSA_ERROR_NOT_SUPPORTED;
		}
		break;
	case PSA_ALG_CCM:
		metadata->algorithm = METADATA_ALG_AES_CCM;
		if (psa_get_key_type(key_attr) != PSA_KEY_TYPE_AES) {
			return PSA_ERROR_NOT_SUPPORTED;
		}
		break;
	case PSA_ALG_ECB_NO_PADDING:
		metadata->algorithm = METADATA_ALG_AES_ECB;
		if (psa_get_key_type(key_attr) != PSA_KEY_TYPE_AES) {
			return PSA_ERROR_NOT_SUPPORTED;
		}
		break;
	case PSA_ALG_CTR:
		metadata->algorithm = METADATA_ALG_AES_CTR;
		if (psa_get_key_type(key_attr) != PSA_KEY_TYPE_AES) {
			return PSA_ERROR_NOT_SUPPORTED;
		}
		break;
	case PSA_ALG_CBC_NO_PADDING:
		metadata->algorithm = METADATA_ALG_AES_CBC;
		if (psa_get_key_type(key_attr) != PSA_KEY_TYPE_AES) {
			return PSA_ERROR_NOT_SUPPORTED;
		}
		break;
#ifdef PSA_ALG_SP800_108_COUNTER_CMAC
	case PSA_ALG_SP800_108_COUNTER_CMAC:
		metadata->algorithm = METADATA_ALG_SP800_108_COUNTER_CMAC;
		if (psa_get_key_type(key_attr) != PSA_KEY_TYPE_AES) {
			return PSA_ERROR_NOT_SUPPORTED;
		}
		break;
#endif
	case PSA_ALG_CMAC:
		metadata->algorithm = METADATA_ALG_CMAC;
		if (psa_get_key_type(key_attr) != PSA_KEY_TYPE_AES) {
			return PSA_ERROR_NOT_SUPPORTED;
		}
		break;

	case PSA_ALG_PURE_EDDSA:
		if (PSA_KEY_TYPE_ECC_GET_FAMILY(psa_get_key_type(key_attr)) !=
		    PSA_ECC_FAMILY_TWISTED_EDWARDS) {
			return PSA_ERROR_NOT_SUPPORTED;
		}
		/* Don't support private keys that are only used for verify */
		if (!can_sign(key_attr) &&
		    PSA_KEY_TYPE_IS_ECC_KEY_PAIR(psa_get_key_type(key_attr))) {
			return PSA_ERROR_NOT_SUPPORTED;
		}
		metadata->algorithm = METADATA_ALG_ED25519;
		break;

	case PSA_ALG_ECDSA(PSA_ALG_ANY_HASH):
		if (PSA_KEY_TYPE_ECC_GET_FAMILY(psa_get_key_type(key_attr)) !=
		    PSA_ECC_FAMILY_SECP_R1) {
			return PSA_ERROR_NOT_SUPPORTED;
		}

		/* Don't support private keys that are only used for verify */
		if (!can_sign(key_attr) &&
		    PSA_KEY_TYPE_IS_ECC_KEY_PAIR(psa_get_key_type(key_attr))) {
			return PSA_ERROR_NOT_SUPPORTED;
		}
		metadata->algorithm = METADATA_ALG_ECDSA;
		break;
	default:
		return PSA_ERROR_NOT_SUPPORTED;
	}

	psa_key_usage_t resulting_usage = 0;

	for (size_t i = 0; i < ARRAY_SIZE(metadata_usage_flags_mapping); i++) {
		if (psa_get_key_usage_flags(key_attr) & metadata_usage_flags_mapping[i]) {
			metadata->usage_flags |= 1 << i;
			resulting_usage |= metadata_usage_flags_mapping[i];
		}
	}

	if (~resulting_usage & psa_get_key_usage_flags(key_attr)) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	switch (psa_get_key_bits(key_attr)) {
	case 128:
		metadata->size = METADATA_ALG_KEY_BITS_128;
		break;
	case 192:
		metadata->size = METADATA_ALG_KEY_BITS_192;
		break;
	case 255:
		metadata->size = METADATA_ALG_KEY_BITS_255;
		break;
	case 256:
		metadata->size = METADATA_ALG_KEY_BITS_256;
		break;
	default:
		return PSA_ERROR_NOT_SUPPORTED;
	}

	metadata->key_usage_scheme = CRACEN_PSA_GET_KEY_USAGE_SCHEME(
		MBEDTLS_SVC_KEY_ID_GET_KEY_ID(psa_get_key_id(key_attr)));

	if (metadata->key_usage_scheme == KMU_METADATA_SCHEME_PROTECTED) {
		if (psa_get_key_usage_flags(key_attr) & PSA_KEY_USAGE_EXPORT) {
			return PSA_ERROR_INVALID_ARGUMENT;
		}
		if (psa_get_key_usage_flags(key_attr) & PSA_KEY_USAGE_COPY) {
			return PSA_ERROR_INVALID_ARGUMENT;
		}
	}

	switch (PSA_KEY_LIFETIME_GET_PERSISTENCE(psa_get_key_lifetime(key_attr))) {
	case PSA_KEY_PERSISTENCE_READ_ONLY:
		metadata->rpolicy = LIB_KMU_REV_POLICY_LOCKED;
		break;
	case PSA_KEY_PERSISTENCE_DEFAULT:
		metadata->rpolicy = LIB_KMU_REV_POLICY_ROTATING;
		break;
	case CRACEN_KEY_PERSISTENCE_REVOKABLE:
		metadata->rpolicy = LIB_KMU_REV_POLICY_REVOKED;
		break;
	default:
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	return PSA_SUCCESS;
}

psa_status_t cracen_kmu_provision(const psa_key_attributes_t *key_attr, int slot_id,
				  const uint8_t *key_buffer, size_t key_buffer_size)
{
	kmu_metadata metadata;
	uint8_t *push_address;

	/* Provisioning data for encrypted keys:
	 *     - Nonce
	 *     - Key material (first 128 bits)
	 *     - Key material (optional for keys > 128 bits.)
	 *     - Tag
	 */
	uint8_t encrypted_workmem[CRACEN_KMU_SLOT_KEY_SIZE * 4] = {};
	size_t encrypted_outlen = 0;

	psa_status_t status = verify_provisioning_state();

	if (status != PSA_SUCCESS) {
		return status;
	}
	status = convert_from_psa_attributes(key_attr, &metadata);

	if (status) {
		return status;
	}

	switch (metadata.key_usage_scheme) {
	case KMU_METADATA_SCHEME_PROTECTED:
		push_address = (uint8_t *)CRACEN_PROTECTED_RAM_AES_KEY0;
		/* Only 128, 192 and 256 bit keys are supported. */
		if (key_buffer_size != 16 && key_buffer_size != 24 && key_buffer_size != 32) {
			return PSA_ERROR_INVALID_ARGUMENT;
		}
		break;
	case KMU_METADATA_SCHEME_ENCRYPTED:
	case KMU_METADATA_SCHEME_RAW:
		push_address = (uint8_t *)kmu_push_area;
		if (key_buffer_size != 16 && key_buffer_size != 24 && key_buffer_size != 32) {
			return PSA_ERROR_INVALID_ARGUMENT;
		}
		break;
	case KMU_METADATA_SCHEME_SEED:
		push_address = (uint8_t *)NRF_CRACEN->SEED;
		if (key_buffer_size != 16 * 3) {
			return PSA_ERROR_INVALID_ARGUMENT;
		}
		break;
	default:
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	if (metadata.key_usage_scheme == KMU_METADATA_SCHEME_ENCRYPTED) {
		/* Copy key material to workbuffer, zero-pad key and align to key slot size. */
		memcpy(encrypted_workmem + CRACEN_KMU_SLOT_KEY_SIZE, key_buffer, key_buffer_size);
		status = cracen_kmu_encrypt(encrypted_workmem + CRACEN_KMU_SLOT_KEY_SIZE,
					    ROUND_UP(key_buffer_size, CRACEN_KMU_SLOT_KEY_SIZE),
					    &metadata, encrypted_workmem, sizeof(encrypted_workmem),
					    &encrypted_outlen);

		if (status != PSA_SUCCESS) {
			return status;
		}
		key_buffer = encrypted_workmem;
		key_buffer_size = encrypted_outlen;
	}

	/* Verify that required slots are empty */
	size_t num_slots = (MAX(encrypted_outlen, key_buffer_size) + CRACEN_KMU_SLOT_KEY_SIZE - 1) /
			   CRACEN_KMU_SLOT_KEY_SIZE;

	for (size_t i = 0; i < num_slots; i++) {
		if (!lib_kmu_is_slot_empty(slot_id + i)) {
			status = PSA_ERROR_ALREADY_EXISTS;
			goto exit;
		}
	}

	if (num_slots > 1) {
		status = set_provisioning_in_progress(slot_id, num_slots);
		if (status != PSA_SUCCESS) {
			return status;
		}
	}

	struct kmu_src_t kmu_desc = {};

	for (size_t i = 0; i < num_slots; i++) {
		kmu_desc.dest = (uint64_t *)(push_address + (CRACEN_KMU_SLOT_KEY_SIZE * i));
		kmu_desc.metadata = UINT32_MAX;
		if (i == 0) {
			memcpy(&kmu_desc.metadata, &metadata, sizeof(metadata));
		}
		kmu_desc.rpolicy = metadata.rpolicy;
		memcpy(kmu_desc.value, key_buffer + CRACEN_KMU_SLOT_KEY_SIZE * i,
		       CRACEN_KMU_SLOT_KEY_SIZE);

		int st = lib_kmu_provision_slot(slot_id + i, &kmu_desc);

		if (st) {
			/* We've already verified that this slot empty, so it should not fail. */
			status = PSA_ERROR_HARDWARE_FAILURE;

			(void)verify_provisioning_state();
			break;
		}

		safe_memzero(&kmu_desc, sizeof(kmu_desc));
	}

	if (num_slots > 1 && status == PSA_SUCCESS) {
		status = end_provisioning(slot_id, num_slots);
	}

exit:
	return status;
}

psa_status_t cracen_kmu_get_key_slot(mbedtls_svc_key_id_t key_id, psa_key_lifetime_t *lifetime,
				     psa_drv_slot_number_t *slot_number)
{
	kmu_metadata metadata;

	unsigned int slot_id = CRACEN_PSA_GET_KMU_SLOT(MBEDTLS_SVC_KEY_ID_GET_KEY_ID(key_id));

	int kmu_status = lib_kmu_read_metadata((int)slot_id, (uint32_t *)&metadata);

	if (kmu_status == -LIB_KMU_REVOKED) {
		return PSA_ERROR_NOT_PERMITTED;
	}

	if (kmu_status == -LIB_KMU_ERROR || is_secondary_slot(&metadata)) {
		return PSA_ERROR_DOES_NOT_EXIST;
	}

	psa_key_persistence_t read_only = metadata.rpolicy == LIB_KMU_REV_POLICY_ROTATING
						  ? PSA_KEY_PERSISTENCE_DEFAULT
						  : PSA_KEY_PERSISTENCE_READ_ONLY;

	*lifetime = PSA_KEY_LIFETIME_FROM_PERSISTENCE_AND_LOCATION(read_only,
								   PSA_KEY_LOCATION_CRACEN_KMU);
	*slot_number = slot_id;

	return PSA_SUCCESS;
}

static psa_status_t push_kmu_key_to_ram(uint8_t *key_buffer, size_t key_buffer_size)
{
	if (key_buffer_size > sizeof(kmu_push_area)) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	psa_status_t status;

	/* The kmu_push_area is guarded by the symmetric mutex since it is the most common use case.
	 * Here the decision was to avoid defining another mutex to handle the push buffer for the
	 * rest of the use cases.
	 */
	nrf_security_mutex_lock(&cracen_mutex_symmetric);
	status = silex_statuscodes_to_psa(cracen_kmu_prepare_key(key_buffer));
	if (status == PSA_SUCCESS) {
		memcpy(key_buffer, kmu_push_area, key_buffer_size);
		safe_memzero(kmu_push_area, sizeof(kmu_push_area));
	}
	nrf_security_mutex_unlock(&cracen_mutex_symmetric);

	return status;
}

psa_status_t cracen_kmu_get_builtin_key(psa_drv_slot_number_t slot_number,
					psa_key_attributes_t *attributes, uint8_t *key_buffer,
					size_t key_buffer_size, size_t *key_buffer_length)
{
	kmu_metadata metadata;
	int kmu_status = lib_kmu_read_metadata((int)slot_number, (uint32_t *)&metadata);

	if (kmu_status == -LIB_KMU_REVOKED) {
		return PSA_ERROR_NOT_PERMITTED;
	}

	if (kmu_status == -LIB_KMU_ERROR || is_secondary_slot(&metadata)) {
		return PSA_ERROR_DOES_NOT_EXIST;
	}

	psa_status_t status = verify_provisioning_state();

	if (status != PSA_SUCCESS) {
		return status;
	}

	if (!attributes) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	status = convert_to_psa_attributes(&metadata, attributes);

	if (status != PSA_SUCCESS) {
		return status;
	}

	if (key_buffer_size >= cracen_get_opaque_size(attributes)) {
		*key_buffer_length = cracen_get_opaque_size(attributes);
		kmu_opaque_key_buffer *key = (kmu_opaque_key_buffer *)key_buffer;

		key->key_usage_scheme = metadata.key_usage_scheme;
		switch (metadata.size) {
		case METADATA_ALG_KEY_BITS_128:
			key->number_of_slots = 1;
			break;
		case METADATA_ALG_KEY_BITS_192:
		case METADATA_ALG_KEY_BITS_255:
		case METADATA_ALG_KEY_BITS_256:
			if (psa_get_key_type(attributes) ==
			    PSA_KEY_TYPE_ECC_PUBLIC_KEY(PSA_ECC_FAMILY_SECP_R1)) {
				key->number_of_slots = 4;
			} else {
				key->number_of_slots = 2;
			}
			break;
		case METADATA_ALG_KEY_BITS_384_SEED:
			key->number_of_slots = 3;
			break;
		default:
			return PSA_ERROR_DATA_INVALID;
		}

		if (key->key_usage_scheme == KMU_METADATA_SCHEME_ENCRYPTED) {
			key->number_of_slots += 2;
		}

		key->slot_id = slot_number;
	} else {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	/* ECC keys are getting loading into the key buffer like volatile keys */
	if (PSA_KEY_TYPE_IS_ECC(psa_get_key_type(attributes))) {
		if (psa_get_key_type(attributes) ==
		    PSA_KEY_TYPE_ECC_PUBLIC_KEY(PSA_ECC_FAMILY_SECP_R1)) {
			*key_buffer = SI_ECC_PUBKEY_UNCOMPRESSED;
			key_buffer++;
			key_buffer_size--;
		}
		return push_kmu_key_to_ram(key_buffer, key_buffer_size);
	}

	return PSA_SUCCESS;
}
