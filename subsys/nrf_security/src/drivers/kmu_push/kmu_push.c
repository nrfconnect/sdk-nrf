/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <cracen/mem_helpers.h>
#include <cracen/lib_kmu.h>
#include <cracen/cracen_kmu.h>
#include <psa/crypto.h>
#include <stdint.h>
#include <string.h>
#include <cracen/common.h>
#include "kmu_push.h"

/** @brief Metadata version is chosen to not conflict with CRACEN KMU versions */
#define KMU_PUSH_METADATA_VERSION (15)

/** @brief Union to punt KMU_push metadata to integer value
 *
 * This type holds the metadata for KMU push driver and provides a direct
 * conversion via punting to an unsigned integer type.
 */
typedef union {
	uint32_t value;
	struct {
		uint32_t metadata_version: 4;	/**!< Metadata in the same location as CRACEN KMU metadata. */
		uint32_t num_slots: 8;		/**!< Size in number of KMU slots of this key. */
		uint32_t num_slots_left: 8;	/**!< Number of KMU slots before end of key. */
		uint32_t key_bits: 10;		/**!< Key size in bits. */
		uint32_t rpolicy: 2;		/**!< Rpolicy of the KMU slot */
	};
} kmu_push_metadata;
/** @brief structure type for volatile key representaation of a KMU push key */
typedef struct {
	uint16_t slot_id;
	uint16_t num_slots;
} kmu_push_opaque_key_buffer;

static psa_status_t provision_slots(const psa_key_attributes_t *attributes, size_t slot_id,
				    size_t num_slots, const kmu_push_key_buffer_t* kmu_push,
				    size_t key_bits)
{
	int kmu_err = -LIB_KMU_ERROR;
	const psa_key_lifetime_t lifetime = psa_get_key_lifetime(attributes);
	const psa_key_persistence_t persistence = PSA_KEY_LIFETIME_GET_PERSISTENCE(lifetime);
	kmu_push_metadata metadata = {0};
	struct kmu_src kmu_src = {0};
	size_t block_size = 0;
	uint8_t *cur_src = kmu_push->key_buffer;
	size_t size_left = kmu_push->buffer_size;

	/* Metadata is stable */
	switch(persistence){
	case PSA_KEY_PERSISTENCE_DEFAULT:
		kmu_src.rpolicy = LIB_KMU_REV_POLICY_ROTATING;
		break;
	case LIB_KMU_REV_POLICY_REVOKED:
		/* Revokable key slots currently not supported */
		return PSA_ERROR_NOT_SUPPORTED;
	case LIB_KMU_REV_POLICY_LOCKED:
		/* Read-only key slots currently not supported */
		return PSA_ERROR_NOT_SUPPORTED;
	default:
		return PSA_ERROR_BAD_STATE;
	}

	/* Stable KMU push metadata */
	metadata.metadata_version = KMU_PUSH_METADATA_VERSION;
	metadata.num_slots = num_slots;
	metadata.key_bits = key_bits;
	metadata.rpolicy = kmu_src.rpolicy;

	for (size_t i = 0; i < num_slots; i++) {
		metadata.num_slots_left = num_slots - i - 1;
		kmu_src.metadata = metadata.value;
		block_size = size_left > CRACEN_KMU_SLOT_KEY_SIZE ?
			CRACEN_KMU_SLOT_KEY_SIZE : size_left;
		cur_src =  kmu_push->key_buffer + (i * CRACEN_KMU_SLOT_KEY_SIZE);
		kmu_src.dest = kmu_push->dest_address + (i * CRACEN_KMU_SLOT_KEY_SIZE);

		safe_memzero(kmu_src.value, CRACEN_KMU_SLOT_KEY_SIZE);
		memcpy(kmu_src.value, cur_src, block_size);

		kmu_err = lib_kmu_provision_slot(slot_id + i, &kmu_src);
		if (kmu_err != LIB_KMU_SUCCESS) {
			return PSA_ERROR_NOT_PERMITTED;
		}

		size_left -= block_size;
	}

	return PSA_SUCCESS;
}

static psa_status_t get_metadata(unsigned int slot_id, kmu_push_metadata *metadata)
{
	uint32_t metadata_val;
	const int kmu_status = lib_kmu_read_metadata(slot_id, &metadata_val);
	switch (kmu_status) {
	case LIB_KMU_SUCCESS:
		break;
	case -LIB_KMU_REVOKED:
		return PSA_ERROR_NOT_PERMITTED;
	case -LIB_KMU_ERROR:
		return PSA_ERROR_DOES_NOT_EXIST;
	default:
		return PSA_ERROR_BAD_STATE;
	}

	metadata->value = metadata_val;
	return PSA_SUCCESS;
}

