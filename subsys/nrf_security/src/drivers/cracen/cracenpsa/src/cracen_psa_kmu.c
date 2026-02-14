/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <nrfx.h>
#include <cracen/mem_helpers.h>
#include <cracen/statuscodes.h>
#include <cracen/lib_kmu.h>
#include <nrf_security_mutexes.h>
#include <psa/crypto.h>
#include <stdint.h>
#include <string.h>
#include <sxsymcrypt/internal.h>
#include <hw_unique_key.h>
#include <cracen_psa.h>
#include <cracen_psa_ctr_drbg.h>
#include <cracen/common.h>
#include <internal/ecc/cracen_ecc_helpers.h>
#include <cracen/cracen_kmu.h>

/* The size of the key CBR (Compact Binary Respresentation), bytes */
#define CRACEN_KMU_CBR_SIZE		     1u
/* Public key sizes of ECC curves (secpXXXr1), bytes */
#define CRACEN_KMU_SECP_R1_256_PUB_KEY_SIZE  (CRACEN_KMU_CBR_SIZE + 64u)
#define CRACEN_KMU_SECP_R1_384_PUB_KEY_SIZE  (CRACEN_KMU_CBR_SIZE + 96u)

#define CRACEN_KMU_128_BIT_KEY_SIZE	     16u
#define CRACEN_KMU_192_BIT_KEY_SIZE	     24u
#define CRACEN_KMU_256_BIT_KEY_SIZE	     32u
#define CRACEN_KMU_384_BIT_KEY_SIZE	     48u

/* Reserved slot, used to record whether provisioning is in progress for a set of slots.
 * We only use the metadata field, formatted as follows:
 *      Bits 31-16: Unused
 *      Bits 15-8:  slot-id
 *      Bits 7-0:   number of slots
 */
#define PROVISIONING_SLOT 186

#define SECONDARY_SLOT_METADATA_VALUE UINT32_MAX

extern nrf_security_mutex_t cracen_mutex_symmetric;

#if DT_NODE_EXISTS(DT_NODELABEL(nrf_kmu_reserved_push_area))

#include <zephyr/dt-bindings/memory-attr/memory-attr.h>
#include <zephyr/linker/devicetree_regions.h>
#define KMU_PUSH_AREA_NODE DT_NODELABEL(nrf_kmu_reserved_push_area)
uint8_t kmu_push_area[CRACEN_KMU_PUSH_AREA_SIZE] Z_GENERIC_SECTION(
	LINKER_DT_NODE_REGION_NAME(KMU_PUSH_AREA_NODE));

#else

/* The section .nrf_kmu_reserved_push_area is placed at the top RAM address
 * by the linker scripts. We do that for both the secure and non-secure builds.
 * Since this buffer is placed on the top of RAM we don't need to have the alignment
 * attribute anymore.
 */
uint8_t
kmu_push_area[CRACEN_KMU_PUSH_AREA_SIZE] __attribute__((section(".nrf_kmu_reserved_push_area")));
#endif

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
	METADATA_ALG_ED25519PH = 12,
	METADATA_ALG_HMAC = 13,
	METADATA_ALG_ECDH = 14,
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

#ifdef PSA_NEED_CRACEN_KMU_ENCRYPTED_KEYS

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
	psa_status_t psa_status;
	psa_key_attributes_t mkek_attr = PSA_KEY_ATTRIBUTES_INIT;

	psa_set_key_type(&mkek_attr, PSA_KEY_TYPE_AES);
	psa_set_key_id(&mkek_attr, mbedtls_svc_key_id_make(0, CRACEN_BUILTIN_MKEK_ID));
	psa_set_key_lifetime(&mkek_attr,
			     PSA_KEY_LIFETIME_FROM_PERSISTENCE_AND_LOCATION(
				     CRACEN_KEY_PERSISTENCE_READ_ONLY, PSA_KEY_LOCATION_CRACEN));

	cracen_key_derivation_operation_t op = {};

	psa_status = cracen_key_derivation_setup(&op, PSA_ALG_SP800_108_COUNTER_CMAC);
	if (psa_status != PSA_SUCCESS) {
		return psa_status;
	}
	psa_status = cracen_key_derivation_input_key(&op, PSA_KEY_DERIVATION_INPUT_SECRET,
						     &mkek_attr, NULL, 0);
	if (psa_status != PSA_SUCCESS) {
		return psa_status;
	}
	psa_status =
		cracen_key_derivation_input_bytes(&op, PSA_KEY_DERIVATION_INPUT_LABEL, "KMU", 3);
	if (psa_status != PSA_SUCCESS) {
		return psa_status;
	}
	psa_status = cracen_key_derivation_input_bytes(&op, PSA_KEY_DERIVATION_INPUT_CONTEXT,
						       context, CRACEN_KMU_SLOT_KEY_SIZE);
	if (psa_status != PSA_SUCCESS) {
		return psa_status;
	}
	psa_status = cracen_key_derivation_output_bytes(&op, key, 32);
	return psa_status;
}

static psa_status_t cracen_kmu_encrypt(const uint8_t *key, size_t key_length,
				       kmu_metadata *metadata, uint8_t *encrypted_buffer,
				       size_t encrypted_buffer_size,
				       size_t *encrypted_buffer_length)
{
	uint8_t key_buffer[32] = {};
	psa_status_t psa_status;
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;

	psa_set_key_algorithm(&attr, PSA_ALG_GCM);
	psa_set_key_type(&attr, PSA_KEY_TYPE_AES);
	psa_set_key_bits(&attr, 256);
	psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_ENCRYPT);

	if (encrypted_buffer_size > CRACEN_KMU_SLOT_KEY_SIZE) {
		psa_status = cracen_get_random(NULL, encrypted_buffer, CRACEN_KMU_SLOT_KEY_SIZE);
		if (psa_status != PSA_SUCCESS) {
			return psa_status;
		}
	} else {
		return PSA_ERROR_GENERIC_ERROR;
	}

	uint8_t *nonce = encrypted_buffer;

	encrypted_buffer += CRACEN_KMU_SLOT_KEY_SIZE;
	encrypted_buffer_size -= CRACEN_KMU_SLOT_KEY_SIZE;

	psa_status = get_encryption_key(nonce, key_buffer);
	if (psa_status != PSA_SUCCESS) {
		return psa_status;
	}

	psa_status = cracen_aead_encrypt(&attr, key_buffer, sizeof(key_buffer), PSA_ALG_GCM, nonce,
					 12, (const uint8_t *)metadata, sizeof(*metadata), key,
					 key_length, encrypted_buffer, encrypted_buffer_size,
					 encrypted_buffer_length);

	if (psa_status == PSA_SUCCESS) {
		*encrypted_buffer_length += CRACEN_KMU_SLOT_KEY_SIZE;
	}
	return psa_status;
}

