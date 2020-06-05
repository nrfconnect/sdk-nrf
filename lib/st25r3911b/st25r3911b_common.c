/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <kernel.h>
#include <sys/byteorder.h>
#include <device.h>
#include <soc.h>
#include <drivers/gpio.h>
#include <logging/log.h>

#include "st25r3911b_reg.h"
#include "st25r3911b_spi.h"
#include "st25r3911b_interrupt.h"
#include "st25r3911b_common.h"
#include "st25r3911b_dt.h"

LOG_MODULE_DECLARE(st25r3911b);

#define T_OSC_STABLE 10
#define T_POWER_SUPP_MEAS 100
#define T_CA_TIMEOUT 10
#define T_COMMON_CMD 10

#define POWER_SUPP_3V3_MAX_LVL 3600
#define POWER_SUPP_3V3_INIT_VAL 2400
#define POWER_SUPP_3V3_STEP 100
#define POWER_SUPP_5V_INIT_VAL 3900
#define POWER_SUPP_5V_STEP 120
#define POWER_SUPP_MEAS_INTEGER 23
#define POWER_SUPP_MEAS_FRACTION 438
#define POWER_SUPP_MEAS_FRACTION_DIVISOR 100
#define REGULATOR_INIT_REG_VALUE 5

#define FIELD_THRESHOLD_TRG_DEFAULT 0x30
#define FIELD_THRESHOLD_RFE_DEFAULT 0x03

#define FIFO_TX_WATER_LVL_32 32
#define FIFO_TX_WATER_LVL_16 16
#define FIFO_RX_WATER_LVL_80 80
#define FIFO_RX_WATER_LVL_64 64

#define FIFO_TX_WATER_16_EMPTY (ST25R3911B_MAX_FIFO_LEN - FIFO_TX_WATER_LVL_16)
#define FIFO_TX_WATER_32_EMPTY (ST25R3911B_MAX_FIFO_LEN - FIFO_TX_WATER_LVL_32)

#define NFCA_LED_PORT DT_GPIO_LABEL(ST25R3911B_NODE, led_nfca_gpios)
#define NFCA_LED_PIN DT_GPIO_PIN(ST25R3911B_NODE, led_nfca_gpios)

#define NFCB_LED_PORT DT_GPIO_LABEL(ST25R3911B_NODE, led_nfcb_gpios)
#define NFCB_LED_PIN DT_GPIO_PIN(ST25R3911B_NODE, led_nfcb_gpios)

#define NFCF_LED_PORT DT_GPIO_LABEL(ST25R3911B_NODE, led_nfcf_gpios)
#define NFCF_LED_PIN DT_GPIO_PIN(ST25R3911B_NODE, led_nfcf_gpios)


static struct device *gpio_dev;

static int command_process(u8_t cmd, u32_t *irq_mask, u32_t timeout)
{
	int err;
	u32_t status;

	err = st25r3911b_irq_enable(*irq_mask);
	if (err) {
		return err;
	}

	err = st25r3911b_cmd_execute(cmd);
	if (err) {
		st25r3911b_irq_disable(*irq_mask);
		return err;
	}

	status = st25r3911b_irq_wait_for_irq(*irq_mask,
					     timeout);

	err = st25r3911b_irq_disable(*irq_mask);
	if (err) {
		return err;
	}

	*irq_mask = status;

	if (!status) {
		return -ENXIO;
	}

	return 0;
}

static int osc_start(void)
{
	int err;
	u8_t reg = 0;
	u32_t status;

	err = st25r3911b_reg_read(ST25R3911B_REG_OP_CTRL, &reg);
	if (err) {
		return err;
	}

	if (!(reg & ST25R3911B_REG_OP_CTRL_EN)) {

		LOG_DBG("Oscilator is disabled, enabling it");

		err = st25r3911b_irq_enable(ST25R3911B_IRQ_MASK_OSC);
		if (err) {
			return err;
		}

		err = st25r3911b_reg_modify(ST25R3911B_REG_OP_CTRL,
					    0, ST25R3911B_REG_OP_CTRL_EN);
		if (err) {
			st25r3911b_irq_disable(ST25R3911B_IRQ_MASK_OSC);
			return err;
		}

		status = st25r3911b_irq_wait_for_irq(ST25R3911B_IRQ_MASK_OSC,
						     T_OSC_STABLE);

		err = st25r3911b_irq_disable(ST25R3911B_IRQ_MASK_OSC);
		if (err) {
			return err;
		}

		if (!status) {
			return -ENXIO;
		}

		LOG_DBG("Oscillator started");
	}

	return 0;
}

