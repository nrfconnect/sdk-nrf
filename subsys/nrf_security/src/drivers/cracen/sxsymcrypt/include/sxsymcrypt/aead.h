/** Authenticated encryption with associated data(AEAD).
 *
 * @file
 * @copyright Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * Examples:
 * The following examples show typical sequences of function calls for
 * encryption and decryption a message.
 *  @code
 *  1. One-shot operation
 *     a. Encryption
 * sx_aead_create_aesgcm_enc(ctx, ...)
 * sx_aead_feed_aad(ctx, aad, aadsz)
 * sx_aead_crypt(ctx, datain, datainz, dataout)
 * sx_aead_produce_tag(ctx, tag)
 * sx_aead_wait(ctx)
 *     b. Decryption
 * sx_aead_create_aesgcm_dec(ctx, ...)
 * sx_aead_feed_aad(ctx, aad, aadsz)
 * sx_aead_crypt(ctx, datain, datainz, dataout)
 * sx_aead_verify_tag(ctx, tag)
 * sx_aead_wait(ctx)
 *  2. Context-saving operation
 * a. Encryption
 * First ad round:
 *    sx_aead_create_aesccm_enc(ctx)
 *    sx_aead_feed_aad(ctx, aad, aadsz)
 *    sx_aead_save_state(ctx)
 *    sx_aead_wait(ctx)
 * Intermediary rounds:
 *    sx_aead_resume_state(ctx)
 *    sx_aead_feed_aad(ctx, aad, aadsz)
 *    sx_aead_save_state(ctx)
 *    sx_aead_wait(ctx)
 * Last round:
 *    sx_aead_resume_state(ctx)
 *    sx_aead_feed_aad(ctx, aad, aadsz)
 *    sx_aead_save_state(ctx)
 *    sx_aead_wait(ctx)
 * First crypt data round:
 *    sx_aead_resume_state(ctx)
 *    sx_aead_crypt(ctx, 'first chunk')
 *    sx_aead_save_state(ctx)
 *    sx_aead_wait(ctx)
 * Intermediary rounds:
 *    sx_aead_resume_state(ctx)
 *    sx_aead_crypt(ctx, 'n-th chunk')
 *    sx_aead_save_state(ctx)
 *    sx_aead_wait(ctx)
 * Last round:
 *    sx_aead_resume_state(ctx)
 *    sx_aead_crypt(ctx, 'last chunk')
 *    sx_aead_produce_tag(ctx, tag)
 *    sx_aead_wait(ctx)
 * b. Decryption
 * First round:
 *    sx_aead_create_aesccm_dec(ctx)
 *    sx_aead_feed_aad(ctx, aad, aadsz)
 *    sx_aead_crypt(ctx, 'first chunk')
 *    sx_aead_save_state(ctx)
 *    sx_aead_wait(ctx)
 * Intermediary rounds:
 *    sx_aead_resume_state(ctx)
 *    sx_aead_crypt(ctx, 'n-th chunk')
 *    sx_aead_save_state(ctx)
 *    sx_aead_wait(ctx)
 * Last round:
 *    sx_aead_resume_state(ctx)
 *    sx_aead_crypt(ctx, 'last chunk')
 *    sx_aead_verify_tag(ctx, tag)
 *    sx_aead_wait(ctx)
 *  @endcode
 **/

#ifndef AEAD_HEADER_FILE
#define AEAD_HEADER_FILE

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

/** Initialization vector (IV) size, in bytes, for GCM encryption/decryption */
#define SX_GCM_IV_SZ 12u

/** Size, in bytes, of GCM authentication tag */
#define SX_GCM_TAG_SZ 16u

/** Initialization vector (IV) size, in bytes, for CHACHA20_POLY1305 encryption/decryption */
#define SX_CHACHAPOLY_IV_SZ (12U)

struct sxaead;

