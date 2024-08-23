/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>

#include <bluetooth/services/fast_pair/fmdn.h>

#include "app_motion_detector.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(fp_fmdn, LOG_LEVEL_INF);

static bool motion_detector_active;

static void fmdn_motion_detector_start(void)
{
	__ASSERT_NO_MSG(!motion_detector_active);
	LOG_INF("FMDN: motion detector started");
	motion_detector_active = true;
}

static bool fmdn_motion_detector_period_expired(void)
{
	/* This is a mock of motion detector - it reports that motion has been detected
	 * every other time. It needs to be properly implemented in the application.
	 */
	static bool motion_detected;

	__ASSERT_NO_MSG(motion_detector_active);
	motion_detected = !motion_detected;
	LOG_INF("FMDN: motion detector period expired. Reporting that the motion was %sdetected",
		motion_detected ? "" : "not ");
	return motion_detected;
}

static void fmdn_motion_detector_stop(void)
{
	__ASSERT_NO_MSG(motion_detector_active);
	LOG_INF("FMDN: motion detector stopped");
	motion_detector_active = false;
}

static const struct bt_fast_pair_fmdn_motion_detector_cb fmdn_motion_detector_cb = {
	.start = fmdn_motion_detector_start,
	.period_expired = fmdn_motion_detector_period_expired,
	.stop = fmdn_motion_detector_stop,
};

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
