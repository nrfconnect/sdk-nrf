/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "led_widget.h"

#include <array>

/* A lightweight wrapper for factory reset LEDs */
template <uint8_t size> class FactoryResetLEDsWrapper {
public:
	using Gpio = uint32_t;
	using Leds = std::array<std::pair<Gpio, Nrf::LEDWidget>, size>;

	explicit FactoryResetLEDsWrapper(std::array<Gpio, size> aLeds)
	{
		auto idx{ 0 };
		for (const auto &led : aLeds)
			mLeds[idx++] = std::make_pair(led, Nrf::LEDWidget());
		Init();
	}
	void Set(bool aState)
	{
		for (auto &led : mLeds)
			led.second.Set(aState);
	}
	void Blink(uint32_t aRate)
	{
		for (auto &led : mLeds)
			led.second.Blink(aRate);
	}

private:
	void Init()
	{
		Nrf::LEDWidget::InitGpio();
		for (auto &led : mLeds)
			led.second.Init(led.first);
	}
	Leds mLeds;
};
