/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdbool.h>
#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/random/random.h>

#include "dult.h"
#include "dult_user.h"
#include "dult_motion_detector.h"
#include "dult_near_owner_state.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(dult_motion_detector, CONFIG_DULT_LOG_LEVEL);

/* Sampling rate in MOTION_POLL_STATE_PASSIVE state. */
#define SEPARATED_UT_SAMPLING_RATE1	\
	K_MSEC(CONFIG_DULT_MOTION_DETECTOR_SEPARATED_UT_SAMPLING_RATE1)
/* Sampling rate in MOTION_POLL_STATE_ACTIVE state. */
#define SEPARATED_UT_SAMPLING_RATE2	\
	K_MSEC(CONFIG_DULT_MOTION_DETECTOR_SEPARATED_UT_SAMPLING_RATE2)
#define SEPARATED_UT_BACKOFF_PERIOD	\
	K_MINUTES(CONFIG_DULT_MOTION_DETECTOR_SEPARATED_UT_BACKOFF_PERIOD)

#define SEPARATED_UT_TIMEOUT_PERIOD_MIN		\
	(CONFIG_DULT_MOTION_DETECTOR_SEPARATED_UT_TIMEOUT_PERIOD_MIN)
#define SEPARATED_UT_TIMEOUT_PERIOD_MAX		\
	(CONFIG_DULT_MOTION_DETECTOR_SEPARATED_UT_TIMEOUT_PERIOD_MAX)
#define SEPARATED_UT_TIMEOUT_PERIOD_DIFF	\
	(SEPARATED_UT_TIMEOUT_PERIOD_MAX - SEPARATED_UT_TIMEOUT_PERIOD_MIN)

#define SEPARATED_UT_ACTIVE_POLL_DURATION	\
	K_SECONDS(CONFIG_DULT_MOTION_DETECTOR_SEPARATED_UT_ACTIVE_POLL_DURATION)
#define SEPARATED_UT_MAX_SOUND_COUNT		\
	CONFIG_DULT_MOTION_DETECTOR_SEPARATED_UT_MAX_SOUND_COUNT

BUILD_ASSERT(SEPARATED_UT_TIMEOUT_PERIOD_DIFF >= 0);

static bool is_enabled;
static const struct dult_motion_detector_cb *motion_detector_cb;
static const struct dult_motion_detector_sound_cb *sound_cb;

static void motion_enable_work_handle(struct k_work *work);
static void motion_poll_work_handle(struct k_work *work);
static void motion_poll_duration_work_handle(struct k_work *work);

static K_WORK_DELAYABLE_DEFINE(motion_enable_work, motion_enable_work_handle);
static K_WORK_DELAYABLE_DEFINE(motion_poll_work, motion_poll_work_handle);
static K_WORK_DELAYABLE_DEFINE(motion_poll_duration_work, motion_poll_duration_work_handle);

enum motion_poll_state {
	MOTION_POLL_STATE_STOPPED,
	MOTION_POLL_STATE_PASSIVE,
	MOTION_POLL_STATE_PASSIVE_SOUND_REQUESTED,
	MOTION_POLL_STATE_ACTIVE,
	MOTION_POLL_STATE_ACTIVE_SOUND_REQUESTED,
};

static enum motion_poll_state poll_state = MOTION_POLL_STATE_STOPPED;
static uint8_t sound_count;

static void motion_enable_work_handle(struct k_work *work)
{
	int ret;

	LOG_DBG("Enabling the motion detector");

	__ASSERT(motion_detector_cb, "Motion detector callback structure is not registered");
	__ASSERT(motion_detector_cb->start, "Motion detector start callback is not populated");

	if (motion_detector_cb && motion_detector_cb->start) {
		motion_detector_cb->start();

		poll_state = MOTION_POLL_STATE_PASSIVE;
		__ASSERT_NO_MSG(!k_work_delayable_is_pending(&motion_poll_work));
		ret = k_work_schedule(&motion_poll_work, SEPARATED_UT_SAMPLING_RATE1);
		__ASSERT_NO_MSG(ret == 1);
	} else {
		LOG_ERR("Motion detector start callback is not populated");
	}
}

static void state_reset(void)
{
	int ret;

	poll_state = MOTION_POLL_STATE_STOPPED;
	sound_count = 0;

	/* The state_reset might be called directly from work and thus the calling work may still
	 * be running.
	 */
	ret = k_work_cancel_delayable(&motion_enable_work);
	__ASSERT_NO_MSG((!ret) || (ret == (K_WORK_RUNNING | K_WORK_CANCELING)));
	ret = k_work_cancel_delayable(&motion_poll_work);
	__ASSERT_NO_MSG((!ret) || (ret == (K_WORK_RUNNING | K_WORK_CANCELING)));
	ret = k_work_cancel_delayable(&motion_poll_duration_work);
	__ASSERT_NO_MSG((!ret) || (ret == (K_WORK_RUNNING | K_WORK_CANCELING)));
}

