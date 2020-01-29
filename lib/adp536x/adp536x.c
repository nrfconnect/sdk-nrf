/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <device.h>
#include <drivers/i2c.h>
#include <sys/util.h>

#define ADP536X_I2C_ADDR				0x46

/* Register addresses */
#define ADP536X_MANUF_MODEL				0x00
#define ADP536X_SILICON_REV				0x01
#define ADP536X_CHG_VBUS_ILIM				0x02
#define ADP536X_CHG_TERM_SET				0x03
#define ADP536X_CHG_CURRENT_SET				0x04
#define ADP536X_CHG_V_THRESHOLD				0x05
#define ADP536X_CHG_TIMER_SET				0x06
#define ADP536X_CHG_FUNC				0x07
#define ADP536X_CHG_STATUS_1				0x08
#define ADP536X_CHG_STATUS_2				0x09
#define ADP536X_BAT_PROTECT_CTRL			0x11
#define ADP536X_BAT_OC_CHG				0x15
#define ADP536X_BUCK_CFG				0x29
#define ADP536X_BUCK_OUTPUT				0x2A
#define ADP536X_BUCKBST_CFG				0x2B
#define ADP536X_BUCKBST_OUTPUT				0x2C
#define ADP536X_DEFAULT_SET_REG				0x37

/* Manufacturer and model ID register. */
#define ADP536X_MANUF_MODEL_MANUF_MSK			GENMASK(7, 4)
#define ADP536X_MANUF_MODEL_MANUF(x)			(((x) & 0x0F) << 4)
#define ADP536X_MANUF_MODEL_MODEL_MSK			GENMASK(3, 0)
#define ADP536X_MANUF_MODEL_MODEL(x)			((x) & 0x0F)

/* Silicon revision register. */
#define ADP536X_SILICON_REV_REV_MSK			GENMASK(4, 0)
#define ADP536X_SILICON_REV_REV(x)			((x) & 0x0F)

/* Charger VBUS ILIM register. */
#define ADP536X_CHG_VBUS_ILIM_VADPICHG_MSK		GENMASK(7, 5)
#define ADP536X_CHG_VBUS_ILIM_VADPICHG(x)		(((x) & 0x07) << 5)
/* Bit 4 not used, according to datasheet. */
#define ADP536X_CHG_VBUS_ILIM_VSYSTEM_MSK		BIT(3)
#define ADP536X_CHG_VBUS_ILIM_VSYSTEM(x)		(((x) & 0x01) << 3)
#define ADP536X_CHG_VBUS_ILIM_ILIM_MSK			GENMASK(2, 0)
#define ADP536X_CHG_VBUS_ILIM_ILIM(x)			((x) & 0x07)

/* Charger termination settings register. */
#define ADP536X_CHG_TERM_SET_VTRM_MSK			GENMASK(7, 2)
#define ADP536X_CHG_TERM_SET_VTRM(x)			(((x) & 0x3F) << 2)
#define ADP536X_CHG_TERM_SET_ITRK_DEAD_MSK		GENMASK(1, 0)
#define ADP536X_CHG_TERM_SET_ITRK_DEAD(x)		((x) & 0x03)

/* Charger current setting register. */
#define ADP536X_CHG_CURRENT_SET_IEND_MSK		GENMASK(7, 5)
#define ADP536X_CHG_CURRENT_SET_IEND(x)			(((x) & 0x07) << 5)
#define ADP536X_CHG_CURRENT_SET_ICHG_MSK		GENMASK(4, 0)
#define ADP536X_CHG_CURRENT_SET_ICHG(x)			((x) & 0x1F)

