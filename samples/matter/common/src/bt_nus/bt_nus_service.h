/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include <platform/Zephyr/BLEAdvertisingArbiter.h>

#include <bluetooth/services/nus.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>

namespace Nrf {

class NUSService {
public:
	using CommandCallback = void (*)(void *context);
	struct Command {
		char command[CONFIG_CHIP_NUS_MAX_COMMAND_LEN];
		CommandCallback callback;
		void *context;
	};

	/**
	 * @brief Initialize Nordic UART Service (NUS)
	 *
	 * Initialize internal structures and set Bluetooth LE advertisement parameters.
	 */
	bool Init(uint8_t priority, uint16_t minInterval, uint16_t maxInterval);

	/**
	 * @brief Start the Nordic UART Service server
	 *
	 * Register Nordic UART Service that supports controlling lock and unlock operations
	 * using Bluetooth LE commands. The Nordic UART Service may begin immediately.
	 */
	bool StartServer();

	/**
	 * @brief Stop the Nordic UART Service server
	 *
	 * Unregister Nordic UART Service  that supports controlling lock and unlock operations
	 * using Bluetooth LE commands. The Nordic UART Service will be disabled and the next service with the highest priority will begin immediately.
	 */
	void StopServer();

	/**
	 * @brief Send data to the connected device
	 * 
	 * Send data to the connected and paired device.
	 * 
	 * @return false if the device cannot send data, true otherwise.
	 */
	bool SendData(const char *const data, size_t length);

	/**
	 * @brief Register a new command for NUS service
	 * 
	 * The new command consist of a callback that will be called when the device receives the provided command name.
	 * The name of the command must be null-terminated.
	 * 
	 * @return false if there is no space for the next command (See CONFIG_CHIP_NUS_MAX_COMMANDS kConfig), or the name is empty.
	 */
	bool RegisterCommand(const char *const name, size_t length, CommandCallback callback, void *context);

	friend NUSService &GetLockNUSService();
	static NUSService sInstance;

	/**
	 * Static callbacks for the Bluetooth LE connection
	*/
	static void Connected(struct bt_conn *conn, uint8_t err);
	static void Disconnected(struct bt_conn *conn, uint8_t reason);
	static void SecurityChanged(struct bt_conn *conn, bt_security_t level, enum bt_security_err err);

private:

	void DispatchCommand(const char *const data, uint16_t len);

	static void RxCallback(struct bt_conn *conn, const uint8_t *const data, uint16_t len);
	static void AuthPasskeyDisplay(struct bt_conn *conn, unsigned int passkey);
	static void AuthCancel(struct bt_conn *conn);
	static void PairingComplete(struct bt_conn *conn, bool bonded);
	static void PairingFailed(struct bt_conn *conn, enum bt_security_err reason);
	static char* LogAddress(struct bt_conn *conn);

	static struct bt_conn_auth_cb sConnAuthCallbacks;
	static struct bt_conn_auth_info_cb sConnAuthInfoCallbacks;
	static struct bt_nus_cb sNusCallbacks;

	bool mIsStarted = false;
	chip::DeviceLayer::BLEAdvertisingArbiter::Request mAdvertisingRequest = {};
	std::array<bt_data, 2> mAdvertisingItems;
	std::array<bt_data, 1> mServiceItems;
	std::array<Command, CONFIG_CHIP_NUS_MAX_COMMANDS> mCommandsList;
	uint8_t mCommandsCounter = 0;
	struct bt_conn *mBTConnection;


};

inline NUSService &GetNUSService()
{
	return NUSService::sInstance;
}

} /* namespace Nrf */
