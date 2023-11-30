/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _FP_KEYS_H_
#define _FP_KEYS_H_

#include <stdint.h>
#include <stddef.h>
#include "fp_common.h"

/**
 * @defgroup fp_keys Fast Pair keys
 * @brief Internal API for Fast Pair key handling implementation
 *
 * Fast Pair Keys subsystem handles Fast Pair key generation and message encryption/decryption.
 * The implementation also ensures that keys used for a given connection are invalidated according
 * to rules defined by the Fast Pair specification. The module must be initialized with
 * @ref bt_fast_pair_enable before using API functions.
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @typedef fp_keys_validate_cb
 * @brief Callback used to validate decrypted message.
 *
 * @param[in] conn	Pointer to Bluetooth connection (determines Fast Pair Seeker).
 * @param[in] req	Pointer to buffer containing decrypted request.
 * @param[out] context  Pointer used to store operation context (used to pass the parsed message).
 *
 * @return 0 If the decrypted request is valid. Otherwise, a (negative) error code is returned.
 */
typedef int (*fp_keys_validate_cb)(const struct bt_conn *conn, const uint8_t *req, void *context);

/** @brief Fast Pair keys key generation parameters. */
struct fp_keys_keygen_params {
	/** Encrypted Fast Pair Key-based Pairing Request. The encrypted request length must be
	 *  equal to FP_CRYPTO_AES128_BLOCK_LEN.
	 */
	const uint8_t *req_enc;

	/** Seeker's public key (or NULL if the public key is not provided). The public key length
	 *  must be equal to FP_CRYPTO_ECDH_PUBLIC_KEY_LEN.
	 */
	const uint8_t *public_key;

	/** Callback used to validate decrypted message. */
	fp_keys_validate_cb req_validate_cb;

	/** Pointer used to store operation context (used to pass the parsed message). */
	void *context;
};

/** Encrypt response for the Fast Pair Seeker.
 *
 * Length of buffer used to store encrypted response must at least equal to
 * FP_CRYPTO_AES128_BLOCK_LEN. Raw response length must be equal to FP_CRYPTO_AES128_BLOCK_LEN
 * (Fast Pair response size).
 *
 * @param[in] conn	Pointer to Bluetooth connection (determines used key).
 * @param[out] out	Pointer to buffer used to store encrypted response.
 * @param[in] in	Pointer to buffer containing raw response.
 *
 * @return 0 If the operation was successful. Otherwise, a (negative) error code is returned.
 */
int fp_keys_encrypt(const struct bt_conn *conn, uint8_t *out, const uint8_t *in);

/** Decrypt request from the Fast Pair Seeker.
 *
 * Length of buffer used to store decrypted request must at least equal to
 * FP_CRYPTO_AES128_BLOCK_LEN. Encrypted request length must be equal to FP_CRYPTO_AES128_BLOCK_LEN
 * (Fast Pair request size).
 *
 * @param[in] conn	Pointer to Bluetooth connection (determines used key).
 * @param[out] out	Pointer to buffer used to store decrypted request.
 * @param[in] in	Pointer to buffer containing encrypted request.
 *
 * @return 0 If the operation was successful. Otherwise, a (negative) error code is returned.
 */
int fp_keys_decrypt(const struct bt_conn *conn, uint8_t *out, const uint8_t *in);

/** Encode data to Additional Data packet for the Fast Pair Seeker.
 *
 * Length of buffer used to store Additional Data packet must be at least equal to sum of
 * @ref FP_CRYPTO_ADDITIONAL_DATA_HEADER_LEN and data_len.
 *
 * @param[in] conn	Pointer to Bluetooth connection (determines used key).
 * @param[out] out	Pointer to buffer used to store Additional Data packet.
 * @param[in] data	Data to be encoded to packet.
 * @param[in] data_len	Length of data (in bytes) to be encoded to packet.
 * @param[in] nonce	64-bit (8-byte) nonce.
 *
 * @return 0 If the operation was successful. Otherwise, a (negative) error code is returned.
 */
