/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "hw_codec.h"

#include <zephyr/kernel.h>
#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/device.h>
#include <zephyr/shell/shell.h>

#include "macros_common.h"
#include "cs47l63.h"
#include "cs47l63_spec.h"
#include "cs47l63_reg_conf.h"
#include "cs47l63_comm.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(hw_codec, CONFIG_MODULE_HW_CODEC_LOG_LEVEL);

#define VOLUME_ADJUST_STEP_DB 3
#define HW_CODEC_SELECT_DELAY_MS 2
#define BASE_10 10

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

int hw_codec_volume_set(uint8_t set_val)
{
	int ret;
	uint32_t volume_reg_val;

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

int hw_codec_volume_adjust(int8_t adjustment_db)
{
	int ret;
	static uint32_t prev_volume_reg_val = OUT_VOLUME_DEFAULT;

	LOG_DBG("Adj dB in: %d", adjustment_db);

	if (adjustment_db == 0) {
		ret = cs47l63_write_reg(&cs47l63_driver, CS47L63_OUT1L_VOLUME_1,
				(prev_volume_reg_val | CS47L63_OUT_VU) & ~CS47L63_OUT1L_MUTE);
		return ret;
	}

	uint32_t volume_reg_val;

	ret = cs47l63_read_reg(&cs47l63_driver, CS47L63_OUT1L_VOLUME_1, &volume_reg_val);
	if (ret) {
		return ret;
	}

	volume_reg_val &= CS47L63_OUT1L_VOL_MASK;

	/* The adjustment is in dB, 1 bit equals 0.5 dB,
	 * so multiply by 2 to get increments of 1 dB
	 */
	int32_t new_volume_reg_val = volume_reg_val + (adjustment_db * 2);

	if (new_volume_reg_val < 0) {
		LOG_WRN("Volume at MIN (-64dB)");
		new_volume_reg_val = 0;

	} else if (new_volume_reg_val > MAX_VOLUME_REG_VAL) {
		LOG_WRN("Volume at MAX (0dB)");
		new_volume_reg_val = MAX_VOLUME_REG_VAL;

	}

	ret = cs47l63_write_reg(&cs47l63_driver, CS47L63_OUT1L_VOLUME_1,
		((uint32_t)new_volume_reg_val | CS47L63_OUT_VU) & ~CS47L63_OUT1L_MUTE);

	if (ret) {
		return ret;
	}

	prev_volume_reg_val = new_volume_reg_val;

	/* This is rounded down to nearest integer */
	LOG_DBG("Volume: %" PRId32 " dB", (new_volume_reg_val / 2) - MAX_VOLUME_DB);

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
	uint32_t volume_reg_val;

	ret = cs47l63_read_reg(&cs47l63_driver, CS47L63_OUT1L_VOLUME_1, &volume_reg_val);
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
	uint32_t volume_reg_val;

	ret = cs47l63_read_reg(&cs47l63_driver, CS47L63_OUT1L_VOLUME_1, &volume_reg_val);
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
	if (IS_ENABLED(CONFIG_WALKIE_TALKIE_DEMO)) {
		ret = cs47l63_comm_reg_conf_write(pdm_mic_enable_configure,
						  ARRAY_SIZE(pdm_mic_enable_configure));
		if (ret) {
			return ret;
		}
	} else {
		ret = cs47l63_comm_reg_conf_write(line_in_enable, ARRAY_SIZE(line_in_enable));
		if (ret) {
			return ret;
		}
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

static int cmd_input(const struct shell *shell, size_t argc, char **argv)
{
	int ret;
	uint8_t idx;

	enum hw_codec_input {
		LINE_IN,
		PDM_MIC,
		NUM_INPUTS,
	};

	if (argc != 2) {
		shell_error(shell, "Only one argument required, provided: %d", argc);
		return -EINVAL;
	}

	if ((CONFIG_AUDIO_DEV == GATEWAY) && IS_ENABLED(CONFIG_AUDIO_SOURCE_USB)) {
		shell_error(shell, "Can't select PDM mic if audio source is USB");
		return -EINVAL;
	}

	if ((CONFIG_AUDIO_DEV == HEADSET) && !IS_ENABLED(CONFIG_STREAM_BIDIRECTIONAL)) {
		shell_error(shell, "Can't select input if headset is not in bidirectional stream");
		return -EINVAL;
	}

	if (!isdigit((int)argv[1][0])) {
		shell_error(shell, "Supplied argument is not numeric");
		return -EINVAL;
	}

	idx = strtoul(argv[1], NULL, BASE_10);

	switch (idx) {
	case LINE_IN: {
		if (CONFIG_AUDIO_DEV == HEADSET) {
			ret = cs47l63_comm_reg_conf_write(line_in_enable,
							  ARRAY_SIZE(line_in_enable));
			if (ret) {
				shell_error(shell, "Failed to enable LINE-IN");
				return ret;
			}
		}

		ret = cs47l63_write_reg(&cs47l63_driver, CS47L63_ASP1TX1_INPUT1, 0x800012);
		if (ret) {
			shell_error(shell, "Failed to route LINE-IN to I2S");
			return ret;
		}

		ret = cs47l63_write_reg(&cs47l63_driver, CS47L63_ASP1TX2_INPUT1, 0x800013);
		if (ret) {
			shell_error(shell, "Failed to route LINE-IN to I2S");
			return ret;
		}

		shell_print(shell, "Selected LINE-IN as input");
		break;
	}
	case PDM_MIC: {
		if (CONFIG_AUDIO_DEV == GATEWAY) {
			ret = cs47l63_comm_reg_conf_write(pdm_mic_enable_configure,
							  ARRAY_SIZE(pdm_mic_enable_configure));
			if (ret) {
				shell_error(shell, "Failed to enable PDM mic");
				return ret;
			}
		}

		ret = cs47l63_write_reg(&cs47l63_driver, CS47L63_ASP1TX1_INPUT1, 0x800010);
		if (ret) {
			shell_error(shell, "Failed to route PDM mic to I2S");
			return ret;
		}

		ret = cs47l63_write_reg(&cs47l63_driver, CS47L63_ASP1TX2_INPUT1, 0x800011);
		if (ret) {
			shell_error(shell, "Failed to route PDM mic to I2S");
			return ret;
		}

		shell_print(shell, "Selected PDM mic as input");
		break;
	}
	default:
		shell_error(shell, "Invalid input");
		return -EINVAL;
	}

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(hw_codec_cmd,
			       SHELL_COND_CMD(CONFIG_SHELL, input, NULL,
					      " Select input\n\t0: LINE_IN\n\t\t1: PDM_MIC",
					      cmd_input),
			       SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(hw_codec, &hw_codec_cmd, "Change settings on HW codec", NULL);
