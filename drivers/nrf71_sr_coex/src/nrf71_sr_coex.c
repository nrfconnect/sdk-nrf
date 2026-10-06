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
 *   CM sends CM2CD_* event -> coex_event_handler() stores the result and wakes
 *   up whoever was waiting for that event.
 *
 * Most APIs follow the same pattern:
 *   1. Send a CD2CM command
 *   2. Wait (up to CM2CD_EVENT_WAIT_MS) for the matching CM2CD completion event
 *   3. Validate success/failure and return to the caller
 *
 * Statistics are special: coex_cd_get_stats() triggers the round-trip, but the
 * actual numbers are copied into cd_state inside coex_event_handler() when
 * CM2CD_STATISTICS_EVENT arrives. coex_cd_get_last_stats() only copies out that
 * cache.
 *
 * Locking rules (important when adding code):
 *   - cd_state.lock guards every cd_state field.
 *   - cd_state.cmd_lock serializes one CD2CM/CM2CD transaction at a time.
 *   - Lock order is cmd_lock -> lock. Never hold cd_state.lock while calling a
 *     coex_cm_* function: those acquire cmd_lock and then take cd_state.lock
 *     again from the wait path, which would deadlock. Take a local snapshot of
 *     the cd_state fields you need, release the lock, then send.
 */

#include <errno.h>
#include <string.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

/* Wi-Fi FMAC coexistence transport (cmd/event/reg access). */
#include <drivers/wifi/nrf71/nrf71_wifi_coex.h>
/* CD2CM / CM2CD message structures shared with RPU firmware. */
#include <nrf71_coex_if.h>
/* CD to Short-Range driver interface. */
#include <nrf71_cd_sr_if.h>

#include <nrf71_sr_coex_api.h>
#include "nrf71_sr_coex_internal.h"

LOG_MODULE_REGISTER(nrf71_sr_coex, CONFIG_NRF71_SR_COEX_DRIVER_LOG_LEVEL);

/* Max time (ms) to wait for a CM2CD response after posting a CD2CM command. */
#define CM2CD_EVENT_WAIT_MS CONFIG_NRF71_SR_COEX_CM_EVENT_WAIT_MS

/* Antenna configuration assumed until coex_cd_configure_coexc() selects another one. */
#define CD_DEFAULT_ANTENNA_CFG COEX_SHARED_ANT_CFG

/*
 * Default Wi-Fi priority ranges sent to the CM at startup.
 * May be overridden later via CD2CM_SET_PRIORITY_RANGES.
 *
 * Each range is {start, end, step}: start = numerically largest (lowest
 * priority), end = smallest (highest priority).
 */
static const struct coex_wifi_priority_range_t default_wifi_range = {
	.sw_request_priority_range = {10, 5, 1},
	.client0_ccconf_pti_range = {20, 15, 1},   /* high-priority Wi-Fi Rx */
	.client1_ccconf_pti_range = {25, 20, 1},   /* high-priority Wi-Fi Tx */
	.client2_ccconf_pti_range = {140, 120, 3}, /* low-priority Wi-Fi Rx */
	.client3_ccconf_pti_range = {160, 145, 3}, /* low-priority Wi-Fi Tx */
	.hw_client_priority_level = 5,
};

/* Default SR priority ranges; same {start, end, step} convention. */
static const struct coex_sr_priority_range_t default_sr_range = {
	.sr_rx_client_ccconf_pti_range = {30, 25, 1},
	.sr_tx_client_ccconf_pti_range = {40, 30, 2},
	.sr_rx_client_critical_ccconf_pti_range = {20, 15, 1},
	.sr_tx_client_critical_ccconf_pti_range = {25, 20, 1},
	.client_priority_level = 5,
};

/*
 * Default user coexistence parameters (protection probabilities, antenna mode).
 * Sent via CD2CM_UPDATE_COEX_USER_PARAMS. Values are 0-100 (percent).
 */
static const struct coex_user_params_t default_user_params = {
	.message_id = CD2CM_UPDATE_COEX_USER_PARAMS,
	.listen2inactive_sr_rx_prot_prob_ps = 100,
	.inactive2listen_sr_rx_prot_prob_ps = 100,
	.inactive2listen_sr_rx_prot_prob_calib = 100,
	.listen2inactive_sr_rx_prot_prob_calib = 100,
	.wifi_scan_puncture_info = {.wifi_scan_prot_prob = 100},
	.wifi_beacon_prot_prob = 100,
	.wifi_conn_prot_prob = 100,
	.wifi_calib_prot_prob = 100,
	.shared_ant_control = ANT_ALLOC_STATIC_WIFI,
};