static psa_status_t get_lifetime(kmu_push_metadata *metadata, psa_key_lifetime_t * lifetime)
{
	psa_key_persistence_t persistence;
	switch (metadata->rpolicy) {
	case LIB_KMU_REV_POLICY_ROTATING:
		persistence = PSA_KEY_PERSISTENCE_DEFAULT;
		break;
	case LIB_KMU_REV_POLICY_REVOKED:
		/* Revokable key slots currently not supported */
		return PSA_ERROR_NOT_SUPPORTED;
	case LIB_KMU_REV_POLICY_LOCKED:
		/* Read-only key slots currently not supported */
		return PSA_ERROR_NOT_SUPPORTED;
	default:
		return PSA_ERROR_BAD_STATE;
	}

	*lifetime = PSA_KEY_LIFETIME_FROM_PERSISTENCE_AND_LOCATION(persistence,
								   PSA_KEY_LOCATION_KMU_PUSH);
	return PSA_SUCCESS;
}

static psa_status_t push_slots(size_t slot_id, size_t num_slots)
{
	int kmu_err = -LIB_KMU_ERROR;
	psa_status_t error = PSA_SUCCESS;

	/* Push all key material even if there is an error, but report it*/
	for (size_t i = 0; i < num_slots; i++) {
		kmu_err = lib_kmu_push_slot(slot_id + i);
		if(kmu_err != LIB_KMU_SUCCESS) {
			error = PSA_ERROR_NOT_PERMITTED;
		}
	}

	return error;
}

static psa_status_t destroy_slots(size_t slot_id, size_t num_slots)
{
	int kmu_err = -LIB_KMU_ERROR;
	psa_status_t error = PSA_SUCCESS;

	/* Continue even if there is an error, but report it */
	for (size_t i = 0; i < num_slots; i++) {
		kmu_err = lib_kmu_revoke_slot(slot_id + i);
		if (kmu_err != LIB_KMU_SUCCESS) {
			error = PSA_ERROR_NOT_PERMITTED;
		}
	}

	return error;
}

psa_status_t kmu_push_import_key(const psa_key_attributes_t *attributes, const uint8_t *data,
				 size_t data_length, uint8_t *key_buffer, size_t key_buffer_size,
				 size_t *key_buffer_length, size_t *key_bits)
{
	psa_status_t error;
	const psa_key_lifetime_t lifetime = psa_get_key_lifetime(attributes);
	const psa_key_persistence_t persistence = PSA_KEY_LIFETIME_GET_PERSISTENCE(lifetime);
	const kmu_push_key_buffer_t *kmu_push_buffer = (kmu_push_key_buffer_t *) data;
	size_t slot_id = CRACEN_PSA_GET_KMU_SLOT(
				MBEDTLS_SVC_KEY_ID_GET_KEY_ID(psa_get_key_id(attributes)));
	size_t num_slots = 0;
	size_t num_key_bits = psa_get_key_bits(attributes);
	bool push_immediately = false;
	bool destroy_after = false;

	/* KMU will not use a external buffer for key after call */
	ARG_UNUSED(key_buffer);
	ARG_UNUSED(key_buffer_length);
	ARG_UNUSED(key_bits);

	if(data == NULL || data_length == 0) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	if (data_length != sizeof(kmu_push_key_buffer_t)) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	if (num_key_bits == 0 || PSA_BITS_TO_BYTES(num_key_bits) > kmu_push_buffer->buffer_size) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	num_slots = DIV_ROUND_UP(kmu_push_buffer->buffer_size, CRACEN_KMU_SLOT_KEY_SIZE);

	/* Ensure that we are not using slots not intended for general usage */
	if ((slot_id + num_slots) >= PROTECTED_RAM_INVALIDATION_DATA_SLOT1) {
		return PSA_ERROR_NOT_PERMITTED;
	}

	/* Ensure that the slots are empty */
	for (size_t i = 0; i < num_slots; i++) {
		if (!lib_kmu_is_slot_empty(slot_id + i)) {
			return PSA_ERROR_NOT_PERMITTED;
		}
	}

	push_immediately = (kmu_push_buffer->usage_mask &
				KMU_PUSH_USAGE_MASK_PUSH_IMMEDIATELY) != 0;
	destroy_after = (kmu_push_buffer->usage_mask &
				KMU_PUSH_USAGE_MASK_DESTROY_AFTER) != 0;
	/* Read-only and DESTROY_AFTER is not valid combination*/

	error = provision_slots(attributes, slot_id, num_slots, kmu_push_buffer, num_key_bits);
	if (error != PSA_SUCCESS) {
		goto error;
	}

	if (push_immediately) {
		error = push_slots(slot_id, num_slots);
		if (error != PSA_SUCCESS) {
			goto error;
		}
	}

	if (destroy_after) {
		error = destroy_slots(slot_id, num_slots);
		if (error != PSA_SUCCESS) {
			goto error;
		}
	}

	*key_bits = num_key_bits;
	return PSA_SUCCESS;

error:
	/* Read-only keys can't be erased. Return the error. */
	if (persistence == CRACEN_KEY_PERSISTENCE_READ_ONLY) {
		return error;
	}

	/* Erase all key slots swallowing errors. */
	(void) destroy_slots(slot_id, num_slots);

	return error;
}

