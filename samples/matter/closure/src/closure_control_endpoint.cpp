/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "closure_control_endpoint.h"
#include "closure_manager.h"
#include <app-common/zap-generated/attributes/Accessors.h>
#include <app-common/zap-generated/cluster-enums.h>
#include <app-common/zap-generated/cluster-objects.h>
#include <platform/CHIPDeviceLayer.h>
#include <protocols/interaction_model/StatusCode.h>
#include <zephyr/logging/log.h>

#ifdef CONFIG_NCS_SAMPLE_MATTER_TEST_EVENT_TRIGGERS
#include <event_triggers/event_triggers.h>
#endif

LOG_MODULE_DECLARE(closure_cntrl, CONFIG_CHIP_APP_LOG_LEVEL);

using namespace chip;
using namespace chip::app::Clusters::ClosureControl;

using Status = chip::Protocols::InteractionModel::Status;
using TargetPositionEnum = chip::app::Clusters::ClosureControl::TargetPositionEnum;
using ThreeLevelAutoEnum = chip::app::Clusters::Globals::ThreeLevelAutoEnum;
using OptionalAttributeEnum = chip::app::Clusters::ClosureControl::OptionalAttributeEnum;
namespace
{

constexpr ElapsedS kDefaultCountdownTime = 30;

enum class ClosureControlTestEventTrigger : uint64_t {
	kMainStateIsError = 0x0104000000000000, // MainState is Error(3) Test Event | Simulate that the device is in
						// error state, add at least one element to the CurrentErrorList
						// attribute
	kMainStateIsProtected = 0x0104000000000001, // MainState is Protected(5) Test Event | Simulate that the device
						    // is in protected state
	kMainStateIsDisengaged = 0x0104000000000002, // MainState is Disengaged(6) Test Event | Simulate that the device
						     // is in disengaged state
	kMainStateIsSetupRequired = 0x0104000000000003, // MainState is SetupRequired(7) Test Event | Simulate that the
							// device is in SetupRequired state
	kClearEvent = 0x0104000000000004, // MainState Test clear Event | Returns the device to pre-test status for that
					  // test event.
};

} // namespace

Status ClosureControlDelegate::HandleCalibrateCommand()
{
	if (mManager == nullptr) {
		LOG_ERR("No manager bound to ClosureControlDelegate");
		return Status::Failure;
	}
	return mManager->OnCalibrateCommand();
}

Status ClosureControlDelegate::HandleMoveToCommand(const chip::Optional<TargetPositionEnum> &position,
						   const chip::Optional<bool> &latch,
						   const chip::Optional<ThreeLevelAutoEnum> &speed)
{
	if (mManager == nullptr) {
		LOG_ERR("No manager bound to ClosureControlDelegate");
		return Status::Failure;
	}
	return mManager->OnMoveToCommand(position, latch, speed);
}

Status ClosureControlDelegate::HandleStopCommand()
{
	if (mManager == nullptr) {
		LOG_ERR("No manager bound to ClosureControlDelegate");
		return Status::Failure;
	}
	return mManager->OnStopCommand();
}

bool ClosureControlDelegate::IsReadyToMove()
{
	/* This function should return true if the closure is ready to move.
	   For now, we will return true. */
	return true;
}

ElapsedS ClosureControlDelegate::GetCalibrationCountdownTime()
{
	/* This function should return the calibration countdown time.
	   For now, we will return kDefaultCountdownTime. */
	return kDefaultCountdownTime;
}

ElapsedS ClosureControlDelegate::GetMovingCountdownTime()
{
	return mManager->GetMoveCountdownTime();
}

ElapsedS ClosureControlDelegate::GetWaitingForMotionCountdownTime()
{
	/* This function should return the waiting for motion countdown time.
	   For now, we will return kDefaultCountdownTime. */
	return kDefaultCountdownTime;
}

CHIP_ERROR ClosureControlEndpoint::Init()
{
	ClusterConformance conformance;
	conformance.FeatureMap()
		.Set(Feature::kPositioning)
		.Set(Feature::kSpeed)
		.Set(Feature::kVentilation)
		.Set(Feature::kPedestrian);
	conformance.OptionalAttributes().Set(OptionalAttributeEnum::kCountdownTime);

	ClusterInitParameters initParams;

	ReturnErrorOnFailure(mLogic.Init(conformance, initParams));
	ReturnErrorOnFailure(mInterface.Init());

#ifdef CONFIG_NCS_SAMPLE_MATTER_TEST_EVENT_TRIGGERS
	ReturnErrorOnFailure(Nrf::Matter::TestEventTrigger::Instance().RegisterTestEventTriggerHandler(&mDelegate));
#endif

	return CHIP_NO_ERROR;
}

CHIP_ERROR ClosureControlEndpoint::WriteAllAttributes(const MainStateEnum &mainState,
						      const GenericOverallCurrentState &currentState,
						      const GenericOverallTargetState &targetState)
{
	chip::DeviceLayer::StackLock lock;
	ReturnErrorOnFailure(mLogic.SetMainState(mainState));
	ReturnErrorOnFailure(mLogic.SetOverallCurrentState(currentState));
	ReturnErrorOnFailure(mLogic.SetOverallTargetState(targetState));
	ReturnErrorOnFailure(mLogic.SetCountdownTimeFromDelegate(mDelegate.GetMovingCountdownTime()));
	return CHIP_NO_ERROR;
}

CHIP_ERROR ClosureControlEndpoint::OnMovementStopped(const MainStateEnum &mainState,
						     const GenericOverallCurrentState &currentState,
						     const GenericOverallTargetState &targetState)
{
	ReturnErrorOnFailure(WriteAllAttributes(mainState, currentState, targetState));
	ReturnErrorOnFailure(GetLogic().GenerateMovementCompletedEvent());
	return CHIP_NO_ERROR;
}

CHIP_ERROR ClosureControlEndpoint::OnMovementUpdate(const MainStateEnum &mainState,
						    const GenericOverallCurrentState &currentState,
						    const GenericOverallTargetState &targetState)
{
	return WriteAllAttributes(mainState, currentState, targetState);
}

CHIP_ERROR ClosureControlDelegate::HandleEventTrigger(uint64_t eventTrigger)
{
	eventTrigger = clearEndpointInEventTrigger(eventTrigger);
	ClosureControlTestEventTrigger trigger = static_cast<ClosureControlTestEventTrigger>(eventTrigger);
	ClusterLogic *logic = GetLogic();

	switch (trigger) {
	case ClosureControlTestEventTrigger::kMainStateIsError:
		ReturnErrorOnFailure(logic->SetMainState(MainStateEnum::kError));
		ReturnErrorOnFailure(logic->AddErrorToCurrentErrorList(ClosureErrorEnum::kBlockedBySensor));
		break;
	case ClosureControlTestEventTrigger::kMainStateIsProtected:
		ReturnErrorOnFailure(logic->SetMainState(MainStateEnum::kProtected));
		break;
	case ClosureControlTestEventTrigger::kMainStateIsDisengaged:
		ReturnErrorOnFailure(logic->SetMainState(MainStateEnum::kDisengaged));
		break;
	case ClosureControlTestEventTrigger::kMainStateIsSetupRequired:
		ReturnErrorOnFailure(logic->SetMainState(MainStateEnum::kSetupRequired));
		break;
	case ClosureControlTestEventTrigger::kClearEvent:
		ReturnErrorOnFailure(logic->SetMainState(MainStateEnum::kStopped));
		logic->ClearCurrentErrorList();
		break;
	default:
		return CHIP_ERROR_INVALID_ARGUMENT;
	}
	return CHIP_NO_ERROR;
}
