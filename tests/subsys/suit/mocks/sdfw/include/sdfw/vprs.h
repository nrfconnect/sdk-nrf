/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */


#ifndef VPRS_H__
#define VPRS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>

/**
 * @brief       Starts the System Controller firmware using the information from DTS
 *              and provided start address.
 *              The start address parameter allows to change it within SUIT manifest.
 *
 * @return      Non negative integer for success.
 *              Negative integer for failure.
 */
int vprs_sysctrl_start(uintptr_t start_address);

#ifdef __cplusplus
}
#endif

#endif /* VPRS_H__ */
