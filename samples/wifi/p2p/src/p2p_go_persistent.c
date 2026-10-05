/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief P2P autonomous persistent group with in-code persistence verification.
 * Creates a persistent group, then re-invokes it by id across cycles and checks
 * the SSID is reused. GO-only (no peer); persistence is RAM-only here.
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(p2p_go_persistent, CONFIG_LOG_DEFAULT_LEVEL);

#include <zephyr/kernel.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <zephyr/net/net_if.h>
#include <zephyr/net/wifi_mgmt.h>

/* Wi-Fi SSID is at most 32 bytes; +1 for the NUL terminator. */
#define SSID_BUF_SIZE 33

/* Time to let a group-add settle before the stored network is read, and to let
 * a group-remove settle before the next cycle's group-add (avoids -EBUSY).
 */
#define SETTLE_MS 2000

static int go_bssid_from_str(const char *mac_str, uint8_t mac[WIFI_MAC_ADDR_LEN])
{
	if (!mac_str || mac_str[0] == '\0') {
		return -1;
	}

	return sscanf(mac_str, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
		      &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5])
		      == WIFI_MAC_ADDR_LEN ? 0 : -1;
}

/* persistent_id < 0: create a new persistent group (bare "persistent").
 * persistent_id >= 0: re-invoke the stored persistent group with that id.
 */
static int wifi_p2p_group_add(int persistent_id)
{
	struct wifi_p2p_params params = { 0 };
	struct net_if *iface = net_if_get_first_wifi();

	if (!iface) {
		LOG_ERR("Failed to get Wi-Fi interface");
		return -1;
	}

	params.oper = WIFI_P2P_GROUP_ADD;
	params.group_add.freq = CONFIG_SAMPLE_P2P_GO_FREQ;
	params.group_add.vht = IS_ENABLED(CONFIG_SAMPLE_P2P_GO_VHT);
	params.group_add.go_bssid_length = 0;

	if (persistent_id < 0) {
		params.group_add.persistent = -1;
		params.group_add.persistent_set = true;
	} else {
		params.group_add.persistent = persistent_id;
		params.group_add.persistent_set = false;
	}

	if (strlen(CONFIG_SAMPLE_P2P_GO_BSSID) > 0) {
		uint8_t bssid[WIFI_MAC_ADDR_LEN];

		if (go_bssid_from_str(CONFIG_SAMPLE_P2P_GO_BSSID, bssid) == 0) {
			memcpy(params.group_add.go_bssid, bssid, WIFI_MAC_ADDR_LEN);
			params.group_add.go_bssid_length = WIFI_MAC_ADDR_LEN;
		} else {
			LOG_WRN("Invalid P2P_GO_BSSID, using auto");
		}
	}

	if (net_mgmt(NET_REQUEST_WIFI_P2P_OPER, iface, &params, sizeof(params))) {
		LOG_ERR("P2P group add failed");
		return -1;
	}

	if (persistent_id < 0) {
		LOG_INF("P2P group add initiated (new persistent group)");
	} else {
		LOG_INF("P2P group add initiated (re-invoke persistent id=%d)", persistent_id);
	}
	return 0;
}

static int wifi_p2p_group_remove(void)
{
	struct wifi_p2p_params params = { 0 };
	struct net_if *iface = net_if_get_first_wifi();

	if (!iface) {
		LOG_ERR("Failed to get Wi-Fi interface");
		return -1;
	}

	params.oper = WIFI_P2P_GROUP_REMOVE;
	strncpy(params.group_remove.ifname, CONFIG_SAMPLE_P2P_IFACE_NAME,
		CONFIG_NET_INTERFACE_NAME_LEN);
	params.group_remove.ifname[CONFIG_NET_INTERFACE_NAME_LEN] = '\0';

	if (net_mgmt(NET_REQUEST_WIFI_P2P_OPER, iface, &params, sizeof(params))) {
		LOG_ERR("P2P group remove failed");
		return -1;
	}

	LOG_INF("P2P group remove initiated: %s", params.group_remove.ifname);
	return 0;
}