static void backoff_setup(void)
{
	int ret;

	LOG_DBG("Setting up motion detector backoff");

	state_reset();
	__ASSERT_NO_MSG(!k_work_delayable_is_pending(&motion_enable_work));
	ret = k_work_schedule(&motion_enable_work, SEPARATED_UT_BACKOFF_PERIOD);
	__ASSERT_NO_MSG(ret == 1);
}

static void motion_detector_stop(void)
{
	__ASSERT(motion_detector_cb, "Motion detector callback structure is not registered");
	__ASSERT(motion_detector_cb->stop, "Motion detector stop callback is not populated");

	if (motion_detector_cb && motion_detector_cb->stop) {
		motion_detector_cb->stop();
	} else {
		LOG_ERR("Motion detector stop callback is not populated");
	}
}

static void motion_poll_handle(void)
{
	bool motion_detected;

	__ASSERT(motion_detector_cb, "Motion detector callback structure is not registered");
	__ASSERT(motion_detector_cb->period_expired,
		 "Motion detector period_expired callback is not populated");

	if (!motion_detector_cb || !motion_detector_cb->period_expired) {
		LOG_ERR("Motion detector period_expired callback is not populated");
		return;
	}

	motion_detected = motion_detector_cb->period_expired();
	if (motion_detected) {
		/* Don't reschedule the work. It will be rescheduled after
		 * a sound playing action is finished.
		 */
		__ASSERT_NO_MSG(sound_cb);
		sound_cb->sound_start();
		poll_state = (poll_state == MOTION_POLL_STATE_PASSIVE) ?
			     MOTION_POLL_STATE_PASSIVE_SOUND_REQUESTED :
			     MOTION_POLL_STATE_ACTIVE_SOUND_REQUESTED;
		sound_count++;

		if (sound_count >= SEPARATED_UT_MAX_SOUND_COUNT) {
			LOG_DBG("Stopping the motion detector: %d sounds played",
				SEPARATED_UT_MAX_SOUND_COUNT);

			motion_detector_stop();
			backoff_setup();
		}
	} else {
		(void) k_work_reschedule(&motion_poll_work,
					 (poll_state == MOTION_POLL_STATE_PASSIVE) ?
					 SEPARATED_UT_SAMPLING_RATE1 : SEPARATED_UT_SAMPLING_RATE2);
	}
}

static void motion_poll_duration_work_handle(struct k_work *work)
{
	LOG_DBG("Stopping the motion detector: active poll duration timeout");

	motion_detector_stop();
	backoff_setup();
}

static void motion_poll_work_handle(struct k_work *work)
{
	if (poll_state == MOTION_POLL_STATE_PASSIVE) {
		LOG_DBG("Passive motion polling");

		motion_poll_handle();
	} else if (poll_state == MOTION_POLL_STATE_ACTIVE) {
		LOG_DBG("Active motion polling");

		motion_poll_handle();
	} else {
		__ASSERT(0, "Invalid motion polling state");
	}
}

static void separated_mode_transition_handle(void)
{
	int err;
	int ret;
	uint16_t separated_ut_timeout_period_seed;
	uint32_t separated_ut_timeout_period;

	/* Calculate the positive randomized time factor. */
	err = sys_csrand_get(&separated_ut_timeout_period_seed,
			     sizeof(separated_ut_timeout_period_seed));
	if (err) {
		LOG_WRN("FMDN State: sys_csrand_get failed: %d", err);

		sys_rand_get(&separated_ut_timeout_period_seed,
			     sizeof(separated_ut_timeout_period_seed));
	}

	BUILD_ASSERT(SEPARATED_UT_TIMEOUT_PERIOD_DIFF < UINT16_MAX);

	/* Convert the random part range from <0; UINT16_MAX> to
	 * <SEPARATED_UT_TIMEOUT_PERIOD_MIN; SEPARATED_UT_TIMEOUT_PERIOD_MAX>
	 */
	separated_ut_timeout_period = SEPARATED_UT_TIMEOUT_PERIOD_DIFF;
	separated_ut_timeout_period *= separated_ut_timeout_period_seed;
	separated_ut_timeout_period /= UINT16_MAX;
	separated_ut_timeout_period += SEPARATED_UT_TIMEOUT_PERIOD_MIN;

	LOG_DBG("Starting the work for enabling the motion detector. "
		"Randomized timeout set to: %" PRIu32 " minutes", separated_ut_timeout_period);
	__ASSERT_NO_MSG(!k_work_delayable_is_pending(&motion_enable_work));
	ret = k_work_schedule(&motion_enable_work, K_MINUTES(separated_ut_timeout_period));
	__ASSERT_NO_MSG(ret == 1);
}

