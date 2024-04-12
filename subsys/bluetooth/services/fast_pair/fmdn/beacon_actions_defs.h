/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/* This configuration file is included only once from the Beacon Actions module
 * and holds information about packet structure exchanged by the Beacon Actions
 * characteristic.
 */

/* This structure enforces the header file is included only once in the build.
 * Violating this requirement triggers a multiple definition error at link time.
 */
const struct {} beacon_actions_defs_include_once;

/* Byte length of fields in data frames exchanged by the Beacon Actions characteristic. */
#define BEACON_ACTIONS_RANDOM_NONCE_LEN      CONFIG_BT_FAST_PAIR_FMDN_RANDOM_NONCE_LEN
#define BEACON_ACTIONS_DATA_ID_LEN           1
#define BEACON_ACTIONS_DATA_LENGTH_LEN       1
#define BEACON_ACTIONS_ONE_TIME_AUTH_KEY_LEN CONFIG_BT_FAST_PAIR_FMDN_AUTH_SEG_LEN
#define BEACON_ACTIONS_HEADER_LEN       \
	(BEACON_ACTIONS_DATA_ID_LEN +   \
	BEACON_ACTIONS_DATA_LENGTH_LEN)
#define BEACON_ACTIONS_MAJOR_VERSION_LEN 1

/* Byte length of fields in the Beacon Actions Read response. */
#define BEACON_ACTIONS_READ_RSP_VERSION_LEN BEACON_ACTIONS_MAJOR_VERSION_LEN
#define BEACON_ACTIONS_READ_RSP_LEN            \
	(BEACON_ACTIONS_READ_RSP_VERSION_LEN + \
	BEACON_ACTIONS_RANDOM_NONCE_LEN)

/* Byte length of fields in the Beacon Actions response. */
#define BEACON_ACTIONS_RSP_AUTH_SEG_LEN CONFIG_BT_FAST_PAIR_FMDN_AUTH_SEG_LEN

/* Byte length of fields in the Beacon Parameters request. */
#define BEACON_PARAMETERS_REQ_PAYLOAD_LEN BEACON_ACTIONS_ONE_TIME_AUTH_KEY_LEN

/* Byte length of fields in the Beacon Parameters response. */
#define BEACON_PARAMETERS_RSP_TX_POWER_LEN     1
#define BEACON_PARAMETERS_RSP_CLOCK_LEN        4
#define BEACON_PARAMETERS_RSP_ECC_TYPE_LEN     1
#define BEACON_PARAMETERS_RSP_RINGING_COMP_LEN 1
#define BEACON_PARAMETERS_RSP_RINGING_CAP_LEN  1
#define BEACON_PARAMETERS_RSP_PADDING_LEN      8
#define BEACON_PARAMETERS_RSP_ADD_DATA_LEN       \
	(BEACON_PARAMETERS_RSP_TX_POWER_LEN +    \
	BEACON_PARAMETERS_RSP_CLOCK_LEN +        \
	BEACON_PARAMETERS_RSP_ECC_TYPE_LEN +     \
	BEACON_PARAMETERS_RSP_RINGING_COMP_LEN + \
	BEACON_PARAMETERS_RSP_RINGING_CAP_LEN +  \
	BEACON_PARAMETERS_RSP_PADDING_LEN)
#define BEACON_PARAMETERS_RSP_PAYLOAD_LEN  \
	(BEACON_ACTIONS_RSP_AUTH_SEG_LEN + \
	BEACON_PARAMETERS_RSP_ADD_DATA_LEN)
#define BEACON_PARAMETERS_RSP_LEN          \
	(BEACON_ACTIONS_HEADER_LEN +       \
	BEACON_PARAMETERS_RSP_PAYLOAD_LEN)

/* Values used in the Beacon Parameters response. */
#define BEACON_PARAMETERS_RSP_PADDING_VAL 0x00

/* Byte length of fields in the Provisioning State request. */
#define PROVISIONING_STATE_REQ_PAYLOAD_LEN BEACON_ACTIONS_ONE_TIME_AUTH_KEY_LEN

/* Byte length of fields in the Provisioning State response. */
#define PROVISIONING_STATE_RSP_BITFIELD_LEN 1
#define PROVISIONING_STATE_RSP_EID_LEN      FP_FMDN_STATE_EID_LEN
#define PROVISIONING_STATE_RSP_ADD_DATA_LEN    \
	(PROVISIONING_STATE_RSP_BITFIELD_LEN + \
	PROVISIONING_STATE_RSP_EID_LEN)
