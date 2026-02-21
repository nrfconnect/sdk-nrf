/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <cracen_psa_key_management.h>

#include <cracen/common.h>
#include <cracen/statuscodes.h>
#include <cracen/cracen_kmu.h>
#include <cracen_psa.h>
#include <cracen_psa_ctr_drbg.h>
#include <cracen_psa_ikg.h>
#include <internal/ecc/cracen_ecc_key_management.h>
#include <internal/rsa/cracen_rsa_key_management.h>
#include <internal/pake/cracen_wpa3_key_management.h>
#include <internal/pake/cracen_spake2p_key_management.h>
#include <internal/pake/cracen_srp_key_management.h>
#include <cracen_psa_builtin_key_policy.h>
#include <nrf_security_mutexes.h>
#include <stddef.h>
#include <string.h>
#include <zephyr/sys/__assert.h>

extern nrf_security_mutex_t cracen_mutex_symmetric;

psa_status_t cracen_export_public_key(const psa_key_attributes_t *attributes,
				      const uint8_t *key_buffer, size_t key_buffer_size,
				      uint8_t *data, size_t data_size, size_t *data_length)
{
	__ASSERT_NO_MSG(data);
	__ASSERT_NO_MSG(data_length);

	psa_key_type_t key_type = psa_get_key_type(attributes);
	*data_length = 0;

	if (data_size == 0) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	if (IS_ENABLED(PSA_NEED_CRACEN_KEY_TYPE_ECC_KEY_PAIR_EXPORT)) {
		if (PSA_KEY_TYPE_IS_ECC_KEY_PAIR(key_type)) {
			return export_ecc_public_key_from_keypair(attributes, key_buffer,
								  key_buffer_size, data, data_size,
								  data_length);
		} else if (PSA_KEY_TYPE_IS_ECC_PUBLIC_KEY(key_type)) {
			return ecc_export_key(attributes, key_buffer, key_buffer_size, data,
					      data_size, data_length);
		} else {
			/* For compliance */
		}
	}

	if (IS_ENABLED(PSA_NEED_CRACEN_KEY_TYPE_SPAKE2P_KEY_PAIR_EXPORT_SECP_R1_256)) {
		if (PSA_KEY_TYPE_IS_SPAKE2P_KEY_PAIR(key_type)) {
			return export_spake2p_public_key_from_keypair(attributes, key_buffer,
								      key_buffer_size, data,
								      data_size, data_length);
		} else if (PSA_KEY_TYPE_IS_SPAKE2P_PUBLIC_KEY(key_type)) {
			return ecc_export_key(attributes, key_buffer, key_buffer_size, data,
					      data_size, data_length);
		} else {
			/* For compliance */
		}
	}

	if (key_type == PSA_KEY_TYPE_RSA_KEY_PAIR &&
	    IS_ENABLED(PSA_NEED_CRACEN_KEY_TYPE_RSA_KEY_PAIR_EXPORT)) {
		return export_rsa_public_key_from_keypair(attributes, key_buffer, key_buffer_size,
							  data, data_size, data_length);
	} else if (key_type == PSA_KEY_TYPE_RSA_PUBLIC_KEY &&
		   IS_ENABLED(PSA_NEED_CRACEN_KEY_TYPE_RSA_PUBLIC_KEY)) {
		return rsa_export_public_key(attributes, key_buffer, key_buffer_size, data,
					     data_size, data_length);
	} else {
		/* For compliance */
	}

	return PSA_ERROR_NOT_SUPPORTED;
}

