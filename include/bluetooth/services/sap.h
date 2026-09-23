/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef BT_SAP_H_
#define BT_SAP_H_

/**
 * @file
 * @defgroup bt_sap Secure Application Pairing
 * @{
 * @brief API for the Secure Application Pairing (SAP) library.
 */

#include <stdbool.h>
#include <zephyr/bluetooth/conn.h>

#include <bluetooth/services/sap_protocol.h>

struct bt_sap_context;
struct bt_sap_session;

/** @brief Local SAP device credential. */
struct bt_sap_device_credential {
	/** Identity private key. */
	uint8_t private_key[SAP_IDENTITY_PRIVATE_KEY_LEN];
	/** Device certificate matching @ref private_key. */
	struct sap_certificate cert;
};

/** @brief SAP authentication session state. */
enum bt_sap_session_state {
	/** Session is allocated and no authentication exchange is active. */
	BT_SAP_STATE_IDLE = 0,
	/** Central is waiting for the peripheral authentication message. */
	BT_SAP_STATE_WAIT_PERIPHERAL_AUTH,
	/** Session is authenticated. */
	BT_SAP_STATE_AUTHENTICATED,
	/** Session failed authentication or secure frame handling. */
	BT_SAP_STATE_FAILED,
};

/** @brief SAP event type. */
enum bt_sap_event_type {
	/** SAP authentication completed. */
	BT_SAP_EVENT_AUTHENTICATED = 0,
	/** SAP authentication or secure frame handling failed. */
	BT_SAP_EVENT_FAILED,
	/** SAP session disconnected or was released. */
	BT_SAP_EVENT_DISCONNECTED,
	/** Authenticated application payload received. */
	BT_SAP_EVENT_PAYLOAD_RECEIVED,
};

/** @brief SAP event snapshot passed to callbacks. */
struct bt_sap_event {
	/** Event type. */
	enum bt_sap_event_type type;
	/** Local SAP role for the session. */
	enum sap_role role;
	/** Session state when the event snapshot was captured. */
	enum bt_sap_session_state state;
	/** Stable SAP session identifier within the context. */
	uint8_t session_id;
	/** Application user data attached to the SAP session when the event was captured. */
	void *user_data;
	/** Authenticated peer device identifier, if known. */
	uint8_t peer_device_id;
	/** Authenticated peer group identifier, if known. */
	uint8_t peer_group_id;
	/** Whether the SAP session is authenticated. */
	bool authenticated;
	/** Event reason code, or zero when no error occurred. */
	int reason;
};

/** @brief SAP authentication and transport policy. */
struct bt_sap_policy {
	/** Local credential used for SAP authentication. */
	const struct bt_sap_device_credential *local_credential;
	/** Certificate authority public key used to verify peer certificates. */
	const uint8_t *ca_public_key;
	/** Length of @ref ca_public_key in bytes. */
	size_t ca_public_key_len;
	/** Expected peer certificate group identifier. */
	uint8_t expected_group_id;
	/** Allowed central device identifier when authenticating a central. */
	uint8_t allowed_central_id;
};

/** @brief Callback structure used by the SAP service.
 *
 * SAP invokes callbacks synchronously from the API call or transport receive
 * path that produced the event. Callback implementations must return quickly
 * and must not store event or data pointers beyond the callback lifetime. Use
 * application work items for long-running processing.
 */
struct bt_sap_cb {
	/** @brief Send SAP authentication transport data.
	 *
	 * The data buffer is only valid for the duration of the callback. A
	 * transport that sends asynchronously must copy it before returning.
	 *
	 * @param[in] session SAP session.
	 * @param[in] msg_type SAP authentication message type.
	 * @param[in] data Serialized SAP authentication frame.
	 * @param[in] len Length of @p data in bytes.
	 *
	 * @retval 0 If the transport accepted the data.
	 *           Otherwise, a negative error code is returned.
	 */
	int (*send_auth)(struct bt_sap_session *session, uint8_t msg_type, const uint8_t *data,
			 size_t len);

