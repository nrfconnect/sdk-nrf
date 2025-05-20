/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef __TRUSTED_STORAGE_BACKEND_H_
#define __TRUSTED_STORAGE_BACKEND_H_

#include <psa/error.h>
#include <psa/storage_common.h>

psa_status_t trusted_get_info(const psa_storage_uid_t uid, const char *prefix,
			     struct psa_storage_info_t *p_info);

psa_status_t trusted_get(const psa_storage_uid_t uid, const char *prefix, size_t data_offset,
			size_t data_length, void *p_data, size_t *p_data_length);

psa_status_t trusted_set(const psa_storage_uid_t uid, const char *prefix, size_t data_length,
			const void *p_data, psa_storage_create_flags_t create_flags);

psa_status_t trusted_remove(const psa_storage_uid_t uid, const char *prefix);

uint32_t trusted_get_support(void);

psa_status_t trusted_create(const psa_storage_uid_t uid, size_t capacity,
			   psa_storage_create_flags_t create_flags);

psa_status_t trusted_set_extended(const psa_storage_uid_t uid, size_t data_offset,
				 size_t data_length, const void *p_data);

#endif /* __TRUSTED_STORAGE_BACKEND_H_*/