psa_status_t kmu_push_get_key_slot(mbedtls_svc_key_id_t key_id, psa_key_lifetime_t *lifetime,
				   psa_drv_slot_number_t *slot_number)
{
	psa_status_t error;
	unsigned int slot_id = CRACEN_PSA_GET_KMU_SLOT(MBEDTLS_SVC_KEY_ID_GET_KEY_ID(key_id));
	kmu_push_metadata metadata;

	error = get_metadata(slot_id, &metadata);
	if (error != PSA_SUCCESS) {
		return error;
	}
	/* Check that this is the first slot */
	if (metadata.metadata_version != KMU_PUSH_METADATA_VERSION ||
	    metadata.num_slots - 1 != metadata.num_slots_left) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	/* Populate the lifetime */
	error = get_lifetime(&metadata, lifetime);
	if (error != PSA_SUCCESS) {
		return error;
	}

	*slot_number = slot_id;

	return PSA_SUCCESS;
}

psa_status_t kmu_push_get_opaque_size(const psa_key_attributes_t *attributes, size_t *key_size)
{
	*key_size = sizeof(kmu_push_opaque_key_buffer);
	return PSA_SUCCESS;
}

psa_status_t kmu_push_get_builtin_key(psa_drv_slot_number_t slot_number,
				      psa_key_attributes_t *attributes, uint8_t *key_buffer,
				      size_t key_buffer_size, size_t *key_buffer_length)
{
	psa_status_t error;
	psa_key_lifetime_t lifetime;
	kmu_push_metadata metadata;
	kmu_push_opaque_key_buffer *opaque_key_buffer;
	mbedtls_svc_key_id_t key_id = mbedtls_svc_key_id_make(0,
					PSA_KEY_HANDLE_FROM_CRACEN_KMU_SLOT(
						CRACEN_KMU_KEY_USAGE_SCHEME_PROTECTED,slot_number));
	size_t num_slots = DIV_ROUND_UP(
				PSA_BITS_TO_BYTES(psa_get_key_bits(attributes)),
					CRACEN_KMU_SLOT_KEY_SIZE);

	error = get_metadata(slot_number, &metadata);
	if (error != PSA_SUCCESS) {
		return error;
	}

	/* Ensure we are only erasing KMU push key with this */
	if (metadata.metadata_version != KMU_PUSH_METADATA_VERSION){
		return PSA_ERROR_NOT_PERMITTED;
	}

	/* Ensure the number of keys to erase corresponds to store key */
	if (metadata.num_slots != num_slots) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	/* Ensure that this is the first key if multiple slots is used*/
	if (metadata.num_slots_left != (num_slots - 1)) {
		return PSA_ERROR_DATA_INVALID;
	}

	error = get_lifetime(&metadata, &lifetime);
	if (error != PSA_SUCCESS) {
		return error;
	}

	psa_set_key_lifetime(attributes, lifetime);
	psa_set_key_bits(attributes, metadata.key_bits);
	psa_set_key_id(attributes, key_id);

	if (key_buffer_size >= sizeof(kmu_push_opaque_key_buffer)) {
		opaque_key_buffer = (kmu_push_opaque_key_buffer *) key_buffer;
		opaque_key_buffer->slot_id = slot_number;
		opaque_key_buffer->num_slots = metadata.num_slots;
		*key_buffer_length = sizeof(kmu_opaque_key_buffer);
	}

	return PSA_SUCCESS;
}


psa_status_t kmu_push_destroy_key(const psa_key_attributes_t *attributes)
{
	psa_status_t error;
	const psa_key_lifetime_t lifetime = psa_get_key_lifetime(attributes);
	const psa_key_persistence_t persistence = PSA_KEY_LIFETIME_GET_PERSISTENCE(lifetime);
	size_t slot_id = CRACEN_PSA_GET_KMU_SLOT(
				MBEDTLS_SVC_KEY_ID_GET_KEY_ID(psa_get_key_id(attributes)));
	size_t num_slots = 0;
	size_t key_bits = psa_get_key_bits(attributes);

	/* Check if key can be erased */
	if (persistence == CRACEN_KEY_PERSISTENCE_READ_ONLY) {
		return PSA_ERROR_NOT_PERMITTED;
	}

	/* Ensure that key size is given */
	if (key_bits == 0) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	/* Get the slot count from bit size and verify range */
	num_slots = DIV_ROUND_UP(PSA_BITS_TO_BYTES(key_bits), CRACEN_KMU_SLOT_KEY_SIZE);
	if (slot_id + num_slots >= PROTECTED_RAM_INVALIDATION_DATA_SLOT1) {
		return PSA_ERROR_NOT_PERMITTED;
	}

	error = destroy_slots(slot_id, num_slots);
	if (error != PSA_SUCCESS) {
		return error;
	}

	return error;
}
