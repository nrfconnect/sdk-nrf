/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdlib.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/printk.h>
#include <zephyr/shell/shell.h>

#include "fuel_gauge.h"

#define SLEEP_TIME_MS 1000

static const struct device *vbat = DEVICE_DT_GET(DT_NODELABEL(npm2100ek_vbat));
static enum battery_type battery_model;
static bool fuel_gauge_initialized;

static const char *const battery_model_str[] = {[BATTERY_TYPE_ALKALINE_AA] = "Alkaline AA",
						[BATTERY_TYPE_ALKALINE_AAA] = "Alkaline AAA",
						[BATTERY_TYPE_ALKALINE_2SAA] = "Alkaline 2SAA",
						[BATTERY_TYPE_ALKALINE_2SAAA] = "Alkaline 2SAAA",
						[BATTERY_TYPE_ALKALINE_LR44] = "Alkaline LR44",
						[BATTERY_TYPE_LITHIUM_CR2032] = "Lithium CR2032"};

int main(void)
{
	if (IS_ENABLED(CONFIG_BATTERY_MODEL_ALKALINE_AA)) {
		battery_model = BATTERY_TYPE_ALKALINE_AA;
	} else if (IS_ENABLED(CONFIG_BATTERY_MODEL_ALKALINE_AAA)) {
		battery_model = BATTERY_TYPE_ALKALINE_AAA;
	} else if (IS_ENABLED(CONFIG_BATTERY_MODEL_ALKALINE_2SAA)) {
		battery_model = BATTERY_TYPE_ALKALINE_2SAA;
	} else if (IS_ENABLED(CONFIG_BATTERY_MODEL_ALKALINE_2SAAA)) {
		battery_model = BATTERY_TYPE_ALKALINE_2SAAA;
	} else if (IS_ENABLED(CONFIG_BATTERY_MODEL_ALKALINE_LR44)) {
		battery_model = BATTERY_TYPE_ALKALINE_LR44;
	} else if (IS_ENABLED(CONFIG_BATTERY_MODEL_LITHIUM_CR2032)) {
		battery_model = BATTERY_TYPE_LITHIUM_CR2032;
	} else {
		printk("Configuration error: no battery model selected.\n");
		return 0;
	}

	fuel_gauge_initialized = false;

	if (!device_is_ready(vbat)) {
		printk("vbat device not ready.\n");
		return 0;
	}

	printk("PMIC device ok\n");

	while (1) {
		if (!fuel_gauge_initialized) {
			int err;

			err = fuel_gauge_init(vbat, battery_model);
			if (err < 0) {
				printk("Could not initialise fuel gauge.\n");
				return 0;
			}
			printk("Fuel gauge initialised for %s battery.\n",
			       battery_model_str[battery_model]);

			fuel_gauge_initialized = true;
		}
		fuel_gauge_update(vbat);
		k_msleep(SLEEP_TIME_MS);
	}
}

#if CONFIG_SHELL
static int cmd_battery_model_set(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);

	if (strcmp(argv[0], "Alkaline_AA") == 0) {
		battery_model = BATTERY_TYPE_ALKALINE_AA;
	} else if (strcmp(argv[0], "Alkaline_AAA") == 0) {
		battery_model = BATTERY_TYPE_ALKALINE_AAA;
	} else if (strcmp(argv[0], "Alkaline_2SAA") == 0) {
		battery_model = BATTERY_TYPE_ALKALINE_2SAA;
	} else if (strcmp(argv[0], "Alkaline_2SAAA") == 0) {
		battery_model = BATTERY_TYPE_ALKALINE_2SAAA;
	} else if (strcmp(argv[0], "Alkaline_LR44") == 0) {
		battery_model = BATTERY_TYPE_ALKALINE_LR44;
	} else if (strcmp(argv[0], "Lithium_CR2032") == 0) {
		battery_model = BATTERY_TYPE_LITHIUM_CR2032;
	} else {
		shell_error(sh, "Invalid battery model");
		return 0;
	}

	fuel_gauge_initialized = false;

	return 0;
}

static int cmd_battery_model_get(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(sh, "Battery model: %s", battery_model_str[battery_model]);

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	subcmd_battery_model,
	SHELL_CMD_ARG(Alkaline_AA, NULL, "Alkaline AA battery", cmd_battery_model_set, 1, 0),
	SHELL_CMD_ARG(Alkaline_AAA, NULL, "Alkaline AAA battery", cmd_battery_model_set, 1, 0),
	SHELL_CMD_ARG(Alkaline_2SAA, NULL, "Alkaline 2SAA battery", cmd_battery_model_set, 1, 0),
	SHELL_CMD_ARG(Alkaline_2SAAA, NULL, "Alkaline 2SAAA battery", cmd_battery_model_set, 1, 0),
	SHELL_CMD_ARG(Alkaline_LR44, NULL, "Alkaline LR44 battery", cmd_battery_model_set, 1, 0),
	SHELL_CMD_ARG(Lithium_CR2032, NULL, "Lithium CR2032 battery", cmd_battery_model_set, 1, 0),
	SHELL_SUBCMD_SET_END);

SHELL_CMD_ARG_REGISTER(battery_model, &subcmd_battery_model, "Select battery model",
		       cmd_battery_model_get, 1, 0);
#endif /* CONFIG_SHELL */
