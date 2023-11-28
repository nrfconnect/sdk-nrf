/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "le_audio.h"

#include <zephyr/bluetooth/audio/bap.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(le_audio, CONFIG_BLE_LOG_LEVEL);

/*TODO: Create helper function in host to perform this action. */
bool le_audio_ep_state_check(struct bt_bap_ep *ep, enum bt_bap_ep_state state)
{
	int ret;
	struct bt_bap_ep_info ep_info;

	if (ep == NULL) {
		/* If an endpoint is NULL it is not in any of the states */
		return false;
	}

	ret = bt_bap_ep_get_info(ep, &ep_info);
	if (ret) {
		LOG_WRN("Unable to get info for ep");
		return false;
	}

	if (ep_info.state == state) {
		return true;
	}

	return false;
}
