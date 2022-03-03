/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _FP_CRYPTO_H_
#define _FP_CRYPTO_H_

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
#define FP_SHA256_HASH_LEN     32U
/** Length of AES-128 block (128 bits = 16 bytes). */
#define FP_AES128_BLOCK_LEN    16U
/** Length of ECDH shared key (256 bits = 32 bytes). */
#define FP_ECDH_SHARED_KEY_LEN 32U

/** Hash value using SHA-256.
 *
 * @param[out] out 256-bit (32-byte) buffer to receive hashed result.
 * @param[in] in Input data.
 * @param[in] data_len Length of input data.
 *
 * @return 0 If the operation was successful. Otherwise, a (negative) error code is returned.
 */
int fp_sha256(uint8_t *out, const uint8_t *in, size_t data_len);

/** Encrypt message using AES-128.
 *
 * @param[out] out 128-bit (16-byte) buffer to receive encrypted message.
 * @param[in] in 128-bit (16-byte) plaintext message.
 * @param[in] k 128-bit (16-byte) AES key.
 *
 * @return 0 If the operation was successful. Otherwise, a (negative) error code is returned.
 */
int fp_aes128_encrypt(uint8_t *out, const uint8_t *in, const uint8_t *k);

/** Decrypt message using AES-128.
 *
 * @param[out] out 128-bit (16-byte) buffer to receive plaintext message.
 * @param[in] in 128-bit (16-byte) ciphertext message.
 * @param[in] k 128-bit (16-byte) AES key.
 *
 * @return 0 If the operation was successful. Otherwise, a (negative) error code is returned.
 */
int fp_aes128_decrypt(uint8_t *out, const uint8_t *in, const uint8_t *k);

/** Compute a shared secret key using Elliptic-Curve Diffie-Hellman algorithm.
 *
 * Keys are assumed to be secp256r1 elliptic curve keys.
 *
 * @param[in] public_key 512-bit (64-byte) someone else's public key.
 * @param[in] private_key 256-bit (32-byte) your private key.
 * @param[out] secret_key 256-bit (32-byte) buffer to receive shared secret key.
 *
 * @return 0 If the operation was successful. Otherwise, a (negative) error code is returned.
 */
int fp_ecdh_shared_secret(const uint8_t *public_key, const uint8_t *private_key,
			  uint8_t *secret_key);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _FAST_PAIR_CRYPTO_H_ */
