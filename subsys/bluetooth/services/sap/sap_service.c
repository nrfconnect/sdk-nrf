/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/logging/log.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <psa/crypto.h>

#include "sap_internal.h"
#include "sap_crypto.h"

LOG_MODULE_REGISTER(sap_service, CONFIG_BT_SAP_LOG_LEVEL);

#define SAP_AEAD_COUNTER_MAX  ((1ULL << 48) - 1ULL)
#define SAP_AEAD_MATERIAL_LEN (SAP_AEAD_KEY_LEN + (2U * SAP_AEAD_NONCE_BASE_LEN))

BUILD_ASSERT(CONFIG_BT_SAP_MAX_APP_PAYLOAD_SIZE <= UINT16_MAX,
	     "SAP application payload size must fit GATT transport length fields");
BUILD_ASSERT(SAP_REQUIRED_ATT_MTU <= UINT16_MAX, "SAP ATT MTU requirement must fit uint16_t");
BUILD_ASSERT(CONFIG_BT_L2CAP_TX_MTU >= SAP_REQUIRED_ATT_MTU,
	     "SAP requires CONFIG_BT_L2CAP_TX_MTU >= SAP_REQUIRED_ATT_MTU");
BUILD_ASSERT(BT_L2CAP_RX_MTU >= SAP_REQUIRED_ATT_MTU,
	     "SAP requires CONFIG_BT_BUF_ACL_RX_SIZE large enough for SAP_REQUIRED_ATT_MTU");

#define SAP_TRACE_HEXDUMP_DBG(data, len, msg)                                                      \
	do {                                                                                       \
		if (IS_ENABLED(CONFIG_BT_SAP_UNSAFE_PROTOCOL_TRACE)) {                             \
			LOG_HEXDUMP_DBG(data, len, msg);                                           \
		}                                                                                  \
	} while (false)

struct sap_event_publication {
	struct bt_sap_cb callbacks;
	struct bt_sap_event event;
};

struct sap_event_queue {
	struct sap_event_publication *events;
	size_t capacity;
	size_t count;
};

static struct bt_sap_context sap_contexts[CONFIG_BT_SAP_MAX_CONTEXTS];
static K_MUTEX_DEFINE(sap_context_pool_lock);

static int sap_send_central_auth(struct bt_sap_session *session);

static void sap_memzero(void *mem, size_t size)
{
	volatile uint8_t *p = mem;

	while (size > 0U) {
		*p++ = 0U;
		size--;
	}
}

static size_t sap_transport_payload_max(const struct bt_sap_session *session)
{
	uint16_t mtu = 0U;

	if (session->runtime.conn != NULL) {
		mtu = bt_gatt_get_mtu(session->runtime.conn);
	}

	if (mtu == 0U) {
		mtu = CONFIG_BT_L2CAP_TX_MTU;
	}

	if (mtu <= SAP_ATT_VALUE_OVERHEAD) {
		return 0U;
	}

	return (size_t)mtu - SAP_ATT_VALUE_OVERHEAD;
}

static int sap_send_auth_frame(struct bt_sap_session *session, uint8_t msg_type,
			       const uint8_t *frame, size_t frame_len)
{
	if (frame_len == 0U || frame_len > SAP_MAX_AUTH_FRAME_LEN) {
		return -EMSGSIZE;
	}

	if (sap_transport_payload_max(session) < SAP_MAX_FRAME_LEN) {
		return -EMSGSIZE;
	}

	return session->runtime.ctx->callbacks.send_auth(session, msg_type, frame, frame_len);
}

static int sap_send_secure_frame(struct bt_sap_session *session, uint8_t msg_type,
				 const uint8_t *frame, size_t frame_len)
{
	if (frame_len == 0U || frame_len > SAP_MAX_SECURE_FRAME_LEN) {
		return -EMSGSIZE;
	}

	if (sap_transport_payload_max(session) < SAP_MAX_FRAME_LEN) {
		return -EMSGSIZE;
	}

	return session->runtime.ctx->callbacks.send_secure(session, msg_type, frame, frame_len);
}

static struct bt_sap_event sap_event_snapshot(const struct bt_sap_session *session,
					      enum bt_sap_event_type type, int reason)
{
	return (struct bt_sap_event){
		.type = type,
		.role = session->runtime.role,
		.state = session->runtime.state,
		.session_id = session->runtime.id,
		.user_data = session->runtime.user_data,
		.peer_device_id = session->auth.peer_cert.body.device_id,
		.peer_group_id = session->auth.peer_cert.body.group_id,
		.authenticated = session->runtime.state == BT_SAP_STATE_AUTHENTICATED,
		.reason = reason,
	};
}

static void sap_event_queue_add(struct sap_event_queue *queue, struct bt_sap_session *session,
				enum bt_sap_event_type type, int reason)
{
	struct bt_sap_context *ctx = session->runtime.ctx;
	struct sap_event_publication *publication;

	if (queue == NULL || ctx == NULL) {
		return;
	}

	if (queue->count >= queue->capacity) {
		LOG_WRN("SAP event queue full; dropping event %u", type);
		return;
	}

	publication = &queue->events[queue->count++];
	publication->callbacks = ctx->callbacks;
	publication->event = sap_event_snapshot(session, type, reason);
}

static void sap_publish_event(const struct sap_event_publication *publication)
{
	const struct bt_sap_event *event = &publication->event;
	const struct bt_sap_cb *callbacks = &publication->callbacks;

	switch (event->type) {
	case BT_SAP_EVENT_AUTHENTICATED:
		if (callbacks->authenticated != NULL) {
			callbacks->authenticated(event);
		}
		break;
	case BT_SAP_EVENT_FAILED:
		if (callbacks->failed != NULL) {
			callbacks->failed(event);
		}
		break;
	case BT_SAP_EVENT_DISCONNECTED:
		if (callbacks->disconnected != NULL) {
			callbacks->disconnected(event);
		}
		break;
	default:
		break;
	}
}

static void sap_event_queue_publish(struct sap_event_queue *queue)
{
	for (size_t i = 0U; i < queue->count; i++) {
		sap_publish_event(&queue->events[i]);
	}

	queue->count = 0U;
}

