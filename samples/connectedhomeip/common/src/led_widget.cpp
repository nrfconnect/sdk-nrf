/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "led_widget.h"

#include <dk_buttons_and_leds.h>
#include <zephyr.h>

void LEDWidget::InitGpio()
{
	dk_leds_init();
}

void LEDWidget::Init(uint32_t gpioNum)
{
	mLastChangeTimeMS = 0;
	mBlinkOnTimeMS = 0;
	mBlinkOffTimeMS = 0;
	mGPIONum = gpioNum;
	mState = false;
	Set(false);
}

void LEDWidget::Invert()
{
	Set(!mState);
}

void LEDWidget::Set(bool state)
{
	mLastChangeTimeMS = mBlinkOnTimeMS = mBlinkOffTimeMS = 0;
	DoSet(state);
}

void LEDWidget::Blink(uint32_t changeRateMS)
{
	Blink(changeRateMS, changeRateMS);
}

void LEDWidget::Blink(uint32_t onTimeMS, uint32_t offTimeMS)
{
	mBlinkOnTimeMS = onTimeMS;
	mBlinkOffTimeMS = offTimeMS;
	Animate();
}

void LEDWidget::Animate()
{
	if (mBlinkOnTimeMS != 0 && mBlinkOffTimeMS != 0) {
		int64_t nowMS = k_uptime_get();
		int64_t stateDurMS = mState ? mBlinkOnTimeMS : mBlinkOffTimeMS;

		if (nowMS > mLastChangeTimeMS + stateDurMS) {
			DoSet(!mState);
			mLastChangeTimeMS = nowMS;
		}
	}
}

void LEDWidget::DoSet(bool state)
{
	mState = state;
	dk_set_led(mGPIONum, state);
}
