/** Cryptographic message hashing.
 *
 * @file
 *
 * @copyright Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
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

#ifdef __cplusplus
extern "C" {
#endif

struct sxhash;
struct sxhashalg;

/** Creates a hash operation context
 *
 * This function initializes the user allocated object \p c with a new hash
 * operation context and reserves the HW resource. After successful execution
 * of this function, the context \p c can be passed to any of the hashing
 * functions.
 *
 * @param[out] c hash operation context
 * @param[in] alg hash algorithm
 * @param[in] csz size of the hash operation context
 * @return ::SX_OK
 * @return ::SX_ERR_INCOMPATIBLE_HW
 * @return ::SX_ERR_RETRY
 */
int sx_hash_create(struct sxhash *c, const struct sxhashalg *alg, size_t csz);

/** Return the digest size produced by the given hash algorithm */
size_t sx_hash_get_alg_digestsz(const struct sxhashalg *alg);

/** Return the block size used by the given hash algorithm */
size_t sx_hash_get_alg_blocksz(const struct sxhashalg *alg);

/** Resume hashing in context-saving (partial hashing).
 *
 * This function shall be called when using context-saving to load the state
 * that was previously saved by sx_hash_save_state() in the sxhash operation
 * context \p c. It must be called with the same sxhash operation context
 * \p c that was used with sx_hash_save_state(). It will reserve all
 * hardware resources required to run the partial hashing.
 *
 * @param[in,out] c hash operation context
 * @return ::SX_OK
 * @return ::SX_ERR_UNINITIALIZED_OBJ
 *
 * @pre - sx_hash_save_state() function must be called before
 * @pre - must be called before sx_hash_feed() for the next partial message
 */
int sx_hash_resume_state(struct sxhash *c);

/** Assign data to be hashed.
 *
 * This function adds a chunk of data to be hashed. It can be called multiple
 * times to assemble pieces of the message scattered in memory.
 *
 * In context-saving, the sum of the sizes of the chunks fed must be multiple
 * of the block size of the algorithm used. If this condition is not met before
 * calling sx_hash_save_state(), sx_hash_save_state() will return
 * ::SX_ERR_WRONG_SIZE_GRANULARITY
 *
 * @param[in,out] c hash operation context
 * @param[in] msg message to be hashed
 * @param[in] sz size in bytes of \p msg, maximum value is 2^24-1 bytes
 * @return ::SX_OK
 * @return ::SX_ERR_UNINITIALIZED_OBJ
 * @return ::SX_ERR_FEED_COUNT_EXCEEDED
 * @return ::SX_ERR_TOO_BIG
 *
 * @pre - one of the sx_hash_create_*() functions must be called first
 * @pre - if in context-saving, sx_hash_resume_state() must be called first
 *
 * @remark - if return value is ::SX_ERR_FEED_COUNT_EXCEEDED or
 *           ::SX_ERR_TOO_BIG, \p c cannot be used anymore.
 * @remark - if this function is called with \p sz equal to 0, no data will be
 *           assigned to be hashed, ::SX_OK will be returned.
 * @remark - default maximum number of feeds for single-call digest is 6 and for
 *           context-saving is 4.
 * @remark - maximum sum of the chunk sizes fed is 2^32-1 bytes
 */
int sx_hash_feed(struct sxhash *c, const char *msg, size_t sz);

/** Starts the partial hashing operation.
 *
 * This function updates the partial hashing based on the data chunks fed since
 * the last call to sx_hash_resume_state().
 *
 * In order to export the state of partial hashing, the total size of data
 * fed in the current resume-save step must be multiple of block size of
 * the algorithm used. For SHA1/224/256/SM3 block size is 64 bytes and
 * for SHA384/512 block size is 128 bytes.
 *
 * The function will return immediately. The hash state will be saved
 * in the sxhash structure after the operation successfully completed.
 * The user shall check operation status with sx_hash_status() or
 * sx_hash_wait().
 *
 * @param[in,out] c hash operation context
 * @return ::SX_OK
 * @return ::SX_ERR_UNINITIALIZED_OBJ
 * @return ::SX_ERR_WRONG_SIZE_GRANULARITY
 *
 * @pre - one of the sx_hash_create_*() functions or sx_hash_resume_state()
 *        must be called first
 *
 * @remark - if return value is ::SX_ERR_WRONG_SIZE_GRANULARITY, \p c cannot
 *           be used anymore.
 * @remark - the content of the input data buffers, provided with previous
 *           calls to sx_hash_feed(), should not be changed until the operation
 *           is completed. Checking the completion of an operation is done by
 *           using sx_hash_wait() or sx_hash_status().
 */
