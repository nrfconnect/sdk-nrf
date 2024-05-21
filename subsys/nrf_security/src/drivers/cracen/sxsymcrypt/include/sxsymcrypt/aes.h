/** Common simple block cipher modes with AES.
 *
 * @file
 * @copyright Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SXSYMCRYPT_AES_HEADER_FILE
#define SXSYMCRYPT_AES_HEADER_FILE

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdbool.h>

#include "internal.h"

/** Prepares an AES XTS block cipher encryption.
 *
 * This function initializes the user allocated object \p c with a new block
 * cipher operation context needed to run the AES XTS encryption and reserves
 * the HW resource.
 *
 * After successful execution of this function, the context \p c can be passed
 * to any of the block cipher functions.
 *
 * @param[out] c block cipher operation context
 * @param[in] key1 first key used for the block cipher operation. expected size
 *                 16, 24 or 32 bytes, must be equal to \p key2 size
 * @param[in] key2 second key used for the block cipher operation, expected
 *                 size 16, 24 or 32 bytes, must be equal to \p key1 size
 * @param[in] iv initialization vector, size must be 16 bytes
 * @return ::SX_OK
 * @return ::SX_ERR_INVALID_KEYREF
 * @return ::SX_ERR_HW_KEY_NOT_SUPPORTED
 * @return ::SX_ERR_INVALID_KEY_SZ
 * @return ::SX_ERR_INCOMPATIBLE_HW
 * @return ::SX_ERR_RETRY
 *
 * @pre - key references provided by \p key1 and \p key2 must be initialized
 *        using sx_keyref_load_material() or sx_keyref_load_by_id().
 *        Both keys must be initialised using the same type of function().
 *        When using sx_keyref_load_by_id(), the id of \p key2 must be
 *        \p key1 id + 1. sx_keyref_load_by_id() can only be used with hardware
 *        that supports hardware keys for AES XTS.
 */
int sx_blkcipher_create_aesxts_enc(struct sxblkcipher *c, const struct sxkeyref *key1,
				   const struct sxkeyref *key2, const char *iv);

/** Prepares an AES XTS block cipher decryption
 *
 * This function initializes the user allocated object \p c with a new block
 * cipher operation context needed to run the AES XTS decryption and reserves
 * the HW resource.
 *
 * After successful execution of this function, the context \p c can be passed
 * to any of the block cipher functions.
 *
 * @param[out] c block cipher operation context
 * @param[in] key1 first key used for the block cipher operation. expected size
 *                 16, 24 or 32 bytes, must be equal to \p key2 size
 * @param[in] key2 second key used for the block cipher operation, expected
 *                 size 16, 24 or 32 bytes, must be equal to \p key1 size
 * @param[in] iv initialization vector, size must be 16 bytes
 * @return ::SX_OK
 * @return ::SX_ERR_INVALID_KEYREF
 * @return ::SX_ERR_HW_KEY_NOT_SUPPORTED
 * @return ::SX_ERR_INVALID_KEY_SZ
 * @return ::SX_ERR_INCOMPATIBLE_HW
 * @return ::SX_ERR_RETRY
 *
 * @pre - key references provided by \p key1 and \p key2 must be initialized
 *        using sx_keyref_load_material() or sx_keyref_load_by_id().
 *        Both keys must be initialised using the same type of function().
 *        When using sx_keyref_load_by_id(), the id of \p key2 must be
 *        \p key1 id + 1. sx_keyref_load_by_id() can only be used with hardware
 *        that supports hardware keys for AES XTS.
 */
int sx_blkcipher_create_aesxts_dec(struct sxblkcipher *c, const struct sxkeyref *key1,
				   const struct sxkeyref *key2, const char *iv);

/** Prepares an AES CTR block cipher encryption.
 *
 * This function initializes the user allocated object \p c with a new block
 * cipher operation context needed to run the AES CTR encryption and reserves
 * the HW resource.
 *
 * After successful execution of this function, the context \p c can be passed
 * to any of the block cipher functions.
 *
 * @param[out] c block cipher operation context
 * @param[in] key key used for the block cipher operation, expected size
 *                16, 24 or 32 bytes
 * @param[in] iv initialization vector, size must be 16 bytes
 * @return ::SX_OK
 * @return ::SX_ERR_INVALID_KEYREF
 * @return ::SX_ERR_INVALID_KEY_SZ
 * @return ::SX_ERR_INCOMPATIBLE_HW
 * @return ::SX_ERR_RETRY
 *
 * @pre - key reference provided by \p key must be initialized using
 *        sx_keyref_load_material() or sx_keyref_load_by_id()
 */