/** Adds AAD chunks
 *
 * This function is used for adding AAD buffer given by \p aad. The function
 * will return immediately.
 *
 * @param[in,out] c AEAD operation context
 * @param[in] aad additional authentication data(AAD), with size \p aadsz
 * @param[in] aadsz size, in bytes, of the additional authenticated data(AAD),
 *                  can be 0 if \p aad is empty
 * @return ::SX_OK
 * @return ::SX_ERR_UNINITIALIZED_OBJ
 * @return ::SX_ERR_TOO_BIG
 *
 * @pre - one of the sx_aead_create_*() functions must be called first
 *
 * @remark - the additional authentication data can be empty(\p aadsz = 0)
 * @remark - \p aad buffer should not be changed until the operation is
 *           completed.
 * @remark - in context saving mode, \p aadsz must be multiple of block size
 *           unless it is the last chunk
 */
int sx_aead_feed_aad(struct sxaead *c, const char *aad, size_t aadsz);

/** Adds data to be encrypted or decrypted.
 *
 * This function is used for adding data to be processed. The function will
 * return immediately.
 *
 * The result of the operation will be transferred to \p dataout after the
 * operation is successfully completed.
 *
 * For context saving, \p datain size(\p datainsz) must be a multiple of 16
 * bytes for AES GCM and CCM and a multiple of 64 bytes for ChaCha20Poly1305,
 * except the last buffer.
 *
 * @param[in,out] c AEAD operation context
 * @param[in] datain data to be encrypted or decryoted, with size \p datainsz
 * @param[in] datainsz size, in bytes, of the data to be encrypted or decrypted
 * @param[out] dataout encrypted or decrypted data, must have \p datainsz bytes
 * @return ::SX_OK
 * @return ::SX_ERR_UNINITIALIZED_OBJ
 * @return ::SX_ERR_TOO_BIG
 *
 * @pre - one of the sx_aead_create_*() functions must be called first
 *
 * @remark - \p datain buffer should not be changed until the operation is
 *           completed.
 * @remark - in context saving mode, \p datainsz must be multiple of block size
 *           unless it is the last chunk
 */
int sx_aead_crypt(struct sxaead *c, const char *datain, size_t datainsz, char *dataout);

/** Starts an AEAD encryption and tag computation.
 *
 * The function will return immediately.
 *
 * The computed tag will be transferred to \p tag only after the operation is
 * successfully completed.
 *
 * The user shall check operation status with sx_aead_status() or
 * sx_aead_wait().
 *
 * @param[in,out] c AEAD operation context
 * @param[out] tag authentication tag
 * @return ::SX_OK
 * @return ::SX_ERR_UNINITIALIZED_OBJ
 * @return ::SX_ERR_TOO_BIG
 *
 * @pre - one of the sx_aead_feed_aad() or sx_aead_crypt() functions must be
 *        called first
 *
 * @remark - if used with context saving(last chunk), the fed data size for
 *         the last chunk can not be 0
 */
int sx_aead_produce_tag(struct sxaead *c, char *tag);

/** Starts an AEAD decryption and tag validation.
 *
 * The function will return immediately.
 *
 * The user shall check operation status with sx_aead_status() or
 * sx_aead_wait().
 *
 * @param[in,out] c AEAD operation context
 * @param[in] tag authentication tag
 * @return ::SX_OK
 * @return ::SX_ERR_UNINITIALIZED_OBJ
 * @return ::SX_ERR_TOO_BIG
 *
 * @pre - one of the sx_aead_feed_aad() or sx_aead_crypt() functions must be
 *        called first
 *
 * @remark - \p tag buffer should not be changed until the operation is
 *           completed.
 * @remark - if used with context saving(last chunk), the fed data size for
 *         the last chunk can not be 0
 */
int sx_aead_verify_tag(struct sxaead *c, const char *tag);

