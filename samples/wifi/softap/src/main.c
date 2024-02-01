/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief Wi-Fi Softap sample
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(softap, CONFIG_LOG_DEFAULT_LEVEL);

#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/wifi_utils.h>

#define WIFI_SAP_MGMT_EVENTS (NET_EVENT_WIFI_AP_ENABLE_RESULT)

static struct net_mgmt_event_callback wifi_sap_mgmt_cb;

static void handle_wifi_ap_enable_result(struct net_mgmt_event_callback *cb)
{
	const struct wifi_status *status =
		(const struct wifi_status *)cb->info;

	if (status->status) {
		LOG_ERR("AP enable request failed (%d)", status->status);
	} else {
		LOG_INF("AP enable requested");
	}
}

static void wifi_mgmt_event_handler(struct net_mgmt_event_callback *cb,
				    uint32_t mgmt_event, struct net_if *iface)
{
	switch (mgmt_event) {
	case NET_EVENT_WIFI_AP_ENABLE_RESULT:
		handle_wifi_ap_enable_result(cb);
		break;
	default:
		break;
	}
}

static int __wifi_args_to_params(struct wifi_connect_req_params *params)
{
#ifdef CONFIG_SOFTAP_SAMPLE_2_4GHz
	params->band = WIFI_FREQ_BAND_2_4_GHZ;
#elif CONFIG_SOFTAP_SAMPLE_5GHz
	params->band = WIFI_FREQ_BAND_5_GHZ;
#endif
	params->channel = CONFIG_SOFTAP_SAMPLE_CHANNEL;

	/* SSID */
	params->ssid = CONFIG_SOFTAP_SAMPLE_SSID;
	params->ssid_length = strlen(params->ssid);
	if (params->ssid_length > WIFI_SSID_MAX_LEN) {
		LOG_ERR("SSID length is too long, expected is %d characters long",
			WIFI_SSID_MAX_LEN);
		return -1;
	}

#if defined(CONFIG_SOFTAP_SAMPLE_KEY_MGMT_WPA2)
	params->security = 1;
#elif defined(CONFIG_SOFTAP_SAMPLE_KEY_MGMT_WPA2_256)
	params->security = 2;
#elif defined(CONFIG_SOFTAP_SAMPLE_KEY_MGMT_WPA3)
	params->security = 3;
#else
	params->security = 0;
#endif

#if !defined(CONFIG_SOFTAP_SAMPLE_KEY_MGMT_NONE)
	params->psk = CONFIG_SOFTAP_SAMPLE_PASSWORD;
	params->psk_length = strlen(params->psk);
#endif

	return 0;
}

static void wifi_softap_enable(void)
{
	struct net_if *iface;
	static struct wifi_connect_req_params cnx_params;
	int ret;

	iface = net_if_get_first_wifi();
	if (!iface) {
		LOG_ERR("Failed to get Wi-Fi iface");
		return;
	}

	if (__wifi_args_to_params(&cnx_params)) {
		return;
	}

	if (!wifi_utils_validate_chan(cnx_params.band, cnx_params.channel)) {
		LOG_ERR("Invalid channel %d in %d band",
			cnx_params.channel, cnx_params.band);
		return;
	}

	ret = net_mgmt(NET_REQUEST_WIFI_AP_ENABLE, iface, &cnx_params,
		       sizeof(struct wifi_connect_req_params));
	if (ret) {
		LOG_ERR("AP mode enable failed: %s", strerror(-ret));
	} else {
		LOG_INF("AP mode enabled");
	}
}

static void wifi_set_reg_domain(void)
{
	struct net_if *iface;
	struct wifi_reg_domain regd = {0};
	int ret;

	iface = net_if_get_first_wifi();
	if (!iface) {
		LOG_ERR("Failed to get Wi-Fi iface");
		return;
	}

	regd.oper = WIFI_MGMT_SET;
	strncpy(regd.country_code, CONFIG_SOFTAP_SAMPLE_REG_DOMAIN,
		(WIFI_COUNTRY_CODE_LEN + 1));

	ret = net_mgmt(NET_REQUEST_WIFI_REG_DOMAIN, iface,
		       &regd, sizeof(regd));
	if (ret) {
		LOG_ERR("Cannot %s Regulatory domain: %d", "SET", ret);
	} else {
		LOG_INF("Regulatory domain set to %s", CONFIG_SOFTAP_SAMPLE_REG_DOMAIN);
	}
}

int main(void)
{
	net_mgmt_init_event_callback(&wifi_sap_mgmt_cb,
				     wifi_mgmt_event_handler,
				     WIFI_SAP_MGMT_EVENTS);

	net_mgmt_add_event_callback(&wifi_sap_mgmt_cb);

	wifi_set_reg_domain();

	/* TODO: SHEL-2458 */
	k_msleep(20);

	wifi_softap_enable();

	return 0;
}