/* Charger voltage threshold. */
#define ADP536X_CHG_V_THRESHOLD_DIS_RCH_MSK		BIT(7)
#define ADP536X_CHG_V_THRESHOLD_DIS_RCH(x)		(((x) & 0x01) << 7)
#define ADP536X_CHG_V_THRESHOLD_VRCH_MSK		GENMASK(6, 5)
#define ADP536X_CHG_V_THRESHOLD_VRCH(x)			(((x) & 0x03) << 5)
#define ADP536X_CHG_V_THRESHOLD_VTRK_DEAS_MSK		GENMASK(4, 3)
#define ADP536X_CHG_V_THRESHOLD_VTRK_DEAS(x)		(((x) & 0x03) << 3)
#define ADP536X_CHG_V_THRESHOLD_VWEAK_MSK		GENMASK(2, 0)
#define ADP536X_CHG_V_THRESHOLD_VWEAK(x)		((x) & 0x07)

/* Charger timer setting register. */
#define ADP536X_CHG_TIMER_SET_EN_TEND_MSK		BIT(3)
#define ADP536X_CHG_TIMER_SET_EN_TEND(x)		(((x) & 0x01) << 3)
#define ADP536X_CHG_TIMER_SET_EN_CHG_TIMER_MSK		BIT(2)
#define ADP536X_CHG_TIMER_SET_EN_CHG_TIMER(x)		(((x) & 0x01) << 2)
#define ADP536X_CHG_TIMER_SET_CHG_TMR_PERIOD_MSK	GENMASK(1, 0)
#define ADP536X_CHG_TIMER_SET_CHG_TMR_PERIOD(x)		((x) & 0x03)

/* Charger functional settings register. */
#define ADP536X_CHG_FUNC_EN_JEITA_MSK			BIT(7)
#define ADP536X_CHG_FUNC_EN_JEITA(x)			(((x) & 0x01) << 7)
#define ADP536X_CHG_FUNC_ILIM_JEITA_COOL_MSK		BIT(6)
#define ADP536X_CHG_FUNC_ILIM_JEAITA_COOL(x)		(((x) & 0x01) << 6)
/* Bit number 5 is unused according to datasheet.  */
#define ADP536X_CHG_FUNC_OFF_ISOFET_MSK			BIT(4)
#define ADP536X_CHG_FUNC_OFF_ISOFET(x)			(((x) & 0x01) << 4)
#define ADP536X_CHG_FUNC_EN_LDO_MSK			BIT(3)
#define ADP536X_CHG_FUNC_EN_LDO(x)			(((x) & 0x01) << 3)
#define ADP536X_CHG_FUNC_EN_EOC_MSK			BIT(2)
#define ADP536X_CHG_FUNC_EN_EOC(x)			(((x) & 0x01) << 2)
#define ADP536X_CHG_FUNC_EN_ADPICHG_MSK			BIT(1)
#define ADP536X_CHG_FUNC_EN_ADPICHG(x)			(((x) & 0x01) << 1)
#define ADP536X_CHG_FUNC_EN_CHG_MSK			BIT(0)
#define ADP536X_CHG_FUNC_EN_CHG(x)			((x) & 0x01)

/* Battery protection control register. */
#define ADP536X_BAT_PROTECT_CTRL_ISOFET_OVCHG_MSK	BIT(4)
#define ADP536X_BAT_PROTECT_CTRL_ISOFET_OVCHG(x)	(((x) & 0x01) << 4)
#define ADP536X_BAT_PROTECT_CTRL_OC_DIS_HICCUP_MSK	BIT(3)
#define ADP536X_BAT_PROTECT_CTRL_OC_DIS_HICCUP(x)	(((x) & 0x01) << 3)
#define ADP536X_BAT_PROTECT_CTRL_OC_CHG_HICCUP_MSK	BIT(2)
#define ADP536X_BAT_PROTECT_CTRL_OC_CHG_HICCUP(x)	(((x) & 0x01) << 2)
#define ADP536X_BAT_PROTECT_CTRL_EN_CHGLB_MSK		BIT(1)
#define ADP536X_BAT_PROTECT_CTRL_EN_CHGLB(x)		(((x) & 0x01) << 1)
#define ADP536X_BAT_PROTECT_CTRL_EN_BATPRO_MSK		BIT(0)
#define ADP536X_BAT_PROTECT_CTRL_EN_BATPRO(x)		((x) & 0x01)

