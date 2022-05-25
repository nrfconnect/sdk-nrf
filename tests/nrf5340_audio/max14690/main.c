/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <ztest.h>
#include <zephyr/device.h>
#include "max14690.h"

#define RET_SUCCESS 0
#define TEST_MAX14690_I2C_ADDR 0x28

static void i2c_reg_read_byte_mock_std_setup(uint8_t reg_addr_in, uint8_t return_val)
{
	ztest_expect_value(i2c_reg_read_byte, dev_addr, TEST_MAX14690_I2C_ADDR);
	ztest_expect_value(i2c_reg_read_byte, reg_addr, reg_addr_in);
	ztest_returns_value(i2c_reg_read_byte,
			    return_val); /* Mocked existing register value in MAX14690 */
}

static void i2c_reg_write_byte_mock_std_setup(uint8_t reg_addr_in, uint8_t write_val)
{
	ztest_expect_value(i2c_reg_write_byte, dev_addr, TEST_MAX14690_I2C_ADDR);
	ztest_expect_value(i2c_reg_write_byte, reg_addr, reg_addr_in);
	ztest_expect_value(i2c_reg_write_byte, value, write_val);
}

/* Passing test, normal function */
void test_init_ok(void)
{
	i2c_reg_read_byte_mock_std_setup(0, 0x01);

	const struct device *dummy_ptr = (const struct device *)0x01;

	zassert_equal(max14690_init(dummy_ptr), RET_SUCCESS, "Init did not return 0");
}

/* Chip returning wrong ID */
void test_init_wrong_chip_id(void)
{
	i2c_reg_read_byte_mock_std_setup(0, 0x02);

	const struct device *dummy_ptr = (const struct device *)0x01;

	zassert_not_equal(max14690_init(dummy_ptr), RET_SUCCESS,
			  "Init did not fail due to wrong chip ID");
}

/* Charge timer set cfg value OK*/
void test_charge_tmr_set_cfg(void)
{
	i2c_reg_read_byte_mock_std_setup(0x0C, 0x09);
	i2c_reg_write_byte_mock_std_setup(0x0C, 0x39);
	i2c_reg_read_byte_mock_std_setup(0x0C, 0x39);

	zassert_equal(max14690_mtn_chg_tmr_cfg(PMIC_MTN_CHG_TMR_60_MIN), RET_SUCCESS, "Fail");
}

/* Wrong value read back from register after write */
void test_wrong_value_read_back(void)
{
	i2c_reg_read_byte_mock_std_setup(0x0D, 0xFF);
	i2c_reg_write_byte_mock_std_setup(0x0D, 0xFE);
	i2c_reg_read_byte_mock_std_setup(0x0D, 0xFF);

	zassert_not_equal(max14690_buck_1_cfg_ind_cfg(PMIC_BUCK_CFG_IND_2_2_UH), RET_SUCCESS,
			  "Fail");
}

/* Buck 1 set cfg inductor value OK*/
void test_buck_1_set_cfg_ind(void)
{
	i2c_reg_read_byte_mock_std_setup(0x0D, 0xFF);
	i2c_reg_write_byte_mock_std_setup(0x0D, 0xFE);
	i2c_reg_read_byte_mock_std_setup(0x0D, 0xFE);

	zassert_equal(max14690_buck_1_cfg_ind_cfg(PMIC_BUCK_CFG_IND_2_2_UH), RET_SUCCESS, "Fail");
}

/* Test, correct mode already set. No need to perform furhter write. */
void test_buck_1_set_cfg_mode_identical(void)
{
	i2c_reg_read_byte_mock_std_setup(0x0D, 0xE7);
	/* No further calls needed as value in reg is already correct */

	zassert_equal(max14690_buck_1_cfg_mode_cfg(PMIC_BUCK_CFG_EN_WHEN_MPC1_HIGH), RET_SUCCESS,
		      "Fail");
}

