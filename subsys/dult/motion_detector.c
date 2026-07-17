/*
 * Copyright (c) 2024-2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdbool.h>
#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/random/random.h>

#include <dult/dult.h>
#include <dult/test.h>
#include "dult_user.h"
#include "dult_user_slot.h"
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

#define SEPARATED_UT_ACTIVE_POLL_DURATION	\
	K_SECONDS(CONFIG_DULT_MOTION_DETECTOR_SEPARATED_UT_ACTIVE_POLL_DURATION)
#define SEPARATED_UT_MAX_SOUND_COUNT		\
	CONFIG_DULT_MOTION_DETECTOR_SEPARATED_UT_MAX_SOUND_COUNT

BUILD_ASSERT(CONFIG_DULT_MOTION_DETECTOR_SEPARATED_UT_TIMEOUT_PERIOD_MAX >=
	     CONFIG_DULT_MOTION_DETECTOR_SEPARATED_UT_TIMEOUT_PERIOD_MIN);

/* Convert a Kconfig minute period to the internal uint32_t seconds. */
#define SEPARATED_UT_MIN_TO_SEC(_min) ((uint32_t)(_min) * SEC_PER_MIN)

/* Seconds ceiling in minutes; bounds the Kconfig values before conversion. */
#define SEPARATED_UT_PERIOD_MAX_MIN (DULT_TEST_MOTION_DETECTOR_PERIOD_MAX / SEC_PER_MIN)

/* Kconfig minute periods must be non-negative and fit the seconds ceiling. */
BUILD_ASSERT(IN_RANGE(CONFIG_DULT_MOTION_DETECTOR_SEPARATED_UT_BACKOFF_PERIOD,
		      0, SEPARATED_UT_PERIOD_MAX_MIN));
BUILD_ASSERT(IN_RANGE(CONFIG_DULT_MOTION_DETECTOR_SEPARATED_UT_TIMEOUT_PERIOD_MIN,
		      0, SEPARATED_UT_PERIOD_MAX_MIN));
BUILD_ASSERT(IN_RANGE(CONFIG_DULT_MOTION_DETECTOR_SEPARATED_UT_TIMEOUT_PERIOD_MAX,
		      0, SEPARATED_UT_PERIOD_MAX_MIN));

static uint32_t ut_backoff_period =
	SEPARATED_UT_MIN_TO_SEC(CONFIG_DULT_MOTION_DETECTOR_SEPARATED_UT_BACKOFF_PERIOD);
static uint32_t ut_timeout_period_min =
	SEPARATED_UT_MIN_TO_SEC(CONFIG_DULT_MOTION_DETECTOR_SEPARATED_UT_TIMEOUT_PERIOD_MIN);
static uint32_t ut_timeout_period_max =
	SEPARATED_UT_MIN_TO_SEC(CONFIG_DULT_MOTION_DETECTOR_SEPARATED_UT_TIMEOUT_PERIOD_MAX);

static bool is_enabled;
static const struct dult_motion_detector_sound_cb *sound_cb;

/* Per-user memory reference holding the user's motion detector callback. */
static size_t motion_detector_cb_id = DULT_USER_SLOT_MEM_REF_ID_UNSET;

/* Resolve the motion detector callback of the currently associated user. */
static const struct dult_motion_detector_cb *motion_detector_cb_get(void)
{
	void *ref = NULL;

	if (motion_detector_cb_id == DULT_USER_SLOT_MEM_REF_ID_UNSET) {
		return NULL;
	}

	(void) dult_user_slot_mem_ref_get(dult_user_get_associated(), motion_detector_cb_id, &ref);

	return ref;
}

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
	const struct dult_motion_detector_cb *motion_detector_cb = motion_detector_cb_get();
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
	ret = k_work_schedule(&motion_enable_work, K_SECONDS(ut_backoff_period));
	__ASSERT_NO_MSG(ret == 1);
}

