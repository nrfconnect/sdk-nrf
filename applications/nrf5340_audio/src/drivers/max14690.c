/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "max14690.h"

#include <zephyr/kernel.h>
#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(max14690);

#define CONFIG_PMIC_I2C_ADDR (0x28)
#define MAX14690_CHIP_ID (0x01)

#define PMIC_REG_CHIPID (0x00)
#define PMIC_REG_CHIPREV (0x01)
#define PMIC_REG_STATUS_A (0x02)
#define PMIC_REG_STATUS_B (0x03)
#define PMIC_REG_STATUS_C (0x04)
#define PMIC_REG_INT_A (0x05)
#define PMIC_REG_INT_B (0x06)
#define PMIC_REG_INT_MASK_A (0x07)
#define PMIC_REG_INT_MASK_B (0x08)
#define PMIC_REG_I_LIM_CNTL (0x09)
#define PMIC_REG_CHG_CNTL_A (0x0A)
#define PMIC_REG_CHG_CNTL_B (0x0B)
#define PMIC_REG_CHG_TMR (0x0C)
#define PMIC_REG_BUCK_1_CFG (0x0D)
#define PMIC_REG_BUCK_1_V_SET (0x0E)
#define PMIC_REG_BUCK_2_CFG (0x0F)
#define PMIC_REG_BUCK_2_V_SET (0x10)
/* 0x11 unused/reserved */
#define PMIC_REG_LDO_1_CFG (0x12)
#define PMIC_REG_LDO_1_V_SET (0x13)
#define PMIC_REG_LDO_2_CFG (0x14)
#define PMIC_REG_LDO_2_V_SET (0x15)
#define PMIC_REG_LDO_3_CFG (0x16)
#define PMIC_REG_LDO_3_V_SET (0x17)
#define PMIC_REG_THRM_CFG (0x18)
#define PMIC_REG_MON_CFG (0x19)
#define PMIC_REG_BOOT_CFG (0x1A)
#define PMIC_REG_PIN_STAT (0x1B)
#define PMIC_REG_BUCK_1_2_EXTRA_CTRL (0x1C)
#define PMIC_REG_PWR_CFG (0x1D)
/* 0x1E unused/NULL */
#define PMIC_REG_PWR_OFF (0x1F)

#define PMIC_PWR_OFF_CMD (0xB2)

const static struct device *i2c_dev;

/* @brief Calculates the bit mask for a given number of settings.
 *  @param[in] num_settings: Number of settings
 *  @param[out] mask:        Mask of "1"
 *  @Note: Example. num_settings = 8, mask gives 00000111
 */
static int mask_calculate(uint8_t num_settings, uint8_t *mask)
{
	if ((num_settings & (num_settings - 1)) != 0) {
		LOG_ERR("num_settings does not cover an entire area! Must be 1, 2, 4, ...");
		return -EINVAL;
	}

	*mask = num_settings - 1;
	return 0;
}

/* @brief Gets/reads a register from the device.
 */
static int register_get(uint8_t reg, uint8_t *data)
{
	int ret;

	if (i2c_dev == NULL) {
		return -EIO;
	}

	ret = i2c_reg_read_byte(i2c_dev, CONFIG_PMIC_I2C_ADDR, reg, data);
	if (ret) {
		return ret;
	}

	return 0;
}

/* @brief Writes a register to the device.
 */
static int register_set(uint8_t reg, uint8_t data)
{
	int ret;

	if (i2c_dev == NULL) {
		return -EIO;
	}

	uint8_t rd;

	ret = i2c_reg_write_byte(i2c_dev, CONFIG_PMIC_I2C_ADDR, reg, data);
	if (ret) {
		return ret;
	}

	ret = i2c_reg_read_byte(i2c_dev, CONFIG_PMIC_I2C_ADDR, reg, &rd);
	if (ret) {
		return ret;
	}

	if (rd != data) {
		LOG_ERR("Rd from MAX14690 uneq to written val. Reg: 0x%x", reg);
		LOG_ERR("wr: 0x%x", data);
		LOG_ERR("rd: 0x%x", rd);
		return -EIO;
	}

	return 0;
}