static void sap_reset_session(struct bt_sap_session *session, struct sap_event_queue *events)
{
	if (session->runtime.in_use) {
		session->runtime.state = BT_SAP_STATE_IDLE;
		sap_event_queue_add(events, session, BT_SAP_EVENT_DISCONNECTED, 0);
	}

	(void)psa_destroy_key(session->auth.local_ecdh_key_id);
	(void)psa_destroy_key(session->secure.aead_key_id);

	if (session->runtime.conn != NULL) {
		bt_conn_unref(session->runtime.conn);
	}

	memset(session, 0, sizeof(*session));
}

static void sap_fail(struct bt_sap_session *session, int reason, struct sap_event_queue *events)
{
	if (session->runtime.state == BT_SAP_STATE_FAILED) {
		return;
	}

	session->runtime.state = BT_SAP_STATE_FAILED;
	session->secure.key_ready = false;

	sap_event_queue_add(events, session, BT_SAP_EVENT_FAILED, reason);
}

static void sap_notify_authenticated(struct bt_sap_session *session, const char *debug_msg,
				     struct sap_event_queue *events)
{
	session->runtime.state = BT_SAP_STATE_AUTHENTICATED;

	if (debug_msg != NULL) {
		LOG_DBG("%s", debug_msg);
	}

	if (session->runtime.authenticated_notified) {
		return;
	}

	session->runtime.authenticated_notified = true;
	sap_event_queue_add(events, session, BT_SAP_EVENT_AUTHENTICATED, 0);
}

int bt_sap_credential_from_bytes(struct bt_sap_device_credential *credential,
				 const uint8_t *private_key, size_t private_key_len,
				 const uint8_t *cert, size_t cert_len)
{
	if (credential == NULL || private_key == NULL || cert == NULL) {
		return -EINVAL;
	}

	if (private_key_len != sizeof(credential->private_key) ||
	    cert_len != sizeof(credential->cert)) {
		return -EINVAL;
	}

	memset(credential, 0, sizeof(*credential));
	memcpy(credential->private_key, private_key, private_key_len);
	memcpy(&credential->cert, cert, cert_len);

	return 0;
}

static int sap_verify_certificate(struct bt_sap_session *session,
				  const struct sap_certificate *cert, uint8_t expected_role_mask)
{
	int err;

	if (cert->body.version != SAP_VERSION) {
		return -EPROTO;
	}

	if ((cert->body.role_mask & expected_role_mask) == 0U) {
		return -EACCES;
	}

	if (cert->body.group_id != session->runtime.ctx->policy.expected_group_id) {
		return -EACCES;
	}

	if ((expected_role_mask == SAP_ROLE_MASK_CENTRAL) &&
	    (cert->body.device_id != session->runtime.ctx->policy.allowed_central_id)) {
		return -EACCES;
	}

	err = sap_crypto_verify_identity(session->runtime.ctx->policy.ca_public_key,
					 session->runtime.ctx->policy.ca_public_key_len,
					 (const uint8_t *)&cert->body, sizeof(cert->body),
					 cert->ca_signature, sizeof(cert->ca_signature));
	return err;
}

static void sap_build_nonce(uint8_t *nonce, const uint8_t *base, uint64_t counter)
{
	struct net_buf_simple buf;

	net_buf_simple_init_with_data(&buf, nonce, SAP_AEAD_NONCE_LEN);
	net_buf_simple_reset(&buf);
	net_buf_simple_add_mem(&buf, base, SAP_AEAD_NONCE_BASE_LEN);
	net_buf_simple_add_le48(&buf, counter);
}

static int sap_make_central_auth_sig(const struct bt_sap_session *session, uint8_t *buffer,
				     size_t size, size_t *len)
{
	struct net_buf_simple transcript;
	const bool central_role = session->runtime.role == SAP_ROLE_CENTRAL;
	const uint8_t *central_nonce =
		central_role ? session->auth.local_nonce : session->auth.peer_nonce;
	const struct sap_cert_body *central_cert =
		central_role ? &session->runtime.ctx->policy.local_credential->cert.body
			     : &session->auth.peer_cert.body;
	const uint8_t *central_ecdh_public =
		central_role ? session->auth.local_ecdh_public : session->auth.peer_ecdh_public;
	size_t central_ecdh_public_len = central_role ? session->auth.local_ecdh_public_len
						      : session->auth.peer_ecdh_public_len;
	size_t needed = 1U + SAP_NONCE_LEN + sizeof(struct sap_cert_body) + SAP_ECDH_PUBLIC_KEY_LEN;

	if (central_ecdh_public_len > SAP_ECDH_PUBLIC_KEY_LEN) {
		return -EMSGSIZE;
	}

	if (size < needed) {
		return -ENOMEM;
	}

	net_buf_simple_init_with_data(&transcript, buffer, size);
	net_buf_simple_reset(&transcript);
	net_buf_simple_add_u8(&transcript, SAP_SIG_CENTRAL_AUTH);
	net_buf_simple_add_mem(&transcript, central_nonce, SAP_NONCE_LEN);
	net_buf_simple_add_mem(&transcript, central_cert, sizeof(struct sap_cert_body));
	net_buf_simple_add_mem(&transcript, central_ecdh_public, central_ecdh_public_len);

	*len = transcript.len;
	return 0;
}