static psa_status_t cracen_kmu_decrypt(kmu_metadata *metadata, size_t number_of_slots)
{
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;

	psa_set_key_algorithm(&attr, PSA_ALG_GCM);
	psa_set_key_type(&attr, PSA_KEY_TYPE_AES);
	psa_set_key_bits(&attr, 256);
	psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_DECRYPT);

	uint8_t key_buffer[32] = {50};
	psa_status_t psa_status = get_encryption_key(kmu_push_area, key_buffer);

	if (psa_status != PSA_SUCCESS) {
		return psa_status;
	}

	size_t outlen = 0;

	return cracen_aead_decrypt(&attr, key_buffer, sizeof(key_buffer), PSA_ALG_GCM,
				   kmu_push_area, 12, (const uint8_t *)metadata, sizeof(*metadata),
				   kmu_push_area + CRACEN_KMU_SLOT_KEY_SIZE,
				   (number_of_slots - 1) * CRACEN_KMU_SLOT_KEY_SIZE, kmu_push_area,
				   sizeof(kmu_push_area), &outlen);
}

#endif /* PSA_NEED_CRACEN_KMU_ENCRYPTED_KEYS */

#if defined(CONFIG_CRACEN_PROVISION_PROT_RAM_INV_SLOTS_ON_INIT) ||                                 \
	defined(CONFIG_CRACEN_PROVISION_PROT_RAM_INV_SLOTS_WITH_IMPORT)
psa_status_t cracen_provision_prot_ram_inv_slots(void)
{
	uint8_t rng_buffer[2 * CRACEN_KMU_SLOT_KEY_SIZE];
	bool needs_provisioning;
	psa_key_attributes_t key_attributes = PSA_KEY_ATTRIBUTES_INIT;
	psa_status_t psa_status;
	int kmu_slot;

	needs_provisioning = lib_kmu_is_slot_empty(PROTECTED_RAM_INVALIDATION_DATA_SLOT1) ||
			     lib_kmu_is_slot_empty(PROTECTED_RAM_INVALIDATION_DATA_SLOT2);

	if (!needs_provisioning) {
		return PSA_SUCCESS;
	}

	psa_status = cracen_get_random(NULL, rng_buffer, sizeof(rng_buffer));
	if (psa_status != PSA_SUCCESS) {
		return psa_status;
	}

	/* Just use attributes which are suitable for a normal protected RAM
	 * key so that the existed KMU provision API can be used.
	 */
	psa_set_key_usage_flags(&key_attributes, PSA_KEY_USAGE_ENCRYPT);
	psa_set_key_algorithm(&key_attributes, PSA_ALG_CTR);
	psa_set_key_type(&key_attributes, PSA_KEY_TYPE_AES);
	psa_set_key_bits(&key_attributes, 256);
	psa_set_key_lifetime(&key_attributes,
			     PSA_KEY_LIFETIME_FROM_PERSISTENCE_AND_LOCATION(
				     PSA_KEY_PERSISTENCE_DEFAULT, PSA_KEY_LOCATION_CRACEN_KMU));

	/* The rng material here is 256 bits so it will occupy 2 KMU slots during
	 * the provisioning call.
	 */
	psa_set_key_id(&key_attributes,
		       mbedtls_svc_key_id_make(0, PSA_KEY_HANDLE_FROM_CRACEN_KMU_SLOT(
							  CRACEN_KMU_KEY_USAGE_SCHEME_PROTECTED,
							  PROTECTED_RAM_INVALIDATION_DATA_SLOT1)));

	kmu_slot = CRACEN_PSA_GET_KMU_SLOT(
		MBEDTLS_SVC_KEY_ID_GET_KEY_ID(psa_get_key_id(&key_attributes)));

	psa_status =
		cracen_kmu_provision(&key_attributes, kmu_slot, rng_buffer, sizeof(rng_buffer));
	safe_memzero(rng_buffer, sizeof(rng_buffer));
	return psa_status;
}
#endif /* CONFIG_CRACEN_PROVISION_PROT_RAM_INV_SLOTS_ON_INIT ||
	* CONFIG_CRACEN_PROVISION_PROT_RAM_INV_SLOTS_WITH_IMPORT
	*/
/* Used internally in sxsymcrypt so we use sx return codes here. */
int cracen_kmu_prepare_key(const uint8_t *user_data)
{
	const kmu_opaque_key_buffer *key = (const kmu_opaque_key_buffer *)user_data;

	switch (key->key_usage_scheme) {
	case CRACEN_KMU_KEY_USAGE_SCHEME_RAW:
	case CRACEN_KMU_KEY_USAGE_SCHEME_PROTECTED:
		for (size_t i = 0; i < key->number_of_slots; i++) {
			if (lib_kmu_push_slot(key->slot_id + i) != 0) {
				return SX_ERR_UNKNOWN_ERROR;
			}
		}
		return SX_OK;
#ifdef PSA_NEED_CRACEN_KMU_ENCRYPTED_KEYS
	case CRACEN_KMU_KEY_USAGE_SCHEME_ENCRYPTED: {
		kmu_metadata metadata;

		if (lib_kmu_read_metadata((int)key->slot_id, (uint32_t *)&metadata) != 0) {
			return SX_ERR_INVALID_KEYREF;
		}
		for (size_t i = 0; i < key->number_of_slots; i++) {
			if (lib_kmu_push_slot(key->slot_id + i) != 0) {
				return SX_ERR_UNKNOWN_ERROR;
			}
		}

		psa_status_t psa_status = cracen_kmu_decrypt(&metadata, key->number_of_slots);

		if (psa_status != PSA_SUCCESS) {
			return SX_ERR_PLATFORM_ERROR;
		}

		return SX_OK;
	}
#endif /* PSA_NEED_CRACEN_KMU_ENCRYPTED_KEYS */
	default:
		return SX_ERR_INVALID_KEYREF;
	}

	return SX_OK;
}

