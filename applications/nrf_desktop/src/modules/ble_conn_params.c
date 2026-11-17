/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>
#include <bluetooth/gatt_dm.h>

#ifdef CONFIG_CAF_BLE_USE_LLPM
#include <bluetooth/hci_vs_sdc.h>
#endif /* CONFIG_CAF_BLE_USE_LLPM */

#define MODULE ble_conn_params
#include <caf/events/module_state_event.h>
#include <caf/events/ble_common_event.h>
#include "ble_event.h"

#include "usb_event.h"
#include "hogp_event.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DESKTOP_BLE_CONN_PARAMS_LOG_LEVEL);

#define CONN_INTERVAL_LLPM_US		1000   /* 1 ms */
#if (CONFIG_CAF_BLE_USE_LLPM && (CONFIG_BT_MAX_CONN >= 2))
 #define CONN_INTERVAL_BLE_US		10000 /* 10 ms */
#else
 #define CONN_INTERVAL_BLE_US		7500 /* 7.5 ms */
#endif
#define CONN_SUPERVISION_TIMEOUT	400

#define CONN_PARAMS_ERROR_TIMEOUT	K_MSEC(100)

#define CONN_INTERVAL_PRE_LLPM_MAX_US  10000U /* SoftDevice Controller Limitation DRGN-11297 */

#define CONN_INTERVAL_MS_TO_REG(_x)    (((_x) * USEC_PER_MSEC) / 1250U) /* REG = MS / 1,25 ms */
#define CONN_INTERVAL_USB_SUSPEND CONN_INTERVAL_MS_TO_REG(CONFIG_DESKTOP_BLE_USB_MANAGED_CI_VALUE)
#define CONN_LATENCY_USB_SUSPEND CONFIG_DESKTOP_BLE_USB_MANAGED_LATENCY_VALUE

#define SCI_ALLOWED_RANGE_UPDATE_RETRY_TIMEOUT	K_SECONDS(2)

struct connected_peer {
	struct bt_conn *conn;
	bool discovered;
	bool use_llpm;
	bool use_sci;
	uint16_t requested_latency;
	bool conn_param_update_pending;
	bool sci_params_range_update_in_progress;
	bool sci_min_interval_update_pending;
};

static struct connected_peer peers[CONFIG_BT_MAX_CONN];
static bool usb_suspended;

