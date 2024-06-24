/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef LOG_RPC_H_
#define LOG_RPC_H_

#include <stddef.h>

/**
 * @file
 * @defgroup log_rpc nRF RPC logging
 * @{
 * @brief Logging over nRF RPC
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Retrieves the crash log retained on the remote device.
 *
 * The function issues an nRF RPC command to obtain the last crash log retained
 * on the remote device, and then it copies the received log chunk into the
 * specified buffer.
 *
 * The remote may either send the entire crash log in one go, if it fits within
 * the output buffer, or it may send a chunk of the log. Therefore, the caller
 * should repeatedly call this function, with an increasing offset, until it
 * returns 0 (indicating no more data), or a negative value (indicated an error).
 *
 * @param offset        Crash log offset.
 * @param buffer        Output buffer to copy the received log chunk.
 * @param buffer_length Output buffer length.
 *
 * @returns             The number of characters copied into the output buffer.
 * @returns -errno      Indicates failure.
 */
int log_rpc_get_crash_log(size_t offset, char *buffer, size_t buffer_length);

#ifdef __cplusplus
}
#endif

/**
 *@}
 */

#endif /* LOG_RPC_H_ */
