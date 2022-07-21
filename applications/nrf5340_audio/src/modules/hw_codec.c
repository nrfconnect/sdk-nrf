/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "hw_codec.h"

#include <zephyr/kernel.h>
#include <stdlib.h>
#include <stdint.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/device.h>

#include "macros_common.h"
#include "cs47l63.h"
#include "cs47l63_spec.h"
#include "cs47l63_reg_conf.h"
#include "cs47l63_comm.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(HW_CODEC, CONFIG_LOG_HW_CODEC_LEVEL);

#define VOLUME_ADJUST_STEP_DB 3
#define HW_CODEC_SELECT_DELAY_MS 2

static cs47l63_t cs47l63_driver;
static const struct gpio_dt_spec hw_codec_sel =
	GPIO_DT_SPEC_GET(DT_NODELABEL(hw_codec_sel_out), gpios);

/**@brief Write to multiple registers in CS47L63
 */
static int cs47l63_comm_reg_conf_write(const uint32_t config[][2], uint32_t num_of_regs)
{
	int ret;
	uint32_t reg;
	uint32_t value;

	for (int i = 0; i < num_of_regs; i++) {
		reg = config[i][0];
		value = config[i][1];

		if (reg == SPI_BUSY_WAIT) {
			LOG_DBG("Busy waiting instead of writing to CS47L63");
			/* Wait for us defined in value */
			k_busy_wait(value);
		} else {
			ret = cs47l63_write_reg(&cs47l63_driver, reg, value);
			if (ret) {
				return ret;
			}
		}
	}

	return 0;
}

/**@brief Select the on-board HW codec
 */
static int hw_codec_on_board_set(void)
{
	int ret;

	if (!device_is_ready(hw_codec_sel.port)) {
		return -ENXIO;
	}

	ret = gpio_pin_configure_dt(&hw_codec_sel, GPIO_OUTPUT_LOW);
	if (ret) {
		return ret;
	}

	/* Allow for switches to flip when selecting on board hw_codec */
	k_msleep(HW_CODEC_SELECT_DELAY_MS);

	return 0;
}

int hw_codec_volume_set(uint16_t set_val)
{
	int ret;
	uint16_t volume_reg_val;

	volume_reg_val = set_val;
	if (volume_reg_val == 0) {
		LOG_WRN("Volume at MIN (-64dB)");
	} else if (volume_reg_val >= MAX_VOLUME_REG_VAL) {
		LOG_WRN("Volume at MAX (0dB)");
		volume_reg_val = MAX_VOLUME_REG_VAL;
	}

	ret = cs47l63_write_reg(&cs47l63_driver, CS47L63_OUT1L_VOLUME_1,
				volume_reg_val | CS47L63_OUT_VU);
	if (ret) {
		return ret;
	}
	return 0;
}

int hw_codec_volume_adjust(int8_t adjustment)
{
	int ret;
	static uint8_t prev_volume_reg_val = OUT_VOLUME_DEFAULT;

	if (adjustment == 0) {
		ret = cs47l63_write_reg(&cs47l63_driver, CS47L63_OUT1L_VOLUME_1,
					(prev_volume_reg_val | CS47L63_OUT_VU) &
						~CS47L63_OUT1L_MUTE);
		if (ret) {
			return ret;
		}

		return 0;
	}

	/* Get adjustment in dB, 1 bit is 0.5 dB,
	 * so multiply by 2 to get increments of 1 dB
	 */
	adjustment *= 2;

	int16_t volume_reg_val;

	ret = cs47l63_read_reg(&cs47l63_driver, CS47L63_OUT1L_VOLUME_1,
			       (uint32_t *)&volume_reg_val);
	if (ret) {
		return ret;
	}

	volume_reg_val &= CS47L63_OUT1L_VOL_MASK;

	volume_reg_val += adjustment;
	if (volume_reg_val < 0) {
		LOG_WRN("Volume at MIN (-64dB)");
		volume_reg_val = 0;
	} else if (volume_reg_val > MAX_VOLUME_REG_VAL) {
		LOG_WRN("Volume at MAX (0dB)");
		volume_reg_val = MAX_VOLUME_REG_VAL;
	}

	ret = cs47l63_write_reg(&cs47l63_driver, CS47L63_OUT1L_VOLUME_1,
				(volume_reg_val | CS47L63_OUT_VU) & ~CS47L63_OUT1L_MUTE);
	if (ret) {
		return ret;
	}

	prev_volume_reg_val = volume_reg_val;
#if (CONFIG_LOG_HW_CODEC_LEVEL >= LOG_LEVEL_INF)
	int volume_in_db = (volume_reg_val / 2) - MAX_VOLUME_DB;

	/* This is rounded down to nearest integer */
	LOG_INF("Volume: %ddB", volume_in_db);
#endif /* (CONFIG_LOG_HW_CODEC_LEVEL >= LOG_LEVEL_INF) */

	return 0;
}

