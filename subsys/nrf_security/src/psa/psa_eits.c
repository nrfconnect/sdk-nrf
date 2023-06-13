/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <psa/internal_trusted_storage.h>

#include "psa_eits_backend.h"

psa_status_t psa_its_set(psa_storage_uid_t uid, uint32_t data_length, const void *p_data,
			 psa_storage_create_flags_t create_flags)
{
	if (p_data == NULL || data_length == 0) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	return psa_its_set_backend(uid, data_length, p_data, create_flags);
}

psa_status_t psa_its_get(psa_storage_uid_t uid, uint32_t data_offset, uint32_t data_length,
			 void *p_data, size_t *p_data_length)
{
	if (p_data == NULL || p_data_length == NULL || data_length == 0) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	return psa_its_get_backend(uid, data_offset, data_length, p_data, p_data_length);
}

psa_status_t psa_its_get_info(psa_storage_uid_t uid, struct psa_storage_info_t *p_info)
{
	if (p_info == NULL) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	return psa_its_get_info_backend(uid, p_info);
}

psa_status_t psa_its_remove(psa_storage_uid_t uid)
{
	return psa_its_remove_backend(uid);
}