/**
 * All driver state in one place.
 *
 *   - Radio up/down flags (wifi_up, sr_up, sr_coex_enabled)
 *   - COEXC hardware state (coexc_configured)
 *   - Antenna configuration (antenna_cfg)
 *   - Working copies of config the app can change (wifi_range, sr_range, user_params)
 *   - Last CM2CD event received (last_event)
 *   - Cached statistics from the last GET_STATS (last_stats, last_patch_stats)
 */
static struct {
	struct k_mutex lock;       /* Protects every cd_state field below. */
	struct k_mutex cmd_lock;   /* Serializes post/wait CM transactions. */
	struct k_sem cm_event_sem; /* Signaled when any CM2CD event arrives. */

	bool wifi_up;              /* Wi-Fi reported powered-up and CM configured. */
	bool sr_up;                /* SR reported powered-up. */
	bool sr_coex_enabled;      /* true when both radios up and coex may run. */

	bool transition_active;    /* Guard against overlapping power notifications. */

	/*
	 * COEXC registers programmed. Sticky for the lifetime of the driver: COEXC
	 * lives in the always-on global domain, so its tables outlive an RPU reset.
	 * Cleared only when programming fails and the hardware state is unknown.
	 */
	bool coexc_configured;

	enum coex_antenna_cfg_type antenna_cfg;       /* Working copy of antenna configuration. */
	struct coex_wifi_priority_range_t wifi_range; /* Working copy of Wi-Fi ranges. */
	struct coex_sr_priority_range_t sr_range;     /* Working copy of SR ranges. */
	struct coex_user_params_t user_params;        /* Working copy of user params. */

	struct cm2cd_event_status_name_t last_event;  /* Latest CM2CD event header. */

	bool stats_valid;          /* last_stats filled from CM2CD_STATISTICS_EVENT. */
	bool patch_stats_valid;    /* last_patch_stats filled from same event. */
	struct cm_stats_t last_stats;                 /* ROM CM statistics payload. */
	struct cm_fsm_patch_stats_t last_patch_stats; /* Patch CM statistics payload. */
} cd_state;

/* ---- Short-Range driver APIs (until the SR driver provides them) ---- */

/**
 * Called by the CD to enable/disable coexistence inside the SR driver.
 * Weak stub returns success until the real SR driver is linked.
 */
__weak unsigned int coex_sr_enable(unsigned int enable_coex)
{
	ARG_UNUSED(enable_coex);
	return 1U;
}

/**
 * Called by the CD to push SR priority ranges into the SR driver.
 * Weak stub returns success until the real SR driver is linked.
 */
__weak unsigned int coex_sr_set_client_priority(
	const struct coex_sr_priority_range_t *sr_priority_range)
{
	ARG_UNUSED(sr_priority_range);
	return 1U;
}

/* ---- cd_state helpers ---- */

/**
 * Clear all Wi-Fi-side runtime state on Wi-Fi power-down and after a failed bring-up.
 *
 * Only state owned by the Wi-Fi/RPU session is dropped: the CM configuration is lost
 * when the RPU resets, and the cached statistics describe that dead session.
 *
 * coexc_configured is deliberately NOT cleared. COEXC register contents are
 * unaffected by the RPU reset because the block lives in the global domain and
 * is programmed by host.
 */
