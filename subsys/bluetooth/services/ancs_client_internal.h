/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef BT_ANCS_CLIENT_INTERNAL_H_
#define BT_ANCS_CLIENT_INTERNAL_H_

#include <bluetooth/services/ancs_client.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
	ANCS_NS_NOTIF_ENABLED,
	ANCS_DS_NOTIF_ENABLED,
	ANCS_CP_WRITE_PENDING
};

/**@brief Call Notification Source notification callback function.
 *
 * @param[in] ancs_c    ANCS client instance.
 * @param[in] err       0 if the notification is valid.
 *                      Otherwise, contains a (negative) error code.
 * @param[in] notif     iOS notification structure.
 */
static inline void bt_ancs_do_ns_notif_cb(struct bt_ancs_client *ancs_c, int err,
					  const struct bt_ancs_evt_notif *notif)
{
	if (ancs_c->ns_notif_cb) {
		ancs_c->ns_notif_cb(ancs_c, err, notif);
	}
}

/**@brief Call Data Source notification callback function.
 *
 * @param[in] ancs_c    ANCS client instance.
 * @param[in] response  Attribute response structure.
 */
static inline void bt_ancs_do_ds_notif_cb(struct bt_ancs_client *ancs_c,
					  const struct bt_ancs_attr_response *response)
{
	if (ancs_c->ds_notif_cb) {
		ancs_c->ds_notif_cb(ancs_c, response);
	}
}

/**@brief Internal function for writing to ANCS Control Point.
 *
 * The caller is expected to set the ANCS_CP_WRITE_PENDING and to set up
 * the Control Point data buffer before calling this function.
 *
 * @param[in] ancs_c  ANCS client instance.
 * @param[in] len     Length of data in Control Point data buffer.
 * @param[in] func    Callback function for handling NP write response.
 *
 * @retval 0 If the operation is successful.
 *           Otherwise, a (negative) error code is returned.
 */
int bt_ancs_cp_write(struct bt_ancs_client *ancs_c, uint16_t len,
		     bt_ancs_write_cb func);

#ifdef __cplusplus
}
#endif

#endif /* BT_ANCS_CLIENT_INTERNAL_H_ */
