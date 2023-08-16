/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "bridge_manager.h"
#include "bridged_devices_creator.h"

#ifdef CONFIG_BRIDGED_DEVICE_BT
#include "ble_connectivity_manager.h"
#endif /* CONFIG_BRIDGED_DEVICE_BT */

#include <zephyr/shell/shell.h>

static int AddBridgedDeviceHandler(const struct shell *shell, size_t argc, char **argv)
{
	int deviceType = strtoul(argv[1], NULL, 0);
	char *nodeLabel = nullptr;
	CHIP_ERROR result = CHIP_NO_ERROR;

#if defined(CONFIG_BRIDGED_DEVICE_BT)
	int bleDeviceIndex = strtoul(argv[2], NULL, 0);

	if (argv[3]) {
		nodeLabel = argv[3];
	}

	bt_addr_le_t address;
	if (BLEConnectivityManager::Instance().GetScannedDeviceAddress(&address, bleDeviceIndex) != CHIP_NO_ERROR) {
		shell_fprintf(shell, SHELL_ERROR, "Invalid Bluetooth LE device index.\n");
	} else {
		shell_fprintf(shell, SHELL_ERROR, "Found device address\n");
	}

	result = BridgedDeviceCreator::CreateDevice(deviceType, nodeLabel, address);

#elif defined(CONFIG_BRIDGED_DEVICE_SIMULATED)

	if (argv[2]) {
		nodeLabel = argv[2];
	}

	result = BridgedDeviceCreator::CreateDevice(deviceType, nodeLabel);

#else
	return -ENOTSUP;

#endif /* CONFIG_BRIDGED_DEVICE_BT */

	if (result == CHIP_NO_ERROR) {
		shell_fprintf(shell, SHELL_INFO, "Done\n");
	} else {
		shell_fprintf(shell, SHELL_ERROR, "Device add failed\n");
	}

	return 0;
}

static int RemoveBridgedDeviceHandler(const struct shell *shell, size_t argc, char **argv)
{
	int endpointId = strtoul(argv[1], NULL, 0);

	if (CHIP_NO_ERROR == BridgedDeviceCreator::RemoveDevice(endpointId)) {
		shell_fprintf(shell, SHELL_INFO, "Done\n");
	} else {
		shell_fprintf(shell, SHELL_ERROR, "Error: device remove failed\n");
	}
	return 0;
}

#ifdef CONFIG_BRIDGED_DEVICE_BT
static void BluetoothScanResult(BLEConnectivityManager::ScannedDevice *devices, uint8_t count, void *context)
{
	if (!devices || !context) {
		return;
	}

	const struct shell *shell = reinterpret_cast<struct shell *>(context);

	shell_fprintf(shell, SHELL_INFO, "Scan result:\n");
	shell_fprintf(shell, SHELL_INFO, "-----------------------------\n");
	shell_fprintf(shell, SHELL_INFO, "| Index |      Address      |\n");
	shell_fprintf(shell, SHELL_INFO, "-----------------------------\n");
	for (int i = 0; i < count; i++) {
		shell_fprintf(shell, SHELL_INFO, "| %d     | %02x:%02x:%02x:%02x:%02x:%02x |\n", i,
			      devices[i].mAddr.a.val[5], devices[i].mAddr.a.val[4], devices[i].mAddr.a.val[3],
			      devices[i].mAddr.a.val[2], devices[i].mAddr.a.val[1], devices[i].mAddr.a.val[0]);
	}
}

static int ScanBridgedDeviceHandler(const struct shell *shell, size_t argc, char **argv)
{
	shell_fprintf(shell, SHELL_INFO, "Scanning for 10 s ...\n");

	BLEConnectivityManager::Instance().Scan(BluetoothScanResult, const_cast<struct shell *>(shell));

	return 0;
}
#endif /* CONFIG_BRIDGED_DEVICE_BT */

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_matter_bridge,
#ifdef CONFIG_BRIDGED_DEVICE_BT
	SHELL_CMD_ARG(
		add, NULL,
		"Adds bridged device. \n"
		"Usage: add <bridged_device_type> <ble_device_index> [node_label]\n"
		"* bridged_device_type - the bridged device's type, e.g. 256 - OnOff Light, 291 - EnvironmentalSensor\n"
		"* ble_device_index - the Bluetooth LE device's index on the list returned by the scan command\n"
		"* node_label - the optional bridged device's node label\n",
		AddBridgedDeviceHandler, 3, 1),
#else
	SHELL_CMD_ARG(
		add, NULL,
		"Adds bridged device. \n"
		"Usage: add <bridged_device_type> [node_label]\n"
		"* bridged_device_type - the bridged device's type, e.g. 256 - OnOff Light, 770 - TemperatureSensor, 775 - HumiditySensor\n"
		"* node_label - the optional bridged device's node label\n",
		AddBridgedDeviceHandler, 2, 1),
#endif /* CONFIG_BRIDGED_DEVICE_BT */
	SHELL_CMD_ARG(
		remove, NULL,
		"Removes bridged device. \n"
		"Usage: remove <bridged_device_endpoint_id>\n"
		"* bridged_device_endpoint_id - the bridged device's endpoint on which it was previously created\n",
		RemoveBridgedDeviceHandler, 2, 0),
#ifdef CONFIG_BRIDGED_DEVICE_BT
	SHELL_CMD_ARG(scan, NULL,
		      "Scan for Bluetooth LE devices to bridge. \n"
		      "Usage: scan\n",
		      ScanBridgedDeviceHandler, 1, 0),
#endif /* CONFIG_BRIDGED_DEVICE_BT */
	SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(matter_bridge, &sub_matter_bridge, "Matter bridge commands", NULL);
