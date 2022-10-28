/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "ina231.h"

#include <zephyr/kernel.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/device.h>

#include "macros_common.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ina231, CONFIG_INA231_LOG_LEVEL);

#define RESET_TIME_MS 1
#define INA231_DEFAULT_CFG_REG_VAL (0x4127)

static const struct device *i2c_dev = DEVICE_DT_GET(DT_NODELABEL(i2c1));
static struct ina231_twi_config const *ina231_twi_cfg;

static int reg_read(uint8_t addr, uint16_t *reg_val)
{
	int ret;
	struct i2c_msg msgs[2];
	uint16_t reg = 0;

	if (!device_is_ready(i2c_dev) || ina231_twi_cfg == NULL) {
		return -EIO;
	}

	memset(msgs, 0, sizeof(msgs));

	/* Setup I2C messages */
	/*Send the address to read from */
	msgs[0].buf = &addr;
	msgs[0].len = 1;
	msgs[0].flags = I2C_MSG_WRITE;

	/* Read from device. STOP after this. */
	msgs[1].buf = (uint8_t *)&reg;
	msgs[1].len = 2;
	msgs[1].flags = I2C_MSG_RESTART | I2C_MSG_READ | I2C_MSG_STOP;

	ret = i2c_transfer(i2c_dev, &msgs[0], 2, ina231_twi_cfg->addr);
	if (ret) {
		return ret;
	}

	*reg_val = (uint16_t)((reg & 0x00FF) << 8) | ((reg & 0xFF00) >> 8);

	return 0;
}

static int reg_write(uint8_t addr, uint16_t data)
{
	int ret;
	struct i2c_msg msg;
	uint8_t write_bytes[3];

	if (!device_is_ready(i2c_dev) || ina231_twi_cfg == NULL) {
		return -EIO;
	}

	memset(&msg, 0, sizeof(msg));

	write_bytes[0] = addr;
	write_bytes[1] = ((data & 0xFF00) >> 8);
	write_bytes[2] = (data & 0x00FF);

	/* Setup I2C message */
	msg.buf = write_bytes;
	msg.len = 3;
	msg.flags = I2C_MSG_WRITE | I2C_MSG_STOP;

	ret = i2c_transfer(i2c_dev, &msg, 1, ina231_twi_cfg->addr);
	if (ret) {
		return ret;
	}

	return 0;
}

int ina231_open(struct ina231_twi_config const *cfg)
{
	if (ina231_twi_cfg != NULL || cfg == NULL) {
		return -EINVAL;
	}

	ina231_twi_cfg = cfg;

	return 0;
}

int ina231_close(struct ina231_twi_config const *cfg)
{
	if (cfg == NULL || ina231_twi_cfg != cfg || ina231_twi_cfg == NULL) {
		return -EINVAL;
	}

	ina231_twi_cfg = NULL;

	return 0;
}

int ina231_reset(void)
{
	int ret;

	ret = reg_write(REG_INA231_CONFIGURATION, INA231_CONFIG_RST_Msk);
	if (ret) {
		return ret;
	}

	/* Wait for device reset to complete. */
	(void)k_sleep(K_MSEC(RESET_TIME_MS));

	/* Check that reset bit is cleared and default values set in the config reg */
	uint16_t cfg_read;

	ret = reg_read(REG_INA231_CONFIGURATION, &cfg_read);
	if (ret) {
		return ret;
	}

	if (cfg_read != INA231_DEFAULT_CFG_REG_VAL) {
		LOG_ERR("Cfg reg reads 0x%x Should be: 0x%x", cfg_read, INA231_DEFAULT_CFG_REG_VAL);
		return -EIO;
	}

	return ret;
}

int ina231_config_set(struct ina231_config_reg *config)
{
	uint16_t config_reg;

	config_reg = (config->avg << INA231_CONFIG_AVG_Pos) & INA231_CONFIG_AVG_Msk;
	config_reg |= (config->vbus_conv_time << INA231_CONFIG_VBUS_CONV_TIME_Pos) &
		      INA231_CONFIG_VBUS_CONV_TIME_Msk;
	config_reg |= (config->vsh_conv_time << INA231_CONFIG_VSH_CONV_TIME_Pos) &
		      INA231_CONFIG_VSH_CONV_TIME_Msk;
	config_reg |= (config->mode << INA231_CONFIG_MODE_Pos) & INA231_CONFIG_MODE_Msk;
	/* Bit 14 needs to be set according to datasheet. */
	config_reg |= (INA231_CONFIG_MAGIC_ONE_Default << INA231_CONFIG_MAGIC_ONE_Pos) &
		      INA231_CONFIG_MAGIC_ONE_Msk;

	return reg_write(REG_INA231_CONFIGURATION, config_reg);
}

