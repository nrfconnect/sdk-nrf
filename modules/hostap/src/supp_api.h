/*
 * Copyright (c) 2022 Nordic Semiconductor
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 */

#ifndef ZEPHYR_SUPP_MGMT_H
#define ZEPHYR_SUPP_MGMT_H

#include <zephyr/net/wifi_mgmt.h>

#define MAX_SSID_LEN 32
#define MAC_ADDR_LEN 6

/**
 * @brief Request a connection
 *
 * @param dev: Wi-Fi interface name to use
 * @param params: Connection details
 *
 * @return: 0 for OK; -1 for ERROR
 */
int z_wpa_supplicant_connect(const struct device *dev,
						struct wifi_connect_req_params *params);
/**
 * @brief Forces station to disconnect and stops any subsequent scan
 *  or connection attempts
 *
 * @param dev: Wi-Fi interface name to use
 *
 * @return: 0 for OK; -1 for ERROR
 */
int z_wpa_supplicant_disconnect(const struct device *dev);

/**
 * @brief
 *
 * @param dev: Wi-Fi interface name to use
 * @param status: Status structure to fill
 *
 * @return: 0 for OK; -1 for ERROR
 */
int z_wpa_supplicant_status(const struct device *dev,
	struct wifi_iface_status *status);

/**
 * @brief Request a scan
 *
 * @param dev Wi-Fi interface name to use
 * @param cb Callback to be called for each scan result
 *
 * @return 0 for OK; -1 for ERROR
 */
int z_wpa_supplicant_scan(const struct device *dev,scan_result_cb_t cb);

#if defined(CONFIG_NET_STATISTICS_WIFI) || defined(__DOXYGEN__)
/**
 * @brief Get Wi-Fi statistics
 *
 * @param dev Wi-Fi interface name to use
 * @param stats Pointer to stats structure to fill
 *
 * @return 0 for OK; -1 for ERROR
 */
int z_wpa_supplicant_get_stats(const struct device *dev,
				struct net_stats_wifi *stats);
#endif /* CONFIG_NET_STATISTICS_WIFI || __DOXYGEN__ */

/**
 * @brief Set Wi-Fi power save configuration
 *
 * @param dev Wi-Fi interface name to use
 * @param params Power save parameters to set
 *
 * @return 0 for OK; -1 for ERROR
 */
int z_wpa_supplicant_set_power_save(const struct device *dev,
				struct wifi_ps_params *params);

/**
 * @brief Set Wi-Fi TWT parameters
 *
 * @param dev Wi-Fi interface name to use
 * @param params TWT parameters to set
 * @return 0 for OK; -1 for ERROR
 */
int z_wpa_supplicant_set_twt(const struct device *dev,
				struct wifi_twt_params *params);

/**
 * @brief Get Wi-Fi power save configuration
 *
 * @param dev Wi-Fi interface name to use
 * @param config Address of power save configuration to fill
 * @return 0 for OK; -1 for ERROR
 */
int z_wpa_supplicant_get_power_save_config(const struct device *dev,
				struct wifi_ps_config *config);

/**
 * @brief Set Wi-Fi Regulatory domain
 *
 * @param dev Wi-Fi interface name to use
 * @param reg_domain Regulatory domain to set
 * @return 0 for OK; -1 for ERROR
 */
int z_wpa_supplicant_reg_domain(const struct device *dev,
				struct wifi_reg_domain *reg_domain);
#endif /* ZEPHYR_SUPP_MGMT_H */
