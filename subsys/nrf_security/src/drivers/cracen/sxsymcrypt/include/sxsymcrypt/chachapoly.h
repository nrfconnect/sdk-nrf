/** Authenticated encryption with associated data(AEAD) with ChaCha20-Poly1305
 * and stream cipher based on ChaCha20 block cipher.
 *
 * ChaCha20-Poly1305 "create operation" functions:
 * sx_aead_create_chacha20poly1305_enc and sx_aead_create_chacha20poly1305_dec.
 * ChaCha20 "create operation" functions:
 * sx_blkciper_create_chacha20_enc and sx_blkciper_create_chacha20_dec.
 *
 * After creation of the AEAD operation, the user can feed data by using
 * sx_aead_feed_aad() and crypt text with sx_aead_crypt(). To start the
 * operation, the user must call sx_aead_produce_tag() when doing an encryption
 * or sx_aead_verify_tag() when doing a decryption. The status of the operation
 * must be checked using sx_aead_status() or sx_aead_wait(). For further details
 * check aead.h.
 *
 * After creation of the block cipher operation, the user can crypt text by using
 * sx_blkcipher_crypt(). To start the operation, the  user must call
 * sx_blkcipher_run(). The status of the operation must be checked using
 * sx_blkcipher_status() or sx_blkcipher_wait(). For further details check
 * blkcipher.h.
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
#include <stdint.h>
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
int sx_aead_create_chacha20poly1305_enc(struct sxaead *c, struct sxkeyref *key,
					const uint8_t *nonce, size_t tagsz);

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
int sx_aead_create_chacha20poly1305_dec(struct sxaead *c, struct sxkeyref *key,
					const uint8_t *nonce, size_t tagsz);

/** Prepares a ChaCha20 encryption.
 *
 * This function initializes the user allocated object \p c with a new block
 * cipher operation context needed to run the ChaCha20 encryption and reserves
 * the HW resource.
 *
 * After successful execution of this function, the context \p c can be passed
 * to any of the block cipher functions.
 *
 * @param[out] c block cipher operation context
 * @param[in] key key used for the block cipher operation, expected size 32 bytes
 * @param[in] counter initialization counter, maximum value is 0xFFFFFFFF.
 * @param[in] nonce nonce used for ChaCha20 operation, size must be 12 bytes
 * @return ::SX_OK
 * @return ::SX_ERR_INVALID_KEYREF
 * @return ::SX_ERR_INVALID_KEY_SZ
 * @return ::SX_ERR_INCOMPATIBLE_HW
 * @return ::SX_ERR_RETRY
 *
 * @pre - key reference provided by \p key must be initialized using
 *        sx_keyref_load_material() or sx_keyref_load_by_id()
 */
int sx_blkcipher_create_chacha20_enc(struct sxblkcipher *c, struct sxkeyref *key,
				     const uint8_t *counter, const uint8_t *nonce);

/** Prepares a ChaCha20 decryption.
 *
 * This function initializes the user allocated object \p c with a new block
 * cipher operation context needed to run the ChaCha20 decryption and reserves
 * the HW resource.
 *
 * After successful execution of this function, the context \p c can be passed
 * to any of the block cipher functions.
 *
 * @param[out] c block cipher operation context
 * @param[in] key key used for the block cipher operation, expected size 32 bytes
 * @param[in] counter initialization counter, maximum value is 0xFFFFFFFF, must
 *                    be provided in little-endian format.
 * @param[in] nonce nonce used for ChaCha20 operation, size must be 12 bytes
 * @return ::SX_OK
 * @return ::SX_ERR_INVALID_KEYREF
 * @return ::SX_ERR_INVALID_KEY_SZ
 * @return ::SX_ERR_INCOMPATIBLE_HW
 * @return ::SX_ERR_RETRY
 *
 * @pre - key reference provided by \p key must be initialized using
 *        sx_keyref_load_material() or sx_keyref_load_by_id()
 */
int sx_blkcipher_create_chacha20_dec(struct sxblkcipher *c, struct sxkeyref *key,
				     const uint8_t *counter, const uint8_t *nonce);

#ifdef __cplusplus
}
#endif

#endif
