/** SM4 block cipher, supported modes are ECB, CBC, OFB, CFB and CTR.
 *
 * The "create operation" functions are specific to SM4. The SM4 encrypt and
 * decrypt are done using the following block cipher API functions:
 * sx_blkcipher_resume_state(), sx_blkcipher_save_state(), sx_blkcipher_decrypt,
 * sx_blkcipher_status() and sx_blkcipher_wait().
 *
 * @file
 * @copyright Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * Examples:
 * The following examples show typical sequences of function calls for
 * encrypting a message. The decryption mechanism is identical to the
 * encryption besides the create function that needs to be one of the
 * sx_blkcipher_sm4*_dec() functions.
   @code
   1. One-shot encryption operation
	  sx_blkcipher_create_sm4cbc_enc(ctx, ...)
	  sx_blkcipher_crypt(ctx, ...)
	  sx_blkcipher_run(ctx)
	  sx_blkcipher_wait(ctx)
   2. Context-saving encryption operation
	  First round:
	      sx_blkcipher_create_sm4cbc_enc(ctx)
	      sx_blkcipher_crypt(ctx, 'first chunk')
	      sx_blkcipher_save_state(ctx)
	      sx_blkcipher_wait(ctx)
	  Intermediary rounds:
	      sx_blkcipher_resume_state(ctx)
	      sx_blkcipher_crypt(ctx, 'n-th chunk')
	      sx_blkcipher_save_state(ctx)
	      sx_blkcipher_wait(ctx)
	  Last round:
	      sx_blkcipher_resume_state(ctx)
	      sx_blkcipher_crypt(ctx, 'last chunk')
	      sx_blkcipher_run(ctx)
	      sx_blkcipher_wait(ctx)
   @endcode
 */

#ifndef SM4_HEADER_FILE
#define SM4_HEADER_FILE

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include "internal.h"

/** Prepares an SM4 CTR block cipher encryption.
 *
 * This function initializes the user allocated object \p c with a new block
 * cipher operation context needed to run the SM4 CTR encryption and reserves
 * the HW resource.
 *
 * After successful execution of this function, the context \p c can be passed
 * to any of the block cipher functions.
 *
 * @param[out] c block cipher operation context
 * @param[in] key key used for the block cipher operation, expected size 16
 * bytes
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
int sx_blkcipher_create_sm4ctr_enc(struct sxblkcipher *c, const struct sxkeyref *key,
				   const char *iv);

/** Prepares an SM4 CTR block cipher decryption
 *
 * This function initializes the user allocated object \p c with a new block
 * cipher operation context needed to run the SM4 CTR decryption and reserves
 * the HW resource.
 *
 * After successful execution of this function, the context \p c can be passed
 * to any of the block cipher functions.
 *
 * @param[out] c block cipher operation context
 * @param[in] key key used for the block cipher operation, expected size 16
 * bytes
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
int sx_blkcipher_create_sm4ctr_dec(struct sxblkcipher *c, const struct sxkeyref *key,
				   const char *iv);

/** Prepares an SM4 ECB block cipher encryption.
 *
 * This function initializes the user allocated object \p c with a new block
 * cipher operation context needed to run the SM4 ECB encryption and reserves
 * the HW resource.
 *
 * After successful execution of this function, the context \p c can be passed
 * to any of the block cipher functions.
 *
 * @param[out] c block cipher operation context
 * @param[in] key key used for the block cipher operation, expected size 16
 * bytes
 * @return ::SX_OK
 * @return ::SX_ERR_INVALID_KEYREF
 * @return ::SX_ERR_INVALID_KEY_SZ
 * @return ::SX_ERR_INCOMPATIBLE_HW
 * @return ::SX_ERR_RETRY
 *
 * @pre - key reference provided by \p key must be initialized using
 *        sx_keyref_load_material() or sx_keyref_load_by_id()
 */
int sx_blkcipher_create_sm4ecb_enc(struct sxblkcipher *c, const struct sxkeyref *key);

/** Prepares an SM4 ECB block cipher decryption
 *
 * This function initializes the user allocated object \p c with a new block
 * cipher operation context needed to run the SM4 ECB decryption and reserves
 * the HW resource.
 *
 * After successful execution of this function, the context \p c can be passed
 * to any of the block cipher functions.
 *
 * @param[out] c block cipher operation context
 * @param[in] key key used for the block cipher operation, expected size 16
 * bytes
 * @return ::SX_OK
 * @return ::SX_ERR_INVALID_KEYREF
 * @return ::SX_ERR_INVALID_KEY_SZ
 * @return ::SX_ERR_INCOMPATIBLE_HW
 * @return ::SX_ERR_RETRY
 *
 * @pre - key reference provided by \p key must be initialized using
 *        sx_keyref_load_material() or sx_keyref_load_by_id()
 */
