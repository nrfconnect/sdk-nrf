/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SAP_PROTOCOL_H__
#define SAP_PROTOCOL_H__

/**
 * @file
 * @defgroup bt_sap_protocol Secure Application Pairing Protocol
 * @{
 * @brief Protocol constants and wire-format structures for Secure Application Pairing (SAP).
 *
 * @section bt_sap_protocol_wire Wire format
 *
 * SAP certificate:
 *
 * | Offset | Size | Field | Notes |
 * | ---: | ---: | --- | --- |
 * | 0 | 1 | version | SAP certificate version. |
 * | 1 | 1 | role_mask | Bitmask of allowed SAP roles. |
 * | 2 | 1 | device_id | Device identifier assigned by the certificate authority. |
 * | 3 | 1 | group_id | Group identifier assigned by the certificate authority. |
 * | 4 | 65 | public_key | Uncompressed P-256 identity public key. |
 * | 69 | 64 | ca_signature | ECDSA signature over bytes 0..68. |
 *
 * SAP authentication frames:
 *
 * | Offset | Size | Field | Central auth | Peripheral auth |
 * | ---: | ---: | --- | --- | --- |
 * | 0 | 1 | version | SAP_VERSION | SAP_VERSION |
 * | 1 | 1 | type | SAP_MSG_CENTRAL_AUTH | SAP_MSG_PERIPHERAL_AUTH |
 * | 2 | 133 | cert | Central certificate | Peripheral certificate |
 * | 135 | 16 | nonce | Central nonce | Peripheral nonce |
 * | 151 | 65 | ecdh_public_key | Central ECDH key | Peripheral ECDH key |
 * | 216 | 64 | signature | Central auth signature | Peripheral auth signature |
 *
 * SAP secure frame:
 *
 * | Offset | Size | Field | Notes |
 * | ---: | ---: | --- | --- |
 * | 0 | 1 | version | SAP_VERSION |
 * | 1 | 1 | type | Application message type, @ref SAP_APP_MSG_TYPE_MIN or greater. |
 * | 2 | 6 | counter_le | Little-endian AEAD frame counter. |
 * | 8 | payload + 16 | ciphertext_and_tag | AES-GCM output; bytes 0..7 are authenticated as AAD. |
 */

#include <zephyr/types.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/sys/util.h>

/** @brief SAP protocol version. */
#define SAP_VERSION 1U

/** @brief Length of SAP nonce values in bytes. */
#define SAP_NONCE_LEN		     16U
/** @brief Length of an identity private key in bytes. */
#define SAP_IDENTITY_PRIVATE_KEY_LEN 32U
/** @brief Length of an uncompressed identity public key in bytes. */
#define SAP_IDENTITY_PUBLIC_KEY_LEN  65U
/** @brief Length of an identity signature in bytes. */
#define SAP_IDENTITY_SIGNATURE_LEN   64U
/** @brief Length of an uncompressed ECDH public key in bytes. */
#define SAP_ECDH_PUBLIC_KEY_LEN	     65U
/** @brief Length of an AEAD key in bytes. */
#define SAP_AEAD_KEY_LEN	     16U
/** @brief Length of an AEAD nonce base in bytes. */
#define SAP_AEAD_NONCE_BASE_LEN	     6U
/** @brief Length of an AEAD frame counter in bytes. */
#define SAP_AEAD_COUNTER_LEN	     6U
/** @brief Length of a complete AEAD nonce in bytes. */
#define SAP_AEAD_NONCE_LEN	     12U
/** @brief Length of an AEAD tag in bytes. */
#define SAP_AEAD_TAG_LEN	     16U
/** @brief Length of the secure frame header in bytes. */
#define SAP_SECURE_HEADER_LEN	     8U
/** @brief ATT operation overhead for GATT characteristic values. */
#define SAP_ATT_VALUE_OVERHEAD	     3U

/** @brief Certificate role mask bit for central credentials. */
#define SAP_ROLE_MASK_CENTRAL	 BIT(0)
/** @brief Certificate role mask bit for peripheral credentials. */
#define SAP_ROLE_MASK_PERIPHERAL BIT(1)

/** @brief SAP GATT service UUID value. */
#define BT_UUID_SAP_SERVICE_VAL                                                                    \
	BT_UUID_128_ENCODE(0x7a18e2d1, 0x3bd2, 0x4f31, 0x8c4b, 0xb6c5b8f7a001)
/** @brief SAP authentication characteristic UUID value. */
#define BT_UUID_SAP_AUTH_VAL BT_UUID_128_ENCODE(0x7a18e2d1, 0x3bd2, 0x4f31, 0x8c4b, 0xb6c5b8f7a002)
/** @brief SAP secure TX characteristic UUID value. */
#define BT_UUID_SAP_SECURE_TX_VAL                                                                  \
	BT_UUID_128_ENCODE(0x7a18e2d1, 0x3bd2, 0x4f31, 0x8c4b, 0xb6c5b8f7a003)
/** @brief SAP secure RX characteristic UUID value. */
#define BT_UUID_SAP_SECURE_RX_VAL                                                                  \
	BT_UUID_128_ENCODE(0x7a18e2d1, 0x3bd2, 0x4f31, 0x8c4b, 0xb6c5b8f7a004)
