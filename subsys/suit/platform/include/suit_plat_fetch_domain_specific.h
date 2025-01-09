/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SUIT_PLAT_FETCH_DOMAIN_SPECIFIC_H__
#define SUIT_PLAT_FETCH_DOMAIN_SPECIFIC_H__

#include <stdint.h>
#include <suit_types.h>
#include <suit_platform_internal.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Return destination component type supported by suit_plat_fetch for the current domain
 */
bool suit_plat_fetch_domain_specific_is_type_supported(suit_component_type_t component_type);

/** @brief Return destination component type supported by suit_plat_fetch_integrated
 *  for the current domain
 */
bool suit_plat_fetch_integrated_domain_specific_is_type_supported(
	suit_component_type_t component_type);

/**
 * @brief Domain specific part of the core part of the suit_plat_fetch function.
 */
int suit_plat_check_fetch_domain_specific(suit_component_t dst_handle,
					  suit_component_type_t dst_component_type,
					  struct zcbor_string *uri,
					  struct zcbor_string *manifest_component_id,
					  struct suit_encryption_info *enc_info);

/**
 * @brief Domain specific part of the core part of the suit_plat_fetch function.
 */
int suit_plat_fetch_domain_specific(suit_component_t dst_handle,
				    suit_component_type_t dst_component_type,
				    struct zcbor_string *uri,
				    struct zcbor_string *manifest_component_id,
				    struct suit_encryption_info *enc_info);

/**
 * @brief Domain specific part of the core part of the suit_plat_fetch_integrated function
 */
int suit_plat_check_fetch_integrated_domain_specific(suit_component_t dst_handle,
						     suit_component_type_t dst_component_type,
						     struct zcbor_string *payload,
						     struct zcbor_string *manifest_component_id,
						     struct suit_encryption_info *enc_info);

/**
 * @brief Domain specific part of the core part of the suit_plat_fetch_integrated function
 */
int suit_plat_fetch_integrated_domain_specific(suit_component_t dst_handle,
					       suit_component_type_t dst_component_type,
					       struct zcbor_string *payload,
					       struct zcbor_string *manifest_component_id,
					       struct suit_encryption_info *enc_info);

#ifdef __cplusplus
}
#endif

#endif /* SUIT_PLAT_FETCH_DOMAIN_SPECIFIC_H__ */
