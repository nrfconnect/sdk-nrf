/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#ifdef CONFIG_CHIP_LIB_SHELL
#include <lib/core/CHIPError.h>
#include <lib/shell/Engine.h>
#include <lib/shell/commands/Help.h>

namespace SwitchCommands
{
void RegisterSwitchCommands();

} /* namespace SwitchCommands */

#endif
