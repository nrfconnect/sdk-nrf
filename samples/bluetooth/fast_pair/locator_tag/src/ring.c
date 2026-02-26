/*
 * Copyright (c) 2024-2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>

#include <zephyr/kernel.h>

#include <bluetooth/fast_pair/fhn/fhn.h>

#include "app_ring.h"
#include "app_ui.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(fp_fhn, LOG_LEVEL_INF);

#define RING_COMP_STATE_STR_LEN (100U)

static uint8_t fhn_ring_active_comp_bm;
static enum bt_fast_pair_fhn_ring_src fhn_ring_src;

static void ring_state_update(struct bt_fast_pair_fhn_ring_state_param *param)
{
	int err;

	err = bt_fast_pair_fhn_ring_state_update(fhn_ring_src, param);
	if (err) {
		LOG_ERR("FHN: bt_fast_pair_fhn_ring_state_update failed (err %d)",
			err);
		return;
	}

	app_ui_state_change_indicate(
		APP_UI_STATE_RINGING,
		(param->active_comp_bm != BT_FAST_PAIR_FHN_RING_COMP_BM_NONE));

	fhn_ring_active_comp_bm = param->active_comp_bm;
}

static void ring_request_handle(enum app_ui_request request)
{
	struct bt_fast_pair_fhn_ring_state_param param = {0};

	/* It is assumed that the callback executes in the cooperative
	 * thread context as it interacts with the FHN API.
	 */
	__ASSERT_NO_MSG(!k_is_preempt_thread());
	__ASSERT_NO_MSG(!k_is_in_isr());

	if (request == APP_UI_REQUEST_RINGING_STOP) {
		if (fhn_ring_active_comp_bm == BT_FAST_PAIR_FHN_RING_COMP_BM_NONE) {
			return;
		}

		param.trigger = BT_FAST_PAIR_FHN_RING_TRIGGER_UI_STOPPED;
		param.active_comp_bm = BT_FAST_PAIR_FHN_RING_COMP_BM_NONE;

		ring_state_update(&param);

		LOG_INF("FHN: stopping the ringing action on button press");
	}
}

static void ring_provisioning_state_changed(bool provisioned)
{
	/* Clean up the ringing state in case it is necessary. */
	if (!provisioned && (fhn_ring_active_comp_bm != BT_FAST_PAIR_FHN_RING_COMP_BM_NONE)) {
		struct bt_fast_pair_fhn_ring_state_param param = {0};

		param.trigger = BT_FAST_PAIR_FHN_RING_TRIGGER_GATT_STOPPED;
		param.active_comp_bm = BT_FAST_PAIR_FHN_RING_COMP_BM_NONE;

		ring_state_update(&param);
	}
}

static struct bt_fast_pair_fhn_info_cb fhn_info_cb = {
	.provisioning_state_changed = ring_provisioning_state_changed,
};

static uint8_t ring_comp_bm_all_configured_get(void)
{
	uint8_t ring_comp_bm_all;

	BUILD_ASSERT(!IS_ENABLED(CONFIG_BT_FAST_PAIR_FHN_RING_COMP_NONE));

	if (IS_ENABLED(CONFIG_BT_FAST_PAIR_FHN_RING_COMP_ONE)) {
		/* Assuming that the single component configurations consists of the case. */
		ring_comp_bm_all = BT_FAST_PAIR_FHN_RING_COMP_CASE;
	} else if (IS_ENABLED(CONFIG_BT_FAST_PAIR_FHN_RING_COMP_TWO)) {
		/* Assuming that the two component configuration of left and right buds
		 * as indicated by the specification.
		 */
		ring_comp_bm_all =
			BT_FAST_PAIR_FHN_RING_COMP_RIGHT |
			BT_FAST_PAIR_FHN_RING_COMP_LEFT;
	} else if (IS_ENABLED(CONFIG_BT_FAST_PAIR_FHN_RING_COMP_THREE)) {
		ring_comp_bm_all =
			BT_FAST_PAIR_FHN_RING_COMP_RIGHT |
			BT_FAST_PAIR_FHN_RING_COMP_LEFT |
			BT_FAST_PAIR_FHN_RING_COMP_CASE;
	} else {
		/* Unsupported configuration. */
		__ASSERT_NO_MSG(0);
	}

	return ring_comp_bm_all;
}

