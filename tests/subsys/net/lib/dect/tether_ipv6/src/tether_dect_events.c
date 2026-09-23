/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * Minimal DECT net_mgmt event state for tether integration tests (feeds test_dect_utils).
 */

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_mgmt.h>

#include <net/dect/dect_net_l2_mgmt.h>

LOG_MODULE_REGISTER(tether_dect_events, LOG_LEVEL_INF);

K_SEM_DEFINE(tether_activation_done_sem, 0, 1);

bool dect_scan_result_received;
bool dect_scan_done_received;
enum dect_status_values dect_scan_done_status;
struct dect_scan_result_evt received_beacon_data;
bool beacon_data_valid;

bool dect_association_changed_received;
bool dect_association_created_received;
bool dect_association_failed_received;
bool dect_association_released_received;
int dect_association_released_count;
struct dect_association_changed_evt received_association_data;

bool dect_network_status_received;
struct dect_network_status_evt received_network_status_data;

bool dect_activate_done_received;
enum dect_status_values dect_activate_done_status;
bool dect_deactivate_done_received;
enum dect_status_values dect_deactivate_done_status;

bool dect_rssi_scan_result_received;
struct dect_rssi_scan_result_evt received_rssi_scan_result_data;
bool dect_rssi_scan_done_received;
enum dect_status_values dect_rssi_scan_done_status;

static struct net_mgmt_event_callback tether_dect_mgmt_cb;

static void tether_dect_mgmt_handler(struct net_mgmt_event_callback *cb, uint64_t mgmt_event,
				       struct net_if *iface)
{
	ARG_UNUSED(iface);

	switch (mgmt_event) {
	case NET_EVENT_DECT_ACTIVATE_DONE:
		if (cb->info != NULL) {
			const struct dect_common_resp_evt *evt = cb->info;

			dect_activate_done_status = evt->status;
		}
		dect_activate_done_received = true;
		k_sem_give(&tether_activation_done_sem);
		break;
	case NET_EVENT_DECT_DEACTIVATE_DONE:
		if (cb->info != NULL) {
			const struct dect_common_resp_evt *evt = cb->info;

			dect_deactivate_done_status = evt->status;
		}
		dect_deactivate_done_received = true;
		break;
	case NET_EVENT_DECT_SCAN_RESULT:
		if (cb->info != NULL) {
			memcpy(&received_beacon_data, cb->info, sizeof(received_beacon_data));
			beacon_data_valid = true;
		}
		dect_scan_result_received = true;
		break;
	case NET_EVENT_DECT_SCAN_DONE:
		if (cb->info != NULL) {
			const struct dect_common_resp_evt *evt = cb->info;

			dect_scan_done_status = evt->status;
		}
		dect_scan_done_received = true;
		break;
	case NET_EVENT_DECT_RSSI_SCAN_RESULT:
		if (cb->info != NULL) {
			memcpy(&received_rssi_scan_result_data, cb->info,
			       sizeof(received_rssi_scan_result_data));
		}
		dect_rssi_scan_result_received = true;
		break;
	case NET_EVENT_DECT_RSSI_SCAN_DONE:
		if (cb->info != NULL) {
			const struct dect_common_resp_evt *evt = cb->info;

			dect_rssi_scan_done_status = evt->status;
		}
		dect_rssi_scan_done_received = true;
		break;
	case NET_EVENT_DECT_ASSOCIATION_CHANGED:
		if (cb->info != NULL) {
			const struct dect_association_changed_evt *evt = cb->info;

			memcpy(&received_association_data, evt, sizeof(received_association_data));
			if (evt->association_change_type == DECT_ASSOCIATION_CREATED) {
				dect_association_created_received = true;
			} else if (evt->association_change_type == DECT_ASSOCIATION_RELEASED) {
				dect_association_released_received = true;
				dect_association_released_count++;
			} else if (evt->association_change_type ==
				   DECT_ASSOCIATION_REQ_FAILED_MDM) {
				dect_association_failed_received = true;
			}
		}
		dect_association_changed_received = true;
		break;
	case NET_EVENT_DECT_NETWORK_STATUS:
		if (cb->info != NULL) {
			memcpy(&received_network_status_data, cb->info,
			       sizeof(received_network_status_data));
		}
		dect_network_status_received = true;
		break;
	default:
		break;
	}
}

void tether_dect_events_reset_flags(void)
{
	dect_scan_result_received = false;
	dect_scan_done_received = false;
	beacon_data_valid = false;

	dect_association_changed_received = false;
	dect_association_created_received = false;
	dect_association_failed_received = false;
	dect_association_released_received = false;

	dect_network_status_received = false;

	dect_activate_done_received = false;
	dect_deactivate_done_received = false;

	dect_rssi_scan_result_received = false;
	dect_rssi_scan_done_received = false;
}

int tether_dect_events_init(void)
{
	net_mgmt_init_event_callback(
		&tether_dect_mgmt_cb, tether_dect_mgmt_handler,
		NET_EVENT_DECT_ACTIVATE_DONE | NET_EVENT_DECT_DEACTIVATE_DONE |
			NET_EVENT_DECT_SCAN_RESULT | NET_EVENT_DECT_SCAN_DONE |
			NET_EVENT_DECT_RSSI_SCAN_RESULT | NET_EVENT_DECT_RSSI_SCAN_DONE |
			NET_EVENT_DECT_ASSOCIATION_CHANGED | NET_EVENT_DECT_NETWORK_STATUS);
	net_mgmt_add_event_callback(&tether_dect_mgmt_cb);
	return 0;
}

void tether_dect_events_deinit(void)
{
	net_mgmt_del_event_callback(&tether_dect_mgmt_cb);
}
