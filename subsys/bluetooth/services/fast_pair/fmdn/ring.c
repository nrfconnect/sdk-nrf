/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(fp_fmdn_ring, CONFIG_BT_FAST_PAIR_LOG_LEVEL);

#include <dult.h>
#include <bluetooth/services/fast_pair/fast_pair.h>

#include "fp_activation.h"
#include "fp_fmdn_ring.h"
#include "fp_fmdn_dult_integration.h"

/* Maximum timeout for the ringing request in deciseconds (10 minutes). */
#define MAX_TIMEOUT_DSEC BT_FAST_PAIR_FMDN_RING_TIMEOUT_MS_TO_DS(10 * 60 * MSEC_PER_SEC)

static const struct bt_fast_pair_fmdn_ring_cb *user_cb;

static enum bt_fast_pair_fmdn_ring_src ring_src;
static uint8_t ring_active_comp_bm = BT_FAST_PAIR_FMDN_RING_COMP_BM_NONE;

static void fmdn_ring_timeout_handle(struct k_work *work);

static K_WORK_DELAYABLE_DEFINE(ring_timeout_work, fmdn_ring_timeout_handle);

static bool is_enabled;

static void fmdn_ring_timeout_handle(struct k_work *work)
{
	ARG_UNUSED(work);

	__ASSERT_NO_MSG(bt_fast_pair_is_ready());
	__ASSERT((user_cb && user_cb->timeout_expired),
		 "FMDN Ring: timeout callback is not populated.");
	__ASSERT_NO_MSG(ring_active_comp_bm != BT_FAST_PAIR_FMDN_RING_COMP_BM_NONE);

	user_cb->timeout_expired(ring_src);
}

static bool fmdn_ring_active_comp_bm_validate(uint8_t new_ring_active_comp_bm)
{
	uint8_t invalid_active_comp_bm_mask = BT_FAST_PAIR_FMDN_RING_COMP_BM_NONE;

	/* Validate the bitmask. */
	invalid_active_comp_bm_mask |= BT_FAST_PAIR_FMDN_RING_COMP_RIGHT;
	invalid_active_comp_bm_mask |= BT_FAST_PAIR_FMDN_RING_COMP_LEFT;
	invalid_active_comp_bm_mask |= BT_FAST_PAIR_FMDN_RING_COMP_CASE;
	invalid_active_comp_bm_mask = ~invalid_active_comp_bm_mask;

	if ((new_ring_active_comp_bm != BT_FAST_PAIR_FMDN_RING_COMP_BM_ALL) &&
	    ((new_ring_active_comp_bm & invalid_active_comp_bm_mask) != 0)) {
		return false;
	}

	return true;
}

int fp_fmdn_ring_req_handle(
	enum bt_fast_pair_fmdn_ring_src src,
	const struct bt_fast_pair_fmdn_ring_req_param *param)
{
	__ASSERT_NO_MSG(bt_fast_pair_is_ready());

	/* Validate the ringing capabilities. */
	if (IS_ENABLED(CONFIG_BT_FAST_PAIR_FMDN_RING_COMP_NONE)) {
		LOG_ERR("FMDN Ring: no support for ringing components");

		return -ENOTSUP;
	}

	/* Validate the registered callbacks. */
	__ASSERT((user_cb && user_cb->start_request && user_cb->stop_request),
		 "FMDN Ring: start or stop callback is not populated.");

	/* Validate the active ringing component bitmask. */
	if (!fmdn_ring_active_comp_bm_validate(param->active_comp_bm)) {
		LOG_ERR("FMDN Ring: request contains invalid ringing bitmask");

		return -EINVAL;
	}

	/* If the active ringing component bitmask is zero, the following bytes are ignored. */
	if (param->active_comp_bm != BT_FAST_PAIR_FMDN_RING_COMP_BM_NONE) {
		/* The timeout cannot be equal to zero or be greater than 10 minutes. */
		if ((param->timeout == 0) || (param->timeout > MAX_TIMEOUT_DSEC)) {
			LOG_ERR("FMDN Ring: request contains invalid timeout value");

			return -EINVAL;
		}

		/* Validate the ringing volume. */
		if ((param->volume < BT_FAST_PAIR_FMDN_RING_VOLUME_DEFAULT) ||
		    (param->volume > BT_FAST_PAIR_FMDN_RING_VOLUME_HIGH)) {
			LOG_ERR("FMDN Ring: request contains invalid volume value");

			return -EINVAL;
		}

		if (!IS_ENABLED(CONFIG_BT_FAST_PAIR_FMDN_RING_VOLUME) &&
		    (param->volume != BT_FAST_PAIR_FMDN_RING_VOLUME_DEFAULT)) {
			LOG_ERR("FMDN Ring: request contains unsupported volume selection");

			return -EINVAL;
		}
	}

	/* Trigger appropriate action after successful validation. */
	if (param->active_comp_bm == BT_FAST_PAIR_FMDN_RING_COMP_BM_NONE) {
		user_cb->stop_request(src);
	} else {
		user_cb->start_request(src, param);
	}

	return 0;
}