static char *ring_comp_state_str_get(uint8_t active_comp_bm)
{
	int ret;
	static char ring_comp_state_str[RING_COMP_STATE_STR_LEN];

	BUILD_ASSERT(!IS_ENABLED(CONFIG_BT_FAST_PAIR_FHN_RING_COMP_NONE));

	if (IS_ENABLED(CONFIG_BT_FAST_PAIR_FHN_RING_COMP_ONE)) {
		/* Assuming that the single component configurations consists of the case. */
		ret = snprintf(ring_comp_state_str,
			       sizeof(ring_comp_state_str),
			       "Case=%sactive",
			       (active_comp_bm & BT_FAST_PAIR_FHN_RING_COMP_CASE) ? "" : "in");
	} else if (IS_ENABLED(CONFIG_BT_FAST_PAIR_FHN_RING_COMP_TWO)) {
		/* Assuming that the two component configuration of left and right buds
		 * as indicated by the specification.
		 */
		ret = snprintf(ring_comp_state_str,
			       sizeof(ring_comp_state_str),
			       "Right=%sactive, Left=%sactive",
			       (active_comp_bm & BT_FAST_PAIR_FHN_RING_COMP_RIGHT) ? "" : "in",
			       (active_comp_bm & BT_FAST_PAIR_FHN_RING_COMP_LEFT) ? "" : "in");
	} else if (IS_ENABLED(CONFIG_BT_FAST_PAIR_FHN_RING_COMP_THREE)) {
		ret = snprintf(ring_comp_state_str,
			       sizeof(ring_comp_state_str),
			       "Right=%sactive, Left=%sactive, Case=%sactive",
			       (active_comp_bm & BT_FAST_PAIR_FHN_RING_COMP_RIGHT) ? "" : "in",
			       (active_comp_bm & BT_FAST_PAIR_FHN_RING_COMP_LEFT) ? "" : "in",
			       (active_comp_bm & BT_FAST_PAIR_FHN_RING_COMP_CASE) ? "" : "in");
	} else {
		/* Unsupported configuration. */
		__ASSERT_NO_MSG(0);
	}

	if (ret < 0) {
		strcpy(ring_comp_state_str, "Unknown (snprintf encoding error)");
		LOG_ERR("FHN: snprintf failed (err %d)", ret);
	}

	if (ret >= sizeof(ring_comp_state_str)) {
		LOG_ERR("FHN: ring state string buffer too small");
	}

	return ring_comp_state_str;
}

static const char *ring_src_str_get(enum bt_fast_pair_fhn_ring_src src)
{
	static const char * const ring_src_description[] = {
		[BT_FAST_PAIR_FHN_RING_SRC_FHN_BT_GATT] = "Bluetooth GATT FHN",
		[BT_FAST_PAIR_FHN_RING_SRC_DULT_BT_GATT] = "Bluetooth GATT DULT",
		[BT_FAST_PAIR_FHN_RING_SRC_DULT_MOTION_DETECTOR] = "Motion Detector DULT",
	};

	__ASSERT((src < ARRAY_SIZE(ring_src_description)) && ring_src_description[src],
		 "Unknown ring source");

	return ring_src_description[src];
};

