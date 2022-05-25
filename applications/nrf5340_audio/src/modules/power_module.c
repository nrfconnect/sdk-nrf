/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "power_module.h"

#include <zephyr/kernel.h>
#include <errno.h>
#include <zephyr/drivers/gpio.h>
#include <string.h>
#include <zephyr/shell/shell.h>
#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include <soc.h>
#include <zephyr/debug/stack.h>

#include "ina231.h"
#include "macros_common.h"
#include "board.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(pwr_module, CONFIG_LOG_POWER_MODULE_LEVEL);

#define BASE_10 10

/* Add addresses to all INA231 here */
#define INA231_VBAT_ADDR (0x44)
#define INA231_VDD1_CODEC_ADDR (0x45)
#define INA231_VDD2_CODEC_ADDR (0x41)
#define INA231_VDD2_NRF_ADDR (0x40)

#define INA231_SHUNT_VOLTAGE_LSB (0.0000025f)
#define INA231_BUS_VOLTAGE_LSB (0.00125f)

#define INA231_VBAT_MAX_EXPECTED_CURRENT (0.12f)
#define INA231_VDD1_CODEC_MAX_EXPECTED_CURRENT (0.02f)
#define INA231_VDD2_CODEC_MAX_EXPECTED_CURRENT (0.02f)
#define INA231_VDD2_NRF_MAX_EXPECTED_CURRENT (0.02f)

#define INA231_VBAT_SHUNT_RESISTOR (0.51f)
#define INA231_VDD1_CODEC_SHUNT_RESISTOR (2.2f)
#define INA231_VDD2_CODEC_SHUNT_RESISTOR (2.2f)
#define INA231_VDD2_NRF_SHUNT_RESISTOR (1.0f)

#define INA231_VBAT_CURRENT_LSB (INA231_VBAT_MAX_EXPECTED_CURRENT / 32768.0f)
#define INA231_VDD1_CODEC_CURRENT_LSB (INA231_VDD1_CODEC_MAX_EXPECTED_CURRENT / 32768.0f)
#define INA231_VDD2_CODEC_CURRENT_LSB (INA231_VDD2_CODEC_MAX_EXPECTED_CURRENT / 32768.0f)
#define INA231_VDD2_NRF_CURRENT_LSB (INA231_VDD2_NRF_MAX_EXPECTED_CURRENT / 32768.0f)

#define INA231_VBAT_POWER_LSB (25.0f * INA231_VBAT_CURRENT_LSB)
#define INA231_VDD1_CODEC_POWER_LSB (25.0f * INA231_VDD1_CODEC_CURRENT_LSB)
#define INA231_VDD2_CODEC_POWER_LSB (25.0f * INA231_VDD2_CODEC_CURRENT_LSB)
#define INA231_VDD2_NRF_POWER_LSB (25.0f * INA231_VDD2_NRF_CURRENT_LSB)

#define INA231_VBAT_CALIBRATION (0.00512f / (INA231_VBAT_CURRENT_LSB * INA231_VBAT_SHUNT_RESISTOR))
#define INA231_VDD1_CODEC_CALIBRATION                                                              \
	(0.00512f / (INA231_VDD1_CODEC_CURRENT_LSB * INA231_VDD1_CODEC_SHUNT_RESISTOR))
#define INA231_VDD2_CODEC_CALIBRATION                                                              \
	(0.00512f / (INA231_VDD2_CODEC_CURRENT_LSB * INA231_VDD2_CODEC_SHUNT_RESISTOR))
#define INA231_VDD2_NRF_CALIBRATION                                                                \
	(0.00512f / (INA231_VDD2_NRF_CURRENT_LSB * INA231_VDD2_NRF_SHUNT_RESISTOR))

#define INA231_COUNT ((uint8_t)ARRAY_SIZE(ina231))

static const struct gpio_dt_spec curr_mon_alert =
	GPIO_DT_SPEC_GET(DT_NODELABEL(curr_mon_alert_in), gpios);

static struct gpio_callback gpio_cb;

/* Default callback for measurement prints */
static void ina231_measurements_print(uint8_t rail);

static uint8_t ina231_active_mask;
static uint8_t ina231_data_ready_mask;

static uint8_t configured_conv_time = INA231_CONFIG_VBUS_CONV_TIME_4156_US;
static uint8_t configured_avg = INA231_CONFIG_AVG_1024;

/* Calculate the total time for measurements and add 10% due to thread being lowest priority */
static uint8_t measurement_timeout_seconds;

