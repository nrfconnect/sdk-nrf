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
namespace
{

constexpr ElapsedS kDefaultCountdownTime = 30;

enum class ClosureControlTestEventTrigger : Nrf::Matter::TestEventTrigger::EventTriggerId {
	kMainStateIsError = 0x0104000000000000,
	kMainStateIsProtected = 0x0104000000000001,
	kMainStateIsDisengaged = 0x0104000000000002,
	kMainStateIsSetupRequired = 0x0104000000000003,
	kClearEvent = 0x0104000000000004,
};

} /* namespace */

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
	conformance.FeatureMap().Set(Feature::kPositioning).Set(Feature::kSpeed).Set(Feature::kVentilation);
	conformance.OptionalAttributes().Set<Attributes::CountdownTime::Id>();

	ClusterInitParameters initParams;

	ReturnErrorOnFailure(mInterface.Init(conformance, initParams));
	mDelegate.SetCluster(&mInterface.Cluster());

#ifdef CONFIG_NCS_SAMPLE_MATTER_TEST_EVENT_TRIGGERS
	ReturnErrorOnFailure(Nrf::Matter::TestEventTrigger::Instance().RegisterTestEventTriggerHandler(&mDelegate));
#endif

	return CHIP_NO_ERROR;
}

CHIP_ERROR
ClosureControlEndpoint::WriteAllAttributes(const MainStateEnum &mainState,
					   const GenericOverallCurrentState &currentState,
					   const chip::app::DataModel::Nullable<GenericOverallTargetState> &targetState)
{
	auto &cluster = mInterface.Cluster();
	chip::DeviceLayer::StackLock lock;
	ReturnErrorOnFailure(cluster.SetMainState(mainState));
	ReturnErrorOnFailure(cluster.SetOverallCurrentState(chip::app::DataModel::MakeNullable(currentState)));
	ReturnErrorOnFailure(cluster.SetCountdownTimeFromDelegate(mDelegate.GetMovingCountdownTime()));

	if (!targetState.IsNull()) {
		ReturnErrorOnFailure(cluster.SetOverallTargetState(targetState));
	}

	return CHIP_NO_ERROR;
}

CHIP_ERROR
ClosureControlEndpoint::OnMovementStopped(const MainStateEnum &mainState,
					  const GenericOverallCurrentState &currentState,
					  const chip::app::DataModel::Nullable<GenericOverallTargetState> &targetState)
{
	ReturnErrorOnFailure(WriteAllAttributes(mainState, currentState, targetState));
	ReturnErrorOnFailure(GetLogic().GenerateMovementCompletedEvent());
	return CHIP_NO_ERROR;
}

CHIP_ERROR
ClosureControlEndpoint::OnMovementUpdate(const MainStateEnum &mainState, const GenericOverallCurrentState &currentState,
					 const chip::app::DataModel::Nullable<GenericOverallTargetState> &targetState)
{
	return WriteAllAttributes(mainState, currentState, targetState);
}

CHIP_ERROR ClosureControlDelegate::HandleEventTrigger(uint64_t eventTrigger)
{
	eventTrigger = clearEndpointInEventTrigger(eventTrigger);

	auto trigger = static_cast<ClosureControlTestEventTrigger>(eventTrigger);
	auto *cluster = GetCluster();
	VerifyOrReturnError(cluster != nullptr, CHIP_ERROR_INCORRECT_STATE);

	switch (trigger) {
	case ClosureControlTestEventTrigger::kMainStateIsError:
		ReturnErrorOnFailure(cluster->SetMainState(MainStateEnum::kError));
		ReturnErrorOnFailure(cluster->AddErrorToCurrentErrorList(ClosureErrorEnum::kBlockedBySensor));
		break;
	case ClosureControlTestEventTrigger::kMainStateIsProtected:
		ReturnErrorOnFailure(cluster->SetMainState(MainStateEnum::kProtected));
		break;
	case ClosureControlTestEventTrigger::kMainStateIsDisengaged:
		ReturnErrorOnFailure(cluster->SetMainState(MainStateEnum::kDisengaged));
		break;
	case ClosureControlTestEventTrigger::kMainStateIsSetupRequired:
		ReturnErrorOnFailure(cluster->SetMainState(MainStateEnum::kSetupRequired));
		break;
	case ClosureControlTestEventTrigger::kClearEvent:
		ReturnErrorOnFailure(cluster->SetMainState(MainStateEnum::kStopped));
		cluster->ClearCurrentErrorList();
		break;
	default:
		return CHIP_ERROR_INVALID_ARGUMENT;
	}
	return CHIP_NO_ERROR;
}