static int measure_voltage(u8_t source, u32_t *voltage)
{
	int err;
	u8_t reg;
	u32_t result;
	u32_t irq_mask;

	source &= ST25R3911B_REG_REGULATOR_CTRL_MPSV_MASK;

	/* Set measure source to VDD. */
	err = st25r3911b_reg_modify(ST25R3911B_REG_REGULATOR_CTRL,
				    ST25R3911B_REG_REGULATOR_CTRL_MPSV_MASK,
				    source);
	if (err) {
		return err;
	}

	irq_mask = ST25R3911B_IRQ_MASK_DCT;

	err = command_process(ST25R3911B_CMD_MEASURE_POWER_SUPPLY,
			      &irq_mask,
			      T_POWER_SUPP_MEAS);
	if (err) {
		return err;
	}

	LOG_DBG("Power supply voltage measurement");

	/* Get result */
	err = st25r3911b_reg_read(ST25R3911B_REG_AD_CONVERTER_OUT, &reg);
	if (err) {
		return err;
	}

	/* Calculate voltage level in mV */
	result = reg * POWER_SUPP_MEAS_INTEGER;
	result += ((reg * POWER_SUPP_MEAS_FRACTION) +
		    POWER_SUPP_MEAS_FRACTION_DIVISOR / 2) /
		    POWER_SUPP_MEAS_FRACTION_DIVISOR;

	*voltage = result;

	LOG_DBG("Measured supply voltage %u mV", result);

	return 0;
}

static int calibrate_antenna(void)
{
	int err;
	u8_t reg = 0;
	u32_t irq_mask;

	err = st25r3911b_reg_read(ST25R3911B_REG_ANTENNA_CAL_CTRL, &reg);
	if (err) {
		return err;
	}

	/* Check if is possible to automatic calibration. */
	if (reg & ST25R3911B_REG_ANTENNA_CAL_CTRL_TRIM_S) {
		LOG_ERR("Antenna auto calibration is not possible");
		return -EACCES;
	}

	LOG_DBG("Wait for antenna calibration %u ms", T_COMMON_CMD);

	irq_mask = ST25R3911B_IRQ_MASK_DCT;

	err = command_process(ST25R3911B_CMD_CALIBRATE_ANTENNA,
			      &irq_mask,
			      T_COMMON_CMD);
	if (err) {
		return err;
	}

	LOG_DBG("Antenna calibration success");

	return 0;
}

static int rx_tx_enable(void)
{
	u8_t mask = ST25R3911B_REG_OP_CTRL_TX_EN |
		    ST25R3911B_REG_OP_CTRL_RX_EN;

	return st25r3911b_reg_modify(ST25R3911B_REG_OP_CTRL, 0, mask);
}

static u32_t convert_voltage(u8_t voltage, bool is_5V)
{
	u32_t init_val = is_5V ? POWER_SUPP_5V_INIT_VAL : POWER_SUPP_3V3_INIT_VAL;
	u32_t step_val = is_5V ? POWER_SUPP_5V_STEP : POWER_SUPP_3V3_STEP;

	return init_val + voltage * step_val;
}

