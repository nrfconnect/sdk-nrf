/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief WiFi TWT sample
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(twt, CONFIG_LOG_DEFAULT_LEVEL);

#include <nrfx_clock.h>
#include <zephyr/kernel.h>
#include <stdio.h>
#include <stdlib.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/printk.h>
#include <zephyr/init.h>

#include <zephyr/net/net_if.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/net_event.h>
#include <zephyr/drivers/gpio.h>

#include "net_private.h"
#include "traffic_gen.h"

#define WIFI_SHELL_MODULE "wifi"

#define WIFI_SHELL_MGMT_EVENTS (NET_EVENT_WIFI_CONNECT_RESULT |		\
				NET_EVENT_WIFI_DISCONNECT_RESULT|	\
				NET_EVENT_WIFI_TWT)

#define MAX_SSID_LEN        32
#define STATUS_POLLING_MS   300
#define TWT_RESP_TIMEOUT_S    20

struct traffic_gen_config tg_config;

static struct net_mgmt_event_callback wifi_shell_mgmt_cb;
static struct net_mgmt_event_callback net_shell_mgmt_cb;

static struct {
	const struct shell *sh;
	union {
		struct {
			uint8_t connected	: 1;
			uint8_t connect_result	: 1;
			uint8_t disconnect_requested	: 1;
			uint8_t _unused		: 5;
		};
		uint8_t all;
	};
} context;

static bool twt_supported, twt_resp_received;

static bool wait_for_twt_resp_received(void)
{
	int i, timeout_polls = (TWT_RESP_TIMEOUT_S * 1000) / STATUS_POLLING_MS;

	for (i = 0; i < timeout_polls; i++) {
		k_sleep(K_MSEC(STATUS_POLLING_MS));
		if (twt_resp_received) {
			return true;
		}
	}

	return false;
}

static int setup_twt(void)
{
	struct net_if *iface = net_if_get_first_wifi();
	struct wifi_twt_params params = { 0 };
	int ret;

	params.operation = WIFI_TWT_SETUP;
	params.negotiation_type = WIFI_TWT_INDIVIDUAL;
	params.setup_cmd = WIFI_TWT_SETUP_CMD_REQUEST;
	params.dialog_token = 1;
	params.flow_id = 1;
	params.setup.responder = 0;
	params.setup.trigger = IS_ENABLED(CONFIG_TWT_TRIGGER_ENABLE);
	params.setup.implicit = 1;
	params.setup.announce = IS_ENABLED(CONFIG_TWT_ANNOUNCED_MODE);
	params.setup.twt_wake_interval = CONFIG_TWT_WAKE_INTERVAL;
	params.setup.twt_interval = CONFIG_TWT_INTERVAL;

	ret = net_mgmt(NET_REQUEST_WIFI_TWT, iface, &params, sizeof(params));
	if (ret) {
		LOG_INF("TWT setup failed: %d", ret);
		return ret;
	}

	LOG_INF("TWT setup requested");

	return 0;
}

static int teardown_twt(void)
{
	struct net_if *iface = net_if_get_first_wifi();
	struct wifi_twt_params params = { 0 };
	int ret;

	params.operation = WIFI_TWT_TEARDOWN;
	params.negotiation_type = WIFI_TWT_INDIVIDUAL;
	params.setup_cmd = WIFI_TWT_TEARDOWN;
	params.dialog_token = 1;
	params.flow_id = 1;

	ret = net_mgmt(NET_REQUEST_WIFI_TWT, iface, &params, sizeof(params));
	if (ret) {
		LOG_ERR("%s with %s failed, reason : %s",
			wifi_twt_operation2str[params.operation],
			wifi_twt_negotiation_type2str[params.negotiation_type],
			get_twt_err_code_str(params.fail_reason));
		return ret;
	}

	LOG_INF("TWT teardown success");

	return 0;
}

static int cmd_wifi_status(void)
{
	struct net_if *iface = net_if_get_default();
	struct wifi_iface_status status = { 0 };
	int ret;

	ret = net_mgmt(NET_REQUEST_WIFI_IFACE_STATUS, iface, &status,
				sizeof(struct wifi_iface_status));
	if (ret) {
		LOG_INF("Status request failed: %d\n", ret);
		return ret;
	}

	LOG_INF("==================");
	LOG_INF("State: %s", wifi_state_txt(status.state));

	if (status.state >= WIFI_STATE_ASSOCIATED) {
		uint8_t mac_string_buf[sizeof("xx:xx:xx:xx:xx:xx")];

		LOG_INF("Interface Mode: %s",
		       wifi_mode_txt(status.iface_mode));
		LOG_INF("Link Mode: %s",
		       wifi_link_mode_txt(status.link_mode));
		LOG_INF("SSID: %-32s", status.ssid);
		LOG_INF("BSSID: %s",
		       net_sprint_ll_addr_buf(
				status.bssid, WIFI_MAC_ADDR_LEN,
				mac_string_buf, sizeof(mac_string_buf)));
		LOG_INF("Band: %s", wifi_band_txt(status.band));
		LOG_INF("Channel: %d", status.channel);
		LOG_INF("Security: %s", wifi_security_txt(status.security));
		LOG_INF("MFP: %s", wifi_mfp_txt(status.mfp));
		LOG_INF("RSSI: %d", status.rssi);
		LOG_INF("TWT: %s", status.twt_capable ? "Supported" : "Not supported");

		if (status.twt_capable) {
			twt_supported = 1;
		}
	}

	return 0;
}

