/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>

#include <bluetooth/services/fast_pair/fmdn.h>

#include "app_motion_detector.h"
#include "app_ui.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(fp_fmdn, LOG_LEVEL_INF);

static bool motion_detector_active;
static bool motion_detected;

static void fmdn_motion_detector_start(void)
{
	__ASSERT_NO_MSG(!motion_detector_active);
	LOG_INF("FMDN: motion detector started");
	motion_detector_active = true;
	app_ui_state_change_indicate(APP_UI_STATE_MOTION_DETECTOR_ACTIVE, motion_detector_active);
}

static bool fmdn_motion_detector_period_expired(void)
{
	bool ret = motion_detected;

	__ASSERT_NO_MSG(motion_detector_active);
	motion_detected = false;
	LOG_INF("FMDN: motion detector period expired. Reporting that the motion was %sdetected",
		ret ? "" : "not ");
	return ret;
}

static void fmdn_motion_detector_stop(void)
{
	__ASSERT_NO_MSG(motion_detector_active);
	LOG_INF("FMDN: motion detector stopped");
	motion_detector_active = false;
	app_ui_state_change_indicate(APP_UI_STATE_MOTION_DETECTOR_ACTIVE, motion_detector_active);
	motion_detected = false;
}

static const struct bt_fast_pair_fmdn_motion_detector_cb fmdn_motion_detector_cb = {
	.start = fmdn_motion_detector_start,
	.period_expired = fmdn_motion_detector_period_expired,
	.stop = fmdn_motion_detector_stop,
};

static void motion_detector_request_handle(enum app_ui_request request)
{
	if (request == APP_UI_REQUEST_MOTION_INDICATE) {
		if (motion_detector_active) {
			LOG_INF("FMDN: motion indicated");
			motion_detected = true;
		} else {
			LOG_INF("FMDN: motion indicated: motion detector is inactive");
		}
	}
}

int app_motion_detector_init(void)
{
	int err;

	err = bt_fast_pair_fmdn_motion_detector_cb_register(&fmdn_motion_detector_cb);
	if (err) {
		LOG_ERR("FMDN: bt_fast_pair_fmdn_motion_detector_cb_register failed (err %d)", err);
		return err;
	}

	return 0;
}

APP_UI_REQUEST_LISTENER_REGISTER(motion_detector_request_handler, motion_detector_request_handle);
