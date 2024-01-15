/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "bridge_manager.h"

#ifdef CONFIG_BRIDGED_DEVICE_BT
#include "ble_bridged_device_factory.h"
#include "ble_connectivity_manager.h"
#else
#include "simulated_bridged_device_factory.h"
#endif /* CONFIG_BRIDGED_DEVICE_BT */

#include <zephyr/shell/shell.h>

#if defined(CONFIG_BRIDGED_DEVICE_BT) && defined(CONFIG_BT_SMP)
static void BluetoothConnectionSecurityRequest(void *context)
{
	if (!context) {
		return;
	}

	const struct shell *shell = reinterpret_cast<struct shell *>(context);

	shell_fprintf(shell, SHELL_INFO, "---------------------------------------------------------------------\n");
	shell_fprintf(shell, SHELL_INFO, "| Bridged Bluetooth LE device authentication                        |\n");
	shell_fprintf(shell, SHELL_INFO, "|                                                                   |\n");
	shell_fprintf(shell, SHELL_INFO, "| Insert pin code displayed by the Bluetooth LE peripheral device   |\n");
	shell_fprintf(shell, SHELL_INFO, "| to authenticate the pairing operation.                            |\n");
	shell_fprintf(shell, SHELL_INFO, "|                                                                   |\n");
	shell_fprintf(shell, SHELL_INFO, "| To do that, use matter_bridge pincode <pincode> shell command.    |\n");
	shell_fprintf(shell, SHELL_INFO, "---------------------------------------------------------------------\n");
}
#endif /* CONFIG_BRIDGED_DEVICE_BT && CONFIG_BT_SMP */

static int AddBridgedDeviceHandler(const struct shell *shell, size_t argc, char **argv)
{
	char *nodeLabel = nullptr;
	CHIP_ERROR result = CHIP_NO_ERROR;

#if defined(CONFIG_BRIDGED_DEVICE_BT)
	int bleDeviceIndex = strtoul(argv[1], NULL, 0);

	if (argv[2]) {
		nodeLabel = argv[2];
	}

	bt_addr_le_t address;
	if (Nrf::BLEConnectivityManager::Instance().GetScannedDeviceAddress(&address, bleDeviceIndex) !=
	    CHIP_NO_ERROR) {
		shell_fprintf(shell, SHELL_ERROR, "Invalid Bluetooth LE device index.\n");
	} else {
		shell_fprintf(shell, SHELL_ERROR, "Found device address\n");
	}

	uint16_t uuid;
	if (Nrf::BLEConnectivityManager::Instance().GetScannedDeviceUuid(uuid, bleDeviceIndex) != CHIP_NO_ERROR) {
		shell_fprintf(shell, SHELL_ERROR, "Invalid Bluetooth LE device index.\n");
	}

#ifdef CONFIG_BT_SMP
	Nrf::BLEConnectivityManager::ConnectionSecurityRequest request;
	request.mCallback = BluetoothConnectionSecurityRequest;
	request.mContext = const_cast<struct shell *>(shell);
	result = BleBridgedDeviceFactory::CreateDevice(uuid, address, nodeLabel, &request);
#else
	result = BleBridgedDeviceFactory::CreateDevice(uuid, address, nodeLabel);
#endif /* CONFIG_BT_SMP */

#elif defined(CONFIG_BRIDGED_DEVICE_SIMULATED)
	int deviceType = strtoul(argv[1], NULL, 0);

	if (argv[2]) {
		nodeLabel = argv[2];
	}

	result = SimulatedBridgedDeviceFactory::CreateDevice(deviceType, nodeLabel);

#else
	return -ENOTSUP;

#endif /* CONFIG_BRIDGED_DEVICE_BT */

	if (result == CHIP_NO_ERROR) {
		shell_fprintf(shell, SHELL_INFO, "Done\n");
	} else if (result == CHIP_ERROR_INCORRECT_STATE) {
		shell_fprintf(shell, SHELL_INFO, "Device is already added\n");
	} else {
		shell_fprintf(shell, SHELL_ERROR, "Device add failed\n");
	}

	return 0;
}

static int RemoveBridgedDeviceHandler(const struct shell *shell, size_t argc, char **argv)
{
	int endpointId = strtoul(argv[1], NULL, 0);

#if defined(CONFIG_BRIDGED_DEVICE_BT)
	if (CHIP_NO_ERROR == BleBridgedDeviceFactory::RemoveDevice(endpointId)) {
		shell_fprintf(shell, SHELL_INFO, "Done\n");
	}
#else
	if (CHIP_NO_ERROR == SimulatedBridgedDeviceFactory::RemoveDevice(endpointId)) {
		shell_fprintf(shell, SHELL_INFO, "Done\n");
	}
#endif
	else {
		shell_fprintf(shell, SHELL_ERROR, "Error: device remove failed\n");
	}
	return 0;
}