static void motion_detector_stop(void)
{
	const struct dult_motion_detector_cb *motion_detector_cb = motion_detector_cb_get();

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
	const struct dult_motion_detector_cb *motion_detector_cb = motion_detector_cb_get();
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

static uint32_t randomized_timeout_calculate(void)
{
	uint32_t seed;
	uint32_t diff;
	uint64_t pick;
	int err;

	err = sys_csrand_get(&seed, sizeof(seed));
	if (err) {
		LOG_WRN("DULT: sys_csrand_get failed: %d", err);
		sys_rand_get(&seed, sizeof(seed));
	}

	__ASSERT_NO_MSG(ut_timeout_period_max >= ut_timeout_period_min);
	diff = ut_timeout_period_max - ut_timeout_period_min;

	/* Map the random seed onto the timeout window: scale the seed from its full
	 * [0, UINT32_MAX] range down to [0, diff], then offset by the minimum, to
	 * obtain a uniform sample in [ut_timeout_period_min, ut_timeout_period_max]
	 * seconds, inclusive. The uint64_t intermediate prevents overflow in
	 * diff * seed for any uint32_t diff.
	 */
	pick = ((uint64_t) diff * seed) / UINT32_MAX;

	return ut_timeout_period_min + (uint32_t) pick;
}

static void separated_mode_transition_handle(void)
{
	uint32_t separated_ut_timeout_period = randomized_timeout_calculate();
	int ret;

	LOG_DBG("Starting the work for enabling the motion detector. "
		"Randomized timeout set to: %" PRIu32 " seconds", separated_ut_timeout_period);
	__ASSERT_NO_MSG(!k_work_delayable_is_pending(&motion_enable_work));
	ret = k_work_schedule(&motion_enable_work, K_SECONDS(separated_ut_timeout_period));
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
	int err;
	void *ref = NULL;

	if (!dult_user_is_registered(user)) {
		return -EACCES;
	}

	if (!(user->accessory_capabilities &
	      BIT(DULT_ACCESSORY_CAPABILITY_MOTION_DETECTOR_UT_BIT_POS))) {
		LOG_ERR("DULT Motion Detector: motion detector capability must be declared to "
			"register callbacks");
		return -EINVAL;
	}

	if (!cb || !cb->start || !cb->period_expired || !cb->stop) {
		return -EINVAL;
	}

	if (motion_detector_cb_id == DULT_USER_SLOT_MEM_REF_ID_UNSET) {
		err = dult_user_slot_mem_ref_register(&motion_detector_cb_id);
		if (err) {
			return err;
		}
	}

	/* Stored per user, persists across dult_reset(); cleared by
	 * dult_user_unregister(). Registering twice for the same user is rejected.
	 */

	(void) dult_user_slot_mem_ref_get(user, motion_detector_cb_id, &ref);
	if (ref) {
		LOG_ERR("DULT Motion Detector: motion detector callbacks already registered");
		return -EALREADY;
	}

	return dult_user_slot_mem_ref_set(user, motion_detector_cb_id, (void *)cb);
}

int dult_motion_detector_enable(void)
{
	static bool dult_near_owner_state_cb_registered;

	if (is_enabled) {
		LOG_ERR("DULT Motion Detector: already enabled");
		return -EALREADY;
	}

	if (!motion_detector_cb_get()) {
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

	return 0;
}

int dult_test_motion_detector_separated_ut_period_set(
	const struct dult_test_motion_detector_separated_ut_period *data)
{
	if (!IS_ENABLED(CONFIG_DULT_MOTION_DETECTOR_TEST_MODE)) {
		LOG_ERR("DULT Motion Detector: separated UT period can only be set in test mode");
		return -ENOTSUP;
	}

	if (!data) {
		return -EINVAL;
	}

	if ((data->backoff > DULT_TEST_MOTION_DETECTOR_PERIOD_MAX) ||
	    (data->timeout_min > DULT_TEST_MOTION_DETECTOR_PERIOD_MAX) ||
	    (data->timeout_max > DULT_TEST_MOTION_DETECTOR_PERIOD_MAX)) {
		LOG_ERR("DULT Motion Detector: separated UT period: "
			"value exceeds %" PRIu32 " s cap (backoff=%" PRIu32 " s, "
			"timeout_min=%" PRIu32 " s, timeout_max=%" PRIu32 " s)",
			(uint32_t) DULT_TEST_MOTION_DETECTOR_PERIOD_MAX, data->backoff,
			data->timeout_min, data->timeout_max);
		return -EINVAL;
	}

	if (data->timeout_max < data->timeout_min) {
		LOG_ERR("DULT Motion Detector: separated UT period: "
			"timeout_max (%" PRIu32 " s) must be >= timeout_min (%" PRIu32 " s)",
			data->timeout_max, data->timeout_min);
		return -EINVAL;
	}

	ut_backoff_period = data->backoff;
	ut_timeout_period_min = data->timeout_min;
	ut_timeout_period_max = data->timeout_max;

	LOG_DBG("DULT Motion Detector: setting separated UT period: backoff=%" PRIu32 " s, "
		"timeout=[%" PRIu32 ", %" PRIu32 "] s",
		data->backoff, data->timeout_min, data->timeout_max);

	return 0;
}