/* @brief Modifies a register in the device.
 *  @Note: Set and clear masks cannot overlap.
 */
static int register_modify(uint8_t reg, uint8_t set_mask, uint8_t clear_mask)
{
	int ret;

	if ((set_mask | clear_mask) == 0) {
		return -EINVAL;
	}

	uint8_t tmp;

	ret = register_get(reg, &tmp);
	if (ret) {
		return ret;
	}

	if ((set_mask & clear_mask) == 0) {
		uint8_t old_val = tmp;

		tmp |= set_mask;
		tmp &= ~(clear_mask);

		return ((tmp != old_val) ? register_set(reg, tmp) : 0);
	}

	return -EINVAL;
}

/* @brief If certain bits should be altered in a register
 *  @param[in] reg: Register address
 *  @param[in] value: Value to be written
 *  @param[in] mask: Mask of "1" where bits shall be changed
 */
static int register_bitfield_modify(uint8_t reg, uint8_t value, uint8_t mask)
{
	if (value & ~mask) {
		LOG_ERR("No bits outside the mask can be set");
		return -EINVAL;
	}

	uint8_t clear_mask = value ^ mask;

	return register_modify(reg, value, clear_mask);
}

int max14690_buck_1_cfg_ind_cfg(enum pmic_buck_cfg_ind cfg)
{
	int ret;
	uint8_t mask;

	ret = mask_calculate(PMIC_BUCK_CFG_IND_LEN, &mask);
	if (ret) {
		return ret;
	}

	return register_bitfield_modify(PMIC_REG_BUCK_1_CFG, cfg << PMIC_BUCK_CFG_IND_OFFS,
					mask << PMIC_BUCK_CFG_IND_OFFS);
}

int max14690_buck_1_cfg_mode_cfg(enum pmic_buck_cfg_mode cfg)
{
	int ret;
	uint8_t mask;

	ret = mask_calculate(PMIC_BUCK_CFG_MODE_LEN, &mask);
	if (ret) {
		return ret;
	}

	return register_bitfield_modify(PMIC_REG_BUCK_1_CFG, cfg << PMIC_BUCK_CFG_MODE_OFFS,
					mask << PMIC_BUCK_CFG_MODE_OFFS);
}

int max14690_buck_1_cfg_enable_cfg(enum pmic_buck_cfg_en cfg)
{
	int ret;
	uint8_t mask;

	ret = mask_calculate(PMIC_BUCK_CFG_EN_LEN, &mask);
	if (ret) {
		return ret;
	}

	return register_bitfield_modify(PMIC_REG_BUCK_1_CFG, cfg << PMIC_BUCK_CFG_EN_OFFS,
					mask << PMIC_BUCK_CFG_EN_OFFS);
}

int max14690_buck_2_cfg_ind_cfg(enum pmic_buck_cfg_ind cfg)
{
	int ret;
	uint8_t mask;

	ret = mask_calculate(PMIC_BUCK_CFG_IND_LEN, &mask);
	if (ret) {
		return ret;
	}

	return register_bitfield_modify(PMIC_REG_BUCK_2_CFG, cfg << PMIC_BUCK_CFG_IND_OFFS,
					mask << PMIC_BUCK_CFG_IND_OFFS);
}

int max14690_buck_2_cfg_mode_cfg(enum pmic_buck_cfg_mode cfg)
{
	int ret;
	uint8_t mask;

	ret = mask_calculate(PMIC_BUCK_CFG_MODE_LEN, &mask);
	if (ret) {
		return ret;
	}

	return register_bitfield_modify(PMIC_REG_BUCK_2_CFG, cfg << PMIC_BUCK_CFG_MODE_OFFS,
					mask << PMIC_BUCK_CFG_MODE_OFFS);
}

