/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "bridge_manager.h"
#include "bridged_device_factory.h"

#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>

#ifdef CONFIG_BRIDGED_DEVICE_BT
#include "ble_bridged_device.h"
#include "ble_connectivity_manager.h"
#endif /* CONFIG_BRIDGED_DEVICE_BT */

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

#ifdef CONFIG_BRIDGED_DEVICE_SIMULATED
static BridgedDeviceDataProvider *CreateSimulatedProvider(int deviceType)
{
	return BridgeFactory::GetSimulatedDataProviderFactory().Create(
		static_cast<MatterBridgedDevice::DeviceType>(deviceType), BridgeManager::HandleUpdate);
}
#endif /* CONFIG_BRIDGED_DEVICE_SIMULATED */

#ifdef CONFIG_BRIDGED_DEVICE_BT

static BridgedDeviceDataProvider *CreateBleProvider(int deviceType)
{
	return BridgeFactory::GetBleDataProviderFactory().Create(
		static_cast<MatterBridgedDevice::DeviceType>(deviceType), BridgeManager::HandleUpdate);
}

struct BluetoothConnectionContext {
	const struct shell *shell;
	int deviceType;
	char nodeLabel[MatterBridgedDevice::kNodeLabelSize];
	BLEBridgedDeviceProvider *provider;
};
#endif /* CONFIG_BRIDGED_DEVICE_BT */

static void AddDevice(const struct shell *shell, int deviceType, const char *nodeLabel,
		      BridgedDeviceDataProvider *provider)
{
	VerifyOrReturn(provider != nullptr, shell_fprintf(shell, SHELL_INFO, "No valid data provider!\n"));

	CHIP_ERROR err;
	if (deviceType == MatterBridgedDevice::DeviceType::EnvironmentalSensor) {
		/* Handle special case for environmental sensor */
		auto *temperatureBridgedDevice = BridgeFactory::GetBridgedDeviceFactory().Create(
			MatterBridgedDevice::DeviceType::TemperatureSensor, nodeLabel);

		VerifyOrReturn(temperatureBridgedDevice != nullptr, delete provider,
			       shell_fprintf(shell, SHELL_INFO, "Cannot allocate Matter device of given type\n"));

		auto *humidityBridgedDevice = BridgeFactory::GetBridgedDeviceFactory().Create(
			MatterBridgedDevice::DeviceType::HumiditySensor, nodeLabel);

		VerifyOrReturn(humidityBridgedDevice != nullptr, delete provider, delete temperatureBridgedDevice,
			       shell_fprintf(shell, SHELL_INFO, "Cannot allocate Matter device of given type\n"));

		MatterBridgedDevice *newBridgedDevices[] = { temperatureBridgedDevice, humidityBridgedDevice };
		err = BridgeManager::Instance().AddBridgedDevices(newBridgedDevices, provider,
								  ARRAY_SIZE(newBridgedDevices));

	} else {
		auto *newBridgedDevice = BridgeFactory::GetBridgedDeviceFactory().Create(
			static_cast<MatterBridgedDevice::DeviceType>(deviceType), nodeLabel);

		MatterBridgedDevice *newBridgedDevices[] = { newBridgedDevice };
		VerifyOrReturn(newBridgedDevice != nullptr, delete provider,
			       shell_fprintf(shell, SHELL_INFO, "Cannot allocate Matter device of given type\n"));
		err = BridgeManager::Instance().AddBridgedDevices(newBridgedDevices, provider,
								  ARRAY_SIZE(newBridgedDevices));
	}

	if (err == CHIP_NO_ERROR) {
		shell_fprintf(shell, SHELL_INFO, "Done\n");
	} else if (err == CHIP_ERROR_INVALID_STRING_LENGTH) {
		shell_fprintf(shell, SHELL_ERROR, "Error: too long node label (max %d)\n",
			      MatterBridgedDevice::kNodeLabelSize);
	} else if (err == CHIP_ERROR_NO_MEMORY) {
		shell_fprintf(shell, SHELL_ERROR, "Error: no memory\n");
	} else if (err == CHIP_ERROR_INVALID_ARGUMENT) {
		shell_fprintf(shell, SHELL_ERROR, "Error: invalid device type\n");
	} else {
		shell_fprintf(shell, SHELL_ERROR, "Error: internal\n");
	}
}

#ifdef CONFIG_BRIDGED_DEVICE_BT
static void BluetoothDeviceConnected(BLEBridgedDevice *device, bt_gatt_dm *discoveredData, bool discoverySucceeded,
				     void *context)
{
	if (context && device && discoveredData && discoverySucceeded) {
		BluetoothConnectionContext *ctx = reinterpret_cast<BluetoothConnectionContext *>(context);
		chip::Platform::UniquePtr<BluetoothConnectionContext> ctxPtr(ctx);

		if (ctxPtr->provider->MatchBleDevice(device)) {
			return;
		}

		if (ctxPtr->provider->ParseDiscoveredData(discoveredData)) {
			return;
		}

		AddDevice(ctx->shell, ctx->deviceType, ctx->nodeLabel, ctx->provider);
	}
}
#endif /* CONFIG_BRIDGED_DEVICE_BT */

static int AddBridgedDeviceHandler(const struct shell *shell, size_t argc, char **argv)
{
	int deviceType = strtoul(argv[1], NULL, 0);
	char *nodeLabel = nullptr;

#if defined(CONFIG_BRIDGED_DEVICE_BT)
	int bleDeviceIndex = strtoul(argv[2], NULL, 0);

	if (argv[3]) {
		nodeLabel = argv[3];
	}

	/* The device object can be created once the Bluetooth LE connection will be established. */
	BluetoothConnectionContext *context = chip::Platform::New<BluetoothConnectionContext>();

	if (!context) {
		return -ENOMEM;
	}

	chip::Platform::UniquePtr<BluetoothConnectionContext> contextPtr(context);
	contextPtr->shell = shell;
	contextPtr->deviceType = deviceType;

	if (nodeLabel) {
		strcpy(contextPtr->nodeLabel, nodeLabel);
	}

	BridgedDeviceDataProvider *provider = CreateBleProvider(contextPtr->deviceType);

	if (!provider) {
		shell_fprintf(shell, SHELL_INFO, "Cannot allocate data provider of given type\n");
		return -ENOMEM;
	}

	contextPtr->provider = static_cast<BLEBridgedDeviceProvider *>(provider);

	CHIP_ERROR err = BLEConnectivityManager::Instance().Connect(
		bleDeviceIndex, BluetoothDeviceConnected, contextPtr.get(), contextPtr->provider->GetServiceUuid());

	if (err == CHIP_NO_ERROR) {
		contextPtr.release();
	}

#elif defined(CONFIG_BRIDGED_DEVICE_SIMULATED)

	if (argv[2]) {
		nodeLabel = argv[2];
	}

	/* The device is simulated, so it can be added immediately. */
	BridgedDeviceDataProvider *provider = CreateSimulatedProvider(deviceType);
	AddDevice(shell, deviceType, nodeLabel, provider);
#else
	return -ENOTSUP;

#endif /* CONFIG_BRIDGED_DEVICE_BT */

	return 0;
}

static int RemoveBridgedDeviceHandler(const struct shell *shell, size_t argc, char **argv)
{
	int endpointId = strtoul(argv[1], NULL, 0);

	if (CHIP_NO_ERROR == BridgeManager::Instance().RemoveBridgedDevice(endpointId)) {
		shell_fprintf(shell, SHELL_INFO, "Done\n");
	} else {
		shell_fprintf(shell, SHELL_ERROR, "Error: device not found\n");
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