static void near_owner_mode_transition_handle(void)
{
	if (poll_state != MOTION_POLL_STATE_STOPPED) {
		LOG_DBG("Stopping the motion detector: owner nearby");
		motion_detector_stop();
	} else {
		LOG_DBG("Motion detector is not running: owner nearby");
	}
	state_reset();
}

static void near_owner_state_changed(enum dult_near_owner_state_mode mode)
{
	if (!is_enabled) {
		return;
	}

	switch (mode) {
	case DULT_NEAR_OWNER_STATE_MODE_SEPARATED:
		separated_mode_transition_handle();
		break;

	case DULT_NEAR_OWNER_STATE_MODE_NEAR_OWNER:
		near_owner_mode_transition_handle();
		break;

	default:
		__ASSERT_NO_MSG(false);
		break;
	}
}

static struct dult_near_owner_state_cb near_owner_state_cb = {
	.state_changed = near_owner_state_changed,
};

static bool sound_requested(enum motion_poll_state state)
{
	return (state == MOTION_POLL_STATE_PASSIVE_SOUND_REQUESTED) ||
	       (state == MOTION_POLL_STATE_ACTIVE_SOUND_REQUESTED);
}

static void sound_completed_handle(void)
{
	if (sound_requested(poll_state)) {
		int ret;

		if (poll_state == MOTION_POLL_STATE_PASSIVE_SOUND_REQUESTED) {
			__ASSERT_NO_MSG(!k_work_delayable_is_pending(&motion_poll_duration_work));
			ret = k_work_schedule(&motion_poll_duration_work,
					      SEPARATED_UT_ACTIVE_POLL_DURATION);
			__ASSERT_NO_MSG(ret == 1);
		}

		__ASSERT_NO_MSG(!k_work_delayable_is_pending(&motion_poll_work));
		ret = k_work_schedule(&motion_poll_work, SEPARATED_UT_SAMPLING_RATE2);
		__ASSERT_NO_MSG(ret == 1);
		poll_state = MOTION_POLL_STATE_ACTIVE;
	}
}

void dult_motion_detector_sound_state_change_notify(bool active, bool native)
{
	if (!active) {
		sound_completed_handle();
	}
}

void dult_motion_detector_sound_cb_register(const struct dult_motion_detector_sound_cb *cb)
{
	__ASSERT(!sound_cb,
		 "DULT motion detector: sound callback already registered");
	__ASSERT(cb && cb->sound_start,
		 "DULT motion detector: input callback structure with invalid parameters");

	sound_cb = cb;
}

int dult_motion_detector_cb_register(const struct dult_user *user,
				     const struct dult_motion_detector_cb *cb)
{
	if (dult_user_is_ready()) {
		LOG_ERR("DULT Motion Detector: module must be disabled to register callbacks");
		return -EACCES;
	}

	if (!dult_user_is_registered(user)) {
		return -EACCES;
	}

	if (!(user->accessory_capabilities &
	      BIT(DULT_ACCESSORY_CAPABILITY_MOTION_DETECTOR_UT_BIT_POS))) {
		LOG_ERR("DULT Motion Detector: motion detector capability must be declared to "
			"register callbacks");
		return -EINVAL;
	}

	if (motion_detector_cb) {
		LOG_ERR("DULT Motion Detector: motion detector callbacks already registered");
		return -EALREADY;
	}

	if (!cb || !cb->start || !cb->period_expired || !cb->stop) {
		return -EINVAL;
	}

	motion_detector_cb = cb;

	return 0;
}

int dult_motion_detector_enable(void)
{
	static bool dult_near_owner_state_cb_registered;

	if (is_enabled) {
		LOG_ERR("DULT Motion Detector: already enabled");
		return -EALREADY;
	}

	if (!motion_detector_cb) {
		LOG_ERR("DULT Motion Detector: callbacks must be registered at this point");
		return -EINVAL;
	}

	if (!dult_near_owner_state_cb_registered) {
		dult_near_owner_state_cb_register(&near_owner_state_cb);
		dult_near_owner_state_cb_registered = true;
	}

	is_enabled = true;

	near_owner_state_changed(dult_near_owner_state_get());

	return 0;
}

int dult_motion_detector_reset(void)
{
	if (!is_enabled) {
		LOG_ERR("DULT Motion Detector: is not enabled");
		return -EALREADY;
	}

	is_enabled = false;

	if (poll_state != MOTION_POLL_STATE_STOPPED) {
		motion_detector_stop();
	}
	state_reset();

	motion_detector_cb = NULL;

	return 0;
}
