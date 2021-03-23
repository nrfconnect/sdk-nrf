/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef BT_ANCS_APP_ATTR_GET_H__
#define BT_ANCS_APP_ATTR_GET_H__

/** @file
 *
 * @addtogroup bt_ancs_client
 * @{
 */

#include <bluetooth/services/ancs_client.h>

#ifdef __cplusplus
extern "C" {
#endif

/**@brief Function for requesting attributes for an app.
 *
 * @param[in] ancs_c  iOS notification structure. This structure must be supplied by
 *                    the application. It identifies the particular client instance to use.
 * @param[in] app_id  App identifier of the app for which to request app attributes.
 * @param[in] len     Length of the app identifier.
 *
 * @retval 0 If all operations were successful.
 *           Otherwise, a (negative) error code is returned.
 */
int bt_ancs_app_attr_request(struct bt_ancs_client *ancs_c,
			     const uint8_t *app_id, uint32_t len);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* BT_ANCS_APP_ATTR_GET_H__ */