#define PROVISIONING_STATE_RSP_PAYLOAD_LEN   \
	(BEACON_ACTIONS_RSP_AUTH_SEG_LEN +   \
	PROVISIONING_STATE_RSP_ADD_DATA_LEN)
#define PROVISIONING_STATE_RSP_LEN          \
	(BEACON_ACTIONS_HEADER_LEN +        \
	PROVISIONING_STATE_RSP_PAYLOAD_LEN)

/* Byte length of the EIK hash field from the Ephemeral Identity Key Set/Clear request. */
#define EPHEMERAL_IDENTITY_KEY_REQ_EIK_HASH_LEN 8

/* Byte length of fields in the Ephemeral Identity Key Set request. */
#define EPHEMERAL_IDENTITY_KEY_SET_REQ_EIK_LEN 32
#define EPHEMERAL_IDENTITY_KEY_SET_REQ_UNPROVISIONED_PAYLOAD_LEN \
	(BEACON_ACTIONS_ONE_TIME_AUTH_KEY_LEN +                  \
	EPHEMERAL_IDENTITY_KEY_SET_REQ_EIK_LEN)
#define EPHEMERAL_IDENTITY_KEY_SET_REQ_PROVISIONED_PAYLOAD_LEN      \
	(EPHEMERAL_IDENTITY_KEY_SET_REQ_UNPROVISIONED_PAYLOAD_LEN + \
	EPHEMERAL_IDENTITY_KEY_REQ_EIK_HASH_LEN)

/* Byte length of fields in the Ephemeral Identity Key Set response. */
#define EPHEMERAL_IDENTITY_KEY_SET_RSP_PAYLOAD_LEN \
	(BEACON_ACTIONS_RSP_AUTH_SEG_LEN)
#define EPHEMERAL_IDENTITY_KEY_SET_RSP_LEN \
	(BEACON_ACTIONS_HEADER_LEN +       \
	EPHEMERAL_IDENTITY_KEY_SET_RSP_PAYLOAD_LEN)

/* Byte length of fields in the Ephemeral Identity Key Clear request. */
#define EPHEMERAL_IDENTITY_KEY_CLEAR_REQ_PAYLOAD_LEN \
	(BEACON_ACTIONS_ONE_TIME_AUTH_KEY_LEN +      \
	EPHEMERAL_IDENTITY_KEY_REQ_EIK_HASH_LEN)

/* Byte length of fields in the Ephemeral Identity Key Clear response. */
#define EPHEMERAL_IDENTITY_KEY_CLEAR_RSP_PAYLOAD_LEN \
	(BEACON_ACTIONS_RSP_AUTH_SEG_LEN)
#define EPHEMERAL_IDENTITY_KEY_CLEAR_RSP_LEN \
	(BEACON_ACTIONS_HEADER_LEN +         \
	EPHEMERAL_IDENTITY_KEY_CLEAR_RSP_PAYLOAD_LEN)

/* Byte length of fields in the Ephemeral Identity Key Read request. */
#define EPHEMERAL_IDENTITY_KEY_READ_REQ_PAYLOAD_LEN \
	BEACON_ACTIONS_ONE_TIME_AUTH_KEY_LEN

/* Byte length of fields in the Ephemeral Identity Key Read response. */
#define EPHEMERAL_IDENTITY_KEY_READ_RSP_EIK_LEN 32
#define EPHEMERAL_IDENTITY_KEY_READ_RSP_ADD_DATA_LEN \
	(EPHEMERAL_IDENTITY_KEY_READ_RSP_EIK_LEN)
#define EPHEMERAL_IDENTITY_KEY_READ_RSP_PAYLOAD_LEN \
	(BEACON_ACTIONS_RSP_AUTH_SEG_LEN +          \
	EPHEMERAL_IDENTITY_KEY_READ_RSP_ADD_DATA_LEN)
#define EPHEMERAL_IDENTITY_KEY_READ_RSP_LEN \
	(BEACON_ACTIONS_HEADER_LEN +        \
	EPHEMERAL_IDENTITY_KEY_READ_RSP_PAYLOAD_LEN)

