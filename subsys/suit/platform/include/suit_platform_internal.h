/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SUIT_PLATFORM_INTERNAL_H__
#define SUIT_PLATFORM_INTERNAL_H__

#include <stdint.h>
#include <suit_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief SUIT component type.
 *
 * @details To provide a better flexibility in the component ID definitions,
 *          it is advised to use the decoding function and the type defined below
 *          to evaluate which type of the component is being processed.
 */
typedef enum {
	/** Reserved value, indicating that component type is not defined by any of the known
	 *  values.
	 */
	SUIT_COMPONENT_TYPE_UNSUPPORTED,

	/** MCU memory-mapped slot (MRAM, RAM, external flash). */
	SUIT_COMPONENT_TYPE_MEM,

	/** Reference to dependency candidate envelope in candidate envelope or in the download
	 *  cache.
	 */
	SUIT_COMPONENT_TYPE_CAND_MFST,

	/** Reference to integrated payload in candidate envelope or payload in the download cache.
	 */
	SUIT_COMPONENT_TYPE_CAND_IMG,

	/** Installed envelope, holding severed manifest and its authentication block. */
	SUIT_COMPONENT_TYPE_INSTLD_MFST,

	/** SOC-Specific component. Necessary in case if installation to or reading from this
	 *  component type goes beyond 'memcpy-like' operations.
	 */
	SUIT_COMPONENT_TYPE_SOC_SPEC,

	/** SUIT Cache pool, where images downloaded during 'payload-fetch' sequence execution can
	 * be stored, ready for installation.
	 */
	SUIT_COMPONENT_TYPE_CACHE_POOL,
} suit_component_type_t;

/** Set the pointer to the implementation-specific data. */
int suit_plat_component_impl_data_set(suit_component_t handle, void *impl_data);

/** Return the pointer to the implementation-specific data. */
int suit_plat_component_impl_data_get(suit_component_t handle, void **impl_data);

/** Return the full component ID associated with the component. */
int suit_plat_component_id_get(suit_component_t handle, struct zcbor_string **component_id);

/** Return component type based on component handle */
int suit_plat_component_type_get(suit_component_t handle, suit_component_type_t *component_type);

#ifdef __cplusplus
}
#endif

#endif /* SUIT_PLATFORM_INTERNAL_H__ */
