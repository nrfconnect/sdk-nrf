/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef BT_COEX_TEST_FUNCTIONS_
#define BT_COEX_TEST_FUNCTIONS_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <bluetooth/scan.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_coex_test_func, CONFIG_LOG_DEFAULT_LEVEL);

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

/* For net_sprint_ll_addr_buf */
#include "net_private.h"
#include <zephyr_coex.h>
#include <zephyr_coex_struct.h>
#include "bt_utils.h"
#include "zephyr_fmac_main.h"

#define WIFI_CONNECTION_TIMEOUT 30 /* in seconds */
#define WIFI_DHCP_TIMEOUT 10 /* in seconds */

#define DEMARCATE_TEST_START_END

#define RSSI_INIT_VALUE 127
#define KSLEEP_TEST_DUR_CHECK_1SEC K_SECONDS(1)

static uint32_t wifi_conn_attempt_cnt;
static uint32_t wifi_conn_success_cnt;
static uint32_t wifi_conn_fail_cnt;
static uint32_t wifi_conn_timeout_cnt;
static uint32_t wifi_dhcp_timeout_cnt;

static uint32_t wifi_disconn_attempt_cnt;
static uint32_t wifi_disconn_success_cnt;
static uint32_t wifi_disconn_fail_cnt;
static uint32_t wifi_disconn_no_conn_cnt;

static uint32_t wifi_disconn_cnt_stability;
uint8_t wait4_peer_ble2_start_connection;

static struct net_mgmt_event_callback wifi_sta_mgmt_cb;
static struct net_mgmt_event_callback net_addr_mgmt_cb;
static uint8_t wait4_peer_wifi_client_to_start_tp_test;

static struct sockaddr_in in4_addr_my = {
	.sin_family = AF_INET,
	.sin_port = htons(CONFIG_NET_CONFIG_PEER_IPV4_PORT),
};

#define WIFI_MGMT_EVENTS (NET_EVENT_WIFI_CONNECT_RESULT | NET_EVENT_WIFI_DISCONNECT_RESULT)

/**
 * @brief Handle net management events
 *
 * @return No return value.
 */
void net_mgmt_event_handler(struct net_mgmt_event_callback *cb, uint32_t mgmt_event,
		struct net_if *iface);

/**
 * @brief Handle Wi-Fi management events
 *
 * @return None
 */
void wifi_mgmt_event_handler(struct net_mgmt_event_callback *cb, uint32_t mgmt_event,
		struct net_if *iface);

/**
 * @brief BT throughtput test run
 *
 * @return None
 */
void bt_run_benchmark(void);

/**
 * @brief Call Wi-Fi connection event
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int wifi_connection(void);

/**
 * @brief configure PTA registers
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int config_pta(bool is_ant_mode_sep, bool is_ble_central,
				bool is_wlan_server);

/**
 * @brief Start wi-fi traffic for zperf udp upload or configure
 * zperf traffic for udp download
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int wifi_run_udp_traffic(void);

/**
 * @brief Start wi-fi traffic for zperf tcp upload or configure
 * zperf traffic for tcp download
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int wifi_run_tcp_traffic(void);

/**
 * @brief start Bluetooth LE activity using thread start
 *
 * @return None
 */
void bt_start_activity(void);

/**
 * @brief start Wi-Fi activity using thread start
 *
 * @return None
 */
void start_wifi_activity(void);

/**
 * @brief check if iperf traffic is complete
 *
 * @return None
 */
void wifi_check_traffic_status(void);

/**
 * @brief Run BLE traffic using thread join
 *
 * @return None
 */
void bt_run_activity(void);

/**
 * @brief Disconnect Wi-Fi
 *
 * @return None
 */
void wifi_disconnection(void);

/**
 * @brief Exit Bluetooth LE throughput test
 *
 * @return None
 */
void bt_exit_throughput_test(void);

static struct {
	uint8_t connected :1;
	uint8_t disconnect_requested: 1;
	uint8_t _unused : 6;
} context;

K_SEM_DEFINE(wait_for_next, 0, 1);
K_SEM_DEFINE(udp_tcp_callback, 0, 1);

struct wifi_iface_status status = { 0 };

K_THREAD_DEFINE(run_bt_traffic,
	CONFIG_WIFI_THREAD_STACK_SIZE,
	bt_run_benchmark,
	NULL,
	NULL,
	NULL,
	CONFIG_WIFI_THREAD_PRIORITY,
	0,
	K_TICKS_FOREVER);

/**
 * @brief Print Wi-Fi command status
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int wifi_command_status(void);

/**
 * @brief Initailise Wi-Fi arguments in variables
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int __wifi_args_to_params(struct wifi_connect_req_params *params);

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
int wifi_parse_ipv4_addr(char *host, struct sockaddr_in *addr);

/**
 * @brief wait for next Wi-Fi event
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int wait_for_next_event(const char *event_name, int timeout);

/**
 * @brief Callback for UDP download results
 *
 * @return None
 */
void wifi_udp_download_results_cb(enum zperf_status status,
							struct zperf_results *result,
							void *user_data);

/**
 * @brief Callback for UDP upload results
 *
 * @return None
 */
void wifi_udp_upload_results_cb(enum zperf_status status,
							struct zperf_results *result,
							void *user_data);

/**
 * @brief Callback for TCP download results
 *
 * @return None
 */
void wifi_tcp_download_results_cb(enum zperf_status status,
							struct zperf_results *result,
							void *user_data);

/**
 * @brief Callback for TCP upload results
 *
 * @return None
 */
void wifi_tcp_upload_results_cb(enum zperf_status status,
							struct zperf_results *result,
							void *user_data);

/**
 * @brief Callback for Wi-Fi DHCP IP address
 *
 * @return None
 */
void print_dhcp_ip(struct net_mgmt_event_callback *cb);

/**
 * @brief Callback for Wi-Fi connection result
 *
 * @return None
 */
void wifi_handle_connect_result(struct net_mgmt_event_callback *cb);

/**
 * @brief Callback for Wi-Fi disconnection result
 *
 * @return None
 */
void wifi_handle_disconnect_result(struct net_mgmt_event_callback *cb);

/**
 * @brief Callback for Wi-Fi DHCP IP addreds assigned
 *
 * @return No return value.
 */
void print_dhcp_ip(struct net_mgmt_event_callback *cb);

/**
 * @brief Print common test parameters info
 *
 * @return None
 */
void print_common_test_params(bool is_ant_mode_sep, bool test_ble, bool test_wlan,
	bool is_ble_central);

#endif /* BT_COEX_TEST_FUNCTIONS_ */
