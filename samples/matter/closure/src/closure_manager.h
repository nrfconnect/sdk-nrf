/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 * @class ClosureManager
 * @brief Manages the initialization and operations related to closure and
 *        closure panel endpoints in the application.
 *
 * @note This class is part of the closure application example
 */

#pragma once

#include "board/board.h"
#include "closure_control_endpoint.h"
#include "physical_device.h"
#include "physical_device_observer.h"
#include <lib/core/DataModelTypes.h>
#include <zephyr/kernel.h>

/**
 * @class ClosureManager
 * @brief Main class responsible for handleing application logic
 * It receives callbacks from ClosureControlDelegate and uses ClosureControlEndpoint
 * to update attribues on the matter stack.
 * Using interface IPhysicalDevice it sends appropriate commands to the physical device
 * (in this sample implemented as a PWM led)
 */
class ClosureManager : public IPhysicalDeviceObserver {
	using TargetPositionEnum = chip::app::Clusters::ClosureControl::TargetPositionEnum;
	using MainStateEnum = chip::app::Clusters::ClosureControl::MainStateEnum;
	using ThreeLevelAutoEnum = chip::app::Clusters::Globals::ThreeLevelAutoEnum;
	using GenericOverallCurrentState = chip::app::Clusters::ClosureControl::GenericOverallCurrentState;
	using GenericOverallTargetState = chip::app::Clusters::ClosureControl::GenericOverallTargetState;
	using ElapsedS = chip::ElapsedS;

public:
	ClosureManager(IPhysicalDevice &device, chip::EndpointId closureEndpoint);

	CHIP_ERROR Init();

	/**
	 * @brief Handles the Calibrate command for the Closure.
	 *
	 *
	 * @return chip::Protocols::InteractionModel::Status Status of the command handling operation.
	 */
	chip::Protocols::InteractionModel::Status OnCalibrateCommand();

	/**
	 * @brief Handles the MoveTo command for the Closure.
	 *
	 * This method processes the MoveTo command, which is used to initiate a motion action
	 * for a closure.
	 *
	 * @param position Optional target position for the closure device.
	 * @param latch Optional flag indicating whether the closure should latch after moving.
	 * @param speed Optional speed setting for the movement, represented as a ThreeLevelAutoEnum.
	 * @return chip::Protocols::InteractionModel::Status Status of the command handling operation.
	 */
	chip::Protocols::InteractionModel::Status OnMoveToCommand(const chip::Optional<TargetPositionEnum> position,
								  const chip::Optional<bool> latch,
								  const chip::Optional<ThreeLevelAutoEnum> speed);

	/**
	 * @brief Handles the Stop command for the Closure.
	 *
	 * This method processes the Stop command, which is used to stop an action for a closure.
	 *
	 * @return chip::Protocols::InteractionModel::Status
	 *         Returns Status::Success if the Stop command is handled successfully,
	 *         or an appropriate error status otherwise.
	 */
	chip::Protocols::InteractionModel::Status OnStopCommand();

	void OnMovementStopped(uint16_t currentPosition) override;
	void OnMovementUpdate(uint16_t currentPosition, uint16_t timeLeft, bool justStarted = false) override;

	ElapsedS GetMoveCountdownTime() { return mMoveCountdownTime; }

private:
	ElapsedS mMoveCountdownTime = 0;
	IPhysicalDevice &mPhysicalDevice;
	MainStateEnum mMainState;
	GenericOverallCurrentState mCurrentState;
	GenericOverallTargetState mTargetState;

	ClosureControlEndpoint mClosureControlEndpoint;
	const chip::EndpointId mClosureEndpoint;
};
