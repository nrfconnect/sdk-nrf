/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/* This header file is used in the implementation units of the OOB message module
 * and holds information about packet structure defined in the Android Ranging
 * specification: https://source.android.com/docs/core/connect/ranging-oob-spec.
 */

/* Byte length of header fields that are defined in the OOB message. */
#define OOB_MSG_VERSION_LEN    (1U)
#define OOB_MSG_MSG_ID_LEN     (1U)
#define OOB_MSG_MSG_ID_HDR_LEN \
	(OOB_MSG_VERSION_LEN + \
	 OOB_MSG_MSG_ID_LEN)

/* Byte length of common ranging technology fields that are defined in the payload are
 * of certain OOB message types.
 */
#define OOB_MSG_RANGING_TECH_ID_LEN    (1U)
#define OOB_MSG_RANGING_TECH_SIZE_LEN  (1U)
#define OOB_MSG_RANGING_TECH_HDR_LEN   \
	(OOB_MSG_RANGING_TECH_ID_LEN + \
	 OOB_MSG_RANGING_TECH_SIZE_LEN)

/* Byte length of payload fields in the Ranging Capability Request message. */
#define OOB_MSG_RANGING_CAPABILITY_REQ_BM_LEN      (2U)
#define OOB_MSG_RANGING_CAPABILITY_REQ_PAYLOAD_LEN \
	(OOB_MSG_RANGING_CAPABILITY_REQ_BM_LEN)

/* Byte length of payload fields in the Ranging Capability Response message. */
#define OOB_MSG_RANGING_CAPABILITY_RSP_BM_LEN                  (2U)
#define OOB_MSG_RANGING_CAPABILITY_RSP_UWB_PAYLOAD_LEN         (20U)
#define OOB_MSG_RANGING_CAPABILITY_RSP_BLE_CS_PAYLOAD_LEN      (9U)
#define OOB_MSG_RANGING_CAPABILITY_RSP_WIFI_NAN_PAYLOAD_LEN    (6U)
#define OOB_MSG_RANGING_CAPABILITY_RSP_BLE_RSSI_PAYLOAD_LEN    (8U)
#define OOB_MSG_RANGING_CAPABILITY_RSP_PAYLOAD_MAX_LEN         \
	(OOB_MSG_RANGING_CAPABILITY_RSP_BM_LEN +               \
	 OOB_MSG_RANGING_CAPABILITY_RSP_UWB_PAYLOAD_LEN +      \
	 OOB_MSG_RANGING_CAPABILITY_RSP_BLE_CS_PAYLOAD_LEN +   \
	 OOB_MSG_RANGING_CAPABILITY_RSP_WIFI_NAN_PAYLOAD_LEN + \
	 OOB_MSG_RANGING_CAPABILITY_RSP_BLE_RSSI_PAYLOAD_LEN)

/* Byte length of payload fields in the Ranging Configuration message. */
#define OOB_MSG_RANGING_CONFIG_REQ_BM_LEN                      (2U)
#define OOB_MSG_RANGING_CONFIG_REQ_RFU_LEN                     (2U)
#define OOB_MSG_RANGING_CONFIG_REQ_UWB_PAYLOAD_MIN_LEN         (19U + 8U)
#define OOB_MSG_RANGING_CONFIG_REQ_UWB_PAYLOAD_MAX_LEN         (19U + 32U)
#define OOB_MSG_RANGING_CONFIG_REQ_BLE_CS_PAYLOAD_LEN          (9U)
#define OOB_MSG_RANGING_CONFIG_REQ_WIFI_NAN_PAYLOAD_MIN_LEN    (5U + 1U)
#define OOB_MSG_RANGING_CONFIG_REQ_WIFI_NAN_PAYLOAD_MAX_LEN    (5U + 255U)
#define OOB_MSG_RANGING_CONFIG_REQ_BLE_RSSI_PAYLOAD_LEN        (8U)
#define OOB_MSG_RANGING_CONFIG_REQ_PAYLOAD_MAX_LEN             \
	(OOB_MSG_RANGING_CONFIG_REQ_BM_LEN +                   \
	 OOB_MSG_RANGING_CONFIG_REQ_UWB_PAYLOAD_MAX_LEN +      \
	 OOB_MSG_RANGING_CONFIG_REQ_BLE_CS_PAYLOAD_LEN +       \
	 OOB_MSG_RANGING_CONFIG_REQ_WIFI_NAN_PAYLOAD_MAX_LEN + \
	 OOB_MSG_RANGING_CONFIG_REQ_BLE_RSSI_PAYLOAD_LEN)

/* Byte length of payload fields in the Ranging Configuration Response message. */
#define OOB_MSG_RANGING_CONFIG_RSP_BM_LEN      (2U)
#define OOB_MSG_RANGING_CONFIG_RSP_PAYLOAD_LEN \
	(OOB_MSG_RANGING_CONFIG_RSP_BM_LEN)

/* Byte length of payload fields in the Stop Ranging message. */
#define OOB_MSG_STOP_RANGING_REQ_BM_LEN      (2U)
#define OOB_MSG_STOP_RANGING_REQ_PAYLOAD_LEN \
	(OOB_MSG_STOP_RANGING_REQ_BM_LEN)

/* Byte length of payload fields in the Stop Ranging Response message. */
#define OOB_MSG_STOP_RANGING_RSP_BM_LEN      (2U)
#define OOB_MSG_STOP_RANGING_RSP_PAYLOAD_LEN \
	(OOB_MSG_STOP_RANGING_RSP_BM_LEN)