#define ADP536X_BAT_OC_CHG_OC_CHG_MSK			GENMASK(7, 5)
#define ADP536X_BAT_OC_CHG_OC_CHG(x)			(((x) & 0x07) << 5)
#define ADP536X_BAT_OC_CHG_DGT_OC_CHG_MSK		GENMASK(4, 3)
#define ADP536X_BAT_OC_CHG_DGT_OC_CHG(x)		(((x) & 0x03) << 3)

/* Buck configure register. */
#define ADP536X_BUCK_CFG_DISCHG_BUCK_MSK		BIT(1)
#define ADP536X_BUCK_CFG_DISCHG_BUCK(x)			(((x) & 0x01) << 1)

/* Buck output voltage setting register. */
#define ADP536X_BUCK_OUTPUT_VOUT_BUCK_MSK		GENMASK(5, 0)
#define ADP536X_BUCK_OUTPUT_VOUT_BUCK(x)		(((x) & 0x3F) << 0)
#define ADP536X_BUCK_OUTPUT_BUCK_DLY_MSK		GENMASK(7, 6)
#define ADP536X_BUCK_OUTPUT_BUCK_DLY(x)			(((x) & 0x03) << 6)

/* Buck/boost output voltage setting register. */
#define ADP536X_BUCKBST_OUTPUT_VOUT_BUCKBST_MSK		GENMASK(5, 0)
#define ADP536X_BUCKBST_OUTPUT_VOUT_BUCKBST(x)		(((x) & 0x3F) << 0)
#define ADP536X_BUCKBST_OUT_BUCK_DLY_MSK		GENMASK(7, 6)
#define ADP536X_BUCKBST_OUT_BUCK_DLY(x)			(((x) & 0x03) << 6)

/* Buck/boost configure register. */
#define ADP536X_BUCKBST_CFG_EN_BUCKBST_MSK		BIT(0)
#define ADP536X_BUCKBST_CFG_EN_BUCKBST(x)		(((x) & 0x01) << 0)

/* DEFAULT_SET register. */
#define ADP536X_DEFAULT_SET_MSK				GENMASK(7, 0)
#define ADP536X_DEFAULT_SET(x)				(((x) & 0xFF) << 0)


static struct device *i2c_dev;

static int adp536x_reg_read(u8_t reg, u8_t *buff)
{
	return i2c_reg_read_byte(i2c_dev, ADP536X_I2C_ADDR, reg, buff);
}

static int adp536x_reg_write(u8_t reg, u8_t val)
{
	return i2c_reg_write_byte(i2c_dev, ADP536X_I2C_ADDR, reg, val);
}

static int adp536x_reg_write_mask(u8_t reg_addr,
			       u32_t mask,
			       u8_t data)
{
	int err;
	u8_t tmp;

	err = adp536x_reg_read(reg_addr, &tmp);
	if (err) {
		return err;
	}

	tmp &= ~mask;
	tmp |= data;

	return adp536x_reg_write(reg_addr, tmp);
}

int adp536x_charger_current_set(u8_t value)
{
	return adp536x_reg_write_mask(ADP536X_CHG_CURRENT_SET,
					ADP536X_CHG_CURRENT_SET_ICHG_MSK,
					ADP536X_CHG_CURRENT_SET_ICHG(value));
}

int adp536x_vbus_current_set(u8_t value)
{
	return adp536x_reg_write_mask(ADP536X_CHG_VBUS_ILIM,
					ADP536X_CHG_VBUS_ILIM_ILIM_MSK,
					ADP536X_CHG_VBUS_ILIM_ILIM(value));
}

int adp536x_charger_termination_voltage_set(u8_t value)
{
	return adp536x_reg_write_mask(ADP536X_CHG_TERM_SET,
					ADP536X_CHG_TERM_SET_VTRM_MSK,
					ADP536X_CHG_TERM_SET_VTRM(value));
}

int adp536x_charger_ldo_enable(bool enable)
{
	return adp536x_reg_write_mask(ADP536X_CHG_FUNC,
					ADP536X_CHG_FUNC_EN_LDO_MSK,
					ADP536X_CHG_FUNC_EN_LDO(enable));
}

