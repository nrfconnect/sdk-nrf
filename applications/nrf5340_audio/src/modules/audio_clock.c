/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "audio_clock.h"

#include <errno.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>
#include <nrfx_clock_hfclkaudio.h>

int audio_clock_set(uint16_t freq_value)
{
	freq_value = CLAMP(freq_value, APLL_FREQ_MIN, APLL_FREQ_MAX);

#if NRF_CLOCK_HAS_HFCLKAUDIO
	nrfx_clock_hfclkaudio_config_set(freq_value);

#else
	return -ENOTSUP;
#endif /* NRF_CLOCK_HAS_HFCLKAUDIO */

	return 0;
}

int audio_clock_init(void)
{
	int ret;

#if NRF_CLOCK_HAS_HFCLKAUDIO
	ret = nrfx_clock_divider_set(NRF_CLOCK_DOMAIN_HFCLK, NRF_CLOCK_HFCLK_DIV_1);
	if (ret) {
		return ret;
	}

	ret = audio_clock_set(APLL_FREQ_CENTER);
	if (ret) {
		return ret;
	}

	NRF_CLOCK->TASKS_HFCLKAUDIOSTART = 1;

	/* Wait for ACLK to start */
	while (!NRF_CLOCK_EVENT_HFCLKAUDIOSTARTED) {
		k_sleep(K_MSEC(1));
	}
#else
	return -ENOTSUP;
#endif /* NRF_CLOCK_HAS_HFCLKAUDIO */

	return 0;
}
