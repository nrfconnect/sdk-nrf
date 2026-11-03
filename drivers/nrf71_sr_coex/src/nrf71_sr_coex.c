/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief Host-side Coexistence Driver (CD) for nRF71 Wi-Fi / SR coexistence.
 *
 * The nRF71 chip has Wi-Fi and Short-Range radios that may share one antenna.
 * They must not transmit at the same time in a way that causes interference.
 * "Coexistence" is the logic that arbitrates who gets the antenna and when.
 *
 * This file implements the host-side Coexistence Driver (CD). It sits between:
 *   - Wi-Fi driver
 *   - SR driver     (BLE/other radio on the same SoC)
 *   - Coexistence Manager (CM) firmware running on the RPU
 *
 * Command flow (host -> RPU):
 *   CD builds a CD2CM_* message and sends it -> CM processes it
 *
 * Event flow (RPU -> host):
 *   CM sends CM2CD_* event -> coex_event_handler() queues the event header and
 *   wakes up whoever was waiting for that event.
 *
 * CM lifecycle:
 *   The Wi-Fi driver reports ADMIN_UP/ADMIN_DOWN through its state notifier.
 *   Every ADMIN_UP (boot, interface up, RPU recovery) re-applies the full CM
 *   configuration on the CD workqueue:
 *
 *     WIFI_DOWN   --ADMIN_UP-->   CONFIGURING
 *     CONFIGURING --ok-->         ACTIVE
 *     CONFIGURING --fail-->       RETRY_WAIT, back to CONFIGURING after
 *                                 CM_RETRY_INTERVAL_MS, up to CM_RETRY_MAX
 *                                 times, then FAILED
 *     any         --ADMIN_DOWN--> WIFI_DOWN
 *
 *   Each ADMIN_UP/ADMIN_DOWN bumps wifi_epoch. The apply work checks the epoch
 *   between CM transactions and drops its result if it changed, and ADMIN_DOWN
 *   wakes a transaction that is still waiting so it returns -ECANCELED instead
 *   of sitting on the CM2CD timeout.
 *
 * Locking rules (important when adding code):
 *   - cd_lock guards every cd_state field.
 *   - cd_cmd_lock serializes one CD2CM/CM2CD transaction at a time.
 *   - Lock order is cd_cmd_lock -> cd_lock. Never hold cd_lock while calling a
 *     coex_cm_* function. Take a local snapshot of the cd_state fields you need,
 *     release the lock, then send.
 *   - The Wi-Fi state callback runs in the Wi-Fi driver with its locks held. It
 *     only takes cd_lock and never blocks on a CM transaction.
 */

#include <errno.h>
#include <string.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>

/* Wi-Fi FMAC coexistence transport (cmd/event). */
#include <drivers/wifi/nrf71/nrf71_wifi_coex.h>
/* CD2CM / CM2CD message structures shared with RPU firmware. */
#include <nrf71_coex_if.h>
/* CD to Short-Range driver interface. */
#include <nrf71_cd_sr_if.h>

#include <drivers/nrf71_sr_coex/nrf71_sr_coex_sr.h>

#include "nrf71_sr_coex_internal.h"

LOG_MODULE_REGISTER(nrf71_sr_coex, CONFIG_NRF71_SR_COEX_DRIVER_LOG_LEVEL);

/* Max time (ms) to wait for a CM2CD response after posting a CD2CM command. */
#define CM2CD_EVENT_WAIT_MS CONFIG_NRF71_SR_COEX_CM_EVENT_WAIT_MS

#define CD_CM_RETRY_INTERVAL_MS CONFIG_NRF71_SR_COEX_CM_RETRY_INTERVAL_MS

/*
 * Number of CM2CD event headers kept between coex_event_handler() and the waiting
 * transaction. Transactions are serialized and each expects one reply, so a handful
 * of entries covers unsolicited or late events that arrive while one is waiting.
 */
#define CD_CM_EVENT_QUEUE_LEN 8U