/* Test, correct mode already set. No need to perform furhter write. */
void test_buck_1_set_cfg_mode(void)
{
	i2c_reg_read_byte_mock_std_setup(0x0D, 0x42);
	i2c_reg_write_byte_mock_std_setup(0x0D, 0x44);
	i2c_reg_read_byte_mock_std_setup(0x0D, 0x44);

	zassert_equal(max14690_buck_1_cfg_mode_cfg(PMIC_BUCK_CFG_MODE_EN_WHEN_MPC0_HIGH),
		      RET_SUCCESS, "Fail");
}

/* Test, correct enable already set. No need to perform furhter write. */
void test_buck_1_set_cfg_enable_identical(void)
{
	i2c_reg_read_byte_mock_std_setup(0x0D, 0xEF);
	/* No further calls needed as value in reg is already correct */

	zassert_equal(max14690_buck_1_cfg_enable_cfg(PMIC_BUCK_CFG_EN_ENABLED), RET_SUCCESS,
		      "Fail");
}

/* Test, correct enable set. */
void test_buck_1_set_cfg_enable(void)
{
	i2c_reg_read_byte_mock_std_setup(0x0D, 0xF0);
	i2c_reg_write_byte_mock_std_setup(0x0D, 0xE8);
	i2c_reg_read_byte_mock_std_setup(0x0D, 0xE8);

	zassert_equal(max14690_buck_1_cfg_enable_cfg(PMIC_BUCK_CFG_EN_ENABLED), RET_SUCCESS,
		      "Fail");
}

/* Test to set buck 1 to 1800 mv. This should be converted to reg value 40 (25 mV step). */
void test_buck_1_set_valid_voltage_max(void)
{
	i2c_reg_write_byte_mock_std_setup(0x0E, 40);
	i2c_reg_read_byte_mock_std_setup(0x0E, 40);

	zassert_equal(max14690_buck_1_voltage_cfg(1800), RET_SUCCESS, "fail");
}

/* Test to set buck 1 to 800 mv. This should be converted to reg value 0 (25 mV step). */
void test_buck_1_set_valid_voltage_min(void)
{
	i2c_reg_write_byte_mock_std_setup(0x0E, 0);
	i2c_reg_read_byte_mock_std_setup(0x0E, 0);

	zassert_equal(max14690_buck_1_voltage_cfg(800), RET_SUCCESS, "fail");
}

/* Test to set buck 1 to 950 mv. This should be converted to reg value 6 (25 mV step). */
void test_buck_1_set_valid_voltage_other(void)
{
	i2c_reg_write_byte_mock_std_setup(0x0E, 6);
	i2c_reg_read_byte_mock_std_setup(0x0E, 6);

	zassert_equal(max14690_buck_1_voltage_cfg(950), RET_SUCCESS, "fail");
}

/* Test to set buck 1 too low. Should return error code. */
void test_buck_1_set_valid_voltage_too_low(void)
{
	zassert_not_equal(max14690_buck_1_voltage_cfg(775), RET_SUCCESS, "fail");
}

/* Test to set buck 1 too high Should return error code. */
void test_buck_1_set_valid_voltage_too_high(void)
{
	zassert_not_equal(max14690_buck_1_voltage_cfg(1825), RET_SUCCESS, "fail");
}

/* Test to set buck 1 to an invalid value. Should return error code. */
void test_buck_1_set_valid_voltage_illegal_val(void)
{
	zassert_not_equal(max14690_buck_1_voltage_cfg(815), RET_SUCCESS, "fail");
}

/* Buck 2 set cfg inductor value OK*/
void test_buck_2_set_cfg_ind(void)
{
	i2c_reg_read_byte_mock_std_setup(0x0F, 0xFF);
	i2c_reg_write_byte_mock_std_setup(0x0F, 0xFE);
	i2c_reg_read_byte_mock_std_setup(0x0F, 0xFE);

	zassert_equal(max14690_buck_2_cfg_ind_cfg(PMIC_BUCK_CFG_IND_2_2_UH), RET_SUCCESS, "Fail");
}

