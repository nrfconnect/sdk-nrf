/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 *
 * Examples:
 * The following examples show typical sequences of function calls for hashing
 * a message.
   @code
   1. Single-call hash computation
       sx_hash_create(ctx, &sxhashalg_sha2_256, ctxsize)
       sx_hash_feed(ctx, 'chunk 1')
       sx_hash_feed(ctx, 'chunk 2')
       sx_hash_digest(ctx)
       sx_hash_wait(ctx)
   2. Context-saving hash computation
       First round:
	   sx_hash_create(ctx, &sxhashalg_sha2_256, ctxsize)
	   sx_hash_feed(ctx, 'chunk 1 of first round')
	   sx_hash_feed(ctx, 'chunk 2 of first round')
	   sx_hash_save_state(ctx)
	   sx_hash_wait(ctx)
       Intermediary rounds:
	   sx_hash_resume_state(ctx)
	   sx_hash_feed(ctx, 'chunk 1 of round i')
	   sx_hash_feed(ctx, 'chunk 2 of round i')
	   sx_hash_feed(ctx, 'chunk 3 of round i')
	   sx_hash_save_state(ctx)
	   sx_hash_wait(ctx)
       Last round:
	   sx_hash_resume_state(ctx)
	   sx_hash_feed(ctx, 'chunk 1 of last round')
	   sx_hash_feed(ctx, 'chunk 2 of last round')
	   sx_hash_digest(ctx)
	   sx_hash_wait(ctx)
   @endcode
 */

#ifndef HASH_HEADER_FILE
#define HASH_HEADER_FILE

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct sxhash;
struct sxhashalg;

/** @brief Create a hash operation context.
 *
 * This function initializes the user-allocated object @p c with a new hash
 * operation context and reserves the hardware resource. After successful
 * execution of this function, the context @p c can be passed to any of the
 * hashing functions.
 *
 * @param[out] c   Hash operation context.
 * @param[in] alg  Hash algorithm.
 * @param[in] csz  Size of the hash operation context.
 *
 * @retval ::SX_OK                          If the operation completed successfully.
 * @retval ::SX_ERR_INCOMPATIBLE_HW         If no compatible hardware is available.
 * @retval ::SX_ERR_RETRY                   If the hardware is busy and the operation must be
 *                                          retried.
 */
int sx_hash_create(struct sxhash *c, const struct sxhashalg *alg, size_t csz);

/** @brief Get the digest size produced by the given hash algorithm.
 *
 * @param[in] alg Hash algorithm.
 *
 * @return Digest size in bytes.
 */
size_t sx_hash_get_alg_digestsz(const struct sxhashalg *alg);

/** @brief Get the block size used by the given hash algorithm.
 *
 * @param[in] alg Hash algorithm.
 *
 * @return Block size in bytes.
 */
size_t sx_hash_get_alg_blocksz(const struct sxhashalg *alg);

/** @brief Resume hashing in context-saving (partial hashing).
 *
 * This function shall be called when using context-saving to load the state
 * that was previously saved by sx_hash_save_state() in the hash operation
 * context @p c. It must be called with the same hash operation context @p c
 * that was used with sx_hash_save_state(). It reserves all hardware resources
 * required to run the partial hashing.
 *
 * @pre sx_hash_save_state() must be called before this function.
 * @pre This function must be called before sx_hash_feed() for the next partial message.
 *
 * @param[in,out] c Hash operation context.
 *
 * @retval ::SX_OK                          If the operation completed successfully.
 * @retval ::SX_ERR_UNINITIALIZED_OBJ       If the context is not initialized.
 */
int sx_hash_resume_state(struct sxhash *c);

