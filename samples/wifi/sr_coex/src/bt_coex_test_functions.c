/**
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief Wi-Fi and Bluetooth LE coexistence test functions
 */

#include "bt_coex_test_functions.h"

int8_t wifi_rssi = RSSI_INIT_VALUE;
static int print_wifi_conn_status_once = 1;


void wifi_memset_context(void)
{
	memset(&context, 0, sizeof(context));
}

int wifi_command_status(void)
{
	struct net_if *iface = net_if_get_default();

	if (net_mgmt(NET_REQUEST_WIFI_IFACE_STATUS, iface, &status,
				sizeof(struct wifi_iface_status))) {
		LOG_INF("Status request failed");

		return -ENOEXEC;
	}

#ifndef CONFIG_PRINTS_FOR_AUTOMATION
	LOG_INF("Status: successful");
	LOG_INF("==================");
	LOG_INF("State: %s", wifi_state_txt(status.state));
#endif

	if (status.state >= WIFI_STATE_ASSOCIATED) {
		uint8_t mac_string_buf[sizeof("xx:xx:xx:xx:xx:xx")];

		if (print_wifi_conn_status_once == 1) {

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
			/* LOG_INF("MFP: %s", wifi_mfp_txt(status.mfp)); */
			LOG_INF("WiFi RSSI: %d", status.rssi);

			print_wifi_conn_status_once++;
		}
		wifi_rssi = status.rssi;
	}

	return 0;
}

void wifi_net_mgmt_callback_functions(void)
{
	net_mgmt_init_event_callback(&wifi_sta_mgmt_cb, wifi_mgmt_event_handler,
		WIFI_MGMT_EVENTS);

	net_mgmt_add_event_callback(&wifi_sta_mgmt_cb);

	net_mgmt_init_event_callback(&net_addr_mgmt_cb, net_mgmt_event_handler,
		NET_EVENT_IPV4_DHCP_BOUND);
	net_mgmt_add_event_callback(&net_addr_mgmt_cb);

#ifdef CLOCK_FEATURE_HFCLK_DIVIDE_PRESENT
	nrfx_clock_divider_set(NRF_CLOCK_DOMAIN_HFCLK, NRF_CLOCK_HFCLK_DIV_1);
#endif

	LOG_INF("Starting %s with CPU frequency: %d MHz", CONFIG_BOARD, SystemCoreClock/MHZ(1));

	k_sleep(K_SECONDS(1));
}
void net_mgmt_event_handler(struct net_mgmt_event_callback *cb, uint32_t mgmt_event,
		struct net_if *iface)
{
	switch (mgmt_event) {
	case NET_EVENT_IPV4_DHCP_BOUND:
		print_dhcp_ip(cb);
		break;
	default:
		break;
	}
}

void wifi_mgmt_event_handler(struct net_mgmt_event_callback *cb,
		uint32_t mgmt_event, struct net_if *iface)
{
	const struct device *dev = iface->if_dev->dev;
	struct wifi_nrf_vif_ctx_zep *vif_ctx_zep = NULL;

	vif_ctx_zep = dev->data;

	switch (mgmt_event) {
	case NET_EVENT_WIFI_CONNECT_RESULT:
		wifi_handle_connect_result(cb);
		break;
	case NET_EVENT_WIFI_DISCONNECT_RESULT:
		wifi_handle_disconnect_result(cb);
		break;
	default:
		break;
	}
}

void wifi_handle_connect_result(struct net_mgmt_event_callback *cb)
{
	const struct wifi_status *status =
		(const struct wifi_status *) cb->info;

	if (status->status) {
		LOG_ERR("Wi-Fi Connection request failed (%d)", status->status);
	} else {
#ifdef CONFIG_DEBUG_PRINT_WIFI_CONN_INFO
		LOG_INF("Connected");
#endif
		context.connected = true;
	}

	wifi_command_status();

	k_sem_give(&wait_for_next);
}

void wifi_handle_disconnect_result(struct net_mgmt_event_callback *cb)
{
#ifndef CONFIG_PRINTS_FOR_AUTOMATION
	const struct wifi_status *status = (const struct wifi_status *) cb->info;
#endif

	if (context.disconnect_requested) {
#ifndef CONFIG_PRINTS_FOR_AUTOMATION
		LOG_INF("Disconnection request %s (%d)", status->status ? "failed" : "done",
			status->status);
#endif
		context.disconnect_requested = false;
	} else {
#ifndef CONFIG_PRINTS_FOR_AUTOMATION
		LOG_INF("Disconnected");
#endif
		context.connected = false;
	}
	wifi_disconn_cnt_stability++;
#ifdef CONFIG_DEBUG_PRINT_WIFI_CONN_INFO
	wifi_command_status();
#endif
}

