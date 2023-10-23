/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief Header containing network stack interface specific declarations for
 * the Zephyr OS layer of the Wi-Fi driver.
 */

#ifndef __ZEPHYR_NET_IF_H__
#define __ZEPHYR_NET_IF_H__
#include <zephyr/device.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/ethernet.h>
#include <fmac_structs.h>
#include <zephyr/net/wifi_mgmt.h>

void nrf_wifi_if_init_zep(struct net_if *iface);

int nrf_wifi_if_start_zep(const struct device *dev);

int nrf_wifi_if_stop_zep(const struct device *dev);

int nrf_wifi_if_set_config_zep(const struct device *dev,
			       enum ethernet_config_type type,
			       const struct ethernet_config *config);

enum ethernet_hw_caps nrf_wifi_if_caps_get(const struct device *dev);

int nrf_wifi_if_send(const struct device *dev,
		     struct net_pkt *pkt);

void nrf_wifi_if_rx_frm(void *os_vif_ctx,
			void *frm);

enum nrf_wifi_status nrf_wifi_if_carr_state_chg(void *os_vif_ctx,
						enum nrf_wifi_fmac_if_carr_state carr_state);

int nrf_wifi_stats_get(const struct device *dev,
		       struct net_stats_wifi *stats);

struct net_stats_eth *nrf_wifi_eth_stats_get(const struct device *dev);
#endif /* __ZEPHYR_NET_IF_H__ */
