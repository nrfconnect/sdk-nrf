/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "board/board.h"

namespace Nrf::Matter
{
namespace Certification
{

	void ButtonHandler(Nrf::ButtonState state, Nrf::ButtonMask hasChanged);

} /* namespace Certification */
} /* namespace Nrf::Matter */
