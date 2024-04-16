/** Functions for sicrypto tasks and environment.
 *
 * @file
 *
 * @copyright Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SICRYPTO_HEADER_FILE
#define SICRYPTO_HEADER_FILE

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <cracen/statuscodes.h>
#include "internal.h"

struct sx_pk_cnx;

/** Asymmetric encryption plaintext or ciphertext.
 *
 * This structure is used to represent plaintexts and ciphertexts in asymmetric
 * (i.e. public key) encryption and decryption operations. It is used with the
 * tasks that implement the RSAES-OAEP and RSAES-PKCS1-v1_5 encryption
 * schemes.
 */
struct si_ase_text {
	char *addr;
	size_t sz;
};

/** Close a sicrypto environment.
 *
 * After execution of this function, \p env is no longer usable. The tasks
 * that were linked to \p env should no longer be used after \p env has been
 * closed.
 *
 * @param[in,out] env      Sicrypto environment to close.
 */
void si_env_close(struct sienv *env);

/** Initialize a task structure.
 *
 * Each sitask structure needs to be initialized at least once with this
 * function. Such initialization must be done before the sitask structure is
 * passed for the first time to a task creation function.
 *
 * After a task has completed processing and is no longer needed, its sitask
 * structure can be reused for another operation (it can be passed to another
 * create function) without having to call si_task_init() again.
 *
 * This function performs initializations, it links the task to the \p env
 * environment and to the \p workmem buffer.
 *
 * The \p workmem buffer must reside in DMA accessible memory. This buffer will
 * be used by each operation that is carried out using the \p t structure.
 *
 * @param[in,out] t        Task structure to initialize.
 * @param[in] workmem      Buffer that the task will use to store intermediate
 *                         results.
 * @param[in] workmemsz    Size in bytes of the \p workmem buffer.
 */
void si_task_init(struct sitask *t, char *workmem, size_t workmemsz);

/** Return the status of a task.
 *
 * This function can be called at any moment after task creation to get the
 * status code of the task.
 *
 * @param[in,out] t        Task structure to use.
 * @return                 The value of the task's status code.
 */
int si_task_status(struct sitask *t);

/** Wait for a running task to complete and return the status code value.
 *
 * This function can be called at any moment after task creation. If the task is
 * running, the function waits for the task execution to complete and then
 * returns the final task status. If the task was created but not yet started,
 * or if the task's execution is already complete, this function immediately
 * returns the task status.
 *
 * @param[in,out] t        Task structure to use.
 * @return                 The value of the task's status code.
 */
int si_task_wait(struct sitask *t);

/** Provide a task with a buffer of "raw" input data.
 *
 * This function is used for tasks that need to be given an input message on
 * which to compute a hash, a MAC or a signature. It can also be used with a
 * random number generator task, to give the entropy that the task needs for
 * seeding and reseeding. See the documentation of the different task types for
 * more details on which tasks use this function and which do not.
 *
 * A create task function must have been called on the \p t structure before
 * \p t can be passed to the si_task_consume() function.
 *
 * @param[in,out] t        Task structure to use.
 * @param[in] data         Input data buffer.
 * @param[in] sz           Size in bytes of the input data buffer.
 *
 * The buffer pointed by \p data must reside in DMA memory and it must be
 * preserved until the task's execution is complete.
 */
void si_task_consume(struct sitask *t, const char *data, size_t sz);

/** Start the execution of a task.
 *
 * Refer to the documentation of the different task types to see which tasks are
 * started with si_task_run() and which with si_task_produce().
 *
 * Before calling si_task_run(), the task must have been created and provided
 * with the data it needs to perform its operation.
 *
 * @param[in,out] t        Task structure to use.
 */
void si_task_run(struct sitask *t);

/** Start the execution of a task by requesting it to produce output data.
 *
 * Refer to the documentation of the different task types to see which tasks are
 * started with si_task_run() and which with si_task_produce().
 *
 * Before calling si_task_produce(), the task must have been created and
 * provided with the data it needs to perform its operation.
 *
 * @param[in,out] t        Task structure to use.
 * @param[out] out         Buffer where the task's output data is written. The
 *                         size of the buffer must be \p sz bytes.
 * @param[in] sz           Size in bytes of the output data that the task is
 *                         requested to produce.
 *
 * The buffer pointed by \p out must reside in DMA memory.
 */
void si_task_produce(struct sitask *t, char *out, size_t sz);

/** Start the execution of a partial processing step of a task.
 *
 * When this function is used to start a task, the task will process the data
 * that was provided to it until that moment. After that processing is complete,
 * the task is ready to receive more input data and perform another processing
 * step. This way the task can be used in an "incremental processing" way.
 *
 * Not all tasks support the use of si_task_partial_run(): refer to the
 * documentation of the different task types to see which ones support
 * si_task_partial_run().
 *
 * Before calling si_task_partial_run(), the task must have been created and
 * provided with the data it needs to perform its operation.
 *
 * @param[in,out] t        Task structure to use.
 */
void si_task_partial_run(struct sitask *t);

#ifdef __cplusplus
}
#endif

#endif