int sx_blkcipher_create_sm4ecb_dec(struct sxblkcipher *c, const struct sxkeyref *key);

/** Prepares an SM4 CBC block cipher encryption.
 *
 * This function initializes the user allocated object \p c with a new block
 * cipher operation context needed to run the SM4 CBC encryption and reserves
 * the HW resource.
 *
 * After successful execution of this function, the context \p c can be passed
 * to any of the block cipher functions.
 *
 * @param[out] c block cipher operation context
 * @param[in] key key used for the block cipher operation, expected size 16
 * bytes
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
int sx_blkcipher_create_sm4cbc_enc(struct sxblkcipher *c, const struct sxkeyref *key,
				   const char *iv);

/** Prepares an SM4 CBC block cipher decryption
 *
 * This function initializes the user allocated object \p c with a new block
 * cipher operation context needed to run the SM4 CBC decryption and reserves
 * the HW resource.
 *
 * After successful execution of this function, the context \p c can be passed
 * to any of the block cipher functions.
 *
 * @param[out] c block cipher operation context
 * @param[in] key key used for the block cipher operation, expected size 16
 * bytes
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
int sx_blkcipher_create_sm4cbc_dec(struct sxblkcipher *c, const struct sxkeyref *key,
				   const char *iv);

/** Prepares an SM4 CFB block cipher encryption.
 *
 * This function initializes the user allocated object \p c with a new block
 * cipher operation context needed to run the SM4 CFB encryption and reserves
 * the HW resource.
 *
 * After successful execution of this function, the context \p c can be passed
 * to any of the block cipher functions.
 *
 * @param[out] c block cipher operation context
 * @param[in] key key used for the block cipher operation, expected size 16
 * bytes
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
int sx_blkcipher_create_sm4cfb_enc(struct sxblkcipher *c, const struct sxkeyref *key,
				   const char *iv);

/** Prepares an SM4 CFB block cipher decryption
 *
 * This function initializes the user allocated object \p c with a new block
 * cipher operation context needed to run the SM4 CFB decryption and reserves
 * the HW resource.
 *
 * After successful execution of this function, the context \p c can be passed
 * to any of the block cipher functions.
 *
 * @param[out] c block cipher operation context
 * @param[in] key key used for the block cipher operation, expected size 16
 * bytes
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
int sx_blkcipher_create_sm4cfb_dec(struct sxblkcipher *c, const struct sxkeyref *key,
				   const char *iv);

/** Prepares an SM4 OFB block cipher encryption.
 *
 * This function initializes the user allocated object \p c with a new block
 * cipher operation context needed to run the SM4 OFB encryption and reserves
 * the HW resource.
 *
 * After successful execution of this function, the context \p c can be passed
 * to any of the block cipher functions.
 *
 * @param[out] c block cipher operation context
 * @param[in] key key used for the block cipher operation, expected size 16
 * bytes
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
int sx_blkcipher_create_sm4ofb_enc(struct sxblkcipher *c, const struct sxkeyref *key,
				   const char *iv);

/** Prepares an SM4 OFB block cipher decryption
 *
 * This function initializes the user allocated object \p c with a new block
 * cipher operation context needed to run the SM4 OFB decryption and reserves
 * the HW resource.
 *
 * After successful execution of this function, the context \p c can be passed
 * to any of the block cipher functions.
 *
 * @param[out] c block cipher operation context
 * @param[in] key key used for the block cipher operation, expected size 16
 * bytes
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
int sx_blkcipher_create_sm4ofb_dec(struct sxblkcipher *c, const struct sxkeyref *key,
				   const char *iv);

/** Prepares an SM4 GCM AEAD encryption operation.
 *
 * This function initializes the user allocated object \p c with a new AEAD
 * encryption operation context needed to run the SM4 GCM operation and
 * reserves the HW resource.
 *
 * After successful execution of this function, the context \p c can be passed
 * to any of the AEAD functions.
 *
 * @param[out] c AEAD operation context
 * @param[in] key key used for the AEAD operation, expected size 16 bytes
 * @param[in] iv initialization vector, size must be 12 bytes
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
int sx_aead_create_sm4gcm_enc(struct sxaead *c, const struct sxkeyref *key, const char *iv);

/** Prepares an SM4 GCM AEAD decryption operation.
 *
 * This function initializes the user allocated object \p c with a new AEAD
 * decryption operation context needed to run the SM4 GCM operation and
 * reserves the HW resource.
 *
 * After successful execution of this function, the context \p c can be passed
 * to any of the AEAD functions.
 *
 * @param[out] c AEAD operation context
 * @param[in] key key used for the AEAD operation, expected size 16 bytes
 * @param[in] iv initialization vector, size must be 12 bytes
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
int sx_aead_create_sm4gcm_dec(struct sxaead *c, const struct sxkeyref *key, const char *iv);

/** Prepares an SM4 CCM AEAD encryption operation.
 *
 * This function initializes the user allocated object \p c with a new AEAD
 * encryption operation context needed to run the SM4 GCM operation and
 * reserves the HW resource.
 *
 * After successful execution of this function, the context \p c can be passed
 * to any of the AEAD functions.
 *
 * @param[out] c AEAD operation context
 * @param[in] key key used for the AEAD operation, expected size 16 bytes
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
int sx_aead_create_sm4ccm_enc(struct sxaead *c, const struct sxkeyref *key, const char *nonce,
			      size_t noncesz, size_t tagsz, size_t aadsz, size_t datasz);

/** Prepares an SM4 CCM AEAD decryption operation.
 *
 * This function initializes the user allocated object \p c with a new AEAD
 * decryption operation context needed to run the SM4 GCM operation and
 * reserves the HW resource.
 *
 * After successful execution of this function, the context \p c can be passed
 * to any of the AEAD functions.
 *
 * @param[out] c AEAD operation context
 * @param[in] key key used for the AEAD operation, expected size 16 bytes
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
int sx_aead_create_sm4ccm_dec(struct sxaead *c, const struct sxkeyref *key, const char *nonce,
			      size_t noncesz, size_t tagsz, size_t aadsz, size_t datasz);

/** Prepares an SM4 XTS block cipher encryption.
 *
 * This function initializes the user allocated object \p c with a new block
 * cipher operation context needed to run the SM4 XTS encryption and reserves
 * the HW resource.
 *
 * After successful execution of this function, the context \p c can be passed
 * to any of the block cipher functions.
 *
 * @param[out] c block cipher operation context
 * @param[in] key1 first key used for the block cipher operation, size must
 *                 be 16 bytes
 * @param[in] key2 second key used for the block cipher operation, size must
 *                 be 16 bytes
 * @param[in] iv initialization vector, size must be 16 bytes
 * @return ::SX_OK
 * @return ::SX_ERR_INVALID_KEYREF
 * @return ::SX_ERR_HW_KEY_NOT_SUPPORTED
 * @return ::SX_ERR_INVALID_KEY_SZ
 * @return ::SX_ERR_INCOMPATIBLE_HW
 * @return ::SX_ERR_RETRY
 *
 * @pre - key references provided by \p key1 and \p key2 must be initialized
 *        using sx_keyref_load_material()
 */
