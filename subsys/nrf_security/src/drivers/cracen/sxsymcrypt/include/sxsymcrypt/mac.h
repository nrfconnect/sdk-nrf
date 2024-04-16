/** Common functions used for generation a MAC (message authentication code).
 *
 * @file
 * @copyright Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MAC_HEADER_FILE
#define MAC_HEADER_FILE

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include "internal.h"

/** Feeds data to be used for MAC generation.
 *
 * The function will return immediately.
 *
 * In order to start the operation sx_mac_generate() must be called.
 *
 * @param[in,out] c MAC operation context
 * @param[in] datain data to be processed, with size \p sz
 * @param[in] sz size, in bytes, of data to be processed
 * @return ::SX_OK
 * @return ::SX_ERR_UNINITIALIZED_OBJ
 * @return ::SX_ERR_TOO_BIG
 * @return ::SX_ERR_FEED_COUNT_EXCEEDED
 *
 * @pre - sx_mac_create_*() function must be called first
 *
 * @remark - this function can be called even if data size, \p sz, is 0.
 * @remark - this function can be called multiple times to feed multiple chunks
 *           scattered in memory.
 */
int sx_mac_feed(struct sxmac *c, const char *datain, size_t sz);

/** Starts MAC generation operation.
 *
 * This function is used to start MAC generation.
 * The function will return immediately.
 *
 * The result will be transferred only after the operation is successfully
 * completed. The user shall check operation status with sx_mac_status()
 * or sx_mac_wait().
 *
 * @param[in,out] c MAC operation context
 * @param[out] mac generated MAC
 * @return ::SX_OK
 * @return ::SX_ERR_UNINITIALIZED_OBJ
 * @return ::SX_ERR_OUTPUT_BUFFER_TOO_SMALL
 *
 * @pre - sx_mac_feed() function must be called first
 *
 * @remark - if used with context saving(last chunk), the fed data size for
 *         the last chunk can not be 0
 */
int sx_mac_generate(struct sxmac *c, char *mac);

/** Resumes MAC operation in context-saving.
 *
 * This function shall be called when using context-saving to load the state
 * that was previously saved by sx_mac_save_state() in the sxmac operation
 * context \p c. It must be called with the same sxmac operation context \p c
 * that was used with sx_mac_save_state(). It will reserve all hardware
 * resources required to run the partial MAC operation.
 *
 * After successful execution of this function, the context \p c can be passed
 * to any of the block cipher functions, except the sx_mac_create_*()
 * functions.
 *
 * @param[in,out] c block cipher operation context
 * @return ::SX_OK
 * @return ::SX_ERR_UNINITIALIZED_OBJ
 * @return ::SX_ERR_CONTEXT_SAVING_NOT_SUPPORTED
 * @return ::SX_ERR_INVALID_KEYREF
 * @return ::SX_ERR_INVALID_KEY_SZ
 * @return ::SX_ERR_INCOMPATIBLE_HW
 * @return ::SX_ERR_RETRY
 *
 * @pre - sx_mac_create_*() and sx_mac_save_state() functions
 *        must be called before, for first part of the message.
 * @pre - must be called for each part of the message(besides first), before
 *        sx_mac_crypt() or sx_mac_save_state().
 *
 * @remark - the user must not change the key until all parts of the message to
 *           be encrypted/decrypted are processed.
 */
int sx_mac_resume_state(struct sxmac *c);

/** Starts a partial MAC operation.
 *
 * This function is used to start a partial MAC operation on data fed using
 * sx_mac_feed().
 * The function will return immediately.
 *
 * The partial result will be transferred only after the operation is
 * successfully completed. The user shall check operation status with
 * sx_mac_status() or sx_mac_wait().
 *
 * @param[in,out] c block cipher operation context
 * @return ::SX_OK
 * @return ::SX_ERR_UNINITIALIZED_OBJ
 * @return ::SX_ERR_CONTEXT_SAVING_NOT_SUPPORTED
 * @return ::SX_ERR_INPUT_BUFFER_TOO_SMALL
 * @return ::SX_ERR_WRONG_SIZE_GRANULARITY
 *
 * @pre - sx_mac_crypt() should be called first.
 *
 * @remark - the user must not change the key until all parts of the message to
 *           be encrypted/decrypted are processed.
 */
int sx_mac_save_state(struct sxmac *c);

/** Waits until the given MAC generation operation has finished
 *
 * This function returns when the MAC generation operation was successfully
 * completed, or when an error has occurred that caused the operation to
 * terminate. The return value of this function is the operation status.
 *
 * After this call, all resources have been released and \p c cannot be used
 * again unless sx_mac_create_*() is used.
 *
 * @param[in,out] c MAC generation operation context
 * @return ::SX_OK
 * @return ::SX_ERR_UNINITIALIZED_OBJ
 * @return ::SX_ERR_DMA_FAILED
 * @return ::SX_ERR_INVALID_TAG
 *
 * @see sx_mac_status().
 *
 * @remark - this function is blocking until operation finishes.
 */
int sx_mac_wait(struct sxmac *c);

/** Returns the MAC generation operation status.
 *
 * If the operation is still ongoing, return ::SX_ERR_HW_PROCESSING.
 * In that case, the user can retry later.
 *
 * When this function returns with a code different than ::SX_ERR_HW_PROCESSING,
 * the MAC generation operation has ended and all resources used by MAC
 * generation operation context \p c have been released. In this case, \p c
 * cannot be used for a new operation until one of the sx_mac_create_*()
 * functions is called again.
 *
 * @param[in,out] c MAC operation context
 * @return ::SX_OK
 * @return ::SX_ERR_UNINITIALIZED_OBJ
 * @return ::SX_ERR_HW_PROCESSING
 * @return ::SX_ERR_DMA_FAILED
 * @return ::SX_ERR_INVALID_TAG
 *
 * @pre - sx_mac_feed() and sx_mac_generate() functions must be called first
 */
int sx_mac_status(struct sxmac *c);

/** Free the MAC operation.
 *
 * This functions will free the MAC operation and release the reserved hardware.
 *
 * @param[in,out] c MAC operation context
 *
 */
void sx_mac_free(struct sxmac *c);

/** Find an available MAC engine for the operation.
 *
 * @param[in,out] c MAC operation context
 * @param[in]     mode_compatible Compatible hardware for the operation
 *
 * @return ::SX_OK
 * @return ::SX_ERR_DMA_FAILED
 * @return ::SX_ERR_UNKNOWN_ERROR
 *
 */
int sx_mac_hw_reserve(struct sxmac *c);

#ifdef __cplusplus
}
#endif

#endif