/* Test, correct mode already set. No need to perform furhter write. */
void test_buck_2_set_cfg_mode_identical(void)
{
	i2c_reg_read_byte_mock_std_setup(0x0F, 0xE7);
	/* No further calls needed as value in reg is already correct */

	zassert_equal(max14690_buck_2_cfg_mode_cfg(PMIC_BUCK_CFG_EN_WHEN_MPC1_HIGH), RET_SUCCESS,
		      "Fail");
}

/* Test, correct mode already set. No need to perform furhter write. */
void test_buck_2_set_cfg_mode(void)
{
	i2c_reg_read_byte_mock_std_setup(0x0F, 0x42);
	i2c_reg_write_byte_mock_std_setup(0x0F, 0x44);
	i2c_reg_read_byte_mock_std_setup(0x0F, 0x44);

	zassert_equal(max14690_buck_2_cfg_mode_cfg(PMIC_BUCK_CFG_MODE_EN_WHEN_MPC0_HIGH),
		      RET_SUCCESS, "Fail");
}

/* Test, correct enable already set. No need to perform furhter write. */
void test_buck_2_set_cfg_enable_identical(void)
{
	i2c_reg_read_byte_mock_std_setup(0x0F, 0xEF);
	/* No further calls needed as value in reg is already correct */

	zassert_equal(max14690_buck_2_cfg_enable_cfg(PMIC_BUCK_CFG_EN_ENABLED), RET_SUCCESS,
		      "Fail");
}

/* Test, correct enable set. */
void test_buck_2_set_cfg_enable(void)
{
	i2c_reg_read_byte_mock_std_setup(0x0F, 0xF0);
	i2c_reg_write_byte_mock_std_setup(0x0F, 0xE8);
	i2c_reg_read_byte_mock_std_setup(0x0F, 0xE8);

	zassert_equal(max14690_buck_2_cfg_enable_cfg(PMIC_BUCK_CFG_EN_ENABLED), RET_SUCCESS,
		      "Fail");
}

/* Test to set buck 2 to 3300 mv. This should be converted to reg value 36 (50 mV step). */
void test_buck_2_set_valid_voltage_max(void)
{
	i2c_reg_write_byte_mock_std_setup(0x10, 36);
	i2c_reg_read_byte_mock_std_setup(0x10, 36);

	zassert_equal(max14690_buck_2_voltage_cfg(3300), RET_SUCCESS, "fail");
}

/* Test to set buck 2 to 1500 mv. This should be converted to reg value 0 (50 mV step). */
void test_buck_2_set_valid_voltage_min(void)
{
	i2c_reg_write_byte_mock_std_setup(0x10, 0);
	i2c_reg_read_byte_mock_std_setup(0x10, 0);

	zassert_equal(max14690_buck_2_voltage_cfg(1500), RET_SUCCESS, "fail");
}

/* Test to set buck 2 to 2000 mv. This should be converted to reg value 10 (50 mV step). */
void test_buck_2_set_valid_voltage_other(void)
{
	i2c_reg_write_byte_mock_std_setup(0x10, 10);
	i2c_reg_read_byte_mock_std_setup(0x10, 10);

	zassert_equal(max14690_buck_2_voltage_cfg(2000), RET_SUCCESS, "fail");
}

/* Test to set buck 2 too low. Should return error code. */
void test_buck_2_set_valid_voltage_too_low(void)
{
	zassert_not_equal(max14690_buck_2_voltage_cfg(1475), RET_SUCCESS, "fail");
}

/* Test to set buck 2 too high. Should return error code. */
void test_buck_2_set_valid_voltage_too_high(void)
{
	zassert_not_equal(max14690_buck_2_voltage_cfg(3325), RET_SUCCESS, "fail");
}

/* Test to set buck 2 to an illegal value. Should return error code. */
void test_buck_2_set_valid_voltage_illegal_val(void)
{
	zassert_not_equal(max14690_buck_2_voltage_cfg(3322), RET_SUCCESS, "fail");
}

