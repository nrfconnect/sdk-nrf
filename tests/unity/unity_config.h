/* Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#ifndef UNITY_CONFIG_H__
#define UNITY_CONFIG_H__

#ifndef CONFIG_BOARD_NATIVE_POSIX
#include <stddef.h>
#include <stdio.h>
#define UNITY_EXCLUDE_SETJMP_H 1
#define UNITY_OUTPUT_CHAR(a) printf("%c", a)
#endif

#endif /* UNITY_CONFIG_H__ */