struct ina231_conf {
	uint8_t address;
	struct ina231_config_reg config;
	struct power_module_data meas_data;
	nrf_power_module_handler_t callback;
	float power_lsb;
	float current_lsb;
	char name[RAIL_NAME_MAX_SIZE];
};

/* Add entry for each INA231 here */
static struct ina231_conf ina231[] = {
	{ .address = INA231_VBAT_ADDR,
	  .callback = ina231_measurements_print,
	  .power_lsb = INA231_VBAT_POWER_LSB,
	  .current_lsb = INA231_VBAT_CURRENT_LSB,
	  .name = "      VBAT" },
	{ .address = INA231_VDD1_CODEC_ADDR,
	  .callback = ina231_measurements_print,
	  .power_lsb = INA231_VDD1_CODEC_POWER_LSB,
	  .current_lsb = INA231_VDD1_CODEC_CURRENT_LSB,
	  .name = "VDD1_CODEC" },
	{ .address = INA231_VDD2_CODEC_ADDR,
	  .callback = ina231_measurements_print,
	  .power_lsb = INA231_VDD2_CODEC_POWER_LSB,
	  .current_lsb = INA231_VDD2_CODEC_CURRENT_LSB,
	  .name = "VDD2_CODEC" },
	{ .address = INA231_VDD2_NRF_ADDR,
	  .callback = ina231_measurements_print,
	  .power_lsb = INA231_VDD2_NRF_POWER_LSB,
	  .current_lsb = INA231_VDD2_NRF_CURRENT_LSB,
	  .name = "  VDD2_NRF" },
};

static struct ina231_twi_config ina231_twi_cfg = {
	.addr = 0,
};

static struct k_thread power_module_data;
K_THREAD_STACK_DEFINE(power_module_stack, CONFIG_POWER_MODULE_STACK_SIZE);

K_SEM_DEFINE(sem_pwr_module, 0, 1);

/**@brief  Print measurements for given rail
 */
static void ina231_measurements_print(uint8_t rail)
{
	struct power_module_data data;

	power_module_data_get(rail, &data);

	int v_rest = ((int)(data.bus_voltage * 1000) % 1000);
	int c_rest = ((int)(data.current * 1000000) % 1000);
	int p_rest = ((int)(data.power * 1000000) % 1000);

	LOG_INF("%s:\t%d.%03dV, %02d.%03dmA, %02d.%03dmW", ina231[rail].name, (int)data.bus_voltage,
		v_rest, (int)(data.current * 1000), c_rest, (int)(data.power * 1000), p_rest);
}

/**@brief   Get latest measurements from active INA231s
 *
 * @return  0 if successful, ret if not
 */
static int ina231_register_values_get(void)
{
	int ret;

	/* Check all INAs that are started */
	for (uint8_t i = 0; i < INA231_COUNT; i++) {
		if (ina231_active_mask & (1 << i)) {
			ina231_twi_cfg.addr = ina231[i].address;

			ret = ina231_open(&ina231_twi_cfg);
			if (ret) {
				return ret;
			}

			uint16_t mask_enable;

			ret = ina231_mask_enable_get(&mask_enable);
			if (ret) {
				return ret;
			}

			/* Populate statics with measurement data */
			if (mask_enable & INA231_MASK_ENABLE_CVRF_Msk) {
				/* Data ready */
				uint16_t shunt_voltage_reg;
				uint16_t bus_voltage_reg;
				uint16_t current_reg;
				uint16_t power_reg;

				ret = ina231_shunt_voltage_reg_get(&shunt_voltage_reg);
				if (ret) {
					return ret;
				}

				ret = ina231_bus_voltage_reg_get(&bus_voltage_reg);
				if (ret) {
					return ret;
				}

				ret = ina231_current_reg_get(&current_reg);
				if (ret) {
					return ret;
				}

				ret = ina231_power_reg_get(&power_reg);
				if (ret) {
					return ret;
				}

				ina231[i].meas_data.shunt_voltage =
					shunt_voltage_reg * INA231_SHUNT_VOLTAGE_LSB;
				ina231[i].meas_data.current = current_reg * ina231[i].current_lsb;
				ina231[i].meas_data.power = power_reg * ina231[i].power_lsb;
				ina231[i].meas_data.bus_voltage =
					bus_voltage_reg * INA231_BUS_VOLTAGE_LSB;

				ina231_data_ready_mask |= (1 << i);
			}

			ret = ina231_close(&ina231_twi_cfg);
			if (ret) {
				return ret;
			}
		}
	}

	return 0;
}