static int sap_make_peripheral_auth_sig(const struct bt_sap_session *session, uint8_t *buffer,
					size_t size, size_t *len)
{
	struct net_buf_simple transcript;
	const bool central_role = session->runtime.role == SAP_ROLE_CENTRAL;
	const uint8_t *central_nonce =
		central_role ? session->auth.local_nonce : session->auth.peer_nonce;
	const uint8_t *peripheral_nonce =
		central_role ? session->auth.peer_nonce : session->auth.local_nonce;
	const struct sap_cert_body *central_cert =
		central_role ? &session->runtime.ctx->policy.local_credential->cert.body
			     : &session->auth.peer_cert.body;
	const struct sap_cert_body *peripheral_cert =
		central_role ? &session->auth.peer_cert.body
			     : &session->runtime.ctx->policy.local_credential->cert.body;
	const uint8_t *central_ecdh_public =
		central_role ? session->auth.local_ecdh_public : session->auth.peer_ecdh_public;
	const uint8_t *peripheral_ecdh_public =
		central_role ? session->auth.peer_ecdh_public : session->auth.local_ecdh_public;
	size_t central_ecdh_public_len = central_role ? session->auth.local_ecdh_public_len
						      : session->auth.peer_ecdh_public_len;
	size_t peripheral_ecdh_public_len = central_role ? session->auth.peer_ecdh_public_len
							 : session->auth.local_ecdh_public_len;
	size_t needed = 1U + SAP_NONCE_LEN + SAP_NONCE_LEN + sizeof(struct sap_cert_body) +
			sizeof(struct sap_cert_body) + SAP_ECDH_PUBLIC_KEY_LEN +
			SAP_ECDH_PUBLIC_KEY_LEN;

	if (central_ecdh_public_len > SAP_ECDH_PUBLIC_KEY_LEN ||
	    peripheral_ecdh_public_len > SAP_ECDH_PUBLIC_KEY_LEN) {
		return -EMSGSIZE;
	}

	if (size < needed) {
		return -ENOMEM;
	}

	net_buf_simple_init_with_data(&transcript, buffer, size);
	net_buf_simple_reset(&transcript);
	net_buf_simple_add_u8(&transcript, SAP_SIG_PERIPHERAL_AUTH);
	net_buf_simple_add_mem(&transcript, central_nonce, SAP_NONCE_LEN);
	net_buf_simple_add_mem(&transcript, peripheral_nonce, SAP_NONCE_LEN);
	net_buf_simple_add_mem(&transcript, central_cert, sizeof(struct sap_cert_body));
	net_buf_simple_add_mem(&transcript, peripheral_cert, sizeof(struct sap_cert_body));
	net_buf_simple_add_mem(&transcript, central_ecdh_public, central_ecdh_public_len);
	net_buf_simple_add_mem(&transcript, peripheral_ecdh_public, peripheral_ecdh_public_len);

	*len = transcript.len;
	return 0;
}

static int sap_derive_session_keys(struct bt_sap_session *session)
{
	uint8_t secret[32];
	uint8_t salt[SAP_NONCE_LEN * 2U];
	uint8_t transcript[2U + (2U * SAP_ECDH_PUBLIC_KEY_LEN)];
	uint8_t transcript_hash[32];
	uint8_t info[(sizeof("SAP session") - 1U) + sizeof(transcript_hash)];
	uint8_t material[SAP_AEAD_MATERIAL_LEN];
	struct net_buf_simple salt_buf;
	struct net_buf_simple transcript_buf;
	struct net_buf_simple info_buf;
	const bool central_role = session->runtime.role == SAP_ROLE_CENTRAL;
	const uint8_t *central_nonce =
		central_role ? session->auth.local_nonce : session->auth.peer_nonce;
	const uint8_t *peripheral_nonce =
		central_role ? session->auth.peer_nonce : session->auth.local_nonce;
	const struct sap_cert_body *central_cert =
		central_role ? &session->runtime.ctx->policy.local_credential->cert.body
			     : &session->auth.peer_cert.body;
	const struct sap_cert_body *peripheral_cert =
		central_role ? &session->auth.peer_cert.body
			     : &session->runtime.ctx->policy.local_credential->cert.body;
	const uint8_t *central_ecdh_public =
		central_role ? session->auth.local_ecdh_public : session->auth.peer_ecdh_public;
	const uint8_t *peripheral_ecdh_public =
		central_role ? session->auth.peer_ecdh_public : session->auth.local_ecdh_public;
	const uint8_t *central_nonce_base = &material[SAP_AEAD_KEY_LEN];
	const uint8_t *peripheral_nonce_base =
		&material[SAP_AEAD_KEY_LEN + SAP_AEAD_NONCE_BASE_LEN];
	size_t central_ecdh_public_len = central_role ? session->auth.local_ecdh_public_len
						      : session->auth.peer_ecdh_public_len;
	size_t peripheral_ecdh_public_len = central_role ? session->auth.peer_ecdh_public_len
							 : session->auth.local_ecdh_public_len;
	size_t secret_len;
	size_t hash_len;
	psa_status_t status;
	int err;

	if (central_ecdh_public_len > SAP_ECDH_PUBLIC_KEY_LEN ||
	    peripheral_ecdh_public_len > SAP_ECDH_PUBLIC_KEY_LEN) {
		return -EMSGSIZE;
	}

	/* P-256 ECDH shared-secret calculation over the peer public key. */
	status = psa_raw_key_agreement(
		PSA_ALG_ECDH, session->auth.local_ecdh_key_id, session->auth.peer_ecdh_public,
		session->auth.peer_ecdh_public_len, secret, sizeof(secret), &secret_len);
	if (status != PSA_SUCCESS) {
		LOG_ERR("SAP shared secret derivation failed (%d)", status);
		return -EIO;
	}

	net_buf_simple_init_with_data(&salt_buf, salt, sizeof(salt));
	net_buf_simple_reset(&salt_buf);
	net_buf_simple_add_mem(&salt_buf, central_nonce, SAP_NONCE_LEN);
	net_buf_simple_add_mem(&salt_buf, peripheral_nonce, SAP_NONCE_LEN);

	net_buf_simple_init_with_data(&transcript_buf, transcript, sizeof(transcript));
	net_buf_simple_reset(&transcript_buf);
	net_buf_simple_add_u8(&transcript_buf, central_cert->device_id);
	net_buf_simple_add_u8(&transcript_buf, peripheral_cert->device_id);
	net_buf_simple_add_mem(&transcript_buf, central_ecdh_public, central_ecdh_public_len);
	net_buf_simple_add_mem(&transcript_buf, peripheral_ecdh_public, peripheral_ecdh_public_len);

	/* Some CRACEN-backed HKDF implementations limit the info field to 128 bytes. */
	status = psa_hash_compute(PSA_ALG_SHA_256, transcript_buf.data, transcript_buf.len,
				  transcript_hash, sizeof(transcript_hash), &hash_len);
	if (status != PSA_SUCCESS || hash_len != sizeof(transcript_hash)) {
		LOG_ERR("SAP transcript hash failed (%d)", status);
		sap_memzero(secret, sizeof(secret));
		return -EIO;
	}

	net_buf_simple_init_with_data(&info_buf, info, sizeof(info));
	net_buf_simple_reset(&info_buf);
	net_buf_simple_add_mem(&info_buf, "SAP session", sizeof("SAP session") - 1U);
	net_buf_simple_add_mem(&info_buf, transcript_hash, sizeof(transcript_hash));

	err = sap_crypto_hkdf_sha256((struct sap_crypto_const_buffer){secret, secret_len},
				     (struct sap_crypto_const_buffer){salt_buf.data, salt_buf.len},
				     (struct sap_crypto_const_buffer){info_buf.data, info_buf.len},
				     (struct sap_crypto_buffer){material, sizeof(material)});
	sap_memzero(secret, sizeof(secret));
	if (err != 0) {
		LOG_ERR("SAP HKDF failed (%d)", err);
		return err;
	}

	err = sap_crypto_import_aes_gcm_key(material, SAP_AEAD_KEY_LEN,
					    &session->secure.aead_key_id);
	if (err != 0) {
		LOG_ERR("SAP AEAD key import failed (%d)", err);
		sap_memzero(material, sizeof(material));
		return err;
	}

	if (session->runtime.role == SAP_ROLE_CENTRAL) {
		memcpy(session->secure.tx_nonce_base, central_nonce_base,
		       sizeof(session->secure.tx_nonce_base));
		memcpy(session->secure.rx_nonce_base, peripheral_nonce_base,
		       sizeof(session->secure.rx_nonce_base));
	} else {
		memcpy(session->secure.tx_nonce_base, peripheral_nonce_base,
		       sizeof(session->secure.tx_nonce_base));
		memcpy(session->secure.rx_nonce_base, central_nonce_base,
		       sizeof(session->secure.rx_nonce_base));
	}

	sap_memzero(material, sizeof(material));
	session->secure.key_ready = true;
	session->secure.tx_counter = 0U;
	session->secure.rx_counter = 0U;
	LOG_DBG("SAP session material ready: role=%u peer=%u", session->runtime.role,
		session->auth.peer_cert.body.device_id);

	return 0;
}

