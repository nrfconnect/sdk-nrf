/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "board/board.h"

namespace Nrf
{
namespace Matter
{
	namespace Certification
	{
		void Init();
		void ButtonHandler(Nrf::ButtonState state, Nrf::ButtonMask hasChanged);
	} /* namespace Certification */
} /* namespace Matter */
} /* namespace Nrf */
