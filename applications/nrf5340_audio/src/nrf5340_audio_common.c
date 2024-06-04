/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <nrfx_clock.h>

#include "audio_system.h"
#include "bt_mgmt.h"
#include "channel_assignment.h"
#include "fw_info_app.h"
#include "nrf5340_audio_common.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(nrf5340_audio_common, CONFIG_NRF5340_AUDIO_COMMON_LOG_LEVEL);

int nrf5340_audio_common_init(void)
{
	int ret;

	/* Use this to turn on 128 MHz clock for cpu_app */
	ret = nrfx_clock_divider_set(NRF_CLOCK_DOMAIN_HFCLK, NRF_CLOCK_HFCLK_DIV_1);
	ret -= NRFX_ERROR_BASE_NUM;
	if (ret) {
		return ret;
	}

	ret = fw_info_app_print();
	if (ret) {
		LOG_ERR("Failed to print application FW info");
		return ret;
	}

	ret = bt_mgmt_init();
	if (ret) {
		LOG_ERR("Failed to initialize bt_mgmt");
		return ret;
	}

	ret = audio_system_init();
	if (ret) {
		LOG_ERR("Failed to initialize the audio system");
		return ret;
	}

	return 0;
}
