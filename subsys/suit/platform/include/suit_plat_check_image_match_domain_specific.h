/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SUIT_PLAT_CHECK_IMAGE_MATCH_DOMAIN_SPECIFIC_H__
#define SUIT_PLAT_CHECK_IMAGE_MATCH_DOMAIN_SPECIFIC_H__

#include <stdint.h>
#include <suit_types.h>
#include <suit_platform_internal.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Domain specific part of the of the suit_plat_check_image_match function
 */
int suit_plat_check_image_match_domain_specific(suit_component_t component,
						enum suit_cose_alg alg_id,
						struct zcbor_string *digest,
						struct zcbor_string *component_id,
						suit_component_type_t component_type);

#ifdef __cplusplus
}
#endif

#endif /* SUIT_PLAT_CHECK_IMAGE_MATCH_DOMAIN_SPECIFIC_H__ */