static int sap_encrypt_internal(struct bt_sap_session *session, uint8_t msg_type, uint64_t counter,
				struct sap_crypto_const_buffer payload,
				struct sap_crypto_buffer output, size_t *out_len)
{
	struct net_buf_simple frame;
	uint8_t nonce[SAP_AEAD_NONCE_LEN];
	size_t cipher_len;
	uint8_t *ciphertext;
	psa_status_t status;

	if (!session->secure.key_ready) {
		return -EACCES;
	}

	if (output.len < sizeof(struct sap_secure_header) + payload.len + SAP_AEAD_TAG_LEN) {
		return -ENOMEM;
	}

	if (counter > SAP_AEAD_COUNTER_MAX) {
		return -EOVERFLOW;
	}

	net_buf_simple_init_with_data(&frame, output.data, output.len);
	net_buf_simple_reset(&frame);
	net_buf_simple_add_u8(&frame, SAP_VERSION);
	net_buf_simple_add_u8(&frame, msg_type);
	net_buf_simple_add_le48(&frame, counter);
	sap_build_nonce(nonce, session->secure.tx_nonce_base, counter);

	ciphertext = net_buf_simple_add(&frame, payload.len + SAP_AEAD_TAG_LEN);
	status = psa_aead_encrypt(session->secure.aead_key_id, PSA_ALG_GCM, nonce, sizeof(nonce),
				  frame.data, sizeof(struct sap_secure_header), payload.data,
				  payload.len, ciphertext, payload.len + SAP_AEAD_TAG_LEN,
				  &cipher_len);
	if (status != PSA_SUCCESS || cipher_len != payload.len + SAP_AEAD_TAG_LEN) {
		return -EIO;
	}

	*out_len = sizeof(struct sap_secure_header) + cipher_len;

	return 0;
}

static int sap_decrypt_internal(struct bt_sap_session *session, const uint8_t *data, size_t len,
				uint8_t *msg_type, uint8_t *out, size_t out_size, size_t *out_len)
{
	uint8_t nonce[SAP_AEAD_NONCE_LEN];
	uint64_t counter;
	psa_status_t status;

	if (len < sizeof(struct sap_secure_header) + SAP_AEAD_TAG_LEN) {
		return -EMSGSIZE;
	}

	*msg_type = data[1];
	counter = sys_get_le48(&data[2]);

	if (data[0] != SAP_VERSION) {
		return -EPROTO;
	}

	if (counter != session->secure.rx_counter) {
		return -EACCES;
	}

	sap_build_nonce(nonce, session->secure.rx_nonce_base, counter);
	status = psa_aead_decrypt(session->secure.aead_key_id, PSA_ALG_GCM, nonce, sizeof(nonce),
				  data, sizeof(struct sap_secure_header),
				  &data[sizeof(struct sap_secure_header)],
				  len - sizeof(struct sap_secure_header), out, out_size, out_len);
	if (status != PSA_SUCCESS) {
		return -EIO;
	}

	session->secure.rx_counter++;
	return 0;
}

static int sap_policy_validate(enum sap_role role, const struct bt_sap_policy *policy,
			       const struct bt_sap_cb *callbacks)
{
	if (policy == NULL || callbacks == NULL) {
		return -EINVAL;
	}

	if (role != SAP_ROLE_CENTRAL && role != SAP_ROLE_PERIPHERAL) {
		return -EINVAL;
	}

	if (policy->local_credential == NULL || policy->ca_public_key == NULL ||
	    policy->ca_public_key_len != SAP_IDENTITY_PUBLIC_KEY_LEN) {
		return -EINVAL;
	}

	if (callbacks->send_auth == NULL || callbacks->send_secure == NULL) {
		return -EINVAL;
	}

	return 0;
}