psa_status_t cracen_push_prot_ram_inv_slots(void)
{
	bool any_slot_empty;

	any_slot_empty = lib_kmu_is_slot_empty(PROTECTED_RAM_INVALIDATION_DATA_SLOT1) ||
			 lib_kmu_is_slot_empty(PROTECTED_RAM_INVALIDATION_DATA_SLOT2);
	if (any_slot_empty) {
		return PSA_ERROR_HARDWARE_FAILURE;
	}

	if (lib_kmu_push_slot(PROTECTED_RAM_INVALIDATION_DATA_SLOT1) != LIB_KMU_SUCCESS) {
		return PSA_ERROR_HARDWARE_FAILURE;
	}

	if (lib_kmu_push_slot(PROTECTED_RAM_INVALIDATION_DATA_SLOT2) != LIB_KMU_SUCCESS) {
		return PSA_ERROR_HARDWARE_FAILURE;
	}

	return PSA_SUCCESS;
}

int cracen_kmu_clean_key(const uint8_t *user_data)
{
	const kmu_opaque_key_buffer *key = (const kmu_opaque_key_buffer *)user_data;

	switch (key->key_usage_scheme) {
	case CRACEN_KMU_KEY_USAGE_SCHEME_PROTECTED:
		if (cracen_push_prot_ram_inv_slots() != PSA_SUCCESS) {
			return SX_ERR_UNKNOWN_ERROR;
		}

		break;
	default:
		break;
	}

	safe_memzero(kmu_push_area, sizeof(kmu_push_area));

	return SX_OK;
}

/**
 * @brief Checks whether this is the secondary slot. If true, the metadata resides in
 *        one of the previous slots;
 */
static bool is_secondary_slot(kmu_metadata *metadata)
{
	const uint32_t value = SECONDARY_SLOT_METADATA_VALUE;

	_Static_assert(sizeof(value) == sizeof(*metadata), "");
	return memcmp(&value, metadata, sizeof(value)) == 0;
}

static psa_status_t read_primary_slot_metadata(unsigned int slot_id, kmu_metadata *metadata)
{
	const int kmu_status = lib_kmu_read_metadata(slot_id, (uint32_t *)metadata);

	if (kmu_status == -LIB_KMU_REVOKED) {
		return PSA_ERROR_NOT_PERMITTED;
	}
	if (kmu_status == -LIB_KMU_ERROR || is_secondary_slot(metadata)) {
		return PSA_ERROR_DOES_NOT_EXIST;
	}
	return PSA_SUCCESS;
}

static psa_status_t get_kmu_slot_id_and_metadata(mbedtls_svc_key_id_t key_id,
						 unsigned int *slot_id, kmu_metadata *metadata)
{
	*slot_id = CRACEN_PSA_GET_KMU_SLOT(MBEDTLS_SVC_KEY_ID_GET_KEY_ID(key_id));
	return read_primary_slot_metadata(*slot_id, metadata);
}

#if	defined(PSA_NEED_CRACEN_PURE_EDDSA_TWISTED_EDWARDS) || \
	defined(PSA_NEED_CRACEN_ED25519PH)		    || \
	defined(PSA_NEED_CRACEN_ECDSA)			    || \
	defined(PSA_NEED_CRACEN_HMAC)
static bool can_sign(const psa_key_attributes_t *key_attr)
{
	return (psa_get_key_usage_flags(key_attr) & PSA_KEY_USAGE_SIGN_MESSAGE) ||
	       (psa_get_key_usage_flags(key_attr) & PSA_KEY_USAGE_SIGN_HASH);
}
#endif

#if defined(PSA_NEED_CRACEN_ECDH)
static bool can_derive(const psa_key_attributes_t *key_attr)
{
	return psa_get_key_usage_flags(key_attr) & PSA_KEY_USAGE_DERIVE;
}
#endif /* PSA_NEED_CRACEN_ECDH */

/**
 * @brief Check provisioning state, and delete slots that were not completely provisioned.
 *
 * @return psa_status_t
 */