uint8_t fp_fmdn_ring_comp_num_encode(void)
{
	/* Define the encoding for the ringing components configuration. */
	enum ring_comp {
		RING_COMP_NONE  = 0x00,
		RING_COMP_ONE   = 0x01,
		RING_COMP_TWO   = 0x02,
		RING_COMP_THREE = 0x03,
	};

	__ASSERT_NO_MSG(bt_fast_pair_is_ready());

	if (IS_ENABLED(CONFIG_BT_FAST_PAIR_FMDN_RING_COMP_NONE)) {
		return RING_COMP_NONE;
	} else if (IS_ENABLED(CONFIG_BT_FAST_PAIR_FMDN_RING_COMP_ONE)) {
		return RING_COMP_ONE;
	} else if (IS_ENABLED(CONFIG_BT_FAST_PAIR_FMDN_RING_COMP_TWO)) {
		return RING_COMP_TWO;
	} else if (IS_ENABLED(CONFIG_BT_FAST_PAIR_FMDN_RING_COMP_THREE)) {
		return RING_COMP_THREE;
	}

	__ASSERT(0, "FMDN Ring: incorrect Ringing Components selection");

	return 0;
}

uint8_t fp_fmdn_ring_cap_encode(void)
{
	/* Define the encoding for the ringing capabilities configuration. */
	enum ring_cap {
		RING_CAP_NONE   = 0x00,
		RING_CAP_VOLUME = 0x01,
	};

	__ASSERT_NO_MSG(bt_fast_pair_is_ready());

	if (IS_ENABLED(CONFIG_BT_FAST_PAIR_FMDN_RING_VOLUME)) {
		return RING_CAP_VOLUME;
	}

	return RING_CAP_NONE;
}

uint8_t fp_fmdn_ring_active_comp_bm_get(void)
{
	__ASSERT_NO_MSG(bt_fast_pair_is_ready());

	return ring_active_comp_bm;
}

uint16_t fp_fmdn_ring_timeout_get(void)
{
	k_ticks_t rem_ticks;

	__ASSERT_NO_MSG(bt_fast_pair_is_ready());

	rem_ticks = k_work_delayable_remaining_get(&ring_timeout_work);

	/* Convert ticks to deciseconds. */
	return BT_FAST_PAIR_FMDN_RING_TIMEOUT_MS_TO_DS(k_ticks_to_ms_floor32(rem_ticks));
}

bool fp_fmdn_ring_is_active_comp_bm_supported(uint8_t active_comp_bm)
{
	size_t comp_num;

	__ASSERT_NO_MSG(bt_fast_pair_is_ready());

	if (active_comp_bm == BT_FAST_PAIR_FMDN_RING_COMP_BM_ALL) {
		return true;
	}

	/* Count the number of components. */
	comp_num = 0;
	comp_num += ((active_comp_bm & BT_FAST_PAIR_FMDN_RING_COMP_RIGHT) > 0);
	comp_num += ((active_comp_bm & BT_FAST_PAIR_FMDN_RING_COMP_LEFT) > 0);
	comp_num += ((active_comp_bm & BT_FAST_PAIR_FMDN_RING_COMP_CASE) > 0);

	return (comp_num <= fp_fmdn_ring_comp_num_encode());
}