int sx_blkcipher_create_aesctr_enc(struct sxblkcipher *c, const struct sxkeyref *key,
				   const char *iv);

/** Prepares an AES CTR block cipher decryption
 *
 * This function initializes the user allocated object \p c with a new block
 * cipher operation context needed to run the AES CTR decryption and reserves
 * the HW resource.
 *
 * After successful execution of this function, the context \p c can be passed
 * to any of the block cipher functions.
 *
 * @param[out] c block cipher operation context
 * @param[in] key key used for the block cipher operation, expected size
 *            16, 24 or 32 bytes
 * @param[in] iv initialization vector, size must be 16 bytes
 * @return ::SX_OK
 * @return ::SX_ERR_INVALID_KEYREF
 * @return ::SX_ERR_INVALID_KEY_SZ
 * @return ::SX_ERR_INCOMPATIBLE_HW
 * @return ::SX_ERR_RETRY
 *
 * @pre - key reference provided by \p key must be initialized using
 *        sx_keyref_load_material() or sx_keyref_load_by_id()
 */
int sx_blkcipher_create_aesctr_dec(struct sxblkcipher *c, const struct sxkeyref *key,
				   const char *iv);

/** Prepares an AES ECB block cipher encryption.
 *
 * This function initializes the user allocated object \p c with a new block
 * cipher operation context needed to run the AES ECB encryption and reserves
 * the HW resource.
 *
 * After successful execution of this function, the context \p c can be passed
 * to any of the block cipher functions.
 *
 * @param[out] c block cipher operation context
 * @param[in] key key used for the block cipher operation, expected size is
 *                16, 24 or 32 bytes
 * @param[in] aes_countermeasures  Enable the side channel counter-measures for AES
 * @return ::SX_OK
 * @return ::SX_ERR_INVALID_KEYREF
 * @return ::SX_ERR_INVALID_KEY_SZ
 * @return ::SX_ERR_INCOMPATIBLE_HW
 * @return ::SX_ERR_RETRY
 *
 * @pre - key reference provided by \p key must be initialized using
 *        sx_keyref_load_material() or sx_keyref_load_by_id()
 *
 * @remark - AES ECB does not support context saving.
 */
int sx_blkcipher_create_aesecb_enc(struct sxblkcipher *c, const struct sxkeyref *key,
				   bool aes_countermeasures);

/** Prepares an AES ECB block cipher decryption
 *
 * This function initializes the user allocated object \p c with a new block
 * cipher operation context needed to run the AES ECB decryption and reserves
 * the HW resource.
 *
 * After successful execution of this function, the context \p c can be passed
 * to any of the block cipher functions.
 *
 * @param[out] c block cipher operation context
 * @param[in] key key used for the block cipher operation, expected size
 *                16, 24 or 32 bytes
 * @return ::SX_OK
 * @return ::SX_ERR_INVALID_KEYREF
 * @return ::SX_ERR_INVALID_KEY_SZ
 * @return ::SX_ERR_INCOMPATIBLE_HW
 * @return ::SX_ERR_RETRY
 *
 * @pre - key reference provided by \p key must be initialized using
 *        sx_keyref_load_material() or sx_keyref_load_by_id()
 *
 * @remark - AES ECB does not support context saving.
 */
int sx_blkcipher_create_aesecb_dec(struct sxblkcipher *c, const struct sxkeyref *key);

/** Prepares an AES CBC block cipher encryption.
 *
 * This function initializes the user allocated object \p c with a new block
 * cipher operation context needed to run the AES CBC encryption and reserves
 * the HW resource.
 *
 * After successful execution of this function, the context \p c can be passed
 * to any of the block cipher functions.
 *
 * @param[out] c block cipher operation context
 * @param[in] key key used for the block cipher operation, expected size
		  16, 24 or 32 bytes
 * @param[in] iv initialization vector, size must be 16 bytes
 * @return ::SX_OK
 * @return ::SX_ERR_INVALID_KEYREF
 * @return ::SX_ERR_INVALID_KEY_SZ
 * @return ::SX_ERR_INCOMPATIBLE_HW
 * @return ::SX_ERR_RETRY
 *
 * @pre - key reference provided by \p key must be initialized using
 *        sx_keyref_load_material() or sx_keyref_load_by_id()
 */
