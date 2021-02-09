/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "lighting_manager.h"

#include <logging/log.h>

#include <drivers/pwm.h>
#include <zephyr.h>

LOG_MODULE_DECLARE(app);

LightingManager LightingManager::sLight;

int LightingManager::Init(const char *pwmDeviceName, uint32_t pwmChannel)
{
	mState = State::On;
	mLevel = kMaxLevel;
	mPwmDevice = device_get_binding(pwmDeviceName);
	mPwmChannel = pwmChannel;

	if (!mPwmDevice) {
		LOG_ERR("Cannot find PWM device %s", log_strdup(pwmDeviceName));
		return -ENODEV;
	}

	Set(false);
	return 0;
}

void LightingManager::SetCallbacks(LightingCallback_fn aActionInitiated_CB, LightingCallback_fn aActionCompleted_CB)
{
	mActionInitiated_CB = aActionInitiated_CB;
	mActionCompleted_CB = aActionCompleted_CB;
}

bool LightingManager::InitiateAction(Action aAction, uint8_t value, bool chipInitiated)
{
	bool actionInitiated = false;
	State newState;

	/* Initiate On/Off Action only when the previous one is complete. */
	if (mState == State::Off && aAction == Action::On) {
		actionInitiated = true;
		newState = State::On;
	} else if (mState == State::On && aAction == Action::Off) {
		actionInitiated = true;
		newState = State::Off;
	} else if (aAction == Action::Level && value != mLevel) {
		actionInitiated = true;
		newState = (value == 0) ? State::Off : State::On;
	}

	if (actionInitiated) {
		if (mActionInitiated_CB) {
			mActionInitiated_CB(aAction);
		}

		if (aAction == Action::On || aAction == Action::Off) {
			Set(newState == State::On);
		} else if (aAction == Action::Level) {
			SetLevel(value);
		}

		mChipInitiatedAction = chipInitiated;

		if (mActionCompleted_CB) {
			mActionCompleted_CB(aAction);
		}
	}

	return actionInitiated;
}

void LightingManager::SetLevel(uint8_t aLevel)
{
	LOG_INF("Setting brightness level to %u", aLevel);
	mLevel = aLevel;
	UpdateLight();
}

void LightingManager::Set(bool aOn)
{
	mState = aOn ? State::On : State::Off;
	UpdateLight();
}

void LightingManager::UpdateLight()
{
	constexpr uint32_t kPwmWidthUs = 20000u;
	const uint8_t level = mState == State::On ? mLevel : 0;
	pwm_pin_set_usec(mPwmDevice, mPwmChannel, kPwmWidthUs, kPwmWidthUs * level / kMaxLevel, 0);
}