int hw_codec_volume_decrease(void)
{
	int ret;

	ret = hw_codec_volume_adjust(-VOLUME_ADJUST_STEP_DB);
	if (ret) {
		return ret;
	}

	return 0;
}

int hw_codec_volume_increase(void)
{
	int ret;

	ret = hw_codec_volume_adjust(VOLUME_ADJUST_STEP_DB);
	if (ret) {
		return ret;
	}

	return 0;
}

int hw_codec_volume_mute(void)
{
	int ret;
	uint16_t volume_reg_val;

	ret = cs47l63_read_reg(&cs47l63_driver, CS47L63_OUT1L_VOLUME_1,
			       (uint32_t *)&volume_reg_val);
	if (ret) {
		return ret;
	}

	BIT_SET(volume_reg_val, CS47L63_OUT1L_MUTE_MASK);

	ret = cs47l63_write_reg(&cs47l63_driver, CS47L63_OUT1L_VOLUME_1,
				volume_reg_val | CS47L63_OUT_VU);
	if (ret) {
		return ret;
	}

	return 0;
}

int hw_codec_volume_unmute(void)
{
	int ret;
	uint16_t volume_reg_val;

	ret = cs47l63_read_reg(&cs47l63_driver, CS47L63_OUT1L_VOLUME_1,
			       (uint32_t *)&volume_reg_val);
	if (ret) {
		return ret;
	}

	BIT_CLEAR(volume_reg_val, CS47L63_OUT1L_MUTE_MASK);

	ret = cs47l63_write_reg(&cs47l63_driver, CS47L63_OUT1L_VOLUME_1,
				volume_reg_val | CS47L63_OUT_VU);
	if (ret) {
		return ret;
	}

	return 0;
}

int hw_codec_default_conf_enable(void)
{
	int ret;

	ret = cs47l63_comm_reg_conf_write(clock_configuration, ARRAY_SIZE(clock_configuration));
	if (ret) {
		return ret;
	}

	ret = cs47l63_comm_reg_conf_write(GPIO_configuration, ARRAY_SIZE(GPIO_configuration));
	if (ret) {
		return ret;
	}

	ret = cs47l63_comm_reg_conf_write(asp1_enable, ARRAY_SIZE(asp1_enable));
	if (ret) {
		return ret;
	}

	ret = cs47l63_comm_reg_conf_write(output_enable, ARRAY_SIZE(output_enable));
	if (ret) {
		return ret;
	}

	ret = hw_codec_volume_adjust(0);
	if (ret) {
		return ret;
	}

#if ((CONFIG_AUDIO_DEV == GATEWAY) && (CONFIG_AUDIO_SOURCE_I2S))
	ret = cs47l63_comm_reg_conf_write(input_enable, ARRAY_SIZE(input_enable));
	if (ret) {
		return ret;
	}
#endif /* ((CONFIG_AUDIO_DEV == GATEWAY) && (CONFIG_AUDIO_SOURCE_I2S)) */

#if ((CONFIG_AUDIO_DEV == HEADSET) && CONFIG_STREAM_BIDIRECTIONAL)
	ret = cs47l63_comm_reg_conf_write(pdm_mic_enable_configure,
					  ARRAY_SIZE(pdm_mic_enable_configure));
	if (ret) {
		return ret;
	}
#endif /* ((CONFIG_AUDIO_DEV == HEADSET) && CONFIG_STREAM_BIDIRECTIONAL) */

	/* Toggle FLL to start up CS47L63 */
	ret = cs47l63_comm_reg_conf_write(FLL_toggle, ARRAY_SIZE(FLL_toggle));
	if (ret) {
		return ret;
	}

	return 0;
}

int hw_codec_soft_reset(void)
{
	int ret;

	ret = cs47l63_comm_reg_conf_write(output_disable, ARRAY_SIZE(output_disable));
	if (ret) {
		return ret;
	}

	ret = cs47l63_comm_reg_conf_write(soft_reset, ARRAY_SIZE(soft_reset));
	if (ret) {
		return ret;
	}

	return 0;
}

int hw_codec_init(void)
{
	int ret;

	/* Set to internal/on board codec */
	ret = hw_codec_on_board_set();
	if (ret) {
		return ret;
	}

	ret = cs47l63_comm_init(&cs47l63_driver);
	if (ret) {
		return ret;
	}

	/* Run a soft reset on start to make sure all registers are default values */
	ret = cs47l63_comm_reg_conf_write(soft_reset, ARRAY_SIZE(soft_reset));
	if (ret) {
		return ret;
	}
	cs47l63_driver.state = CS47L63_STATE_STANDBY;

	return 0;
}