int sx_blkcipher_create_sm4xts_enc(struct sxblkcipher *c, const struct sxkeyref *key1,
				   const struct sxkeyref *key2, const char *iv);

/** Prepares an SM4 XTS block cipher decryption
 *
 * This function initializes the user allocated object \p c with a new block
 * cipher operation context needed to run the SM4 XTS decryption and reserves
 * the HW resource.
 *
 * After successful execution of this function, the context \p c can be passed
 * to any of the block cipher functions.
 *
 * @param[out] c block cipher operation context
 * @param[in] key1 first key used for the block cipher operation, size must
 *                 be 16 bytes
 * @param[in] key2 second key used for the block cipher operation, size must
 *                 be 16 bytes
 * @param[in] iv initialization vector, size must be 16 bytes
 * @return ::SX_OK
 * @return ::SX_ERR_INVALID_KEYREF
 * @return ::SX_ERR_HW_KEY_NOT_SUPPORTED
 * @return ::SX_ERR_INVALID_KEY_SZ
 * @return ::SX_ERR_INCOMPATIBLE_HW
 * @return ::SX_ERR_RETRY
 *
 * @pre - key references provided by \p key1 and \p key2 must be initialized
 *        using sx_keyref_load_material()
 */
int sx_blkcipher_create_sm4xts_dec(struct sxblkcipher *c, const struct sxkeyref *key1,
				   const struct sxkeyref *key2, const char *iv);

/** Prepares an SM4 CMAC generation.
 *
 * This function initializes the user allocated object \p c with a new SM4 CMAC
 * operation context needed to run the MAC generation and reserves the HW
 * resource.
 *
 * After successful execution of this function, the context \p c can be passed
 * to any of the MAC functions.
 *
 * @param[out] c MAC operation context
 * @param[in] key key used for the MAC generation operation, size must be
 *                16 bytes
 * @return ::SX_OK
 * @return ::SX_ERR_INVALID_KEYREF
 * @return ::SX_ERR_INVALID_KEY_SZ
 * @return ::SX_ERR_INCOMPATIBLE_HW
 * @return ::SX_ERR_RETRY
 *
 * @pre - key reference provided by \p key must be initialized using
 *        sx_keyref_load_material() or sx_keyref_load_by_id()
 */
int sx_mac_create_sm4cmac(struct sxmac *c, const struct sxkeyref *key);

#ifdef __cplusplus
}
#endif

#endif
