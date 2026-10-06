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
 *   CM sends CM2CD_* event -> coex_event_handler() stores the result.
 *
 * Statistics are special: the actual numbers are copied into cd_state inside
 * coex_event_handler() when STATISTICS_EVENT arrives.
 *
 * Locking rules (important when adding code):
 *   - cd_state.lock guards every cd_state field.
 */

#include <errno.h>
#include <string.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

/* Wi-Fi FMAC coexistence transport (cmd/event access). */
#include <drivers/wifi/nrf71/nrf71_wifi_coex.h>
/* CD2CM / CM2CD message structures shared with RPU firmware. */
#include <nrf71_coex_if.h>
/* CD to Short-Range driver interface. */
#include <nrf71_cd_sr_if.h>

#include "nrf71_sr_coex_internal.h"

LOG_MODULE_REGISTER(nrf71_sr_coex, CONFIG_NRF71_SR_COEX_DRIVER_LOG_LEVEL);

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
 *   - Working copies of the CM configuration (wifi_range, sr_range, user_params)
 *   - Pending CD2CM request awaiting a CM2CD event (cm_req)
 *   - Cached statistics from the last GET_STATS (last_stats)
 */
static struct {
	struct k_mutex lock;       /* Protects every cd_state field below. */

	bool wifi_up;              /* Wi-Fi reported powered-up and CM configured. */
	bool sr_up;                /* SR reported powered-up. */
	bool sr_coex_enabled;      /* true when both radios up and coex may run. */

	bool transition_active;    /* Guard against overlapping power notifications. */

	struct coex_wifi_priority_range_t wifi_range; /* Working copy of Wi-Fi ranges. */
	struct coex_sr_priority_range_t sr_range;     /* Working copy of SR ranges. */
	struct coex_user_params_t user_params;        /* Working copy of user params. */

	/* Pending synchronous CD->CM request awaiting a CM2CD event. */
	struct {
		bool pending;
		enum cm_event_to_host_t expected;
	} cm_req;