psa_status_t cracen_import_key(const psa_key_attributes_t *attributes, const uint8_t *data,
			       size_t data_length, uint8_t *key_buffer, size_t key_buffer_size,
			       size_t *key_buffer_length, size_t *key_bits)
{
	__ASSERT_NO_MSG(key_buffer);
	__ASSERT_NO_MSG(key_buffer_length);
	__ASSERT_NO_MSG(key_bits);

	psa_key_type_t key_type = psa_get_key_type(attributes);
	*key_bits = 0;
	*key_buffer_length = 0;

	if (key_buffer_size == 0) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	psa_key_location_t location =
		PSA_KEY_LIFETIME_GET_LOCATION(psa_get_key_lifetime(attributes));
#ifdef PSA_NEED_CRACEN_KMU_DRIVER
	if (location == PSA_KEY_LOCATION_CRACEN_KMU) {
		int slot_id = CRACEN_PSA_GET_KMU_SLOT(
			MBEDTLS_SVC_KEY_ID_GET_KEY_ID(psa_get_key_id(attributes)));
		psa_key_attributes_t stored_attributes;

#ifdef CONFIG_CRACEN_PROVISION_PROT_RAM_INV_SLOTS_WITH_IMPORT
		if (slot_id == PROTECTED_RAM_INVALIDATION_DATA_SLOT1) {
			/* The key bits are required for the psa_import_key to succeed */
			*key_bits = 256;
			return cracen_provision_prot_ram_inv_slots();
		}
#endif
		if (!cracen_kmu_key_user_allowed(attributes)) {
			return PSA_ERROR_NOT_PERMITTED;
		}

		size_t opaque_key_size;
		psa_status_t status = cracen_get_opaque_size(attributes, &opaque_key_size);

		if (status != PSA_SUCCESS) {
			return status;
		}

		if (key_buffer_size < opaque_key_size) {
			return PSA_ERROR_BUFFER_TOO_SMALL;
		}

		status = cracen_kmu_provision(attributes, slot_id, data, data_length);
		if (status != PSA_SUCCESS) {
			return status;
		}

		status = cracen_kmu_get_builtin_key(slot_id, &stored_attributes, key_buffer,
						    key_buffer_size, key_buffer_length);
		if (status != PSA_SUCCESS) {
			return status;
		}

		*key_bits = psa_get_key_bits(&stored_attributes);

		return status;
	}
#endif

	if (location != PSA_KEY_LOCATION_LOCAL_STORAGE) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	if (IS_ENABLED(PSA_NEED_CRACEN_KEY_TYPE_ECC_KEY_PAIR_IMPORT)) {
		if (PSA_KEY_TYPE_IS_ECC_KEY_PAIR(key_type)) {
			return import_ecc_private_key(attributes, data, data_length, key_buffer,
						      key_buffer_size, key_buffer_length, key_bits);
		} else if (PSA_KEY_TYPE_IS_ECC_PUBLIC_KEY(key_type)) {
			return import_ecc_public_key(attributes, data, data_length, key_buffer,
						     key_buffer_size, key_buffer_length, key_bits);
		} else {
			/* For compliance */
		}
	}

	if (PSA_KEY_TYPE_IS_RSA(key_type) &&
	    IS_ENABLED(PSA_NEED_CRACEN_KEY_TYPE_RSA_KEY_PAIR_IMPORT)) {
		return import_rsa_key(attributes, data, data_length, key_buffer, key_buffer_size,
				      key_buffer_length, key_bits);
	}

	if (PSA_KEY_TYPE_IS_SPAKE2P(key_type) && IS_ENABLED(PSA_NEED_CRACEN_SPAKE2P)) {
		return import_spake2p_key(attributes, data, data_length, key_buffer,
					  key_buffer_size, key_buffer_length, key_bits);
	}

	if (PSA_KEY_TYPE_IS_SRP(key_type) && IS_ENABLED(PSA_NEED_CRACEN_SRP_6)) {
		return import_srp_key(attributes, data, data_length, key_buffer, key_buffer_size,
				      key_buffer_length, key_bits);
	}

	if (PSA_KEY_TYPE_IS_WPA3_SAE_ECC(key_type) && IS_ENABLED(PSA_NEED_CRACEN_WPA3_SAE)) {
		return import_wpa3_sae_pt_key(attributes, data, data_length, key_buffer,
					  key_buffer_size, key_buffer_length, key_bits);
	}

	return PSA_ERROR_NOT_SUPPORTED;
}

