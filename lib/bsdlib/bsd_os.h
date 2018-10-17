/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/**
 * @file bsd_os.h
 * @brief OS specific definitions.
 */
#ifndef BSD_OS_H__
#define BSD_OS_H__

#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* TODO: add documentation in this file */

void bsd_os_init(void);

s32_t bsd_os_timedwait(u32_t context, u32_t timeout);

void bsd_os_errno_set(int errno_val);

void bsd_os_application_irq_clear(void);

void bsd_os_application_irq_set(void);

extern void bsd_os_application_irq_handler(void);

#ifdef __cplusplus
}
#endif

#endif /* BSD_OS_H__ */

/** @} */