int bt_sap_init(struct bt_sap_context **ctx_out, enum sap_role role,
		const struct bt_sap_policy *policy, const struct bt_sap_cb *callbacks)
{
	struct bt_sap_context *ctx = NULL;
	int err;

	if (ctx_out == NULL) {
		return -EINVAL;
	}

	*ctx_out = NULL;

	err = sap_policy_validate(role, policy, callbacks);
	if (err != 0) {
		return err;
	}

	if (psa_crypto_init() != PSA_SUCCESS) {
		return -EIO;
	}

	k_mutex_lock(&sap_context_pool_lock, K_FOREVER);
	for (size_t i = 0U; i < ARRAY_SIZE(sap_contexts); i++) {
		if (!sap_contexts[i].in_use) {
			ctx = &sap_contexts[i];
			memset(ctx, 0, sizeof(*ctx));
			ctx->id = (uint8_t)(i + 1U);
			ctx->in_use = true;
			k_mutex_init(&ctx->lock);
			break;
		}
	}
	k_mutex_unlock(&sap_context_pool_lock);

	if (ctx == NULL) {
		return -ENOMEM;
	}

	ctx->role = role;
	ctx->policy = *policy;
	ctx->callbacks = *callbacks;

	err = sap_crypto_import_identity_private(policy->local_credential->private_key,
						 sizeof(policy->local_credential->private_key),
						 &ctx->local_sign_key_id);
	if (err != 0) {
		k_mutex_lock(&sap_context_pool_lock, K_FOREVER);
		memset(ctx, 0, sizeof(*ctx));
		k_mutex_unlock(&sap_context_pool_lock);
		return err;
	}

	*ctx_out = ctx;

	return 0;
}

void bt_sap_uninit(struct bt_sap_context *ctx)
{
	struct sap_event_publication event_storage[CONFIG_BT_SAP_MAX_PEERS];
	struct sap_event_queue events = {
		.events = event_storage,
		.capacity = ARRAY_SIZE(event_storage),
	};
	size_t i;

	if (ctx == NULL) {
		return;
	}

	k_mutex_lock(&ctx->lock, K_FOREVER);

	for (i = 0; i < ARRAY_SIZE(ctx->sessions); i++) {
		sap_reset_session(&ctx->sessions[i], &events);
	}

	(void)psa_destroy_key(ctx->local_sign_key_id);
	ctx->local_sign_key_id = 0;

	k_mutex_unlock(&ctx->lock);

	sap_event_queue_publish(&events);

	k_mutex_lock(&sap_context_pool_lock, K_FOREVER);
	ctx->in_use = false;
	k_mutex_unlock(&sap_context_pool_lock);
}

struct bt_sap_session *bt_sap_on_connected(struct bt_sap_context *ctx, struct bt_conn *conn)
{
	size_t i;

	if (ctx == NULL || conn == NULL) {
		return NULL;
	}

	k_mutex_lock(&ctx->lock, K_FOREVER);
	for (i = 0; i < ARRAY_SIZE(ctx->sessions); i++) {
		if (!ctx->sessions[i].runtime.in_use) {
			ctx->sessions[i].runtime.ctx = ctx;
			ctx->sessions[i].runtime.conn = bt_conn_ref(conn);
			ctx->sessions[i].runtime.role = ctx->role;
			ctx->sessions[i].runtime.id = (uint8_t)(i + 1U);
			ctx->sessions[i].runtime.in_use = true;
			ctx->sessions[i].runtime.state = BT_SAP_STATE_IDLE;
			LOG_DBG("SAP session allocated: role=%u", ctx->role);
			k_mutex_unlock(&ctx->lock);
			return &ctx->sessions[i];
		}
	}

	k_mutex_unlock(&ctx->lock);
	return NULL;
}

static struct bt_sap_session *sap_session_from_conn_unlocked(struct bt_sap_context *ctx,
							     struct bt_conn *conn)
{
	for (size_t i = 0; i < ARRAY_SIZE(ctx->sessions); i++) {
		if (ctx->sessions[i].runtime.in_use && (ctx->sessions[i].runtime.conn == conn)) {
			return &ctx->sessions[i];
		}
	}

	return NULL;
}

void bt_sap_on_disconnected(struct bt_sap_context *ctx, struct bt_conn *conn)
{
	struct sap_event_publication event_storage[1];
	struct sap_event_queue events = {
		.events = event_storage,
		.capacity = ARRAY_SIZE(event_storage),
	};
	struct bt_sap_session *session;

	if (ctx == NULL || conn == NULL) {
		return;
	}

	k_mutex_lock(&ctx->lock, K_FOREVER);
	session = sap_session_from_conn_unlocked(ctx, conn);

	if (session != NULL) {
		LOG_DBG("SAP session released: role=%u peer=%u", session->runtime.role,
			session->auth.peer_cert.body.device_id);
		sap_reset_session(session, &events);
	}
	k_mutex_unlock(&ctx->lock);

	sap_event_queue_publish(&events);
}

struct bt_sap_session *bt_sap_session_from_conn(struct bt_sap_context *ctx, struct bt_conn *conn)
{
	struct bt_sap_session *session;

	if (ctx == NULL || conn == NULL) {
		return NULL;
	}

	k_mutex_lock(&ctx->lock, K_FOREVER);
	session = sap_session_from_conn_unlocked(ctx, conn);
	k_mutex_unlock(&ctx->lock);

	return session;
}

void bt_sap_session_set_user_data(struct bt_sap_session *session, void *user_data)
{
	struct bt_sap_context *ctx;

	if (session == NULL || session->runtime.ctx == NULL) {
		return;
	}

	ctx = session->runtime.ctx;
	k_mutex_lock(&ctx->lock, K_FOREVER);
	if (session->runtime.ctx == ctx) {
		session->runtime.user_data = user_data;
	}
	k_mutex_unlock(&ctx->lock);
}

int bt_sap_start(struct bt_sap_session *session)
{
	struct bt_sap_context *ctx;
	int err;

	if (session == NULL || session->runtime.ctx == NULL) {
		return -EINVAL;
	}

	ctx = session->runtime.ctx;
	k_mutex_lock(&ctx->lock, K_FOREVER);

	if (session->runtime.ctx != ctx) {
		err = -EINVAL;
		goto out;
	}

	if (session->runtime.role != SAP_ROLE_CENTRAL) {
		err = -ENOTSUP;
		goto out;
	}

	if (session->runtime.state != BT_SAP_STATE_IDLE) {
		err = -EPROTO;
		goto out;
	}

	err = sap_send_central_auth(session);

out:
	k_mutex_unlock(&ctx->lock);
	return err;
}

