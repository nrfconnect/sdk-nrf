/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief nRF71 Wi-Fi / Short-Range coexistence driver core.
 *
 * Host-side Coexistence Driver (CD) for the nRF71 combo SoC. It is the primary
 * coexistence control entity: it configures coexistence at startup through the
 * Coexistence Manager (CM), exposes the coex_cd_* APIs to the Short-Range and
 * Wi-Fi drivers, and invokes the coex_sr_* APIs on the Short-Range driver.
 *
 * The API contract (coex_cd_* / coex_sr_* and the CD2CM / CM2CD message types)
 * is defined by the firmware interface headers nrf71_coex_if.h (CD to CM) and
 * nrf71_cd_sr_if.h (CD to SR). See the "Coexistence Driver Implementation
 * Specification" (CAL-7316).
 *
 * Scope: this is the Phase 1 driver, aligned with the ROMed firmware interface.
 * It performs startup coexistence configuration (priority ranges, user and
 * internal parameters, enable) and radio power-down/up handling. Short-Range
 * Single Priority Window requests and Periodic Priority Window generation are
 * Phase 2 features and return -ENOTSUP.
 *
 * CM2CD events are delivered by the Wi-Fi FMAC event path
 * (NRF_WIFI_EVENT_COEX_CONFIG) calling nrf71_wifi_coex_on_event(), which is
 * dispatched to the handler this driver registers with the transport.
 */

#include <errno.h>
#include <string.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <drivers/wifi/nrf71/nrf71_wifi_coex.h>
#include <common/fw_if/nrf71_coex_if.h>
#include <common/fw_if/nrf71_cd_sr_if.h>

#include "nrf71_sr_coex_internal.h"

LOG_MODULE_REGISTER(nrf71_sr_coex, CONFIG_NRF71_SR_COEX_DRIVER_LOG_LEVEL);

/*
 * Default coexistence configuration applied at startup. The priority ranges and
 * protection probabilities mirror the reference values in the CM test bench
 * (coex_manager_tb.c); the integrator may override them at runtime via the
 * CD2CM_SET_PRIORITY_RANGES / CD2CM_UPDATE_COEX_USER_PARAMS paths once the
 * certified values are established.
 *
 * Ranges are {start, end, step}: start is the numerically largest value
 * (lowest priority), end the smallest (highest priority).
 */
static const struct coex_wifi_priority_range_t default_wifi_range = {
	.sw_request_priority_range = {10, 5, 1},
	.client0_ccconf_pti_range = {20, 15, 1},
	.client1_ccconf_pti_range = {25, 20, 1},
	.client2_ccconf_pti_range = {140, 120, 3},
	.client3_ccconf_pti_range = {160, 145, 3},
	.hw_client_priority_level = 5,
};

static const struct coex_sr_priority_range_t default_sr_range = {
	.sr_rx_client_ccconf_pti_range = {30, 25, 1},
	.sr_tx_client_ccconf_pti_range = {40, 30, 2},
	.sr_rx_client_critical_ccconf_pti_range = {20, 15, 1},
	.sr_tx_client_critical_ccconf_pti_range = {25, 20, 1},
	.client_priority_level = 5,
};

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

/* Driver runtime state (authoritative host-side coexistence state). */
static struct {
	struct k_mutex lock;

	bool wifi_up;
	bool sr_up;
	bool sr_coex_enabled;
	bool transition_active;

	struct coex_wifi_priority_range_t wifi_range;
	struct coex_sr_priority_range_t sr_range;
	struct coex_user_params_t user_params;

	/* Pending synchronous CD->CM request awaiting a CM2CD event. */
	struct {
		bool pending;
		enum cm_event_to_host_t expected;
	} cm_req;

	struct cm_stats_t last_stats;
} cd;

/* ---- Short-Range driver APIs (weak stubs until the SR driver provides them) ---- */

__weak unsigned int coex_sr_enable(unsigned int enable_coex)
{
	ARG_UNUSED(enable_coex);
	return 1U;
}

__weak unsigned int coex_sr_set_client_priority(
	const struct coex_sr_priority_range_t *sr_priority_range)
{
	ARG_UNUSED(sr_priority_range);
	return 1U;
}

/* ---- CM2CD event handling ---- */

static void coex_event_handler(void *ctx, const void *event, size_t len)
{
	ARG_UNUSED(ctx);

	k_mutex_lock(&cd.lock, K_FOREVER);

	if (!cd.cm_req.pending) {
		k_mutex_unlock(&cd.lock);
		LOG_DBG("Unexpected coex event (no pending request)");
		return;
	}

	switch (cd.cm_req.expected) {
	case STATISTICS_EVENT:
		if (len >= sizeof(struct cm_stats_t)) {
			memcpy(&cd.last_stats, event, sizeof(cd.last_stats));
		} else {
			LOG_WRN("Short statistics event (%zu bytes)", len);
		}
		break;
	default:
		break;
	}

	cd.cm_req.pending = false;
	k_mutex_unlock(&cd.lock);
}

/* ---- Helpers ---- */

/* Apply the coexistence configuration to the CM (and SR driver). No lock. */
static int cd_apply_cm_config(void)
{
	int ret;

	ret = coex_cm_set_priority_ranges(&cd.wifi_range, &cd.sr_range);
	if (ret) {
		return ret;
	}

	if (coex_sr_set_client_priority(&cd.sr_range) == 0U) {
		LOG_WRN("SR driver rejected priority ranges");
	}

	ret = coex_cm_update_user_params(&cd.user_params);
	if (ret) {
		return ret;
	}

	ret = coex_cm_update_coex_params();
	if (ret) {
		return ret;
	}

	return coex_cm_enable(true);
}

