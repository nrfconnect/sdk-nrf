/** AES counter-measures mask load
 *
 * @file
 * @copyright Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * Examples:
 * The following example shows typical sequence of function calls for
 * loading the counter-measure mask.
   @code
       sx_cm_load_mask(ctx, value)
       sx_cm_load_mask_wait(ctx)
   @endcode
 */

#ifndef CMMASK_HEADER_FILE
#define CMMASK_HEADER_FILE

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include "internal.h"

/** Loads random used in the AES counter-measures.
 *
 * Counter-measures are available for the AES operations and
 * require a secure random value to operate.
 * This function loads the cryptographically secure random \p value
 * to the counter-measures mask operation.
 * It is required that the relevant hardware is enabled and
 * reserved before this function is called.
 *
 * This function returns when the counter-measures load operation was
 * successfully completed, or when an error has occurred that caused the
 * operation to terminate.
 *
 * No resources are released on return of this function. It's the caller's
 * responsibility to release the resources.
 *
 *
 * @param[in]     csprng_value counter-measures random value to be loaded
 *
 * @remark - this function is blocking until operation finishes.
 *
 * @remark - it is under the user responsibility to call it after Cracen power on
 *           (not automatically called).
 * @remark - for more details see the technical report: "Secure and Efficient
 *           Masking of AES - A Mission Impossible?", June 2004)
 *
 * @return ::SX_OK
 * @return ::SX_ERR_DMA_FAILED
 */
int sx_cm_load_mask(uint32_t csprng_value);

#ifdef __cplusplus
}
#endif

#endif
