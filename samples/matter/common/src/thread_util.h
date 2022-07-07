/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include <cstdint>
#include <core/CHIPError.h>

void StartDefaultThreadNetwork(uint64_t datasetTimestamp = 0);

#ifdef CONFIG_OPENTHREAD_DEFAULT_TX_POWER
CHIP_ERROR SetDefaultThreadOutputPower();
#endif
