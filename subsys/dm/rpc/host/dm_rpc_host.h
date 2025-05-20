/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef DM_RPC_HOST_H_
#define DM_RPC_HOST_H_

#include <zephyr/kernel.h>
#include <stdlib.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Allocation of the buffer in the shared memory.
 *
 * @param[in] buffer_len Buffer size.
 *
 * @retval On success, a pointer to the memory block.
 *         If the function fails, a NULL pointer is returned.
 */
void *dm_rpc_get_buffer(size_t buffer_len);

/** @brief Send measurement data and wait for the client side to finish processing it.
 *
 * @param[in] data Pointer to data.
 * @param[in] len Data size.
 */
void dm_rpc_calc_and_process(const void *data, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* DM_RPC_HOST_H_ */