static int adjust_regulator(u32_t *mV_adjust)
{
	int err;
	u8_t reg;
	u8_t voltage = 0;
	u8_t conf = 0;
	u32_t mV;
	u32_t irq_mask;

	LOG_DBG("Regulator adjustment");

	err = st25r3911b_reg_read(ST25R3911B_REG_REGULATOR_CTRL, &reg);
	if (err) {
		return err;
	}

	/* Check possibility of auto calibration. This bit should be 0 */
	if (reg & ST25R3911B_REG_REGULATOR_CTRL_REG_S) {
		LOG_ERR("Regulator auto calibration is not possible");
		return -EACCES;
	}

	LOG_DBG("Wait for regulator adjust %u ms", T_COMMON_CMD);

	irq_mask = ST25R3911B_IRQ_MASK_DCT;

	err = command_process(ST25R3911B_CMD_ADJUST_REGULATOR,
			      &irq_mask,
			      T_COMMON_CMD);
	if (err) {
		return err;
	}

	/* Check voltage value in mV. */
	err = st25r3911b_reg_read(ST25R3911B_REG_REGULATOR_TIM_DISP, &voltage);
	if (err) {
		return err;
	}

	/* Read configuration reg to check power source 3V3 or 5V. */
	err = st25r3911b_reg_read(ST25R3911B_REG_IO_CONF2, &conf);
	if (err) {
		return err;
	}

	voltage >>= ST25R3911B_REG_REGULATOR_TIM_DISP_REG_POS;
	voltage -= REGULATOR_INIT_REG_VALUE;

	if (conf & ST25R3911B_REG_IO_CONF2_SUP3V) {
		mV = convert_voltage(voltage, false);

		LOG_DBG("Power supply 3.3V");

	} else {
		mV = convert_voltage(voltage, true);

		LOG_DBG("Power supply 5V");
	}

	LOG_DBG("Power supply voltage after adjustment %u mV", mV);

	*mV_adjust = mV;

	return 0;
}

int st25r3911b_rx_tx_disable(void)
{
	u8_t mask = ST25R3911B_REG_OP_CTRL_TX_EN |
		    ST25R3911B_REG_OP_CTRL_RX_EN;

	return st25r3911b_reg_modify(ST25R3911B_REG_OP_CTRL, mask, 0);
}

int st25r3911b_non_response_timer_set(u16_t fc, bool long_range, bool emv)
{
	int err;
	u8_t tim_control = 0;
	u8_t clr_mask = ST25R3911B_REG_TIM_CTRl_NRT_STEP |
			ST25R3911B_REG_TIM_CTRl_NRT_EMV;
	u8_t reg_data[2] = {0};

	if (emv) {
		tim_control |= ST25R3911B_REG_TIM_CTRl_NRT_EMV;
	}

	if (long_range) {
		tim_control |= ST25R3911B_REG_TIM_CTRl_NRT_STEP;
	}

	sys_put_be16(fc, reg_data);

	err = st25r3911b_reg_modify(ST25R3911B_REG_TIM_CTRl,
				    clr_mask, tim_control);
	if (err) {
		return err;
	}

	err = st25r3911b_reg_write(ST25R3911B_REG_NO_RSP_TIM_REG1, reg_data[0]);
	if (err) {
		return err;
	}

	return st25r3911b_reg_write(ST25R3911B_REG_NO_RSP_TIM_REG2,
				    reg_data[1]);
}

int st25r3911b_mask_receive_timer_set(u32_t fc)
{
	LOG_DBG("Set mask receive timer to %u fc", fc);
	return st25r3911b_reg_write(ST25R3911B_REG_MASK_RX_TIM,
				    ST25R3911B_FC_TO_64FC(fc));
}