int fp_fmdn_ring_state_param_set(
	enum bt_fast_pair_fmdn_ring_src src,
	const struct bt_fast_pair_fmdn_ring_state_param *param)
{
	__ASSERT_NO_MSG(bt_fast_pair_is_ready());

	if (IS_ENABLED(CONFIG_BT_FAST_PAIR_FMDN_RING_COMP_NONE)) {
		LOG_ERR("FMDN Ring: no ringing component declared in the configuration");

		return -ENOTSUP;
	}

	/* Disallow the all opcode in the set API. */
	if (param->active_comp_bm == BT_FAST_PAIR_FMDN_RING_COMP_BM_ALL) {
		LOG_ERR("FMDN Ring: all component bitmask value disallowed");

		return -EINVAL;
	}

	/* Validate the active ringing component bitmask. */
	if (!fmdn_ring_active_comp_bm_validate(param->active_comp_bm)) {
		LOG_ERR("FMDN Ring: invalid ringing component bitmask: 0x%02X",
			param->active_comp_bm);

		return -EINVAL;
	}

	/* Validate the timeout value. */
	if (param->timeout > MAX_TIMEOUT_DSEC) {
		LOG_ERR("FMDN Ring: invalid timeout value: %d [ds]",
			param->timeout);

		return -EINVAL;
	}

	/* Validate the trigger. */
	if ((param->trigger < BT_FAST_PAIR_FMDN_RING_TRIGGER_STARTED) ||
	    (param->trigger > BT_FAST_PAIR_FMDN_RING_TRIGGER_GATT_STOPPED)) {
		LOG_ERR("FMDN Ring: invalid state change trigger");

		return -EINVAL;
	}

	/* Reject the started trigger with zero timeout or no active components. */
	if ((param->trigger == BT_FAST_PAIR_FMDN_RING_TRIGGER_STARTED) &&
	    ((param->timeout == 0) ||
	     (param->active_comp_bm == BT_FAST_PAIR_FMDN_RING_COMP_BM_NONE))) {
		LOG_ERR("FMDN Ring: cannot use the start trigger with zero timeout"
			" or no active components");

		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_BT_FAST_PAIR_FMDN_DULT)) {
		int err;
		struct dult_sound_state_param dult_param = {0};
		const uint16_t dult_timeout_min = BT_FAST_PAIR_FMDN_RING_TIMEOUT_MS_TO_DS(
			DULT_SOUND_DURATION_MIN_MS);

		if ((src == BT_FAST_PAIR_FMDN_RING_SRC_DULT_BT_GATT) &&
		    (param->trigger == BT_FAST_PAIR_FMDN_RING_TRIGGER_STARTED) &&
		    (param->timeout < dult_timeout_min)) {
			LOG_ERR("FMDN Ring: timeout too low for DULT GATT source: "
				"on the ringing start operation: %d < %d [ds]",
				param->timeout, dult_timeout_min);

			return -EINVAL;
		}

		dult_param.src = (src == BT_FAST_PAIR_FMDN_RING_SRC_DULT_BT_GATT) ?
			DULT_SOUND_SRC_BT_GATT : DULT_SOUND_SRC_EXTERNAL;
		dult_param.active =
			(param->active_comp_bm != BT_FAST_PAIR_FMDN_RING_COMP_BM_NONE);

		err = dult_sound_state_update(fp_fmdn_dult_integration_user_get(),
					      &dult_param);
		__ASSERT(!err,
			"FMDN Ring: DULT user unregistered on sound state update");
	}

	ring_active_comp_bm = param->active_comp_bm;
	ring_src = src;

	if (param->active_comp_bm == BT_FAST_PAIR_FMDN_RING_COMP_BM_NONE) {
		(void) k_work_cancel_delayable(&ring_timeout_work);
	}

	if (param->timeout != 0) {
		if (param->trigger == BT_FAST_PAIR_FMDN_RING_TRIGGER_STARTED) {
			const uint16_t timeout_ms =
				BT_FAST_PAIR_FMDN_RING_TIMEOUT_DS_TO_MS(param->timeout);

			(void) k_work_reschedule(&ring_timeout_work, K_MSEC(timeout_ms));
		} else {
			LOG_WRN("FMDN Ring: ignoring timeout for triggers other than started");
		}
	}

	return 0;
}

static int fmdn_ring_cb_validate(const struct bt_fast_pair_fmdn_ring_cb *cb)
{
	if (IS_ENABLED(CONFIG_BT_FAST_PAIR_FMDN_RING_COMP_NONE)) {
		return 0;
	}

	if (!cb) {
		return -EINVAL;
	}

	/* All callbacks should be present if the device is capable of ringing. */
	if (!cb->start_request || !cb->stop_request || !cb->timeout_expired) {
		return -EINVAL;
	}

	return 0;
}

int bt_fast_pair_fmdn_ring_cb_register(const struct bt_fast_pair_fmdn_ring_cb *cb)
{
	int err;

	if (bt_fast_pair_is_ready()) {
		return -EOPNOTSUPP;
	}

	if (IS_ENABLED(CONFIG_BT_FAST_PAIR_FMDN_RING_COMP_NONE) && cb) {
		LOG_WRN("FMDN Ring: setting ringing callbacks has no effect"
			" when no ringing components are declared in Kconfig");

		return 0;
	}

	err = fmdn_ring_cb_validate(cb);
	if (err) {
		return err;
	}

	user_cb = cb;

	return 0;
}

