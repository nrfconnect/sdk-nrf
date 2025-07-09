/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "board/board.h"

#include <app/clusters/identify-server/identify-server.h>

namespace Nrf::Matter
{
namespace Certification
{
	void ButtonHandler(Nrf::ButtonState state, Nrf::ButtonMask hasChanged);
	void IdentifyStartHandler(Identify *);
	void IdentifyStopHandler(Identify *);
	void TriggerIdentifyEffectHandler(Identify *);
} /* namespace Certification */
} /* namespace Nrf::Matter */