int adp536x_charging_enable(bool enable)
{
	return adp536x_reg_write_mask(ADP536X_CHG_FUNC,
					ADP536X_CHG_FUNC_EN_CHG_MSK,
					ADP536X_CHG_FUNC_EN_CHG(enable));
}

int adp536x_charger_status_1_read(u8_t *buf)
{
	return adp536x_reg_read(ADP536X_CHG_STATUS_1, buf);
}

int adp536x_charger_status_2_read(u8_t *buf)
{
	return adp536x_reg_read(ADP536X_CHG_STATUS_2, buf);
}

int adp536x_oc_dis_hiccup_set(bool enable)
{
	return adp536x_reg_write_mask(ADP536X_BAT_PROTECT_CTRL,
				ADP536X_BAT_PROTECT_CTRL_OC_DIS_HICCUP_MSK,
				ADP536X_BAT_PROTECT_CTRL_OC_DIS_HICCUP(enable));
}

int adp536x_oc_chg_hiccup_set(bool enable)
{
	return adp536x_reg_write_mask(ADP536X_BAT_PROTECT_CTRL,
				ADP536X_BAT_PROTECT_CTRL_OC_CHG_HICCUP_MSK,
				ADP536X_BAT_PROTECT_CTRL_OC_CHG_HICCUP(enable));
}

int adp536x_oc_chg_current_set(u8_t value)
{
	return adp536x_reg_write_mask(ADP536X_BAT_OC_CHG,
					ADP536X_BAT_OC_CHG_OC_CHG_MSK,
					ADP536X_BAT_OC_CHG_OC_CHG(value));
}

int adp536x_buck_1v8_set(void)
{
	/* 1.8V equals to 0b11000 = 0x18 according to ADP536X datasheet. */
	u8_t value = 0x18;

	return adp536x_reg_write_mask(ADP536X_BUCK_OUTPUT,
					ADP536X_BUCK_OUTPUT_VOUT_BUCK_MSK,
					ADP536X_BUCK_OUTPUT_VOUT_BUCK(value));
}

int adp536x_buck_discharge_set(bool enable)
{
	return adp536x_reg_write_mask(ADP536X_BUCK_CFG,
				ADP536X_BUCK_CFG_DISCHG_BUCK_MSK,
				ADP536X_BUCK_CFG_DISCHG_BUCK(enable));
}

int adp536x_buckbst_3v3_set(void)
{
	/* 3.3V equals to 0b10011 = 0x13, according to ADP536X datasheet. */
	u8_t value = 0x13;

	return adp536x_reg_write_mask(ADP536X_BUCKBST_OUTPUT,
				ADP536X_BUCKBST_OUTPUT_VOUT_BUCKBST_MSK,
				ADP536X_BUCKBST_OUTPUT_VOUT_BUCKBST(value));
}

int adp536x_buckbst_enable(bool enable)
{
	return adp536x_reg_write_mask(ADP536X_BUCKBST_CFG,
					ADP536X_BUCKBST_CFG_EN_BUCKBST_MSK,
					ADP536X_BUCKBST_CFG_EN_BUCKBST(enable));
}

static int adp536x_default_set(void)
{
	/* The value 0x7F has to be written to this register to accomplish
	 * factory reset of the I2C registers, according to ADP536X datasheet.
	 */
	return adp536x_reg_write_mask(ADP536X_DEFAULT_SET_REG,
					ADP536X_DEFAULT_SET_MSK,
					ADP536X_DEFAULT_SET(0x7F));
}

int adp536x_factory_reset(void)
{
	int err;

	err = adp536x_default_set();
	if (err) {
		printk("adp536x_default_set failed: %d\n", err);
		return err;
	}

	return 0;
}

int adp536x_init(const char *dev_name)
{
	int err = 0;

	i2c_dev = device_get_binding(dev_name);
	if (err) {
		err = -ENODEV;
	}

	return err;
}
