/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef OT_RPC_MACROS_H
#define OT_RPC_MACROS_H

#include <stddef.h>

#define OT_RPC_UNUSED(x) (void)(x)
#define OT_RPC_ARRAY_SIZE(x) ((size_t) (sizeof((x)) / sizeof((x)[0])))

#endif /* OT_RPC_MACROS_H */
