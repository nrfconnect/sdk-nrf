/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _FP_CRYPTO_H_
#define _FP_CRYPTO_H_

#include <zephyr/types.h>

#include "fp_common.h"

/**
 * @defgroup fp_crypto Fast Pair crypto
 * @brief Internal API for Fast Pair crypto
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** Length of SHA256 hash result (256 bits = 32 bytes). */
#define FP_CRYPTO_SHA256_HASH_LEN		32U
/** Length of AES-128 block (128 bits = 16 bytes). */
#define FP_CRYPTO_AES128_BLOCK_LEN		16U
/** Length of AES-128 key (128 bits = 16 bytes). */
#define FP_CRYPTO_AES128_KEY_LEN		16U
/** Length of ECDH public key (512 bits = 64 bytes). */
#define FP_CRYPTO_ECDH_PUBLIC_KEY_LEN		64U
/** Length of ECDH shared key (256 bits = 32 bytes). */
#define FP_CRYPTO_ECDH_SHARED_KEY_LEN		32U
/** Length of nonce in Additional Data packet (64 bits = 8 bytes). */
#define FP_CRYPTO_ADDITIONAL_DATA_NONCE_LEN	8U
/** Length of Additional Data packet header (128 bits = 16 bytes). */
#define FP_CRYPTO_ADDITIONAL_DATA_HEADER_LEN	16U
/** Length of battery info (1-byte length and type field and 3-byte battery values field). */
#define FP_CRYPTO_BATTERY_INFO_LEN		4U

/** Hash value using SHA-256.
 *
 * @param[out] out 256-bit (32-byte) buffer to receive hashed result.
 * @param[in] in Input data.
 * @param[in] data_len Length of input data.
 *
 * @return 0 If the operation was successful. Otherwise, a (negative) error code is returned.
 */
int fp_crypto_sha256(uint8_t *out, const uint8_t *in, size_t data_len);

/** Generate HMAC-SHA256.
 *
 * @param[out] out 256-bit (32-byte) buffer to receive hashed result.
 * @param[in] in Input data.
 * @param[in] data_len Length of input data.
 * @param[in] hmac_key HMAC key used to encrypt data.
 * @param[in] hmac_key_len Length of HMAC key.
 *
 * @return 0 If the operation was successful. Otherwise, a (negative) error code is returned.
 */
int fp_crypto_hmac_sha256(uint8_t *out,
			  const uint8_t *in,
			  size_t data_len,
			  const uint8_t *hmac_key,
			  size_t hmac_key_len);

/** Encrypt message using AES-128-ECB.
 *
 * @param[out] out 128-bit (16-byte) buffer to receive encrypted message.
 * @param[in] in 128-bit (16-byte) plaintext message.
 * @param[in] k 128-bit (16-byte) AES key.
 *
 * @return 0 If the operation was successful. Otherwise, a (negative) error code is returned.
 */
int fp_crypto_aes128_ecb_encrypt(uint8_t *out, const uint8_t *in, const uint8_t *k);

/** Decrypt message using AES-128-ECB.
 *
 * @param[out] out 128-bit (16-byte) buffer to receive plaintext message.
 * @param[in] in 128-bit (16-byte) ciphertext message.
 * @param[in] k 128-bit (16-byte) AES key.
 *
 * @return 0 If the operation was successful. Otherwise, a (negative) error code is returned.
 */
int fp_crypto_aes128_ecb_decrypt(uint8_t *out, const uint8_t *in, const uint8_t *k);

/** Encrypt data using AES-128-CTR.
 *
 * @param[out] out Buffer to receive encrypted data.
 * @param[in] in Plaintext data.
 * @param[in] data_len Length of input and output data (in bytes).
 * @param[in] key 128-bit (16-byte) AES key.
 * @param[in] nonce 64-bit (8-byte) nonce.
 *
 * @return 0 If the operation was successful. Otherwise, a (negative) error code is returned.
 */
int fp_crypto_aes128_ctr_encrypt(uint8_t *out, const uint8_t *in, size_t data_len,
				 const uint8_t *key, const uint8_t *nonce);

