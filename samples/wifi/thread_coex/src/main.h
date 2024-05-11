/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MAIN_H_
#define MAIN_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);

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

#include "coex.h"
#include "fmac_main.h"


/**
 * @brief Function to test Wi-Fi throughput client/server and Thread throughput client/server
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int wifi_tput_ot_tput(bool test_wifi, bool is_ant_mode_sep, bool test_thread,
		bool is_ot_client, bool is_wifi_server, bool is_wifi_zperf_udp,
		bool is_ot_zperf_udp, bool is_sr_protocol_ble);

/**
 * @brief memset_context
 *
 * @return No return value.
 */
void memset_context(void);

/**
 * @brief Handle net management callbacks
 *
 * @return No return value.
 */
void wifi_mgmt_callback_functions(void);

/**
 * @brief Wi-Fi init.
 *
 * @return None
 */
void wifi_init(void);

/**
 * @brief Network configuration
 *
 * @return status
 */
int net_config_init_app(const struct device *dev, const char *app_info);

#endif /* MAIN_H_ */
