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
#include <zephyr/audio/codec.h>
#include <zephyr/device.h>
#include <zephyr/drivers/audio/cs47l63.h>
#include <zephyr/shell/shell.h>
#include <zephyr/zbus/zbus.h>

#include "macros_common.h"
#include "zbus_common.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(hw_codec, CONFIG_MODULE_HW_CODEC_LOG_LEVEL);

#define VOLUME_ADJUST_STEP_DB 3
#define VOLUME_DEFAULT_DB     (-15)
#define VOLUME_MIN_DB	      (-64)
#define VOLUME_MAX_DB	      0
#define MAX_VOLUME_REG_VAL    128
#define BASE_10		      10
#define I2S_MCK_FREQ_HZ	      6144000

#if ((CONFIG_AUDIO_DEV == GATEWAY) && (CONFIG_AUDIO_SOURCE_I2S))
#define CODEC_INPUT (IS_ENABLED(CONFIG_WALKIE_TALKIE_DEMO) ? CS47L63_INPUT_PDM : CS47L63_INPUT_LINE)
#elif ((CONFIG_AUDIO_DEV == HEADSET) && CONFIG_STREAM_BIDIRECTIONAL)
#define CODEC_INPUT CS47L63_INPUT_PDM
#endif

#if defined(CODEC_INPUT)
#define CODEC_DIR AUDIO_DAI_DIR_TXRX
#else
#define CODEC_DIR AUDIO_DAI_DIR_TX
#endif

ZBUS_SUBSCRIBER_DEFINE(volume_evt_sub, CONFIG_VOLUME_MSG_SUB_QUEUE_SIZE);

static const struct device *const codec_dev = DEVICE_DT_GET(DT_NODELABEL(cs47l63));

static int volume_db = VOLUME_DEFAULT_DB;

static k_tid_t volume_msg_sub_thread_id;
static struct k_thread volume_msg_sub_thread_data;

K_THREAD_STACK_DEFINE(volume_msg_sub_thread_stack, CONFIG_VOLUME_MSG_SUB_STACK_SIZE);

/**
 * @brief	Convert the zbus volume to the actual volume setting for the HW codec.
 *
 * @note	The range for zbus volume is from 0 to 255 and the
 *		range for HW codec volume is from 0 to 128.
 */
static uint16_t zbus_vol_conversion(uint8_t volume)
{
	return (((uint16_t)volume + 1) / 2);
}

/**
 * @brief	Handle volume events from zbus.
 */
static void volume_msg_sub_thread(void)
{
	int ret;

	const struct zbus_channel *chan;

	while (1) {
		ret = zbus_sub_wait(&volume_evt_sub, &chan, K_FOREVER);
		ERR_CHK(ret);

		struct volume_msg msg;

		ret = zbus_chan_read(chan, &msg, ZBUS_READ_TIMEOUT_MS);
		if (ret) {
			LOG_ERR("Failed to read from zbus: %d", ret);
		}

		uint8_t event = msg.event;
		uint8_t volume = msg.volume;

		LOG_DBG("Received event = %d, volume = %d", event, volume);

		switch (event) {
		case VOLUME_UP:
			LOG_DBG("Volume up received");
			ret = hw_codec_volume_increase();
			if (ret) {
				LOG_ERR("Failed to increase volume, ret: %d", ret);
			}
			break;
		case VOLUME_DOWN:
			LOG_DBG("Volume down received");
			ret = hw_codec_volume_decrease();
			if (ret) {
				LOG_ERR("Failed to decrease volume, ret: %d", ret);
			}
			break;
		case VOLUME_SET:
			LOG_DBG("Volume set received");
			ret = hw_codec_volume_set(zbus_vol_conversion(volume));
			if (ret) {
				LOG_ERR("Failed to set the volume to %d, ret: %d", volume, ret);
			}
			break;
		case VOLUME_MUTE:
			LOG_DBG("Volume mute received");
			ret = hw_codec_volume_mute();
			if (ret) {
				LOG_ERR("Failed to mute volume, ret: %d", ret);
			}
			break;
		case VOLUME_UNMUTE:
			LOG_DBG("Volume unmute received");
			ret = hw_codec_volume_unmute();
			if (ret) {
				LOG_ERR("Failed to unmute volume, ret: %d", ret);
			}
			break;
		default:
			LOG_WRN("Unexpected/unhandled volume event: %d", event);
			break;
		}

		STACK_USAGE_PRINT("volume_msg_thread", &volume_msg_sub_thread_data);
	}
}

