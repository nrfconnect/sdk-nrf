/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <psa/internal_trusted_storage.h>
#include <psa/storage_common.h>

#include "../secure_storage_backend.h"

psa_status_t psa_its_get_info(psa_storage_uid_t uid, struct psa_storage_info_t *p_info)
{
	if (p_info == NULL) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	return secure_get_info(uid, CONFIG_PSA_INTERNAL_TRUSTED_STORAGE_PREFIX, p_info);
}

psa_status_t psa_its_get(psa_storage_uid_t uid, size_t data_offset, size_t data_length,
			 void *p_data, size_t *p_data_length)
{
	if (data_length == 0 || p_data == NULL || p_data_length == NULL) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	return secure_get(uid, CONFIG_PSA_INTERNAL_TRUSTED_STORAGE_PREFIX, data_offset, data_length,
			  p_data, p_data_length);
}

psa_status_t psa_its_set(psa_storage_uid_t uid, size_t data_length, const void *p_data,
			 psa_storage_create_flags_t create_flags)
{
	if (data_length == 0 || p_data == NULL) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	return secure_set(uid, CONFIG_PSA_INTERNAL_TRUSTED_STORAGE_PREFIX, data_length, p_data,
			  create_flags);
}

psa_status_t psa_its_remove(psa_storage_uid_t uid)
{
	return secure_remove(uid, CONFIG_PSA_INTERNAL_TRUSTED_STORAGE_PREFIX);
}
