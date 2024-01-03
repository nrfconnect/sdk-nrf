/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include <cstdint>

namespace FactoryResetConsts
{
constexpr uint32_t kFactoryResetTriggerTimeout = 3000;
constexpr uint32_t kFactoryResetCancelWindowTimeout = 3000;
} /* namespace FactoryResetConsts */

namespace AdvertisingConsts
{
#if NUMBER_OF_BUTTONS == 2
constexpr uint32_t kAdvertisingTriggerTimeout = 3000;
#endif
} // namespace AdvertisingConsts

namespace LedConsts
{
constexpr uint32_t kBlinkRate_ms{ 500 };
constexpr uint32_t kIdentifyBlinkRate_ms{ 500 };
namespace StatusLed
{
	namespace BleConnected
	{
		constexpr uint32_t kOn_ms{ 100 };
		constexpr uint32_t kOff_ms{ kOn_ms };
	} /* namespace BleConnected */
	namespace Disconnected
	{
		constexpr uint32_t kOn_ms{ 50 };
		constexpr uint32_t kOff_ms{ 950 };
	} /* namespace Disconnected */

} /* namespace StatusLed */
} /* namespace LedConsts */