int sx_hash_save_state(struct sxhash *c);

/** Starts the hashing operation.
 *
 * This function is used to start the computation of the digest.
 * If used in context-saving approach, this function will compute the digest
 * based on last computed state and last chunks of the message.
 *
 * The function will return immediately. The result will be transferred to
 * \p digest only after the operation is successfully completed. The user shall
 * check operation status with sx_hash_status() or sx_hash_wait().
 *
 * @param[in,out] c hash operation context
 * @param[out] digest result of the hash operation, user must allocate enough
 *                    memory for it. In order to get the amount of memory needed
 *                    for the digest, the user can use sx_hash_get_digestsz() or
 *                    check the corresponding sx_hash_create_*() function.
 * @return ::SX_OK
 * @return ::SX_ERR_UNINITIALIZED_OBJ
 *
 * @pre - one of the sx_hash_create_*() functions must be called first
 *
 * @remark - the content of the input data buffers, provided with previous
 *           calls to sx_hash_feed(), should not be changed until the operation
 *           is completed. Checking the completion of an operation is done by
 *           using sx_hash_wait() or sx_hash_status().
 */
int sx_hash_digest(struct sxhash *c, char *digest);

/** Waits until the given hash operation has finished
 *
 * This function returns when the hash operation was successfully completed,
 * or when an error has occurred that caused the operation to terminate.
 *
 * The return value of this function is the operation status.
 *
 * After this call, all resources have been released and \p c cannot be used
 * again unless sx_hash_create_*() is used.
 *
 * @param[in,out] c hash operation context
 * @return ::SX_OK
 * @return ::SX_ERR_UNINITIALIZED_OBJ
 * @return ::SX_ERR_HW_PROCESSING
 * @return ::SX_ERR_DMA_FAILED
 *
 * @pre - sx_hash_digest() functions must be called first
 * @pre - if in context-saving, sx_hash_save_state() must be called first
 *
 * @see sx_hash_status().
 *
 * @remark - this function is blocking until operation finishes.
 * @remark - this function calls sx_hash_status() in loop until operation
 *           is completed.
 */
int sx_hash_wait(struct sxhash *c);

/** Returns the status of the given hash operation context.
 *
 * If the operation is still ongoing, return ::SX_ERR_HW_PROCESSING.
 * In that case, the user can retry later.
 *
 * When this function returns with a code different than ::SX_ERR_HW_PROCESSING,
 * the hash operation has ended and all associated hardware resources
 * used by hash operation context \p c have been released.
 * If sx_hash_digest() was used, \p c cannot be used for a new operation
 * until one of the sx_hash_create_*() functions is called again.
 * If sx_hash_save_state() was used, then \p c shall be reused with
 * sx_hash_resume().
 *
 * @param[in,out] c hash operation context
 * @return ::SX_OK
 * @return ::SX_ERR_UNINITIALIZED_OBJ
 * @return ::SX_ERR_HW_PROCESSING
 * @return ::SX_ERR_DMA_FAILED
 *
 * @pre - sx_hash_digest() functions must be called first
 * @pre - if in context-saving, sx_hash_save_state() must be called first.
 */
int sx_hash_status(struct sxhash *c);

/** Returns digest size in bytes for the hash operation context.
 *
 * @param[in] c hash operation context
 * @return digest size in bytes of the algorithm specified by \p c
 *
 * @pre - one of the sx_hash_create_*() functions must be called first
 */
size_t sx_hash_get_digestsz(struct sxhash *c);

/** Returns block size in bytes for the hash operation context.
 *
 * @param[in] c hash operation context
 * @return block size in bytes of the algorithm specified by \p c
 *
 * @pre - one of the sx_hash_create_*() functions must be called first
 */
size_t sx_hash_get_blocksz(struct sxhash *c);

/** Free a created hash operation context.
 *
 * A created hash operation context can have reserved hardware and
 * software resources. Those are free-ed automatically on error
 * or when the operation finishes as reported by sx_hash_status() or
 * sx_hash_wait(). If for some reason, the hash operation will not
 * be started, it can be abandoned which will release all reserved
 * resources.
 *
 * Can only be called after a call to one of the sx_hash_create_*() or
 * after sx_hash_resume(). It must be called *BEFORE* starting to run
 * the operation with sx_hash_digest() or sx_hash_save_state().
 *
 * @param[in,out] c hash operation context
 */
void sx_hash_free(struct sxhash *c);

#ifdef __cplusplus
}
#endif

#endif
