/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef BT_SAP_DEMO_CREDENTIALS_H_
#define BT_SAP_DEMO_CREDENTIALS_H_

#include <zephyr/types.h>

#include <bluetooth/services/sap.h>

const struct bt_sap_device_credential *demo_credentials_select(enum sap_role role);
const uint8_t *demo_credentials_ca_public_key(size_t *len);

#endif /* BT_SAP_DEMO_CREDENTIALS_H_ */