static int sap_send_central_auth(struct bt_sap_session *session)
{
	uint8_t msg_buf[sizeof(struct sap_msg_central_auth)];
	struct net_buf_simple msg;
	uint8_t sign_buf[320];
	uint8_t *signature;
	size_t sign_len;
	size_t sig_len = 0U;
	psa_status_t status;
	int err;

	if (psa_generate_random(session->auth.local_nonce, sizeof(session->auth.local_nonce)) !=
	    PSA_SUCCESS) {
		return -EIO;
	}

	err = sap_crypto_generate_ecdh_keypair(&session->auth.local_ecdh_key_id);
	if (err != 0) {
		LOG_ERR("SAP central auth key generation failed (%d)", err);
		return err;
	}
	LOG_DBG("SAP sending central auth");

	status = psa_export_public_key(
		session->auth.local_ecdh_key_id, session->auth.local_ecdh_public,
		sizeof(session->auth.local_ecdh_public), &session->auth.local_ecdh_public_len);
	if (status != PSA_SUCCESS) {
		LOG_ERR("SAP central auth public key export failed (%d)", status);
		return -EIO;
	}

	/* PSA exports P-256 public keys as 65-byte uncompressed SEC1 points. */
	if (session->auth.local_ecdh_public_len != SAP_ECDH_PUBLIC_KEY_LEN) {
		LOG_ERR("SAP central auth public key has unexpected length (%zu)",
			session->auth.local_ecdh_public_len);
		return -EMSGSIZE;
	}

	net_buf_simple_init_with_data(&msg, msg_buf, sizeof(msg_buf));
	net_buf_simple_reset(&msg);
	net_buf_simple_add_u8(&msg, SAP_VERSION);
	net_buf_simple_add_u8(&msg, SAP_MSG_CENTRAL_AUTH);
	net_buf_simple_add_mem(&msg, &session->runtime.ctx->policy.local_credential->cert,
			       sizeof(struct sap_certificate));
	net_buf_simple_add_mem(&msg, session->auth.local_nonce, SAP_NONCE_LEN);
	net_buf_simple_add_mem(&msg, session->auth.local_ecdh_public, SAP_ECDH_PUBLIC_KEY_LEN);
	signature = net_buf_simple_add(&msg, SAP_IDENTITY_SIGNATURE_LEN);

	err = sap_make_central_auth_sig(session, sign_buf, sizeof(sign_buf), &sign_len);
	if (err != 0) {
		LOG_ERR("SAP central auth transcript build failed (%d)", err);
		return err;
	}

	err = sap_crypto_sign_identity(session->runtime.ctx->local_sign_key_id, sign_buf, sign_len,
				       signature, SAP_IDENTITY_SIGNATURE_LEN, &sig_len);
	if (err != 0 || sig_len != SAP_IDENTITY_SIGNATURE_LEN) {
		LOG_ERR("SAP central auth signing failed (%d, sig_len=%zu)", err, sig_len);
		return -EIO;
	}

	session->runtime.state = BT_SAP_STATE_WAIT_PERIPHERAL_AUTH;
	LOG_DBG("SAP central auth sent: device_id=%u role_mask=0x%02x",
		session->runtime.ctx->policy.local_credential->cert.body.device_id,
		session->runtime.ctx->policy.local_credential->cert.body.role_mask);
	SAP_TRACE_HEXDUMP_DBG(msg.data, msg.len, "SAP auth tx");
	err = sap_send_auth_frame(session, SAP_MSG_CENTRAL_AUTH, msg.data, msg.len);
	if (err != 0) {
		session->runtime.state = BT_SAP_STATE_IDLE;
	}

	return err;
}

static int sap_send_peripheral_auth(struct bt_sap_session *session, struct sap_event_queue *events)
{
	uint8_t msg_buf[sizeof(struct sap_msg_peripheral_auth)];
	struct net_buf_simple msg;
	uint8_t sign_buf[384];
	uint8_t *signature;
	size_t sign_len;
	size_t sig_len = 0U;
	psa_status_t status;
	int err;

	if (psa_generate_random(session->auth.local_nonce, sizeof(session->auth.local_nonce)) !=
	    PSA_SUCCESS) {
		return -EIO;
	}

	err = sap_crypto_generate_ecdh_keypair(&session->auth.local_ecdh_key_id);
	if (err != 0) {
		LOG_ERR("SAP peripheral auth key generation failed (%d)", err);
		return err;
	}
	LOG_DBG("SAP sending peripheral auth");

	status = psa_export_public_key(
		session->auth.local_ecdh_key_id, session->auth.local_ecdh_public,
		sizeof(session->auth.local_ecdh_public), &session->auth.local_ecdh_public_len);
	if (status != PSA_SUCCESS) {
		LOG_ERR("SAP peripheral auth public key export failed (%d)", status);
		return -EIO;
	}

	/* PSA exports P-256 public keys as 65-byte uncompressed SEC1 points. */
	if (session->auth.local_ecdh_public_len != SAP_ECDH_PUBLIC_KEY_LEN) {
		LOG_ERR("SAP peripheral auth public key has unexpected length (%zu)",
			session->auth.local_ecdh_public_len);
		return -EMSGSIZE;
	}

	err = sap_derive_session_keys(session);
	if (err != 0) {
		LOG_ERR("SAP peripheral auth session key derivation failed (%d)", err);
		return err;
	}

	net_buf_simple_init_with_data(&msg, msg_buf, sizeof(msg_buf));
	net_buf_simple_reset(&msg);
	net_buf_simple_add_u8(&msg, SAP_VERSION);
	net_buf_simple_add_u8(&msg, SAP_MSG_PERIPHERAL_AUTH);
	net_buf_simple_add_mem(&msg, &session->runtime.ctx->policy.local_credential->cert,
			       sizeof(struct sap_certificate));
	net_buf_simple_add_mem(&msg, session->auth.local_nonce, SAP_NONCE_LEN);
	net_buf_simple_add_mem(&msg, session->auth.local_ecdh_public, SAP_ECDH_PUBLIC_KEY_LEN);
	signature = net_buf_simple_add(&msg, SAP_IDENTITY_SIGNATURE_LEN);

	err = sap_make_peripheral_auth_sig(session, sign_buf, sizeof(sign_buf), &sign_len);
	if (err != 0) {
		LOG_ERR("SAP peripheral auth transcript build failed (%d)", err);
		return err;
	}

	err = sap_crypto_sign_identity(session->runtime.ctx->local_sign_key_id, sign_buf, sign_len,
				       signature, SAP_IDENTITY_SIGNATURE_LEN, &sig_len);
	if (err != 0 || sig_len != SAP_IDENTITY_SIGNATURE_LEN) {
		LOG_ERR("SAP peripheral auth signing failed (%d, sig_len=%zu)", err, sig_len);
		return -EIO;
	}

	LOG_DBG("SAP peripheral auth sent: device_id=%u",
		session->runtime.ctx->policy.local_credential->cert.body.device_id);
	SAP_TRACE_HEXDUMP_DBG(msg.data, msg.len, "SAP auth tx");
	err = sap_send_auth_frame(session, SAP_MSG_PERIPHERAL_AUTH, msg.data, msg.len);
	if (err != 0) {
		LOG_ERR("SAP peripheral auth send failed (%d, len=%zu)", err, (size_t)msg.len);
		return err;
	}

	sap_notify_authenticated(session, "SAP authenticated: peripheral auth sent", events);
	return 0;
}

