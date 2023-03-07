/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef __ZEPHYR_LOGGING_LOG_H__
#define __ZEPHYR_LOGGING_LOG_H__

/* Compatebility header for using Zephyr API in TF-M.
 *
 * The macros and functions here can be used by code that is common for both
 * Zephyr and TF-M RTOS.
 *
 * The functionality will be forwarded to TF-M equivalent of the Zephyr API.
 */

#include "tfm_sp_log.h"

#define LOG_MODULE_DECLARE(...)
#define LOG_MODULE_REGISTER(...)

#define LOG_ERR(...) LOG_ERRFMT(__VA_ARGS__)
#define LOG_WRN(...) LOG_WRNFMT(__VA_ARGS__)
#define LOG_INF(...) LOG_INFFMT(__VA_ARGS__)
#define LOG_DBG(...) LOG_DBGFMT(__VA_ARGS__)

#endif /* __ZEPHYR_LOGGING_LOG_H__ */