void print_dhcp_ip(struct net_mgmt_event_callback *cb)
{
	/* Get DHCP info from struct net_if_dhcpv4 and print */
	const struct net_if_dhcpv4 *dhcpv4 = cb->info;
	const struct in_addr *addr = &dhcpv4->requested_ip;
	char dhcp_info[128];

	net_addr_ntop(AF_INET, addr, dhcp_info, sizeof(dhcp_info));

#ifdef CONFIG_DEBUG_PRINT_WIFI_DHCP_INFO
	LOG_INF("IP address: %s", dhcp_info);
#endif
	k_sem_give(&wait_for_next);
}

int __wifi_args_to_params(struct wifi_connect_req_params *params)
{
	params->timeout = SYS_FOREVER_MS;

	/* SSID */
	params->ssid = CONFIG_STA_SSID;
	params->ssid_length = strlen(params->ssid);

#if defined(CONFIG_STA_KEY_MGMT_WPA2)
		params->security = 1;
#elif defined(CONFIG_STA_KEY_MGMT_WPA2_256)
		params->security = 2;
#elif defined(CONFIG_STA_KEY_MGMT_WPA3)
		params->security = 3;
#else
		params->security = 0;
#endif

#if !defined(CONFIG_STA_KEY_MGMT_NONE)
		params->psk = CONFIG_STA_PASSWORD;
		params->psk_length = strlen(params->psk);
#endif

	params->channel = WIFI_CHANNEL_ANY;

	/* MFP (optional) */
	params->mfp = WIFI_MFP_OPTIONAL;

	return 0;
}

int wifi_connect(void)
{
	struct net_if *iface = net_if_get_default();
	static struct wifi_connect_req_params cnx_params;

	/* LOG_INF("Connection requested"); */
	__wifi_args_to_params(&cnx_params);

	if (net_mgmt(NET_REQUEST_WIFI_CONNECT, iface,
			&cnx_params, sizeof(struct wifi_connect_req_params))) {
		LOG_ERR("Wi-Fi Connection request failed");
		return -ENOEXEC;
	}
	return 0;
}

int wifi_disconnect(void)
{
	struct net_if *iface = net_if_get_default();
	int status;

	context.disconnect_requested = true;

	status = net_mgmt(NET_REQUEST_WIFI_DISCONNECT, iface, NULL, 0);

	if (status) {
		context.disconnect_requested = false;

		if (status == -EALREADY) {
			/* LOG_ERR("Already disconnected"); */
			wifi_disconn_no_conn_cnt++;
		} else {
			/* LOG_ERR("Disconnect request failed"); */
			wifi_disconn_fail_cnt++;
			return -ENOEXEC;
		}
	} else {
		wifi_disconn_success_cnt++;
	}
	return 0;
}

int wifi_parse_ipv4_addr(char *host, struct sockaddr_in *addr)
{
	int ret;

	if (!host) {
		return -EINVAL;
	}
	ret = net_addr_pton(AF_INET, host, &addr->sin_addr);
	if (ret < 0) {
		LOG_ERR("Invalid IPv4 address %s", host);
		return -EINVAL;
	}
	LOG_INF("Wi-Fi peer IPv4 address %s", host);

	return 0;
}

int wait_for_next_event(const char *event_name, int timeout)
{
	int wait_result;

#ifdef CONFIG_DEBUG_PRINT_WIFI_CONN_INFO
	if (event_name) {
		LOG_INF("Waiting for %s", event_name);
	}
#endif
	wait_result = k_sem_take(&wait_for_next, K_SECONDS(timeout));
	if (wait_result) {
		LOG_ERR("Timeout waiting for %s -> %d", event_name, wait_result);
		return -1;
	}
#ifdef CONFIG_DEBUG_PRINT_WIFI_CONN_INFO
	LOG_INF("Got %s", event_name);
#endif
	k_sem_reset(&wait_for_next);

	return 0;
}

