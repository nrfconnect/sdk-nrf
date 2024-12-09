/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SUIT_PLAT_CHECK_CONTENT_DOMAIN_SPECIFIC_H__
#define SUIT_PLAT_CHECK_CONTENT_DOMAIN_SPECIFIC_H__

#include <stdint.h>
#include <suit_types.h>
#include <suit_platform_internal.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Check if component type is supported by suit_plat_check_content for the current domain.
 */
bool suit_plat_check_content_domain_specific_is_type_supported(
	suit_component_type_t component_type);

/**
 * @brief Domain specific part of the core part of the suit_plat_check_content function.
 */
int suit_plat_check_content_domain_specific(suit_component_t handle,
					    suit_component_type_t component_type,
					    struct zcbor_string *content);

#ifdef __cplusplus
}
#endif

#endif /* SUIT_PLAT_CHECK_CONTENT_DOMAIN_SPECIFIC_H__ */
