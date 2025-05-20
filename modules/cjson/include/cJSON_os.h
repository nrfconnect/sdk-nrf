/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef cJSON_OS_H__
#define cJSON_OS_H__

#include <stdint.h>

/**
 * @brief Initialize cJSON with OS hooks.
 */
void cJSON_Init(void);

/**
 * @brief Free a string created and returned by cJSON.
 * @return returns the return value of the free implementation.
 * @param ptr IN -- pointer to string to free
 */
void cJSON_FreeString(char *ptr);

#endif /* cJSON_OS_H__ */