	/** @brief Send SAP secure transport data.
	 *
	 * The data buffer is only valid for the duration of the callback. A
	 * transport that sends asynchronously must copy it before returning.
	 *
	 * @param[in] session SAP session.
	 * @param[in] msg_type SAP secure message type.
	 * @param[in] data Serialized SAP secure frame.
	 * @param[in] len Length of @p data in bytes.
	 *
	 * @retval 0 If the transport accepted the data.
	 *           Otherwise, a negative error code is returned.
	 */
	int (*send_secure)(struct bt_sap_session *session, uint8_t msg_type, const uint8_t *data,
			   size_t len);

	/** @brief Receive an authenticated application payload.
	 *
	 * The data buffer is only valid for the duration of the callback.
	 *
	 * @param[in] event Payload event snapshot. The pointer is only valid
	 *                  for the duration of the callback.
	 * @param[in] msg_type SAP application message type.
	 * @param[in] data Authenticated application payload.
	 * @param[in] len Length of @p data in bytes.
	 */
	void (*secure_payload_received)(const struct bt_sap_event *event, uint8_t msg_type,
					const uint8_t *data, size_t len);

	/** @brief SAP authentication completed for a session.
	 *
	 * @param[in] event Authentication event. The pointer is only valid for
	 *                  the duration of the callback.
	 */
	void (*authenticated)(const struct bt_sap_event *event);

	/** @brief SAP authentication or secure frame handling failed.
	 *
	 * @param[in] event Failure event. The pointer is only valid for the
	 *                  duration of the callback.
	 */
	void (*failed)(const struct bt_sap_event *event);

	/** @brief SAP session disconnected or was released.
	 *
	 * @param[in] event Disconnect event. The pointer is only valid for the
	 *                  duration of the callback.
	 */
	void (*disconnected)(const struct bt_sap_event *event);
};

/** @brief Load a SAP credential from serialized key and certificate bytes.
 *
 * @param[out] credential Credential to populate.
 * @param[in] private_key Identity private key bytes.
 * @param[in] private_key_len Length of @p private_key in bytes.
 * @param[in] cert SAP certificate bytes.
 * @param[in] cert_len Length of @p cert in bytes.
 *
 * @retval 0 If the credential was loaded.
 * @retval -EINVAL If a pointer is NULL or a length does not match the SAP
 *                 credential format.
 */
int bt_sap_credential_from_bytes(struct bt_sap_device_credential *credential,
				 const uint8_t *private_key, size_t private_key_len,
				 const uint8_t *cert, size_t cert_len);

/** @brief Initialize a SAP context.
 *
 * The policy and callback tables are copied into the context. The caller must
 * keep the credential and certificate authority public key referenced by
 * @p policy valid for the lifetime of the context.
 *
 * @param[out] ctx SAP context allocated by the service.
 * @param[in] role Local SAP role.
 * @param[in] policy SAP policy. Must not be NULL.
 * @param[in] callbacks SAP callback table. Must not be NULL.
 *
 * @retval 0 If the context was initialized.
 * @retval -EINVAL If an input pointer, policy, or callback is invalid.
 * @retval -ENOMEM If no SAP context slot is available.
 *           Otherwise, a negative error code is returned.
 */
int bt_sap_init(struct bt_sap_context **ctx, enum sap_role role, const struct bt_sap_policy *policy,
		const struct bt_sap_cb *callbacks);

/** @brief Uninitialize a SAP context.
 *
 * Active sessions are reset and the local signing key is destroyed.
 *
 * @param[in,out] ctx SAP context.
 */
void bt_sap_uninit(struct bt_sap_context *ctx);

/** @brief Allocate a SAP session for a Bluetooth connection.
 *
 * @param[in,out] ctx SAP context.
 * @param[in] conn Bluetooth connection.
 *
 * @return Pointer to the allocated SAP session, or NULL if no session slot is
 *         available.
 */
