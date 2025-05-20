/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef EXTMEM_REMOTE_H__
#define EXTMEM_REMOTE_H__

#ifdef __cplusplus
extern "C" {
#endif

#define EXTMEM_RESULT_SUCCESS 0 /**< The operation completed successfully. */
#define EXTMEM_RESULT_REMOTE_ERROR 2 /**< The remote failed to execute the operation. */

/** @brief Initialize external memory service.
 *
 * @details Initialize the external memory service state.
 *
 * @retval 0 if successful.
 * @retval -EIO if service could not be initialized.
 */
int extmem_remote_init(void);

#ifdef __cplusplus
}
#endif

#endif /* EXTMEM_REMOTE_H__ */
