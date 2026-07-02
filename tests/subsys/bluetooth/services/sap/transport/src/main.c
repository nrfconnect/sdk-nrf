/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>
#include <string.h>

#include <zephyr/bluetooth/conn.h>
#include <zephyr/ztest.h>
#include <psa/crypto.h>

#include "sap_internal.h"
#include "sap_crypto.h"

BUILD_ASSERT(CONFIG_BT_L2CAP_TX_MTU >= SAP_REQUIRED_ATT_MTU);

static uint8_t sent_frame[SAP_MAX_FRAME_LEN];
static size_t sent_len;
static size_t sent_count;
static uint8_t received_payload[CONFIG_BT_SAP_MAX_APP_PAYLOAD_SIZE];
static size_t received_len;
static size_t received_count;
static uint8_t received_msg_type;

static void reset_captures(void)
{
	memset(sent_frame, 0, sizeof(sent_frame));
	sent_len = 0U;
	sent_count = 0U;
	memset(received_payload, 0, sizeof(received_payload));
	received_len = 0U;
	received_count = 0U;
	received_msg_type = 0U;
}

static int capture_send_secure(struct bt_sap_session *session, uint8_t msg_type,
			       const uint8_t *data, size_t len)
{
	ARG_UNUSED(session);
	ARG_UNUSED(msg_type);

	zassert_true(len <= sizeof(sent_frame));

	memcpy(sent_frame, data, len);
	sent_len = len;
	sent_count++;

	return 0;
}

static void capture_payload(const struct bt_sap_event *event, uint8_t msg_type, const uint8_t *data,
			    size_t len)
{
	ARG_UNUSED(event);

	zassert_true(len <= sizeof(received_payload));

	received_msg_type = msg_type;
	memcpy(received_payload, data, len);
	received_len = len;
	received_count++;
}

static struct bt_sap_session *init_aead_session(struct bt_sap_context *ctx, enum sap_role role,
						bool receiver,
						const uint8_t nonce_base[SAP_AEAD_NONCE_BASE_LEN])
{
	static const uint8_t key[SAP_AEAD_KEY_LEN] = {
		0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
		0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
	};
	struct bt_sap_session *session;

	memset(ctx, 0, sizeof(*ctx));
	k_mutex_init(&ctx->lock);
	ctx->id = 1U;
	ctx->in_use = true;
	ctx->role = role;

	if (receiver) {
		ctx->callbacks.secure_payload_received = capture_payload;
	} else {
		ctx->callbacks.send_secure = capture_send_secure;
	}

	session = &ctx->sessions[0];
	session->runtime.ctx = ctx;
	session->runtime.role = role;
	session->runtime.state = BT_SAP_STATE_AUTHENTICATED;
	session->runtime.id = 1U;
	session->runtime.in_use = true;
	zassert_equal(psa_crypto_init(), PSA_SUCCESS);
	zassert_ok(sap_crypto_import_aes_gcm_key(key, sizeof(key), &session->secure.aead_key_id));
	session->secure.key_ready = true;

	if (receiver) {
		memcpy(session->secure.rx_nonce_base, nonce_base, SAP_AEAD_NONCE_BASE_LEN);
	} else {
		memcpy(session->secure.tx_nonce_base, nonce_base, SAP_AEAD_NONCE_BASE_LEN);
	}

	return session;
}

ZTEST(bt_sap_transport, test_secure_tx_and_rx_single_frame)
{
	struct bt_sap_context tx_ctx;
	struct bt_sap_context rx_ctx;
	struct bt_sap_session *tx_session;
	struct bt_sap_session *rx_session;
	uint8_t payload[100];
	const uint8_t nonce_base[SAP_AEAD_NONCE_BASE_LEN] = {0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5};
	size_t frame_len = sizeof(struct sap_secure_header) + sizeof(payload) + SAP_AEAD_TAG_LEN;
	int err;

	reset_captures();
	tx_session = init_aead_session(&tx_ctx, SAP_ROLE_CENTRAL, false, nonce_base);
	rx_session = init_aead_session(&rx_ctx, SAP_ROLE_PERIPHERAL, true, nonce_base);

	for (size_t i = 0U; i < sizeof(payload); i++) {
		payload[i] = (uint8_t)i;
	}

	err = bt_sap_send_secure(tx_session, SAP_APP_MSG_TYPE_MIN, payload, sizeof(payload));
	zassert_ok(err);
	zassert_equal(sent_count, 1U);
	zassert_equal(sent_len, frame_len);

	err = bt_sap_handle_secure_rx(rx_session, sent_frame, sent_len);
	zassert_ok(err);
	zassert_equal(received_count, 1U);
	zassert_equal(received_msg_type, SAP_APP_MSG_TYPE_MIN);
	zassert_equal(received_len, sizeof(payload));
	zassert_mem_equal(received_payload, payload, sizeof(payload));

	bt_sap_uninit(&tx_ctx);
	bt_sap_uninit(&rx_ctx);
}