/** Decrypt data using AES-128-CTR.
 *
 * @param[out] out Buffer to receive plaintext data.
 * @param[in] in Ciphertext data.
 * @param[in] data_len Length of input and output data (in bytes).
 * @param[in] key 128-bit (16-byte) AES key.
 * @param[in] nonce 64-bit (8-byte) nonce used to encrypt data.
 *
 * @return 0 If the operation was successful. Otherwise, a (negative) error code is returned.
 */
int fp_crypto_aes128_ctr_decrypt(uint8_t *out, const uint8_t *in, size_t data_len,
				 const uint8_t *key, const uint8_t *nonce);

/** Compute a shared secret key using Elliptic-Curve Diffie-Hellman algorithm.
 *
 * Keys are assumed to be secp256r1 elliptic curve keys.
 *
 * @param[out] secret_key 256-bit (32-byte) buffer to receive shared secret key.
 * @param[in] public_key 512-bit (64-byte) someone else's public key.
 * @param[in] private_key 256-bit (32-byte) your private key.
 *
 * @return 0 If the operation was successful. Otherwise, a (negative) error code is returned.
 */
int fp_crypto_ecdh_shared_secret(uint8_t *secret_key, const uint8_t *public_key,
				 const uint8_t *private_key);

/** Compute an Anti-Spoofing AES key from an ECDH shared secret key.
 *
 * @param[out] out 128-bit (16-byte) buffer to receive AES key.
 * @param[in] in 256-bit (32-byte) ECDH shared secret key.
 *
 * @return 0 If the operation was successful. Otherwise, a (negative) error code is returned.
 */
int fp_crypto_aes_key_compute(uint8_t *out, const uint8_t *in);

/** Get Account Key Filter size.
 *
 * @param[in] n Number of Account Keys.
 *
 * @return Account Key Filter size.
 */
size_t fp_crypto_account_key_filter_size(size_t n);

/** Compute an Account Key Filter (variable-length Bloom filter).
 *
 * @param[out] out Buffer to receive Account Key Filter. Buffer size must be at least
 *                 @ref fp_crypto_account_key_filter_size.
 * @param[in] account_key_list Pointer to array of Account Keys. Array length has to be at least
 *			       equal to number of Account Keys.
 * @param[in] n Number of account keys (n >= 1).
 * @param[in] salt Random 2-byte value - Salt. The Salt is concatenated with Account Keys in
 *		   big-endian format.
 * @param[in] battery_info Battery info or NULL if there is no battery info. Length of battery info
 *			   must be equal to @ref FP_CRYPTO_BATTERY_INFO_LEN.
 *
 * @return 0 If the operation was successful. Otherwise, a (negative) error code is returned.
 */
int fp_crypto_account_key_filter(uint8_t *out, const struct fp_account_key *account_key_list,
				 size_t n, uint16_t salt, const uint8_t *battery_info);

/** Encode data to Additional Data packet.
 *
 * @param[out] out_packet Buffer to receive Additional Data packet. Buffer size must be at least
 *                        equal to sum of @ref FP_CRYPTO_ADDITIONAL_DATA_HEADER_LEN and data_len.
 * @param[in] data Data to be encoded to packet.
 * @param[in] data_len Length of data (in bytes) to be encoded to packet.
 * @param[in] aes_key 128-bit (16-byte) AES key.
 * @param[in] nonce 64-bit (8-byte) nonce.
 *
 * @return 0 If the operation was successful. Otherwise, a (negative) error code is returned.
 */
int fp_crypto_additional_data_encode(uint8_t *out_packet, const uint8_t *data, size_t data_len,
				     const uint8_t *aes_key, const uint8_t *nonce);

/** Check Additional Data packet integrity and decode data from the packet.
 *
 * @param[out] out_data Buffer to receive data decoded from Additional Data packet. Buffer size must
 *                      be at least equal to packet_len minus
 *                      @ref FP_CRYPTO_ADDITIONAL_DATA_HEADER_LEN.
 * @param[in] in_packet Additional Data packet.
 * @param[in] packet_len Length (in bytes) of Additional Data packet.
 * @param[in] aes_key 128-bit (16-byte) AES key.
 *
 * @return 0 If the operation was successful and packet passed integrity check.
 *           Otherwise, a (negative) error code is returned.
 */
int fp_crypto_additional_data_decode(uint8_t *out_data, const uint8_t *in_packet, size_t packet_len,
				     const uint8_t *aes_key);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _FP_CRYPTO_H_ */