struct bt_sap_session *bt_sap_on_connected(struct bt_sap_context *ctx, struct bt_conn *conn);

/** @brief Release the SAP session associated with a Bluetooth connection.
 *
 * @param[in,out] ctx SAP context.
 * @param[in] conn Bluetooth connection.
 */
void bt_sap_on_disconnected(struct bt_sap_context *ctx, struct bt_conn *conn);

/** @brief Get the SAP session associated with a Bluetooth connection.
 *
 * @param[in] ctx SAP context.
 * @param[in] conn Bluetooth connection.
 *
 * @return Pointer to the matching SAP session, or NULL if no session matches.
 */
struct bt_sap_session *bt_sap_session_from_conn(struct bt_sap_context *ctx, struct bt_conn *conn);

/** @brief Set application data associated with a SAP session.
 *
 * @param[in,out] session SAP session.
 * @param[in] user_data Application-defined pointer.
 */
void bt_sap_session_set_user_data(struct bt_sap_session *session, void *user_data);

/** @brief Start SAP authentication as the central.
 *
 * @param[in,out] session SAP session.
 *
 * @retval 0 If the central authentication message was sent.
 * @retval -ENOTSUP If the session role is not central.
 * @retval -EPROTO If the session is not idle.
 * @retval -EMSGSIZE If the negotiated ATT MTU cannot carry SAP frames.
 * @retval -EIO If nonce generation fails.
 *               Otherwise, a negative error code from the send callback is
 *               returned.
 */
int bt_sap_start(struct bt_sap_session *session);

/** @brief Handle received SAP authentication transport data.
 *
 * @param[in,out] session SAP session.
 * @param[in] data Serialized SAP authentication frame.
 * @param[in] len Length of @p data in bytes.
 *
 * @retval 0 If the message was handled.
 * @retval -EMSGSIZE If the negotiated ATT MTU cannot carry SAP frames or
 *                   @p len exceeds the maximum authentication frame length.
 *           Otherwise, a negative error code is returned.
 */
int bt_sap_handle_auth_rx(struct bt_sap_session *session, const uint8_t *data, size_t len);

/** @brief Handle received SAP secure transport data.
 *
 * Application payloads are delivered through
 * @ref bt_sap_cb.secure_payload_received.
 *
 * @param[in,out] session SAP session.
 * @param[in] data Serialized SAP secure frame.
 * @param[in] len Length of @p data in bytes.
 *
 * @retval 0 If the frame was handled.
 * @retval -EMSGSIZE If the negotiated ATT MTU cannot carry SAP frames or
 *                   @p len exceeds the maximum secure frame length.
 *           Otherwise, a negative error code is returned.
 */
int bt_sap_handle_secure_rx(struct bt_sap_session *session, const uint8_t *data, size_t len);

/** @brief Send an authenticated SAP application payload.
 *
 * Application message types must be greater than or equal to
 * @ref SAP_APP_MSG_TYPE_MIN.
 *
 * @param[in,out] session SAP session.
 * @param[in] msg_type SAP application message type.
 * @param[in] payload Application payload.
 * @param[in] len Length of @p payload in bytes.
 *
 * @retval 0 If the secure frame was sent.
 * @retval -EPROTO If @p msg_type is reserved for SAP protocol messages.
 * @retval -EACCES If the session is not authenticated or secure transport
 *                 is not ready.
 * @retval -EMSGSIZE If @p len exceeds CONFIG_BT_SAP_MAX_APP_PAYLOAD_SIZE or
 *                   the negotiated ATT MTU cannot carry SAP frames.
 *                   Otherwise, a negative error code is returned.
 */
int bt_sap_send_secure(struct bt_sap_session *session, uint8_t msg_type, const uint8_t *payload,
		       size_t len);

/**
 * @}
 */

#endif /* BT_SAP_H_ */
