/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/util.h>
#include <adp536x.h>

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
#define ADP536X_BAT_CAP					0x20
#define ADP536X_BAT_SOC					0x21
#define ADP536X_VBAT_READ_H				0x25
#define ADP536X_VBAT_READ_L				0x26
#define ADP536X_FUEL_GAUGE_MODE				0x27
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

/* DEFAULT_SET register. */
#define ADP536X_DEFAULT_SET_MSK				GENMASK(7, 0)
#define ADP536X_DEFAULT_SET(x)				(((x) & 0xFF) << 0)

/* Fuel gauge configure register. */
#define ADP536X_FUEL_GAUGE_MODE_FG_MODE_MSK		BIT(1)
#define ADP536X_FUEL_GAUGE_MODE_FG_MODE(x)		(((x) & 0x01) << 1)
#define ADP536X_FUEL_GAUGE_EN_FG_MSK			BIT(0)
#define ADP536X_FUEL_GAUGE_EN_FG(x)			((x) & 0x01)

static const struct device *i2c_dev;

static int adp536x_reg_read(uint8_t reg, uint8_t *buff)
{
	return i2c_reg_read_byte(i2c_dev, ADP536X_I2C_ADDR, reg, buff);
}

static int adp536x_reg_write(uint8_t reg, uint8_t val)
{
	return i2c_reg_write_byte(i2c_dev, ADP536X_I2C_ADDR, reg, val);
}

static int adp536x_reg_write_mask(uint8_t reg_addr,
			       uint32_t mask,
			       uint8_t data)
{
	int err;
	uint8_t tmp;

	err = adp536x_reg_read(reg_addr, &tmp);
	if (err) {
		return err;
	}

	tmp &= ~mask;
	tmp |= data;

	return adp536x_reg_write(reg_addr, tmp);
}

int adp536x_charger_current_set(uint8_t value)
{
	return adp536x_reg_write_mask(ADP536X_CHG_CURRENT_SET,
					ADP536X_CHG_CURRENT_SET_ICHG_MSK,
					ADP536X_CHG_CURRENT_SET_ICHG(value));
}

int adp536x_vbus_current_set(uint8_t value)
{
	return adp536x_reg_write_mask(ADP536X_CHG_VBUS_ILIM,
					ADP536X_CHG_VBUS_ILIM_ILIM_MSK,
					ADP536X_CHG_VBUS_ILIM_ILIM(value));
}

int adp536x_charger_termination_voltage_set(uint8_t value)
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

int adp536x_charger_status_1_read(uint8_t *buf)
{
	return adp536x_reg_read(ADP536X_CHG_STATUS_1, buf);
}

int adp536x_charger_status_2_read(uint8_t *buf)
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

int adp536x_oc_chg_current_set(uint8_t value)
{
	return adp536x_reg_write_mask(ADP536X_BAT_OC_CHG,
					ADP536X_BAT_OC_CHG_OC_CHG_MSK,
					ADP536X_BAT_OC_CHG_OC_CHG(value));
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

int adp536x_init(const struct device *dev)
{
	if (!device_is_ready(dev)) {
		return -ENODEV;
	}

	i2c_dev = dev;

	return 0;
}

int adp536x_fg_set_mode(enum adp536x_fg_enabled en, enum adp536x_fg_mode mode)
{
	return adp536x_reg_write_mask(
		ADP536X_FUEL_GAUGE_MODE,
		ADP536X_FUEL_GAUGE_MODE_FG_MODE_MSK | ADP536X_FUEL_GAUGE_EN_FG_MSK,
		ADP536X_FUEL_GAUGE_MODE_FG_MODE(mode) | ADP536X_FUEL_GAUGE_EN_FG(en));
}

int adp536x_fg_soc(uint8_t *percentage)
{
	int err;
	uint8_t tmp;

	err = adp536x_reg_read(ADP536X_BAT_SOC, &tmp);
	if (err) {
		return err;
	}
	*percentage = tmp & 0x7f;

	return 0;
}

int adp536x_fg_volts(uint16_t *millivolts)
{
	int err;
	uint8_t msb, lsb;
	uint16_t v;

	err = adp536x_reg_read(ADP536X_VBAT_READ_H, &msb);
	if (err) {
		return err;
	}

	err = adp536x_reg_read(ADP536X_VBAT_READ_L, &lsb);
	if (err) {
		return err;
	}

	v = msb;
	v <<= 5;
	v |= (lsb >> 3);

	*millivolts = v;
	return 0;
}
