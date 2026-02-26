/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/settings/settings.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(fp_fhn_clock, CONFIG_BT_FAST_PAIR_LOG_LEVEL);

#include <bluetooth/fast_pair/fast_pair.h>

#include "fp_activation.h"
#include "fp_fhn_clock.h"
#include "fp_storage_clock.h"

static uint32_t storage_clock_boot_checkpoint;

static void fhn_clock_storage_work_handle(struct k_work *work);

static K_WORK_DELAYABLE_DEFINE(fhn_clock_storage_work,
			       fhn_clock_storage_work_handle);

static bool is_enabled;

static uint32_t fhn_clock_read(void)
{
	int64_t sys_uptime;
	uint32_t fhn_clock;

	/* Calculate elapsed time since bootup. */
	sys_uptime = k_uptime_get();

	/* Convert from milliseconds to seconds. */
	sys_uptime /= MSEC_PER_SEC;

	/* Cast the uptime value in seconds onto the uint32_t type.
	 * The uint32_t type of clock value should allow over a hundred
	 * years of operation without the overflow:
	 * 2^32 / 3600 / 24 / 365 ~ 136 years
	 */
	fhn_clock = sys_uptime;

	/* Calculate the absolute time using the checkpoint from NVM. */
	fhn_clock += storage_clock_boot_checkpoint;

	return fhn_clock;
}

uint32_t fp_fhn_clock_read(void)
{
	uint32_t fhn_clock;

	__ASSERT_NO_MSG(is_enabled);

	fhn_clock = fhn_clock_read();

	LOG_DBG("FHN Clock: reading the last value: %u [s]", fhn_clock);

	return fhn_clock;
}

static void fhn_clock_storage_work_handle(struct k_work *work)
{
	int err;
	uint32_t fhn_clock;

	__ASSERT_NO_MSG(is_enabled);

	ARG_UNUSED(work);

	fhn_clock = fhn_clock_read();

	/* Set up the subsequent work handler for updating the clock value in NVM. */
	(void) k_work_reschedule(&fhn_clock_storage_work,
		K_MINUTES(CONFIG_BT_FAST_PAIR_FHN_CLOCK_NVM_UPDATE_TIME));

	err = fp_storage_clock_checkpoint_update(fhn_clock);
	if (err) {
		/* Retry the storage operation on error. */
		(void) k_work_reschedule(&fhn_clock_storage_work,
			K_SECONDS(CONFIG_BT_FAST_PAIR_FHN_CLOCK_NVM_UPDATE_RETRY_TIME));

		LOG_ERR("FHN Clock: fp_storage_clock_checkpoint_update failed: %d", err);
	} else {
		LOG_DBG("FHN Clock: storing the last value: %u [s]", fhn_clock);
	}
}

static int fp_fhn_clock_init(void)
{
	int err;
	uint32_t fhn_clock;
	uint32_t elapsed_time;
	uint32_t current_checkpoint = 0;
	static const uint32_t update_period =
		CONFIG_BT_FAST_PAIR_FHN_CLOCK_NVM_UPDATE_TIME * 60;

	if (is_enabled) {
		LOG_WRN("FHN Clock: module already initialized");
		return 0;
	}

	err = fp_storage_clock_boot_checkpoint_get(&storage_clock_boot_checkpoint);
	if (err) {
		LOG_ERR("FHN Clock: fp_storage_clock_boot_checkpoint_get failed: %d", err);
		return err;
	}

	err = fp_storage_clock_current_checkpoint_get(&current_checkpoint);
	if (err) {
		LOG_ERR("FHN Clock: fp_storage_clock_current_checkpoint_get failed: %d", err);
		return err;
	}
	__ASSERT_NO_MSG(current_checkpoint >= storage_clock_boot_checkpoint);

	/* Calculate the time from the last update. */
	fhn_clock = fhn_clock_read();
	__ASSERT_NO_MSG(fhn_clock >= current_checkpoint);

	/* Set up the first work handler for updating the clock value in NVM. */
	elapsed_time = (fhn_clock - current_checkpoint);
	if (elapsed_time < update_period) {
		const uint32_t update_time = (update_period - elapsed_time);

		(void) k_work_reschedule(&fhn_clock_storage_work, K_SECONDS(update_time));
	} else {
		(void) k_work_reschedule(&fhn_clock_storage_work, K_NO_WAIT);
	}

	is_enabled = true;

	return 0;
}

static int fp_fhn_clock_uninit(void)
{
	if (!is_enabled) {
		LOG_WRN("FHN Clock: module already uninitialized");
		return 0;
	}

	is_enabled = false;

	(void) k_work_cancel_delayable(&fhn_clock_storage_work);

	return 0;
}

/* The clock module requires clock storage module for initialization. */
BUILD_ASSERT(CONFIG_BT_FAST_PAIR_STORAGE_INTEGRATION_INIT_PRIORITY <
	     FP_ACTIVATION_INIT_PRIORITY_DEFAULT);
FP_ACTIVATION_MODULE_REGISTER(fp_fhn_clock,
			      FP_ACTIVATION_INIT_PRIORITY_DEFAULT,
			      fp_fhn_clock_init,
			      fp_fhn_clock_uninit);
