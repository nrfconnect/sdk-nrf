/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include <cstdint>

class LEDWidget {
public:
	static void InitGpio();
	void Init(uint32_t gpioNum);
	void Set(bool state);
	void Invert();
	void Blink(uint32_t changeRateMS);
	void Blink(uint32_t onTimeMS, uint32_t offTimeMS);
	void Animate();

private:
	int64_t mLastChangeTimeMS;
	uint32_t mBlinkOnTimeMS;
	uint32_t mBlinkOffTimeMS;
	uint32_t mGPIONum;
	bool mState;

	void DoSet(bool state);
};
