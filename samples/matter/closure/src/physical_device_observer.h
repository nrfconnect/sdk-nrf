/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once
#include <cstdint>

/**
 * @interface IPhysicalDeviceObserver
 * @brief Defines callbacks the physical device uses to inform the application about real closure state.
 */
class IPhysicalDeviceObserver {
public:
	/**
	 * @brief This callback is called when the closure stops moving
	 *
	 * It should handle this information and forward it to the Matter stack
	 *
	 * @param currenPosition position of the closure in 0.01% (0-10000)
	 */
	virtual void OnMovementStopped(uint16_t currentPosition) = 0;
	/**
	 * @brief This callback is every time the physical device has new knowledge about the position of the closure.
	 *
	 * It should handle this information and store it internally or forward it to the Matter stack if necessary.
	 *
	 * @param currenPosition position of the closure in 0.01% (0-10000)
	 * @param timeLeft how much time it should take for the closure to finish movement
	 * @param justStarted true if this is the first callback after the movement has started
	 */
	virtual void OnMovementUpdate(uint16_t currentPosition, uint16_t timeLeft, bool justStarted = false) = 0;
	virtual ~IPhysicalDeviceObserver() = default;
};