static int sap_handle_central_auth(struct bt_sap_session *session, struct net_buf_simple *msg,
				   struct sap_event_queue *events)
{
	const struct sap_certificate *cert;
	const uint8_t *central_nonce;
	const uint8_t *ecdh_public_key;
	const uint8_t *signature;
	uint8_t sign_buf[320];
	size_t sign_len;
	int err;

	if (session->runtime.role != SAP_ROLE_PERIPHERAL ||
	    session->runtime.state != BT_SAP_STATE_IDLE) {
		return -EPROTO;
	}

	if (msg->len != sizeof(struct sap_msg_central_auth) - 2U) {
		return -EPROTO;
	}

	cert = net_buf_simple_pull_mem(msg, sizeof(*cert));
	central_nonce = net_buf_simple_pull_mem(msg, SAP_NONCE_LEN);
	ecdh_public_key = net_buf_simple_pull_mem(msg, SAP_ECDH_PUBLIC_KEY_LEN);
	signature = net_buf_simple_pull_mem(msg, SAP_IDENTITY_SIGNATURE_LEN);

	err = sap_verify_certificate(session, cert, SAP_ROLE_MASK_CENTRAL);
	if (err != 0) {
		LOG_ERR("SAP central auth certificate verify failed (%d)", err);
		return err;
	}
	LOG_DBG("SAP received central auth");

	session->auth.peer_cert = *cert;
	memcpy(session->auth.peer_nonce, central_nonce, SAP_NONCE_LEN);
	memcpy(session->auth.peer_ecdh_public, ecdh_public_key, SAP_ECDH_PUBLIC_KEY_LEN);
	session->auth.peer_ecdh_public_len = SAP_ECDH_PUBLIC_KEY_LEN;

	err = sap_make_central_auth_sig(session, sign_buf, sizeof(sign_buf), &sign_len);
	if (err != 0) {
		LOG_ERR("SAP central auth transcript build failed on peripheral (%d)", err);
		return err;
	}

	err = sap_crypto_verify_identity(cert->body.public_key, sizeof(cert->body.public_key),
					 sign_buf, sign_len, signature, SAP_IDENTITY_SIGNATURE_LEN);
	if (err != 0) {
		LOG_ERR("SAP central auth transcript verify failed on peripheral (%d)", err);
		return err;
	}
	LOG_DBG("SAP central auth verified: device_id=%u group=0x%02x",
		session->auth.peer_cert.body.device_id, session->auth.peer_cert.body.group_id);

	return sap_send_peripheral_auth(session, events);
}

static int sap_handle_peripheral_auth(struct bt_sap_session *session, struct net_buf_simple *msg,
				      struct sap_event_queue *events)
{
	const struct sap_certificate *cert;
	const uint8_t *peripheral_nonce;
	const uint8_t *ecdh_public_key;
	const uint8_t *signature;
	uint8_t sign_buf[384];
	size_t sign_len;
	int err;

	if (session->runtime.state != BT_SAP_STATE_WAIT_PERIPHERAL_AUTH) {
		return -EPROTO;
	}

	if (msg->len != sizeof(struct sap_msg_peripheral_auth) - 2U) {
		return -EPROTO;
	}

	cert = net_buf_simple_pull_mem(msg, sizeof(*cert));
	peripheral_nonce = net_buf_simple_pull_mem(msg, SAP_NONCE_LEN);
	ecdh_public_key = net_buf_simple_pull_mem(msg, SAP_ECDH_PUBLIC_KEY_LEN);
	signature = net_buf_simple_pull_mem(msg, SAP_IDENTITY_SIGNATURE_LEN);

	err = sap_verify_certificate(session, cert, SAP_ROLE_MASK_PERIPHERAL);
	if (err != 0) {
		LOG_ERR("SAP peripheral auth certificate verify failed (%d)", err);
		return err;
	}

	session->auth.peer_cert = *cert;
	memcpy(session->auth.peer_nonce, peripheral_nonce, SAP_NONCE_LEN);
	memcpy(session->auth.peer_ecdh_public, ecdh_public_key, SAP_ECDH_PUBLIC_KEY_LEN);
	session->auth.peer_ecdh_public_len = SAP_ECDH_PUBLIC_KEY_LEN;
	LOG_DBG("SAP received peripheral auth");

	err = sap_make_peripheral_auth_sig(session, sign_buf, sizeof(sign_buf), &sign_len);
	if (err != 0) {
		return err;
	}

	err = sap_crypto_verify_identity(session->auth.peer_cert.body.public_key,
					 sizeof(session->auth.peer_cert.body.public_key), sign_buf,
					 sign_len, signature, SAP_IDENTITY_SIGNATURE_LEN);
	if (err != 0) {
		return err;
	}
	LOG_DBG("SAP peripheral auth verified");

	err = sap_derive_session_keys(session);
	if (err != 0) {
		return err;
	}

	sap_notify_authenticated(session, "SAP authenticated: peripheral auth verified", events);
	return 0;
}