int st25r3911b_init(void)
{
	int err;

	err = st25r3911b_spi_init();
	if (err) {
		return err;
	}

	/* Set default settings */
	err = st25r3911b_cmd_execute(ST25R3911B_CMD_SET_DEFAULT);
	if (err) {
		return err;
	}

	/* By default all interrupts are on, so disable them */
	err = st25r3911b_irq_disable(ST25R3911B_IRQ_MASK_ALL);
	if (err) {
		return err;
	}

	/* Clear all interrupts */
	err = st25r39_irq_clear();
	if (err) {
		return err;
	}

	/* Start oscillator and wait for it to be stable */
	err = osc_start();
	if (err) {
		LOG_ERR("Oscillator start failed");
		return err;
	}

	u32_t mV = 0;

	/* Measure a supply voltage to detect if device
	 * is powered by 5V or 3.3V.
	 */
	err = measure_voltage(ST25R3911B_REG_REGULATOR_CTRL_MPSV_VDD, &mV);
	if (err) {
		LOG_ERR("Voltage measure failed");
		return err;
	}

	/* Set power supply 5V or 3V3 */
	err = st25r3911b_reg_modify(ST25R3911B_REG_IO_CONF2,
				    ST25R3911B_REG_IO_CONF2_SUP3V,
				    (mV < POWER_SUPP_3V3_MAX_LVL) ?
				    ST25R3911B_REG_IO_CONF2_SUP3V : 0);
	if (err) {
		LOG_ERR("Power supply voltage level set failed");
		return err;
	}

	/* Adjust voltage regulator. */
	err = adjust_regulator(&mV);
	if (err) {
		LOG_ERR("Regulator adjust failed");
		return err;
	}

	/* Calibrate antenna, according to errata
	 * this should be always done twice
	 */
	err = calibrate_antenna();
	if (err) {
		LOG_ERR("Antenna calibration failed");
		return err;
	}

	err = calibrate_antenna();
	if (err) {
		LOG_ERR("Antenna calibration failed");
		return err;
	}

	/* Adjust regulator after antenna calibration */
	err = adjust_regulator(&mV);
	if (err) {
		LOG_ERR("Regulator adjust failed");
		return err;
	}

	/* Disable RX and TX mode */
	err = st25r3911b_rx_tx_disable();
	if (err) {
		return err;
	}

	/* Clear FIFO */
	return st25r3911b_cmd_execute(ST25R3911B_CMD_CLEAR);
}

int st25r3911b_tx_len_set(u16_t len)
{
	int err;
	u8_t msb_val;

	if (len > ST25R3911B_MAX_TX_LEN) {
		return -EFAULT;
	}

	msb_val = (len >> ST25R3911B_REG_NUM_TX_BYTES_NTX_SHIFT_LSB) & 0xFF;

	err = st25r3911b_reg_modify(ST25R3911B_REG_NUM_TX_BYTES_REG2,
				    ST25R3911B_REG_NUM_TX_BYTES_REG2_NTX_MASK,
				    (len << ST25R3911B_REG_NUM_TX_BYTES_NTX_SHIFT) &
				    ST25R3911B_REG_NUM_TX_BYTES_REG2_NTX_MASK);
	if (err) {
		return err;
	}

	err = st25r3911b_reg_write(ST25R3911B_REG_NUM_TX_BYTES_REG1,
				   msb_val);
	if (!err) {
		LOG_DBG("Fifo Tx length set to %u", len);
	}

	return err;
}