void wifi_tcp_upload_results_cb(enum zperf_status status, struct zperf_results *result,
		void *user_data)
{
	uint32_t client_rate_in_kbps;

	switch (status) {
	case ZPERF_SESSION_STARTED:
		LOG_INF("New TCP session started.\n");
		wait4_peer_wifi_client_to_start_tp_test = 1;
		break;

	case ZPERF_SESSION_FINISHED: {

		if (result->client_time_in_us != 0U) {
			client_rate_in_kbps = (uint32_t)
				(((uint64_t)result->nb_packets_sent *
				  (uint64_t)result->packet_size * (uint64_t)8 *
				  (uint64_t)USEC_PER_SEC) /
				 ((uint64_t)result->client_time_in_us * 1024U));
		} else {
			client_rate_in_kbps = 0U;
		}

		LOG_INF("Duration:\t%u", result->client_time_in_us);
		LOG_INF("Num packets:\t%u", result->nb_packets_sent);
		LOG_INF("Num errors:\t%u (retry or fail)\n",
						result->nb_packets_errors);
		LOG_INF("\nclient data rate = %u kbps", client_rate_in_kbps);
		k_sem_give(&udp_tcp_callback);
		break;
	}

	case ZPERF_SESSION_ERROR:
		LOG_INF("TCP upload failed\n");
		break;
	}
}

void wifi_tcp_download_results_cb(enum zperf_status status, struct zperf_results *result,
		void *user_data)
{
	uint32_t rate_in_kbps;

	switch (status) {
	case ZPERF_SESSION_STARTED:
		LOG_INF("New TCP session started.\n");
		wait4_peer_wifi_client_to_start_tp_test = 1;
		break;

	case ZPERF_SESSION_FINISHED: {

		/* Compute baud rate */
		if (result->time_in_us != 0U) {
			rate_in_kbps = (uint32_t)
				(((uint64_t)result->total_len * 8ULL *
				  (uint64_t)USEC_PER_SEC) /
				 ((uint64_t)result->time_in_us * 1024ULL));
		} else {
			rate_in_kbps = 0U;
		}

		LOG_INF("TCP session ended\n");
		LOG_INF("%u bytes in %u ms:", result->total_len,
					result->time_in_us/USEC_PER_MSEC);
		LOG_INF("\nThroughput:%u kbps", rate_in_kbps);
		LOG_INF("");
		k_sem_give(&udp_tcp_callback);
		break;
	}

	case ZPERF_SESSION_ERROR:
		LOG_INF("TCP session error.\n");
		break;
	}
}

void wifi_udp_upload_results_cb(enum zperf_status status, struct zperf_results *result,
		void *user_data)
{
	unsigned int client_rate_in_kbps;

	switch (status) {
	case ZPERF_SESSION_STARTED:
		LOG_INF("New UDP session started");
		wait4_peer_wifi_client_to_start_tp_test = 1;
		break;
	case ZPERF_SESSION_FINISHED:
		LOG_INF("Wi-Fi benchmark: Upload completed!");
		if (result->client_time_in_us != 0U) {
			client_rate_in_kbps = (uint32_t)
				(((uint64_t)result->nb_packets_sent *
				  (uint64_t)result->packet_size * (uint64_t)8 *
				  (uint64_t)USEC_PER_SEC) /
				 ((uint64_t)result->client_time_in_us * 1024U));
		} else {
			client_rate_in_kbps = 0U;
		}
		/* print results */
		LOG_INF("Upload results:");
		LOG_INF("%u bytes in %u ms",
				(result->nb_packets_sent * result->packet_size),
				(result->client_time_in_us / USEC_PER_MSEC));
		/**LOG_INF("%u packets sent", result->nb_packets_sent);
		 *LOG_INF("%u packets lost", result->nb_packets_lost);
		 *LOG_INF("%u packets received", result->nb_packets_rcvd);
		 */
		LOG_INF("client data rate = %u kbps", client_rate_in_kbps);
		k_sem_give(&udp_tcp_callback);
		break;
	case ZPERF_SESSION_ERROR:
		LOG_ERR("UDP session error");
		break;
	}
}