static psa_status_t clean_up_unfinished_provisioning(void)
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
static psa_status_t set_provisioning_in_progress(uint32_t slot_id, uint32_t num_slots)
{
	struct kmu_src kmu_desc = {};

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
static psa_status_t end_provisioning(uint32_t slot_id, uint32_t num_slots)
{
	if (lib_kmu_revoke_slot(PROVISIONING_SLOT) != LIB_KMU_SUCCESS) {
		return PSA_ERROR_HARDWARE_FAILURE;
	}
	return PSA_SUCCESS;
}

static psa_status_t get_kmu_slot_count(kmu_metadata metadata, const psa_key_attributes_t *key_attr,
				       unsigned int *slot_count)
{
	switch (metadata.size) {
	case METADATA_ALG_KEY_BITS_128:
		*slot_count = 1;
		break;
	case METADATA_ALG_KEY_BITS_192:
	case METADATA_ALG_KEY_BITS_255:
	case METADATA_ALG_KEY_BITS_256:
		if (psa_get_key_type(key_attr) ==
			PSA_KEY_TYPE_ECC_PUBLIC_KEY(PSA_ECC_FAMILY_SECP_R1)) {
			*slot_count = 4;
		} else {
			*slot_count = 2;
		}
		break;
	case METADATA_ALG_KEY_BITS_384_SEED:
		*slot_count = 3;
		break;
	case METADATA_ALG_KEY_BITS_384:
		if (psa_get_key_type(key_attr) ==
			PSA_KEY_TYPE_ECC_PUBLIC_KEY(PSA_ECC_FAMILY_SECP_R1)) {
			*slot_count = 6;
		} else {
			*slot_count = 3;
		}
		break;
	default:
		return PSA_ERROR_DATA_INVALID;
	}

	if (metadata.key_usage_scheme == CRACEN_KMU_KEY_USAGE_SCHEME_ENCRYPTED) {
		*slot_count += 2; /* nonce + tag */
	}

	return PSA_SUCCESS;
}

static psa_status_t get_kmu_slot_id_and_count(const psa_key_attributes_t *key_attr,
					      unsigned int *slot_id, unsigned int *slot_count)
{
	kmu_metadata metadata;
	psa_status_t psa_status;

	psa_status = get_kmu_slot_id_and_metadata(psa_get_key_id(key_attr), slot_id, &metadata);
	if (psa_status != PSA_SUCCESS) {
		return psa_status;
	}

	return get_kmu_slot_count(metadata, key_attr, slot_count);
}

psa_status_t cracen_kmu_destroy_key(const psa_key_attributes_t *attributes)
{
	psa_key_lifetime_t lifetime = psa_get_key_lifetime(attributes);

	psa_key_location_t location = PSA_KEY_LIFETIME_GET_LOCATION(lifetime);
	psa_key_persistence_t persistence = PSA_KEY_LIFETIME_GET_PERSISTENCE(lifetime);

	if (location == PSA_KEY_LOCATION_CRACEN_KMU) {
		psa_status_t psa_status;
		unsigned int slot_id, slot_count;

		if (persistence == CRACEN_KEY_PERSISTENCE_READ_ONLY) {
			return PSA_ERROR_NOT_PERMITTED;
		}

		psa_status = get_kmu_slot_id_and_count(attributes, &slot_id, &slot_count);
		if (psa_status != PSA_SUCCESS) {
			return psa_status;
		}

		/* If the slot we attempt to destroy is blocked we will get a hardware failure, and
		 * there is no way in hardware to distingush between an actual failure and the slot
		 * being blocked. Therefore we attempt to push the key here to verify if the key is
		 * blocked or not.
		 */
		for (size_t i = 0; i < slot_count; i++) {
			if (lib_kmu_push_slot(slot_id + i) != 0) {
				return PSA_ERROR_NOT_PERMITTED;
			}
		}

		/* Clean the key data from the push area and protected ram to ensure it's not
		 * exposed. We use the protected scheme since the key type is not known at
		 * this point and that clears both.
		 */
		kmu_opaque_key_buffer temp_key_buffer = {
			.key_usage_scheme = CRACEN_KMU_KEY_USAGE_SCHEME_PROTECTED,
			.number_of_slots = slot_count,
			.slot_id = slot_id};
		cracen_kmu_clean_key((const uint8_t *)&temp_key_buffer);

		psa_status = set_provisioning_in_progress(slot_id, slot_count);
		if (psa_status != PSA_SUCCESS) {
			return psa_status;
		}
		/* This will revoke the related key slots. */
		return clean_up_unfinished_provisioning();
	}

	return PSA_ERROR_DOES_NOT_EXIST;
}

static psa_status_t convert_to_psa_attributes(kmu_metadata *metadata,
					      psa_key_attributes_t *key_attr)
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
		key_persistence = CRACEN_KEY_PERSISTENCE_READ_ONLY;
		break;
	default:
		return PSA_ERROR_STORAGE_FAILURE;
	}

	psa_set_key_lifetime(key_attr, PSA_KEY_LIFETIME_FROM_PERSISTENCE_AND_LOCATION(
					       key_persistence, PSA_KEY_LOCATION_CRACEN_KMU));

	if (metadata->key_usage_scheme == CRACEN_KMU_KEY_USAGE_SCHEME_SEED) {
		psa_set_key_type(key_attr, PSA_KEY_TYPE_RAW_DATA);
		psa_set_key_bits(key_attr, PSA_BYTES_TO_BITS(HUK_SIZE_BYTES));
		return PSA_SUCCESS;
	}

	psa_key_usage_t usage_flags = 0;

	uint32_t metadata_usage_flags = metadata->usage_flags;
	size_t i = 0;

	while (metadata_usage_flags) {
		if (metadata_usage_flags & 1) {
			if (i >= ARRAY_SIZE(metadata_usage_flags_mapping)) {
				return PSA_ERROR_GENERIC_ERROR;
			}
			usage_flags |= metadata_usage_flags_mapping[i];
		}

		metadata_usage_flags >>= 1;
		i++;
	}

	psa_set_key_usage_flags(key_attr, usage_flags);

	switch (metadata->algorithm) {
#ifdef PSA_NEED_CRACEN_STREAM_CIPHER_CHACHA20
	case METADATA_ALG_CHACHA20:
		psa_set_key_type(key_attr, PSA_KEY_TYPE_CHACHA20);
		psa_set_key_algorithm(key_attr, PSA_ALG_STREAM_CIPHER);
		break;
#endif /* PSA_NEED_CRACEN_STREAM_CIPHER_CHACHA20 */
#ifdef PSA_NEED_CRACEN_CHACHA20_POLY1305
	case METADATA_ALG_CHACHA20_POLY1305:
		psa_set_key_type(key_attr, PSA_KEY_TYPE_CHACHA20);
		psa_set_key_algorithm(key_attr, PSA_ALG_CHACHA20_POLY1305);
		break;
#endif /* PSA_NEED_CRACEN_CHACHA20_POLY1305 */
#ifdef PSA_NEED_CRACEN_GCM_AES
	case METADATA_ALG_AES_GCM:
		psa_set_key_type(key_attr, PSA_KEY_TYPE_AES);
		psa_set_key_algorithm(key_attr, PSA_ALG_GCM);
		break;
#endif /* PSA_NEED_CRACEN_GCM_AES */
#ifdef PSA_NEED_CRACEN_CCM_AES
	case METADATA_ALG_AES_CCM:
		psa_set_key_type(key_attr, PSA_KEY_TYPE_AES);
		psa_set_key_algorithm(key_attr, PSA_ALG_CCM);
		break;
#endif /* PSA_NEED_CRACEN_CCM_AES */
#ifdef PSA_NEED_CRACEN_ECB_NO_PADDING_AES
	case METADATA_ALG_AES_ECB:
		psa_set_key_type(key_attr, PSA_KEY_TYPE_AES);
		psa_set_key_algorithm(key_attr, PSA_ALG_ECB_NO_PADDING);
		break;
#endif /* PSA_NEED_CRACEN_ECB_NO_PADDING_AES */
#ifdef PSA_NEED_CRACEN_CTR_AES
	case METADATA_ALG_AES_CTR:
		psa_set_key_type(key_attr, PSA_KEY_TYPE_AES);
		psa_set_key_algorithm(key_attr, PSA_ALG_CTR);
		break;
#endif /* PSA_NEED_CRACEN_CTR_AES */
#ifdef PSA_NEED_CRACEN_CBC_NO_PADDING_AES
	case METADATA_ALG_AES_CBC:
		psa_set_key_type(key_attr, PSA_KEY_TYPE_AES);
		psa_set_key_algorithm(key_attr, PSA_ALG_CBC_NO_PADDING);
		break;
#endif /* PSA_NEED_CRACEN_CBC_NO_PADDING_AES */
#ifdef PSA_NEED_CRACEN_SP800_108_COUNTER_CMAC
	case METADATA_ALG_SP800_108_COUNTER_CMAC:
		psa_set_key_type(key_attr, PSA_KEY_TYPE_AES);
		psa_set_key_algorithm(key_attr, PSA_ALG_SP800_108_COUNTER_CMAC);
		break;
#endif /* PSA_NEED_CRACEN_SP800_108_COUNTER_CMAC */
#ifdef PSA_NEED_CRACEN_CMAC
	case METADATA_ALG_CMAC:
		psa_set_key_type(key_attr, PSA_KEY_TYPE_AES);
		psa_set_key_algorithm(key_attr, PSA_ALG_CMAC);
		break;
#endif /* PSA_NEED_CRACEN_CMAC */
#ifdef PSA_NEED_CRACEN_PURE_EDDSA_TWISTED_EDWARDS
	case METADATA_ALG_ED25519:
		/* If the key can sign it is assumed it is a private key */
		psa_set_key_type(
			key_attr,
			can_sign(key_attr)
				? PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_TWISTED_EDWARDS)
				: PSA_KEY_TYPE_ECC_PUBLIC_KEY(PSA_ECC_FAMILY_TWISTED_EDWARDS));
		psa_set_key_algorithm(key_attr, PSA_ALG_PURE_EDDSA);
		break;