int st25r3911b_field_on(u8_t collision_threshold, u8_t peer_threshold,
			u8_t delay)
{
	int err;
	u32_t irq_mask;

	if (collision_threshold != ST25R3911B_NO_THRESHOLD_ANTICOLLISION) {
		err = st25r3911b_reg_modify(ST25R3911B_REG_FIELD_THRESHOLD,
					    ST25R3911B_REG_FIELD_THRESHOLD_RFE_MASK,
					    collision_threshold & ST25R3911B_REG_FIELD_THRESHOLD_RFE_MASK);
	} else {
		err = st25r3911b_reg_modify(ST25R3911B_REG_FIELD_THRESHOLD,
					    ST25R3911B_REG_FIELD_THRESHOLD_RFE_MASK,
					    FIELD_THRESHOLD_RFE_DEFAULT);
	}

	if (err) {
		return err;
	}

	if (peer_threshold != ST25R3911B_NO_THRESHOLD_ANTICOLLISION) {
		err = st25r3911b_reg_modify(ST25R3911B_REG_FIELD_THRESHOLD,
					    ST25R3911B_REG_FIELD_THRESHOLD_TRG_MASK,
					    peer_threshold << ST25R3911B_REG_FIELD_THRESHOLD_TRG_IO);
	} else {
		err = st25r3911b_reg_modify(ST25R3911B_REG_FIELD_THRESHOLD,
					    ST25R3911B_REG_FIELD_THRESHOLD_TRG_MASK,
					    FIELD_THRESHOLD_TRG_DEFAULT);
	}

	if (err) {
		return err;
	}

	irq_mask = ST25R3911B_IRQ_MASK_CAC | ST25R3911B_IRQ_MASK_CAT;

	LOG_DBG("Wait for field on");

	err = command_process(ST25R3911B_CMD_NFC_INITIAL_FIELD_ON,
			      &irq_mask,
			      T_CA_TIMEOUT);
	if (err) {
		return err;
	}

	if (irq_mask & ST25R3911B_IRQ_MASK_CAT) {
		/* Also enable Receiver */
		err = rx_tx_enable();
		if (err) {
			return err;
		} else {
			LOG_DBG("Field on, wait GT time %u ms", delay);

			/* Wait specific Guard Time when listener
			 * is exposedto an Unmodulated Carrier.
			 */
			k_sleep(K_MSEC(delay));

			return 0;
		}

	} else if (irq_mask & ST25R3911B_IRQ_MASK_CAC) {
		LOG_ERR("RF collision detected");
	} else {
		/* Do nothing */
	}

	return -EACCES;
}

int st25r3911b_technology_led_set(enum st25r3911b_leds led, bool on)
{
	int err;
	gpio_pin_t pin;
	const char *port;

	switch (led) {
#if DT_NODE_HAS_PROP(ST25R3911B_NODE, led_nfca_gpios)
	case ST25R3911B_NFCA_LED:
		pin = NFCA_LED_PIN;
		port = NFCA_LED_PORT;

		break;
#endif /* DT_NODE_HAS_PROP(ST25R3911B_NODE, led_nfca_gpios) */

#if DT_NODE_HAS_PROP(ST25R3911B_NODE, led_nfcb_gpios)
	case ST25R3911B_NFCB_LED:
		pin = NFCB_LED_PIN;
		port = NFCB_LED_PORT;

		break;
#endif /* DT_NODE_HAS_PROP(ST25R3911B_NODE, led_nfcb_gpios) */

#if DT_NODE_HAS_PROP(ST25R3911B_NODE, led_nfcf_gpios)
	case ST25R3911B_NFCF_LED:
		pin = NFCF_LED_PIN;
		port = NFCF_LED_PORT;

		break;
#endif /* DT_NODE_HAS_PROP(ST25R3911B_NODE, led_nfcf_gpios) */

	default:
		return -ENOTSUP;
	}

	gpio_dev = device_get_binding(port);
	if (!gpio_dev) {
		LOG_ERR("GPIO device binding error.");
		return -ENXIO;
	}

	err = gpio_pin_configure(gpio_dev, pin,
				 GPIO_OUTPUT |
				 GPIO_PULL_UP);
	if (err) {
		return err;
	}

	return gpio_pin_set_raw(gpio_dev, pin, on);
}

int st25r3911b_fifo_reload_lvl_get(u8_t *tx_lvl, u8_t *rx_lvl)
{
	int err;
	u8_t val;

	if ((!tx_lvl) || (!rx_lvl)) {
		return -EINVAL;
	}

	err = st25r3911b_reg_read(ST25R3911B_REG_IO_CONF1, &val);
	if (err) {
		return err;
	}

	*tx_lvl = (val & ST25R3911B_REG_IO_CONF1_FIFO_LT) ?
		   FIFO_TX_WATER_16_EMPTY : FIFO_TX_WATER_32_EMPTY;
	*rx_lvl = (val & ST25R3911B_REG_IO_CONF1_FIFO_LR) ?
		   FIFO_RX_WATER_LVL_80 : FIFO_RX_WATER_LVL_64;

	LOG_DBG("Fifo water level, Tx byte to reload %u, Rx byte to read %u",
		*tx_lvl, *rx_lvl);

	return 0;
}
