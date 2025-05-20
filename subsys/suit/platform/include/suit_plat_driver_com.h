/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SUIT_PLAT_DRIVER_COM_H__
#define SUIT_PLAT_DRIVER_COM_H__

#include <suit_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Validate component vid against expected value
 *
 * @param handle Component handle
 * @param vid_uuid Component vid
 * @return int Error code
 */
int suit_plat_com_vid_check(suit_component_t handle, struct zcbor_string *vid_uuid);

/**
 * @brief Validate component cid against expected value
 *
 * @param handle Component handle
 * @param cid_uuid Component cid
 * @return int Error code
 */
int suit_plat_com_cid_check(suit_component_t handle, struct zcbor_string *cid_uuid);

#ifdef __cplusplus
}
#endif

#endif /* SUIT_PLAT_DRIVER_COM_H__ */
