/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SUIT_PLAT_RETRIEVE_MANIFEST_DOMAIN_SPECIFIC_H__
#define SUIT_PLAT_RETRIEVE_MANIFEST_DOMAIN_SPECIFIC_H__

#include <stdint.h>
#include <suit_types.h>
#include <suit_platform_internal.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Domain specific part of the of the suit_plat_retrieve_manifest function
 */
int suit_plat_retrieve_manifest_domain_specific(struct zcbor_string *component_id,
						suit_component_type_t component_type,
						const uint8_t **envelope_str, size_t *envelope_len);


#ifdef __cplusplus
}
#endif

#endif /* SUIT_PLAT_RETRIEVE_MANIFEST_DOMAIN_SPECIFIC_H__ */