ZTEST(bt_sap_transport, test_aead_secure_rx_rejects_replay_and_bad_tag)
{
	struct bt_sap_context tx_ctx;
	struct bt_sap_context rx_ctx;
	struct bt_sap_context bad_tag_ctx;
	struct bt_sap_session *tx_session;
	struct bt_sap_session *rx_session;
	struct bt_sap_session *bad_tag_session;
	const uint8_t nonce_base[SAP_AEAD_NONCE_BASE_LEN] = {0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5};
	const uint8_t payload[] = "aead-payload";
	uint8_t encrypted_frame[SAP_MAX_FRAME_LEN];
	size_t encrypted_len;
	int err;

	reset_captures();
	tx_session = init_aead_session(&tx_ctx, SAP_ROLE_CENTRAL, false, nonce_base);
	rx_session = init_aead_session(&rx_ctx, SAP_ROLE_PERIPHERAL, true, nonce_base);

	err = bt_sap_send_secure(tx_session, SAP_APP_MSG_TYPE_MIN, payload, sizeof(payload) - 1U);
	zassert_ok(err);
	memcpy(encrypted_frame, sent_frame, sent_len);
	encrypted_len = sent_len;

	err = bt_sap_handle_secure_rx(rx_session, encrypted_frame, encrypted_len);
	zassert_ok(err);
	zassert_equal(received_count, 1U);
	zassert_equal(received_msg_type, SAP_APP_MSG_TYPE_MIN);
	zassert_equal(received_len, sizeof(payload) - 1U);
	zassert_mem_equal(received_payload, payload, sizeof(payload) - 1U);

	err = bt_sap_handle_secure_rx(rx_session, encrypted_frame, encrypted_len);
	zassert_equal(err, -EACCES);
	zassert_equal(rx_session->runtime.state, BT_SAP_STATE_FAILED);

	bt_sap_uninit(&rx_ctx);

	reset_captures();
	bad_tag_session = init_aead_session(&bad_tag_ctx, SAP_ROLE_PERIPHERAL, true, nonce_base);
	encrypted_frame[encrypted_len - 1U] ^= 0x01U;
	err = bt_sap_handle_secure_rx(bad_tag_session, encrypted_frame, encrypted_len);
	zassert_equal(err, -EIO);
	zassert_equal(bad_tag_session->runtime.state, BT_SAP_STATE_FAILED);
	zassert_equal(received_count, 0U);

	bt_sap_uninit(&tx_ctx);
	bt_sap_uninit(&bad_tag_ctx);
}

ZTEST(bt_sap_transport, test_send_secure_rejects_failed_session)
{
	struct bt_sap_context ctx;
	struct bt_sap_session *session;
	const uint8_t nonce_base[SAP_AEAD_NONCE_BASE_LEN] = {0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5};
	const uint8_t payload[] = "blocked";
	int err;

	reset_captures();
	session = init_aead_session(&ctx, SAP_ROLE_CENTRAL, false, nonce_base);
	session->runtime.state = BT_SAP_STATE_FAILED;

	err = bt_sap_send_secure(session, SAP_APP_MSG_TYPE_MIN, payload, sizeof(payload) - 1U);

	zassert_equal(err, -EACCES);
	zassert_equal(sent_count, 0U);

	bt_sap_uninit(&ctx);
}

ZTEST(bt_sap_transport, test_auth_rx_rejects_malformed_frame)
{
	struct bt_sap_context ctx;
	struct bt_sap_session *session;
	uint8_t auth[] = {
		SAP_VERSION,
		SAP_MSG_CENTRAL_AUTH,
		0xaa,
		0xbb,
	};
	int err;

	memset(&ctx, 0, sizeof(ctx));
	k_mutex_init(&ctx.lock);
	ctx.id = 1U;
	ctx.in_use = true;
	session = &ctx.sessions[0];
	session->runtime.ctx = &ctx;
	session->runtime.role = SAP_ROLE_PERIPHERAL;
	session->runtime.state = BT_SAP_STATE_IDLE;
	session->runtime.id = 1U;
	session->runtime.in_use = true;

	err = bt_sap_handle_auth_rx(session, auth, sizeof(auth));
	zassert_equal(err, -EPROTO);
	zassert_equal(session->runtime.state, BT_SAP_STATE_FAILED);

	bt_sap_uninit(&ctx);
}

static void sap_transport_after(void *fixture)
{
	ARG_UNUSED(fixture);

	reset_captures();
}

ZTEST_SUITE(bt_sap_transport, NULL, NULL, NULL, NULL, sap_transport_after);