/** Resumes AEAD operation in context-saving.
 *
 * This function shall be called when using context-saving to load the state
 * that was previously saved by sx_aead_save_state() in the sxaead
 * operation context \p c. It must be called with the same sxaead operation
 * context \p c that was used with sx_aead_save_state(). It will reserve
 * all hardware resources required to run the partial AEAD operation.
 * Previously used mode and direction are already stored in sxaead \p c.
 *
 * After successful execution of this function, the context \p c can be passed
 * to any of the AEAD functions, except the sx_aead_create_*()
 * functions.
 *
 * @param[in,out] c AEAD operation context
 * @return ::SX_OK
 * @return ::SX_ERR_UNINITIALIZED_OBJ
 * @return ::SX_ERR_CONTEXT_SAVING_NOT_SUPPORTED
 * @return ::SX_ERR_INVALID_KEYREF
 * @return ::SX_ERR_INVALID_KEY_SZ
 * @return ::SX_ERR_INCOMPATIBLE_HW
 * @return ::SX_ERR_RETRY
 *
 * @pre - sx_aead_create_aes*() and sx_aead_save_state() functions
 *        must be called before, for first part of the message.
 * @pre - must be called for each part of the message(besides first), before
 *        sx_aead_crypt() or sx_aead_save_state().
 *
 * @remark - the user must not change the key until all parts of the message to
 *           be encrypted/decrypted are processed.
 */
int sx_aead_resume_state(struct sxaead *c);

/** Starts a partial AEAD operation.
 *
 * This function is used to start a partial encryption or decryption of
 * \p datain. The function will return immediately.
 *
 * The partial result will be transferred to \p dataout only after the operation
 * is successfully completed. The user shall check operation status with
 * sx_aead_status() or sx_aead_wait().
 *
 * @param[in,out] c AEAD operation context
 * @return ::SX_OK
 * @return ::SX_ERR_UNINITIALIZED_OBJ
 * @return ::SX_ERR_CONTEXT_SAVING_NOT_SUPPORTED
 * @return ::SX_ERR_WRONG_SIZE_GRANULARITY
 *
 * @pre - sx_aead_crypt() should be called first.
 *
 * @remark - the user must not change the key until all parts of the message to
 *           be encrypted/decrypted are processed.
 * @remark - when in context saving, the sizes of the chunks fed must be
 *           multiple of 16 bytes, besides the last chunk that can be any size,
 *           but not 0
 */
int sx_aead_save_state(struct sxaead *c);

/** Waits until the given AEAD operation has finished
 *
 * This function returns when the AEAD operation was successfully completed,
 * or when an error has occurred that caused the operation to terminate.
 *
 * The return value of this function is the operation status.
 *
 * After this call, all resources have been released and \p c cannot be used
 * again unless sx_aead_create_*() is used.
 *
 * @param[in,out] c AEAD operation context
 * @return ::SX_OK
 * @return ::SX_ERR_UNINITIALIZED_OBJ
 * @return ::SX_ERR_DMA_FAILED
 * @return ::SX_ERR_INVALID_TAG
 *
 * @pre - sx_aead_encrypt or sx_aead_decrypt() function must be called first
 *
 * @see sx_aead_status().
 *
 * @remark - this function is blocking until operation finishes.
 */
int sx_aead_wait(struct sxaead *c);

/** Returns the AEAD operation status.
 *
 * If the operation is still ongoing, return ::SX_ERR_HW_PROCESSING.
 * In that case, the user can retry later.
 *
 * When this function returns with a code different than ::SX_ERR_HW_PROCESSING,
 * the AEAD operation has ended and all resources used by the AEAD operation
 * context \p c have been released. In this case, \p c cannot be used for a new
 * operation until one of the sx_aead_create_*() functions is called again.
 *
 * @param[in,out] c AEAD operation context
 * @return ::SX_OK
 * @return ::SX_ERR_UNINITIALIZED_OBJ
 * @return ::SX_ERR_HW_PROCESSING
 * @return ::SX_ERR_DMA_FAILED
 * @return ::SX_ERR_INVALID_TAG
 *
 * @pre - sx_aead_encrypt or sx_aead_decrypt() function must be called first
 *
 * @remark -  if authentication fails during decryption, ::SX_ERR_INVALID_TAG
 *            will be returned. In this case, the decrypted text is not valid
 *            and shall not be used.
 */
int sx_aead_status(struct sxaead *c);

/**
 * @brief Free AEAD operation context.
 *
 * @param[in,out] c AEAD operation context.
 */
void sx_aead_free(struct sxaead *c);

#ifdef __cplusplus
}
#endif

#endif