#ifdef PSA_NEED_CRACEN_KMU_DRIVER
static
psa_status_t generate_key_for_kmu(const psa_key_attributes_t *attributes, uint8_t *key_buffer,
				  size_t key_buffer_size, size_t *key_buffer_length)
{
	psa_key_type_t key_type = psa_get_key_type(attributes);
	uint8_t key[CRACEN_KMU_MAX_KEY_SIZE];
	size_t key_bits;
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;

	if (PSA_BITS_TO_BYTES(psa_get_key_bits(attributes)) > sizeof(key)) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	if (PSA_KEY_TYPE_IS_ECC_KEY_PAIR(key_type) &&
	    IS_ENABLED(PSA_NEED_CRACEN_KEY_TYPE_ECC_KEY_PAIR_GENERATE)) {
		status = generate_ecc_private_key(attributes, key,
						  PSA_BITS_TO_BYTES(psa_get_key_bits(attributes)),
						  key_buffer_length);
		if (status != PSA_SUCCESS) {
			return status;
		}
	} else if (key_type == PSA_KEY_TYPE_AES || key_type == PSA_KEY_TYPE_HMAC ||
		   key_type == PSA_KEY_TYPE_CHACHA20) {
		status = cracen_get_random(NULL, key,
					   PSA_BITS_TO_BYTES(psa_get_key_bits(attributes)));
		if (status != PSA_SUCCESS) {
			return status;
		}
	} else {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	return cracen_import_key(attributes, key, PSA_BITS_TO_BYTES(psa_get_key_bits(attributes)),
				 key_buffer, key_buffer_size, key_buffer_length, &key_bits);
}
#endif /* PSA_NEED_CRACEN_KMU_DRIVER */

psa_status_t cracen_generate_key(const psa_key_attributes_t *attributes, uint8_t *key_buffer,
				 size_t key_buffer_size, size_t *key_buffer_length)
{
	__ASSERT_NO_MSG(key_buffer);
	__ASSERT_NO_MSG(key_buffer_length);

	psa_key_type_t key_type = psa_get_key_type(attributes);
	psa_key_location_t location =
		PSA_KEY_LIFETIME_GET_LOCATION(psa_get_key_lifetime(attributes));

#ifdef PSA_NEED_CRACEN_KMU_DRIVER
	if (location == PSA_KEY_LOCATION_CRACEN_KMU) {
		if (!cracen_kmu_key_user_allowed(attributes)) {
			return PSA_ERROR_NOT_PERMITTED;
		}
		return generate_key_for_kmu(attributes, key_buffer, key_buffer_size,
					    key_buffer_length);
	}
#endif

	if (location != PSA_KEY_LOCATION_LOCAL_STORAGE) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	if (key_buffer_size == 0) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	if (PSA_KEY_TYPE_IS_ECC_KEY_PAIR(key_type) &&
	    IS_ENABLED(PSA_NEED_CRACEN_KEY_TYPE_ECC_KEY_PAIR_GENERATE)) {
		return generate_ecc_private_key(attributes, key_buffer, key_buffer_size,
						key_buffer_length);
	}

	if (key_type == PSA_KEY_TYPE_RSA_KEY_PAIR &&
	    IS_ENABLED(PSA_NEED_CRACEN_KEY_TYPE_RSA_KEY_PAIR_GENERATE)) {
		return generate_rsa_private_key(attributes, key_buffer, key_buffer_size,
						key_buffer_length);
	}

	return PSA_ERROR_NOT_SUPPORTED;
}

psa_status_t cracen_get_builtin_key(psa_drv_slot_number_t slot_number,
				    psa_key_attributes_t *attributes, uint8_t *key_buffer,
				    size_t key_buffer_size, size_t *key_buffer_length)
{
	/* According to the PSA Crypto Driver specification, the PSA core will set the `id`
	 * and the `lifetime` field of the attribute struct. We will fill all the other
	 * attributes, and update the `lifetime` field to be more specific.
	 */

	switch (slot_number) {
	case CRACEN_BUILTIN_IDENTITY_KEY_ID:
	case CRACEN_BUILTIN_MKEK_ID:
	case CRACEN_BUILTIN_MEXT_ID:
		if (IS_ENABLED(CONFIG_CRACEN_IKG)) {
			return cracen_ikg_get_builtin_key(slot_number, attributes, key_buffer,
							  key_buffer_size, key_buffer_length);
		} else {
			return PSA_ERROR_NOT_SUPPORTED;
		}
	default:
#ifdef PSA_NEED_CRACEN_KMU_DRIVER
		if (!cracen_kmu_key_user_allowed(attributes)) {
			return PSA_ERROR_NOT_PERMITTED;
		}
		return cracen_kmu_get_builtin_key(slot_number, attributes, key_buffer,
						  key_buffer_size, key_buffer_length);
#else
		return PSA_ERROR_DOES_NOT_EXIST;
#endif
	}
}

psa_status_t cracen_get_key_slot(mbedtls_svc_key_id_t key_id, psa_key_lifetime_t *lifetime,
				 psa_drv_slot_number_t *slot_number)
{
	switch (MBEDTLS_SVC_KEY_ID_GET_KEY_ID(key_id)) {
	case CRACEN_BUILTIN_IDENTITY_KEY_ID:
		*slot_number = CRACEN_BUILTIN_IDENTITY_KEY_ID;
		break;
	case CRACEN_BUILTIN_MKEK_ID:
		*slot_number = CRACEN_BUILTIN_MKEK_ID;
		break;
	case CRACEN_BUILTIN_MEXT_ID:
		*slot_number = CRACEN_BUILTIN_MEXT_ID;
		break;
	default:
#ifdef PSA_NEED_CRACEN_KMU_DRIVER
		return cracen_kmu_get_key_slot(key_id, lifetime, slot_number);
#else
		return PSA_ERROR_DOES_NOT_EXIST;
#endif
	};

	*lifetime = PSA_KEY_LIFETIME_FROM_PERSISTENCE_AND_LOCATION(CRACEN_KEY_PERSISTENCE_READ_ONLY,
								   PSA_KEY_LOCATION_CRACEN);

	return PSA_SUCCESS;
}

psa_status_t cracen_export_key(const psa_key_attributes_t *attributes, const uint8_t *key_buffer,
			       size_t key_buffer_size, uint8_t *data, size_t data_size,
			       size_t *data_length)
{
#ifdef PSA_NEED_CRACEN_KMU_DRIVER
	int status;
	int nested_err;
	psa_key_location_t location =
		PSA_KEY_LIFETIME_GET_LOCATION(psa_get_key_lifetime(attributes));

	if (location == PSA_KEY_LOCATION_CRACEN_KMU) {
		/* The keys will already be in the key buffer as they got loaded there by a previous
		 * call to cracen_get_builtin_key or cached in the memory.
		 */
		psa_key_type_t key_type = psa_get_key_type(attributes);

		psa_ecc_family_t ecc_fam = PSA_KEY_TYPE_ECC_GET_FAMILY(key_type);
		if (ecc_fam == PSA_ECC_FAMILY_TWISTED_EDWARDS ||
		    ecc_fam == PSA_ECC_FAMILY_SECP_R1 || key_type == PSA_KEY_TYPE_HMAC) {
			memcpy(data, key_buffer, key_buffer_size);
			*data_length = key_buffer_size;
			return PSA_SUCCESS;
		}

		size_t key_out_size = PSA_BITS_TO_BYTES(psa_get_key_bits(attributes));

		if (key_out_size > data_size) {
			return PSA_ERROR_BUFFER_TOO_SMALL;
		}

		/* The kmu_push_area is guarded by the symmetric mutex since it is the most common
		 * use case. Here the decision was to avoid defining another mutex to handle the
		 * push buffer for the rest of the use cases.
		 */
		nrf_security_mutex_lock(cracen_mutex_symmetric);
		status = cracen_kmu_prepare_key(key_buffer);
		if (status == SX_OK) {
			memcpy(data, kmu_push_area, key_out_size);
			*data_length = key_out_size;
		}

		nested_err = cracen_kmu_clean_key(key_buffer);

		nrf_security_mutex_unlock(cracen_mutex_symmetric);
		status = sx_handle_nested_error(nested_err, status);

		return silex_statuscodes_to_psa(status);
	}
#endif

	return PSA_ERROR_DOES_NOT_EXIST;
}

psa_status_t cracen_copy_key(psa_key_attributes_t *attributes, const uint8_t *source_key,
			     size_t source_key_length, uint8_t *target_key_buffer,
			     size_t target_key_buffer_size, size_t *target_key_buffer_length)
{
#ifdef PSA_NEED_CRACEN_KMU_DRIVER
	psa_key_location_t location =
		PSA_KEY_LIFETIME_GET_LOCATION(psa_get_key_lifetime(attributes));

	/* PSA core only invokes this if source location matches target location.
	 * Whether copy usage is allowed has been validated at this point.
	 */
	if (location != PSA_KEY_LOCATION_CRACEN_KMU) {
		return PSA_ERROR_DOES_NOT_EXIST;
	}

	if (PSA_KEY_TYPE_IS_ECC(psa_get_key_type(attributes)) ||
	    (psa_get_key_type(attributes) == PSA_KEY_TYPE_HMAC)) {
		size_t key_bits;

		return cracen_import_key(attributes, source_key, source_key_length,
					 target_key_buffer, target_key_buffer_size,
					 target_key_buffer_length, &key_bits);
	}

	int sx_status;
	int nested_err;
	psa_status_t psa_status = PSA_ERROR_HARDWARE_FAILURE;
	size_t key_size = PSA_BITS_TO_BYTES(psa_get_key_bits(attributes));

	nrf_security_mutex_lock(cracen_mutex_symmetric);
	sx_status = cracen_kmu_prepare_key(source_key);

	if (sx_status == SX_OK) {
		size_t key_bits;

		psa_status = cracen_import_key(attributes, kmu_push_area, key_size,
					       target_key_buffer, target_key_buffer_size,
					       target_key_buffer_length, &key_bits);
	}

	nested_err = cracen_kmu_clean_key(source_key);

	nrf_security_mutex_unlock(cracen_mutex_symmetric);

	sx_status = sx_handle_nested_error(nested_err, sx_status);

	if (sx_status != SX_OK) {
		return silex_statuscodes_to_psa(sx_status);
	}

	return psa_status;
#endif

	return PSA_ERROR_DOES_NOT_EXIST;
}

psa_status_t cracen_destroy_key(const psa_key_attributes_t *attributes)
{
#ifdef PSA_NEED_CRACEN_KMU_DRIVER
	return cracen_kmu_destroy_key(attributes);
#endif

	return PSA_ERROR_DOES_NOT_EXIST;
}

psa_status_t cracen_derive_key(const psa_key_attributes_t *attributes, const uint8_t *input,
			       size_t input_length, uint8_t *key, size_t key_size,
			       size_t *key_length)
{
	psa_key_type_t key_type = psa_get_key_type(attributes);

#ifdef PSA_NEED_CRACEN_KMU_DRIVER
	/* This is called from psa_key_derivation_output_key(), the arguments we
	 * receive here are for the newly-created key, so check that we are allowed
	 * to create that new key.
	 */
	if (PSA_KEY_LIFETIME_GET_LOCATION(psa_get_key_lifetime(attributes))
					  == PSA_KEY_LOCATION_CRACEN_KMU
	    && !cracen_kmu_key_user_allowed(attributes)) {
		return PSA_ERROR_NOT_PERMITTED;
	}
#endif /* PSA_NEED_CRACEN_KMU_DRIVER */

	if (PSA_KEY_TYPE_IS_SPAKE2P_KEY_PAIR(key_type) && IS_ENABLED(PSA_NEED_CRACEN_SPAKE2P)) {
		return cracen_derive_spake2p_key(attributes, input, input_length, key, key_size,
						 key_length);
	}

	if (PSA_KEY_TYPE_IS_WPA3_SAE_ECC(key_type) && IS_ENABLED(PSA_NEED_CRACEN_WPA3_SAE)) {
		return cracen_derive_wpa3_sae_pt_key(attributes, input, input_length, key, key_size,
						     key_length);
	}

	return PSA_ERROR_NOT_SUPPORTED;
}