/* ---- CD APIs exposed to the Short-Range driver ---- */

int coex_cd_sr_software_client_request(const struct coex_sr_sw_client_params_t *client_params,
				       enum coex_sr_sw_client_req_status_t *grant_status)
{
	/*
	 * Short-Range Single Priority Window (SPW) requests are a Phase 2
	 * feature. The ROMed firmware interface has no CD2CM_SR_SW_CLIENT_REQUEST
	 * message, so the request cannot be forwarded to the CM yet.
	 */
	ARG_UNUSED(client_params);
	ARG_UNUSED(grant_status);

	return -ENOTSUP;
}

int coex_cd_update_short_range_activity_info(
	const struct short_range_activity_info_t *activity_info)
{
	/*
	 * Periodic Priority Window (PPW) generation is a Phase 2 feature and is
	 * not exercised against the ROMed firmware in this release.
	 */
	ARG_UNUSED(activity_info);

	return -ENOTSUP;
}

int coex_cd_sr_power_notify(enum coex_sr_power_event_t event)
{
	switch (event) {
	case COEX_SR_PREPARE_POWER_DOWN:
		k_mutex_lock(&cd.lock, K_FOREVER);
		if (cd.transition_active) {
			k_mutex_unlock(&cd.lock);
			return -EBUSY;
		}
		cd.transition_active = true;
		cd.sr_up = false;
		cd.sr_coex_enabled = false;
		cd.transition_active = false;
		k_mutex_unlock(&cd.lock);

		(void)coex_sr_enable(0U);
		return 0;

	case COEX_SR_POWERED_UP_READY:
		k_mutex_lock(&cd.lock, K_FOREVER);
		if (cd.transition_active) {
			k_mutex_unlock(&cd.lock);
			return -EBUSY;
		}
		cd.transition_active = true;
		k_mutex_unlock(&cd.lock);

		if (coex_sr_set_client_priority(&cd.sr_range) == 0U) {
			LOG_WRN("SR driver rejected priority ranges on power-up");
		}
		(void)coex_sr_enable(1U);

		k_mutex_lock(&cd.lock, K_FOREVER);
		cd.sr_up = true;
		/* New SR requests are accepted only when Wi-Fi/CM are also ready. */
		cd.sr_coex_enabled = cd.wifi_up && nrf71_wifi_coex_is_ready();
		cd.transition_active = false;
		k_mutex_unlock(&cd.lock);
		return 0;

	default:
		return -EINVAL;
	}
}

int coex_cd_wifi_power_notify(enum coex_wifi_power_event_t event)
{
	int ret;

	switch (event) {
	case COEX_WIFI_PREPARE_POWER_DOWN:
		k_mutex_lock(&cd.lock, K_FOREVER);
		if (cd.transition_active) {
			k_mutex_unlock(&cd.lock);
			return -EBUSY;
		}
		cd.transition_active = true;
		cd.wifi_up = false;
		cd.sr_coex_enabled = false;
		cd.transition_active = false;
		k_mutex_unlock(&cd.lock);
		return 0;

	case COEX_WIFI_POWERED_UP_READY:
		k_mutex_lock(&cd.lock, K_FOREVER);
		if (cd.transition_active) {
			k_mutex_unlock(&cd.lock);
			return -EBUSY;
		}
		cd.transition_active = true;
		k_mutex_unlock(&cd.lock);

		/* Restore CM configuration after Wi-Fi/RPU is ready. */
		ret = cd_apply_cm_config();

		k_mutex_lock(&cd.lock, K_FOREVER);
		cd.wifi_up = (ret == 0);
		cd.sr_coex_enabled = cd.wifi_up && cd.sr_up;
		cd.transition_active = false;
		k_mutex_unlock(&cd.lock);

		return (ret == 0) ? 0 : -EIO;

	default:
		return -EINVAL;
	}
}

/* ---- Initialisation ---- */

static int nrf71_sr_coex_init(void)
{
	int ret;

	k_mutex_init(&cd.lock);

	cd.wifi_range = default_wifi_range;
	cd.sr_range = default_sr_range;
	cd.user_params = default_user_params;

	(void)nrf71_wifi_coex_register_event_cb(coex_event_handler, NULL);

	/* The Wi-Fi driver initialises at POST_KERNEL; this runs at APPLICATION
	 * level. The RPU/transport may still be brought up asynchronously, so
	 * apply the configuration best-effort and let coex_cd_wifi_power_notify()
	 * re-apply it once Wi-Fi signals ready.
	 */
	cd.wifi_up = nrf71_wifi_coex_is_ready();
	cd.sr_up = false;
	cd.sr_coex_enabled = false;

	if (cd.wifi_up) {
		ret = cd_apply_cm_config();
		if (ret) {
			LOG_WRN("Deferred coex config (%d); will retry on Wi-Fi power-up", ret);
			cd.wifi_up = false;
		} else {
			LOG_INF("nRF71 SR coexistence configured");
		}
	} else {
		LOG_DBG("Wi-Fi transport not ready; coex config deferred");
	}

	return 0;
}

SYS_INIT(nrf71_sr_coex_init, APPLICATION, CONFIG_NRF71_SR_COEX_DRIVER_INIT_PRIORITY);