#endif /* PSA_NEED_CRACEN_PURE_EDDSA_TWISTED_EDWARDS */
#ifdef PSA_NEED_CRACEN_ED25519PH
	case METADATA_ALG_ED25519PH:
		/* If the key can sign it is assumed it is a private key */
		psa_set_key_type(
			key_attr,
			can_sign(key_attr)
				? PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_TWISTED_EDWARDS)
				: PSA_KEY_TYPE_ECC_PUBLIC_KEY(PSA_ECC_FAMILY_TWISTED_EDWARDS));
		psa_set_key_algorithm(key_attr, PSA_ALG_ED25519PH);
		break;
#endif /* PSA_NEED_CRACEN_ED25519PH */
#ifdef PSA_NEED_CRACEN_ECDSA
	case METADATA_ALG_ECDSA:
		psa_set_key_type(key_attr,
				 can_sign(key_attr)
					 ? PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1)
					 : PSA_KEY_TYPE_ECC_PUBLIC_KEY(PSA_ECC_FAMILY_SECP_R1));
		psa_set_key_algorithm(key_attr, PSA_ALG_ECDSA(PSA_ALG_ANY_HASH));
		break;
#endif /* PSA_NEED_CRACEN_ECDSA */
#ifdef PSA_NEED_CRACEN_HMAC
	case METADATA_ALG_HMAC:
		psa_set_key_type(key_attr, PSA_KEY_TYPE_HMAC);
		psa_set_key_algorithm(key_attr, PSA_ALG_HMAC(PSA_ALG_SHA_256));
		break;
#endif /* PSA_NEED_CRACEN_HMAC */
#ifdef PSA_NEED_CRACEN_ECDH
	case METADATA_ALG_ECDH:
		psa_set_key_type(key_attr, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1));
		psa_set_key_algorithm(key_attr, PSA_ALG_ECDH);
		break;
#endif
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
	case METADATA_ALG_KEY_BITS_384:
		psa_set_key_bits(key_attr, 384);
		break;
	}

	switch (metadata->key_usage_scheme) {
	case CRACEN_KMU_KEY_USAGE_SCHEME_PROTECTED:
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
	}

	return PSA_SUCCESS;
}

