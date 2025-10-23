/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once
#include <lib/core/CHIPError.h>

#include "physical_device_observer.h"

/**
 * @interface IPhysicalDevice
 * @brief Interface representing the closure
 *
 * It is used by the ClosureManager to interface with the physical device
 */
class IPhysicalDevice {
public:
	virtual ~IPhysicalDevice() = default;

	IPhysicalDevice() = default;
	IPhysicalDevice(const IPhysicalDevice &) = delete;
	IPhysicalDevice &operator=(const IPhysicalDevice &) = delete;

	void SetObserver(IPhysicalDeviceObserver *observer) { mObserver = observer; }
	virtual CHIP_ERROR Init() = 0;
	/**
	 * @brief Used to initialize movement
	 * @param position target position in 0.01% (0-10000)
	 * @param speed desired speed of movement in 0.01%/second
	 */
	virtual CHIP_ERROR MoveTo(uint16_t position, uint16_t speed) = 0;
	/**
	 * @brief Used to stop movement
	 */
	virtual CHIP_ERROR Stop() = 0;

protected:
	IPhysicalDeviceObserver *mObserver = nullptr;
};
