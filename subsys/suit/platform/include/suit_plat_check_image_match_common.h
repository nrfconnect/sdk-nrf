/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SUIT_PLAT_CHECK_IMAGE_MATCH_COMMON_H__
#define SUIT_PLAT_CHECK_IMAGE_MATCH_COMMON_H__

#include <stdint.h>
#include <suit_types.h>
#include <suit_platform_internal.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief suit_plat_check_image_match for "memory mapped" components.
 *        Calculates the digest of the memory area represented by @p component
 *        and compares it to the digest in @p digest.
 */
int suit_plat_check_image_match_mem_mapped(suit_component_t component,
					   enum suit_cose_alg alg_id,
					   struct zcbor_string *digest);

/**
 * @brief suit_plat_check_image_match for manifest components.
 *        Retrieves the envelope from the implementation data of @p component,
 *        extracts the manifest digest from the envelope,
 *        and compares it to the digest in @p digest.
 */
int suit_plat_check_image_match_mfst(suit_component_t component, enum suit_cose_alg alg_id,
				     struct zcbor_string *digest);

#ifdef __cplusplus
}
#endif

#endif /* SUIT_PLAT_CHECK_IMAGE_MATCH_COMMON_H__ */