int max14690_buck_2_cfg_enable_cfg(enum pmic_buck_cfg_en cfg)
{
	int ret;
	uint8_t mask;

	ret = mask_calculate(PMIC_BUCK_CFG_EN_LEN, &mask);
	if (ret) {
		return ret;
	}

	return register_bitfield_modify(PMIC_REG_BUCK_2_CFG, cfg << PMIC_BUCK_CFG_EN_OFFS,
					mask << PMIC_BUCK_CFG_EN_OFFS);
}

/* @brief Checks if selected voltage is valid and writes it to the PMIC.
 */
static int buck_voltage_cfg(uint16_t mv, uint16_t voltage_min_mv, uint16_t voltage_max_mv,
			    uint16_t voltage_step_mv, uint8_t reg)
{
	if (mv < voltage_min_mv || mv > voltage_max_mv) {
		LOG_ERR("Buck voltage setting out of range");
		return -EINVAL;
	}

	if (((mv - voltage_min_mv) % voltage_step_mv) != 0) {
		LOG_ERR("Buck voltage step not valid");
		return -EINVAL;
	}

	uint8_t reg_val_wr = (mv - voltage_min_mv) / voltage_step_mv;

	return register_set(reg, reg_val_wr);
}

int max14690_buck_1_voltage_cfg(uint16_t mv)
{
	return buck_voltage_cfg(mv, PMIC_BUCK_1_VOLTAGE_MIN_MV, PMIC_BUCK_1_VOLTAGE_MAX_MV,
				PMIC_BUCK_1_VOLTAGE_STEP_MV, PMIC_REG_BUCK_1_V_SET);
}

int max14690_buck_2_voltage_cfg(uint16_t mv)
{
	return buck_voltage_cfg(mv, PMIC_BUCK_2_VOLTAGE_MIN_MV, PMIC_BUCK_2_VOLTAGE_MAX_MV,
				PMIC_BUCK_2_VOLTAGE_STEP_MV, PMIC_REG_BUCK_2_V_SET);
}

int max14690_ldo_1_cfg_mode_cfg(enum pmic_ldo_cfg_mode cfg)
{
	int ret;
	uint8_t mask;

	ret = mask_calculate(PMIC_LDO_CFG_MODE_LEN, &mask);
	if (ret) {
		return ret;
	}

	return register_bitfield_modify(PMIC_REG_LDO_1_CFG, cfg << PMIC_LDO_CFG_MODE_OFFS,
					mask << PMIC_LDO_CFG_MODE_OFFS);
}

int max14690_ldo_1_cfg_enable_cfg(enum pmic_ldo_cfg_en cfg)
{
	int ret;
	uint8_t mask;

	ret = mask_calculate(PMIC_LDO_CFG_EN_LEN, &mask);
	if (ret) {
		return ret;
	}

	return register_bitfield_modify(PMIC_REG_LDO_1_CFG, cfg << PMIC_LDO_CFG_EN_OFFS,
					mask << PMIC_LDO_CFG_EN_OFFS);
}

int max14690_ldo_1_cfg_dsc_cfg(enum pmic_ldo_cfg_dsc cfg)
{
	int ret;
	uint8_t mask;

	ret = mask_calculate(PMIC_LDO_CFG_DSC_LEN, &mask);
	if (ret) {
		return ret;
	}

	return register_bitfield_modify(PMIC_REG_LDO_1_CFG, cfg << PMIC_LDO_CFG_DSC_OFFS,
					mask << PMIC_LDO_CFG_DSC_OFFS);
}

int max14690_ldo_2_cfg_mode_cfg(enum pmic_ldo_cfg_mode cfg)
{
	int ret;
	uint8_t mask;

	ret = mask_calculate(PMIC_LDO_CFG_MODE_LEN, &mask);
	if (ret) {
		return ret;
	}

	return register_bitfield_modify(PMIC_REG_LDO_2_CFG, cfg << PMIC_LDO_CFG_MODE_OFFS,
					mask << PMIC_LDO_CFG_MODE_OFFS);
}