void wifi_udp_download_results_cb(enum zperf_status status, struct zperf_results *result,
		void *user_data)
{
	switch (status) {
	case ZPERF_SESSION_STARTED:
		LOG_INF("New session started.");
		wait4_peer_wifi_client_to_start_tp_test = 1;
		break;

	case ZPERF_SESSION_FINISHED: {
		uint32_t rate_in_kbps;

		/* Compute baud rate */
		if (result->time_in_us != 0U) {
			rate_in_kbps = (uint32_t)
				(((uint64_t)result->total_len * 8ULL *
				  (uint64_t)USEC_PER_SEC) /
				 ((uint64_t)result->time_in_us * 1024ULL));
		} else {
			rate_in_kbps = 0U;
		}

		LOG_INF("End of session!");

		LOG_INF("Download results:");
		LOG_INF("%u bytes in %u ms",
				(result->nb_packets_rcvd * result->packet_size),
				(result->time_in_us / USEC_PER_MSEC));
		/**
		 *LOG_INF(" received packets:\t%u",
		 *		  result->nb_packets_rcvd);
		 *LOG_INF(" nb packets lost:\t%u",
		 *		  result->nb_packets_lost);
		 *LOG_INF(" nb packets outorder:\t%u",
		 *		  result->nb_packets_outorder);
		 */
		LOG_INF("\nThroughput:%u kbps", rate_in_kbps);
		LOG_INF("");
		k_sem_give(&udp_tcp_callback);
		break;
	}

	case ZPERF_SESSION_ERROR:
		LOG_INF("UDP session error.");
		break;
	}
}

enum nrf_wifi_pta_wlan_op_band wifi_mgmt_to_pta_band(enum wifi_frequency_bands band)
{
	switch (band) {
	case WIFI_FREQ_BAND_2_4_GHZ:
		return NRF_WIFI_PTA_WLAN_OP_BAND_2_4_GHZ;
	case WIFI_FREQ_BAND_5_GHZ:
		return NRF_WIFI_PTA_WLAN_OP_BAND_5_GHZ;
	default:
		return NRF_WIFI_PTA_WLAN_OP_BAND_NONE;
	}
}

int wifi_run_tcp_traffic(void)
{
	int ret = 0;

#ifdef CONFIG_WIFI_ZPERF_SERVER
	struct zperf_download_params params;

	params.port = CONFIG_NET_CONFIG_PEER_IPV4_PORT;

	ret = zperf_tcp_download(&params, wifi_tcp_download_results_cb, NULL);
	if (ret != 0) {
		LOG_ERR("Failed to start TCP server session: %d", ret);
		return ret;
	}
	LOG_INF("TCP server started on port %u\n", params.port);
#else
	struct zperf_upload_params params;
	/* Start Wi-Fi TCP traffic */
	LOG_INF("Starting Wi-Fi benchmark: Zperf TCP client");
	params.duration_ms = CONFIG_COEX_TEST_DURATION;
	params.rate_kbps = CONFIG_WIFI_ZPERF_RATE;
	params.packet_size = CONFIG_WIFI_ZPERF_PKT_SIZE;
	wifi_parse_ipv4_addr(CONFIG_NET_CONFIG_PEER_IPV4_ADDR, &in4_addr_my);
	net_sprint_ipv4_addr(&in4_addr_my.sin_addr);

	memcpy(&params.peer_addr, &in4_addr_my, sizeof(in4_addr_my));

	ret = zperf_tcp_upload_async(&params, wifi_tcp_upload_results_cb, NULL);
	if (ret != 0) {
		LOG_ERR("Failed to start TCP session: %d", ret);
		return ret;
	}
#endif

	return 0;
}

int wifi_run_udp_traffic(void)
{
	int ret = 0;

#ifdef CONFIG_WIFI_ZPERF_SERVER
	struct zperf_download_params params;

	params.port = CONFIG_NET_CONFIG_PEER_IPV4_PORT;

	ret = zperf_udp_download(&params, wifi_udp_download_results_cb, NULL);
	if (ret != 0) {
		LOG_ERR("Failed to start UDP server session: %d", ret);
		return ret;
	}
#else
	struct zperf_upload_params params;

	/* Start Wi-Fi UDP traffic */
	LOG_INF("Starting Wi-Fi benchmark: Zperf UDP client");
	params.duration_ms = CONFIG_COEX_TEST_DURATION;
	params.rate_kbps = CONFIG_WIFI_ZPERF_RATE;
	params.packet_size = CONFIG_WIFI_ZPERF_PKT_SIZE;
	wifi_parse_ipv4_addr(CONFIG_NET_CONFIG_PEER_IPV4_ADDR, &in4_addr_my);
	net_sprint_ipv4_addr(&in4_addr_my.sin_addr);

	memcpy(&params.peer_addr, &in4_addr_my, sizeof(in4_addr_my));
	ret = zperf_udp_upload_async(&params, wifi_udp_upload_results_cb, NULL);
	if (ret != 0) {
		LOG_ERR("Failed to start Wi-Fi UDP benchmark: %d", ret);
		return ret;
	}
#endif

	return 0;
}