int sx_blkcipher_create_aescbc_enc(struct sxblkcipher *c, const struct sxkeyref *key,
				   const char *iv);

/** Prepares an AES CBC block cipher decryption
 *
 * This function initializes the user allocated object \p c with a new block
 * cipher operation context needed to run the AES CBC decryption and reserves
 * the HW resource.
 *
 * After successful execution of this function, the context \p c can be passed
 * to any of the block cipher functions.
 *
 * @param[out] c block cipher operation context
 * @param[in] key key used for the block cipher operation, expected size
 *                16, 24 or 32 bytes
 * @param[in] iv initialization vector, size must be 16 bytes
 * @return ::SX_OK
 * @return ::SX_ERR_INVALID_KEYREF
 * @return ::SX_ERR_INVALID_KEY_SZ
 * @return ::SX_ERR_INCOMPATIBLE_HW
 * @return ::SX_ERR_RETRY
 *
 * @pre - key reference provided by \p key must be initialized using
 *        sx_keyref_load_material() or sx_keyref_load_by_id()
 */
int sx_blkcipher_create_aescbc_dec(struct sxblkcipher *c, const struct sxkeyref *key,
				   const char *iv);

/** Prepares an AES CFB block cipher encryption.
 *
 * This function initializes the user allocated object \p c with a new block
 * cipher operation context needed to run the AES CFB encryption and reserves
 * the HW resource.
 *
 * After successful execution of this function, the context \p c can be passed
 * to any of the block cipher functions.
 *
 * @param[out] c block cipher operation context
 * @param[in] key key used for the block cipher operation, expected size
 *                16, 24 or 32 bytes
 * @param[in] iv initialization vector, size must be 16 bytes
 * @return ::SX_OK
 * @return ::SX_ERR_INVALID_KEYREF
 * @return ::SX_ERR_INVALID_KEY_SZ
 * @return ::SX_ERR_INCOMPATIBLE_HW
 * @return ::SX_ERR_RETRY
 *
 * @pre - key reference provided by \p key must be initialized using
 *        sx_keyref_load_material() or sx_keyref_load_by_id()
 */
int sx_blkcipher_create_aescfb_enc(struct sxblkcipher *c, const struct sxkeyref *key,
				   const char *iv);

/** Prepares an AES CFB block cipher decryption
 *
 * This function initializes the user allocated object \p c with a new block
 * cipher operation context needed to run the AES CFB decryption and reserves
 * the HW resource.
 *
 * After successful execution of this function, the context \p c can be passed
 * to any of the block cipher functions.
 *
 * @param[out] c block cipher operation context
 * @param[in] key key used for the block cipher operation, expected size
 *                16, 24 or 32 bytes
 * @param[in] iv initialization vector, size must be 16 bytes
 * @return ::SX_OK
 * @return ::SX_ERR_INVALID_KEYREF
 * @return ::SX_ERR_INVALID_KEY_SZ
 * @return ::SX_ERR_INCOMPATIBLE_HW
 * @return ::SX_ERR_RETRY
 *
 * @pre - key reference provided by \p key must be initialized using
 *        sx_keyref_load_material() or sx_keyref_load_by_id()
 */
int sx_blkcipher_create_aescfb_dec(struct sxblkcipher *c, const struct sxkeyref *key,
				   const char *iv);

/** Prepares an AES OFB block cipher encryption.
 *
 * This function initializes the user allocated object \p c with a new block
 * cipher operation context needed to run the AES OFB encryption and reserves
 * the HW resource.
 *
 * After successful execution of this function, the context \p c can be passed
 * to any of the block cipher functions.
 *
 * @param[out] c block cipher operation context
 * @param[in] key key used for the block cipher operation, expected size
 *                16, 24 or 32 bytes
 * @param[in] iv initialization vector, size must be 16 bytes
 * @return ::SX_OK
 * @return ::SX_ERR_INVALID_KEYREF
 * @return ::SX_ERR_INVALID_KEY_SZ
 * @return ::SX_ERR_INCOMPATIBLE_HW
 * @return ::SX_ERR_RETRY
 *
 * @pre - key reference provided by \p key must be initialized using
 *        sx_keyref_load_material() or sx_keyref_load_by_id()
 */
