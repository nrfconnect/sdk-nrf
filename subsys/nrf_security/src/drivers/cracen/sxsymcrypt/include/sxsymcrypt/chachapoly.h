/** Authenticated encryption with associated data(AEAD) with ChaCha20-Poly1305.
 *
 * The "create operation" functions are specific to ChaCha20-Poly1305 and
 * specify also the direction, encryption or decryption,
 * sx_aead_create_chacha20poly1305_enc and sx_aead_create_chacha20poly1305_dec.
 * After creation, the user must add the data to be computed by using
 * sx_aead_feed_aad() and sx_aead_crypt(). To start the operation, the user
 * must call sx_aead_produce_tag() when doing an encryption or
 * sx_aead_verify_tag() when doing a decryption. The status of the operation
 * must be checked using sx_aead_status() or sx_aead_wait().
 *
 *
 * @file
 * @copyright Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * Examples:
 * The following examples show typical sequences of function calls for
 * encryption and decryption a message.
   @code
   1. One-shot operation
      a. Encryption
	  sx_aead_create_chacha20poly1305_enc(ctx, ...)
	   sx_aead_feed_aad(ctx, aad, aadsz)
	   sx_aead_crypt(ctx, datain, datainz, dataout)
	   sx_aead_produce_tag(ctx, tag)
	   sx_aead_wait(ctx)
       b. Decryption
	   sx_aead_create_chacha20poly1305_dec(ctx, ...)
	   sx_aead_feed_aad(ctx, aad, aadsz)
	   sx_aead_crypt(ctx, datain, datainz, dataout)
	   sx_aead_verify_tag(ctx, tag)
	   sx_aead_wait(ctx)
   2. Context-saving operation
	 a. Encryption
	 First round:
	    sx_aead_create_chacha20poly1305_enc(ctx)
	    sx_aead_feed_aad(ctx, aad, aadsz)
	    sx_aead_crypt(ctx, 'first chunk')
	    sx_aead_save_state(ctx)
	    sx_aead_wait(ctx)
	 Intermediary rounds:
	    sx_aead_resume_state(ctx)
	    sx_aead_crypt(ctx, 'n-th chunk')
	    sx_aead_save_state(ctx)
	    sx_aead_wait(ctx)
	 Last round:
	    sx_aead_resume_state(ctx)
	    sx_aead_crypt(ctx, 'last chunk')
	    sx_aead_produce_tag(ctx, tag)
	    sx_aead_wait(ctx)
	 b. Decryption
	 First round:
	    sx_aead_create_chacha20poly1305_dec(ctx)
	    sx_aead_feed_aad(ctx, aad, aadsz)
	    sx_aead_crypt(ctx, 'first chunk')
	    sx_aead_save_state(ctx)
	    sx_aead_wait(ctx)
	 Intermediary rounds:
	    sx_aead_resume_state(ctx)
	    sx_aead_crypt(ctx, 'n-th chunk')
	    sx_aead_save_state(ctx)
	    sx_aead_wait(ctx)
	 Last round:
	    sx_aead_resume_state(ctx)
	    sx_aead_crypt(ctx, 'last chunk')
	    sx_aead_verify_tag(ctx, tag)
	    sx_aead_wait(ctx)
   @endcode
 */

#ifndef CHACHAPOLY_HEADER_FILE
#define CHACHAPOLY_HEADER_FILE

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include "internal.h"

/** Prepares a ChaCha20-Poly1305 AEAD encryption operation.
 *
 * This function initializes the user allocated object \p c with a new AEAD
 * encryption operation context needed to run the ChaCha20-Poly1305 operation
 * and reserves the HW resource.
 *
 * After successful execution of this function, the context \p c can be passed
 * to any of the AEAD functions.
 *
 * @param[out] c AEAD operation context
 * @param[in] key key used for the AEAD operation, expected size 32 bytes
 * @param[in] nonce nonce used for the AEAD operation, size must be 12 bytes
 * @param[in] tagsz size, in bytes, of the tag used for the AEAD operation,
 *            must be a value in {4, 6, 8, 10, 12, 14, 16}
 *
 * @return ::SX_OK
 * @return ::SX_ERR_INVALID_KEY_SZ
 * @return ::SX_ERR_INCOMPATIBLE_HW
 * @return ::SX_ERR_RETRY
 *
 * @pre - key reference provided by \p key must be initialized using
 *        sx_keyref_load_material()
 *
 * @remark - \p key and \p nonce buffers should not be changed until the
 *           operation is completed.
 */