void wifi_check_traffic_status(void)
{
	/* Run Wi-Fi traffic */
	if (k_sem_take(&udp_tcp_callback, K_FOREVER) != 0) {
		LOG_ERR("Results are not ready");
	} else {
#ifdef CONFIG_WIFI_ZPERF_PROT_UDP
		LOG_INF("Wi-Fi UDP session finished");
#else
		LOG_INF("Wi-Fi TCP session finished");
#endif
	}
}

int wifi_connection(void)
{
	wifi_conn_attempt_cnt++;
	/* Wi-Fi connection */
	wifi_connect();

	if (wait_for_next_event("Wi-Fi Connection", WIFI_CONNECTION_TIMEOUT)) {
		wifi_conn_timeout_cnt++;
		wifi_conn_fail_cnt++;
		return -1;
	}

	if (wait_for_next_event("Wi-Fi DHCP", WIFI_DHCP_TIMEOUT)) {
		wifi_dhcp_timeout_cnt++;
		return -1;
	}
	wifi_conn_success_cnt++;
	return 0;
}

void wifi_disconnection(void)
{
	int ret = 0;

	wifi_disconn_attempt_cnt++;
	/* Wi-Fi disconnection */
#ifdef CONFIG_DEBUG_PRINT_WIFI_CONN_INFO
	LOG_INF("Disconnecting Wi-Fi");
#endif
	ret = wifi_disconnect();
	if (ret != 0) {
		LOG_INF("Disconnect failed");
	}
}

void bt_start_activity(void)
{
	k_thread_start(run_bt_traffic);
}

void bt_run_activity(void)
{
	k_thread_join(run_bt_traffic, K_FOREVER);
}

void bt_exit_throughput_test(void)
{
	/** This is called if role is central. Disconnection in the
	 *case of peripheral is taken care by the peer central.
	 */
	bt_throughput_test_exit();
}

void bt_run_benchmark(void)
{
	bt_throughput_test_run();
}

int config_pta(bool is_ant_mode_sep, bool is_ble_central, bool is_wlan_server)
{
	int ret = 0;
	enum nrf_wifi_pta_wlan_op_band wlan_band = wifi_mgmt_to_pta_band(status.band);

	if (wlan_band == NRF_WIFI_PTA_WLAN_OP_BAND_NONE) {
		LOG_ERR("Invalid Wi-Fi band: %d", wlan_band);
		return -1;
	}

	/* Configure PTA registers of Coexistence Hardware */
	LOG_INF("Configuring PTA for %s", wifi_band_txt(status.band));
	ret = nrf_wifi_coex_config_pta(wlan_band, is_ant_mode_sep, is_ble_central,
			is_wlan_server);
	if (ret != 0) {
		LOG_ERR("Failed to configure PTA coex hardware: %d", ret);
		return -1;
	}
	return 0;
}

void print_common_test_params(bool is_ant_mode_sep, bool test_ble, bool test_wlan,
		bool is_ble_central)
{

	bool ble_coex_enable = IS_ENABLED(CONFIG_MPSL_CX);
	bool is_wifi_band_2pt4g = IS_ENABLED(CONFIG_WIFI_BAND_2PT4G);

	/* LOG_INF("-------------------------------- Test parameters"); */

	if (test_wlan && test_ble) {
		LOG_INF("Running Wi-Fi and BLE tests");
	} else {
		if (test_wlan) {
			LOG_INF("Running Wi-Fi only test");
		} else {
			LOG_INF("Running BLE only test");
		}
	}

	if (is_wifi_band_2pt4g) {
		LOG_INF("Wi-Fi operates in 2.4G band");
	} else {
		LOG_INF("Wi-Fi operates in 5G band");
	}
	if (is_ble_central) {
		LOG_INF("BLE role: Central");
	} else {
		LOG_INF("BLE role: Peripheral");
	}
	if (ble_coex_enable) {
		LOG_INF("BLE posts requests to PTA");
	} else {
		LOG_INF("BLE doesn't post requests to PTA");
	}

	if (is_ant_mode_sep) {
		LOG_INF("Antenna mode: Separate antennas");
	} else {
		LOG_INF("Antenna mode: Shared antennas");
	}

	LOG_INF("Test duration in milliseconds: %d", CONFIG_COEX_TEST_DURATION);
	LOG_INF("--------------------------------");
}

