/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "led_widget.h"

#ifdef CONFIG_DK_LIBRARY
#include <dk_buttons_and_leds.h>
#endif
#include <zephyr/kernel.h>

namespace Nrf {

static LEDWidget::LEDWidgetStateUpdateHandler sStateUpdateCallback;

void LEDWidget::InitGpio()
{
#ifdef CONFIG_DK_LIBRARY
	dk_leds_init();
#endif
}

void LEDWidget::SetStateUpdateCallback(LEDWidgetStateUpdateHandler stateUpdateCb)
{
	if (stateUpdateCb)
		sStateUpdateCallback = stateUpdateCb;
}

void LEDWidget::Init(uint32_t gpioNum)
{
#ifdef CONFIG_DK_LIBRARY
	mBlinkOnTimeMS = 0;
	mBlinkOffTimeMS = 0;
	mGPIONum = gpioNum;
	mState = false;

	k_timer_init(&mLedTimer, &LEDWidget::LedStateTimerHandler, nullptr);
	k_timer_user_data_set(&mLedTimer, this);

	Set(false);
#endif
}

void LEDWidget::Invert()
{
	Set(!mState);
}

void LEDWidget::Set(bool state)
{
#ifdef CONFIG_DK_LIBRARY
	k_timer_stop(&mLedTimer);
	mBlinkOnTimeMS = mBlinkOffTimeMS = 0;
	DoSet(state);
#endif
}

void LEDWidget::Blink(uint32_t changeRateMS)
{
	Blink(changeRateMS, changeRateMS);
}

void LEDWidget::Blink(uint32_t onTimeMS, uint32_t offTimeMS)
{
#ifdef CONFIG_DK_LIBRARY
	k_timer_stop(&mLedTimer);

	mBlinkOnTimeMS = onTimeMS;
	mBlinkOffTimeMS = offTimeMS;

	if (mBlinkOnTimeMS != 0 && mBlinkOffTimeMS != 0) {
		DoSet(!mState);
		ScheduleStateChange();
	}
#endif
}

void LEDWidget::ScheduleStateChange()
{
#ifdef CONFIG_DK_LIBRARY
	k_timer_start(&mLedTimer, K_MSEC(mState ? mBlinkOnTimeMS : mBlinkOffTimeMS), K_NO_WAIT);
#endif
}

void LEDWidget::DoSet(bool state)
{
#ifdef CONFIG_DK_LIBRARY
	mState = state;
	dk_set_led(mGPIONum, state);
#endif
}

void LEDWidget::UpdateState()
{
	/* Prevent from keep updating the state if LED was set to solid On/Off value */
	if (mBlinkOnTimeMS != 0 && mBlinkOffTimeMS != 0) {
		DoSet(!mState);
		ScheduleStateChange();
	}
}

void LEDWidget::LedStateTimerHandler(k_timer *timer)
{
	if (sStateUpdateCallback)
		sStateUpdateCallback(*reinterpret_cast<LEDWidget *>(timer->user_data));
}

} /* namespace Nrf */
