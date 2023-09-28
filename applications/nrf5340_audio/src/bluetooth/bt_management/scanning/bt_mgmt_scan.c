/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "bt_mgmt.h"

#include <zephyr/bluetooth/bluetooth.h>

#include "bt_mgmt_scan_for_broadcast_internal.h"
#include "bt_mgmt_scan_for_conn_internal.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_mgmt_scan, CONFIG_BT_MGMT_SCAN_LOG_LEVEL);

static char srch_name[BLE_SEARCH_NAME_MAX_LEN];

static void addr_print(void)
{
	int ret;
	char addr_str[BT_ADDR_LE_STR_LEN];
	static struct bt_le_oob _oob = {.addr = 0};

	/* This call will create a new RPA if CONFIG_BT_PRIVACY is enabled */
	ret = bt_le_oob_get_local(BT_ID_DEFAULT, &_oob);
	if (ret) {
		LOG_ERR("bt_le_oob_get_local failed");
		return;
	}

	(void)bt_addr_le_to_str(&_oob.addr, addr_str, BT_ADDR_LE_STR_LEN);
	LOG_INF("Local addr: %s. May time out. Updates not printed", addr_str);
}

int bt_mgmt_scan_start(uint16_t scan_intvl, uint16_t scan_win, enum bt_mgmt_scan_type type,
		       char const *const name)
{
	int ret;

	static int scan_interval = CONFIG_BT_BACKGROUND_SCAN_INTERVAL;
	static int scan_window = CONFIG_BT_BACKGROUND_SCAN_WINDOW;

	/* Only change search name if a new name has been supplied */
	if (name != NULL) {
		size_t name_size = MIN(strlen(name), BLE_SEARCH_NAME_MAX_LEN - 1);

		memcpy(srch_name, name, name_size);
		srch_name[name_size] = '\0';
	}

	if (scan_intvl != 0) {
		scan_interval = scan_intvl;
	}

	if (scan_win != 0) {
		scan_window = scan_win;
	}

	struct bt_le_scan_param *scan_param =
		BT_LE_SCAN_PARAM(NRF5340_AUDIO_GATEWAY_SCAN_TYPE, BT_LE_SCAN_OPT_FILTER_DUPLICATE,
				 scan_interval, scan_window);

	if (type == BT_MGMT_SCAN_TYPE_CONN && IS_ENABLED(CONFIG_BT_CENTRAL)) {
		ret = bt_mgmt_scan_for_conn_start(scan_param, srch_name);
	} else if (type == BT_MGMT_SCAN_TYPE_BROADCAST &&
		   IS_ENABLED(CONFIG_BT_BAP_BROADCAST_SINK)) {
		ret = bt_mgmt_scan_for_broadcast_start(scan_param, srch_name);
	} else {
		LOG_WRN("Invalid scan type: %d, scan not started", type);
		return -EINVAL;
	}

	if (ret) {
		return ret;
	}

	/* As the scanning is started before addr_print, and due to host API limitations,
	 * this call will create a new RPA if CONFIG_BT_PRIVACY is enabled:
	 * There is a possibility that a device may connect using the previous RPA.
	 */
	addr_print();

	/* NOTE: The string below is used by the Nordic CI system */
	LOG_INF("Scanning successfully started");
	return 0;
}