/* Set LDO mode from 0 to 1 - OK*/
void test_ldo_1_cfg_mode_set(void)
{
	i2c_reg_read_byte_mock_std_setup(0x12, 0x00);
	i2c_reg_write_byte_mock_std_setup(0x12, 0x01);
	i2c_reg_read_byte_mock_std_setup(0x12, 0x01);

	zassert_equal(max14690_ldo_1_cfg_mode_cfg(PMIC_LDO_CFG_MODE_LOAD_SWITCH), RET_SUCCESS,
		      "Fail");
}

/* Select different enable configuration for LDO 1 - OK */
void test_ldo_1_cfg_enable_set(void)
{
	i2c_reg_read_byte_mock_std_setup(0x12, 0xFA);
	i2c_reg_write_byte_mock_std_setup(0x12, 0xFC);
	i2c_reg_read_byte_mock_std_setup(0x12, 0xFC);

	zassert_equal(max14690_ldo_1_cfg_enable_cfg(PMIC_LDO_CFG_EN_WHEN_MPC0_HIGH), RET_SUCCESS,
		      "Fail");
}

/* Set discharge control to 0 for LDO 1 - OK*/
void test_ldo_1_cfg_dsc_set(void)
{
	i2c_reg_read_byte_mock_std_setup(0x12, 0xFF);
	i2c_reg_write_byte_mock_std_setup(0x12, 0xF7);
	i2c_reg_read_byte_mock_std_setup(0x12, 0xF7);

	zassert_equal(max14690_ldo_1_cfg_dsc_cfg(PMIC_LDO_CFG_DSC_HARD_RESET_ONLY), RET_SUCCESS,
		      "Fail");
}

/* Set LDO mode from 0 to 1 - OK*/
void test_ldo_2_cfg_mode_set(void)
{
	i2c_reg_read_byte_mock_std_setup(0x14, 0x00);
	i2c_reg_write_byte_mock_std_setup(0x14, 0x01);
	i2c_reg_read_byte_mock_std_setup(0x14, 0x01);

	zassert_equal(max14690_ldo_2_cfg_mode_cfg(PMIC_LDO_CFG_MODE_LOAD_SWITCH), RET_SUCCESS,
		      "Fail");
}

/* Select different enable configuration for LDO 2 - OK*/
void test_ldo_2_cfg_enable_set(void)
{
	i2c_reg_read_byte_mock_std_setup(0x14, 0xFA);
	i2c_reg_write_byte_mock_std_setup(0x14, 0xFC);
	i2c_reg_read_byte_mock_std_setup(0x14, 0xFC);

	zassert_equal(max14690_ldo_2_cfg_enable_cfg(PMIC_LDO_CFG_EN_WHEN_MPC0_HIGH), RET_SUCCESS,
		      "Fail");
}

/* Set discharge control to 0 for LDO 2 - OK*/
void test_ldo_2_cfg_dsc_set(void)
{
	i2c_reg_read_byte_mock_std_setup(0x14, 0xFF);
	i2c_reg_write_byte_mock_std_setup(0x14, 0xF7);
	i2c_reg_read_byte_mock_std_setup(0x14, 0xF7);

	zassert_equal(max14690_ldo_2_cfg_dsc_cfg(PMIC_LDO_CFG_DSC_HARD_RESET_ONLY), RET_SUCCESS,
		      "Fail");
}

/* Set LDO mode from 0 to 1 - OK*/
void test_ldo_3_cfg_mode_set(void)
{
	i2c_reg_read_byte_mock_std_setup(0x16, 0x00);
	i2c_reg_write_byte_mock_std_setup(0x16, 0x01);
	i2c_reg_read_byte_mock_std_setup(0x16, 0x01);

	zassert_equal(max14690_ldo_3_cfg_mode_cfg(PMIC_LDO_CFG_MODE_LOAD_SWITCH), RET_SUCCESS,
		      "Fail");
}