int bt_sap_handle_auth_rx(struct bt_sap_session *session, const uint8_t *data, size_t len)
{
	struct sap_event_publication event_storage[2];
	struct sap_event_queue events = {
		.events = event_storage,
		.capacity = ARRAY_SIZE(event_storage),
	};
	struct bt_sap_context *ctx;
	struct net_buf_simple msg;
	uint8_t msg_data[SAP_MAX_AUTH_FRAME_LEN];
	int err = -EPROTO;
	uint8_t version;
	uint8_t type;

	if (session == NULL || data == NULL || session->runtime.ctx == NULL) {
		return -EINVAL;
	}

	ctx = session->runtime.ctx;
	k_mutex_lock(&ctx->lock, K_FOREVER);
	if (session->runtime.ctx != ctx) {
		err = -EINVAL;
		goto out;
	}

	if (sap_transport_payload_max(session) < SAP_MAX_FRAME_LEN ||
	    len > SAP_MAX_AUTH_FRAME_LEN) {
		SAP_TRACE_HEXDUMP_DBG(data, len, "SAP auth rx");
		sap_fail(session, -EMSGSIZE, &events);
		err = -EMSGSIZE;
		goto out;
	}

	if (len < 2U) {
		SAP_TRACE_HEXDUMP_DBG(data, len, "SAP auth rx");
		sap_fail(session, -EMSGSIZE, &events);
		err = -EMSGSIZE;
		goto out;
	}

	memcpy(msg_data, data, len);
	net_buf_simple_init_with_data(&msg, msg_data, len);
	version = net_buf_simple_pull_u8(&msg);
	type = net_buf_simple_pull_u8(&msg);
	SAP_TRACE_HEXDUMP_DBG(data, len, "SAP auth rx");

	if (version != SAP_VERSION) {
		sap_fail(session, -EPROTO, &events);
		err = -EPROTO;
		goto out;
	}

	switch (type) {
	case SAP_MSG_CENTRAL_AUTH:
		err = sap_handle_central_auth(session, &msg, &events);
		break;
	case SAP_MSG_PERIPHERAL_AUTH:
		err = sap_handle_peripheral_auth(session, &msg, &events);
		break;
	default:
		err = -EPROTO;
		break;
	}

	if (err != 0) {
		sap_fail(session, err, &events);
	}

out:
	k_mutex_unlock(&ctx->lock);
	sap_event_queue_publish(&events);
	return err;
}

int bt_sap_send_secure(struct bt_sap_session *session, uint8_t msg_type, const uint8_t *payload,
		       size_t len)
{
	struct bt_sap_context *ctx;
	uint8_t buffer[SAP_MAX_FRAME_LEN];
	size_t out_len;
	uint64_t counter;
	int err;

	if (session == NULL || session->runtime.ctx == NULL || (payload == NULL && len != 0U)) {
		return -EINVAL;
	}

	ctx = session->runtime.ctx;
	k_mutex_lock(&ctx->lock, K_FOREVER);

	if (session->runtime.ctx != ctx) {
		err = -EINVAL;
		goto out;
	}

	if (msg_type < SAP_APP_MSG_TYPE_MIN) {
		err = -EPROTO;
		goto out;
	}

	if (session->runtime.state != BT_SAP_STATE_AUTHENTICATED || !session->secure.key_ready) {
		err = -EACCES;
		goto out;
	}

	if (len > CONFIG_BT_SAP_MAX_APP_PAYLOAD_SIZE) {
		err = -EMSGSIZE;
		goto out;
	}

	counter = session->secure.tx_counter;
	err = sap_encrypt_internal(session, msg_type, counter,
				   (struct sap_crypto_const_buffer){payload, len},
				   (struct sap_crypto_buffer){buffer, sizeof(buffer)}, &out_len);
	if (err != 0) {
		goto out;
	}

	SAP_TRACE_HEXDUMP_DBG(buffer, out_len, "SAP secure tx");

	err = sap_send_secure_frame(session, msg_type, buffer, out_len);
	if (err == 0) {
		session->secure.tx_counter = counter + 1U;
	}

out:
	k_mutex_unlock(&ctx->lock);
	return err;
}

int bt_sap_handle_secure_rx(struct bt_sap_session *session, const uint8_t *data, size_t len)
{
	struct sap_event_publication event_storage[2];
	struct sap_event_queue events = {
		.events = event_storage,
		.capacity = ARRAY_SIZE(event_storage),
	};
	struct bt_sap_context *ctx;
	void (*payload_cb)(const struct bt_sap_event *event, uint8_t msg_type,
			   const uint8_t *payload, size_t payload_len) = NULL;
	struct bt_sap_event payload_event = {0};
	uint8_t plaintext[CONFIG_BT_SAP_MAX_APP_PAYLOAD_SIZE];
	uint8_t msg_type;
	size_t plaintext_len;
	int err;

	if (session == NULL || data == NULL || session->runtime.ctx == NULL) {
		return -EINVAL;
	}

	ctx = session->runtime.ctx;
	k_mutex_lock(&ctx->lock, K_FOREVER);
	if (session->runtime.ctx != ctx) {
		err = -EINVAL;
		goto out;
	}

	if (sap_transport_payload_max(session) < SAP_MAX_FRAME_LEN ||
	    len > SAP_MAX_SECURE_FRAME_LEN) {
		sap_fail(session, -EMSGSIZE, &events);
		err = -EMSGSIZE;
		goto out;
	}

	SAP_TRACE_HEXDUMP_DBG(data, len, "SAP secure rx");

	if (session->runtime.state != BT_SAP_STATE_AUTHENTICATED || !session->secure.key_ready) {
		sap_fail(session, -EACCES, &events);
		err = -EACCES;
		goto out;
	}

	err = sap_decrypt_internal(session, data, len, &msg_type, plaintext, sizeof(plaintext),
				   &plaintext_len);
	if (err != 0) {
		sap_fail(session, err, &events);
		goto out;
	}

	if (msg_type < SAP_APP_MSG_TYPE_MIN) {
		err = -EPROTO;
	} else {
		payload_cb = ctx->callbacks.secure_payload_received;
		payload_event = sap_event_snapshot(session, BT_SAP_EVENT_PAYLOAD_RECEIVED, 0);
		err = 0;
	}

	if (err != 0) {
		sap_fail(session, err, &events);
	}

out:
	k_mutex_unlock(&ctx->lock);
	sap_event_queue_publish(&events);
	if (err == 0 && payload_cb != NULL) {
		payload_cb(&payload_event, msg_type, plaintext, plaintext_len);
	}
	sap_memzero(plaintext, sizeof(plaintext));
	return err;
}
