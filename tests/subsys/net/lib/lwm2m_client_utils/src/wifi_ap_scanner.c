/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/ztest.h>
#include <zephyr/fff.h>

#include <net/lwm2m_client_utils_location.h>
#include "stubs.h"

#include <zephyr/net/wifi_mgmt.h>

void lwm2m_wifi_net_mgmt_event_handler(struct net_mgmt_event_callback *cb, uint32_t mgmt_event,
				       struct net_if *iface);

struct wifi_scan_result scans[] = {
	[0].mac = {0, 1, 2, 3, 4, 5},
	[0].channel = 11,
	[0].band = 0,
	[0].rssi = -75,
	[0].ssid = "Test",
	[1].mac = {5, 4, 3, 2, 1, 0},
	[1].channel = 1,
	[1].band = 0,
	[1].rssi = -42,
	[1].ssid = "Test two",
};

struct wifi_status status = {
	.status = 0,
};

struct net_mgmt_event_callback callback;

static void setup(void)
{
	/* Register resets */
	DO_FOREACH_FAKE(RESET_FAKE);

	/* reset common FFF internal structures */
	FFF_RESET_HISTORY();
}

ZTEST_SUITE(lwm2m_client_utils_wifi_ap_scanner, NULL, NULL, NULL, NULL, NULL);


ZTEST(lwm2m_client_utils_wifi_ap_scanner, test_update_access_points)
{

	setup();

	lwm2m_wifi_request_scan();
	callback.info = &scans[0];
	lwm2m_wifi_net_mgmt_event_handler(&callback, NET_EVENT_WIFI_SCAN_RESULT, NULL);

	callback.info = &scans[1];
	lwm2m_wifi_net_mgmt_event_handler(&callback, NET_EVENT_WIFI_SCAN_RESULT, NULL);

	callback.info = &status;
	lwm2m_wifi_net_mgmt_event_handler(&callback, NET_EVENT_WIFI_SCAN_DONE, NULL);

	zassert_equal(lwm2m_set_s32_fake.call_count, 6,
		      "Wi-Fi info not updated");
	zassert_equal(lwm2m_set_opaque_fake.call_count, 2,
		      "Mac addressess not updated");
	zassert_equal(lwm2m_set_string_fake.call_count, 2,
		      "SSID not updated");
}
