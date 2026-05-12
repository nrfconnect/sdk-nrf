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

#include "tfm_log.h"

#define LOG_MODULE_DECLARE(...)
#define LOG_MODULE_REGISTER(...)

#define LOG_ERR(fmt, ...) ERROR(fmt "\r\n", ##__VA_ARGS__)
#define LOG_WRN(fmt, ...) WARN(fmt "\r\n", ##__VA_ARGS__)
#define LOG_INF(fmt, ...) INFO(fmt "\r\n", ##__VA_ARGS__)
#define LOG_DBG(fmt, ...) VERBOSE(fmt "\r\n", ##__VA_ARGS__)

/* This can be used for simple messages before the SPM is initialized in thread
 * mode. The legacy SPMLOG_*MSG macros are no longer available; route these to
 * the unprivileged logging API as well, which is the closest equivalent.
 */
#define LOG_ERR_MSG(msg) ERROR(msg "\r\n")
#define LOG_INF_MSG(msg) INFO(msg "\r\n")
#define LOG_DBG_MSG(msg) VERBOSE(msg "\r\n")

#endif /* __ZEPHYR_LOGGING_LOG_H__ */
