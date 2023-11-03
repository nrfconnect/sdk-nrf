/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef __FOO_H
#define __FOO_H

#include <zephyr/toolchain.h>

int foo_init(void *handle);

int foo_execute(void);

#endif /* __FOO_H */
