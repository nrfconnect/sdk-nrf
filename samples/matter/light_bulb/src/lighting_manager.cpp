/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "lighting_manager.h"

#include <zephyr/drivers/pwm.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/zephyr.h>

LOG_MODULE_DECLARE(app, CONFIG_MATTER_LOG_LEVEL);

LightingManager LightingManager::sLight;

int LightingManager::Init(const pwm_dt_spec *pwmDevice, uint8_t minLevel, uint8_t maxLevel)
{
	mState = State::On;
	mMinLevel = minLevel;
	mMaxLevel = maxLevel;
	mLevel = maxLevel;
	mPwmDevice = pwmDevice;

	if (!device_is_ready(mPwmDevice->dev)) {
		LOG_ERR("PWM device %s is not ready", mPwmDevice->dev->name);
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
	const uint8_t maxEffectiveLevel = mMaxLevel - mMinLevel;
	const uint8_t effectiveLevel = mState == State::On ? MIN(mLevel - mMinLevel, maxEffectiveLevel) : 0;

	pwm_set_pulse_dt(mPwmDevice, static_cast<uint32_t>(static_cast<const uint64_t>(mPwmDevice->period) * effectiveLevel / maxEffectiveLevel));
}
