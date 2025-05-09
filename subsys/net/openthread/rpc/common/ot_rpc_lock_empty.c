/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "ot_rpc_lock.h"

#include <zephyr/toolchain.h>

__weak void ot_rpc_mutex_lock(void)
{
}

__weak void ot_rpc_mutex_unlock(void)
{
}
