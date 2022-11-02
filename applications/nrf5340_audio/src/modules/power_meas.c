/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(power_meas, CONFIG_MODULE_POWER_LOG_LEVEL);

#define POWER_RAIL_NUM 4U

struct power_rail_config {
	const char *name;
	const struct device *sensor;
};

static const struct power_rail_config rail_config[POWER_RAIL_NUM] = {
	{"VBAT", DEVICE_DT_GET(DT_NODELABEL(vbat_sensor))},
	{"VDD1_CODEC", DEVICE_DT_GET(DT_NODELABEL(vdd1_codec_sensor))},
	{"VDD2_CODEC", DEVICE_DT_GET(DT_NODELABEL(vdd2_codec_sensor))},
	{"VDD2_NRF", DEVICE_DT_GET(DT_NODELABEL(vdd2_nrf_sensor))},
};
static bool rail_enabled[POWER_RAIL_NUM];

K_THREAD_STACK_DEFINE(meas_stack, CONFIG_POWER_MEAS_STACK_SIZE);
static struct k_thread meas_thread;
K_SEM_DEFINE(meas_sem, 0, 1);
static bool meas_enabled;

/**
 * @brief Read a power rail data (voltage/current/power) and log results.
 *
 * @param[in] config Power rail configuration.
 *
 * @retval 0 on success
 * @retval -errno on sensor fetch/get error.
 */
static int read_and_log(const struct power_rail_config *config)
{
	int ret;
	struct sensor_value voltage, current, power;

	ret = sensor_sample_fetch(config->sensor);
	if (ret < 0) {
		return ret;
	}

	ret = sensor_channel_get(config->sensor, SENSOR_CHAN_VOLTAGE, &voltage);
	if (ret < 0) {
		return ret;
	}

	ret = sensor_channel_get(config->sensor, SENSOR_CHAN_CURRENT, &current);
	if (ret < 0) {
		return ret;
	}

	ret = sensor_channel_get(config->sensor, SENSOR_CHAN_POWER, &power);
	if (ret < 0) {
		return ret;
	}

	LOG_INF("%-10s:\t%.3fV, %06.3fmA, %06.3fmW", config->name,
		sensor_value_to_double(&voltage),
		sensor_value_to_double(&current) * 1000.0f,
		sensor_value_to_double(&power) * 1000.0f);

	return 0;
}

/** @brief Measurement thread */
static void meas_thread_fn(void *dummy1, void *dummy2, void *dummy3)
{
	ARG_UNUSED(dummy1);
	ARG_UNUSED(dummy2);
	ARG_UNUSED(dummy3);

	while (1) {
		if (!meas_enabled) {
			k_sem_take(&meas_sem, K_FOREVER);
		}

		for (size_t i = 0U; i < POWER_RAIL_NUM; i++) {
			int ret;

			if (!rail_enabled[i]) {
				continue;
			}

			ret = read_and_log(&rail_config[i]);
			if (ret < 0) {
				LOG_ERR("Could not read %s", rail_config->name);
			}
		}

		k_msleep(CONFIG_POWER_MEAS_INTERVAL_MS);
	}
}

static int power_meas_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	/* check if all sensors are ready */
	for (size_t i = 0U; i < POWER_RAIL_NUM; i++) {
		if (!device_is_ready(rail_config[i].sensor)) {
			LOG_ERR("INA231 device not ready: %s\n",
				rail_config->name);
			return -ENODEV;
		}
	}

	/* enable all sensors and measurement if configured */
	if (IS_ENABLED(CONFIG_POWER_MEAS_START_ON_BOOT)) {
		meas_enabled = true;

		for (size_t i = 0U; i < POWER_RAIL_NUM; i++) {
			rail_enabled[i] = true;
		}
	}

	/* start measurement thread */
	(void)k_thread_create(&meas_thread, meas_stack,
			      CONFIG_POWER_MEAS_STACK_SIZE,
			      meas_thread_fn, NULL, NULL, NULL,
			      K_PRIO_PREEMPT(CONFIG_POWER_MEAS_THREAD_PRIO),
			      0, K_NO_WAIT);

	return 0;
}

SYS_INIT(power_meas_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);


static int cmd_info(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	for (size_t i = 0U; i < POWER_RAIL_NUM; i++) {
		shell_print(shell, "%-10s: %s", rail_config[i].name,
			    rail_enabled[i] ? "enabled" : "disabled");
	}

	return 0;
}

static int cmd_config(const struct shell *shell, size_t argc, const char **argv)
{
	bool enable;

	if (argc != 3) {
		shell_error(shell,
			    "Usage: power config RAIL|all enable|disable");
		return -EINVAL;
	}

	enable = strcmp(argv[2], "enable") == 0;

	if (strcmp(argv[1], "all") == 0) {
		for (size_t i = 0U; i < POWER_RAIL_NUM; i++) {
			rail_enabled[i] = enable;
		}

		shell_print(shell, "All rails %s",
			    enable ? "enabled" : "disabled");
	} else {
		size_t i;

		for (i = 0U; i < POWER_RAIL_NUM; i++) {
			if (strcmp(argv[1], rail_config[i].name) == 0) {
				break;
			}
		}

		if (i == POWER_RAIL_NUM) {
			shell_error(shell, "Invalid power rail");
			return -EINVAL;
		}

		rail_enabled[i] = enable;

		shell_print(shell, "Rail %s %s", rail_config[i].name,
			    enable ? "enabled" : "disabled");
	}

	return 0;
}

static int cmd_meas_start(const struct shell *shell, size_t argc,
			  const char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (meas_enabled) {
		shell_error(shell, "Measurement already started");
		return -EALREADY;
	}

	meas_enabled = true;
	k_sem_give(&meas_sem);

	shell_print(shell, "Measurement started");

	return 0;
}

static int cmd_meas_stop(const struct shell *shell, size_t argc,
			 const char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	meas_enabled = false;

	shell_print(shell, "Measurement stopped");

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	power_meas_cmd,
	SHELL_COND_CMD(CONFIG_SHELL, info, NULL, "Show power rails info",
		       cmd_info),
	SHELL_COND_CMD(CONFIG_SHELL, config, NULL, "Configure power rails",
		       cmd_config),
	SHELL_COND_CMD(CONFIG_SHELL, start, NULL, "Start measurements",
		       cmd_meas_start),
	SHELL_COND_CMD(CONFIG_SHELL, stop, NULL, "Stop measurements",
		       cmd_meas_stop),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(power, &power_meas_cmd, "Configure power measurements",
		   NULL);
