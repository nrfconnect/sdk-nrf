/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include <platform/Zephyr/BLEAdvertisingArbiter.h>

#include <array>

namespace Nrf {

/**
 * @brief DFU over SMP helper class
 *
 * The purpose of this class is to enable Device Firmware Upgrade mechanism
 * using Simple Management Protocol (SMP) over Bluetooth LE. Besides
 * facilitating initialization of the SMP server, it is capable of requesting
 * BLE advertising in a way that is compatible with other application components
 * that use BLE, such as Matter BLE layer.
 */
class DFUOverSMP {
public:
	/**
	 * @brief Initialize DFU over SMP utility
	 *
	 * Initialize internal structures and register necessary commands in the SMP
	 * server.
	 */
	void Init();

	/**
	 * @brief Confirm the current firmware image
	 *
	 * In case the current image is run tentatively after performing the
	 * firmware update, approve it to prevent the system from restoring the
	 * previous image on the next boot.
	 */
	void ConfirmNewImage();

	/**
	 * @brief Start BLE SMP server
	 *
	 * Register SMP BLE service that supports image management commands and
	 * request BLE advertising. The BLE advertising may begin immediately, or be
	 * deferred if another component with higher priority uses BLE.
	 */
	void StartServer();

private:
	bool mIsStarted = false;
	chip::DeviceLayer::BLEAdvertisingArbiter::Request mAdvertisingRequest = {};
	std::array<bt_data, 2> mAdvertisingItems;

	friend DFUOverSMP &GetDFUOverSMP();
	static DFUOverSMP sDFUOverSMP;
};

inline DFUOverSMP &GetDFUOverSMP()
{
	return DFUOverSMP::sDFUOverSMP;
}

} /* namespace Nrf */
