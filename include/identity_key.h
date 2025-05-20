/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef IDENTITY_KEY_H_
#define IDENTITY_KEY_H_

#include <stdint.h>
#include <stdbool.h>

/**
 * @file
 * @defgroup identity_key Identity key APIs
 * @{
 *
 * @brief API for identity key on CryptoCell devices with KMU
 */

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Identity key size in bytes, corresponding to ECC secp256r1 */
#define IDENTITY_KEY_SIZE_BYTES                 (32)

/** @brief Error value when MKEK is missing from the KMU */
#define IDENTITY_KEY_ERR_MKEK_MISSING           (0x15501)

/** @brief Error value when identity key is missing from the KMU */
#define IDENTITY_KEY_ERR_MISSING                (0x15502)

/** @brief Error value when identity key can't be read */
#define IDENTITY_KEY_ERR_READ_FAILED            (0x15503)

/** @brief Error value when identity key can't be written */
#define IDENTITY_KEY_ERR_WRITE_FAILED           (0x15504)

/** @brief Error value when identity key generation failed */
#define IDENTITY_KEY_ERR_GENERATION_FAILED      (0x15505)

/** @brief Return code for success */
#define IDENTITY_KEY_SUCCESS                    (0x0)

/**
 * @brief Function to check that the MKEK is present
 *
 * MKEK is a prerequisite to encrypt and decrypt the identity key.
 *
 * @return true if MKEK is written, otherwise false
 */
bool identity_key_mkek_is_written(void);

/**
 * @brief Function to check if identity key is written
 *
 * @return true if the identity key is written, otherwise false
 */
bool identity_key_is_written(void);

/**
 * @brief Function to read the identity key from KMU
 *
 * @details The key is read from KMU and decrypted using
 *          the Master Key Encryption Key (MKEK).
 *
 * @param key   Buffer to hold the decrypted identity key
 *
 * @return IDENTITY_KEY_SUCCESS on success, otherwise a negative identity key error code
 */
int identity_key_read(uint8_t key[IDENTITY_KEY_SIZE_BYTES]);

/** @brief Function to clear out an identity key after usage
 *
 * @details This function will clear out the memory of an identity key.
 *          This must be called after the identity key is used to ensure that
 *          it is not leaked in the system.
 *
 * @param[in,out]   key         Key to clear out
 */
void identity_key_free(uint8_t key[IDENTITY_KEY_SIZE_BYTES]);

/**
 * @brief Function to write a random identity key to KMU
 *
 * The identity key will be encrypted using the Master Key Encryption Key (MKEK).
 *
 * @note This function is generally only used in provisioning of the device
 *       and hence is not part of the code running on the end-product.
 *
 * @return IDENTITY_KEY_SUCCESS on success, otherwise a negative identity key error code
 */
int identity_key_write_random(void);

/**
 * @brief Function to write a previously generated identity key to the KMU
 *
 * The identity key will be encrypted using the Master Key Encryption Key (MKEK).
 *
 * This function can be used in a scheme where the key is securely provisioned to
 * the device in production.
 *
 * @note This function is generally only used in provisioning of the device
 *       and hence is not part of the code running on the end-product.
 *
 * @return IDENTITY_KEY_SUCCESS on success, otherwise a negative identity key error code
 */
int identity_key_write_key(uint8_t key[IDENTITY_KEY_SIZE_BYTES]);

/**
 * @brief Function to write a dummy identity key to KMU
 *
 * The identity key will be encrypted using the Master Key Encryption Key (MKEK).
 *
 * @warning The dummy identity key is must only be used for debugging and testing purposes.
 *          Never use this function in production!
 *
 * @return IDENTITY_KEY_SUCCESS on success, otherwise a negative identity key error code
 */
int identity_key_write_dummy(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* IDENTITY_KEY_H_ */