int max14690_ldo_2_cfg_enable_cfg(enum pmic_ldo_cfg_en cfg)
{
	int ret;
	uint8_t mask;

	ret = mask_calculate(PMIC_LDO_CFG_EN_LEN, &mask);
	if (ret) {
		return ret;
	}

	return register_bitfield_modify(PMIC_REG_LDO_2_CFG, cfg << PMIC_LDO_CFG_EN_OFFS,
					mask << PMIC_LDO_CFG_EN_OFFS);
}

int max14690_ldo_2_cfg_dsc_cfg(enum pmic_ldo_cfg_dsc cfg)
{
	int ret;
	uint8_t mask;

	ret = mask_calculate(PMIC_LDO_CFG_DSC_LEN, &mask);
	if (ret) {
		return ret;
	}

	return register_bitfield_modify(PMIC_REG_LDO_2_CFG, cfg << PMIC_LDO_CFG_DSC_OFFS,
					mask << PMIC_LDO_CFG_DSC_OFFS);
}

int max14690_ldo_3_cfg_mode_cfg(enum pmic_ldo_cfg_mode cfg)
{
	int ret;
	uint8_t mask;

	ret = mask_calculate(PMIC_LDO_CFG_MODE_LEN, &mask);
	if (ret) {
		return ret;
	}

	return register_bitfield_modify(PMIC_REG_LDO_3_CFG, cfg << PMIC_LDO_CFG_MODE_OFFS,
					mask << PMIC_LDO_CFG_MODE_OFFS);
}

int max14690_ldo_3_cfg_enable_cfg(enum pmic_ldo_cfg_en cfg)
{
	int ret;
	uint8_t mask;

	ret = mask_calculate(PMIC_LDO_CFG_EN_LEN, &mask);
	if (ret) {
		return ret;
	}

	return register_bitfield_modify(PMIC_REG_LDO_3_CFG, cfg << PMIC_LDO_CFG_EN_OFFS,
					mask << PMIC_LDO_CFG_EN_OFFS);
}

int max14690_ldo_3_cfg_dsc_cfg(enum pmic_ldo_cfg_dsc cfg)
{
	int ret;
	uint8_t mask;

	ret = mask_calculate(PMIC_LDO_CFG_DSC_LEN, &mask);
	if (ret) {
		return ret;
	}

	return register_bitfield_modify(PMIC_REG_LDO_3_CFG, cfg << PMIC_LDO_CFG_DSC_OFFS,
					mask << PMIC_LDO_CFG_DSC_OFFS);
}

int max14690_boot_cfg_stay_on_cfg(enum pmic_pwr_cfg_stay_on cfg)
{
	int ret;
	uint8_t mask;

	ret = mask_calculate(PMIC_PWR_CFG_STAY_ON_LEN, &mask);
	if (ret) {
		return ret;
	}

	return register_bitfield_modify(PMIC_REG_PWR_CFG, cfg << PMIC_PWR_CFG_STAY_ON_OFFS,
					mask << PMIC_PWR_CFG_STAY_ON_OFFS);
}

int max14690_boot_cfg_pfn_cfg(enum pmic_pwr_cfg_pfn cfg)
{
	int ret;
	uint8_t mask;

	ret = mask_calculate(PMIC_PWR_CFG_PFN_LEN, &mask);
	if (ret) {
		return ret;
	}

	return register_bitfield_modify(PMIC_REG_PWR_CFG, cfg << PMIC_PWR_CFG_PFN_OFFS,
					mask << PMIC_PWR_CFG_PFN_OFFS);
}

int max14690_mtn_chg_tmr_cfg(enum pmic_mtn_chg_tmr cfg)
{
	int ret;
	uint8_t mask;

	ret = mask_calculate(PMIC_MTN_CHG_TMR_LEN, &mask);
	if (ret) {
		return ret;
	}

	return register_bitfield_modify(PMIC_REG_CHG_TMR, cfg << PMIC_MTN_CHG_TMR_OFFS,
					mask << PMIC_MTN_CHG_TMR_OFFS);
}