static int output_property_set(audio_property_t property, audio_property_value_t val)
{
	int ret;

	ret = audio_codec_set_property(codec_dev, property, AUDIO_CHANNEL_ALL, val);
	if (ret) {
		return ret;
	}

	return audio_codec_apply_properties(codec_dev);
}

static int volume_apply(int new_volume_db)
{
	int ret;

	ret = output_property_set(AUDIO_PROPERTY_OUTPUT_VOLUME,
				  (audio_property_value_t){.vol = new_volume_db});
	if (ret) {
		return ret;
	}

	ret = output_property_set(AUDIO_PROPERTY_OUTPUT_MUTE,
				  (audio_property_value_t){.mute = false});
	if (ret) {
		return ret;
	}

	volume_db = new_volume_db;

	LOG_DBG("Volume: %d dB", volume_db);

	return 0;
}

static void codec_error_cb(const struct device *dev, uint32_t errors)
{
	int ret;

	LOG_ERR("HW codec error: 0x%x", errors);

	ret = audio_codec_clear_errors(dev);
	if (ret) {
		LOG_ERR("Failed to clear HW codec errors: %d", ret);
	}
}

int hw_codec_volume_set(uint8_t set_val)
{
	uint32_t volume_reg_val;

	volume_reg_val = set_val;
	if (volume_reg_val == 0) {
		LOG_WRN("Volume at MIN (-64dB)");
	} else if (volume_reg_val >= MAX_VOLUME_REG_VAL) {
		LOG_WRN("Volume at MAX (0dB)");
		volume_reg_val = MAX_VOLUME_REG_VAL;
	}

	/* This is rounded down to nearest integer */
	return volume_apply((volume_reg_val / 2) + VOLUME_MIN_DB);
}

int hw_codec_volume_adjust(int8_t adjustment_db)
{
	int new_volume_db;

	LOG_DBG("Adj dB in: %d", adjustment_db);

	if (adjustment_db == 0) {
		return volume_apply(volume_db);
	}

	new_volume_db = volume_db + adjustment_db;
	if (new_volume_db <= VOLUME_MIN_DB) {
		LOG_WRN("Volume at MIN (-64dB)");
		new_volume_db = VOLUME_MIN_DB;
	} else if (new_volume_db >= VOLUME_MAX_DB) {
		LOG_WRN("Volume at MAX (0dB)");
		new_volume_db = VOLUME_MAX_DB;
	}

	return volume_apply(new_volume_db);
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
	return output_property_set(AUDIO_PROPERTY_OUTPUT_MUTE,
				   (audio_property_value_t){.mute = true});
}

int hw_codec_volume_unmute(void)
{
	return output_property_set(AUDIO_PROPERTY_OUTPUT_MUTE,
				   (audio_property_value_t){.mute = false});
}

int hw_codec_default_conf_enable(void)
{
	int ret;

	ret = audio_codec_route_output(codec_dev, AUDIO_CHANNEL_ALL, CS47L63_OUTPUT_HP);
	if (ret) {
		return ret;
	}

#if defined(CODEC_INPUT)
	ret = audio_codec_route_input(codec_dev, AUDIO_CHANNEL_ALL, CODEC_INPUT);
	if (ret) {
		return ret;
	}
#endif /* defined(CODEC_INPUT) */

	ret = hw_codec_volume_adjust(0);
	if (ret) {
		return ret;
	}

	return audio_codec_start(codec_dev, CODEC_DIR);
}

int hw_codec_soft_reset(void)
{
	return audio_codec_stop(codec_dev, CODEC_DIR);
}

