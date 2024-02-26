/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <psa/protected_storage.h>

#include "../trusted_storage_backend.h"

psa_status_t psa_ps_get_info(psa_storage_uid_t uid, struct psa_storage_info_t *p_info)
{
	return trusted_get_info(uid, CONFIG_PSA_PROTECTED_STORAGE_PREFIX, p_info);
}

psa_status_t psa_ps_get(psa_storage_uid_t uid, size_t data_offset, size_t data_length, void *p_data,
			size_t *p_data_length)
{
	return trusted_get(uid, CONFIG_PSA_PROTECTED_STORAGE_PREFIX, data_offset, data_length,
			  p_data, p_data_length);
}

psa_status_t psa_ps_set(psa_storage_uid_t uid, size_t data_length, const void *p_data,
			psa_storage_create_flags_t create_flags)
{
	return trusted_set(uid, CONFIG_PSA_PROTECTED_STORAGE_PREFIX, data_length, p_data,
			  create_flags);
}

psa_status_t psa_ps_remove(psa_storage_uid_t uid)
{
	return trusted_remove(uid, CONFIG_PSA_PROTECTED_STORAGE_PREFIX);
}

uint32_t psa_ps_get_support(void)
{
	return trusted_get_support();
}

psa_status_t psa_ps_create(psa_storage_uid_t uid, size_t capacity,
			   psa_storage_create_flags_t create_flags)
{
	return trusted_create(uid, capacity, create_flags);
}

psa_status_t psa_ps_set_extended(psa_storage_uid_t uid, size_t data_offset, size_t data_length,
				 const void *p_data)
{
	return trusted_set_extended(uid, data_offset, data_length, p_data);
}