/* Select different enable configuration for LDO 3 - OK*/
void test_ldo_3_cfg_enable_set(void)
{
	i2c_reg_read_byte_mock_std_setup(0x16, 0xFA);
	i2c_reg_write_byte_mock_std_setup(0x16, 0xFC);
	i2c_reg_read_byte_mock_std_setup(0x16, 0xFC);

	zassert_equal(max14690_ldo_3_cfg_enable_cfg(PMIC_LDO_CFG_EN_WHEN_MPC0_HIGH), RET_SUCCESS,
		      "Fail");
}

/* Set discharge control to 0 for LDO 3 - OK*/
void test_ldo_3_cfg_dsc_set(void)
{
	i2c_reg_read_byte_mock_std_setup(0x16, 0xFF);
	i2c_reg_write_byte_mock_std_setup(0x16, 0xF7);
	i2c_reg_read_byte_mock_std_setup(0x16, 0xF7);

	zassert_equal(max14690_ldo_3_cfg_dsc_cfg(PMIC_LDO_CFG_DSC_HARD_RESET_ONLY), RET_SUCCESS,
		      "Fail");
}

/* Enable JEITA and monitoring selector - OK*/
void test_thrm_cfg_jeita_and_therm_set(void)
{
	i2c_reg_read_byte_mock_std_setup(0x18, 0x00);
	i2c_reg_write_byte_mock_std_setup(0x18, 0x03);
	i2c_reg_read_byte_mock_std_setup(0x18, 0x03);

	zassert_equal(max14690_pmic_thrm_cfg(PMIC_THRM_CFG_THERM_ENABLED_JEITA_ENABLED),
		      RET_SUCCESS, "Fail");
}

/* Enable JEITA and dinable monitoring selector - OK*/
void test_thrm_cfg_jeita_and_therm_invalid_value_set(void)
{
	zassert_equal(max14690_pmic_thrm_cfg(PMIC_THRM_CFG_DO_NOT_USE), -EINVAL, "Fail");
}

/* Enable JEITA and monitoring selector - OK*/
void test_mon_cfg_ratio_set(void)
{
	i2c_reg_read_byte_mock_std_setup(0x19, 0x30);
	i2c_reg_write_byte_mock_std_setup(0x19, 0x10);
	i2c_reg_read_byte_mock_std_setup(0x19, 0x10);

	zassert_equal(max14690_pmic_mon_ratio_cfg(PMIC_MON_RATIO_3_1), RET_SUCCESS, "Fail");
}

/* Set StayOn bit - OK*/
void test_pwr_cfg_stay_on_set(void)
{
	i2c_reg_read_byte_mock_std_setup(0x1D, 0x00);
	i2c_reg_write_byte_mock_std_setup(0x1D, 0x01);
	i2c_reg_read_byte_mock_std_setup(0x1D, 0x01);

	zassert_equal(max14690_boot_cfg_stay_on_cfg(PMIC_PWR_CFG_STAY_ON_ENABLED), RET_SUCCESS,
		      "Fail");
}

/* Clear StayOn bit - OK*/
void test_pwr_cfg_stay_on_clear(void)
{
	i2c_reg_read_byte_mock_std_setup(0x1D, 0xFF);
	i2c_reg_write_byte_mock_std_setup(0x1D, 0xFE);
	i2c_reg_read_byte_mock_std_setup(0x1D, 0xFE);

	zassert_equal(max14690_boot_cfg_stay_on_cfg(PMIC_PWR_CFG_STAY_ON_DISABLED), RET_SUCCESS,
		      "Fail");
}

/* Set PFN bit - OK*/
void test_pwr_cfg_pfn_set(void)
{
	i2c_reg_read_byte_mock_std_setup(0x1D, 0x7F);
	i2c_reg_write_byte_mock_std_setup(0x1D, 0xFF);
	i2c_reg_read_byte_mock_std_setup(0x1D, 0xFF);

	zassert_equal(max14690_boot_cfg_pfn_cfg(PMIC_PWR_CFG_PFN_ENABLED), RET_SUCCESS, "Fail");
}