/* Parse the [P2P-PERSISTENT] entry's id and SSID out of the
 * WIFI_P2P_LIST_NETWORKS text table. Returns 0 on success, negative otherwise.
 */
static int wifi_p2p_get_persistent(int *id_out, char *ssid_out, size_t ssid_size)
{
	static char buf[WIFI_P2P_LIST_NETWORKS_BUF_SIZE];
	struct wifi_p2p_params params = { 0 };
	struct net_if *iface = net_if_get_first_wifi();
	char *line;

	if (!iface) {
		LOG_ERR("Failed to get Wi-Fi interface");
		return -1;
	}

	memset(buf, 0, sizeof(buf));
	params.oper = WIFI_P2P_LIST_NETWORKS;
	params.list_networks.buf = buf;
	params.list_networks.buf_size = sizeof(buf);

	if (net_mgmt(NET_REQUEST_WIFI_P2P_OPER, iface, &params, sizeof(params))) {
		LOG_ERR("P2P list_networks request failed");
		return -1;
	}

	line = buf;
	while (line && *line) {
		char *nl = strchr(line, '\n');

		if (nl) {
			*nl = '\0';
		}

		if (strstr(line, "P2P-PERSISTENT")) {
			int id;
			char ssid[SSID_BUF_SIZE];

			if (sscanf(line, "%d %32s", &id, ssid) == 2) {
				*id_out = id;
				strncpy(ssid_out, ssid, ssid_size - 1);
				ssid_out[ssid_size - 1] = '\0';
				return 0;
			}
		}

		if (!nl) {
			break;
		}
		line = nl + 1;
	}

	return -ENOENT;
}

int p2p_go_persistent_run(void)
{
	int ret;
	int stored_id = -1;
	char stored_ssid[SSID_BUF_SIZE] = { 0 };
	int verified = 0;
	int mismatched = 0;

	LOG_INF("Starting %s with CPU frequency: %d MHz", CONFIG_BOARD, SystemCoreClock/MHZ(1));
	k_sleep(K_SECONDS(1));

	for (int cycle = 1; cycle <= CONFIG_SAMPLE_P2P_GO_PERSISTENT_CYCLES; cycle++) {
		bool reinvoke = (cycle > 1 && stored_id >= 0);
		int cur_id;
		char cur_ssid[SSID_BUF_SIZE];

		LOG_INF("Persistence cycle %d/%d (%s)", cycle,
			CONFIG_SAMPLE_P2P_GO_PERSISTENT_CYCLES,
			reinvoke ? "re-invoke by id" : "create");

		ret = wifi_p2p_group_add(reinvoke ? stored_id : -1);
		if (ret < 0) {
			return ret;
		}

		k_sleep(K_MSEC(SETTLE_MS));

		ret = wifi_p2p_get_persistent(&cur_id, cur_ssid, sizeof(cur_ssid));
		if (ret < 0) {
			LOG_ERR("No stored persistent network found after group add");
		} else if (!reinvoke) {
			stored_id = cur_id;
			strncpy(stored_ssid, cur_ssid, sizeof(stored_ssid) - 1);
			stored_ssid[sizeof(stored_ssid) - 1] = '\0';
			LOG_INF("Stored persistent network: id=%d ssid=\"%s\"",
				stored_id, stored_ssid);
		} else if (strcmp(cur_ssid, stored_ssid) == 0) {
			verified++;
			LOG_INF("PERSISTENCE VERIFIED: re-invoked id=%d reused ssid=\"%s\"",
				stored_id, cur_ssid);
		} else {
			mismatched++;
			LOG_ERR("PERSISTENCE MISMATCH: expected ssid=\"%s\" got \"%s\"",
				stored_ssid, cur_ssid);
		}

		ret = wifi_p2p_group_remove();
		if (ret < 0) {
			return ret;
		}

		k_sleep(K_MSEC(SETTLE_MS));
	}

	LOG_INF("Persistence verification done: %d verified, %d mismatched", verified, mismatched);
	return mismatched ? -EIO : 0;
}
