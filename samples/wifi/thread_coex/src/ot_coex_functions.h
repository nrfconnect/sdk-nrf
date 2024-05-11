/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef OT_COEX_FUNCTIONS_
#define OT_COEX_FUNCTIONS_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ot_coex_functions, CONFIG_LOG_DEFAULT_LEVEL);

#include <nrfx_clock.h>

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_uart.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/net/zperf.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/wifi_utils.h>

#include <net/wifi_mgmt_ext.h>

/* For net_sprint_ll_addr_buf */
#include "net_private.h"

#include "coex.h"
#include "coex_struct.h"
#include "fmac_main.h"
#include "ot_utils.h"

#define DEMARCATE_TEST_START

#define KSLEEP_WHILE_CHECK_1SEC K_SECONDS(1)
#define KSLEEP_100MSEC K_MSEC(100)

static uint8_t wait4_peer_wifi_client_to_start_tput;

static struct sockaddr_in in4_addr_my = {
	.sin_family = AF_INET,
	.sin_port = htons(CONFIG_NET_CONFIG_PEER_IPV4_PORT),
};

static struct {
	uint8_t connected :1;
	uint8_t disconnect_requested: 1;
	uint8_t _unused : 6;
} wifi_context;

K_SEM_DEFINE(wait_for_next, 0, 1);
K_SEM_DEFINE(udp_tcp_callback, 0, 1);

/**
 * @brief configure PTA registers
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int config_pta(bool is_ant_mode_sep, bool is_ot_client,
				bool is_wifi_server, bool is_sr_protocol_ble);

/**
 * @brief Start wi-fi traffic for zperf udp upload or download
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int run_wifi_traffic_udp(bool is_wifi_server);

/**
 * @brief Start wi-fi traffic for zperf tcp upload or download
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int run_wifi_traffic_tcp(bool is_wifi_server);

/**
 * @brief check if Wi-Fi traffic is complete
 *
 * @return None
 */
void check_wifi_traffic(void);

/**
 * @brief Disconnect Wi-Fi
 *
 * @return None
 */
void wifi_disconnection(void);

/**
 * @brief Exit Thread throughput test
 *
 * @return None
 */
void ot_throughput_test_exit(void);

/**
 * @brief Print Wi-Fi status
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int cmd_wifi_status(void);

/**
 * @brief Request Wi-Fi connection
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int wifi_connect(void);

/**
 * @brief Request Wi-Fi disconnection
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int wifi_disconnect(void);

/**
 * @brief parse Wi-Fi IPv4 address
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int parse_ipv4_addr(char *host, struct sockaddr_in *addr);

/**
 * @brief wait for next Wi-Fi event
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int wifi_wait_for_next_event(const char *event_name, int timeout);

/**
 * @brief Callback for UDP download results
 *
 * @return None
 */
void udp_download_results_cb(enum zperf_status status,
	struct zperf_results *result,
	void *user_data);

/**
 * @brief Callback for UDP upload results
 *
 * @return None
 */
void udp_upload_results_cb(enum zperf_status status,
	struct zperf_results *result,
	void *user_data);

/**
 * @brief Callback for TCP download results
 *
 * @return None
 */
void tcp_download_results_cb(enum zperf_status status,
	struct zperf_results *result,
	void *user_data);

/**
 * @brief Callback for TCP upload results
 *
 * @return None
 */
void tcp_upload_results_cb(enum zperf_status status,
	struct zperf_results *result,
	void *user_data);
/**
 * @brief Print common test parameters info
 *
 * @return None
 */
void print_common_test_params(bool is_ant_mode_sep, bool test_thread, bool test_wifi,
	bool is_ot_client);

/**
 * @brief Print throughput test parameters info
 *
 * @return None
 */
int print_wifi_tput_ot_tput_test_details(bool is_ot_client, bool is_wifi_server,
	bool is_wifi_zperf_udp,	bool is_ot_zperf_udp);

/* wifi_utils.h */
#define MAX_SSID_LEN 32
#define WIFI_CONNECTION_TIMEOUT_SEC 30 /* in seconds */
#define WIFI_DHCP_TIMEOUT_SEC 10 /* in seconds */

#define DEMARCATE_TEST_START

#define TXPOWER_INIT_VALUE 127
#define RSSI_INIT_VALUE 127

#define HIGHEST_CHANNUM_24G 14

uint32_t wifi_disconn_success_cnt;
uint32_t wifi_disconn_fail_cnt;
uint32_t wifi_disconn_no_conn_cnt;

int8_t wifi_rssi = RSSI_INIT_VALUE;
static int print_wifi_conn_status_once = 1;

struct wifi_iface_status wifi_if_status = { 0 };

static struct net_mgmt_event_callback wifi_sta_mgmt_cb;
static struct net_mgmt_event_callback net_addr_mgmt_cb;

#define WIFI_MGMT_EVENTS (NET_EVENT_WIFI_CONNECT_RESULT | NET_EVENT_WIFI_DISCONNECT_RESULT)

/**
 * @brief Handle Wi-Fi management events
 *
 * @return No return value.
 */
void wifi_mgmt_event_handler(struct net_mgmt_event_callback *cb, uint32_t mgmt_event,
	struct net_if *iface);

/**
 * @brief Handle net management events
 *
 * @return No return value.
 */
void net_mgmt_event_handler(struct net_mgmt_event_callback *cb, uint32_t mgmt_event,
	struct net_if *iface);

/**
 * @brief Callback for Wi-Fi connection result
 *
 * @return No return value.
 */
void handle_wifi_connect_result(struct net_mgmt_event_callback *cb);

/**
 * @brief Callback for Wi-Fi disconnection result
 *
 * @return No return value.
 */
void handle_wifi_disconnect_result(struct net_mgmt_event_callback *cb);

/**
 * @brief Callback for Wi-Fi scan result
 *
 * @return No return value.
 */
void handle_wifi_scan_result(struct net_mgmt_event_callback *cb);

/**
 * @brief Callback for Wi-Fi scan done
 *
 * @return No return value.
 */
void handle_wifi_scan_done(struct net_mgmt_event_callback *cb);

/**
 * @brief Callback for Wi-Fi DHCP IP address
 *
 * @return None
 */
void print_dhcp_ip(struct net_mgmt_event_callback *cb);

#endif /* OT_COEX_FUNCTIONS_ */