/* Byte length of fields in the Ring request. */
#define RING_REQ_STATE_LEN   1
#define RING_REQ_TIMEOUT_LEN 2
#define RING_REQ_VOLUME_LEN  1
#define RING_REQ_PAYLOAD_LEN                    \
	(BEACON_ACTIONS_ONE_TIME_AUTH_KEY_LEN + \
	RING_REQ_STATE_LEN +                    \
	RING_REQ_TIMEOUT_LEN +                  \
	RING_REQ_VOLUME_LEN)

/* Byte length of fields in the Ring response. */
#define RING_RSP_STATUS_LEN      1
#define RING_RSP_STATE_LEN       1
#define RING_RSP_REM_TIMEOUT_LEN 2
#define RING_RSP_ADD_DATA_LEN  \
	(RING_RSP_STATUS_LEN + \
	RING_RSP_STATE_LEN +   \
	RING_RSP_REM_TIMEOUT_LEN)
#define RING_RSP_PAYLOAD_LEN               \
	(BEACON_ACTIONS_RSP_AUTH_SEG_LEN + \
	RING_RSP_ADD_DATA_LEN)
#define RING_RSP_LEN                 \
	(BEACON_ACTIONS_HEADER_LEN + \
	RING_RSP_PAYLOAD_LEN)

/* Byte length of fields in the Ringing State Read request. */
#define RINGING_STATE_READ_REQ_PAYLOAD_LEN \
	(BEACON_ACTIONS_ONE_TIME_AUTH_KEY_LEN)

/* Byte length of fields in the Ringing State Read response. */
#define RINGING_STATE_READ_RSP_STATE_LEN       1
#define RINGING_STATE_READ_RSP_REM_TIMEOUT_LEN 2
#define RINGING_STATE_READ_RSP_ADD_DATA_LEN \
	(RINGING_STATE_READ_RSP_STATE_LEN + \
	RINGING_STATE_READ_RSP_REM_TIMEOUT_LEN)
#define RINGING_STATE_READ_RSP_PAYLOAD_LEN \
	(BEACON_ACTIONS_RSP_AUTH_SEG_LEN + \
	RINGING_STATE_READ_RSP_ADD_DATA_LEN)
#define RINGING_STATE_READ_RSP_LEN   \
	(BEACON_ACTIONS_HEADER_LEN + \
	RINGING_STATE_READ_RSP_PAYLOAD_LEN)

/* Byte length of fields in the Activate Unwanted Tracking Protection mode request. */
#define ACTIVATE_UTP_MODE_REQ_CONTROL_FLAGS_LEN 1
#define ACTIVATE_UTP_MODE_REQ_PAYLOAD_LEN       \
	(BEACON_ACTIONS_ONE_TIME_AUTH_KEY_LEN + \
	 ACTIVATE_UTP_MODE_REQ_CONTROL_FLAGS_LEN)

/* Byte length of fields in the Activate Unwanted Tracking Protection mode response. */
#define ACTIVATE_UTP_MODE_RSP_PAYLOAD_LEN \
	(BEACON_ACTIONS_RSP_AUTH_SEG_LEN)
#define ACTIVATE_UTP_MODE_RSP_LEN    \
	(BEACON_ACTIONS_HEADER_LEN + \
	ACTIVATE_UTP_MODE_RSP_PAYLOAD_LEN)

/* Byte length of fields in the Deactivate Unwanted Tracking Protection mode request. */
#define DEACTIVATE_UTP_MODE_REQ_EIK_HASH_LEN 8
#define DEACTIVATE_UTP_MODE_REQ_PAYLOAD_LEN     \
	(BEACON_ACTIONS_ONE_TIME_AUTH_KEY_LEN + \
	DEACTIVATE_UTP_MODE_REQ_EIK_HASH_LEN)

/* Byte length of fields in the Deactivate Unwanted Tracking Protection mode response. */
#define DEACTIVATE_UTP_MODE_RSP_PAYLOAD_LEN \
	(BEACON_ACTIONS_RSP_AUTH_SEG_LEN)
#define DEACTIVATE_UTP_MODE_RSP_LEN  \
	(BEACON_ACTIONS_HEADER_LEN + \
	DEACTIVATE_UTP_MODE_RSP_PAYLOAD_LEN)
