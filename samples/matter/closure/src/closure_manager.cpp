/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "closure_manager.h"
#include "closure_control_endpoint.h"

#include <app-common/zap-generated/cluster-objects.h>
#include <app/util/attribute-storage.h>
#include <dk_buttons_and_leds.h>
#include <platform/CHIPDeviceLayer.h>

#include <app-common/zap-generated/attributes/Accessors.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "physical_device.h"

#include "finite_map.h"

LOG_MODULE_DECLARE(closure_manager, CONFIG_CHIP_APP_LOG_LEVEL);

using namespace chip;
using namespace chip::app;
using namespace chip::DeviceLayer;
using namespace chip::app::Clusters;
using namespace chip::app::Clusters::ClosureControl;
using namespace chip::app::Clusters::Globals;

namespace
{
/* If you plan to dynamically populate the maps change the ExactPos2Enum function to support that */
constexpr Nrf::FiniteMap<TargetPositionEnum, uint16_t, 4> sPositionMapTarget = { {
	{ TargetPositionEnum::kMoveToFullyClosed, 10000 },
	{ TargetPositionEnum::kMoveToFullyOpen, 0 },
	{ TargetPositionEnum::kMoveToSignaturePosition, 4000 },
	{ TargetPositionEnum::kMoveToVentilationPosition, 8000 },
} };

constexpr Nrf::FiniteMap<CurrentPositionEnum, uint16_t, 4> sPositionMapCurr = { {
	{ CurrentPositionEnum::kFullyClosed, 10000 },
	{ CurrentPositionEnum::kFullyOpened, 0 },
	{ CurrentPositionEnum::kOpenedAtSignature, 4000 },
	{ CurrentPositionEnum::kOpenedForVentilation, 8000 },
} };

constexpr Nrf::FiniteMap<ThreeLevelAutoEnum, uint16_t, 4> sSpeedMap = { {
	{ ThreeLevelAutoEnum::kLow, 1000 },
	{ ThreeLevelAutoEnum::kMedium, 2500 },
	{ ThreeLevelAutoEnum::kHigh, 5000 },
	{ ThreeLevelAutoEnum::kAuto, 2500 },
} };

constexpr uint32_t kCountdownTimeSeconds = 10;

CurrentPositionEnum ExactPos2Enum(uint16_t currPositionExact)
{
	/* This implementation assumes that the map is full (which it is) */
	for (auto &item : sPositionMapCurr.mMap) {
		if (currPositionExact == item.value) {
			return item.key;
		}
	}
	return CurrentPositionEnum::kPartiallyOpened;
}
} // namespace

ClosureManager::ClosureManager(IPhysicalDevice &device, chip::EndpointId closureEndpoint)
	: mPhysicalDevice(device), mClosureControlEndpoint(closureEndpoint), mClosureEndpoint(closureEndpoint)
{
	mMainState = MainStateEnum::kStopped;
	mCurrentState.position = chip::MakeOptional(CurrentPositionEnum::kFullyOpened);
	mCurrentState.speed = chip::MakeOptional(ThreeLevelAutoEnum::kAuto);

	/* After the reboot data target should be null. */
	mTargetState.SetNull();
}

CHIP_ERROR ClosureManager::Init()
{
	mPhysicalDevice.SetObserver(this);
	ReturnErrorOnFailure(mPhysicalDevice.Init());
	DeviceLayer::StackLock lock;

	/* Closure endpoints initialization */
	mClosureControlEndpoint.GetDelegate().SetManager(this);
	ReturnErrorOnFailure(mClosureControlEndpoint.Init());

	/* Set Taglist for Closure endpoints */
	ReturnErrorOnFailure(SetTagList(mClosureEndpoint, mPhysicalDevice.GetSemanticTagList()));

	ReturnErrorOnFailure(mClosureControlEndpoint.WriteAllAttributes(mMainState, mCurrentState, mTargetState));

	return CHIP_NO_ERROR;
}

chip::Protocols::InteractionModel::Status ClosureManager::OnCalibrateCommand()
{
	return chip::Protocols::InteractionModel::Status::UnsupportedCommand;
}

chip::Protocols::InteractionModel::Status ClosureManager::OnStopCommand()
{
	mPhysicalDevice.Stop();
	return chip::Protocols::InteractionModel::Status::Success;
}