/** @brief Assign data to be hashed.
 *
 * This function adds a chunk of data to be hashed. It can be called multiple
 * times to assemble pieces of the message scattered in memory.
 *
 * In context-saving, the sum of the sizes of the chunks fed must be a multiple
 * of the block size of the algorithm used. If this condition is not met before
 * calling sx_hash_save_state(), sx_hash_save_state() returns
 * ::SX_ERR_WRONG_SIZE_GRANULARITY.
 *
 * @pre One of the sx_hash_create_*() functions must be called first.
 * @pre If in context-saving, sx_hash_resume_state() must be called first.
 *
 * @param[in,out] c   Hash operation context.
 * @param[in] msg     Message to be hashed.
 * @param[in] sz      Size in bytes of @p msg. Maximum value is 2^24-1 bytes.
 *
 * @retval ::SX_OK                          If the operation completed successfully.
 * @retval ::SX_ERR_UNINITIALIZED_OBJ       If the context is not initialized.
 * @retval ::SX_ERR_FEED_COUNT_EXCEEDED     If the maximum number of feeds is exceeded.
 * @retval ::SX_ERR_TOO_BIG                 If the total @p sz of fed data exceeds the limit.
 *
 * @remark If the return value is ::SX_ERR_FEED_COUNT_EXCEEDED or ::SX_ERR_TOO_BIG,
 *         @p c cannot be used anymore.
 * @remark If this function is called with @p sz equal to 0, no data is assigned to be
 *         hashed and ::SX_OK is returned.
 * @remark The default maximum number of feeds for single-call digest is 6 and for
 *         context-saving is 4.
 * @remark The maximum sum of the chunk sizes fed is 2^32-1 bytes.
 */
int sx_hash_feed(struct sxhash *c, const uint8_t *msg, size_t sz);

/** @brief Start the partial hashing operation.
 *
 * This function updates the partial hashing based on the data chunks fed since
 * the last call to sx_hash_resume_state().
 *
 * In order to export the state of partial hashing, the total size of data fed
 * in the current resume-save step must be a multiple of the block size of the
 * algorithm used. For SHA1/224/256/SM3, the block size is 64 bytes, and for
 * SHA384/512, the block size is 128 bytes.
 *
 * The function returns immediately. The hash state is saved in the sxhash
 * structure after the operation completes successfully. The user shall check
 * the operation status with sx_hash_status() or sx_hash_wait().
 *
 * @pre One of the sx_hash_create_*() functions or sx_hash_resume_state() must be called
 *      first.
 *
 * @param[in,out] c Hash operation context.
 *
 * @retval ::SX_OK                          If the operation completed successfully.
 * @retval ::SX_ERR_UNINITIALIZED_OBJ       If the context is not initialized.
 * @retval ::SX_ERR_WRONG_SIZE_GRANULARITY  If the fed data size is not a multiple of the
 *                                          block size.
 *
 * @remark If the return value is ::SX_ERR_WRONG_SIZE_GRANULARITY, @p c cannot be used
 *         anymore.
 * @remark The content of the input data buffers provided with previous calls to
 *         sx_hash_feed() must not be changed until the operation is completed. Check
 *         completion by using sx_hash_wait() or sx_hash_status().
 */
int sx_hash_save_state(struct sxhash *c);

/** @brief Start the hashing operation.
 *
 * This function starts the computation of the digest. If used in the
 * context-saving approach, this function computes the digest based on the last
 * computed state and the last chunks of the message.
 *
 * The function returns immediately. The result is transferred to @p digest only
 * after the operation completes successfully. The user shall check the operation
 * status with sx_hash_status() or sx_hash_wait().
 *
 * @pre One of the sx_hash_create_*() functions must be called first.
 *
 * @param[in,out] c       Hash operation context.
 * @param[out] digest     Result of the hash operation. The user must allocate enough
 *                        memory for it. To get the amount of memory needed for the
 *                        digest, use sx_hash_get_digestsz() or check the corresponding
 *                        sx_hash_create_*() function.
 *
 * @retval ::SX_OK                          If the operation completed successfully.
 * @retval ::SX_ERR_UNINITIALIZED_OBJ       If the context is not initialized.
 *
 * @remark The content of the input data buffers provided with previous calls to
 *         sx_hash_feed() must not be changed until the operation is completed. Check
 *         completion by using sx_hash_wait() or sx_hash_status().
 */
int sx_hash_digest(struct sxhash *c, uint8_t *digest);