static psa_status_t convert_from_psa_attributes(const psa_key_attributes_t *key_attr,
						kmu_metadata *metadata)
{
	int kmu_slot;
	memset(metadata, 0, sizeof(*metadata));
	metadata->metadata_version = 0;

	metadata->key_usage_scheme = CRACEN_PSA_GET_KEY_USAGE_SCHEME(
		MBEDTLS_SVC_KEY_ID_GET_KEY_ID(psa_get_key_id(key_attr)));

	switch (metadata->key_usage_scheme) {
	case CRACEN_KMU_KEY_USAGE_SCHEME_PROTECTED:
	case CRACEN_KMU_KEY_USAGE_SCHEME_SEED:
	case CRACEN_KMU_KEY_USAGE_SCHEME_ENCRYPTED:
	case CRACEN_KMU_KEY_USAGE_SCHEME_RAW:
		break;
	default:
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	if (metadata->key_usage_scheme == CRACEN_KMU_KEY_USAGE_SCHEME_PROTECTED) {
		if (psa_get_key_usage_flags(key_attr) & PSA_KEY_USAGE_EXPORT) {
			return PSA_ERROR_INVALID_ARGUMENT;
		}
		if (psa_get_key_usage_flags(key_attr) & PSA_KEY_USAGE_COPY) {
			return PSA_ERROR_INVALID_ARGUMENT;
		}
	}

	if (metadata->key_usage_scheme == CRACEN_KMU_KEY_USAGE_SCHEME_SEED) {
		metadata->rpolicy = LIB_KMU_REV_POLICY_LOCKED;
		metadata->size = METADATA_ALG_KEY_BITS_384_SEED;
		return PSA_SUCCESS;
	}

	switch (psa_get_key_algorithm(key_attr)) {
#ifdef PSA_NEED_CRACEN_STREAM_CIPHER_CHACHA20
	case PSA_ALG_STREAM_CIPHER:
		metadata->algorithm = METADATA_ALG_CHACHA20;
		if (psa_get_key_type(key_attr) != PSA_KEY_TYPE_CHACHA20) {
			return PSA_ERROR_NOT_SUPPORTED;
		}
		break;
#endif /* PSA_NEED_CRACEN_STREAM_CIPHER_CHACHA20 */
#ifdef PSA_NEED_CRACEN_CHACHA20_POLY1305
	case PSA_ALG_CHACHA20_POLY1305:
		metadata->algorithm = METADATA_ALG_CHACHA20_POLY1305;
		if (psa_get_key_type(key_attr) != PSA_KEY_TYPE_CHACHA20) {
			return PSA_ERROR_NOT_SUPPORTED;
		}
		break;
#endif /* PSA_NEED_CRACEN_CHACHA20_POLY1305 */
#ifdef PSA_NEED_CRACEN_GCM_AES
	case PSA_ALG_GCM:
		metadata->algorithm = METADATA_ALG_AES_GCM;
		if (psa_get_key_type(key_attr) != PSA_KEY_TYPE_AES) {
			return PSA_ERROR_NOT_SUPPORTED;
		}
		break;
#endif /* PSA_NEED_CRACEN_GCM_AES */
#ifdef PSA_NEED_CRACEN_CCM_AES
	case PSA_ALG_CCM:
		metadata->algorithm = METADATA_ALG_AES_CCM;
		if (psa_get_key_type(key_attr) != PSA_KEY_TYPE_AES) {
			return PSA_ERROR_NOT_SUPPORTED;
		}
		break;
#endif /* PSA_NEED_CRACEN_CCM_AES */
#ifdef PSA_NEED_CRACEN_ECB_NO_PADDING_AES
	case PSA_ALG_ECB_NO_PADDING:
		metadata->algorithm = METADATA_ALG_AES_ECB;
		if (psa_get_key_type(key_attr) != PSA_KEY_TYPE_AES) {
			return PSA_ERROR_NOT_SUPPORTED;
		}
		break;
#endif /* PSA_NEED_CRACEN_ECB_NO_PADDING_AES */
#ifdef PSA_NEED_CRACEN_CTR_AES
	case PSA_ALG_CTR:
		metadata->algorithm = METADATA_ALG_AES_CTR;
		if (psa_get_key_type(key_attr) != PSA_KEY_TYPE_AES) {
			return PSA_ERROR_NOT_SUPPORTED;
		}
		break;
#endif /* PSA_NEED_CRACEN_CTR_AES */
#ifdef PSA_NEED_CRACEN_CBC_NO_PADDING_AES
	case PSA_ALG_CBC_NO_PADDING:
		metadata->algorithm = METADATA_ALG_AES_CBC;
		if (psa_get_key_type(key_attr) != PSA_KEY_TYPE_AES) {
			return PSA_ERROR_NOT_SUPPORTED;
		}
		break;
#endif /* PSA_NEED_CRACEN_CBC_NO_PADDING_AES */
#ifdef PSA_NEED_CRACEN_SP800_108_COUNTER_CMAC
	case PSA_ALG_SP800_108_COUNTER_CMAC:
		metadata->algorithm = METADATA_ALG_SP800_108_COUNTER_CMAC;
		if (psa_get_key_type(key_attr) != PSA_KEY_TYPE_AES) {
			return PSA_ERROR_NOT_SUPPORTED;
		}
		break;
#endif /* PSA_NEED_CRACEN_SP800_108_COUNTER_CMAC */
#ifdef PSA_NEED_CRACEN_CMAC
	case PSA_ALG_CMAC:
		metadata->algorithm = METADATA_ALG_CMAC;
		if (psa_get_key_type(key_attr) != PSA_KEY_TYPE_AES) {
			return PSA_ERROR_NOT_SUPPORTED;
		}
		break;

#endif /* PSA_NEED_CRACEN_CMAC */
#ifdef PSA_NEED_CRACEN_ED25519PH
	case PSA_ALG_ED25519PH:
		if (PSA_KEY_TYPE_ECC_GET_FAMILY(psa_get_key_type(key_attr)) !=
		    PSA_ECC_FAMILY_TWISTED_EDWARDS) {
			return PSA_ERROR_NOT_SUPPORTED;
		}
		/* Don't support private keys that are only used for verify */
		if (!can_sign(key_attr) &&
		    PSA_KEY_TYPE_IS_ECC_KEY_PAIR(psa_get_key_type(key_attr))) {
			return PSA_ERROR_NOT_SUPPORTED;
		}
		metadata->algorithm = METADATA_ALG_ED25519PH;
		break;
#endif /* PSA_NEED_CRACEN_ED25519PH */
#ifdef PSA_NEED_CRACEN_PURE_EDDSA_TWISTED_EDWARDS
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
#endif /* PSA_NEED_CRACEN_PURE_EDDSA_TWISTED_EDWARDS */
#ifdef PSA_NEED_CRACEN_ECDSA
	case PSA_ALG_ECDSA(PSA_ALG_ANY_HASH):
	case PSA_ALG_ECDSA(PSA_ALG_SHA_256):
	case PSA_ALG_ECDSA(PSA_ALG_SHA_384):
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
#endif /* PSA_NEED_CRACEN_ECDSA */
#ifdef PSA_NEED_CRACEN_HMAC
	case PSA_ALG_HMAC(PSA_ALG_SHA_256):
		if (!can_sign(key_attr) && PSA_ALG_IS_HMAC(psa_get_key_type(key_attr))) {
			return PSA_ERROR_NOT_SUPPORTED;
		}
		metadata->algorithm = METADATA_ALG_HMAC;
		break;
#endif /* PSA_NEED_CRACEN_HMAC */
#ifdef PSA_NEED_CRACEN_ECDH
	case PSA_ALG_ECDH:
		if (!can_derive(key_attr) || PSA_KEY_TYPE_ECC_GET_FAMILY(psa_get_key_type(
						     key_attr)) != PSA_ECC_FAMILY_SECP_R1) {
			return PSA_ERROR_NOT_SUPPORTED;
		}
		metadata->algorithm = METADATA_ALG_ECDH;
		break;
#endif /* PSA_NEED_CRACEN_ECDH */
	default:
		/* Ignore the algorithm for the protected ram invalidation kmu slot because
		 * it will never be used for crypto operations.
		 */
		kmu_slot = CRACEN_PSA_GET_KMU_SLOT(
			MBEDTLS_SVC_KEY_ID_GET_KEY_ID(psa_get_key_id(key_attr)));
		if (kmu_slot != PROTECTED_RAM_INVALIDATION_DATA_SLOT1) {
			return PSA_ERROR_NOT_SUPPORTED;
		}
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
	case 384:
		metadata->size = METADATA_ALG_KEY_BITS_384;
		break;
	default:
		return PSA_ERROR_NOT_SUPPORTED;
	}

	metadata->key_usage_scheme = CRACEN_PSA_GET_KEY_USAGE_SCHEME(
		MBEDTLS_SVC_KEY_ID_GET_KEY_ID(psa_get_key_id(key_attr)));

	if (metadata->key_usage_scheme == CRACEN_KMU_KEY_USAGE_SCHEME_PROTECTED) {
		if (psa_get_key_usage_flags(key_attr) & PSA_KEY_USAGE_EXPORT) {
			return PSA_ERROR_INVALID_ARGUMENT;
		}
		if (psa_get_key_usage_flags(key_attr) & PSA_KEY_USAGE_COPY) {
			return PSA_ERROR_INVALID_ARGUMENT;
		}
	}

	switch (PSA_KEY_LIFETIME_GET_PERSISTENCE(psa_get_key_lifetime(key_attr))) {
	case PSA_KEY_PERSISTENCE_READ_ONLY:
	case CRACEN_KEY_PERSISTENCE_READ_ONLY:
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

#ifdef PSA_NEED_CRACEN_KMU_ENCRYPTED_KEYS
	/* Provisioning data for encrypted keys:
	 *     - Nonce
	 *     - Key material (first 128 bits)
	 *     - Key material (optional for keys > 128 bits. Up to 512 bits.)
	 *     - Tag
	 */
	uint8_t encrypted_workmem[CRACEN_KMU_SLOT_KEY_SIZE * 6] = {};
#endif
	psa_status_t psa_status = clean_up_unfinished_provisioning();

	if (psa_status != PSA_SUCCESS) {
		return psa_status;
	}
	psa_status = convert_from_psa_attributes(key_attr, &metadata);

	if (psa_status) {
		return psa_status;
	}

	switch (metadata.key_usage_scheme) {
	case CRACEN_KMU_KEY_USAGE_SCHEME_PROTECTED:
		/* Only AES keys are supported. */
		if (psa_get_key_type(key_attr) != PSA_KEY_TYPE_AES) {
			return PSA_ERROR_NOT_SUPPORTED;
		}
		push_address = (uint8_t *)CRACEN_PROTECTED_RAM_AES_KEY0;
		/* Only 128, 192 and 256 bit keys are supported. */
		if (key_buffer_size != CRACEN_KMU_128_BIT_KEY_SIZE &&
		    key_buffer_size != CRACEN_KMU_192_BIT_KEY_SIZE &&
		    key_buffer_size != CRACEN_KMU_256_BIT_KEY_SIZE) {
			return PSA_ERROR_INVALID_ARGUMENT;
		}
		break;
#ifdef PSA_NEED_CRACEN_KMU_ENCRYPTED_KEYS
	case CRACEN_KMU_KEY_USAGE_SCHEME_ENCRYPTED:
		if (key_buffer_size > CRACEN_KMU_PUSH_AREA_SIZE) {
			/** ECC keys longer than CRACEN_KMU_PUSH_AREA_SIZE are not supported,
			 *  e.g. secp384r1 public keys since it would require two more KMU slots
			 */
			return PSA_ERROR_NOT_SUPPORTED;
		}
#endif /* PSA_NEED_CRACEN_KMU_ENCRYPTED_KEYS */
	case CRACEN_KMU_KEY_USAGE_SCHEME_RAW:
		push_address = (uint8_t *)kmu_push_area;
		if (key_buffer_size == CRACEN_KMU_SECP_R1_256_PUB_KEY_SIZE ||
		    key_buffer_size == CRACEN_KMU_SECP_R1_384_PUB_KEY_SIZE) {
			/* ECC public keys are 65 bytes (or 97 bytes in case of P384 key),
			 * but the first byte is CBR and compressed points are not supported
			 * so the first byte is removed here and appended when retrieved
			 */
			key_buffer++;
			key_buffer_size--;
		} else if (key_buffer_size != CRACEN_KMU_128_BIT_KEY_SIZE &&
			   key_buffer_size != CRACEN_KMU_192_BIT_KEY_SIZE &&
			   key_buffer_size != CRACEN_KMU_256_BIT_KEY_SIZE &&
			   key_buffer_size != CRACEN_KMU_384_BIT_KEY_SIZE) {
			return PSA_ERROR_INVALID_ARGUMENT;
		} else {
			/* Nothing to do. Added for compliance */
		}
		break;
	case CRACEN_KMU_KEY_USAGE_SCHEME_SEED:
		push_address = (uint8_t *)NRF_CRACEN->SEED;
		if (key_buffer_size != HUK_SIZE_BYTES) {
			return PSA_ERROR_INVALID_ARGUMENT;
		}
		break;
	default:
		return PSA_ERROR_INVALID_ARGUMENT;
	}

#ifdef PSA_NEED_CRACEN_KMU_ENCRYPTED_KEYS
	if (metadata.key_usage_scheme == CRACEN_KMU_KEY_USAGE_SCHEME_ENCRYPTED) {
		/* Copy key material to workbuffer, zero-pad key and align to key slot size. */
		memcpy(encrypted_workmem + CRACEN_KMU_SLOT_KEY_SIZE, key_buffer, key_buffer_size);
		psa_status = cracen_kmu_encrypt(encrypted_workmem + CRACEN_KMU_SLOT_KEY_SIZE,
						ROUND_UP(key_buffer_size, CRACEN_KMU_SLOT_KEY_SIZE),
						&metadata, encrypted_workmem,
						sizeof(encrypted_workmem), &key_buffer_size);

		if (psa_status != PSA_SUCCESS) {
			return psa_status;
		}
		key_buffer = encrypted_workmem;
	}
#endif /* PSA_NEED_CRACEN_KMU_ENCRYPTED_KEYS */

	/* Verify that required slots are empty */
	const size_t num_slots = DIV_ROUND_UP(key_buffer_size, CRACEN_KMU_SLOT_KEY_SIZE);

	for (size_t i = 0; i < num_slots; i++) {
		if (!lib_kmu_is_slot_empty(slot_id + i)) {
			psa_status = PSA_ERROR_ALREADY_EXISTS;
			goto exit;
		}
	}

	if (num_slots > 1) {
		psa_status = set_provisioning_in_progress(slot_id, num_slots);
		if (psa_status != PSA_SUCCESS) {
			return psa_status;
		}
	}

	struct kmu_src kmu_desc = {};

	for (size_t i = 0; i < num_slots; i++) {
		kmu_desc.dest = (uint32_t)push_address + (CRACEN_KMU_SLOT_KEY_SIZE * i);
		if (i == 0) {
			memcpy(&kmu_desc.metadata, &metadata, sizeof(kmu_desc.metadata));
		} else {
			kmu_desc.metadata = SECONDARY_SLOT_METADATA_VALUE;
		}
		kmu_desc.rpolicy = metadata.rpolicy;
		memcpy(kmu_desc.value, key_buffer + CRACEN_KMU_SLOT_KEY_SIZE * i,
		       CRACEN_KMU_SLOT_KEY_SIZE);

		int st = lib_kmu_provision_slot(slot_id + i, &kmu_desc);

		if (st) {
			/* We've already verified that this slot empty, so it should not fail. */
			psa_status = PSA_ERROR_HARDWARE_FAILURE;

			clean_up_unfinished_provisioning();
			break;
		}

		safe_memzero(&kmu_desc, sizeof(kmu_desc));
	}

	if (num_slots > 1 && psa_status == PSA_SUCCESS) {
		psa_status = end_provisioning(slot_id, num_slots);
	}

exit:
	return psa_status;
}

psa_status_t cracen_kmu_get_key_slot(mbedtls_svc_key_id_t key_id, psa_key_lifetime_t *lifetime,
				     psa_drv_slot_number_t *slot_number)
{
	psa_status_t psa_status;
	unsigned int slot_id;
	kmu_metadata metadata;
	psa_key_persistence_t persistence;

	psa_status = get_kmu_slot_id_and_metadata(key_id, &slot_id, &metadata);
	if (psa_status != PSA_SUCCESS) {
		return psa_status;
	}

	switch (metadata.rpolicy) {
	case LIB_KMU_REV_POLICY_ROTATING:
		persistence = PSA_KEY_PERSISTENCE_DEFAULT;
		break;
	case LIB_KMU_REV_POLICY_REVOKED:
		persistence = CRACEN_KEY_PERSISTENCE_REVOKABLE;
		break;
	case LIB_KMU_REV_POLICY_LOCKED:
		persistence = CRACEN_KEY_PERSISTENCE_READ_ONLY;
		break;
	default:
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	*lifetime = PSA_KEY_LIFETIME_FROM_PERSISTENCE_AND_LOCATION(persistence,
								   PSA_KEY_LOCATION_CRACEN_KMU);
	*slot_number = slot_id;

	return PSA_SUCCESS;
}

psa_status_t cracen_kmu_block(const psa_key_attributes_t *key_attr)
{
	psa_status_t psa_status;
	unsigned int slot_id, slot_count;

	psa_status = get_kmu_slot_id_and_count(key_attr, &slot_id, &slot_count);
	if (psa_status != PSA_SUCCESS) {
		return psa_status;
	}

	if (lib_kmu_block_slot_range(slot_id, slot_count) != LIB_KMU_SUCCESS) {
		return PSA_ERROR_GENERIC_ERROR;
	}
	return PSA_SUCCESS;
}

static psa_status_t push_kmu_key_to_ram(uint8_t *key_buffer, size_t key_buffer_size)
{
	if (key_buffer_size > sizeof(kmu_push_area)) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	psa_status_t psa_status;

	/* The kmu_push_area is guarded by the symmetric mutex since it is the most common use case.
	 * Here the decision was to avoid defining another mutex to handle the push buffer for the
	 * rest of the use cases.
	 */
	nrf_security_mutex_lock(cracen_mutex_symmetric);
	psa_status = silex_statuscodes_to_psa(cracen_kmu_prepare_key(key_buffer));
	if (psa_status == PSA_SUCCESS) {
		memcpy(key_buffer, kmu_push_area, key_buffer_size);
		safe_memzero(kmu_push_area, sizeof(kmu_push_area));
	}
	nrf_security_mutex_unlock(cracen_mutex_symmetric);

	return psa_status;
}

psa_status_t cracen_kmu_get_builtin_key(psa_drv_slot_number_t slot_number,
					psa_key_attributes_t *attributes, uint8_t *key_buffer,
					size_t key_buffer_size, size_t *key_buffer_length)
{
	kmu_metadata metadata;
	psa_status_t psa_status;
	size_t opaque_key_size;

	psa_status = read_primary_slot_metadata(slot_number, &metadata);
	if (psa_status != PSA_SUCCESS) {
		return psa_status;
	}

	if (slot_number == PROTECTED_RAM_INVALIDATION_DATA_SLOT1 ||
	    slot_number == PROTECTED_RAM_INVALIDATION_DATA_SLOT2) {
		/* The protected ram invalidation slots are not used for crypto operations. */
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	psa_status = clean_up_unfinished_provisioning();
	if (psa_status != PSA_SUCCESS) {
		return psa_status;
	}

	if (!attributes) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	psa_status = convert_to_psa_attributes(&metadata, attributes);

	if (psa_status != PSA_SUCCESS) {
		return psa_status;
	}


	psa_status = cracen_get_opaque_size(attributes, &opaque_key_size);
	if (psa_status != PSA_SUCCESS) {
		return psa_status;
	}

	if (key_buffer_size >= opaque_key_size) {
		if (psa_get_key_type(attributes) ==
		    PSA_KEY_TYPE_ECC_PUBLIC_KEY(PSA_ECC_FAMILY_SECP_R1)) {
			*key_buffer = CRACEN_ECC_PUBKEY_UNCOMPRESSED;
			key_buffer++;
			key_buffer_size--;
		}
		*key_buffer_length = opaque_key_size;
		kmu_opaque_key_buffer *key = (kmu_opaque_key_buffer *)key_buffer;
		unsigned int slot_count;

		key->key_usage_scheme = metadata.key_usage_scheme;

		psa_status = get_kmu_slot_count(metadata, attributes, &slot_count);
		if (psa_status != PSA_SUCCESS) {
			return psa_status;
		}
		key->number_of_slots = slot_count;

		key->slot_id = slot_number;
	} else {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	/* ECC keys are getting loading into the key buffer like volatile keys */
	if (PSA_KEY_TYPE_IS_ECC(psa_get_key_type(attributes))) {
		return push_kmu_key_to_ram(key_buffer, key_buffer_size);
	}

	/* HMAC keys are getting loading into the key buffer like volatile keys */
	if (psa_get_key_type(attributes) == PSA_KEY_TYPE_HMAC) {
		return push_kmu_key_to_ram(key_buffer, key_buffer_size);
	}

	return PSA_SUCCESS;
}
