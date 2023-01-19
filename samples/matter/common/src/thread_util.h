/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include <lib/core/CHIPError.h>

#ifdef CONFIG_OPENTHREAD_DEFAULT_TX_POWER
CHIP_ERROR SetDefaultThreadOutputPower();
#endif