int sx_blkcipher_create_aesofb_enc(struct sxblkcipher *c, const struct sxkeyref *key,
				   const char *iv);

/** Prepares an AES OFB block cipher decryption
 *
 * This function initializes the user allocated object \p c with a new block
 * cipher operation context needed to run the AES OFB decryption and reserves
 * the HW resource.
 *
 * After successful execution of this function, the context \p c can be passed
 * to any of the block cipher functions.
 *
 * @param[out] c block cipher operation context
 * @param[in] key key used for the block cipher operation, expected size
 *                16, 24 or 32 bytes
 * @param[in] iv initialization vector, size must be 16 bytes
 * @return ::SX_OK
 * @return ::SX_ERR_INVALID_KEYREF
 * @return ::SX_ERR_INVALID_KEY_SZ
 * @return ::SX_ERR_INCOMPATIBLE_HW
 * @return ::SX_ERR_RETRY
 *
 * @pre - key reference provided by \p key must be initialized using
 *        sx_keyref_load_material() or sx_keyref_load_by_id()
 */
int sx_blkcipher_create_aesofb_dec(struct sxblkcipher *c, const struct sxkeyref *key,
				   const char *iv);

/** Prepares an AES GCM AEAD encryption operation.
 *
 * This function initializes the user allocated object \p c with a new AEAD
 * encryption operation context needed to run the AES GCM operation and
 * reserves the HW resource.
 *
 * After successful execution of this function, the context \p c can be passed
 * to any of the AEAD functions.
 *
 * @param[out] c AEAD operation context
 * @param[in] key key used for the AEAD operation, expected size
 *                16, 24 or 32 bytes
 * @param[in] iv initialization vector, size must be 12 bytes
 * @param[in] tagsz size, in bytes, of the tag used for the AEAD operation,
 *            must be a value in {4, 6, 8, 10, 12, 14, 16}
 * @return ::SX_OK
 * @return ::SX_ERR_INVALID_KEYREF
 * @return ::SX_ERR_INVALID_KEY_SZ
 * @return ::SX_ERR_INCOMPATIBLE_HW
 * @return ::SX_ERR_RETRY
 *
 * @pre - key reference provided by \p key must be initialized using
 *        sx_keyref_load_material() or sx_keyref_load_by_id()
 *
 * @remark - \p key and \p iv buffers should not be changed until the operation
 *           is completed.
 * @remark - GMAC is supported by using GCM with plaintext with size 0.
 * @remark - GCM and GMAC support AAD split in multiple chunks, using context
 *           saving.
 */
int sx_aead_create_aesgcm_enc(struct sxaead *c, const struct sxkeyref *key, const char *iv,
			      size_t tagsz);

/** Prepares an AES GCM AEAD decryption operation.
 *
 * This function initializes the user allocated object \p c with a new AEAD
 * decryption operation context needed to run the AES GCM operation and
 * reserves the HW resource.
 *
 * After successful execution of this function, the context \p c can be passed
 * to any of the AEAD functions.
 *
 * @param[out] c AEAD operation context
 * @param[in] key key used for the AEAD operation, expected size
 *                16, 24 or 32 bytes
 * @param[in] iv initialization vector, size must be 12 bytes
 * @param[in] tagsz size, in bytes, of the tag used for the AEAD operation,
 *            must be a value in {4, 6, 8, 10, 12, 14, 16}
 * @return ::SX_OK
 * @return ::SX_ERR_INVALID_KEYREF
 * @return ::SX_ERR_INVALID_KEY_SZ
 * @return ::SX_ERR_INCOMPATIBLE_HW
 * @return ::SX_ERR_RETRY
 *
 * @pre - key reference provided by \p key must be initialized using
 *        sx_keyref_load_material() or sx_keyref_load_by_id()
 *
 * @remark - \p key and \p iv buffers should not be changed until the operation
 *           is completed.
 * @remark - GMAC is supported by using GCM with ciphertext with size 0.
 * @remark - GCM and GMAC support AAD split in multiple chunks, using context
 *           saving.
 */
int sx_aead_create_aesgcm_dec(struct sxaead *c, const struct sxkeyref *key, const char *iv,
			      size_t tagsz);

/** Maximum size, in bytes, of CCM authentication tag */
#define SX_CCM_MAX_TAG_SZ 16u

