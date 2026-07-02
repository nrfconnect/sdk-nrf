/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SAP_INTERNAL_H__
#define SAP_INTERNAL_H__

#include <psa/crypto.h>
#include <zephyr/kernel.h>
#include <zephyr/bluetooth/conn.h>

#include <bluetooth/services/sap.h>

struct sap_session_runtime {
	struct bt_sap_context *ctx;
	struct bt_conn *conn;
	void *user_data;
	enum sap_role role;
	enum bt_sap_session_state state;
	uint8_t id;
	bool in_use;
	bool authenticated_notified;
};

struct sap_session_auth {
	struct sap_certificate peer_cert;
	uint8_t local_nonce[SAP_NONCE_LEN];
	uint8_t peer_nonce[SAP_NONCE_LEN];
	uint8_t local_ecdh_public[SAP_ECDH_PUBLIC_KEY_LEN];
	size_t local_ecdh_public_len;
	uint8_t peer_ecdh_public[SAP_ECDH_PUBLIC_KEY_LEN];
	size_t peer_ecdh_public_len;
	psa_key_id_t local_ecdh_key_id;
};

struct sap_session_secure {
	bool key_ready;
	uint8_t tx_nonce_base[SAP_AEAD_NONCE_BASE_LEN];
	uint8_t rx_nonce_base[SAP_AEAD_NONCE_BASE_LEN];
	uint64_t tx_counter;
	uint64_t rx_counter;
	psa_key_id_t aead_key_id;
};

struct bt_sap_session {
	struct sap_session_runtime runtime;
	struct sap_session_auth auth;
	struct sap_session_secure secure;
};

struct bt_sap_context {
	struct k_mutex lock;
	enum sap_role role;
	struct bt_sap_policy policy;
	struct bt_sap_cb callbacks;
	struct bt_sap_session sessions[CONFIG_BT_SAP_MAX_PEERS];
	psa_key_id_t local_sign_key_id;
	uint8_t id;
	bool in_use;
};

#endif /* SAP_INTERNAL_H__ */