static void dult_sound_start(enum dult_sound_src src)
{
	int err;
	struct bt_fast_pair_fmdn_ring_req_param dult_ring_req_param = {0};
	const uint16_t ring_timeout = CONFIG_BT_FAST_PAIR_FMDN_RING_REQ_TIMEOUT_DULT_BT_GATT;

	BUILD_ASSERT(CONFIG_BT_FAST_PAIR_FMDN_RING_REQ_TIMEOUT_DULT_BT_GATT >=
		     DULT_SOUND_DURATION_MIN_MS);

	/* Detecting Unwanted Location Trackers (DULT) specification:
	 * 3.12.4.1. Play sound:
	 * The sound maker MUST play sound for a minimum duration of 5 seconds.
	 *
	 * Guidelines specific to FMDN to be compliant with DULT spec:
	 * Guidelines for implementing the Sound_Start opcode:
	 *  * The command should trigger ringing in all available components.
	 *  * The maximal supported volume should be used.
	 */
	dult_ring_req_param.active_comp_bm = BT_FAST_PAIR_FMDN_RING_COMP_BM_ALL;
	dult_ring_req_param.timeout = BT_FAST_PAIR_FMDN_RING_TIMEOUT_MS_TO_DS(ring_timeout);
	dult_ring_req_param.volume = IS_ENABLED(CONFIG_BT_FAST_PAIR_FMDN_RING_VOLUME) ?
		BT_FAST_PAIR_FMDN_RING_VOLUME_HIGH : BT_FAST_PAIR_FMDN_RING_VOLUME_DEFAULT;

	err = fp_fmdn_ring_req_handle(BT_FAST_PAIR_FMDN_RING_SRC_DULT_BT_GATT,
				      &dult_ring_req_param);
	__ASSERT(!err,
		 "FMDN Ring: invalid translation from DULT Sound to FMDN Ring "
		 "parameters on the sound start action");
}

static void dult_sound_stop(enum dult_sound_src src)
{
	int err;
	struct bt_fast_pair_fmdn_ring_req_param dult_ring_req_param = {0};

	dult_ring_req_param.active_comp_bm = BT_FAST_PAIR_FMDN_RING_COMP_BM_NONE;

	err = fp_fmdn_ring_req_handle(BT_FAST_PAIR_FMDN_RING_SRC_DULT_BT_GATT,
				      &dult_ring_req_param);
	__ASSERT(!err,
		 "FMDN Ring: invalid translation from DULT Sound to FMDN Ring "
		 "parameters on the sound stop action");
}

static const struct dult_sound_cb dult_sound_cb = {
	.sound_start = dult_sound_start,
	.sound_stop = dult_sound_stop,
};

static int fp_fmdn_ring_dult_init(void)
{
	int err;

	if (!IS_ENABLED(CONFIG_BT_FAST_PAIR_FMDN_DULT)) {
		return 0;
	}

	err = dult_sound_cb_register(fp_fmdn_dult_integration_user_get(),
				     &dult_sound_cb);
	if (err) {
		LOG_ERR("FMDN Ring: dult_sound_cb_register returned error: %d", err);
		return err;
	}

	return 0;
}

static int fp_fmdn_ring_init(void)
{
	int err;

	if (is_enabled) {
		LOG_WRN("FMDN Ring: module already initialized");
		return 0;
	}

	if (!user_cb && !IS_ENABLED(CONFIG_BT_FAST_PAIR_FMDN_RING_COMP_NONE)) {
		LOG_ERR("FMDN Ring: no user callbacks during init");
		return -EINVAL;
	}

	err = fp_fmdn_ring_dult_init();
	if (err) {
		return err;
	}

	is_enabled = true;

	return 0;
}

static int fp_fmdn_ring_uninit(void)
{
	if (!is_enabled) {
		LOG_WRN("FMDN Ring: module already uninitialized");
		return 0;
	}

	is_enabled = false;

	ring_active_comp_bm = BT_FAST_PAIR_FMDN_RING_COMP_BM_NONE;

	(void) k_work_cancel_delayable(&ring_timeout_work);

	return 0;
}

FP_ACTIVATION_MODULE_REGISTER(fp_fmdn_ring,
			      FP_ACTIVATION_INIT_PRIORITY_DEFAULT,
			      fp_fmdn_ring_init,
			      fp_fmdn_ring_uninit);