/**@brief   Pin alert handler for INA231_ALERT
 */
static void alert_pin_handler(const struct device *gpio_port, struct gpio_callback *cb,
			      uint32_t pins)
{
	if (pins == BIT(curr_mon_alert.pin)) {
		k_sem_give(&sem_pwr_module);
	}
}

/**@brief   Init function to set up pin interrupt from INA231
 *
 * @return  0 if successful, ret if not
 */
static int gpio_interrupt_init(void)
{
	int ret;

	if (!device_is_ready(curr_mon_alert.port)) {
		LOG_ERR("Error: curr_mon_alert is not ready");
		return -ENXIO;
	}

	gpio_flags_t gpio_flags = (GPIO_INPUT | GPIO_PULL_UP);

	ret = gpio_pin_configure_dt(&curr_mon_alert, gpio_flags);
	if (ret) {
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(&curr_mon_alert, GPIO_INT_EDGE_TO_INACTIVE);
	if (ret) {
		return ret;
	}

	gpio_init_callback(&gpio_cb, alert_pin_handler, BIT(curr_mon_alert.pin));
	ret = gpio_add_callback(curr_mon_alert.port, &gpio_cb);
	if (ret) {
		return ret;
	}

	return 0;
}

/**@brief   Restart active ina231s with latest configuration, called after change in config
 */
static int power_module_update_measurement_config(void)
{
	int ret;
	uint8_t prev_active_mask = ina231_active_mask;

	/* Reset active mask in case some ina231 doesn't
	 * start up after config change
	 */
	ina231_active_mask = 0;

	for (uint8_t i = 0; i < INA231_COUNT; i++) {
		if (prev_active_mask & (1 << i)) {
			ret = power_module_measurement_start(i, NULL);
			if (ret) {
				return ret;
			}
		}
	}
	return 0;
}

/**@brief   Thread to get data from INA231s
 */
static void power_module_thread(void *dummy1, void *dummy2, void *dummy3)
{
	int ret;

#if (CONFIG_POWER_MODULE_MEAS_START_ON_BOOT)
	for (uint8_t i = 0; i < INA231_COUNT; i++) {
		ret = power_module_measurement_start(i, NULL);
		ERR_CHK(ret);
	}
#endif /* (CONFIG_POWER_MODULE_MEAS_START_ON_BOOT) */

	while (1) {
		if (ina231_active_mask) {
			/* Since the INA231s are sharing one interrupt line we
			 * could miss an interrupt. A timeout will prevent this
			 * from happening
			 */
			ret = k_sem_take(&sem_pwr_module, K_SECONDS(measurement_timeout_seconds));
			/* Only print warning if there are active ina231*/
			if (ret == -EAGAIN) {
				/* They can have been stopped while waiting for semaphore */
				if (ina231_active_mask) {
					LOG_WRN("Timed out, probably missed falling edge");
				}
				ret = 0;
			}
		} else {
			ret = k_sem_take(&sem_pwr_module, K_FOREVER);
		}

		ERR_CHK(ret);

		ret = ina231_register_values_get();
		ERR_CHK_MSG(ret, "Failed");

		/* Only send data once all active ina231s are ready */
		if (ina231_active_mask == ina231_data_ready_mask) {
			for (uint8_t i = 0; i < INA231_COUNT; i++) {
				if (ina231_active_mask & 1 << i) {
					ina231[i].callback(i);
				}
			}
		}
		STACK_USAGE_PRINT("power module", &power_module_data);
	}
}

void power_module_data_get(enum ina_name name, struct power_module_data *data)
{
	memcpy(data, &ina231[name].meas_data, sizeof(struct power_module_data));
	ina231_data_ready_mask &= ~(1U << name);

	/* Every config mode (Enum) below INA231_CONFIG_MODE_SHUNT_CONT
	 * will only measure once
	 */
	if (ina231[name].config.mode < INA231_CONFIG_MODE_SHUNT_CONT) {
		ina231_active_mask &= ~(1U << name);
		LOG_DBG("Setting %d to inactive", name);
	}
}

uint8_t power_module_avg_get(void)
{
	return (uint8_t)configured_avg;
}

int power_module_avg_set(uint8_t avg)
{
	int ret;

	if (avg >= INA231_CONFIG_AVG_NO_OF_VALUES) {
		return -ENOTSUP;
	}
	uint32_t measurement_time_ms = ((ina231_config_avg_to_num[avg] *
					 ina231_vbus_conv_time_to_us[configured_conv_time]) /
						1000 +
					(ina231_config_avg_to_num[avg] *
					 ina231_vsh_conv_time_to_us[configured_conv_time]) /
						1000);

	if (measurement_time_ms < CONFIG_POWER_MODULE_MIN_MEAS_TIME_MS) {
		return -ERANGE;
	}

	configured_avg = avg;
	ret = power_module_update_measurement_config();
	if (ret) {
		return ret;
	}

	return 0;
}

uint8_t power_module_conv_time_get(void)
{
	return (uint8_t)configured_conv_time;
}

int power_module_conv_time_set(uint8_t conv_time)
{
	int ret;

	if (conv_time >= INA231_CONFIG_VBUS_NO_OF_VALUES) {
		return -ENOTSUP;
	}
	uint32_t measurement_time_ms = ((ina231_config_avg_to_num[configured_avg] *
					 ina231_vbus_conv_time_to_us[conv_time]) /
						1000 +
					(ina231_config_avg_to_num[configured_avg] *
					 ina231_vsh_conv_time_to_us[conv_time]) /
						1000);

	if (measurement_time_ms < CONFIG_POWER_MODULE_MIN_MEAS_TIME_MS) {
		return -ERANGE;
	}

	configured_conv_time = conv_time;
	ret = power_module_update_measurement_config();
	if (ret) {
		return ret;
	}

	return 0;
}

int power_module_measurement_start(enum ina_name name, nrf_power_module_handler_t data_handler)
{
	int ret;

	LOG_DBG("Starting measurement on: %d", name);

	/* Update array over active INAs */
	if (data_handler != NULL) {
		ina231[name].callback = data_handler;
	}

	ina231_twi_cfg.addr = ina231[name].address;

	ret = ina231_open(&ina231_twi_cfg);
	if (ret) {
		return ret;
	}

	uint16_t mask_enable;

	ret = ina231_mask_enable_get(&mask_enable);
	if (ret) {
		return ret;
	}

	mask_enable |= (INA231_MASK_ENABLE_CNVR_Enable << INA231_MASK_ENABLE_CNVR_Pos);

	ret = ina231_mask_enable_set(mask_enable);
	if (ret) {
		return ret;
	}

	/* Total measurement time = ((bus_voltage_conversion_time + shunt_voltage_conversion_time) *
	 *			     number_of_measurement_to_average_over)
	 * In addition to this we add 10% to account for scheduling
	 */
	measurement_timeout_seconds = ceil((conv_time_enum_to_sec(configured_conv_time) * 2) *
					   average_enum_to_int(configured_avg) * 1.1);

	/* Config correct INA */
	ina231[name].config.avg = (enum ina231_config_avg)configured_avg;
	ina231[name].config.vbus_conv_time =
		(enum ina231_config_vbus_conv_time)configured_conv_time;
	ina231[name].config.vsh_conv_time = (enum ina231_config_vsh_conv_time)configured_conv_time;
	ina231[name].config.mode = INA231_CONFIG_MODE_SHUNT_BUS_CONT;

	ret = ina231_config_set(&ina231[name].config);
	if (ret) {
		return ret;
	}

	ret = ina231_close(&ina231_twi_cfg);
	if (ret) {
		return ret;
	}

	ina231_active_mask |= (1 << name);

	return 0;
}

int power_module_measurement_stop(enum ina_name name)
{
	int ret;

	LOG_DBG("Stopping measurement on: %d", name);
	/* Update array over active INAs */
	ina231_active_mask &= ~(1U << name);
	ina231_data_ready_mask &= ~(1U << name);

	ina231_twi_cfg.addr = ina231[name].address;

	ret = ina231_open(&ina231_twi_cfg);
	if (ret) {
		return ret;
	}

	ina231[name].config.mode = INA231_CONFIG_MODE_POWER_DOWN_CONT;

	ret = ina231_config_set(&ina231[name].config);
	if (ret) {
		return ret;
	}

	ret = ina231_close(&ina231_twi_cfg);
	if (ret) {
		return ret;
	}

	return 0;
}

int power_module_init(void)
{
	int ret;

	for (uint8_t i = 0; i < INA231_COUNT; i++) {
		ina231_twi_cfg.addr = ina231[i].address;
		ret = ina231_open(&ina231_twi_cfg);
		if (ret) {
			return ret;
		}

		ret = ina231_reset();
		if (ret) {
			return ret;
		}

		uint16_t calib;

		switch (i) {
		case VBAT:
			calib = (uint16_t)(INA231_VBAT_CALIBRATION);
			break;
		case VDD1_CODEC:
			calib = (uint16_t)(INA231_VDD1_CODEC_CALIBRATION);
			break;
		case VDD2_CODEC:
			calib = (uint16_t)(INA231_VDD2_CODEC_CALIBRATION);
			break;
		case VDD2_NRF:
			calib = (uint16_t)(INA231_VDD2_NRF_CALIBRATION);
			break;
		default:
			LOG_ERR("Unrecognized ina: %d", i);
			return -EIO;
		}

		ret = ina231_calibration_set(calib);
		if (ret) {
			return ret;
		}

		ret = ina231_close(&ina231_twi_cfg);
		if (ret) {
			return ret;
		}

		ret = power_module_measurement_stop((enum ina_name)i);
		if (ret) {
			return ret;
		}
	}

	ret = gpio_interrupt_init();
	if (ret) {
		return ret;
	}

	LOG_DBG("Power module initialized");

	(void)k_thread_create(&power_module_data, power_module_stack,
			      CONFIG_POWER_MODULE_STACK_SIZE, (k_thread_entry_t)power_module_thread,
			      NULL, NULL, NULL, K_PRIO_PREEMPT(CONFIG_POWER_MODULE_THREAD_PRIO), 0,
			      K_NO_WAIT);
	ret = k_thread_name_set(&power_module_data, "PWR MODULE");
	if (ret) {
		LOG_ERR("Failed to name thread, ret: %d", ret);
	}

	return 0;
}

static int cmd_print_all_rails(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	for (uint8_t i = 0; i < INA231_COUNT; i++) {
		shell_print(shell, "Id %d: - %s", i, ina231[i].name);
	}

	return 0;
}

static int cmd_pwr_meas_start(const struct shell *shell, size_t argc, const char **argv)
{
	int ret;
	uint8_t rail_idx;

	/* First argument is function, second is rail idx */
	if (argc != 2) {
		shell_error(shell, "Wrong number of arguments provided");
		return -EINVAL;
	}

	if (strcmp(argv[1], "all") == 0) {
		for (uint8_t i = 0; i < INA231_COUNT; i++) {
			ret = power_module_measurement_start(i, NULL);
			if (ret) {
				return ret;
			}
		}
		shell_print(shell, "Started measurements for all rails");

		return 0;

	} else if (!isdigit((int)argv[1][0])) {
		shell_error(shell, "Supplied argument is not numeric");
		return -EINVAL;
	}

	rail_idx = strtoul(argv[1], NULL, BASE_10);

	if (rail_idx >= INA231_COUNT) {
		shell_error(shell, "Selected rail ID out of range");
		return -EINVAL;
	}

	ret = power_module_measurement_start(rail_idx, NULL);
	if (ret) {
		return ret;
	}

	shell_print(shell, "Started measurements on idx: %d, name: %s", rail_idx,
		    ina231[rail_idx].name);

	return 0;
}

static int cmd_pwr_meas_stop(const struct shell *shell, size_t argc, const char **argv)
{
	int ret;
	uint8_t rail_idx;

	/* First argument is function, second is rail idx */
	if (argc != 2) {
		shell_error(shell, "Wrong number of arguments provided");
		return -EINVAL;
	}

	if (strcmp(argv[1], "all") == 0) {
		for (uint8_t i = 0; i < INA231_COUNT; i++) {
			ret = power_module_measurement_stop(i);
			if (ret) {
				return ret;
			}
		}
		shell_print(shell, "Stopped measurements for all rails");

		return 0;

	} else if (!isdigit((int)argv[1][0])) {
		shell_error(shell, "Supplied argument is not numeric");
		return -EINVAL;
	}

	rail_idx = strtoul(argv[1], NULL, BASE_10);

	if (rail_idx >= INA231_COUNT) {
		shell_error(shell, "Selected rail ID out of range");
		return -EINVAL;
	}

	ret = power_module_measurement_stop(rail_idx);
	if (ret) {
		return ret;
	}

	shell_print(shell, "Stopped measurements on idx: %d, name: %s", rail_idx,
		    ina231[rail_idx].name);

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	pwr_meas_cmd,
	SHELL_COND_CMD(CONFIG_SHELL, start, NULL, "Start measurement.", cmd_pwr_meas_start),
	SHELL_COND_CMD(CONFIG_SHELL, stop, NULL, "Stop measurement.", cmd_pwr_meas_stop),
	SHELL_COND_CMD(CONFIG_SHELL, print, NULL, "Print all rails.", cmd_print_all_rails),
	SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(power, &pwr_meas_cmd, "Start and stop power measurements", NULL);
