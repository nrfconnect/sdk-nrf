/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "garage_door_impl.h"
#include "closure_manager.h"
#include <app-common/zap-generated/attributes/Accessors.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

using namespace chip;
using namespace chip::DeviceLayer;

static constexpr uint32_t kMoveIntervalMs = 100;

GarageDoorImpl::GarageDoorImpl(const pwm_dt_spec *spec) : mSpec(spec) {}

CHIP_ERROR GarageDoorImpl::Init()
{
	if (mPhysicalIndicator.Init(mSpec, 0, 255) != 0) {
		LOG_ERR("Cannot initialize the physical indicator");
		return CHIP_ERROR_INCORRECT_STATE;
	}
	return CHIP_NO_ERROR;
}

CHIP_ERROR GarageDoorImpl::MoveTo(uint16_t position, uint16_t speed)
{
	LOG_DBG("Starting movement");
	mTargetPosition = position;
	mSpeed = speed;

	ReturnErrorOnFailure(
		SystemLayer().StartTimer(System::Clock::Milliseconds32(kMoveIntervalMs), TimerTimeoutCallback, this));
	mJustStarted = true;
	return CHIP_NO_ERROR;
}

CHIP_ERROR GarageDoorImpl::Stop()
{
	SystemLayer().CancelTimer(TimerTimeoutCallback, this);
	LOG_DBG("Movement stopped");
	mObserver->OnMovementStopped(mCurrentPosition);
	return CHIP_NO_ERROR;
}

void GarageDoorImpl::TimerTimeoutCallback(System::Layer *systemLayer, void *appState)
{
	auto *self = static_cast<GarageDoorImpl *>(appState);
	VerifyOrReturn(self != nullptr);
	self->HandleTimer();
}

void GarageDoorImpl::HandleTimer()
{
	bool finished = false;
	uint32_t movePerTick32 = (static_cast<uint32_t>(mSpeed) * kMoveIntervalMs) / 1000U;
	uint16_t movePerTick = static_cast<uint16_t>(std::min<uint32_t>(movePerTick32, UINT16_MAX));
	uint16_t distanceLeft = 0;
	if (mCurrentPosition <= mTargetPosition) {
		distanceLeft = mTargetPosition - mCurrentPosition;
		if (movePerTick >= distanceLeft) {
			finished = true;
			mCurrentPosition = mTargetPosition;
		} else {
			mCurrentPosition += movePerTick;
		}
	} else {
		distanceLeft = mCurrentPosition - mTargetPosition;
		if (movePerTick >= distanceLeft) {
			finished = true;
			mCurrentPosition = mTargetPosition;
		} else {
			mCurrentPosition -= movePerTick;
		}
	}

	uint8_t brightness = static_cast<uint8_t>((static_cast<uint32_t>(mCurrentPosition) * 255U + 5000U) / 10000U);

	mPhysicalIndicator.InitiateAction(Nrf::PWMDevice::LEVEL_ACTION, 0, &brightness);

	if (finished) {
		Stop();
	} else {
		uint32_t timeLeftMs = (static_cast<uint32_t>(distanceLeft) * kMoveIntervalMs) / movePerTick;
		uint16_t timeLeftS = static_cast<uint16_t>((timeLeftMs + 999) / 1000); /*ceil*/
		mObserver->OnMovementUpdate(mCurrentPosition, timeLeftS, mJustStarted);
		mJustStarted = false;
		auto err = SystemLayer().StartTimer(System::Clock::Milliseconds32(kMoveIntervalMs),
						    TimerTimeoutCallback, this);
		if (err != CHIP_NO_ERROR) {
			LOG_ERR("Timer failed to restart, chip error code: %lx", static_cast<long>(err.AsInteger()));
		}
	}
}