chip::Protocols::InteractionModel::Status
ClosureManager::OnMoveToCommand(const chip::Optional<TargetPositionEnum> position, const chip::Optional<bool> latch,
				const chip::Optional<ThreeLevelAutoEnum> speed)
{
	/* Currently we do not support latching. */
	if (mClosureControlEndpoint.GetLogic().GetConformance().HasFeature(ClosureControl::Feature::kMotionLatching) &&
	    latch.HasValue()) {
		return chip::Protocols::InteractionModel::Status::UnsupportedCommand;
	}

	/* Determine target position.
	 * If position argument is provided, use it and have target position as a fallback.
	 */
	chip::Optional<TargetPositionEnum> targetPosition = NullOptional;
	if (position.HasValue()) {
		targetPosition.SetValue(position.Value());
	} else if (!mTargetState.IsNull() && mTargetState.Value().position.HasValue() &&
		   !mTargetState.Value().position.Value().IsNull()) {
		targetPosition.SetValue(mTargetState.Value().position.Value().Value());
	}

	/* If we have a target position, initiate movement. */
	if (targetPosition.HasValue()) {
		auto pos = sPositionMapTarget.TryAt(targetPosition.Value());
		if (!pos.has_value()) {
			LOG_ERR("Target position not found in map");
			return chip::Protocols::InteractionModel::Status::ConstraintError;
		}

		auto spd = sSpeedMap.TryAt(speed.ValueOr(mCurrentState.speed.Value()));
		if (!spd.has_value()) {
			LOG_ERR("Speed value not found in map");
			return chip::Protocols::InteractionModel::Status::ConstraintError;
		}

		mPhysicalDevice.MoveTo(pos.value(), spd.value());

		mMainState = MainStateEnum::kMoving;
		mTargetState.SetNonNull(GenericOverallTargetState(
			targetPosition, NullOptional, MakeOptional(speed.ValueOr(mCurrentState.speed.Value()))));
		mCurrentState.secureState.SetNonNull(false);
	}

	mCurrentState.speed.SetValue(speed.ValueOr(mCurrentState.speed.ValueOr(ThreeLevelAutoEnum::kAuto)));

	auto err = mClosureControlEndpoint.WriteAllAttributes(mMainState, mCurrentState, mTargetState);
	if (err != CHIP_NO_ERROR) {
		LOG_ERR("Unable to update attributes on move, error code: %lx", static_cast<long>(err.AsInteger()));
		return chip::Protocols::InteractionModel::Status::Failure;
	}
	return chip::Protocols::InteractionModel::Status::Success;
}

void ClosureManager::OnMovementStopped(uint16_t currentPosition)
{
	mCurrentState.position = MakeOptional(ExactPos2Enum(currentPosition));
	mMainState = MainStateEnum::kStopped;
	mMoveCountdownTime = 0;

	auto currPosition = sPositionMapCurr.TryAt(CurrentPositionEnum::kFullyClosed);
	if (currPosition.has_value() && currentPosition == currPosition.value()) {
		mCurrentState.secureState.SetNonNull(true);
	}

	if (!mTargetState.IsNull() && mTargetState.Value().position.HasValue()) {
		auto targetPosOpt = sPositionMapTarget.TryAt(mTargetState.Value().position.Value().Value());
		if (targetPosOpt.has_value() && targetPosOpt.value() == currentPosition) {
			mTargetState.SetNull();
		}
	}

	auto err = mClosureControlEndpoint.OnMovementStopped(mMainState, mCurrentState, mTargetState);
	if (err != CHIP_NO_ERROR) {
		LOG_ERR("Unable to update attributes for closureControlEndpoint after stop, error code: %lx",
			static_cast<long>(err.AsInteger()));
	} else {
		LOG_DBG("Attributes updated on movement stopped");
	}
}
void ClosureManager::OnSetup(uint16_t currentPosition)
{
	mCurrentState.position = MakeOptional(ExactPos2Enum(currentPosition));
}

void ClosureManager::OnMovementUpdate(uint16_t currentPosition, uint16_t timeLeft, bool justStarted)
{
	mCurrentState.position = MakeOptional(ExactPos2Enum(currentPosition));
	mMoveCountdownTime = timeLeft;
	auto err = mClosureControlEndpoint.OnMovementUpdate(mMainState, mCurrentState, mTargetState);
	if (err != CHIP_NO_ERROR) {
		LOG_ERR("Unable to update attributes for closureControlEndpoint while moving, error code: %lx",
			static_cast<long>(err.AsInteger()));
	}
}

/* Implementation of this callback is required by matter 1.5*/
void MatterClosureControlClusterServerAttributeChangedCallback(const chip::app::ConcreteAttributePath &attributePath)
{
	LOG_DBG("Closure Control cluster ID: " ChipLogFormatMEI, ChipLogValueMEI(attributePath.mAttributeId));
}
