/*
 * Copyright (c) 2024-2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>

#include <bluetooth/fast_pair/fhn/fhn.h>

#include "app_motion_detector.h"
#include "app_ui.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(fp_fhn, LOG_LEVEL_INF);

static bool motion_detector_active;
static bool motion_detected;

static void fhn_motion_detector_start(void)
{
	__ASSERT_NO_MSG(!motion_detector_active);
	LOG_INF("FHN: motion detector started");
	motion_detector_active = true;
	app_ui_state_change_indicate(APP_UI_STATE_MOTION_DETECTOR_ACTIVE, motion_detector_active);
}

static bool fhn_motion_detector_period_expired(void)
{
	bool ret = motion_detected;

	__ASSERT_NO_MSG(motion_detector_active);
	motion_detected = false;
	LOG_INF("FHN: motion detector period expired. Reporting that the motion was %sdetected",
		ret ? "" : "not ");
	return ret;
}

static void fhn_motion_detector_stop(void)
{
	__ASSERT_NO_MSG(motion_detector_active);
	LOG_INF("FHN: motion detector stopped");
	motion_detector_active = false;
	app_ui_state_change_indicate(APP_UI_STATE_MOTION_DETECTOR_ACTIVE, motion_detector_active);
	motion_detected = false;
}

static const struct bt_fast_pair_fhn_motion_detector_cb fhn_motion_detector_cb = {
	.start = fhn_motion_detector_start,
	.period_expired = fhn_motion_detector_period_expired,
	.stop = fhn_motion_detector_stop,
};

static void motion_detector_request_handle(enum app_ui_request request)
{
	if (request == APP_UI_REQUEST_MOTION_INDICATE) {
		if (motion_detector_active) {
			LOG_INF("FHN: motion indicated");
			motion_detected = true;
		} else {
			LOG_INF("FHN: motion indicated: motion detector is inactive");
		}
	}
}

int app_motion_detector_init(void)
{
	int err;

	err = bt_fast_pair_fhn_motion_detector_cb_register(&fhn_motion_detector_cb);
	if (err) {
		LOG_ERR("FHN: bt_fast_pair_fhn_motion_detector_cb_register failed (err %d)", err);
		return err;
	}

	return 0;
}

APP_UI_REQUEST_LISTENER_REGISTER(motion_detector_request_handler, motion_detector_request_handle);
