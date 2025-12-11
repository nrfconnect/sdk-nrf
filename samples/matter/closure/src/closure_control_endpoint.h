/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include <app/clusters/closure-control-server/closure-control-cluster-delegate.h>
#include <app/clusters/closure-control-server/closure-control-cluster-logic.h>
#include <app/clusters/closure-control-server/closure-control-cluster-matter-context.h>
#include <app/clusters/closure-control-server/closure-control-cluster-objects.h>
#include <app/clusters/closure-control-server/closure-control-server.h>

#include "pwm/pwm_device.h"
#include <app-common/zap-generated/cluster-objects.h>
#include <app/TestEventTriggerDelegate.h>
#include <lib/core/CHIPError.h>
#include <lib/core/DataModelTypes.h>
#include <protocols/interaction_model/StatusCode.h>

class ClosureManager;

/**
 * @class ClosureControlDelegate
 * @class Handles communication with the matter stack
 *
 * This class is responsible for comminicating with the Matter stack
 * It implements callback, such as HandleStopCommand, HandleMoveToCommand, HandleCalibrateCommand
 * That are called by Matter stack whenever a command is Invoked.
 * It also implements functions such as GetCalibrationCountdownTime, GetMovingCountdownTime,
 * GetWaitingForMotionCountdownTime That are called by Matter stack when it requires to know this information It mostly
 * forwards the callbacks, and receives data from ClosureManager
 * @param mManager Pointer to the ClosureManager
 * @param mLogic Pointer to the ClusterLogic (Matter stack implementation of ClosureControl)
 */
class ClosureControlDelegate : public chip::app::Clusters::ClosureControl::DelegateBase,
			       public chip::TestEventTriggerHandler {
	using MainStateEnum = chip::app::Clusters::ClosureControl::MainStateEnum;
	using ThreeLevelAutoEnum = chip::app::Clusters::Globals::ThreeLevelAutoEnum;
	using GenericOverallCurrentState = chip::app::Clusters::ClosureControl::GenericOverallCurrentState;
	using GenericOverallTargetState = chip::app::Clusters::ClosureControl::GenericOverallTargetState;
	using TargetPositionEnum = chip::app::Clusters::ClosureControl::TargetPositionEnum;
	using ClusterLogic = chip::app::Clusters::ClosureControl::ClusterLogic;
	using MatterContext = chip::app::Clusters::ClosureControl::MatterContext;
	using ElapsedS = chip::ElapsedS;
	using EndpointId = chip::EndpointId;
	using Interface = chip::app::Clusters::ClosureControl::Interface;
	using Status = chip::Protocols::InteractionModel::Status;

public:
	ClosureControlDelegate() {}

	/* DelegateBase overrides */
	Status HandleStopCommand() override;
	Status HandleMoveToCommand(const chip::Optional<TargetPositionEnum> &position,
				   const chip::Optional<bool> &latch,
				   const chip::Optional<ThreeLevelAutoEnum> &speed) override;
	Status HandleCalibrateCommand() override;
	void SetManager(ClosureManager *manager) { mManager = manager; }
	bool IsReadyToMove() override;
	ElapsedS GetCalibrationCountdownTime() override;
	ElapsedS GetMovingCountdownTime() override;
	ElapsedS GetWaitingForMotionCountdownTime() override;

	/* TestEventTriggerHandler overrides */
	CHIP_ERROR HandleEventTrigger(uint64_t eventTrigger) override;

	/* Delegate specific functions */
	void SetLogic(ClusterLogic *logic) { mLogic = logic; }
	ClusterLogic *GetLogic() const { return mLogic; }

private:
	ClosureManager *mManager = nullptr;
	ClusterLogic *mLogic;
};

/**
 * @class ClosureControlEndpoint
 * @brief Represents a Closure Control cluster endpoint.
 *
 * This class encapsulates the logic and interfaces required to manage a Closure Control cluster
 * endpoint. It integrates the delegate, context, logic, and interface components for the
 * endpoint.
 *
 * @param mEndpoint The endpoint ID associated with this Closure Control endpoint.
 * @param mContext The Matter context for the endpoint.
 * @param mDelegate The delegate instance for handling commands.
 * @param mLogic The cluster logic associated with the endpoint.
 * @param mInterface The interface for interacting with the cluster.
 */
class ClosureControlEndpoint {
public:
	using MainStateEnum = chip::app::Clusters::ClosureControl::MainStateEnum;
	using ThreeLevelAutoEnum = chip::app::Clusters::Globals::ThreeLevelAutoEnum;
	using GenericOverallCurrentState = chip::app::Clusters::ClosureControl::GenericOverallCurrentState;
	using GenericOverallTargetState = chip::app::Clusters::ClosureControl::GenericOverallTargetState;
	using TargetPositionEnum = chip::app::Clusters::ClosureControl::TargetPositionEnum;
	using ClusterLogic = chip::app::Clusters::ClosureControl::ClusterLogic;
	using MatterContext = chip::app::Clusters::ClosureControl::MatterContext;
	using ElapsedS = chip::ElapsedS;
	using EndpointId = chip::EndpointId;
	using Interface = chip::app::Clusters::ClosureControl::Interface;
	ClosureControlEndpoint(EndpointId endpoint)
		: mEndpoint(endpoint), mContext(mEndpoint), mDelegate(), mLogic(mDelegate, mContext),
		  mInterface(mEndpoint, mLogic)
	{
		mDelegate.SetLogic(&mLogic);
	}
	/**
	 * @brief Updates all the attributes in the Matter Stack
	 *
	 * @param mainState new MainState
	 * @param currentState newCurrentState
	 * @param targetState newTargetState
	 *
	 * @return CHIP_ERROR indicating the result of the operation
	 */

	CHIP_ERROR WriteAllAttributes(const MainStateEnum &mainState, const GenericOverallCurrentState &currentState,
				      const chip::app::DataModel::Nullable<GenericOverallTargetState> &targetState);
	/**
	 * @brief Initializes the ClosureControlEndpoint instance.
	 *
	 * @return CHIP_ERROR indicating the result of the initialization.
	 */
	CHIP_ERROR Init();

	/**
	 * @brief Retrieves the delegate associated with this Closure Control endpoint.
	 *
	 * @return Reference to the ClosureControlDelegate instance.
	 */
	ClosureControlDelegate &GetDelegate() { return mDelegate; }

	/**
	 * @brief Returns a reference to the ClusterLogic instance associated with this object.
	 *
	 * @return ClusterLogic& Reference to the internal ClusterLogic object.
	 */
	ClusterLogic &GetLogic() { return mLogic; }

	CHIP_ERROR OnMovementStopped(const MainStateEnum &mainState, const GenericOverallCurrentState &currentState,
				     const chip::app::DataModel::Nullable<GenericOverallTargetState> &targetState);

	CHIP_ERROR OnMovementUpdate(const MainStateEnum &mainState, const GenericOverallCurrentState &currentState,
				    const chip::app::DataModel::Nullable<GenericOverallTargetState> &targetState);

private:
	EndpointId mEndpoint = chip::kInvalidEndpointId;
	MatterContext mContext;
	ClosureControlDelegate mDelegate;
	ClusterLogic mLogic;
	Interface mInterface;
};
