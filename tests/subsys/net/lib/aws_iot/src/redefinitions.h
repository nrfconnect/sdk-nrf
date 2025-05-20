/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/sys/util.h>
#include <stdbool.h>

#ifndef REDEFINITION_H_
#define REDEFINITION_H_

/* Redefine IS_ENABLED so that it always evaluates to true in the UUT. */
#undef IS_ENABLED
#define IS_ENABLED(x) true

#endif /* REDEFINITION_ */