static void fhn_ring_start_request(
	enum bt_fast_pair_fhn_ring_src src,
	const struct bt_fast_pair_fhn_ring_req_param *ring_param)
{
	struct bt_fast_pair_fhn_ring_state_param param = {0};
	static const char * const volume_str[] = {
		[BT_FAST_PAIR_FHN_RING_VOLUME_DEFAULT] = "Default",
		[BT_FAST_PAIR_FHN_RING_VOLUME_LOW] = "Low",
		[BT_FAST_PAIR_FHN_RING_VOLUME_MEDIUM] = "Medium",
		[BT_FAST_PAIR_FHN_RING_VOLUME_HIGH] = "High",
	};
	uint8_t active_comp_bm =
		(ring_param->active_comp_bm == BT_FAST_PAIR_FHN_RING_COMP_BM_ALL) ?
		ring_comp_bm_all_configured_get() : ring_param->active_comp_bm;

	LOG_INF("FHN: starting ringing action with the following parameters:");
	LOG_INF("\tSource:\t\t%s", ring_src_str_get(src));
	LOG_INF("\tComponents:\t%s (BM=0x%02X)",
		ring_comp_state_str_get(active_comp_bm),
		ring_param->active_comp_bm);
	LOG_INF("\tTimeout:\t%d [ds]", ring_param->timeout);
	LOG_INF("\tVolume:\t\t%s (0x%02X)",
		(ring_param->volume < ARRAY_SIZE(volume_str)) ?
			volume_str[ring_param->volume] : "Unknown",
		ring_param->volume);

	param.trigger = BT_FAST_PAIR_FHN_RING_TRIGGER_STARTED;
	param.active_comp_bm = active_comp_bm;
	param.timeout = ring_param->timeout;

	/* In this sample, the application always accepts the new ring source
	 * regardless of the current active source.
	 */
	fhn_ring_src = src;

	ring_state_update(&param);
}

static void fhn_ring_timeout_expired(enum bt_fast_pair_fhn_ring_src src)
{
	struct bt_fast_pair_fhn_ring_state_param param = {0};

	/* The timeout source cannot differ from the last ring start operation that
	 * was accepted using the bt_fast_pair_fhn_ring_state_update function.
	 */
	__ASSERT_NO_MSG(src == fhn_ring_src);

	LOG_INF("FHN: stopping the ringing action on timeout");
	LOG_INF("\tSource:\t%s", ring_src_str_get(src));

	param.trigger = BT_FAST_PAIR_FHN_RING_TRIGGER_TIMEOUT_STOPPED;
	param.active_comp_bm = BT_FAST_PAIR_FHN_RING_COMP_BM_NONE;

	ring_state_update(&param);
}

static void fhn_ring_stop_request(enum bt_fast_pair_fhn_ring_src src)
{
	struct bt_fast_pair_fhn_ring_state_param param = {0};

	LOG_INF("FHN: stopping the ringing action on GATT request");
	LOG_INF("\tSource:\t%s", ring_src_str_get(src));

	param.trigger = BT_FAST_PAIR_FHN_RING_TRIGGER_GATT_STOPPED;
	param.active_comp_bm = BT_FAST_PAIR_FHN_RING_COMP_BM_NONE;

	/* In this sample, the application always accepts the new ring source
	 * regardless of the current active source.
	 */
	fhn_ring_src = src;

	ring_state_update(&param);
}

static const struct bt_fast_pair_fhn_ring_cb fhn_ring_cb = {
	.start_request = fhn_ring_start_request,
	.timeout_expired = fhn_ring_timeout_expired,
	.stop_request = fhn_ring_stop_request,
};

int app_ring_init(void)
{
	int err;

	err = bt_fast_pair_fhn_ring_cb_register(&fhn_ring_cb);
	if (err) {
		LOG_ERR("FHN: bt_fast_pair_fhn_ring_cb_register failed (err %d)", err);
		return err;
	}

	err = bt_fast_pair_fhn_info_cb_register(&fhn_info_cb);
	if (err) {
		LOG_ERR("Fast Pair: bt_fast_pair_fhn_info_cb_register failed (err %d)", err);
		return err;
	}

	return 0;
}

APP_UI_REQUEST_LISTENER_REGISTER(ring_request_handler, ring_request_handle);
