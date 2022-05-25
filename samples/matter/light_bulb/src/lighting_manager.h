/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "app_event.h"

#include <cstdint>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pwm.h>

class LightingManager {
public:
	enum class Action : uint8_t { On, Off, Level };

	enum class State : uint8_t { On, Off };

	using LightingCallback_fn = void (*)(Action);

	int Init(const pwm_dt_spec *pwmDevice, uint8_t minLevel, uint8_t maxLevel);
	bool IsTurnedOn() const { return mState == State::On; }
	uint8_t GetLevel() const { return mLevel; }
	bool IsActionChipInitiated() const { return mChipInitiatedAction; }
	bool InitiateAction(Action aAction, uint8_t value, bool chipInitiated);
	void SetCallbacks(LightingCallback_fn aActionInitiated_CB, LightingCallback_fn aActionCompleted_CB);

private:
	friend LightingManager &LightingMgr();
	State mState;
	uint8_t mMinLevel;
	uint8_t mMaxLevel;
	uint8_t mLevel;
	const pwm_dt_spec *mPwmDevice;
	uint32_t mPwmChannel;
	bool mChipInitiatedAction;

	LightingCallback_fn mActionInitiated_CB;
	LightingCallback_fn mActionCompleted_CB;

	void Set(bool aOn);
	void SetLevel(uint8_t aLevel);
	void UpdateLight();

	static LightingManager sLight;
};

inline LightingManager &LightingMgr(void)
{
	return LightingManager::sLight;
}