	struct cm_stats_t last_stats;                 /* ROM CM statistics payload. */
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

/* ---- CM2CD event handling ---- */

/**
 * CM2CD event callback registered with the Wi-Fi FMAC coexistence path.
 *
 * Runs in Wi-Fi driver context when the Coexistence Manager sends an event.
 * Completes the pending request and copies statistics if present.
 */
static void coex_event_handler(void *ctx, const void *event, size_t len)
{
	ARG_UNUSED(ctx);

	k_mutex_lock(&cd_state.lock, K_FOREVER);

	if (!cd_state.cm_req.pending) {
		k_mutex_unlock(&cd_state.lock);
		LOG_DBG("Unexpected coex event (no pending request)");
		return;
	}

	switch (cd_state.cm_req.expected) {
	case STATISTICS_EVENT:
		if (len >= sizeof(struct cm_stats_t)) {
			memcpy(&cd_state.last_stats, event, sizeof(cd_state.last_stats));
		} else {
			LOG_WRN("Short statistics event (%zu bytes)", len);
		}
		break;
	default:
		break;
	}

	cd_state.cm_req.pending = false;
	k_mutex_unlock(&cd_state.lock);
}

/* ---- Helper functions ---- */

/**
 * Push the current coexistence settings to the Coexistence Manager (CM).
 *
 * Full bring-up sequence, run on every
 * coex_cd_wifi_power_notify(COEX_WIFI_POWERED_UP_READY). Order matters: CM
 * configuration first, then enable.
 *
 * On failure the CM is left partially configured. The caller is responsible for
 * clearing the runtime flags so the whole sequence is retried on the next Wi-Fi power-up.
 */
static int cd_apply_cm_config(void)
{
	int ret;

	/* Step 1: CD2CM_SET_PRIORITY_RANGES, plus the SR-driver mirror. */
	ret = coex_cm_set_priority_ranges(&cd_state.wifi_range, &cd_state.sr_range);
	if (ret != 0) {
		return ret;
	}

	/*
	 * Mirror the SR ranges into the SR driver so its hardware uses matching PTI
	 * values. Non-fatal if SR rejects them: the CM already has the ranges.
	 */
	if (coex_sr_set_client_priority(&cd_state.sr_range) == 0U) {
		LOG_WRN("SR driver rejected priority ranges");
	}

	/* Step 2: CD2CM_UPDATE_COEX_USER_PARAMS (protection probabilities, etc.). */
	ret = coex_cm_update_user_params(&cd_state.user_params);
	if (ret != 0) {
		return ret;
	}

	/* Step 3: CD2CM_UPDATE_COEX_PARAMS with the default NRF_COEX_PARAMS blob. */
	ret = coex_cm_update_coex_params();
	if (ret != 0) {
		return ret;
	}

	/* Step 4: CD2CM_ENABLE_COEXISTENCE. */
	return coex_cm_enable(true);
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
 * ranges, user params, coex params and the enable have to be sent again.
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
		 * Wi-Fi/RPU is shutting down: block new coexistence activity.
		 */
		k_mutex_lock(&cd_state.lock, K_FOREVER);
		if (cd_state.transition_active) {
			k_mutex_unlock(&cd_state.lock);
			return -EBUSY;
		}
		cd_state.transition_active = true;
		cd_state.wifi_up = false;
		cd_state.sr_coex_enabled = false;
		cd_state.transition_active = false;
		k_mutex_unlock(&cd_state.lock);
		return 0;

	case COEX_WIFI_POWERED_UP_READY:
		/*
		 * Wi-Fi transport and RPU are back: restore the full coexistence setup.
		 * cd_apply_cm_config() runs outside cd_state.lock because it posts
		 * CD2CM commands.
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
			cd_state.wifi_up = false;
			cd_state.sr_coex_enabled = false;
		}
		cd_state.transition_active = false;
		k_mutex_unlock(&cd_state.lock);

		return (ret == 0) ? 0 : -EIO;

	default:
		return -EINVAL;
	}
}

/* ---- Initialisation ---- */

/**
 * Driver init, run at APPLICATION level after the Wi-Fi driver.
 *
 *   - Initialise the cd_state mutex
 *   - Load the default priority ranges and user params
 *   - Register coex_event_handler() for all incoming CM2CD events
 *   - Apply the CM configuration if the Wi-Fi transport is already up
 */
static int nrf71_sr_coex_init(void)
{
	int ret;

	k_mutex_init(&cd_state.lock);

	/* Working copies used by cd_apply_cm_config(). */
	cd_state.wifi_range = default_wifi_range;
	cd_state.sr_range = default_sr_range;
	cd_state.user_params = default_user_params;

	/*
	 * Register the callback for CM2CD events arriving over the Wi-Fi FMAC path.
	 * coex_event_handler() completes the pending request and stores statistics.
	 * This must be in place before the first CD2CM command is posted.
	 */
	(void)nrf71_wifi_coex_register_event_cb(coex_event_handler, NULL);

	/*
	 * The Wi-Fi driver initialises at POST_KERNEL; this runs at APPLICATION
	 * level. The RPU/transport may still be brought up asynchronously, so
	 * apply the configuration best-effort and let coex_cd_wifi_power_notify()
	 * re-apply it once Wi-Fi signals ready.
	 */
	cd_state.wifi_up = nrf71_wifi_coex_is_ready();
	cd_state.sr_up = false;
	cd_state.sr_coex_enabled = false;

	if (cd_state.wifi_up) {
		ret = cd_apply_cm_config();
		if (ret != 0) {
			LOG_WRN("Deferred coex config (%d); will retry on Wi-Fi power-up", ret);
			cd_state.wifi_up = false;
		} else {
			LOG_INF("nRF71 SR coexistence configured");
		}
	} else {
		LOG_DBG("Wi-Fi transport not ready; coex config deferred");
	}

	return 0;
}

SYS_INIT(nrf71_sr_coex_init, APPLICATION, CONFIG_NRF71_SR_COEX_DRIVER_INIT_PRIORITY);
