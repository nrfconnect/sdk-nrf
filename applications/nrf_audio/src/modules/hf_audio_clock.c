/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "hf_audio_clock.h"

#include <errno.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>
#include <nrfx_clock_hfclkaudio.h>
#include "audio_rate_control.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(rate_control, CONFIG_MODULE_RATE_CONTROL_LOG_LEVEL);

/* Use nanoseconds to reduce rounding errors */
/* clang-format off */
#define APLL_FREQ_ADJ(t) (-((t)*1000) / 331)
/* clang-format on */

static uint32_t current_freq;

int hf_audio_clock_update(void *const control_val_u, bool calibrate)
{
	int32_t err_us = *(int32_t *)control_val_u;
	int32_t freq_value = current_freq + APLL_FREQ_ADJ(err_us);

	if ((freq_value > (APLL_FREQ_MAX)) || (freq_value < (APLL_FREQ_MIN))) {
		if (calibrate) {
			return -EINVAL;
		}
	}

	freq_value = CLAMP(freq_value, APLL_FREQ_MIN, APLL_FREQ_MAX);

	if (calibrate) {
		current_freq = freq_value;
	}

	nrfx_clock_hfclkaudio_config_set(freq_value);

	return 0;
}

int hf_audio_clock_init(void)
{
	int ret;

	ret = nrfx_clock_divider_set(NRF_CLOCK_DOMAIN_HFCLK, NRF_CLOCK_HFCLK_DIV_1);
	if (ret) {
		return ret;
	}

	current_freq = APLL_FREQ_CENTER;

	nrfx_clock_hfclkaudio_config_set(current_freq);

	// ret = nrfx_clock_hfclkaudio_init(NULL);
	// if (ret != 0 && ret != -EALREADY) {
	// 	return ret;
	// }

	// nrfx_clock_hfclkaudio_start();

	return 0;
}

struct audio_rate_control_ops hf_audio_clock_rate_control_ops = {
	.update = hf_audio_clock_update,
	.initialize = hf_audio_clock_init,
};

static int hf_audio_clock_register(void)
{
	return audio_rate_control_register(AUDIO_PLL, &hf_audio_clock_rate_control_ops);
}

SYS_INIT(hf_audio_clock_register, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