static void cd_mark_wifi_runtime_down_locked(void)
{
	cd_state.wifi_up = false;
	cd_state.sr_coex_enabled = false;
	cd_state.stats_valid = false;
	cd_state.patch_stats_valid = false;
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

/**
 * Drop CM2CD signals left over from an earlier transaction.
 *
 * cm_event_sem is a counting semaphore, so a CM2CD event that arrives after
 * cd_wait_for_cm_event() timed out stays pending and would immediately wake the
 * next transaction with a stale last_event. Called with cmd_lock held, so no
 * other transaction can be waiting on a signal that is still wanted.
 */
static void cd_drain_cm_event_sem(void)
{
	while (k_sem_take(&cd_state.cm_event_sem, K_NO_WAIT) == 0) {
		/* Discard stale signal. */
	}
}

/**
 * Block until event_name is received or timeout_ms elapses.
 *
 * After sending a CD2CM command, the caller waits here. Each time
 * coex_event_handler() receives ANY CM2CD event, it signals cm_event_sem.
 * This function wakes up and checks whether the event matches the expected one.
 * If not, it logs the unexpected event and keeps waiting for the time left, so
 * unrelated events cannot extend the total wait beyond timeout_ms.
 */
static int cd_wait_for_cm_event(enum cm_event_to_host_t event_name, uint32_t timeout_ms)
{
	const int64_t deadline_ms = k_uptime_get() + timeout_ms;
	enum cm_event_to_host_t received;
	unsigned int command_status;
	int64_t remaining_ms;

	/* Wake on each CM2CD event posted to cm_event_sem by coex_event_handler(). */
	while (((remaining_ms = deadline_ms - k_uptime_get()) > 0) &&
	       (k_sem_take(&cd_state.cm_event_sem, K_MSEC(remaining_ms)) == 0)) {
		k_mutex_lock(&cd_state.lock, K_FOREVER);
		received = (enum cm_event_to_host_t)cd_state.last_event.event_name;
		command_status = cd_state.last_event.command_status;
		k_mutex_unlock(&cd_state.lock);

		if (received == event_name) {
			return 0;
		}

		/* Unexpected CM2CD event; log it and keep waiting for the expected one. */
		LOG_DBG("CM2CD %s (status=%u) while waiting for %s",
			cd_cm2cd_event_name(received), command_status,
			cd_cm2cd_event_name(event_name));
	}

	return -ETIMEDOUT;
}

/** Validate the last received CM2CD event after a successful wait. */
static int cd_validate_last_cm_event(enum cm_event_to_host_t event_name)
{
	k_mutex_lock(&cd_state.lock, K_FOREVER);

	switch (event_name) {
	case CM2CD_STATISTICS_EVENT:
		/* Statistics event must include cm_stats_t after the header. */
		if (!cd_state.stats_valid) {
			LOG_ERR("CM2CD %s without stats payload", cd_cm2cd_event_name(event_name));
			k_mutex_unlock(&cd_state.lock);
			return -EIO;
		}

		if (cd_state.last_event.command_status != COMMAND_PROCESSING_SUCCESS) {
			LOG_ERR("CM2CD %s command processing failed (status=%u)",
				cd_cm2cd_event_name(event_name),
				cd_state.last_event.command_status);
			k_mutex_unlock(&cd_state.lock);
			return -EIO;
		}

		k_mutex_unlock(&cd_state.lock);
		return 0;

	/* Patched CM2CD command-completion events: */
	default:
		if (cd_state.last_event.command_status != COMMAND_PROCESSING_SUCCESS) {
			LOG_ERR("CM2CD %s command processing FAIL (status=%u)",
				cd_cm2cd_event_name(event_name),
				cd_state.last_event.command_status);
			k_mutex_unlock(&cd_state.lock);
			return -EIO;
		}

		LOG_DBG("CM2CD %s command processing SUCCESS", cd_cm2cd_event_name(event_name));
		k_mutex_unlock(&cd_state.lock);
		return 0;
	}
}

/**
 * CM2CD event callback registered with the Wi-Fi FMAC coexistence path.
 *
 * Runs in Wi-Fi driver context when the Coexistence Manager sends an event. Stores
 * the latest header, copies statistics if present, and wakes
 * cd_wait_for_cm_event() via cm_event_sem.
 *
 * This is the single place where CM replies are captured. coex_cd_get_stats()
 * does not copy statistics itself; this handler does it when the event is
 * CM2CD_STATISTICS_EVENT.
 */
static void coex_event_handler(void *ctx, const void *event, size_t len)
{
	const struct cm2cd_event_status_name_t *hdr;
	enum cm_event_to_host_t event_name;
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

	k_mutex_lock(&cd_state.lock, K_FOREVER);
	cd_state.last_event = *hdr;

	switch (event_name) {
	case CM2CD_STATISTICS_EVENT:
		cd_state.stats_valid = false;
		cd_state.patch_stats_valid = false;

		/* Payload layout: [header][cm_stats_t][optional cm_fsm_patch_stats_t]. */
		if (len >= (stats_offset + sizeof(struct cm_stats_t))) {
			memcpy(&cd_state.last_stats, (const uint8_t *)event + stats_offset,
			       sizeof(cd_state.last_stats));
			cd_state.stats_valid = true;

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
		break;

	/* Patched CM2CD command-completion events. */
	default:
		if (hdr->command_status != COMMAND_PROCESSING_SUCCESS) {
			LOG_WRN("CM2CD %s command processing FAIL (status=%u)",
				cd_cm2cd_event_name(event_name), hdr->command_status);
		} else {
			LOG_DBG("CM2CD %s command processing SUCCESS",
				cd_cm2cd_event_name(event_name));
		}
		break;
	}

	k_mutex_unlock(&cd_state.lock);
	k_sem_give(&cd_state.cm_event_sem);
}

/**
 * Post a CD2CM command and wait for its CM2CD completion event.
 *
 * Covers all CD2CM to CM2CD pairs; cmd_lock ensures only one such transaction
 * runs at a time, so two CM commands can never interleave.
 */
int coex_cd_cm_send_and_wait(const void *cmd, size_t len, enum cm_event_to_host_t expected_event)
{
	int ret;

	if ((cmd == NULL) || (len == 0U)) {
		return -EINVAL;
	}

	k_mutex_lock(&cd_state.cmd_lock, K_FOREVER);

	/* Discard any CM2CD signal left over from a timed-out transaction. */
	cd_drain_cm_event_sem();

	/* Step 1: post the CD2CM command to the CM. */
	ret = coex_cm_send(cmd, len);
	if (ret != 0) {
		k_mutex_unlock(&cd_state.cmd_lock);
		return ret;
	}

	LOG_DBG("Posted CD2CM command, waiting for %s", cd_cm2cd_event_name(expected_event));

	/* Step 2: block until expected_event appears (see coex_event_handler). */
	ret = cd_wait_for_cm_event(expected_event, CM2CD_EVENT_WAIT_MS);
	if (ret != 0) {
		/* Expected_event did not arrive within CM2CD_EVENT_WAIT_MS. */
		LOG_ERR("Timeout waiting for CM2CD %s", cd_cm2cd_event_name(expected_event));
		k_mutex_unlock(&cd_state.cmd_lock);
		return ret;
	}

	/* Step 3: check command_status / stats payload for expected_event. */
	ret = cd_validate_last_cm_event(expected_event);
	k_mutex_unlock(&cd_state.cmd_lock);
	return ret;
}

/* ---- Helper functions ---- */

/**
 * Make sure COEXC hardware is configured before the first CM command is sent.
 *
 * Uses the antenna configuration recorded in cd_state so a product that selected
 * COEX_SEPARATE_ANT_CFG through coex_cd_configure_coexc() is verified and, if
 * needed, programmed against that configuration rather than the shared-antenna default.
 */
static int cd_configure_coexc_from_state(void)
{
	enum coex_antenna_cfg_type antenna_cfg;
	int ret;

	k_mutex_lock(&cd_state.lock, K_FOREVER);
	if (cd_state.coexc_configured) {
		k_mutex_unlock(&cd_state.lock);
		return 0;
	}
	antenna_cfg = cd_state.antenna_cfg;
	k_mutex_unlock(&cd_state.lock);

	/* Write CCMALLOW, CCCONF, TURNAROUND. */
	ret = cd_coexc_configuration(antenna_cfg);
	if (ret != 0) {
		LOG_ERR("COEXC configure failed: %s (%d)", strerror(-ret), ret);
		return ret;
	}

	k_mutex_lock(&cd_state.lock, K_FOREVER);
	cd_state.coexc_configured = true;
	k_mutex_unlock(&cd_state.lock);

	return 0;
}

/**
 * Send the working priority ranges from cd_state to the CM and the SR driver.
 *
 * The ranges are snapshotted under cd_state.lock and the lock is released
 * before the CM transaction, because coex_cm_* takes cmd_lock (see the locking
 * rules at the top of this file).
 */
static int cd_send_priority_ranges_from_state(void)
{
	struct coex_wifi_priority_range_t wifi_range;
	struct coex_sr_priority_range_t sr_range;
	int ret;

	k_mutex_lock(&cd_state.lock, K_FOREVER);
	wifi_range = cd_state.wifi_range;
	sr_range = cd_state.sr_range;
	k_mutex_unlock(&cd_state.lock);

	ret = coex_cm_set_priority_ranges(&wifi_range, &sr_range);
	if (ret != 0) {
		return ret;
	}

	/*
	 * Mirror the SR ranges into the SR driver so its hardware uses matching PTI
	 * values. Non-fatal if SR rejects them: the CM already has the ranges.
	 */
	if (coex_sr_set_client_priority(&sr_range) == 0U) {
		LOG_WRN("SR driver rejected priority ranges");
	}

	return 0;
}

/** Send the working user params from cd_state to the CM (snapshot, then send). */
static int cd_send_user_params_from_state(void)
{
	struct coex_user_params_t user_params;

	k_mutex_lock(&cd_state.lock, K_FOREVER);
	user_params = cd_state.user_params;
	k_mutex_unlock(&cd_state.lock);

	return coex_cm_update_user_params(&user_params);
}

/**
 * Push the current coexistence settings to the Coexistence Manager (CM).
 *
 * Full bring-up sequence, run on every
 * coex_cd_wifi_power_notify(COEX_WIFI_POWERED_UP_READY). Order matters: COEXC hardware first, then
 * CM configuration, then enable. Each step waits for its CM2CD completion
 * event before the next one starts.
 *
 * On failure the CM is left partially configured. The caller is responsible for
 * clearing the runtime flags (see cd_mark_wifi_runtime_down_locked()) so the
 * whole sequence is retried on the next Wi-Fi power-up.
 */
static int cd_apply_cm_config(void)
{
	int ret;

	/* Step 1: Configure COEXC registers before talking to the CM. */
	ret = cd_configure_coexc_from_state();
	if (ret != 0) {
		return ret;
	}

	/* Step 2: CD2CM_SET_PRIORITY_RANGES, plus the SR-driver mirror. */
	ret = cd_send_priority_ranges_from_state();
	if (ret != 0) {
		return ret;
	}

	/* Step 3: CD2CM_UPDATE_COEX_USER_PARAMS (protection probabilities, etc.). */
	ret = cd_send_user_params_from_state();
	if (ret != 0) {
		return ret;
	}

	/* Step 4: CD2CM_UPDATE_COEX_PARAMS with the default NRF_COEX_PARAMS blob. */
	ret = coex_cm_update_coex_params();
	if (ret != 0) {
		return ret;
	}

	/* Step 5: CD2CM_ENABLE_COEXISTENCE. */
	return coex_cm_enable(true);
}

/* ---- CM configuration / runtime APIs ---- */

/**
 * Post CD2CM_ENABLE_COEXISTENCE and wait for CM2CD_ENABLE_COEXISTENCE_EVENT.
 *
 * This only flips the enable state inside the CM. It deliberately does not
 * change sr_coex_enabled, which tracks radio power state rather than CM policy,
 * so a caller that disables the CM is expected to stop issuing coexistence
 * commands on its own.
 */
int coex_cd_enable(bool enable)
{
	return coex_cm_enable(enable);
}

/**
 * Post CD2CM_SET_PRIORITY_RANGES, wait for CM2CD_SET_PRIORITY_RANGES_EVENT and
 * retain the ranges in cd_state.
 *
 * Retaining the ranges matters because cd_state is replayed later:
 * cd_apply_cm_config() resends them after a Wi-Fi power-up, and
 * coex_cd_sr_power_notify() pushes cd_state.sr_range into the SR driver on SR
 * power-up. Without this the CM would hold the new ranges while a later
 * power-up silently reverted the SR side to the build-time defaults.
 */
int coex_cd_set_priority_ranges(const struct coex_wifi_priority_range_t *wifi_range,
				const struct coex_sr_priority_range_t *sr_range)
{
	int ret;

	if ((wifi_range == NULL) || (sr_range == NULL)) {
		return -EINVAL;
	}

	ret = coex_cm_set_priority_ranges(wifi_range, sr_range);
	if (ret != 0) {
		return ret;
	}

	k_mutex_lock(&cd_state.lock, K_FOREVER);
	cd_state.wifi_range = *wifi_range;
	cd_state.sr_range = *sr_range;
	k_mutex_unlock(&cd_state.lock);

	/* Keep the SR driver in sync with the ranges the CM just accepted. */
	if (coex_sr_set_client_priority(sr_range) == 0U) {
		LOG_WRN("SR driver rejected priority ranges");
	}

	return 0;
}

/**
 * Post CD2CM_UPDATE_COEX_USER_PARAMS, wait for the completion event and retain
 * the params in cd_state so they are replayed after a Wi-Fi power-up.
 */
int coex_cd_update_user_params(const struct coex_user_params_t *user_params)
{
	int ret;

	if (user_params == NULL) {
		return -EINVAL;
	}

	ret = coex_cm_update_user_params(user_params);
	if (ret != 0) {
		return ret;
	}

	k_mutex_lock(&cd_state.lock, K_FOREVER);
	cd_state.user_params = *user_params;
	/* The CM expects the message id inside the payload as well. */
	cd_state.user_params.message_id = CD2CM_UPDATE_COEX_USER_PARAMS;
	k_mutex_unlock(&cd_state.lock);

	return 0;
}

/** Post CD2CM_UPDATE_COEX_PARAMS (default blob) and wait for CM2CD_UPDATE_COEX_PARAMS_EVENT. */
int coex_cd_update_coex_params(void)
{
	return coex_cm_update_coex_params();
}

/**
 * Post CD2CM_UPDATE_COEX_PARAMS with a caller-supplied blob and wait for
 * CM2CD_UPDATE_COEX_PARAMS_EVENT.
 *
 * The blob is not retained, so cd_apply_cm_config() reapplies the default
 * NRF_COEX_PARAMS blob after a Wi-Fi power-up. Callers that need a custom blob
 * to survive an RPU reset must send it again themselves.
 */
int coex_cd_update_coex_params_blob(const uint8_t *blob, size_t blob_len)
{
	return coex_cm_update_coex_params_blob(blob, blob_len);
}

/**
 * Post CD2CM_GET_STATS and wait for CM2CD_STATISTICS_EVENT.
 *
 * Asks the CM for coexistence counters (grants, denials, etc.). This function
 * only triggers the request and waits; the numbers are stored by
 * coex_event_handler() when the statistics event arrives. After this returns 0,
 * call coex_cd_get_last_stats() to copy out the cached snapshot.
 */
int coex_cd_get_stats(void)
{
	return coex_cm_get_stats();
}

/**
 * Copy the cm_stats_t retained from the last CM2CD_STATISTICS_EVENT.
 *
 * coex_event_handler() writes cd_state.last_stats under cd_state.lock, so
 * copying under the same lock guarantees the caller a consistent snapshot even
 * if another statistics event arrives at the same time.
 */
int coex_cd_get_last_stats(struct cm_stats_t *stats)
{
	int ret = -ENODATA;

	if (stats == NULL) {
		return -EINVAL;
	}

	k_mutex_lock(&cd_state.lock, K_FOREVER);
	if (cd_state.stats_valid) {
		*stats = cd_state.last_stats;
		ret = 0;
	}
	k_mutex_unlock(&cd_state.lock);

	return ret;
}

/**
 * Copy the patch statistics retained from the last CM2CD_STATISTICS_EVENT.
 * Same locking rules as coex_cd_get_last_stats().
 */
int coex_cd_get_last_patch_stats(struct cm_fsm_patch_stats_t *patch_stats)
{
	int ret = -ENODATA;

	if (patch_stats == NULL) {
		return -EINVAL;
	}

	k_mutex_lock(&cd_state.lock, K_FOREVER);
	if (cd_state.patch_stats_valid) {
		*patch_stats = cd_state.last_patch_stats;
		ret = 0;
	}
	k_mutex_unlock(&cd_state.lock);

	return ret;
}

/* ---- COEXC hardware APIs ---- */

/**
 * Program the COEXC tables for the given antenna configuration.
 *
 * This lets an application program the hardware explicitly for a selected antenna
 * configuration. The selected configuration is recorded in cd_state.antenna_cfg
 * so a later reprogram uses the same tables.
 *
 * COEXC is in the global domain on nRF71 and is programmed through host and
 * the Wi-Fi transport is not involved.
 */
int coex_cd_configure_coexc(enum coex_antenna_cfg_type antenna_cfg_type)
{
	int ret;

	if ((antenna_cfg_type != COEX_SHARED_ANT_CFG) &&
	    (antenna_cfg_type != COEX_SEPARATE_ANT_CFG)) {
		return -EINVAL;
	}

	ret = cd_coexc_configuration(antenna_cfg_type);

	k_mutex_lock(&cd_state.lock, K_FOREVER);
	if (ret == 0) {
		cd_state.antenna_cfg = antenna_cfg_type;
		cd_state.coexc_configured = true;
	} else {
		/* Hardware state is unknown; force a verify/reprogram on next bring-up. */
		cd_state.coexc_configured = false;
	}
	k_mutex_unlock(&cd_state.lock);

	return ret;
}

/* ---- CD APIs exposed to the Short-Range driver ---- */

/**
 * SR radio power lifecycle notifications from the Short-Range driver.
 *
 * Keeps cd_state in step with the SR power state and drives the coex_sr_* hooks
 * that talk to the SR radio driver. The SR driver calls this when it is about
 * to sleep and again once it has finished booting.
 *
 * sr_coex_enabled is the runtime "may issue coexistence commands" gate: set
 * only while both radios are up and the Wi-Fi transport is alive, cleared on
 * either radio's power-down.
 */
int coex_cd_sr_power_notify(enum coex_sr_power_event_t event)
{
	switch (event) {
	case COEX_SR_PREPARE_POWER_DOWN:
		/*
		 * SR is about to sleep or power off. Clear the local flags so the
		 * coexistence gate stays closed until both radios are back.
		 */
		k_mutex_lock(&cd_state.lock, K_FOREVER);
		if (cd_state.transition_active) {
			k_mutex_unlock(&cd_state.lock);
			return -EBUSY;
		}
		cd_state.transition_active = true;
		cd_state.sr_up = false;
		cd_state.sr_coex_enabled = false;
		cd_state.transition_active = false;
		k_mutex_unlock(&cd_state.lock);

		/* Tell the SR driver to stop participating in coexistence. */
		(void)coex_sr_enable(0U);
		return 0;

	case COEX_SR_POWERED_UP_READY:
		/*
		 * SR is powered and ready. transition_active is held across the
		 * coex_sr_* calls below so a concurrent power notification cannot
		 * mutate cd_state mid-transition.
		 */
		k_mutex_lock(&cd_state.lock, K_FOREVER);
		if (cd_state.transition_active) {
			k_mutex_unlock(&cd_state.lock);
			return -EBUSY;
		}
		cd_state.transition_active = true;
		k_mutex_unlock(&cd_state.lock);

		/*
		 * Push the retained SR priority ranges into the SR driver. Best-effort:
		 * the CM already has them from cd_apply_cm_config().
		 */
		if (coex_sr_set_client_priority(&cd_state.sr_range) == 0U) {
			LOG_WRN("SR driver rejected priority ranges on power-up");
		}

		/* Enable coexistence inside the SR driver. */
		(void)coex_sr_enable(1U);

		/* New SR requests are accepted only when Wi-Fi/CM are also ready. */
		k_mutex_lock(&cd_state.lock, K_FOREVER);
		cd_state.sr_up = true;
		cd_state.sr_coex_enabled = cd_state.wifi_up && nrf71_wifi_coex_is_ready();
		cd_state.transition_active = false;
		k_mutex_unlock(&cd_state.lock);
		return 0;

	default:
		return -EINVAL;
	}
}

/**
 * Wi-Fi radio power lifecycle notifications from the Wi-Fi driver.
 *
 * The Wi-Fi side owns the transport that carries CD2CM traffic, so Wi-Fi
 * power-up is what triggers the CM configuration via cd_apply_cm_config().
 * The CM runs on the RPU and loses all of its state across a reset, so priority
 * ranges, user params, coex params and the enable have to be sent again. COEXC
 * is unaffected: it is in the global domain and keeps the tables it was given.
 *
 * Wi-Fi power-down only clears local flags; it sends no CM disable command
 * because the transport is about to disappear anyway.
 */
int coex_cd_wifi_power_notify(enum coex_wifi_power_event_t event)
{
	int ret;

	switch (event) {
	case COEX_WIFI_PREPARE_POWER_DOWN:
		/*
		 * Wi-Fi/RPU is shutting down: block new coexistence activity and drop
		 * the CM-session state (cached statistics) that dies with the RPU.
		 * COEXC programming is left alone; see cd_mark_wifi_runtime_down_locked().
		 */
		k_mutex_lock(&cd_state.lock, K_FOREVER);
		if (cd_state.transition_active) {
			k_mutex_unlock(&cd_state.lock);
			return -EBUSY;
		}
		cd_state.transition_active = true;
		cd_mark_wifi_runtime_down_locked();
		cd_state.transition_active = false;
		k_mutex_unlock(&cd_state.lock);
		return 0;

	case COEX_WIFI_POWERED_UP_READY:
		/*
		 * Wi-Fi transport and RPU are back: restore the full coexistence setup.
		 * cd_apply_cm_config() runs outside cd_state.lock because it posts
		 * CD2CM commands and waits for CM2CD events.
		 */
		k_mutex_lock(&cd_state.lock, K_FOREVER);
		if (cd_state.transition_active) {
			k_mutex_unlock(&cd_state.lock);
			return -EBUSY;
		}
		cd_state.transition_active = true;
		k_mutex_unlock(&cd_state.lock);

		/*
		 * Re-apply full CM configuration after RPU/Wi-Fi comes back.
		 * This includes enabling coexistence in Coexistence Manager.
		 */
		ret = cd_apply_cm_config();

		k_mutex_lock(&cd_state.lock, K_FOREVER);
		/*
		 * wifi_up reflects CM config success, not merely "Wi-Fi driver loaded".
		 * sr_coex_enabled additionally requires SR to be up.
		 */
		if (ret == 0) {
			cd_state.wifi_up = true;
			cd_state.sr_coex_enabled = cd_state.sr_up;
		} else {
			/* Partial configuration: retry the whole sequence next power-up. */
			cd_mark_wifi_runtime_down_locked();
		}
		cd_state.transition_active = false;
		k_mutex_unlock(&cd_state.lock);

		return (ret == 0) ? 0 : -EIO;

	default:
		return -EINVAL;
	}
}

bool coex_cd_wifi_is_up(void)
{
	bool wifi_up;

	k_mutex_lock(&cd_state.lock, K_FOREVER);
	wifi_up = cd_state.wifi_up;
	k_mutex_unlock(&cd_state.lock);

	return wifi_up;
}

/* ---- Initialisation ---- */

/**
 * Driver init, run at APPLICATION level after the Wi-Fi driver.
 *
 *   - Initialise the cd_state mutexes and the CM event semaphore
 *   - Load the default antenna configuration, priority ranges and user params
 *   - Register coex_event_handler() for all incoming CM2CD events
 *
 * CM programming is deliberately deferred until
 * coex_cd_wifi_power_notify(COEX_WIFI_POWERED_UP_READY) so commands are not
 * sent while the RPU/VIF is still booting during SYS_INIT.
 */
static int nrf71_sr_coex_init(void)
{
	LOG_DBG("SR Coexistence initialization");

	/* Synchronisation primitives for cd_state and serialized CM transactions. */
	k_mutex_init(&cd_state.lock);
	k_mutex_init(&cd_state.cmd_lock);
	k_sem_init(&cd_state.cm_event_sem, 0, K_SEM_MAX_LIMIT);

	/* Working copies used by cd_apply_cm_config() and the coex_cd_* APIs. */
	cd_state.antenna_cfg = CD_DEFAULT_ANTENNA_CFG;
	cd_state.wifi_range = default_wifi_range;
	cd_state.sr_range = default_sr_range;
	cd_state.user_params = default_user_params;

	/* Radios are down until the Wi-Fi/SR drivers report power-up. */
	cd_state.wifi_up = false;
	cd_state.sr_up = false;
	cd_state.sr_coex_enabled = false;

	/*
	 * Register the callback for CM2CD events arriving over the Wi-Fi FMAC path.
	 * coex_event_handler() stores event headers/stats and signals cm_event_sem
	 * so coex_cd_cm_send_and_wait() can complete. This must be in place before
	 * the first CD2CM command is posted.
	 */
	(void)nrf71_wifi_coex_register_event_cb(coex_event_handler, NULL);

	return 0;
}

SYS_INIT(nrf71_sr_coex_init, APPLICATION, CONFIG_NRF71_SR_COEX_DRIVER_INIT_PRIORITY);