#ifdef CONFIG_BRIDGED_DEVICE_SIMULATED_ONOFF_SHELL
static int SimulatedBridgedDeviceOnOffWriteHandler(const struct shell *shell, size_t argc, char **argv)
{
	using DeviceType = Nrf::MatterBridgedDevice::DeviceType;
	const char *command = argv[0];
	uint8_t value = strtoul(argv[1], nullptr, 0);
	chip::EndpointId endpointId = strtoul(argv[2], nullptr, 0);

	uint16_t deviceType{};
	auto *provider = Nrf::BridgeManager().Instance().GetProvider(endpointId, deviceType);

	if (provider) {
		if ((0 == strcmp(command, "onoff") && deviceType != DeviceType::OnOffLight)) {
			shell_fprintf(shell, SHELL_ERROR, "Error: Unsupported device\n");
			return 0;
		}
		CHIP_ERROR err = provider->UpdateState(chip::app::Clusters::OnOff::Id,
						       chip::app::Clusters::OnOff::Attributes::OnOff::Id, &value);
		if (err != CHIP_NO_ERROR) {
			shell_fprintf(shell, SHELL_ERROR, "Cannot update OnOff device state\n");
			return 0;
		}
		shell_fprintf(shell, SHELL_INFO, "Done\n");

	} else {
		shell_fprintf(shell, SHELL_ERROR, "Error: Device inaccessible\n");
	}

	return 0;
}
#endif

#if defined(CONFIG_BRIDGED_DEVICE_SIMULATED) && defined(CONFIG_BRIDGE_ONOFF_LIGHT_SWITCH_BRIDGED_DEVICE)
static int SimulatedBridgedDeviceOnOffLightSwitchWriteHandler(const struct shell *shell, size_t argc, char **argv)
{
	using DeviceType = Nrf::MatterBridgedDevice::DeviceType;
	const char *command = argv[0];
	uint8_t value = strtoul(argv[1], nullptr, 0);
	chip::EndpointId endpointId = strtoul(argv[2], nullptr, 0);

	DeviceType deviceType{};
	auto *provider = BridgeManager().Instance().GetProvider(endpointId, deviceType);

	if (provider) {
		if ((0 == strcmp(command, "onoff_switch") && deviceType != DeviceType::OnOffLightSwitch)) {
			shell_fprintf(shell, SHELL_ERROR, "Error: Unsupported device\n");
			return 0;
		}
		CHIP_ERROR err = provider->UpdateState(chip::app::Clusters::OnOff::Id,
						       chip::app::Clusters::OnOff::Attributes::OnOff::Id, &value);
		if (err != CHIP_NO_ERROR) {
			shell_fprintf(shell, SHELL_ERROR, "Cannot update OnOff Light Switch device state\n");
			return 0;
		}
		shell_fprintf(shell, SHELL_INFO, "Done\n");

	} else {
		shell_fprintf(shell, SHELL_ERROR, "Error: Device inaccessible\n");
	}

	return 0;
}
#endif

#ifdef CONFIG_BRIDGED_DEVICE_BT
static void BluetoothScanResult(Nrf::BLEConnectivityManager::ScannedDevice *devices, uint8_t count, void *context)
{
	if (!devices || !context) {
		return;
	}

	const struct shell *shell = reinterpret_cast<struct shell *>(context);

	shell_fprintf(shell, SHELL_INFO, "Scan result:\n");
	shell_fprintf(shell, SHELL_INFO, "---------------------------------------------------------------------\n");
	shell_fprintf(shell, SHELL_INFO, "| Index |      Address      |                   UUID                 \n");
	shell_fprintf(shell, SHELL_INFO, "---------------------------------------------------------------------\n");
	for (int i = 0; i < count; i++) {
		shell_fprintf(shell, SHELL_INFO, "| %d     | %02x:%02x:%02x:%02x:%02x:%02x | 0x%04x (%s)\n", i,
			      devices[i].mAddr.a.val[5], devices[i].mAddr.a.val[4], devices[i].mAddr.a.val[3],
			      devices[i].mAddr.a.val[2], devices[i].mAddr.a.val[1], devices[i].mAddr.a.val[0],
			      devices[i].mUuid, BleBridgedDeviceFactory::GetUuidString(devices[i].mUuid));
	}
}

