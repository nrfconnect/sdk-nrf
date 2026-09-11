/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SAP_DEMO_PROTOCOL_H__
#define SAP_DEMO_PROTOCOL_H__

#include <zephyr/bluetooth/uuid.h>

#include <bluetooth/services/sap_protocol.h>

#define BT_UUID_SAP_DEMO_PROTECTED_SERVICE_VAL                                                     \
	BT_UUID_128_ENCODE(0x7a18e2d1, 0x3bd2, 0x4f31, 0x8c4b, 0xb6c5b8f7a101)
#define BT_UUID_SAP_DEMO_PROTECTED_STATUS_VAL                                                      \
	BT_UUID_128_ENCODE(0x7a18e2d1, 0x3bd2, 0x4f31, 0x8c4b, 0xb6c5b8f7a102)

#define BT_UUID_SAP_DEMO_PROTECTED_SERVICE                                                         \
	BT_UUID_DECLARE_128(BT_UUID_SAP_DEMO_PROTECTED_SERVICE_VAL)
#define BT_UUID_SAP_DEMO_PROTECTED_STATUS BT_UUID_DECLARE_128(BT_UUID_SAP_DEMO_PROTECTED_STATUS_VAL)

enum sap_demo_message_type {
	SAP_DEMO_MSG_TEXT = SAP_APP_MSG_TYPE_MIN,
};

#endif /* SAP_DEMO_PROTOCOL_H__ */
