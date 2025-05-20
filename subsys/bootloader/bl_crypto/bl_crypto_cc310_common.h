/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef BOOTLOADER_CRYPTO_CC310_COMMON_H__
#define BOOTLOADER_CRYPTO_CC310_COMMON_H__

#include <stddef.h>

int cc310_bl_init(void);

void cc310_bl_backend_enable(void);

void cc310_bl_backend_disable(void);

#endif
