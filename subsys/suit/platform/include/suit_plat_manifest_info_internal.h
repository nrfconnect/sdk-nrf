/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SUIT_PLAT_MANIFEST_INFO_INTERNAL_H__
#define SUIT_PLAT_MANIFEST_INFO_INTERNAL_H__

#include <suit_metadata.h>
#include <zcbor_common.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Gets an array of supported manifest class_info struct. This is
 *        a function which shall be supported on all domains.
 *
 * @param[out]     class_info An array of `suit_manifest_class_info_t` structures to store the
 *                            supported manifest class information.
 * @param[in,out]  size       as input - maximal amount of elements an array can hold,
 *                             as output - amount of stored elements
 * @return SUIT_SUCCESS on success, error code otherwise.
 *
 */
int suit_plat_supported_manifest_class_infos_get(suit_manifest_class_info_t *class_info,
						 size_t *size);

/**
 * @brief Find a role for a manifest with given class ID.
 *
 * @param[in]   class_id  Manifest class ID.
 * @param[out]  role      Pointer to the role variable.
 *
 * @return SUIT_SUCCESS on success, error code otherwise.
 */
int suit_plat_manifest_role_get(const suit_manifest_class_id_t *class_id,
				suit_manifest_role_t *role);

#ifdef __cplusplus
}
#endif

#endif /* SUIT_PLAT_MANIFEST_INFO_INTERNAL_H__ */
