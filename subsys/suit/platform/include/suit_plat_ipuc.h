/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief Support for in-place updateable componenet.
 *
 * SUIT manifest may instruct the platform that component is inactive, by overriding
 * image-size parameter with value 0. Such component may be updated in place,
 * also memory associated to such component may be utilized for other purposes,
 * like updating SDFW or SDFW_Recovery from external flash.
 */

#ifndef SUIT_PLAT_IPUC_H__
#define SUIT_PLAT_IPUC_H__

#include <suit_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Declare component as updateable.
 *
 * @param[in] handle  Handle to the updateable component.
 *
 * @retval SUIT_ERR_UNSUPPORTED_COMPONENT_ID  if component handle is invalid.
 * @retval SUIT_ERR_OVERFLOW  if the maximum number of updateable components has been reached.
 * @retval SUIT_SUCCESS if updateable component was successfully declared.
 */
int suit_plat_ipuc_declare(suit_component_t handle);

/** @brief Revoke declaration of updateable component.
 *
 * @param[in] handle  Handle to the component that is no longer updateable.
 *
 * @retval SUIT_ERR_UNSUPPORTED_COMPONENT_ID  if component handle is invalid.
 * @retval SUIT_SUCCESS if declaration of updateable component was successfully revoked.
 */
int suit_plat_ipuc_revoke(suit_component_t handle);

/** @brief Find an updateable component ID, which is not smaller than specified size.
 *
 * @param[in] min_size  Minimal size of the updateable component.
 *
 * @returns Pointer to the updateable component ID, NULL otherwise.
 */
struct zcbor_string *suit_plat_find_sdfw_mirror_ipuc(size_t min_size);

#ifdef __cplusplus
}
#endif

#endif /* SUIT_PLAT_IPUC_H__ */