/**
 * One received CM2CD event, as queued by coex_event_handler().
 *
 * The header is queued, not stored in a single slot, so two events that arrive
 * back to back are both seen by the waiter, in order.
 */
struct cd_cm_event {
	/** Event header as sent by the CM. */
	struct cm2cd_event_status_name_t hdr;
	/** Non-zero when this event carried a cm_stats_t payload. */
	uint8_t stats_valid;
	/** Non-zero for the token ADMIN_DOWN queues to wake a waiter. */
	uint8_t abort;
};

K_MSGQ_DEFINE(cd_cm_event_q, sizeof(struct cd_cm_event), CD_CM_EVENT_QUEUE_LEN, 4);

static K_MUTEX_DEFINE(cd_lock);
static K_MUTEX_DEFINE(cd_cmd_lock);

static K_THREAD_STACK_DEFINE(cd_wq_stack, CONFIG_NRF71_SR_COEX_WQ_STACK_SIZE);
static struct k_work_q cd_wq;

static void cd_apply_work_handler(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(cd_apply_work, cd_apply_work_handler);

/* Bumped on every Wi-Fi ADMIN_UP/ADMIN_DOWN, see the file header. */
static atomic_t wifi_epoch;

/*
 * Default Wi-Fi priority ranges sent to the CM at startup.
 *
 * Each range is {start, end, step}: start = numerically largest (lowest
 * priority), end = smallest (highest priority).
 */
#define CD_DEFAULT_WIFI_RANGE \
	{ \
		.sw_request_priority_range = {10, 5, 1}, \
		.client0_ccconf_pti_range = {20, 15, 1},   /* high-priority Wi-Fi Rx */ \
		.client1_ccconf_pti_range = {25, 20, 1},   /* high-priority Wi-Fi Tx */ \
		.client2_ccconf_pti_range = {140, 120, 3}, /* low-priority Wi-Fi Rx */ \
		.client3_ccconf_pti_range = {160, 145, 3}, /* low-priority Wi-Fi Tx */ \
		.hw_client_priority_level = 5, \
	}

/* Default SR priority ranges; same {start, end, step} convention. */
#define CD_DEFAULT_SR_RANGE \
	{ \
		.sr_rx_client_ccconf_pti_range = {30, 25, 1}, \
		.sr_tx_client_ccconf_pti_range = {40, 30, 2}, \
		.sr_rx_client_critical_ccconf_pti_range = {20, 15, 1}, \
		.sr_tx_client_critical_ccconf_pti_range = {25, 20, 1}, \
		.client_priority_level = 5, \
	}

/*
 * Default user coexistence parameters (protection probabilities, antenna mode).
 * Sent via CD2CM_UPDATE_COEX_USER_PARAMS. Values are 0-100 (percent).
 */
#define CD_DEFAULT_USER_PARAMS \
	{ \
		.message_id = CD2CM_UPDATE_COEX_USER_PARAMS, \
		.listen2inactive_sr_rx_prot_prob_ps = 100, \
		.inactive2listen_sr_rx_prot_prob_ps = 100, \
		.inactive2listen_sr_rx_prot_prob_calib = 100, \
		.listen2inactive_sr_rx_prot_prob_calib = 100, \
		.wifi_scan_puncture_info = {.wifi_scan_prot_prob = 100}, \
		.wifi_beacon_prot_prob = 100, \
		.wifi_conn_prot_prob = 100, \
		.wifi_calib_prot_prob = 100, \
		.shared_ant_control = ANT_ALLOC_STATIC_WIFI, \
	}

/** CM configuration state, see the file header. */
enum cd_wifi_state {
	CD_WIFI_DOWN,
	CD_WIFI_CONFIGURING,
	CD_WIFI_RETRY_WAIT,
	CD_WIFI_ACTIVE,
	CD_WIFI_FAILED,
};

/** All driver state in one place, guarded by cd_lock. */
static struct {
	enum cd_wifi_state wifi_state;
	unsigned int retries_left;

	bool sr_transition_active; /* Guard against overlapping SR power notifications. */

	struct coex_wifi_priority_range_t wifi_range;
	struct coex_sr_priority_range_t sr_range;
	struct coex_user_params_t user_params;

	bool stats_valid;          /* last_stats filled from CM2CD_STATISTICS_EVENT. */
	bool patch_stats_valid;    /* last_patch_stats filled from same event. */
	struct cm_stats_t last_stats;                 /* ROM CM statistics payload. */
	struct cm_fsm_patch_stats_t last_patch_stats; /* Patch CM statistics payload. */
} cd_state = {
	.wifi_state = CD_WIFI_DOWN,
	.wifi_range = CD_DEFAULT_WIFI_RANGE,
	.sr_range = CD_DEFAULT_SR_RANGE,
	.user_params = CD_DEFAULT_USER_PARAMS,
};

/* ---- Short-Range driver callbacks ---- */

/* The SR driver may register before this driver's SYS_INIT runs. */
static atomic_ptr_t sr_driver_cb = ATOMIC_PTR_INIT(NULL);

int nrf71_sr_coex_sr_register(const struct nrf71_sr_coex_sr_driver_cb *cb)
{
	if (cb == NULL) {
		return -EINVAL;
	}

	if (!atomic_ptr_cas(&sr_driver_cb, NULL, (atomic_ptr_val_t)cb)) {
		return -EALREADY;
	}

	return 0;
}

static void cd_pti_range_from_wire(struct nrf71_sr_coex_pti_range *dst, const unsigned char *src)
{
	dst->start = src[0];
	dst->end = src[1];
	dst->step = src[2];
}

/* Without a registered SR driver, report success so the CD runs unchanged. */
static int cd_sr_enable(bool enable)
{
	const struct nrf71_sr_coex_sr_driver_cb *cb = atomic_ptr_get(&sr_driver_cb);

	if ((cb == NULL) || (cb->enable == NULL)) {
		return 0;
	}

	return cb->enable(enable);
}

static int cd_sr_set_priority(const struct coex_sr_priority_range_t *sr_range)
{
	const struct nrf71_sr_coex_sr_driver_cb *cb = atomic_ptr_get(&sr_driver_cb);
	struct nrf71_sr_coex_sr_prio prio;

	if ((cb == NULL) || (cb->set_priority == NULL)) {
		return 0;
	}

	cd_pti_range_from_wire(&prio.rx, sr_range->sr_rx_client_ccconf_pti_range);
	cd_pti_range_from_wire(&prio.tx, sr_range->sr_tx_client_ccconf_pti_range);
	cd_pti_range_from_wire(&prio.rx_critical,
			       sr_range->sr_rx_client_critical_ccconf_pti_range);
	cd_pti_range_from_wire(&prio.tx_critical,
			       sr_range->sr_tx_client_critical_ccconf_pti_range);
	prio.level = sr_range->client_priority_level;

	return cb->set_priority(&prio);
}

/* ---- CM2CD event handling ---- */

/** Map CM2CD event enum to a string for debug logs. */
static const char *cd_cm2cd_event_name(enum cm_event_to_host_t event)
{
	switch (event) {
	case CM2CD_STATISTICS_EVENT:
		return "CM2CD_STATISTICS_EVENT";
	case CM2CD_UPDATE_COEX_PARAMS_EVENT:
		return "CM2CD_UPDATE_COEX_PARAMS_EVENT";
	case CM2CD_UPDATE_COEX_USER_PARAMS_EVENT:
		return "CM2CD_UPDATE_COEX_USER_PARAMS_EVENT";
	case CM2CD_ENABLE_COEXISTENCE_EVENT:
		return "CM2CD_ENABLE_COEXISTENCE_EVENT";
	case CM2CD_SET_PRIORITY_RANGES_EVENT:
		return "CM2CD_SET_PRIORITY_RANGES_EVENT";
	default:
		return "CM2CD_UNKNOWN_EVENT";
	}
}

/** Queue an event, dropping the oldest one if nobody is draining the queue. */
static void cd_cm_event_put(const struct cd_cm_event *ev)
{
	struct cd_cm_event dropped;

	if (k_msgq_put(&cd_cm_event_q, ev, K_NO_WAIT) != 0) {
		LOG_WRN("CM2CD event queue full, dropping the oldest event");
		(void)k_msgq_get(&cd_cm_event_q, &dropped, K_NO_WAIT);
		(void)k_msgq_put(&cd_cm_event_q, ev, K_NO_WAIT);
	}
}

/**
 * Block until event_name is received, the wait is aborted, or timeout_ms elapses.
 *
 * coex_event_handler() queues every CM2CD event header in arrival order. This
 * takes them one at a time and returns the first one that matches the expected
 * event. Events that do not match are logged and skipped, and the time already
 * spent counts against timeout_ms, so unrelated events cannot extend the wait.
 *
 * A late reply that arrives after the next command was posted can't be told
 * apart from the real one: the CM2CD header carries no sequence number. That
 * needs a token echoed by the CM firmware.
 */
static int cd_wait_for_cm_event(enum cm_event_to_host_t event_name, uint32_t timeout_ms,
				struct cd_cm_event *matched)
{
	const int64_t deadline_ms = k_uptime_get() + timeout_ms;
	struct cd_cm_event ev;
	int64_t remaining_ms;

	while ((remaining_ms = deadline_ms - k_uptime_get()) > 0) {
		if (k_msgq_get(&cd_cm_event_q, &ev, K_MSEC(remaining_ms)) != 0) {
			break;
		}

		if (ev.abort != 0U) {
			return -ECANCELED;
		}

		if ((enum cm_event_to_host_t)ev.hdr.event_name == event_name) {
			*matched = ev;
			return 0;
		}

		LOG_DBG("CM2CD %s (status=%u) while waiting for %s",
			cd_cm2cd_event_name((enum cm_event_to_host_t)ev.hdr.event_name),
			ev.hdr.command_status, cd_cm2cd_event_name(event_name));
	}

	return -ETIMEDOUT;
}

/** Validate the CM2CD event that completed a transaction. */
static int cd_validate_cm_event(enum cm_event_to_host_t event_name, const struct cd_cm_event *ev)
{
	if ((event_name == CM2CD_STATISTICS_EVENT) && (ev->stats_valid == 0U)) {
		LOG_ERR("CM2CD %s without stats payload", cd_cm2cd_event_name(event_name));
		return -EIO;
	}

	if (ev->hdr.command_status != COMMAND_PROCESSING_SUCCESS) {
		LOG_ERR("CM2CD %s command processing failed (status=%u)",
			cd_cm2cd_event_name(event_name), ev->hdr.command_status);
		return -EIO;
	}

	LOG_DBG("CM2CD %s command processing SUCCESS", cd_cm2cd_event_name(event_name));

	return 0;
}

/**
 * CM2CD event callback registered with the Wi-Fi FMAC coexistence path.
 *
 * Runs in the Wi-Fi driver's IPC workqueue when the CM sends an event. Copies
 * statistics if present and queues the event header for cd_wait_for_cm_event().
 */
static void coex_event_handler(void *ctx, const void *event, size_t len)
{
	const struct cm2cd_event_status_name_t *hdr;
	enum cm_event_to_host_t event_name;
	struct cd_cm_event queued = {0};
	size_t stats_offset;

	ARG_UNUSED(ctx);

	if (event == NULL) {
		LOG_ERR("CM2CD event dropped: NULL payload");
		return;
	}

	if (len < sizeof(*hdr)) {
		LOG_ERR("CM2CD event dropped: %zu bytes, need at least %zu for the header", len,
			sizeof(*hdr));
		return;
	}

	hdr = event;
	event_name = (enum cm_event_to_host_t)hdr->event_name;
	stats_offset = sizeof(*hdr);

	LOG_DBG("CM2CD %s received (%zu bytes, raw event_name=%u, status=%u)",
		cd_cm2cd_event_name(event_name), len, hdr->event_name, hdr->command_status);

	queued.hdr = *hdr;

	if (event_name == CM2CD_STATISTICS_EVENT) {
		k_mutex_lock(&cd_lock, K_FOREVER);

		cd_state.stats_valid = false;
		cd_state.patch_stats_valid = false;

		/* Payload layout: [header][cm_stats_t][optional cm_fsm_patch_stats_t]. */
		if (len >= (stats_offset + sizeof(struct cm_stats_t))) {
			memcpy(&cd_state.last_stats, (const uint8_t *)event + stats_offset,
			       sizeof(cd_state.last_stats));
			cd_state.stats_valid = true;
			queued.stats_valid = 1U;

			stats_offset += sizeof(cd_state.last_stats);
			if (len >= (stats_offset + sizeof(cd_state.last_patch_stats))) {
				memcpy(&cd_state.last_patch_stats,
				       (const uint8_t *)event + stats_offset,
				       sizeof(cd_state.last_patch_stats));
				cd_state.patch_stats_valid = true;
			}
		} else {
			LOG_WRN("Short statistics event (%zu bytes)", len);
		}

		k_mutex_unlock(&cd_lock);
	}

	cd_cm_event_put(&queued);
}

/**
 * Post a CD2CM command and wait for its CM2CD completion event.
 *
 * Covers all CD2CM to CM2CD pairs; cd_cmd_lock ensures only one such transaction
 * runs at a time, so two CM commands can never interleave.
 */
int coex_cd_cm_send_and_wait(const void *cmd, size_t len, enum cm_event_to_host_t expected_event)
{
	struct cd_cm_event reply;
	int ret;

	if ((cmd == NULL) || (len == 0U)) {
		return -EINVAL;
	}

	k_mutex_lock(&cd_cmd_lock, K_FOREVER);

	/* Drop replies to timed-out transactions and stale abort tokens. */
	k_msgq_purge(&cd_cm_event_q);

	ret = coex_cm_send(cmd, len);
	if (ret != 0) {
		goto out;
	}

	LOG_DBG("Posted CD2CM command, waiting for %s", cd_cm2cd_event_name(expected_event));

	ret = cd_wait_for_cm_event(expected_event, CM2CD_EVENT_WAIT_MS, &reply);
	if (ret == -ETIMEDOUT) {
		LOG_ERR("Timeout waiting for CM2CD %s", cd_cm2cd_event_name(expected_event));
		goto out;
	} else if (ret != 0) {
		goto out;
	}

	ret = cd_validate_cm_event(expected_event, &reply);
out:
	k_mutex_unlock(&cd_cmd_lock);

	return ret;
}

/* ---- CM configuration ---- */

static bool cd_epoch_changed(atomic_val_t epoch)
{
	return atomic_get(&wifi_epoch) != epoch;
}

/**
 * Push the stored coexistence configuration to the CM.
 *
 * Order matters: CM configuration first, then enable. Each step waits for its
 * CM2CD completion event before the next one starts. The configuration is
 * snapshotted under cd_lock and sent without it.
 *
 * @retval -ECANCELED Wi-Fi went down or came up again while this was running.
 */
static int cd_apply_cm_config(atomic_val_t epoch)
{
	struct coex_wifi_priority_range_t wifi_range;
	struct coex_sr_priority_range_t sr_range;
	struct coex_user_params_t user_params;
	int ret;

	k_mutex_lock(&cd_lock, K_FOREVER);
	wifi_range = cd_state.wifi_range;
	sr_range = cd_state.sr_range;
	user_params = cd_state.user_params;
	k_mutex_unlock(&cd_lock);

	ret = coex_cm_set_priority_ranges(&wifi_range, &sr_range);
	if (ret != 0) {
		return ret;
	}

	/*
	 * Mirror the SR ranges into the SR driver so its hardware uses matching PTI
	 * values. Non-fatal if SR rejects them: the CM already has the ranges.
	 */
	ret = cd_sr_set_priority(&sr_range);
	if (ret != 0) {
		LOG_WRN("SR driver rejected priority ranges (%d)", ret);
	}

	if (cd_epoch_changed(epoch)) {
		return -ECANCELED;
	}

	ret = coex_cm_update_user_params(&user_params);
	if (ret != 0) {
		return ret;
	}

	if (cd_epoch_changed(epoch)) {
		return -ECANCELED;
	}

	ret = coex_cm_update_coex_params();
	if (ret != 0) {
		return ret;
	}

	if (cd_epoch_changed(epoch)) {
		return -ECANCELED;
	}

	return coex_cm_enable(true);
}

static void cd_apply_work_handler(struct k_work *work)
{
	atomic_val_t epoch;
	bool first_attempt;
	int ret;

	ARG_UNUSED(work);

	k_mutex_lock(&cd_lock, K_FOREVER);
	if ((cd_state.wifi_state != CD_WIFI_CONFIGURING) &&
	    (cd_state.wifi_state != CD_WIFI_RETRY_WAIT)) {
		k_mutex_unlock(&cd_lock);
		return;
	}
	cd_state.wifi_state = CD_WIFI_CONFIGURING;
	epoch = atomic_get(&wifi_epoch);
	k_mutex_unlock(&cd_lock);

	ret = cd_apply_cm_config(epoch);

	k_mutex_lock(&cd_lock, K_FOREVER);

	if (cd_epoch_changed(epoch)) {
		LOG_DBG("Wi-Fi state changed during CM configuration (%d)", ret);
		goto out;
	}

	first_attempt = (cd_state.retries_left == CONFIG_NRF71_SR_COEX_CM_RETRY_MAX);

	if (ret == 0) {
		cd_state.wifi_state = CD_WIFI_ACTIVE;
		LOG_INF("nRF71 SR coexistence configured");
	} else if ((ret == -ETIMEDOUT) && first_attempt) {
		/*
		 * No reply at all on the first attempt: the firmware most likely has no
		 * CM2CD support. Retrying would only add CM_EVENT_WAIT_MS stalls.
		 */
		cd_state.wifi_state = CD_WIFI_FAILED;
		LOG_ERR("CM did not respond, coexistence disabled");
	} else if (cd_state.retries_left > 0) {
		cd_state.retries_left--;
		cd_state.wifi_state = CD_WIFI_RETRY_WAIT;
		LOG_WRN("CM configuration failed (%d), retrying in %d ms", ret,
			CD_CM_RETRY_INTERVAL_MS);
		(void)k_work_reschedule_for_queue(&cd_wq, &cd_apply_work,
						  K_MSEC(CD_CM_RETRY_INTERVAL_MS));
	} else {
		cd_state.wifi_state = CD_WIFI_FAILED;
		LOG_ERR("CM configuration failed (%d), coexistence disabled", ret);
	}
out:
	k_mutex_unlock(&cd_lock);
}

/* ---- Wi-Fi state notifier ---- */

static void cd_wifi_admin_up(void)
{
	k_mutex_lock(&cd_lock, K_FOREVER);
	atomic_inc(&wifi_epoch);
	cd_state.wifi_state = CD_WIFI_CONFIGURING;
	cd_state.retries_left = CONFIG_NRF71_SR_COEX_CM_RETRY_MAX;
	k_mutex_unlock(&cd_lock);

	(void)k_work_reschedule_for_queue(&cd_wq, &cd_apply_work, K_NO_WAIT);
}

/*
 * Runs before the Wi-Fi driver tears the interface down and must not block, so
 * an in-flight CM transaction is only told to stop, not waited for.
 */
static void cd_wifi_admin_down(void)
{
	struct cd_cm_event abort_ev = {
		.abort = 1U,
	};

	k_mutex_lock(&cd_lock, K_FOREVER);
	atomic_inc(&wifi_epoch);
	cd_state.wifi_state = CD_WIFI_DOWN;
	/* Statistics describe the CM session that just ended. */
	cd_state.stats_valid = false;
	cd_state.patch_stats_valid = false;
	k_mutex_unlock(&cd_lock);

	(void)k_work_cancel_delayable(&cd_apply_work);
	cd_cm_event_put(&abort_ev);
}

static void cd_wifi_state_cb(const struct nrf71_wifi_coex_state_evt *evt, void *ctx)
{
	ARG_UNUSED(ctx);

	switch (evt->type) {
	case NRF71_WIFI_COEX_STATE_ADMIN_UP:
		LOG_DBG("Wi-Fi admin up");
		cd_wifi_admin_up();
		break;
	case NRF71_WIFI_COEX_STATE_ADMIN_DOWN:
		LOG_DBG("Wi-Fi admin down");
		cd_wifi_admin_down();
		break;
	default:
		break;
	}
}

/* ---- CD APIs exposed to the Short-Range driver ---- */

/*
 * SR power state does not affect the CM, which is reached over Wi-Fi only, so
 * this only drives the SR driver callbacks.
 */
int nrf71_sr_coex_sr_power_notify(enum nrf71_sr_coex_sr_power_event event)
{
	struct coex_sr_priority_range_t sr_range;
	int ret;

	switch (event) {
	case NRF71_SR_COEX_SR_PREPARE_POWER_DOWN:
		k_mutex_lock(&cd_lock, K_FOREVER);
		if (cd_state.sr_transition_active) {
			k_mutex_unlock(&cd_lock);
			return -EBUSY;
		}
		k_mutex_unlock(&cd_lock);

		(void)cd_sr_enable(false);
		return 0;

	case NRF71_SR_COEX_SR_POWERED_UP_READY:
		/*
		 * sr_transition_active is held across the SR driver calls below so a
		 * concurrent power notification can't run in between.
		 */
		k_mutex_lock(&cd_lock, K_FOREVER);
		if (cd_state.sr_transition_active) {
			k_mutex_unlock(&cd_lock);
			return -EBUSY;
		}
		cd_state.sr_transition_active = true;
		sr_range = cd_state.sr_range;
		k_mutex_unlock(&cd_lock);

		/* Best-effort: the CM already has the ranges. */
		ret = cd_sr_set_priority(&sr_range);
		if (ret != 0) {
			LOG_WRN("SR driver rejected priority ranges on power-up (%d)", ret);
		}

		ret = cd_sr_enable(true);
		if (ret != 0) {
			LOG_WRN("SR driver rejected coexistence enable on power-up (%d)", ret);
			ret = -EIO;
		}

		k_mutex_lock(&cd_lock, K_FOREVER);
		cd_state.sr_transition_active = false;
		k_mutex_unlock(&cd_lock);
		return ret;

	default:
		return -EINVAL;
	}
}

/* ---- Initialisation ---- */

static int nrf71_sr_coex_sys_init(void)
{
	const struct k_work_queue_config wq_cfg = {
		.name = "nrf71_coex_cm",
	};
	int ret;

	k_work_queue_start(&cd_wq, cd_wq_stack, K_THREAD_STACK_SIZEOF(cd_wq_stack),
			   CONFIG_NRF71_SR_COEX_WQ_PRIORITY, &wq_cfg);

	/* Must be in place before the first CD2CM command is posted. */
	(void)nrf71_wifi_coex_register_event_cb(coex_event_handler, NULL);

	/* Replays ADMIN_UP if a Wi-Fi interface is already up. */
	ret = nrf71_wifi_coex_register_state_cb(cd_wifi_state_cb, NULL);
	if (ret != 0) {
		LOG_ERR("Failed to register for Wi-Fi state (%d)", ret);
		return ret;
	}

	return 0;
}

SYS_INIT(nrf71_sr_coex_sys_init, APPLICATION, CONFIG_NRF71_SR_COEX_DRIVER_INIT_PRIORITY);