int wifi_tput_ble_tput(bool test_wlan, bool is_ant_mode_sep,
	bool test_ble, bool is_ble_central, bool is_wlan_server, bool is_zperf_udp)
{
	int ret = 0;
	int64_t test_start_time = 0;

	LOG_INF("-------------------------------- Test parameters");
	LOG_INF("Test case: wifi_tput_ble_tput");

	if (is_wlan_server) {
		if (is_zperf_udp) {
			LOG_INF("Wi-Fi transport protocol, role: UDP, server");
		} else {
			LOG_INF("Wi-Fi transport protocol, role: TCP, server");
		}
	} else {
		if (is_zperf_udp) {
			LOG_INF("Wi-Fi transport protocol, role: UDP, client");
		} else {
			LOG_INF("Wi-Fi transport protocol, role: TCP, client");
		}
	}

	print_common_test_params(is_ant_mode_sep, test_ble, test_wlan, is_ble_central);

	if (test_wlan) {
		ret = wifi_connection();
		if (ret != 0) {
			LOG_ERR("Wi-Fi connection failed. Running the test");
			LOG_ERR("further is not meaningful. So, exiting the test");
			return ret;
		}
#if defined(CONFIG_NRF700X_BT_COEX)
		config_pta(is_ant_mode_sep, is_ble_central, is_wlan_server);
#endif/* CONFIG_NRF700X_BT_COEX */
	}

	if (test_ble) {
		if (!is_ble_central) {
			LOG_INF("Make sure peer BLE role is central");
			k_sleep(K_SECONDS(3));
		}
		ret = bt_throughput_test_init(is_ble_central);
		if (ret != 0) {
			LOG_ERR("Failed to BT throughput init: %d", ret);
			return ret;
		}

		if (!is_wlan_server) {
			if (!is_ble_central) {
				if (test_wlan && test_ble) {
#ifdef CONFIG_PRINTS_FOR_AUTOMATION
					while (!wait4_peer_ble2_start_connection) {
						/* Peer BLE starts the the test. */
						LOG_INF("Run BLE central on peer");
						k_sleep(K_SECONDS(1));
					}
					wait4_peer_ble2_start_connection = 0;
#endif
				}
			}
		}
	}
	if (!is_wlan_server) {
#ifdef DEMARCATE_TEST_START_END
		LOG_INF("-------------------------start");
#endif
	}
	if (!is_wlan_server) {
		test_start_time = k_uptime_get_32();
	}
	if (test_wlan) {
		if (is_zperf_udp) {
			ret = wifi_run_udp_traffic();
		} else {
			ret = wifi_run_tcp_traffic();
		}

		if (ret != 0) {
			LOG_ERR("Failed to start Wi-Fi throughput: %d", ret);
			return ret;
		}
		if (is_wlan_server) {
			while (!wait4_peer_wifi_client_to_start_tp_test) {
#ifdef CONFIG_PRINTS_FOR_AUTOMATION
				LOG_INF("start WiFi client on peer");
#endif
				k_sleep(K_SECONDS(1));
			}
			wait4_peer_wifi_client_to_start_tp_test = 0;
			test_start_time = k_uptime_get_32();
		}
	}
	if (is_wlan_server) {
#ifdef DEMARCATE_TEST_START_END
		LOG_INF("-------------------------start");
#endif
	}

	if (test_ble) {
		if (is_ble_central) {
			bt_start_activity();
		} else {
			/* If DUT BLE is peripheral then the peer starts the activity. */
			while (true) {
				if (k_uptime_get_32() - test_start_time >
					CONFIG_COEX_TEST_DURATION) {
					break;
				}
				k_sleep(KSLEEP_TEST_DUR_CHECK_1SEC);
			}
		}
	}

	if (test_wlan) {
		wifi_check_traffic_status();
	}

	if (test_ble) {
		if (is_ble_central) {
			/* run BLE activity and wait for the test duration */
			bt_run_activity();
		} else {
			/* Peer BLE that acts as central runs the traffic. */
		}
	}

	if (test_wlan) {
		wifi_disconnection();
	}

	if (test_ble) {
		if (is_ble_central) {
			bt_exit_throughput_test();
		}
	}

#ifdef DEMARCATE_TEST_START_END
	LOG_INF("-------------------------end");
#endif

	return 0;
}
