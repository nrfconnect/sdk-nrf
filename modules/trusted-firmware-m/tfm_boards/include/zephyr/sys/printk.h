/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef __ZEPHYR_SYS_PRINTK_H
#define __ZEPHYR_SYS_PRINTK_H

/* Compatebility header for using Zephyr API in TF-M.
 *
 * The macros and functions here can be used by code that is common for both
 * Zephyr and TF-M RTOS.
 *
 * The functionality will be forwarded to TF-M equivalent of the Zephyr API.
 */

#include "tfm_sp_log.h"

#define printk(fmt, ...) printf(fmt, ##__VA_ARGS__)

#endif /* __ZEPHYR_SYS_PRINTK_H */