int hw_codec_init(void)
{
	int ret;
	struct audio_codec_cfg cfg = {
		.mclk_freq = I2S_MCK_FREQ_HZ,
		.dai_type = AUDIO_DAI_TYPE_I2S,
		.dai_cfg.i2s.word_size = CONFIG_AUDIO_BIT_DEPTH_BITS,
		.dai_cfg.i2s.channels = CONFIG_I2S_CH_NUM,
		.dai_cfg.i2s.format = I2S_FMT_DATA_FORMAT_I2S,
		.dai_cfg.i2s.options = I2S_OPT_BIT_CLK_TARGET | I2S_OPT_FRAME_CLK_TARGET,
		.dai_cfg.i2s.frame_clk_freq = CONFIG_I2S_LRCK_FREQ_HZ,
		.dai_route = (CODEC_DIR & AUDIO_DAI_DIR_RX) ? AUDIO_ROUTE_PLAYBACK_CAPTURE
							    : AUDIO_ROUTE_PLAYBACK,
	};

	if (!device_is_ready(codec_dev)) {
		LOG_ERR("HW codec not ready");
		return -ENODEV;
	}

	ret = audio_codec_configure(codec_dev, &cfg);
	if (ret) {
		return ret;
	}

	ret = audio_codec_register_error_callback(codec_dev, codec_error_cb);
	if (ret) {
		return ret;
	}

	volume_msg_sub_thread_id = k_thread_create(
		&volume_msg_sub_thread_data, volume_msg_sub_thread_stack,
		CONFIG_VOLUME_MSG_SUB_STACK_SIZE, (k_thread_entry_t)volume_msg_sub_thread, NULL,
		NULL, NULL, K_PRIO_PREEMPT(CONFIG_VOLUME_MSG_SUB_THREAD_PRIO), 0, K_NO_WAIT);
	ret = k_thread_name_set(volume_msg_sub_thread_id, "Msg_sub_vol");
	ERR_CHK(ret);

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
	case LINE_IN:
		ret = audio_codec_route_input(codec_dev, AUDIO_CHANNEL_ALL, CS47L63_INPUT_LINE);
		if (ret) {
			shell_error(shell, "Failed to route LINE-IN to I2S");
			return ret;
		}

		shell_print(shell, "Selected LINE-IN as input");
		break;
	case PDM_MIC:
		ret = audio_codec_route_input(codec_dev, AUDIO_CHANNEL_ALL, CS47L63_INPUT_PDM);
		if (ret) {
			shell_error(shell, "Failed to route PDM mic to I2S");
			return ret;
		}

		shell_print(shell, "Selected PDM mic as input");
		break;
	default:
		shell_error(shell, "Invalid input");
		return -EINVAL;
	}

	return 0;
}

/**
 * @brief Shell command to choose between left, right, and mixed stereo as the I2S output
 *
 * @param shell Shell instance
 * @param argc Number of arguments
 * @param argv Array of arguments
 *
 * @return 0 on success, -EINVAL on error
 */
static int cmd_i2s_to_output_mapping(const struct shell *shell, size_t argc, char **argv)
{
	int ret;
	uint8_t idx;
	audio_channel_t channel;

	enum hw_codec_i2s_channel_to_mono_output {
		LEFT,
		RIGHT,
		MIXED_STEREO,
		NUM_OUTPUTS,
	};

	if (argc != 2) {
		shell_error(shell, "Only one argument required, provided: %d", argc);
		return -EINVAL;
	}

	if (!isdigit((int)argv[1][0])) {
		shell_error(shell, "Supplied argument is not numeric");
		return -EINVAL;
	}

	idx = strtoul(argv[1], NULL, BASE_10);

	switch (idx) {
	case LEFT:
		channel = AUDIO_CHANNEL_FRONT_LEFT;
		break;
	case RIGHT:
		channel = AUDIO_CHANNEL_FRONT_RIGHT;
		break;
	case MIXED_STEREO:
		channel = AUDIO_CHANNEL_ALL;
		break;
	default:
		shell_error(shell, "Invalid output");
		return -EINVAL;
	}

	ret = audio_codec_route_output(codec_dev, channel, CS47L63_OUTPUT_HP);
	if (ret) {
		shell_error(shell, "Failed to route I2S to output");
		return ret;
	}

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	hw_codec_cmd,
	SHELL_COND_CMD(CONFIG_SHELL, input, NULL, " Select input\n\t0: LINE_IN\n\t\t1: PDM_MIC",
		       cmd_input),
	SHELL_COND_CMD(CONFIG_SHELL, i2s_output, NULL,
		       " Select I2S output\n\t0: Left\n\t1: Right\n\t2: Mixed stereo",
		       cmd_i2s_to_output_mapping),
	SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(hw_codec, &hw_codec_cmd, "Change settings on HW codec", NULL);