/** @brief SAP GATT service UUID. */
#define BT_UUID_SAP_SERVICE   BT_UUID_DECLARE_128(BT_UUID_SAP_SERVICE_VAL)
/** @brief SAP authentication characteristic UUID. */
#define BT_UUID_SAP_AUTH      BT_UUID_DECLARE_128(BT_UUID_SAP_AUTH_VAL)
/** @brief SAP secure TX characteristic UUID. */
#define BT_UUID_SAP_SECURE_TX BT_UUID_DECLARE_128(BT_UUID_SAP_SECURE_TX_VAL)
/** @brief SAP secure RX characteristic UUID. */
#define BT_UUID_SAP_SECURE_RX BT_UUID_DECLARE_128(BT_UUID_SAP_SECURE_RX_VAL)

/** @brief First SAP message type value reserved for application payloads. */
#define SAP_APP_MSG_TYPE_MIN 0x80U

/** @brief SAP device role. */
enum sap_role {
	/** Central role. */
	SAP_ROLE_CENTRAL = 1,
	/** Peripheral role. */
	SAP_ROLE_PERIPHERAL = 2,
};

/** @brief SAP authentication message types. */
enum sap_message_type {
	/** Central authentication message. */
	SAP_MSG_CENTRAL_AUTH = 1,
	/** Peripheral authentication message. */
	SAP_MSG_PERIPHERAL_AUTH = 2,
};

/** @brief Transcript signature purposes. */
enum sap_signature_purpose {
	/** Signature purpose for central authentication. */
	SAP_SIG_CENTRAL_AUTH = 0xA2,
	/** Signature purpose for peripheral authentication. */
	SAP_SIG_PERIPHERAL_AUTH = 0xA3,
};

/** @brief SAP certificate body signed by the certificate authority. */
struct sap_cert_body {
	/** SAP certificate version. */
	uint8_t version;
	/** Mask of roles allowed for the credential. */
	uint8_t role_mask;
	/** Device identifier assigned by the certificate authority. */
	uint8_t device_id;
	/** Group identifier assigned by the certificate authority. */
	uint8_t group_id;
	/** Identity public key. */
	uint8_t public_key[SAP_IDENTITY_PUBLIC_KEY_LEN];
} __packed;

/** @brief SAP certificate. */
struct sap_certificate {
	/** Certificate body. */
	struct sap_cert_body body;
	/** Certificate authority signature over @ref sap_cert_body. */
	uint8_t ca_signature[SAP_IDENTITY_SIGNATURE_LEN];
} __packed;

/** @brief Central authentication message. */
struct sap_msg_central_auth {
	/** SAP protocol version. */
	uint8_t version;
	/** Message type. */
	uint8_t type;
	/** Central certificate. */
	struct sap_certificate cert;
	/** Central nonce. */
	uint8_t central_nonce[SAP_NONCE_LEN];
	/** Central ECDH public key. */
	uint8_t ecdh_public_key[SAP_ECDH_PUBLIC_KEY_LEN];
	/** Central authentication signature. */
	uint8_t signature[SAP_IDENTITY_SIGNATURE_LEN];
} __packed;

/** @brief Peripheral authentication message. */
struct sap_msg_peripheral_auth {
	/** SAP protocol version. */
	uint8_t version;
	/** Message type. */
	uint8_t type;
	/** Peripheral certificate. */
	struct sap_certificate cert;
	/** Peripheral nonce. */
	uint8_t peripheral_nonce[SAP_NONCE_LEN];
	/** Peripheral ECDH public key. */
	uint8_t ecdh_public_key[SAP_ECDH_PUBLIC_KEY_LEN];
	/** Peripheral authentication signature. */
	uint8_t signature[SAP_IDENTITY_SIGNATURE_LEN];
} __packed;

/** @brief Header for encrypted SAP secure frames. */
struct sap_secure_header {
	/** SAP protocol version. */
	uint8_t version;
	/** Message type. */
	uint8_t type;
	/** Little-endian AEAD frame counter. */
	uint8_t counter_le[SAP_AEAD_COUNTER_LEN];
} __packed;

/** @brief Maximum SAP authentication frame length. */
#define SAP_MAX_AUTH_FRAME_LEN                                                                     \
	MAX(sizeof(struct sap_msg_central_auth), sizeof(struct sap_msg_peripheral_auth))
/** @brief Maximum SAP secure frame length. */
#define SAP_MAX_SECURE_FRAME_LEN                                                                   \
	(CONFIG_BT_SAP_MAX_APP_PAYLOAD_SIZE + sizeof(struct sap_secure_header) + SAP_AEAD_TAG_LEN)
/** @brief Maximum SAP frame length. */
#define SAP_MAX_FRAME_LEN    MAX(SAP_MAX_AUTH_FRAME_LEN, SAP_MAX_SECURE_FRAME_LEN)
/** @brief Minimum negotiated ATT MTU required to carry any SAP frame. */
#define SAP_REQUIRED_ATT_MTU (SAP_MAX_FRAME_LEN + SAP_ATT_VALUE_OVERHEAD)

/**
 * @}
 */

#endif /* SAP_PROTOCOL_H__ */