#ifdef CONFIG_BT_SMP
static int InsertBridgedDevicePincodeHandler(const struct shell *shell, size_t argc, char **argv)
{
	int bleDeviceIndex = strtoul(argv[0], NULL, 0);
	unsigned int pincode = strtoul(argv[1], NULL, 0);

	bt_addr_le_t address;
	if (Nrf::BLEConnectivityManager::Instance().GetScannedDeviceAddress(&address, bleDeviceIndex) !=
	    CHIP_NO_ERROR) {
		shell_fprintf(shell, SHELL_ERROR, "Invalid Bluetooth LE device index.\n");
	} else {
		shell_fprintf(shell, SHELL_ERROR, "Found device address\n");
	}

	Nrf::BLEConnectivityManager::Instance().SetPincode(address, pincode);
	return 0;
}
#endif /* CONFIG_BT_SMP */

static int ScanBridgedDeviceHandler(const struct shell *shell, size_t argc, char **argv)
{
	shell_fprintf(shell, SHELL_INFO, "Scanning for 10 s ...\n");

	Nrf::BLEConnectivityManager::Instance().Scan(BluetoothScanResult, const_cast<struct shell *>(shell));

	return 0;
}
#endif /* CONFIG_BRIDGED_DEVICE_BT */

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_matter_bridge,
#ifdef CONFIG_BRIDGED_DEVICE_BT
	SHELL_CMD_ARG(add, NULL,
		      "Adds bridged device. \n"
		      "Usage: add <ble_device_index> [node_label]\n"
		      "* ble_device_index - the Bluetooth LE device's index on the list returned by the scan command\n"
		      "* node_label - the optional bridged device's node label\n",
		      AddBridgedDeviceHandler, 2, 1),
#else
	SHELL_CMD_ARG(
		add, NULL,
		"Adds bridged device. \n"
		"Usage: add <bridged_device_type> [node_label]\n"
		"* bridged_device_type - the bridged device's type, e.g. 15 - Generic Switch, 256 - OnOff Light, 259 - OnOff Light Switch, 770 - TemperatureSensor, 775 - HumiditySensor, \n"
		"* node_label - the optional bridged device's node label\n",
		AddBridgedDeviceHandler, 2, 1),
#endif /* CONFIG_BRIDGED_DEVICE_BT */
	SHELL_CMD_ARG(
		remove, NULL,
		"Removes bridged device. \n"
		"Usage: remove <bridged_device_endpoint_id>\n"
		"* bridged_device_endpoint_id - the bridged device's endpoint on which it was previously created\n",
		RemoveBridgedDeviceHandler, 2, 0),
#ifdef CONFIG_BRIDGED_DEVICE_SIMULATED_ONOFF_SHELL
	SHELL_CMD_ARG(
		onoff, NULL,
		"Changes the current state of the simulated onoff device. \n"
		"Usage: onoff <new_state> <bridged_device_endpoint_id>\n"
		"* new_state - new state of the simulated onoff device\n"
		"* bridged_device_endpoint_id - the bridged device's endpoint on which it was previously created\n",
		SimulatedBridgedDeviceOnOffWriteHandler, 3, 0),
#endif
#if defined(CONFIG_BRIDGED_DEVICE_SIMULATED) && defined(CONFIG_BRIDGE_ONOFF_LIGHT_SWITCH_BRIDGED_DEVICE)
	SHELL_CMD_ARG(
		onoff_switch, NULL,
		"Changes the current state of the simulated onoff light switch device. \n"
		"Usage: onoff_switch <new_state> <bridged_device_endpoint_id>\n"
		"* new_state - new state of the simulated onoff light switch device\n"
		"* bridged_device_endpoint_id - the bridged device's endpoint on which it was previously created\n",
		SimulatedBridgedDeviceOnOffLightSwitchWriteHandler, 3, 0),
#endif
#ifdef CONFIG_BRIDGED_DEVICE_BT
	SHELL_CMD_ARG(scan, NULL,
		      "Scan for Bluetooth LE devices to bridge. \n"
		      "Usage: scan\n",
		      ScanBridgedDeviceHandler, 1, 0),
#ifdef CONFIG_BT_SMP
	SHELL_CMD_ARG(pincode, NULL,
		      "Insert pincode for Bluetooth LE device pairing. \n"
		      "Usage: pincode <pincode>\n"
		      "* pincode - is a pin required for Bluetooth LE pairing authentication\n",
		      InsertBridgedDevicePincodeHandler, 2, 0),
#endif /* CONFIG_BT_SMP */
#endif /* CONFIG_BRIDGED_DEVICE_BT */
	SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(matter_bridge, &sub_matter_bridge, "Matter bridge commands", NULL);