int sx_aead_create_chacha20poly1305_enc(struct sxaead *c, const struct sxkeyref *key,
					const char *nonce, size_t tagsz);

/** Prepares a ChaCha20-Poly1305 AEAD decryption operation.
 *
 * This function initializes the user allocated object \p c with a new AEAD
 * decryption operation context needed to run the ChaCha20-Poly1305 operation
 * and reserves the HW resource.
 *
 * After successful execution of this function, the context \p c can be passed
 * to any of the AEAD functions.
 *
 * @param[out] c AEAD operation context
 * @param[in] key key used for the AEAD operation, expected size 32 bytes
 * @param[in] nonce nonce used for the AEAD operation, size must be 12 bytes
 * @param[in] tagsz size, in bytes, of the tag used for the AEAD operation,
 *            must be a value in {4, 6, 8, 10, 12, 14, 16}
 *
 * @return ::SX_OK
 * @return ::SX_ERR_INVALID_KEY_SZ
 * @return ::SX_ERR_INCOMPATIBLE_HW
 * @return ::SX_ERR_RETRY
 *
 * @pre - key reference provided by \p key must be initialized using
 *        sx_keyref_load_material()
 *
 * @remark - \p key and \p nonce buffers should not be changed until the
 *           operation is completed.
 * @remark - ChaCha20-Poly1305 supports AAD split in multiple chunks, using
 *           context saving.
 */
int sx_aead_create_chacha20poly1305_dec(struct sxaead *c, const struct sxkeyref *key,
					const char *nonce, size_t tagsz);

/** Prepares a ChaCha20 cipher encryption operation.
 *
 * This function initializes the user allocated object \p c with a new cipher
 * encryption operation context needed to run the ChaCha20 operation
 * and reserves the HW resource.
 *
 * After successful execution of this function, the context \p c can be passed
 * to any of the cipher functions.
 *
 * @param[out] c Cipher operation context
 * @param[in] key key used for the cipher operation, expected size 32 bytes
 * @param[in] nonce nonce used for the cipher operation, size must be 16 bytes
 *                  with the first 4 bytes being the counter and the rest 12
 *                  bytes are the nonce
 *
 * @return ::SX_OK
 * @return ::SX_ERR_INVALID_KEY_SZ
 * @return ::SX_ERR_INCOMPATIBLE_HW
 * @return ::SX_ERR_RETRY
 *
 * @pre - key reference provided by \p key must be initialized using
 *        sx_keyref_load_material()
 *
 * @remark - \p key and \p nonce buffers should not be changed until the
 *           operation is completed.
 */
int sx_blkcipher_create_chacha20_enc(struct sxblkcipher *c, const struct sxkeyref *key,
				     const char *counter, const char *nonce);

/** Prepares a ChaCha20 cipher decryption operation.
 *
 * This function initializes the user allocated object \p c with a new cipher
 * decryption operation context needed to run the ChaCha20 operation
 * and reserves the HW resource.
 *
 * After successful execution of this function, the context \p c can be passed
 * to any of the cipher functions.
 *
 * @param[out] c Cipher operation context
 * @param[in] key key used for the cipher operation, expected size 32 bytes
 * @param[in] nonce nonce used for the cipher operation, size must be 16 bytes
 *                  with the first 4 bytes being the counter and the rest 12
 *                  bytes are the nonce
 *
 * @return ::SX_OK
 * @return ::SX_ERR_INVALID_KEY_SZ
 * @return ::SX_ERR_INCOMPATIBLE_HW
 * @return ::SX_ERR_RETRY
 *
 * @pre - key reference provided by \p key must be initialized using
 *        sx_keyref_load_material()
 *
 * @remark - \p key and \p nonce buffers should not be changed until the
 *           operation is completed.
 */
int sx_blkcipher_create_chacha20_dec(struct sxblkcipher *c, const struct sxkeyref *key,
				     const char *counter, const char *nonce);

#ifdef __cplusplus
}
#endif

#endif
