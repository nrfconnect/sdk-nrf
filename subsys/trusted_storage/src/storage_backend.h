/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef __STORAGE_BACKEND_H_
#define __STORAGE_BACKEND_H_

#include <psa/error.h>
#include <psa/storage_common.h>

/* Gets an object up to object_size size */
psa_status_t storage_get_object(const psa_storage_uid_t uid, const char *prefix, void *object_data,
				const size_t object_size, size_t *object_length);

/* Writes an object */
psa_status_t storage_set_object(const psa_storage_uid_t uid, const char *prefix,
				const void *object_data, const size_t object_size);

/* Deletes an object */
psa_status_t storage_remove_object(const psa_storage_uid_t uid, const char *prefix);

#endif /* __STORAGE_BACKEND_H_*/