int max14690_fast_chg_tmr_cfg(enum pmic_fast_chg_tmr cfg)
{
	int ret;
	uint8_t mask;

	ret = mask_calculate(PMIC_FAST_CHG_TMR_LEN, &mask);
	if (ret) {
		return ret;
	}

	return register_bitfield_modify(PMIC_REG_CHG_TMR, cfg << PMIC_FAST_CHG_TMR_OFFS,
					mask << PMIC_FAST_CHG_TMR_OFFS);
}

int max14690_pre_chg_tmr_cfg(enum pmic_pre_chg_tmr cfg)
{
	int ret;
	uint8_t mask;

	ret = mask_calculate(PMIC_PRE_CHG_TMR_LEN, &mask);
	if (ret) {
		return ret;
	}

	return register_bitfield_modify(PMIC_REG_CHG_TMR, cfg << PMIC_PRE_CHG_TMR_OFFS,
					mask << PMIC_PRE_CHG_TMR_OFFS);
}

int max14690_pmic_thrm_cfg(enum pmic_thrm_cfg cfg)
{
	int ret;
	uint8_t mask;

	if (cfg == PMIC_THRM_CFG_DO_NOT_USE) {
		LOG_ERR("Trying to set invalid value");
		return -EINVAL;
	}

	ret = mask_calculate(PMIC_THRM_CFG_LEN, &mask);
	if (ret) {
		return ret;
	}

	return register_bitfield_modify(PMIC_REG_THRM_CFG, cfg << PMIC_THRM_CFG_OFFS,
					mask << PMIC_THRM_CFG_OFFS);
}

int max14690_pmic_mon_ratio_cfg(enum pmic_mon_ratio_cfg cfg)
{
	int ret;
	uint8_t mask;

	ret = mask_calculate(PMIC_MON_RATIO_LEN, &mask);
	if (ret) {
		return ret;
	}

	return register_bitfield_modify(PMIC_REG_MON_CFG, cfg << PMIC_MON_RATIO_CFG_OFFS,
					mask << PMIC_MON_RATIO_CFG_OFFS);
}

int max14690_pmic_mon_off_mode_cfg(enum pmic_mon_off_mode_cfg cfg)
{
	int ret;
	uint8_t mask;

	ret = mask_calculate(PMIC_MON_OFF_MODE_LEN, &mask);
	if (ret) {
		return ret;
	}

	return register_bitfield_modify(PMIC_REG_MON_CFG, cfg << PMIC_MON_OFF_MODE_CFG_OFFS,
					mask << PMIC_MON_OFF_MODE_CFG_OFFS);
}

int max14690_pmic_mon_ctr_cfg(enum pmic_mon_ctr_cfg cfg)
{
	int ret;
	uint8_t mask;

	ret = mask_calculate(PMIC_MON_CTR_LEN, &mask);
	if (ret) {
		return ret;
	}

	return register_bitfield_modify(PMIC_REG_MON_CFG, cfg << PMIC_MON_CTR_OFFS,
					mask << PMIC_MON_CTR_OFFS);
}

int max14690_pwr_off(void)
{
	return register_set(PMIC_REG_PWR_OFF, PMIC_PWR_OFF_CMD);
}

int max14690_init(const struct device *dev)
{
	i2c_dev = dev;

	if (i2c_dev == NULL) {
		LOG_ERR("Failed to get pointer to I2C device!");
		return -EINVAL;
	}

	/* Check Chip ID */
	uint8_t read;

	if (register_get(PMIC_REG_CHIPID, &read)) {
		LOG_ERR("Failed to read hardware ID register");
		return -EIO;
	}

	if (read != MAX14690_CHIP_ID) {
		LOG_ERR("Chip ID mismatch");
		return -EINVAL;
	}

	return 0;
}
