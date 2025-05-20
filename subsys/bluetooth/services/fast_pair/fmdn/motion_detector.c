/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(fp_fmdn_motion_detector, CONFIG_BT_FAST_PAIR_LOG_LEVEL);

#include <bluetooth/services/fast_pair/fast_pair.h>
#include <bluetooth/services/fast_pair/fmdn.h>
#include <dult.h>

#include "fp_fmdn_dult_integration.h"
#include "fp_activation.h"

static struct dult_motion_detector_cb dult_motion_detector_cb;
static bool dult_motion_detector_cb_registered;

int bt_fast_pair_fmdn_motion_detector_cb_register(
	const struct bt_fast_pair_fmdn_motion_detector_cb *cb)
{
	if (bt_fast_pair_is_ready()) {
		return -EOPNOTSUPP;
	}

	if (dult_motion_detector_cb_registered) {
		return -EALREADY;
	}

	if (!cb || !cb->start || !cb->period_expired || !cb->stop) {
		return -EINVAL;
	}

	dult_motion_detector_cb.start = cb->start;
	dult_motion_detector_cb.period_expired = cb->period_expired;
	dult_motion_detector_cb.stop = cb->stop;
	dult_motion_detector_cb_registered = true;

	return 0;
}

static int init(void)
{
	int err;
	const struct dult_user *user = fp_fmdn_dult_integration_user_get();

	__ASSERT_NO_MSG(user->accessory_capabilities &
			BIT(DULT_ACCESSORY_CAPABILITY_MOTION_DETECTOR_UT_BIT_POS));

	if (!dult_motion_detector_cb_registered) {
		LOG_ERR("FMDN: Motion detector callbacks not registered");
		return -EACCES;
	}

	err = dult_motion_detector_cb_register(user, &dult_motion_detector_cb);
	if (err) {
		LOG_ERR("FMDN: dult_motion_detector_cb_register returned error: %d", err);
		return err;
	}

	return 0;
}

static int uninit(void)
{
	return 0;
}

FP_ACTIVATION_MODULE_REGISTER(fp_fmdn_motion_detector,
			      FP_ACTIVATION_INIT_PRIORITY_DEFAULT,
			      init,
			      uninit);