/** Prepares an AES CCM AEAD encryption operation.
 *
 * This function initializes the user allocated object \p c with a new AEAD
 * encryption operation context needed to run the AES GCM operation and
 * reserves the HW resource.
 *
 * After successful execution of this function, the context \p c can be passed
 * to any of the AEAD functions.
 *
 * @param[out] c AEAD operation context
 * @param[in] key key used for the AEAD operation, expected size
 *                16, 24 or 32 bytes
 * @param[in] noncesz size, in bytes, of the nonce, between 7 and 13 bytes
 * @param[in] nonce nonce used for the AEAD operation, with size \p noncesz
 * @param[in] tagsz size, in bytes, of the tag used for the AEAD operation,
 *            must be a value in {4, 6, 8, 10, 12, 14, 16}
 * @param[in] aadsz size, in bytes, of the additional authenticated data(AAD)
 * @param[in] datasz size, in bytes, of the data to be processed
 * @return ::SX_OK
 * @return ::SX_ERR_INVALID_KEYREF
 * @return ::SX_ERR_INVALID_KEY_SZ
 * @return ::SX_ERR_INVALID_TAG_SIZE
 * @return ::SX_ERR_INCOMPATIBLE_HW
 * @return ::SX_ERR_RETRY
 *
 * @pre - key reference provided by \p key must be initialized using
 *        sx_keyref_load_material() or sx_keyref_load_by_id()
 *
 * @remark - the same aadsz and datasz must be provided to sx_aead_encrypt()
 *           or sx_aead_decrypt() functions.
 * @remark - \p key and \p nonce buffers should not be changed until the
 *           operation is completed.
 * @remark - This does not create the CCM B_0 and B_1 header block, it must be
 *           created according to RFC3610 2.2 and provided via
 *           sx_aead_feed_aad()
 */
int sx_aead_create_aesccm_enc(struct sxaead *c, const struct sxkeyref *key, const char *nonce,
			      size_t noncesz, size_t tagsz, size_t aadsz, size_t datasz);

/** Prepares an AES CCM AEAD decryption operation.
 *
 * This function initializes the user allocated object \p c with a new AEAD
 * decryption operation context needed to run the AES GCM operation and
 * reserves the HW resource.
 *
 * After successful execution of this function, the context \p c can be passed
 * to any of the AEAD functions.
 *
 * @param[out] c AEAD operation context
 * @param[in] key key used for the AEAD operation, expected size
 *                16, 24 or 32 bytes
 * @param[in] noncesz size, in bytes, of the nonce, between 7 and 13 bytes
 * @param[in] nonce nonce used for the AEAD operation, with size \p noncesz
 * @param[in] tagsz size, in bytes, of the tag used for the AEAD operation,
 *            must be a value in {4, 6, 8, 10, 12, 14, 16}
 * @param[in] aadsz size, in bytes, of the additional authenticated data(AAD)
 * @param[in] datasz size, in bytes, of the data to be processed
 * @return ::SX_OK
 * @return ::SX_ERR_INVALID_KEYREF
 * @return ::SX_ERR_INVALID_KEY_SZ
 * @return ::SX_ERR_INVALID_TAG_SIZE
 * @return ::SX_ERR_INCOMPATIBLE_HW
 * @return ::SX_ERR_RETRY
 *
 * @pre - key reference provided by \p key must be initialized using
 *        sx_keyref_load_material() or sx_keyref_load_by_id()
 *
 * @remark - the same aadsz and datasz must be provided to sx_aead_encrypt()
 *           or sx_aead_decrypt() functions.
 * @remark - \p key and \p nonce buffers should not be changed until the
 *           operation is completed.
 * @remark - This does not create the CCM B_0 and B_1 header block, it must be
 *           created according to RFC3610 2.2 and provided via
 *           sx_aead_feed_aad()
 */
int sx_aead_create_aesccm_dec(struct sxaead *c, const struct sxkeyref *key, const char *nonce,
			      size_t noncesz, size_t tagsz, size_t aadsz, size_t datasz);

static inline bool sx_aead_aesccm_nonce_size_is_valid(size_t noncesz)
{
	return (noncesz > 6) && (noncesz < 14);
}

/**
 * @brief Free resources related to blkcipher operation.
 *
 * @param[out] c block cipher operation context
 */
void sx_blkcipher_free(struct sxblkcipher *c);
#ifdef __cplusplus
}
#endif

#endif
