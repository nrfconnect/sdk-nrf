/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef RP_LOG_H_
#define RP_LOG_H_

#ifdef __cplusplus
extern "C" {
#endif

#define RP_MODULE_PREFIX  _CONCAT(RP_, RP_LOG_MODULE)

#include <logging/log.h>
LOG_MODULE_REGISTER(RP_MODULE_PREFIX, CONFIG_RP_SER_LOG_LEVEL);

/**
 * @brief Macro for logging a message with the severity level ERR.
 *
 * @param ... printf-style format string, optionally followed by arguments
 *            to be formatted and inserted in the resulting string.
 */
#define RP_LOG_ERR(...)  LOG_ERR(__VA_ARGS__)

/**
 * @brief Macro for logging a message with the severity level WRN.
 *
 * @param ... printf-style format string, optionally followed by arguments
 *            to be formatted and inserted in the resulting string.
 */
#define RP_LOG_WRN(...)  LOG_WRN(__VA_ARGS__)

/**
 * @brief Macro for logging a message with the severity level INF.
 *
 * @param ... printf-style format string, optionally followed by arguments
 *            to be formatted and inserted in the resulting string.
 */
#define RP_LOG_INF(...)  LOG_INF(__VA_ARGS__)

/**
 * @brief Macro for logging a message with the severity level DBG.
 *
 * @param ... printf-style format string, optionally followed by arguments
 *            to be formatted and inserted in the resulting string.
 */
#define RP_LOG_DBG(...)  LOG_DBG(__VA_ARGS__)

/**
 * @brief Macro for logging a memory dump with the severity level ERR.
 *
 * @param[in] p_memory Pointer to the memory region to be dumped.
 * @param[in] length   Length of the memory region in bytes.
 */
#define RP_LOG_HEXDUMP_ERR(p_memory, length) \
	LOG_HEXDUMP_ERR(p_memory, length, "")

/**
 * @brief Macro for logging a memory dump with the severity level WRN.
 *
 * @param[in] p_memory Pointer to the memory region to be dumped.
 * @param[in] length   Length of the memory region in bytes.
 */
#define RP_LOG_HEXDUMP_WRN(p_memory, length) \
	LOG_HEXDUMP_WRN(p_memory, length, "")

/**
 * @brief Macro for logging a memory dump with the severity level INF.
 *
 * @param[in] p_memory Pointer to the memory region to be dumped.
 * @param[in] length   Length of the memory region in bytes.
 */
#define RP_LOG_HEXDUMP_INF(p_memory, length) \
	LOG_HEXDUMP_INF(p_memory, length, "")

/**
 * @brief Macro for logging a memory dump with the severity level DBG.
 *
 * @param[in] p_memory Pointer to the memory region to be dumped.
 * @param[in] length   Length of the memory region in bytes.
 */
#define RP_LOG_HEXDUMP_DBG(p_memory, length) \
	LOG_HEXDUMP_DBG(p_memory, length, "")

/**
 *@}
 */

#ifdef __cplusplus
}
#endif

#endif /* RP_LOG_H_ */