static void handle_wifi_connect_result(struct net_mgmt_event_callback *cb)
{
	const struct wifi_status *status =
		(const struct wifi_status *) cb->info;

	if (context.connected) {
		return;
	}

	if (status->status) {
		LOG_ERR("Connection failed (%d)", status->status);
	} else {
		LOG_INF("Connected");
		context.connected = true;
	}

	context.connect_result = true;
}

static void handle_wifi_disconnect_result(struct net_mgmt_event_callback *cb)
{
	const struct wifi_status *status =
		(const struct wifi_status *) cb->info;

	if (!context.connected) {
		return;
	}

	if (context.disconnect_requested) {
		LOG_INF("Disconnection request %s (%d)",
			 status->status ? "failed" : "done",
					status->status);
		context.disconnect_requested = false;
	} else {
		LOG_INF("Received Disconnected");
		context.connected = false;
	}

	cmd_wifi_status();
}

static void print_twt_params(uint8_t dialog_token, uint8_t flow_id,
			     enum wifi_twt_negotiation_type negotiation_type,
			     bool responder, bool implicit, bool announce,
			     bool trigger, uint32_t twt_wake_interval,
			     uint64_t twt_interval)
{
	LOG_INF("TWT Dialog token: %d",
	      dialog_token);
	LOG_INF("TWT flow ID: %d",
	      flow_id);
	LOG_INF("TWT negotiation type: %s",
	      wifi_twt_negotiation_type2str[negotiation_type]);
	LOG_INF("TWT responder: %s",
	       responder ? "true" : "false");
	LOG_INF("TWT implicit: %s",
	      implicit ? "true" : "false");
	LOG_INF("TWT announce: %s",
	      announce ? "true" : "false");
	LOG_INF("TWT trigger: %s",
	      trigger ? "true" : "false");
	LOG_INF("TWT wake interval: %d us",
	      twt_wake_interval);
	LOG_INF("TWT interval: %lld us",
	      twt_interval);
	LOG_INF("========================");
}

static void handle_wifi_twt_event(struct net_mgmt_event_callback *cb)
{
	const struct wifi_twt_params *resp =
		(const struct wifi_twt_params *)cb->info;

	if (resp->operation == WIFI_TWT_TEARDOWN) {
		LOG_INF("TWT teardown received for flow ID %d\n",
		      resp->flow_id);
		return;
	}

	if (resp->resp_status == WIFI_TWT_RESP_RECEIVED) {
		twt_resp_received = 1;
		LOG_INF("TWT response: %s",
		      wifi_twt_setup_cmd2str[resp->setup_cmd]);
		LOG_INF("== TWT negotiated parameters ==");
		print_twt_params(resp->dialog_token,
				 resp->flow_id,
				 resp->negotiation_type,
				 resp->setup.responder,
				 resp->setup.implicit,
				 resp->setup.announce,
				 resp->setup.trigger,
				 resp->setup.twt_wake_interval,
				 resp->setup.twt_interval);
	} else {
		LOG_INF("TWT response timed out\n");
	}
}

static void wifi_mgmt_event_handler(struct net_mgmt_event_callback *cb,
				     uint32_t mgmt_event, struct net_if *iface)
{
	switch (mgmt_event) {
	case NET_EVENT_WIFI_CONNECT_RESULT:
		handle_wifi_connect_result(cb);
		break;
	case NET_EVENT_WIFI_DISCONNECT_RESULT:
		handle_wifi_disconnect_result(cb);
		break;
	case NET_EVENT_WIFI_TWT:
		handle_wifi_twt_event(cb);
		break;
#ifdef CONFIG_SCHEDULED_TX
	case NET_EVENT_WIFI_TWT_SLEEP_STATE: {
		int *twt_state;

		twt_state = (int *)(cb->info);
		if (*twt_state == WIFI_TWT_STATE_SLEEP) {
			traffic_gen_pause(&tg_config);
		} else if (*twt_state == WIFI_TWT_STATE_AWAKE)  {
			traffic_gen_resume(&tg_config);
		} else {
			LOG_INF("UNKNOWN TWT STATE %d", *twt_state);
		}
	}
		break;
#endif
	default:
		break;
	}
}

static void print_dhcp_ip(struct net_mgmt_event_callback *cb)
{
	/* Get DHCP info from struct net_if_dhcpv4 and print */
	const struct net_if_dhcpv4 *dhcpv4 = cb->info;
	const struct in_addr *addr = &dhcpv4->requested_ip;
	char dhcp_info[128];

	net_addr_ntop(AF_INET, addr, dhcp_info, sizeof(dhcp_info));

	LOG_INF("DHCP IP address: %s", dhcp_info);
}