static void conn_params_update_fn(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(conn_params_update, conn_params_update_fn);

static void sci_allowed_range_update_retry_fn(struct k_work *work);
static struct k_work_delayable sci_allowed_range_update_retry;

static struct connected_peer *find_connected_peer(const struct bt_conn *conn)
{
	for (size_t i = 0; i < ARRAY_SIZE(peers); i++) {
		if (peers[i].conn == conn) {
			return &peers[i];
		}
	}

	return NULL;
}

static int set_le_conn_param(struct bt_conn *conn, uint16_t interval, uint16_t latency)
{
	struct bt_le_conn_param param = {
		.interval_min = interval,
		.interval_max = interval,
		.latency = latency,
		.timeout = CONN_SUPERVISION_TIMEOUT,
	};

	return bt_conn_le_param_update(conn, &param);
}

/* Handle llpm encoding in interval_us values */
static int strip_llpm_encoding_to_us(uint32_t interval_us)
{
	uint16_t reg = BT_GAP_US_TO_CONN_INTERVAL(interval_us);
	bool is_llpm = ((reg & 0x0d00) == 0x0d00) ? true : false;

	return (reg & BIT_MASK(8)) * ((is_llpm) ? 1000 : 1250);
}

static int set_llpm_conn_param(struct bt_conn *conn, uint16_t latency)
{
#ifdef CONFIG_CAF_BLE_USE_LLPM
	sdc_hci_cmd_vs_conn_update_t cmd_conn_update;
	uint16_t conn_handle;

	int err = bt_hci_get_conn_handle(conn, &conn_handle);

	if (err) {
		LOG_ERR("Failed obtaining conn_handle (err:%d)", err);
		return err;
	}

	cmd_conn_update.conn_handle = conn_handle;
	cmd_conn_update.conn_interval_us = CONN_INTERVAL_LLPM_US;
	cmd_conn_update.conn_latency = latency;
	cmd_conn_update.supervision_timeout = CONN_SUPERVISION_TIMEOUT;

	return hci_vs_sdc_conn_update(&cmd_conn_update);
#else
	__ASSERT_NO_MSG(false);
	return -ENOTSUP;
#endif /* CONFIG_CAF_BLE_USE_LLPM */
}

static int set_conn_params(struct connected_peer *peer)
{
	int err;

	__ASSERT_NO_MSG(peer->conn);

	if (IS_ENABLED(CONFIG_DESKTOP_BLE_USB_MANAGED_CI) && usb_suspended) {
		err = set_le_conn_param(peer->conn, CONN_INTERVAL_USB_SUSPEND,
					CONN_LATENCY_USB_SUSPEND);
	} else if (peer->use_llpm) {
		__ASSERT_NO_MSG(IS_ENABLED(CONFIG_CAF_BLE_USE_LLPM));

		struct bt_conn_info info;

		err = bt_conn_get_info(peer->conn, &info);
		if (err) {
			LOG_ERR("Cannot get conn info (%d)", err);
			return err;
		}
		uint32_t curr_ci_us = strip_llpm_encoding_to_us(info.le.interval_us);

		if (curr_ci_us > CONN_INTERVAL_PRE_LLPM_MAX_US) {
			err = set_le_conn_param(peer->conn,
						BT_GAP_US_TO_CONN_INTERVAL(CONN_INTERVAL_BLE_US),
						peer->requested_latency);
		} else {
			err = set_llpm_conn_param(peer->conn, peer->requested_latency);
		}
	} else {
		err = set_le_conn_param(peer->conn,
					BT_GAP_US_TO_CONN_INTERVAL(CONN_INTERVAL_BLE_US),
					peer->requested_latency);
	}

	if (!err) {
		peer->conn_param_update_pending = true;
	}

	return err;
}

static void update_peer_conn_params(struct connected_peer *peer)
{
	__ASSERT_NO_MSG(peer);

	/* Do not update peripheral's connection parameters before the discovery
	 * is completed or when conn param update is already pending.
	 */
	if (!peer->discovered || peer->conn_param_update_pending) {
		return;
	}

	int err = set_conn_params(peer);

	if (err) {
		LOG_WRN("Cannot update conn parameters for peer %p (err:%d)",
			(void *)peer->conn, err);
		/* Retry to update the connection parameters after an error. */
		(void)k_work_reschedule(&conn_params_update, CONN_PARAMS_ERROR_TIMEOUT);
	} else {
		LOG_INF("Update conn params for peer: %p (%s, requested latency: %" PRIu16
			", USB suspended: %s)",
			(void *)peer->conn,
			peer->use_llpm ? "LLPM" : "BLE",
			peer->requested_latency,
			IS_ENABLED(CONFIG_DESKTOP_BLE_USB_MANAGED_CI) && usb_suspended ?
				"true" : "false");
	}
}

static bool conn_params_update_required(struct connected_peer *peer)
{
	if (!peer->conn) {
		return false;
	}

	if (IS_ENABLED(CONFIG_DESKTOP_BLE_CONN_PARAMS_HID_SCI_ENABLE) && peer->use_sci) {
		/* Parameters update with HID SCI is performed by requesting
		 * the appropriate SCI mode.
		 */
		return false;
	}

	struct bt_conn_info info;
	int err = bt_conn_get_info(peer->conn, &info);

	if (err) {
		LOG_ERR("Cannot get conn info (%d)", err);
		return true;
	}

	__ASSERT_NO_MSG(info.role == BT_CONN_ROLE_CENTRAL);

	uint32_t interval_us = strip_llpm_encoding_to_us(info.le.interval_us);
	if (IS_ENABLED(CONFIG_DESKTOP_BLE_USB_MANAGED_CI) && usb_suspended) {
		if ((interval_us != BT_CONN_INTERVAL_TO_US(CONN_INTERVAL_USB_SUSPEND)) ||
		    (info.le.latency != CONN_LATENCY_USB_SUSPEND)) {
			return true;
		}
	} else if ((peer->use_llpm && (interval_us != CONN_INTERVAL_LLPM_US)) ||
		   (!peer->use_llpm && (interval_us != CONN_INTERVAL_BLE_US)) ||
		   (info.le.latency != peer->requested_latency)) {
		return true;
	}

	return false;
}

static void conn_params_update_fn(struct k_work *work)
{
	__ASSERT_NO_MSG(work != NULL);

	for (size_t i = 0; i < ARRAY_SIZE(peers); i++) {
		if (conn_params_update_required(&peers[i])) {
			update_peer_conn_params(&peers[i]);
		}
	}
}

static void request_sci_mode(struct bt_conn *conn, enum bt_hids_sci_mode_value mode)
{
	/* Currently we assume the happy path always works and the mode which
	 * is requested is the one that the peripheral will use.
	 */
	struct hogp_sci_mode_req_event *sci_event = new_hogp_sci_mode_req_event();

	sci_event->conn = conn;
	sci_event->mode = mode;
	APP_EVENT_SUBMIT(sci_event);
}

static void preferred_sci_mode_request(struct connected_peer *peer)
{
	if (peer->sci_params_range_update_in_progress) {
		/* The mode change request is postponed until
		 * the connection rate update completes.
		 */
		return;
	}

	if (IS_ENABLED(CONFIG_DESKTOP_BLE_USB_MANAGED_CI) && usb_suspended) {
		LOG_INF("Request LOW POWER SCI mode for peer %p",
			(void *)peer->conn);
		request_sci_mode(peer->conn, BT_HIDS_SCI_MODE_LOW_POWER);
	} else {
		/* Note: currently the dongle only uses FAST SCI mode
		 * for "active" operation.
		 * If this changes in the future, the dongle will need to
		 * cache the SCI mode and restore it when the USB is resumed.
		 *
		 * Also note that this dongle works under the assumption that
		 * it is the peripheral's responsibility to either not change
		 * the HID SCI mode or re-request LOW_POWER
		 * mode if it wants to remain in the LOW_POWER mode.
		 */
		LOG_INF("Request FAST SCI mode for peer %p",
			(void *)peer->conn);
		request_sci_mode(peer->conn, BT_HIDS_SCI_MODE_FAST);
	}
}

static void sci_usb_state_change_handler(bool *non_sci_peers_present)
{
	*non_sci_peers_present = false;

	for (size_t i = 0; i < ARRAY_SIZE(peers); i++) {
		struct connected_peer *peer = &peers[i];

		if (!peer->conn) {
			continue;
		}

		if (!peer->use_sci) {
			*non_sci_peers_present = true;
			continue;
		}

		preferred_sci_mode_request(peer);
	}
}

static void ble_peer_conn_params_event_handler(const struct ble_peer_conn_params_event *event)
{
	struct connected_peer *peer = find_connected_peer(event->id);

	__ASSERT_NO_MSG(peer);

	if (IS_ENABLED(CONFIG_DESKTOP_BLE_CONN_PARAMS_HID_SCI_ENABLE) && peer->use_sci) {
		LOG_WRN("Unexpected connection parameters event for HID SCI peer %p (updated: %s)",
			(void *)peer->conn, event->updated ? "true" : "false");
		return;
	}

	if (event->updated) {
		peer->conn_param_update_pending = false;
		LOG_INF("Conn params for peer: %p updated.", (void *)peer->conn);
	} else {
		peer->requested_latency = event->latency;
		LOG_INF("Request to update conn: %p latency to: %"PRIu16, event->id,
			event->latency);
	}

	(void)k_work_reschedule(&conn_params_update, K_NO_WAIT);
}

static void usb_state_event_handler(enum usb_state new_state)
{
	switch (new_state) {
	case USB_STATE_SUSPENDED:
		usb_suspended = true;
		break;

	case USB_STATE_DISCONNECTED:
	case USB_STATE_ACTIVE:
		usb_suspended = false;
		break;

	default:
		/* Ignore other USB state events */
		return;
	}

	bool non_sci_peers_present = true;

	if (IS_ENABLED(CONFIG_DESKTOP_BLE_CONN_PARAMS_HID_SCI_ENABLE)) {
		sci_usb_state_change_handler(&non_sci_peers_present);
	}

	if (non_sci_peers_present) {
		/* This would be a no-op if all peers use HID SCI. */
		(void)k_work_reschedule(&conn_params_update, K_NO_WAIT);
	}
}

static uint8_t get_sci_peer_count(void)
{
	size_t sci_peer_count = 0;

	for (size_t i = 0; i < ARRAY_SIZE(peers); i++) {
		if (peers[i].conn && peers[i].use_sci) {
			sci_peer_count++;
		}
	}

	return sci_peer_count;
}

#if CONFIG_DESKTOP_BLE_CONN_PARAMS_HID_SCI_ENABLE
BUILD_ASSERT((CONFIG_DESKTOP_BLE_CONN_PARAMS_SCI_INTERVAL_MIN_125US * CONFIG_BT_MAX_CONN) <=
	     UINT16_MAX,
	     "Minimum SCI connection interval times max connections exceeds uint16_t");
BUILD_ASSERT((CONFIG_DESKTOP_BLE_CONN_PARAMS_SCI_INTERVAL_MIN_125US * CONFIG_BT_MAX_CONN) <=
	     CONFIG_DESKTOP_BLE_CONN_PARAMS_SCI_INTERVAL_MAX_125US,
	     "Minimum SCI connection interval times max connections exceeds configured maximum "
	     "connection interval. Connection interval scaling will not be possible.");
#endif

static int get_allowed_sci_conn_rate_params(uint8_t sci_peers_count,
					    struct bt_conn_le_conn_rate_param *params)
{
	__ASSERT_NO_MSG(sci_peers_count > 0);

#if CONFIG_DESKTOP_BLE_CONN_PARAMS_HID_SCI_ENABLE
	static const uint16_t sci_interval_min_125us =
		CONFIG_DESKTOP_BLE_CONN_PARAMS_SCI_INTERVAL_MIN_125US;
	static const uint16_t sci_interval_max_125us =
		CONFIG_DESKTOP_BLE_CONN_PARAMS_SCI_INTERVAL_MAX_125US;
	static const uint16_t sci_subrate_min =
		CONFIG_DESKTOP_BLE_CONN_PARAMS_SCI_SUBRATE_MIN;
	static const uint16_t sci_subrate_max =
		CONFIG_DESKTOP_BLE_CONN_PARAMS_SCI_SUBRATE_MAX;
	static const uint16_t sci_max_latency =
		CONFIG_DESKTOP_BLE_CONN_PARAMS_SCI_MAX_LATENCY;
	static const uint16_t sci_continuation_num =
		CONFIG_DESKTOP_BLE_CONN_PARAMS_SCI_CONTINUATION_NUM;
	static const uint16_t sci_supervision_timeout_10ms =
		CONFIG_DESKTOP_BLE_CONN_PARAMS_SCI_SUPERVISION_TIMEOUT_10MS;
#else /* CONFIG_DESKTOP_BLE_CONN_PARAMS_HID_SCI_ENABLE */
	/* SCI Kconfig symbols are unavailable when HID SCI is disabled; use placeholders so
	 * this function still compiles (it is never called in that case).
	 */
	static const uint16_t sci_interval_min_125us;
	static const uint16_t sci_interval_max_125us;
	static const uint16_t sci_subrate_min;
	static const uint16_t sci_subrate_max;
	static const uint16_t sci_max_latency;
	static const uint16_t sci_continuation_num;
	static const uint16_t sci_supervision_timeout_10ms;

	__ASSERT_NO_MSG(false);
#endif /* CONFIG_DESKTOP_BLE_CONN_PARAMS_HID_SCI_ENABLE */

	int err;
	uint16_t local_min_interval_us = 0;
	uint16_t interval_min_125us = sci_interval_min_125us * sci_peers_count;

	err = bt_conn_le_read_min_conn_interval(&local_min_interval_us);
	if (!err) {
		__ASSERT_NO_MSG(local_min_interval_us % 125U == 0);
		__ASSERT_NO_MSG(local_min_interval_us != 0);

		if (BT_CONN_SCI_INTERVAL_TO_US(sci_interval_min_125us) < local_min_interval_us) {
			LOG_WRN("Configured minimum connection interval (%u us) is below "
				"controller minimum (%u us); using %u * %u (peers count) = %u us",
				BT_CONN_SCI_INTERVAL_TO_US(sci_interval_min_125us),
				local_min_interval_us,
				local_min_interval_us, sci_peers_count,
				local_min_interval_us * sci_peers_count);

			interval_min_125us = (local_min_interval_us / 125U) * sci_peers_count;
		}
		if (interval_min_125us > sci_interval_max_125us) {
			LOG_WRN("Calculated minimum connection interval (%u us) "
				"is larger than configured maximum (%u us); using %u us",
				BT_CONN_SCI_INTERVAL_TO_US(interval_min_125us),
				BT_CONN_SCI_INTERVAL_TO_US(sci_interval_max_125us),
				BT_CONN_SCI_INTERVAL_TO_US(sci_interval_max_125us));
			interval_min_125us = sci_interval_max_125us;
		}

	} else {
		LOG_ERR("Failed to read min conn interval (err %d)", err);
	}

	params->interval_min_125us = interval_min_125us;
	params->interval_max_125us = sci_interval_max_125us;
	params->subrate_min = sci_subrate_min;
	params->subrate_max = sci_subrate_max;
	params->max_latency = sci_max_latency;
	params->continuation_number = sci_continuation_num;
	params->supervision_timeout_10ms = sci_supervision_timeout_10ms;
	params->min_ce_len_125us = BT_HCI_LE_SCI_CE_LEN_MIN_125US;
	params->max_ce_len_125us = BT_HCI_LE_SCI_CE_LEN_MAX_125US;

	return err;
}

static void update_single_sci_peer_allowed_range(struct connected_peer *peer,
	const struct bt_conn_le_conn_rate_param *conn_rate_params)
{
	if (peer->conn && peer->use_sci) {
		if (peer->sci_params_range_update_in_progress) {
			peer->sci_min_interval_update_pending = true;
			return;
		}

		int err = bt_conn_le_conn_rate_request(peer->conn, conn_rate_params);

		if (!err) {
			peer->sci_params_range_update_in_progress = true;
		} else {
			LOG_WRN("Failed to set conn rate params for peer %p (err %d)",
				(void *)peer->conn, err);
			preferred_sci_mode_request(peer);

			/* Reattempt to update the allowed SCI range for this peer after some time
			 * so that the peripheral can use the proper parameters if possible.
			 */
			peer->sci_min_interval_update_pending = true;
			k_work_reschedule(&sci_allowed_range_update_retry,
					  SCI_ALLOWED_RANGE_UPDATE_RETRY_TIMEOUT);
		}
	}
}

static void update_sci_peers_allowed_range(void)
{
	struct bt_conn_le_conn_rate_param conn_rate_params;

	int err = get_allowed_sci_conn_rate_params(get_sci_peer_count(), &conn_rate_params);

	if (err) {
		return;
	}

	for (size_t i = 0; i < ARRAY_SIZE(peers); i++) {
		update_single_sci_peer_allowed_range(&peers[i], &conn_rate_params);
	}
}

static void sci_allowed_range_update_retry_fn(struct k_work *work)
{
	/* Unused parameter */
	(void) work;

	struct bt_conn_le_conn_rate_param conn_rate_params;
	int err = get_allowed_sci_conn_rate_params(get_sci_peer_count(), &conn_rate_params);

	if (err) {
		return;
	}

	for (size_t i = 0; i < ARRAY_SIZE(peers); i++) {
		if (peers[i].sci_min_interval_update_pending) {
			peers[i].sci_min_interval_update_pending = false;
			update_single_sci_peer_allowed_range(&peers[i], &conn_rate_params);
		}
	}
}

static void ble_peer_sci_conn_rate_event_handler(const struct ble_peer_sci_conn_rate_event *event)
{
	struct connected_peer *peer = find_connected_peer(event->id);

	if (!peer) {
		return;
	}

	if (!peer->sci_params_range_update_in_progress) {
		/* This event originated from a peripheral request, ignore it. */
		return;
	}

	peer->sci_params_range_update_in_progress = false;

	__ASSERT_NO_MSG(peer->use_sci);

	if (event->status != BT_HCI_ERR_SUCCESS) {
		LOG_ERR("Failed to set conn rate params for peer %p (err 0x%02" PRIx8 ")",
			(void *)peer->conn, event->status);
		/* Still try to request the preferred SCI mode on the peripheral,
		 * even though the parameters might not be optimal.
		 */
	}

	if ((event->status == BT_HCI_ERR_LL_PROC_COLLISION) ||
	    (event->status == BT_HCI_ERR_DIFF_TRANS_COLLISION)) {
		/* Conn rate request failed due to a collision with another
		 * procedure. Retry.
		 */
		peer->sci_min_interval_update_pending = true;
	}

	if (peer->sci_min_interval_update_pending) {
		struct bt_conn_le_conn_rate_param conn_rate_params;
		int err;

		peer->sci_min_interval_update_pending = false;

		err = get_allowed_sci_conn_rate_params(get_sci_peer_count(), &conn_rate_params);
		if (!err) {
			update_single_sci_peer_allowed_range(peer, &conn_rate_params);
		}
	} else {
		/* As the Connection Rate Update Request came directly from the central
		 * (not through the HID Control Point SCI Mode request), the peripheral
		 * is currently in the out-of-spec state.
		 * Request the preferred SCI mode to clear the out-of-spec state
		 * on the peripheral so that it resumes normal operation.
		 */
		preferred_sci_mode_request(peer);
	}
}

static void peer_connected(struct bt_conn *conn)
{
	struct connected_peer *new_peer = NULL;

	for (size_t i = 0; i < ARRAY_SIZE(peers); i++) {
		if (!peers[i].conn) {
			new_peer = &peers[i];
			break;
		}
	}
	__ASSERT_NO_MSG(new_peer);

	new_peer->conn = conn;
}

static void peer_disconnected(struct bt_conn *conn)
{
	struct connected_peer *peer = find_connected_peer(conn);

	if (peer) {
		peer->conn = NULL;
		peer->use_llpm = false;
		peer->use_sci = false;
		peer->discovered = false;
		peer->requested_latency = 0;
		peer->conn_param_update_pending = false;
		peer->sci_params_range_update_in_progress = false;
		peer->sci_min_interval_update_pending = false;
	}

	if (IS_ENABLED(CONFIG_DESKTOP_BLE_CONN_PARAMS_HID_SCI_ENABLE) &&
	    get_sci_peer_count() > 0) {
		update_sci_peers_allowed_range();
	}
}

static void peer_discovered(struct bt_conn *conn, bool peer_llpm_support, bool peer_sci_support)
{
	struct connected_peer *peer = find_connected_peer(conn);

	if (peer) {
		/* Note: if a nRF Desktop peripheral supports both HID SCI and LLPM,
		 * HID SCI will take precedence.
		 * Currently no nRF Desktop peripheral supports both HID SCI and LLPM,
		 * however the code is present for better compatibility with future implementations.
		 */
		peer->use_sci = IS_ENABLED(CONFIG_DESKTOP_BLE_CONN_PARAMS_HID_SCI_ENABLE)
				&& peer_sci_support;
		peer->use_llpm = IS_ENABLED(CONFIG_CAF_BLE_USE_LLPM) && peer_llpm_support
				 && !peer->use_sci;
		peer->discovered = true;
		(void)k_work_reschedule(&conn_params_update, K_NO_WAIT);

		if (IS_ENABLED(CONFIG_DESKTOP_BLE_CONN_PARAMS_HID_SCI_ENABLE) && peer->use_sci) {
			if (get_sci_peer_count() > 1) {
				update_sci_peers_allowed_range();
			} else {
				preferred_sci_mode_request(peer);
			}
		}
	}
}

static void hogp_sci_mode_changed_event_handler(const struct hogp_sci_mode_changed_event *event)
{
	struct connected_peer *peer = find_connected_peer(event->conn);

	if (!peer) {
		return;
	}

	__ASSERT_NO_MSG(peer->use_sci);

	LOG_INF("Peer %p SCI mode: 0x%02" PRIx8, (void *)peer->conn, (uint8_t)event->mode);

	if (IS_ENABLED(CONFIG_DESKTOP_BLE_USB_MANAGED_CI) && usb_suspended &&
	    (event->mode != BT_HIDS_SCI_MODE_LOW_POWER)) {
		/* A peer may initiate a HID SCI mode change by itself (for example
		 * while waking up from power down state).
		 * Immediately switch back to LOW_POWER SCI mode to avoid excessive power
		 * consumption.
		 */
		LOG_INF("Peer %p switched out of LOW_POWER mode while in USB suspend.",
			(void *)peer->conn);
		LOG_INF("Requesting LOW_POWER SCI mode for peer %p", (void *)peer->conn);
		request_sci_mode(peer->conn, BT_HIDS_SCI_MODE_LOW_POWER);
	}
}

static int set_default_sci_conn_params(void)
{
	struct bt_conn_le_conn_rate_param params;

	(void)get_allowed_sci_conn_rate_params(1, &params);

	return bt_conn_le_conn_rate_set_defaults(&params);
}

static bool app_event_handler(const struct app_event_header *aeh)
{
	if (is_module_state_event(aeh)) {
		const struct module_state_event *event =
			cast_module_state_event(aeh);

		if (check_state(event, MODULE_ID(ble_state), MODULE_STATE_READY)) {
			static bool initialized;

			__ASSERT_NO_MSG(!initialized);
			initialized = true;

			if (IS_ENABLED(CONFIG_DESKTOP_BLE_CONN_PARAMS_HID_SCI_ENABLE)) {
				k_work_init_delayable(&sci_allowed_range_update_retry,
						      sci_allowed_range_update_retry_fn);

				int err = set_default_sci_conn_params();

				if (err) {
					LOG_ERR("Failed to set default conn rate params (err %d)",
						err);
					module_set_state(MODULE_STATE_ERROR);
					return false;
				}
			}

			module_set_state(MODULE_STATE_READY);
		}

		return false;
	}

	if (is_ble_discovery_complete_event(aeh)) {
		const struct ble_discovery_complete_event *event =
			cast_ble_discovery_complete_event(aeh);

		peer_discovered(bt_gatt_dm_conn_get(event->dm), event->peer_llpm_support,
				event->peer_sci_support);

		return false;
	}

	if (is_ble_peer_event(aeh)) {
		const struct ble_peer_event *event =
			cast_ble_peer_event(aeh);

		if (event->state == PEER_STATE_CONNECTED) {
			peer_connected(event->id);
		} else if (event->state == PEER_STATE_DISCONNECTED) {
			peer_disconnected(event->id);
		}

		return false;
	}

	if (is_ble_peer_conn_params_event(aeh)) {
		ble_peer_conn_params_event_handler(
			cast_ble_peer_conn_params_event(aeh));

		return false;
	}

	if (IS_ENABLED(CONFIG_DESKTOP_BLE_USB_MANAGED_CI) &&
	    is_usb_state_event(aeh)) {
		struct usb_state_event *event = cast_usb_state_event(aeh);

		usb_state_event_handler(event->state);

		return false;
	}

	if (IS_ENABLED(CONFIG_DESKTOP_BLE_CONN_PARAMS_HID_SCI_ENABLE) &&
	    is_hogp_sci_mode_changed_event(aeh)) {
		hogp_sci_mode_changed_event_handler(cast_hogp_sci_mode_changed_event(aeh));

		return false;
	}

	if (IS_ENABLED(CONFIG_DESKTOP_BLE_CONN_PARAMS_HID_SCI_ENABLE) &&
	    is_ble_peer_sci_conn_rate_event(aeh)) {
		ble_peer_sci_conn_rate_event_handler(cast_ble_peer_sci_conn_rate_event(aeh));

		return false;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, module_state_event);
APP_EVENT_SUBSCRIBE(MODULE, ble_discovery_complete_event);
APP_EVENT_SUBSCRIBE(MODULE, ble_peer_event);
APP_EVENT_SUBSCRIBE(MODULE, ble_peer_conn_params_event);
#ifdef CONFIG_DESKTOP_BLE_USB_MANAGED_CI
APP_EVENT_SUBSCRIBE(MODULE, usb_state_event);
#endif
#ifdef CONFIG_DESKTOP_BLE_CONN_PARAMS_HID_SCI_ENABLE
APP_EVENT_SUBSCRIBE(MODULE, hogp_sci_mode_changed_event);
APP_EVENT_SUBSCRIBE(MODULE, ble_peer_sci_conn_rate_event);
#endif
