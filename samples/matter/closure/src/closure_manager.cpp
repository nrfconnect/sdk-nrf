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
using namespace chip::app::Clusters::ClosureControl;
using namespace chip::app::Clusters::Globals;

/* If you plan to dynamically populate the maps change the ExactPos2Enum function to support that */
static Nrf::FiniteMap<TargetPositionEnum, uint16_t, 5> sPositionMapTarget = { {
	{ TargetPositionEnum::kMoveToFullyClosed, 10000 },
	{ TargetPositionEnum::kMoveToFullyOpen, 0 },
	{ TargetPositionEnum::kMoveToPedestrianPosition, 0 },
	{ TargetPositionEnum::kMoveToSignaturePosition, 0 },
	{ TargetPositionEnum::kMoveToVentilationPosition, 7500 },
} };

static Nrf::FiniteMap<CurrentPositionEnum, uint16_t, 5> sPositionMapCurr = { {
	{ CurrentPositionEnum::kFullyClosed, 10000 },
	{ CurrentPositionEnum::kFullyOpened, 0 },
	{ CurrentPositionEnum::kOpenedForPedestrian, 0 },
	{ CurrentPositionEnum::kOpenedAtSignature, 0 },
	{ CurrentPositionEnum::kOpenedForVentilation, 7500 },
} };

static Nrf::FiniteMap<Clusters::Globals::ThreeLevelAutoEnum, uint16_t, 4> sSpeedMap = { {
	{ Clusters::Globals::ThreeLevelAutoEnum::kLow, 1000 },
	{ Clusters::Globals::ThreeLevelAutoEnum::kMedium, 2500 },
	{ Clusters::Globals::ThreeLevelAutoEnum::kHigh, 5000 },
	{ Clusters::Globals::ThreeLevelAutoEnum::kAuto, 2500 },
} };

namespace
{
constexpr uint32_t kCountdownTimeSeconds = 10;

/* Define the Namespace and Tag for the endpoint */
constexpr uint8_t kNamespaceClosure = 0x44;
constexpr uint8_t kTagClosureGarageDoor = 0x05;

/* Define the list of semantic tags for the endpoint */
const Clusters::Descriptor::Structs::SemanticTagStruct::Type kClosureControlEndpointTagList[] = {
	{ .namespaceID = kNamespaceClosure,
	  .tag = kTagClosureGarageDoor,
	  .label = chip::MakeOptional(DataModel::Nullable<chip::CharSpan>("Closure.GarageDoor"_span)) }
};

} // namespace
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

ClosureManager::ClosureManager(IPhysicalDevice &device, chip::EndpointId closureEndpoint)
	: mPhysicalDevice(device), mClosureControlEndpoint(closureEndpoint), mClosureEndpoint(closureEndpoint)
{
	mMainState = MainStateEnum::kStopped;
	mCurrentState.position = chip::MakeOptional(CurrentPositionEnum::kFullyOpened);
	mCurrentState.speed = chip::MakeOptional(ThreeLevelAutoEnum::kAuto);
	mTargetState.speed = chip::MakeOptional(ThreeLevelAutoEnum::kAuto);
}

CHIP_ERROR ClosureManager::Init()
{
	ReturnErrorOnFailure(mPhysicalDevice.Init());
	mPhysicalDevice.SetObserver(this);
	DeviceLayer::StackLock lock;

	/* Closure endpoints initialization */
	mClosureControlEndpoint.GetDelegate().SetManager(this);
	ReturnErrorOnFailure(mClosureControlEndpoint.Init());

	/* Set Taglist for Closure endpoints */
	ReturnErrorOnFailure(SetTagList(
		mClosureEndpoint,
		Span<const Clusters::Descriptor::Structs::SemanticTagStruct::Type>(kClosureControlEndpointTagList)));

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
	if (!position.HasValue()) {
		return chip::Protocols::InteractionModel::Status::Failure;
	}
	auto pos = sPositionMapTarget[position.Value()];
	auto spd = sSpeedMap[speed.ValueOr(mCurrentState.speed.Value())];
	mPhysicalDevice.MoveTo(pos, spd);

	mMainState = MainStateEnum::kMoving;
	mTargetState.position = position;
	mTargetState.speed = speed;

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
	mCurrentState.speed = mTargetState.speed;
	mMainState = MainStateEnum::kStopped;
	mTargetState.position.ClearValue();
	mTargetState.speed.ClearValue();
	mMoveCountdownTime = 0;
	auto err = mClosureControlEndpoint.OnMovementStopped(mMainState, mCurrentState, mTargetState);
	if (err != CHIP_NO_ERROR) {
		LOG_ERR("Unable to update attributes for closureControlEndpoint after stop, error code: %lx",
			static_cast<long>(err.AsInteger()));
	} else {
		LOG_DBG("Attributes updated on movement stopped");
	}
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