int ina231_config_get(struct ina231_config_reg *config)
{
	int ret;

	uint16_t config_reg;

	ret = reg_read(REG_INA231_CONFIGURATION, &config_reg);
	if (ret) {
		return ret;
	}

	config->avg = (enum ina231_config_avg)((config_reg & INA231_CONFIG_AVG_Msk) >>
					       INA231_CONFIG_AVG_Pos);

	config->vbus_conv_time = (enum ina231_config_vbus_conv_time)(
		(config_reg & INA231_CONFIG_VBUS_CONV_TIME_Msk) >>
		INA231_CONFIG_VBUS_CONV_TIME_Pos);

	config->vsh_conv_time = (enum ina231_config_vsh_conv_time)(
		(config_reg & INA231_CONFIG_VSH_CONV_TIME_Msk) >> INA231_CONFIG_VSH_CONV_TIME_Pos);

	config->mode = (enum ina231_config_mode)((config_reg & INA231_CONFIG_MODE_Msk) >>
						 INA231_CONFIG_MODE_Pos);

	return 0;
}

int ina231_shunt_voltage_reg_get(uint16_t *p_voltage)
{
	return reg_read(REG_INA231_SHUNT_VOLTAGE, p_voltage);
}

int ina231_bus_voltage_reg_get(uint16_t *p_voltage)
{
	return reg_read(REG_INA231_BUS_VOLTAGE, p_voltage);
}

int ina231_power_reg_get(uint16_t *p_power)
{
	return reg_read(REG_INA231_POWER, p_power);
}

int ina231_current_reg_get(uint16_t *p_current)
{
	return reg_read(REG_INA231_CURRENT, p_current);
}

int ina231_calibration_set(uint16_t calib)
{
	return reg_write(REG_INA231_CALIBRATION, calib);
}

int ina231_calibration_get(uint16_t *p_calib)
{
	return reg_read(REG_INA231_CALIBRATION, p_calib);
}

int ina231_mask_enable_set(uint16_t mask_enable)
{
	return reg_write(REG_INA231_MASK_ENABLE, mask_enable);
}

int ina231_mask_enable_get(uint16_t *p_mask_enable)
{
	return reg_read(REG_INA231_MASK_ENABLE, p_mask_enable);
}

int ina231_alert_limit_set(uint16_t alert_limit)
{
	return reg_write(REG_INA231_MASK_ENABLE, alert_limit);
}

int ina231_alert_limit_get(uint16_t *p_alert_limit)
{
	return reg_read(REG_INA231_MASK_ENABLE, p_alert_limit);
}

float conv_time_enum_to_sec(enum ina231_config_vbus_conv_time conv_time)
{
	switch (conv_time) {
	case INA231_CONFIG_VBUS_CONV_TIME_140_US:
		return 0.00014;
	case INA231_CONFIG_VBUS_CONV_TIME_204_US:
		return 0.000204;
	case INA231_CONFIG_VBUS_CONV_TIME_332_US:
		return 0.000332;
	case INA231_CONFIG_VBUS_CONV_TIME_588_US:
		return 0.000588;
	case INA231_CONFIG_VBUS_CONV_TIME_1100_US:
		return 0.0011;
	case INA231_CONFIG_VBUS_CONV_TIME_2116_US:
		return 0.002116;
	case INA231_CONFIG_VBUS_CONV_TIME_4156_US:
		return 0.004156;
	case INA231_CONFIG_VBUS_CONV_TIME_8244_US:
		return 0.008244;
	default:
		LOG_ERR("Invalid conversion time: %d", conv_time);
		ERR_CHK(-EINVAL);
	}

	CODE_UNREACHABLE;
	return 0;
}

uint16_t average_enum_to_int(enum ina231_config_avg average)
{
	switch (average) {
	case INA231_CONFIG_AVG_1:
		return 1;
	case INA231_CONFIG_AVG_4:
		return 4;
	case INA231_CONFIG_AVG_16:
		return 16;
	case INA231_CONFIG_AVG_64:
		return 64;
	case INA231_CONFIG_AVG_128:
		return 128;
	case INA231_CONFIG_AVG_256:
		return 256;
	case INA231_CONFIG_AVG_512:
		return 512;
	case INA231_CONFIG_AVG_1024:
		return 1024;
	default:
		LOG_ERR("Invalid average: %d", average);
		ERR_CHK(-EINVAL);
	}

	CODE_UNREACHABLE;
	return 0;
}
