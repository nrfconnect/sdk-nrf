/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>

#include "bridge_manager.h"

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

static int add_bridged_device(const struct shell *shell, size_t argc, char **argv)
{
	int deviceType = strtoul(argv[1], NULL, 0);
	char *nodeLabel = nullptr;

	if (argv[2]) {
		nodeLabel = argv[2];
	}

	CHIP_ERROR err =
		GetBridgeManager().AddBridgedDevice(static_cast<BridgedDevice::DeviceType>(deviceType), nodeLabel);
	if (err == CHIP_NO_ERROR) {
		shell_fprintf(shell, SHELL_INFO, "Done\n");
	} else if (err == CHIP_ERROR_INVALID_STRING_LENGTH) {
		shell_fprintf(shell, SHELL_ERROR, "Error: too long node label (max %d)\n",
			      BridgedDevice::kNodeLabelSize);
	} else if (err == CHIP_ERROR_NO_MEMORY) {
		shell_fprintf(shell, SHELL_ERROR, "Error: no memory\n");
	} else if (err == CHIP_ERROR_INVALID_ARGUMENT) {
		shell_fprintf(shell, SHELL_ERROR, "Error: invalid device type\n");
	} else {
		shell_fprintf(shell, SHELL_ERROR, "Error: internal\n");
	}
	return 0;
}

static int remove_bridged_device(const struct shell *shell, size_t argc, char **argv)
{
	int endpointId = strtoul(argv[1], NULL, 0);

	if (CHIP_NO_ERROR == GetBridgeManager().RemoveBridgedDevice(endpointId)) {
		shell_fprintf(shell, SHELL_INFO, "Done\n");
	} else {
		shell_fprintf(shell, SHELL_ERROR, "Error: device not found\n");
	}
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_matter_bridge,
	SHELL_CMD_ARG(
		add, NULL,
		"Adds bridged device. \n"
		"Usage: add <bridged_device_type> [node_label]\n"
		"* bridged_device_type - the bridged device's type, e.g. 256 - OnOff Light, 770 - TemperatureSensor, 775 - HumiditySensor\n"
		"* node_label - the optional bridged device's node label",
		add_bridged_device, 2, 1),
	SHELL_CMD_ARG(
		remove, NULL,
		"Removes bridged device. \n"
		"Usage: remove <bridged_device_endpoint_id>\n"
		"* bridged_device_endpoint_id - the bridged device's endpoint on which it was previously created\n",
		remove_bridged_device, 2, 0),
	SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(matter_bridge, &sub_matter_bridge, "Matter bridge commands", NULL);
