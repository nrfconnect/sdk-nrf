/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef BT_ANCS_CLIENT_INTERNAL_H_
#define BT_ANCS_CLIENT_INTERNAL_H_

#ifdef __cplusplus
extern "C" {
#endif

enum {
	ANCS_NS_NOTIF_ENABLED,
	ANCS_DS_NOTIF_ENABLED,
	ANCS_CP_WRITE_PENDING
};

int bt_ancs_cp_write(struct bt_ancs_client *ancs_c, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif /* BT_ANCS_CLIENT_INTERNAL_H_ */