static void net_mgmt_event_handler(struct net_mgmt_event_callback *cb,
				    uint32_t mgmt_event, struct net_if *iface)
{
	switch (mgmt_event) {
	case NET_EVENT_IPV4_DHCP_BOUND:
		print_dhcp_ip(cb);
		break;
	default:
		break;
	}
}

static int __wifi_args_to_params(struct wifi_connect_req_params *params)
{
	params->timeout =  CONFIG_STA_CONN_TIMEOUT_SEC * MSEC_PER_SEC;

	if (params->timeout == 0) {
		params->timeout = SYS_FOREVER_MS;
	}

	/* SSID */
	params->ssid = CONFIG_TWT_SAMPLE_SSID;
	params->ssid_length = strlen(params->ssid);

#if defined(CONFIG_TWT_STA_KEY_MGMT_WPA2)
	params->security = 1;
#elif defined(CONFIG_TWT_STA_KEY_MGMT_WPA2_256)
	params->security = 2;
#elif defined(CONFIG_TWT_STA_KEY_MGMT_WPA3)
	params->security = 3;
#else
	params->security = 0;
#endif

#if !defined(CONFIG_TWT_STA_KEY_MGMT_NONE)
	params->psk = CONFIG_TWT_SAMPLE_PASSWORD;
	params->psk_length = strlen(params->psk);
#endif
	params->channel = WIFI_CHANNEL_ANY;

	/* MFP (optional) */
	params->mfp = WIFI_MFP_OPTIONAL;

	return 0;
}

static int wifi_connect(void)
{
	struct net_if *iface = net_if_get_default();
	static struct wifi_connect_req_params cnx_params;

	context.connected = false;
	context.connect_result = false;
	__wifi_args_to_params(&cnx_params);

	if (net_mgmt(NET_REQUEST_WIFI_CONNECT, iface,
				&cnx_params, sizeof(struct wifi_connect_req_params))) {
		LOG_ERR("Connection request failed");

		return -ENOEXEC;
	}

	LOG_INF("Connection requested");

	return 0;
}

int main(void)
{
	memset(&context, 0, sizeof(context));

	net_mgmt_init_event_callback(&wifi_shell_mgmt_cb,
			wifi_mgmt_event_handler,
			WIFI_SHELL_MGMT_EVENTS);

	net_mgmt_add_event_callback(&wifi_shell_mgmt_cb);

	net_mgmt_init_event_callback(&net_shell_mgmt_cb,
			net_mgmt_event_handler,
			NET_EVENT_IPV4_DHCP_BOUND);

	net_mgmt_add_event_callback(&net_shell_mgmt_cb);

	LOG_INF("Starting %s with CPU frequency: %d MHz", CONFIG_BOARD, SystemCoreClock/MHZ(1));
	k_sleep(K_SECONDS(1));

	LOG_INF("Static IP address (overridable): %s/%s -> %s",
			CONFIG_NET_CONFIG_MY_IPV4_ADDR,
			CONFIG_NET_CONFIG_MY_IPV4_NETMASK,
			CONFIG_NET_CONFIG_MY_IPV4_GW);

	while (1) {
		wifi_connect();

		while (!context.connect_result) {
			cmd_wifi_status();
			k_sleep(K_MSEC(STATUS_POLLING_MS));
		}

		cmd_wifi_status();

		if (context.connected) {
			int ret;

			k_sleep(K_SECONDS(2));

			if (!twt_supported) {
				LOG_INF("AP is not TWT capable, exiting the sample\n");
				return -1;
			}

			LOG_INF("AP is TWT capable, establishing TWT");

			ret = setup_twt();
			if (ret) {
				LOG_ERR("Failed to establish TWT flow: %d\n", ret);
				return -1;
			}

			if (wait_for_twt_resp_received()) {
				LOG_INF("TWT Setup success");
			} else {
				LOG_INF("TWT Setup timed out\n");
				return -1;
			}

#ifdef CONFIG_TRAFFIC_GEN
			traffic_gen_init(&tg_config);

			ret = traffic_gen_start(&tg_config);
			if (ret < 0) {
				LOG_ERR("Failed to start traffic role ");
				return -1;
			}

			ret = traffic_gen_wait_for_report(&tg_config);
			if (ret < 0) {
				LOG_ERR("Failed to get traffic report ");
				return -1;
			}
			traffic_gen_get_report(&tg_config);
#endif /* CONFIG_TRAFFIC_GEN */

			/* Wait for few service periods before tearing down */
			k_sleep(K_USEC(5 * CONFIG_TWT_INTERVAL));

			ret = teardown_twt();
			if (ret) {
				LOG_ERR("Failed to teardown TWT flow: %d\n", ret);
				return -1;
			}

			k_sleep(K_FOREVER);
		}
	}

	return 0;
}