/** @brief Start the SHAKE XOF operation.
 *
 * This function starts the computation of the SHAKE digest, which has a variable
 * size. If used in the context-saving approach, this function computes the digest
 * based on the last computed state and the last chunks of the message.
 *
 * The function returns immediately. The result is transferred to @p digest only
 * after the operation completes successfully. The user shall check the operation
 * status with sx_hash_status() or sx_hash_wait().
 *
 * @pre One of the sx_hash_create_*() functions must be called first.
 *
 * @param[in,out] c         Hash operation context.
 * @param[in] skip          Number of bytes to discard (bytes already returned to the
 *                          caller).
 * @param[out] digest       Result of the hash operation. The user must allocate enough
 *                          memory for it.
 * @param[in] digest_sz     Number of new output bytes to produce.
 *
 * @retval ::SX_OK                          If the operation completed successfully.
 * @retval ::SX_ERR_UNINITIALIZED_OBJ       If the context is not initialized.
 *
 * @remark The content of the input data buffers provided with previous calls to
 *         sx_hash_feed() must not be changed until the operation is completed. Check
 *         completion by using sx_hash_wait() or sx_hash_status().
 */
int sx_hash_shake_digest(struct sxhash *c, size_t skip, uint8_t *digest, size_t digest_sz);

/** @brief Wait until the given hash operation has finished.
 *
 * This function returns when the hash operation completes successfully, or when
 * an error occurs that causes the operation to terminate.
 *
 * The return value of this function is the operation status.
 *
 * After this call, all resources have been released and @p c cannot be used again
 * unless sx_hash_create_*() is called.
 *
 * @pre sx_hash_digest() must be called first.
 * @pre If in context-saving, sx_hash_save_state() must be called first.
 *
 * @param[in,out] c Hash operation context.
 *
 * @retval ::SX_OK                          If the operation completed successfully.
 * @retval ::SX_ERR_UNINITIALIZED_OBJ       If the context is not initialized.
 * @retval ::SX_ERR_HW_PROCESSING           If the hardware is still processing the operation.
 * @retval ::SX_ERR_DMA_FAILED              If the DMA operation failed.
 *
 * @sa sx_hash_status
 *
 * @remark This function blocks until the operation finishes.
 * @remark This function calls sx_hash_status() in a loop until the operation completes.
 */
int sx_hash_wait(struct sxhash *c);

/** @brief Get the status of the given hash operation context.
 *
 * If the operation is still ongoing, this function returns ::SX_ERR_HW_PROCESSING.
 * In that case, the user can retry later.
 *
 * When this function returns with a code other than ::SX_ERR_HW_PROCESSING, the hash
 * operation has ended and all associated hardware resources used by hash operation
 * context @p c have been released. If sx_hash_digest() was used, @p c cannot be used
 * for a new operation until one of the sx_hash_create_*() functions is called again.
 * If sx_hash_save_state() was used, then @p c shall be reused with sx_hash_resume_state().
 *
 * @pre sx_hash_digest() must be called first.
 * @pre If in context-saving, sx_hash_save_state() must be called first.
 *
 * @param[in,out] c Hash operation context.
 *
 * @retval ::SX_OK                          If the operation completed successfully.
 * @retval ::SX_ERR_UNINITIALIZED_OBJ       If the context is not initialized.
 * @retval ::SX_ERR_HW_PROCESSING           If the hardware is still processing the operation.
 * @retval ::SX_ERR_DMA_FAILED              If the DMA operation failed.
 */
int sx_hash_status(struct sxhash *c);

/** @brief Get the digest size in bytes for the hash operation context.
 *
 * @pre One of the sx_hash_create_*() functions must be called first.
 *
 * @param[in] c Hash operation context.
 *
 * @return Digest size in bytes of the algorithm specified by @p c.
 */
size_t sx_hash_get_digestsz(struct sxhash *c);

/** @brief Get the block size in bytes for the hash operation context.
 *
 * @pre One of the sx_hash_create_*() functions must be called first.
 *
 * @param[in] c Hash operation context.
 *
 * @return Block size in bytes of the algorithm specified by @p c.
 */
size_t sx_hash_get_blocksz(struct sxhash *c);

#ifdef __cplusplus
}
#endif

#endif