/* Clear PFN bit - OK*/
void test_pwr_cfg_pfn_clear(void)
{
	i2c_reg_read_byte_mock_std_setup(0x1D, 0x80);
	i2c_reg_write_byte_mock_std_setup(0x1D, 0x00);
	i2c_reg_read_byte_mock_std_setup(0x1D, 0x00);

	zassert_equal(max14690_boot_cfg_pfn_cfg(PMIC_PWR_CFG_PFN_DISABLED), RET_SUCCESS, "Fail");
}

void test_pwr_off(void)
{
	i2c_reg_write_byte_mock_std_setup(0x1F, 0xB2);
	i2c_reg_read_byte_mock_std_setup(0x1F, 0xB2);

	zassert_equal(max14690_pwr_off(), RET_SUCCESS, "Fail");
}

void test_main(void)
{
	ztest_test_suite(test_suite_max14690,
		ztest_unit_test(test_init_ok),
		ztest_unit_test(test_init_wrong_chip_id),
		ztest_unit_test(test_wrong_value_read_back),
		ztest_unit_test(test_buck_1_set_cfg_ind),
		ztest_unit_test(test_buck_1_set_cfg_mode_identical),
		ztest_unit_test(test_buck_1_set_cfg_mode),
		ztest_unit_test(test_buck_1_set_cfg_enable_identical),
		ztest_unit_test(test_buck_1_set_cfg_enable),
		ztest_unit_test(test_buck_1_set_valid_voltage_max),
		ztest_unit_test(test_buck_1_set_valid_voltage_min),
		ztest_unit_test(test_buck_1_set_valid_voltage_other),
		ztest_unit_test(test_buck_1_set_valid_voltage_too_low),
		ztest_unit_test(test_buck_1_set_valid_voltage_too_high),
		ztest_unit_test(test_buck_1_set_valid_voltage_illegal_val),
		ztest_unit_test(test_buck_2_set_cfg_ind),
		ztest_unit_test(test_buck_2_set_cfg_mode_identical),
		ztest_unit_test(test_buck_2_set_cfg_mode),
		ztest_unit_test(test_buck_2_set_cfg_enable_identical),
		ztest_unit_test(test_buck_2_set_cfg_enable),
		ztest_unit_test(test_buck_2_set_valid_voltage_max),
		ztest_unit_test(test_buck_2_set_valid_voltage_min),
		ztest_unit_test(test_buck_2_set_valid_voltage_other),
		ztest_unit_test(test_buck_2_set_valid_voltage_too_low),
		ztest_unit_test(test_buck_2_set_valid_voltage_too_high),
		ztest_unit_test(test_buck_2_set_valid_voltage_illegal_val),
		ztest_unit_test(test_ldo_1_cfg_mode_set),
		ztest_unit_test(test_ldo_1_cfg_enable_set),
		ztest_unit_test(test_ldo_1_cfg_dsc_set),
		ztest_unit_test(test_ldo_2_cfg_mode_set),
		ztest_unit_test(test_ldo_2_cfg_enable_set),
		ztest_unit_test(test_ldo_2_cfg_dsc_set),
		ztest_unit_test(test_ldo_3_cfg_mode_set),
		ztest_unit_test(test_ldo_3_cfg_enable_set),
		ztest_unit_test(test_ldo_3_cfg_dsc_set),
		ztest_unit_test(test_pwr_cfg_stay_on_set),
		ztest_unit_test(test_pwr_cfg_stay_on_clear),
		ztest_unit_test(test_pwr_cfg_pfn_set),
		ztest_unit_test(test_pwr_cfg_pfn_clear),
		ztest_unit_test(test_thrm_cfg_jeita_and_therm_set),
		ztest_unit_test(test_thrm_cfg_jeita_and_therm_invalid_value_set),
		ztest_unit_test(test_charge_tmr_set_cfg),
		ztest_unit_test(test_mon_cfg_ratio_set),
		ztest_unit_test(test_pwr_off)
	);

	ztest_run_test_suite(test_suite_max14690);
}
