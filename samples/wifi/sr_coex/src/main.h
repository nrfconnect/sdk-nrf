/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MAIN_H_
#define MAIN_H_

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

#include <zephyr_coex.h>

/**
 * @brief Function to test Wi-Fi throughput client/server and BLE throughput central/peripheral
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int wifi_tput_ble_tput(bool test_wlan, bool is_ant_mode_sep, bool test_ble,
		bool is_ble_central, bool is_wlan_server, bool is_zperf_udp);

/**
 * @brief wifi_memset_context
 *
 * @return No return value.
 */
void wifi_memset_context(void);

/**
 * @brief Handle net management callbacks
 *
 * @return No return value.
 */
void wifi_net_mgmt_callback_functions(void);
#endif /* MAIN_H_ */