int fp_keys_additional_data_encode(const struct bt_conn *conn, uint8_t *out, const uint8_t *data,
				   size_t data_len, const uint8_t *nonce);

/** Decode and check integrity of Additional Data packet received from the Fast Pair Seeker.
 *
 * Length of buffer used to store decoded data must be at least equal to packet_len minus
 * @ref FP_CRYPTO_ADDITIONAL_DATA_HEADER_LEN.
 *
 * @param[in] conn		Pointer to Bluetooth connection (determines used key).
 * @param[out] out_data		Pointer to buffer used to store decoded data.
 * @param[in] in_packet		Additional Data packet.
 * @param[in] packet_len	Length (in bytes) of Additional Data packet.
 *
 * @return 0 If the operation was successful. Otherwise, a (negative) error code is returned.
 */
int fp_keys_additional_data_decode(const struct bt_conn *conn, uint8_t *out_data,
				   const uint8_t *in_packet, size_t packet_len);

/** Generate Fast Pair key for a given Fast Pair Seeker.
 *
 * The key is stored internally by the Fast Pair Keys and it is used to encrypt and decrypt messages
 * exchanged with the Fast Pair Seeker.
 *
 * @param[in] conn		    Pointer to Bluetooth connection (determines Fast Pair Seeker).
 * @param[in] fp_keys_keygen_params Additional parameters used to generate Fast Pair Key.
 *
 * @return 0 If the operation was successful. Otherwise, a (negative) error code is returned.
 */
int fp_keys_generate_key(const struct bt_conn *conn, struct fp_keys_keygen_params *keygen_params);

/** Store Account Key of a given Fast Pair Seeker in non-volatile memory.
 *
 * @param[in] conn		Pointer to Bluetooth connection (determines Fast Pair Seeker).
 * @param[in] account_key	Pointer to structure containing the Account Key.
 *
 * @return 0 If the operation was successful. Otherwise, a (negative) error code is returned.
 */
int fp_keys_store_account_key(const struct bt_conn *conn, const struct fp_account_key *account_key);

/** Store Personalized Name written by Fast Pair Seeker in non-volatile memory.
 *
 * @param[in] conn	Pointer to Bluetooth connection (determines Fast Pair Seeker).
 * @param[in] pn	Pointer to Personalized Name string.
 *
 * @return 0 If the operation was successful. Otherwise, a (negative) error code is returned.
 */
int fp_keys_store_personalized_name(const struct bt_conn *conn, const char *pn);

/** Inform Fast Pair Keys about an expected Personalized Name write.
 *
 * The function must be called to allow Personalized Name write after receiving an Action Request.
 * It is not used to allow Personalized Name write right after an Account Key write.
 *
 * @param[in] conn	Pointer to Bluetooth connection (determines Fast Pair Seeker).
 *
 * @return 0 If the operation was successful. Otherwise, a (negative) error code is returned.
 */
int fp_keys_wait_for_personalized_name(const struct bt_conn *conn);

/** Inform Fast Pair Keys about the progress of the Bluetooth authentication procedure.
 *
 * The function must be called to extend validity of the Fast Pair key for given Seeker and to allow
 * Account Key write.
 *
 * @param[in] conn		Pointer to Bluetooth connection (determines Fast Pair Seeker).
 * @param[in] authenticated	Boolean informing if connection was already authenticated.
 */
void fp_keys_bt_auth_progress(const struct bt_conn *conn, bool authenticated);

/** Drop key of a given Fast Pair Seeker.
 *
 * Invalidate key used for message encryption/decryption for a given Fast Pair Seeker. The key must
 * be invalidated if the Fast Pair Seeker breaks the Fast Pair specification.
 *
 * @param[in] conn		Pointer to Bluetooth connection (determines Fast Pair Seeker).
 *
 * @return 0 If the operation was successful. Otherwise, a (negative) error code is returned.
 */
void fp_keys_drop_key(const struct bt_conn *conn);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _FP_KEYS_H_ */
