/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "bt_mgmt.h"
#include "fw_info_app.h"
#include "audio_system.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(nrf5340_audio_common, CONFIG_NRF5340_AUDIO_COMMON_LOG_LEVEL);

int nrf5340_audio_common_init(void)
{
	int ret;

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
