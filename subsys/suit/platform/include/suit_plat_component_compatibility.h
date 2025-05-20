/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SUIT_PLAT_CHECK_COMPONENT_COMPATIBILITY_H__
#define SUIT_PLAT_CHECK_COMPONENT_COMPATIBILITY_H__

#include <suit_metadata.h>
#include <zcbor_common.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Check component ID and manifest class ID against supported manifests
 *
 * @param class_id Class ID to be checked
 * @param component_id Component ID to be checked
 * @return int ) in case of success, otherwise error code
 */
int suit_plat_component_compatibility_check(const suit_manifest_class_id_t *class_id,
					    struct zcbor_string *component_id);

#ifdef __cplusplus
}
#endif

#endif /* SUIT_PLAT_CHECK_COMPONENT_COMPATIBILITY_H__ */
